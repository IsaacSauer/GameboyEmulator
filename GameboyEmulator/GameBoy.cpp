#include "pch.h"
#include "GameBoy.h"
#include <fstream>
#include <iostream>
#include <time.h>
#include "opc_test/disassembler.h"
#include "CartridgeInfo.h"
#include "Tile.h"
#include "Measure.h"

GameBoy::GameBoy(const std::string& gameFile)
	: GameBoy{}
{
	//LoadGame(gameFile);
}

//values of banks needed / size etc retrieved from https://gbdev.gg8.se/wiki/articles/The_Cartridge_Header#0149_-_RAM_Size
void GameBoy::LoadGame(const std::string& gbFile)
{
	fileName = gbFile;

	std::ifstream file{ fileName, std::ios::binary };
	assert(file.good());

	file.seekg(0, std::ios::end);
	const std::ifstream::pos_type size{ file.tellg() };
	//std::cout << "rom size: " << size << std::endl;
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

	std::cout << "ram size: " << std::to_string(header.ramSizeValue) << std::endl;

	RamBankEnabled = header.ramSizeValue;

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

	RamBanks.resize(numberOfBanks * 0x8000);

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

		if (true)
		{
#pragma region Old
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
				while (IsRunning && !IsPaused && CurrentCycles < cycleBudget)
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
#pragma endregion
		}
		else
		{
			Measure measure{ "measurement" };

			const unsigned int cycleBudget{ static_cast<unsigned>(ceil(4194304.0f / fps)) * SpeedMultiplier };

			unsigned int stepCycles{ CurrentCycles };
			Cpu.ExecuteNextOpcode();
			stepCycles = CurrentCycles - stepCycles;

			HandleTimers(stepCycles, cycleBudget);

			measure.Start();
			Cpu.HandleGraphics(stepCycles, cycleBudget,
				!OnlyDrawLast || !(IsCycleFrameBound & 1) || IsCycleFrameBound & 1 && CyclesFramesToRun == 1);
			auto diff = measure.Stop();

			Cpu.HandleInterupts();

			if(diff > 500)
				std::cout << diff << std::endl;

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

void GameBoy::AssignDrawCallback(const std::function<void(const FrameBuffer&)>& _vblank_callback)
{
	Cpu.register_vblank_callback(_vblank_callback);
}

uint8_t GameBoy::ReadMemory(const uint16_t pos)
{
	if (m_TestingOpcodes)
		return (uint8_t)Cpu.mmu_read(pos);

	if (InRange(pos, 0x0, 0x7FFF) || //ROM bank
		InRange(pos, 0xA000, 0xBFFF)) //RAM Bank
		return MBCReadOptimal(pos);

	if (pos == 0xFF00) //Input
		return GetJoypadState();

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

	if (Mbc == mbc3)
	{
		if (InRange(address, 0x4000, 0x7fff))
		{
			MBCWriteOptimal(address, data);
			return;
		}

		//External (cartridge) ram
		if (InRange(address, 0xa000, 0xbfff))
			MBCWriteOptimal(address, data);
		return;
	}

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
			RamBanks[ActiveRomRamBank.GetRamBank() * 0x8000 + (address - 0xA000)] = data;
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
	Memory[address] = byte;
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
		std::cout << "toggling ram bank enable" << std::endl;

		if (byte == 0x0A)
			RamBankEnabled = true;

		if (byte == 0x00)
			RamBankEnabled = false;
	}

	if (InRange(address, 0x2000, 0x3FFF))
	{
		std::cout << "switching rom bank" << std::endl;

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
		std::cout << "switching ram bank" << std::endl;

		if (byte <= 0x03)
			RamOverRtc = true;

		if (byte >= 0x08 && byte <= 0xC)
			RamOverRtc = false;
	}

	if (InRange(address, 0xA000, 0xBFFF))
	{
		std::cout << "writing to RAM" << std::endl;

		if (!RamBankEnabled) { return; }

		if (RamOverRtc)
		{
			auto offset_into_ram = 0x8000 * ActiveRomRamBank.GetRamBank();
			auto address_in_ram = (address - 0xA000) + offset_into_ram;
			RamBanks[address_in_ram] = byte;
		}
	}
}

uint8_t GameBoy::MBCNoneRead(const uint16_t& address)
{
	if (address < Rom.size())
		return Rom[address];

	return 0;
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
	//std::cout << "value of ram bank: " << std::to_string(ActiveRomRamBank.GetRamBank()) << std::endl;

	if (InRange(address, 0x0000, 0x3FFF))
	{
		std::cout << "reading from rom bank 1" << std::endl;
		return Rom[address];
	}

	if (InRange(address, 0x4000, 0x7FFF))
	{
		std::cout << "reading from switchable rom bank" << std::endl;

		const uint16_t address_into_bank = address - 0x4000;
		const uint16_t bank_offset = 0x4000 * ActiveRomRamBank.GetRomBank();
		const uint16_t address_in_rom = bank_offset + address_into_bank;
		return Rom[address_in_rom];
	}

	if (InRange(address, 0xA000, 0xBFFF) &&
		RamOverRtc)
	{
			std::cout << "reading from RAM" << std::endl;

			const uint16_t address_into_bank = address - 0xA000;
			const uint16_t bank_offset = 0x8000 * ActiveRomRamBank.GetRamBank();
			const uint16_t address_in_ram = bank_offset + address_into_bank;
			return RamBanks[address_in_ram];
	}

	return Memory[address];
}

void GameBoy::MBCWriteOptimal(const uint16_t& address, const uint8_t byte)
{
	switch (Mbc)
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
		break;
	case unknown:
		break;
	default:
		break;
	}
}

uint8_t GameBoy::MBCReadOptimal(const uint16_t& address)
{
	switch (Mbc)
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
		break;
	case unknown:
		break;
	default:
		break;
	}
}

void GameBoy::SwitchRomBank(uint8_t bank)
{
	ActiveRomRamBank.romBank = bank;
}

void GameBoy::SwitchRamBank(uint8_t bank)
{

}
