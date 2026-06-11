/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Visibility macros for the public API.
 */

#ifndef GSNI_VISIBILITY_H
#define GSNI_VISIBILITY_H

#if defined(_WIN32) || defined(__CYGWIN__)
  #define GSNI_EXPORT __declspec(dllexport)
  #define GSNI_IMPORT __declspec(dllimport)
#else
  #if __GNUC__ >= 4
    #define GSNI_EXPORT __attribute__((visibility("default")))
    #define GSNI_IMPORT
  #else
    #define GSNI_EXPORT
    #define GSNI_IMPORT
  #endif
#endif

#ifndef GSNI_API
  #ifdef GSNI_COMPILATION
    #define GSNI_API GSNI_EXPORT
  #else
    #define GSNI_API GSNI_IMPORT
  #endif
#endif

#endif /* GSNI_VISIBILITY_H */
