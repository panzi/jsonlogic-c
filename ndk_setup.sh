#!/bin/bash

set -e

SELF=$(readlink -f "$0")
DIR=$(dirname "$SELF")

NDK_VERSION=${NDK_VERSION:-r23b}
ANDROID_VERSION=${ANDROID_VERSION:-31}

if [[ -e "$DIR/ndk" ]]; then
	echo "$DIR/ndk already exists.">&2
	echo "If you want to do a clean install of the Android NDK toolchain delete this directory.">&2
	exit 1
fi

mkdir "$DIR/ndk"

cd "$DIR/ndk"

curl "https://dl.google.com/android/repository/android-ndk-${NDK_VERSION}-linux.zip" -o "android-ndk-${NDK_VERSION}-linux.zip"
unzip "android-ndk-${NDK_VERSION}-linux.zip"
