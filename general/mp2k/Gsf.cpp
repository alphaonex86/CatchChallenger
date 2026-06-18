#include "Gsf.hpp"

#include <zlib.h>
#include <cstring>
#include <fstream>
#include <iostream>

namespace CatchChallenger {

static void put32(std::vector<uint8_t> &b, uint32_t v)
{ b.push_back(v & 0xFF); b.push_back((v >> 8) & 0xFF); b.push_back((v >> 16) & 0xFF); b.push_back((v >> 24) & 0xFF); }
static uint32_t get32(const uint8_t *p)
{ return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

std::vector<uint8_t> gsfBuildProgram(uint32_t entry, uint32_t loadOffset,
                                     const uint8_t *data, uint32_t dataSize)
{
    std::vector<uint8_t> p;
    p.reserve(12 + dataSize);
    put32(p, entry); put32(p, loadOffset); put32(p, dataSize);
    if(data != NULL && dataSize > 0)
        p.insert(p.end(), data, data + dataSize);
    return p;
}

bool gsfSplitProgram(const std::vector<uint8_t> &program, uint32_t &entry,
                     uint32_t &loadOffset, const uint8_t *&data, uint32_t &dataSize)
{
    if(program.size() < 12)
        return false;
    entry = get32(program.data() + 0);
    loadOffset = get32(program.data() + 4);
    dataSize = get32(program.data() + 8);
    // Clamp to what is actually present (a truncated file must not over-read).
    if((uint64_t)12 + dataSize > program.size())
        dataSize = (uint32_t)(program.size() - 12);
    data = program.data() + 12;
    return true;
}

bool gsfWriteFile(const std::string &path, const std::vector<uint8_t> &program,
                  const std::string &tags)
{
    // zlib-compress the program.
    uLongf bound = compressBound((uLong)program.size());
    std::vector<uint8_t> comp(bound);
    int zr = compress2(comp.data(), &bound, program.data(), (uLong)program.size(), 9);
    if(zr != Z_OK)
    {
        std::cerr << "gsfWriteFile: zlib compress failed (" << zr << ") for " << path << std::endl;
        return false;
    }
    comp.resize(bound);
    const uint32_t crc = (uint32_t)crc32(crc32(0L, Z_NULL, 0), comp.data(), (uInt)comp.size());

    std::ofstream f(path, std::ios::binary);
    if(!f)
    {
        std::cerr << "gsfWriteFile: cannot open " << path << std::endl;
        return false;
    }
    std::vector<uint8_t> hdr;
    hdr.push_back('P'); hdr.push_back('S'); hdr.push_back('F'); hdr.push_back(0x22);
    put32(hdr, 0);                          // R = reserved size (none)
    put32(hdr, (uint32_t)comp.size());      // N = compressed length
    put32(hdr, crc);                        // CRC32 of the compressed bytes
    f.write(reinterpret_cast<const char *>(hdr.data()), (std::streamsize)hdr.size());
    f.write(reinterpret_cast<const char *>(comp.data()), (std::streamsize)comp.size());
    if(!tags.empty())
    {
        static const char marker[5] = { '[', 'T', 'A', 'G', ']' };
        f.write(marker, 5);
        f.write("\n", 1);
        f.write(tags.data(), (std::streamsize)tags.size());
    }
    return (bool)f;
}

// Streaming inflate into a growing buffer (the uncompressed size is not in the
// PSF header; a gsflib's program is whole-ROM-sized, so grow in chunks).
static bool inflateAll(const uint8_t *src, size_t srcLen, std::vector<uint8_t> &out)
{
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));
    if(inflateInit(&zs) != Z_OK)
        return false;
    zs.next_in = const_cast<Bytef *>(src);
    zs.avail_in = (uInt)srcLen;
    out.clear();
    std::vector<uint8_t> chunk(1 << 20); // 1 MiB
    int zr = Z_OK;
    while(zr != Z_STREAM_END)
    {
        zs.next_out = chunk.data();
        zs.avail_out = (uInt)chunk.size();
        zr = inflate(&zs, Z_NO_FLUSH);
        if(zr != Z_OK && zr != Z_STREAM_END)
        {
            inflateEnd(&zs);
            return false;
        }
        out.insert(out.end(), chunk.data(), chunk.data() + (chunk.size() - zs.avail_out));
    }
    inflateEnd(&zs);
    return true;
}

