#include <QCoreApplication>
#include "MyProto.h"

static const char hexdig[] = "0123456789abcdef";
static void log_hex(const char* tag, unsigned char* data, int len){
    char msg[50], *ptr;
    int i;
    ptr = msg;

    printf("%s\r\n", tag);
    for(i=0; i<len; i++) {
        *ptr++ = hexdig[0x0f & (data[i] >> 4)];
        *ptr++ = hexdig[0x0f & data[i]];
        if ((i & 0x0f) == 0x0f) {
            *ptr = '\0';
            ptr = msg;
            printf("%s\r\n", msg);
        } else {
            *ptr++ = ' ';
        }
    }
    if (i & 0x0f) {
        *ptr = '\0';
        printf("%s\r\n", msg);
    }
}

#define PB_STR(str) {(uint8_t*)str, (uint32_t)strlen(str)}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    uint8_t* data;
    int out_len;
    {
        MyType mytype;
        MyProto myproto;

        MyType_init(&mytype);
        MyType_set_i32(&mytype, 123);

        MyProto_init(&myproto);
        MyProto_set_i32(&myproto, 456);
        MyProto_set_i64(&myproto, 789);
        pb_string str = PB_STR("hello");
        MyProto_set_str(&myproto, &str);
        MyProto_set_msg(&myproto, &mytype);

        data = MyProto_to_byte_array(&myproto, &out_len);
        log_hex("", data, out_len);
    }

    {
        MyProto myproto;
        MyProto_init(&myproto);
        MyProto_from_byte_array(&myproto, data, out_len);
        printf("%.*s", myproto.str.size, myproto.str.data);

    }


    return a.exec();
}
