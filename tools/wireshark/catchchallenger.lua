--
-- CatchChallenger Wireshark dissector (TCP, plaintext frames)
--
-- Drop into ~/.config/wireshark/plugins/ (or use:
--   wireshark -X lua_script:catchchallenger.lua capture.pcapng).
--
-- Decoded as: PacketCode | [QueryNumber] | [DynamicSize] | Payload
-- Following general/base/ProtocolParsing.hpp + ProtocolParsingGeneral.cpp.
--
-- Limitations: no Zstandard payload decompression (Wireshark Lua can't pull
-- in libzstd); reply size is resolved via per-conversation query tracking,
-- so partial captures (started mid-session) will show replies as "unknown
-- reply size".
--

local p_cc = Proto("catchchallenger", "CatchChallenger")

-- ---------------------------------------------------------------------------
-- Fixed-size table mirrors ProtocolParsing::initialiseTheVariable().
--   nil  => unknown / blocked (0xFF)
--   -1   => dynamic, uint32 LE size follows (0xFE)
--   >=0  => exact payload size in bytes
-- ---------------------------------------------------------------------------
local FIXED = {}
do
    -- 0xFE = dynamic
    local D = -1
    -- queries / messages
    FIXED[0x02]=2;     FIXED[0x03]=D;     FIXED[0x04]=1;     FIXED[0x05]=D
    FIXED[0x06]=D;     FIXED[0x07]=0;     FIXED[0x08]=1;     FIXED[0x09]=3
    FIXED[0x0A]=D;     FIXED[0x0B]=0;     FIXED[0x0C]=0;     FIXED[0x0D]=2
    FIXED[0x0E]=1;     FIXED[0x0F]=D;     FIXED[0x10]=D;     FIXED[0x11]=2
    FIXED[0x12]=D;     FIXED[0x13]=6;     FIXED[0x14]=D;     FIXED[0x15]=0
    FIXED[0x16]=0;     FIXED[0x17]=D;     FIXED[0x18]=0;     FIXED[0x19]=1
    FIXED[0x1A]=0;     FIXED[0x1B]=2;     FIXED[0x1C]=2;     FIXED[0x1D]=2
    FIXED[0x1E]=2;     FIXED[0x1F]=1
    FIXED[0x3E]=4;     FIXED[0x3F]=2

    FIXED[0x40]=D;     FIXED[0x44]=D;     FIXED[0x45]=D;     FIXED[0x46]=D
    FIXED[0x47]=D;     FIXED[0x48]=D;     FIXED[0x4D]=4;     FIXED[0x50]=D
    FIXED[0x51]=0;     FIXED[0x52]=D;     FIXED[0x53]=D;     FIXED[0x54]=D
    FIXED[0x55]=D;     FIXED[0x56]=D;     FIXED[0x57]=D;     FIXED[0x58]=D
    FIXED[0x59]=0;     FIXED[0x5A]=0;     FIXED[0x5B]=0
    FIXED[0x5C]=D;     FIXED[0x5D]=D;     FIXED[0x5E]=0;     FIXED[0x5F]=D
    FIXED[0x60]=D;     FIXED[0x61]=D;     FIXED[0x62]=D;     FIXED[0x63]=D
    FIXED[0x64]=2;     FIXED[0x65]=0;     FIXED[0x66]=D;     FIXED[0x67]=D
    FIXED[0x68]=D;     FIXED[0x69]=D;     FIXED[0x6A]=0;     FIXED[0x6B]=D
    FIXED[0x6C]=1
    FIXED[0x75]=8;     FIXED[0x76]=D;     FIXED[0x77]=D;     FIXED[0x78]=D
    FIXED[0x7F]=D

    -- queries that need a queryNumber (0x80+)
    FIXED[0x80]=D;     FIXED[0x81]=D;     FIXED[0x82]=D;     FIXED[0x83]=D
    FIXED[0x84]=0;     FIXED[0x85]=2;     FIXED[0x86]=2;     FIXED[0x87]=0
    FIXED[0x88]=10;    FIXED[0x89]=10;    FIXED[0x8A]=2
    FIXED[0x8B]=12;    FIXED[0x8C]=12
    FIXED[0x92]=D
    FIXED[0x93]=4   -- CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER (default)
    FIXED[0xA0]=5;     FIXED[0xA1]=D
    FIXED[0xA8]=64;    FIXED[0xA9]=64    -- 2 * CATCHCHALLENGER_HASH_SIZE (32)
    FIXED[0xAA]=D
    FIXED[0xAB]=5;     FIXED[0xAC]=9;     FIXED[0xAD]=32
    FIXED[0xB0]=0;     FIXED[0xB1]=0;     FIXED[0xB2]=D
    FIXED[0xB8]=9;     FIXED[0xBD]=32;    FIXED[0xBE]=13;    FIXED[0xBF]=0
    FIXED[0xC0]=1;     FIXED[0xC1]=1
    FIXED[0xDF]=D;     FIXED[0xE0]=D;     FIXED[0xE1]=D;     FIXED[0xE2]=2
    FIXED[0xE3]=0
    FIXED[0xF8]=8;     FIXED[0xF9]=0
