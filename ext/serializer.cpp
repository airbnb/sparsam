#include "serializer.h"
#include <functional>
#include <map>
#include <ruby/encoding.h>
#include <stdio.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <boost/make_shared.hpp>
#include <vector>

using namespace std;
using ::apache::thrift::protocol::TType;
using ::apache::thrift::transport::TMemoryBuffer;
using namespace ::apache::thrift;

VALUE sym_for_class;
VALUE sym_for_binary;
VALUE sym_for_type;
VALUE sym_for_element;
VALUE sym_for_key;
VALUE sym_for_value;
VALUE sym_for_optional;
VALUE sym_for_name;
VALUE intern_for_FIELDS;
VALUE intern_for_new;
VALUE intern_for_keys;
VALUE intern_for_values;
ID intern_for_setfield_ivar;
VALUE intern_for_to_a;
VALUE klass_for_union;
VALUE klass_for_integer;
VALUE klass_for_float;
VALUE klass_for_string;
VALUE klass_for_hash;
VALUE klass_for_set;
VALUE klass_for_array;

extern VALUE Sparsam;
extern VALUE SparsamNativeError;
VALUE SparsamTypeMismatchError;
VALUE SparsamMissingMandatory;
VALUE SparsamUnionException;
VALUE SparsamUnknownTypeException;

VALUE SetClass;

KlassFieldsCache klassCache; // consider the memory leaked.
std::unordered_set<VALUE> unions;

void *serializer_create() { return (void *)(new ThriftSerializer()); }

void serializer_free(void *data) {
  ThriftSerializer *ts = (ThriftSerializer *)(data);
  delete ts;
}

void initialize_constants() {
  sym_for_class = ID2SYM(rb_intern("class"));
  sym_for_binary = ID2SYM(rb_intern("binary"));
  sym_for_type = ID2SYM(rb_intern("type"));
  sym_for_element = ID2SYM(rb_intern("element"));
  sym_for_key = ID2SYM(rb_intern("key"));
  sym_for_value = ID2SYM(rb_intern("value"));
  sym_for_optional = ID2SYM(rb_intern("optional"));
  sym_for_name = ID2SYM(rb_intern("name"));
  intern_for_FIELDS = rb_intern("FIELDS");
  intern_for_new = rb_intern("new");
  intern_for_keys = rb_intern("keys");
  intern_for_values = rb_intern("values");
  intern_for_to_a = rb_intern("to_a");
  intern_for_setfield_ivar = rb_intern("@setfield");
  klass_for_set = rb_const_get_at(rb_cObject, rb_intern("Set"));
  klass_for_integer = rb_const_get_at(rb_cObject, rb_intern("Integer"));
  klass_for_float = rb_const_get_at(rb_cObject, rb_intern("Float"));
  klass_for_string = rb_const_get_at(rb_cObject, rb_intern("String"));
  klass_for_hash = rb_const_get_at(rb_cObject, rb_intern("Hash"));
  klass_for_array = rb_const_get_at(rb_cObject, rb_intern("Array"));
}

void initialize_runtime_constants() {
  klass_for_union = rb_const_get_at(Sparsam, rb_intern("Union"));
  SparsamMissingMandatory = rb_const_get_at(Sparsam, rb_intern("MissingMandatory"));
  SparsamTypeMismatchError = rb_const_get_at(Sparsam, rb_intern("TypeMismatch"));
  SparsamUnionException = rb_const_get_at(Sparsam, rb_intern("UnionException"));
  SparsamUnknownTypeException =
      rb_const_get_at(Sparsam, rb_intern("UnknownTypeException"));
  SetClass = rb_const_get_at(rb_cObject, rb_intern("Set"));
}

