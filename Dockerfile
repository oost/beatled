# syntax=docker/dockerfile:1

####################################
# Build server
####################################

FROM debian AS build-server
ARG TARGETPLATFORM
ARG BUILDPLATFORM

RUN echo "I am running on $BUILDPLATFORM, building for $TARGETPLATFORM, uname $(uname -m)" 

RUN   apt-get update \
  &&  apt-get install -y cmake make build-essential gawk pkg-config git ninja-build \
  curl zip unzip tar texinfo bison libncurses-dev libfftw3-dev \
  && rm -rf /var/lib/apt/lists/*

RUN cd /tmp \
    && git clone https://github.com/Microsoft/vcpkg.git -n \ 
    && cd vcpkg \
    && git checkout master \
    && ./bootstrap-vcpkg.sh 

# WORKDIR /tmp

# RUN curl -L https://github.com/rvagg/rpi-newer-crosstools/archive/eb68350c5c8ec1663b7fe52c742ac4271e3217c5.tar.gz -o rpi-toolchain.tar.gz && \
#   mkdir -p /home/toolchains && \
#   tar xzf rpi-toolchain.tar.gz -C /home/toolchains && \
#   mv /home/toolchains/rpi-newer-crosstools-eb68350c5c8ec1663b7fe52c742ac4271e3217c5 /home/toolchains/arm-rpi-linux-gnueabihf

# https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/Raspberry%20Pi%20GCC%20Cross-Compiler%20Toolchains/Bullseye/GCC%2010.3.0/Raspberry%20Pi%201%2C%20Zero/cross-gcc-10.3.0-pi_0-1.tar.gz/download



WORKDIR /app/server

COPY server /app/server

RUN uname -a

RUN cmake -DCMAKE_TOOLCHAIN_FILE=/tmp/vcpkg/scripts/buildsystems/vcpkg.cmake -B build -S . \
  && cmake --build build \
  && cmake --install build

# ####################################
# # Build front end
# ####################################
# FROM --platform=$BUILDPLATFORM node AS build-client

# RUN   apt-get update \
#   &&  apt-get install -y tar 

# WORKDIR /home
# COPY client /home 
# RUN npm install && npm run build 

# ####################################
# # Bring together all the parts
# ####################################

# FROM scratch AS export
# COPY --from=build-server /app/server/build/target server
# COPY --from=build-client /home/build client
# COPY scripts scripts
