/*
 * PROJECT: GEMMapper
 * FILE: approximate_search_stages.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef APPROXIMATE_SEARCH_STAGES_H_
#define APPROXIMATE_SEARCH_STAGES_H_

#include "utils/essentials.h"
#include "approximate_search/approximate_search.h"

/*
 * Basic Cases Selector
 */
void approximate_search_begin(
    approximate_search_t* const search);

/*
 * Exact Filtering Adaptive
 */
void approximate_search_exact_filtering_adaptive(
    approximate_search_t* const search,
    matches_t* const matches);
void approximate_search_exact_filtering_adaptive_cutoff(
    approximate_search_t* const search,
    matches_t* const matches);

/*
 * Filtering Verification (+ realign)
 */
void approximate_search_verify(
    approximate_search_t* const search,
    matches_t* const matches);

/*
 * Neighborhood Search
 */
void approximate_search_neighborhood(
    approximate_search_t* const search,
    matches_t* const matches);

/*
 * Unbound Filtering (+ realign)
 */
void approximate_search_align_local(
    approximate_search_t* const search,
    matches_t* const matches);

/*
 * End of the search
 */
void approximate_search_end(
    approximate_search_t* const search,
    matches_t* const matches);

#endif /* APPROXIMATE_SEARCH_STAGES_H_ */
