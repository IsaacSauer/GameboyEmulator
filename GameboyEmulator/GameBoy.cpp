#include "pch.h"
#include "GameBoy.h"
#include <fstream>
#include <iostream>
#include <time.h>
#include "opc_test/disassembler.h"
#include "CartridgeInfo.h"

GameBoy::GameBoy(const std::string& gameFile)
	: GameBoy{}
{
	LoadGame(gameFile);
}

//values of banks needed / size etc retrieved from https://gbdev.gg8.se/wiki/articles/The_Cartridge_Header#0149_-_RAM_Size
void GameBoy::LoadGame(const std::string& gbFile)
{
	fileName = gbFile;

	std::ifstream file{ gbFile, std::ios::binary };
	assert(file.good());

	file.seekg(0, std::ios::end);
	const std::ifstream::pos_type size{ file.tellg() };
	std::cout << "rom size: " << size << std::endl;
	Rom.resize(size, 0);

	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(Rom.data()), size);

	file.close();

	////Disassmble required opcodes to txt file
	//Disassemble();

	auto cartridge = get_info(Rom);
	const GameHeader header{ ReadHeader() };
	Mbc = header.mbc;

	std::cout << "title of game: " << header.title << std::endl;

	using namespace std::placeholders;
	switch (Mbc)
	{
	case mbc1:
		MBCWrite = std::bind(&GameBoy::MBC1Write, this, _1, _2);
		MBCRead = std::bind(&GameBoy::MBC1Read, this, _1);
		break;
	case mbc2:
		break;
	case mbc3:
		MBCWrite = std::bind(&GameBoy::MBC3Write, this, _1, _2);
		MBCRead = std::bind(&GameBoy::MBC3Read, this, _1);
		break;
	case mbc4:
		break;
	case mbc5:
		break;
	case none:
		MBCWrite = std::bind(&GameBoy::MBCNoneWrite, this, _1, _2);
		MBCRead = std::bind(&GameBoy::MBCNoneRead, this, _1);
		break;
	}

	std::cout << "ram size: " << std::to_string(header.ramSizeValue) << std::endl;

	RamBankEnabled = header.ramSizeValue;
	switch (header.ramSizeValue)
	{
	case 0x00:
		RamBanks.resize(0x0);
		break;
	case 0x01:
		RamBanks.resize(0x800);
		break;
	case 0x02:
		RamBanks.resize(0x2000);
		break;
	case 0x03:
		RamBanks.resize(0x8000);
		break;
	case 0x04:
		RamBanks.resize(0x20000);
		break;
	case 0x05:
		RamBanks.resize(0x10000);
		break;
	default:
		RamBanks.resize(0);
		break;
	}

	std::cout << "memory bank: " << std::to_string(Mbc) << std::endl;
	Cpu.Reset();
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

	while (IsRunning)
	{
		if (m_Reset)
			LoadGame(fileName);

		const clock_t currentTicks = { clock() / (CLOCKS_PER_SEC / 1000) };

		//I'm keeping fps in mind to ensure a smooth simulation, if we make the cyclebudget infinite, we have 0 control
		if (!IsPaused && static_cast<float>(lastValidTick) + timeAdded < static_cast<float>(currentTicks))
		{
			if (AutoSpeed && static_cast<float>(currentTicks) - (static_cast<float>(lastValidTick) + timeAdded) >= .5f && SpeedMultiplier >= 2)
			{
				--SpeedMultiplier;
			}
			CurrentCycles = 0;

			//4194304 cycles can be done in a second (standard gameboy)
			const unsigned int cycleBudget{ static_cast<unsigned>(ceil(4194304.0f / fps)) * SpeedMultiplier };
			while (!IsPaused && CurrentCycles < cycleBudget)
			{
				unsigned int stepCycles{ CurrentCycles };
				Cpu.ExecuteNextOpcode();
				stepCycles = CurrentCycles - stepCycles;

				//If we're cycle bound, subtract cycles and call pause if needed
				if (IsCycleFrameBound & 2 && (CyclesFramesToRun - stepCycles > CyclesFramesToRun || !(CyclesFramesToRun -= stepCycles)))
				{
					IsCycleFrameBound = 0;
					IsPaused = true;
				}

				HandleTimers(stepCycles, cycleBudget);

				//Draw if we don't care, are not frame bound or are on the final frame
				Cpu.HandleGraphics(stepCycles, cycleBudget,
					!OnlyDrawLast || !(IsCycleFrameBound & 1) || IsCycleFrameBound & 1 && CyclesFramesToRun == 1);

				//If vblank interrupt and we're frame bound, subtract frames and call pause if needed
				if (/*interrupt_flag.value()*/GetIF() & 1 && IsCycleFrameBound & 1 && !--CyclesFramesToRun)
				{
					IsCycleFrameBound = 0;
					IsPaused = true;
				}

				Cpu.HandleInterupts();
			}
			lastValidTick = currentTicks;
			idle = false;
		}
		else if (!idle)
		{
			if (AutoSpeed && static_cast<float>(lastValidTick) + timeAdded - static_cast<float>(currentTicks) >= .5f)
			{
				++SpeedMultiplier;
			}
			idle = true;
		}
	}
}

