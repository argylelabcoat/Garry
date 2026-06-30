#include "gdext.h"
#include "garry_db.h"
#include <string.h>
#include <stdlib.h>

GDExtensionClassLibraryPtr garry_lib = NULL;
GarryGdextInterface ggi = {0};

#define LOAD(name, symbol) \
    ggi.name = (GDExtensionInterfaceGetProcAddress)(get_proc(symbol)); \
    if (!ggi.name) { fprintf(stderr, "[garry] missing: " symbol "\n"); }

void garry_gdext_init(GDExtensionInterfaceGetProcAddress get_proc) {
    ggi.mem_alloc = (GDExtensionInterfaceMemAlloc)get_proc("mem_alloc");
    ggi.mem_free  = (GDExtensionInterfaceMemFree)get_proc("mem_free");

    ggi.variant_destroy             = (GDExtensionInterfaceVariantDestroy)get_proc("variant_destroy");
    ggi.variant_get_type            = (GDExtensionInterfaceVariantGetType)get_proc("variant_get_type");
    ggi.variant_call                = (GDExtensionInterfaceVariantCall)get_proc("variant_call");
    ggi.variant_booleanize          = (GDExtensionInterfaceVariantBooleanize)get_proc("variant_booleanize");
    ggi.get_variant_from_type_constructor = (GDExtensionInterfaceGetVariantFromTypeConstructor)get_proc("get_variant_from_type_constructor");
    ggi.get_variant_to_type_constructor   = (GDExtensionInterfaceGetVariantToTypeConstructor)get_proc("get_variant_to_type_constructor");

    ggi.string_new_with_utf8_chars  = (GDExtensionInterfaceStringNewWithUtf8Chars)get_proc("string_new_with_utf8_chars");
    ggi.string_to_utf8_chars        = (GDExtensionInterfaceStringToUtf8Chars)get_proc("string_to_utf8_chars");

    ggi.sn_new_with_utf8_chars      = (GDExtensionInterfaceStringNameNewWithUtf8Chars)get_proc("string_name_new_with_utf8_chars");

    ggi.variant_get_ptr_destructor  = (GDExtensionInterfaceVariantGetPtrDestructor)get_proc("variant_get_ptr_destructor");
    if (ggi.variant_get_ptr_destructor) {
        ggi.str_destructor = ggi.variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING);
        ggi.sn_destructor  = ggi.variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    }

    ggi.classdb_register_class      = (GDExtensionInterfaceClassdbRegisterExtensionClass5)get_proc("classdb_register_extension_class5");
    ggi.classdb_register_method     = (GDExtensionInterfaceClassdbRegisterExtensionClassMethod)get_proc("classdb_register_extension_class_method");
    ggi.classdb_register_signal     = (GDExtensionInterfaceClassdbRegisterExtensionClassSignal)get_proc("classdb_register_extension_class_signal");
    ggi.classdb_register_property   = (GDExtensionInterfaceClassdbRegisterExtensionClassProperty)get_proc("classdb_register_extension_class_property");
    ggi.classdb_unregister_class    = (GDExtensionInterfaceClassdbUnregisterExtensionClass)get_proc("classdb_unregister_extension_class");
    ggi.classdb_construct_object    = (GDExtensionInterfaceClassdbConstructObject2)get_proc("classdb_construct_object2");
    ggi.classdb_get_method_bind     = (GDExtensionInterfaceClassdbGetMethodBind)get_proc("classdb_get_method_bind");

    ggi.object_set_instance         = (GDExtensionInterfaceObjectSetInstance)get_proc("object_set_instance");
    ggi.object_get_instance_binding = (GDExtensionInterfaceObjectGetInstanceBinding)get_proc("object_get_instance_binding");
    ggi.object_set_instance_binding = (GDExtensionInterfaceObjectSetInstanceBinding)get_proc("object_set_instance_binding");
    ggi.object_method_bind_call     = (GDExtensionInterfaceObjectMethodBindCall)get_proc("object_method_bind_call");
    ggi.object_method_bind_ptrcall = (GDExtensionInterfaceObjectMethodBindPtrcall)get_proc("object_method_bind_ptrcall");

    ggi.callable_custom_create      = (GDExtensionInterfaceCallableCustomCreate2)get_proc("callable_custom_create2");

    ggi.print_error   = (GDExtensionInterfacePrintError)get_proc("print_error");

    if (!ggi.mem_alloc)                        fprintf(stderr, "[garry] missing: mem_alloc\n");
    if (!ggi.variant_destroy)                  fprintf(stderr, "[garry] missing: variant_destroy\n");
    if (!ggi.string_new_with_utf8_chars)       fprintf(stderr, "[garry] missing: string_new_with_utf8_chars\n");
    if (!ggi.sn_new_with_utf8_chars)           fprintf(stderr, "[garry] missing: string_name_new_with_utf8_chars\n");
    if (!ggi.classdb_register_class)           fprintf(stderr, "[garry] missing: classdb_register_extension_class5\n");
    if (!ggi.classdb_construct_object)         fprintf(stderr, "[garry] missing: classdb_construct_object2\n");
    if (!ggi.object_method_bind_call)         fprintf(stderr, "[garry] missing: object_method_bind_call\n");
    if (!ggi.callable_custom_create)           fprintf(stderr, "[garry] missing: callable_custom_create2\n");
}

