#include "protobuf_c.h"


#define  kMaxVarintBytes (10)
#define  kMaxVarint32Bytes (5)

typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int32_t int32;
typedef int64_t int64;


inline static int BufferSize(pb_inputstream* s) {
    return (int)(s->buffer_end_ - s->buffer_);
}

inline static void Advance(pb_inputstream* s, int amount) {
    s->buffer_ += amount;
}


inline static PBOOL ReadVarint64FromArray(uint8_t* buffer, uint64_t* value, uint8_t** newptr) {
    uint8_t* ptr = buffer;
    uint32_t b;

    // Splitting into 32-bit pieces gives better performance on 32-bit
    // processors.
    uint32_t part0 = 0, part1 = 0, part2 = 0;

    b = *(ptr++); part0  = b      ; if (!(b & 0x80)) goto done;
    part0 -= 0x80;
    b = *(ptr++); part0 += b <<  7; if (!(b & 0x80)) goto done;
    part0 -= 0x80 << 7;
    b = *(ptr++); part0 += b << 14; if (!(b & 0x80)) goto done;
    part0 -= 0x80 << 14;
    b = *(ptr++); part0 += b << 21; if (!(b & 0x80)) goto done;
    part0 -= 0x80 << 21;
    b = *(ptr++); part1  = b      ; if (!(b & 0x80)) goto done;
    part1 -= 0x80;
    b = *(ptr++); part1 += b <<  7; if (!(b & 0x80)) goto done;
    part1 -= 0x80 << 7;
    b = *(ptr++); part1 += b << 14; if (!(b & 0x80)) goto done;
    part1 -= 0x80 << 14;
    b = *(ptr++); part1 += b << 21; if (!(b & 0x80)) goto done;
    part1 -= 0x80 << 21;
    b = *(ptr++); part2  = b      ; if (!(b & 0x80)) goto done;
    part2 -= 0x80;
    b = *(ptr++); part2 += b <<  7; if (!(b & 0x80)) goto done;
    // "part2 -= 0x80 << 7" is irrelevant because (0x80 << 7) << 56 is 0.

    // We have overrun the maximum size of a varint (10 bytes).  Assume
    // the data is corrupt.
    *newptr = ptr;
    return FALSE;
done:
    *value = ((uint64_t)(part0)) |
            ((uint64_t)(part1) << 28) |
            ((uint64_t)(part2) << 56);
    *newptr = ptr;
    return TRUE;
}

inline static PBOOL ReadVarint64Slow(pb_inputstream* s, uint64_t* value) {
    // Slow path:  This read might cross the end of the buffer, so we
    // need to check and refresh the buffer if and when it does.

    uint64_t result = 0;
    int count = 0;
    uint32_t b;

    do {
        if (count == kMaxVarintBytes) {
            *value = 0;
            return FALSE;
        }
        if (s->buffer_ == s->buffer_end_) {
            *value = 0;
            return FALSE;
        }
        b = *s->buffer_;
        result |= (uint64_t)(b & 0x7F) << (7 * count);
        Advance(s, 1);
        ++count;
    } while (b & 0x80);

    *value = result;
    return TRUE;
}

inline static PBOOL InternalReadStringInline(pb_inputstream* s, pb_string* buffer,
                                              int size) {
    if (size < 0) return FALSE;  // security: size is often user-supplied
    if (BufferSize(s) >= size) {
        buffer->data = s->buffer_;
        buffer->size = size;
        Advance(s, size);
        return TRUE;
    }else{
        s->buffer_ = s->buffer_end_;
        buffer->data = NULL;
        buffer->size = 0;
        return FALSE;
    }
}

static PBOOL ReadVarint64Fallback(pb_inputstream* s, uint64_t* out) {
    if (BufferSize(s) >= kMaxVarintBytes ||
            // Optimization:  We're also safe if the buffer is non-empty and it ends
            // with a byte that would terminate a varint.
            (s->buffer_end_ > s->buffer_ && !(s->buffer_end_[-1] & 0x80))) {
        uint64_t temp;
        uint8_t* ptr;
        PBOOL ret = ReadVarint64FromArray(s->buffer_, &temp, &ptr);
        if (!ret) {
            *out = 0;
            return FALSE;
        }
        s->buffer_ = ptr;
        *out = temp;
        return TRUE;
    } else {
        return ReadVarint64Slow(s, out);
    }
}

