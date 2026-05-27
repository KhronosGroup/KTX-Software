#include "utils.h"

#define FLOOD_PUSH(y, xl, xr, dy) if (((y + (dy)) >= 0) && ((y + (dy)) < (int)height)) { stack.push_back(fill_segment(y, xl, xr, dy)); }

float compute_block_max_std_dev(const basisu::color_rgba* pPixels, uint32_t block_width, uint32_t block_height, uint32_t num_comps)
{
	tracked_stat comp_stats[4];

	for (uint32_t y = 0; y < block_height; y++)
	{
		for (uint32_t x = 0; x < block_width; x++)
		{
			const basisu::color_rgba* pPixel = pPixels + x + y * block_width;

			for (uint32_t c = 0; c < num_comps; c++)
				comp_stats[c].update(pPixel->m_comps[c]);
		}
	}

	float max_std_dev = 0.0f;
	for (uint32_t i = 0; i < num_comps; i++)
		max_std_dev = std::max(max_std_dev, comp_stats[i].get_std_dev());
	return max_std_dev;
}

inline bool flood_fill_is_inside(std::vector<basisu::color_rgba>& image, int x, int y, int w, int h, const basisu::color_rgba& b)
{
  if (x < 0 || x >= w || y < 0 || y >= h)
    return false;
	return image[x + y * w] == b;
}

struct fill_segment
{
	int16_t m_y, m_xl, m_xr, m_dy;

	fill_segment(int y, int xl, int xr, int dy) :
		m_y((int16_t)y), m_xl((int16_t)xl), m_xr((int16_t)xr), m_dy((int16_t)dy)
	{
	}
};

// See http://www.realtimerendering.com/resources/GraphicsGems/gems/SeedFill.c
uint32_t flood_fill(std::vector<basisu::color_rgba>& image, int x, int y,
    uint32_t width, uint32_t height, const basisu::color_rgba& c,
    const basisu::color_rgba& b, std::vector<pixel_coord>* pSet_pixels)
{
	uint32_t total_set = 0;

	if (!flood_fill_is_inside(image, x, y, width, height, b))
		return 0;

	std::vector<fill_segment> stack;
	stack.reserve(64);

	FLOOD_PUSH(y, x, x, 1);
	FLOOD_PUSH(y + 1, x, x, -1);

	while (stack.size())
	{
		fill_segment s = stack.back();
		stack.pop_back();

		int x1 = s.m_xl, x2 = s.m_xr, dy = s.m_dy;
		y = s.m_y + s.m_dy;

		for (x = x1; (x >= 0) && flood_fill_is_inside(image, x, y, width, height, b); x--)
		{
			image[x + y * width] = c;
			total_set++;
			if (pSet_pixels)
				pSet_pixels->push_back(pixel_coord(x, y));
		}

		int l;

		if (x >= x1)
			goto skip;

		l = x + 1;
		if (l < x1)
			FLOOD_PUSH(y, l, x1 - 1, -dy);

		x = x1 + 1;

		do
		{
			for (; x <= ((int)width - 1) && flood_fill_is_inside(image, x, y, width, height, b); x++)
			{
				image[x + y * width] = c;
				total_set++;
				if (pSet_pixels)
					pSet_pixels->push_back(pixel_coord(x, y));
			}
			FLOOD_PUSH(y, l, x - 1, dy);

			if (x > (x2 + 1))
				FLOOD_PUSH(y, x2 + 1, x - 1, -dy);

		skip:
			for (x++; x <= x2 && !flood_fill_is_inside(image, x, y, width, height, b); x++)
				;

			l = x;
		} while (x <= x2);
	}

	return total_set;
}

