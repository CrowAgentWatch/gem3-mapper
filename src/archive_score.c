/*
 * PROJECT: GEMMapper
 * FILE: archive_score.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include "archive_score.h"
#include "matches_classify.h"
#include "paired_matches_classify.h"

/*
 * SM Scoring Utils
 */
GEM_INLINE double archive_score_diff_exponential(
    const int32_t reference_score,const int32_t match_score,double exp_coefficient) {
  return (double)reference_score*exp((double)(match_score-reference_score)*exp_coefficient);
}
GEM_INLINE uint8_t archive_score_probability_to_mapq(const double probability,const double sum_probability) {
   const double mapq = -10. * log10(1.-(probability/sum_probability));
  if (mapq > 60.) {
    return 60;
  } else if (mapq < 0.) {
    return 0;
  } else {
    return (uint8_t) mapq;
  }
}
GEM_INLINE uint8_t archive_score_scale_probability(
    const double probability,const double sum_probability,
    const uint8_t floor,const uint8_t ceil) {
//  double mapq_pr = -10. * log10(1.-(probability/sum_probability));
//  if (mapq_pr <= 0.0) return floor;
//  if (mapq_pr >= 60.0) return ceil;
//  return floor + (uint8_t)((mapq_pr*(double)(ceil-floor))/60.);
  const double range = ceil-floor;
  return floor + (uint8_t)((range*probability)/sum_probability);
}
/*
 * GEM Score
 */