end

-- Reply-size table: same indexing trick as the C side
-- (packetFixedSize[256 + replyTo - 128]).
local FIXED_REPLY = {}
do
    local D = -1
    FIXED_REPLY[0x80]=D; FIXED_REPLY[0x81]=D; FIXED_REPLY[0x82]=D
    FIXED_REPLY[0x83]=1; FIXED_REPLY[0x84]=D; FIXED_REPLY[0x85]=D
    FIXED_REPLY[0x86]=D; FIXED_REPLY[0x87]=D; FIXED_REPLY[0x88]=D
    FIXED_REPLY[0x89]=D; FIXED_REPLY[0x8A]=D; FIXED_REPLY[0x8B]=D
    FIXED_REPLY[0x8C]=D; FIXED_REPLY[0x92]=D; FIXED_REPLY[0x93]=D
    FIXED_REPLY[0xA0]=D; FIXED_REPLY[0xA1]=D
    FIXED_REPLY[0xA8]=D; FIXED_REPLY[0xA9]=D
    FIXED_REPLY[0xAA]=5; FIXED_REPLY[0xAB]=1
    FIXED_REPLY[0xAC]=D; FIXED_REPLY[0xAD]=1
    -- B0/BF/C0/C1 use CATCHCHALLENGER_SERVER_MAXIDBLOCK*4. The default
    -- MAXIDBLOCK is 16, so 64 bytes — only valid for default builds. Treat
    -- as dynamic so capture-side parser doesn't deadlock if value diverges.
    FIXED_REPLY[0xB0]=D; FIXED_REPLY[0xB1]=20; FIXED_REPLY[0xB2]=D
    FIXED_REPLY[0xB8]=D; FIXED_REPLY[0xBD]=D; FIXED_REPLY[0xBE]=D
    FIXED_REPLY[0xBF]=D; FIXED_REPLY[0xC0]=D; FIXED_REPLY[0xC1]=D
    FIXED_REPLY[0xDF]=1; FIXED_REPLY[0xE0]=1; FIXED_REPLY[0xE1]=0
    FIXED_REPLY[0xE2]=0; FIXED_REPLY[0xE3]=0
    FIXED_REPLY[0xF8]=D; FIXED_REPLY[0xF9]=0
end

local PACKET_NAME = {
    [0x01] = "Reply (client->server)",
    [0x7F] = "Reply (server->client)",
    [0x02] = "Move",
    [0x03] = "Insert player",
    [0x04] = "Remove player",
    [0x05] = "Reinsert player",
    [0x06] = "Full player list",
    [0x07] = "MapVisibility full",
    [0x09] = "Direct chat",
    [0x0A] = "Server chat",
    [0x40] = "Datapack file list",
    [0x4D] = "Logical group",
    [0x64] = "Number of players",
    [0x93] = "Connect to game server (token)",
    [0xA0] = "Login query",
    [0xA1] = "Login reply data",
    [0xA8] = "Pre-login server",
    [0xA9] = "Pre-login client",
    [0xAA] = "Server list",
    [0xAB] = "Select character",
    [0xAC] = "Add character",
    [0xAD] = "Login token",
    [0xB0] = "Datapack signature query",
    [0xB1] = "Datapack signature reply",
    [0xBD] = "Datapack hash",
    [0xBE] = "Datapack file query",
    [0xC0] = "Datapack request",
    [0xC1] = "Datapack request",
}

-- ---------------------------------------------------------------------------
-- Protocol fields
-- ---------------------------------------------------------------------------
local f_packet_code  = ProtoField.uint8 ("catchchallenger.code", "Packet code", base.HEX)
local f_packet_name  = ProtoField.string("catchchallenger.name", "Packet name")
local f_query_number = ProtoField.uint8 ("catchchallenger.qid",  "Query number", base.DEC)
local f_data_size    = ProtoField.uint32("catchchallenger.size", "Payload size", base.DEC)
local f_payload      = ProtoField.bytes ("catchchallenger.data", "Payload")
local f_is_reply     = ProtoField.bool  ("catchchallenger.reply", "Is reply")
local f_reply_to     = ProtoField.uint8 ("catchchallenger.reply_to", "Reply to packet", base.HEX)