GameHeader GameBoy::ReadHeader()
{
	GameHeader header{};
	memcpy(header.title, static_cast<void*>(Rom.data() + 0x134), 16);

	//Set MBC chip version
	switch (Rom[0x147])
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
		assert(Rom[0x147] == 0x0); //if not 0x0 it's undocumented
	}

	header.ramSizeValue = Rom[header::ram_size];

	return header;
}

void GameBoy::TestCPU()
{
	m_TestingOpcodes = true;
	Cpu.TestCPU();
	m_TestingOpcodes = false;
}

void GameBoy::Disassemble()
{
	//disassemble
	std::vector<std::string> opcodes{};
	for (int i{}; i < Rom.size(); ++i)
		disassemble(Rom.data() + i, opcodes);
	//distinct
	std::sort(opcodes.begin(), opcodes.end());
	opcodes.erase(std::unique(opcodes.begin(), opcodes.end()), opcodes.end());
	//write to file
	std::ofstream ofile{ fileName + "_opcodes.txt" };
	for (auto code : opcodes)
		ofile << code << '\n';
	ofile.close();
}

uint8_t GameBoy::ReadMemory(const uint16_t pos)
{
	if (m_TestingOpcodes)
		return (uint8_t)Cpu.mmu_read(pos);

	if (InRange(pos, 0x0, 0x7FFF)) //RAM Bank x
		return MBCRead(pos);

	if (pos <= 0x3FFF) //ROM Bank 0
		return Rom[pos];
	if (pos == 0xFF00)
		return GetJoypadState();
	if (pos - 0x4000 <= 0x7FFF - 0x4000) //ROM Bank x
		return MBCRead(pos);
	//return Rom[pos + ((ActiveRomRamBank.GetRomBank() - 1) * 0x4000)];
	if (InRange(pos, 0xA000, 0xBFFF)) //RAM Bank x
		return MBCRead(pos);
	//return RamBanks[(ActiveRomRamBank.GetRamBank() * 0x2000) + (pos - 0xA000)];

	return Memory[pos];
}

uint16_t GameBoy::ReadMemoryWord(uint16_t& pos)
{
	if (m_TestingOpcodes)
	{
		const uint16_t res{ static_cast<uint16_t>(static_cast<uint16_t>(Cpu.mmu_read(pos)) | static_cast<uint16_t>(Cpu.mmu_read(pos + 1)) << 8) };
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
		Cpu.mmu_write(address, data);
		return;
	}

	//if (Mbc != none)
	//{
	//	if (InRange(address, 0x0, 0x7fff))
	//		MBCWrite(address, data); return;

	//	//External (cartridge) ram
	//	if (InRange(address, 0xa000, 0xbfff))
	//		MBCWrite(address, data); return;
	//}

	if (address <= 0x1FFF) //Enable/Disable RAM
	{
		if (Mbc <= mbc1 || Mbc == mbc2 && !(address & 0x100))
			RamBankEnabled = (data & 0xF) == 0xA;
	}
	else if (InRange(address, static_cast<uint16_t>(0x2000), static_cast<uint16_t>(0x3FFF))) //5 bits;Lower 5 bits of ROM Bank
	{
		if (Mbc <= mbc1 || Mbc == mbc2 && address & 0x100)
			ActiveRomRamBank.romBank = (data ? data : 1) & 0x1F;
	}
	else if (InRange(address, static_cast<uint16_t>(0x4000), static_cast<uint16_t>(0x5FFF))) //2 bits;Ram or upper 2 bits of ROM bank
	{
		ActiveRomRamBank.ramOrRomBank = data;
	}
	else if (InRange(address, static_cast<uint16_t>(0x6000), static_cast<uint16_t>(0x7FFF))) //1 bit; Rom or Ram mode of ^
	{
		ActiveRomRamBank.isRam = data;
	}
	else if (InRange(address, static_cast<uint16_t>(0xA000), static_cast<uint16_t>(0xBFFF))) //External RAM
	{
		if (RamBankEnabled)
			RamBanks[ActiveRomRamBank.GetRamBank() * 0x2000 + (address - 0xA000)] = data;
	}
	else if (InRange(address, static_cast<uint16_t>(0xC000), static_cast<uint16_t>(0xDFFF))) //Internal RAM
	{
		Memory[address] = data;
	}
	else if (InRange(address, static_cast<uint16_t>(0xE000), static_cast<uint16_t>(0xFDFF))) //ECHO RAM
	{
		Memory[address] = data;
		Memory[address - 0x2000] = data;
	}
	else if (InRange(address, static_cast<uint16_t>(0xFEA0), static_cast<uint16_t>(0xFEFF))) //Mysterious Restricted Range
	{
	}
	else if (address == 0xFF04) //Reset DIV
	{
		DIVTimer = 0;
		DivCycles = 0;
	}
	else if (address == 0xFF07) //Set timer Clock speed
	{
		TACTimer = data & 0x7;
	}
	else if (address == 0xFF44)  //Horizontal scanline reset
	{
		Memory[address] = 0;
	}
	else if (address == 0xFF46)  //DMA transfer
	{
		const uint16_t src{ static_cast<uint16_t>(static_cast<uint16_t>(data) << 8) };
		for (int i{ 0 }; i < 0xA0; ++i)
			WriteMemory(static_cast<uint16_t>(0xFE00 + i), ReadMemory(static_cast<uint16_t>(src + i)));
	}
	else
		Memory[address] = data;
}

