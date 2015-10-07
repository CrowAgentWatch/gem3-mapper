/*
 * PROJECT: GEMMapper
 * FILE: search_parameters.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#include "search_parameters.h"

/*
 * Macro Utils
 */
#define SEARCH_INSTANTIATE_VALUE(parameters,field_name,pattern_length) \
  parameters->field_name##_nominal = integer_proportion(parameters->search_parameters->field_name,pattern_length)

/*
 * Region profile default parameters
 */
GEM_INLINE void search_parameters_init_replacements(search_parameters_t* const search_parameters) {
  // Reset
  memset(search_parameters->allowed_chars,0,256*sizeof(bool));
  memset(search_parameters->allowed_enc,0,DNA_EXT_RANGE*sizeof(bool));
  search_parameters->mismatch_alphabet[0] = DNA_CHAR_A;
  search_parameters->mismatch_alphabet[1] = DNA_CHAR_C;
  search_parameters->mismatch_alphabet[2] = DNA_CHAR_G;
  search_parameters->mismatch_alphabet[3] = DNA_CHAR_T;
  search_parameters->mismatch_alphabet_length = 4;
  search_parameters->allowed_chars[DNA_CHAR_A] = true;
  search_parameters->allowed_chars[DNA_CHAR_C] = true;
  search_parameters->allowed_chars[DNA_CHAR_G] = true;
  search_parameters->allowed_chars[DNA_CHAR_T] = true;
  search_parameters->allowed_chars[DNA_CHAR_N] = false;
  search_parameters->allowed_enc[ENC_DNA_CHAR_A] = true;
  search_parameters->allowed_enc[ENC_DNA_CHAR_C] = true;
  search_parameters->allowed_enc[ENC_DNA_CHAR_G] = true;
  search_parameters->allowed_enc[ENC_DNA_CHAR_T] = true;
  search_parameters->allowed_enc[ENC_DNA_CHAR_N] = false;
}
GEM_INLINE void search_parameters_init_error_model(search_parameters_t* const search_parameters) {
  search_parameters->search_max_matches = 1000;
  search_parameters->complete_search_error = 0.04;
  search_parameters->complete_strata_after_best = 1.0;
  search_parameters->alignment_max_error = 0.08;
  search_parameters->alignment_max_error_after_best = 0.04;
  search_parameters->unbounded_alignment = unbounded_alignment_if_unmapped;
  search_parameters->max_bandwidth = 0.20;
  search_parameters->alignment_min_identity = 0.40;
  search_parameters->alignment_scaffolding = true;
  search_parameters->alignment_scaffolding_min_coverage = 0.80;
  search_parameters->alignment_scaffolding_homopolymer_min_context = 2;
  search_parameters->alignment_scaffolding_min_matching_length = 10;
  search_parameters->cigar_curation = true;
  search_parameters->cigar_curation_min_end_context = 2;
}
GEM_INLINE void search_parameters_init_alignment_model(search_parameters_t* const search_parameters) {
  search_parameters->alignment_model = alignment_model_gap_affine;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_A] = +1;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_C] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_G] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_T] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_A] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_C] = +1;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_G] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_T] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_A] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_C] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_G] = +1;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_T] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_A] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_C] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_G] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_T] = +1;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_A] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_C] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_G] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_T] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.generic_match_score = 1;
  search_parameters->swg_penalties.generic_mismatch_score = -4;
  search_parameters->swg_penalties.gap_open_score = -6;
  search_parameters->swg_penalties.gap_extension_score = -1;
  search_parameters->swg_threshold = 0.20; // 0.20*read_length*match_score
}
GEM_INLINE void search_parameters_init_internals(search_parameters_t* const search_parameters) {
  // Region-Minimal Scheme = (20,4,2,2)
  search_parameters->rp_minimal.region_th = 20;
  search_parameters->rp_minimal.max_steps = 4;
  search_parameters->rp_minimal.dec_factor = 2;
  search_parameters->rp_minimal.region_type_th = 2;
  // Region-Boost Scheme = (500,1,8,50)
  search_parameters->rp_boost.region_th = 500;
  search_parameters->rp_boost.max_steps = 1;
  search_parameters->rp_boost.dec_factor = 8;
  search_parameters->rp_boost.region_type_th = 50;
  // Region-Delimit Scheme = (100,4,2,2)
  search_parameters->rp_delimit.region_th = 100;
  search_parameters->rp_delimit.max_steps = 4;
  search_parameters->rp_delimit.dec_factor = 2;
  search_parameters->rp_delimit.region_type_th = 2;
  // Filtering Thresholds
  search_parameters->filtering_region_factor = 1.0;
  search_parameters->filtering_threshold = 1000;
}
GEM_INLINE void search_parameters_init(search_parameters_t* const search_parameters) {
  // Mapping strategy
  search_parameters->mapping_mode = mapping_adaptive_filtering_thorough;
  // Qualities
  search_parameters->quality_model = quality_model_type_gem;
  search_parameters->quality_format = qualities_ignore;
  search_parameters->quality_threshold = 26;
  // Replacements
  search_parameters_init_replacements(search_parameters);
  // Search error model
  search_parameters_init_error_model(search_parameters);
  // Alignment Model/Score
  search_parameters_init_alignment_model(search_parameters);
  // Paired End
  paired_search_parameters_init(&search_parameters->paired_search_parameters);
  // Bisulfite
  search_parameters->bisulfite_read = bisulfite_read_inferred;
  // Internals
  search_parameters_init_internals(search_parameters);
}
GEM_INLINE void search_configure_mapping_strategy(
    search_parameters_t* const search_parameters,const mapping_mode_t mapping_mode) {
  search_parameters->mapping_mode = mapping_mode;
}
GEM_INLINE void search_configure_quality_model(
    search_parameters_t* const search_parameters,const quality_model_t quality_model,
    const quality_format_t quality_format,const uint64_t quality_threshold) {
  search_parameters->quality_model = quality_model;
  search_parameters->quality_format = quality_format;
  search_parameters->quality_threshold = quality_threshold;
}
GEM_INLINE void search_configure_matches(
    search_parameters_t* const search_parameters,const uint64_t search_max_matches) {
  search_parameters->search_max_matches = search_max_matches;
}
GEM_INLINE void search_configure_replacements(
    search_parameters_t* const search_parameters,
    const char* const mismatch_alphabet,const uint64_t mismatch_alphabet_length) {
  // Reset
  search_parameters_init_replacements(search_parameters);
  // Filter replacements
  uint64_t i, count;
  for (i=0,count=0;i<mismatch_alphabet_length;i++) {
    if (is_dna(mismatch_alphabet[i])) {
      const char c = dna_normalized(mismatch_alphabet[i]);
      search_parameters->mismatch_alphabet[count] = c;
      search_parameters->allowed_chars[(uint8_t)c] = true;
      search_parameters->allowed_enc[dna_encode(c)] = true;
      ++count;
    }
  }
  gem_cond_fatal_error(count==0,ASP_REPLACEMENT_EMPTY);
  search_parameters->mismatch_alphabet_length = count;
}
GEM_INLINE void search_configure_alignment_model(
    search_parameters_t* const search_parameters,const alignment_model_t alignment_model) {
  search_parameters->alignment_model = alignment_model;
}
GEM_INLINE void search_configure_alignment_match_scores(
    search_parameters_t* const search_parameters,const uint64_t matching_score) {
  // Match
  search_parameters->swg_penalties.generic_match_score = matching_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_A] = matching_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_C] = matching_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_G] = matching_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_T] = matching_score;
}
GEM_INLINE void search_configure_alignment_mismatch_scores(
    search_parameters_t* const search_parameters,const uint64_t mismatch_penalty) {
  // Mismatch
  const int64_t mismatch_score = -((int64_t)mismatch_penalty);
  search_parameters->swg_penalties.generic_mismatch_score = mismatch_penalty;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_C] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_G] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_T] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_N] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_A] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_G] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_T] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_N] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_A] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_C] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_T] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_N] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_A] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_C] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_G] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_N] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_A] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_C] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_G] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_T] = mismatch_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_N] = mismatch_score;
}
GEM_INLINE void search_configure_alignment_gap_scores(
    search_parameters_t* const search_parameters,
    const uint64_t gap_open_penalty,const uint64_t gap_extension_penalty) {
  // Gaps
  search_parameters->swg_penalties.gap_open_score = -((int32_t)gap_open_penalty);
  search_parameters->swg_penalties.gap_extension_score = -((int32_t)gap_extension_penalty);
}
GEM_INLINE void search_instantiate_values(as_parameters_t* const parameters,const uint64_t pattern_length) {
  /* Nominal search parameters (Evaluated to read-length) */
  SEARCH_INSTANTIATE_VALUE(parameters,complete_search_error,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,complete_strata_after_best,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,alignment_max_error,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,alignment_max_error_after_best,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,max_bandwidth,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,alignment_min_identity,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,alignment_scaffolding_min_coverage,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,alignment_scaffolding_homopolymer_min_context,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,alignment_scaffolding_min_matching_length,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,cigar_curation_min_end_context,pattern_length);
  SEARCH_INSTANTIATE_VALUE(parameters,swg_threshold,pattern_length);
  search_parameters_t* const search_parameters = parameters->search_parameters;
  if (search_parameters->swg_threshold <= 0.0 || search_parameters->swg_threshold >= 1.0) {
    parameters->swg_threshold_nominal = search_parameters->swg_threshold;
  } else {
    swg_penalties_t* const swg_penalties = &search_parameters->swg_penalties;
    const int64_t pattern_identity_th = search_parameters->swg_threshold*(double)pattern_length;
    parameters->swg_threshold_nominal = pattern_identity_th * swg_penalties->generic_match_score;
  }
}
/*
 * Display
 */