void serializer_init(void *serializer, int protocol, void *str_arg1,
                     uint32_t len) {
  using ::boost::shared_ptr;
  using ::apache::thrift::protocol::TProtocol;
  using ::apache::thrift::protocol::TBinaryProtocol;
  using ::apache::thrift::protocol::TCompactProtocol;

  ThriftSerializer *ts = (ThriftSerializer *)(serializer);
  shared_ptr<TMemoryBuffer> strBuffer;
  if (str_arg1 != NULL) {
    strBuffer = boost::make_shared<TMemoryBuffer>(
      (uint8_t *)str_arg1,
      len,
      TMemoryBuffer::TAKE_OWNERSHIP
    );
  } else {
    strBuffer = boost::make_shared<TMemoryBuffer>();
  }
  Proto proto = static_cast<Proto>(protocol);
  if (proto == compact) {
    ts->tprot = shared_ptr<TProtocol>(new TCompactProtocol(strBuffer));
  } else if (proto == binary) {
    ts->tprot = shared_ptr<TProtocol>(new TBinaryProtocol(strBuffer));
  } else {
    rb_raise(SparsamNativeError, "Unknown protocol %d", proto);
  }
  ts->tmb = strBuffer;
}

#define get_ts()                                                               \
  void *self_data = NULL;                                                      \
  Data_Get_Struct(self, void, self_data);                                      \
  ThriftSerializer *ts = (ThriftSerializer *)(self_data);

#define watch_for_texcept() try {

#define catch_thrift_and_reraise()                                             \
  }                                                                            \
  catch (::apache::thrift::TException e) {                                     \
    rb_raise(SparsamNativeError, "%s", e.what());                              \
    return Qnil;                                                               \
  }


static inline VALUE make_ruby_string(const string &val) {
  return rb_enc_str_new(val.c_str(), val.size(), rb_utf8_encoding());
}

static inline VALUE make_ruby_binary(const string &val) {
  return rb_str_new(val.c_str(), val.size());
}

static void raise_exc_with_struct_and_field_names(
        VALUE exc_class,
        VALUE msg_prefix,
        VALUE outer_struct_class,
        VALUE field_sym) {
  VALUE struct_name = rb_class_name(outer_struct_class);
  VALUE field_name = rb_sym_to_s(field_sym);

  VALUE msg = rb_sprintf("%s (in %s#%s)",
      RSTRING_PTR(msg_prefix),
      RSTRING_PTR(struct_name),
      RSTRING_PTR(field_name));
  VALUE args[3] = {msg, struct_name, field_name};
  VALUE e = rb_class_new_instance(3, args, exc_class);
  rb_exc_raise(e);
}

static void raise_type_mismatch(VALUE outer_struct, VALUE field_sym) {
    raise_exc_with_struct_and_field_names(
      SparsamTypeMismatchError,
      rb_str_new2("Mismatched type"),
      CLASS_OF(outer_struct),
      field_sym);
}

static inline long raise_type_mismatch_as_value(VALUE outer_struct, VALUE field_sym) {
  raise_type_mismatch(outer_struct, field_sym);
  return 0;
}

static inline void Sparsam_Check_Type(VALUE x, int t, VALUE outer_struct, VALUE field_sym) {
  if (!(RB_TYPE_P(x, t))) {
    raise_type_mismatch(outer_struct, field_sym);
  }
}


static inline VALUE make_ruby_bool(bool val) { return val ? Qtrue : Qfalse; }

void ThriftSerializer::skip_n_type(uint32_t n, TType ttype) {
  for (uint32_t i = 0; i < n; i++) {
    this->tprot->skip(ttype);
  }
}

void ThriftSerializer::skip_n_pair(uint32_t n, TType type_a, TType type_b) {
  for (uint32_t i = 0; i < n; i++) {
    this->tprot->skip(type_a);
    this->tprot->skip(type_b);
  }
}

// Blatantly copied protobuf's design
// https://git.io/vHuUn
// CONVERT is new here, because we're targeting ruby
#define HANDLE_TYPE(TYPE, CPPTYPE, READ_METHOD, CONVERT)                       \
  case protocol::T_##TYPE: {                                                   \
    CPPTYPE value;                                                             \
    this->tprot->read##READ_METHOD(value);                                     \
    ret = CONVERT(value);                                                      \
    break;                                                                     \
  }

