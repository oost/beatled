FROM ubuntu
ARG boost_libs
ARG boost_libs=none

# Prepare build environment
RUN apt-get update && \
  apt-get -qq -y install gcc g++ \
  cmake curl wget pkg-config \
  libtool python3 python3-pip

RUN pip3 install --upgrade cmake
RUN pip3 install conan && \
  conan remote add stiffstream https://api.bintray.com/conan/stiffstream/public && \
  conan remote add public-conan https://api.bintray.com/conan/bincrafters/public-conan

RUN mkdir restinio-conan-example
COPY *.cpp restinio-conan-example/
COPY conanfile.txt restinio-conan-example
COPY CMakeLists.txt restinio-conan-example

RUN echo "*** Building an example ***" \
  && cd restinio-conan-example \
  && mkdir build \
  && cd build \
  && conan install --install-folder cmake-build-release --build=missing ..  \
  && cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake-build-release/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release\
  && cmake --build . --config Release

EXPOSE 8080

WORKDIR /restinio-conan-example/build

CMD ./hello_world_minimal
