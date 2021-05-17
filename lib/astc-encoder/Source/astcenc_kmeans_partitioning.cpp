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
 * @brief Functions for approximate partitioning by kmeans clustering.
 *
 * Do this in 2 stages:
 * 1: basic clustering, a couple of passes just to get a few clusters
 * 2: clustering based on line, a few passes until it seems to stabilize.
 *
 * After clustering is done, we use the clustering result to construct one
 * bitmap for each partition. We then scan though the partition table, counting
 * how well the bitmaps matched.
  */

#include "astcenc_internal.h"

/**
 * @brief Pick some initital kmeans cluster centers.
 *
 * @param      blk               The image block color data to compress.
 * @param      texel_count       The number of texels in the block.
 * @param      partition_count   The number of partitions in the block.
 * @param[out] cluster_centers   The initital partition cluster center colors.
 */
static void kmeans_init(
	const imageblock& blk,
	int texel_count,
	int partition_count,
	vfloat4 cluster_centers[4]
) {
	promise(texel_count > 0);
	promise(partition_count > 0);

	int clusters_selected = 0;
	float distances[MAX_TEXELS_PER_BLOCK];

	// Pick a random sample as first cluster center; 145897 from random.org
	int sample = 145897 % texel_count;
	vfloat4 center_color = blk.texel(sample);
	cluster_centers[clusters_selected] = center_color;
	clusters_selected++;

	// Compute the distance to the first cluster center
	float distance_sum = 0.0f;
	for (int i = 0; i < texel_count; i++)
	{
		vfloat4 color = blk.texel(i);
		vfloat4 diff = color - center_color;
		float distance = dot_s(diff, diff);
		distance_sum += distance;
		distances[i] = distance;
	}

	// More numbers from random.org for weighted-random center selection
	const float cluster_cutoffs[9] = {
		0.626220f, 0.932770f, 0.275454f,
		0.318558f, 0.240113f, 0.009190f,
		0.347661f, 0.731960f, 0.156391f
	};

	int cutoff = (clusters_selected - 1) + 3 * (partition_count - 2);

	// Pick the remaining samples as needed
	while (true)
	{
		// Pick the next center in a weighted-random fashion.
		float summa = 0.0f;
		float distance_cutoff = distance_sum * cluster_cutoffs[cutoff++];
		for (sample = 0; sample < texel_count; sample++)
		{
			summa += distances[sample];
			if (summa >= distance_cutoff)
			{
				break;
			}
		}

		// Clamp to a valid range and store the selected cluster center
		sample = astc::min(sample, texel_count - 1);

		center_color = blk.texel(sample);
		cluster_centers[clusters_selected++] = center_color;
		if (clusters_selected >= partition_count)
		{
			break;
		}

		// Compute the distance to the new cluster center, keep the min dist
		distance_sum = 0.0f;
		for (int i = 0; i < texel_count; i++)
		{
			vfloat4 color = blk.texel(i);
			vfloat4 diff = color - center_color;
			float distance = dot_s(diff, diff);
			distance = astc::min(distance, distances[i]);
			distance_sum += distance;
			distances[i] = distance;
		}
	}
}

/**
 * @brief Assign texels to clusters, based on a set of chosen center points.
 *
 * @todo Can partition of texel be uint8_t not int?
 *
 * @param      blk                  The image block color data to compress.
 * @param      texel_count          The number of texels in the block.
 * @param      partition_count      The number of partitions in the block.
 * @param      cluster_centers      The partition cluster center colors.
 * @param[out] partition_of_texel   The partition assigned for each texel.
 */
static void kmeans_assign(
	const imageblock& blk,
	int texel_count,
	int partition_count,
	const vfloat4 cluster_centers[4],
	int partition_of_texel[MAX_TEXELS_PER_BLOCK]
) {
	promise(texel_count > 0);
	promise(partition_count > 0);

	int partition_texel_count[4] { 0 };

	// Find the best partition for every texel
	for (int i = 0; i < texel_count; i++)
	{
		float best_distance = std::numeric_limits<float>::max();
		int best_partition = -1;

		vfloat4 color = blk.texel(i);
		for (int j = 0; j < partition_count; j++)
		{
			vfloat4 diff = color - cluster_centers[j];
			float distance = dot_s(diff, diff);
			if (distance < best_distance)
			{
				best_distance = distance;
				best_partition = j;
			}
		}

		partition_of_texel[i] = best_partition;
		partition_texel_count[best_partition]++;
	}

	// It is possible to get a situation where a partition ends up without any
	// texels. In this case, we assign texel N to partition N. This is silly,
	// but ensures that every partition retains at least one texel. Reassigning
	// a texel in this manner may cause another partition to go empty, so if we
	// actually did a reassignment, we run the whole loop over again.
	int problem_case;
	do
	{
		problem_case = 0;
		for (int i = 0; i < partition_count; i++)
		{
			if (partition_texel_count[i] == 0)
			{
				partition_texel_count[partition_of_texel[i]]--;
				partition_texel_count[i]++;
				partition_of_texel[i] = i;
				problem_case = 1;
			}
		}
	} while (problem_case != 0);
}