VALUE ThriftSerializer::readAny(TType ttype, FieldInfo *field_info) {
  VALUE ret = Qnil;
  switch (ttype) {

    // Handle all the non-container types by marco
    HANDLE_TYPE(I16, int16_t, I16, INT2FIX)
    HANDLE_TYPE(I32, int32_t, I32, INT2FIX)
    HANDLE_TYPE(I64, int64_t, I64, LL2NUM)
    HANDLE_TYPE(BOOL, bool, Bool, make_ruby_bool)
    HANDLE_TYPE(DOUBLE, double, Double, DBL2NUM)
    HANDLE_TYPE(BYTE, int8_t, Byte, INT2FIX)

  case protocol::T_STRING: {
    string value;
    if (field_info->isBinaryString) { // if (field_info[:binary])
      this->tprot->readBinary(value);
      ret = make_ruby_binary(value);
    } else {
      this->tprot->readString(value);
      ret = make_ruby_string(value);
    }
    break;
  }

  case protocol::T_LIST: {
    TType element_type;
    uint32_t size;

    this->tprot->readListBegin(element_type, size);
    if (field_info->elementType == NULL ||
        element_type != field_info->elementType->ftype) {
      this->skip_n_type(size, element_type);
      break;
    }
    ret = rb_ary_new2(size);

    for (uint32_t i = 0; i < size; i++) {
      rb_ary_store(ret, i,
                   this->readAny(element_type, field_info->elementType));
    }
    this->tprot->readListEnd();

    break;
  }

  case protocol::T_SET: {
    TType element_type;
    uint32_t size;

    this->tprot->readSetBegin(element_type, size);
    if (field_info->elementType == NULL ||
        element_type != field_info->elementType->ftype) {
      this->skip_n_type(size, element_type);
      break;
    }
    VALUE ary = rb_ary_new2(size);

    for (uint32_t i = 0; i < size; i++) {
      rb_ary_store(ary, i,
                   this->readAny(element_type, field_info->elementType));
    }
    ret = rb_class_new_instance(1, &ary, klass_for_set);
    this->tprot->readSetEnd();
    break;
  }

  case protocol::T_STRUCT: {
    string cname;
    this->tprot->readStructBegin(cname);
    if (unions.count(field_info->klass) == 1) {
      ret = this->readUnion(field_info->klass);
    } else {
      ret = this->readStruct(field_info->klass);
    }
    this->tprot->readStructEnd();
    break;
  }

  case protocol::T_MAP: {
    TType key_type, value_type;
    uint32_t size;
    VALUE k, v;

    this->tprot->readMapBegin(key_type, value_type, size);

    if (field_info->keyType == NULL ||
        field_info->elementType == NULL) { // no type check to be consistent
      skip_n_pair(size, key_type, value_type);
      break;
    }

    ret = rb_hash_new();
    for (uint32_t i = 0; i < size; i++) {
      k = this->readAny(key_type, field_info->keyType);
      v = this->readAny(value_type, field_info->elementType);
      rb_hash_aset(ret, k, v);
    }
    this->tprot->readMapEnd();
    break;
  }

  default:
    this->tprot->skip(ttype);
    rb_raise(SparsamUnknownTypeException, "Received unknown type with id: %d", ttype);
    break;
  }

  return ret;
}

#undef HANDLE_TYPE

