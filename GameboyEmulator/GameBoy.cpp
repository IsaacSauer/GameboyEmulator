#include "pch.h"
#include "GameBoy.h"
#include <fstream>
#include <iostream>
#include <time.h>
#include <algorithm>

#include "opc_test/disassembler.h"
#include "CartridgeInfo.h"
#include "FrameBuffer.h"
#include "Measure.h"
#include "bitwise.h"

GameBoy::GameBoy(const std::string& gameFile)
	: GameBoy{}
{
}

//values of banks needed / size etc retrieved from https://gbdev.gg8.se/wiki/articles/The_Cartridge_Header#0149_-_RAM_Size
void GameBoy::LoadGame(const std::string& gbFile)
{
	m_FileName = gbFile;

	std::ifstream file{ m_FileName, std::ios::binary };
	assert(file.good());

	file.seekg(0, std::ios::end);
	const std::ifstream::pos_type size{ file.tellg() };
	std::cout << "rom size: " << size << std::endl;
	m_Rom.resize(size, 0);

	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(m_Rom.data()), size);

	file.close();

	////Disassmble required opcodes to txt file
	//Disassemble();

	auto cartridge = get_info(m_Rom);
	const GameHeader header{ ReadHeader() };
	m_Mbc = header.mbc;

	std::cout << "title of game: " << header.title << std::endl;

	std::cout << "ram size: " << std::to_string(header.ramSizeValue) << std::endl;

	std::cout << "actual rom size: " << std::to_string(m_Rom[0x0148]) << std::endl;

	m_RamBankEnabled = header.ramSizeValue;

	int numberOfBanks{};
	switch (header.ramSizeValue)
	{
	case 0x00:
		numberOfBanks = 0;
		break;
	case 0x01:
		//Listed in various unofficial docs as 2KB. However, a 2KB RAM chip was never used in a cartridge. The source for this value is unknown.
		break;
	case 0x02:
		numberOfBanks = 1;
		break;
	case 0x03:
		numberOfBanks = 4;
		break;
	case 0x04:
		numberOfBanks = 16;
		break;
	case 0x05:
		numberOfBanks = 8;
		break;
	default:
		numberOfBanks = 0;
		break;
	}

	m_RamBanks.resize(numberOfBanks * 0x8000);

	std::cout << "memory bank: " << std::to_string(m_Mbc) << std::endl;
	m_Cpu.Reset();
}

void GameBoy::Update()
{
	/*If we increase the allowed ticks per update, we'll experience "time jumps" the screen will be drawn slower than the changes are performed
	 * Potentially, for tetris, the game starts and the next frame it's over..
	 */

	const float fps{ 59.73f };
	const float timeAdded{ 1000 / fps };
	bool idle{};

	clock_t lastValidTick{ clock() / (CLOCKS_PER_SEC / 1000) };

	Measure timer{ "Update" };
	std::ofstream outputStream;
	outputStream.open("measures.txt");

	while (m_IsRunning)
	{
		const clock_t currentTicks = { clock() / (CLOCKS_PER_SEC / 1000) };
		//I'm keeping fps in mind to ensure a smooth simulation, if we make the cyclebudget infinite, we have 0 control
		if (!m_IsPaused && static_cast<float>(lastValidTick) + timeAdded < static_cast<float>(currentTicks))
		{
			if (m_AutoSpeed && static_cast<float>(currentTicks) - (static_cast<float>(lastValidTick) + timeAdded) >= .5f && m_SpeedMultiplier >= 2)
			{
				--m_SpeedMultiplier;
			}
			m_CurrentCycles = 0;
			//4194304 cycles can be done in a second (standard gameboy)
			const unsigned int cycleBudget{ static_cast<unsigned>(ceil(4194304.0f / fps)) * m_SpeedMultiplier };
			while (m_IsRunning && !m_IsPaused && m_CurrentCycles < cycleBudget)
			{
				unsigned int stepCycles{ m_CurrentCycles };
				m_Cpu.ExecuteNextOpcode();
				stepCycles = m_CurrentCycles - stepCycles;
				//If we're cycle bound, subtract cycles and call pause if needed
				if (m_IsCycleFrameBound & 2 && (m_CyclesFramesToRun - stepCycles > m_CyclesFramesToRun || !(m_CyclesFramesToRun -= stepCycles)))
				{
					m_IsCycleFrameBound = 0;
					m_IsPaused = true;
				}
				HandleTimers(stepCycles, cycleBudget);
				//Draw if we don't care, are not frame bound or are on the final frame
				m_Cpu.HandleGraphics(stepCycles, cycleBudget,
					!m_OnlyDrawLast || !(m_IsCycleFrameBound & 1) || m_IsCycleFrameBound & 1 && m_CyclesFramesToRun == 1);
				//If vblank interrupt and we're frame bound, subtract frames and call pause if needed
				if (GetIF() & 1 && m_IsCycleFrameBound & 1 && !--m_CyclesFramesToRun)
				{
					m_IsCycleFrameBound = 0;
					m_IsPaused = true;
				}
				m_Cpu.HandleInterupts();
			}
			lastValidTick = currentTicks;
			idle = false;
		}
		else if (!idle)
		{
			if (m_AutoSpeed && static_cast<float>(lastValidTick) + timeAdded - static_cast<float>(currentTicks) >= .5f)
			{
				++m_SpeedMultiplier;
			}
			idle = true;
		}
	}

	outputStream.close();
}

