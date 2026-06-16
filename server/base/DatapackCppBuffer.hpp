// DatapackCppBuffer.hpp — "datapack-cpp" flash-resident datapack/settings.
//
// Goal (ESP32 / no-filesystem): keep the whole datapack AND the server settings
// inside the executable as HUMAN-READABLE C++ const data living in .rodata
// (flash, XIP), read in place at runtime with NO filesystem, NO byte blob and
// NO HPS byte-deserialization. Entirely #ifdef-gated so every other build is
// byte-for-byte unchanged (no extra code, no extra cost).
//
// How it works — reuse, don't reinvent:
//   The server already serializes the datapack+settings via per-type
//   serialize()/parse() methods (HPS). Instead of driving them with a byte
//   stream, we drive them with two buffers that speak the SAME interface HPS
//   buffers do (operator<</operator>>, write/read, write_char/read_char):
//
//   * CppEmitBuffer  (stage1, build flag CATCHCHALLENGER_DATAPACK_CPP_EMIT)
//       intercepts every scalar at operator<< and records the DECODED value
//       (not varint bytes) into per-width vectors; aggregates/containers recurse
//       through the existing serializers. emitTo() then writes those vectors as
//       readable const arrays + a tiny accessor into datapack_cpp.{cpp,hpp}.
//
//   * CppReadBuffer  (stage2/ESP32, build flag CATCHCHALLENGER_DATAPACK_CPP)
//       is the mirror: operator>> pops the next decoded value from the const
//       flash arrays and feeds it to the SAME parse() methods. The bulk const
//       data stays in flash; only the runtime containers (small for the 'test'
//       maincode) are built in RAM, with their strings pointing into flash.
//
//   Scalars reached through a struct/top-level operator<< become readable typed
//   values (D_U64/D_I64/D_F64) and std::string become flash string literals
//   (D_STR). Container sizes/keys and raw POD-vector bytes go to D_RAW as-is;
//   they round-trip identically on both sides. Emit order == parse order, so the
//   per-array cursors stay in lock-step.

#ifndef CATCHCHALLENGER_DATAPACKCPPBUFFER_HPP
#define CATCHCHALLENGER_DATAPACKCPPBUFFER_HPP

#if defined(CATCHCHALLENGER_DATAPACK_CPP) || defined(CATCHCHALLENGER_DATAPACK_CPP_EMIT)

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include "../../general/hps/serializer.h"
// Needed so the Serializer<char,…> override below can delegate to the REAL
// signed-char (zigzag) serializer, not the generic t.parse() primary template.
#include "../../general/hps/basic_type/int_serializer.h"

namespace CatchChallenger {

// The const-array view the generated datapack_cpp.cpp hands to CppReadBuffer.
// Every pointer is into .rodata (flash); nothing here is copied.
struct DatapackCppData
{
    const uint64_t      *u64;   size_t u64n;   // unsigned scalars (+bool)
    const int64_t       *i64;   size_t i64n;   // signed scalars
    const double        *f64;   size_t f64n;   // float/double scalars
    const char * const  *str;   size_t strn;   // std::string values (flash literals)
    const uint32_t      *strlen;                // length of each str[] (embedded NUL safe)
    const unsigned char *raw;   size_t rawn;   // container sizes/keys + raw POD bytes
};

// Platform-INDEPENDENT signedness used to pick the i64 vs u64 array. Plain
// `char` has implementation-defined signedness (signed on x86/host, UNSIGNED on
// xtensa/ESP32 and ARM); routing it by std::is_signed<T> would send the SAME
// field to different arrays on emit (host) vs read (target) and desync the
// cursors. Treat plain `char` as signed everywhere (matching the host emitter),
// so the one baked stream is read identically on every architecture. All
// fixed-width types (int8_t/uint8_t/…) keep their own well-defined signedness.
template <class T>
constexpr bool dcpp_is_signed()
{
    return std::is_same<typename std::remove_cv<T>::type, char>::value
            ? true
            : std::is_signed<T>::value;
}

#ifdef CATCHCHALLENGER_DATAPACK_CPP
//---------------------------------------------------------------------------
// CppReadBuffer — runtime, reads the flash const arrays in place (no FS, no copy)
//---------------------------------------------------------------------------
class CppReadBuffer
{
public:
    explicit CppReadBuffer(const DatapackCppData &d) :
        data(d), uPos(0), iPos(0), fPos(0), sPos(0), rPos(0) {}

