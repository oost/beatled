
if [ -z ${VCPKG_ROOT+x} ]; then
  echo "Please set VCPKG_ROOT"
  exit 1
fi

#
# 2. Set the path to the toolchains
#
vcpkg_toolchain_file=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

#
# 3. Select a pair "Android abi" / "vcpkg triplet"
# Uncomment one of the four possibilities below
#

vcpkg_target_triplet=x64-linux

# android_abi=x86
# vcpkg_target_triplet=x86-android

# android_abi=arm64-v8a
# vcpkg_target_triplet=arm64-android

# android_abi=x86_64
# vcpkg_target_triplet=x64-android


#
# 4. Install the library via vcpkg
#
# $VCPKG_ROOT/vcpkg install jsoncpp:$vcpkg_target_triplet

#
# 5. Test the build
#
rm -rf build
mkdir build
cd build 
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$vcpkg_toolchain_file \
  -DVCPKG_TARGET_TRIPLET=$vcpkg_target_triplet \
  --trace-expand

  # -G"Unix Makefiles" \

make