void GameBoy::WriteMemoryWord(const uint16_t pos, const uint16_t value)
{
	if (m_TestingOpcodes)
	{
		Cpu.mmu_write(pos, value & 0xFF);
		Cpu.mmu_write(pos + 1, value >> 8);
		return;
	}

	WriteMemory(pos, value & 0xFF);
	WriteMemory(pos + 1, value >> 8);
}

void GameBoy::RequestInterrupt(Interupts bit) noexcept
{
	//interrupt_flag.set_bit_to(bit, true);
	////interrupt_flag.set(interrupt_flag.value() | (1 << bit));
	////interrupt_flag |= 1 << bit;
	GetIF() |= 1 << bit;
}

void GameBoy::SetKey(const Key key, const bool pressed)
{
	if (pressed)
	{
		const uint8_t oldJoyPad{ JoyPadState };
		JoyPadState &= ~(1 << key);

		//Previosuly 1
		if (oldJoyPad & 1 << key)
		{
			if (!(Memory[0xff00] & 0x20) && !(key + 1 % 2)) //Button Keys
				RequestInterrupt(joypad);
			else if (!(Memory[0xff00] & 0x10) && !(key % 2)) //Directional keys
				RequestInterrupt(joypad);
		}
	}
	else
		JoyPadState |= (1 << key);
}

void GameBoy::HandleTimers(const unsigned stepCycles, const unsigned cycleBudget)
{
	// This function may be placed under the cpu class...

	//if this never breaks, which I highly doubt it will, change the type to uint8
	assert(stepCycles <= 0xFF);

	const unsigned cyclesOneDiv{ (cycleBudget / 16384) };

	//TODO: turn this into a while loop
	if ((DivCycles += stepCycles) >= cyclesOneDiv)
	{
		DivCycles -= cyclesOneDiv;
		++DIVTimer;
	}

	if (TACTimer & 0x4)
	{
		TIMACycles += stepCycles;

		unsigned int threshold{};
		switch (TACTimer & 0x3)
		{
		case 0:
			threshold = cycleBudget / 4096;
			break;
		case 1:
			threshold = cycleBudget / 262144;
			break;
		case 2:
			threshold = cycleBudget / 65536;
			break;
		case 3:
			threshold = cycleBudget / 16384;
			break;
		default:
			assert(true);
		}

		while (threshold != 0 && TIMACycles >= threshold)
		{
			if (!++TIMATimer)
			{
				TIMATimer = TMATimer;
				//interrupt_flag |= Interupts::timer;
				GetIF() |= 0x4;
			}
			TIMACycles -= threshold; //threshold == 0??
		}
	}
}

uint8_t GameBoy::GetJoypadState() const
{
	uint8_t res{ Memory[0xff00] };

	//Button keys
	if (!(res & 0x20))
	{
		res |= !!(JoyPadState & 1 << aButton);
		res |= !!(JoyPadState & 1 << bButton) << 1;
		res |= !!(JoyPadState & 1 << select) << 2;
		res |= !!(JoyPadState & 1 << start) << 3;
	}
	else if (!(res & 0x10))
	{
		res |= !!(JoyPadState & 1 << right);
		res |= !!(JoyPadState & 1 << left) << 1;
		res |= !!(JoyPadState & 1 << up) << 2;
		res |= !!(JoyPadState & 1 << down) << 3;
	}
	return res;
}

