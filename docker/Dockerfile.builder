# syntax=docker/dockerfile:1

####################################
# Build server
####################################

FROM debian:bullseye-backports AS build-server

RUN echo "I am running on $TARGETPLATFORM, uname $(uname -m)" 

RUN apt-get update \
  &&  apt-get install -t bullseye-backports -y cmake make build-essential gawk pkg-config git ninja-build \
  curl zip unzip tar texinfo bison libncurses-dev libfftw3-dev \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libc6-dev-arm64-cross \
  cmake libfftw3-dev file qemu binfmt-support qemu-user-static qemu-utils qemu-system-arm libasound-dev
  # && rm -rf /var/lib/apt/lists/*

RUN cd /tmp \
    && git clone https://github.com/Microsoft/vcpkg.git -n \ 
    && cd vcpkg \
    && git checkout master \
    && ./bootstrap-vcpkg.sh -useSystemBinaries

COPY server/cmake/arm64-linux-dynamic.cmake /tmp/vcpkg/triplets

ENV VCPKG_HOME=/tmp/vcpkg
ENV TARGET_TRIPLET=arm64-linux

RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET} fmt spdlog restinio asio nlohmann-json portaudio date bfgroup-lyra fftw3 audiofile libsamplerate catch2 kissfft 

RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET}-dynamic portaudio

WORKDIR /home/build 
# COPY server /home/source/ 

# WORKDIR /home/source 
