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
enum class GBColor
{
	Color0, /* White */
	Color1, /* Light gray */
	Color2, /* Dark gray */
	Color3, /* Black */
};
enum class Color : uint16_t
{
	White = 0xFFFF,
	LightGray = 0xAAAA,
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

class Tile {
public:
    Tile(Address& address, GameBoy& mmu, unsigned int size_multiplier = 1);

    auto get_pixel(unsigned int x, unsigned int y) const->GBColor;

private:
    static inline auto pixel_index(unsigned int x, unsigned int y)->unsigned int;
    static inline auto get_pixel_line(uint8_t byte1, uint8_t byte2)->uint8_t*;

	GBColor buffer[TILE_HEIGHT_PX * 2 * TILE_WIDTH_PX]{};
};

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