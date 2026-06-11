FROM debian AS build-server

RUN echo "I am running on $TARGETPLATFORM, uname $(uname -m)" 

RUN apt-get update \
  &&  apt-get install -y cmake make build-essential gawk pkg-config git ninja-build \
  curl zip unzip tar texinfo bison libncurses-dev libfftw3-dev \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libc6-dev-arm64-cross \
  cmake libfftw3-dev file \
  && rm -rf /var/lib/apt/lists/*

RUN cd /tmp \
    && git clone https://github.com/Microsoft/vcpkg.git -n \ 
    && cd vcpkg \
    && git checkout master \
    && ./bootstrap-vcpkg.sh 

WORKDIR /home/build 
COPY source /home/source/ 

WORKDIR /home/source 

ENV VCPKG_HOME=/tmp/vcpkg
ENV TARGET_TRIPLET=arm64-linux

RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET} fmt spdlog

RUN cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=${TARGET_TRIPLET} \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/toolchains/linux.cmake \
  -B ../build -S . --debug-output --trace-expand | tee ../build/out.log \
  && cmake --build ../build \
  && cmake --install ../build --prefix /home/final

# Slim runtime stage (mirrors docker/Dockerfile.beatled's runtime pattern;
# `scratch` had no libc/runtime libs, so the binaries couldn't run).
FROM debian:trixie-slim@sha256:4e401d95de7083948053197a9c3913343cd06b706bf15eb6a0c3ccd26f436a0e
RUN useradd -r -s /usr/sbin/nologin beatled
WORKDIR /home
COPY --from=build-server /home/final .
USER beatled