    // Raw bytes (container sizes via HPS varint, raw POD-vector payloads, token).
    void read(char *content, size_t length)
    {
        size_t i=0;
        while(i<length)
        {
            content[i]=(rPos<data.rawn)?static_cast<char>(data.raw[rPos]):0;
            rPos++;
            i++;
        }
    }
    char read_char()
    {
        const char c=(rPos<data.rawn)?static_cast<char>(data.raw[rPos]):0;
        rPos++;
        return c;
    }

    // Scalars: pop the decoded value from the matching typed array.
    //
    // The array choice MUST be platform-independent: the stream is emitted ONCE
    // on the host (where plain `char` is signed) and read back on EVERY target.
    // On xtensa (ESP32) and ARM, plain `char` is UNSIGNED, so a naive
    // std::is_signed<T> test would route `char` to a DIFFERENT array than emit
    // did → the i64/u64 cursors desync → every later field (incl. the datapack
    // hash) reads the wrong value. dcpp_is_signed<T>() pins plain `char` to the
    // signed array on all platforms so emit and read always agree.
    template <class T>
    typename std::enable_if<std::is_arithmetic<T>::value, CppReadBuffer&>::type
    operator>>(T &v)
    {
        if(std::is_floating_point<T>::value)
            v=static_cast<T>((fPos<data.f64n)?data.f64[fPos++]:0);
        else if(dcpp_is_signed<T>())
            v=static_cast<T>((iPos<data.i64n)?data.i64[iPos++]:0);
        else
            v=static_cast<T>((uPos<data.u64n)?data.u64[uPos++]:0);
        return *this;
    }
    template <class T>
    typename std::enable_if<!std::is_arithmetic<T>::value, CppReadBuffer&>::type
    operator>>(T &v)
    {
        hps::Serializer<T, CppReadBuffer>::parse(v, *this);
        return *this;
    }
    CppReadBuffer& operator>>(std::string &v)
    {
        if(sPos<data.strn)
        {
            v.assign(data.str[sPos], data.strlen[sPos]);
            sPos++;
        }
        else
            v.clear();
        return *this;
    }

    // debug-only; the load path prints serialBuffer->tellg() deltas.
    inline size_t tellg() const { return rPos; }

private:
    const DatapackCppData &data;
    size_t uPos,iPos,fPos,sPos,rPos;
};
#endif // CATCHCHALLENGER_DATAPACK_CPP

#ifdef CATCHCHALLENGER_DATAPACK_CPP_EMIT
//---------------------------------------------------------------------------
// CppEmitBuffer — stage1, records decoded values then writes datapack_cpp.cpp
//---------------------------------------------------------------------------
} // namespace CatchChallenger
#include <vector>
#include <fstream>
#include <iostream>
namespace CatchChallenger {

class CppEmitBuffer
{
public:
    void write(const char *content, size_t length)
    {
        size_t i=0;
        while(i<length){ raw.push_back(static_cast<unsigned char>(content[i])); i++; }
    }
    void write_char(const char c){ raw.push_back(static_cast<unsigned char>(c)); }

    template <class T>
    typename std::enable_if<std::is_arithmetic<T>::value, CppEmitBuffer&>::type
    operator<<(const T &v)
    {
        // Same platform-independent routing as CppReadBuffer (see dcpp_is_signed):
        // plain `char` always goes to the signed array so the read side, even on
        // an unsigned-char target (xtensa/ARM), pulls it back from the same array.
        if(std::is_floating_point<T>::value)
            f64.push_back(static_cast<double>(v));
        else if(dcpp_is_signed<T>())
            i64.push_back(static_cast<int64_t>(v));
        else
            u64.push_back(static_cast<uint64_t>(v));
        return *this;
    }
    template <class T>
    typename std::enable_if<!std::is_arithmetic<T>::value, CppEmitBuffer&>::type
    operator<<(const T &v)
    {
        hps::Serializer<T, CppEmitBuffer>::serialize(v, *this);
        return *this;
    }
    CppEmitBuffer& operator<<(const std::string &v)
    {
        str.push_back(v);
        return *this;
    }
    inline size_t tellp() const { return raw.size(); }

