/*
 * PROJECT: GEMMapper
 * FILE: sampled_sa.c
 * DATE: 06/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include "sampled_sa.h"
#include "packed_integer_array.h"

/*
 * Sampled-SA Model & Version
 */
#define SAMPLED_SA_MODEL_NO  1002ul

/*
 * Checkers
 */
#define SAMPLED_SA_CHECK(sampled_sa) GEM_CHECK_NULL(sampled_sa)

/*
 * Loader/Setup
 */
sampled_sa_t* sampled_sa_read_mem(mm_t* const memory_manager) {
  // Allocate handler
  sampled_sa_t* const sampled_sa = mm_alloc(sampled_sa_t);
  // Read Meta-Data
  const uint64_t sampled_sa_model_no = mm_read_uint64(memory_manager);
  gem_cond_fatal_error(sampled_sa_model_no!=SAMPLED_SA_MODEL_NO,SAMPLED_SA_WRONG_MODEL_NO,
      sampled_sa_model_no,(uint64_t)SAMPLED_SA_MODEL_NO);
  sampled_sa->index_length = mm_read_uint64(memory_manager);
  sampled_sa->sa_sampling_rate = mm_read_uint64(memory_manager);
  sampled_sa->text_sampling_rate = mm_read_uint64(memory_manager);
  // Read PackedIntegerArray
  sampled_sa->packed_integer_array = packed_integer_array_read_mem(memory_manager);
  // Return
  return sampled_sa;
}
void sampled_sa_write(fm_t* const file_manager,sampled_sa_t* const sampled_sa) {
  SAMPLED_SA_CHECK(sampled_sa);
  // Write Meta-Data
  fm_write_uint64(file_manager,SAMPLED_SA_MODEL_NO);
  fm_write_uint64(file_manager,sampled_sa->index_length);
  fm_write_uint64(file_manager,sampled_sa->sa_sampling_rate);
  fm_write_uint64(file_manager,sampled_sa->text_sampling_rate);
  // Write PackedIntegerArray
  packed_integer_array_write(file_manager,sampled_sa->packed_integer_array);
}
void sampled_sa_delete(sampled_sa_t* const sampled_sa) {
  SAMPLED_SA_CHECK(sampled_sa);
  // Free PackedIntegerArray
  packed_integer_array_delete(sampled_sa->packed_integer_array);
  // Free handler
  mm_free(sampled_sa);
}
/*
 * Builder
 */
sampled_sa_builder_t* sampled_sa_builder_new(
    const uint64_t index_length,const uint64_t num_builders,
    const sampling_rate_t sa_sampling_rate,const sampling_rate_t text_sampling_rate,
    mm_slab_t* const mm_slab) {
  // Allocate handler
  sampled_sa_builder_t* const sampled_sa = mm_alloc(sampled_sa_builder_t);
  // Init meta-data
  sampled_sa->index_length = index_length;
  sampled_sa->sa_sampling_rate = sa_sampling_rate;
  sampled_sa->text_sampling_rate = text_sampling_rate;
  // Allocate Samples
  const uint64_t sample_length = integer_upper_power_of_two(index_length);
  sampled_sa->num_chunks = num_builders;
  sampled_sa->array_builder = mm_calloc(num_builders,packed_integer_array_builder_t*,true);
  uint64_t i;
  for (i=0;i<num_builders;++i) {
    sampled_sa->array_builder[i] = packed_integer_array_builder_new(sample_length,mm_slab);
  }
  // Allocate Sampled-Positions Bitmap
  MUTEX_INIT(sampled_sa->sampled_bitmap_mutex);
  sampled_sa->sampled_bitmap_size = DIV_CEIL(index_length,UINT64_LENGTH);
  sampled_sa->sampled_bitmap_mem = (uint64_t*)mm_calloc(sampled_sa->sampled_bitmap_size,uint64_t,true);
  // Return
  return sampled_sa;
}
void sampled_sa_builder_delete_samples(sampled_sa_builder_t* const sampled_sa) {
  uint64_t i;
  for (i=0;i<sampled_sa->num_chunks;++i) {
    packed_integer_array_builder_delete(sampled_sa->array_builder[i]);
  }
  mm_free(sampled_sa->array_builder);
}
void sampled_sa_builder_delete(sampled_sa_builder_t* const sampled_sa) {
  mm_free(sampled_sa->sampled_bitmap_mem);
  MUTEX_DESTROY(sampled_sa->sampled_bitmap_mutex);
  mm_free(sampled_sa);
}
void sampled_sa_builder_set_sample(
    sampled_sa_builder_t* const sampled_sa,const uint64_t chunk_number,
    const uint64_t array_position,const uint64_t sa_value) {
  // Store sample
  packed_integer_array_builder_store(sampled_sa->array_builder[chunk_number],sa_value);
  // Mark Sample
  const uint64_t block_pos = array_position / UINT64_LENGTH;
  const uint64_t block_mod = array_position % UINT64_LENGTH;
  const uint64_t mask = UINT64_ONE_MASK<<block_mod;
  MUTEX_BEGIN_SECTION(sampled_sa->sampled_bitmap_mutex) {
    sampled_sa->sampled_bitmap_mem[block_pos] |= mask;
  } MUTEX_END_SECTION(sampled_sa->sampled_bitmap_mutex);
}
void sampled_sa_builder_write(fm_t* const file_manager,sampled_sa_builder_t* const sampled_sa) {
  // Write Meta-Data
  fm_write_uint64(file_manager,SAMPLED_SA_MODEL_NO);
  fm_write_uint64(file_manager,sampled_sa->index_length);
  fm_write_uint64(file_manager,sampled_sa->sa_sampling_rate);
  fm_write_uint64(file_manager,sampled_sa->text_sampling_rate);
  // Write PackedIntegerArray
  packed_integer_array_builder_write(file_manager,sampled_sa->array_builder,sampled_sa->num_chunks);
}
uint64_t sampled_sa_builder_get_sa_sampling_rate(const sampled_sa_builder_t* const sampled_sa) {
  return (1<<sampled_sa->sa_sampling_rate);
}
uint64_t sampled_sa_builder_get_text_sampling_rate(const sampled_sa_builder_t* const sampled_sa) {
  return (1<<sampled_sa->text_sampling_rate);
}
uint64_t* sampled_sa_builder_get_sampled_bitmap(const sampled_sa_builder_t* const sampled_sa) {
  return sampled_sa->sampled_bitmap_mem;
}
/*
 * Accessors
 */
