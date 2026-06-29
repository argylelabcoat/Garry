/**
 * @file export.h
 * @brief DLL/shared-library export macro for the Garry public API.
 *
 * Defines the @ref GARRY_API symbol that controls visibility of public
 * functions when building or consuming the Garry library as a shared
 * library (DLL on Windows, .so/.dylib on Unix).
 *
 * Intent: Provide a single, portable macro that marks every public
 * function for correct linkage regardless of platform or build mode.
 * Rationale: Without this, symbols are stripped from DLLs on Windows and
 * may not be visible from consumers on other platforms.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_EXPORT_H
#define GARRY_EXPORT_H

/**
 * @def GARRY_API
 * @brief Marks a function as part of the Garry public API.
 *
 * Usage:
 * - **Building the Garry DLL** — define @c GARRY_BUILDING_DLL so symbols
 *   are exported (@c dllexport / @c visibility("default")).
 * - **Consuming the Garry DLL** — define @c GARRY_DLL so symbols are
 *   imported (@c dllimport). On Unix this is a no-op.
 * - **Static linking** — neither macro is needed; @ref GARRY_API resolves
 *   to nothing.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef GARRY_BUILDING_DLL
#define GARRY_API __declspec(dllexport)
#elif defined(GARRY_DLL)
#define GARRY_API __declspec(dllimport)
#else
#define GARRY_API
#endif
#else
#if __GNUC__ >= 4
#define GARRY_API __attribute__((visibility("default")))
#else
#define GARRY_API
#endif
#endif

#endif /* GARRY_EXPORT_H */
