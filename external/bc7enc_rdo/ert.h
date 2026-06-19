#pragma once

#ifdef _MSC_VER
#pragma warning (disable:4201) // nameless struct/union
#endif

#include <cstdint>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <vector>

namespace ert
{
  template <typename S> inline S clamp(S value, S low, S high) {
    return (value < low) ? low : ((value > high) ? high : value);
  }

  // Copied from Basis Universal's basist::color_rgba. Even though this is the
  // same as basist::color_rgba (minus some functions), using them
  // interchangeably is probably undefined behavior (UB).
  //
  // Q. Why not use basist::color_rgba directly? This would require adding
  //    Basis Universal as an external dependency to an otherwise
  //    zero-dependency project. In case that is acceptable, color32 in rgbcx
  //    can also be replaced (to some degree) by basist::color_rgba.
  struct color_rgba
  {
  	union
  	{
  		uint8_t m_comps[4];
  
  		struct
  		{
  			uint8_t r;
  			uint8_t g;
  			uint8_t b;
  			uint8_t a;
  		};
  	};
  
  	inline color_rgba()
  	{
  		static_assert(sizeof(*this) == 4, "sizeof(*this) != 4");
    }
  	
  
  	inline color_rgba(int y)
  	{
  		set(y);
  	}
  
  	inline color_rgba(int y, int na)
  	{
  		set(y, na);
  	}
  
  	inline color_rgba(int sr, int sg, int sb, int sa)
  	{
  		set(sr, sg, sb, sa);
  	}
  
  	inline color_rgba &set(int y)
  	{
  		m_comps[0] = static_cast<uint8_t>(clamp<int>(y, 0, 255));
  		m_comps[1] = m_comps[0];
  		m_comps[2] = m_comps[0];
  		m_comps[3] = 255;
  		return *this;
  	}
  
  	inline color_rgba &set(int y, int na)
  	{
  		m_comps[0] = static_cast<uint8_t>(clamp<int>(y, 0, 255));
  		m_comps[1] = m_comps[0];
  		m_comps[2] = m_comps[0];
  		m_comps[3] = static_cast<uint8_t>(clamp<int>(na, 0, 255));
  		return *this;
  	}
  
  	inline color_rgba &set(int sr, int sg, int sb, int sa)
  	{
  		m_comps[0] = static_cast<uint8_t>(clamp<int>(sr, 0, 255));
  		m_comps[1] = static_cast<uint8_t>(clamp<int>(sg, 0, 255));
  		m_comps[2] = static_cast<uint8_t>(clamp<int>(sb, 0, 255));
  		m_comps[3] = static_cast<uint8_t>(clamp<int>(sa, 0, 255));
  		return *this;
  	}
  
    inline bool operator== (const color_rgba &rhs) const
    {
      if (m_comps[0] != rhs.m_comps[0]) return false;
      if (m_comps[1] != rhs.m_comps[1]) return false;
      if (m_comps[2] != rhs.m_comps[2]) return false;
      if (m_comps[3] != rhs.m_comps[3]) return false;
      return true;
    }
  
  	inline int get_601_luma() const { return (19595U * m_comps[0] + 38470U * m_comps[1] + 7471U * m_comps[2] + 32768U) >> 16U; }
  	inline int get_709_luma() const { return (13938U * m_comps[0] + 46869U * m_comps[1] + 4729U * m_comps[2] + 32768U) >> 16U; } 
  	inline int get_luma(bool luma_601) const { return luma_601 ? get_601_luma() : get_709_luma(); }
  };


  struct pixel_coord
  {
  	uint16_t m_x, m_y;
  	pixel_coord() { }
  	pixel_coord(uint32_t x, uint32_t y) : m_x((uint16_t)x), m_y((uint16_t)y) { }
  };
  
  
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


	struct reduce_entropy_params
	{
		// m_lambda: The post-processor tries to reduce distortion*smooth_block_scale + rate*lambda (rate is approximate LZ bits and distortion is scaled MS error multiplied against the smooth block MSE weighting factor).
		// Larger values push the postprocessor towards optimizing more for lower rate, and smaller values more for distortion. 0=minimal distortion.
		float m_lambda;

		// m_lookback_window_size: The number of bytes the encoder can look back from each block to find matches. The larger this value, the slower the encoder but the higher the quality per LZ compressed bit.
		uint32_t m_lookback_window_size;

