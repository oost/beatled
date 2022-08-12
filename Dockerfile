# syntax=docker/dockerfile:1
FROM --platform=$BUILDPLATFORM arm32v6/alpine AS build
ARG TARGETPLATFORM
ARG BUILDPLATFORM
RUN echo "I am running on $BUILDPLATFORM, building for $TARGETPLATFORM" 
RUN echo $(cat /etc/os-release) && echo $(uname -m)
RUN   apk update \
  && apk add \
  libc6-compat \
  gcc \
  cmake \
  build-base \ 
  pkgconfig \
  git \ 
  curl \
  ninja \ 
  zip \
  unzip \
  tar  \
  && rm -rf /var/lib/apt/lists/*
# RUN   apt-get update \
#   &&  apt-get install -y \
#   gcc \
#   cmake \
#   build-essential \ 
#   pkg-config \
#   git \ 
#   curl \
#   zip \
#   unzip \
#   tar  \
#   && rm -rf /var/lib/apt/lists/*
WORKDIR /home
RUN git clone https://github.com/Microsoft/vcpkg.git
WORKDIR /home/vcpkg
RUN export VCPKG_FORCE_SYSTEM_BINARIES=1
RUN ./bootstrap-vcpkg.sh
RUN echo $(cmake --version)
RUN ./vcpkg install restinio clara

COPY ./server /app
WORKDIR /app
RUN cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/vcpkg/scripts/buildsystems/vcpkg.cmake 
RUN cmake --build build

FROM scratch AS export
COPY --from=build /app/build .
