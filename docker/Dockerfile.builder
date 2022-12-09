# syntax=docker/dockerfile:1

####################################
# Build server
####################################

FROM debian:bullseye-backports AS build-server

RUN echo "I am running on $TARGETPLATFORM, uname $(uname -m)" 

RUN dpkg --add-architecture arm64 \
  && apt-get update \
  &&  apt-get install -t bullseye-backports -y cmake make build-essential gawk pkg-config git ninja-build \
  curl zip unzip tar texinfo bison libncurses-dev file \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libc6-dev-arm64-cross \
  cmake libfftw3-dev:arm64 libasound-dev:arm64 portaudio19-dev:arm64  
  # qemu binfmt-support qemu-user-static qemu-utils qemu-system-arm  
  # autoconf libtool
  # && rm -rf /var/lib/apt/lists/*

RUN cd /tmp \
    && git clone https://github.com/Microsoft/vcpkg.git -n \ 
    && cd vcpkg \
    && git checkout master \
    && ./bootstrap-vcpkg.sh -useSystemBinaries

COPY server/cmake/arm64-linux-custom.cmake /tmp/vcpkg/triplets

ENV VCPKG_HOME=/tmp/vcpkg
ENV TARGET_TRIPLET=arm64-linux-custom

RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET} fmt spdlog restinio asio nlohmann-json date fftw3 portaudio bfgroup-lyra audiofile libsamplerate catch2 kissfft 
# alsa portaudio 

# RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET}-dynamic portaudio

WORKDIR /home/build 
# COPY server /home/source/ 

# WORKDIR /home/source 