/**
 * @brief Compute new cluster centers based on their center of gravity.
 *
 * @param       blk                  The image block color data to compress.
 * @param       texel_count          The number of texels in the block.
 * @param       partition_count      The number of partitions in the block.
 * @param[out]  cluster_centers      The new cluster center colors.
 * @param       partition_of_texel   The partition assigned for each texel.
 */
static void kmeans_update(
	const imageblock& blk,
	int texel_count,
	int partition_count,
	vfloat4 cluster_centers[4],
	const int partition_of_texel[MAX_TEXELS_PER_BLOCK]
) {
	promise(texel_count > 0);
	promise(partition_count > 0);

	vfloat4 color_sum[4] {
		vfloat4::zero(),
		vfloat4::zero(),
		vfloat4::zero(),
		vfloat4::zero()
	};

	int partition_texel_count[4] { 0 };

	// Find the center-of-gravity in each cluster
	for (int i = 0; i < texel_count; i++)
	{
		int partition = partition_of_texel[i];
		color_sum[partition] += blk.texel(i);;
		partition_texel_count[partition]++;
	}

	// Set the center of gravity to be the new cluster center
	for (int i = 0; i < partition_count; i++)
	{
		float scale = 1.0f / static_cast<float>(partition_texel_count[i]);
		cluster_centers[i] = color_sum[i] * scale;
	}
}

/**
 * @brief Compute bit-mismatch for partitioning in 2-partition mode.
 *
 * @param a   The texel assignment bitvector for the block.
 * @param b   The texel assignment bitvector for the partition table.
 *
 * @return    The number of bit mismatches.
 */
static inline int partition_mismatch2(
	const uint64_t a[2],
	const uint64_t b[2]
) {
	int v1 = astc::popcount(a[0] ^ b[0]) + astc::popcount(a[1] ^ b[1]);
	int v2 = astc::popcount(a[0] ^ b[1]) + astc::popcount(a[1] ^ b[0]);
	return astc::min(v1, v2);
}

/**
 * @brief Compute bit-mismatch for partitioning in 3-partition mode.
 *
 * @param a   The texel assignment bitvector for the block.
 * @param b   The texel assignment bitvector for the partition table.
 *
 * @return    The number of bit mismatches.
 */
static inline int partition_mismatch3(
	const uint64_t a[3],
	const uint64_t b[3]
) {
	int p00 = astc::popcount(a[0] ^ b[0]);
	int p01 = astc::popcount(a[0] ^ b[1]);
	int p02 = astc::popcount(a[0] ^ b[2]);

	int p10 = astc::popcount(a[1] ^ b[0]);
	int p11 = astc::popcount(a[1] ^ b[1]);
	int p12 = astc::popcount(a[1] ^ b[2]);

	int p20 = astc::popcount(a[2] ^ b[0]);
	int p21 = astc::popcount(a[2] ^ b[1]);
	int p22 = astc::popcount(a[2] ^ b[2]);

	int s0 = p11 + p22;
	int s1 = p12 + p21;
	int v0 = astc::min(s0, s1) + p00;

	int s2 = p10 + p22;
	int s3 = p12 + p20;
	int v1 = astc::min(s2, s3) + p01;

	int s4 = p10 + p21;
	int s5 = p11 + p20;
	int v2 = astc::min(s4, s5) + p02;

	return astc::min(v0, v1, v2);
}

/**
 * @brief Compute bit-mismatch for partitioning in 4-partition mode.
 *
 * @param a   The texel assignment bitvector for the block.
 * @param b   The texel assignment bitvector for the partition table.
 *
 * @return    The number of bit mismatches.
 */