VALUE ThriftSerializer::readStruct(VALUE klass) {

  string cname;
  FieldBegin fieldBegin;
  TType typeId;
  FieldInfo *fieldInfo;
  VALUE ret = rb_class_new_instance(0, NULL, klass); // ret = &klass.new
  auto fields = FindOrCreateFieldInfoMap(klass);

  while (true) {
    this->tprot->readFieldBegin(cname, fieldBegin.ftype, fieldBegin.fid);
    if (fieldBegin.ftype == protocol::T_STOP) {
      break;
    }
    auto iter = fields->find(fieldBegin.fid);

    if (iter == fields->end()) {
      this->tprot->skip(fieldBegin.ftype);
      this->tprot->readFieldEnd();
      continue;
    }

    fieldInfo = iter->second;

    typeId = fieldInfo->ftype;

    if (typeId != fieldBegin.ftype) {
      raise_exc_with_struct_and_field_names(
        SparsamTypeMismatchError,
        rb_sprintf("Mismatched type (definition: %d, found: %d)", fieldBegin.ftype, typeId),
        klass,
        fieldInfo->symName);
    }

    VALUE rb_value = this->readAny(fieldBegin.ftype, iter->second);
    if (!NIL_P(rb_value)) {
      rb_ivar_set(ret, fieldInfo->ivarName, rb_value);
    }

    this->tprot->readFieldEnd();
  }
  return ret;
}

VALUE ThriftSerializer::readUnion(VALUE klass) {
  string cname;
  FieldBegin fieldBegin;

  VALUE ret = rb_class_new_instance(0, NULL, klass); // ret = &klass.new
  auto fields = FindOrCreateFieldInfoMap(klass);

  VALUE key, rb_value;

  this->tprot->readFieldBegin(cname, fieldBegin.ftype, fieldBegin.fid);
  if (fieldBegin.ftype == protocol::T_STOP) {
    return ret;
  }

  auto iter = fields->find(fieldBegin.fid);

  if (iter == fields->end()) {
    this->tprot->skip(fieldBegin.ftype);
    this->tprot->readFieldEnd();
    return ret;
  }

  rb_value = this->readAny(fieldBegin.ftype, iter->second);
  if (!NIL_P(rb_value)) {
    rb_ivar_set(ret, intern_for_setfield_ivar, iter->second->symName);
    rb_ivar_set(ret, iter->second->ivarName, rb_value);
  }

  this->tprot->readFieldEnd();
  this->tprot->readFieldBegin(cname, fieldBegin.ftype, fieldBegin.fid);

  if (fieldBegin.ftype != protocol::T_STOP) {
    rb_raise(SparsamUnionException, "More than one element in union.");
    return Qnil;
  }

  return ret;
}

// for the unary `+` before lambda:
// https://stackoverflow.com/a/18889029/4944625
// explicit cast to work with signature: (int (*)(...))
#define HASH_FOREACH_BEGIN(hash, ...)                                          \
  void *_args[] = {__VA_ARGS__};                                               \
  rb_hash_foreach(hash, (int (*)(ANYARGS))(+[](VALUE k, VALUE v, VALUE args) { \
    void **argv = (void **) args;

#define HASH_FOREACH_RET() return (int)ST_CONTINUE;

#define HASH_FOREACH_ABORT() return (int)ST_STOP;

#define HASH_FOREACH_END()                                                     \
  HASH_FOREACH_RET()                                                           \
  }), (VALUE) _args);

#define HANDLE_TYPE(TYPE, WRITE_METHOD, CONVERT)                               \
  case protocol::T_##TYPE: {                                                   \
    this->tprot->write##WRITE_METHOD(CONVERT);                                 \
    break;                                                                     \
  }

static inline long raise_bignum_range_error_as_value() {
  rb_raise(rb_eRangeError, "bignum too big to convert");
  return 0;
}

#define CONVERT_FIXNUM(CONVERT)                                                \
  ((FIXNUM_P(actual)) ?                                                        \
    CONVERT(actual) :                                                          \
    ((RB_TYPE_P(actual, T_BIGNUM)) ?                                           \
      raise_bignum_range_error_as_value() :                                    \
      raise_type_mismatch_as_value(outer_struct, field_sym)))

