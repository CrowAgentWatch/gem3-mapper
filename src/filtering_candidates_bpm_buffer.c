/*
 * PROJECT: GEMMapper
 * FILE: filtering_candidates_bpm_buffer.c
 * DATE: 06/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#include "filtering_candidates_bpm_buffer.h"
#include "align_bpm_pattern.h"

/*
 * Debug
 */
#define DEBUG_FILTERING_CANDIDATES  GEM_DEEP_DEBUG

/*
 * Profile
 */
#define PROFILE_LEVEL PMED


/*
 * BPM-Buffer API (Verification)
 */
GEM_INLINE uint64_t filtering_candidates_bpm_buffer_add(
    filtering_candidates_t* const filtering_candidates,
    pattern_t* const pattern,bpm_gpu_buffer_t* const bpm_gpu_buffer) {
  // Check number of pending regions
  const uint64_t pending_candidates = filtering_candidates_get_num_candidate_regions(filtering_candidates);
  if (pending_candidates==0) return 0;
  // Add the pattern(chunks) to the buffer (add new queries)
  bpm_gpu_buffer_put_pattern(bpm_gpu_buffer,pattern);
  // Fetch pattern dimensions
  const uint64_t max_error = pattern->max_effective_filtering_error;
  const uint64_t pattern_length = pattern->key_length;
  const uint64_t num_pattern_chunks = pattern->bpm_pattern.gpu_num_chunks;
  const uint64_t pattern_tile_tall = pattern->bpm_pattern.gpu_entries_per_chunk*BPM_GPU_PATTERN_ENTRY_LENGTH;
  // Traverse all candidates (text-space) & add them to the buffer
  const filtering_region_t* candidate_region = vector_get_mem(filtering_candidates->filtering_regions,filtering_region_t);
  uint64_t candidate_pos, pattern_chunk, total_candidates_added;
  for (candidate_pos=0,total_candidates_added=0;candidate_pos<pending_candidates;++candidate_pos,++candidate_region) {
    // Locate candidate sequence
    const uint64_t begin_position = candidate_region->begin_position;
    const uint64_t candidate_length = candidate_region->end_position - begin_position;
    // Calculate tile dimensions
    pattern_tiled_t pattern_tiled;
    const bool pattern_can_align = pattern_tiled_init(&pattern_tiled,
        pattern_length,pattern_tile_tall,candidate_length,max_error);
    if (!pattern_can_align) continue;
    // Initialize current tile variables
    for (pattern_chunk=0;pattern_chunk<num_pattern_chunks;++pattern_chunk,++total_candidates_added) {
      // BPM-GPU put candidate
      bpm_gpu_buffer_put_candidate(bpm_gpu_buffer,
          begin_position+pattern_tiled.tile_offset,pattern_tiled.tile_wide,pattern_chunk);
      // Calculate next tile
      pattern_tiled_calculate_next(&pattern_tiled);
    }
  }
  // Return the final number of chunk-candidates added to the buffer
  PROF_ADD_COUNTER(GP_BMP_TILED_NUM_TILES,total_candidates_added);
  PROF_ADD_COUNTER(GP_BMP_TILED_NUM_TILES_VERIFIED,total_candidates_added);
  return total_candidates_added;
}
GEM_INLINE uint64_t filtering_candidates_bpm_buffer_retrieve(
    filtering_candidates_t* const filtering_candidates,archive_text_t* const archive_text,
    text_collection_t* const text_collection,pattern_t* const pattern,
    bpm_gpu_buffer_t* const bpm_gpu_buffer,const uint64_t candidate_offset_begin,
    const uint64_t candidate_offset_end,mm_stack_t* const mm_stack) {
  /*
   * Retrieve filtering-regions from BPM-Buffer
   */
  PROFILE_START(GP_FC_RETRIEVE_BPM_BUFFER_CANDIDATE_REGIONS,PROFILE_LEVEL);
  // Clear filtering candidates
  filtering_candidates_clear(filtering_candidates);
  if (gem_expect_false(candidate_offset_begin==candidate_offset_end)) return 0;
  // Fetch Parameters
  const uint64_t key_length = pattern->key_length;
  const uint64_t max_error = pattern->max_effective_filtering_error;
  // Fetch tile dimensions
  const uint64_t num_chunks = pattern->bpm_pattern.gpu_num_chunks;
  const uint64_t pattern_tile_tall = pattern->bpm_pattern.gpu_entries_per_chunk*BPM_GPU_PATTERN_ENTRY_LENGTH;
  // Prepare filtering-regions vectors
  const uint64_t pending_candidates = (candidate_offset_end-candidate_offset_begin)/num_chunks;
  vector_reserve(filtering_candidates->verified_regions,pending_candidates,false);
  vector_reserve(filtering_candidates->filtering_regions,pending_candidates,false);
  vector_reserve(filtering_candidates->discarded_regions,pending_candidates,false);
  filtering_region_t* regions_accepted = vector_get_mem(filtering_candidates->filtering_regions,filtering_region_t);
  filtering_region_t* regions_discarded = vector_get_mem(filtering_candidates->discarded_regions,filtering_region_t);
  verified_region_t* regions_verified = vector_get_mem(filtering_candidates->verified_regions,verified_region_t);
  // Traverse all candidates (text-space) & sum-up their alignment distance
  uint64_t num_accepted_regions = 0;
  uint64_t candidate_idx=candidate_offset_begin, pattern_chunk, candidate_pos;
  for (candidate_pos=0;candidate_pos<pending_candidates;++candidate_pos) {
    // Get the accepted candidate
    uint64_t candidate_begin_position, candidate_end_position, candidate_length;
    uint32_t chunk_length;
    bpm_gpu_buffer_get_candidate(bpm_gpu_buffer,candidate_idx,&candidate_begin_position,&chunk_length);
    bpm_gpu_buffer_get_candidate(bpm_gpu_buffer,candidate_idx+(num_chunks-1),&candidate_end_position,&chunk_length);
    candidate_end_position += chunk_length;
    candidate_length = candidate_end_position - candidate_begin_position;
    // Calculate tile dimensions (Sum up the alignment distance of all the tiles)
    pattern_tiled_t pattern_tiled;
    const bool pattern_can_align = pattern_tiled_init(&pattern_tiled,key_length,pattern_tile_tall,candidate_length,max_error);
    if (!pattern_can_align) continue;
    bool unaligned_tiled = false;
    uint64_t global_distance = 0, distance_link_tiles = 0;
    for (pattern_chunk=0;pattern_chunk<num_chunks;++pattern_chunk,++candidate_idx) {
      // Retrieve alignment distance
      uint32_t tile_distance=0, tile_match_column=0;
      bpm_gpu_buffer_get_candidate_result(bpm_gpu_buffer,candidate_idx,&tile_distance,&tile_match_column);
      pattern_tiled.tile_distance = tile_distance;
      pattern_tiled.tile_match_column = tile_match_column;
      global_distance += tile_distance;
      if (tile_distance > max_error) unaligned_tiled = true;
      // Bound the joint distance by estimating the cost of connecting the path through the tiles
      distance_link_tiles += pattern_tiled_bound_matching_path(&pattern_tiled);
      // Calculate next tile
      pattern_tiled_calculate_next(&pattern_tiled);
    }
    // Check total distance & Compose the retrieved region
    if (!unaligned_tiled && global_distance <= max_error) {
      // Configure accepted candidate
      regions_accepted->status = filtering_region_accepted;
      regions_accepted->text_trace_offset = UINT64_MAX; // Not retrieved yet
      regions_accepted->begin_position = candidate_begin_position;
      regions_accepted->end_position = candidate_end_position;
      regions_accepted->base_position_offset = 0;
      // Configure regions matching (we sacrifice this information as to save memory)
      match_scaffold_init(&regions_accepted->match_scaffold);
      // Distance Bound estimation
      regions_accepted->align_distance_min_bound = (global_distance > max_error) ? max_error : global_distance;
      global_distance += distance_link_tiles;
      regions_accepted->align_distance = (global_distance > max_error) ? max_error : global_distance;
      regions_accepted->align_match_end_column =
          BOUNDED_ADDITION(pattern_tiled.prev_tile_match_position,regions_accepted->align_distance,candidate_length-1);
      ++regions_accepted; ++num_accepted_regions;
    } else {
      // Configure discarded candidate
      regions_discarded->status = filtering_region_verified_discarded;
      regions_discarded->text_trace_offset = UINT64_MAX; // Not retrieved yet
      regions_discarded->begin_position = candidate_begin_position;
      regions_discarded->end_position = candidate_end_position;
      // Distance Bound estimation
      regions_discarded->align_distance_min_bound = global_distance;
      regions_discarded->align_distance = global_distance + distance_link_tiles;
      // Configure regions matching (we sacrifice this information as to save memory)
      match_scaffold_init(&regions_discarded->match_scaffold);
      ++regions_discarded;
    }
    // Add to verified regions
    regions_verified->begin_position = candidate_begin_position;
    regions_verified->end_position = candidate_end_position;
    ++regions_verified;
  }
  // Update Accepted/Discarded/Verified
  vector_update_used(filtering_candidates->verified_regions,regions_verified);
  vector_update_used(filtering_candidates->filtering_regions,regions_accepted);
  vector_update_used(filtering_candidates->discarded_regions,regions_discarded);
  PROFILE_STOP(GP_FC_RETRIEVE_BPM_BUFFER_CANDIDATE_REGIONS,PROFILE_LEVEL);
  // DEBUG
  gem_cond_debug_block(DEBUG_FILTERING_CANDIDATES) {
    tab_fprintf(gem_log_get_stream(),"[GEM]>Filtering.Candidates (verify_regions_BPM_buffer)\n");
    tab_global_inc();
    filtering_candidates_print_regions(gem_log_get_stream(),filtering_candidates,text_collection,false,false);
    tab_global_dec();
  }
  // Return number of accepted regions
  return num_accepted_regions;
}

