/*
 * PROJECT: GEMMapper
 * FILE: ihash.c
 * DATE: 2/09/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Simple Hash Implementation (Interger Key, Generic Value)
 */

#include "hash.h"

#include "mm.h"
#include "uthash.h"

#define IHASH_SIZE_PER_ELEMENT 56

/*
 * Constructor
 */
GEM_INLINE ihash_t* ihash_new(void) {
  ihash_t* const ihash = mm_alloc(ihash_t);
  ihash->head = NULL; // uthash initializer
  return ihash;
}
GEM_INLINE void ihash_clear(ihash_t* const ihash) {
  HASH_CHECK(ihash);
  ihash_element_t *ihash_element, *tmp;
  HASH_ITER(hh,ihash->head,ihash_element,tmp) {
    HASH_DEL(ihash->head,ihash_element);
    mm_free(ihash_element);
  }
}
GEM_INLINE void ihash_delete(ihash_t* const ihash) {
  HASH_CHECK(ihash);
  ihash_clear(ihash);
  mm_free(ihash);
}
/*
 * Basic (Type-unsafe) Accessors
 */
GEM_INLINE ihash_element_t* ihash_get_ihash_element(ihash_t* const ihash,const int64_t key) {
  HASH_CHECK(ihash);
  ihash_element_t *ihash_element;
  HASH_FIND_INT(ihash->head,&key,ihash_element);
  return ihash_element;
}
GEM_INLINE void ihash_insert_element(ihash_t* ihash,const int64_t key,void* const element) {
  HASH_CHECK(ihash);
  GEM_CHECK_NULL(element);
  ihash_element_t* ihash_element = ihash_get_ihash_element(ihash,key);
  if (gem_expect_true(ihash_element==NULL)) {
    ihash_element = mm_alloc(ihash_element_t);
    ihash_element->key = key;
    ihash_element->element = element;
    HASH_ADD_INT(ihash->head,key,ihash_element);
  } else {
    // No removal of replaced element
    ihash_element->element = element;
  }
}
GEM_INLINE void ihash_remove_element(ihash_t* ihash,const int64_t key) {
  HASH_CHECK(ihash);
  ihash_element_t* ihash_element = ihash_get_ihash_element(ihash,key);
  if (ihash_element) {
    HASH_DEL(ihash->head,ihash_element);
  }
}
GEM_INLINE void* ihash_get_element(ihash_t* const ihash,const int64_t key) {
  HASH_CHECK(ihash);
  ihash_element_t* const ihash_element = ihash_get_ihash_element(ihash,key);
  return gem_expect_true(ihash_element!=NULL) ? ihash_element->element : NULL;
}
/*
 * Type-safe Accessors
 */
GEM_INLINE bool ihash_is_contained(ihash_t* const ihash,const int64_t key) {
  HASH_CHECK(ihash);
  return (ihash_get_ihash_element(ihash,key)!=NULL);
}
GEM_INLINE uint64_t ihash_get_num_elements(ihash_t* const ihash) {
  HASH_CHECK(ihash);
  return (uint64_t)HASH_COUNT(ihash->head);
}
GEM_INLINE uint64_t ihash_get_size(ihash_t* const ihash) {
  HASH_CHECK(ihash);
  /*
   * The hash handle consumes about 32 bytes per item on a 32-bit system, or 56 bytes per item on a 64-bit system.
   * The other overhead costs (the buckets and the table) are negligible in comparison.
   * You can use HASH_OVERHEAD to get the overhead size, in bytes, for a hash table.
   */
  return ihash_get_num_elements(ihash)*IHASH_SIZE_PER_ELEMENT;
}
/*
 * Miscellaneous
 */
int ihash_cmp_keys(int64_t* a,int64_t* b) {
  /*
   * return (int) -1 if (a < b)
   * return (int)  0 if (a == b)
   * return (int)  1 if (a > b)
   */
  return *a-*b;
}
#define ihash_cmp_keys_wrapper(arg1,arg2) ihash_cmp_keys((int64_t*)arg1,(int64_t*)arg2)
GEM_INLINE void ihash_sort_by_key(ihash_t* ihash) {
  HASH_CHECK(ihash);
  HASH_SORT(ihash->head,ihash_cmp_keys_wrapper); // Sort
}
/*
 * Iterator
 */
GEM_INLINE ihash_iterator_t* ihash_iterator_new(ihash_t* const ihash) {
  HASH_CHECK(ihash);
  // Allocate
  ihash_iterator_t* const iterator = mm_alloc(ihash_iterator_t);
  // Init
  iterator->current = NULL;
  iterator->next = ihash->head;
  return iterator;
}
GEM_INLINE void ihash_iterator_delete(ihash_iterator_t* const iterator) {
  IHASH_ITERATOR_CHECK(iterator);
  mm_free(iterator);
}
GEM_INLINE bool ihash_iterator_eoi(ihash_iterator_t* const iterator) {
  IHASH_ITERATOR_CHECK(iterator);
  return (iterator->next!=NULL);
}
GEM_INLINE bool ihash_iterator_next(ihash_iterator_t* const iterator) {
  IHASH_ITERATOR_CHECK(iterator);
  if (gem_expect_true(iterator->next!=NULL)) {
    iterator->current = iterator->next;
    iterator->next = iterator->next->hh.next;
    return true;
  } else {
    iterator->current = NULL;
    return false;
  }
}
GEM_INLINE int64_t ihash_iterator_get_key(ihash_iterator_t* const iterator) {
  IHASH_ITERATOR_CHECK(iterator);
  return iterator->current->key;
}
GEM_INLINE void* ihash_iterator_get_element(ihash_iterator_t* const iterator) {
  IHASH_ITERATOR_CHECK(iterator);
  return iterator->current->element;
}
