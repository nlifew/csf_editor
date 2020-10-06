


#ifndef _CSF_CMDLINE_HPP
#define _CSF_CMDLINE_HPP

#include "global.hpp"

namespace csf
{

class cmdline
{
private:
    static const uint32_t MAX_BUFF_LEN = 2047;

    unsigned char m_buff[MAX_BUFF_LEN + 1]; // [1]
    int m_buff_len, m_buff_offset;
    int m_next_len;

    // [1] 关于为什么是 unsigned char[] 而不是 char[]
    // 为了过滤掉空格等字符，需要这样的判断 m_buff[i] <= ' '
    // 如果是 char[]，遇到非 ascii 字符时，m_buff[i] 可能 < 0
    // 从而判断失败

public:

    cmdline() : m_buff_len(0), m_buff_offset(0), m_next_len(0) 
    { 
        m_buff[0] = '\0'; 
    }


    int read_line()
    {
        fgets((char*) m_buff, MAX_BUFF_LEN, stdin);

        m_buff_offset = 0;
        m_buff_len = strlen((char*) m_buff);

        return feof(stdin) || ferror(stdin);
    }


    bool has_next()
    {
        m_next_len = 0;

        while (m_buff_offset < m_buff_len && m_buff[m_buff_offset] <= ' ') {
            m_buff_offset ++;
        }
        if (m_buff_offset >= m_buff_len) {
            return false;
        }

        int i = m_buff_offset; int quotations = 0;
        while (i < m_buff_len && (m_buff[i] > ' ' || (quotations & 1))) {
            if (m_buff[i] == '"') quotations ++;
            i ++;
            m_next_len ++;
        }
        m_buff[i] = '\0';
        return true;
    }


    char* get(int *p_len = nullptr) 
    {
        if (p_len) *p_len = m_next_len;
        return (char*) (m_buff + m_buff_offset);
    }


    char* next(int *p_len = nullptr)
    {
        if (p_len) *p_len = m_next_len;

        auto str = (char*) (m_buff + m_buff_offset);

        m_buff_offset += m_next_len;
        m_next_len = 0;

        return str;
    }

};


};


#endif