#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>

#include "t_oop_generator.h"
#include "platform.h"
#include "version.h"

using std::map;
using std::ofstream;
using std::ostringstream;
using std::string;
using std::stringstream;
using std::vector;

static const string endl = "\n"; // avoid ostream << std::endl flushes

/*
 * Heavily based on/copied from the t_rb_generator
 * But with modifications to generate simpler ruby classes
 * with the expectation that we don't need all of thrifts features.
 * We currently go to/from buffers (strings) and hence don't need
 * services and exceptions.
 */

class t_ruby_ofstream: public std::ofstream {
  private:
    int indent_;

  public:
    t_ruby_ofstream() : std::ofstream(), indent_(0) {}
    explicit t_ruby_ofstream(const char* filename,
        ios_base::openmode mode = ios_base::out,
        int indent = 0)
      : std::ofstream(filename, mode), indent_(indent) {}

    t_ruby_ofstream& indent() {
      for (int i = 0; i < indent_; ++i) {
        *this << "  ";
      }
      return *this;
    }

    void indent_up() { indent_++; }
    void indent_down() { indent_--; }
};

class t_ruby_generator : public t_oop_generator {
  public:
    t_ruby_generator(
        t_program* program,
        const map<string,string>& parsed_options,
        const string& option_string
    ): t_oop_generator(program) {
      out_dir_base_ = "gen-ruby";
      namespaced_ = (parsed_options.find("namespaced") != parsed_options.end());
    }

    void init_generator();
    void close_generator();

    void generate_typedef(t_typedef* ttypedef);
    void generate_enum(t_enum* tenum);
    void generate_const(t_const* tconst);
    void generate_struct(t_struct* tstruct);
    void generate_union(t_struct* tunion);
    t_ruby_ofstream& render_const_value(t_ruby_ofstream& out, t_type* type, t_const_value* value);

