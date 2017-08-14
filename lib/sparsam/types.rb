# -*- coding: UTF-8 -*-
require 'set'

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
end
