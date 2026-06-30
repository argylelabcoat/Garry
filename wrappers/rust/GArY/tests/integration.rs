use std::collections::HashMap;

use garry::serializer::{self, Deserialize, Serialize, Value};

#[derive(Debug, PartialEq)]
struct User {
    name: String,
    age: i32,
}

impl Serialize for User {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("name".to_owned(), self.name.serialize());
        map.insert("age".to_owned(), self.age.serialize());
        Value::Map(map)
    }
}

impl Deserialize for User {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let name = map
            .get("name")
            .and_then(|v| v.as_str())
            .ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?
            .to_owned();
        let age = map
            .get("age")
            .and_then(|v| v.as_i32())
            .ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        Ok(User { name, age })
    }
}

#[derive(Debug, PartialEq)]
struct Config {
    host: String,
    port: i32,
    debug: bool,
}

impl Serialize for Config {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("host".to_owned(), self.host.serialize());
        map.insert("port".to_owned(), self.port.serialize());
        map.insert("debug".to_owned(), self.debug.serialize());
        Value::Map(map)
    }
}

impl Deserialize for Config {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let host = map
            .get("host")
            .and_then(|v| v.as_str())
            .ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?
            .to_owned();
        let port = map
            .get("port")
            .and_then(|v| v.as_i32())
            .ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let debug = map
            .get("debug")
            .and_then(|v| v.as_bool())
            .ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        Ok(Config { host, port, debug })
    }
}

#[test]
fn test_value_null() {
    let v = Value::Null;
    let encoded = serializer::encode(&v);
    let decoded = serializer::decode(&encoded).unwrap();
    assert_eq!(v, decoded);
}

#[test]
fn test_value_bool() {
    let v = Value::Bool(true);
    let encoded = serializer::encode(&v);
    let decoded = serializer::decode(&encoded).unwrap();
    assert_eq!(v, decoded);

    let v = Value::Bool(false);
    let encoded = serializer::encode(&v);
    let decoded = serializer::decode(&encoded).unwrap();
    assert_eq!(v, decoded);
}

#[test]
fn test_value_i32() {
    let values = vec![0, 1, -1, i32::MAX, i32::MIN, 42];
    for num in values {
        let v = Value::I32(num);
        let encoded = serializer::encode(&v);
        let decoded = serializer::decode(&encoded).unwrap();
        assert_eq!(v, decoded);
    }
}

#[test]
fn test_value_i64() {
    let values = vec![0, 1, -1, i64::MAX, i64::MIN, 42];
    for num in values {
        let v = Value::I64(num);
        let encoded = serializer::encode(&v);
        let decoded = serializer::decode(&encoded).unwrap();
        assert_eq!(v, decoded);
    }
}

#[test]
fn test_value_f32() {
    let values = vec![0.0, 1.0, -1.0, 3.14, f32::MIN, f32::MAX];
    for num in values {
        let v = Value::F32(num);
        let encoded = serializer::encode(&v);
        let decoded = serializer::decode(&encoded).unwrap();
        assert_eq!(v, decoded);
    }
}

#[test]
fn test_value_f64() {
    let values = vec![0.0, 1.0, -1.0, 3.141592653589793, f64::MIN, f64::MAX];
    for num in values {
        let v = Value::F64(num);
        let encoded = serializer::encode(&v);
        let decoded = serializer::decode(&encoded).unwrap();
        assert_eq!(v, decoded);
    }
}

#[test]
fn test_value_string() {
    let values = vec!["", "hello", "hello world", "日本語", "🎉"];
    for s in values {
        let v = Value::String(s.to_owned());
        let encoded = serializer::encode(&v);
        let decoded = serializer::decode(&encoded).unwrap();
        assert_eq!(v, decoded);
    }
}

#[test]
fn test_value_bytes() {
    let values = vec![vec![], vec![0, 1, 2, 3], vec![255, 254, 253]];
    for b in values {
        let v = Value::Bytes(b);
        let encoded = serializer::encode(&v);
        let decoded = serializer::decode(&encoded).unwrap();
        assert_eq!(v, decoded);
    }
}

#[test]
fn test_value_array() {
    let v = Value::Array(vec![
        Value::I32(1),
        Value::String("hello".to_owned()),
        Value::Bool(true),
        Value::Null,
    ]);
    let encoded = serializer::encode(&v);
    let decoded = serializer::decode(&encoded).unwrap();
    assert_eq!(v, decoded);
}

#[test]
fn test_value_map() {
    let mut map = HashMap::new();
    map.insert("key1".to_owned(), Value::I32(42));
    map.insert("key2".to_owned(), Value::String("value".to_owned()));
    let v = Value::Map(map);
    let encoded = serializer::encode(&v);
    let decoded = serializer::decode(&encoded).unwrap();
    assert_eq!(v, decoded);
}

