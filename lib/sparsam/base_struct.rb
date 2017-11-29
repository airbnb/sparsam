module Sparsam
  module BaseStruct
    def self.included(base)
      base.extend(ClassMethods)
    end

    module ClassMethods
      def field_accessor(klass, field_key, field_info)
        field_name = field_info[:name]
        klass.class_eval(<<-EOF, __FILE__, __LINE__)
          attr_accessor :'#{field_name}'
        EOF
      end
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
