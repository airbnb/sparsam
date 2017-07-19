# Thrift Compiler for Sparsam

## Building

Requirements for Linux:
```
sudo apt-get update || sudo apt-get install cmake automake bison flex g++ git libboost1.55-all-dev libevent-dev libssl-dev libtool make pkg-config -y
```

Requirements for Mac
```
brew install thrift bison boost cmake
```

Use the following steps to build using cmake:

    mkdir build
    cd build
    cmake ..
    make

After executing these commands, a binary file named `sparsam-gen` will be built in the `build` directory. 

## Usage
```
sparsam-gen -o <out-dir> <user.thrift>
```
