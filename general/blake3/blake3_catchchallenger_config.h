/*
 * blake3_catchchallenger_config.h
 *
 * CatchChallenger-specific BLAKE3 build configuration.
 * Disables every architecture-specific SIMD path (x86 SSE/AVX
 * and ARM NEON) for maximum portability — i486, Geode LX800,
 * MIPS2, RISC-V, ARM with no NEON, and Android arm64 builds
 * where the upstream NEON source file (blake3_neon.c) is not
 * vendored. With every path disabled, blake3 falls back to
 * blake3_portable.c which compiles everywhere.
 *
 * This header is included from blake3_impl.h so the defines
 * take effect without needing compiler -D flags.
 */

#ifndef BLAKE3_CATCHCHALLENGER_CONFIG_H
#define BLAKE3_CATCHCHALLENGER_CONFIG_H

#define BLAKE3_NO_SSE2
#define BLAKE3_NO_SSE41
#define BLAKE3_NO_AVX2
#define BLAKE3_NO_AVX512
/* Force NEON off so AArch64 (Android arm64, etc.) doesn't link against
 * the missing blake3_hash_many_neon symbol. */
#define BLAKE3_USE_NEON 0

#endif /* BLAKE3_CATCHCHALLENGER_CONFIG_H */
