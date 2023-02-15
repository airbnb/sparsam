# -*- coding: UTF-8 -*-

require 'sparsam/base_type'

module Sparsam
  class Union
    include ::Sparsam::BaseType

    # rubocop:disable Airbnb/OptArgParameters
    def initialize(name = nil, value = nil)
      # rubocop:enable Airbnb/OptArgParameters
      if name
        if name.is_a? Hash
          if name.size > 1
            raise ::Sparsam::UnionException,
              "#{self.class} cannot be instantiated with more than one field!"
          end
          value = name.values.first
          name = name.keys.first
        end
      end

      if name
        accessors = name_to_accessors(name)
        unless accessors
          raise Sparsam::Exception, "Unknown key given to #{self.class}.new: #{name}"
        end

        send(accessors.writer, value)
      end
    end

    def ==(other)
      other.equal?(self) ||
      other.instance_of?(self.class) &&
      @setfield == other.get_set_field &&
      get_value == other.get_value
    end
    alias_method :eql?, :==

    def hash
      [self.class.name, @setfield, get_value].hash
    end

    def get_set_field
      @setfield
    end

    def get_value
      if @setfield
        send(name_to_accessors(@setfield).reader)
      end
    end

    def self.field_accessor(klass, field_key, field_info)
      field_name = field_info[:name]
      klass.class_eval(<<-EOF, __FILE__, __LINE__)
        def #{field_name}
          if :'#{field_name}' == @setfield
            instance_variable_get(:'@#{field_name}')
          else
            raise ::Sparsam::UnionException, "#{field_name} is not union's set field"
          end
        end

        def #{field_name}=(value)
          if @setfield
            remove_instance_variable(:"@\#{@setfield}")
          end

          @setfield = :'#{field_name}'
          instance_variable_set(:'@#{field_name}', value)
        end
      EOF
    end
  end
end
