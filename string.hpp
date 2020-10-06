

#ifndef _CSF_STRING_HPP
#define _CSF_STRING_HPP


#include "global.hpp"

namespace csf
{

using csf_char_t = wchar_t;

class csf_string
{
private:

    static const uint32_t MAGIC      =  0x53545220;
    static const uint32_t MAGIC_W    =  0x53545257;

    static const uint32_t MAX_VALUE_LEN =   1529;
    static const uint32_t MAX_EXTRA_LEN =   255;


    uint32_t m_value_len;
    csf_char_t m_value[MAX_VALUE_LEN + 1];
    uint32_t m_extra_len;
    uint8_t m_extra[MAX_EXTRA_LEN + 1];

public:

    csf_string() :m_value_len(0), m_extra_len(0)
    {
        m_value[0] = '\0';
        m_extra[0] = '\0';
    }

    void read_from_file(file_reader& r) 
    {
        uint32_t magic = r.read_int();
        switch (magic) {
            case MAGIC:
            case MAGIC_W:
                break;
            default:
                abort_if(1, "invalid csf_string magic: %#x at %#x, expected %#x\n", 
                    magic, r.where(), MAGIC);
        }

        m_value_len = r.read_int();

        abort_if(m_value_len > MAX_VALUE_LEN, 
            "too long string length %u at %#x, max length is %u\n",
            m_value_len, r.where(), MAX_VALUE_LEN);

        r.read_bytes(m_value, m_value_len * sizeof(csf_char_t));
        m_value[m_value_len] = '\0';

        if (magic == MAGIC_W) {
            m_extra_len = r.read_int();

            abort_if(m_extra_len > MAX_EXTRA_LEN, 
                "too long extra length %u at %#x, max length is %u\n",
                m_extra_len, r.where(), MAX_EXTRA_LEN);

            r.read_bytes(m_extra, m_extra_len);
            m_extra[m_extra_len] = '\0';
        }
    }


    void write_to_file(file_writer& w)
    {
        uint32_t magic = m_extra_len > 0 ? MAGIC_W : MAGIC;
        w.write_bytes(magic);

        w.write_bytes(m_value_len);
        w.write_bytes(m_value, m_value_len * sizeof(csf_char_t));

        if (m_extra_len > 0) {
            w.write_bytes(m_extra_len);
            w.write_bytes(m_extra, m_extra_len);
        }
    }

    char* get_value(int *p_size = nullptr) 
    {
        // 1. 对于每个字节，按字节翻转
        csf_char_t buff[MAX_VALUE_LEN + 1];
        for (size_t i = 0, n = sizeof(csf_char_t) * m_value_len; i < n; i++) {
            ((uint8_t*) buff)[i] = 0xFF - ((uint8_t*) m_value)[i];
        }
        buff[m_value_len] = '\0';

        // 2. 按照字符数的 3 倍空间申请字节缓冲区
        size_t buff_len = 3 * m_value_len;
        char *str = new char[buff_len + 1];
        int str_len = wcstombs(str, (wchar_t*) buff, buff_len);

        if (str_len < 0) {
            delete[] str;
            str = nullptr;
            str_len = 0;
        }
        if (p_size) *p_size = str_len;
        return str;
    }

    char* get_extra(int *p_size = nullptr)
    {
        if (p_size) *p_size = m_extra_len;

        if (m_extra_len == 0) {
            return nullptr;
        }

        char *buff = new char[m_extra_len + 1];
        memcpy(buff, m_extra, m_extra_len + 1);
        return buff;
    }

    void set_extra(const char *src)
    {
        if (src == nullptr) {
            m_extra_len = 0;
            m_extra[0] = '\0';
            return;
        }

        int len = strlen(src);

        abort_if(len > MAX_EXTRA_LEN, "too long extra length %u, max is %u\n",
            len, MAX_EXTRA_LEN);

        memcpy(m_extra, src, len);
        m_extra_len = len;
        m_extra[m_extra_len] = '\0';
    }

    void set_value(const char *src)
    {
        if (src == nullptr) {
            m_value_len = 0;
            m_value[0] = '\0';
            return;
        }

        int n = mbstowcs((wchar_t*) m_value, src, MAX_VALUE_LEN);
        abort_if(n == -1, "invalid multi bytes string\n");

        m_value_len = n;

        // 反转字节
        for (int i = 0; i < n * sizeof(wchar_t); i++) {
            ((uint8_t *) m_value)[i] = 0xFF - ((uint8_t *) m_value)[i];
        }
    }
};

};

#endif