    void generate_initialize(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_rb_struct(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_rb_function_helpers(t_function* tfunction);
    void generate_rb_simple_constructor(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_rb_simple_exception_constructor(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_field_constants(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_field_constructors(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_field_defns(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_field_accessors(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_field_data(t_ruby_ofstream& out,
        t_type* field_type,
        const std::string& field_name,
        t_const_value* field_value,
        bool optional);
    void generate_writer(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_reader(t_ruby_ofstream& out, t_struct* tstruct);
    void generate_serialize_map_element(t_ruby_ofstream& out, t_map* tmap);
    void generate_serialize_struct(t_ruby_ofstream& out,
        t_struct* tstruct,
        string name);
    void generate_serialize_field(t_ruby_ofstream& out,
        t_field* tfield);
    void _generate_serialize_field(t_ruby_ofstream& out,
        t_type* tfield,
        const string& name);
    void generate_serialize_container(
        t_ruby_ofstream& out,
        t_type* ttype,
        string name);
    void generate_deserialize_field(
        t_ruby_ofstream& out,
        const string& to_field,
        t_type* type);
    void generate_deserialize_container(
        t_ruby_ofstream& out,
        const string& to_field,
        t_type* type);

    void generate_rdoc(t_ruby_ofstream& out, t_doc* tdoc);

    std::string rb_autogen_comment();
    std::string render_includes();
    std::string declare_field(t_field* tfield);
    std::string type_name(t_type* ttype);
    std::string full_type_name(t_type* ttype);
    std::string function_signature(t_function* tfunction, std::string prefix = "");
    std::string argument_list(t_struct* tstruct);
    std::string type_to_enum(t_type* ttype);
    std::string rb_namespace_to_path_prefix(std::string rb_namespace);

    std::vector<std::string> ruby_modules(t_program* p) {
      std::string ns = p->get_namespace("rb");
      std::vector<std::string> modules;
      if (ns.empty()) {
        return modules;
      }

      std::string::iterator pos = ns.begin();
      while (true) {
        std::string::iterator delim = std::find(pos, ns.end(), '.');
        modules.push_back(capitalize(std::string(pos, delim)));
        pos = delim;
        if (pos == ns.end()) {
          break;
        }
        ++pos;
      }

      return modules;
    }

    void begin_namespace(t_ruby_ofstream&, std::vector<std::string>);
    void end_namespace(t_ruby_ofstream&, std::vector<std::string>);

    // No services!
    void generate_service(t_service* tservice) {};

  private:
    t_ruby_ofstream f_types_;
    t_ruby_ofstream f_consts_;

    std::string namespace_dir_;
    std::string require_prefix_;

    bool namespaced_;
};

void t_ruby_generator::init_generator() {
  string subdir = get_out_dir();

  // Make output directory
  MKDIR(subdir.c_str());

  if (namespaced_) {
    require_prefix_ = rb_namespace_to_path_prefix(program_->get_namespace("rb"));

    string dir = require_prefix_;
    string::size_type loc;

    while ((loc = dir.find("/")) != string::npos) {
      subdir = subdir + dir.substr(0, loc) + "/";
      MKDIR(subdir.c_str());
      dir = dir.substr(loc + 1);
    }
  }

  namespace_dir_ = subdir;

  // Make output file
  string f_types_name = namespace_dir_ + underscore(program_name_) + "_types.rb";
  f_types_.open(f_types_name.c_str());

  string f_consts_name = namespace_dir_ + underscore(program_name_) + "_constants.rb";
  f_consts_.open(f_consts_name.c_str());

  // Print header
  f_types_
    << rb_autogen_comment() << endl
    << "require 'sparsam'\n"
    << render_includes() << endl;

  begin_namespace(f_types_, ruby_modules(program_));

  f_consts_
    << rb_autogen_comment() << endl
    << "require 'sparsam'\n"
    << "require '"<< require_prefix_ << underscore(program_name_) << "_types'" << endl
    << endl;
  begin_namespace(f_consts_, ruby_modules(program_));
}

string t_ruby_generator::rb_autogen_comment() {
  return std::string("#\n") + "# Autogenerated by Thrift Compiler (" + THRIFT_VERSION + ")\n"
         + "#\n" + "# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING\n" + "#\n";
}

void t_ruby_generator::close_generator() {
  // Close types file
  end_namespace(f_types_, ruby_modules(program_));
  end_namespace(f_consts_, ruby_modules(program_));
  f_types_.close();
  f_consts_.close();
}

// no typedefs in Ruby
void t_ruby_generator::generate_typedef(t_typedef* ttypedef) {
  (void)ttypedef;
}

/**
 * Generates code for an enumerated type. Done using a class to scope
 * the values.
 *
 * @param tenum The enumeration
 */
void t_ruby_generator::generate_enum(t_enum* tenum) {
  f_types_.indent() << "module " << capitalize(tenum->get_name()) << endl;
  f_types_.indent_up();

  vector<t_enum_value*> constants = tenum->get_constants();
  vector<t_enum_value*>::iterator c_iter;
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int value = (*c_iter)->get_value();

    string name = capitalize((*c_iter)->get_name());

    generate_rdoc(f_types_, *c_iter);
    f_types_.indent() << name << " = " << value << endl;
  }

  // Create a hash mapping values back to their names (as strings) since ruby has no native enum
  // type
  f_types_.indent() << "VALUE_MAP = {";
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    // Populate the hash
    int value = (*c_iter)->get_value();
    if (c_iter != constants.begin())
      f_types_ << ", ";
    f_types_ << value << " => \"" << capitalize((*c_iter)->get_name()) << "\"";
  }
  f_types_ << "}" << endl;

  // Create a set with valid values for this enum
  f_types_.indent() << "VALID_VALUES = Set.new([";
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    // Populate the set
    if (c_iter != constants.begin())
      f_types_ << ", ";
    f_types_ << capitalize((*c_iter)->get_name());
  }
  f_types_ << "]).freeze" << endl;

  f_types_.indent_down();
  f_types_.indent() << "end" << endl << endl;
}

/**
 * Generate a constant value
 */
void t_ruby_generator::generate_const(t_const* tconst) {
  t_type* type = tconst->get_type();
  string name = tconst->get_name();
  t_const_value* value = tconst->get_value();

  name[0] = toupper(name[0]);

  f_consts_.indent() << name << " = ";
  render_const_value(f_consts_, type, value) << endl << endl;
}

/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
t_ruby_ofstream& t_ruby_generator::render_const_value(
    t_ruby_ofstream& out,
    t_type* type,
    t_const_value* value) {
  type = get_true_type(type);
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
      out << "%q\"" << get_escaped_string(value) << '"';
      break;
    case t_base_type::TYPE_BOOL:
      out << (value->get_integer() > 0 ? "true" : "false");
      break;
    case t_base_type::TYPE_BYTE:
    case t_base_type::TYPE_I16:
    case t_base_type::TYPE_I32:
    case t_base_type::TYPE_I64:
      out << value->get_integer();
      break;
    case t_base_type::TYPE_DOUBLE:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        out << value->get_integer();
      } else {
        out << value->get_double();
      }
      break;
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    out.indent() << value->get_integer();
  } else if (type->is_struct() || type->is_xception()) {
    out << full_type_name(type) << ".new({" << endl;
    out.indent_up();
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      t_type* field_type = NULL;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == NULL) {
        throw "type error: " + type->get_name() + " has no field " + v_iter->first->get_string();
      }
      out.indent();
      render_const_value(out, g_type_string, v_iter->first) << " => ";
      render_const_value(out, field_type, v_iter->second) << "," << endl;
    }
    out.indent_down();
    out.indent() << "})";
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    out << "{" << endl;
    out.indent_up();
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out.indent();
      render_const_value(out, ktype, v_iter->first) << " => ";
      render_const_value(out, vtype, v_iter->second) << "," << endl;
    }
    out.indent_down();
    out.indent() << "}";
  } else if (type->is_list() || type->is_set()) {
    t_type* etype;
    if (type->is_list()) {
      etype = ((t_list*)type)->get_elem_type();
    } else {
      etype = ((t_set*)type)->get_elem_type();
    }
    if (type->is_set()) {
      out << "Set.new([" << endl;
    } else {
      out << "[" << endl;
    }
    out.indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out.indent();
      render_const_value(out, etype, *v_iter) << "," << endl;
    }
    out.indent_down();
    if (type->is_set()) {
      out.indent() << "])";
    } else {
      out.indent() << "]";
    }
  } else {
    throw "CANNOT GENERATE CONSTANT FOR TYPE: " + type->get_name();
  }
  return out;
}