GEM_INLINE void search_parameters_print(FILE* const stream,search_parameters_t* const search_parameters) {
  // TODO
  // TODO
  // TODO
}
GEM_INLINE void as_parameters_print(FILE* const stream,as_parameters_t* const parameters) {
  tab_fprintf(stream,"[GEM]>AS.Parameters\n");
  tab_fprintf(stream,"=> Search.Parameters\n"); // TODO
//  tab_global_inc(); // TODO
//  search_parameters_print(stream,parameters->search_parameters); // TODO
//  tab_global_dec(); // TODO
  tab_global_inc();
  tab_fprintf(stream,"=> Nominal.Parameters\n");
  tab_fprintf(stream,"  => Complete.search.error %lu\n",parameters->complete_search_error_nominal);
  tab_fprintf(stream,"  => Complete.strata.after.best %lu\n",parameters->complete_strata_after_best_nominal);
  tab_fprintf(stream,"  => Alingment.max.error %lu\n",parameters->alignment_max_error_nominal);
  tab_fprintf(stream,"  => Alingment.max.error.after.best %lu\n",parameters->alignment_max_error_after_best_nominal);
  tab_fprintf(stream,"  => Alignment_min.identity %lu\n",parameters->alignment_min_identity_nominal);
  tab_fprintf(stream,"  => Alignment.scaffolding.min.coverage %lu\n",parameters->alignment_scaffolding_min_coverage_nominal);
  tab_fprintf(stream,"  => Alignment.scaffolding.homopolymer.min_context %lu\n",parameters->alignment_scaffolding_homopolymer_min_context_nominal);
  tab_fprintf(stream,"  => Alignment.scaffolding.min.matching.length %lu\n",parameters->alignment_scaffolding_min_matching_length_nominal);
  tab_fprintf(stream,"  => Max.bandwidth %lu\n",parameters->max_bandwidth_nominal);
  tab_fprintf(stream,"  => Cigar.curation.min.end.context %lu\n",parameters->cigar_curation_min_end_context_nominal);
  tab_fprintf(stream,"  => Swg.threshold %lu\n",parameters->swg_threshold_nominal);
  tab_global_dec();
}
