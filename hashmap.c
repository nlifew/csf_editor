


#include <stdlib.h>
#include <string.h>
#include "hashmap.h"


int hashmap_setup(hashmap *map, hashmap_options *ops)
{
    memset(map, 0, sizeof(hashmap));

    memcpy(&map->ops, ops, sizeof(hashmap_options));

    // 调整初始容量
    int n = ops->capacity - 1;
    n |= n >> 1; 
    n |= n >> 2; 
    n |= n >> 4; 
    n |= n >> 8; 
    n |= n >> 16;
    map->ops.capacity = n == -1 ? 16 : n + 1;

    // 调整扩容阈值
    if (map->ops.load_factor == 0) {
        map->ops.load_factor = 0.75f;
    }

    // 调整默认的 hash 函数和 cmp 函数
    if (map->ops.hash == NULL) {
        map->ops.hash = hashmap_def_hash;
    }
    if (map->ops.cmp == NULL) {
        map->ops.cmp = hashmap_def_cmp;
    }

    // 申请空间
    size_t buff_len = sizeof(hashmap_entry*) * map->ops.capacity;
    map->tables = (hashmap_entry**) malloc(buff_len);
    if (map->tables == NULL) {
        return -1;
    }
    memset(map->tables, 0, buff_len);

    return 0;
}


static int hashmap_hash(const void *key, int (*hash)(const void*))
{
    int h = hash(key);
    return h ^ (h >> 16);
}

static void hashmap_set_head(hashmap *map, hashmap_entry *e)
{
    if (map->head == e) {
        return;
    }
    

    if (e->after != NULL) {
        e->after->before = e->before;
    } else {
        map->tail = e->before;
    }

    e->before->after = e->after;
    e->before = NULL;
    e->after = map->head;
    map->head->before = e;
    map->head = e;
}

static hashmap_entry* hashmap_find_entry(hashmap *map, const void *key, int hash, hashmap_entry **pPrev)
{
    hashmap_entry *e, *prev = NULL;

    int index = hash & (map->ops.capacity - 1);
    for (e = map->tables[index]; e; e = e->next) {
        if (hash == e->hash && (key == e->key || map->ops.cmp(key, e->key) == 0)) {
            // 如果 access_order 设为 true，
            // 需要把这个节点设置为双向链表的头节点
            if (map->ops.access_order) {
                hashmap_set_head(map, e);
            }
            break;
        }
        prev = e;
    }
    if (pPrev) *pPrev = prev;
    return e;
}


hashmap_entry* hashmap_contains(hashmap *map, const void *key)
{
    int hash = hashmap_hash(key, map->ops.hash);
    return hashmap_find_entry(map, key, hash, NULL);
}


const void* hashmap_get(hashmap *map, const void *key)
{
    int hash = hashmap_hash(key, map->ops.hash);
    hashmap_entry *e = hashmap_find_entry(map, key, hash, NULL);
    return e ? e->value : NULL;
}


const void* hashmap_remove(hashmap *map, const void *key, const void **old_key)
{
    if (old_key) *old_key = NULL;


    int hash = hashmap_hash(key, map->ops.hash);
    int index = hash & (map->ops.capacity - 1);

    hashmap_entry *prev = NULL;
    hashmap_entry *e = hashmap_find_entry(map, key, hash, &prev);

    if (e == NULL) {
        return NULL;
    }

    // 先从 table 中移除这个 entry
    if (prev == NULL) {
        map->tables[index] = e->next;
    } else {
        prev->next = e->next;
    }

    // 还需要调整双向链表
    if (map->head == map->tail) {
        // 双向链表中只有一个元素，直接置为 null
        map->head = map->tail = NULL;
    }
    else if (e == map->head) {
        // 位于双向链表的头部，调整头指针
        map->head = e->after;
        e->after->before = NULL;
    }
    else if (e == map->tail) {
        // 位于双向链表尾部，调整尾指针
        map->tail = e->before;
        e->before->after = NULL;
    }
    else {
        // 位于双向链表中间
        e->before->after = e->after;
        e->after->before = e->before;
    }

    map->size --;

    if (old_key) *old_key = e->key;
    const void *old = e->value;
    free(e);
    return old;
}



static hashmap_entry* hashmap_new_entry(const void *key, const void *value, int hash)
{
    hashmap_entry* e = (hashmap_entry*) malloc(sizeof(hashmap_entry));
    if (e) {
        e->key = key;
        e->value = value;
        e->hash = hash;
        e->next = NULL;
        e->before = e->after = NULL;
    }
    return e;
}

static void hashmap_resize(hashmap *map)
{
    if (map->size <= (int) (map->ops.capacity * map->ops.load_factor)) {
        return;
    }

    // cat_hashmap(map);

    int old_capacity = map->ops.capacity;
    int new_capacity = old_capacity << 1;
    hashmap_entry **new_tables = (hashmap_entry**) realloc(
                    map->tables, sizeof(hashmap_entry*) * new_capacity);

    if (new_tables == NULL) {
        return;
    }

    map->ops.capacity = new_capacity;
    map->tables = new_tables;

    for (int i = 0; i < old_capacity; i++) {

        hashmap_entry *lo_head = NULL, *lo_tail = NULL;
        hashmap_entry *hi_head = NULL, *hi_tail = NULL;

        for (hashmap_entry *e = new_tables[i]; e; e = e->next) {
            int index = e->hash & (new_capacity - 1);
            if (index == i) {
                if (lo_head == NULL) {
                    lo_head = lo_tail = e;
                } else {
                    lo_tail->next = e;
                    lo_tail = e;
                }
            } 
            else {
                if (hi_head == NULL) {
                    hi_head = hi_tail = e;
                } else {
                    hi_tail->next = e;
                    hi_tail = e;
                }
            }
        }
        if (hi_tail) hi_tail->next = NULL;
        if (lo_tail) lo_tail->next = NULL;

        new_tables[i] = lo_head;
        new_tables[old_capacity + i] = hi_head;
    }

    // cat_hashmap(map);
}