inline static PBOOL ReadVarint32Slow(pb_inputstream* s, uint32_t* value) {
    // Directly invoke ReadVarint64Fallback, since we already tried to optimize
    // for one-byte varints.
    uint64_t temp;
    PBOOL ret = ReadVarint64Fallback(s, &temp);
    *value = (uint32_t)(temp);
    return ret;
}

inline static PBOOL ReadVarint32FromArray(uint32_t first_byte,  uint8_t* buffer, uint32_t* value, uint8_t** newptr) {
    // Fast path:  We have enough bytes left in the buffer to guarantee that
    // this read won't cross the end, so we can skip the checks.
    //  GOOGLE_DCHECK_EQ(*buffer, first_byte);
    //  GOOGLE_DCHECK_EQ(first_byte & 0x80, 0x80) << first_byte;
    uint8_t* ptr = buffer;
    uint32_t b;
    uint32_t result = first_byte - 0x80;
    ++ptr;  // We just processed the first byte.  Move on to the second.
    b = *(ptr++); result += b <<  7; if (!(b & 0x80)) goto done;
    result -= 0x80 << 7;
    b = *(ptr++); result += b << 14; if (!(b & 0x80)) goto done;
    result -= 0x80 << 14;
    b = *(ptr++); result += b << 21; if (!(b & 0x80)) goto done;
    result -= 0x80 << 21;
    b = *(ptr++); result += b << 28; if (!(b & 0x80)) goto done;
    // "result -= 0x80 << 28" is irrevelant.

    // If the input is larger than 32 bits, we still need to read it all
    // and discard the high-order bits.
    for (int i = 0; i < kMaxVarintBytes - kMaxVarint32Bytes; i++) {
        b = *(ptr++); if (!(b & 0x80)) goto done;
    }

    *newptr = ptr;
    return FALSE;
done:
    *value = result;
    *newptr = ptr;
    return TRUE;
}

static int64_t ReadVarint32Fallback(pb_inputstream* s, uint32_t first_byte_or_zero) {
    if (BufferSize(s) >= kMaxVarintBytes ||
            // Optimization:  We're also safe if the buffer is non-empty and it ends
            // with a byte that would terminate a varint.
            (s->buffer_end_ > s->buffer_ && !(s->buffer_end_[-1] & 0x80))) {
        //    GOOGLE_DCHECK_NE(first_byte_or_zero, 0)
        //        << "Caller should provide us with *buffer_ when buffer is non-empty";
        uint32_t temp;
        uint8_t* ptr;
        PBOOL ret = ReadVarint32FromArray(first_byte_or_zero, s->buffer_, &temp, &ptr);
        if (!ret) return -1;
        s->buffer_ = ptr;
        return temp;
    } else {
        // Really slow case: we will incur the cost of an extra function call here,
        // but moving this out of line reduces the size of this function, which
        // improves the common case. In micro benchmarks, this is worth about 10-15%
        uint32_t temp;
        return ReadVarint32Slow(s, &temp) ? (int64_t)(temp) : -1;
    }
}

inline static PBOOL ReadVarint32(pb_inputstream* s, uint32_t* out){
    uint32_t v = 0;
    if (s->buffer_ < s->buffer_end_) {
        v = *s->buffer_;
        if (v < 0x80) {
            *out = v;
            Advance(s, 1);
            return TRUE;
        }
    }
    int64_t result = ReadVarint32Fallback(s, v);
    *out = (uint32_t)(result);
    return result >= 0;
}

inline static PBOOL ReadBytesToString(pb_inputstream* s,
                                     pb_string* value) {
    uint32 length;
    return ReadVarint32(s, &length) &&
            InternalReadStringInline(s, value, length);
}


