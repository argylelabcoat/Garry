use std::collections::HashMap;
use std::fmt;

#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    Null,
    Bool(bool),
    I32(i32),
    I64(i64),
    F32(f32),
    F64(f64),
    String(String),
    Bytes(Vec<u8>),
    Array(Vec<Value>),
    Map(HashMap<String, Value>),
}

impl Value {
    pub fn is_null(&self) -> bool {
        matches!(self, Value::Null)
    }

    pub fn as_bool(&self) -> Option<bool> {
        match self {
            Value::Bool(b) => Some(*b),
            _ => None,
        }
    }

    pub fn as_i32(&self) -> Option<i32> {
        match self {
            Value::I32(v) => Some(*v),
            _ => None,
        }
    }

    pub fn as_i64(&self) -> Option<i64> {
        match self {
            Value::I64(v) => Some(*v),
            _ => None,
        }
    }

    pub fn as_f32(&self) -> Option<f32> {
        match self {
            Value::F32(v) => Some(*v),
            _ => None,
        }
    }

    pub fn as_f64(&self) -> Option<f64> {
        match self {
            Value::F64(v) => Some(*v),
            _ => None,
        }
    }

    pub fn as_str(&self) -> Option<&str> {
        match self {
            Value::String(s) => Some(s),
            _ => None,
        }
    }

    pub fn as_bytes(&self) -> Option<&[u8]> {
        match self {
            Value::Bytes(b) => Some(b),
            _ => None,
        }
    }

    pub fn as_array(&self) -> Option<&[Value]> {
        match self {
            Value::Array(a) => Some(a),
            _ => None,
        }
    }

    pub fn as_map(&self) -> Option<&HashMap<String, Value>> {
        match self {
            Value::Map(m) => Some(m),
            _ => None,
        }
    }
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Value::Null => write!(f, "null"),
            Value::Bool(b) => write!(f, "{}", b),
            Value::I32(v) => write!(f, "{}", v),
            Value::I64(v) => write!(f, "{}", v),
            Value::F32(v) => write!(f, "{}", v),
            Value::F64(v) => write!(f, "{}", v),
            Value::String(s) => write!(f, "\"{}\"", s),
            Value::Bytes(_) => write!(f, "<bytes>"),
            Value::Array(a) => {
                write!(f, "[")?;
                for (i, v) in a.iter().enumerate() {
                    if i > 0 {
                        write!(f, ", ")?;
                    }
                    write!(f, "{}", v)?;
                }
                write!(f, "]")
            }
            Value::Map(m) => {
                write!(f, "{{")?;
                for (i, (k, v)) in m.iter().enumerate() {
                    if i > 0 {
                        write!(f, ", ")?;
                    }
                    write!(f, "\"{}\": {}", k, v)?;
                }
                write!(f, "}}")
            }
        }
    }
}

const TAG_NULL: u8 = 0x00;
const TAG_BOOL: u8 = 0x01;
const TAG_I32: u8 = 0x06;
const TAG_I64: u8 = 0x08;
const TAG_F32: u8 = 0x0A;
const TAG_F64: u8 = 0x0B;
const TAG_STRING: u8 = 0x0C;
const TAG_BYTES: u8 = 0x0D;

pub fn encode(value: &Value) -> Vec<u8> {
    let mut buf = Vec::new();
    encode_into(value, &mut buf);
    buf
}

fn encode_into(value: &Value, buf: &mut Vec<u8>) {
    match value {
        Value::Null => buf.push(TAG_NULL),
        Value::Bool(b) => {
            buf.push(TAG_BOOL);
            buf.push(if *b { 1 } else { 0 });
        }
        Value::I32(v) => {
            buf.push(TAG_I32);
            buf.extend_from_slice(&v.to_le_bytes());
        }
        Value::I64(v) => {
            buf.push(TAG_I64);
            buf.extend_from_slice(&v.to_le_bytes());
        }
        Value::F32(v) => {
            buf.push(TAG_F32);
            buf.extend_from_slice(&v.to_le_bytes());
        }
        Value::F64(v) => {
            buf.push(TAG_F64);
            buf.extend_from_slice(&v.to_le_bytes());
        }
        Value::String(s) => {
            buf.push(TAG_STRING);
            buf.extend_from_slice(s.as_bytes());
        }
        Value::Bytes(b) => {
            buf.push(TAG_BYTES);
            buf.extend_from_slice(&(b.len() as i32).to_le_bytes());
            buf.extend_from_slice(b);
        }
        Value::Array(arr) => {
            buf.push(TAG_ARRAY);
            buf.extend_from_slice(&(arr.len() as i32).to_le_bytes());
            for item in arr {
                encode_into(item, buf);
            }
        }
        Value::Map(map) => {
            buf.push(TAG_MAP);
            buf.extend_from_slice(&(map.len() as i32).to_le_bytes());
            for (k, v) in map {
                let key_bytes = k.as_bytes();
                buf.extend_from_slice(&(key_bytes.len() as i32).to_le_bytes());
                buf.extend_from_slice(key_bytes);
                encode_into(v, buf);
            }
        }
    }
}

