require 'sparsam/base_type'
require 'sparsam/base_struct'

module Sparsam
  class ApplicationException < StandardError
    include ::Sparsam::BaseType
    include ::Sparsam::StructInitialization
    include ::Sparsam::BaseStruct
  end
end
