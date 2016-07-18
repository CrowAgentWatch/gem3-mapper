/*
 * PROJECT: GEMMapper
 * FILE: approximate_search_control.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef APPROXIMATE_SEARCH_CONTROL_H_
#define APPROXIMATE_SEARCH_CONTROL_H_

#include "utils/essentials.h"
#include "approximate_search/approximate_search.h"
#include "matches/matches_predictors.h"

/*
 * Search Limits
 */
void asearch_control_adjust_current_max_error(
    approximate_search_t* const search,
    matches_t* const matches);


#endif /* APPROXIMATE_SEARCH_CONTROL_H_ */
