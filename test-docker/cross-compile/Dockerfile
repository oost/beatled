FROM debian:bullseye-backports AS build-server

RUN echo "I am running on $TARGETPLATFORM, uname $(uname -m)" 

RUN apt-get update \
  &&  apt-get install -t bullseye-backports -y cmake make build-essential gawk pkg-config git ninja-build \
  curl zip unzip tar texinfo bison libncurses-dev libfftw3-dev \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libc6-dev-arm64-cross \
  cmake/bullseye-backports libfftw3-dev file 

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

RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET} fmt

# RUN cmake -G Ninja \
#   -DCMAKE_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake \
#   -DVCPKG_TARGET_TRIPLET=${TARGET_TRIPLET} \
#   -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/toolchains/linux.cmake \
#   -B ../build -S . 
