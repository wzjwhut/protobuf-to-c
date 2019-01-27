#ifndef pb_MYPROTO_H
#define pb_MYPROTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "protobuf_c.h"

#ifndef PBOOL
#define PBOOL int
#endif
typedef struct MyType {
    pb_message _imessage;
    uint32_t _has_bits_[1];
    size_t _cached_size_;
    PBOOL _inited;
    int32_t i32;
} MyType;
typedef enum MessageType {
    MessageType_TYPE1 = 1,
    MessageType_TYPE2 = 2,
} MessageType;
typedef struct MyProto {
    pb_message _imessage;
    uint32_t _has_bits_[1];
    size_t _cached_size_;
    PBOOL _inited;
    MessageType message_type;
    pb_string str;
    int32_t i32;
    int64_t i64;
    MyType msg;
} MyProto;
void MyType_init(MyType* proto);
inline static PBOOL MyType_has_i32(MyType* proto) {
    return (proto->_has_bits_[0] & 0x1u) != 0;
}
inline static void MyType_set_has_i32(MyType* proto) {
    proto->_has_bits_[0] |= 0x1u;
}
inline static void MyType_set_i32(MyType* proto, const int32_t value){
    proto->_has_bits_[0] |= 0x1u;
    proto->i32 = value;
}
inline static uint8_t* MyType_to_byte_array(MyType* proto, int* out_len){
    return pb_message_to_byte_array(&proto->_imessage, out_len);
}

inline static void MyType_from_byte_array(MyType* proto, uint8_t* data,
                              int len){
    pb_message_from_byte_array(&proto->_imessage, data, len);
}
inline static size_t MyType_bytes_size(MyType* proto){
    return proto->_imessage.ByteSizeLong(&proto->_imessage);
}

inline static int MyType_serialize_to(MyType* proto, uint8_t* dest, int dest_len){
    return pb_message_serialize_to(&proto->_imessage, dest, dest_len);
}
void MyProto_init(MyProto* proto);
inline static PBOOL MyProto_has_message_type(MyProto* proto) {
    return (proto->_has_bits_[0] & 0x1u) != 0;
}
inline static void MyProto_set_has_message_type(MyProto* proto) {
    proto->_has_bits_[0] |= 0x1u;
}
inline static void MyProto_set_message_type(MyProto* proto, const MessageType value){
    proto->_has_bits_[0] |= 0x1u;
    proto->message_type = value;
}
inline static PBOOL MyProto_has_str(MyProto* proto) {
    return (proto->_has_bits_[0] & 0x2u) != 0;
}
inline static void MyProto_set_has_str(MyProto* proto) {
    proto->_has_bits_[0] |= 0x2u;
}
inline static void MyProto_set_str(MyProto* proto, const pb_string* value){
    proto->_has_bits_[0] |= 0x2u;
    proto->str = *value;
}
inline static PBOOL MyProto_has_i32(MyProto* proto) {
    return (proto->_has_bits_[0] & 0x4u) != 0;
}
inline static void MyProto_set_has_i32(MyProto* proto) {
    proto->_has_bits_[0] |= 0x4u;
}
inline static void MyProto_set_i32(MyProto* proto, const int32_t value){
    proto->_has_bits_[0] |= 0x4u;
    proto->i32 = value;
}
inline static PBOOL MyProto_has_i64(MyProto* proto) {
    return (proto->_has_bits_[0] & 0x8u) != 0;
}
inline static void MyProto_set_has_i64(MyProto* proto) {
    proto->_has_bits_[0] |= 0x8u;
}
inline static void MyProto_set_i64(MyProto* proto, const int64_t value){
    proto->_has_bits_[0] |= 0x8u;
    proto->i64 = value;
}
inline static PBOOL MyProto_has_msg(MyProto* proto) {
    return (proto->_has_bits_[0] & 0x10u) != 0;
}
inline static void MyProto_set_has_msg(MyProto* proto) {
    proto->_has_bits_[0] |= 0x10u;
}
inline static void MyProto_set_msg(MyProto* proto, const MyType* value){
    proto->_has_bits_[0] |= 0x10u;
    proto->msg = *value;
}
inline static uint8_t* MyProto_to_byte_array(MyProto* proto, int* out_len){
    return pb_message_to_byte_array(&proto->_imessage, out_len);
}

inline static void MyProto_from_byte_array(MyProto* proto, uint8_t* data,
                              int len){
    pb_message_from_byte_array(&proto->_imessage, data, len);
}
inline static size_t MyProto_bytes_size(MyProto* proto){
    return proto->_imessage.ByteSizeLong(&proto->_imessage);
}

inline static int MyProto_serialize_to(MyProto* proto, uint8_t* dest, int dest_len){
    return pb_message_serialize_to(&proto->_imessage, dest, dest_len);
}


#ifdef __cplusplus
}
#endif

#endif // pb_MYPROTO_H


