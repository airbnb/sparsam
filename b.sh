#!/bin/bash
set -e
set -x

export LDFLAGS="-L/opt/homebrew/opt/thrift@0.9/lib -L/opt/homebrew/opt/boost/lib"
export CPPFLAGS="-I/opt/homebrew/opt/thrift@0.9/include -I/opt/homebrew/opt/boost/include"

bundle exec rake


