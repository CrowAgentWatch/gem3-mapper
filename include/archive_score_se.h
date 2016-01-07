/*
 * PROJECT: GEMMapper
 * FILE: archive_score_se.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: // TODO
 */

#ifndef ARCHIVE_SCORE_SE_H_
#define ARCHIVE_SCORE_SE_H_

#include "archive_search.h"
#include "matches.h"

/*
 * Scoring Utils
 */
uint8_t archive_score_probability_to_mapq(
    const double probability,const double sum_probability);
uint8_t archive_score_probability_scale(
    const double probability,const double sum_probability,
    const uint8_t floor,const uint8_t ceil);

/*
 * SE Score Categories
 */
uint8_t archive_score_matches_se_default_ties(matches_predictors_t* const predictors);
uint8_t archive_score_matches_se_default_mmap(matches_predictors_t* const predictors);
uint8_t archive_score_matches_se_default_unique(matches_predictors_t* const predictors);

/*
 * Archive Scoring SE
 */
void archive_score_matches_se(archive_search_t* const archive_search,matches_t* const matches);

#endif /* ARCHIVE_SCORE_SE_H_ */
