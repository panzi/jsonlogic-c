#!/bin/bash

set -e

SELF=$(readlink -f "$0")
DIR=$(dirname "$SELF")

if [[ -e "$DIR/msvc" ]]; then
	echo "$DIR/msvc already exists.">&2
	echo "If you want to do a clean install of the MSVC toolchain delete this directory.">&2
	exit 1
fi

mkdir "$DIR/msvc"

cd "$DIR/msvc"

curl -L https://github.com/panzi/msvc-wine/archive/refs/heads/install.py.zip -o msvc-wine.zip
unzip msvc-wine.zip
msvc-wine-install.py/vsdownload.py --dest install
msvc-wine-install.py/install.py install
