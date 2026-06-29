/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_txn.h
 * @brief Transaction lifecycle wrappers (begin, commit, rollback).
 */

#ifndef GARRY_STORAGE_TXN_H
#define GARRY_STORAGE_TXN_H

#include "transaction.h"

garry_txn_id garry_storage_begin(garry_engine_handle *eng);
void garry_storage_commit(garry_engine_handle *eng, garry_txn_id txn);
void garry_storage_rollback(garry_engine_handle *eng, garry_txn_id txn);

#endif /* GARRY_STORAGE_TXN_H */
