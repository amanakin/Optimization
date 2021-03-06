#include <cstdlib>
#include "list.hpp"
#include <cstring>

typedef const char* KeyType;
typedef const char* ValueType;

const double LoadFactor = 0.65;

//-----------------------------------------------------------------------------

#ifdef SLOW

unsigned long long HashingFunction(const char* key)
{   
    unsigned long long hash = 5381;

    while (*key)
    {
        hash = ((hash << 5) + hash) + *key;
        key++;
    }
    return hash;
}

#else

extern "C" unsigned long long HashingFunction(const char*);

#endif

//-----------------------------------------------------------------------------

typedef enum hash_error_en
{
    HASH_OK = 0,
    HASH_ERROR = 1,
    HASH_REALLOC_ERROR = 2
} hash_error;

struct HashTableEl
{
    KeyType   key;
    ValueType value;
};

struct HashTable
{
    size_t capacity;
    size_t size;
    My_list<HashTableEl> *buckets;
};

//-----------------------------------------------------------------------------

hash_error HashTable_add(HashTable *ths, KeyType key, ValueType value);

hash_error HashTable_rehash(HashTable *ths, size_t new_capacity);

hash_error HashTable_construct(HashTable *ths, size_t new_capacity);

hash_error HashTable_put(HashTable *ths, KeyType new_key, ValueType new_value);

hash_error HashTable_destruct(HashTable *ths);

#ifdef SLOW
ValueType* HashTable_get(HashTable *ths, KeyType key);
#else
extern "C" ValueType* HashTable_get(HashTable* ths, KeyType key);
#endif

//=============================================================================

hash_error HashTable_construct(HashTable *ths, size_t new_capacity)
{
    ths->capacity = (new_capacity <= 0) ? 1 : new_capacity;

    ths->buckets = (My_list<HashTableEl> *)calloc(ths->capacity, sizeof(My_list<HashTableEl>));

    if (ths->buckets == NULL)
        return HASH_REALLOC_ERROR;
    
    HashTableEl default_el = {};
    default_el.key   = NULL;
    default_el.value = NULL;

    for (size_t i = 0; i < ths->capacity; i++)
        ths->buckets[i].construct(1, default_el);
    
    ths->size = 0;
    return HASH_OK;
}

//-----------------------------------------------------------------------------

hash_error HashTable_rehash(HashTable *ths, size_t new_capacity)
{
    /* Without shrink to fit, only expand on */
    if (new_capacity < ths->capacity)
        return HASH_ERROR;
    
    HashTable new_hash_table = {};
    HashTable_construct(&new_hash_table, new_capacity);

    for (size_t i = 0; i < ths->capacity; i++)
    {
        My_list<HashTableEl> *curr_bucket = &(ths->buckets[i]);

        list_iterator iter = {};
        iter = curr_bucket->begin();
        size_t curr_size = curr_bucket->get_size();

        for (size_t j = 0; j < curr_size; j++, curr_bucket->iter_increase(iter))
            HashTable_add(&new_hash_table, (*curr_bucket)[iter].key, (*curr_bucket)[iter].value);

        curr_bucket->destruct();
    }
    
    free(ths->buckets);
    ths->buckets = new_hash_table.buckets;
    ths->capacity = new_hash_table.capacity;
    ths->size = new_hash_table.size;

    return HASH_OK;
}

//-----------------------------------------------------------------------------

hash_error HashTable_add(HashTable *ths, KeyType key, ValueType value)
{
    unsigned long long new_hash = HashingFunction(key);
    
    HashTableEl new_el = {};
    new_el.key   = key;
    new_el.value = value;

    ths->buckets[new_hash % ths->capacity].push_front(new_el);

    ths->size++;
    
    if (((double)ths->size / ths->capacity) > LoadFactor)
        HashTable_rehash(ths, ths->capacity * 2);
    
    return HASH_OK;
}

//-----------------------------------------------------------------------------

#ifdef SLOW

ValueType* HashTable_get(HashTable *ths, KeyType key)
{
    unsigned long long new_hash = HashingFunction(key);

    My_list<HashTableEl> *curr_bucket = &(ths->buckets[new_hash % ths->capacity]);
    size_t curr_size = curr_bucket->size;

    list_iterator iter = {};
    iter = curr_bucket->begin();

    for (size_t i = 0; i < curr_size; i++, curr_bucket->iter_increase(iter))
    {
        if (!strcmp(key, (*curr_bucket)[iter].key))
            return &((*curr_bucket)[iter].value);
    }

    return NULL;
}

#endif

//-----------------------------------------------------------------------------

hash_error HashTable_put(HashTable *ths, KeyType new_key, ValueType new_value)
{
    const char ** value_ptr = HashTable_get(ths, new_key);

    if (value_ptr == NULL)
    {
        HashTable_add(ths, new_key, new_value);
        return HASH_OK;
    }
    else
    {
        *value_ptr = new_value;
        return HASH_OK;
    }
}

//-----------------------------------------------------------------------------

hash_error HashTable_destruct(HashTable *ths)
{
    for (size_t i = 0; i < ths->capacity; i++)
        ths->buckets[i].destruct();
    
    free(ths->buckets);

    return HASH_OK;
}
