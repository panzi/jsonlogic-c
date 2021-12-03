#!/usr/bin/bash

set -e

SELF=$(readlink -f "$0")
DIR=$(dirname "$SELF")

cd "$DIR"

release_id=$(git describe --tags 2>/dev/null || git rev-parse HEAD)
zip_file="jsonlogic-c-$release_id.zip"

if [[ -e build ]]; then
    rm -r build
fi

if [[ -e "$zip_file" ]]; then
    rm -r "$zip_file"
fi

nproc=$(nproc)
for release in OFF ON; do
    for linkage in shared static; do
        for platform in linux mingw; do
            for bits in 32 64; do
                echo make RELEASE=$release TARGET=$platform$bits $linkage
                make -j"$nproc" TARGET=$platform$bits RELEASE=$release $linkage >/dev/null
            done
        done
        for arch in x86 x64 arm arm64; do
            echo make -f Makefile.msvc RELEASE=$release ARCH=$arch $linkage
            WINEDEBUG=-all make -j"$nproc" -f Makefile.msvc ARCH=$arch RELEASE=$release $linkage >/dev/null
        done
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
    msbv_setup.sh
