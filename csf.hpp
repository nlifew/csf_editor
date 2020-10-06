

#ifndef _CSF_FILE_HPP
#define _CSF_FILE_HPP


#include "header.hpp"
#include "label.hpp"
#include "string_utils.hpp"

namespace csf
{

class csf_file
{
private:
    csf_header m_header;
    hashmap m_labels;

public:

    csf_file()
    {
        hashmap_options ops = {
            .capacity = 1024 * 8,
            .load_factor = 0.75f,
            .access_order = 0,
            .hash = (int (*)(const void*)) string_utils::hash,
            .cmp = (int (*)(const void*, const void*)) ::strcmp,
        };
        hashmap_setup(&m_labels, &ops);
    }

    csf_file(const csf_file&) = delete;
    csf_file& operator=(const csf_file&) = delete;

    ~csf_file()
    {
        int n = 0;
        auto labels = (csf_label**) hashmap_values(&m_labels, &n);
        for (int i = 0; i < n; i++) {
            delete labels[i];
        }
        free(labels);
        hashmap_destroy(&m_labels);
    }


    int size() { return m_labels.size; }


    csf_label** children(int *p_size) 
    {
        // 包裹一层，这样外部类就能通过 delete[] 释放内存

        int n = 0;
        auto src = (csf_label**) hashmap_values(&m_labels, &n);
        auto dst = new csf_label*[n];
        memcpy(dst, src, sizeof(csf_label*) * n);
        free(src);

        if (p_size) *p_size = n;
        return dst;
    }


    csf_label* find(const char *name)
    {
        return (csf_label *) hashmap_get(&m_labels, name);
    }

    void remove(const char *name)
    {
        auto label = (csf_label *) hashmap_remove(&m_labels, name, nullptr);
        delete label;
    }

    void insert(csf_label *label)
    {
        // 创建一个副本
        label = new csf_label(*label);

        // printf("[insert after][%p]\n", label);

        auto old = (csf_label*) hashmap_put(&m_labels, label->name(), label, nullptr);
        delete old;
    }

    void read_from_file(file_reader& r)
    {
        m_header.read_from_file(r);
        for (int i = 0, n = m_header.get_label_num(); i < n; i++) {
            csf_label *label = new csf_label();
            label->read_from_file(r);
            auto old = (csf_label*) hashmap_put(&m_labels, label->name(), label, nullptr);
            delete old;
        }
    }


    void write_to_file(file_writer& w)
    {
        // 先写入 header 占位
        const unsigned long where = w.where();
        m_header.write_to_file(w);
        

        int n = 0;
        csf_label **labels = (csf_label**) hashmap_values(&m_labels, &n);
        
        // 遍历并写入到文件
        int string_num = 0;
        for (int i = 0; i < n; i++) {
            // printf("[write_to_file][%p]\n", labels[i]);
            string_num += labels[i]->size();
            labels[i]->write_to_file(w);
        }
        free(labels);

        // 需要重新计算 string_num
        w.jump(where, SEEK_SET);

        m_header.set_string_num(string_num);
        m_header.set_label_num(n);
        m_header.write_to_file(w);
    }

};

};


#endif