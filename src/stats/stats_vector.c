/*
 * PROJECT: GEMMapper
 * FILE: stats_vector.h
 * DATE: 07/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: TODO
 */

#include "stats/stats_vector.h"

/*
 * Constructors
 */
stats_vector_t* stats_vector_customed_range_new(
    uint64_t* const restrict customed_range_values,
    const uint64_t num_ranges,
    const uint64_t out_of_range_bucket_size) {
  // Allocate handler
  stats_vector_t* const restrict stats_vector = mm_alloc(stats_vector_t);
  // Init
  stats_vector->type = STATS_VECTOR_CUSTOMED_RANGE;
  stats_vector->counters = mm_calloc(num_ranges,uint64_t,true);
  // Customed range vetor
  stats_vector->customed_range_values = customed_range_values;
  stats_vector->num_counters = num_ranges;
  stats_vector->max_index = num_ranges-1;
  stats_vector->max_value = customed_range_values[stats_vector->max_index];
  // Values out of range
  stats_vector->out_of_range_bucket_size = out_of_range_bucket_size;
  stats_vector->out_values = ihash_new();
  return stats_vector;
}
stats_vector_t* stats_vector_step_range_new(
    const uint64_t max_value,const uint64_t step,
    const uint64_t out_of_range_bucket_size) {
  const uint64_t range = max_value+1;
  const uint64_t num_values = (range+(step-1))/step;
  // Allocate handler
  stats_vector_t* const restrict stats_vector = mm_alloc(stats_vector_t);
  // Init
  stats_vector->type = STATS_VECTOR_STEP_RANGE;
  stats_vector->counters = mm_calloc(num_values,uint64_t,true);
  stats_vector->num_counters = num_values;
  stats_vector->max_index = num_values-1;
  stats_vector->max_value = num_values*step;
  // Step Range
  stats_vector->step = step;
  // Values out of range
  stats_vector->out_of_range_bucket_size = out_of_range_bucket_size;
  stats_vector->out_values = ihash_new();
  return stats_vector;
}
stats_vector_t* stats_vector_raw_new(
    const uint64_t num_values,
    const uint64_t out_of_range_bucket_size) {
  // Allocate handler
  stats_vector_t* const restrict stats_vector = mm_alloc(stats_vector_t);
  // Init
  stats_vector->type = STATS_VECTOR_RAW;
  stats_vector->counters = (num_values) ? mm_calloc(num_values,uint64_t,true) : NULL;
  // Raw
  stats_vector->num_counters = num_values;
  stats_vector->max_index = num_values-1;
  stats_vector->max_value = num_values;
  // Values out of range
  stats_vector->out_of_range_bucket_size = out_of_range_bucket_size;
  stats_vector->out_values = ihash_new();
  return stats_vector;
}
stats_vector_t* stats_vector_new_from_template(stats_vector_t* const restrict stats_vector_template) {
  // Allocate handler
  stats_vector_t* const restrict stats_vector = mm_alloc(stats_vector_t);
  // Copy template
  stats_vector->type = stats_vector_template->type;
  stats_vector->counters = (stats_vector_template->num_counters) ?
      mm_calloc(stats_vector_template->num_counters,uint64_t,true) : NULL;
  stats_vector->num_counters = stats_vector_template->num_counters;
  stats_vector->max_index = stats_vector_template->max_index;
  stats_vector->max_value = stats_vector_template->max_value;
  stats_vector->step = stats_vector_template->step;
  stats_vector->customed_range_values = stats_vector_template->customed_range_values;
  stats_vector->out_of_range_bucket_size = stats_vector_template->out_of_range_bucket_size;
  stats_vector->out_values = ihash_new();
  // Return
  return stats_vector;
}
void stats_vector_clear(stats_vector_t* const restrict stats_vector) {
  memset(stats_vector->counters,0,stats_vector->num_counters);
  ihash_clear(stats_vector->out_values);
}
void stats_vector_delete(stats_vector_t* const restrict stats_vector) {
  free(stats_vector->counters);
  ihash_delete(stats_vector->out_values);
  free(stats_vector);
}
/*
 * Index (value -> index)
 */
