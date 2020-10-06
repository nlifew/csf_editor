

#include "csf.hpp"
#include "cmdline.hpp"
#include <limits.h>
#include <unistd.h>
#include <locale.h>
#include <regex>

#define VERSION "0.1"

using namespace csf;




static int cmd_open(cmdline& cmd);
static int cmd_insert(cmdline& cmd);
static int cmd_remove(cmdline& cmd);
static int cmd_list(cmdline& cmd);
static int cmd_save(cmdline& cmd);
static int cmd_close(cmdline& cmd);
static int cmd_exit(cmdline& cmd);
static int cmd_quit(cmdline& cmd);
static int cmd_help(cmdline& cmd);
static int cmd_version(cmdline& cmd);
// static int cmd_fix(cmdline& cmd);


typedef struct
{
    int (*func)(cmdline&);
    const char *s_cmd;
    const char *l_cmd;
    const char *usage;
} function;


static function FUNCTIONS[] = {
    {cmd_open,      "o",   "open",     " FILE_NAME                                 open a .csf file and close before\n"},
    {cmd_insert,    "i",   "insert",   " --key=KEY --value=VALUE [--extra=EXTRA]   insert a new label to .csf, and the old will be overrided\n"},
    {cmd_remove,    "r",   "remove",   " KEY                                       remove a key from .csf file\n"
                                        "                                          If no one existed, nothing happened\n"},
    {cmd_list,      "l",   "list",     " [KEY]                                     list matched regex or all items\n"},
    {cmd_save,      "s",   "save",     " [FILE_NAME]                               save all items or save as a new .csf file\n"},
    {cmd_close,     "c",   "close",    "                                           close the working .csf file\n"},
    {cmd_exit,      "e",   "exit",     "                                           save, close and exit\n"},
    {cmd_quit,      "q",   "quit",     "                                           quit but no save\n"},
    {cmd_help,      "h",   "help",     "                                           show help info\n"},
    {cmd_version,   "v",   "version",  "                                           show current version\n"},
    // {cmd_fix,       "f",   "fix",      " [FILE_NAME]                               remove multi-value in each label\n"}
};


static void setup_functions(hashmap *map)
{
    const int N = sizeof(FUNCTIONS) / sizeof(FUNCTIONS[0]);

    hashmap_options ops = {
        .capacity = N,
        .load_factor = 0.75f,
        .access_order = 1,
        .hash = (int (*)(const void*)) string_utils::hash,
        .cmp = (int (*)(const void*, const void*)) ::strcmp,
    };
    hashmap_setup(map, &ops);
    for (int i = 0; i < N; i++) {
        function *f = FUNCTIONS + i;
        hashmap_put(map, f->s_cmd, (const void*) f->func, nullptr);
        hashmap_put(map, f->l_cmd, (const void*) f->func, nullptr);
    }
}




static void usage()
{
    const char *header = "RedAlert csf Editor version %s\n"
            "Usage: This tool seems like a simple shell, here are some commands.\n";
    printf("%s\n", VERSION);

    const int N = sizeof(FUNCTIONS) / sizeof(FUNCTIONS[0]);
    for (int i = 0; i < N; i++) {
        function *f = FUNCTIONS + i;
        printf("%s %s %s", f->s_cmd, f->l_cmd, f->usage);
    }
}



static csf_file *m_csf_file = nullptr;
static char csf_path[PATH_MAX + 1];


int main(int argc, char const *argv[])
{
    setlocale(LC_ALL, "");

    csf_path[0] = '\0';

    hashmap functions;
    setup_functions(&functions);

    cmdline cmd;

    do {
        printf("[%s]:$ ", csf_path);

        if (cmd.read_line()) {
            break;
        }
        if (! cmd.has_next()) {
            continue;
        }

        // while (cmd.has_next()) {
        //     int n = 0;
        //     const char *s = cmd.next(&n);
        //     printf("[%s][%d]\n", s, n);
        // }

        auto function = (int (*)(cmdline&)) hashmap_get(&functions, cmd.next());


        if (function == nullptr) {
            printf("unknown cmdline, type help to show help info\n");
        }
        else if (function(cmd) < 0) {
            break;
        }

    } while (1);

    hashmap_destroy(&functions);
    return 0;
}



