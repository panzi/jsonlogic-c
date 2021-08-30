#!/usr/bin/bash

set -e

SELF=$(readlink -f "$0")
DIR=$(dirname "$SELF")

curl https://jsonlogic.com/tests.json -o "$DIR/tests.json"