    // Write the recorded arrays + accessor into <dir>/datapack_cpp.{cpp,hpp}.
    bool emitTo(const std::string &dir) const
    {
        {
            std::ofstream h(dir+"/datapack_cpp.hpp",std::ofstream::binary|std::ofstream::trunc);
            if(!h.is_open()) return false;
            h << "// Generated by catchchallenger-server-cli --save"
                 " (CATCHCHALLENGER_DATAPACK_CPP_EMIT). Do not edit.\n"
                 "// The whole datapack + server settings as human-readable C++ const\n"
                 "// data; the stage2/ESP32 build reads it in place from flash (.rodata),\n"
                 "// no filesystem. See test/ESP32.md.\n"
                 "#ifndef DATAPACK_CPP_HPP\n#define DATAPACK_CPP_HPP\n"
                 "#include \"DatapackCppBuffer.hpp\"\n"
                 "namespace CatchChallenger { const DatapackCppData &datapack_cpp_data(); }\n"
                 "#endif\n";
        }
        std::ofstream o(dir+"/datapack_cpp.cpp",std::ofstream::binary|std::ofstream::trunc);
        if(!o.is_open()) return false;
        o << "// Generated by catchchallenger-server-cli --save"
             " (CATCHCHALLENGER_DATAPACK_CPP_EMIT). Do not edit.\n"
             "// Human-readable datapack+settings; lives in .rodata (flash/XIP), read in\n"
             "// place at runtime with no filesystem. See test/ESP32.md.\n"
             "#include \"datapack_cpp.hpp\"\n"
             "namespace CatchChallenger {\n";
        emitU64(o,"_dcpp_u64",u64);
        emitI64(o,"_dcpp_i64",i64);
        emitF64(o,"_dcpp_f64",f64);
        emitStr(o,"_dcpp_str","_dcpp_strlen",str);
        emitRaw(o,"_dcpp_raw",raw);
        o << "static const DatapackCppData _dcpp = {\n"
             "  _dcpp_u64, " << u64.size() << ",\n"
             "  _dcpp_i64, " << i64.size() << ",\n"
             "  _dcpp_f64, " << f64.size() << ",\n"
             "  _dcpp_str, " << str.size() << ",\n"
             "  _dcpp_strlen,\n"
             "  _dcpp_raw, " << raw.size() << "\n};\n"
             "const DatapackCppData &datapack_cpp_data(){ return _dcpp; }\n"
             "} // namespace CatchChallenger\n";
        std::cout << "datapack-cpp emit: " << dir << "/datapack_cpp.cpp  ("
                  << u64.size() << " u64, " << i64.size() << " i64, "
                  << f64.size() << " f64, " << str.size() << " str, "
                  << raw.size() << " raw bytes)" << std::endl;
        return o.good();
    }

private:
    std::vector<uint64_t> u64;
    std::vector<int64_t>  i64;
    std::vector<double>   f64;
    std::vector<std::string> str;
    std::vector<unsigned char> raw;