GameHeader GameBoy::ReadHeader()
{
	GameHeader header{};
	memcpy(header.title, static_cast<void*>(m_Rom.data() + 0x134), 16);

	//Set MBC chip version
	switch (m_Rom[0x147])
	{
	case 0x00:
	case 0x08:
	case 0x09:
		header.mbc = none;
		break;

	case 0x01:
	case 0x02:
	case 0x03:
	case 0xFF:
		header.mbc = mbc1;
		break;

	case 0x05:
	case 0x06:
		header.mbc = mbc2;
		break;

	case 0x0f:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		header.mbc = mbc3;
		break;

	case 0x15:
	case 0x16:
	case 0x17:
		header.mbc = mbc4;
		break;

	case 0x19:
	case 0x1a:
	case 0x1b:
	case 0x1c:
	case 0x1d:
	case 0x1e:
		header.mbc = mbc5;
		break;

	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x20:
	case 0x22:
	case 0xFC:
	case 0xFD:
	case 0xFE:
		header.mbc = unknown;
		break;

	default:
		assert(m_Rom[0x147] == 0x0); //if not 0x0 it's undocumented
	}

	header.ramSizeValue = m_Rom[header::ram_size];

	return header;
}

void GameBoy::TestCPU()
{
	m_TestingOpcodes = true;
	m_Cpu.TestCPU();
	m_TestingOpcodes = false;
}

void GameBoy::Disassemble()
{
	//disassemble
	std::vector<std::string> opcodes{};
	for (int i{}; i < m_Rom.size(); ++i)
		disassemble(m_Rom.data() + i, opcodes);
	//distinct
	std::sort(opcodes.begin(), opcodes.end());
	opcodes.erase(std::unique(opcodes.begin(), opcodes.end()), opcodes.end());
	//write to file
	std::ofstream ofile{ m_FileName + "_opcodes.txt" };
	for (auto code : opcodes)
		ofile << code << '\n';
	ofile.close();
}

void GameBoy::AssignDrawCallback(const std::function<void(const std::vector<uint16_t>&)>& _vblank_callback)
{
	m_Cpu.register_vblank_callback(_vblank_callback);
}

uint8_t GameBoy::ReadMemory(const uint16_t pos)
{
	if (m_TestingOpcodes)
		return (uint8_t)m_Cpu.mmu_read(pos);

	if (InRange(pos, 0x0, 0x7FFF) || //ROM bank
		InRange(pos, 0xA000, 0xBFFF)) //RAM Bank
		return MBCRead(pos);

	if (pos == 0xFF00) //Input
		return GetJoypadState();

	return m_Memory[pos];
}

uint16_t GameBoy::ReadMemoryWord(uint16_t& pos)
{
	if (m_TestingOpcodes)
	{
		const uint16_t res{ static_cast<uint16_t>(static_cast<uint16_t>(m_Cpu.mmu_read(pos)) | static_cast<uint16_t>(m_Cpu.mmu_read(pos + 1)) << 8) };
		pos += 2;
		return res;
	}

	const uint16_t res{ static_cast<uint16_t>(static_cast<uint16_t>(ReadMemory(pos)) | static_cast<uint16_t>(ReadMemory(pos + 1)) << 8) };
	pos += 2;
	return res;
}

