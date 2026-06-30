#include "garry_db.h"
#include "gdext.h"
#include <garry/api.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static GarrySN sn_MeowDB;
static GarrySN sn_Node;

static void *db_binding_create(void *token, void *instance) {
    (void)token;
    return instance;
}

static void db_binding_free(void *token, void *instance, void *binding) {
    (void)token;
    (void)instance;
    (void)binding;
}

static GDExtensionBool db_binding_reference_callback(void *token,
                                                      void *instance,
                                                      GDExtensionBool reference) {
    (void)token;
    (void)instance;
    (void)reference;
    return 1;
}

static const GDExtensionInstanceBindingCallbacks db_binding_callbacks = {
    db_binding_create,
    db_binding_free,
    db_binding_reference_callback
};

static GDExtensionObjectPtr db_create_instance(void *class_userdata, GDExtensionBool p_notify_postinitialize) {
    (void)class_userdata;
    (void)p_notify_postinitialize;
    GDExtensionObjectPtr obj = ggi.classdb_construct_object(
        (GDExtensionConstStringNamePtr)sn_Node);

    MeowDBData *data = (MeowDBData *)calloc(1, sizeof(MeowDBData));
    data->owner = obj;
    data->db = NULL;

    ggi.object_set_instance(obj,
        (GDExtensionConstStringNamePtr)sn_MeowDB,
        (GDExtensionClassInstancePtr)data);

    ggi.object_set_instance_binding(obj, garry_lib, data, &db_binding_callbacks);

    return obj;
}

static void db_free_instance(void *class_userdata,
                              GDExtensionClassInstancePtr instance) {
    (void)class_userdata;
    MeowDBData *data = (MeowDBData *)instance;
    if (!data) return;
    if (data->db) {
        if (data->txn_active) {
            garry_txn_rollback(data->db, data->txn_handle);
            data->txn_active = 0;
        }
        garry_database_close(data->db);
        data->db = NULL;
    }
    free(data);
}

MeowDBData *garry_db_get_data(GDExtensionObjectPtr obj) {
    if (!obj) return NULL;
    return (MeowDBData *)ggi.object_get_instance_binding(obj, garry_lib, &db_binding_callbacks);
}

static void extract_string_arg(const GDExtensionConstVariantPtr *args, int idx,
                                char *buf, int max_len) {
    GarryVar arg;
    memcpy(arg, args[idx], GARRT_VAR_SIZE);
    GarryStr s;
    ggi.get_variant_to_type_constructor(GDEXTENSION_VARIANT_TYPE_STRING)(s, (GDExtensionVariantPtr)arg);
    garry_str_to_c(s, buf, max_len);
    garry_str_free(s);
}

/* --- open(path: String) -> bool --- */

static void db_open_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata;
    r_error->error = GDEXTENSION_CALL_OK;
    if (argc < 1) { r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS; return; }
    MeowDBData *d = (MeowDBData *)instance;
    if (!d) { garry_var_from_bool((uint8_t *)r_return, 0); return; }

    if (d->db) {
        if (d->txn_active) {
            garry_txn_rollback(d->db, d->txn_handle);
            d->txn_active = 0;
        }
        garry_database_close(d->db);
        d->db = NULL;
    }

    char path[1024] = {0};
    extract_string_arg(args, 0, path, (int)sizeof(path) - 1);

    d->db = garry_database_open(path);
    if (!d->db) {
        d->db = garry_database_create(path);
    }

    if (d->db) {
        snprintf(d->path, sizeof(d->path), "%s", path);
    }

    garry_var_from_bool((uint8_t *)r_return, d->db ? 1 : 0);
}

