#include "garry.h"
#include <cstring>

static void CheckStatus(Napi::Env env, garry_status_t status) {
    if (status == GARRY_OK) return;
    const char* msg = garry_strerror(status);
    Napi::Error::New(env, msg).ThrowAsJavaScriptException();
}

// --- Module-level constructors ---

Napi::Function GarryTransaction::ctor;
Napi::Function GarryCursor::ctor;

// --- GarryDatabase ---

Napi::Object GarryDatabase::Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "GarryDatabase", {
        InstanceMethod("get", &GarryDatabase::Get),
        InstanceMethod("set", &GarryDatabase::Set),
        InstanceMethod("delete", &GarryDatabase::Delete),
        InstanceMethod("exists", &GarryDatabase::Exists),
        InstanceMethod("getData", &GarryDatabase::GetData),
        InstanceMethod("beginTransaction", &GarryDatabase::BeginTransaction),
        InstanceMethod("openCursor", &GarryDatabase::OpenCursor),
        InstanceMethod("close", &GarryDatabase::Close),
    });
    exports.Set("GarryDatabase", func);
    return exports;
}

GarryDatabase::GarryDatabase(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<GarryDatabase>(info), db_(nullptr), closed_(false) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String path required").ThrowAsJavaScriptException();
        return;
    }
    std::string path = info[0].As<Napi::String>().Utf8Value();

    if (info.Length() >= 2 && info[1].IsObject()) {
        Napi::Object cfg = info[1].As<Napi::Object>();
        garry_config config = garry_config_default();
        if (cfg.Has("pool_size")) config.pool_size = cfg.Get("pool_size").As<Napi::Number>().Int32Value();
        if (cfg.Has("max_record_size")) config.max_record_size = cfg.Get("max_record_size").As<Napi::Number>().Int32Value();
        if (cfg.Has("page_size")) config.page_size = cfg.Get("page_size").As<Napi::Number>().Int32Value();
        if (cfg.Has("max_txns")) config.max_txns = cfg.Get("max_txns").As<Napi::Number>().Int32Value();
        if (cfg.Has("max_versions")) config.max_versions = cfg.Get("max_versions").As<Napi::Number>().Int32Value();
        if (cfg.Has("max_key_size")) config.max_key_size = cfg.Get("max_key_size").As<Napi::Number>().Int32Value();
        if (cfg.Has("max_subscripts")) config.max_subscripts = cfg.Get("max_subscripts").As<Napi::Number>().Int32Value();
        if (cfg.Has("compression")) config.compression = cfg.Get("compression").As<Napi::Number>().Int32Value();
        if (cfg.Has("btree_flags")) config.btree_flags = cfg.Get("btree_flags").As<Napi::Number>().Int32Value();
        db_ = garry_database_create_with_config(path.c_str(), config);
    } else if (info.Length() >= 2 && info[1].IsBoolean() && !info[1].As<Napi::Boolean>().Value()) {
        db_ = garry_database_open(path.c_str());
    } else {
        db_ = garry_database_create(path.c_str());
    }

    if (!db_) {
        Napi::Error::New(env, "Failed to create database").ThrowAsJavaScriptException();
    }
}

GarryDatabase::~GarryDatabase() {
    if (db_ && !closed_) {
        garry_database_close(db_);
        db_ = nullptr;
        closed_ = true;
    }
}

void GarryDatabase::ThrowIfClosed() {
    if (closed_) {
        Napi::Error::New(Env(), "Database is closed").ThrowAsJavaScriptException();
    }
}

Napi::Value GarryDatabase::Get(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfClosed();
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key required").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();
    garry_i32 klen = keyBuf.Length();

    garry_u8 valBuf[GARRY_MAX_RECORD_SIZE];
    garry_i32 vlen = GARRY_MAX_RECORD_SIZE;
    garry_txn txn = garry_txn_begin(db_);
    garry_status_t st = garry_get(db_, txn, keyBuf.Data(), klen, valBuf, &vlen);
    garry_txn_rollback(db_, txn);
    if (st == GARRY_ERR_NOT_FOUND) return env.Null();
    if (st != GARRY_OK) { CheckStatus(env, st); return env.Undefined(); }
    return Napi::Buffer<uint8_t>::Copy(env, valBuf, vlen);
}