GEM_INLINE uint8_t archive_score_matches_gem_se_ties(matches_predictors_t* const predictors) {
  // Classify ties
  const double pr = matches_classify_ties(predictors);
  if (pr < MATCHES_TIES_CI) return 0;
  return archive_score_scale_probability(pr-MATCHES_TIES_CI,1.-MATCHES_TIES_CI,1,29);
}
GEM_INLINE uint8_t archive_score_matches_gem_se_mmap(matches_predictors_t* const predictors) {
  // Classify multimaps
  const double pr = matches_classify_mmaps(predictors);
  if (pr < MATCHES_MMAPS_CI) return 0;
  return archive_score_scale_probability(pr-MATCHES_MMAPS_CI,1.-MATCHES_MMAPS_CI,30,49);
}
GEM_INLINE uint8_t archive_score_matches_gem_se_unique(matches_predictors_t* const predictors) {
  const double pr = matches_classify_unique(predictors);
  if (pr < MATCHES_UNIQUE_CI) return 0;
  return archive_score_scale_probability(pr-MATCHES_UNIQUE_CI,1.-MATCHES_UNIQUE_CI,50,60);
}
GEM_INLINE void archive_score_matches_gem_se(archive_search_t* const archive_search,matches_t* const matches) {
  // Parameters
  match_trace_t* const match = matches_get_match_trace_buffer(matches);
  const uint64_t num_matches = matches_get_num_match_traces(matches);
  const uint64_t read_length = sequence_get_length(&archive_search->sequence);
  const swg_penalties_t* const swg_penalties = &archive_search->as_parameters.search_parameters->swg_penalties;
  const uint64_t max_region_length = archive_search_get_max_region_length(archive_search);
  const uint64_t proper_length = fm_index_get_proper_length(archive_search->archive->fm_index);
  /*
   * Classify
   *   50-60   Unique
   *   30-49   MMaps
   *    1-29   Ties (Distinguishable)
   *       0   Uncertain & subdominant
   */
  // Score subdominant matches (MAPQ=0)
  uint64_t i;
  for (i=0;i<num_matches;++i) match[i].mapq_score = 0;
  // Search without enough resolution
  if (num_matches > 0 && matches->max_complete_stratum <= 1) return;
  // Compute class
  matches_predictors_t predictors;
  matches_class_t matches_class;
  matches_class = matches_classify(matches);
  switch (matches_class) {
    case matches_class_unmapped:
      return;
    case matches_class_unique:
      // Compute metrics
      matches_classify_compute_predictors(matches,&predictors,
          swg_penalties,read_length,max_region_length,proper_length,
          matches->max_complete_stratum==ALL ? 0 : matches->max_complete_stratum);
      // Compute probability
      match[0].mapq_score = archive_score_matches_gem_se_unique(&predictors);
      break;
    default: {
      // Filter by SWG-score
      const double swg_threshold = archive_search->as_parameters.search_parameters->swg_threshold;
      if (swg_threshold != 0.0) {
        matches_filter_by_swg_score(matches,swg_threshold,archive_search->mm_stack);
      }
      // Sort
      matches_sort_by_distance(matches);
      // Compute metrics
      matches_classify_compute_predictors(matches,&predictors,
          swg_penalties,read_length,max_region_length,proper_length,
          matches->max_complete_stratum==ALL ? 0 : matches->max_complete_stratum);
      // Classify
      matches_class = matches_classify(matches);
      switch (matches_class) {
        case matches_class_tie_indistinguishable:
        case matches_class_tie_swg_score:
        case matches_class_tie_edit_distance:
          match[0].mapq_score = 0;
          break;
        case matches_class_tie_event_distance:
          match[0].mapq_score = archive_score_matches_gem_se_ties(&predictors);
          break;
        case matches_class_mmap:
        case matches_class_unique:
          match[0].mapq_score = archive_score_matches_gem_se_mmap(&predictors);
          break;
        default:
          GEM_INVALID_CASE();
          break;
      }
      break;
    }
  }
}
GEM_INLINE uint8_t archive_score_matches_gem_pe_ties(matches_predictors_t* const predictors) {
  // Classify ties
  const double pr = paired_matches_classify_ties(predictors);
  if (pr < PAIRED_MATCHES_TIES_CI) return 0;
  return archive_score_scale_probability(pr-PAIRED_MATCHES_TIES_CI,1.-PAIRED_MATCHES_TIES_CI,10,29);
}
GEM_INLINE uint8_t archive_score_matches_gem_pe_mmap(matches_predictors_t* const predictors) {
  // Classify multimaps
  const double pr = paired_matches_classify_mmaps(predictors);
  if (pr < PAIRED_MATCHES_MMAPS_CI) return 0;
  return archive_score_scale_probability(pr-PAIRED_MATCHES_MMAPS_CI,1.-PAIRED_MATCHES_MMAPS_CI,30,49);
}
GEM_INLINE uint8_t archive_score_matches_gem_pe_unique(matches_predictors_t* const predictors) {
  const double pr = paired_matches_classify_unique(predictors);
  if (pr < PAIRED_MATCHES_UNIQUE_CI) return 0;
  return archive_score_scale_probability(pr-PAIRED_MATCHES_UNIQUE_CI,1.-PAIRED_MATCHES_UNIQUE_CI,50,60);
}
GEM_INLINE void archive_score_matches_gem_pe(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  // Parameters
  paired_map_t* const paired_map = paired_matches_get_maps(paired_matches);
  const uint64_t num_paired_map = paired_matches_get_num_maps(paired_matches);
  // Score subdominant matches (MAPQ=0)
  uint64_t i;
  for (i=0;i<num_paired_map;++i) paired_map[i].mapq_score = 0;
  // Ambiguous
  const uint64_t mcs_end1 = paired_matches->matches_end1->max_complete_stratum;
  const uint64_t mcs_end2 = paired_matches->matches_end2->max_complete_stratum;
  const matches_class_t matches_class = paired_matches_classify(paired_matches);
  if (matches_class==matches_class_unmapped ||
     ((archive_search_end1->end_class==matches_class_unmapped || mcs_end1<=1) && mcs_end2<=1)) {
    return;
  }
  // Compute metrics
  const uint64_t read_length_end1 = sequence_get_length(&archive_search_end1->sequence);
  const uint64_t read_length_end2 = sequence_get_length(&archive_search_end2->sequence);
  const uint64_t total_read_length = read_length_end1 + read_length_end2;
  const swg_penalties_t* const swg_penalties = &archive_search_end1->as_parameters.search_parameters->swg_penalties;
  const uint64_t max_region_length_end1 = archive_search_get_max_region_length(archive_search_end1);
  const uint64_t max_region_length_end2 = archive_search_get_max_region_length(archive_search_end2);
  const uint64_t max_region_length = MAX(max_region_length_end1,max_region_length_end2);
  const uint64_t proper_length = fm_index_get_proper_length(archive_search_end1->archive->fm_index);
  matches_predictors_t predictors;
  paired_matches_classify_compute_predictors(paired_matches,&predictors,
      swg_penalties,total_read_length,max_region_length,proper_length,UINT64_MAX);
  // Classify
  switch (matches_class) {
    case matches_class_unmapped:
      break;
    case matches_class_tie_indistinguishable:
    case matches_class_tie_swg_score:
    case matches_class_tie_edit_distance:
      break;
    case matches_class_tie_event_distance:
      paired_map[0].mapq_score = archive_score_matches_gem_pe_ties(&predictors);
      break;
    case matches_class_mmap:
      paired_map[0].mapq_score = archive_score_matches_gem_pe_mmap(&predictors);
      break;
    case matches_class_unique:
      paired_map[0].mapq_score = archive_score_matches_gem_pe_unique(&predictors);
      break;
    default:
      GEM_INVALID_CASE();
      break;
  }
}
/*
 * Stratify Matches
 */