static void db_open_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !args || !args[0]) {
        *(GDExtensionBool *)r_return = 0;
        return;
    }

    if (d->db) {
        if (d->txn_active) {
            garry_txn_rollback(d->db, d->txn_handle);
            d->txn_active = 0;
        }
        garry_database_close(d->db);
        d->db = NULL;
    }

    char path[1024] = {0};
    garry_str_to_c((const uint8_t *)args[0], path, (int)sizeof(path) - 1);

    d->db = garry_database_open(path);
    if (!d->db) {
        d->db = garry_database_create(path);
    }

    if (d->db) {
        snprintf(d->path, sizeof(d->path), "%s", path);
    }

    *(GDExtensionBool *)r_return = d->db ? 1 : 0;
}

/* --- close() -> bool --- */

static void db_close_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata; (void)args; (void)argc;
    r_error->error = GDEXTENSION_CALL_OK;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) {
        garry_var_from_bool((uint8_t *)r_return, 0);
        return;
    }
    garry_database_close(d->db);
    d->db = NULL;
    garry_var_from_bool((uint8_t *)r_return, 1);
}

static void db_close_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata; (void)args;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) {
        *(GDExtensionBool *)r_return = 0;
        return;
    }
    garry_database_close(d->db);
    d->db = NULL;
    *(GDExtensionBool *)r_return = 1;
}

/* --- get(key: String) -> PackedByteArray --- */

static void db_get_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata;
    r_error->error = GDEXTENSION_CALL_OK;
    if (argc < 1) { r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS; return; }
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) { garry_var_nil((uint8_t *)r_return); return; }

    char key[256] = {0};
    extract_string_arg(args, 0, key, (int)sizeof(key) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_u8 value[GARRY_MAX_RECORD_SIZE];
    garry_i32 vlen = (garry_i32)sizeof(value);
    garry_status_t rc = garry_get(d->db, txn, (const garry_u8 *)key, (garry_i32)strlen(key), value, &vlen);
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    if (rc == GARRY_OK && vlen > 0) {
        garry_ensure_ctors();
        GDExtensionVariantFromTypeConstructorFunc ctor_from_packed =
            ggi.get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY);
        if (ctor_from_packed) {
            uint8_t *byte_ptr = value;
            ctor_from_packed((GDExtensionVariantPtr)r_return, &byte_ptr);
        } else {
            garry_var_nil((uint8_t *)r_return);
        }
    } else {
        garry_var_nil((uint8_t *)r_return);
    }
}

static void db_get_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !args || !args[0]) {
        memset(r_return, 0, GARRT_VAR_SIZE);
        return;
    }

    char key[256] = {0};
    garry_str_to_c((const uint8_t *)args[0], key, (int)sizeof(key) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_u8 value[GARRY_MAX_RECORD_SIZE];
    garry_i32 vlen = (garry_i32)sizeof(value);
    garry_status_t rc = garry_get(d->db, txn, (const garry_u8 *)key, (garry_i32)strlen(key), value, &vlen);
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    if (rc == GARRY_OK && vlen > 0) {
        garry_ensure_ctors();
        GDExtensionVariantFromTypeConstructorFunc ctor_from_packed =
            ggi.get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY);
        if (ctor_from_packed) {
            uint8_t *byte_ptr = value;
            ctor_from_packed((GDExtensionVariantPtr)r_return, &byte_ptr);
        } else {
            memset(r_return, 0, GARRT_VAR_SIZE);
        }
    } else {
        memset(r_return, 0, GARRT_VAR_SIZE);
    }
}

/* --- set(key: String, value: PackedByteArray) -> bool --- */

