/*
 * PROJECT: GEMMapper
 * FILE: ihash.h
 * DATE: 2/09/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Simple Hash Implementation (Interger Key, Generic Value)
 */

#ifndef IHASH_H_
#define IHASH_H_

#include "system/commons.h"
#include "resources/uthash/uthash.h"

/*
 * Integer Key Hash
 */
typedef struct {
  int64_t key;
  void* element;
  UT_hash_handle hh;
} ihash_element_t;
typedef struct {
  ihash_element_t* head;
} ihash_t;
typedef struct {
  ihash_element_t* current;
  ihash_element_t* next;
} ihash_iterator_t;

/*
 * Constructor
 */
ihash_t* ihash_new(void);
void ihash_clear(ihash_t* const restrict ihash);
void ihash_delete(ihash_t* const restrict ihash);

/*
 * Basic (Type-unsafe) Accessors
 */
ihash_element_t* ihash_get_ihash_element(ihash_t* const restrict ihash,const int64_t key);
void ihash_insert_element(ihash_t* const restrict ihash,const int64_t key,void* const restrict element);
void ihash_remove_element(ihash_t* const restrict ihash,const int64_t key);
void* ihash_get_element(ihash_t* const restrict ihash,const int64_t key);

/*
 * Type-safe Accessors
 */
#define ihash_insert(ihash,key,element) ihash_insert_element(ihash,key,(void*)(element))
#define ihash_remove(ihash,key) ihash_remove_element(ihash,key)
#define ihash_get(ihash,key,type) ((type*)ihash_get_element(ihash,key))
bool ihash_is_contained(ihash_t* const restrict ihash,const int64_t key);
uint64_t ihash_get_num_elements(ihash_t* const restrict ihash);
uint64_t ihash_get_size(ihash_t* const restrict ihash);

/*
 * Miscellaneous
 */
void ihash_sort_by_key(ihash_t* const restrict ihash);

/*
 * Iterator
 */
#define IHASH_BEGIN_ITERATE(ihash,it_ikey,it_element,type) { \
  ihash_element_t *ihash_##ih_element, *ihash_##tmp; \
  HASH_ITER(hh,ihash->head,ihash_##ih_element,ihash_##tmp) { \
    type* const restrict it_element = (type*)(ihash_##ih_element->element);
#define IHASH_END_ITERATE }}

#define IHASH_BEGIN_ITERATE__KEY(ihash,it_ikey,it_element,type) { \
  ihash_element_t *ihash_##ih_element, *ihash_##tmp; \
  HASH_ITER(hh,ihash->head,ihash_##ih_element,ihash_##tmp) { \
    type* const restrict it_element = (type*)(ihash_##ih_element->element); \
    int64_t const it_ikey = ihash_##ih_element->key;
#define IHASH_END_ITERATE__KEY }}

ihash_iterator_t* ihash_iterator_new(ihash_t* const restrict ihash);
void ihash_iterator_delete(ihash_iterator_t* const restrict ihash_iterator);

bool ihash_iterator_eoi(ihash_iterator_t* const restrict iterator);
bool ihash_iterator_next(ihash_iterator_t* const restrict ihash_iterator);
int64_t ihash_iterator_get_key(ihash_iterator_t* const restrict ihash_iterator);
void* ihash_iterator_get_element(ihash_iterator_t* const restrict ihash_iterator);

#endif /* IHASH_H_ */
