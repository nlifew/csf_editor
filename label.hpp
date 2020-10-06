


#ifndef _CSF_LABEL_HPP
#define _CSF_LABEL_HPP


#include "string.hpp"
#include <vector>

namespace csf
{
class csf_label
{

private:
    static const uint32_t MAGIC = 0x4c424c20;
    static const uint32_t MAX_NAME_LEN = 255;

    uint32_t m_name_len;
    uint8_t m_name[MAX_NAME_LEN + 1];

    std::vector<csf_string*> m_strings;

public:

    csf_label() : m_name_len(0) { m_name[0] = '\0'; }

    ~csf_label()
    {
        for (auto p : m_strings) {
            delete p;
        }
    }

    csf_label(const csf_label& o)
    {
        m_name_len = o.m_name_len;
        memcpy(m_name, o.m_name, m_name_len + 1);

        for (auto p : o.m_strings) {
            m_strings.push_back(new csf_string(*p));
        }
    }


    int size() { return m_strings.size(); }


    csf_string* get(int idx) { return m_strings.at(idx); }


    void set(int idx, csf_string *src)
    {
        const csf_string& o = *src;
        csf_string *dst = new csf_string(o);
        m_strings[idx] = dst;
    }

    void add(int idx, csf_string *src)
    {
        const csf_string& o = *src;
        csf_string *dst = new csf_string(o);

        // printf("[%p] m_value_len[%d], m_extra_len[%d]\n", dst, dst->m_value_len, dst->m_extra_len);

        m_strings.insert(m_strings.begin() + idx, dst);
    }

    void remove(int idx)
    {
        m_strings.erase(m_strings.begin() + idx);
    }

    csf_string** children(int *p_size)
    {
        const int N = m_strings.size();
        csf_string **tables = new csf_string*[N];

        int i = 0;
        for (auto p : m_strings) {
            tables[i ++] = p;
        }

        if (p_size) *p_size = N;
        return tables;
    }

    const char *name() { return (char*) m_name; }


    void set_name(const char *name) 
    {
        int n;
        if (name == nullptr || (n = strlen(name)) == 0) {
            m_name[0] = '\0';
            m_name_len = 0;
            return;
        }
        abort_if(n > MAX_NAME_LEN, "name length too long %d, max is %d\n", n, MAX_NAME_LEN);

        memcpy(m_name, name, n + 1);
        m_name_len = n;
    }


    void read_from_file(file_reader& r)
    {
        uint32_t magic = r.read_int();
        abort_if(magic != MAGIC, "invalid csf_label magic: %#x at %#x, expected %#x\n",
            magic, r.where(), MAGIC);

        uint32_t string_len = r.read_int();


        m_name_len = r.read_int();
        abort_if(m_name_len > MAX_NAME_LEN, "too long name length: %u at %#x, max is %u\n",
            m_name_len, r.where(), MAX_NAME_LEN);
        r.read_bytes(m_name, m_name_len);
        m_name[m_name_len] = '\0';

        for (auto p : m_strings) delete p;
        m_strings.clear();

        for (int i = 0; i < string_len; i++) {
            csf_string *str = new csf_string();
            str->read_from_file(r);
            m_strings.push_back(str);
        }
    }

    void write_to_file(file_writer& w)
    {
        w.write_bytes(MAGIC);
        w.write_bytes((uint32_t) m_strings.size()); // [1]
        w.write_bytes(m_name_len);
        w.write_bytes(m_name, m_name_len);

        // [1]: vector.size() 返回值为 size_type，
        // 必须将其强制转化为 32 位类型

        for (auto p : m_strings) {
            // printf("[write_to_file][%p]\n", p);
            p->write_to_file(w);
        }
    }
};
};

#endif