    static void emitU64(std::ostream &o,const char *name,const std::vector<uint64_t> &v)
    {
        o << "static const uint64_t " << name << "[]={";
        if(v.empty()) o << "0";   // C++ forbids a zero-size array
        size_t i=0;
        while(i<v.size()){ if(i%16==0)o<<"\n"; o<<v[i]<<"u,"; i++; }
        o << "};\n";
    }
    static void emitI64(std::ostream &o,const char *name,const std::vector<int64_t> &v)
    {
        o << "static const int64_t " << name << "[]={";
        if(v.empty()) o << "0";
        size_t i=0;
        while(i<v.size())
        {
            if(i%16==0)o<<"\n";
            // INT64_MIN can't be written as a literal (-9223372036854775808ll
            // parses as negation of an out-of-range positive); emit it safely.
            if(v[i]==INT64_MIN) o<<"(-9223372036854775807ll-1),";
            else o<<v[i]<<"ll,";
            i++;
        }
        o << "};\n";
    }
    static void emitF64(std::ostream &o,const char *name,const std::vector<double> &v)
    {
        o.precision(17);
        o << "static const double " << name << "[]={";
        if(v.empty()) o << "0";
        size_t i=0;
        while(i<v.size()){ if(i%8==0)o<<"\n"; o<<v[i]<<","; i++; }
        o << "};\n";
    }
    static void emitStr(std::ostream &o,const char *name,const char *lenName,
                        const std::vector<std::string> &v)
    {
        o << "static const char * const " << name << "[]={";
        if(v.empty()) o << "\"\"";
        size_t i=0;
        while(i<v.size())
        {
            if(i%4==0)o<<"\n";
            o<<"\""; emitEscaped(o,v[i]); o<<"\",";
            i++;
        }
        o << "};\n";
        o << "static const uint32_t " << lenName << "[]={";
        if(v.empty()) o << "0";
        i=0;
        while(i<v.size()){ if(i%16==0)o<<"\n"; o<<v[i].size()<<"u,"; i++; }
        o << "};\n";
    }
    static void emitRaw(std::ostream &o,const char *name,const std::vector<unsigned char> &v)
    {
        o << "static const unsigned char " << name << "[]={";
        if(v.empty()) o << "0";
        size_t i=0;
        while(i<v.size()){ if(i%20==0)o<<"\n"; o<<static_cast<unsigned>(v[i])<<","; i++; }
        o << "};\n";
    }
    // Escape with 3-digit octal for non-printables (\NNN is unambiguous vs a
    // following digit, unlike \xNN), so embedded-NUL / binary strings survive.
    static void emitEscaped(std::ostream &o,const std::string &s)
    {
        size_t i=0;
        while(i<s.size())
        {
            const unsigned char c=static_cast<unsigned char>(s[i]);
            if(c=='\\') o<<"\\\\";
            else if(c=='"') o<<"\\\"";
            else if(c>=0x20 && c<0x7f) o<<static_cast<char>(c);
            else
            {
                char b[5];
                b[0]='\\';
                b[1]=static_cast<char>('0'+((c>>6)&0x7));
                b[2]=static_cast<char>('0'+((c>>3)&0x7));
                b[3]=static_cast<char>('0'+(c&0x7));
                b[4]='\0';
                o<<b;
            }
            i++;
        }
    }
};
#endif // CATCHCHALLENGER_DATAPACK_CPP_EMIT

} // namespace CatchChallenger

// Pin plain `char` (de)serialization to a FIXED encoding for the datapack-cpp
// buffers, on EVERY platform.
//
// Why: HPS dispatches Serializer<char,B> by std::is_signed<char>, which is
// implementation-defined — signed on x86/host (where the flash stream is
// emitted), but UNSIGNED on xtensa/ESP32 and ARM. The signed path uses zigzag
// varints, the unsigned path uses plain base-127 varints; the two decode the
// SAME bytes to DIFFERENT values. The datapack hash is a std::vector<char>, so
// the host emits it zigzag-encoded and an unsigned-char target read it as
// base-127 → 696F2C97… instead of CBC816B4… (no crash: it consumes the same
// byte count, only the value is wrong). Forcing `char` through the signed
// (signed-char / zigzag) serializer on both the emit and the read buffer makes
// the one baked stream decode identically everywhere. Host emit is unchanged
// (it was already signed), so the existing datapack_cpp.cpp stays valid.
//
// Scoped to the two datapack-cpp buffer types ONLY (CppReadBuffer /
// CppEmitBuffer), so no other HPS buffer (e.g. StreamInputBuffer used elsewhere
// in the same datapack-cpp binary for the dictionary) is touched and there is no
// ODR concern. The full specialisation beats HPS's is_signed/is_unsigned partial
// specialisations, so it is unambiguous. On the host (signed char) it is a no-op
// behaviourally (already zigzag); it only changes the unsigned-char targets,
// bringing them in line with the host-emitted stream.
namespace hps {

#ifdef CATCHCHALLENGER_DATAPACK_CPP
template <>
class Serializer<char, CatchChallenger::CppReadBuffer, void> {
 public:
  static void parse(char& num, CatchChallenger::CppReadBuffer& ib) {
    signed char tmp;
    Serializer<signed char, CatchChallenger::CppReadBuffer>::parse(tmp, ib);
    num = static_cast<char>(tmp);
  }
};
#endif

#ifdef CATCHCHALLENGER_DATAPACK_CPP_EMIT
template <>
class Serializer<char, CatchChallenger::CppEmitBuffer, void> {
 public:
  static void serialize(const char& num, CatchChallenger::CppEmitBuffer& ob) {
    Serializer<signed char, CatchChallenger::CppEmitBuffer>::serialize(
        static_cast<signed char>(num), ob);
  }
};
#endif

}  // namespace hps

#endif // CATCHCHALLENGER_DATAPACK_CPP || _EMIT
#endif // CATCHCHALLENGER_DATAPACKCPPBUFFER_HPP