static void db_set_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata;
    r_error->error = GDEXTENSION_CALL_OK;
    if (argc < 2) { r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS; return; }
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) { garry_var_from_bool((uint8_t *)r_return, 0); return; }

    char key[256] = {0};
    extract_string_arg(args, 0, key, (int)sizeof(key) - 1);

    GarryVar val_var;
    memcpy(val_var, args[1], GARRT_VAR_SIZE);
    garry_ensure_ctors();
    GDExtensionTypeFromVariantConstructorFunc ctor_to_packed =
        ggi.get_variant_to_type_constructor(GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY);

    if (!ctor_to_packed) {
        garry_var_from_bool((uint8_t *)r_return, 0);
        return;
    }

    uint8_t packed_buf[16];
    ctor_to_packed(packed_buf, (GDExtensionVariantPtr)val_var);

    uint8_t *data_ptr = NULL;
    int64_t data_size = 0;
    memcpy(&data_ptr, packed_buf, sizeof(data_ptr));
    memcpy(&data_size, packed_buf + sizeof(data_ptr), sizeof(data_size));

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_status_t rc = garry_set(d->db, txn, (const garry_u8 *)key, (garry_i32)strlen(key),
                                   data_ptr, (garry_i32)data_size);
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    garry_var_from_bool((uint8_t *)r_return, rc == GARRY_OK ? 1 : 0);
}

static void db_set_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !args || !args[0] || !args[1]) {
        *(GDExtensionBool *)r_return = 0;
        return;
    }

    char key[256] = {0};
    garry_str_to_c((const uint8_t *)args[0], key, (int)sizeof(key) - 1);

    const uint8_t *packed_data = (const uint8_t *)args[1];
    uint8_t *data_ptr = NULL;
    int64_t data_size = 0;
    memcpy(&data_ptr, packed_data, sizeof(data_ptr));
    memcpy(&data_size, packed_data + sizeof(data_ptr), sizeof(data_size));

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_status_t rc = garry_set(d->db, txn, (const garry_u8 *)key, (garry_i32)strlen(key),
                                   data_ptr, (garry_i32)data_size);
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    *(GDExtensionBool *)r_return = rc == GARRY_OK ? 1 : 0;
}

/* --- delete(key: String) -> bool --- */

static void db_delete_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata;
    r_error->error = GDEXTENSION_CALL_OK;
    if (argc < 1) { r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS; return; }
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) { garry_var_from_bool((uint8_t *)r_return, 0); return; }

    char key[256] = {0};
    extract_string_arg(args, 0, key, (int)sizeof(key) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_status_t rc = garry_delete(d->db, txn, (const garry_u8 *)key, (garry_i32)strlen(key));
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    garry_var_from_bool((uint8_t *)r_return, rc == GARRY_OK ? 1 : 0);
}

static void db_delete_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !args || !args[0]) {
        *(GDExtensionBool *)r_return = 0;
        return;
    }

    char key[256] = {0};
    garry_str_to_c((const uint8_t *)args[0], key, (int)sizeof(key) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_status_t rc = garry_delete(d->db, txn, (const garry_u8 *)key, (garry_i32)strlen(key));
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    *(GDExtensionBool *)r_return = rc == GARRY_OK ? 1 : 0;
}

/* --- exists(key: String) -> bool --- */

static void db_exists_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata;
    r_error->error = GDEXTENSION_CALL_OK;
    if (argc < 1) { r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS; return; }
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) { garry_var_from_bool((uint8_t *)r_return, 0); return; }

    char key[256] = {0};
    extract_string_arg(args, 0, key, (int)sizeof(key) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_i32 data_type = garry_data(d->db, txn, (const garry_u8 *)key, (garry_i32)strlen(key));
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    garry_var_from_bool((uint8_t *)r_return, (data_type == GARRY_DATA_HAS_VALUE || data_type == GARRY_DATA_HAS_BOTH) ? 1 : 0);
}

static void db_exists_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !args || !args[0]) {
        *(GDExtensionBool *)r_return = 0;
        return;
    }

    char key[256] = {0};
    garry_str_to_c((const uint8_t *)args[0], key, (int)sizeof(key) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_i32 data_type = garry_data(d->db, txn, (const garry_u8 *)key, (garry_i32)strlen(key));
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    *(GDExtensionBool *)r_return = (data_type == GARRY_DATA_HAS_VALUE || data_type == GARRY_DATA_HAS_BOTH) ? 1 : 0;
}

/* --- get_string(key: String) -> String --- */

