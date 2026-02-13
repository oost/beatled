# syntax=docker/dockerfile:1

####################################
# Build server
####################################

FROM debian:trixie-backports AS build-server

RUN echo "I am running on $TARGETPLATFORM, uname $(uname -m)" 

RUN dpkg --add-architecture arm64 \
  && apt-get update \
  &&  apt-get install -t trixie-backports -y \
    cmake make autoconf automake autoconf-archive libtool \
    build-essential gawk pkg-config git ninja-build \
    curl zip unzip tar texinfo bison libncurses-dev file \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libc6-dev-arm64-cross \
    libfftw3-dev:arm64 libasound-dev:arm64 portaudio19-dev:arm64 \
    libasound2-dev:arm64 libflac-dev:arm64 libogg-dev:arm64 \
    libtool:arm64 libvorbis-dev:arm64 libopus-dev:arm64 libmp3lame-dev:arm64 \
    libmpg123-dev:arm64 libpulse-dev:arm64 libpython3-dev
  #  linux-headers 
  # qemu binfmt-support qemu-user-static qemu-utils qemu-system-arm  
  # autoconf libtool
  # && rm -rf /var/lib/apt/lists/*

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN cd /tmp \
    && git clone https://github.com/Microsoft/vcpkg.git -n \ 
    && cd vcpkg \
    && git checkout 66c0373dc7fca549e5803087b9487edfe3aca0a1 \
    && ./bootstrap-vcpkg.sh

COPY server/cmake/arm64-linux-custom.cmake /tmp/vcpkg/triplets

ENV VCPKG_HOME=/tmp/vcpkg
ENV TARGET_TRIPLET=arm64-linux-custom

RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET} \ 
    fmt spdlog restinio asio nlohmann-json \
    date fftw3  bfgroup-lyra  \
    libsamplerate catch2 kissfft \
    audiofile portaudio openssl \
    range-v3 http-parser pybind11
# vamp-sdk
#   vamp-sdk has a depenedency on mpg123 which doesn't compile currently:
#   #error "Bad decoder choice together with fixed point math!"


# alsa

# RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET}-dynamic portaudio

WORKDIR /home/build 
# COPY server /home/source/ 

# WORKDIR /home/source 
