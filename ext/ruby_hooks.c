#include <ruby.h>
#include <ruby/intern.h>
#include "serializer.h"
#include "stdio.h"

VALUE Sparsam = Qnil;
VALUE static_zero_array;
VALUE SparsamNativeError = Qnil;

ID intern_for_DEFAULT_VALUES;
ID intern_for_assign_defaults;
ID intern_for_assign_from_arg;

static void deallocate(void* data) { serializer_free(data); }

static VALUE allocate(VALUE klass) {
  void* data = serializer_create();
  return Data_Wrap_Struct(klass, NULL, deallocate, data);
}

static VALUE sparsam_init_bang(VALUE self) {
  initialize_runtime_constants();
  return self;
}

static VALUE initialize(VALUE self, VALUE type_arg, VALUE str_arg) {
  void* self_data = NULL;
  void* input_string = NULL;

  Check_Type(type_arg, T_FIXNUM);
  int prot = NUM2INT(type_arg);

  Check_Type(str_arg, T_STRING);

  uint32_t len = RSTRING_LEN(str_arg);
  if (len != 0) {
    input_string = calloc(len, sizeof(char));
    memcpy(input_string, StringValuePtr(str_arg), len);
  }

  Data_Get_Struct(self, void*, self_data);
  serializer_init(self_data, prot, input_string, len);

  return self;
}

VALUE sparsam_struct_initialize(int argc, VALUE* argv, VALUE self) {
  if (argc > 1) {
    rb_raise(rb_eArgError,
             "wrong number of arguments (given %d, expected 0..1)", argc);
  }

  VALUE defaults = rb_const_get(rb_obj_class(self), intern_for_DEFAULT_VALUES);

  if (defaults != Qnil) {
    rb_funcall(self, intern_for_assign_defaults, 1, defaults);
  }

  if (argc == 1) {
    rb_funcall(self, intern_for_assign_from_arg, 1, argv[0]);
  }

  if (rb_block_given_p()) {
    rb_yield(self);
  }

  return Qnil;
}

void Init_sparsam_native() {
  Sparsam = rb_define_module("Sparsam");
  rb_define_singleton_method(Sparsam, "init!", sparsam_init_bang, 0);
  rb_define_singleton_method(Sparsam, "cache_fields", cache_fields, 1);
  VALUE SparsamSerializer =
      rb_define_class_under(Sparsam, "Serializer", rb_cObject);
  SparsamNativeError =
      rb_define_class_under(Sparsam, "Exception", rb_eStandardError);
  rb_define_alloc_func(SparsamSerializer, allocate);
  rb_define_method(SparsamSerializer, "initialize", initialize, 2);
  rb_define_method(SparsamSerializer, "serialize", serializer_writeStruct, 2);
  rb_define_method(SparsamSerializer, "deserialize", serializer_readStruct, 1);
  rb_define_method(SparsamSerializer, "deserializeUnion", serializer_readUnion,
                   1);
  rb_define_const(Sparsam, "CompactProtocol", INT2FIX(compact));
  rb_define_const(Sparsam, "BinaryProtocol", INT2FIX(binary));
  rb_define_const(Sparsam, "UNION", INT2FIX(t_union));
  rb_define_const(Sparsam, "STRUCT", INT2FIX(t_struct));
  rb_define_const(Sparsam, "NORMAL", INT2FIX(normal));
  rb_define_const(Sparsam, "STRICT", INT2FIX(strict));
  rb_define_const(Sparsam, "RECURSIVE", INT2FIX(recursive));

  intern_for_DEFAULT_VALUES = rb_intern("DEFAULT_VALUES");
  intern_for_assign_defaults = rb_intern("assign_defaults");
  intern_for_assign_from_arg = rb_intern("assign_from_arg");

  VALUE SparsamStructInitialization =
      rb_define_module_under(Sparsam, "StructInitialization");
  rb_define_method(SparsamStructInitialization, "initialize",
                   sparsam_struct_initialize, -1);
  initialize_constants();
}
