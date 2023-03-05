# -*- coding: UTF-8 -*-

module Sparsam
  class Exception < StandardError
    def initialize(msg)
      super
    end
  end

  class MissingMandatory < ::Sparsam::Exception
    attr_reader :struct_name, :field_name

    # rubocop:disable Airbnb/OptArgParameters
    def initialize(msg, struct_name = nil, field_name = nil)
      # rubocop:enable Airbnb/OptArgParameters
      @struct_name = struct_name
      @field_name = field_name
      super(msg)
    end
  end

  class TypeMismatch < ::Sparsam::Exception
    attr_reader :struct_name, :field_name

    # rubocop:disable Airbnb/OptArgParameters
    def initialize(msg, struct_name = nil, field_name = nil)
      # rubocop:enable Airbnb/OptArgParameters
      @struct_name = struct_name
      @field_name = field_name
      super(msg)
    end
  end

  class UnionException < ::Sparsam::Exception
    def initialize(msg)
      super
    end
  end

  class UnknownTypeException < ::Sparsam::Exception
    def initialize(msg)
      super
    end
  end
end