void t_ruby_generator::generate_struct(t_struct* tstruct) {
    generate_rb_struct(f_types_, tstruct);
}

void t_ruby_generator::generate_initialize(
    t_ruby_ofstream& out,
    t_struct* tstruct) {
  out.indent() << "def initialize(d=nil, &block)" << endl;
  out.indent_up();
  if (tstruct->is_union()) {
    out.indent() << "@my_type = ::Sparsam::Native::UNION" << endl;
  } else {
    out.indent() << "@my_type = ::Sparsam::Native::STRUCT" << endl;
  }
  out.indent() << "@thrift_field_map = {}" << endl;
  out.indent() << "super" << endl;
  out.indent_down();
  out.indent() << "end" << endl;
}
// Generates structs AND unions
void t_ruby_generator::generate_rb_struct(
    t_ruby_ofstream& out,
    t_struct* tstruct) {
  generate_rdoc(out, tstruct);
  if (tstruct->is_union()) {
    out.indent() << "class " << type_name(tstruct)   << " < ::Sparsam::Union" << endl;
    out.indent_up();
  } else if (tstruct->is_xception()) {
    out.indent() << "class " << type_name(tstruct)   << " < ::Sparsam::ApplicationException" << endl;
    out.indent_up();
  } else {
    out.indent() << "class " << type_name(tstruct)   << " < ::Sparsam::Struct" << endl;
    out.indent_up();
  }

  // These are now handled by meta programming in ruby
  //generate_initialize(out, tstruct);
  generate_field_defns(out, tstruct);
  //generate_field_constants(out, tstruct);
  //generate_field_accessors(out, tstruct);
  //generate_reader(out, tstruct);
  //generate_writer(out, tstruct);
  out.indent() << "init_thrift_struct(self)" << endl;

  out.indent_down();
  out.indent() << "end" << endl << endl;
}

