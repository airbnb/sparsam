# -*- coding: UTF-8 -*-

require 'rubygems'
require 'rspec'
require 'json'

RSpec.configure do |configuration|
  configuration.before(:each) do
  end
end

$LOAD_PATH.unshift File.join(File.dirname(__FILE__), 'gen-ruby')

require 'sparsam'
require 'user_types'
serialized =        "\x15\x14\x18\x10woohoo blackbird\x1C\x15\xC8\x01\x18\bsubdata!\x00\x1A<\x15"\
                    "\x02\x18\fid_s default\x00\x15\x04\x18\fid_s default\x00\x15\x06\x18\fid_s "\
                    "default\x00+\x02X\x02\x03one\x04\x03two,\x15\xD0\x0F\x00\x00"
serialized_binary = "\b\x00\x01\x00\x00\x00\n\v\x00\x02\x00\x00\x00\x10woohoo blackbird\f\x00"\
                    "\x03\b\x00\x01\x00\x00\x00d\v\x00\x02\x00\x00\x00\bsubdata!\x00\x0E\x00"\
                    "\x04\f\x00\x00\x00\x03\b\x00\x01\x00\x00\x00\x01\v\x00\x02\x00\x00\x00\f"\
                    "id_s default\x00\b\x00\x01\x00\x00\x00\x02\v\x00\x02\x00\x00\x00\fid_s defaul"\
                    "t\x00\b\x00\x01\x00\x00\x00\x03\v\x00\x02\x00\x00\x00\fid_s default\x00\r\x00"\
                    "\x06\b\v\x00\x00\x00\x02\x00\x00\x00\x01\x00\x00\x00\x03one\x00\x00\x00\x02"\
                    "\x00\x00\x00\x03two\f\x00\b\b\x00\x01\x00\x00\x03\xE8\x00\x00"