#undef LOAD

static GDExtensionVariantFromTypeConstructorFunc ctor_from_object = NULL;
static GDExtensionVariantFromTypeConstructorFunc ctor_from_string = NULL;
static GDExtensionVariantFromTypeConstructorFunc ctor_from_string_name = NULL;
static GDExtensionVariantFromTypeConstructorFunc ctor_from_int    = NULL;
static GDExtensionVariantFromTypeConstructorFunc ctor_from_bool   = NULL;

static GDExtensionTypeFromVariantConstructorFunc ctor_to_string = NULL;
static GDExtensionTypeFromVariantConstructorFunc ctor_to_int    = NULL;

void garry_ensure_ctors(void) {
    if (!ctor_from_object) {
        ctor_from_object = ggi.get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_OBJECT);
        ctor_from_string = ggi.get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_STRING);
        ctor_from_string_name = ggi.get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        ctor_from_int    = ggi.get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_INT);
        ctor_from_bool   = ggi.get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_BOOL);
        ctor_to_string   = ggi.get_variant_to_type_constructor(GDEXTENSION_VARIANT_TYPE_STRING);
        ctor_to_int      = ggi.get_variant_to_type_constructor(GDEXTENSION_VARIANT_TYPE_INT);
    }
}

void garry_var_from_object(GarryVar out, GDExtensionObjectPtr obj) {
    garry_ensure_ctors();
    ctor_from_object((GDExtensionVariantPtr)out, obj);
}

void garry_var_from_str(GarryVar out, const GarryStr s) {
    garry_ensure_ctors();
    ctor_from_string((GDExtensionVariantPtr)out, (void *)s);
}

void garry_var_from_cstr(GarryVar out, const char *cstr) {
    GarryStr s;
    garry_str_new(s, cstr);
    garry_var_from_str(out, s);
    garry_str_free(s);
}

void garry_var_from_string_name(GarryVar out, const GarrySN sn) {
    garry_ensure_ctors();
    ctor_from_string_name((GDExtensionVariantPtr)out, (const void *)sn);
}

void garry_var_from_int(GarryVar out, int64_t v) {
    garry_ensure_ctors();
    ctor_from_int((GDExtensionVariantPtr)out, &v);
}

void garry_var_from_bool(GarryVar out, int v) {
    garry_ensure_ctors();
    GDExtensionBool b = (GDExtensionBool)v;
    ctor_from_bool((GDExtensionVariantPtr)out, &b);
}

void garry_var_nil(GarryVar out) {
    memset(out, 0, GARRT_VAR_SIZE);
}

