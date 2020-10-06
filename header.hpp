

#ifndef _CSF_HEADER_HPP
#define _CSF_HEADER_HPP


#include "global.hpp"



namespace csf
{

class csf_header
{

private:
    static const uint32_t MAGIC = 0x43534620;
    static const uint32_t VERSION = 3;

    static const uint32_t LANGUAGE_ENGLISH_AMERACAN =   0;
    static const uint32_t LANGUAGE_ENGLISH_BRITISH  =   1;
    static const uint32_t LANGUAGE_GERMAN           =   2;
    static const uint32_t LANGUAGE_FRENCH           =   3;
    static const uint32_t LANGUAGE_SPANISH          =   4;
    static const uint32_t LANGUAGE_ITALIAN          =   5;
    static const uint32_t LANGUAGE_JAPANESE         =   6;
    static const uint32_t LANGUAGE_MEANINGLESS      =   7;
    static const uint32_t LANGUAGE_KOREAN           =   8;
    static const uint32_t LANGUAGE_CHINESE          =   9;       
    static const uint32_t LANGUAGE_UNKNOWN          =   10;


    uint32_t m_label_num;
    uint32_t m_string_num;
    uint32_t m_language;

public:
    csf_header() : m_label_num(0), m_string_num(0), m_language(LANGUAGE_ENGLISH_AMERACAN) 
    {
    }


    void read_from_file(file_reader& r)
    {
        uint32_t magic = r.read_int();
        abort_if(magic != MAGIC, "invalid csf_header magic: %#x at %#x, expected %#x\n",
            magic, r.where(), MAGIC);

        uint32_t version = r.read_int();
        abort_if(version != VERSION, "invalid csf_header version: %u at %#x, expected %u\n", 
            version, r.where(), VERSION);

        m_label_num = r.read_int();
        m_string_num = r.read_int();
        uint32_t unknown = r.read_int();
        
        m_language = r.read_int();
        abort_if(m_language >= LANGUAGE_UNKNOWN, "invalid csf_header language: %u at %#x\n",
            m_language, r.where());
    }

    void write_to_file(file_writer& w)
    {
        w.write_bytes(MAGIC);
        w.write_bytes(VERSION);
        w.write_bytes(m_label_num);
        w.write_bytes(m_string_num);
        w.write_bytes((uint32_t) 0);
        w.write_bytes(m_language);
    }



    uint32_t get_label_num() { return m_label_num; }

    uint32_t get_string_num() { return m_string_num; }


    void set_label_num(uint32_t label) { m_label_num = label; }

    void set_string_num(uint32_t string) { m_string_num = string; }
};
};


#endif