void t_ruby_generator::generate_field_accessors(
    t_ruby_ofstream& out,
    t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    out.indent() << "def " << (*f_iter)->get_name() << endl;
    out.indent_up();
    if (tstruct->is_union()) {
      out.indent() << "if :" << (*f_iter)->get_name() << " == @setfield" << endl;
      out.indent_up();
      out.indent() << "@thrift_field_map[" << upcase_string((*f_iter)->get_name()) << "]" << endl;
      out.indent_down();
      out.indent() << "else" << endl;
      out.indent_up();
      out.indent() << "Raise RuntimeError, \"" << (*f_iter)->get_name() << " is not set in this union\"" << endl;
      out.indent_down();
      out.indent() << "end" << endl;
    } else {
      out.indent() << "thrift_field_map[" << upcase_string((*f_iter)->get_name()) << "]" << endl;
    }
    out.indent_down();
    out.indent() << "end" << endl;
    out.indent() << "def " << (*f_iter)->get_name() << "=(value)" << endl;
    out.indent_up();
    if (tstruct->is_union()) {
      out.indent() << "if @setfield == nil || :"<< (*f_iter)->get_name() << " == @setfield" << endl;
      out.indent_up();
      out.indent() << "@setfield = :" << (*f_iter)->get_name() << endl;
      out.indent() << "thrift_field_map[" << upcase_string((*f_iter)->get_name()) << "] = value" << endl;
      out.indent_down();
      out.indent() << "else" << endl;
      out.indent_up();
      out.indent() << "Raise RuntimeError, \"" << (*f_iter)->get_name() << " is not set in this union\"" << endl;
      out.indent_down();
      out.indent() << "end" << endl;
    } else {
      out.indent() << "thrift_field_map[" << upcase_string((*f_iter)->get_name()) << "] = value" << endl;
    }
    out.indent_down();
    out.indent() << "end" << endl;
  }
}

void t_ruby_generator::generate_field_constants(t_ruby_ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    std::string field_name = (*f_iter)->get_name();
    std::string cap_field_name = upcase_string(field_name);

    out.indent() << cap_field_name << " = " << (*f_iter)->get_key() << endl;
  }
  out << endl;
}

void t_ruby_generator::generate_reader(t_ruby_ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  out.indent() << "def self.read(sparsam)" << endl;
  out.indent_up();
  out.indent() << "obj = " << tstruct->get_name() << ".new" << endl;
  out.indent() << "sparsam.read_StructBegin()" << endl;
  out.indent() << "field_storage = [0,0]" << endl;

  //start while loop
  out.indent() << "while true do" << endl;
  out.indent_up();

  out.indent() << "field_storage = sparsam.read_FieldBegin(field_storage)" << endl;
  out.indent() << "if field_storage[0] == ::Sparsam::Types::STOP then" << endl;
  out.indent_up();
  out.indent() << "break" << endl;
  out.indent_down();
  out.indent() << "end" << endl;
  out.indent() << "case field_storage[1]" << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    string up_field_name = upcase_string((*f_iter)->get_name());
    out.indent() << "when " << up_field_name  << endl;
    out.indent_up();
    string to_field = "obj.thrift_field_map[" + up_field_name + "]";
    if (tstruct->is_union()) {
      out.indent() << "obj.setfield = :" << (*f_iter)->get_name() << endl;
    }
    generate_deserialize_field(out, to_field, (*f_iter)->get_type());
    out.indent_down();
  }
  out.indent() << "end" << endl;
  out.indent() << "sparsam.read_FieldEnd()" << endl;

  // end while loop
  out.indent_down();
  out.indent() << "end" << endl;
  out.indent() << "sparsam.read_StructEnd()" << endl;
  out.indent() << "return obj" << endl;
  out.indent_down();
  out.indent() << "end" << endl << endl;
}