inline static PBOOL ReadVarint64(pb_inputstream* s, uint64_t* value) {
    if (s->buffer_ < s->buffer_end_ && *s->buffer_ < 0x80) {
        *value = *s->buffer_;
        Advance(s, 1);
        return TRUE;
    }
    return ReadVarint64Fallback(s, value);
}

inline static int ReadVarintSizeAsIntSlow(pb_inputstream* s) {
    // Directly invoke ReadVarint64Fallback, since we already tried to optimize
    // for one-byte varints.
    uint64_t temp;
    PBOOL ret = ReadVarint64Fallback(s, &temp);
    if (!ret || temp > (uint64)(2147483647)) return -1;
    return (int)temp;
}

inline static int ReadVarintSizeAsIntFallback(pb_inputstream* s) {
    if (BufferSize(s) >= kMaxVarintBytes ||
            // Optimization:  We're also safe if the buffer is non-empty and it ends
            // with a byte that would terminate a varint.
            (s->buffer_end_ > s->buffer_ && !(s->buffer_end_[-1] & 0x80))) {
        uint64 temp;
        uint8_t* ptr;
        PBOOL ret = ReadVarint64FromArray(s->buffer_, &temp, &ptr);
        if (!ret || temp > (uint64)(2147483647)) return -1; //INT_MAX
        s->buffer_ = ptr;
        return (int)temp;
    } else {
        // Really slow case: we will incur the cost of an extra function call here,
        // but moving this out of line reduces the size of this function, which
        // improves the common case. In micro benchmarks, this is worth about 10-15%
        return ReadVarintSizeAsIntSlow(s);
    }
}

inline static PBOOL ReadVarintSizeAsInt(pb_inputstream* s, int* value) {
    if (s->buffer_ < s->buffer_end_) {
        int v = *s->buffer_;
        if (v < 0x80) {
            *value = v;
            Advance(s, 1);
            return TRUE;
        }
    }
    *value = ReadVarintSizeAsIntFallback(s);
    return *value >= 0;
}


PBOOL ReadInt32(pb_inputstream* s, int32_t* out){
    uint32_t temp;
    if (!ReadVarint32(s, &temp)) return FALSE;
    *out = (int32_t)(temp);
    return TRUE;
}

PBOOL ReadUint32(pb_inputstream* s, uint32_t* value){
    return ReadVarint32(s, value);
}

PBOOL ReadTag(pb_inputstream* s, uint32_t* value){
    if (s->buffer_ == s->buffer_end_){
        *value = 0;
        return FALSE;
    }
    return ReadVarint64(s, value);
}

inline static PBOOL Skip(pb_inputstream* s, int count) {
    if (count < 0) return FALSE;  // security: count is often user-supplied
    const int original_buffer_size = BufferSize(s);

    if (count <= original_buffer_size) {
        // Just skipping within the current buffer.  Easy.
        Advance(s, count);
        return TRUE;
    }else{
        return FALSE;
    }
}

inline uint8* ReadLittleEndian64FromArray(
        uint8* buffer, uint64* value) {
    memcpy(value, buffer, sizeof(*value));
    return buffer + sizeof(*value);
}

inline static PBOOL ReadLittleEndian64(pb_inputstream* s, uint64* value) {
    if (BufferSize(s) >= (int)(sizeof(*value))) {
        s->buffer_ = ReadLittleEndian64FromArray(s->buffer_, value);
        return TRUE;
    } else {
        return FALSE;
    }
}

inline uint8* ReadLittleEndian32FromArray(
        uint8* buffer,
        uint32* value) {
    memcpy(value, buffer, sizeof(*value));
    return buffer + sizeof(*value);

}

inline static PBOOL ReadLittleEndian32(pb_inputstream* s, uint32* value) {
    if (BufferSize(s) >= (int)(sizeof(*value))) {
        s->buffer_ = ReadLittleEndian32FromArray(s->buffer_, value);
        return TRUE;
    } else {
        return FALSE;
    }
}

