

#ifndef _CSF_GLOBAL_HPP
#define _CSF_GLOBAL_HPP


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "hashmap.h"

namespace csf
{

#define abort_if(x, fmt, ...) if (x) { \
    fprintf(stderr, fmt "%s(%d)\n", ##__VA_ARGS__, strerror(errno), errno); \
    exit(1); \
}

class file_reader
{
private:
    FILE *m_fp;

public:
    explicit file_reader(const char *name) { m_fp = fopen(name, "rb"); }


    ~file_reader() { fclose(m_fp); m_fp = nullptr; }


    unsigned long where() { return ftell(m_fp); }


    void read_bytes(void *dst, size_t len) 
    {
        size_t n = fread(dst, 1, len, m_fp);
        abort_if(n != len, "expected read %lu bytes, actually %lu bytes.\n", len, n);
    }


    uint8_t read_char() { uint8_t t; read_bytes(&t, sizeof(uint8_t)); return t; }

    uint16_t read_short() { uint16_t t; read_bytes(&t, sizeof(uint16_t)); return t; }

    uint32_t read_int() { uint32_t t; read_bytes(&t, sizeof(uint32_t)); return t; }

    uint64_t read_long() { uint64_t t; read_bytes(&t, sizeof(uint64_t)); return t; }
};



class file_writer
{
private:
    FILE *m_fp;

public:
    explicit file_writer(const char *name) { m_fp = fopen(name, "wb"); }

    ~file_writer() { fclose(m_fp); m_fp = nullptr; }


    void jump(long offset, int from) { fseek(m_fp, offset, from); }


    unsigned long where() { return ftell(m_fp); }


    void write_bytes(void *dst, size_t len)
    {
        size_t n = fwrite(dst, 1, len, m_fp);
        abort_if(n != len, "expected write %lu bytes, actually %lu bytes.\n", len, n);
    }

    // template <typename T>
    // void write_bytes(T *dst) { write_bytes(dst, sizeof(T)); }


    template <typename T>
    void write_bytes(T t) { write_bytes(&t, sizeof(T)); }
};

}



#endif