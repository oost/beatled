# syntax=docker/dockerfile:1

####################################
# Build server
####################################

FROM --platform=$BUILDPLATFORM debian AS build-server
ARG TARGETPLATFORM
ARG BUILDPLATFORM

RUN echo "I am running on $BUILDPLATFORM, building for $TARGETPLATFORM" 

RUN   apt-get update \
  &&  apt-get install -y gcc g++ gfortran cmake make build-essential gawk pkg-config git ninja-build \
  curl zip unzip tar gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf git texinfo bison libncurses-dev \
  && rm -rf /var/lib/apt/lists/*


WORKDIR /tmp

RUN curl -L https://github.com/rvagg/rpi-newer-crosstools/archive/eb68350c5c8ec1663b7fe52c742ac4271e3217c5.tar.gz -o rpi-toolchain.tar.gz && \
  mkdir -p /home/toolchains && \
  tar xzf rpi-toolchain.tar.gz -C /home/toolchains && \
  mv /home/toolchains/rpi-newer-crosstools-eb68350c5c8ec1663b7fe52c742ac4271e3217c5 /home/toolchains/arm-rpi-linux-gnueabihf

# https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/Raspberry%20Pi%20GCC%20Cross-Compiler%20Toolchains/Bullseye/GCC%2010.3.0/Raspberry%20Pi%201%2C%20Zero/cross-gcc-10.3.0-pi_0-1.tar.gz/download



WORKDIR /app/server

COPY server /app/server

RUN cmake -DCMAKE_TOOLCHAIN_FILE=rpiz-toolchain.cmake -B build -S . \
  && cmake --build build \
  && cmake --install build

####################################
# Build front end
####################################
FROM node AS build-client

RUN   apt-get update \
  &&  apt-get install -y tar 

WORKDIR /home
COPY client /home 
RUN npm install && npm run build 

####################################
# Bring together all the parts
####################################

FROM scratch AS export
COPY --from=build-server /app/server/build/target server
COPY --from=build-client /home/build client
COPY scripts scripts
