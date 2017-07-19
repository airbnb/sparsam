# -*- coding: UTF-8 -*-
require 'set'
require 'sparsam/exceptions'

module Sparsam
  module Types
    STOP = 0
    VOID = 1
    BOOL = 2
    BYTE = 3
    DOUBLE = 4
    I16 = 6
    I32 = 8
    I64 = 10
    STRING = 11
    STRUCT = 12
    MAP = 13
    SET = 14
    LIST = 15

    COLLECTIONS = Set.new([
      SET,
      LIST,
      MAP,
    ]).freeze
  end

  # Deprecated type checking

  def self.check_type(value, field, name, skip_nil = true)
    return if value.nil? && skip_nil

    valid =
      case field[:type]
      when Types::VOID
        nil === value
      when Types::BOOL
        true === value || false === value
      when Types::BYTE, Types::I16, Types::I32, Types::I64
        Integer === value
      when Types::DOUBLE
        Float === value
      when Types::STRING
        String === value
      when Types::STRUCT
        Struct === value || Union === value
      when Types::MAP
        Hash === value
      when Types::SET
        Set === value
      when Types::LIST
        Array === value
      else
        false
      end

    unless valid
      raise TypeMismatch, "Expected #{type_name(field[:type])}, " \
        "received #{value.class} for field #{name}"
    end

    # check elements now
    case field[:type]
    when Types::MAP
      # This is still allocations per MAP, but better than per map entry
      key_str = "#{name}.key"
      value_str = "#{name}.value"

      value.each_pair do |k, v|
        check_type(k, field[:key], key_str, false)
        check_type(v, field[:value], value_str, false)
      end
    when Types::SET, Types::LIST
      element_str = "#{name}.element"

      value.each do |el|
        check_type(el, field[:element], element_str, false)
      end
    when Types::STRUCT
      unless field[:class] == value.class
        raise TypeMismatch, "Expected #{field[:class]}, received #{value.class} for field #{name}"
      end
    end
  end

  TYPE_NAME_SYM_MAPPING = Types.constants.each_with_object({}) do |const, h|
    h[Types.const_get(const)] = const.to_sym
  end

  TYPE_NAME_MAPPING = TYPE_NAME_SYM_MAPPING.each_with_object({}) do |(k, const), h|
    h[k] = "Types::#{const}".freeze
  end.freeze

  def self.type_name_sym(type)
    TYPE_NAME_SYM_MAPPING[type]
  end

  def self.type_name(type)
    TYPE_NAME_MAPPING[type]
  end

  module MessageTypes
    CALL = 1
    REPLY = 2
    EXCEPTION = 3
    ONEWAY = 4
  end
end