Napi::Value GarryDatabase::Set(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfClosed();
    if (info.Length() < 2 || !info[0].IsBuffer() || !info[1].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key and value required").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();
    Napi::Buffer<uint8_t> valBuf = info[1].As<Napi::Buffer<uint8_t>>();

    garry_txn txn = garry_txn_begin(db_);
    garry_status_t st = garry_set(db_, txn, keyBuf.Data(), keyBuf.Length(), valBuf.Data(), valBuf.Length());
    if (st == GARRY_OK) garry_txn_commit(db_, txn);
    else garry_txn_rollback(db_, txn);
    CheckStatus(env, st);
    return env.Undefined();
}

Napi::Value GarryDatabase::Delete(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfClosed();
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key required").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();

    garry_txn txn = garry_txn_begin(db_);
    garry_status_t st = garry_delete(db_, txn, keyBuf.Data(), keyBuf.Length());
    if (st == GARRY_OK) garry_txn_commit(db_, txn);
    else garry_txn_rollback(db_, txn);
    CheckStatus(env, st);
    return env.Undefined();
}

Napi::Value GarryDatabase::Exists(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfClosed();
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key required").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();
    garry_txn txn = garry_txn_begin(db_);
    garry_i32 data_type = garry_data(db_, txn, keyBuf.Data(), keyBuf.Length());
    garry_txn_commit(db_, txn);
    return Napi::Boolean::New(env, data_type == GARRY_DATA_HAS_VALUE || data_type == GARRY_DATA_HAS_BOTH);
}

Napi::Value GarryDatabase::GetData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfClosed();
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key required").ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();
    garry_txn txn = garry_txn_begin(db_);
    garry_i32 data_type = garry_data(db_, txn, keyBuf.Data(), keyBuf.Length());
    garry_txn_commit(db_, txn);
    return Napi::Number::New(env, data_type);
}

Napi::Value GarryDatabase::BeginTransaction(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfClosed();
    return GarryTransaction::ctor.New({ Napi::External<garry_database>::New(env, db_) });
}

Napi::Value GarryDatabase::OpenCursor(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfClosed();
    const garry_u8* prefix = nullptr;
    garry_i32 plen = 0;
    Napi::Buffer<uint8_t> prefixBuf;
    if (info.Length() >= 1 && info[0].IsBuffer()) {
        prefixBuf = info[0].As<Napi::Buffer<uint8_t>>();
        prefix = prefixBuf.Data();
        plen = prefixBuf.Length();
    }
    garry_txn txn = garry_txn_begin(db_);
    garry_cursor* cur = garry_cursor_open(db_, txn, prefix, plen);
    if (!cur) {
        garry_txn_rollback(db_, txn);
        Napi::Error::New(env, "Failed to open cursor").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    return GarryCursor::ctor.New({ Napi::External<garry_cursor>::New(env, cur), Napi::External<garry_database>::New(env, db_), Napi::Number::New(env, txn) });
}

void GarryDatabase::Close(const Napi::CallbackInfo& info) {
    if (db_ && !closed_) {
        garry_database_close(db_);
        db_ = nullptr;
        closed_ = true;
    }
}

// --- GarryTransaction ---

Napi::Object GarryTransaction::Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "GarryTransaction", {
        InstanceMethod("get", &GarryTransaction::Get),
        InstanceMethod("set", &GarryTransaction::Set),
        InstanceMethod("delete", &GarryTransaction::Delete),
        InstanceMethod("exists", &GarryTransaction::Exists),
        InstanceMethod("getData", &GarryTransaction::GetData),
        InstanceMethod("commit", &GarryTransaction::Commit),
        InstanceMethod("rollback", &GarryTransaction::Rollback),
    });
    ctor = func;
    exports.Set("GarryTransaction", func);
    return exports;
}

GarryTransaction::GarryTransaction(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<GarryTransaction>(info), db_(nullptr), txn_(0), committed_(false), rolled_back_(false) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Internal error: expected database handle").ThrowAsJavaScriptException();
        return;
    }
    db_ = info[0].As<Napi::External<garry_database>>().Data();
    txn_ = garry_txn_begin(db_);
}

void GarryTransaction::ThrowIfFinished() {
    if (committed_ || rolled_back_) {
        Napi::Error::New(Env(), "Transaction already finished").ThrowAsJavaScriptException();
    }
}

