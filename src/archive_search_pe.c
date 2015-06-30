/*
 * PROJECT: GEMMapper
 * FILE: archive_search_pe.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include "archive_search_pe.h"
#include "archive_search_se.h"
#include "archive_select.h"
#include "archive_score.h"
#include "matches_classify.h"

/*
 * Debug
 */
#define DEBUG_ARCHIVE_SEARCH_READ_NAME GEM_DEEP_DEBUG

/*
 * Setup
 */
GEM_INLINE void archive_search_paired_end_configure(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    mm_search_t* const mm_search) {
  archive_search_configure(archive_search_end1,paired_end1,mm_search);
  archive_search_configure(archive_search_end2,paired_end2,mm_search);
}

/*
 * PE Archive Search building blocks
 */
GEM_INLINE uint64_t archive_search_paired_end_extend_matches(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches,const sequence_end_t candidate_end) {
  PROF_START(GP_ARCHIVE_SEARCH_PE_EXTEND_CANDIDATES);
  // Parameters
  search_parameters_t* const search_parameters = archive_search_end1->as_parameters.search_parameters;
  paired_search_parameters_t* const paired_search_parameters = &search_parameters->paired_search_parameters;
  mapper_stats_t* const mapper_stats = archive_search_end1->mapper_stats;
  archive_t* const archive = archive_search_end1->archive;
  // Extend in all possible concordant orientations
  matches_t* const matches_end1 = paired_matches->matches_end1;
  matches_t* const matches_end2 = paired_matches->matches_end2;
  /*
   * Configure extension
   *   All extensions are done against the forward strand. If the candidate is in the reverse,
   *   the reverse-candidate region is generated reverse-complementing the forward. Thus, all
   *   the extension is done using @forward_search_state (matches contain all the strand needed info)
   */
  archive_search_t* candidate_archive_search;
  matches_t* candidate_matches;
  matches_t* extended_matches;
  as_parameters_t* as_parameters;
  if (candidate_end==paired_end2) {
    // Extend towards end/2
    candidate_archive_search = archive_search_end2;
    as_parameters = &candidate_archive_search->as_parameters;
    extended_matches = matches_end1;
    candidate_matches = matches_end2;
  } else {
    // Extend towards end/1
    candidate_archive_search = archive_search_end1;
    as_parameters = &candidate_archive_search->as_parameters;
    extended_matches = matches_end2;
    candidate_matches = matches_end1;
  }
  filtering_candidates_t* const filtering_candidates = candidate_archive_search->forward_search_state.filtering_candidates;
  pattern_t* const pattern = &candidate_archive_search->forward_search_state.pattern;
  text_collection_t* const text_collection = candidate_archive_search->text_collection;
  mm_stack_t* const mm_stack = candidate_archive_search->mm_stack;
  const uint64_t num_base_candidate_matches = matches_get_num_match_traces(candidate_matches);
  uint64_t matches_found = 0, total_matches_found = 0;
  // Iterate over all matches of the extended end
  VECTOR_ITERATE(extended_matches->position_matches,extended_match,en,match_trace_t) {
    if (paired_search_parameters->pair_orientation_FR == pair_orientation_concordant) {
      // Extend (filter nearby region)
      strand_t candidate_strand;
      if (extended_match->strand==Forward) {
        candidate_strand = Reverse;
        matches_found = filtering_candidates_extend_match(
            filtering_candidates,archive,text_collection,extended_match,pattern,true,true,
            as_parameters,mapper_stats,paired_matches,candidate_end,mm_stack);
      } else { // end2==Reverse
        candidate_strand = Forward;
        // TODO improve using Text-Dimensions
        extended_match->match_alignment.match_position =  // Locates against forward strand
            inverse_locator_map(archive->locator,(uint8_t*)extended_match->sequence_name,
            extended_match->strand,extended_match->text_position);
        matches_found = filtering_candidates_extend_match(
            filtering_candidates,archive,text_collection,extended_match,pattern,false,false,
            as_parameters,mapper_stats,paired_matches,candidate_end,mm_stack);
      }
      // Decode & Pair found matches
      total_matches_found += matches_found;
	    match_trace_t* const mates_array = matches_get_match_trace_buffer(candidate_matches);
	    // TODO Do the decode once at pairing & carry the proper strand at matches insertion
      archive_select_decode_trace_matches(candidate_archive_search,candidate_matches,mates_array,
          total_matches_found+num_base_candidate_matches,true,candidate_strand);
    }
  }
  PROF_STOP(GP_ARCHIVE_SEARCH_PE_EXTEND_CANDIDATES);
  return total_matches_found;
}
/*
 * PE Generated Extension Candidates
 */