bool gsfReadFile(const std::string &path, std::vector<uint8_t> &programOut, std::string &tagsOut)
{
    tagsOut.clear();
    std::ifstream f(path, std::ios::binary);
    if(!f)
    {
        std::cerr << "gsfReadFile: cannot open " << path << std::endl;
        return false;
    }
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if(buf.size() < 16 || !(buf[0] == 'P' && buf[1] == 'S' && buf[2] == 'F'))
    {
        std::cerr << "gsfReadFile: not a PSF file: " << path << std::endl;
        return false;
    }
    if(buf[3] != 0x22)
        std::cerr << "gsfReadFile: unexpected PSF version 0x" << std::hex << (int)buf[3]
                  << std::dec << " (expected 0x22 GSF) in " << path << std::endl;
    const uint32_t R = get32(buf.data() + 4);
    const uint32_t N = get32(buf.data() + 8);
    const uint32_t crc = get32(buf.data() + 12);
    const size_t compOff = (size_t)16 + R;
    if(compOff + N > buf.size())
    {
        std::cerr << "gsfReadFile: truncated program in " << path << std::endl;
        return false;
    }
    const uint32_t got = (uint32_t)crc32(crc32(0L, Z_NULL, 0), buf.data() + compOff, (uInt)N);
    if(got != crc)
        std::cerr << "gsfReadFile: CRC mismatch in " << path << " (stored 0x" << std::hex
                  << crc << " got 0x" << got << std::dec << ") — continuing" << std::endl;
    if(!inflateAll(buf.data() + compOff, N, programOut))
    {
        std::cerr << "gsfReadFile: inflate failed for " << path << std::endl;
        return false;
    }
    // Tags: "[TAG]" marker right after the compressed program.
    const size_t tagOff = compOff + N;
    if(tagOff + 5 <= buf.size() && std::memcmp(buf.data() + tagOff, "[TAG]", 5) == 0)
    {
        const char *s = reinterpret_cast<const char *>(buf.data() + tagOff + 5);
        tagsOut.assign(s, buf.size() - (tagOff + 5));
    }
    return true;
}

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }

std::string gsfTag(const std::string &tags, const std::string &key)
{
    size_t i = 0;
    while(i < tags.size())
    {
        size_t eol = tags.find('\n', i);
        if(eol == std::string::npos)
            eol = tags.size();
        size_t eq = tags.find('=', i);
        if(eq != std::string::npos && eq < eol)
        {
            // trim the key (whitespace around it is ignored)
            size_t ks = i, ke = eq;
            while(ks < ke && (tags[ks] == ' ' || tags[ks] == '\t' || tags[ks] == '\r')) ++ks;
            while(ke > ks && (tags[ke - 1] == ' ' || tags[ke - 1] == '\t' || tags[ke - 1] == '\r')) --ke;
            bool match = (ke - ks) == key.size();
            size_t k = 0;
            while(match && k < key.size()) { if(lc(tags[ks + k]) != lc(key[k])) match = false; ++k; }
            if(match)
            {
                size_t vs = eq + 1, ve = eol;
                while(vs < ve && (tags[vs] == ' ' || tags[vs] == '\t' || tags[vs] == '\r')) ++vs;
                while(ve > vs && (tags[ve - 1] == ' ' || tags[ve - 1] == '\t' || tags[ve - 1] == '\r')) --ve;
                return tags.substr(vs, ve - vs);
            }
        }
        i = eol + 1;
    }
    return std::string();
}

} // namespace CatchChallenger
