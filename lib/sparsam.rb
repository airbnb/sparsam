# -*- coding: UTF-8 -*-

require 'sparsam_native'

require 'sparsam/types'
require 'sparsam/exceptions'
require 'sparsam/struct'
require 'sparsam/application_exception'
require 'sparsam/union'
require 'sparsam/deserializer'

Sparsam.init!

module Sparsam
  # Deprecated
  def self.validate(klass, data, strictness)
    data.serialize
    true
  end
end
