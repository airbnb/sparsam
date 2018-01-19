#ifndef __SERIALIZER_H__
#include <ruby.h>
#include <ruby/intern.h>
#ifndef NUM2SHORT
#define NUM2SHORT NUM2INT
#endif
#ifdef __cplusplus
extern "C" {
#endif

enum Proto {
  compact = 0,
  binary = 1,
};

enum TOType {
  t_union = 0,
  t_struct = 1,
};

enum ValidateStrictness { normal = 0, strict = 1, recursive = 2 };

void serializer_free(void *data);
void *serializer_create();
void serializer_init(void *serializer, int protocol, void *str_arg1,
                     uint32_t len);

VALUE serializer_readStruct(VALUE self, VALUE klass);
VALUE serializer_writeStruct(VALUE self, VALUE klass, VALUE data);

VALUE cache_fields(VALUE self, VALUE klass);

VALUE serializer_validate(VALUE self, VALUE klass, VALUE data,
                          VALUE strictness);

void initialize_constants();
void initialize_runtime_constants();

#ifdef __cplusplus
} // end extern "C"

#include <thrift/stdcxx.h>
#include <map>
#include <string>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <unordered_set>
#include "third-party/sparsepp/sparsepp/spp.h"

using ::apache::thrift::protocol::TType;

typedef uint16_t FieldIdIndex;
typedef uint16_t KlassIndex;

typedef int16_t FieldID;

typedef struct FieldBegin {
  TType ftype;
  FieldID fid;
} FieldBegin;

typedef struct FieldInfo {
  TType ftype;
  VALUE klass; // set if TTYPE is struct or union
  ID ivarName; // set if field is on struct
  VALUE symName; // set if field is on struct/union
  bool isOptional;
  bool isBinaryString;
  FieldInfo *elementType; // element of list or set, or map
  FieldInfo *keyType;     // type of key in maps
} FieldInfo;

typedef std::map<FieldID, FieldInfo *> FieldInfoMap;
typedef spp::sparse_hash_map<VALUE, FieldInfoMap *> KlassFieldsCache;

class ThriftSerializer {
public:
  ThriftSerializer(){};
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol > tprot;
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::transport::TMemoryBuffer > tmb;

  VALUE readStruct(VALUE klass);
  void writeStruct(VALUE klass, VALUE data);

private:
  VALUE readUnion(VALUE klass);
  VALUE readAny(TType ttype, FieldInfo *field_info);
  void writeAny(TType ttype, FieldInfo *field_info, VALUE data, VALUE outer_struct, VALUE field_sym);
  void skip_n_type(uint32_t n, TType ttype);
  void skip_n_pair(uint32_t n, TType type_a, TType type_b);
};

FieldInfoMap *FindOrCreateFieldInfoMap(VALUE klass);
FieldInfo *CreateFieldInfo(VALUE field_map_entry);
FieldInfoMap *CreateFieldInfoMap(VALUE klass);

#endif
#endif