void GameBoy::WriteMemory(uint16_t address, uint8_t data)
{
	if (m_TestingOpcodes)
	{
		m_Cpu.mmu_write(address, data);
		return;
	}

	//External ROM
	if (InRange(address, 0x0000, 0x7fff))
	{
		MBCWrite(address, data);
		return;
	}

	//External (cartridge) ram
	if (InRange(address, 0xa000, 0xbfff))
	{
		MBCWrite(address, data);
		return;
	}

	if (InRange(address, static_cast<uint16_t>(0xC000), static_cast<uint16_t>(0xDFFF))) //Internal RAM
	{
		m_Memory[address] = data;
		return;
	}

	if (address == 0xFF04) //Reset DIV
	{
		m_DIVTimer = 0;
		m_DivCycles = 0;
	}
	else if (address == 0xFF07) //Set timer Clock speed
	{
		m_TACTimer = data & 0x7;
	}
	else if (address == 0xFF44)  //Horizontal scanline reset
	{
		m_Memory[address] = 0;
	}
	else if (address == 0xFF46)  //DMA transfer
	{
		const uint16_t src{ static_cast<uint16_t>(static_cast<uint16_t>(data) << 8) };
		for (int i{ 0 }; i < 0xA0; ++i)
			WriteMemory(static_cast<uint16_t>(0xFE00 + i), ReadMemory(static_cast<uint16_t>(src + i)));
	}
	else
		m_Memory[address] = data;
}

void GameBoy::WriteMemoryWord(const uint16_t pos, const uint16_t value)
{
	if (m_TestingOpcodes)
	{
		m_Cpu.mmu_write(pos, value & 0xFF);
		m_Cpu.mmu_write(pos + 1, value >> 8);
		return;
	}

	WriteMemory(pos, value & 0xFF);
	WriteMemory(pos + 1, value >> 8);
}

void GameBoy::RequestInterrupt(Interupts bit) noexcept
{
	GetIF() |= 1 << bit;
}

void GameBoy::SetKey(const Key key, const bool pressed)
{
	if (pressed)
	{
		const uint8_t oldJoyPad{ m_JoyPadState };
		m_JoyPadState &= ~(1 << key);

		//Previosuly 1
		if (oldJoyPad & 1 << key)
		{
			if (!(m_Memory[0xff00] & 0x20) && !(key + 1 % 2)) //Button Keys
				RequestInterrupt(joypad);
			else if (!(m_Memory[0xff00] & 0x10) && !(key % 2)) //Directional keys
				RequestInterrupt(joypad);
		}
	}
	else
		m_JoyPadState |= (1 << key);
}

void GameBoy::SetColor(int color, float* value)
{
	int i0 = std::clamp((int)std::roundf(value[0] * 15), 0, 15);
	int i1 = std::clamp((int)std::roundf(value[1] * 15), 0, 15);
	int i2 = std::clamp((int)std::roundf(value[2] * 15), 0, 15);

	uint16_t newCol{};
	newCol |= i0;
	newCol <<= 4;
	newCol |= i1;
	newCol <<= 4;
	newCol |= i2;
	newCol <<= 4;
	newCol |= 0xF;

	switch (color)
	{
	case 0:
		ReColor::recolor0 = newCol;
		break;
	case 1:
		ReColor::recolor1 = newCol;
		break;
	case 2:
		ReColor::recolor2 = newCol;
		break;
	case 3:
		ReColor::recolor3 = newCol;
		break;
	default:
		break;
	}
}

void GameBoy::HandleTimers(const unsigned stepCycles, const unsigned cycleBudget)
{
	// This function may be placed under the cpu class...

	//if this never breaks, which I highly doubt it will, change the type to uint8
	assert(stepCycles <= 0xFF);

	const unsigned cyclesOneDiv{ (cycleBudget / 16384) };

	//TODO: turn this into a while loop
	if ((m_DivCycles += stepCycles) >= cyclesOneDiv)
	{
		m_DivCycles -= cyclesOneDiv;
		++m_DIVTimer;
	}

	//if (m_TACTimer & 0x4)
	//{
	//	m_TIMACycles += stepCycles;
	//	unsigned int threshold{};
	//	switch (m_TACTimer & 0x3)
	//	{
	//	case 0:
	//		threshold = cycleBudget / 4096;
	//		break;
	//	case 1:
	//		threshold = cycleBudget / 262144;
	//		break;
	//	case 2:
	//		threshold = cycleBudget / 65536;
	//		break;
	//	case 3:
	//		threshold = cycleBudget / 16384;
	//		break;
	//	default:
	//		assert(true);
	//	}
	//	while (threshold != 0 && m_TIMACycles >= threshold)
	//	{
	//		if (!++m_TIMATimer)
	//		{
	//			m_TIMATimer = m_TMATimer;
	//			GetIF() |= 0x4;
	//		}
	//		m_TIMACycles -= threshold; //threshold == 0??
	//	}
	//}
}

