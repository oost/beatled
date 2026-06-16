# syntax=docker/dockerfile:1

####################################
# Build server
####################################

# Build on bookworm to match the deployment target (Raspberry Pi OS /
# Debian 12, glibc 2.36, libstdc++ 3.4.30). Building on a newer release
# bakes in GLIBC_2.38 / GLIBCXX_3.4.32 symbol requirements the Pi can't
# satisfy. The aarch64 cross-toolchain comes from bookworm main (not
# backports) so the produced binary links against the same libc the Pi runs.
FROM debian:bookworm AS build-server

RUN echo "I am running on $TARGETPLATFORM, uname $(uname -m)" 

RUN dpkg --add-architecture arm64 \
  && apt-get update \
  &&  apt-get install -y \
    cmake make autoconf automake autoconf-archive libtool \
    build-essential gawk pkg-config git ninja-build \
    curl zip unzip tar texinfo bison libncurses-dev file \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libc6-dev-arm64-cross \
    libfftw3-dev:arm64 libasound-dev:arm64 portaudio19-dev:arm64 \
    libasound2-dev:arm64 libflac-dev:arm64 libogg-dev:arm64 \
    libtool:arm64 libvorbis-dev:arm64 libopus-dev:arm64 libmp3lame-dev:arm64 \
    libmpg123-dev:arm64 libpulse-dev:arm64 libpython3-dev \
  && rm -rf /var/lib/apt/lists/*
  #  linux-headers
  # qemu binfmt-support qemu-user-static qemu-utils qemu-system-arm
  # autoconf libtool

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
# Pin to the same vcpkg commit as server/vcpkg.json's builtin-baseline and
# .github/workflows/server-ci.yml so the cross-compiled deploy binary is built
# against the same library versions that CI and native builds test against.
# This SHA is the vcpkg release tag 2026.01.16.
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
    range-v3 http-parser
# pybind11 intentionally omitted: it is only needed to build the BTrack
# Python plugin (modules-and-plug-ins), which the beat_server binary does
# not use. It drags in the python3 subtree (libffi, libb2, ...) whose
# vcpkg portfiles repeatedly fail to cross-compile. The deploy build sets
# -DBUILD_PLUGINS=OFF so nothing references pybind11.
# vamp-sdk
#   vamp-sdk has a depenedency on mpg123 which doesn't compile currently:
#   #error "Bad decoder choice together with fixed point math!"


# alsa

# RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET}-dynamic portaudio

WORKDIR /home/build 
# COPY server /home/source/ 

# WORKDIR /home/source 