#define CONVERT_I64                                                            \
  ((FIXNUM_P(actual)) ?                                                        \
    (LONG_LONG)FIX2LONG(actual) :                                              \
    ((RB_TYPE_P(actual, T_BIGNUM)) ?                                           \
      rb_big2ll(actual) :                                                      \
      raise_type_mismatch_as_value(outer_struct, field_sym)))

#ifdef RB_FLOAT_TYPE_P
#define FLOAT_TYPE_P(x) RB_FLOAT_TYPE_P(x)
#else
#define FLOAT_TYPE_P(x) RB_TYPE_P(x, T_FLOAT)
#endif

#define CONVERT_FLOAT(CONVERT)                                                 \
  ((FLOAT_TYPE_P(actual)) ?                                                    \
   CONVERT(actual) :                                                           \
   raise_type_mismatch_as_value(outer_struct, field_sym))                      \

static inline bool convertBool(VALUE actual, VALUE outer_struct, VALUE field_sym) {
  switch (actual) {
    case Qtrue:
      return true;
    case Qfalse:
      return false;
    default:
      raise_type_mismatch(outer_struct, field_sym);
  }

  /* unreachable */
  return false;
}

#ifdef FIX2SHORT
#define SHORT_CONVERT(x) FIX2SHORT(x)
#else
#define SHORT_CONVERT(x) ((short)FIX2INT(x))
#endif

static inline char byte_convert(VALUE x) {
  short s = SHORT_CONVERT(x);

  if (s <= 127 && s >= -128) {
    return (char) s;
  } else {
    rb_raise(rb_eRangeError, "integer %d out of range for char", s);
  }

  /* unreachable */
  return 0;
}

void ThriftSerializer::writeAny(TType ttype, FieldInfo *field_info,
                                VALUE actual, VALUE outer_struct, VALUE field_sym) {
  switch (ttype) {
    HANDLE_TYPE(I16, I16, CONVERT_FIXNUM(SHORT_CONVERT))
    HANDLE_TYPE(I32, I32, CONVERT_FIXNUM(FIX2INT))
    HANDLE_TYPE(I64, I64, CONVERT_I64)
    HANDLE_TYPE(BOOL, Bool, convertBool(actual, outer_struct, field_sym))
    HANDLE_TYPE(DOUBLE, Double, CONVERT_FLOAT(NUM2DBL))
    HANDLE_TYPE(BYTE, Byte, CONVERT_FIXNUM(byte_convert))

  case protocol::T_STRING: {
    Sparsam_Check_Type(actual, T_STRING, outer_struct, field_sym);

    string data = string(StringValuePtr(actual), RSTRING_LEN(actual));
    if (field_info->isBinaryString) {
      this->tprot->writeBinary(data);
    } else {
      this->tprot->writeString(data);
    }
    break;
  }

  case protocol::T_LIST: {
    Sparsam_Check_Type(actual, T_ARRAY, outer_struct, field_sym);

    long length = RARRAY_LEN(actual);
    TType elem = field_info->elementType->ftype;
    this->tprot->writeListBegin(elem, static_cast<size_t>(length));
    for (long i = 0; i < length; i++) {
      this->writeAny(elem, field_info->elementType, rb_ary_entry(actual, i), outer_struct, field_sym);
    }
    this->tprot->writeListEnd();
    break;
  }

  case protocol::T_SET: {
    if (CLASS_OF(actual) != SetClass) {
      raise_type_mismatch(outer_struct, field_sym);
    }

    VALUE ary = rb_funcall(actual, intern_for_to_a, 0);
    long length = RARRAY_LEN(ary);
    TType elem = field_info->elementType->ftype;
    this->tprot->writeListBegin(elem, static_cast<size_t>(length));
    for (long i = 0; i < length; i++) {
      this->writeAny(elem, field_info->elementType, rb_ary_entry(ary, i), outer_struct, field_sym);
    }
    this->tprot->writeListEnd();
    break;
  }

  case protocol::T_MAP: {
    Sparsam_Check_Type(actual, T_HASH, outer_struct, field_sym);

    TType keyTType = field_info->keyType->ftype,
          valueTType = field_info->elementType->ftype;
    this->tprot->writeMapBegin(keyTType, valueTType,
                               static_cast<size_t>(RHASH_SIZE(actual)));
    HASH_FOREACH_BEGIN(actual, this, field_info, &outer_struct, &field_sym)
    ThriftSerializer *that = (ThriftSerializer *)argv[0];
    FieldInfo *field_info = (FieldInfo *)argv[1];
    VALUE *outer_struct = (VALUE *)argv[2];
    VALUE *field_sym = (VALUE *)argv[3];
    that->writeAny(field_info->keyType->ftype, field_info->keyType, k, *outer_struct, *field_sym);
    that->writeAny(field_info->elementType->ftype, field_info->elementType, v, *outer_struct, *field_sym);
    HASH_FOREACH_END()
    this->tprot->writeMapEnd();
    break;
  }

  case protocol::T_STRUCT: {
    if (CLASS_OF(actual) != field_info->klass) {
      raise_type_mismatch(outer_struct, field_sym);
    }

    static const string cname = "";
    this->tprot->writeStructBegin(cname.c_str());
    this->writeStruct(field_info->klass, actual);
    this->tprot->writeFieldStop();
    this->tprot->writeStructEnd();
    break;
  }

  default: { break; }
  }
}

