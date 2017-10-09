## Compiler Usage {#compiler}

Sparsam's compiler has the exact same usage as [Thrift Compiler](https://thrift.apache.org/tutorial/#generate-thrift-file-to-source-code),
except that `--gen` switch is not supported (since we are only generating files for ruby).

```
./sparsam-gen -o <out-dir> <schema.thrift>
```

If you want ruby files generated in idiomatic directories, you can run:
```
./sparsam-gen -namespaced -o <out-dir> <schema.thrift>
```

## Serialization & Deserialization {#serialization}

```ruby
# To use Sparsam's generated objects, first import the generated 
require 'gen-ruby/thrift_struct_types.rb'
require 'sparsam'

# You can use this object like any other ruby object
obj = ThriftStruct.new
obj.field = "value"
obj.list_field = [1, 2, 3]
obj.map_field = {1 => 10, 2 => 20}

# Now you can serialize this object (by default, sparsam uses Compact Protocl)
serialized_str = obj.serialize
serialized_str = obj.serialize(Sparsam::CompactProtocol)  # equivalent

# You can also serialize it with Binary Protocol
serialized_bin = obj.serialize(Sparsam::BinaryProtocol)

# To deserialize an object with Compact Protocol
obj_new = Sparsam::Deserializer.deserialize(ThriftStruct, serialized_str)
obj_new = Sparsam::Deserializer.deserialize(ThriftStruct, serialized_str, Sparsam::CompactProtocol)

# Binary protocol can also be used for deserialization
obj_new = Sparsam::Deserializer.deserialize(ThriftStruct, serialized_str, Sparsam::BinaryProtocol)

```

## Validation {#validation}

On top of the default thrift validation behavior that only checks for required fields,
Sparsam offers two additional validation modes.

* `obj.validate` or `obj.validate(Sparsam::DEFAULT)`: default mode. Only checks required fields
* `obj.validate(Sparsam::STRICT)`: strict mode. This mode will check both required fields and type conformance for top-level fields. However, type conformance of fields in nested structs or structs in containers will not be checked.
* `obj.validate(Sparsam::RECURSIVE)`: like strict mode, but also recursively check nested structs.