#[derive(Debug, Clone)]
pub enum DecodeError {
    EmptyInput,
    UnexpectedTag(u8),
    TruncatedData,
    InvalidUtf8,
}

impl fmt::Display for DecodeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            DecodeError::EmptyInput => write!(f, "empty input"),
            DecodeError::UnexpectedTag(t) => write!(f, "unexpected tag: 0x{:02X}", t),
            DecodeError::TruncatedData => write!(f, "truncated data"),
            DecodeError::InvalidUtf8 => write!(f, "invalid UTF-8"),
        }
    }
}

impl std::error::Error for DecodeError {}

pub fn decode(data: &[u8]) -> Result<Value, DecodeError> {
    let (value, _consumed) = decode_at(data, 0)?;
    Ok(value)
}

fn decode_at(data: &[u8], pos: usize) -> Result<(Value, usize), DecodeError> {
    if pos >= data.len() {
        return Err(DecodeError::EmptyInput);
    }

    let tag = data[pos];
    let mut cursor = pos + 1;

    match tag {
        TAG_NULL => Ok((Value::Null, cursor)),
        TAG_BOOL => {
            if cursor >= data.len() {
                return Err(DecodeError::TruncatedData);
            }
            let b = data[cursor] != 0;
            cursor += 1;
            Ok((Value::Bool(b), cursor))
        }
        TAG_I32 => {
            if cursor + 4 > data.len() {
                return Err(DecodeError::TruncatedData);
            }
            let v = i32::from_le_bytes([
                data[cursor],
                data[cursor + 1],
                data[cursor + 2],
                data[cursor + 3],
            ]);
            cursor += 4;
            Ok((Value::I32(v), cursor))
        }
        TAG_I64 => {
            if cursor + 8 > data.len() {
                return Err(DecodeError::TruncatedData);
            }
            let v = i64::from_le_bytes([
                data[cursor],
                data[cursor + 1],
                data[cursor + 2],
                data[cursor + 3],
                data[cursor + 4],
                data[cursor + 5],
                data[cursor + 6],
                data[cursor + 7],
            ]);
            cursor += 8;
            Ok((Value::I64(v), cursor))
        }
        TAG_F32 => {
            if cursor + 4 > data.len() {
                return Err(DecodeError::TruncatedData);
            }
            let v = f32::from_le_bytes([
                data[cursor],
                data[cursor + 1],
                data[cursor + 2],
                data[cursor + 3],
            ]);
            cursor += 4;
            Ok((Value::F32(v), cursor))
        }
        TAG_F64 => {
            if cursor + 8 > data.len() {
                return Err(DecodeError::TruncatedData);
            }
            let v = f64::from_le_bytes([
                data[cursor],
                data[cursor + 1],
                data[cursor + 2],
                data[cursor + 3],
                data[cursor + 4],
                data[cursor + 5],
                data[cursor + 6],
                data[cursor + 7],
            ]);
            cursor += 8;
            Ok((Value::F64(v), cursor))
        }
        TAG_STRING => {
            let s = std::str::from_utf8(&data[cursor..])
                .map_err(|_| DecodeError::InvalidUtf8)?;
            cursor = data.len();
            Ok((Value::String(s.to_owned()), cursor))
        }
        TAG_BYTES => {
            if cursor + 4 > data.len() {
                return Err(DecodeError::TruncatedData);
            }
            let len = i32::from_le_bytes([
                data[cursor],
                data[cursor + 1],
                data[cursor + 2],
                data[cursor + 3],
            ]) as usize;
            cursor += 4;
            if cursor + len > data.len() {
                return Err(DecodeError::TruncatedData);
            }
            let bytes = data[cursor..cursor + len].to_vec();
            cursor += len;
            Ok((Value::Bytes(bytes), cursor))
        }
        TAG_ARRAY => {
            if cursor + 4 > data.len() {
                return Err(DecodeError::TruncatedData);
            }
            let len = i32::from_le_bytes([
                data[cursor],
                data[cursor + 1],
                data[cursor + 2],
                data[cursor + 3],
            ]) as usize;
            cursor += 4;
            let mut items = Vec::with_capacity(len);
            for _ in 0..len {
                let (item, new_pos) = decode_at(data, cursor)?;
                items.push(item);
                cursor = new_pos;
            }
            Ok((Value::Array(items), cursor))
        }
        TAG_MAP => {
            if cursor + 4 > data.len() {
                return Err(DecodeError::TruncatedData);
            }
            let len = i32::from_le_bytes([
                data[cursor],
                data[cursor + 1],
                data[cursor + 2],
                data[cursor + 3],
            ]) as usize;
            cursor += 4;
            let mut map = HashMap::with_capacity(len);
            for _ in 0..len {
                if cursor + 4 > data.len() {
                    return Err(DecodeError::TruncatedData);
                }
                let klen = i32::from_le_bytes([
                    data[cursor],
                    data[cursor + 1],
                    data[cursor + 2],
                    data[cursor + 3],
                ]) as usize;
                cursor += 4;
                if cursor + klen > data.len() {
                    return Err(DecodeError::TruncatedData);
                }
                let key = std::str::from_utf8(&data[cursor..cursor + klen])
                    .map_err(|_| DecodeError::InvalidUtf8)?;
                cursor += klen;
                let (val, new_pos) = decode_at(data, cursor)?;
                map.insert(key.to_owned(), val);
                cursor = new_pos;
            }
            Ok((Value::Map(map), cursor))
        }
        _ => Err(DecodeError::UnexpectedTag(tag)),
    }
}

