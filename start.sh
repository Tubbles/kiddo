#!/bin/bash
set -e
cd "$(dirname "$0")"

git pull || true

# Install system dependencies if missing
install_deps() {
    if command -v pacman &>/dev/null; then
        sudo pacman -S --needed --noconfirm \
            python base-devel cmake ninja pkgconf mesa \
            libx11 libxcb libxrandr libxinerama libxcursor libxi \
            libxext libxfixes libxrender libxcomposite libxdamage \
            libxxf86vm libxkbfile libxmu libxpm libxt libxtst libxv \
            libxss libxaw libice libsm libfontenc \
            xcb-util xcb-util-image xcb-util-keysyms xcb-util-renderutil \
            xcb-util-wm xcb-util-cursor xcb-util-errors \
            util-linux-libs
    elif command -v apt-get &>/dev/null; then
        sudo apt-get install -y \
            build-essential cmake ninja-build pkg-config libgl-dev \
            libx11-dev libx11-xcb-dev libfontenc-dev libice-dev libsm-dev \
            libxaw7-dev libxcomposite-dev libxcursor-dev libxdamage-dev \
            libxext-dev libxfixes-dev libxi-dev libxinerama-dev \
            libxkbfile-dev libxmu-dev libxmuu-dev libxpm-dev libxrandr-dev \
            libxrender-dev libxres-dev libxss-dev libxt-dev libxtst-dev \
            libxv-dev libxxf86vm-dev libxcb-glx0-dev libxcb-render0-dev \
            libxcb-render-util0-dev libxcb-xkb-dev libxcb-icccm4-dev \
            libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev \
            libxcb-shape0-dev libxcb-sync-dev libxcb-xfixes0-dev \
            libxcb-xinerama0-dev libxcb-dri3-dev uuid-dev \
            libxcb-cursor-dev libxcb-dri2-0-dev libxcb-present-dev \
            libxcb-composite0-dev libxcb-ewmh-dev libxcb-res0-dev \
            libxcb-util-dev
    fi
}

install_deps || true

# Set up Python venv for conan
VENV_DIR="$(pwd)/.venv"
if [ ! -d "$VENV_DIR" ]; then
    python3 -m venv "$VENV_DIR"
fi
source "$VENV_DIR/bin/activate"

command -v conan &>/dev/null || pip install conan

conan install . --output-folder=build --build=missing
conan build .

exec ./build/Release/kiddo