uint64_t stats_cvector_get_index(stats_vector_t* const restrict stats_vector,const uint64_t value) {
  uint64_t* const restrict range_values = stats_vector->customed_range_values;
  uint64_t lo = 0;
  uint64_t hi = stats_vector->max_index;
  if (value < range_values[lo] || range_values[hi] <= value) {
    return STATS_VECTOR_OUT_OF_RANGE;
  } else {
    while (hi-lo > 1) { // Search for the bucket
      const uint64_t mid = (hi+lo)/2;
      if (value < range_values[mid]) {
        hi = mid;
      } else {
        lo = mid;
      }
    }
    gem_cond_fatal_error(!(lo+1==hi && range_values[lo] <= value && value < range_values[hi]),ALG_INCONSISNTENCY);
    return lo;
  }
}
uint64_t stats_svector_get_index(stats_vector_t* const restrict stats_vector,const uint64_t value) {
  if (value >= stats_vector->max_value) {
    return STATS_VECTOR_OUT_OF_RANGE;
  } else {
    return value/stats_vector->step;
  }
}
uint64_t stats_rvector_get_index(stats_vector_t* const restrict stats_vector,const uint64_t value) {
  if (stats_vector->max_value <= value) {
    return STATS_VECTOR_OUT_OF_RANGE;
  } else {
    return value;
  }
}
uint64_t stats_hvector_get_index(stats_vector_t* const restrict stats_vector,const uint64_t value) {
  return (stats_vector->max_index+1) + (value-stats_vector->max_value)/stats_vector->out_of_range_bucket_size;
}
uint64_t stats_vector_get_index(stats_vector_t* const restrict stats_vector,const uint64_t value) {
  uint64_t bucket_index;
  switch (stats_vector->type) {
    case STATS_VECTOR_CUSTOMED_RANGE:
      bucket_index = stats_cvector_get_index(stats_vector,value);
      break;
    case STATS_VECTOR_STEP_RANGE:
      bucket_index = stats_svector_get_index(stats_vector,value);
      break;
    case STATS_VECTOR_RAW:
      bucket_index = stats_rvector_get_index(stats_vector,value);
      break;
    default:
      GEM_INVALID_CASE();
      break;
  }
  // Return index
  return (bucket_index==STATS_VECTOR_OUT_OF_RANGE) ? stats_hvector_get_index(stats_vector,value) : bucket_index;
}
/*
 * Vector's Buckets getters
 */
uint64_t* stats_hvector_get_counter(stats_vector_t* const restrict stats_vector,const uint64_t value) {
  const uint64_t bucket_index = stats_hvector_get_index(stats_vector,value);
  // Fetch counter
  uint64_t* counter = ihash_get_element(stats_vector->out_values,bucket_index);
  if (counter!=NULL) return counter;
  // Allocate new counter
  counter = mm_alloc(uint64_t);
  *counter = 0;
  ihash_insert(stats_vector->out_values,bucket_index,counter);
  return counter;
}
uint64_t* stats_vector_get_counter(stats_vector_t* const restrict stats_vector,const uint64_t value) {
  uint64_t bucket_index;
  switch (stats_vector->type) {
    case STATS_VECTOR_CUSTOMED_RANGE:
      bucket_index = stats_cvector_get_index(stats_vector,value);
      break;
    case STATS_VECTOR_STEP_RANGE:
      bucket_index = stats_svector_get_index(stats_vector,value);
      break;
    case STATS_VECTOR_RAW:
      bucket_index = stats_rvector_get_index(stats_vector,value);
      break;
    default:
      GEM_INVALID_CASE();
      break;
  }
  // Return counter
  if (bucket_index==STATS_VECTOR_OUT_OF_RANGE) {
    return stats_hvector_get_counter(stats_vector,value);
  } else {
    return stats_vector->counters+bucket_index;
  }
}
/*
 * Increment/Add bucket counter
 */
void stats_vector_inc(
    stats_vector_t* const restrict stats_vector,
    const uint64_t value) {
  ++(*stats_vector_get_counter(stats_vector,value));
}
void stats_vector_add(
    stats_vector_t* const restrict stats_vector,
    const uint64_t value,
    const uint64_t amount) {
  *stats_vector_get_counter(stats_vector,value) += amount;
}
/*
 * Bucket counters getters (Individual buckets & Accumulated ranges)
 */
uint64_t stats_vector_get_count(stats_vector_t* const restrict stats_vector,const uint64_t value) {
  return *stats_vector_get_counter(stats_vector,value);
}
/*
 * Bucket counters getters (Accumulated ranges)
 */
uint64_t stats_vector_get_accumulated_count(stats_vector_t* const restrict stats_vector) {
  stats_vector_iterator_t* const restrict iterator = stats_vector_iterator_new(stats_vector);
  uint64_t acc_count = 0;
  while (!stats_vector_iterator_eoi(iterator)) {
    acc_count += stats_vector_iterator_get_count(iterator); // Add current counter
    stats_vector_iterator_next(iterator);  // Next
  }
  stats_vector_iterator_delete(iterator);
  return acc_count;
}
uint64_t stats_vector_get_range_accumulated_count(
    stats_vector_t* const restrict stats_vector,const uint64_t value_from,const uint64_t value_to) {
  // TODO
  return 0;
}
/*
 * Inverse Index (index -> value)
 *   Given the stats_vector index returns the corresponding value/range.
 */