uint64_t sampled_sa_get_size(const sampled_sa_t* const sampled_sa) {
  SAMPLED_SA_CHECK(sampled_sa);
  return packed_integer_array_get_size(sampled_sa->packed_integer_array);
}
uint64_t sampled_sa_get_sa_sampling_rate(const sampled_sa_t* const sampled_sa) {
  SAMPLED_SA_CHECK(sampled_sa);
  return (1<<sampled_sa->sa_sampling_rate);
}
uint64_t sampled_sa_get_text_sampling_rate(const sampled_sa_t* const sampled_sa) {
  SAMPLED_SA_CHECK(sampled_sa);
  return (1<<sampled_sa->text_sampling_rate);
}
void sampled_sa_prefetch_sample(const sampled_sa_t* const sampled_sa,const uint64_t array_position) {
  SAMPLED_SA_CHECK(sampled_sa);
  packed_integer_array_prefetch(sampled_sa->packed_integer_array,array_position); // Fetch SA position
}
uint64_t sampled_sa_get_sample(const sampled_sa_t* const sampled_sa,const uint64_t array_position) {
  SAMPLED_SA_CHECK(sampled_sa);
  return packed_integer_array_load(sampled_sa->packed_integer_array,array_position); // Get SA position
}
void sampled_sa_set_sample(sampled_sa_t* const sampled_sa,const uint64_t array_position,const uint64_t sa_value) {
  SAMPLED_SA_CHECK(sampled_sa);
  packed_integer_array_store(sampled_sa->packed_integer_array,array_position,sa_value); // Store SA position
}
/*
 * Display/Stats
 */
void sampled_sa_print_(
    FILE* const stream,
    const uint64_t sa_sampling_rate,const uint64_t sa_sampling_rate_value,
    const uint64_t text_sampling_rate,const uint64_t text_sampling_rate_value,
    const uint64_t index_length,const uint64_t index_size) {
  tab_fprintf(stream,"[GEM]>Sampled-SA\n");
  tab_fprintf(stream,"  => Architecture sSA.pck\n");
  tab_fprintf(stream,"    => ArrayImpl  Packed-Array \n");
  tab_fprintf(stream,"    => Sampling.Hybrid (SA-Space & Text-Space)\n");
  tab_fprintf(stream,"      => SA.Sampling.Rate 2^%"PRIu64" (%"PRIu64")\n",sa_sampling_rate,sa_sampling_rate_value);
  tab_fprintf(stream,"      => Text.Sampling.Rate 2^%"PRIu64" (%"PRIu64")\n",text_sampling_rate,text_sampling_rate_value);
  if (index_size) {
    tab_fprintf(stream,"  => Length %"PRIu64"\n",index_length);
  }
  tab_fprintf(stream,"  => Size %"PRIu64" MB\n",index_size);
  // Flush
  fflush(stream);
}
void sampled_sa_print(FILE* const stream,sampled_sa_t* const sampled_sa,const bool display_data) {
  SAMPLED_SA_CHECK(sampled_sa);
  // Print Sampled-SA
  sampled_sa_print_(stream,
      sampled_sa->sa_sampling_rate,sampled_sa_get_sa_sampling_rate(sampled_sa),
      sampled_sa->text_sampling_rate,sampled_sa_get_text_sampling_rate(sampled_sa),
      sampled_sa->index_length,CONVERT_B_TO_MB(sampled_sa_get_size(sampled_sa)));
  // Print Integer-Array
  tab_global_inc();
  packed_integer_array_print(stream,sampled_sa->packed_integer_array,false);
  tab_global_dec();
  // Flush
  fflush(stream);
}
void sampled_sa_builder_print(FILE* const stream,sampled_sa_builder_t* const sampled_sa) {
  SAMPLED_SA_CHECK(sampled_sa);
  // Print Sampled-SA
  sampled_sa_print_(stream,
      sampled_sa->sa_sampling_rate,sampled_sa_builder_get_sa_sampling_rate(sampled_sa),
      sampled_sa->text_sampling_rate,sampled_sa_builder_get_text_sampling_rate(sampled_sa),
      sampled_sa->index_length,0);
  // Flush
  fflush(stream);
}

