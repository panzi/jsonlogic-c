#!/bin/bash

set -e

SELF=$(readlink -f "$0")
DIR=$(dirname "$SELF")

if [[ "$DIR" != "{DIR//[$' \r\n\t\v']/}" ]]; then
	echo "MSVC installation diroctory may not contain spaces.">&2
	exit 1
fi

if [[ -e "$DIR/msvc" ]]; then
	echo "$DIR/msvc already exists.">&2
	echo "If you want to do a clean install of the MSVC toolchain delete this directory.">&2
	exit 1
fi

mkdir "$DIR/msvc"

cd "$DIR/msvc"

curl https://github.com/mstorsjo/msvc-wine/archive/refs/heads/master.zip -o msvc-wine.zip
unzip msvc-wine.zip
msvc-wine-master/vsdownload.py --dest install
msvc-wine-master/install.sh install
