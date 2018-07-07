module Sparsam
  module Deserializer
    def self.deserialize(klass, serialized_string, prot = Sparsam::CompactProtocol)
      s = Sparsam::Serializer.new(prot, serialized_string)
      if klass <= Sparsam::Union
        s.deserializeUnion(klass)
      else
        s.deserialize(klass)
      end
    end
  end
end
