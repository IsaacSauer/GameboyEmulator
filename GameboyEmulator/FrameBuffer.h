//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//Source: https://github.com/jgilchrist/gbemu/blob/master/src
#pragma once

#include <array>
#include "Register.h"
#include <vector>

class GameBoy;

struct TileInfo
{
	uint8_t line;
	std::vector<uint8_t> pixels;
};
enum class GBColor : uint8_t
{
	Color0, /* White */
	Color1, /* Light gray */
	Color2, /* Dark gray */
	Color3, /* Black */
};
enum class Color : uint16_t
{
	White = 0xFFFF,
	LightGray = 0x8888,
	DarkGray = 0x4444,
	Black = 0x0000,
};
struct Palette
{
	Color color0 = Color::White;
	Color color1 = Color::LightGray;
	Color color2 = Color::DarkGray;
	Color color3 = Color::Black;
};

struct ReColor
{
	static uint16_t recolor0;
	static uint16_t recolor1;
	static uint16_t recolor2;
	static uint16_t recolor3;

	static Color GetRecolored(Color realColor);
	static Color GetReal(Color realColor);
};

const unsigned int TILES_PER_LINE = 32;
const unsigned int TILE_HEIGHT_PX = 8;
const unsigned int TILE_WIDTH_PX = 8;

const Address TILE_SET_ZERO_ADDRESS = 0x8000;
const Address TILE_SET_ONE_ADDRESS = 0x8800;

const Address TILE_MAP_ZERO_ADDRESS = 0x9800;
const Address TILE_MAP_ONE_ADDRESS = 0x9C00;

/* A single tile contains 8 lines, each of which is two bytes */
const unsigned int TILE_BYTES = 2 * 8;

const unsigned int SPRITE_BYTES = 4;

GBColor GetColor(uint8_t pixel_value);

class FrameBuffer {
public:
	FrameBuffer() {}
	FrameBuffer(unsigned int width, unsigned int height);

	void set_pixel(unsigned int x, unsigned int y, Color color);
	auto get_pixel(unsigned int x, unsigned int y) const->Color;

	std::vector<uint16_t>& GetBuffer() const { return *(std::vector<uint16_t>*)(&buffer); }

	void reset();

private:
	unsigned int width{};
	unsigned int height{};

	auto pixel_index(unsigned int x, unsigned int y) const->unsigned int;

	std::vector<Color> buffer{};
};