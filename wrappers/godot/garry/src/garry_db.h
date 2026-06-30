#ifndef GARRT_DB_H
#define GARRT_DB_H

#include <gdextension_interface.h>
#include <stdint.h>
#include <stddef.h>

struct garry_database;

void garry_db_register(void);
void garry_db_unregister(void);

typedef struct MeowDBData {
    GDExtensionObjectPtr owner;
    struct garry_database *db;
    char path[1024];
} MeowDBData;

MeowDBData *garry_db_get_data(GDExtensionObjectPtr obj);

#endif /* GARRT_DB_H */