static void db_get_string_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata;
    r_error->error = GDEXTENSION_CALL_OK;
    if (argc < 1) { r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS; return; }
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) { garry_var_from_cstr((uint8_t *)r_return, ""); return; }

    char key[256] = {0};
    extract_string_arg(args, 0, key, (int)sizeof(key) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    char value[16384] = {0};
    garry_i32 vlen = (garry_i32)sizeof(value);
    garry_status_t rc = garry_get_str(d->db, txn, key, value, vlen);
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    if (rc == GARRY_OK) {
        garry_var_from_cstr((uint8_t *)r_return, value);
    } else {
        garry_var_from_cstr((uint8_t *)r_return, "");
    }
}

static void db_get_string_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !args || !args[0]) {
        garry_str_new((uint8_t *)r_return, "");
        return;
    }

    char key[256] = {0};
    garry_str_to_c((const uint8_t *)args[0], key, (int)sizeof(key) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    char value[16384] = {0};
    garry_i32 vlen = (garry_i32)sizeof(value);
    garry_status_t rc = garry_get_str(d->db, txn, key, value, vlen);
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    if (rc == GARRY_OK) {
        garry_str_new((uint8_t *)r_return, value);
    } else {
        garry_str_new((uint8_t *)r_return, "");
    }
}

/* --- set_string(key: String, value: String) -> bool --- */

static void db_set_string_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata;
    r_error->error = GDEXTENSION_CALL_OK;
    if (argc < 2) { r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS; return; }
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) { garry_var_from_bool((uint8_t *)r_return, 0); return; }

    char key[256] = {0};
    char value[16384] = {0};
    extract_string_arg(args, 0, key, (int)sizeof(key) - 1);
    extract_string_arg(args, 1, value, (int)sizeof(value) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_status_t rc = garry_set_str(d->db, txn, key, value);
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    garry_var_from_bool((uint8_t *)r_return, rc == GARRY_OK ? 1 : 0);
}

static void db_set_string_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !args || !args[0] || !args[1]) {
        *(GDExtensionBool *)r_return = 0;
        return;
    }

    char key[256] = {0};
    char value[16384] = {0};
    garry_str_to_c((const uint8_t *)args[0], key, (int)sizeof(key) - 1);
    garry_str_to_c((const uint8_t *)args[1], value, (int)sizeof(value) - 1);

    garry_txn txn = d->txn_active ? d->txn_handle : garry_txn_begin(d->db);
    garry_status_t rc = garry_set_str(d->db, txn, key, value);
    if (!d->txn_active) garry_txn_commit(d->db, txn);

    *(GDExtensionBool *)r_return = rc == GARRY_OK ? 1 : 0;
}

/* --- begin_transaction() -> void --- */

static void db_begin_transaction_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata; (void)args; (void)argc; (void)r_return;
    r_error->error = GDEXTENSION_CALL_OK;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) return;
    if (d->txn_active) return;
    d->txn_handle = garry_txn_begin(d->db);
    d->txn_active = 1;
}

static void db_begin_transaction_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata; (void)args; (void)r_return;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db) return;
    if (d->txn_active) return;
    d->txn_handle = garry_txn_begin(d->db);
    d->txn_active = 1;
}

/* --- commit() -> void --- */

static void db_commit_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata; (void)args; (void)argc; (void)r_return;
    r_error->error = GDEXTENSION_CALL_OK;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !d->txn_active) return;
    garry_txn_commit(d->db, d->txn_handle);
    d->txn_active = 0;
}

static void db_commit_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata; (void)args; (void)r_return;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !d->txn_active) return;
    garry_txn_commit(d->db, d->txn_handle);
    d->txn_active = 0;
}

/* --- rollback() -> void --- */

