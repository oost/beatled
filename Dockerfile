# syntax=docker/dockerfile:1

####################################
# Build server
####################################

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
    && ./bootstrap-vcpkg.sh -useSystemBinaries

WORKDIR /home/build 
COPY server /home/source/ 

WORKDIR /home/source 

ENV VCPKG_HOME=/tmp/vcpkg
ENV TARGET_TRIPLET=arm64-linux

RUN ${VCPKG_HOME}/vcpkg install --triplet=${TARGET_TRIPLET} fmt spdlog restinio asio nlohmann-json date bfgroup-lyra portaudio fftw3 audiofile libsamplerate catch2 kissfft

RUN cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=${TARGET_TRIPLET} \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/toolchains/linux.cmake \
  -B ../build -S . --debug-output --trace-expand | tee ../build/out.log \
  && cmake --build ../build \
  && cmake --install ../build --prefix /home/final


# ####################################
# # Build front end
# ####################################
# FROM --platform=$BUILDPLATFORM node AS build-client

# RUN   apt-get update \
#   &&  apt-get install -y tar 

# WORKDIR /home
# COPY client /home 
# RUN npm install && npm run build 


FROM scratch
WORKDIR /home 
COPY --from=build-server /home/final .


# ####################################
# # Bring together all the parts
# ####################################

# FROM scratch AS export
# COPY --from=build-server /app/server/build/target server
# COPY --from=build-client /home/build client
# COPY scripts scripts