GEM_INLINE void archive_search_paired_end_generate_extension_candidates(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches,const sequence_end_t candidate_end) {
  // Check
  archive_t* const archive = archive_search_end1->archive;
  gem_check(!archive->indexed_complement || archive_search_end1->emulate_rc_search,ARCHIVE_SEARCH_INDEX_COMPLEMENT_REQUIRED);
  // Generate extension candidates
  search_parameters_t* const search_parameters = archive_search_end1->as_parameters.search_parameters;
  mapper_stats_t* const mapper_stats = archive_search_end1->mapper_stats;
  approximate_search_t* const forward_asearch_end1 = &archive_search_end1->forward_search_state;
  approximate_search_t* const forward_asearch_end2 = &archive_search_end2->forward_search_state;
  if (candidate_end==paired_end2) {
    filtering_candidates_process_extension_candidates(
        forward_asearch_end1->filtering_candidates,forward_asearch_end2->filtering_candidates,
        archive,archive_search_end1->text_collection,&forward_asearch_end1->pattern,&forward_asearch_end2->pattern,
        search_parameters,mapper_stats,paired_matches,archive_search_end1->mm_stack);
  } else {
    filtering_candidates_process_extension_candidates(
        forward_asearch_end2->filtering_candidates,forward_asearch_end1->filtering_candidates,
        archive,archive_search_end1->text_collection,&forward_asearch_end2->pattern,&forward_asearch_end1->pattern,
        search_parameters,mapper_stats,paired_matches,archive_search_end1->mm_stack);
  }
}
/*
 * PE Extension Control
 */