pub fn encode_key(parts: &[&str]) -> Vec<u8> {
    let mut result = Vec::new();
    for part in parts {
        let bytes = part.as_bytes();
        result.push(bytes.len() as u8);
        result.extend_from_slice(bytes);
    }
    result
}

pub fn decode_key(key: &[u8]) -> Vec<String> {
    let mut parts = Vec::new();
    let mut offset = 0;
    while offset < key.len() {
        let len = key[offset] as usize;
        offset += 1;
        if offset + len > key.len() {
            break;
        }
        if let Ok(s) = std::str::from_utf8(&key[offset..offset + len]) {
            parts.push(s.to_owned());
        }
        offset += len;
    }
    parts
}

pub trait Serialize {
    fn serialize(&self) -> Value;
}

pub trait Deserialize: Sized {
    fn deserialize(value: &Value) -> Result<Self, DecodeError>;
}

impl Serialize for Value {
    fn serialize(&self) -> Value {
        self.clone()
    }
}

impl Deserialize for Value {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        Ok(value.clone())
    }
}

impl Serialize for bool {
    fn serialize(&self) -> Value {
        Value::Bool(*self)
    }
}

impl Deserialize for bool {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        value.as_bool().ok_or(DecodeError::UnexpectedTag(0xFF))
    }
}

impl Serialize for i32 {
    fn serialize(&self) -> Value {
        Value::I32(*self)
    }
}

impl Deserialize for i32 {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        value.as_i32().ok_or(DecodeError::UnexpectedTag(0xFF))
    }
}

impl Serialize for i64 {
    fn serialize(&self) -> Value {
        Value::I64(*self)
    }
}

impl Deserialize for i64 {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        value.as_i64().ok_or(DecodeError::UnexpectedTag(0xFF))
    }
}

impl Serialize for f32 {
    fn serialize(&self) -> Value {
        Value::F32(*self)
    }
}

impl Deserialize for f32 {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        value.as_f32().ok_or(DecodeError::UnexpectedTag(0xFF))
    }
}

impl Serialize for f64 {
    fn serialize(&self) -> Value {
        Value::F64(*self)
    }
}

impl Deserialize for f64 {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        value.as_f64().ok_or(DecodeError::UnexpectedTag(0xFF))
    }
}

impl Serialize for str {
    fn serialize(&self) -> Value {
        Value::String(self.to_owned())
    }
}

impl Serialize for String {
    fn serialize(&self) -> Value {
        Value::String(self.clone())
    }
}

impl Deserialize for String {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        value
            .as_str()
            .map(|s| s.to_owned())
            .ok_or(DecodeError::UnexpectedTag(0xFF))
    }
}

impl Serialize for [u8] {
    fn serialize(&self) -> Value {
        Value::Bytes(self.to_vec())
    }
}