int cmd_open(cmdline& cmd)
{
    // 先尝试关闭当前工作内容
    cmdline dummy;
    cmd_close(dummy);

    if (! cmd.has_next()) {
        printf("you need to append a FILE_NAME param\n");
        return 1;
    }

    const char *file_name = "";

    if (access((file_name = cmd.next()), R_OK)) {
        printf("can't open file [%s], is it exist ?\n", file_name);
        return 1;
    }

    file_reader r(file_name);
    m_csf_file = new csf_file();
    m_csf_file->read_from_file(r);
    strncpy(csf_path, file_name, PATH_MAX);

    return 0;
}

int cmd_insert(cmdline& cmd)
{
    if (m_csf_file == nullptr) {
        m_csf_file = new csf_file();
    }


    while (cmd.has_next()) {
        const char *key = cmd.next();

        if (! string_utils::starts_with(key, "--key=")) {
            printf("invalid key prefix, use --key=KEY\n");
            return 1;
        }
        key += 6;   /* strlen("--key=") */
        if (string_utils::is_empty(key)) {
            printf("key is empty\n");
            return 1;
        }


        csf_label label;
        label.set_name(key);


        const char *value;

        while (cmd.has_next() && string_utils::starts_with(value = cmd.get(), "--value=")) {
            value += 8; /* strlen("--value=") */
            cmd.next();

            csf_string str;
            str.set_value(value);

            const char *extra;
            if (cmd.has_next() && string_utils::starts_with(extra = cmd.get(), "--extra=")) {
                extra += 8; /* strlen("--extra=") */
                cmd.next();
                str.set_extra(extra);
            }

            label.add(label.size(), &str);
        }

        if (label.size() == 0) {
            printf("invalid value prefix, use --value=VALUE\n");
            return 1;
        }

        m_csf_file->insert(&label);
    }

    return 0;
}

int cmd_remove(cmdline& cmd)
{
    if (m_csf_file == nullptr) {
        return 1;
    }

    if (! cmd.has_next()) {
        printf("you need to append a KEY param to remove it.\n");
        return 1;
    }

    const char *key = cmd.next();
    m_csf_file->remove(key);

    return 0;
}

int cmd_list(cmdline& cmd)
{
    if (m_csf_file == nullptr) {
        return 1;
    }

    std::regex *r = nullptr;
    if (cmd.has_next()) {
        r = new std::regex(cmd.next());
    }


    int n = 0;
    csf_label **labels = m_csf_file->children(&n);

    for (int i = 0; i < n; i++) {

        if (r && ! std::regex_match(labels[i]->name(), *r)) {
            continue;
        }


        printf("[%s]\t", labels[i]->name());

        int z = 0;
        csf_string **strings = labels[i]->children(&z);
        for (int j = 0; j < z; j++) {
            char *value = strings[j]->get_value();
            char *extra = strings[j]->get_extra();

            printf("[%s]\t[%s] ", value, extra);

            delete[] value;
            delete[] extra;
        }
        delete[] strings;
        printf("\n");
    }
    delete[] labels;

    delete r;
    return 0;
}


int cmd_save(cmdline& cmd)
{
    if (m_csf_file == nullptr) {
        return 0;
    }

    const char *file_name = "";
    if (cmd.has_next()) {               // 如果指定了参数，另存为这个文件
        file_name = cmd.next();
    }
    else if (csf_path[0] != '\0') {     // 如果已经打开了文件，保存到原文件
        file_name = csf_path;
    }
    else {                              // 如果存在临时文件，警告用户内容会丢失
        printf("temp file will be lost, append a FILE_NAME param to save as.\n");
        return 1;
    }


    file_writer w(file_name);
    m_csf_file->write_to_file(w);
    strncpy(csf_path, file_name, PATH_MAX);

    return 0;
}


int cmd_close(cmdline& cmd)
{
    // 先保存当前工作内容
    cmdline dummy;
    cmd_save(dummy);

    // 析构对象
    delete m_csf_file;
    m_csf_file = nullptr;
    csf_path[0] = '\0';

    return 0;
}

int cmd_exit(cmdline& cmd)
{
    cmd_close(cmd);
    return -1;
}


int cmd_quit(cmdline&)
{
    // quit 表示放弃保存，直接退出
    return -1;
}


int cmd_version(cmdline& cmd)
{
    printf("version: %s\n", VERSION);
    return 0;
}

int cmd_help(cmdline& cmd)
{
    usage();
    return 0;
}

// int cmd_fix(cmdline& cmd)
// {
//     if (cmd.has_next() && cmd_open(cmd)) {
//         return 1;
//     }




//     return 0;
// }