#ifndef GARRT_DB_H
#define GARRT_DB_H

#include <gdextension_interface.h>
#include <stdint.h>
#include <stddef.h>

struct garry_database;
typedef int32_t garry_txn;

void garry_db_register(void);
void garry_db_unregister(void);

typedef struct MeowDBData {
    GDExtensionObjectPtr owner;
    struct garry_database *db;
    garry_txn txn_handle;
    int txn_active;
    char path[1024];
} MeowDBData;

MeowDBData *garry_db_get_data(GDExtensionObjectPtr obj);

#endif /* GARRT_DB_H */
