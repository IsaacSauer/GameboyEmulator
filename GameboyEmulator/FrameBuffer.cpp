//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//Source: https://github.com/jgilchrist/gbemu/blob/master/src
#include "pch.h"
#include "FrameBuffer.h"
#include "bitwise.h"
#include "GameBoy.h"

uint16_t ReColor::recolor0 = (uint16_t)0xEFDF;
uint16_t ReColor::recolor1 = (uint16_t)0x8C7F;
uint16_t ReColor::recolor2 = (uint16_t)0x365F;
uint16_t ReColor::recolor3 = (uint16_t)0x012F;

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

void FrameBuffer::set_pixel(unsigned int x, unsigned int y, Color color)
{
	//Recolor
	buffer[pixel_index(x, y)] = ReColor::GetRecolored(color);
}

auto FrameBuffer::get_pixel(unsigned int x, unsigned int y) const -> Color
{
	//Recolor
	return ReColor::GetReal(buffer[pixel_index(x, y)]);
}

inline auto FrameBuffer::pixel_index(unsigned int x, unsigned int y) const -> unsigned int { return (y * width) + x; }

void FrameBuffer::reset() {
	for (unsigned int i = 0; i < width * height; i++) {
		buffer[i] = Color::White;
	}
}

Color ReColor::GetRecolored(Color realColor)
{
	switch (realColor)
	{
	case Color::White:
		return (Color)recolor0;
	case Color::LightGray:
		return (Color)recolor1;
	case Color::DarkGray:
		return (Color)recolor2;
	case Color::Black:
		return (Color)recolor3;
	default:
		break;
	}
	return Color();
}

Color ReColor::GetReal(Color realColor)
{
	uint16_t reCol{ (uint16_t)realColor };
	if (reCol == recolor0)
		return Color::White;
	if (reCol == recolor1)
		return Color::LightGray;
	if (reCol == recolor2)
		return Color::DarkGray;
	if (reCol == recolor3)
		return Color::Black;
	return Color();
}