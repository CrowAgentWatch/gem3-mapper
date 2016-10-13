/*
 *  GEM-Mapper v3 (GEM3)
 *  Copyright (c) 2011-2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 *  This file is part of GEM-Mapper v3 (GEM3).
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * PROJECT: GEM-Mapper v3 (GEM3)
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 *   Region-Profile module provides functions to generate an
 *   fixed/static key partition (with a fixed length size)
 */

#ifndef REGION_PROFILE_FIXED_H_
#define REGION_PROFILE_FIXED_H_

#include "utils/essentials.h"
#include "filtering/region_profile/region_profile.h"

/*
 * Region Profile Schedule (generate the region partition)
 */
void region_profile_generate_fixed_partition(
    region_profile_t* const region_profile,
    const uint8_t* const key,
    const uint64_t key_length,
    const bool* const allowed_enc,
    const uint64_t min_region_length);

/*
 * Region Profile Schedule (query the region partition into the index)
 */
void region_profile_generate_fixed_query(
    region_profile_t* const region_profile,
    fm_index_t* const fm_index,
    const uint8_t* const key);

/*
 * Display/Benchmark
 */
void region_profile_print_benchmark(
    FILE* const stream,
    const region_profile_t* const region_profile,
    fm_index_t* const fm_index,
    const uint8_t* key);

#endif /* REGION_PROFILE_FIXED_H_ */