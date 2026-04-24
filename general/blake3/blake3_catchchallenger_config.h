/*
 * blake3_catchchallenger_config.h
 *
 * CatchChallenger-specific BLAKE3 build configuration.
 * Disables x86 SIMD extensions for maximum portability
 * (i486, Geode LX800, MIPS2, RISC-V, etc.).
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

#endif /* BLAKE3_CATCHCHALLENGER_CONFIG_H */