Napi::Value GarryTransaction::Get(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfFinished();
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key required").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();
    garry_i32 klen = keyBuf.Length();

    garry_u8 valBuf[GARRY_MAX_RECORD_SIZE];
    garry_i32 vlen = GARRY_MAX_RECORD_SIZE;
    garry_status_t st = garry_get(db_, txn_, keyBuf.Data(), klen, valBuf, &vlen);
    if (st == GARRY_ERR_NOT_FOUND) return env.Null();
    if (st != GARRY_OK) { CheckStatus(env, st); return env.Undefined(); }
    return Napi::Buffer<uint8_t>::Copy(env, valBuf, vlen);
}

Napi::Value GarryTransaction::Set(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfFinished();
    if (info.Length() < 2 || !info[0].IsBuffer() || !info[1].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key and value required").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();
    Napi::Buffer<uint8_t> valBuf = info[1].As<Napi::Buffer<uint8_t>>();
    garry_status_t st = garry_set(db_, txn_, keyBuf.Data(), keyBuf.Length(), valBuf.Data(), valBuf.Length());
    CheckStatus(env, st);
    return env.Undefined();
}

Napi::Value GarryTransaction::Delete(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfFinished();
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key required").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();
    garry_status_t st = garry_delete(db_, txn_, keyBuf.Data(), keyBuf.Length());
    CheckStatus(env, st);
    return env.Undefined();
}

Napi::Value GarryTransaction::Exists(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfFinished();
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key required").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();
    garry_i32 data_type = garry_data(db_, txn_, keyBuf.Data(), keyBuf.Length());
    return Napi::Boolean::New(env, data_type == GARRY_DATA_HAS_VALUE || data_type == GARRY_DATA_HAS_BOTH);
}

Napi::Value GarryTransaction::GetData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    ThrowIfFinished();
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer key required").ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }
    Napi::Buffer<uint8_t> keyBuf = info[0].As<Napi::Buffer<uint8_t>>();
    garry_i32 data_type = garry_data(db_, txn_, keyBuf.Data(), keyBuf.Length());
    return Napi::Number::New(env, data_type);
}

void GarryTransaction::Commit(const Napi::CallbackInfo& info) {
    if (committed_ || rolled_back_) return;
    garry_txn_commit(db_, txn_);
    committed_ = true;
}

void GarryTransaction::Rollback(const Napi::CallbackInfo& info) {
    if (committed_ || rolled_back_) return;
    garry_txn_rollback(db_, txn_);
    rolled_back_ = true;
}

// --- GarryCursor ---

Napi::Object GarryCursor::Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "GarryCursor", {
        InstanceMethod("next", &GarryCursor::Next),
        InstanceMethod("close", &GarryCursor::Close),
    });
    ctor = func;
    exports.Set("GarryCursor", func);
    return exports;
}

GarryCursor::GarryCursor(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<GarryCursor>(info), cursor_(nullptr), db_(nullptr), txn_(0) {
    Napi::Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsExternal() || !info[1].IsExternal() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Internal error: expected cursor, db, and txn").ThrowAsJavaScriptException();
        return;
    }
    cursor_ = info[0].As<Napi::External<garry_cursor>>().Data();
    db_ = info[1].As<Napi::External<garry_database>>().Data();
    txn_ = info[2].As<Napi::Number>().Int32Value();
}

Napi::Value GarryCursor::Next(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!cursor_) {
        Napi::Error::New(env, "Cursor is closed").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    garry_u8 keyBuf[GARRY_MAX_KEY_SIZE];
    garry_u8 valBuf[GARRY_MAX_RECORD_SIZE];
    garry_i32 klen = GARRY_MAX_KEY_SIZE;
    garry_i32 vlen = GARRY_MAX_RECORD_SIZE;

    garry_bool has_more = garry_cursor_next(cursor_, keyBuf, &klen, valBuf, &vlen);
    if (!has_more) return env.Null();

    Napi::Object result = Napi::Object::New(env);
    result.Set("key", Napi::Buffer<uint8_t>::Copy(env, keyBuf, klen));
    result.Set("value", Napi::Buffer<uint8_t>::Copy(env, valBuf, vlen));
    return result;
}

void GarryCursor::Close(const Napi::CallbackInfo& info) {
    if (cursor_) {
        garry_cursor_close(cursor_);
        cursor_ = nullptr;
    }
    if (db_ && txn_) {
        garry_txn_commit(db_, txn_);
        db_ = nullptr;
        txn_ = 0;
    }
}

// --- Module init ---

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    GarryDatabase::Init(env, exports);
    GarryTransaction::Init(env, exports);
    GarryCursor::Init(env, exports);
    return exports;
}

NODE_API_MODULE(garry, Init)
