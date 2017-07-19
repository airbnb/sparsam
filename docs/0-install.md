# Installation

## Sparsam Gem Installation

Sparsam depends on Apache Thrift and Boost, so the first step is to install the dependencies:
- On macOS: `brew install thrift boost`
- On linux: `sudo apt-get install libthrift-dev libboost1.55-all-dev`

Afterwards, the gem for sparsam can be installed normally:
```bash
gem install sparsam
```

## Sparsam Compiler Installation

To use sparsam compiler, we recommend downloading a binary file `sparsam-gen` for your system from [GitHub Releases](https://github.com).

Alternatively, the compiler can also be built directly from source.

### Building Compiler from Source

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

