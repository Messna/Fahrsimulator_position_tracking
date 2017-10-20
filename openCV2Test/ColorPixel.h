#pragma once

struct ColorPixel
{
	int x;
	int y;
	int red;
	int green;
	int blue;

	void init(int i_x, int i_y, int i_red, int i_green, int i_blue)
	{
		x = i_x;
		y = i_y;
		red = i_red;
		green = i_green;
		blue = i_blue;
	}
};