#undef HANDLE_TYPE

static bool checkRequiredFields(VALUE klass, VALUE data) {
  auto fields = FindOrCreateFieldInfoMap(klass);
  for (auto const &entry : *fields) {
    if (!entry.second->isOptional) {
      VALUE val = rb_ivar_get(data, entry.second->ivarName);
      if (NIL_P(val)) {
        raise_exc_with_struct_and_field_names(
          SparsamMissingMandatory,
          rb_str_new2("Required field missing"),
          klass,
          entry.second->symName);
        return false;
      }
    }
  }

  return true;
}

void ThriftSerializer::writeStruct(VALUE klass, VALUE data) {
  static const string cname = "";
  FieldBegin fieldBegin;
  FieldInfo *fieldInfo;
  auto fields = FindOrCreateFieldInfoMap(klass);

  if (!checkRequiredFields(klass, data)) {
    return;
  }

  for (auto const & entry : *fields) {
    fieldBegin.fid = entry.first;
    fieldInfo = entry.second;
    fieldBegin.ftype = fieldInfo->ftype;
    VALUE actual = rb_ivar_get(data, fieldInfo->ivarName);
    if (!NIL_P(actual)) {
      this->tprot->writeFieldBegin(cname.c_str(), fieldBegin.ftype, fieldBegin.fid);
      this->writeAny(fieldBegin.ftype, entry.second, actual, data, fieldInfo->symName);
      this->tprot->writeFieldEnd();
    }
  }
}

VALUE serializer_writeStruct(VALUE self, VALUE klass, VALUE data) {
  watch_for_texcept() get_ts();

  if (CLASS_OF(data) != klass) {
    VALUE expected_name = rb_class_name(klass);
    VALUE actual_name = rb_class_name(CLASS_OF(data));

    raise_exc_with_struct_and_field_names(
      SparsamTypeMismatchError,
      rb_sprintf(
        "Mismatched type passed to serialize (expected: %s got: %s)",
        RSTRING_PTR(expected_name),
        RSTRING_PTR(actual_name)),
      data,
      ID2SYM(rb_intern("(root)")));

    RB_GC_GUARD(expected_name);
    RB_GC_GUARD(actual_name);
  }

  static const string cname = "";
  ts->tprot->writeStructBegin(cname.c_str());
  ts->writeStruct(klass, data);
  ts->tprot->writeFieldStop();
  ts->tprot->writeStructEnd();
  string retval = ts->tmb->getBufferAsString();
  return rb_str_new(retval.c_str(), retval.size());
  catch_thrift_and_reraise();
}