p_cc.fields = {
    f_packet_code, f_packet_name, f_query_number, f_data_size,
    f_payload, f_is_reply, f_reply_to,
}

-- ---------------------------------------------------------------------------
-- Per-conversation outstanding queries: queryNumber -> packetCode of the
-- request that is still awaiting a reply. Two directions per conversation:
-- one for client->server queries (replied by 0x7F), one for the inverse.
-- ---------------------------------------------------------------------------
local conv_state = {}

local function conv_key(pinfo)
    -- Stable key per TCP conversation, irrespective of direction.
    local a, b = tostring(pinfo.src) .. ":" .. pinfo.src_port,
                 tostring(pinfo.dst) .. ":" .. pinfo.dst_port
    if a < b then return a .. "|" .. b else return b .. "|" .. a end
end

local function get_state(key)
    local s = conv_state[key]
    if s == nil then
        -- Two slots per direction. Use the lower endpoint string as
        -- "side A"; replies in this direction look up A's outstanding
        -- queries.
        s = { a = {}, b = {} }
        conv_state[key] = s
    end
    return s
end

-- Side identifier for incoming pdu given pinfo (which side made the request).
local function side_of(pinfo)
    local src = tostring(pinfo.src) .. ":" .. pinfo.src_port
    local dst = tostring(pinfo.dst) .. ":" .. pinfo.dst_port
    if src < dst then return "a" else return "b" end
end

-- Wireshark replays packets on second pass, so per-pdu we cache the
-- resolved size keyed by (frame, offset) to avoid mutating state twice.
local pdu_cache = {}

local function pdu_key(pinfo, offset)
    return pinfo.number .. ":" .. offset
end

-- ---------------------------------------------------------------------------
-- One PDU. Returns (consumed, needed_more_bytes_or_zero).
--   needed_more==0  => parsed OK
--   needed_more>0   => need that many more bytes
--   needed_more==-1 => malformed, stop dissection of this segment
-- ---------------------------------------------------------------------------
local function dissect_one(buf, pinfo, tree, offset)
    local avail = buf:len() - offset
    if avail < 1 then return 0, 1 end

    local code = buf(offset, 1):uint()
    local is_reply = (code == 0x01) or (code == 0x7F)
    local needs_qid = is_reply or (code >= 0x80)

    local hdr_len = 1 + (needs_qid and 1 or 0)
    if avail < hdr_len then return 0, hdr_len - avail end

    local qid
    if needs_qid then
        qid = buf(offset + 1, 1):uint()
    end

    -- Resolve payload size
    local pkey = pdu_key(pinfo, offset)
    local cached = pdu_cache[pkey]
    local payload_size
    local reply_to
    local size_kind  -- "fixed", "dynamic", "unknown"
    local size_field_len = 0

    if cached ~= nil then
        payload_size  = cached.size
        size_kind     = cached.kind
        reply_to      = cached.reply_to
        size_field_len = cached.size_field_len
    else
        if is_reply then
            -- Look up the original query type from conversation state.
            local key = conv_key(pinfo)
            local st = get_state(key)
            -- A reply going A->B clears B's outstanding queries (the ones
            -- B had sent to A) — actually the request was sent by the
            -- *other* side, so look at the opposite slot.
            local me = side_of(pinfo)
            local other = (me == "a") and "b" or "a"
            local rt = st[other][qid]
            if rt ~= nil then
                reply_to = rt
                local rsize = FIXED_REPLY[rt]
                if rsize == nil then
                    size_kind = "unknown"
                elseif rsize == -1 then
                    size_kind = "dynamic"
                else
                    size_kind = "fixed"
                    payload_size = rsize
                end
                -- Clear so re-uses of qid don't keep matching.
                if not pinfo.visited then
                    st[other][qid] = nil
                end
            else
                size_kind = "unknown"
            end
        else
            local fsize = FIXED[code]
            if fsize == nil then
                size_kind = "unknown"
            elseif fsize == -1 then
                size_kind = "dynamic"
            else
                size_kind = "fixed"
                payload_size = fsize
            end
            -- Track for the matching reply.
            if needs_qid and not is_reply and not pinfo.visited then
                local key = conv_key(pinfo)
                local st = get_state(key)
                local me = side_of(pinfo)
                st[me][qid] = code
            end
        end

        if size_kind == "dynamic" then
            -- uint32 LE size follows the optional qid byte.
            if avail < hdr_len + 4 then
                return 0, (hdr_len + 4) - avail
            end
            payload_size = buf(offset + hdr_len, 4):le_uint()
            size_field_len = 4
        end

        pdu_cache[pkey] = {
            size = payload_size,
            kind = size_kind,
            reply_to = reply_to,
            size_field_len = size_field_len,
        }
    end

    if size_kind == "unknown" then
        -- Can't progress without knowing size; mark malformed for this
        -- segment so user sees it instead of silently mis-aligning the
        -- rest of the stream.
        local subtree = tree:add(p_cc, buf(offset, avail))
        subtree:add(f_packet_code, buf(offset, 1))
        local nm = PACKET_NAME[code] or string.format("Unknown 0x%02X", code)
        subtree:append_text(string.format(": %s [unknown size — partial capture?]", nm))
        if needs_qid then
            subtree:add(f_query_number, buf(offset + 1, 1))
        end
        subtree:add_expert_info(PI_MALFORMED, PI_WARN,
            "Unknown packet size; can't continue dissecting this segment")
        return avail, -1
    end

    local total = hdr_len + size_field_len + payload_size
    if avail < total then
        return 0, total - avail
    end

    -- Build subtree
    local subtree = tree:add(p_cc, buf(offset, total))
    local nm = PACKET_NAME[code] or string.format("0x%02X", code)
    if is_reply and reply_to then
        local rnm = PACKET_NAME[reply_to] or string.format("0x%02X", reply_to)
        subtree:append_text(string.format(": %s -> %s (qid=%d)", nm, rnm, qid))
    elseif needs_qid then
        subtree:append_text(string.format(": %s (qid=%d)", nm, qid))
    else
        subtree:append_text(string.format(": %s", nm))
    end

    subtree:add(f_packet_code, buf(offset, 1)):append_text(" — " .. nm)
    subtree:add(f_packet_name, nm):set_generated()
    subtree:add(f_is_reply, is_reply):set_generated()
    if needs_qid then
        subtree:add(f_query_number, buf(offset + 1, 1))
    end
    if reply_to then
        subtree:add(f_reply_to, reply_to):set_generated()
    end
    if size_field_len > 0 then
        subtree:add_le(f_data_size, buf(offset + hdr_len, 4))
    else
        subtree:add(f_data_size, payload_size):set_generated()
    end
    if payload_size > 0 then
        subtree:add(f_payload, buf(offset + hdr_len + size_field_len, payload_size))
    end

    return total, 0
