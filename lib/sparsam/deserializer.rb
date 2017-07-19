module Sparsam
  module Deserializer
    def self.deserialize(klass, serialized_string, prot = Sparsam::CompactProtocol)
      s = Sparsam::Serializer.new(prot, serialized_string)
      s.deserialize(klass)
    end
  end
end
