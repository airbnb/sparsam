# -*- coding: UTF-8 -*-

module Sparsam
  class Exception < StandardError
    def initialize(msg)
      super
    end
  end

  class MissingMandatory < Exception
    attr_reader :struct_name, :field_name

    def initialize(msg, struct_name = nil, field_name = nil)
      @struct_name = struct_name
      @field_name = field_name
      super(msg)
    end
  end

  class TypeMismatch < Exception
    attr_reader :struct_name, :field_name

    def initialize(msg, struct_name = nil, field_name = nil)
      @struct_name = struct_name
      @field_name = field_name
      super(msg)
    end
  end

  class UnionException < Exception
    def initialize(msg)
      super
    end
  end

  class UnknownTypeException < Exception
    def initialize(msg)
      super
    end
  end
end