void t_ruby_generator::generate_deserialize_field(
    t_ruby_ofstream& out,
    const string& to_field,
    t_type* f_type) {
  t_type* type = get_true_type(f_type);
  if (type->is_struct() || type->is_xception()) {
    out.indent() << to_field << " = " << f_type->get_name() << ".read(sparsam)";
  } else if (type->is_container()) {
    generate_deserialize_container(out, to_field, type);
  } else if (type->is_base_type() || type->is_enum()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw "compiler error: cannot serialize void field in a struct";
        break;
      case t_base_type::TYPE_STRING:
        if (((t_base_type*)type)->is_binary()) {
          out.indent() << to_field  << " = sparsam.read_Binary()";
        } else {
          out.indent() << to_field  << " = sparsam.read_String()";
        }
        break;
      case t_base_type::TYPE_BOOL:
        out.indent() << to_field  << " = sparsam.read_Bool()";
        break;
      case t_base_type::TYPE_BYTE:
        out.indent() << to_field  << " = sparsam.read_Byte()";
        break;
      case t_base_type::TYPE_I16:
        out.indent() << to_field  << " = sparsam.read_I16()";
        break;
      case t_base_type::TYPE_I32:
        out.indent() << to_field  << " = sparsam.read_I32()";
        break;
      case t_base_type::TYPE_I64:
        out.indent() << to_field  << " = sparsam.read_I64()";
        break;
      case t_base_type::TYPE_DOUBLE:
        out.indent() << to_field  << " = sparsam.read_Double()";
        break;
      default:
        throw "compiler error: no Ruby writer for base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    out.indent() << to_field << " = sparsam.read_I32()";
  } else {
    printf("DO NOT KNOW HOW TO DESERIALIZE FIELD of TYPE '%s'\n",
        type_name(type).c_str());
  }
  out << endl;
}

void t_ruby_generator::generate_deserialize_container(
    t_ruby_ofstream& out,
    const string& to_field,
    t_type* ttype)
{
  if (ttype->is_map()) {
    out.indent() << "ktype, vtype, num = sparsam.read_MapBegin()" << endl;
    out.indent() << "container = {}" << endl;
  } else if (ttype->is_set()) {
    out.indent() << "type, num = sparsam.read_SetBegin()" << endl;
    out.indent() << "container = Set.new" << endl;
  } else if (ttype->is_list()) {
    out.indent() << "type, num = sparsam.read_ListBegin()" << endl;
    out.indent() << "container = []" << endl;
  }

  out.indent() << "for i in (0..(num-1))" << endl;
  out.indent_up();
  if (ttype->is_map()) {
    generate_deserialize_field(out, "k", ((t_map*)ttype)->get_key_type());
    generate_deserialize_field(out, "v", ((t_map*)ttype)->get_val_type());
    out.indent() << "container[k] = v" << endl;
  } else if (ttype->is_set()) {
    generate_deserialize_field(out, "item", ((t_set*)ttype)->get_elem_type());
    out.indent() << "container.add(item)" << endl;
  } else if (ttype->is_list()) {
    generate_deserialize_field(out, "item", ((t_list*)ttype)->get_elem_type());
    out.indent() << "container.push(item)" << endl;
  }
  out.indent_down();
  out.indent() << "end" << endl;
  if (ttype->is_map()) {
    out.indent() << "sparsam.read_MapEnd()" << endl;
  } else if (ttype->is_set()) {
    out.indent() << "sparsam.read_SetEnd()" << endl;
  } else if (ttype->is_list()) {
    out.indent() << "sparsam.read_ListEnd()" << endl;
  }
  out.indent() << to_field << " = container";
}

