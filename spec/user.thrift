struct US {
 1: optional i32 id_i32
 2: optional binary id_s = "id_s default"
 3: optional set<string> string_set
 4: optional set<i32> int_set
}

union UN {
 1: optional i32 id_i32
 2: optional string id_s
}

enum Magic {
  Black,
  White,
  Red,
  Blue,
  Green
}

struct SS {
 1: optional i32 id_i32
 2: optional string id_s
 3: optional US us_i
 4: optional set<US> us_s
 5: optional list<string> s_l
 6: optional map<i32, string> mappy
 7: optional byte byte_field
 8: optional UN un_field
 9: optional Magic magic_field
 10: optional map<i32, map<i32, i32>> troll
}

struct MiniRequired {
 1: i32 id_i32
}

struct EasilyInvalid {
  1: optional EasilyInvalid tail
  2: optional set<EasilyInvalid> s_self
  3: optional list<EasilyInvalid> l_self
  4: optional map<EasilyInvalid, i32> mappy1
  5: optional map<i32, EasilyInvalid> mappy2
  6: optional map<EasilyInvalid, EasilyInvalid> mappy3
  7: optional list<map<set<i32>, map<i32, set<list<map<EasilyInvalid, string>>>>>> sure
  8: optional MiniRequired required_stuff
  9: optional i32 id_i32
}

struct NotSS {
 1: optional string id_s
 3: optional i32 id_i32
}

struct NotSS_plus {
 1: optional string id_s
 2: optional string id_s2
 3: optional i32 id_i32
}

struct nothing {
}

struct EveryType {
  1: optional bool a_bool
  2: optional byte a_byte
  3: optional i16 an_i16
  4: optional i32 an_i32
  5: optional i64 an_i64
  6: optional double a_double
  7: optional binary a_binary
  8: optional string a_string

  9: optional list<i64> an_i64_list
  10: optional set<i64> an_i64_set
  11: optional map<i64, i64> an_i64_map

  12: optional list<map<i64, i64>> a_list_of_i64_maps
  13: optional map<map<i64, i64>, map<i64, i64>> a_map_of_i64_maps

  14: optional US a_struct
  15: optional UN a_union
}
