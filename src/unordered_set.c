/*
 * Copyright (c) 2017-2019 Bailey Thompson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include <errno.h>
#include "include/unordered_set.h"

static const int STARTING_BUCKETS = 8;
static const double RESIZE_AT = 0.75;
static const double RESIZE_RATIO = 1.5;

struct internal_unordered_set {
    size_t key_size;
    unsigned long (*hash)(const void *const key);
    int (*comparator)(const void *const one, const void *const two);
    int size;
    int capacity;
    struct node **buckets;
};

struct node {
    void *key;
    unsigned long hash;
    struct node *next;
};

/*
 * Gets the hash by first calling the user-defined hash, and then using a
 * second hash to prevent hashing clusters if the user-defined hash is
 * sub-optimal.
 */
static unsigned long unordered_set_hash(unordered_set me, const void *const key)
{
    unsigned long hash = me->hash(key);
    hash ^= (hash >> 20UL) ^ (hash >> 12UL);
    return hash ^ (hash >> 7UL) ^ (hash >> 4UL);
}

/**
 * Initializes an unordered set.
 *
 * @param key_size   the size of each key in the unordered set; must be
 *                   positive
 * @param hash       the hash function which computes the hash from the key;
 *                   must not be NULL
 * @param comparator the comparator function which compares two keys; must not
 *                   be NULL
 *
 * @return the newly-initialized unordered set, or NULL if it was not
 *         successfully initialized due to either invalid input arguments or
 *         memory allocation error
 */
unordered_set unordered_set_init(const size_t key_size,
                                 unsigned long (*hash)(const void *const),
                                 int (*comparator)(const void *const,
                                                   const void *const))
{
    struct internal_unordered_set *init;
    if (key_size == 0 || !hash || !comparator) {
        return NULL;
    }
    init = malloc(sizeof(struct internal_unordered_set));
    if (!init) {
        return NULL;
    }
    init->key_size = key_size;
    init->hash = hash;
    init->comparator = comparator;
    init->size = 0;
    init->capacity = STARTING_BUCKETS;
    init->buckets = calloc(STARTING_BUCKETS, sizeof(struct node *));
    if (!init->buckets) {
        free(init);
        return NULL;
    }
    return init;
}

/*
 * Adds the specified node to the set.
 */
static void unordered_set_add_item(unordered_set me, struct node *const add)
{
    struct node *traverse;
    const int index = (int) (add->hash % me->capacity);
    add->next = NULL;
    if (!me->buckets[index]) {
        me->buckets[index] = add;
        return;
    }
    traverse = me->buckets[index];
    while (traverse->next) {
        traverse = traverse->next;
    }
    traverse->next = add;
}

/**
 * Rehashes all the keys in the unordered set. Used when storing references and
 * changing the keys. This should rarely be used.
 *
 * @param me the unordered set to rehash
 *
 * @return 0       if no error
 * @return -ENOMEM if out of memory
 */
int unordered_set_rehash(unordered_set me)
{
    int i;
    struct node **old_buckets = me->buckets;
    me->buckets = calloc((size_t) me->capacity, sizeof(struct node *));
    if (!me->buckets) {
        me->buckets = old_buckets;
        return -ENOMEM;
    }
    for (i = 0; i < me->capacity; i++) {
        struct node *traverse = old_buckets[i];
        while (traverse) {
            struct node *const backup = traverse->next;
            traverse->hash = unordered_set_hash(me, traverse->key);
            unordered_set_add_item(me, traverse);
            traverse = backup;
        }
    }
    free(old_buckets);
    return 0;
}

/**
 * Gets the size of the unordered set.
 *
 * @param me the unordered set to check
 *
 * @return the size of the unordered set
 */
int unordered_set_size(unordered_set me)
{
    return me->size;
}

/**
 * Determines whether or not the unordered set is empty.
 *
 * @param me the unordered set to check
 *
 * @return 1 if the unordered set is empty, otherwise 0
 */
int unordered_set_is_empty(unordered_set me)
{
    return unordered_set_size(me) == 0;
}

/*
 * Increases the size of the set and redistributes the nodes.
 */
static int unordered_set_resize(unordered_set me)
{
    int i;
    const int old_capacity = me->capacity;
    const int new_capacity = (int) (me->capacity * RESIZE_RATIO);
    struct node **old_buckets = me->buckets;
    me->buckets = calloc((size_t) new_capacity, sizeof(struct node *));
    if (!me->buckets) {
        me->buckets = old_buckets;
        return -ENOMEM;
    }
    me->capacity = new_capacity;
    for (i = 0; i < old_capacity; i++) {
        struct node *traverse = old_buckets[i];
        while (traverse) {
            struct node *const backup = traverse->next;
            unordered_set_add_item(me, traverse);
            traverse = backup;
        }
    }
    free(old_buckets);
    return 0;
}

/*
 * Determines if an element is equal to the key.
 */
static int unordered_set_is_equal(unordered_set me,
                                  const struct node *const item,
                                  const unsigned long hash,
                                  const void *const key)
{
    return item->hash == hash && me->comparator(item->key, key) == 0;
}

/*
 * Creates an element to add.
 */
