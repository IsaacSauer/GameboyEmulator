//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//Source: https://github.com/jgilchrist/gbemu/blob/master/src
#include "pch.h"
#include "Tile.h"
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

Tile::Tile(Address& address, GameBoy& mmu, unsigned int size_multiplier)
{
	///* Set the whole framebuffer to be black */
	//for (unsigned int x = 0; x < TILE_WIDTH_PX; x++)
	//{
	//	for (unsigned int y = 0; y < TILE_HEIGHT_PX * size_multiplier; y++)
	//	{
	//		buffer[pixel_index(x, y)] = GBColor::Color0;
	//	}
	//}

	for (unsigned int tile_line = 0; tile_line < TILE_HEIGHT_PX * size_multiplier; tile_line++)
	{
		/* 2 (bytes per line of pixels) * y (lines) */
		unsigned int index_into_tile = 2 * tile_line;
		Address line_start = address + index_into_tile;

		u8 pixels_1 = mmu.ReadMemory(line_start.value());
		u8 pixels_2 = mmu.ReadMemory(line_start.value() + 1);

		for (unsigned int x = 0; x < TILE_WIDTH_PX; x++)
		{
			uint8_t color_value = static_cast<uint8_t>((bitwise::bit_value(pixels_2, 7 - x) << 1) | bitwise::bit_value(pixels_1, 7 - x));
			buffer[pixel_index(x, tile_line)] = GetColor(color_value);
		}
	}
}

auto Tile::get_pixel(unsigned int x, unsigned int y) const -> GBColor
{
	return buffer[pixel_index(x, y)];
}

auto Tile::pixel_index(unsigned int x, unsigned int y) -> unsigned int
{
	return (y * TILE_HEIGHT_PX) + x;
}

auto Tile::get_pixel_line(uint8_t byte1, uint8_t byte2) -> uint8_t*
{
	uint8_t* pixel_line = new uint8_t[8];
	for (uint8_t i = 0; i < 8; i++)
	{
		uint8_t color_value = static_cast<uint8_t>((bitwise::bit_value(byte2, 7 - i) << 1) | bitwise::bit_value(byte1, 7 - i));
		pixel_line[i] = color_value;
	}
	return pixel_line;
}

FrameBuffer::FrameBuffer(unsigned int _width, unsigned int _height) :
	width(_width),
	height(_height),
	buffer(width * height, Color::White)
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