PBOOL SkipField(pb_inputstream* s, uint32 tag){
    int number = GetTagFieldNumber(tag);
    // Field number 0 is illegal.
    if (number == 0) return FALSE;

    switch (GetTagWireType(tag)) {
    case WIRETYPE_VARINT: {
        uint64 value;
        if (!ReadVarint64(s, &value)) return FALSE;
        return TRUE;
    }
    case WIRETYPE_FIXED64: {
        uint64 value;
        if (!ReadLittleEndian64(s, &value)) return FALSE;
        return TRUE;
    }
    case WIRETYPE_LENGTH_DELIMITED: {
        uint32 length;
        if (!ReadVarint32(s, &length)) return FALSE;
        if (!Skip(s, length)) return FALSE;
        return TRUE;
    }
    case WIRETYPE_FIXED32: {
        uint32 value;
        if (!ReadLittleEndian32(s, &value)) return FALSE;
        return TRUE;
    }
    default: {
        return FALSE;
    }
    }
}

PBOOL ReadUint64(pb_inputstream* s, uint64_t* value){
    return ReadVarint64(s, value);
}

PBOOL ReadInt64(pb_inputstream* s, int64_t* value){
    uint64_t temp;
    if (!ReadVarint64(s, &temp)) return FALSE;
    *value = (int64_t)(temp);
    return TRUE;
}

PBOOL ReadBool(pb_inputstream* s, PBOOL* value){
    uint64_t temp;
    if (!ReadVarint64(s, &temp)) return FALSE;
    *value = temp != 0;
    return TRUE;
}

PBOOL ReadEnum(pb_inputstream* s, int32_t* value){
    uint32_t temp;
    if (!ReadVarint32(s, &temp)) return FALSE;
    *value = (int32_t)(temp);
    return TRUE;
}

PBOOL ReadBytes(pb_inputstream* s, pb_string* value) {
    return ReadBytesToString(s, value);
}

PBOOL ReadString(pb_inputstream* s, pb_string* value) {
    return ReadBytesToString(s, value);
}

PBOOL ReadMessage(pb_inputstream* s, pb_message* value) {
    int length;
    if (!ReadVarintSizeAsInt(s, &length)){
        return FALSE;
    }

    pb_inputstream msg_stream;
    msg_stream.buffer_ = s->buffer_;
    msg_stream.buffer_end_ = msg_stream.buffer_ + length;
    if (!value->mergeFromInputStream(value, s)) {
        return FALSE;
    }
    Advance(s, length);
    return TRUE;
}


inline static void OUT_Advance(pb_outstream* s, int amount){
    s->buffer_ += amount;
    s->buffer_size_ -= amount;
}

inline static uint8_t* WriteVarint32ToArray(pb_outstream* s, uint32_t value, uint8_t* target) {
    while (value >= 0x80) {
        *target = (uint8_t)(value | 0x80);
        value >>= 7;
        ++target;
    }
    *target = (uint8_t)(value);
    return target + 1;
}

inline static OUT_WriteRaw(pb_outstream* s, void* data, int size) {
    memcpy(s->buffer_, data, size);
    OUT_Advance(s, size);
}

inline static void WriteVarint32SlowPath(pb_outstream* s, uint32_t value) {
    uint8_t bytes[kMaxVarint32Bytes];
    uint8_t* target = &bytes[0];
    uint8_t* end = WriteVarint32ToArray(s, value, target);
    int size = (int)(end - target);
    OUT_WriteRaw(s, bytes, size);
}

inline static uint8* WriteVarint64ToArray(pb_outstream* s,
                                          uint64 value,
                                          uint8* target) {
    while (value >= 0x80) {
        *target = (uint8)(value | 0x80);
        value >>= 7;
        ++target;
    }
    *target = (uint8)(value);
    return target + 1;
}

inline static void WriteVarint64SlowPath(pb_outstream* s, uint64_t value) {
    uint8 bytes[kMaxVarintBytes];
    uint8* target = &bytes[0];
    uint8* end = WriteVarint64ToArray(s, value, target);
    int size = (int)(end - target);
    OUT_WriteRaw(s, bytes, size);
}

inline static void WriteVarint64(pb_outstream* s, uint64 value) {
    if (s->buffer_size_ >= 10) {
        // Fast path:  We have enough bytes left in the buffer to guarantee that
        // this write won't cross the end, so we can skip the checks.
        uint8* target = s->buffer_;
        uint8* end = WriteVarint64ToArray(s, value, target);
        int size = (int)(end - target);
        OUT_Advance(s, size);
    } else {
        WriteVarint64SlowPath(s, value);
    }
}