static void db_rollback_call(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstVariantPtr *args,
        GDExtensionInt argc,
        GDExtensionVariantPtr r_return,
        GDExtensionCallError *r_error) {
    (void)method_userdata; (void)args; (void)argc; (void)r_return;
    r_error->error = GDEXTENSION_CALL_OK;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !d->txn_active) return;
    garry_txn_rollback(d->db, d->txn_handle);
    d->txn_active = 0;
}

static void db_rollback_ptrcall(
        void *method_userdata,
        GDExtensionClassInstancePtr instance,
        const GDExtensionConstTypePtr *args,
        GDExtensionTypePtr r_return) {
    (void)method_userdata; (void)args; (void)r_return;
    MeowDBData *d = (MeowDBData *)instance;
    if (!d || !d->db || !d->txn_active) return;
    garry_txn_rollback(d->db, d->txn_handle);
    d->txn_active = 0;
}

/* =========================================================================
 * Method registration helpers (matching reference pattern)
 * ========================================================================= */

#define GARRT_PROPERTY_HINT_NONE    0
#define GARRT_PROPERTY_USAGE_DEFAULT 6

static void register_method_no_ret(
    const char *method_name,
    GDExtensionClassMethodCall call_fn,
    GDExtensionClassMethodPtrCall ptrcall_fn,
    int arg_count,
    const char **arg_names,
    GDExtensionVariantType *arg_types)
{
    GarrySN sn_method, ret_class_sn;
    garry_sn_new(sn_method, method_name);
    garry_sn_new(ret_class_sn, "");

    GarrySN arg_sn[8];
    GarrySN arg_class_sn[8];
    GarryStr empty_str;
    garry_str_new(empty_str, "");

    GDExtensionPropertyInfo arg_infos[8];
    GDExtensionClassMethodArgumentMetadata arg_meta[8];
    for (int i = 0; i < arg_count && i < 8; i++) {
        garry_sn_new(arg_sn[i], arg_names[i]);
        garry_sn_new(arg_class_sn[i], "");
        arg_infos[i].type       = arg_types[i];
        arg_infos[i].name       = (GDExtensionStringNamePtr)arg_sn[i];
        arg_infos[i].class_name = (GDExtensionStringNamePtr)arg_class_sn[i];
        arg_infos[i].hint       = GARRT_PROPERTY_HINT_NONE;
        arg_infos[i].hint_string = (GDExtensionStringPtr)empty_str;
        arg_infos[i].usage      = GARRT_PROPERTY_USAGE_DEFAULT;
        arg_meta[i]             = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    }
    GDExtensionPropertyInfo ret_info = {
        .type       = GDEXTENSION_VARIANT_TYPE_NIL,
        .name       = (GDExtensionStringNamePtr)sn_method,
        .class_name = (GDExtensionStringNamePtr)ret_class_sn,
        .hint       = GARRT_PROPERTY_HINT_NONE,
        .hint_string = (GDExtensionStringPtr)empty_str,
        .usage      = GARRT_PROPERTY_USAGE_DEFAULT
    };

    GDExtensionClassMethodInfo info = {
        .name                   = (GDExtensionStringNamePtr)sn_method,
        .method_userdata        = NULL,
        .call_func              = call_fn,
        .ptrcall_func           = ptrcall_fn,
        .method_flags           = GDEXTENSION_METHOD_FLAG_NORMAL,
        .has_return_value       = 0,
        .return_value_info      = &ret_info,
        .return_value_metadata  = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE,
        .argument_count         = (uint32_t)arg_count,
        .arguments_info         = arg_count > 0 ? arg_infos : NULL,
        .arguments_metadata     = arg_count > 0 ? arg_meta : NULL,
        .default_argument_count = 0,
        .default_arguments      = NULL,
    };

    ggi.classdb_register_method(garry_lib, (GDExtensionConstStringNamePtr)sn_MeowDB, &info);

    garry_sn_free(ret_class_sn);
    for (int i = 0; i < arg_count && i < 8; i++) {
        garry_sn_free(arg_class_sn[i]);
        garry_sn_free(arg_sn[i]);
    }
    garry_str_free(empty_str);
    garry_sn_free(sn_method);
}

