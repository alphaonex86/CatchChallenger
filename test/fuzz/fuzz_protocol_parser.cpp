// Coverage-guided libFuzzer harness for the CatchChallenger PROTOCOL PARSER.
//
// It drives arbitrary attacker bytes through the REAL server byte-in path
// ProtocolParsingInputOutput::parseIncommingData() (general/base/
// ProtocolParsingInput.cpp), emulating MULTI-RECV TCP segmentation so the
// header_cut continuation / dynamic-0xFE length reassembly is exercised - that
// is the exact surface of the TCP-split OOB read fixed in parseIncommingData().
// A fuzzer over this entry point would have found that bug automatically.
//
// It links ONLY catchchallenger_general_minimal (the 4 ProtocolParsing*.cpp +
// hashing/settings) - NO datapack, NO DB, NO Qt, NO global init beyond
// initialiseTheVariable() - so a fresh parser object per input is clean state.
//
// BUILD WITHOUT -DCATCHCHALLENGER_HARDENED: the HARDENED clean-tripwire abort()s
// (re-entrancy guard, parse-fail asserts) would kill the fuzz loop on the first
// malformed byte AND re-route dispatch through the client-side check layer.
// Detection here is AddressSanitizer/UBSan, not the tripwire. See CMakeLists.txt.

#include "../../general/base/ProtocolParsing.hpp"
#include "../../general/base/CompressionProtocol.hpp"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>

using namespace CatchChallenger;

// A minimal concrete parser: the 13 pure virtuals as trivial stubs, except
// readFromSocket() which vends the current "recv" chunk, and the parse*()
// handlers which TOUCH every delivered byte so ASan flags any over-read the
// framing layer let through (the corruption half of the fixed bug).
class FuzzParser : public ProtocolParsingInputOutput
{
public:
    FuzzParser() :
        // Server-build ctor takes no args (CATCHCHALLENGERSERVERDROPIFCLENT is
        // auto-defined for CATCHCHALLENGER_SERVER+ALLINONESERVER without HARDENED).
        ProtocolParsingInputOutput(),
        chunk_(nullptr), chunk_len_(0), vended_(false)
    {
        // allowDynamicSize: reach the dynamic-0xFE handlers (post-login surface,
        // where the header_cut/dataSize OOB lived). Fixed-size handlers parse
        // regardless, so this covers both.
        flags |= 0x08;
    }

    // Driver sets the chunk delivered by the NEXT recv, then drives one cycle.
    // (parseIncommingData() is protected on the base; this exposes it.)
    void setChunk(const uint8_t *p, size_t n) { chunk_ = p; chunk_len_ = n; vended_ = false; }
    void drive() { parseIncommingData(); }

    // THE core stub: vend the current chunk ONCE (like a single recv), then
    // return -1 to signal a DRAINED non-blocking socket (EAGAIN). This matches
    // EventLoopClient::read() on Linux EXACTLY: when nothing is available it
    // returns ::read()'s -1, NOT 0 (0 is the ESP32-only branch / EOF). Returning
    // 0 here would be unfaithful — the parser computes size=read()+header_cut
    // .size(), so a 0 makes it reprocess a stale partial-header prefix forever (a
    // false infinite loop the real server never has). Leftover partial headers
    // persist in the base header_cut member across drive() calls — exactly the
    // multi-recv TCP-split path.
    ssize_t readFromSocket(char *data, const size_t &size) override
    {
        if(vended_ || chunk_ == nullptr || chunk_len_ == 0)
            return -1;   // already vended, or an empty recv -> EAGAIN (drained)
        vended_ = true;
        size_t n = (chunk_len_ < size) ? chunk_len_ : size;
        if(n > 0)
            memcpy(data, chunk_, n);
        return static_cast<ssize_t>(n);
    }
    ssize_t writeToSocket(const char * const, const size_t &size) override
    { return static_cast<ssize_t>(size); }

    bool parseMessage(const uint8_t &, const char * const data, const unsigned int &size) override
    { return touch(data, size); }
    bool parseQuery(const uint8_t &, const uint8_t &, const char * const data, const unsigned int &size) override
    { return touch(data, size); }
    bool parseReplyData(const uint8_t &, const uint8_t &, const char * const data, const unsigned int &size) override
    { return touch(data, size); }

    void errorParsingLayer(const std::string &) override {}
    void messageParsingLayer(const std::string &) const override {}
    void reset() override { header_cut.clear(); flags &= 0x18; }   // keep isClient(0x10)+allowDynamicSize(0x08)
    void registerOutputQuery(const uint8_t &, const uint8_t &) override {}
    #if defined(CATCHCHALLENGER_SERVER) && (defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER))
    void moveClientFastPath(const uint8_t &, const uint8_t &) override {}
    #endif
    bool disconnectClient() override { return true; }
    CompressionProtocol::CompressionType getCompressType() const override
    { return CompressionProtocol::CompressionType::None; }
    void closeSocket() override {}

private:
    // Read every delivered byte: under ASan this faults if the parser handed a
    // handler a (data,size) that runs past the valid buffer.
    bool touch(const char * const data, const unsigned int &size)
    {
        volatile uint8_t acc = 0;
        unsigned int i = 0;
        while(i < size)
        {
            acc = static_cast<uint8_t>(acc + static_cast<uint8_t>(data[i]));
            ++i;
        }
        (void)acc;
        return true;
    }

    const uint8_t *chunk_;
    size_t chunk_len_;
    bool vended_;
};

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    // Fill the static packetFixedSize[] table once (AllInOne flavour, matching
    // the audited cli server). Without this the parser sees an all-0xFF table.
    ProtocolParsing::initialiseTheVariable();
    ProtocolParsing::setMaxPlayers(255);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if(size < 2)
        return 0;
    // Byte 0 = number of SMALL leading recv chunks (0..7); the next that-many
    // bytes are their lengths (0..255) - fuzzer-steerable split points so a
    // 1-3 byte chunk can end mid-4-byte-dynamic-length and drive the header_cut
    // path. The REMAINDER is delivered as ONE final recv, which can be up to the
    // whole input: that is what lets a near-COMMONBUFFERSIZE recv land AFTER a
    // header_cut stash (the size = read + 2*header_cut.size() OOB needs a large
    // second recv; a per-chunk cap would never fill the buffer).
    size_t nsplits = data[0] % 8;
    if(size < 1 + nsplits)
        return 0;
    const uint8_t *lens = data + 1;
    const uint8_t *payload = data + 1 + nsplits;
    size_t plen = size - 1 - nsplits;

    FuzzParser parser;   // fresh per input: clean header_cut/flags/buffers
    size_t off = 0;
    size_t i = 0;
    while(i < nsplits && off < plen)
    {
        size_t want = lens[i];
        size_t avail = plen - off;
        size_t take = (want < avail) ? want : avail;
        parser.setChunk(payload + off, take);   // small leading recv
        parser.drive();
        off += take;
        ++i;
    }
    parser.setChunk(payload + off, plen - off); // final big recv (0-len = no-op)
    parser.drive();
    return 0;
}