GEM_INLINE void archive_score_matches_stratify_se(archive_search_t* const archive_search,matches_t* const matches) {
  // Parameters
  match_trace_t* const match = matches_get_match_trace_buffer(matches);
  const uint64_t num_matches = matches_get_num_match_traces(matches);
  const uint64_t read_length = sequence_get_length(&archive_search->sequence);
  const swg_penalties_t* const swg_penalties = &archive_search->as_parameters.search_parameters->swg_penalties;
  const uint64_t max_region_length = archive_search_get_max_region_length(archive_search);
  const uint64_t proper_length = fm_index_get_proper_length(archive_search->archive->fm_index);
  /*
   * Classify
   *   200-250   Unique (high likelihood)
   *   199       Unique (low likelihood)
   *
   *   140-190   MMaps (high likelihood)
   *   139       MMaps (low likelihood)
   *
   *   80-130    Tie Event (high likelihood)
   *   79        Tie Event (low likelihood)
   *
   *   4         Tie Edit
   *   3         Tie SWG
   *   2         Tie Indistinguishable
   *   1         Ambiguous (not enough resolution mcs <= 1)
   *   0         Subdominant
   */
  // Score subdominant matches (MAPQ=0)
  uint64_t i;
  for (i=0;i<num_matches;++i) match[i].mapq_score = 0;
  // Classify
  double pr;
  matches_predictors_t predictors;
  matches_class_t matches_class;
  matches_class = matches_classify(matches);
  if (matches_class!=matches_class_unmapped && matches->max_complete_stratum <= 1) {
    match[0].mapq_score = 1;
  } else {
    switch (matches_class) {
      case matches_class_unmapped: break;
      case matches_class_unique:
        // Compute metrics
        matches_classify_compute_predictors(matches,&predictors,
            swg_penalties,read_length,max_region_length,proper_length,UINT64_MAX);
        // Compute probability
        pr = matches_classify_unique(&predictors);
        match[0].mapq_score = (pr >= 0.95) ? 200 + (uint8_t)((pr-0.95)*1000.0) : 199;
        break;
      default: {
        // Filter by SWG-score
        const double swg_threshold = archive_search->as_parameters.search_parameters->swg_threshold;
        if (swg_threshold != 0.0) {
          matches_filter_by_swg_score(matches,swg_threshold,archive_search->mm_stack);
        }
        // Sort
        matches_sort_by_distance(matches);
        // Compute metrics
        matches_classify_compute_predictors(matches,&predictors,
            swg_penalties,read_length,max_region_length,proper_length,UINT64_MAX);
        // Classify
        matches_class = matches_classify(matches);
        switch (matches_class) {
          case matches_class_tie_indistinguishable:
            match[0].mapq_score = 2;
            break;
          case matches_class_tie_swg_score:
            match[0].mapq_score = 3;
            break;
          case matches_class_tie_edit_distance:
            match[0].mapq_score = 4;
            break;
          case matches_class_tie_event_distance:
            pr = matches_classify_ties(&predictors);
            match[0].mapq_score = (pr >= 0.90) ? 80 + (uint8_t)((pr-0.90)*500.0) : 79;
            break;
          case matches_class_mmap:
          case matches_class_unique:
            pr = matches_classify_mmaps(&predictors);
            match[0].mapq_score = (pr >= 0.90) ? 140 + (uint8_t)((pr-0.90)*500.0) : 139;
            break;
          default:
            GEM_INVALID_CASE();
            break;
        }
        break;
      }
    }
  }
  matches_predictors_print(&predictors,sequence_get_tag(&archive_search->sequence),match[0].mapq_score);
}
GEM_INLINE void archive_score_matches_stratify_pe(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  // Parameters
  paired_map_t* const paired_map = paired_matches_get_maps(paired_matches);
  const uint64_t num_paired_map = paired_matches_get_num_maps(paired_matches);
  const uint64_t read_length_end1 = sequence_get_length(&archive_search_end1->sequence);
  const uint64_t read_length_end2 = sequence_get_length(&archive_search_end2->sequence);
  const uint64_t total_read_length = read_length_end1 + read_length_end2;
  const swg_penalties_t* const swg_penalties = &archive_search_end1->as_parameters.search_parameters->swg_penalties;
  const uint64_t max_region_length_end1 = archive_search_get_max_region_length(archive_search_end1);
  const uint64_t max_region_length_end2 = archive_search_get_max_region_length(archive_search_end2);
  const uint64_t max_region_length = MAX(max_region_length_end1,max_region_length_end2);
  const uint64_t proper_length = fm_index_get_proper_length(archive_search_end1->archive->fm_index);
  // Compute metrics
  matches_predictors_t predictors;
  paired_matches_classify_compute_predictors(paired_matches,&predictors,
      swg_penalties,total_read_length,max_region_length,proper_length,UINT64_MAX);
  // Score subdominant matches (MAPQ=0)
  double pr;
  uint64_t i;
  for (i=0;i<num_paired_map;++i) paired_map[i].mapq_score = 0;
  /*
   * Classify
   *   200-250   Unique (high likelihood)
   *   199       Unique (low likelihood)
   *
   *   140-190   MMaps (high likelihood)
   *   139       MMaps (low likelihood)
   *
   *   80-130    Tie (high likelihood)
   *   79        Tie (low likelihood)
   *
   *   4         Tie Edit
   *   3         Tie SWG
   *   2         Ambiguous (not enough resolution)
   *   1         Tie Indistinguishable (low likelihood)
   *   0         Subdominant
   */
  const matches_class_t matches_class = paired_matches_classify(paired_matches);
  match_trace_t* const match_end1 = paired_map_get_match_end1(paired_matches,paired_map);
  match_trace_t* const match_end2 = paired_map_get_match_end2(paired_matches,paired_map);
  const uint64_t mcs_end1 = paired_matches->matches_end1->max_complete_stratum;
  const uint64_t mcs_end2 = paired_matches->matches_end2->max_complete_stratum;
  const uint8_t mapq_score_end1 = (archive_search_end1->pair_searched) ? match_end1->mapq_score : 0;
  const uint8_t mapq_score_end2 = (archive_search_end2->pair_searched) ? match_end2->mapq_score : 0;
  if (matches_class==matches_class_unmapped) {
    return;
  } else if (matches_class==matches_class_tie_indistinguishable) {
    paired_map[0].mapq_score = 1;
  } else if ((archive_search_end1->end_class==matches_class_unmapped || mcs_end1<=1) && mcs_end2<=1) {
    paired_map[0].mapq_score = 2;
//  } else if (archive_search_end1->pair_searched && archive_search_end2->pair_searched &&
//      (mapq_score_end1==0 && mapq_score_end2==0) && (mcs_end1<=1 || mcs_end2<=1)) {
//    paired_map[0].mapq_score = 5;
  } else {
    switch (matches_class) {
      case matches_class_tie_swg_score:
        paired_map[0].mapq_score = 3;
        break;
      case matches_class_tie_edit_distance:
        paired_map[0].mapq_score = 4;
        break;
      case matches_class_tie_event_distance:
        pr = paired_matches_classify_ties(&predictors);
        paired_map[0].mapq_score = (pr >= 0.80) ? 80 + (uint8_t)((pr-0.80)*250.0) : 79;
        break;
      case matches_class_mmap:
        pr = paired_matches_classify_mmaps(&predictors);
        paired_map[0].mapq_score = (pr >= 0.80) ? 140 + (uint8_t)((pr-0.80)*250.0) : 139;
        break;
      case matches_class_unique:
        pr = paired_matches_classify_unique(&predictors);
        paired_map[0].mapq_score = (pr >= 0.80) ? 200 + (uint8_t)((pr-0.80)*250.0) : 199;
        break;
      default:
        GEM_INVALID_CASE();
        break;
    }
  }
  paired_matches_predictors_print(
      &predictors,sequence_get_tag(&archive_search_end1->sequence),
      paired_map[0].mapq_score,mapq_score_end1,mapq_score_end2);
}
/*
 * SE Scoring
 */