void t_ruby_generator::generate_serialize_field(t_ruby_ofstream& out,
                                                t_field* tfield) {
  string name = tfield->get_name();
  string tmap_name = "@thrift_field_map[" + upcase_string(name) + "]";
  t_type* type = get_true_type(tfield->get_type());

  out.indent() << "sparsam.write_FieldBegin(" << type_to_enum(type) <<  ", " << tfield->get_key() << ")" << endl;
  _generate_serialize_field(out, type, tmap_name);
  out.indent() << "sparsam.write_FieldEnd()" << endl;
}

void t_ruby_generator::_generate_serialize_field(t_ruby_ofstream& out,
                                                 t_type* type,
                                                 const string& name) {

  // Do nothing for void types
  if (type->is_void()) {
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE: " + name;
  }


  if (type->is_struct() || type->is_xception()) {
    generate_serialize_struct(out, (t_struct*)type, name);
  } else if (type->is_container()) {
    generate_serialize_container(out, type, name);
  } else if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw "compiler error: cannot serialize void field in a struct: " + name;
        break;
      case t_base_type::TYPE_STRING:
        if (((t_base_type*)type)->is_binary()) {
          out.indent() << "sparsam.write_Binary(" << name << ")";
        } else {
          out.indent() << "sparsam.write_String(" << name << ")";
        }
        break;
      case t_base_type::TYPE_BOOL:
        out.indent() << "sparsam.write_Bool(" << name << ")";
        break;
      case t_base_type::TYPE_BYTE:
        out.indent() << "sparsam.write_Byte(" << name << ")";
        break;
      case t_base_type::TYPE_I16:
        out.indent() << "sparsam.write_I16(" << name << ")";
        break;
      case t_base_type::TYPE_I32:
        out.indent() << "sparsam.write_I32(" << name << ")";
        break;
      case t_base_type::TYPE_I64:
        out.indent() << "sparsam.write_I64(" << name << ")";
        break;
      case t_base_type::TYPE_DOUBLE:
        out.indent() << "sparsam.write_Double(" << name << ")";
        break;
      default:
        throw "compiler error: no Ruby writer for base type " + t_base_type::t_base_name(tbase)
          + name;
    }
    out << endl;
  } else if (type->is_enum()) {
    out.indent() << "sparsam.write_I32(" << name << ")";
    out << endl;
  } else {
    printf("DO NOT KNOW HOW TO SERIALIZE FIELD '%s' TYPE '%s'\n",
      name.c_str(),
      type_name(type).c_str());
  }
}

void t_ruby_generator::generate_serialize_struct(t_ruby_ofstream& out,
                                                t_struct* tstruct,
                                                string name) {
    out.indent() << name << ".write(sparsam)" << endl;
}

void t_ruby_generator::generate_serialize_container(
    t_ruby_ofstream& out,
    t_type* ttype,
    string name) {

  if (ttype->is_map()) {
    out.indent() << "sparsam.write_MapBegin(" << type_to_enum(((t_map*)ttype)->get_key_type())
                << ", " << type_to_enum(((t_map*)ttype)->get_val_type()) << ", "
                << name << ".length)" << endl;
  } else if (ttype->is_set()) {
    out.indent() << "sparsam.write_SetBegin(" << type_to_enum(((t_set*)ttype)->get_elem_type())
                << ", "
                << name << ".length)" << endl;
  } else if (ttype->is_list()) {
    out.indent() << "sparsam.write_ListBegin(" << type_to_enum(((t_set*)ttype)->get_elem_type())
                << ", "
                << name << ".length)" << endl;
  }

  if (ttype->is_map()) {
    out.indent() << name << ".each { |key, value|" << endl;
    out.indent_up();
    generate_serialize_map_element(out, (t_map*)ttype);
    out.indent_down();
    out.indent() << "}" << endl;
  } else if (ttype->is_set() || ttype->is_list()) {
    out.indent() << name << ".each { |item|" << endl;
    out.indent_up();
    _generate_serialize_field(out, ((t_set*)ttype)->get_elem_type(), "item");
    out.indent_down();
    out.indent() << "}" << endl;
  }

  if (ttype->is_map()) {
    out.indent() << "sparsam.write_MapEnd()" << endl;
  } else if (ttype->is_set()) {
    out.indent() << "sparsam.write_SetEnd()" << endl;
  } else if (ttype->is_list()) {
    out.indent() << "sparsam.write_ListEnd()" << endl;
  }
}

