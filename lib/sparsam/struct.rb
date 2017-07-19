# -*- coding: UTF-8 -*-
require 'sparsam/base_class'

module Sparsam
  class Struct < ::Sparsam::BaseClass
    include ::Sparsam::StructInitialization

    def ==(other)
      return true if other.equal?(self)
      return false unless other.instance_of?(self.class)

      each_field do |fid, info|
        reader = name_to_accessors(info[:name]).reader
        return false unless send(reader) == other.send(reader)
      end
    end
    alias_method :eql?, :==

    def self.field_accessor(klass, field_key, field_info)
      field_name = field_info[:name]
      klass.class_eval(<<-EOF, __FILE__, __LINE__)
        attr_accessor :'#{field_name}'
      EOF
    end

    private

    def assign_defaults(defaults)
      defaults.each do |name, default_value|
        accessors = name_to_accessors(name)
        send(accessors.writer, default_value)
      end
    end

    def assign_from_arg(d)
      d.each do |name, value|
        accessors = name_to_accessors(name)
        unless accessors
          raise Sparsam::Exception, "Unknown key given to #{self.class}.new: #{name}"
        end
        send(accessors.writer, value)
      end
    end
  end
end