static void register_method_ret(
    const char *method_name,
    GDExtensionVariantType ret_type,
    GDExtensionClassMethodCall call_fn,
    GDExtensionClassMethodPtrCall ptrcall_fn,
    int arg_count,
    const char **arg_names,
    GDExtensionVariantType *arg_types)
{
    GarrySN sn_method, sn_ret_name, sn_ret_class;
    garry_sn_new(sn_method, method_name);
    garry_sn_new(sn_ret_name, method_name);
    garry_sn_new(sn_ret_class, "");

    GarryStr empty_str;
    garry_str_new(empty_str, "");

    GarrySN arg_sn[8];
    GarrySN arg_class_sn[8];
    GDExtensionPropertyInfo arg_infos[8];
    GDExtensionClassMethodArgumentMetadata arg_meta[8];
    for (int i = 0; i < arg_count && i < 8; i++) {
        garry_sn_new(arg_sn[i], arg_names[i]);
        garry_sn_new(arg_class_sn[i], "");
        arg_infos[i].type        = arg_types[i];
        arg_infos[i].name        = (GDExtensionStringNamePtr)arg_sn[i];
        arg_infos[i].class_name  = (GDExtensionStringNamePtr)arg_class_sn[i];
        arg_infos[i].hint        = GARRT_PROPERTY_HINT_NONE;
        arg_infos[i].hint_string = (GDExtensionStringPtr)empty_str;
        arg_infos[i].usage       = GARRT_PROPERTY_USAGE_DEFAULT;
        arg_meta[i]              = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    }

    GDExtensionPropertyInfo ret_info = {
        .type        = ret_type,
        .name        = (GDExtensionStringNamePtr)sn_ret_name,
        .class_name  = (GDExtensionStringNamePtr)sn_ret_class,
        .hint        = GARRT_PROPERTY_HINT_NONE,
        .hint_string = (GDExtensionStringPtr)empty_str,
        .usage       = GARRT_PROPERTY_USAGE_DEFAULT
    };

    GDExtensionClassMethodInfo info = {
        .name                   = (GDExtensionStringNamePtr)sn_method,
        .method_userdata        = NULL,
        .call_func              = call_fn,
        .ptrcall_func           = ptrcall_fn,
        .method_flags           = GDEXTENSION_METHOD_FLAG_NORMAL,
        .has_return_value       = 1,
        .return_value_info      = &ret_info,
        .return_value_metadata  = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE,
        .argument_count         = (uint32_t)arg_count,
        .arguments_info         = arg_count > 0 ? arg_infos : NULL,
        .arguments_metadata     = arg_count > 0 ? arg_meta : NULL,
        .default_argument_count = 0,
        .default_arguments      = NULL,
    };

    ggi.classdb_register_method(garry_lib, (GDExtensionConstStringNamePtr)sn_MeowDB, &info);

    garry_sn_free(sn_ret_class);
    garry_sn_free(sn_ret_name);
    for (int i = 0; i < arg_count && i < 8; i++) {
        garry_sn_free(arg_class_sn[i]);
        garry_sn_free(arg_sn[i]);
    }
    garry_str_free(empty_str);
    garry_sn_free(sn_method);
}

/* =========================================================================
 * Class registration
 * ========================================================================= */

