pub mod ffi;
pub mod serializer;

use std::ffi::CString;
use std::fmt;

use serializer::{Deserialize, Serialize};

#[derive(Debug, Clone)]
pub enum GarryError {
    NotFound,
    LockConflict,
    Io,
    Corrupt,
    InvalidArg,
    NoMem,
    BufferTooSmall,
    NullPointer,
    NulError,
    Utf8Error,
    Other(i32),
}

impl fmt::Display for GarryError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            GarryError::NotFound => write!(f, "key not found"),
            GarryError::LockConflict => write!(f, "transaction lock conflict"),
            GarryError::Io => write!(f, "I/O error"),
            GarryError::Corrupt => write!(f, "database corruption"),
            GarryError::InvalidArg => write!(f, "invalid argument"),
            GarryError::NoMem => write!(f, "out of memory"),
            GarryError::BufferTooSmall => write!(f, "buffer too small"),
            GarryError::NullPointer => write!(f, "null pointer"),
            GarryError::NulError => write!(f, "nul terminator error"),
            GarryError::Utf8Error => write!(f, "UTF-8 error"),
            GarryError::Other(code) => write!(f, "error code: {}", code),
        }
    }
}

impl std::error::Error for GarryError {}

impl From<std::ffi::NulError> for GarryError {
    fn from(_: std::ffi::NulError) -> Self {
        GarryError::NulError
    }
}

impl From<std::str::Utf8Error> for GarryError {
    fn from(_: std::str::Utf8Error) -> Self {
        GarryError::Utf8Error
    }
}

fn status_to_result(status: i32) -> Result<(), GarryError> {
    match status {
        ffi::GARRY_OK => Ok(()),
        ffi::GARRY_ERR_NOT_FOUND => Err(GarryError::NotFound),
        ffi::GARRY_ERR_LOCK_CONFLICT => Err(GarryError::LockConflict),
        ffi::GARRY_ERR_IO => Err(GarryError::Io),
        ffi::GARRY_ERR_CORRUPT => Err(GarryError::Corrupt),
        ffi::GARRY_ERR_INVALID_ARG => Err(GarryError::InvalidArg),
        ffi::GARRY_ERR_NOMEM => Err(GarryError::NoMem),
        ffi::GARRY_ERR_BUFFER_TOO_SMALL => Err(GarryError::BufferTooSmall),
        other => Err(GarryError::Other(other)),
    }
}

pub struct Database {
    handle: *mut ffi::garry_database,
}

unsafe impl Send for Database {}
unsafe impl Sync for Database {}

impl Database {
    pub fn create(path: &str) -> Result<Self, GarryError> {
        let c_path = CString::new(path)?;
        let handle = unsafe { ffi::garry_database_create(c_path.as_ptr()) };
        if handle.is_null() {
            return Err(GarryError::NullPointer);
        }
        Ok(Database { handle })
    }

    pub fn create_with_config(path: &str, config: &DatabaseConfig) -> Result<Self, GarryError> {
        let c_path = CString::new(path)?;
        let ffi_config = config.to_ffi();
        let handle = unsafe { ffi::garry_database_create_with_config(c_path.as_ptr(), ffi_config) };
        if handle.is_null() {
            return Err(GarryError::NullPointer);
        }
        Ok(Database { handle })
    }

    pub fn open(path: &str) -> Result<Self, GarryError> {
        let c_path = CString::new(path)?;
        let handle = unsafe { ffi::garry_database_open(c_path.as_ptr()) };
        if handle.is_null() {
            return Err(GarryError::NullPointer);
        }
        Ok(Database { handle })
    }

    pub fn begin_transaction(&self) -> Result<Transaction, GarryError> {
        let txn = unsafe { ffi::garry_txn_begin(self.handle) };
        Ok(Transaction {
            db: self.handle,
            txn,
        })
    }

    pub fn get(&self, key: &str) -> Result<Option<Vec<u8>>, GarryError> {
        let txn = self.begin_transaction()?;
        let result = txn.get(key);
        txn.commit()?;
        result
    }

    pub fn set(&self, key: &str, value: &[u8]) -> Result<(), GarryError> {
        let txn = self.begin_transaction()?;
        txn.set(key, value)?;
        txn.commit()
    }

    pub fn delete(&self, key: &str) -> Result<bool, GarryError> {
        let txn = self.begin_transaction()?;
        let result = txn.delete(key);
        txn.commit()?;
        result
    }