impl Serialize for Vec<u8> {
    fn serialize(&self) -> Value {
        Value::Bytes(self.clone())
    }
}

impl Deserialize for Vec<u8> {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        value
            .as_bytes()
            .map(|b| b.to_vec())
            .ok_or(DecodeError::UnexpectedTag(0xFF))
    }
}

impl<T: Serialize> Serialize for Vec<T> {
    fn serialize(&self) -> Value {
        Value::Array(self.iter().map(|v| v.serialize()).collect())
    }
}

impl<T: Deserialize> Deserialize for Vec<T> {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        let arr = value.as_array().ok_or(DecodeError::UnexpectedTag(0xFF))?;
        arr.iter().map(|v| T::deserialize(v)).collect()
    }
}

impl<T: Serialize> Serialize for Option<T> {
    fn serialize(&self) -> Value {
        match self {
            Some(v) => v.serialize(),
            None => Value::Null,
        }
    }
}

impl<T: Deserialize> Deserialize for Option<T> {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        match value {
            Value::Null => Ok(None),
            _ => Ok(Some(T::deserialize(value)?)),
        }
    }
}

impl<K: Serialize, V: Serialize> Serialize for HashMap<K, V>
where
    K: std::hash::Hash + Eq,
{
    fn serialize(&self) -> Value {
        let map: HashMap<String, Value> = self
            .iter()
            .map(|(k, v)| {
                let key = match k.serialize() {
                    Value::String(s) => s,
                    other => format!("{}", other),
                };
                (key, v.serialize())
            })
            .collect();
        Value::Map(map)
    }
}

impl<V: Deserialize> Deserialize for HashMap<String, V> {
    fn deserialize(value: &Value) -> Result<Self, DecodeError> {
        let map = value.as_map().ok_or(DecodeError::UnexpectedTag(0xFF))?;
        map.iter()
            .map(|(k, v)| Ok((k.clone(), V::deserialize(v)?)))
            .collect()
    }
}

pub fn serialize_object<T: Serialize>(obj: &T) -> Vec<u8> {
    let value = obj.serialize();
    encode(&value)
}

pub fn deserialize_object<T: Deserialize>(data: &[u8]) -> Result<T, DecodeError> {
    let value = decode(data)?;
    T::deserialize(&value)
}

pub fn flatten_to_keys<T: Serialize>(prefix: &str, obj: &T) -> Vec<(Vec<u8>, Vec<u8>)> {
    let value = obj.serialize();
    let mut result = Vec::new();
    let prefix_parts: Vec<&str> = if prefix.is_empty() {
        vec![]
    } else {
        prefix.split('.').collect()
    };
    flatten_value(&prefix_parts, &value, &mut result);
    result
}

fn flatten_value(prefix_parts: &[&str], value: &Value, result: &mut Vec<(Vec<u8>, Vec<u8>)>) {
    match value {
        Value::Map(map) => {
            for (k, v) in map {
                let mut parts = prefix_parts.to_vec();
                parts.push(k);
                flatten_value(&parts, v, result);
            }
        }
        Value::Array(arr) => {
            for (i, v) in arr.iter().enumerate() {
                let mut parts = prefix_parts.to_vec();
                parts.push(Box::leak(i.to_string().into_boxed_str()));
                flatten_value(&parts, v, result);
            }
        }
        _ => {
            let key_bytes = encode_key(prefix_parts);
            let val_bytes = encode(value);
            result.push((key_bytes, val_bytes));
        }
    }
}

pub fn unflatten_from_keys<K: AsRef<[u8]>>(entries: &[(K, Vec<u8>)]) -> Result<Value, DecodeError> {
    let mut root: HashMap<String, Value> = HashMap::new();

    for (key_bytes, val_bytes) in entries {
        let parts = decode_key(key_bytes.as_ref());
        let value = decode(val_bytes)?;
        insert_nested(&mut root, &parts, value);
    }

    Ok(Value::Map(root))
}

fn insert_nested(map: &mut HashMap<String, Value>, parts: &[String], value: Value) {
    if parts.is_empty() {
        return;
    }

    if parts.len() == 1 {
        map.insert(parts[0].clone(), value);
        return;
    }

    let key = &parts[0];
    let remaining = &parts[1..];

    let entry = map
        .entry(key.clone())
        .or_insert_with(|| Value::Map(HashMap::new()));

    if let Value::Map(m) = entry {
        insert_nested(m, remaining, value);
    }
}
