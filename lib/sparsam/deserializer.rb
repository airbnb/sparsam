module Sparsam
  module Deserializer
    # rubocop:disable Airbnb/OptArgParameters
    def self.deserialize(klass, serialized_string, prot = Sparsam::CompactProtocol)
      # rubocop:enable Airbnb/OptArgParameters
      s = Sparsam::Serializer.new(prot, serialized_string)
      if klass <= Sparsam::Union
        s.deserializeUnion(klass)
      else
        s.deserialize(klass)
      end
    end
  end
end
