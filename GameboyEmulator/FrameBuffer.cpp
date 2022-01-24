//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//Source: https://github.com/jgilchrist/gbemu/blob/master/src
#include "pch.h"
#include "FrameBuffer.h"
#include "bitwise.h"
#include "GameBoy.h"

GBColor GetColor(uint8_t pixel_value)
{
	switch (pixel_value)
	{
	case 0:
		return GBColor::Color0;
	case 1:
		return GBColor::Color1;
	case 2:
		return GBColor::Color2;
	case 3:
		return GBColor::Color3;
	}

	return GBColor{};
}

FrameBuffer::FrameBuffer(unsigned int _width, unsigned int _height) :
	width(_width),
	height(_height),
	buffer(width* height, Color::White)
{
}

void FrameBuffer::set_pixel(unsigned int x, unsigned int y, Color color) {
	buffer[pixel_index(x, y)] = color;
}

auto FrameBuffer::get_pixel(unsigned int x, unsigned int y) const -> Color { return buffer[pixel_index(x, y)]; }

inline auto FrameBuffer::pixel_index(unsigned int x, unsigned int y) const -> unsigned int { return (y * width) + x; }

void FrameBuffer::reset() {
	for (unsigned int i = 0; i < width * height; i++) {
		buffer[i] = Color::White;
	}
}