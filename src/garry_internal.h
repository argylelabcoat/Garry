/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file garry_internal.h
 * @brief Internal database handle definition.
 *
 * Encapsulates the engine handle behind an opaque public type so that
 * internal structure details are hidden from API consumers.
 */

#ifndef GARRY_INTERNAL_H
#define GARRY_INTERNAL_H

#include "garry/types.h"
#include "transaction.h"

/** @brief Opaque database handle wrapping the engine instance. */
struct garry_database {
    garry_engine_handle *eng;  /**< Underlying storage engine. */
};

#endif /* GARRY_INTERNAL_H */