uint8_t GameBoy::GetJoypadState() const
{
	uint8_t res{ m_Memory[0xff00] };

	//Button keys
	if (!(res & 0x20))
	{
		res |= !!(m_JoyPadState & 1 << aButton);
		res |= !!(m_JoyPadState & 1 << bButton) << 1;
		res |= !!(m_JoyPadState & 1 << select) << 2;
		res |= !!(m_JoyPadState & 1 << start) << 3;
	}
	else if (!(res & 0x10))
	{
		res |= !!(m_JoyPadState & 1 << right);
		res |= !!(m_JoyPadState & 1 << left) << 1;
		res |= !!(m_JoyPadState & 1 << up) << 2;
		res |= !!(m_JoyPadState & 1 << down) << 3;
	}
	return res;
}

void GameBoy::MBCNoneWrite(const uint16_t& address, const uint8_t byte)
{
	m_Memory[address] = byte;
}

void GameBoy::MBC1Write(const uint16_t& address, const uint8_t byte)
{
	if (InRange(address, 0x0000, 0x1FFF))
		m_RamBankEnabled = true;

	if (InRange(address, 0x2000, 0x3FFF))
	{
		if (byte == 0x0) { SwitchRomBank(0x1); return; }

		if (byte == 0x20) { SwitchRomBank(0x21); return; }
		if (byte == 0x40) { SwitchRomBank(0x41); return; }
		if (byte == 0x60) { SwitchRomBank(0x61); return; }

		uint8_t rom_bank_bits = byte & 0x1F;
		SwitchRomBank(rom_bank_bits);
	}

	if (InRange(address, 0x4000, 0x5FFF))
		ActiveRomRamBank.ramOrRomBank = byte;

	if (InRange(address, 0x6000, 0x7FFF))
		ActiveRomRamBank.isRam = byte;

	if (InRange(address, 0xA000, 0xBFFF))
	{
		if (!m_RamBankEnabled || m_RamBanks.empty()) { return; }

		auto offset_into_ram = 0x8000 * ActiveRomRamBank.GetRamBank();
		auto address_in_ram = (address - 0xA000) + offset_into_ram;
		m_RamBanks[address_in_ram] = byte;
	}
}

void GameBoy::MBC3Write(const uint16_t& address, const uint8_t byte)
{
	if (InRange(address, 0x0000, 0x1FFF))
	{
		//std::cout << "toggling ram bank enable" << std::endl;

		if (byte == 0x0A)
			m_RamBankEnabled = true;

		if (byte == 0x00)
			m_RamBankEnabled = false;
	}

	if (InRange(address, 0x2000, 0x3FFF))
	{
		//std::cout << "switching rom bank" << std::endl;

		if (byte == 0x0)
			SwitchRomBank(0x1);
		else
		{
			uint8_t rom_bank_bits = byte & 0x7F;
			SwitchRomBank(rom_bank_bits);

			std::cout << "rom bank bit: " << std::to_string(rom_bank_bits) << std::endl;
		}
	}

	if (InRange(address, 0x4000, 0x5FFF))
	{
		//std::cout << "switching ram bank" << std::endl;

		if (byte <= 0x03)
		{
			m_RamOverRtc = true;
			SwitchRamBank(byte);
		}

		if (byte >= 0x08 && byte <= 0xC)
			m_RamOverRtc = false;
	}

	if (InRange(address, 0xA000, 0xBFFF))
	{
		//std::cout << "writing to RAM" << std::endl;
		
		if (!m_RamBankEnabled || m_RamBanks.empty()) { return; }

		auto offset_into_ram = 0x8000 * ActiveRomRamBank.ramBank;
		//auto offset_into_ram = 0x2000 * ActiveRomRamBank.GetRamBank();
		auto address_in_ram = (address - 0xA000) + offset_into_ram;
		m_RamBanks[address_in_ram] = byte;
	}
}

void GameBoy::MBC5Write(const uint16_t& address, const uint8_t byte)
{
	if (InRange(address, 0x0000, 0x1FFF))
	{
		//std::cout << "toggling ram bank enable" << std::endl;

		if (byte == 0x0A)
			m_RamBankEnabled = true;

		if (byte == 0x00)
			m_RamBankEnabled = false;
	}

	if (InRange(address, 0x2000, 0x2FFF))
	{
		//std::cout << "switching rom bank" << std::endl;

		uint8_t rom_bank_bits = byte & 0x7F;
		SwitchRomBank(rom_bank_bits);
	}

	if (InRange(address, 0x3000, 0x3FFF))
	{
	}

	if (InRange(address, 0x4000, 0x5FFF))
		SwitchRamBank(byte);

	if (InRange(address, 0xA000, 0xBFFF))
	{
		if (!m_RamBankEnabled || m_RamBanks.empty()) { return; }

		auto offset_into_ram = 0x8000 * ActiveRomRamBank.ramBank;
		//auto offset_into_ram = 0x2000 * ActiveRomRamBank.GetRamBank();
		auto address_in_ram = (address - 0xA000) + offset_into_ram;
		m_RamBanks[address_in_ram] = byte;
	}
}