void stats_vector_get_value_range(
    stats_vector_t* const restrict stats_vector,
    const uint64_t index,
    uint64_t* const restrict lo_value,
    uint64_t* const restrict hi_value) {
  if (gem_expect_true(index <= stats_vector->max_index)) {
    switch (stats_vector->type) {
      case STATS_VECTOR_CUSTOMED_RANGE:
        *lo_value = stats_vector->customed_range_values[index];
        *hi_value = stats_vector->customed_range_values[index+1];
        break;
      case STATS_VECTOR_STEP_RANGE:
        *lo_value = index*stats_vector->step;
        *hi_value = *lo_value + stats_vector->step;
        break;
      case STATS_VECTOR_RAW:
        *lo_value = index;
        *hi_value = index+1;
        break;
      default:
        GEM_INVALID_CASE();
        break;
    }
  } else {
    *lo_value = stats_vector->max_value + (index-stats_vector->max_index-1)*stats_vector->out_of_range_bucket_size;
    *hi_value = *lo_value + stats_vector->out_of_range_bucket_size;
  }
}
/*
 * Merge 2 stats-vector (adding bucket counting)
 */
void stats_vector_merge(stats_vector_t* const restrict stats_dst,stats_vector_t* const restrict stats_src) {
  // TODO
}
/*
 * Display (Printers)
 */
void stats_vector_display(
    FILE* const restrict stream,
    stats_vector_t* const restrict stats_vector,
    const bool display_zeros,
    const bool display_percentage,
    void (*print_label)(uint64_t)) {
  stats_vector_iterator_t* const restrict iterator = stats_vector_iterator_new(stats_vector);
  const uint64_t sum_values = (display_percentage) ? stats_vector_get_accumulated_count(stats_vector) : 0;
  while (!stats_vector_iterator_eoi(iterator)) {
    // Get current counter
    uint64_t lo_range, hi_range;
    const uint64_t counter = stats_vector_iterator_get_count(iterator);
    if (counter > 0 || display_zeros) {
      if (print_label==NULL) {
        // Print [range] => counter
        stats_vector_iterator_get_range(iterator,&lo_range,&hi_range);
        if (hi_range-lo_range>1) {
          if (display_percentage) {
            tab_fprintf(stream,"[%"PRIu64",%"PRIu64") => %"PRIu64" (%2.3f%%)\n",
                lo_range,hi_range,counter,PERCENTAGE(counter,sum_values));
          } else {
            tab_fprintf(stream,"[%"PRIu64",%"PRIu64") => %"PRIu64"\n",
                lo_range,hi_range,counter);
          }
        } else {
          if (display_percentage) {
            tab_fprintf(stream,"[%"PRIu64"] => %"PRIu64" (%2.3f%%)\n",
                lo_range,counter,PERCENTAGE(counter,sum_values));
          } else {
            tab_fprintf(stream,"[%"PRIu64"] => %"PRIu64"\n",lo_range,counter);
          }
        }
      } else {
        print_label(stats_vector_iterator_get_index(iterator)); // Print label
        if (display_percentage) {
          tab_fprintf(stream," => %"PRIu64" (%2.3f%%)\n",counter,PERCENTAGE(counter,sum_values)); // Print counter
        } else {
          tab_fprintf(stream," => %"PRIu64"\n",counter); // Print counter
        }
      }
    }
    // Next
    stats_vector_iterator_next(iterator);
  }
  stats_vector_iterator_delete(iterator);
}
void stats_vector_print_ranges(FILE* const restrict stream,stats_vector_t* const restrict stats_vector) {
  stats_vector_iterator_t* const restrict iterator = stats_vector_iterator_new(stats_vector);
  while (!stats_vector_iterator_eoi(iterator)) {
    // Print Ranges
    uint64_t lo_range, hi_range;
    stats_vector_iterator_get_range(iterator,&lo_range,&hi_range);
    if (hi_range-lo_range>1) {
      fprintf(stream,"[%"PRIu64",%"PRIu64")\t",lo_range,hi_range);
    } else {
      fprintf(stream,"[%"PRIu64"]\t",lo_range);
    }
    // Next
    stats_vector_iterator_next(iterator);
  }
  stats_vector_iterator_delete(iterator);
}
void stats_vector_print_values(
    FILE* const restrict stream,
    stats_vector_t* const restrict stats_vector,
    const bool display_percentage) {
  stats_vector_iterator_t* const restrict iterator = stats_vector_iterator_new(stats_vector);
  const uint64_t sum_values = (display_percentage) ? stats_vector_get_accumulated_count(stats_vector) : 0;
  while (!stats_vector_iterator_eoi(iterator)) {
    // Get current counter
    const uint64_t counter = stats_vector_iterator_get_count(iterator);
    if (display_percentage) {
      fprintf(stream,"%"PRIu64"/%2.3f%%\t",counter,PERCENTAGE(counter,sum_values)); // Print counter
    } else {
      fprintf(stream,"%"PRIu64"\t",counter); // Print counter
    }
    // Next
    stats_vector_iterator_next(iterator);
  }
  stats_vector_iterator_delete(iterator);
}
/*
 * Iterator
 */
