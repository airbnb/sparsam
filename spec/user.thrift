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