GEM_INLINE void archive_score_matches_se(
    archive_search_t* const archive_search,
    const bool paired_mapping,matches_t* const matches) {
  if (matches_get_num_match_traces(matches)==0) return;
  PROF_START(GP_ARCHIVE_SCORE_SE_MATCHES);
  search_parameters_t* const search_parameters = archive_search->as_parameters.search_parameters;
  switch (search_parameters->alignment_model) {
    case alignment_model_none:
      break;
    case alignment_model_hamming:
    case alignment_model_levenshtein:
      matches_sort_by_distance(matches); // Just sort by distance (whichever is selected)
      break;
    case alignment_model_gap_affine:
      /*
       * Select scoring model
       */
      switch (archive_search->select_parameters->mapq_model) {
        case mapq_model_none:
          matches_sort_by_distance(matches); // Sort
          break;
        case mapq_model_classify:
          matches_sort_by_distance(matches); // Sort
          if (paired_mapping) {
            archive_score_matches_gem_se(archive_search,matches);
          } else {
            archive_score_matches_stratify_se(archive_search,matches);
          }
          break;
        case mapq_model_gem:
          matches_sort_by_distance(matches); // Sort
          archive_score_matches_gem_se(archive_search,matches);
//          // Filter by MAPQ-score
//          if (select_parameters->mapq_threshold > 0) {
//            matches_filter_by_mapq(matches,select_parameters->mapq_threshold);
//          }
          break;
        default:
          GEM_INVALID_CASE();
          break;
      }
  }
  PROF_STOP(GP_ARCHIVE_SCORE_SE_MATCHES);
}
/*
 * PE Scoring
 */