VALUE serializer_readStruct(VALUE self, VALUE klass) {
  watch_for_texcept() get_ts();
  string cname;
  VALUE ret;
  ts->tprot->readStructBegin(cname);
  ret = ts->readStruct(klass);
  ts->tprot->readStructEnd();
  return ret;
  catch_thrift_and_reraise();
}

#define R_FIX_TO_TTYPE(x) (static_cast<TType>(FIX2INT(x)))

FieldInfoMap *FindOrCreateFieldInfoMap(VALUE klass) {
  auto iter = klassCache.find(klass);
  if (iter == klassCache.end()) {
    if (RTEST(rb_class_inherited_p(klass, klass_for_union))) {
      unions.insert(klass);
    }
    auto ret = CreateFieldInfoMap(klass);
    klassCache[klass] = ret;
    return ret;
  } else {
    return iter->second;
  }
}

ID field_name_to_ivar_id(VALUE str_name) {
  if (str_name != Qnil) {
    return rb_intern_str(rb_str_concat(rb_str_new2("@"), str_name));
  } else {
    return 0;
  }
}

VALUE field_name_to_sym(VALUE str_name) {
  if (str_name != Qnil) {
    return ID2SYM(rb_intern_str(str_name));
  } else {
    return 0;
  }
}

// each FieldInfoMap has multiple FieldInfos
FieldInfo *CreateFieldInfo(VALUE field_map_entry) {
  FieldInfo *fieldInfo = new FieldInfo();
  fieldInfo->ftype =
      R_FIX_TO_TTYPE(rb_hash_aref(field_map_entry, sym_for_type));
  fieldInfo->isOptional =
      RTEST(rb_hash_aref(field_map_entry, sym_for_optional));
  fieldInfo->ivarName = field_name_to_ivar_id(rb_hash_aref(field_map_entry, sym_for_name));
  fieldInfo->symName = field_name_to_sym(rb_hash_aref(field_map_entry, sym_for_name));
  switch (fieldInfo->ftype) {
  case protocol::T_STRING: {
    if (RTEST(rb_hash_aref(field_map_entry, sym_for_binary))) {
      fieldInfo->isBinaryString = true;
    }
    break;
  }
  case protocol::T_STRUCT: {
    fieldInfo->klass = rb_hash_aref(field_map_entry, sym_for_class);
    break;
  }
  case protocol::T_LIST:
  case protocol::T_SET: {
    fieldInfo->elementType =
        CreateFieldInfo(rb_hash_aref(field_map_entry, sym_for_element));
    break;
  }
  case protocol::T_MAP: {
    fieldInfo->keyType =
        CreateFieldInfo(rb_hash_aref(field_map_entry, sym_for_key));
    fieldInfo->elementType =
        CreateFieldInfo(rb_hash_aref(field_map_entry, sym_for_value));
    break;
  }
  default:
    break;
  }
  return fieldInfo;
}

// each klass has a FieldInfoMap
FieldInfoMap *CreateFieldInfoMap(VALUE klass) {
  FieldInfoMap *fieldMap = new FieldInfoMap();
  VALUE field_map = rb_const_get_at(klass, intern_for_FIELDS);

  HASH_FOREACH_BEGIN(field_map, fieldMap)
  FieldInfoMap *fieldMap = (FieldInfoMap *)argv[0];
  (*fieldMap)[FIX2INT(k)] = CreateFieldInfo(v);
  HASH_FOREACH_END()
  return fieldMap;
}

VALUE cache_fields(VALUE self, VALUE klass) {
  FindOrCreateFieldInfoMap(klass);
  return Qnil;
}

#undef HASH_FOREACH_BEGIN
#undef HASH_FOEACH_RET
#undef HASH_FOREACH_END

#undef R_FIX_TO_TTYPE