describe 'Sparsam' do
  describe Sparsam::Serializer do
    it "respect default values" do
      subdata = US.new
      subdata.id_s.should == "id_s default"
    end

    it "can serialize structs" do
      data = SS.new
      data.id_i32 = 10
      data.id_s = "woohoo blackbird"
      subdata = US.new
      subdata.id_i32 = 100
      subdata.id_s = "subdata!"
      data.us_i = subdata
      data.us_s = Set.new
      data.us_s.add(US.new({ "id_i32" => 1 }))
      data.us_s.add(US.new({ "id_i32" => 2 }))
      data.us_s.add(US.new({ "id_i32" => 3 }))
      data.mappy = {}
      data.mappy[1] = "one"
      data.mappy[2] = "two"
      data.un_field = UN.new({ :id_i32 => 1000 })
      result = data.serialize
      Sparsam.validate(SS, data, Sparsam::RECURSIVE).should == true
      result.force_encoding("BINARY").should == serialized.force_encoding("BINARY")
    end

    it "can handle utf-8 strings" do
      data = SS.new
      data.id_s = "中国电信"
      result = data.serialize
      data2 = Sparsam::Deserializer.deserialize(SS, result)
      data2.id_s.encoding.should == data.id_s.encoding
      data2.id_s.should == data.id_s
    end

    it "can deserialize structs" do
      data = Sparsam::Deserializer.deserialize(SS, serialized)
      data.id_i32.should == 10
      data.id_s.should == "woohoo blackbird"
      data.mappy[1].should == "one"
      data.mappy[2].should == "two"
      data.us_i.id_i32.should == 100
      data.us_i.id_s.should == "subdata!"
      data.un_field.id_i32.should == 1000
      data.us_s.size.should == 3
      ids = Set.new([1, 2, 3])
      data.us_s.each { |val|
        ids.delete(val.id_i32)
        val.id_s.should == "id_s default"
      }
      ids.size.should == 0
    end

    it "can deserialize unions" do
      data = UN.new({ :id_i32 => 1000 })
      result = data.serialize
      data2 = Sparsam::Deserializer.deserialize(UN, result)
      Sparsam.validate(UN, data2, Sparsam::RECURSIVE).should == true
      data2.id_i32.should == 1000
    end

    it "can handle passing in initialization data" do
      init = { "id_i32" => 10, "id_s" => "woohoo blackbird" }
      data = SS.new(init)
      data.id_i32.should == 10
      data.id_s.should == "woohoo blackbird"
    end

    it "will throw exceptions when strict validation received a non-conforming type" do
      data = EasilyInvalid.new
      data.tail = SS.new
      expect {
        Sparsam.validate(NotSS, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      data = EasilyInvalid.new
      data.s_self = Set.new([EasilyInvalid.new, SS.new])
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      data = EasilyInvalid.new
      data.l_self = [EasilyInvalid.new, SS.new]
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      data = EasilyInvalid.new
      data.mappy1 = { SS.new => 123 }
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      data = EasilyInvalid.new
      data.mappy2 = { 123 => SS.new }
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      data = EasilyInvalid.new
      data.mappy3 = { SS.new => SS.new }
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      data = EasilyInvalid.new
      data.mappy3 = { EasilyInvalid.new => SS.new }
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      data = EasilyInvalid.new
      data.mappy3 = { SS.new => EasilyInvalid.new }
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      data = EasilyInvalid.new
      data.mappy3 = { EasilyInvalid.new => EasilyInvalid.new, SS.new => EasilyInvalid.new }
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)

      data = EasilyInvalid.new
      data.id_i32 = "I'm pretty sure this is not an I32 LOL"
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      }.to raise_error(Sparsam::TypeMismatch)
    end

    it "includes additional data in TypeMismatch errors" do
      data = EasilyInvalid.new
      data.id_i32 = "definitely a string"

      e = nil
      begin
        Sparsam.validate(EasilyInvalid, data, Sparsam::STRICT)
      rescue Sparsam::TypeMismatch => exception
        e = exception
      end

      e.struct_name.should == EasilyInvalid.name
      e.field_name.should == "id_i32"
    end

    it "works with crazy thriftness" do
      data = EasilyInvalid.new
      data.sure = [{ Set.new([1]) => { 1 => Set.new([[{ EasilyInvalid.new => "sure" }]]) } }]
      Sparsam.validate(EasilyInvalid, data, Sparsam::RECURSIVE).should == true

      data = EasilyInvalid.new
      data.sure = [{ Set.new([1]) => { 1 => Set.new([[{ EasilyInvalid.new => 123 }]]) } }]
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::RECURSIVE)
      }.to raise_error(Sparsam::TypeMismatch)
    end

    it "will throw exceptions when recursive validation is passed wrong data" do
      data = EasilyInvalid.new
      data.required_stuff = MiniRequired.new
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::RECURSIVE)
      }.to raise_error(Sparsam::MissingMandatory)

      data = EasilyInvalid.new
      data.tail = EasilyInvalid.new
      data.tail.s_self = Set.new([SS.new])
      expect {
        Sparsam.validate(EasilyInvalid, data, Sparsam::RECURSIVE)
      }.to raise_error(Sparsam::TypeMismatch)
    end

    it "will throw exceptions when passed data that doesn't match type" do
      data = SS.new
      data.id_i32 = "I am not an int"
      expect { data.serialize }.to raise_error(StandardError)
    end

    it "will validate required fields" do
      data = MiniRequired.new
      expect { data.validate }.to raise_error(Sparsam::MissingMandatory)
    end

    it "includes additional information on missing required fields in exception" do
      data = MiniRequired.new

      e = nil
      begin
        data.validate
      rescue Sparsam::MissingMandatory => exception
        e = exception
      end

      e.struct_name.should == MiniRequired.name
      e.field_name.should == "id_i32"
    end

    it "will throw errors when given junk data" do
      expect {
        Sparsam::Deserializer.deserialize(SS, "wolololololol")
      }.to raise_error(Sparsam::Exception)
    end

    it "will throw errors when deserializing data with incorrect types" do
      # SS expects field 1 to be an INT
      # this is a struct w/ field 1 as a STRING instead
      data = NotSS.new
      data.id_s = "I am not an INT"
      bad_type = data.serialize
      expect {
        Sparsam::Deserializer.deserialize(SS, bad_type)
      }.to raise_error(Sparsam::TypeMismatch)
    end

    it "can deserialize objects with fields it doesn't know about" do
      # NotSS_plus is NotSS with additional fields
      # Thrift should ignore the additional fields
      data = NotSS_plus.new
      data.id_s = "This is ok"
      data.id_s2 = "This is also ok"
      data.id_i32 = 100
      ser = data.serialize
      notss = Sparsam::Deserializer.deserialize(NotSS, ser)
      notss.id_s.should == "This is ok"
      notss.id_i32.should == 100
      Sparsam::Deserializer.deserialize(Nothing, ser)
    end

    it 'only allows one field to be set in a union' do
      expect {
        UN.new({ :id_i32 => 1000, :id_s => "woops" })
      }.to raise_error(Sparsam::UnionException)

      d = UN.new({ :id_i32 => 1000 })
      d.id_s = "woops"
      d.id_s

      expect {
        d.id_i32
      }.to raise_error(Sparsam::UnionException)

      d.instance_variables.should eq([:@setfield, :@id_s])
    end

    it 'handles empty arrays' do
      data = SS.new
      data.mappy = {}
      data.us_s = Set.new
      ser = data.serialize
      unser = Sparsam::Deserializer.deserialize(SS, ser)
      unser.mappy.size.should == 0
      unser.us_s.size.should == 0
    end

    it "can serialize structs with binary" do
      data = SS.new
      data.id_i32 = 10
      data.id_s = "woohoo blackbird"
      data.mappy = {}
      data.mappy[1] = "one"
      data.mappy[2] = "two"
      subdata = US.new
      subdata.id_i32 = 100
      subdata.id_s = "subdata!"
      data.us_i = subdata
      data.us_s = Set.new
      data.us_s.add(US.new({ "id_i32" => 1 }))
      data.us_s.add(US.new({ "id_i32" => 2 }))
      data.us_s.add(US.new({ "id_i32" => 3 }))
      data.un_field = UN.new({ :id_i32 => 1000 })
      result = data.serialize(Sparsam::BinaryProtocol)
      result.force_encoding("BINARY").should == serialized_binary.force_encoding("BINARY")
    end

    it "can deserialize structs with binary" do
      data = Sparsam::Deserializer.deserialize(SS, serialized_binary, Sparsam::BinaryProtocol)
      data.id_i32.should == 10
      data.id_s.should == "woohoo blackbird"
      data.mappy[1].should == "one"
      data.mappy[2].should == "two"
      data.mappy.size.should == 2
      data.us_i.id_i32.should == 100
      data.us_i.id_s.should == "subdata!"
      data.un_field.id_i32.should == 1000
      data.us_s.size.should == 3
      ids = Set.new([1, 2, 3])
      data.us_s.each { |val|
        ids.delete(val.id_i32)
        val.id_s.should == "id_s default"
      }
      ids.size.should == 0
    end

    it "can handle nested collections like a boss" do
      data = SS.new
      data.troll = {}
      data.troll[1] = { 2 => 3 }
      ser = data.serialize
      new_data = Sparsam::Deserializer.deserialize(SS, ser)
      new_data.troll[1][2].should == 3
    end

    it "doesn't segfault on malformed data" do
      really_bad = "\x0f\x00\x05\x0b\x00\x00\x00\x01\x00\x00\x00\x03\x00"
      expect {
        Sparsam::Deserializer.deserialize(SS, really_bad, Sparsam::BinaryProtocol)
      }.to raise_error(Sparsam::Exception)
    end

    it "handles all sorts of type issues without crashing" do
      field_map = {
        a_bool: :boolean,
        a_byte: :int,
        an_i16: :int,
        an_i32: :int,
        an_i64: :int,
        a_double: :float,
        a_binary: :string,
        a_string: :string,

        an_i64_list: :int_list,
        an_i64_set: :int_set,
        an_i64_map: :int_map,

        a_list_of_i64_maps: :int_map_list,
        a_map_of_i64_maps: :int_map_map,

        a_struct: :struct,
        a_union: :union,
      }

      scalar_values = {
        boolean: true,
        int: 42,
        float: 3.14,
        string: "Hello",
        struct: US.new(id_i32: 10),
        union: UN.new(id_s: "woo"),
        complex: Complex(1),
        rational: Rational(2, 3),
      }

      simple_collection_values = scalar_values.each_with_object({}) do |(type, val), obj|
        obj[:"#{type}_list"] = [val]
        obj[:"#{type}_set"] = Set.new([val])
        obj[:"#{type}_map"] = { val => val }
      end

      nested_collection_values =
        simple_collection_values.each_with_object({}) do |(type, val), obj|
          obj[:"#{type}_list"] = [val]
          obj[:"#{type}_set"] = Set.new([val])
          obj[:"#{type}_map"] = { val => val }
        end

      all_values = scalar_values.merge(simple_collection_values).merge(nested_collection_values)

      field_map.each do |field, type|
        all_values.each do |val_type, val|
          next if val_type == type

          s = EveryType.new
          s.send(:"#{field}=", val)

          # Validation doesn't do range checking, though serialization does
          unless val_type.to_s =~ /bigint/
            expect {
              Sparsam.validate(s.class, s, Sparsam::STRICT)
            }.to(
              raise_error(Sparsam::TypeMismatch),
              "assigning #{field} : #{type} a value of " \
              "#{val.inspect} : #{val_type} did not raise TypeMismatch"
            )
          end

          expect {
            s.serialize
          }.to(
            raise_error(Sparsam::TypeMismatch),
            "assigning #{field} : #{type} a value of " \
            "#{val.inspect} : #{val_type} did not raise TypeMismatch"
          )
        end
      end
    end

    unless RUBY_VERSION =~ /^1\.9/
      it "handles integer ranges" do
        fields = {
          a_byte: 8,
          an_i16: 16,
          an_i32: 32,
          an_i64: 64,
        }

        fields.each do |field, size|
          s = EveryType.new

          max_val = 2**(size - 1) - 1

          [max_val, ~max_val].each do |val|
            s.send(:"#{field}=", val)

            expect {
              s.serialize
            }.not_to raise_error, "#{field} of #{size} bits unable to hold #{val}"
          end

          [max_val + 1, ~(max_val + 1)].each do |val|
            s.send(:"#{field}=", val)
            expect {
              s.serialize
            }.to(
              raise_error(RangeError),
              "#{field} of #{size} bits apparently able to hold value #{val} in defiance of nature"
            )
          end
        end
      end
    end

    it 'handles structs with modified eigenclasses' do
      nested_struct = US.new

      class << nested_struct
        def foo
        end
      end

      data = SS.new(us_i: nested_struct)

      class << data
        def foo
        end
      end

      expect { data.serialize }.not_to raise_error
    end

    describe 'ApplicationException' do
      it 'creates exceptions that can be raised' do
        e = SimpleException.new(message: "Oops")

        e.message.should eq("Oops")

        expect { raise e }.to raise_error(SimpleException, "Oops")
      end

      it 'serializes and reads exceptions' do
        e = SimpleException.new(message: "Oops")

        data = e.serialize

        e2 = Sparsam::Deserializer.deserialize(SimpleException, data)

        e2.message.should eq("Oops")

        expect { raise e2 }.to raise_error(SimpleException, "Oops")
      end
    end
  end
end