GEM_INLINE bool archive_search_paired_end_feasible_extension(archive_search_t* const archive_search) {
  // Check the number of samples to derive the expected template size
  const uint64_t ts_margin_error = archive_search->as_parameters.max_filtering_error_nominal;
  return mapper_stats_template_length_estimation_within_ci(archive_search->mapper_stats,ts_margin_error);
}
GEM_INLINE bool archive_search_paired_end_use_shortcut_extension(archive_search_t* const archive_search,matches_t* const matches) {
//  // Check mapping-mode
//  if (archive_search->as_parameters.search_parameters->mapping_mode!=mapping_adaptive_filtering_fast) return false;
  // Check the number of samples to derive the expected template size
  const uint64_t ts_margin_error = archive_search->as_parameters.max_filtering_error_nominal;
  if (!mapper_stats_template_length_estimation_within_ci(archive_search->mapper_stats,ts_margin_error)) return false;
  // Test if the end can be classify as unique with enough confidence
  if (matches->max_complete_stratum <= 1) return false;
  if (matches_classify(matches)==matches_class_unique) {
    const swg_penalties_t* const swg_penalties = &archive_search->as_parameters.search_parameters->swg_penalties;
    const uint64_t read_length = sequence_get_length(&archive_search->sequence);
    const uint64_t max_region_length = archive_search_get_max_region_length(archive_search);
    const uint64_t proper_length = fm_index_get_proper_length(archive_search->archive->fm_index);
    matches_predictors_t predictors;
    matches_classify_compute_predictors(matches,&predictors,
        swg_penalties,read_length,max_region_length,proper_length,UINT64_MAX);
    return matches_classify_unique(&predictors) >= MATCHES_UNIQUE_CI;
  }
  return false;
}
GEM_INLINE bool archive_search_paired_end_use_recovery_extension(archive_search_t* const archive_search,matches_t* const matches) {
  // Check the number of samples to derive the expected template size
  const uint64_t ts_margin_error = archive_search->as_parameters.max_filtering_error_nominal;
  if (!mapper_stats_template_length_estimation_within_ci(archive_search->mapper_stats,ts_margin_error)) return false;
  // Test if the end can be classify as ambiguous with enough confidence (as to rescue it)
  if (matches->max_complete_stratum <= 1) return true;
  const matches_class_t matches_class = matches_classify(matches);
  switch (matches_class) {
    case matches_class_unmapped:
      return true;
    case matches_class_tie_indistinguishable:
    case matches_class_tie_swg_score:
    case matches_class_tie_edit_distance:
      return true;
    default: {
      const swg_penalties_t* const swg_penalties = &archive_search->as_parameters.search_parameters->swg_penalties;
      const uint64_t read_length = sequence_get_length(&archive_search->sequence);
      const uint64_t max_region_length = archive_search_get_max_region_length(archive_search);
      const uint64_t proper_length = fm_index_get_proper_length(archive_search->archive->fm_index);
      matches_predictors_t predictors;
      matches_classify_compute_predictors(matches,&predictors,
          swg_penalties,read_length,max_region_length,proper_length,UINT64_MAX);
      switch (matches_class) {
        case matches_class_tie_event_distance:
          return true;
//          return matches_classify_ties(&predictors) < MATCHES_TIES_CI;
        case matches_class_mmap:
          return true;
//          return matches_classify_mmaps(&predictors) < MATCHES_MMAPS_CI;
        case matches_class_unique:
          return matches_classify_unique(&predictors) < MATCHES_UNIQUE_CI;
        default:
          GEM_INVALID_CASE();
          break;
      }
    }
  }
  return false;
}
/*
 * PE Online Approximate String Search
 */