/**
 * Serializes the members of a map.
 *
 */
void t_ruby_generator::generate_serialize_map_element(t_ruby_ofstream& out, t_map* tmap) {
  _generate_serialize_field(out, tmap->get_key_type(), "key");
  _generate_serialize_field(out, tmap->get_val_type(), "value");
}

void t_ruby_generator::generate_writer(t_ruby_ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  out.indent() << "def write(sparsam)" << endl;
  out.indent_up();
  out.indent() << "sparsam.write_StructBegin()" << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    string up_field_name = upcase_string((*f_iter)->get_name());
    out.indent() << "if @thrift_field_map.has_key?(" << up_field_name << ") then" << endl;
    out.indent_up();
    generate_serialize_field(out, *f_iter);
    out.indent_down();
    out.indent() << "end" << endl;
  }
  out.indent() << "sparsam.write_FieldStop()" << endl;
  out.indent() << "sparsam.write_StructEnd()" << endl;
  out.indent_down();
  out.indent() << "end" << endl << endl;
}

void t_ruby_generator::generate_field_defns(t_ruby_ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  out.indent() << "FIELDS = {" << endl;
  out.indent_up();
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (f_iter != fields.begin()) {
      out << "," << endl;
    }

    // generate the field docstrings within the FIELDS constant. no real better place...
    generate_rdoc(out, *f_iter);

    out.indent() << (*f_iter)->get_key() << " => ";

    generate_field_data(out,
                        (*f_iter)->get_type(),
                        (*f_iter)->get_name(),
                        (*f_iter)->get_value(),
                        (*f_iter)->get_req() == t_field::T_OPTIONAL);
  }
  out.indent_down();
  out << endl;
  out.indent() << "}" << endl << endl;
}

void t_ruby_generator::generate_field_data(
    t_ruby_ofstream& out,
    t_type* field_type,
    const std::string& field_name = "",
    t_const_value* field_value = NULL,
    bool optional = false) {
  field_type = get_true_type(field_type);

  // Begin this field's defn
  out << "{:type => " << type_to_enum(field_type);

  if (!field_name.empty()) {
    out << ", :name => '" << field_name << "'";
  }

  if (field_value != NULL) {
    out << ", :default => ";
    render_const_value(out, field_type, field_value);
  }

  if (!field_type->is_base_type()) {
    if (field_type->is_struct() || field_type->is_xception()) {
      out << ", :class => " << full_type_name((t_struct*)field_type);
    } else if (field_type->is_list()) {
      out << ", :element => ";
      generate_field_data(out, ((t_list*)field_type)->get_elem_type());
    } else if (field_type->is_map()) {
      out << ", :key => ";
      generate_field_data(out, ((t_map*)field_type)->get_key_type());
      out << ", :value => ";
      generate_field_data(out, ((t_map*)field_type)->get_val_type());
    } else if (field_type->is_set()) {
      out << ", :element => ";
      generate_field_data(out, ((t_set*)field_type)->get_elem_type());
    }
  } else {
    if (((t_base_type*)field_type)->is_binary()) {
      out << ", :binary => true";
    }
  }

  if (optional) {
    out << ", :optional => true";
  }

  if (field_type->is_enum()) {
    out << ", :enum_class => " << full_type_name(field_type);
  }

  // End of this field's defn
  out << "}";
}