void stats_vector_iterator_set_eoi(stats_vector_iterator_t* const restrict sv_iterator) {
  if (sv_iterator->index > sv_iterator->last_index) {
    sv_iterator->eoi = true;
  } else {
    if (sv_iterator->index < sv_iterator->stats_vector->num_counters) { // Counters iteration
      sv_iterator->eoi = false;
    } else { // Hash iteration
      sv_iterator->eoi = ihash_iterator_eoi(sv_iterator->ihash_iterator);
    }
  }
}
stats_vector_iterator_t* stats_vector_iterator_new(stats_vector_t* const restrict stats_vector) {
  // Allocate
  stats_vector_iterator_t* const restrict sv_iterator = mm_alloc(stats_vector_iterator_t);
  // Init
  sv_iterator->stats_vector = stats_vector;
  sv_iterator->index = 0;
  sv_iterator->last_index = UINT64_MAX;
  // Hash iteration
  ihash_sort_by_key(stats_vector->out_values);
  sv_iterator->ihash_iterator = ihash_iterator_new(stats_vector->out_values);
  // Set eoi
  stats_vector_iterator_set_eoi(sv_iterator);
  // Ret
  return sv_iterator;
}
stats_vector_iterator_t* stats_vector_iterator_range_new(
    stats_vector_t* const restrict stats_vector,
    const uint64_t value_from,
    const uint64_t value_to) {
  // Allocate
  stats_vector_iterator_t* const restrict sv_iterator = mm_alloc(stats_vector_iterator_t);
  // Init
  sv_iterator->stats_vector = stats_vector;
  sv_iterator->index = stats_vector_get_index(stats_vector,value_from);
  sv_iterator->last_index = stats_vector_get_index(stats_vector,value_to);
  // Hash iteration
  ihash_sort_by_key(stats_vector->out_values);
  sv_iterator->ihash_iterator = ihash_iterator_new(stats_vector->out_values);
  // Set eoi
  stats_vector_iterator_set_eoi(sv_iterator);
  // Ret
  return sv_iterator;
}
void stats_vector_iterator_delete(stats_vector_iterator_t* const restrict sv_iterator) {
  ihash_iterator_delete(sv_iterator->ihash_iterator);
  mm_free(sv_iterator);
}
bool stats_vector_iterator_eoi(stats_vector_iterator_t* const restrict sv_iterator) {
  return sv_iterator->eoi;
}
void stats_vector_iterator_next(stats_vector_iterator_t* const restrict sv_iterator) {
  if (gem_expect_true(!stats_vector_iterator_eoi(sv_iterator))) {
    if (sv_iterator->index < sv_iterator->stats_vector->num_counters-1) {
      // Regular counters iteration
      ++(sv_iterator->index);
      if (sv_iterator->index > sv_iterator->last_index) sv_iterator->eoi = true;
    } else { // (sv_iterator->index >= sv_iterator->stats_vector->num_counters-1)
      if (ihash_iterator_next(sv_iterator->ihash_iterator)) {
        sv_iterator->eoi = false;
        // Set new index
        sv_iterator->index = ihash_iterator_get_key(sv_iterator->ihash_iterator);
        if (sv_iterator->index > sv_iterator->last_index) sv_iterator->eoi = true;
      } else {
        sv_iterator->eoi = true;
      }
    }
  }
}
uint64_t stats_vector_iterator_get_index(stats_vector_iterator_t* const restrict sv_iterator) {
  return sv_iterator->index;
}
uint64_t stats_vector_iterator_get_count(stats_vector_iterator_t* const restrict sv_iterator) {
  if (sv_iterator->index < sv_iterator->stats_vector->num_counters) { // Counters iteration
    return sv_iterator->stats_vector->counters[sv_iterator->index];
  } else { // Hash iteration
    return *((uint64_t*)ihash_iterator_get_element(sv_iterator->ihash_iterator));
  }
}
void stats_vector_iterator_get_range(
    stats_vector_iterator_t* const restrict sv_iterator,
    uint64_t* const restrict lo_value,
    uint64_t* const restrict hi_value) {
  stats_vector_get_value_range(sv_iterator->stats_vector,sv_iterator->index,lo_value,hi_value);
}

