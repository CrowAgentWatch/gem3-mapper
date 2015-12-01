/*
 * PROJECT: GEMMapper
 * FILE: sampled_rl.c
 * DATE: 06/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include "sampled_rl.h"
#include "packed_integer_array.h"

/*
 * Sampled-RL Model & Version
 */
#define SAMPLED_RL_MODEL_NO  1000ull

/*
 * Loader/Setup
 */
sampled_rl_t* sampled_rl_new(
    const sampling_rate_t sampling_rate,const uint64_t num_samples,const uint64_t max_index) {
  // Allocate handler
  sampled_rl_t* const sampled_rl = mm_alloc(sampled_rl_t);
  sampled_rl->sampling_rate = sampling_rate;
  // Allocate sampled RL
  const uint64_t sample_length = integer_upper_power_of_two(max_index);
  sampled_rl->packed_integer_array = packed_integer_array_new(num_samples,sample_length);
  // Return
  return sampled_rl;
}
sampled_rl_t* sampled_rl_read_mem(mm_t* const memory_manager) {
  // Allocate handler
  sampled_rl_t* const sampled_rl = mm_alloc(sampled_rl_t);
  // Read Meta-Data
  const uint64_t sampled_rl_model_no = mm_read_uint64(memory_manager);
  gem_cond_fatal_error(sampled_rl_model_no!=SAMPLED_RL_MODEL_NO,SAMPLED_RL_WRONG_MODEL_NO,
      sampled_rl_model_no,(uint64_t)SAMPLED_RL_MODEL_NO);
  sampled_rl->sampling_rate = mm_read_uint64(memory_manager);
  // Read PackedIntegerArray
  sampled_rl->packed_integer_array = packed_integer_array_read_mem(memory_manager);
  // Return
  return sampled_rl;
}
void sampled_rl_write(fm_t* const file_manager,sampled_rl_t* const sampled_rl) {
  SAMPLED_RL_CHECK(sampled_rl);
  // Write Meta-Data
  fm_write_uint64(file_manager,SAMPLED_RL_MODEL_NO);
  fm_write_uint64(file_manager,sampled_rl->sampling_rate);
  // Write PackedIntegerArray
  packed_integer_array_write(file_manager,sampled_rl->packed_integer_array);
}
void sampled_rl_delete(sampled_rl_t* const sampled_rl) {
  SAMPLED_RL_CHECK(sampled_rl);
  // Free PackedIntegerArray
  packed_integer_array_delete(sampled_rl->packed_integer_array);
  // Free handler
  mm_free(sampled_rl);
}
/*
 * Accessors
 */
uint64_t sampled_rl_get_size(sampled_rl_t* const sampled_rl) {
  SAMPLED_RL_CHECK(sampled_rl);
  return packed_integer_array_get_size(sampled_rl->packed_integer_array);
}
void sampled_rl_sample(sampled_rl_t* const sampled_rl,const uint64_t array_position,const uint64_t rl_position) {
  SAMPLED_RL_CHECK(sampled_rl);
  // Store SA position
  packed_integer_array_store(sampled_rl->packed_integer_array,array_position,rl_position);
}
uint64_t sampled_rl_get_sample(sampled_rl_t* const sampled_rl,const uint64_t array_position) {
  SAMPLED_RL_CHECK(sampled_rl);
  // Store SA position
  return packed_integer_array_load(sampled_rl->packed_integer_array,array_position);
}
/*
 * Display/Stats
 */
void sampled_rl_print(FILE* const stream,sampled_rl_t* const sampled_rl) {
  SAMPLED_RL_CHECK(sampled_rl);
  tab_fprintf(stream,"[GEM]>Sampled-RL\n");
  tab_fprintf(stream,"  => Architecture sRL.pck\n");
  tab_fprintf(stream,"    => RunLength-Space RL \n");
  tab_fprintf(stream,"    => ArrayImpl       Packed-Array \n");
  tab_fprintf(stream,"  => SamplingRate 2^%"PRIu64"\n",(uint64_t)sampled_rl->sampling_rate);
  tab_fprintf(stream,"  => Size %"PRIu64" MB\n",CONVERT_B_TO_MB(sampled_rl_get_size(sampled_rl)));
  tab_global_inc();
  packed_integer_array_print(stream,sampled_rl->packed_integer_array,false);
  tab_global_dec();
  // Flush
  fflush(stream);
}

