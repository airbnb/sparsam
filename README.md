# Sparsam [![Build Status](https://travis-ci.org/airbnb/sparsam.svg?branch=master)](https://travis-ci.org/airbnb/sparsam) [![Coverage Status](https://coveralls.io/repos/github/airbnb/sparsam/badge.svg?branch=master)](https://coveralls.io/github/airbnb/sparsam?branch=master)
New Thrift bindings and generator for Ruby!

## Super basic Example
See the docs folder for more detailed information
```
$ sparsam-gen my_struct.thrift
$ bundle exec irb
irb(main):001:0> require './gen-ruby/my_struct_types'
=> true
irb(main):002:0> require 'sparsam'
=> true
irb(main):003:0> obj = MyStruct.new
=> #<MyStruct:0x007fa70d924148>
irb(main):004:0> serialized = obj.serialize # turn object into string
=> "\x00"
irb(main):005:0> obj2 = Sparsam::Deserializer.deserialize( MyStruct, serialized ) # deserialize string into obj
=> #<MyStruct:0x007fa70e3ee998>
```
