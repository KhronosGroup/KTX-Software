// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2011-2021 Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// ----------------------------------------------------------------------------

#if !defined(ASTCENC_DECOMPRESS_ONLY)

/**
 * @brief Functions for finding best partition for a block.
 *
 * Major step 1:
 *
 * - Find best partitioning assuming uncorrelated colors.
 * - Find best partitioning assuming RGBS color representation.
 *
 * Finding best partitioning for a block:
 *
 * For each available partitioning compute mean color and dominant direction.
 * This defines two lines, both of which go through the mean-color-value.
 *
 * - One line has a direction defined by the dominant direction; this is used
 *   to assess the error from using an uncorrelated color representation.
 * - The other line goes through (0,0,0,1) and is used to assess the error from
 *   using an RGBS color representation.
 *
 * Then compute the block's squared-errors that result from using the these
 * two lines for endpoint selection.
 */

#include "astcenc_internal.h"

/* See header for documentation. */
void find_best_partitionings(
	const block_size_descriptor& bsd,
	const imageblock& blk,
	const error_weight_block& ewb,
	int partition_count,
	int partition_search_limit,
	int& best_partition_uncor,
	int& best_partition_samec,
	int* best_partition_dualplane
) {
	// constant used to estimate quantization error for a given partitioning;
	// the optimal value for this constant depends on bitrate.
	// These constants have been determined empirically.
	int texels_per_block = bsd.texel_count;
	float weight_imprecision_estim = 0.055f;
	if (texels_per_block <= 20)
	{
		weight_imprecision_estim = 0.03f;
	}
	else if (texels_per_block <= 31)
	{
		weight_imprecision_estim = 0.04f;
	}
	else if (texels_per_block <= 41)
	{
		weight_imprecision_estim = 0.05f;
	}

	promise(partition_count > 0);
	promise(partition_search_limit > 0);

	weight_imprecision_estim = weight_imprecision_estim * weight_imprecision_estim;

	int partition_sequence[PARTITION_COUNT];

	kmeans_compute_partition_ordering(bsd, blk, partition_count, partition_sequence);

	int uses_alpha = imageblock_uses_alpha(&blk);

	const partition_info* ptab = get_partition_table(&bsd, partition_count);

	// Partitioning errors assuming uncorrelated-chrominance endpoints
	float uncor_best_error { ERROR_CALC_DEFAULT };
	int uncor_best_partition { 0 };

	// Partitioning errors assuming same-chrominance endpoints
	// Store two so we can always return one different to uncorr
	float samec_best_errors[2] { ERROR_CALC_DEFAULT, ERROR_CALC_DEFAULT };
	int samec_best_partitions[2] { 0, 0 };

	// Partitioning errors assuming that one color component is uncorrelated
	float sep_best_error { ERROR_CALC_DEFAULT };
	int sep_best_partition { 0 };
	int sep_best_component { 0 };

	bool skip_two_plane = best_partition_dualplane == nullptr;

	if (uses_alpha)
	{
		for (int i = 0; i < partition_search_limit; i++)
		{
			int partition = partition_sequence[i];

			int bk_partition_count = ptab[partition].partition_count;
			if (bk_partition_count < partition_count)
			{
				break;
			}

			// Compute weighting to give to each component in each partition
			partition_metrics pms[4];

			compute_avgs_and_dirs_4_comp(*(ptab + partition), blk, ewb, pms);

			line4 uncor_lines[4];
			line4 samec_lines[4];
			line3 sep_r_lines[4];
			line3 sep_g_lines[4];
			line3 sep_b_lines[4];
			line3 sep_a_lines[4];

			processed_line4 uncor_plines[4];
			processed_line4 samec_plines[4];

			float uncor_line_lens[4];
			float samec_line_lens[4];

			for (int j = 0; j < partition_count; j++)
			{
				partition_metrics& pm = pms[j];

				uncor_lines[j].a = pm.avg;
				uncor_lines[j].b = normalize_safe(pm.dir, unit4());

				uncor_plines[j].amod = (uncor_lines[j].a - uncor_lines[j].b * dot(uncor_lines[j].a, uncor_lines[j].b)) * pm.icolor_scale;
				uncor_plines[j].bs   = uncor_lines[j].b * pm.color_scale;
				uncor_plines[j].bis  = uncor_lines[j].b * pm.icolor_scale;

				samec_lines[j].a = vfloat4::zero();
				samec_lines[j].b = normalize_safe(pm.avg, unit4());

				samec_plines[j].amod = vfloat4::zero();
				samec_plines[j].bs   = samec_lines[j].b * pm.color_scale;
				samec_plines[j].bis  = samec_lines[j].b * pm.icolor_scale;

				if (!skip_two_plane)
				{
					sep_r_lines[j].a = pm.avg.swz<1, 2, 3>();
					vfloat4 dirs_gba = pm.dir.swz<1, 2, 3>();
					sep_r_lines[j].b = normalize_safe(dirs_gba, unit3());

					sep_g_lines[j].a = pm.avg.swz<0, 2, 3>();
					vfloat4 dirs_rba = pm.dir.swz<0, 2, 3>();
					sep_g_lines[j].b = normalize_safe(dirs_rba, unit3());

					sep_b_lines[j].a = pm.avg.swz<0, 1, 3>();
					vfloat4 dirs_rga = pm.dir.swz<0, 1, 3>();
					sep_b_lines[j].b = normalize_safe(dirs_rga, unit3());

					sep_a_lines[j].a = pm.avg.swz<0, 1, 2>();
					vfloat4 dirs_rgb = pm.dir.swz<0, 1, 2>();
					sep_a_lines[j].b = normalize_safe(dirs_rgb, unit3());
				}
			}

			float uncor_error = 0.0f;
			float samec_error = 0.0f;
			vfloat4 sep_error = vfloat4::zero();

			compute_error_squared_rgba(*(ptab + partition),
			                           blk,
			                           ewb,
			                           uncor_plines,
			                           samec_plines,
			                           uncor_line_lens,
			                           samec_line_lens,
			                           uncor_error,
			                           samec_error);

			/*
			   Compute an estimate of error introduced by weight quantization imprecision.
			   This error is computed as follows, for each partition
			   1: compute the principal-axis vector (full length) in error-space
			   2: convert the principal-axis vector to regular RGB-space
			   3: scale the vector by a constant that estimates average quantization error
			   4: for each texel, square the vector, then do a dot-product with the texel's error weight;
			      sum up the results across all texels.
			   4(optimized): square the vector once, then do a dot-product with the average texel error,
			      then multiply by the number of texels.
			 */

			for (int j = 0; j < partition_count; j++)
			{
				partition_metrics& pm = pms[j];
				float tpp = (float)(ptab[partition].partition_texel_count[j]);

				vfloat4 ics = pm.icolor_scale;
				vfloat4 error_weights = pm.error_weight * (tpp * weight_imprecision_estim);

				vfloat4 uncor_vector = uncor_lines[j].b * uncor_line_lens[j] * ics;
				vfloat4 samec_vector = samec_lines[j].b * samec_line_lens[j] * ics;

				uncor_vector = uncor_vector * uncor_vector;
				samec_vector = samec_vector * samec_vector;

				uncor_error += dot_s(uncor_vector, error_weights);
				samec_error += dot_s(samec_vector, error_weights);

				if (!skip_two_plane)
				{
					vfloat4 sep_r_vector = sep_r_lines[j].b * ics.swz<1, 2, 3, 0>();
					vfloat4 sep_g_vector = sep_g_lines[j].b * ics.swz<0, 2, 3, 1>();
					vfloat4 sep_b_vector = sep_b_lines[j].b * ics.swz<0, 1, 3, 2>();
					vfloat4 sep_a_vector = sep_a_lines[j].b * ics.swz<0, 1, 2, 3>();

					sep_r_vector = sep_r_vector * sep_r_vector;
					sep_g_vector = sep_g_vector * sep_g_vector;
					sep_b_vector = sep_b_vector * sep_b_vector;
					sep_a_vector = sep_a_vector * sep_a_vector;

					vfloat4 sep_err_inc(dot3_s(sep_r_vector, error_weights.swz<1, 2, 3, 0>()),
										dot3_s(sep_g_vector, error_weights.swz<0, 2, 3, 1>()),
										dot3_s(sep_b_vector, error_weights.swz<0, 1, 3, 2>()),
										dot3_s(sep_a_vector, error_weights.swz<0, 1, 2, 3>()));

					sep_error = sep_error + sep_err_inc + pm.range_sq * error_weights;
				}
			}

			if (uncor_error < uncor_best_error)
			{
				uncor_best_error = uncor_error;
				uncor_best_partition = partition;
			}

			if (samec_error < samec_best_errors[0])
			{
				samec_best_errors[1] = samec_best_errors[0];
				samec_best_partitions[1] = samec_best_partitions[0];

				samec_best_errors[0] = samec_error;
				samec_best_partitions[0] = partition;
			}
			else if (samec_error < samec_best_errors[1])
			{
				samec_best_errors[1] = samec_error;
				samec_best_partitions[1] = partition;
			}

			if (!skip_two_plane)
			{
				if (sep_error.lane<0>() < sep_best_error)
				{
					sep_best_error = sep_error.lane<0>();
					sep_best_partition = partition;
					sep_best_component = 0;
				}

				if (sep_error.lane<1>() < sep_best_error)
				{
					sep_best_error = sep_error.lane<1>();
					sep_best_partition = partition;
					sep_best_component = 1;
				}

				if (sep_error.lane<2>() < sep_best_error)
				{
					sep_best_error = sep_error.lane<2>();
					sep_best_partition = partition;
					sep_best_component = 2;
				}

				if (sep_error.lane<3>() < sep_best_error)
				{
					sep_best_error = sep_error.lane<3>();
					sep_best_partition = partition;
					sep_best_component = 3;
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < partition_search_limit; i++)
		{
			int partition = partition_sequence[i];

			int bk_partition_count = ptab[partition].partition_count;
			if (bk_partition_count < partition_count)
			{
				break;
			}

			// Compute weighting to give to each component in each partition
			partition_metrics pms[4];
			compute_avgs_and_dirs_3_comp(*(ptab + partition), blk, ewb, 3, pms);

			partition_lines3 plines[4];

			line2 sep_r_lines[4];
			line2 sep_g_lines[4];
			line2 sep_b_lines[4];

			for (int j = 0; j < partition_count; j++)
			{
				partition_metrics& pm = pms[j];
				partition_lines3& pl = plines[j];

				pl.uncor_line.a = pm.avg;
				pl.uncor_line.b = normalize_safe(pm.dir.swz<0, 1, 2>(), unit3());

				pl.samec_line.a = vfloat4::zero();
				pl.samec_line.b = normalize_safe(pm.avg.swz<0, 1, 2>(), unit3());

				pl.uncor_pline.amod = (pl.uncor_line.a - pl.uncor_line.b * dot3(pl.uncor_line.a, pl.uncor_line.b)) * pm.icolor_scale.swz<0, 1, 2, 3>();
				pl.uncor_pline.bs   = (pl.uncor_line.b * pm.color_scale.swz<0, 1, 2, 3>());
				pl.uncor_pline.bis  = (pl.uncor_line.b * pm.icolor_scale.swz<0, 1, 2, 3>());

				pl.samec_pline.amod = vfloat4::zero();
				pl.samec_pline.bs   = (pl.samec_line.b * pm.color_scale.swz<0, 1, 2, 3>());
				pl.samec_pline.bis  = (pl.samec_line.b * pm.icolor_scale.swz<0, 1, 2, 3>());

				if (!skip_two_plane)
				{
					sep_r_lines[j].a = pm.avg.swz<1, 2>();
					vfloat4 dirs_gb = pm.dir.swz<1, 2>();
					sep_r_lines[j].b = normalize_safe(dirs_gb, unit2());

					sep_g_lines[j].a = pm.avg.swz<0, 2>();
					vfloat4 dirs_rb = pm.dir.swz<0, 2>();
					sep_g_lines[j].b = normalize_safe(dirs_rb, unit2());

					sep_b_lines[j].a = pm.avg.swz<0, 1>();
					vfloat4 dirs_rg = pm.dir.swz<0, 1>();
					sep_b_lines[j].b = normalize_safe(dirs_rg, unit2());
				}
			}

			float uncor_error = 0.0f;
			float samec_error = 0.0f;
			vfloat4 sep_error = vfloat4::zero();

			compute_error_squared_rgb(*(ptab + partition),
			                          blk,
			                          ewb,
			                          plines,
			                          uncor_error,
			                          samec_error);

			/*
			   compute an estimate of error introduced by weight imprecision.
			   This error is computed as follows, for each partition
			   1: compute the principal-axis vector (full length) in error-space
			   2: convert the principal-axis vector to regular RGB-space
			   3: scale the vector by a constant that estimates average quantization error.
			   4: for each texel, square the vector, then do a dot-product with the texel's error weight;
			      sum up the results across all texels.
			   4(optimized): square the vector once, then do a dot-product with the average texel error,
			      then multiply by the number of texels.
			 */

			for (int j = 0; j < partition_count; j++)
			{
				partition_metrics& pm = pms[j];
				partition_lines3& pl = plines[j];

				float tpp = (float)(ptab[partition].partition_texel_count[j]);

				vfloat4 ics = pm.icolor_scale;
				ics.set_lane<3>(0.0f);

				vfloat4 error_weights = pm.error_weight * (tpp * weight_imprecision_estim);
				error_weights.set_lane<3>(0.0f);

				vfloat4 uncor_vector = (pl.uncor_line.b * pl.uncor_line_len) * ics;
				vfloat4 samec_vector = (pl.samec_line.b * pl.samec_line_len) * ics;

				uncor_vector = uncor_vector * uncor_vector;
				samec_vector = samec_vector * samec_vector;

				uncor_error += dot3_s(uncor_vector, error_weights);
				samec_error += dot3_s(samec_vector, error_weights);

				if (!skip_two_plane)
				{
					vfloat4 sep_r_vector = sep_r_lines[j].b * ics.swz<1, 2>();
					vfloat4 sep_g_vector = sep_g_lines[j].b * ics.swz<0, 2>();
					vfloat4 sep_b_vector = sep_b_lines[j].b * ics.swz<0, 1>();

					sep_r_vector = sep_r_vector * sep_r_vector;
					sep_g_vector = sep_g_vector * sep_g_vector;
					sep_b_vector = sep_b_vector * sep_b_vector;

					sep_error.set_lane<0>(sep_error.lane<0>() + dot_s(sep_r_vector, error_weights.swz<1, 2>()));
					sep_error.set_lane<1>(sep_error.lane<1>() + dot_s(sep_g_vector, error_weights.swz<0, 2>()));
					sep_error.set_lane<2>(sep_error.lane<2>() + dot_s(sep_b_vector, error_weights.swz<0, 1>()));

					sep_error.set_lane<0>(sep_error.lane<0>() + pm.range_sq.lane<0>() * error_weights.lane<0>());
					sep_error.set_lane<1>(sep_error.lane<1>() + pm.range_sq.lane<1>() * error_weights.lane<1>());
					sep_error.set_lane<2>(sep_error.lane<2>() + pm.range_sq.lane<2>() * error_weights.lane<2>());
				}
			}

			if (uncor_error < uncor_best_error)
			{
				uncor_best_error = uncor_error;
				uncor_best_partition = partition;
			}

			if (samec_error < samec_best_errors[0])
			{
				samec_best_errors[1] = samec_best_errors[0];
				samec_best_partitions[1] = samec_best_partitions[0];

				samec_best_errors[0] = samec_error;
				samec_best_partitions[0] = partition;
			}
			else if (samec_error < samec_best_errors[1])
			{
				samec_best_errors[1] = samec_error;
				samec_best_partitions[1] = partition;
			}

			if (!skip_two_plane)
			{
				if (sep_error.lane<0>() < sep_best_error)
				{
					sep_best_error = sep_error.lane<0>();
					sep_best_partition = partition;
					sep_best_component = 0;
				}

				if (sep_error.lane<1>() < sep_best_error)
				{
					sep_best_error = sep_error.lane<1>();
					sep_best_partition = partition;
					sep_best_component = 1;
				}

				if (sep_error.lane<2>() < sep_best_error)
				{
					sep_best_error = sep_error.lane<2>();
					sep_best_partition = partition;
					sep_best_component = 2;
				}
			}
		}
	}

	best_partition_uncor = uncor_best_partition;

	int index = samec_best_partitions[0] != uncor_best_partition ? 0 : 1;
	best_partition_samec = samec_best_partitions[index];

	if (best_partition_dualplane)
	{
		*best_partition_dualplane = (sep_best_component << PARTITION_BITS) |
		                            (sep_best_partition);
	}
}

#endif