inline static void WriteVarint32(pb_outstream* s, uint32_t value) {
    if (s->buffer_size_ >= 5) {
        // Fast path:  We have enough bytes left in the buffer to guarantee that
        // this write won't cross the end, so we can skip the checks.
        uint8* target = s->buffer_;
        uint8* end = WriteVarint32ToArray(s, value, target);
        int size = (int)(end - target);
        OUT_Advance(s, size);
    } else {
        WriteVarint32SlowPath(s, value);
    }
}



inline static void WriteTag(pb_outstream* s, int field_number, uint32_t type) {
    WriteVarint32(s, MakeTag(field_number, type));
}

inline static void WriteVarint32SignExtended(pb_outstream* s, int32 value) {
    WriteVarint64(s, (uint64)(value));
}

inline static void WriteInt32NoTag(pb_outstream* s,int32 value) {
    WriteVarint32SignExtended(s, value);
}
inline static void WriteUInt32NoTag(pb_outstream* s,uint32 value) {
    WriteVarint32(s, value);
}

inline static void WriteInt64NoTag(pb_outstream* s, int64 value) {
    WriteVarint64(s, (uint64)(value));
}

inline static void WriteUInt64NoTag(pb_outstream* s,uint64 value) {
    WriteVarint64(s, value);
}

inline static void WriteEnumNoTag(pb_outstream* s, int value) {
    WriteVarint32SignExtended(s, value);
}


void WriteInt32(pb_outstream* s, int field_number, int32_t value) {
    WriteTag(s, field_number, WIRETYPE_VARINT);
    WriteInt32NoTag(s, value);
}

void WriteEnum(pb_outstream* s, int field_number, int32_t value){
    WriteTag(s, field_number, WIRETYPE_VARINT);
    WriteEnumNoTag(s, value);
}

void WriteInt64(pb_outstream* s, int field_number, int64_t value)
{
    WriteTag(s, field_number, WIRETYPE_VARINT);
    WriteInt64NoTag(s, value);
}

void WriteBool(pb_outstream* s, int field_number, PBOOL value){
    WriteTag(s, field_number, WIRETYPE_VARINT);
    WriteInt64NoTag(s, value);
}

void WriteUint32(pb_outstream* s, int field_number, uint32_t value)
{
    WriteTag(s, field_number, WIRETYPE_VARINT);
    WriteUInt32NoTag(s, value);
}
void WriteUint64(pb_outstream* s, int field_number, uint64_t value)
{
    WriteTag(s, field_number, WIRETYPE_VARINT);
    WriteUInt64NoTag(s, value);
}

void WriteBytes(pb_outstream* s, int field_number, pb_string* value) {
    WriteTag(s, field_number, WIRETYPE_LENGTH_DELIMITED);
    WriteVarint32(s, value->size);
    OUT_WriteRaw(s, value->data, value->size);
}

void WriteString(pb_outstream* s, int field_number, pb_string* value) {
    // String is for UTF-8 text only
    WriteTag(s, field_number, WIRETYPE_LENGTH_DELIMITED);
    WriteVarint32(s, value->size);
    OUT_WriteRaw(s, value->data, value->size);
}

void WriteMessage(pb_outstream* s, int field_number, pb_message* value){
    WriteTag(s, field_number, WIRETYPE_LENGTH_DELIMITED);
    const int size = value->GetCachedSize(value);
    WriteVarint32(s, size);
    if(s->buffer_size_ >= (uint32_t)size){
        value->Serialize(value, s);
    }else{
        //impossible;
    }
}

inline static int Log2FloorNonZero_Portable(uint32 n) {
    if (n == 0)
        return -1;
    int log = 0;
    uint32 value = n;
    for (int i = 4; i >= 0; --i) {
        int shift = (1 << i);
        uint32 x = value >> shift;
        if (x != 0) {
            value = x;
            log += shift;
        }
    }
    return log;
}

