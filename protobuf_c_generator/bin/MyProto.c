#include "MyProto.h"
inline static int MyType_GetCachedSize(struct pb_message* message){
    MyType* thiz = (MyType*)message;
    return (int)thiz->_cached_size_;
}
inline static void MyType_Serialize(struct pb_message* message,pb_outstream* s){
    MyType* thiz = (MyType*)message;
    if(MyType_has_i32(thiz)){
        WriteInt32(s, 1, thiz->i32);
    }
}
inline static size_t MyType_ByteSizeLong(struct pb_message* message){
    MyType* thiz = (MyType*)message;
    size_t total_size = 0;
    if(thiz->_cached_size_ != (size_t)(-1)){
        return thiz->_cached_size_;
    }
    if(MyType_has_i32(thiz)){
        total_size += 1 + Int32Size(thiz->i32);
    }
    thiz->_cached_size_ = total_size;
    return thiz->_cached_size_;
}
inline static PBOOL MyType_mergeFromInputStream(struct pb_message* message, pb_inputstream* s){
 #define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
    MyType* thiz = (MyType*)message;
    uint32_t tag = -1;
    for (;;) {
        if(!ReadTag(s, &tag)){
            goto handle_unusual;
        }
        switch(GetTagFieldNumber(tag)){
        case 1:{
            if(tag == 8){
                MyType_set_has_i32(thiz);
                DO_(ReadInt32(s, &thiz->i32));
            }else{
                goto handle_unusual;
            }
            break;
        }
        default:{
            handle_unusual:
            if(tag == 0){
                goto success;
            }else{
                DO_(SkipField(s, tag));
            }
            break;
        }
        }
    }
success:
    return TRUE;
failure:
    return FALSE;
#undef DO_
}
void MyType_init(MyType* proto){
    proto->_cached_size_ = (size_t)(-1);
    proto->_has_bits_[0] = 0;
    proto->_inited= TRUE;
    proto->_imessage.ByteSizeLong = MyType_ByteSizeLong;
    proto->_imessage.GetCachedSize = MyType_GetCachedSize;
    proto->_imessage.mergeFromInputStream = MyType_mergeFromInputStream;
    proto->_imessage.Serialize = MyType_Serialize; 
    proto->i32 = 0;
}
inline static int MyProto_GetCachedSize(struct pb_message* message){
    MyProto* thiz = (MyProto*)message;
    return (int)thiz->_cached_size_;
}
inline static void MyProto_Serialize(struct pb_message* message,pb_outstream* s){
    MyProto* thiz = (MyProto*)message;
    if(MyProto_has_message_type(thiz)){
        WriteEnum(s, 12, (int32_t)thiz->message_type);
    }
    if(MyProto_has_str(thiz)){
        WriteString(s, 1, &thiz->str);
    }
    if(MyProto_has_i32(thiz)){
        WriteInt32(s, 2, thiz->i32);
    }
    if(MyProto_has_i64(thiz)){
        WriteInt64(s, 3, thiz->i64);
    }
    if(MyProto_has_msg(thiz)){
        WriteMessage(s, 4,(pb_message*)&thiz->msg);
    }
}
inline static size_t MyProto_ByteSizeLong(struct pb_message* message){
    MyProto* thiz = (MyProto*)message;
    size_t total_size = 0;
    if(thiz->_cached_size_ != (size_t)(-1)){
        return thiz->_cached_size_;
    }
    if(MyProto_has_message_type(thiz)){
        total_size += 1 + EnumSize((int)thiz->message_type);
    }
    if(MyProto_has_str(thiz)){
        total_size += 1 + StringSize(&thiz->str);
    }
    if(MyProto_has_i32(thiz)){
        total_size += 1 + Int32Size(thiz->i32);
    }
    if(MyProto_has_i64(thiz)){
        total_size += 1 + Int64Size(thiz->i64);
    }
    if(MyProto_has_msg(thiz)){
        total_size += 1 + MessageSize((pb_message*)&thiz->msg);
    }
    thiz->_cached_size_ = total_size;
    return thiz->_cached_size_;
}
inline static PBOOL MyProto_mergeFromInputStream(struct pb_message* message, pb_inputstream* s){
 #define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
    MyProto* thiz = (MyProto*)message;
    uint32_t tag = -1;
    for (;;) {
        if(!ReadTag(s, &tag)){
            goto handle_unusual;
        }
        switch(GetTagFieldNumber(tag)){
        case 12:{
            if(tag == 96){
                MyProto_set_has_message_type(thiz);
                DO_(ReadEnum(s, (int32_t*)&thiz->message_type));
            }else{
                goto handle_unusual;
            }
            break;
        }
        case 1:{
            if(tag == 10){
                MyProto_set_has_str(thiz);
                DO_(ReadString(s, &thiz->str));
            }else{
                goto handle_unusual;
            }
            break;
        }
        case 2:{
            if(tag == 16){
                MyProto_set_has_i32(thiz);
                DO_(ReadInt32(s, &thiz->i32));
            }else{
                goto handle_unusual;
            }
            break;
        }
        case 3:{
            if(tag == 24){
                MyProto_set_has_i64(thiz);
                DO_(ReadInt64(s, &thiz->i64));
            }else{
                goto handle_unusual;
            }
            break;
        }
        case 4:{
            if(tag == 34){
                MyProto_set_has_msg(thiz);
                if(!thiz->msg._inited){
                    MyType_init(&thiz->msg);
                }
                DO_(ReadMessage(s, (pb_message*)&thiz->msg));
            }else{
                goto handle_unusual;
            }
            break;
        }
        default:{
            handle_unusual:
            if(tag == 0){
                goto success;
            }else{
                DO_(SkipField(s, tag));
            }
            break;
        }
        }
    }
success:
    return TRUE;
failure:
    return FALSE;
#undef DO_
}
void MyProto_init(MyProto* proto){
    proto->_cached_size_ = (size_t)(-1);
    proto->_has_bits_[0] = 0;
    proto->_inited= TRUE;
    proto->_imessage.ByteSizeLong = MyProto_ByteSizeLong;
    proto->_imessage.GetCachedSize = MyProto_GetCachedSize;
    proto->_imessage.mergeFromInputStream = MyProto_mergeFromInputStream;
    proto->_imessage.Serialize = MyProto_Serialize; 
    proto->message_type = 0;
    proto->str.data = (uint8_t*)"hello world";
    proto->str.size = (uint32_t)11;
    proto->i32 = 0;
    proto->i64 = 0;
    proto->msg._inited = FALSE;
}