void garry_vcall(GarryVar obj_var, const char *method,
                 const GarryVar *args, int argc, GarryVar *result) {
    GarrySN mn;
    garry_sn_new(mn, method);

    const GDExtensionConstVariantPtr *arg_ptrs = NULL;
    GDExtensionConstVariantPtr static_args[16];
    GDExtensionConstVariantPtr *dyn_args = NULL;

    if (argc > 0) {
        if (argc <= 16) {
            for (int i = 0; i < argc; i++) {
                static_args[i] = (GDExtensionConstVariantPtr)args[i];
            }
            arg_ptrs = static_args;
        } else {
            dyn_args = (GDExtensionConstVariantPtr *)malloc(argc * sizeof(GDExtensionConstVariantPtr));
            for (int i = 0; i < argc; i++) {
                dyn_args[i] = (GDExtensionConstVariantPtr)args[i];
            }
            arg_ptrs = (const GDExtensionConstVariantPtr *)dyn_args;
        }
    }

    GarryVar tmp_result;
    memset(tmp_result, 0, GARRT_VAR_SIZE);
    GDExtensionCallError err;
    ggi.variant_call(
        (GDExtensionVariantPtr)obj_var,
        (GDExtensionConstStringNamePtr)mn,
        arg_ptrs,
        (GDExtensionInt)argc,
        result ? (GDExtensionVariantPtr)result : (GDExtensionVariantPtr)tmp_result,
        &err
    );

    if (err.error != GDEXTENSION_CALL_OK) {
        fprintf(stderr, "[garry] variant_call '%s' error: %d\n", method, err.error);
    }

    garry_sn_free(mn);
    if (dyn_args) free(dyn_args);
}

void garry_vcall0(GDExtensionObjectPtr obj, const char *method) {
    GarryVar ov;
    garry_var_from_object(ov, obj);
    garry_vcall(ov, method, NULL, 0, NULL);
    garry_var_free(ov);
}

void garry_vcall_str(GDExtensionObjectPtr obj, const char *method, const char *arg) {
    GarryVar ov, av;
    GarryVar args[1];
    garry_var_from_object(ov, obj);
    garry_var_from_cstr(av, arg);
    memcpy(args[0], av, GARRT_VAR_SIZE);
    garry_vcall(ov, method, args, 1, NULL);
    garry_var_free(av);
    garry_var_free(ov);
}

void garry_vcall_int(GDExtensionObjectPtr obj, const char *method, int64_t arg) {
    GarryVar ov, av;
    GarryVar args[1];
    garry_var_from_object(ov, obj);
    garry_var_from_int(av, arg);
    memcpy(args[0], av, GARRT_VAR_SIZE);
    garry_vcall(ov, method, args, 1, NULL);
    garry_var_free(av);
    garry_var_free(ov);
}

void garry_vcall_bool(GDExtensionObjectPtr obj, const char *method, int arg) {
    GarryVar ov, av;
    GarryVar args[1];
    garry_var_from_object(ov, obj);
    garry_var_from_bool(av, arg);
    memcpy(args[0], av, GARRT_VAR_SIZE);
    garry_vcall(ov, method, args, 1, NULL);
    garry_var_free(av);
    garry_var_free(ov);
}

int garry_var_to_str(const GarryVar var, char *buf, int max_len) {
    garry_ensure_ctors();
    GarryStr s;
    ctor_to_string(s, (GDExtensionVariantPtr)var);
    int len = garry_str_to_c(s, buf, max_len);
    garry_str_free(s);
    return len;
}

void garry_log(const char *msg) {
    fprintf(stderr, "[garry] %s\n", msg);
}

void garry_err(const char *msg) {
    ggi.print_error(msg, "", "", 0, 0);
}

static int db_registered = 0;

static void garry_initialize(void *userdata, GDExtensionInitializationLevel level) {
    (void)userdata;
    if (level == GDEXTENSION_INITIALIZATION_SCENE && !db_registered) {
        garry_db_register();
        db_registered = 1;
    }
}

static void garry_uninitialize(void *userdata, GDExtensionInitializationLevel level) {
    (void)userdata;
    if (level == GDEXTENSION_INITIALIZATION_SCENE && db_registered) {
        garry_db_unregister();
        db_registered = 0;
    }
}

GDExtensionBool gdextension_library_init(
    GDExtensionInterfaceGetProcAddress get_proc,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization *r_init)
{
    garry_lib = library;
    garry_gdext_init(get_proc);

    r_init->minimum_initialization_level = GDEXTENSION_INITIALIZATION_SCENE;
    r_init->userdata                      = NULL;
    r_init->initialize                    = garry_initialize;
    r_init->deinitialize                  = garry_uninitialize;

    return 1;
}
