require 'set'
require 'sparsam/types'

module Sparsam
  module BaseType
    class AccessorNames < Struct.new(:reader, :writer); end

    def self.included(base)
      base.extend(ClassMethods)
    end

    module ClassMethods
      def generate_field_syms(klass)
        field_syms = {}
        klass::FIELDS.each do |_, field_info|
          name = field_info[:name].to_sym
          accessors = AccessorNames.new(name, :"#{name}=")
          field_syms[name] = accessors
          field_syms[name.to_s] = accessors
        end

        klass.const_set(
          :FIELD_SYMS,
          field_syms
        )
      end

      def generate_accessors(klass)
        klass::FIELDS.each do |field_key, field_info|
          field_accessor(klass, field_key, field_info)
        end
      end

      def generate_default_values(klass)
        fields_with_default_values = {}
        klass::FIELDS.each do |fid, field_def|
          unless field_def[:default].nil?
            fields_with_default_values[field_def[:name]] = field_def[:default].freeze
          end
        end

        if fields_with_default_values.empty?
          klass.const_set(
            :DEFAULT_VALUES,
            nil
          )
        else
          klass.const_set(
            :DEFAULT_VALUES,
            fields_with_default_values
          )
        end
      end

      def init_thrift_struct(klass)
        generate_accessors(klass)
        generate_field_syms(klass)
        Sparsam.cache_fields(klass)
        generate_default_values(klass)
        klass.class_eval(<<-EOF, __FILE__, __LINE__)
          def struct_fields
            FIELDS
          end
        EOF
      end
    end

    # rubocop:disable Airbnb/OptArgParameters
    def serialize(prot = Sparsam::CompactProtocol)
      # rubocop:enable Airbnb/OptArgParameters
      s = Sparsam::Serializer.new(prot, "")
      s.serialize(self.class, self)
    end

    # rubocop:disable Airbnb/OptArgParameters
    def validate(mode = Sparsam::NORMAL)
      # rubocop:enable Airbnb/OptArgParameters
      Sparsam.validate(self.class, self, mode)
    end

    private

    def name_to_accessors(name)
      self.class::FIELD_SYMS[name]
    end

    def each_field
      struct_fields.each do |fid, data|
        yield fid, data
      end
    end
  end
end
