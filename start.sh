#!/bin/bash
set -e
cd "$(dirname "$0")"

# Re-exec self after pull so we always run the latest version.
# This must happen before the tee redirect — process substitution
# with exec causes a forked duplicate when combined with re-exec.
if [ -z "$KIDDO_REEXECED" ]; then
    export KIDDO_REEXECED=1
    git pull || true
    exec "./start.sh" "$@"
fi

LOGFILE="$(pwd)/tmp/start.log"
mkdir -p "$(pwd)/tmp"
exec > >(tee -a "$LOGFILE") 2>&1
echo "=== start.sh $(date) ==="

# Install system dependencies if missing.
# Check first without sudo; only escalate when packages are actually needed.
PACMAN_DEPS=(
    python python-virtualenv base-devel cmake ninja pkgconf mesa
    libx11 libxcb libxrandr libxinerama libxcursor libxi
    libxext libxfixes libxrender libxcomposite libxdamage
    libxxf86vm libxkbfile libxmu libxpm libxt libxtst libxv
    libxss libxaw libice libsm libfontenc
    xcb-util xcb-util-image xcb-util-keysyms xcb-util-renderutil
    xcb-util-wm xcb-util-cursor xcb-util-errors
    util-linux-libs
)

APT_DEPS=(
    python3-venv build-essential cmake ninja-build pkg-config libgl-dev
    libx11-dev libx11-xcb-dev libfontenc-dev libice-dev libsm-dev
    libxaw7-dev libxcomposite-dev libxcursor-dev libxdamage-dev
    libxext-dev libxfixes-dev libxi-dev libxinerama-dev
    libxkbfile-dev libxmu-dev libxmuu-dev libxpm-dev libxrandr-dev
    libxrender-dev libxres-dev libxss-dev libxt-dev libxtst-dev
    libxv-dev libxxf86vm-dev libxcb-glx0-dev libxcb-render0-dev
    libxcb-render-util0-dev libxcb-xkb-dev libxcb-icccm4-dev
    libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev
    libxcb-shape0-dev libxcb-sync-dev libxcb-xfixes0-dev
    libxcb-xinerama0-dev libxcb-dri3-dev uuid-dev
    libxcb-cursor-dev libxcb-dri2-0-dev libxcb-present-dev
    libxcb-composite0-dev libxcb-ewmh-dev libxcb-res0-dev
    libxcb-util-dev
)

install_deps() {
    if command -v pacman &>/dev/null; then
        local missing=()
        for pkg in "${PACMAN_DEPS[@]}"; do
            pacman -Qi "$pkg" &>/dev/null || missing+=("$pkg")
        done
        if [ ${#missing[@]} -gt 0 ]; then
            echo "Installing missing packages: ${missing[*]}"
            sudo pacman -S --needed --noconfirm "${missing[@]}"
        else
            echo "All pacman dependencies already installed"
        fi
    elif command -v dpkg &>/dev/null; then
        local missing=()
        for pkg in "${APT_DEPS[@]}"; do
            dpkg -s "$pkg" &>/dev/null 2>&1 || missing+=("$pkg")
        done
        if [ ${#missing[@]} -gt 0 ]; then
            echo "Installing missing packages: ${missing[*]}"
            sudo apt-get install -y "${missing[@]}"
        else
            echo "All apt dependencies already installed"
        fi
    fi
}

# Only build if binary is missing or sources are newer
needs_build() {
    [ ! -f ./build/Release/kiddo ] && return 0
    for f in src/*.c src/*.h CMakeLists.txt conanfile.py Dockerfile; do
        [ "$f" -nt ./build/Release/kiddo ] && return 0
    done
    return 1
}

if needs_build; then
    echo "LOG: build needed"

    # Prefer docker, fall back to podman, then native build
    CONTAINER_CMD=""
    if command -v docker &>/dev/null; then
        CONTAINER_CMD=docker
    elif command -v podman &>/dev/null; then
        CONTAINER_CMD=podman
    fi

    if [ -n "$CONTAINER_CMD" ]; then
        echo "LOG: building with $CONTAINER_CMD..."
        mkdir -p build/Release
        $CONTAINER_CMD build --output=build/Release .
        echo "LOG: container build finished"
    else
        echo "LOG: no container runtime, building natively with conan..."

        echo "LOG: installing deps..."
        install_deps || true
        echo "LOG: deps done"

        # Set up Python venv for conan
        VENV_DIR="$(pwd)/.venv"
        if [ ! -d "$VENV_DIR" ]; then
            echo "LOG: creating venv at $VENV_DIR"
            python3 -m venv "$VENV_DIR"
        fi
        source "$VENV_DIR/bin/activate"
        echo "LOG: venv activated"

        command -v conan &>/dev/null || pip install conan
        conan profile path default &>/dev/null || conan profile detect
        echo "LOG: conan ready"

        echo "LOG: conan install starting..."
        conan install . --output-folder=build --build=missing
        echo "LOG: conan install done"

        echo "LOG: conan build starting..."
        conan build .
        echo "LOG: conan build done"
    fi
else
    echo "LOG: binary up to date, skipping build"
fi

echo "LOG: launching kiddo"
exec ./build/Release/kiddo
