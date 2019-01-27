#ifndef PROTOBUF_C_H
#define PROTOBUF_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef PBOOL
#define PBOOL int
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

typedef uint64_t uint64;
typedef int64_t int64;
typedef uint32_t uint32;
typedef int32_t int32;

typedef enum WireType {
  WIRETYPE_VARINT           = 0,
  WIRETYPE_FIXED64          = 1,
  WIRETYPE_LENGTH_DELIMITED = 2,
  WIRETYPE_START_GROUP      = 3,
  WIRETYPE_END_GROUP        = 4,
  WIRETYPE_FIXED32          = 5,
}WireType;

// Lite alternative to FieldDescriptor::Type.  Must be kept in sync.
typedef enum FieldType {
  TYPE_DOUBLE         = 1,
  TYPE_FLOAT          = 2,
  TYPE_INT64          = 3,
  TYPE_UINT64         = 4,
  TYPE_INT32          = 5,
  TYPE_FIXED64        = 6,
  TYPE_FIXED32        = 7,
  TYPE_BOOL           = 8,
  TYPE_STRING         = 9,
  TYPE_GROUP          = 10,
  TYPE_MESSAGE        = 11,
  TYPE_BYTES          = 12,
  TYPE_UINT32         = 13,
  TYPE_ENUM           = 14,
  TYPE_SFIXED32       = 15,
  TYPE_SFIXED64       = 16,
  TYPE_SINT32         = 17,
  TYPE_SINT64         = 18,
  MAX_FIELD_TYPE      = 18,
}FieldType;

typedef struct pb_inputstream{
    uint8_t* buffer_;
    uint8_t* buffer_end_;
}pb_inputstream;

typedef struct pb_outstream{
    uint8_t* buffer_;
    uint8_t* buffer_end_;
    uint32_t buffer_size_;
}pb_outstream;

typedef struct pb_string{
    uint8_t* data;
    uint32_t size;
}pb_string;




typedef struct pb_message{
    int (*GetCachedSize)(struct pb_message* message);
    void (*Serialize)(struct pb_message* message,pb_outstream* s);
    size_t (*ByteSizeLong)(struct pb_message* message);
    PBOOL (*mergeFromInputStream)(struct pb_message* message, pb_inputstream* stream);

}pb_message;

uint8_t* pb_message_to_byte_array(struct pb_message* proto, int* out_len);
int pb_message_serialize_to(struct pb_message* proto,uint8_t* bytes,int out_len);
void pb_message_from_byte_array(struct pb_message* proto, uint8_t* data,
                              int len);

PBOOL ReadInt32(pb_inputstream* s, int32_t* out);
PBOOL ReadUint32(pb_inputstream* s, uint32_t* out);

PBOOL ReadUint64(pb_inputstream* s, uint64_t* out);
PBOOL ReadInt64(pb_inputstream* s, int64_t* out);

PBOOL ReadBool(pb_inputstream* s, PBOOL* out);
PBOOL ReadEnum(pb_inputstream* s, int32_t* out);

PBOOL ReadBytes(pb_inputstream* s, pb_string* value);
PBOOL ReadString(pb_inputstream* s, pb_string* value);
PBOOL ReadMessage(pb_inputstream* s, pb_message* value);
PBOOL ReadTag(pb_inputstream* s, uint32_t* value);
PBOOL SkipField(pb_inputstream* s, uint32_t tag);

// Write fields, including tags.
void WriteInt32(pb_outstream* s, int field_number, int32_t value);
void WriteInt64(pb_outstream* s, int field_number, int64_t value);
void WriteUint32(pb_outstream* s, int field_number, uint32_t value);
void WriteUint64(pb_outstream* s, int field_number, uint64_t value);
void WriteBytes(pb_outstream* s, int field_number, pb_string* value);
void WriteString(pb_outstream* s, int field_number, pb_string* value);
void WriteEnum(pb_outstream* s, int field_number, int32_t value);
void WriteBool(pb_outstream* s, int field_number, PBOOL value);
void WriteMessage(pb_outstream* s, int field_number, pb_message* value);

size_t Int32Size(int32_t value);
size_t Int64Size(int64_t value);
size_t BoolSize(PBOOL value);
size_t UInt32Size(uint32_t value);
size_t UInt64Size(uint64_t value);
size_t EnumSize(int value);
size_t StringSize(pb_string* value);
size_t BytesSize(pb_string* value);
size_t MessageSize(pb_message* message);

/** inline */
inline static int GetTagFieldNumber(uint32_t tag) {
  return (int)(tag >> 3);
}

inline WireType GetTagWireType(uint32_t tag) {
  return (WireType)(tag & ((1 << 3) - 1));
}

#define GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(FIELD_NUMBER, TYPE)                  \
    (uint32_t)(                                                   \
    ((uint32_t)(FIELD_NUMBER) << 3) \
    | (TYPE))

inline static uint32_t MakeTag(int field_number, WireType type) {
    return GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(field_number, type);
}



#ifdef __cplusplus
}
#endif

#endif // PROTOBUF_C_H
