# Multi-stage build: produces a portable Linux binary via
#   docker build --output=build/Release .
#
# Build stage uses Ubuntu 24.04 (glibc 2.39) for clang-20 with C23 #embed support

FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config \
    python3 python3-venv python3-pip \
    clang-20 \
    libgl-dev \
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
    libxcb-util-dev \
    && rm -rf /var/lib/apt/lists/*

# Set clang-20 as default clang
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-20 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-20 100

WORKDIR /src

# Install conan in a venv
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"
RUN pip install --no-cache-dir conan

# Copy only build metadata first (cache-friendly layer ordering)
COPY conanfile.py CMakeLists.txt ./
COPY test/CMakeLists.txt test/CMakeLists.txt

# Configure conan profile for clang-20 and install deps
RUN conan profile detect \
    && sed -i 's/compiler=gcc/compiler=clang/' ~/.conan2/profiles/default \
    && sed -i 's/compiler.version=.*/compiler.version=20/' ~/.conan2/profiles/default \
    && sed -i '/compiler.cppstd/d' ~/.conan2/profiles/default \
    && printf '\n[conf]\ntools.build:compiler_executables={"c": "clang", "cpp": "clang++"}\n' >> ~/.conan2/profiles/default \
    && sed -i '/"18":/a\                "19":\n                "20":' ~/.conan2/settings.yml \
    && conan install . --output-folder=build --build=missing

# Copy the rest of the source and assets
COPY assets/ assets/
COPY src/ src/
COPY test/ test/

# Build and run tests
RUN conan build . \
    && ./build/Release/test/kiddo_tests

# Output stage: extract just the binary
FROM scratch
COPY --from=build /src/build/Release/kiddo /kiddo