GEM_INLINE void archive_score_matches_pe(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  if (paired_matches_get_num_maps(paired_matches)==0) return;
  PROF_START(GP_ARCHIVE_SCORE_PE_MATCHES);
  search_parameters_t* const search_parameters = archive_search_end1->as_parameters.search_parameters;
  switch (search_parameters->alignment_model) {
    case alignment_model_none:
      break;
    case alignment_model_hamming:
    case alignment_model_levenshtein:
      paired_matches_sort_by_distance(paired_matches); // Just sort by distance (whichever is selected)
      break;
    case alignment_model_gap_affine:
      /*
       * Select scoring model
       */
      switch (archive_search_end1->select_parameters->mapq_model) {
        case mapq_model_none:
          paired_matches_sort_by_distance(paired_matches); // Sort
          break;
        case mapq_model_classify:
          paired_matches_sort_by_distance(paired_matches); // Sort
          archive_score_matches_stratify_pe(archive_search_end1,archive_search_end2,paired_matches);
          break;
        case mapq_model_gem:
          // Sort
          paired_matches_sort_by_distance(paired_matches);
          // Score matches
          archive_score_matches_gem_pe(archive_search_end1,archive_search_end2,paired_matches);
//        // Filter by SWG-score
//        const double swg_threshold = archive_search->as_parameters.search_parameters->swg_threshold;
//        if (swg_threshold != 0.0) {
//          matches_filter_by_swg_score(matches,swg_threshold,archive_search->mm_stack);
//        }
//        // Filter by MAPQ-score
//        if (select_parameters->mapq_threshold > 0) {
//          matches_filter_by_mapq(matches,select_parameters->mapq_threshold);
//          archive_select_filter_paired_matches_mapq(paired_matches,select_parameters->mapq_threshold);
//        }
          break;
        default:
          GEM_INVALID_CASE();
          break;
      }
      break;
    default:
      GEM_INVALID_CASE();
      break;
  }
  PROF_STOP(GP_ARCHIVE_SCORE_PE_MATCHES);
}