    pub fn exists(&self, key: &str) -> Result<bool, GarryError> {
        let txn = self.begin_transaction()?;
        let result = txn.exists(key);
        txn.commit()?;
        result
    }

    pub fn count(&self) -> Result<i32, GarryError> {
        let txn = self.begin_transaction()?;
        let count = unsafe { ffi::garry_count(self.handle, txn.txn) };
        txn.commit()?;
        Ok(count)
    }

    pub fn get_object<T: Deserialize>(&self, key: &str) -> Result<Option<T>, GarryError> {
        let data = self.get(key)?;
        match data {
            Some(bytes) => {
                let value = serializer::decode(&bytes)
                    .map_err(|_| GarryError::Other(-1))?;
                T::deserialize(&value)
                    .map(Some)
                    .map_err(|_| GarryError::Other(-1))
            }
            None => Ok(None),
        }
    }

    pub fn set_object<T: Serialize>(&self, key: &str, obj: &T) -> Result<(), GarryError> {
        let value = obj.serialize();
        let bytes = serializer::encode(&value);
        self.set(key, &bytes)
    }

    pub fn default_config() -> DatabaseConfig {
        let config = unsafe { ffi::garry_config_default() };
        DatabaseConfig::from_ffi(config)
    }
}

impl Drop for Database {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe {
                ffi::garry_database_close(self.handle);
            }
        }
    }
}

pub struct Transaction {
    db: *mut ffi::garry_database,
    txn: i32,
}

unsafe impl Send for Transaction {}

impl Transaction {
    pub fn get(&self, key: &str) -> Result<Option<Vec<u8>>, GarryError> {
        let c_key = CString::new(key)?;
        let mut len: i32 = 0;

        let status = unsafe {
            ffi::garry_get(
                self.db,
                self.txn,
                c_key.as_ptr() as *const u8,
                key.len() as i32,
                std::ptr::null_mut(),
                &mut len,
            )
        };

        if status == ffi::GARRY_ERR_NOT_FOUND {
            return Ok(None);
        }
        status_to_result(status)?;

        let mut buf = vec![0u8; len as usize];
        let status = unsafe {
            ffi::garry_get(
                self.db,
                self.txn,
                c_key.as_ptr() as *const u8,
                key.len() as i32,
                buf.as_mut_ptr(),
                &mut len,
            )
        };
        status_to_result(status)?;
        buf.truncate(len as usize);
        Ok(Some(buf))
    }

    pub fn set(&self, key: &str, value: &[u8]) -> Result<(), GarryError> {
        let c_key = CString::new(key)?;
        let status = unsafe {
            ffi::garry_set(
                self.db,
                self.txn,
                c_key.as_ptr() as *const u8,
                key.len() as i32,
                value.as_ptr(),
                value.len() as i32,
            )
        };
        status_to_result(status)
    }

    pub fn delete(&self, key: &str) -> Result<bool, GarryError> {
        let c_key = CString::new(key)?;
        let status = unsafe {
            ffi::garry_delete(
                self.db,
                self.txn,
                c_key.as_ptr() as *const u8,
                key.len() as i32,
            )
        };
        if status == ffi::GARRY_ERR_NOT_FOUND {
            return Ok(false);
        }
        status_to_result(status)?;
        Ok(true)
    }

    pub fn exists(&self, key: &str) -> Result<bool, GarryError> {
        let c_key = CString::new(key)?;
        let result = unsafe {
            ffi::garry_exists(self.db, self.txn, c_key.as_ptr() as *const u8, key.len() as i32)
        };
        Ok(result == ffi::GARRY_TRUE)
    }

    pub fn count(&self) -> Result<i32, GarryError> {
        let count = unsafe { ffi::garry_count(self.db, self.txn) };
        Ok(count)
    }

    pub fn get_object<T: Deserialize>(&self, key: &str) -> Result<Option<T>, GarryError> {
        let data = self.get(key)?;
        match data {
            Some(bytes) => {
                let value =
                    serializer::decode(&bytes).map_err(|_| GarryError::Other(-1))?;
                T::deserialize(&value)
                    .map(Some)
                    .map_err(|_| GarryError::Other(-1))
            }
            None => Ok(None),
        }
    }

    pub fn set_object<T: Serialize>(&self, key: &str, obj: &T) -> Result<(), GarryError> {
        let value = obj.serialize();
        let bytes = serializer::encode(&value);
        self.set(key, &bytes)
    }

    pub fn commit(self) -> Result<(), GarryError> {
        unsafe {
            ffi::garry_txn_commit(self.db, self.txn);
        }
        Ok(())
    }

