/* config.h for embedded libopus build (no autoconf). */

#ifndef OPUS_CONFIG_H
#define OPUS_CONFIG_H

/* Build as shared/static library */
#define OPUS_BUILD 1

/* Use floating point */
/* #undef FIXED_POINT */
#define FLOAT_APPROX 1

/* Use C99 variable-size arrays */
#define VAR_ARRAYS 1

/* Use alloca on MSVC where VLAs are not available */
/* #define USE_ALLOCA 1 */

/* Standard headers */
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1

/* Math functions */
#define HAVE_LRINT 1
#define HAVE_LRINTF 1

/* Restrict keyword */
#if defined(__GNUC__) || defined(__clang__)
/* GCC/Clang support restrict natively in C99+ mode */
#elif defined(_MSC_VER)
#define restrict __restrict
#else
#define restrict
#endif

/* Package info */
#define PACKAGE_VERSION "embedded"

#endif /* OPUS_CONFIG_H */
