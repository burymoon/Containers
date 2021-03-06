#include "test.h"
#include "../src/include/map.h"

/*
 * Include this struct to verify the tree.
 */
struct internal_map {
    size_t key_size;
    size_t value_size;
    int (*comparator)(const void *const one, const void *const two);
    int size;
    struct node *root;
};

/*
 * Include this struct to verify the tree.
 */
struct node {
    struct node *parent;
    int balance;
    void *key;
    void *value;
    struct node *left;
    struct node *right;
};


/*
 * Verifies that the AVL tree rules are followed. The balance factor of an item
 * must be the right height minus the left height. Also, the left key must be
 * less than the right key.
 */
static int map_verify_recursive(struct node *const item)
{
    int left;
    int right;
    int max;
    if (!item) {
        return 0;
    }
    left = map_verify_recursive(item->left);
    right = map_verify_recursive(item->right);
    max = left > right ? left : right;
    assert(right - left == item->balance);
    if (item->left && item->right) {
        const int left_val = *(int *) item->left->key;
        const int right_val = *(int *) item->right->key;
        assert(left_val < right_val);
    }
    if (item->left) {
        assert(item->left->parent == item);
        assert(item->left->parent->key == item->key);
    }
    if (item->right) {
        assert(item->right->parent == item);
        assert(item->right->parent->key == item->key);
    }
    return max + 1;
}

static int map_compute_size(struct node *const item)
{
    if (!item) {
        return 0;
    }
    return 1 + map_compute_size(item->left) + map_compute_size(item->right);
}

static void map_verify(map me)
{
    map_verify_recursive(me->root);
    assert(map_compute_size(me->root) == map_size(me));
}

static int compare_int(const void *const one, const void *const two)
{
    const int a = *(int *) one;
    const int b = *(int *) two;
    return a - b;
}

static void test_invalid_init(void)
{
    assert(!map_init(0, sizeof(int), compare_int));
    assert(!map_init(sizeof(int), 0, compare_int));
    assert(!map_init(sizeof(int), sizeof(int), NULL));
}

static void mutation_order(map me, const int *const arr, const int size)
{
    int i;
    int actual_size = 0;
    assert(map_is_empty(me));
    for (i = 0; i < size; i++) {
        int num = arr[i];
        if (num > 0) {
            assert(map_put(me, &num, &num) == 0);
            actual_size++;
        } else {
            int actual_num = -1 * num;
            assert(map_remove(me, &actual_num));
            actual_size--;
        }
    }
    assert(map_size(me) == actual_size);
    map_verify(me);
}

/*
 * Targets the (child->balance == 0) branch.
 */