    pub fn rollback(self) {
        unsafe {
            ffi::garry_txn_rollback(self.db, self.txn);
        }
    }
}

pub struct Cursor {
    handle: *mut ffi::garry_cursor,
    db: *mut ffi::garry_database,
    txn: i32,
}

unsafe impl Send for Cursor {}

impl Cursor {
    pub fn open(db: &Database, prefix: Option<&[u8]>) -> Result<Self, GarryError> {
        let txn = db.begin_transaction()?;
        let (ptr, len) = match prefix {
            Some(p) => (p.as_ptr(), p.len() as i32),
            None => (std::ptr::null(), 0),
        };
        let handle = unsafe { ffi::garry_cursor_open(db.handle, txn.txn, ptr, len) };
        if handle.is_null() {
            unsafe { ffi::garry_txn_rollback(db.handle, txn.txn); }
            return Err(GarryError::NullPointer);
        }
        Ok(Cursor { handle, db: db.handle, txn: txn.txn })
    }

    pub fn next(&self) -> Result<Option<(Vec<u8>, Vec<u8>)>, GarryError> {
        let mut key_buf = vec![0u8; ffi::GARRY_MAX_KEY_SIZE];
        let mut key_len: i32 = ffi::GARRY_MAX_KEY_SIZE as i32;
        let mut val_buf = vec![0u8; 16384];
        let mut val_len: i32 = 16384;

        let result = unsafe {
            ffi::garry_cursor_next(
                self.handle,
                key_buf.as_mut_ptr(),
                &mut key_len,
                val_buf.as_mut_ptr(),
                &mut val_len,
            )
        };

        if result == ffi::GARRY_FALSE {
            return Ok(None);
        }

        key_buf.truncate(key_len as usize);
        val_buf.truncate(val_len as usize);
        Ok(Some((key_buf, val_buf)))
    }

    pub fn next_key(&self) -> Result<Option<Vec<u8>>, GarryError> {
        let mut key_buf = vec![0u8; ffi::GARRY_MAX_KEY_SIZE];
        let mut key_len: i32 = ffi::GARRY_MAX_KEY_SIZE as i32;

        let result = unsafe {
            ffi::garry_cursor_next_key(
                self.handle,
                key_buf.as_mut_ptr(),
                &mut key_len,
            )
        };

        if result == ffi::GARRY_FALSE {
            return Ok(None);
        }

        key_buf.truncate(key_len as usize);
        Ok(Some(key_buf))
    }
}

impl Drop for Cursor {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe {
                ffi::garry_cursor_close(self.handle);
            }
        }
        unsafe {
            ffi::garry_txn_commit(self.db, self.txn);
        }
    }
}

#[derive(Debug, Clone)]
pub struct DatabaseConfig {
    pub pool_size: i32,
    pub max_record_size: i32,
    pub page_size: i32,
    pub max_txns: i32,
    pub max_versions: i32,
    pub max_key_size: i32,
    pub max_subscripts: i32,
    pub compression: i32,
    pub btree_flags: i32,
}

impl Default for DatabaseConfig {
    fn default() -> Self {
        Self {
            pool_size: 8,
            max_record_size: 16384,
            page_size: 4096,
            max_txns: 4,
            max_versions: 64,
            max_key_size: 256,
            max_subscripts: 16,
            compression: 0,
            btree_flags: 0,
        }
    }
}

impl DatabaseConfig {
    fn to_ffi(&self) -> ffi::garry_config {
        ffi::garry_config {
            pool_size: self.pool_size,
            max_record_size: self.max_record_size,
            page_size: self.page_size,
            max_txns: self.max_txns,
            max_versions: self.max_versions,
            max_key_size: self.max_key_size,
            max_subscripts: self.max_subscripts,
            compression: self.compression,
            btree_flags: self.btree_flags,
        }
    }

    fn from_ffi(config: ffi::garry_config) -> Self {
        Self {
            pool_size: config.pool_size,
            max_record_size: config.max_record_size,
            page_size: config.page_size,
            max_txns: config.max_txns,
            max_versions: config.max_versions,
            max_key_size: config.max_key_size,
            max_subscripts: config.max_subscripts,
            compression: config.compression,
            btree_flags: config.btree_flags,
        }
    }
}

pub use serializer::{decode_key, encode_key};

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_error_display() {
        assert_eq!(GarryError::NotFound.to_string(), "key not found");
        assert_eq!(GarryError::Io.to_string(), "I/O error");
    }
}
