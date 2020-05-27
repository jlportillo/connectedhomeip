#include "MessageHeader.h"

#include <assert.h>

#include <core/CHIPEncoding.h>
#include <core/CHIPError.h>
#include <support/CodeUtils.h>

/**********************************************
 * Header format (little endian):
 *
 *  16 bit: | VERSION: 4 bit | FLAGS: 4 bit | RESERVED: 8 bit |
 *  32 bit: | MESSAGE_ID                                      |
 *  64 bit: | SOURCE_NODE_ID (iff source node flag is set)    |
 *  64 bit: | DEST_NODE_ID (iff destination node flag is set) |
 *
 **********************************************/

namespace chip {
namespace {

using namespace chip::Encoding;

constexpr size_t kFixedHeaderSizeBytes = 6;
constexpr size_t kNodeIdSizeBytes      = 8;

// Available flags
constexpr uint16_t kFlagDestinationNodeIdPresent = 0x0100;
constexpr uint16_t kFlagSourceNodeIdPresent      = 0x0200;

// Version parsing and setting
constexpr uint16_t kVersionMask = 0xF000;
constexpr int kVersionShift     = 12;

} // namespace

size_t MessageHeader::EncodeSizeBytes() const
{
    size_t size = kFixedHeaderSizeBytes;

    if (mSourceNodeId.HasValue())
    {
        size += kNodeIdSizeBytes;
    }

    if (mDestinationNodeId.HasValue())
    {
        size += kNodeIdSizeBytes;
    }

    return size;
}

CHIP_ERROR MessageHeader::Decode(const uint8_t * data, size_t size)
{
    CHIP_ERROR err    = CHIP_NO_ERROR;
    const uint8_t * p = data;
    uint16_t header;
    int version;

    VerifyOrExit(size >= kFixedHeaderSizeBytes, err = CHIP_ERROR_INVALID_ARGUMENT);

    header  = LittleEndian::Read16(p);
    version = ((header & kVersionMask) >> kVersionShift);
    VerifyOrExit(version == kHeaderVersion, err = CHIP_ERROR_VERSION_MISMATCH);

    mMessageId = LittleEndian::Read32(p);

    assert(p - data == kFixedHeaderSizeBytes);
    size -= kFixedHeaderSizeBytes;

    if (header & kFlagSourceNodeIdPresent)
    {
        VerifyOrExit(size >= kNodeIdSizeBytes, err = CHIP_ERROR_INVALID_ARGUMENT);
        mSourceNodeId.SetValue(LittleEndian::Read64(p));
        size -= kFixedHeaderSizeBytes;
    }
    else
    {
        mSourceNodeId.ClearValue();
    }

    if (header & kFlagDestinationNodeIdPresent)
    {
        VerifyOrExit(size >= kNodeIdSizeBytes, err = CHIP_ERROR_INVALID_ARGUMENT);
        mDestinationNodeId.SetValue(LittleEndian::Read64(p));
        size -= kFixedHeaderSizeBytes;
    }
    else
    {
        mDestinationNodeId.ClearValue();
    }

exit:

    return err;
}

CHIP_ERROR MessageHeader::Encode(uint8_t * data, size_t size, size_t * encode_size)
{
    CHIP_ERROR err  = CHIP_NO_ERROR;
    uint8_t * p     = data;
    uint16_t header = kHeaderVersion << kVersionShift;

    VerifyOrExit(size >= EncodeSizeBytes(), err = CHIP_ERROR_INVALID_ARGUMENT);

    if (mSourceNodeId.HasValue())
    {
        header |= kFlagSourceNodeIdPresent;
    }
    if (mDestinationNodeId.HasValue())
    {
        header |= kFlagDestinationNodeIdPresent;
    }

    LittleEndian::Write16(p, header);
    LittleEndian::Write32(p, mMessageId);
    if (mSourceNodeId.HasValue())
    {
        LittleEndian::Write64(p, mSourceNodeId.Value());
    }
    if (mDestinationNodeId.HasValue())
    {
        LittleEndian::Write64(p, mDestinationNodeId.Value());
    }

    // Written data size provided to caller on success
    *encode_size = p - data;

exit:
    return err;
}

} // namespace chip
