#ifndef GARRT_GDEXT_H
#define GARRT_GDEXT_H

#include <gdextension_interface.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#define GARRT_SN_SIZE   8
#define GARRT_STR_SIZE  8
#define GARRT_VAR_SIZE  40

typedef uint8_t GarrySN[GARRT_SN_SIZE];
typedef uint8_t GarryStr[GARRT_STR_SIZE];
typedef uint8_t GarryVar[GARRT_VAR_SIZE];

#define GARRT_PROPERTY_HINT_NONE    0
#define GARRT_PROPERTY_USAGE_DEFAULT 6

extern GDExtensionClassLibraryPtr garry_lib;

typedef struct GarryGdextInterface {
    GDExtensionInterfaceMemAlloc    mem_alloc;
    GDExtensionInterfaceMemFree     mem_free;

    GDExtensionInterfaceVariantDestroy              variant_destroy;
    GDExtensionInterfaceVariantGetType              variant_get_type;
    GDExtensionInterfaceVariantCall                 variant_call;
    GDExtensionInterfaceVariantBooleanize           variant_booleanize;

    GDExtensionInterfaceGetVariantFromTypeConstructor get_variant_from_type_constructor;
    GDExtensionInterfaceGetVariantToTypeConstructor   get_variant_to_type_constructor;

    GDExtensionInterfaceStringNewWithUtf8Chars      string_new_with_utf8_chars;
    GDExtensionInterfaceStringToUtf8Chars           string_to_utf8_chars;

    GDExtensionInterfaceStringNameNewWithUtf8Chars  sn_new_with_utf8_chars;

    GDExtensionInterfaceVariantGetPtrDestructor     variant_get_ptr_destructor;
    GDExtensionPtrDestructor                        str_destructor;
    GDExtensionPtrDestructor                        sn_destructor;

    GDExtensionInterfaceClassdbRegisterExtensionClass5          classdb_register_class;
    GDExtensionInterfaceClassdbRegisterExtensionClassMethod     classdb_register_method;
    GDExtensionInterfaceClassdbRegisterExtensionClassSignal     classdb_register_signal;
    GDExtensionInterfaceClassdbRegisterExtensionClassProperty    classdb_register_property;
    GDExtensionInterfaceClassdbUnregisterExtensionClass         classdb_unregister_class;
    GDExtensionInterfaceClassdbConstructObject2                 classdb_construct_object;
    GDExtensionInterfaceClassdbGetMethodBind                    classdb_get_method_bind;

    GDExtensionInterfaceObjectSetInstance           object_set_instance;
    GDExtensionInterfaceObjectGetInstanceBinding    object_get_instance_binding;
    GDExtensionInterfaceObjectSetInstanceBinding    object_set_instance_binding;
    GDExtensionInterfaceObjectMethodBindCall         object_method_bind_call;
    GDExtensionInterfaceObjectMethodBindPtrcall     object_method_bind_ptrcall;

    GDExtensionInterfaceCallableCustomCreate2       callable_custom_create;

    GDExtensionInterfaceGlobalGetSingleton         global_get_singleton;

    GDExtensionInterfacePrintError   print_error;
} GarryGdextInterface;

extern GarryGdextInterface ggi;

void garry_gdext_init(GDExtensionInterfaceGetProcAddress get_proc);

static inline void garry_sn_new(GarrySN out, const char *cstr) {
    ggi.sn_new_with_utf8_chars((GDExtensionStringNamePtr)out, cstr);
}
static inline void garry_sn_free(GarrySN sn) {
    if (ggi.sn_destructor) ggi.sn_destructor((GDExtensionTypePtr)sn);
}

static inline void garry_str_new(GarryStr out, const char *cstr) {
    ggi.string_new_with_utf8_chars((GDExtensionStringPtr)out, cstr);
}
static inline void garry_str_free(GarryStr s) {
    if (ggi.str_destructor) ggi.str_destructor((GDExtensionTypePtr)s);
}
static inline int garry_str_to_c(const GarryStr s, char *buf, int max_len) {
    return (int)ggi.string_to_utf8_chars((GDExtensionConstStringPtr)s, buf, (GDExtensionInt)max_len);
}

static inline void garry_var_free(GarryVar v) {
    ggi.variant_destroy((GDExtensionVariantPtr)v);
}

void garry_var_from_object(GarryVar out, GDExtensionObjectPtr obj);
void garry_ensure_ctors(void);
void garry_var_from_str(GarryVar out, const GarryStr s);
void garry_var_from_string_name(GarryVar out, const GarrySN sn);
void garry_var_from_cstr(GarryVar out, const char *cstr);
void garry_var_from_int(GarryVar out, int64_t v);
void garry_var_from_bool(GarryVar out, int v);
void garry_var_nil(GarryVar out);

void garry_vcall(GarryVar obj_var, const char *method,
                 const GarryVar *args, int argc, GarryVar *result);
void garry_vcall0(GDExtensionObjectPtr obj, const char *method);
void garry_vcall_str(GDExtensionObjectPtr obj, const char *method, const char *arg);
void garry_vcall_int(GDExtensionObjectPtr obj, const char *method, int64_t arg);
void garry_vcall_bool(GDExtensionObjectPtr obj, const char *method, int arg);

int garry_var_to_str(const GarryVar var, char *buf, int max_len);

/**
 * @brief Convert a Godot virtual path to an actual file system path.
 *
 * Calls ProjectSettings.globalize_path() to convert user:// and res://
 * paths to absolute file system paths.
 *
 * @param godot_path Input path from Godot (e.g., "user://game.db")
 * @param out Output buffer for converted path
 * @param out_size Size of output buffer
 * @return 1 on success, 0 on failure
 */
int garry_globalize_path(const char *godot_path, char *out, int out_size);

void garry_log(const char *msg);
void garry_err(const char *msg);

#endif /* GARRT_GDEXT_H */