#[test]
fn test_nested_structures() {
    let mut inner_map = HashMap::new();
    inner_map.insert("nested".to_owned(), Value::I32(100));

    let v = Value::Array(vec![
        Value::Map(inner_map),
        Value::Array(vec![Value::I32(1), Value::I32(2)]),
    ]);
    let encoded = serializer::encode(&v);
    let decoded = serializer::decode(&encoded).unwrap();
    assert_eq!(v, decoded);
}

#[test]
fn test_deep_nesting() {
    let mut current = Value::I32(0);
    for i in 1..=15 {
        let mut map = HashMap::new();
        map.insert(format!("level_{}", i), current);
        current = Value::Map(map);
    }

    let encoded = serializer::encode(&current);
    let decoded = serializer::decode(&encoded).unwrap();
    assert_eq!(current, decoded);
}

#[test]
fn test_key_encode_single() {
    let key = serializer::encode_key(&["hello"]);
    assert_eq!(key, vec![5, b'h', b'e', b'l', b'l', b'o']);
}

#[test]
fn test_key_encode_multiple() {
    let key = serializer::encode_key(&["users", "matthew"]);
    assert_eq!(
        key,
        vec![
            5, b'u', b's', b'e', b'r', b's', //
            7, b'm', b'a', b't', b't', b'h', b'e', b'w'
        ]
    );
}

#[test]
fn test_key_decode_roundtrip() {
    let original = vec!["users".to_owned(), "matthew".to_owned(), "settings".to_owned()];
    let encoded = serializer::encode_key(
        &original.iter().map(|s| s.as_str()).collect::<Vec<_>>(),
    );
    let decoded = serializer::decode_key(&encoded);
    assert_eq!(original, decoded);
}

#[test]
fn test_key_encode_empty() {
    let key = serializer::encode_key(&[]);
    assert!(key.is_empty());
}

#[test]
fn test_serialize_trait_bool() {
    let v = true.serialize();
    assert_eq!(v, Value::Bool(true));
    let decoded = bool::deserialize(&v).unwrap();
    assert!(decoded);
}

#[test]
fn test_serialize_trait_i32() {
    let v = 42i32.serialize();
    assert_eq!(v, Value::I32(42));
    let decoded = i32::deserialize(&v).unwrap();
    assert_eq!(decoded, 42);
}

#[test]
fn test_serialize_trait_string() {
    let v = "hello".to_string().serialize();
    assert_eq!(v, Value::String("hello".to_owned()));
    let decoded = String::deserialize(&v).unwrap();
    assert_eq!(decoded, "hello");
}

#[test]
fn test_serialize_trait_vec() {
    let v = vec![1i32, 2, 3].serialize();
    assert_eq!(
        v,
        Value::Array(vec![Value::I32(1), Value::I32(2), Value::I32(3)])
    );
    let decoded = Vec::<i32>::deserialize(&v).unwrap();
    assert_eq!(decoded, vec![1, 2, 3]);
}

#[test]
fn test_serialize_trait_option() {
    let some: Option<i32> = Some(42);
    let v = some.serialize();
    assert_eq!(v, Value::I32(42));
    let decoded = Option::<i32>::deserialize(&v).unwrap();
    assert_eq!(decoded, Some(42));

    let none: Option<i32> = None;
    let v = none.serialize();
    assert_eq!(v, Value::Null);
    let decoded = Option::<i32>::deserialize(&v).unwrap();
    assert_eq!(decoded, None);
}

#[test]
fn test_serialize_trait_hashmap() {
    let mut map = HashMap::new();
    map.insert("key".to_owned(), 42i32);
    let v = map.serialize();
    let decoded = HashMap::<String, i32>::deserialize(&v).unwrap();
    assert_eq!(decoded.get("key"), Some(&42));
}

#[test]
fn test_flatten_unflatten() {
    let user = User {
        name: "Matthew".to_owned(),
        age: 30,
    };

    let entries = serializer::flatten_to_keys("user", &user);
    assert!(!entries.is_empty());

    let restored = serializer::unflatten_from_keys(&entries).unwrap();
    let map = restored.as_map().unwrap();
    let user_map = map.get("user").and_then(|v| v.as_map()).unwrap();
    assert_eq!(user_map.get("name").and_then(|v| v.as_str()), Some("Matthew"));
    assert_eq!(user_map.get("age").and_then(|v| v.as_i32()), Some(30));
}

#[test]
fn test_serialize_object() {
    let config = Config {
        host: "localhost".to_owned(),
        port: 8080,
        debug: true,
    };

    let bytes = serializer::serialize_object(&config);
    let restored: Config = serializer::deserialize_object(&bytes).unwrap();
    assert_eq!(config, restored);
}