inline static int Log2FloorNonZero64_Portable(uint64 n) {
    const uint32 topbits = (uint32)(n >> 32);
    if (topbits == 0) {
        // Top bits are zero, so scan in bottom bits
        return (int)(Log2FloorNonZero_Portable((uint32)(n)));
    } else {
        return 32 + (int)(Log2FloorNonZero_Portable(topbits));
    }
}

inline static size_t VarintSize32(uint32 value) {
    // This computes value == 0 ? 1 : floor(log2(value)) / 7 + 1
    // Use an explicit multiplication to implement the divide of
    // a number in the 1..31 range.
    // Explicit OR 0x1 to avoid calling Bits::Log2FloorNonZero(0), which is
    // undefined.
    uint32 log2value = Log2FloorNonZero_Portable(value | 0x1);
    return (size_t)((log2value * 9 + 73) / 64);
}



inline static size_t VarintSize32SignExtended(int32 value) {
    if (value < 0) {
        return 10;     // TODO(kenton):  Make this a symbolic constant.
    } else {
        return VarintSize32((uint32)(value));
    }
}

inline static size_t VarintSize64(uint64 value) {
    // This computes value == 0 ? 1 : floor(log2(value)) / 7 + 1
    // Use an explicit multiplication to implement the divide of
    // a number in the 1..63 range.
    // Explicit OR 0x1 to avoid calling Bits::Log2FloorNonZero(0), which is
    // undefined.
    uint32 log2value = Log2FloorNonZero64_Portable(value | 0x1);
    return (size_t)((log2value * 9 + 73) / 64);
}

size_t Int32Size(int32_t value) {
    return VarintSize32SignExtended(value);
}
size_t Int64Size(int64_t value) {
    return VarintSize64((uint64)(value));
}
size_t BoolSize(PBOOL value){
    return VarintSize64((uint64)(value));
}
size_t UInt32Size(uint32_t value) {
    return VarintSize32(value);
}
size_t UInt64Size(uint64_t value) {
    return VarintSize64(value);
}

size_t EnumSize(int value) {
    return VarintSize32SignExtended(value);
}

inline static size_t LengthDelimitedSize(size_t length) {
    // The static_cast here prevents an error in certain compiler configurations
    // but is not technically correct--if length is too large to fit in a uint32
    // then it will be silently truncated. We will need to fix this if we ever
    // decide to start supporting serialized messages greater than 2 GiB in size.
    return length + VarintSize32((uint32)(length));
}


size_t StringSize(pb_string* value) {
    return LengthDelimitedSize(value->size);
}
size_t BytesSize(pb_string* value) {
    return LengthDelimitedSize(value->size);
}

size_t MessageSize(pb_message* message){
    return LengthDelimitedSize(message->ByteSizeLong(message));
}


uint8_t* pb_message_to_byte_array(struct pb_message* proto,
                                     int* out_len){
    int bytes_size = (int)proto->ByteSizeLong(proto);
    if(bytes_size<=0){
        *out_len = 0;
        return NULL;
    }
    uint8_t* bytes = (uint8_t*)malloc(bytes_size);
    pb_outstream s;
    s.buffer_ = bytes;
    s.buffer_end_ = bytes + bytes_size;
    s.buffer_size_ = bytes_size;
    proto->Serialize(proto, &s);
    *out_len = bytes_size;
    return bytes;
}

int pb_message_serialize_to(struct pb_message* proto,uint8_t* bytes,int out_len){
    int bytes_size = (int)proto->ByteSizeLong(proto);
    if(bytes_size>out_len){
        return 0;
    }
    pb_outstream s;
    s.buffer_ = bytes;
    s.buffer_end_ = bytes + bytes_size;
    s.buffer_size_ = bytes_size;
    proto->Serialize(proto, &s);
    return bytes_size;
}

void pb_message_from_byte_array(struct pb_message* proto,
                                   uint8_t* data,
                                   int len){
    pb_inputstream stream;
    stream.buffer_ = data;
    stream.buffer_end_ = stream.buffer_ + len;
    proto->mergeFromInputStream(proto, &stream);

}



