		// m_max_allowed_rms_increase_ratio: How much the RMS error of a block is allowed to increase before a trial is rejected. 1.0=no increase allowed, 1.05=5% increase allowed, etc.
		float m_max_allowed_rms_increase_ratio;

		float m_max_smooth_block_std_dev;
		float m_smooth_block_max_mse_scale;

		uint32_t m_color_weights[4];
				
		bool m_try_two_matches;
		bool m_skip_zero_mse_blocks;

		reduce_entropy_params() { clear(); }

		void clear()
		{
			m_lookback_window_size = 128;
			m_lambda = 0.5f;
			m_max_allowed_rms_increase_ratio = 10.0f;
			m_max_smooth_block_std_dev = 18.0f;
			m_smooth_block_max_mse_scale = 10.0f;
			m_color_weights[0] = 1;
			m_color_weights[1] = 1;
			m_color_weights[2] = 1;
			m_color_weights[3] = 1;
			m_try_two_matches = true;
			m_skip_zero_mse_blocks = false;
		}
	};

  struct reduce_entropy_stats {
		uint32_t total_smooth_blocks;
		uint32_t total_second_matches;
  };

	typedef bool (*pUnpack_block_func)(const void* pBlock, color_rgba* pPixels, uint32_t block_index, void* pUser_data);

	// BC7 entropy reduction transform with Deflate/LZMA/LZHAM optimizations
	bool reduce_entropy(void* pBlocks, uint32_t num_blocks,
		uint32_t total_block_stride_in_bytes, uint32_t block_size_to_optimize_in_bytes, uint32_t block_width, uint32_t block_height, uint32_t num_comps,
		const color_rgba* pBlock_pixels, const reduce_entropy_params& params, uint32_t& total_modified,
		pUnpack_block_func pUnpack_block_func, void* pUnpack_block_func_user_data, reduce_entropy_stats& stats,
		const float* pBlock_mse_scales = nullptr);

  inline uint32_t get_clamped_index(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    return clamp<int>(x, 0, width - 1) + clamp<int>(y, 0, height- 1) * width;
  }
  
  inline float compute_block_max_std_dev(const color_rgba* pPixels,
      uint32_t block_width, uint32_t block_height, uint32_t num_comps) {
		tracked_stat comp_stats[4];

		for (uint32_t y = 0; y < block_height; y++)
		{
			for (uint32_t x = 0; x < block_width; x++)
			{
				const color_rgba* pPixel = pPixels + x + y * block_width;

				for (uint32_t c = 0; c < num_comps; c++)
					comp_stats[c].update(pPixel->m_comps[c]);
			}
		}

		float max_std_dev = 0.0f;
		for (uint32_t i = 0; i < num_comps; i++)
			max_std_dev = std::max(max_std_dev, comp_stats[i].get_std_dev());
		return max_std_dev;
  }
  
  // See http://www.realtimerendering.com/resources/GraphicsGems/gems/SeedFill.c
  uint32_t flood_fill(std::vector<color_rgba>& image, int x, int y,
      uint32_t width, uint32_t height, const color_rgba& c,
      const color_rgba& b, std::vector<pixel_coord>* pSet_pixels);

  /*
   * Number of blocks to encode for each thread SHOULD be superior or equal to
   * (rdo_window_size / block_size) otherwise multi-threaded RDO will not be
   * deterministic and will depend on the set window size.
   */
  inline uint32_t
  adjust_num_threads_for_deterministic_rdo(uint32_t requested_num_threads, uint32_t rdo_window_size,
                                           uint32_t block_size, uint32_t num_blocks_total) {
      const auto min_num_blocks_per_thread = std::max<uint32_t>(1U, rdo_window_size / block_size);
      // num_blocks_total / x >= min_num_blocks
      //  => x <= num_blocks_total / min_num_blocks
      //  => x <= floor(num_blocks_total / min_num_blocks)
      // and handle the edge case where min_num_blocks_per_thread > num_blocks_total
      //  => x =  max(1, floor(num_blocks_total / min_num_blocks))
      return std::max<uint32_t>(1U, std::min<uint32_t>(requested_num_threads, floor(num_blocks_total / min_num_blocks_per_thread)));
  }

} // namespace ert