#[derive(Debug, PartialEq)]
struct Level10 { deep_value: String }
#[derive(Debug, PartialEq)]
struct Level9 { child: Level10 }
#[derive(Debug, PartialEq)]
struct Level8 { child: Level9 }
#[derive(Debug, PartialEq)]
struct Level7 { child: Level8 }
#[derive(Debug, PartialEq)]
struct Level6 { child: Level7 }
#[derive(Debug, PartialEq)]
struct Level5 { child: Level6 }
#[derive(Debug, PartialEq)]
struct Level4 { child: Level5 }
#[derive(Debug, PartialEq)]
struct Level3 { child: Level4 }
#[derive(Debug, PartialEq)]
struct Level2 { child: Level3 }
#[derive(Debug, PartialEq)]
struct Level1 { child: Level2 }
#[derive(Debug, PartialEq)]
struct DeepRoot { child: Level1 }

impl Serialize for Level10 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("deep_value".to_owned(), self.deep_value.serialize());
        Value::Map(map)
    }
}

impl Deserialize for Level10 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let deep_value = map
            .get("deep_value")
            .and_then(|v| v.as_str())
            .ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?
            .to_owned();
        Ok(Level10 { deep_value })
    }
}

impl Serialize for Level9 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for Level9 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level10::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(Level9 { child })
    }
}

impl Serialize for Level8 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for Level8 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level9::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(Level8 { child })
    }
}

impl Serialize for Level7 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for Level7 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level8::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(Level7 { child })
    }
}

impl Serialize for Level6 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for Level6 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level7::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(Level6 { child })
    }
}

impl Serialize for Level5 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for Level5 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level6::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(Level5 { child })
    }
}

impl Serialize for Level4 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for Level4 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level5::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(Level4 { child })
    }
}

impl Serialize for Level3 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for Level3 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level4::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(Level3 { child })
    }
}

impl Serialize for Level2 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for Level2 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level3::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(Level2 { child })
    }
}

impl Serialize for Level1 {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for Level1 {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level2::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(Level1 { child })
    }
}

impl Serialize for DeepRoot {
    fn serialize(&self) -> Value {
        let mut map = HashMap::new();
        map.insert("child".to_owned(), self.child.serialize());
        Value::Map(map)
    }
}
impl Deserialize for DeepRoot {
    fn deserialize(value: &Value) -> Result<Self, serializer::DecodeError> {
        let map = value.as_map().ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?;
        let child = Level1::deserialize(map.get("child").ok_or(serializer::DecodeError::UnexpectedTag(0xFF))?)?;
        Ok(DeepRoot { child })
    }
}

#[test]
fn test_deep_nesting_struct_round_trip() {
    let root = DeepRoot {
        child: Level1 {
            child: Level2 {
                child: Level3 {
                    child: Level4 {
                        child: Level5 {
                            child: Level6 {
                                child: Level7 {
                                    child: Level8 {
                                        child: Level9 {
                                            child: Level10 {
                                                deep_value: "found it".to_string(),
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                },
            },
        },
    };

    let bytes = serializer::serialize_object(&root);
    let restored: DeepRoot = serializer::deserialize_object(&bytes).unwrap();
    assert_eq!(root, restored);

    assert_eq!(restored.child.child.child.child.child.child.child.child.child.child.deep_value, "found it");
}

#[test]
fn test_deep_nesting_value_round_trip() {
    let mut current = Value::String("found it".to_string());
    for i in (1..=10).rev() {
        let mut map = HashMap::new();
        map.insert(format!("level_{}", i), current);
        current = Value::Map(map);
    }

    let encoded = serializer::encode(&current);
    let decoded = serializer::decode(&encoded).unwrap();
    assert_eq!(current, decoded);

    let l1 = decoded.as_map().unwrap().get("level_1").unwrap().as_map().unwrap();
    let l2 = l1.get("level_2").unwrap().as_map().unwrap();
    let l3 = l2.get("level_3").unwrap().as_map().unwrap();
    let l4 = l3.get("level_4").unwrap().as_map().unwrap();
    let l5 = l4.get("level_5").unwrap().as_map().unwrap();
    let l6 = l5.get("level_6").unwrap().as_map().unwrap();
    let l7 = l6.get("level_7").unwrap().as_map().unwrap();
    let l8 = l7.get("level_8").unwrap().as_map().unwrap();
    let l9 = l8.get("level_9").unwrap().as_map().unwrap();
    let l10 = l9.get("level_10").unwrap().as_str().unwrap();
    assert_eq!(l10, "found it");
}

#[test]
fn test_deep_nesting_different_values() {
    let root = DeepRoot {
        child: Level1 {
            child: Level2 {
                child: Level3 {
                    child: Level4 {
                        child: Level5 {
                            child: Level6 {
                                child: Level7 {
                                    child: Level8 {
                                        child: Level9 {
                                            child: Level10 {
                                                deep_value: "unique_marker_42".to_string(),
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                },
            },
        },
    };

    let bytes = serializer::serialize_object(&root);
    let restored: DeepRoot = serializer::deserialize_object(&bytes).unwrap();

    let l = &restored.child.child.child.child.child.child.child.child.child.child;
    assert_eq!(l.deep_value, "unique_marker_42");
    assert_ne!(l.deep_value, "found it");
}
