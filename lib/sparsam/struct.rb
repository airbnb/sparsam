# -*- coding: UTF-8 -*-
require 'sparsam/base_type'
require 'sparsam/base_struct'

module Sparsam
  class Struct
    include ::Sparsam::BaseType
    include ::Sparsam::StructInitialization
    include ::Sparsam::BaseStruct

    def ==(other)
      return true if other.equal?(self)
      return false unless other.instance_of?(self.class)

      each_field do |fid, info|
        reader = name_to_accessors(info[:name]).reader
        return false unless send(reader) == other.send(reader)
      end

      true
    end
    alias_method :eql?, :==
  end
end