static struct node *unordered_set_create_element(unordered_set me,
                                                 const unsigned long hash,
                                                 const void *const key)
{
    struct node *const init = malloc(sizeof(struct node));
    if (!init) {
        return NULL;
    }
    init->key = malloc(me->key_size);
    if (!init->key) {
        free(init);
        return NULL;
    }
    memcpy(init->key, key, me->key_size);
    init->hash = hash;
    init->next = NULL;
    return init;
}

/**
 * Adds an element to the unordered set if the unordered set does not already
 * contain it. The pointer to the key being passed in should point to the key
 * type which this unordered set holds. For example, if this unordered set holds
 * key integers, the key pointer should be a pointer to an integer. Since the
 * key is being copied, the pointer only has to be valid when this function is
 * called.
 *
 * @param me  the unordered set to add to
 * @param key the element to add
 *
 * @return 0       if no error
 * @return -ENOMEM if out of memory
 */
int unordered_set_put(unordered_set me, void *const key)
{
    const unsigned long hash = unordered_set_hash(me, key);
    int index;
    if (me->size + 1 >= RESIZE_AT * me->capacity) {
        const int rc = unordered_set_resize(me);
        if (rc != 0) {
            return rc;
        }
    }
    index = (int) (hash % me->capacity);
    if (!me->buckets[index]) {
        me->buckets[index] = unordered_set_create_element(me, hash, key);
        if (!me->buckets[index]) {
            return -ENOMEM;
        }
    } else {
        struct node *traverse = me->buckets[index];
        if (unordered_set_is_equal(me, traverse, hash, key)) {
            return 0;
        }
        while (traverse->next) {
            traverse = traverse->next;
            if (unordered_set_is_equal(me, traverse, hash, key)) {
                return 0;
            }
        }
        traverse->next = unordered_set_create_element(me, hash, key);
        if (!traverse->next) {
            return -ENOMEM;
        }
    }
    me->size++;
    return 0;
}

/**
 * Determines if the unordered set contains the specified element. The pointer
 * to the key being passed in should point to the key type which this unordered
 * set holds. For example, if this unordered set holds key integers, the key
 * pointer should be a pointer to an integer. Since the key is being copied,
 * the pointer only has to be valid when this function is called.
 *
 * @param me  the unordered set to check for the element
 * @param key the element to check
 *
 * @return 1 if the unordered set contained the element, otherwise 0
 */
int unordered_set_contains(unordered_set me, void *const key)
{
    const unsigned long hash = unordered_set_hash(me, key);
    const int index = (int) (hash % me->capacity);
    const struct node *traverse = me->buckets[index];
    while (traverse) {
        if (unordered_set_is_equal(me, traverse, hash, key)) {
            return 1;
        }
        traverse = traverse->next;
    }
    return 0;
}

/**
 * Removes the key from the unordered set if it contains it. The pointer to the
 * key being passed in should point to the key type which this unordered set
 * holds. For example, if this unordered set holds key integers, the key pointer
 * should be a pointer to an integer. Since the key is being copied, the pointer
 * only has to be valid when this function is called.
 *
 * @param me  the unordered set to remove a key from
 * @param key the key to remove
 *
 * @return 1 if the unordered set contained the key, otherwise 0
 */
int unordered_set_remove(unordered_set me, void *const key)
{
    struct node *traverse;
    const unsigned long hash = unordered_set_hash(me, key);
    const int index = (int) (hash % me->capacity);
    if (!me->buckets[index]) {
        return 0;
    }
    traverse = me->buckets[index];
    if (unordered_set_is_equal(me, traverse, hash, key)) {
        me->buckets[index] = traverse->next;
        free(traverse->key);
        free(traverse);
        me->size--;
        return 1;
    }
    while (traverse->next) {
        if (unordered_set_is_equal(me, traverse->next, hash, key)) {
            struct node *const backup = traverse->next;
            traverse->next = traverse->next->next;
            free(backup->key);
            free(backup);
            me->size--;
            return 1;
        }
        traverse = traverse->next;
    }
    return 0;
}

/**
 * Clears the keys from the unordered set.
 *
 * @param me the unordered set to clear
 *
 * @return 0       if no error
 * @return -ENOMEM if out of memory
 */
int unordered_set_clear(unordered_set me)
{
    int i;
    struct node **temp =
            calloc((size_t) STARTING_BUCKETS, sizeof(struct node *));
    if (!temp) {
        return -ENOMEM;
    }
    for (i = 0; i < me->capacity; i++) {
        struct node *traverse = me->buckets[i];
        while (traverse) {
            struct node *const backup = traverse;
            traverse = traverse->next;
            free(backup->key);
            free(backup);
        }
        me->buckets[i] = NULL;
    }
    me->size = 0;
    me->capacity = STARTING_BUCKETS;
    free(me->buckets);
    me->buckets = temp;
    return 0;
}

/**
 * Frees the unordered set memory. Performing further operations after calling
 * this function results in undefined behavior.
 *
 * @param me the unordered set to free from memory
 *
 * @return NULL
 */
unordered_set unordered_set_destroy(unordered_set me)
{
    unordered_set_clear(me);
    free(me->buckets);
    free(me);
    return NULL;
}
