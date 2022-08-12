#!/bin/sh

mkdir lib
cd lib
curl -L https://github.com/Stiffstream/restinio/releases/download/v.0.6.16/restinio-0.6.16-full.tar.bz2 -o restinio.tar.bz2
tar xf restinio.tar.bz2 

cd restinio-0.6.16/dev 
mkdir cmake_build
cd cmake_build
cmake -DCMAKE_INSTALL_PREFIX=target -DCMAKE_BUILD_TYPE=Release -DRESTINIO_TEST=OFF -DRESTINIO_BENCH=OFF -DRESTINIO_SAMPLE=OFF ..
cmake --build . --config Release --target install