void garry_db_register(void) {
    garry_sn_new(sn_MeowDB, "MeowDB");
    garry_sn_new(sn_Node, "Node");

    /* Register the class */
    GDExtensionClassCreationInfo4 ci;
    memset(&ci, 0, sizeof(ci));
    ci.is_virtual         = 0;
    ci.is_abstract        = 0;
    ci.is_exposed         = 1;
    ci.is_runtime         = 0;
    ci.create_instance_func = db_create_instance;
    ci.free_instance_func   = db_free_instance;

    ggi.classdb_register_class(
        garry_lib,
        (GDExtensionConstStringNamePtr)sn_MeowDB,
        (GDExtensionConstStringNamePtr)sn_Node,
        &ci);

    /* --- Register methods --- */

    /* open(path: String) -> bool */
    {
        const char *arg_names[] = {"path"};
        GDExtensionVariantType arg_types[] = {GDEXTENSION_VARIANT_TYPE_STRING};
        register_method_ret("open",
            GDEXTENSION_VARIANT_TYPE_BOOL,
            db_open_call, db_open_ptrcall,
            1, arg_names, arg_types);
    }

    /* close() -> bool */
    register_method_ret("close",
        GDEXTENSION_VARIANT_TYPE_BOOL,
        db_close_call, db_close_ptrcall,
        0, NULL, NULL);

    /* get(key: String) -> PackedByteArray */
    {
        const char *arg_names[] = {"key"};
        GDExtensionVariantType arg_types[] = {GDEXTENSION_VARIANT_TYPE_STRING};
        register_method_ret("get",
            GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY,
            db_get_call, db_get_ptrcall,
            1, arg_names, arg_types);
    }

    /* set(key: String, value: PackedByteArray) -> bool */
    {
        const char *arg_names[] = {"key", "value"};
        GDExtensionVariantType arg_types[] = {
            GDEXTENSION_VARIANT_TYPE_STRING,
            GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY
        };
        register_method_ret("set",
            GDEXTENSION_VARIANT_TYPE_BOOL,
            db_set_call, db_set_ptrcall,
            2, arg_names, arg_types);
    }

    /* delete(key: String) -> bool */
    {
        const char *arg_names[] = {"key"};
        GDExtensionVariantType arg_types[] = {GDEXTENSION_VARIANT_TYPE_STRING};
        register_method_ret("delete",
            GDEXTENSION_VARIANT_TYPE_BOOL,
            db_delete_call, db_delete_ptrcall,
            1, arg_names, arg_types);
    }

    /* exists(key: String) -> bool */
    {
        const char *arg_names[] = {"key"};
        GDExtensionVariantType arg_types[] = {GDEXTENSION_VARIANT_TYPE_STRING};
        register_method_ret("exists",
            GDEXTENSION_VARIANT_TYPE_BOOL,
            db_exists_call, db_exists_ptrcall,
            1, arg_names, arg_types);
    }

    /* get_string(key: String) -> String */
    {
        const char *arg_names[] = {"key"};
        GDExtensionVariantType arg_types[] = {GDEXTENSION_VARIANT_TYPE_STRING};
        register_method_ret("get_string",
            GDEXTENSION_VARIANT_TYPE_STRING,
            db_get_string_call, db_get_string_ptrcall,
            1, arg_names, arg_types);
    }

    /* set_string(key: String, value: String) -> bool */
    {
        const char *arg_names[] = {"key", "value"};
        GDExtensionVariantType arg_types[] = {
            GDEXTENSION_VARIANT_TYPE_STRING,
            GDEXTENSION_VARIANT_TYPE_STRING
        };
        register_method_ret("set_string",
            GDEXTENSION_VARIANT_TYPE_BOOL,
            db_set_string_call, db_set_string_ptrcall,
            2, arg_names, arg_types);
    }

    /* begin_transaction() -> void */
    register_method_no_ret("begin_transaction",
        db_begin_transaction_call, db_begin_transaction_ptrcall,
        0, NULL, NULL);

    /* commit() -> void */
    register_method_no_ret("commit",
        db_commit_call, db_commit_ptrcall,
        0, NULL, NULL);

    /* rollback() -> void */
    register_method_no_ret("rollback",
        db_rollback_call, db_rollback_ptrcall,
        0, NULL, NULL);
}

void garry_db_unregister(void) {
    ggi.classdb_unregister_class(garry_lib,
        (GDExtensionConstStringNamePtr)sn_MeowDB);
    garry_sn_free(sn_Node);
    garry_sn_free(sn_MeowDB);
}
