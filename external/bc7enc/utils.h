#pragma once

#include "encoder/basisu_enc.h"
#include <cmath>
#include <cstdint>

struct pixel_coord
{
	uint16_t m_x, m_y;
	pixel_coord() { }
	pixel_coord(uint32_t x, uint32_t y) : m_x((uint16_t)x), m_y((uint16_t)y) { }
};


template <typename S> inline S clamp(S value, S low, S high) {
  return (value < low) ? low : ((value > high) ? high : value);
}

//
// https://www.johndcook.com/blog/standard_deviation/
// This class is for small numbers of integers, so precision shouldn't be an issue.
// This is also available at: basis_universal/example_transcoding/utils.h
//
class tracked_stat
{
public:
	tracked_stat() { clear(); }

	void clear() { m_num = 0; m_total = 0; m_total2 = 0; }

	void update(uint32_t val) { m_num++; m_total += val; m_total2 += val * val; }

	tracked_stat& operator += (uint32_t val) { update(val); return *this; }

	uint32_t get_number_of_values() const { return m_num; }
	uint64_t get_total() const { return m_total; }
	uint64_t get_total2() const { return m_total2; }

	float get_mean() const { return m_num ? (float)m_total / m_num : 0.0f; };
		
	float get_variance() const { return m_num ? ((float)(m_num * m_total2 - m_total * m_total)) / (m_num * m_num) : 0.0f; }
	float get_std_dev() const { return m_num ? sqrtf((float)(m_num * m_total2 - m_total * m_total)) / m_num : 0.0f; }

	float get_sample_variance() const { return (m_num > 1) ? ((float)(m_num * m_total2 - m_total * m_total)) / (m_num * (m_num - 1)) : 0.0f; }
	float get_sample_std_dev() const { return (m_num > 1) ? sqrtf(get_sample_variance()) : 0.0f; }

private:
	uint32_t m_num;
	uint64_t m_total;
	uint64_t m_total2;
};

inline uint32_t get_clamped_index(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  return clamp<int>(x, 0, width - 1) + clamp<int>(y, 0, height- 1) * width;
}

float compute_block_max_std_dev(const basisu::color_rgba* pPixels,
    uint32_t block_width, uint32_t block_height, uint32_t num_comps);

// See http://www.realtimerendering.com/resources/GraphicsGems/gems/SeedFill.c
uint32_t flood_fill(std::vector<basisu::color_rgba>& image, int x, int y,
    uint32_t width, uint32_t height, const basisu::color_rgba& c,
    const basisu::color_rgba& b, std::vector<pixel_coord>* pSet_pixels);