void t_ruby_generator::begin_namespace(t_ruby_ofstream& out, vector<std::string> modules) {
  for (vector<std::string>::iterator m_iter = modules.begin(); m_iter != modules.end(); ++m_iter) {
    out.indent() << "module " << *m_iter << endl;
    out.indent_up();
  }
}

void t_ruby_generator::end_namespace(t_ruby_ofstream& out, vector<std::string> modules) {
  for (vector<std::string>::reverse_iterator m_iter = modules.rbegin(); m_iter != modules.rend();
       ++m_iter) {
    out.indent_down();
    out.indent() << "end" << endl;
  }
}

string t_ruby_generator::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "NO T_VOID CONSTRUCT";
    case t_base_type::TYPE_STRING:
      return "::Sparsam::Types::STRING";
    case t_base_type::TYPE_BOOL:
      return "::Sparsam::Types::BOOL";
    case t_base_type::TYPE_BYTE:
      return "::Sparsam::Types::BYTE";
    case t_base_type::TYPE_I16:
      return "::Sparsam::Types::I16";
    case t_base_type::TYPE_I32:
      return "::Sparsam::Types::I32";
    case t_base_type::TYPE_I64:
      return "::Sparsam::Types::I64";
    case t_base_type::TYPE_DOUBLE:
      return "::Sparsam::Types::DOUBLE";
    }
  } else if (type->is_enum()) {
    return "::Sparsam::Types::I32";
  } else if (type->is_struct() || type->is_xception()) {
    return "::Sparsam::Types::STRUCT";
  } else if (type->is_map()) {
    return "::Sparsam::Types::MAP";
  } else if (type->is_set()) {
    return "::Sparsam::Types::SET";
  } else if (type->is_list()) {
    return "::Sparsam::Types::LIST";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

void t_ruby_generator::generate_rdoc(t_ruby_ofstream& out, t_doc* tdoc) {
  if (tdoc->has_doc()) {
    out.indent();
    generate_docstring_comment(out, "", "# ", tdoc->get_doc(), "");
  }
}

string t_ruby_generator::full_type_name(t_type* ttype) {
  string prefix = "::";
  vector<std::string> modules = ruby_modules(ttype->get_program());
  for (vector<std::string>::iterator m_iter = modules.begin(); m_iter != modules.end(); ++m_iter) {
    prefix += *m_iter + "::";
  }
  return prefix + type_name(ttype);
}

string t_ruby_generator::render_includes() {
  const vector<t_program*>& includes = program_->get_includes();
  string result = "";
  for (size_t i = 0; i < includes.size(); ++i) {
    if (namespaced_) {
      t_program* included = includes[i];
      std::string included_require_prefix
          = rb_namespace_to_path_prefix(included->get_namespace("rb"));
      std::string included_name = included->get_name();
      result += "require '" + included_require_prefix + underscore(included_name) + "_types'\n";
    } else {
      result += "require '" + underscore(includes[i]->get_name()) + "_types'\n";
    }
  }
  if (includes.size() > 0) {
    result += "\n";
  }
  return result;
}

string t_ruby_generator::rb_namespace_to_path_prefix(string rb_namespace) {
  string namespaces_left = rb_namespace;
  string::size_type loc;

  string path_prefix = "";

  while ((loc = namespaces_left.find(".")) != string::npos) {
    path_prefix = path_prefix + underscore(namespaces_left.substr(0, loc)) + "/";
    namespaces_left = namespaces_left.substr(loc + 1);
  }
  if (namespaces_left.size() > 0) {
    path_prefix = path_prefix + underscore(namespaces_left) + "/";
  }
  return path_prefix;
}

string t_ruby_generator::type_name(t_type* ttype) {
  string prefix = "";

  string name = ttype->get_name();
  if (ttype->is_struct() || ttype->is_xception() || ttype->is_enum()) {
    name = capitalize(ttype->get_name());
  }

  return prefix + name;
}

THRIFT_REGISTER_GENERATOR(
    ruby,
    "Ruby Sparsam bindings",
    "    namespaced:      Generate files in idiomatic namespaced directories.\n")
