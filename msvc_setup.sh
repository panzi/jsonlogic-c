#!/bin/bash

set -e

SELF=$(readlink -f "$0")
DIR=$(dirname "$SELF")

function count_args () {
	echo $#
}

if [[ $(count_args $DIR) -gt 1 ]]; then
	echo "MSVC installation directory may not contain spaces: '$DIR'">&2
	exit 1
fi

if [[ -e "$DIR/msvc" ]]; then
	echo "$DIR/msvc already exists.">&2
	echo "If you want to do a clean install of the MSVC toolchain delete this directory.">&2
	exit 1
fi

mkdir "$DIR/msvc"

cd "$DIR/msvc"

curl -L https://github.com/panzi/msvc-wine/archive/refs/heads/add_double_quotes.zip -o msvc-wine.zip
unzip msvc-wine.zip
msvc-wine-master/vsdownload.py --dest install
msvc-wine-master/install.sh install