GEM_INLINE void archive_search_paired_end_continue(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  PROF_START(GP_ARCHIVE_SEARCH_PE);
  // Parameters
  search_parameters_t* const search_parameters = archive_search_end1->as_parameters.search_parameters;
  paired_search_parameters_t* const paired_search_parameters = &search_parameters->paired_search_parameters;
  mapper_stats_t* const mapper_stats = archive_search_end1->mapper_stats;
  matches_t* const matches_end1 = (paired_matches!=NULL) ? paired_matches->matches_end1 : NULL;
  matches_t* const matches_end2 = (paired_matches!=NULL) ? paired_matches->matches_end2 : NULL;
  // Callback (switch to proper search stage)
  while (archive_search_end1->pe_search_state != archive_search_pe_end) {
    switch (archive_search_end1->pe_search_state) {
      case archive_search_pe_begin: // Beginning of the search (Init)
        archive_search_end1->pair_searched = false;
        archive_search_end1->pair_extended = false;
        archive_search_end2->pair_searched = false;
        archive_search_end2->pair_extended = false;
        archive_search_reset(archive_search_end1); // Init (End/1)
        archive_search_reset(archive_search_end2); // Init (End/2)
      // No break
      case archive_search_pe_search_end1:
        // Full Search (End/1)
        archive_search_finish_search(archive_search_end1,matches_end1);
        archive_select_matches(archive_search_end1,true,matches_end1);
        archive_search_end1->pair_searched = true;
        archive_search_end1->end_class = matches_classify(matches_end1);
        // Test for extension of End/1 (Shortcut to avoid mapping end/2)
        archive_search_end1->pair_extended =
            archive_search_paired_end_use_shortcut_extension(archive_search_end1,matches_end1);
        if (archive_search_end1->pair_extended) {
          // Extend End/1
          PROF_START(GP_ARCHIVE_SEARCH_PE_EXTENSION_SHORTCUT);
          const uint64_t num_matches_found = archive_search_paired_end_extend_matches(
              archive_search_end1,archive_search_end2,paired_matches,paired_end2);
          PROF_STOP(GP_ARCHIVE_SEARCH_PE_EXTENSION_SHORTCUT);
          // Check results of the extension
          if (num_matches_found > 0) {
            PROF_INC_COUNTER(GP_ARCHIVE_SEARCH_PE_EXTENSION_SHORTCUT_SUCCESS);
            archive_search_end1->pe_search_state = archive_search_pe_find_pairs;
            break; // Callback
          }
          /*
           * Extension failed because
           *  (1) Insert size is beyond the expected distribution (Any insert size filtering/search must be discarded)
           *  (2) Sensitivity of the candidates/end1 search is not enough
           */
        }
        // No break
      case archive_search_pe_search_end2:
        // Full Search (End/2)
        archive_search_finish_search(archive_search_end2,matches_end2);
        archive_select_matches(archive_search_end2,true,matches_end2);
        archive_search_end2->pair_searched = true;
        archive_search_end2->end_class = matches_classify(matches_end2);
        // No break
      case archive_search_pe_recovery:
        // Paired-end recovery by extension
        if (!archive_search_end1->pair_extended) {
          if (archive_search_paired_end_use_recovery_extension(archive_search_end2,matches_end2)) {
            // Extend End/1
            PROF_START(GP_ARCHIVE_SEARCH_PE_EXTENSION_RECOVERY);
            archive_search_paired_end_extend_matches(archive_search_end1,archive_search_end2,paired_matches,paired_end2);
            archive_search_end1->pair_extended = true;
            PROF_STOP(GP_ARCHIVE_SEARCH_PE_EXTENSION_RECOVERY);
          }
        }
        if (!archive_search_end2->pair_extended) {
          if (archive_search_paired_end_use_recovery_extension(archive_search_end1,matches_end1)) {
            // Extend End/1
            PROF_START(GP_ARCHIVE_SEARCH_PE_EXTENSION_RECOVERY);
            archive_search_paired_end_extend_matches(archive_search_end1,archive_search_end2,paired_matches,paired_end1);
            archive_search_end2->pair_extended = true;
            PROF_STOP(GP_ARCHIVE_SEARCH_PE_EXTENSION_RECOVERY);
          }
        }
        // No break
      case archive_search_pe_find_pairs: {
        // Pair matches (Cross-link matches from both ends)
        const uint64_t num_matches_end1 = matches_get_num_match_traces(paired_matches->matches_end1);
        const uint64_t num_matches_end2 = matches_get_num_match_traces(paired_matches->matches_end2);
        if (num_matches_end1 > 0 && num_matches_end2 > 0) {
          paired_matches_find_pairs(paired_matches,paired_search_parameters,mapper_stats);
          paired_matches_find_discordant_pairs(paired_matches,paired_search_parameters); // Find discordant (if required)
        }
        // Check number of paired-matches
        if (paired_matches_get_num_maps(paired_matches) == 0) {
          if (!archive_search_end2->pair_searched) {
            archive_search_end1->pe_search_state = archive_search_pe_search_end2;
            break; // Callback
          }
        }
        // Select matches
        paired_matches->max_complete_stratum =
            ((matches_end1->max_complete_stratum!=ALL) ? matches_end1->max_complete_stratum : 0) +
            ((matches_end2->max_complete_stratum!=ALL) ? matches_end2->max_complete_stratum : 0);
        archive_select_paired_matches(archive_search_end1,archive_search_end2,paired_matches);
        archive_search_end1->pe_search_state = archive_search_pe_end; // End of the workflow
        break;
      }
      default:
        GEM_INVALID_CASE();
        break;
    }
  }
  PROF_STOP(GP_ARCHIVE_SEARCH_PE);
}
GEM_INLINE void archive_search_pe_generate_candidates(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  PROF_START(GP_ARCHIVE_SEARCH_PE);
  PROF_START(GP_ARCHIVE_SEARCH_PE_GENERATE_CANDIDATES);
  // Beginning of the search (Init)
  archive_search_end1->pe_search_state = archive_search_pe_begin;
  archive_search_end1->pair_searched = false;
  archive_search_end1->pair_extended = false;
  archive_search_end2->pe_search_state = archive_search_pe_begin;
  archive_search_end2->pair_searched = false;
  archive_search_end2->pair_extended = false;
  // Generate Candidates (End/1)
  archive_search_generate_candidates(archive_search_end1);
  archive_search_end1->pair_searched = true;
  archive_search_end1->pair_extended = archive_search_paired_end_feasible_extension(archive_search_end1);
  if (archive_search_end1->pair_extended) {
    // Extension of end/1
    PROF_START(GP_ARCHIVE_SEARCH_PE_EXTENSION_SHORTCUT);
    archive_search_reset(archive_search_end2); // Init end/2
    archive_search_paired_end_generate_extension_candidates(
        archive_search_end1,archive_search_end2,paired_matches,paired_end2); // Generate candidates (from extension)
    archive_search_end1->pe_search_state = archive_search_pe_find_pairs;
    PROF_STOP(GP_ARCHIVE_SEARCH_PE_EXTENSION_SHORTCUT);
  } else {
    // Generate Candidates (End/2)
    archive_search_generate_candidates(archive_search_end2);
    archive_search_end2->pair_searched = true;
    archive_search_end2->pair_extended = archive_search_paired_end_feasible_extension(archive_search_end2);
    if (archive_search_end2->pair_extended) {
      // Extension of end/1
      PROF_START(GP_ARCHIVE_SEARCH_PE_EXTENSION_SHORTCUT);
      archive_search_paired_end_generate_extension_candidates(
          archive_search_end1,archive_search_end2,paired_matches,paired_end1); // Generate candidates (from extension)
      archive_search_end1->pe_search_state = archive_search_pe_find_pairs;
      PROF_STOP(GP_ARCHIVE_SEARCH_PE_EXTENSION_SHORTCUT);
    }
  }
  PROF_STOP(GP_ARCHIVE_SEARCH_PE_GENERATE_CANDIDATES);
  PROF_STOP(GP_ARCHIVE_SEARCH_PE);
}
GEM_INLINE void archive_search_pe_finish_search(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  PROF_START(GP_ARCHIVE_SEARCH_PE_FINISH_SEARCH);
  // Decode trace-matches found
  const uint64_t num_matches_end1 = matches_get_num_match_traces(paired_matches->matches_end1);
  const uint64_t num_matches_end2 = matches_get_num_match_traces(paired_matches->matches_end2);
  if (num_matches_end1 > 0 && num_matches_end2 > 0) {
    // TODO Revise this
    archive_select_decode_trace_matches_all(archive_search_end1,paired_matches->matches_end1,false,Forward);
    archive_select_decode_trace_matches_all(archive_search_end2,paired_matches->matches_end2,false,Forward);
  }
  // PE search (continue search until the end)
  archive_search_paired_end_continue(archive_search_end1,archive_search_end2,paired_matches);
  PROF_STOP(GP_ARCHIVE_SEARCH_PE_FINISH_SEARCH);
}
/*
 * Paired-End Indexed Search (PE Online Approximate String Search)
 */
GEM_INLINE void archive_search_paired_end(
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  // Init
  archive_search_end1->pe_search_state = archive_search_pe_begin;
  archive_search_end2->pe_search_state = archive_search_pe_begin;
  // PE search
  archive_search_paired_end_continue(archive_search_end1,archive_search_end2,paired_matches);
}
