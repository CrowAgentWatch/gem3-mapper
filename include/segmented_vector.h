/*
 * PROJECT: GEMMapper
 * FILE: segmented_vector.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Memory segmented vector for generic type elements.
 *              Based on slabs for an efficient memory management
 */

#ifndef SEGMENTED_VECTOR_H_
#define SEGMENTED_VECTOR_H_

#include "essentials.h"

/*
 * Checkers
 */
#define SEGMENTED_VECTOR_CHECK(segmented_vector) \
  GEM_CHECK_NULL(segmented_vector)

#define SEGMENTED_VECTOR_RANGE_CHECK(segmented_vector,position) \
  SEGMENTED_VECTOR_CHECK(segmented_vector); \
  gem_fatal_check(position >= svector_get_used(segmented_vector),POSITION_OUT_OF_RANGE, \
    position,0ull,svector_get_used(segmented_vector)-1)

#define SEGMENTED_VECTOR_ITERATOR_CHECK(iterator) \
  GEM_CHECK_NULL(iterator); \
  SEGMENTED_VECTOR_CHECK(iterator->svector)
#define SEGMENTED_VECTOR_READ_ITERATOR_CHECK(iterator) \
  SEGMENTED_VECTOR_ITERATOR_CHECK(iterator); \
  gem_fatal_check(iterator->type!=SVECTOR_READ_ITERATOR,SVECTOR_ITERATOR_WRONG_MODE,"reading")
#define SEGMENTED_VECTOR_WRITE_ITERATOR_CHECK(iterator) \
  SEGMENTED_VECTOR_ITERATOR_CHECK(iterator); \
  gem_fatal_check(iterator->type!=SVECTOR_WRITE_ITERATOR,SVECTOR_ITERATOR_WRONG_MODE,"writing")

/*
 * Data Structures
 */
typedef struct {
  mm_slab_unit_t* slab_unit;       // Slab-Unit
  void* memory;                    // Memory
} vector_segment_t;
typedef struct {
  /* Slab allocator */
  mm_slab_t* mm_slab;              // Memory allocator
  /* Vector segments */
  vector_t* segments;              // Memory segments (vector_segment_t)
  uint64_t elements_used;          // Total elements stored
  /* Dimensions */
  uint64_t segment_size;           // Total size of each segment
  uint64_t element_size;           // Size of each element
  uint64_t elements_per_segment;   // Elements per segment
  uint64_t min_resident_segments;  // Minimum number of segments that are never reaped (returned to source slab)
} svector_t;

/*
 * Iterator (Iterators should be used for reading XOR writing)
 */
typedef enum { SVECTOR_READ_ITERATOR, SVECTOR_WRITE_ITERATOR } svector_iterator_type;
typedef struct {
  svector_t* svector;            // Segmented Vector
  svector_iterator_type type;    // Iterator type
  /* Location */
  uint64_t global_position;      // Global position
  uint64_t local_position;       // Local location (wrt segment)
  uint64_t current_segment;      // Current segment position
  uint64_t element_size;         // svector->element_size
  uint64_t elements_per_segment; // svector->elements_per_segment
  /* Reading */
  uint64_t elements_used;        // svector->elements_used (cached)
  bool eoi;                      // End-Of-Iteration
  /* Memory */
  void* memory;                  // Pointer to current position
} svector_iterator_t;

/*
 * Vector Setup (Initialization & Allocation)
 */
#define svector_new(mm_slab,type) svector_new_(mm_slab,sizeof(type))

svector_t* svector_new_(mm_slab_t* const mm_slab,const uint64_t element_size);
void svector_delete(svector_t* const svector);
void svector_clear(svector_t* const svector);
void svector_reap(svector_t* const svector);

/*
 * Accessors
 */
uint64_t svector_get_used(svector_t* const svector);
void* svector_get_elm(svector_t* const svector,const uint64_t position);
void* svector_get_free_elm(svector_t* const svector);
char* svector_request_char_buffer(svector_t* const svector,const uint64_t length);
char* svector_insert_char_buffer(svector_t* const svector,const char* const buffer,const uint64_t length);

#define svector_is_empty(svector) (svector_get_used(vector)==0)
#define svector_get_element(svector,position,type) ((type*)svector_get_elm(svector,position))
#define svector_get_last_element(svector,type) ((type*)svector_get_elm(svector,svector_get_used(vector)-1))
#define svector_set_element(svector,position,element,type) ((type*)svector_get_elm(svector,position)) = *(element)
#define svector_insert_element(svector,element,type) *((type*)svector_get_free_elm(svector))=*((type*)element)

/*
 * Writer
 */
void svector_write(fm_t* const file_manager,svector_t* const svector);

/*
 * Display/Profile
 */
void svector_print(FILE* const stream,svector_t* const svector);
void svector_record_stats(svector_t* const svector);
void svector_display_stats(FILE* const stream,svector_t* const svector);

/*
 * Iterators
 */
void svector_iterator_new(
    svector_iterator_t* const iterator,svector_t* const svector,
    const svector_iterator_type iterator_type,const uint64_t init_position);
#define svector_iterator_get_element(iterator,type) ((type*)svector_iterator_get_elm(iterator))
void* svector_iterator_get_elm(svector_iterator_t* const iterator);
// Reading
void svector_read_iterator_seek(svector_iterator_t* const iterator,const uint64_t init_position);
bool svector_read_iterator_eoi(svector_iterator_t* const svector_iterator);
void svector_read_iterator_next(svector_iterator_t* const iterator);
// Writing
void svector_write_iterator_next(svector_iterator_t* const iterator);

/*
 * Error Messages
 */
#define GEM_ERROR_SVECTOR_INDEX_OUT_OF_RANGE "SVector. Requested element (%"PRIu64") out of range [0,%"PRIu64")"
#define GEM_ERROR_SVECTOR_INSERT_CHAR_BUFFER_TOO_LONG "String is too long to fit in one vector-segment (%"PRIu64" characters)"
#define GEM_ERROR_SVECTOR_ITERATOR_WRONG_MODE "SVector-Iterator wrong mode. Iterator is for %s only"

#endif /* SEGMENTED_VECTOR_H_ */
