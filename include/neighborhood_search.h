/*
 * PROJECT: GEMMapper
 * FILE: neighborhood_search.h
 * DATE: 06/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef NEIGHBORHOOD_SEARCH_H_
#define NEIGHBORHOOD_SEARCH_H_

#include "essentials.h"
#include "fm_index.h"
#include "interval_set.h"

/*
 * PROF
 */
extern gem_counter_t _ns_nodes_closed_depth;

/*
 * Neighborhood Search
 */
GEM_INLINE void neighborhood_search(
    fm_index_t* const fm_index,
    uint8_t* const key,const uint64_t key_length,const uint64_t max_error,
    interval_set_t* const intervals_result,mm_stack_t* const mm_stack);

#endif /* NEIGHBORHOOD_SEARCH_H_ */
