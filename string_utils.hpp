


#ifndef _CSF_STRING_WRAPPER_HPP
#define _CSF_STRING_WRAPPER_HPP



namespace csf
{

class string_utils
{
private:
    string_utils() = delete;

public:
    static bool is_empty(const char *str) { return str == nullptr || *str == '\0'; }


    static int hash(const char *ptr) 
    {
        int hash = 0;
        for (auto str = (const char*) ptr; *str; str ++) {
            hash += 31 * (*str);
        }
        return hash;
    }


    static bool starts_with(const char *str, const char *prefix)
    {
        if (str == nullptr || prefix == nullptr) return false;

        int m = strlen(str), n = strlen(prefix);
        if (n == 0) return m == 0;
        if (n > m) return false;

        return memcmp(str, prefix, n) == 0;
    }
};
};
#endif