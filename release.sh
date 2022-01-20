#!/usr/bin/bash

set -e

SELF=$(readlink -f "$0")
DIR=$(dirname "$SELF")

cd "$DIR"

NDK_VERSION=${NDK_VERSION:-r23b}

MSVC_PATH=${MSVC_PATH:-$DIR/msvc/install/bin}
NDK_PATH=${NDK_PATH:-$DIR/ndk/android-ndk-$NDK_VERSION}

WITH_MSVC=${WITH_MSV:-ON}
WITH_ANDROID=${WITH_ANDROID:-ON}

release_id=$(git describe --tags 2>/dev/null || git rev-parse HEAD)
zip_file="jsonlogic-c-$release_id.zip"

if [[ -e build ]]; then
    rm -r build
fi

if [[ -e "$zip_file" ]]; then
    rm -r "$zip_file"
fi

export WINEDEBUG=-all
nproc=$(nproc)
host_platform=$(uname -s|tr '[:upper:]' '[:lower:]')
for release in OFF ON; do
    for linkage in shared static; do
        for platform in "$host_platform" mingw; do
            for arch in i686 x86_64; do
                echo make RELEASE=$release TARGET="$platform-$arch" $linkage
                make -j"$nproc" TARGET="$platform-$arch" RELEASE=$release $linkage >/dev/null
            done
        done

        if [[ "$WITH_MSVC" = ON ]]; then
            for arch in x86 x64 arm arm64; do
                echo make -f Makefile.msvc RELEASE=$release ARCH=$arch $linkage
                make -j"$nproc" -f Makefile.msvc MSVC_PATH="$MSVC_PATH" ARCH=$arch RELEASE=$release $linkage >/dev/null
            done
        fi
        
        if [[ "$WITH_ANDROID" = ON ]]; then
            for arch in aarch64 armv7a i686; do
                echo make TARGET="android-$arch" RELEASE=$release $linkage
                make -j"$nproc" NDK_PATH="$NDK_PATH" TARGET="android-$arch" RELEASE=$release $linkage >/dev/null
            done
        fi
    done
done

rm -r build/*/*/obj build/*/*/shared-obj build/*/*/src

zip -r9 "$zip_file" \
    build \
    src \
    Makefile \
    Makefile.msvc \
    README.md \
    release.sh \
    msvc_setup.sh \
    ndk_setup.sh