const void* hashmap_put(hashmap *map, const void *key, const void *value, const void **old_key)
{
    if (old_key) *old_key = NULL;

    int hash = hashmap_hash(key, map->ops.hash);
    int index = hash & (map->ops.capacity - 1);

    hashmap_entry *prev = NULL;
    hashmap_entry *e = hashmap_find_entry(map, key, hash, &prev);

    if (e != NULL) {
        if (old_key) *old_key = e->key;
        const void *old = e->value;
        e->value = value;
        e->key = key;
        return old;
    }

    e = hashmap_new_entry(key, value, hash);
    if (e == NULL) {
        return NULL;
    }
    // 放到 table 中
    if (prev == NULL) {
        map->tables[index] = e;
    }
    else {
        e->next = prev->next;
        prev->next = e;
    }

    // 调整双向链表
    if (map->head == NULL) {
        map->head = map->tail = e;
    }
    else {
        map->tail->after = e;
        e->before = map->tail;
        map->tail = e;
    }

    if (map->ops.access_order) {
        // 如果 access_order 设置为 true,
        // 将按照访问顺序排序，因此要设置为头节点
        hashmap_set_head(map, e);
    }

    // 可能需要扩容
    map->size ++;
    hashmap_resize(map);
    return NULL;
}



hashmap_entry** hashmap_entries(hashmap *map, int *pSize)
{
    int n = map->size, j = 0;

    hashmap_entry** tables = (hashmap_entry**) malloc(n * sizeof(hashmap_entry*));
    if (tables == NULL) {
        goto bail;
    }

    for (hashmap_entry *e = map->head; e; e = e->after) {
        tables[j++] = e;
    }

bail:
    if (tables == NULL) n = 0;
    if (pSize) *pSize = n;
    return tables;
}

void** hashmap_keys(hashmap *map, int *pSize)
{
    int n = map->size, j = 0;

    void** tables = (void**) malloc(n * sizeof(void*));
    if (tables == NULL) {
        goto bail;
    }

    for (hashmap_entry *e = map->head; e; e = e->after) {
        tables[j++] = (void*) e->key;
    }

bail:
    if (tables == NULL) n = 0;
    if (pSize) *pSize = n;
    return tables;
}



void** hashmap_values(hashmap *map, int *pSize)
{
    int n = map->size, j = 0;
    void **tables = (void**) malloc(n * sizeof(void*));
    if (tables == NULL) {
        goto bail;
    }

    for (hashmap_entry *e = map->head; e; e = e->after) {
        tables[j++] = (void*) e->value;
    }
bail:
    if (tables == NULL) n = 0;
    if (pSize) *pSize = n;
    return tables;
}

void hashmap_clear(hashmap *map)
{
    for (hashmap_entry *e = map->head; e; ) {
        hashmap_entry *next = e->after;
        free(e);
        e = next;
    }
    map->size = 0;
    map->head = map->tail = NULL;
    memset(map->tables, 0, sizeof(void*) * map->ops.capacity);
}

void hashmap_destroy(hashmap *map)
{
    hashmap_clear(map);
    free(map->tables);
    map->tables = NULL;
}


#if 0
void cat_hashmap(hashmap *map)
{
    const char* fmt = 
        "hashmap[%p] = {\n"
        "\t.ops = {\n"
        "\t\t.capacity = %d,\n"
        "\t\t.load_factor = %f,\n"
        "\t\t.access_order = %d,\n"
        "\t\t.hash = %p,\n"
        "\t\t.cmp = %p,\n"
        "\t},\n"
        "\t.size = %d,\n"
        "\t.tables = {\n";
    printf(fmt, map, map->ops.capacity, map->ops.load_factor, map->ops.access_order, 
        map->ops.hash, map->ops.cmp, map->size);
    for (int i = 0; i < map->ops.capacity; i++) {
        printf("\t\t[%d]: ", i);
        for (hashmap_entry *e = map->tables[i]; e; e = e->next) {
            printf("[%d][%d]->", *((int*) (e->key)), *((int*) (e->value)));
        }
        printf("NULL\n");
    }
    printf("\t},\n"
        "\t.head = ");

    for (hashmap_entry *e = map->head; e; e = e->after) {
        printf("[%d][%d]->", *((int*) (e->key)), *((int*) (e->value)));
    }
    printf("NULL,\n"
        "\t.tail = ");
    for (hashmap_entry *e = map->tail; e; e = e->before) {
        printf("[%d][%d]->", *((int*) (e->key)), *((int*) (e->value)));
    }
    printf("NULL,\n"
        "}\n");
}

#endif