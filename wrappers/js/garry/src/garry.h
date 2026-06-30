#ifndef GARRY_NAPI_H
#define GARRY_NAPI_H

#include <napi.h>

extern "C" {
    typedef int garry_status_t;
    typedef int garry_bool;
    typedef int garry_i32;
    typedef unsigned char garry_u8;

    #define GARRY_OK 0
    #define GARRY_ERR_NOT_FOUND 1
    #define GARRY_ERR_INVALID_ARG 5
    #define GARRY_DATA_HAS_VALUE 10
    #define GARRY_DATA_HAS_BOTH 11
    #define GARRY_MAX_KEY_SIZE 256
    #define GARRY_MAX_RECORD_SIZE 16384

    typedef struct { int pool_size; int max_record_size; int page_size; int max_txns; int max_versions; int max_key_size; int max_subscripts; int compression; int btree_flags; } garry_config;

    typedef struct garry_database garry_database;
    typedef int garry_txn;
    typedef struct garry_cursor garry_cursor;

    garry_config garry_config_default(void);
    garry_database* garry_database_create(const char* path);
    garry_database* garry_database_create_with_config(const char* path, garry_config config);
    garry_database* garry_database_open(const char* path);
    void garry_database_close(garry_database* db);

    garry_txn garry_txn_begin(garry_database* db);
    void garry_txn_commit(garry_database* db, garry_txn txn);
    void garry_txn_rollback(garry_database* db, garry_txn txn);

    garry_status_t garry_get(garry_database* db, garry_txn txn, const garry_u8* key, garry_i32 klen, garry_u8* value, garry_i32* vlen);
    garry_status_t garry_set(garry_database* db, garry_txn txn, const garry_u8* key, garry_i32 klen, const garry_u8* value, garry_i32 vlen);
    garry_status_t garry_delete(garry_database* db, garry_txn txn, const garry_u8* key, garry_i32 klen);
    garry_i32 garry_data(garry_database* db, garry_txn txn, const garry_u8* key, garry_i32 klen);

    garry_cursor* garry_cursor_open(garry_database* db, garry_txn txn, const garry_u8* prefix, garry_i32 plen);
    garry_bool garry_cursor_next(garry_cursor* cur, garry_u8* key, garry_i32* klen, garry_u8* value, garry_i32* vlen);
    void garry_cursor_close(garry_cursor* cur);

    const char* garry_strerror(garry_status_t status);
}

class GarryDatabase : public Napi::ObjectWrap<GarryDatabase> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    GarryDatabase(const Napi::CallbackInfo& info);
    ~GarryDatabase();

    garry_database* GetHandle() { return db_; }

private:
    garry_database* db_;
    bool closed_;

    Napi::Value Get(const Napi::CallbackInfo& info);
    Napi::Value Set(const Napi::CallbackInfo& info);
    Napi::Value Delete(const Napi::CallbackInfo& info);
    Napi::Value Exists(const Napi::CallbackInfo& info);
    Napi::Value GetData(const Napi::CallbackInfo& info);
    Napi::Value BeginTransaction(const Napi::CallbackInfo& info);
    Napi::Value OpenCursor(const Napi::CallbackInfo& info);
    void Close(const Napi::CallbackInfo& info);

    void ThrowIfClosed();
};

class GarryTransaction : public Napi::ObjectWrap<GarryTransaction> {
public:
    static Napi::Function ctor;
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    GarryTransaction(const Napi::CallbackInfo& info);

private:
    garry_database* db_;
    garry_txn txn_;
    bool committed_;
    bool rolled_back_;

    Napi::Value Get(const Napi::CallbackInfo& info);
    Napi::Value Set(const Napi::CallbackInfo& info);
    Napi::Value Delete(const Napi::CallbackInfo& info);
    Napi::Value Exists(const Napi::CallbackInfo& info);
    Napi::Value GetData(const Napi::CallbackInfo& info);
    void Commit(const Napi::CallbackInfo& info);
    void Rollback(const Napi::CallbackInfo& info);

    void ThrowIfFinished();
};

class GarryCursor : public Napi::ObjectWrap<GarryCursor> {
public:
    static Napi::Function ctor;
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    GarryCursor(const Napi::CallbackInfo& info);

private:
    garry_cursor* cursor_;
    garry_database* db_;
    garry_txn txn_;

    Napi::Value Next(const Napi::CallbackInfo& info);
    void Close(const Napi::CallbackInfo& info);
};

#endif