void GameBoy::MBCNoneWrite(const uint16_t& address, const uint8_t byte)
{
}

void GameBoy::MBC1Write(const uint16_t& address, const uint8_t byte)
{
	if (InRange(address, 0x0000, 0x1FFF))
		RamBankEnabled = true;

	if (InRange(address, 0x2000, 0x3FFF))
	{
		if (byte == 0x0) { ActiveRomRamBank.romBank = 0x1; }

		if (byte == 0x20) { ActiveRomRamBank.romBank = 0x21; return; }
		if (byte == 0x40) { ActiveRomRamBank.romBank = 0x41; return; }
		if (byte == 0x60) { ActiveRomRamBank.romBank = 0x61; return; }

		uint16_t rom_bank_bits = byte & 0x1F;
		ActiveRomRamBank.romBank = rom_bank_bits;
	}

	if (InRange(address, 0x4000, 0x5FFF))
		ActiveRomRamBank.ramOrRomBank = byte;

	if (InRange(address, 0x6000, 0x7FFF))
		ActiveRomRamBank.isRam = byte;

	if (InRange(address, 0xA000, 0xBFFF))
	{
		if (!RamBankEnabled) { return; }

		auto offset_into_ram = 0x2000 * ActiveRomRamBank.GetRamBank();
		auto address_in_ram = (address - 0xA000) + offset_into_ram;
		RamBanks[address_in_ram] = byte;
	}
}

void GameBoy::MBC3Write(const uint16_t& address, const uint8_t byte)
{
	if (InRange(address, 0x0000, 0x1FFF))
	{
		if (byte == 0x0A)
			RamBankEnabled = true;

		if (byte == 0x0)
			RamBankEnabled = false;
	}

	if (InRange(address, 0x2000, 0x3FFF))
	{
		if (byte == 0x0)
			ActiveRomRamBank.romBank = 0x1;

		uint16_t rom_bank_bits = byte & 0x7F;
		ActiveRomRamBank.romBank = rom_bank_bits;
	}

	if (InRange(address, 0x4000, 0x5FFF))
	{
		if (byte <= 0x03)
		{
			m_Ram_Over_Rtc = true;
			RamBanks[address] = byte;
		}

		if (byte >= 0x08 && byte <= 0xC)
			m_Ram_Over_Rtc = false;
	}

	if (InRange(address, 0xA000, 0xBFFF))
	{
		if (!RamBankEnabled) { return; }

		if (m_Ram_Over_Rtc)
		{
			auto offset_into_ram = 0x2000 * ActiveRomRamBank.GetRamBank();
			auto address_in_ram = (address - 0xA000) + offset_into_ram;
			RamBanks[address_in_ram] = byte;
		}
	}
}

uint8_t GameBoy::MBCNoneRead(const uint16_t& address)
{
	if(address < Rom.size())
		return Rom[address];
}

uint8_t GameBoy::MBC1Read(const uint16_t& address)
{
	if (InRange(address, 0x0000, 0x3FFF))
		return Rom[address];

	if (InRange(address, 0x4000, 0x7FFF))
		return Rom[address + ((ActiveRomRamBank.GetRomBank() - 1) * 0x4000)];

	if (InRange(address, 0xA000, 0xBFFF) && RamBankEnabled)
		return RamBanks[(ActiveRomRamBank.GetRamBank() * 0x2000) + (address - 0xA000)];

	return Memory[address];
}

uint8_t GameBoy::MBC3Read(const uint16_t& address)
{
	if (InRange(address, 0x0000, 0x3FFF))
		return Rom[address];

	if (InRange(address, 0x4000, 0x7FFF))
	{
		uint16_t address_into_bank = address - 0x4000;
		unsigned int bank_offset = 0x4000 * ActiveRomRamBank.GetRamBank();

		unsigned int address_in_rom = bank_offset + address_into_bank;
		return Rom[address_in_rom];
	}

	if (InRange(address, 0xA000, 0xBFFF))
	{
		auto offset_into_ram = 0x2000 * ActiveRomRamBank.GetRamBank();
		auto address_in_ram = (address - 0xA000) + offset_into_ram;
		return RamBanks[address_in_ram];
	}

	return uint8_t{};
}