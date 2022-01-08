//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//Source: https://github.com/jgilchrist/gbemu/tree/master/src/cartridge
#pragma once

#include <string>
#include <vector>
#include <memory>

const int TITLE_LENGTH = 11;

namespace header
{
	const int entry_point = 0x100;
	const int logo = 0x104;
	const int title = 0x134;
	const int manufacturer_code = 0x13F;
	const int cgb_flag = 0x143;
	const int new_license_code = 0x144;
	const int sgb_flag = 0x146;
	const int cartridge_type = 0x147;
	const int rom_size = 0x148;
	const int ram_size = 0x149;
	const int destination_code = 0x14A;
	const int old_license_code = 0x14B;
	const int version_number = 0x14C;
	const int header_checksum = 0x14D;
	const int global_checksum = 0x14E;
} // namespace header

enum class CartridgeType
{
	ROMOnly,
	MBC1,
	MBC2,
	MBC3,
	MBC4,
	MBC5,
	Unknown,
};

extern CartridgeType get_type(uint8_t type);
extern std::string describe(CartridgeType type);

extern std::string get_title(std::vector<uint8_t>& rom);

extern std::string get_license(uint16_t old_license, uint16_t new_license);

enum class ROMSize
{
	KB32,
	KB64,
	KB128,
	KB256,
	KB512,
	MB1,
	MB2,
	MB4,
	MB1p1,
	MB1p2,
	MB1p5,
};

extern ROMSize get_rom_size(uint8_t size_code);
extern std::string describe(ROMSize size);

enum class RAMSize
{
	None,
	KB2,
	KB8,
	KB32,
	KB128,
	KB64,
};

extern RAMSize get_ram_size(uint8_t size_code);
extern unsigned int get_actual_ram_size(RAMSize size_code);
extern std::string describe(RAMSize size);

enum class Destination
{
	Japanese,
	NonJapanese,
};

extern Destination get_destination(uint8_t destination);
extern std::string describe(Destination destination);

class CartridgeInfo
{
public:
	std::string title{};

	/* Cartridge information */
	CartridgeType type{};
	Destination destination{};
	ROMSize rom_size{};
	RAMSize ram_size{};
	std::string license_code{};
	uint8_t version{};

	uint16_t header_checksum{};
	uint16_t global_checksum{};

	bool supports_cgb{};
	bool supports_sgb{};
};

extern std::unique_ptr<CartridgeInfo> get_info(std::vector<uint8_t> rom);
