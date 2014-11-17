#!/bin/bash
# Script for building ThirdParty modules for linux.
# You must run UpdateDeps.sh to download and patch the sources before running
# this script.

SCRIPT_DIR=$(cd "$(dirname "$BASH_SOURCE")" ; pwd)
TOP_DIR=$(cd $SCRIPT_DIR/../../.. ; pwd)
cd ${TOP_DIR}
set -e

MAKE_ARGS=-j4
TARGET_ARCH=x86_64-unknown-linux-gnu

# Open files for edit using p4 command line.
# Does nothing if the file is already writable (which is case for external
# github developers).
P4Open()
{
  for file in $@; do
    # If the file does not exist do nothing
    if [ ! -e $file ]; then
      return
    fi
    # If the file is already writable do nothing
    if [ -w $file ]; then
      return
    fi
    # If the file is already writable do nothing
    if ! which p4 > /dev/null; then
      echo "File is not writable and 'p4' command not found."
      exit 1
    fi
    set +x
    p4 open "$file"
    set -x
  done
}

BuildJemalloc()
{
  echo "building jemalloc"
  set -x
  cd Source/ThirdParty/jemalloc/build
  tar xf jemalloc-3.4.0.tar.bz2
  cd jemalloc-3.4.0
  ./configure --with-mangling --with-jemalloc-prefix=je_340_
  make
  local LIB_DIR=../../lib/Linux/$TARGET_ARCH/separate
  P4Open $LIB_DIR/libjemalloc.a
  mkdir -p $LIB_DIR
  cp lib/* $LIB_DIR/
  cp lib/libjemalloc.so.1 ${TOP_DIR}/Binaries/Linux
  set +x
}

BuildOpus()
{
  echo "building libOpus"
  set -x
  cd Source/ThirdParty/libOpus/opus-1.1/
  P4Open configure
  chmod +x configure
  ./configure --with-pic
  make $MAKE_ARGS
  local LIB_DIR=Linux/$TARGET_ARCH
  mkdir -p $LIB_DIR
  cp .libs/libopus.a .libs/libopus*.so* $LIB_DIR
  cp $LIB_DIR/libopus.so.0 ${TOP_DIR}/Binaries/Linux/
  set +x
}

BuildPNG()
{
  echo "building libPNG"
  set -x
  cd Source/ThirdParty/libPNG/libPNG-1.5.2
  cp --remove-destination scripts/makefile.linux Makefile
  cp --remove-destination scripts/pnglibconf.h.prebuilt pnglibconf.h
  P4Open pnglibconf.h
  make $MAKE_ARGS

  local LIB_DIR=lib/Linux/$TARGET_ARCH
  mkdir -p $LIB_DIR
  P4Open $LIB_DIR/libpng.a
  cp --remove-destination libpng.a $LIB_DIR/
  # (amigo) this dylib will mess with our build
  # instead we want to use system's libpng to avoid asserts
  #cp libpng15.so $LIB_DIR/libpng.so
  mkdir -p ${TOP_DIR}/Binaries/Linux/
  #cp -a libpng15.so* ${TOP_DIR}/Binaries/Linux/
  set +x
}

BuildOgg()
{
  echo "building Ogg"
  set -x
  cd Source/ThirdParty/Ogg/libogg-1.2.2/
  P4Open configure
  chmod +x configure
  ./configure --with-pic
  make $MAKE_ARGS

  local LIB_DIR=lib/Linux/$TARGET_ARCH
  P4Open $LIB_DIR/libogg.a
  mkdir -p $LIB_DIR
  cp src/.libs/* $LIB_DIR
  set +x
}

BuildVorbis()
{
  echo "building Vorbis"
  set -x
  cd Source/ThirdParty/Vorbis/libvorbis-1.3.2/
  P4Open configure
  chmod +x configure
  OGG_LIBS=../../Ogg/libogg-1.2.2/lib/Linux \
      OGG_CFLAGS=-I../../../Ogg/libogg-1.2.2/include \
      ./configure --with-pic --disable-oggtest
  make $MAKE_ARGS

  local LIB_DIR=lib/Linux/$TARGET_ARCH
  P4Open $LIB_DIR/libvorbis.a $LIB_DIR/libvorbisfile.a
  mkdir -p $LIB_DIR
  cp lib/.libs/libvorbis*.a $LIB_DIR/
  set +x
  cd -
}

BuildHLSLCC()
{
  echo "building hlslcc"
  set -x
  cd Source/ThirdParty/hlslcc
  P4Open hlslcc/bin/Linux/hlslcc_64
  make $MAKE_ARGS
  set +x
}

BuildMcpp()
{
  echo "building MCPP"
  set -x
  cd Source/ThirdParty/MCPP/mcpp-2.7.2
  P4Open configure
  chmod +x configure
  ./configure --enable-shared --enable-shared --enable-mcpplib
  make $MAKE_ARGS

  local LIB_DIR=lib/Linux/$TARGET_ARCH
  P4Open $LIB_DIR/libmcpp.a
  P4Open $LIB_DIR/libmcpp.so
  mkdir -p $LIB_DIR
  cp --remove-destination ./src/.libs/libmcpp.a $LIB_DIR/
  cp --remove-destination ./src/.libs/libmcpp.so $LIB_DIR/
  cp --remove-destination ./src/.libs/libmcpp.so ${TOP_DIR}/Binaries/Linux/libmcpp.so.0
  set +x
}

BuildFreeType()
{
  echo "building freetype"
  set -x
  cd Source/ThirdParty/FreeType2/FreeType2-2.4.12/src
  pwd
  make $MAKE_ARGS -f ../Builds/Linux/makefile $*

  local LIB_DIR=../Lib/Linux/$TARGET_ARCH
  P4Open $LIB_DIR/libfreetype2412.a
  mkdir -p $LIB_DIR
  cp --remove-destination libfreetype* $LIB_DIR/
  set +x
}

BuildICU()
{
  echo "building libICU"
  set -x
  cd Source/ThirdParty/ICU/icu4c-53_1/source
  P4Open configure
  chmod +x configure
  CPPFLAGS=-fPIC ./configure --enable-static
  pwd
  make $MAKE_ARGS $*

  local LIB_DIR=../Linux/$TARGET_ARCH
  mkdir -p $LIB_DIR
  mkdir -p ${TOP_DIR}/Binaries/ThirdParty/ICU/icu4c-53_1/Linux/$TARGET_ARCH/
  P4Open $LIB_DIR/libicudata.a $LIB_DIR/libicudatad.a $LIB_DIR/libicui18n.a $LIB_DIR/libicui18nd.a
  P4Open $LIB_DIR/libicuio.a $LIB_DIR/libicuiod.a $LIB_DIR/libicule.a $LIB_DIR/libiculed.a
  P4Open $LIB_DIR/libiculx.a $LIB_DIR/libiculxd.a $LIB_DIR/libicutu.a $LIB_DIR/libicutud.a
  P4Open $LIB_DIR/libicuuc.a $LIB_DIR/libicuucd.a
  cp --remove-destination lib/*.a $LIB_DIR
  cp -P --remove-destination lib/*.so* ${TOP_DIR}/Binaries/ThirdParty/ICU/icu4c-53_1/Linux/$TARGET_ARCH/

  set +x
}

BuildLND()
{
  echo "building LinuxNativeDialogs"
  set -x
  cd Source/ThirdParty/LinuxNativeDialogs/UELinuxNativeDialogs/build
  cmake ..
  make $MAKE_ARGS
  local LIB_DIR=../lib/Linux/$TARGET_ARCH/
  if [ -f libqt4dialog.so ]; then
    ln -s libqt4dialog.so libLND.so
    mv *.so $LIB_DIR/
    cp -P --remove-destination $LIB_DIR/*.so ${TOP_DIR}/Binaries/Linux/
  fi
  if [ -f libqt5dialog.so ]; then
    ln -s libqt5dialog.so libLND.so
    mv *.so $LIB_DIR/
    cp -P --remove-destination $LIB_DIR/*.so ${TOP_DIR}/Binaries/Linux/
  fi
  if [ -f libgtk2dialog.so ]; then
    ln -s libgtk2dialog.so libLND.so
    mv *.so $LIB_DIR/
    cp -P --remove-destination $LIB_DIR/*.so ${TOP_DIR}/Binaries/Linux/
  fi
  if [ -f libgtk3dialog.so ]; then
    ln -s libgtk3dialog.so libLND.so
    mv *.so $LIB_DIR/
    cp -P --remove-destination $LIB_DIR/*.so ${TOP_DIR}/Binaries/Linux/
  fi
  set +x
}

BuildForsythTriOO()
{
  echo "building ForsythTriOO"
  set -x
  cd Source/ThirdParty/ForsythTriOO
  mkdir -p Build/Linux
  mkdir -p Lib/Linux/$TARGET_ARCH
  cd Build/Linux
  cmake -DCMAKE_BUILD_TYPE=Debug ../../
  make
  cmake -DCMAKE_BUILD_TYPE=Release ../../
  make
  cp --remove-destination libForsythTriOptimizer* ../../Lib/Linux/$TARGET_ARCH/
  set +x
}

BuildnvTriStrip()
{
  echo "building nvTriStrip"
  set -x
  cd Source/ThirdParty/nvTriStrip/nvTriStrip-1.0.0
  mkdir -p Build/Linux
  mkdir -p Lib/Linux/$TARGET_ARCH
  cd Build/Linux
  cmake -DCMAKE_BUILD_TYPE=Debug ../../
  make
  cmake -DCMAKE_BUILD_TYPE=Release ../../
  make
  cp --remove-destination libnvtristrip* ../../Lib/Linux/$TARGET_ARCH/
  set +x
}

BuildnvTextureTools()
{
  echo "building nvTextureTools"
  set -x
  cd Source/ThirdParty/nvTextureTools/nvTextureTools-2.0.8
  mkdir -p lib/Linux/$TARGET_ARCH
  cd src
  ./configure
  make

  local LIB_DIR=../lib/Linux/$TARGET_ARCH
  cp --remove-destination build-debug/src/nvcore/libnvcore.so $LIB_DIR/
  cp --remove-destination build-debug/src/nvimage/libnvimage.so $LIB_DIR/
  cp --remove-destination build-debug/src/nvmath/libnvmath.so $LIB_DIR/
  cp --remove-destination build-debug/src/nvtt/libnvtt.so $LIB_DIR/
  cp --remove-destination build-debug/src/nvthread/libnvthread.a $LIB_DIR/
  cp --remove-destination build-debug/src/nvtt/squish/libsquish.a $LIB_DIR/
  cp --remove-destination $LIB_DIR/*.so ${TOP_DIR}/Binaries/Linux/
  set +x
}

BuildSDL2()
{
  echo "building SDL2"
  set -x
  cd Source/ThirdParty/SDL2/SDL2-2.0.3/build/SDL2-2.0.3
  P4Open configure
  chmod +x configure
  chmod +x autogen.sh
  chmod +x Android.mk
  ./configure
  make $MAKE_ARGS
  cp --remove-destination include/* ../../include/
  local LIB_DIR=../../lib/Linux/$TARGET_ARCH
  P4Open $LIB_DIR/libEpicSDL2.a
  mkdir -p $LIB_DIR
  cp --remove-destination build/.libs/libSDL2.a $LIB_DIR/libEpicSDL2.a
  cp build/.libs/libSDL2.so $LIB_DIR/libEpicSDL2.so
  cp build/.libs/libSDL2.so ${TOP_DIR}/Binaries/Linux/libSDL2-2.0.so.0
  set +x
}

Buildcoremod()
{
  echo "building coremod"
  set -x
  cd Source/ThirdParty/coremod/coremod-4.2.6
  pwd
  autoconf
  CFLAGS=-fPIC ./configure --enable-static
  make $MAKE_ARGS
  local LIB_DIR=lib/Linux/$TARGET_ARCH
  mkdir -p $LIB_DIR
  cp lib/libxmp-coremod.a $LIB_DIR/libcoremodLinux.a
  set +x
}

FixICU()
{
  echo "setting symlinks for ICU libraries (possibly temporary)"
  set -x
  cd Binaries/ThirdParty/ICU/icu4c-53_1/Linux/x86_64-unknown-linux-gnu
  pwd
  ln -sf libicudata.so libicudata.so.53
  ln -sf libicudata.so libicudata.53.1.so
  ln -sf libicui18n.so libicui18n.so.53
  ln -sf libicui18n.so libicui18n.53.1.so
  ln -sf libicuio.so libicuio.so.53
  ln -sf libicuio.so libicuio.53.1.so
  ln -sf libicule.so libicule.so.53
  ln -sf libicule.so libicule.53.1.so
  ln -sf libiculx.so libiculx.so.53
  ln -sf libiculx.so libiculx.53.1.so
  ln -sf libicutu.so libicutu.so.53
  ln -sf libicutu.so libicutu.53.1.so
  ln -sf libicuuc.so libicuuc.so.53
  ln -sf libicuuc.so libicuuc.53.1.so
  set +x
}

Run()
{
  cd ${TOP_DIR}
  echo "==> $1"
  if [ -n "$VERBOSE" ]; then
    $1
  else
    $1 >> ${SCRIPT_DIR}/BuildThirdParty.log 2>&1
  fi
}

echo "Building ThirdParty libraries"
if [ -z "$VERBOSE" ]; then
  echo "(For full output set VERBOSE=1, otherwise output is logged to BuildThirdParty.log)"
fi

rm -f ${SCRIPT_DIR}/BuildThirdParty.log
if [ -z "$1" ]; then
  Run BuildJemalloc
  Run BuildOpus
  Run BuildOgg
  Run BuildVorbis
  Run BuildPNG
  Run BuildHLSLCC
  Run BuildMcpp
  Run BuildFreeType
  Run BuildLND
  Run BuildForsythTriOO
  Run BuildnvTriStrip
  Run BuildnvTextureTools
  Run BuildSDL2
  Run Buildcoremod
  Run FixICU
else
  Run $1
fi

echo "********** SUCCESS ****************"