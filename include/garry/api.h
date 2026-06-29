/**
 * @file api.h
 * @brief Umbrella header for the Garry storage engine public API.
 *
 * Including this single header pulls in every public Garry header:
 * types, encoding, database, transaction, operations, cursor,
 * navigation, string convenience, and iteration.
 *
 * Intent: Provide a one-stop include for callers that want the full
 * Garry API without tracking individual headers.
 * Rationale: Reduces include-list maintenance — new public functions
 * are automatically available without changing application includes.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_API_H
#define GARRY_API_H

#include "garry/types.h"
#include "garry/encoding.h"
#include "garry/database.h"
#include "garry/transaction.h"
#include "garry/operations.h"
#include "garry/cursor.h"
#include "garry/navigation.h"
#include "garry/garry_string.h"
#include "garry/iterator.h"

#endif /* GARRY_API_H */