static void test_rotate_left_balanced_child(map me)
{
    int i;
    int arr[] = {2, 4, 1, 3, 5, -1};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 2; i <= 5; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets the else branch.
 */
static void test_rotate_left_unbalanced_child(map me)
{
    int i;
    int arr[] = {1, 2, 3};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 1; i <= 3; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets (parent->balance == 2 && child->balance >= 0) in the map_repair
 * function.
 */
static void test_rotate_left(void)
{
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    test_rotate_left_balanced_child(me);
    map_clear(me);
    test_rotate_left_unbalanced_child(me);
    assert(!map_destroy(me));
}

/*
 * Targets the (child->balance == 0) branch.
 */
static void test_rotate_right_balanced_child(map me)
{
    int i;
    int arr[] = {4, 2, 5, 1, 3, -5};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 1; i <= 4; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets the else branch.
 */
static void test_rotate_right_unbalanced_child(map me)
{
    int i;
    int arr[] = {3, 2, 1};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 1; i <= 3; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets (parent->balance == -2 && child->balance <= 0) in the map_repair
 * function.
 */
static void test_rotate_right(void)
{
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    test_rotate_right_balanced_child(me);
    map_clear(me);
    test_rotate_right_unbalanced_child(me);
    assert(!map_destroy(me));
}

/*
 * Targets the (grand_child->balance == 1) branch.
 */
static void test_rotate_left_right_positively_balanced_grand_child(map me)
{
    int i;
    int arr[] = {5, 2, 6, 1, 3, 4};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 1; i <= 6; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets the (grand_child->balance == 0) branch.
 */
static void test_rotate_left_right_neutral_balanced_grand_child(map me)
{
    int i;
    int arr[] = {3, 1, 2};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 1; i <= 3; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets the else branch.
 */
static void test_rotate_left_right_negatively_balanced_grand_child(map me)
{
    int i;
    int arr[] = {5, 2, 6, 1, 4, 3};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 1; i <= 6; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets (parent->balance == -2 && child->balance == 1) in the map_repair
 * function.
 */
static void test_rotate_left_right(void)
{
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    test_rotate_left_right_positively_balanced_grand_child(me);
    map_clear(me);
    test_rotate_left_right_neutral_balanced_grand_child(me);
    map_clear(me);
    test_rotate_left_right_negatively_balanced_grand_child(me);
    assert(!map_destroy(me));
}

/*
 * Targets the (grand_child->balance == 1) branch.
 */
static void test_rotate_right_left_positively_balanced_grand_child(map me)
{
    int i;
    int arr[] = {2, 1, 5, 3, 6, 4};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 1; i <= 6; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets the (grand_child->balance == 0) branch.
 */
static void test_rotate_right_left_neutral_balanced_grand_child(map me)
{
    int i;
    int arr[] = {1, 3, 2};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 1; i <= 3; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets the else branch.
 */
static void test_rotate_right_left_negatively_balanced_grand_child(map me)
{
    int i;
    int arr[] = {2, 1, 5, 4, 6, 3};
    int size = sizeof(arr) / sizeof(arr[0]);
    mutation_order(me, arr, size);
    for (i = 1; i <= 6; i++) {
        assert(map_contains(me, &i));
    }
}

/*
 * Targets (parent->balance == 2 && child->balance == -1) in the map_repair
 * function.
 */
static void test_rotate_right_left(void)
{
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    test_rotate_right_left_positively_balanced_grand_child(me);
    map_clear(me);
    test_rotate_right_left_neutral_balanced_grand_child(me);
    map_clear(me);
    test_rotate_right_left_negatively_balanced_grand_child(me);
    assert(!map_destroy(me));
}

/*
 * Targets the map_repair function.
 */
static void test_auto_balancing(void)
{
    test_rotate_left();
    test_rotate_right();
    test_rotate_left_right();
    test_rotate_right_left();
}

static void test_put_already_existing(void)
{
    int key = 5;
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    assert(map_size(me) == 0);
    map_put(me, &key, &key);
    assert(map_size(me) == 1);
    map_put(me, &key, &key);
    assert(map_size(me) == 1);
    assert(!map_destroy(me));
}

static void test_remove_nothing(void)
{
    int key;
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    key = 3;
    map_put(me, &key, &key);
    key = 5;
    assert(!map_remove(me, &key));
    assert(!map_destroy(me));
}

static void test_contains(void)
{
    int key;
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    key = 7;
    assert(!map_contains(me, &key));
    key = 3;
    map_put(me, &key, &key);
    key = 1;
    map_put(me, &key, &key);
    key = 5;
    map_put(me, &key, &key);
    key = 0;
    assert(!map_contains(me, &key));
    key = 1;
    assert(map_contains(me, &key));
    key = 2;
    assert(!map_contains(me, &key));
    key = 3;
    assert(map_contains(me, &key));
    key = 4;
    assert(!map_contains(me, &key));
    key = 5;
    assert(map_contains(me, &key));
    key = 6;
    assert(!map_contains(me, &key));
    assert(!map_destroy(me));
}

static void test_stress_add(void)
{
    int count = 0;
    int flip = 0;
    int i;
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    for (i = 1234; i < 82400; i++) {
        int is_already_present;
        int num = i % 765;
        is_already_present = map_contains(me, &num);
        map_put(me, &num, &num);
        assert(map_contains(me, &num));
        if (!is_already_present) {
            count++;
        }
        if (i == 1857 && !flip) {
            i *= -1;
            flip = 1;
        }
    }
    assert(count == map_size(me));
    assert(!map_destroy(me));
}

static void test_stress_remove(void)
{
    int i;
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    for (i = 8123; i < 12314; i += 3) {
        map_put(me, &i, &i);
        assert(map_contains(me, &i));
    }
    for (i = 13000; i > 8000; i--) {
        map_remove(me, &i);
        assert(!map_contains(me, &i));
    }
    assert(!map_destroy(me));
}

static void test_unique_delete_one_child(map me)
{
    int arr1[] = {2, 1, -2};
    int arr2[] = {1, 2, -1};
    int arr3[] = {3, 2, 4, 1, -2};
    int arr4[] = {3, 1, 4, 2, -1};
    int arr5[] = {3, 1, 4, 2, -4};
    int arr6[] = {2, 1, 3, 4, -3};
    int sz1 = sizeof(arr1) / sizeof(arr1[0]);
    int sz2 = sizeof(arr2) / sizeof(arr2[0]);
    int sz3 = sizeof(arr3) / sizeof(arr3[0]);
    int sz4 = sizeof(arr4) / sizeof(arr4[0]);
    int sz5 = sizeof(arr5) / sizeof(arr5[0]);
    int sz6 = sizeof(arr6) / sizeof(arr6[0]);
    mutation_order(me, arr1, sz1);
    map_clear(me);
    mutation_order(me, arr2, sz2);
    map_clear(me);
    mutation_order(me, arr3, sz3);
    map_clear(me);
    mutation_order(me, arr4, sz4);
    map_clear(me);
    mutation_order(me, arr5, sz5);
    map_clear(me);
    mutation_order(me, arr6, sz6);
}

static void test_unique_delete_two_children(map me)
{
    int arr1[] = {2, 1, 3, -2};
    int arr2[] = {4, 2, 5, 1, 3, -2};
    int arr3[] = {2, 1, 4, 3, 5, -4};
    int sz1 = sizeof(arr1) / sizeof(arr1[0]);
    int sz2 = sizeof(arr2) / sizeof(arr2[0]);
    int sz3 = sizeof(arr3) / sizeof(arr3[0]);
    mutation_order(me, arr1, sz1);
    map_clear(me);
    mutation_order(me, arr2, sz2);
    map_clear(me);
    mutation_order(me, arr3, sz3);
}

static void test_unique_deletion_patterns(void)
{
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    test_unique_delete_one_child(me);
    map_clear(me);
    test_unique_delete_two_children(me);
    assert(!map_destroy(me));
}

static void test_override_value(void)
{
    int key = 5;
    int value = 17;
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    assert(!map_get(&value, me, &key));
    assert(key == 5);
    assert(value == 17);
    map_put(me, &key, &value);
    key = 5;
    value = 0xdeafbeef;
    map_get(&value, me, &key);
    assert(key == 5);
    assert(value == 17);
    value = 97;
    map_put(me, &key, &value);
    key = 5;
    value = 0xdeafbeef;
    map_get(&value, me, &key);
    assert(key == 5);
    assert(value != 17);
    assert(value == 97);
    assert(map_size(me) == 1);
    assert(!map_destroy(me));
}

static void test_init_out_of_memory(void)
{
    fail_malloc = 1;
    assert(!map_init(sizeof(int), sizeof(int), compare_int));
}

static void test_put_root_out_of_memory(map me)
{
    int key = 2;
    fail_malloc = 1;
    assert(map_put(me, &key, &key) == -ENOMEM);
    fail_malloc = 1;
    delay_fail_malloc = 1;
    assert(map_put(me, &key, &key) == -ENOMEM);
    fail_malloc = 1;
    delay_fail_malloc = 2;
    assert(map_put(me, &key, &key) == -ENOMEM);
}

static void test_put_on_left_out_of_memory(map me)
{
    int key = 1;
    fail_malloc = 1;
    assert(map_put(me, &key, &key) == -ENOMEM);
    fail_malloc = 1;
    delay_fail_malloc = 1;
    assert(map_put(me, &key, &key) == -ENOMEM);
    fail_malloc = 1;
    delay_fail_malloc = 2;
    assert(map_put(me, &key, &key) == -ENOMEM);
}

static void test_put_on_right_out_of_memory(map me)
{
    int key = 3;
    fail_malloc = 1;
    assert(map_put(me, &key, &key) == -ENOMEM);
    fail_malloc = 1;
    delay_fail_malloc = 1;
    assert(map_put(me, &key, &key) == -ENOMEM);
    fail_malloc = 1;
    delay_fail_malloc = 2;
    assert(map_put(me, &key, &key) == -ENOMEM);
}

static void test_put_out_of_memory(void)
{
    int key = 2;
    map me = map_init(sizeof(int), sizeof(int), compare_int);
    assert(me);
    test_put_root_out_of_memory(me);
    assert(map_put(me, &key, &key) == 0);
    test_put_on_left_out_of_memory(me);
    test_put_on_right_out_of_memory(me);
    assert(!map_destroy(me));
}

void test_map(void)
{
    test_invalid_init();
    test_auto_balancing();
    test_put_already_existing();
    test_remove_nothing();
    test_contains();
    test_stress_add();
    test_stress_remove();
    test_unique_deletion_patterns();
    test_override_value();
    test_init_out_of_memory();
    test_put_out_of_memory();
}
