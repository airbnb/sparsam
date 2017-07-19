# -*- coding: UTF-8 -*-

module Sparsam
  class Exception < StandardError
    def initialize(msg)
      super
    end
  end

  class MissingMandatory < Exception
    def initialize(msg)
      super
    end
  end

  class TypeMismatch < Exception
    def initialize(msg)
      super
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
