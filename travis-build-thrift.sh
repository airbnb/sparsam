#!/usr/bin/env bash
set -euxo pipefail

mkdir -p $HOME/thrift
cd $HOME/thrift

echo "$THRIFT_SHA256SUM" > SHA256SUMS
ret=0
sha256sum -c SHA256SUMS 2>&1 || ret=$?

if [ $ret -ne 0 ]; then
    wget -N "$THRIFT_URL"
    tar xvf "$THRIFT_FILE"
    cd "$THRIFT_DIR"
    cmake . -DBUILD_COMPILER=OFF -DBUILD_C_GLIB=OFF -DBUILD_PYTHON=OFF -DBUILD_TESTING=OFF -DBUILD_JAVA=OFF
    make -j
fi