static inline int partition_mismatch4(
	const uint64_t a[4],
	const uint64_t b[4]
) {
	int p00 = astc::popcount(a[0] ^ b[0]);
	int p01 = astc::popcount(a[0] ^ b[1]);
	int p02 = astc::popcount(a[0] ^ b[2]);
	int p03 = astc::popcount(a[0] ^ b[3]);

	int p10 = astc::popcount(a[1] ^ b[0]);
	int p11 = astc::popcount(a[1] ^ b[1]);
	int p12 = astc::popcount(a[1] ^ b[2]);
	int p13 = astc::popcount(a[1] ^ b[3]);

	int p20 = astc::popcount(a[2] ^ b[0]);
	int p21 = astc::popcount(a[2] ^ b[1]);
	int p22 = astc::popcount(a[2] ^ b[2]);
	int p23 = astc::popcount(a[2] ^ b[3]);

	int p30 = astc::popcount(a[3] ^ b[0]);
	int p31 = astc::popcount(a[3] ^ b[1]);
	int p32 = astc::popcount(a[3] ^ b[2]);
	int p33 = astc::popcount(a[3] ^ b[3]);

	int mx23 = astc::min(p22 + p33, p23 + p32);
	int mx13 = astc::min(p21 + p33, p23 + p31);
	int mx12 = astc::min(p21 + p32, p22 + p31);
	int mx03 = astc::min(p20 + p33, p23 + p30);
	int mx02 = astc::min(p20 + p32, p22 + p30);
	int mx01 = astc::min(p21 + p30, p20 + p31);

	int v0 = p00 + astc::min(p11 + mx23, p12 + mx13, p13 + mx12);
	int v1 = p01 + astc::min(p10 + mx23, p12 + mx03, p13 + mx02);
	int v2 = p02 + astc::min(p11 + mx03, p10 + mx13, p13 + mx01);
	int v3 = p03 + astc::min(p11 + mx02, p12 + mx01, p10 + mx12);

	return astc::min(v0, v1, v2, v3);
}

using mismatch_dispatch = int (*)(const uint64_t*, const uint64_t*);

/**
 * @brief Count the partition table mismatches vs the data clustering.
 *
 * @param      bsd               The block size information.
 * @param      partition_count   The number of partitions in the block.
 * @param      bitmaps           The block texel partition assignment patterns.
 * @param[out] mismatch_counts   The array storing per partitioning mismatch counts.
 */
static void count_partition_mismatch_bits(
	const block_size_descriptor& bsd,
	int partition_count,
	const uint64_t bitmaps[4],
	int mismatch_counts[PARTITION_COUNT]
) {
	const partition_info *pt = get_partition_table(&bsd, partition_count);

	// Function pointer dispatch table
	const mismatch_dispatch dispatch[3] {
		partition_mismatch2,
		partition_mismatch3,
		partition_mismatch4
	};

	for (int i = 0; i < PARTITION_COUNT; i++)
	{
		int bitcount = 255;
		if (pt->partition_count == partition_count)
		{
			bitcount = dispatch[partition_count - 2](bitmaps, pt->coverage_bitmaps);
		}

		mismatch_counts[i] = bitcount;
		pt++;
	}
}

/**
 * @brief Use counting sort on the mismatch array to sort partition candidates.
 *
 * @param      mismatch_count       Partitioning mismatch counts, in index order.
 * @param[out] partition_ordering   Partition index values, in mismatch order.
 */
static void get_partition_ordering_by_mismatch_bits(
	const int mismatch_count[PARTITION_COUNT],
	int partition_ordering[PARTITION_COUNT]
) {
	int mscount[256] { 0 };

	// Create the histogram of mismatch counts
	for (int i = 0; i < PARTITION_COUNT; i++)
	{
		mscount[mismatch_count[i]]++;
	}

	// Create a running sum from the histogram array
	// Cells store previous values only; i.e. exclude self after sum
	int summa = 0;
	for (int i = 0; i < 256; i++)
	{
		int cnt = mscount[i];
		mscount[i] = summa;
		summa += cnt;
	}

	// Use the running sum as the index, incrementing after read to allow
	// sequential entries with the same count
	for (int i = 0; i < PARTITION_COUNT; i++)
	{
		int idx = mscount[mismatch_count[i]]++;
		partition_ordering[idx] = i;
	}
}

/* See header for documentation. */
void kmeans_compute_partition_ordering(
	const block_size_descriptor& bsd,
	const imageblock& blk,
	int partition_count,
	int partition_ordering[PARTITION_COUNT]
) {
	vfloat4 cluster_centers[4];
	int texel_partitions[MAX_TEXELS_PER_BLOCK];

	// Use three passes of k-means clustering to partition the block data
	for (int i = 0; i < 3; i++)
	{
		if (i == 0)
		{
			kmeans_init(blk, bsd.texel_count, partition_count, cluster_centers);
		}
		else
		{
			kmeans_update(blk, bsd.texel_count, partition_count, cluster_centers, texel_partitions);
		}

		kmeans_assign(blk, bsd.texel_count, partition_count, cluster_centers, texel_partitions);
	}

	// Construct the block bitmaps of texel assignments to each partition
	uint64_t bitmaps[4] { 0 };
	int texels_to_process = bsd.kmeans_texel_count;
	promise(texels_to_process > 0);
	for (int i = 0; i < texels_to_process; i++)
	{
		int idx = bsd.kmeans_texels[i];
		bitmaps[texel_partitions[idx]] |= 1ULL << i;
	}

	// Count the mismatch between the block and the format's partition tables
	int mismatch_counts[PARTITION_COUNT];
	count_partition_mismatch_bits(bsd, partition_count, bitmaps, mismatch_counts);

	// Sort the partitions based on the number of mismatched bits
	get_partition_ordering_by_mismatch_bits(mismatch_counts, partition_ordering);
}

#endif
