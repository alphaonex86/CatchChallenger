#ifndef CATCHCHALLENGER_GSF_HPP
#define CATCHCHALLENGER_GSF_HPP

// GSF / MiniGSF (GBA Sound Format) container — the established PSF-family format
// for GBA MP2k music, so the files our converter emits play in standard GSF
// players (foobar2000 foo_input_gsf, Highly Advanced, VIOGSF) AND in our own
// client (which decodes them with the software MP2k synth in Mp2kPlayer, with NO
// GBA CPU emulation — it reads the song data the gsflib carries and synthesises).
//
// Container (Neill Corlett PSF v1.4, version byte 0x22), all little-endian:
//   0x00 "PSF"        (3 bytes ASCII)
//   0x03 0x22         (version = GSF)
//   0x04 u32 R        (reserved-area size; 0 for GSF)
//   0x08 u32 N        (compressed-program length)
//   0x0C u32 CRC32    (zlib crc32 of the N COMPRESSED bytes)
//   0x10 R bytes      (reserved area; absent when R==0)
//   0x10+R N bytes    (zlib compress() of the program)
//   0x10+R+N "[TAG]" + key=value lines (LF-separated), to EOF
// Program (after inflate): u32 EntryPoint, u32 LoadOffset, u32 Size, then Size
// bytes (the GBA ROM image for a gsflib; the song-id word for a minigsf).

#include <cstdint>
#include <string>
#include <vector>

namespace CatchChallenger {

// Build a GSF program block (12-byte header {entry, loadOffset, size} + data).
std::vector<uint8_t> gsfBuildProgram(uint32_t entry, uint32_t loadOffset,
                                     const uint8_t *data, uint32_t dataSize);

// PSF-wrap `program` (zlib + crc32) and write it with the tag text appended
// (tags = "key=value\n..."; may be empty).  Returns false on I/O/zlib failure.
bool gsfWriteFile(const std::string &path, const std::vector<uint8_t> &program,
                  const std::string &tags);

// Read+inflate a GSF file: validates "PSF"+0x22, inflates the program, and
// returns the tag text (everything after "[TAG]").  A CRC mismatch is warned
// (std::cerr) but not fatal (project rule: handle, don't abort).  False only on
// a structurally-malformed file or inflate failure.
bool gsfReadFile(const std::string &path, std::vector<uint8_t> &programOut,
                 std::string &tagsOut);

// Split a program block into its header fields + a view of the data (the data
// pointer aliases into `program`).  False if the block is too short.
bool gsfSplitProgram(const std::vector<uint8_t> &program, uint32_t &entry,
                     uint32_t &loadOffset, const uint8_t *&data, uint32_t &dataSize);

// One tag value ("" if absent); key matched case-insensitively.
std::string gsfTag(const std::string &tags, const std::string &key);

} // namespace CatchChallenger

#endif