end

-- ---------------------------------------------------------------------------
-- TCP dissector entry point. Walks all PDUs in the segment; on a partial
-- PDU, sets pinfo.desegment_len so Wireshark hands us the next segment too.
-- ---------------------------------------------------------------------------
function p_cc.dissector(buf, pinfo, tree)
    pinfo.cols.protocol = "CatchChallenger"
    local total = buf:len()
    if total < 1 then return 0 end

    local offset = 0
    local count = 0
    local summary = {}

    while offset < total do
        local consumed, need = dissect_one(buf, pinfo, tree, offset)
        if consumed == 0 and need > 0 then
            -- Need more bytes from TCP layer.
            pinfo.desegment_offset = offset
            pinfo.desegment_len    = need
            break
        end
        if need == -1 then
            -- Malformed; stop here, don't claim past data.
            offset = offset + consumed
            break
        end
        offset = offset + consumed
        count = count + 1
        local code = buf(offset - consumed, 1):uint()
        local nm = PACKET_NAME[code] or string.format("0x%02X", code)
        if #summary < 4 then
            summary[#summary + 1] = nm
        end
    end

    if count > 0 then
        local s = table.concat(summary, ", ")
        if count > #summary then
            s = s .. string.format(", ... (%d total)", count)
        end
        pinfo.cols.info = s
    end
    return offset
end

-- ---------------------------------------------------------------------------
-- Port registration. Defaults cover the test harness (61917) and the default
-- the codebase ships (9999). User can override via Wireshark's "Decode As".
-- ---------------------------------------------------------------------------
local prefs = p_cc.prefs
prefs.ports = Pref.range("TCP ports", "9999,61917",
    "TCP ports CatchChallenger runs on", 65535)

local tcp_table = DissectorTable.get("tcp.port")
local current_ports

local function register_ports(range_str)
    if current_ports then
        for port in tostring(current_ports):gmatch("(%d+)") do
            tcp_table:remove(tonumber(port), p_cc)
        end
    end
    current_ports = range_str
    for port in tostring(range_str):gmatch("(%d+)") do
        tcp_table:add(tonumber(port), p_cc)
    end
end

function p_cc.prefs_changed()
    register_ports(prefs.ports)
end

register_ports(prefs.ports)