uint8_t GameBoy::MBCNoneRead(const uint16_t& address)
{
	if (address < m_Rom.size())
		return m_Rom[address];

	return 0;
}

uint8_t GameBoy::MBC1Read(const uint16_t& address)
{
	if (InRange(address, 0x0000, 0x3FFF))
		return m_Rom[address];

	auto idx = address + ((ActiveRomRamBank.GetRomBank() - 1) * 0x4000);
	if (InRange(address, 0x4000, 0x7FFF) && m_Rom.size() > idx)
		return m_Rom[idx];

	if (InRange(address, 0xA000, 0xBFFF) && m_RamBankEnabled && !m_RamBanks.empty())
		return m_RamBanks[(ActiveRomRamBank.GetRamBank() * 0x8000) + (address - 0xA000)];

	return m_Memory[address];
}

uint8_t GameBoy::MBC3Read(const uint16_t& address)
{
	if (InRange(address, 0x0000, 0x3FFF))
	{
		//std::cout << "reading from rom bank 0" << std::endl;
		return m_Rom[address];
	}

	if (InRange(address, 0x4000, 0x7FFF))
	{
		//std::cout << "reading from switchable rom bank" << std::endl;

		const uint16_t address_into_bank = address - 0x4000;
		const uint16_t bank_offset = 0x4000 * ActiveRomRamBank.GetRomBank();
		const uint16_t address_in_rom = bank_offset + address_into_bank;
		return m_Rom[address_in_rom];
	}

	if (InRange(address, 0xA000, 0xBFFF) &&
		m_RamOverRtc)
	{
		//std::cout << "reading from RAM" << std::endl;

		const uint16_t address_into_bank = address - 0xA000;
		const uint16_t bank_offset = 0x8000 * ActiveRomRamBank.GetRamBank();
		const uint16_t address_in_ram = bank_offset + address_into_bank;
		return m_RamBanks[address_in_ram];
	}

	return m_Memory[address];
}

uint8_t GameBoy::MBC5Read(const uint16_t& address)
{
	if (InRange(address, 0x0000, 0x3FFF))
	{
		//std::cout << "reading from rom bank 0" << std::endl;
		return m_Rom[address];
	}

	auto idx = address + ((ActiveRomRamBank.GetRomBank() - 1) * 0x4000);
	if (InRange(address, 0x4000, 0x7FFF) && m_Rom.size() > idx)
		return m_Rom[idx];

	if (InRange(address, 0xA000, 0xBFFF))
	{
		//std::cout << "reading from RAM" << std::endl;

		const uint16_t address_into_bank = address - 0xA000;
		const uint16_t bank_offset = 0x8000 * ActiveRomRamBank.GetRamBank();
		const uint16_t address_in_ram = bank_offset + address_into_bank;
		return m_RamBanks[address_in_ram];
	}

	return m_Memory[address];
}

void GameBoy::MBCWrite(const uint16_t& address, const uint8_t byte)
{
	switch (m_Mbc)
	{
	case none:
		MBCNoneWrite(address, byte);
		break;
	case mbc1:
		MBC1Write(address, byte);
		break;
	case mbc2:
		break;
	case mbc3:
		//std::cout << "writing mbc3" << std::endl;
		MBC3Write(address, byte);
		break;
	case mbc4:
		break;
	case mbc5:
		MBC5Write(address, byte);
		break;
	case unknown:
		break;
	default:
		break;
	}
}

uint8_t GameBoy::MBCRead(const uint16_t& address)
{
	switch (m_Mbc)
	{
	case none:
		return MBCNoneRead(address);
		break;
	case mbc1:
		return MBC1Read(address);
		break;
	case mbc2:
		break;
	case mbc3:
		return MBC3Read(address);
		break;
	case mbc4:
		break;
	case mbc5:
		return MBC5Read(address);
		break;
	case unknown:
		break;
	default:
		return 0;
		break;
	}
	return 0;
}

void GameBoy::SwitchRomBank(uint8_t bank)
{
	ActiveRomRamBank.romBank = bank;
}

void GameBoy::SwitchRamBank(uint8_t bank)
{
	ActiveRomRamBank.ramBank = bank;
}