# Installation

## Sparsam Gem Installation

Sparsam depends on Apache Thrift and Boost, so the first step is to install the dependencies:
- On macOS: `brew install thrift@0.9 boost`
- On linux: `sudo apt-get install libthrift-dev libboost1.55-all-dev`

Afterwards, the gem for sparsam can be installed normally:
```bash
gem install sparsam
```

Note that sparsam may not compile with newer versions of thrift. e.g.

```
serializer.cpp:113:43: error: no matching constructor for initialization of 'apache::thrift::protocol::TCompactProtocol' (aka 'TCompactProtocolT<apache::thrift::transport::TTransport>')
```

Older versions of thrift (such as version 0.9) are known to work.

## Homebrew Integration

`extconf.rb` will detect and automatically use a `brew` executable on
`$PATH` to configure the compiler to build against Homebrew's thrift and
boost packages.

By default, it will automatically look for and use the `thrift@0.9`
and `boost` packages.

To disable automatic Homebrew integration, pass `--disable-homebrew` to
the build.

To specify an alternative Homebrew package providing thrift, pass
`--with-homebrew-thrift-package=<package>`. e.g.
`--with-homebrew-thrift-package=mythrift`.

To specify an alternative Homebrew package providing boost, pass
`--with-homebrew-boost-package=<package>`.

The default configuration can be set globally using `bundle config`. e.g.:

```
# Disable default Homebrew integration.
$ bundle config --global build.sparsam --disable-homebrew

# To use an alternative Homebrew package providing thrift.
$ bundle config --global build.sparsam '--with-homebrew-thrift-package=mythrift'
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
brew install thrift@0.9 bison boost cmake
```

Use the following steps to build using cmake:

    mkdir build
    cd build
    cmake ..
    make

After executing these commands, a binary file named `sparsam-gen` will be built in the `build` directory. 

