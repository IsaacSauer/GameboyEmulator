#include "pch.h"
#include "LR35902.h"
#include "GameBoy.h"
#include "OpcodeDefinitions.cpp" //Inlines..

#include <fstream>
#include <vector>
#include <functional>

#ifdef _DEBUG
//#define VERBOSE
#endif
#include <iostream>

//#include "opc_test\tester.h"

static int cycles_per_instruction[] = {
	/* 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f       */
	   4, 12,  8,  8,  4,  4,  8,  4, 20,  8,  8,  8,  4,  4,  8,  4, /* 0 */
	   4, 12,  8,  8,  4,  4,  8,  4, 12,  8,  8,  8,  4,  4,  8,  4, /* 1 */
	   8, 12,  8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4, /* 2 */
	   8, 12,  8,  8, 12, 12, 12,  4,  8,  8,  8,  8,  4,  4,  8,  4, /* 3 */
	   4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, /* 4 */
	   4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, /* 5 */
	   4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, /* 6 */
	   8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4, /* 7 */
	   4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, /* 8 */
	   4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, /* 9 */
	   4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, /* a */
	   4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  8,  4,  8,  4, /* b */
	   8, 12, 12, 16, 12, 16,  8, 16,  8, 16, 12,  0, 12, 24,  8, 16, /* c */
	   8, 12, 12,  4, 12, 16,  8, 16,  8, 16, 12,  4, 12,  4,  8, 16, /* d */
	  12, 12,  8,  4,  4, 16,  8, 16, 16,  4, 16,  4,  4,  4,  8, 16, /* e */
	  12, 12,  8,  4,  4, 16,  8, 16, 12,  8, 16,  4,  0,  4,  8, 16, /* f */
};

static int cycles_per_instruction_cb[] = {
	/* 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f       */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* 0 */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* 1 */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* 2 */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* 3 */
	   8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8, /* 4 */
	   8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8, /* 5 */
	   8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8, /* 6 */
	   8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8, /* 7 */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* 8 */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* 9 */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* a */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* b */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* c */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* d */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* e */
	   8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, /* f */
};

LR35902::LR35902(GameBoy& gameboy) : Gameboy{ gameboy }
{
	//TODO: Investigate Threading Bottleneck
	/*for (uint8_t i{ 0 }; i < 10; ++i) {
		DrawThreads[i] = std::thread{&LR35902::ThreadWork, this, i, &DrawDataStruct};
	}*/

#ifdef _DEBUG
	m_RequiredOpcodes = std::vector<unsigned int>(0xff + 1);
#endif // _DEBUG
}

void LR35902::Reset(const bool skipBoot)
{
	//http://bgb.bircd.org/pandocs.htm#powerupsequence

	if (skipBoot)
	{
		//The state after executing the Boot rom
		Register.reg16.AF = 0x01b0;
		Register.reg16.BC = 0x0013;
		Register.reg16.DE = 0x00d8;
		Register.reg16.HL = 0x014D;

		Register.pc = 0x100; //First instruction after boot rom;
		Register.sp = 0xFFFE;

		//io and hram state
		uint8_t* memory{ Gameboy.GetRawMemory() };
		memory[0xFF05] = 0x00;
		memory[0xFF06] = 0x00;
		memory[0xFF07] = 0x00;
		memory[0xFF10] = 0x80;
		memory[0xFF11] = 0xBF;
		memory[0xFF12] = 0xF3;
		memory[0xFF14] = 0xBF;
		memory[0xFF16] = 0x3F;
		memory[0xFF17] = 0x00;
		memory[0xFF19] = 0xBF;
		memory[0xFF1A] = 0x7F;
		memory[0xFF1B] = 0xFF;
		memory[0xFF1C] = 0x9F;
		memory[0xFF1E] = 0xBF;
		memory[0xFF20] = 0xFF;
		memory[0xFF21] = 0x00;
		memory[0xFF22] = 0x00;
		memory[0xFF23] = 0xBF;
		memory[0xFF24] = 0x77;
		memory[0xFF25] = 0xF3;
		memory[0xFF26] = 0xF1;
		memory[0xFF40] = 0x91;
		memory[0xFF42] = 0x00;
		memory[0xFF43] = 0x00;
		memory[0xFF45] = 0x00;
		memory[0xFF47] = 0xFC;
		memory[0xFF48] = 0xFF;
		memory[0xFF49] = 0xFF;
		memory[0xFF4A] = 0x00;
		memory[0xFF4B] = 0x00;
		memory[0xFFFF] = 0x00;
	}
}

void LR35902::ExecuteNextOpcode()
{
	uint8_t opcode{ Gameboy.ReadMemory(Register.pc++) };

#ifdef VERBOSE
	static bool ok{};
	if (ok)
	{
		static std::ofstream file{ "log.txt",std::ios::trunc };
		file << std::to_string(Register.pc) << ", " << std::hex << '[' << LookUp[opcode] << ']' << int(opcode) << ", " << std::to_string(Register.f) << ", " <<
			std::to_string(Register.reg8.A) + ", " <<
			std::to_string(Register.reg8.B) + ", " <<
			std::to_string(Register.reg8.C) + ", " <<
			std::to_string(Register.reg8.D) + ", " <<
			std::to_string(Register.reg8.E) + ", " <<
			std::to_string(Register.reg8.H) + ", " <<
			std::to_string(Register.reg8.L) + ", " <<
			"IF:" + std::to_string(Gameboy.GetIF()) + ", " <<
			"SP:" + std::to_string(Gameboy.ReadMemory(Register.sp)) + ':' + std::to_string(Gameboy.ReadMemory(Register.sp + 1)) << std::endl;
	}
#endif

	if (opcode == 0xcb)
	{
		opcode = Gameboy.ReadMemory(Register.pc++);
		ExecuteOpcodeCB(opcode);
	}
	else
		ExecuteOpcode(opcode);

	if (InteruptChangePending)
	{
		if ((*(uint8_t*)&InteruptChangePending & 8) && Gameboy.ReadMemory(Register.pc - 1) != 0xFB)
		{
			InteruptsEnabled = true;
			InteruptChangePending = false;
		}
		else if ((*(uint8_t*)&InteruptChangePending & 1) && Gameboy.ReadMemory(Register.pc - 1) != 0xF3)
		{
			InteruptsEnabled = false;
			InteruptChangePending = false;
		}
	}
}

void LR35902::ExecuteNextOpcodeTest()
{
	uint8_t opcode{ (uint8_t)mmu_read(Register.pc++) };

#ifdef VERBOSE
	static bool ok{};
	if (ok)
	{
		static std::ofstream file{ "log.txt",std::ios::trunc };
		file << std::to_string(Register.pc) << ", " << std::hex << '[' << LookUp[opcode] << ']' << int(opcode) << ", " << std::to_string(Register.f) << ", " <<
			std::to_string(Register.reg8.A) + ", " <<
			std::to_string(Register.reg8.B) + ", " <<
			std::to_string(Register.reg8.C) + ", " <<
			std::to_string(Register.reg8.D) + ", " <<
			std::to_string(Register.reg8.E) + ", " <<
			std::to_string(Register.reg8.H) + ", " <<
			std::to_string(Register.reg8.L) + ", " <<
			"IF:" + std::to_string(Gameboy.GetIF()) + ", " <<
			"SP:" + std::to_string(Gameboy.ReadMemory(Register.sp)) + ':' + std::to_string(Gameboy.ReadMemory(Register.sp + 1)) << std::endl;
	}
#endif

	if (opcode == 0xcb)
	{
		opcode = (uint8_t)mmu_read(Register.pc++);
		ExecuteOpcodeCB(opcode);
	}
	else
		ExecuteOpcode(opcode);

	if (InteruptChangePending)
	{
		if ((*(uint8_t*)&InteruptChangePending & 8) && Gameboy.ReadMemory(Register.pc - 1) != 0xFB)
		{
			InteruptsEnabled = true;
			InteruptChangePending = false;
		}
		else if ((*(uint8_t*)&InteruptChangePending & 1) && Gameboy.ReadMemory(Register.pc - 1) != 0xF3)
		{
			InteruptsEnabled = false;
			InteruptChangePending = false;
		}
	}
}

void LR35902::TestCPU()
{
	tester_flags flags{};
	flags.keep_going_on_mismatch = 1;
	flags.enable_cb_instruction_testing = 1;
	flags.print_tested_instruction = 1;
	flags.print_verbose_inputs = 1;

	using namespace std::placeholders;
	tester_operations myops{};
	myops.init = std::bind(&LR35902::mycpu_init, this, _1, _2);
	myops.set_state = std::bind(&LR35902::mycpu_set_state, this, _1);;
	myops.get_state = std::bind(&LR35902::mycpu_get_state, this, _1);;
	myops.step = std::bind(&LR35902::mycpu_step, this, _1);

	tester_run(&flags, &myops);
	std::cout << "Test Finished!\n";
}

void LR35902::LogRequiredOpcodes()
{
}

void LR35902::HandleInterupts()
{
	const uint8_t ints{ static_cast<uint8_t>(Gameboy.GetIF() & Gameboy.GetIE()) };

	if (InteruptsEnabled && ints)
	{
		for (int bit{ 0 }; bit < 5; ++bit)
		{
			if ((ints >> bit) & 0x1)
			{
				Gameboy.WriteMemoryWord(Register.sp -= 2, Register.pc);
				switch (bit)
				{
				case 0: Register.pc = 0x40;
					break;//V-Blank
				case 1: Register.pc = 0x48;
					break;//LCD State
				case 2: Register.pc = 0x50;
					break;//Timer
				case 3: Register.pc = 0x58;
					break;//Serial
				case 4: Register.pc = 0x60;
					break;//Joypad
				}
				InteruptsEnabled = false;
				Gameboy.GetIF() &= ~(1 << bit);
				break;
			}
		}
	}
}

void LR35902::HandleGraphics(const unsigned cycles, const unsigned cycleBudget, const bool draw) noexcept
{
	const unsigned cyclesOneDraw{ 456 };
	ConfigureLCDStatus(); //This is why we can't "speed this up" in the traditional sense, games are sensitive to this

	if (Gameboy.GetLY() > 153)
		Gameboy.GetLY() = 0;

	if ((Gameboy.GetLCDC() & 0x80))
	{
		if ((LCDCycles += cycles) >= cyclesOneDraw) { //LCD enabled and we're at our cycle mark
			LCDCycles = 0;
			uint8_t dirtyLY;

			if ((dirtyLY = ++Gameboy.GetLY()) == 144)
			{
				Gameboy.RequestInterrupt(vBlank);
			}
			if (Gameboy.GetLY() > 153)
			{
				Gameboy.GetLY() = 0;
			}
			if (dirtyLY < 144 && draw)
			{
				DrawLine();
			}
		}
	}
}

//Yes, I know, visibility/debugging will suffer...
//Tho, once the macro has been proven to work, it works...
//Also, due to MSVC no longer ignoring trailing commas(VS2019) and refusing to parse ## or the _opt_ semantic, you have to use 2 commas when using the variadic part.. :/
//Note: These multi-line macros might be regarded as unsafe but I don't wanna risk branching so ima leave it like this, yes it's likely the compiler will optimize the if statement out but im not gonna risk it..
#define OPCYCLE(func, cycl) func; cycles = cycl; break
#define BASICOPS(A1, B1, C1, D1, E1, H1, L1, cycles, funcName, ...) \
	case A1: OPCYCLE( funcName( Register.reg8.A __VA_ARGS__), cycles ); \
	case B1: OPCYCLE( funcName( Register.reg8.B __VA_ARGS__), cycles ); \
	case C1: OPCYCLE( funcName( Register.reg8.C __VA_ARGS__), cycles ); \
	case D1: OPCYCLE( funcName( Register.reg8.D __VA_ARGS__), cycles ); \
	case E1: OPCYCLE( funcName( Register.reg8.E __VA_ARGS__), cycles ); \
	case H1: OPCYCLE( funcName( Register.reg8.H __VA_ARGS__), cycles ); \
	case L1: OPCYCLE( funcName( Register.reg8.L __VA_ARGS__), cycles )

uint8_t LR35902::ExecuteOpcode(uint8_t opcode)
{
	//assert(Gameboy.ReadMemory(Register.pc - 1) == opcode); //pc is pointing to first argument
	uint8_t cycles{ (uint8_t)cycles_per_instruction[opcode] };

	switch (opcode)
	{
	case 0x0: //Only advances the program counter by 1. Performs no other operations that would have an effect.
		cycles = 4;
		break;

#pragma region ALU
		/*
		- 0x8F: Add the contents of register A and the CY flag to the contents of register A, and store the results in register A.
		- 0x88: Add the contents of register B and the CY flag to the contents of register A, and store the results in register A.
		- 0x89: Add the contents of register C and the CY flag to the contents of register A, and store the results in register A.
		- 0x8A: Add the contents of register D and the CY flag to the contents of register A, and store the results in register A.
		- 0x8B: Add the contents of register E and the CY flag to the contents of register A, and store the results in register A.
		- 0x8C: Add the contents of register H and the CY flag to the contents of register A, and store the results in register A.
		- 0x8D: Add the contents of register L and the CY flag to the contents of register A, and store the results in register A.
		*/
		BASICOPS(0x8F, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 4, ADC, , true);

	case 0x8E: //Add the contents of memory specified by register pair HL and the CY flag to the contents of register A, and store the results in register A.
		OPCYCLE(ADC(Gameboy.ReadMemory(Register.reg16.HL), true), 8);
	case 0xCE: //Add the contents of the 8-bit immediate operand d8 and the CY flag to the contents of register A, and store the results in register A.
		OPCYCLE(ADC(Gameboy.ReadMemory(Register.pc++), true), 8);

	case 0x80: //Add the contents of register B to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.reg8.B), 4);
	case 0x81: //Add the contents of register C to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.reg8.C), 4);
	case 0x82: //Add the contents of register D to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.reg8.D), 4);
	case 0x83: //Add the contents of register E to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.reg8.E), 4);
	case 0x84: //Add the contents of register H to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.reg8.H), 4);
	case 0x85: //Add the contents of register L to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.reg8.L), 4);
	case 0x87: //Add the contents of register A to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.reg8.A), 4);
	case 0x86: //Add the contents of memory specified by register pair HL to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0xC6: //Add the contents of the 8-bit immediate operand d8 to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Gameboy.ReadMemory(Register.pc++)), 8);
	case 0xE8: //Add the contents of the 8-bit signed (2's complement) immediate operand s8 and the stack pointer SP and store the results in SP.
		OPCYCLE(ADD16(false, Gameboy.ReadMemory(Register.pc++)), 16); //Might have to add this value to the pc
	case 0x09: //Add the contents of register pair BC to the contents of register pair HL, and store the results in register pair HL.
		OPCYCLE(ADD16(true, Register.reg16.BC), 8);
	case 0x19: //Add the contents of register pair DE to the contents of register pair HL, and store the results in register pair HL.
		OPCYCLE(ADD16(true, Register.reg16.DE), 8);
	case 0x29: //Add the contents of register pair HL to the contents of register pair HL, and store the results in register pair HL.
		OPCYCLE(ADD16(true, Register.reg16.HL), 8);
	case 0x39: //Add the contents of register pair SP to the contents of register pair HL, and store the results in register pair HL.
		OPCYCLE(ADD16(true, Register.sp), 8);

	case 0xA0: //Take the logical AND for each bit of the contents of register B and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.reg8.B), 4);
	case 0xA1: //Take the logical AND for each bit of the contents of register C and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.reg8.C), 4);
	case 0xA2: //Take the logical AND for each bit of the contents of register D and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.reg8.D), 4);
	case 0xA3: //Take the logical AND for each bit of the contents of register E and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.reg8.E), 4);
	case 0xA4: //Take the logical AND for each bit of the contents of register H and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.reg8.H), 4);
	case 0xA5: //Take the logical AND for each bit of the contents of register L and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.reg8.L), 4);
	case 0xA6: //Take the logical AND for each bit of the contents of memory specified by register pair HL and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0xA7: //Take the logical AND for each bit of the contents of register A and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.reg8.A), 4);
	case 0xE6: //Take the logical OR for each bit of the contents of the 8-bit immediate operand d8 and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Gameboy.ReadMemory(Register.pc++)), 8);

	case 0xB8:
		//Compare the contents of register B and the contents of register A by calculating A - B, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.reg8.B), 4);
	case 0xB9:
		//Compare the contents of register C and the contents of register A by calculating A - C, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.reg8.C), 4);
	case 0xBA:
		//Compare the contents of register D and the contents of register A by calculating A - D, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.reg8.D), 4);
	case 0xBB:
		//Compare the contents of register E and the contents of register A by calculating A - E, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.reg8.E), 4);
	case 0xBC:
		//Compare the contents of register H and the contents of register A by calculating A - H, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.reg8.H), 4);
	case 0xBD:
		//Compare the contents of register L and the contents of register A by calculating A - L, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.reg8.L), 4);
	case 0xBE:
		//Compare the contents of memory specified by register pair HL and the contents of register A by calculating A - (HL), and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0xBF:
		//Compare the contents of register A and the contents of register A by calculating A - A, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.reg8.A), 4);
	case 0xFE:
		//Compare the contents of register A and the contents of the 8-bit immediate operand d8 by calculating A - d8, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Gameboy.ReadMemory(Register.pc++)), 8);

	case 0x05: //Decrement the contents of register B by 1.
		OPCYCLE(DEC(Register.reg8.B), 4);
	case 0x15: //Decrement the contents of register D by 1.
		OPCYCLE(DEC(Register.reg8.D), 4);
	case 0x25: //Decrement the contents of register H by 1.
		OPCYCLE(DEC(Register.reg8.H), 4);
	case 0x35: //Decrement the contents of memory specified by register pair HL by 1.
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		DEC(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 12;
	}
	break;
	case 0x0D: //Decrement the contents of register C by 1.
		OPCYCLE(DEC(Register.reg8.C), 4);
	case 0x1D: //Decrement the contents of register E by 1.
		OPCYCLE(DEC(Register.reg8.E), 4);
	case 0x2D: //Decrement the contents of register L by 1.
		OPCYCLE(DEC(Register.reg8.L), 4);
	case 0x3D: //Decrement the contents of register A by 1.
		OPCYCLE(DEC(Register.reg8.A), 4);
	case 0x0B: //Decrement the contents of register pair BC by 1.
	{
		uint16_t temp{ Register.reg16.BC };
		DEC(temp);
		cycles = 8;
		Register.reg16.BC = temp;
	}
	break;
	case 0x1B: //Decrement the contents of register pair DE by 1.
	{
		uint16_t temp{ Register.reg16.DE };
		DEC(temp);
		cycles = 8;
		Register.reg16.DE = temp;
	}
	break;
	case 0x2B: //Decrement the contents of register pair HL by 1.
	{
		uint16_t temp{ Register.reg16.HL };
		DEC(temp);
		cycles = 8;
		Register.reg16.HL = temp;
	}
	break;
	case 0x3B: //Decrement the contents of register pair SP by 1.
		OPCYCLE(DEC(Register.sp), 8);

	case 0x04: //Increment the contents of register B by 1.
		OPCYCLE(INC(Register.reg8.B), 4);
	case 0x14: //Increment the contents of register D by 1.
		OPCYCLE(INC(Register.reg8.D), 4);
	case 0x24: //Increment the contents of register H by 1.
		OPCYCLE(INC(Register.reg8.H), 4);
	case 0x34: //Increment the contents of memory specified by register pair HL by 1.
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		INC(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 12;
	}
	break;
	case 0x0C: //Increment the contents of register C by 1.
		OPCYCLE(INC(Register.reg8.C), 4);
	case 0x1C: //Increment the contents of register E by 1.
		OPCYCLE(INC(Register.reg8.E), 4);
	case 0x2C: //Increment the contents of register L by 1.
		OPCYCLE(INC(Register.reg8.L), 4);
	case 0x3C: //Increment the contents of register A by 1.
		OPCYCLE(INC(Register.reg8.A), 4);
	case 0x03: //Increment the contents of register pair BC by 1.
	{
		uint16_t temp{ Register.reg16.BC };
		INC(temp);
		cycles = 8;
		Register.reg16.BC = temp;
	}
	break;
	case 0x13: //Increment the contents of register pair DE by 1.
	{
		uint16_t temp{ Register.reg16.DE };
		INC(temp);
		cycles = 8;
		Register.reg16.DE = temp;
	}
	break;
	case 0x23: //Increment the contents of register pair HL by 1.
	{
		uint16_t temp{ Register.reg16.HL };
		INC(temp);
		cycles = 8;
		Register.reg16.HL = temp;
	}
	break;
	case 0x33: //Increment the contents of register pair SP by 1.
		OPCYCLE(INC(Register.sp), 8);

		BASICOPS(0xB7, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 4, OR);
	case 0xB6:
		//Take the logical OR for each bit of the contents of memory specified by register pair HL and the contents of register A, //and store the results in register A.
		OPCYCLE(OR(Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0xF6:
		//Take the logical OR for each bit of the contents of the 8-bit immediate operand d8 and the contents of register A, //and store the results in register A.
		OPCYCLE(OR(Gameboy.ReadMemory(Register.pc++)), 8);

		/*
		-0x9F: Subtract the contents of register A and the CY flag from the contents of register A, and store the results in register A.
		-0x98: Subtract the contents of register B and the CY flag from the contents of register A, and store the results in register A.
		-0x99: Subtract the contents of register C and the CY flag from the contents of register A, and store the results in register A.
		-0x9A: Subtract the contents of register D and the CY flag from the contents of register A, and store the results in register A.
		-0x9B: Subtract the contents of register E and the CY flag from the contents of register A, and store the results in register A.
		-0x9C: Subtract the contents of register H and the CY flag from the contents of register A, and store the results in register A.
		-0x9D: Subtract the contents of register L and the CY flag from the contents of register A, and store the results in register A.
		*/
		BASICOPS(0x9F, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 4, SBC, , true);

	case 0x9E:
		//Subtract the contents of memory specified by register pair HL and the carry flag CY from the contents of register A, //and store the results in register A.
		OPCYCLE(SBC(Gameboy.ReadMemory(Register.reg16.HL), true), 8);

	case 0xDE:
		//Subtract the contents of the 8-bit immediate operand d8 and the carry flag CY from the contents of register A, //and store the results in register A.
		OPCYCLE(SBC(Gameboy.ReadMemory(Register.pc++)), 8);

		/*
		-0x97: Subtract the contents of register A from the contents of register A, and store the results in register A.
		-0x90: Subtract the contents of register B from the contents of register A, and store the results in register A.
		-0x91: Subtract the contents of register C from the contents of register A, and store the results in register A.
		-0x92: Subtract the contents of register D from the contents of register A, and store the results in register A.
		-0x93: Subtract the contents of register E from the contents of register A, and store the results in register A.
		-0x94: Subtract the contents of register H from the contents of register A, and store the results in register A.
		-0x95: Subtract the contents of register L from the contents of register A, and store the results in register A.
		*/
		BASICOPS(0x97, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 4, SUB);

	case 0x96: //Subtract the contents of memory specified by register pair HL from the contents of register A, and store the results in register A.
		OPCYCLE(SUB(Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0xD6:
		OPCYCLE(SUB(Gameboy.ReadMemory(Register.pc++)), 8);

		BASICOPS(0xAF, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 4, XOR);

	case 0xAE:
		OPCYCLE(XOR(Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0xEE:
		OPCYCLE(XOR(Gameboy.ReadMemory(Register.pc++)), 8);

	case 0x27:
		OPCYCLE(DAA(), 4);
#pragma endregion
#pragma region Loads
	case 0x2a:
		OPCYCLE((LD(Register.reg8.A, Gameboy.ReadMemory(Register.reg16.HL)), Register.reg16.HL = Register.reg16.HL + 1), 8);
	case 0x3a:
		OPCYCLE((LD(Register.reg8.A, Gameboy.ReadMemory(Register.reg16.HL)), Register.reg16.HL = Register.reg16.HL - 1), 8);
	case 0xfa:
		OPCYCLE(LD(Register.reg8.A, Gameboy.ReadMemory(Gameboy.ReadMemoryWord(Register.pc))), 16);
	case 0x22:
		OPCYCLE((LD(Register.reg16.HL, Register.reg8.A), Register.reg16.HL = Register.reg16.HL + 1), 8);
	case 0x32:
		OPCYCLE((LD(Register.reg16.HL, Register.reg8.A), Register.reg16.HL = Register.reg16.HL - 1), 8);
	case 0x36:
		OPCYCLE(LD(Register.reg16.HL, Gameboy.ReadMemory(Register.pc++)), 12);
	case 0xea:
	{
		const uint16_t adrs{ Gameboy.ReadMemoryWord(Register.pc) };
		OPCYCLE(LD(adrs, Register.reg8.A), 16);
	}
	case 0x08:
	{
		Gameboy.WriteMemoryWord(Gameboy.ReadMemoryWord(Register.pc), Register.sp);
		cycles = 20;
	}
	break;
	case 0x31:
		OPCYCLE(LD(&Register.sp, Gameboy.ReadMemoryWord(Register.pc)), 12);
	case 0xf9:
		OPCYCLE(LD(&Register.sp, Register.reg16.HL), 8);
	case 0x02:
		OPCYCLE(LD(Register.reg16.BC, Register.reg8.A), 8);
	case 0x12:
		OPCYCLE(LD(Register.reg16.DE, Register.reg8.A), 8);
	case 0x06:
		OPCYCLE(LD(Register.reg8.B, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x11:
	{
		uint16_t temp{ Register.reg16.DE };
		LD(&temp, Gameboy.ReadMemoryWord(Register.pc));
		cycles = 12;
		Register.reg16.DE = temp;
	}
	break;
	/*
	Load the 2 bytes of immediate data into register pair BC.

	The first byte of immediate data is the lower byte (i.e., bits 0-7),
	and the second byte of immediate data is the higher byte (i.e., bits 8-15).
	*/
	case 0x01:
	{
		uint16_t temp{ Register.reg16.BC };
		LD(&temp, Gameboy.ReadMemoryWord(Register.pc));
		Register.reg16.BC = temp;
		cycles = 12;
	}
	break;
	case 0x21:
	{
		uint16_t temp{ Register.reg16.HL };
		LD(&temp, Gameboy.ReadMemoryWord(Register.pc));
		cycles = 12;
		Register.reg16.HL = temp;
	}
	break;
	case 0xf8:
	{
		uint16_t temp{ Register.reg16.HL };
		LD(&temp, Register.sp + (int8_t)Gameboy.ReadMemory(Register.pc++));
		cycles = 12; //CodeSlinger:0
		Register.reg16.HL = temp;
	}
	break;
	case 0x0a:
		OPCYCLE(LD(Register.reg8.A, Gameboy.ReadMemory(Register.reg16.BC)), 8);
	case 0x0e:
		OPCYCLE(LD(Register.reg8.C, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x16:
		OPCYCLE(LD(Register.reg8.D, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x1a:
		OPCYCLE(LD(Register.reg8.A, Gameboy.ReadMemory(Register.reg16.DE)), 8);
	case 0x1e:
		OPCYCLE(LD(Register.reg8.E, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x26:
		OPCYCLE(LD(Register.reg8.H, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x2e:
		OPCYCLE(LD(Register.reg8.L, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x3e:
		OPCYCLE(LD(Register.reg8.A, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x40:
		OPCYCLE(LD(Register.reg8.B, Register.reg8.B), 4);
	case 0x41:
		OPCYCLE(LD(Register.reg8.B, Register.reg8.C), 4);
	case 0x42:
		OPCYCLE(LD(Register.reg8.B, Register.reg8.D), 4);
	case 0x43:
		OPCYCLE(LD(Register.reg8.B, Register.reg8.E), 4);
	case 0x44:
		OPCYCLE(LD(Register.reg8.B, Register.reg8.H), 4);
	case 0x45:
		OPCYCLE(LD(Register.reg8.B, Register.reg8.L), 4);
	case 0x46:
		OPCYCLE(LD(Register.reg8.B, Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0x47:
		OPCYCLE(LD(Register.reg8.B, Register.reg8.A), 4);
	case 0x48:
		OPCYCLE(LD(Register.reg8.C, Register.reg8.B), 4);
	case 0x49:
		OPCYCLE(LD(Register.reg8.C, Register.reg8.C), 4);
	case 0x4a:
		OPCYCLE(LD(Register.reg8.C, Register.reg8.D), 4);
	case 0x4b:
		OPCYCLE(LD(Register.reg8.C, Register.reg8.E), 4);
	case 0x4c:
		OPCYCLE(LD(Register.reg8.C, Register.reg8.H), 4);
	case 0x4d:
		OPCYCLE(LD(Register.reg8.C, Register.reg8.L), 4);
	case 0x4e:
		OPCYCLE(LD(Register.reg8.C, Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0x4f:
		OPCYCLE(LD(Register.reg8.C, Register.reg8.A), 4);
	case 0x50:
		OPCYCLE(LD(Register.reg8.D, Register.reg8.B), 4);
	case 0x51:
		OPCYCLE(LD(Register.reg8.D, Register.reg8.C), 4);
	case 0x52:
		OPCYCLE(LD(Register.reg8.D, Register.reg8.D), 4);
	case 0x53:
		OPCYCLE(LD(Register.reg8.D, Register.reg8.E), 4);
	case 0x54:
		OPCYCLE(LD(Register.reg8.D, Register.reg8.H), 4);
	case 0x55:
		OPCYCLE(LD(Register.reg8.D, Register.reg8.L), 4);
	case 0x56:
		OPCYCLE(LD(Register.reg8.D, Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0x57:
		OPCYCLE(LD(Register.reg8.D, Register.reg8.A), 4);
	case 0x58:
		OPCYCLE(LD(Register.reg8.E, Register.reg8.B), 4);
	case 0x59:
		OPCYCLE(LD(Register.reg8.E, Register.reg8.C), 4);
	case 0x5a:
		OPCYCLE(LD(Register.reg8.E, Register.reg8.D), 4);
	case 0x5b:
		OPCYCLE(LD(Register.reg8.E, Register.reg8.E), 4);
	case 0x5c:
		OPCYCLE(LD(Register.reg8.E, Register.reg8.H), 4);
	case 0x5d:
		OPCYCLE(LD(Register.reg8.E, Register.reg8.L), 4);
	case 0x5e:
		OPCYCLE(LD(Register.reg8.E, Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0x5f:
		OPCYCLE(LD(Register.reg8.E, Register.reg8.A), 4);
	case 0x60:
		OPCYCLE(LD(Register.reg8.H, Register.reg8.B), 4);
	case 0x61:
		OPCYCLE(LD(Register.reg8.H, Register.reg8.C), 4);
	case 0x62:
		OPCYCLE(LD(Register.reg8.H, Register.reg8.D), 4);
	case 0x63:
		OPCYCLE(LD(Register.reg8.H, Register.reg8.E), 4);
	case 0x64:
		OPCYCLE(LD(Register.reg8.H, Register.reg8.H), 4);
	case 0x65:
		OPCYCLE(LD(Register.reg8.H, Register.reg8.L), 4);
	case 0x66:
		OPCYCLE(LD(Register.reg8.H, Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0x67:
		OPCYCLE(LD(Register.reg8.H, Register.reg8.A), 4);
	case 0x68:
		OPCYCLE(LD(Register.reg8.L, Register.reg8.B), 4);
	case 0x69:
		OPCYCLE(LD(Register.reg8.L, Register.reg8.C), 4);
	case 0x6a:
		OPCYCLE(LD(Register.reg8.L, Register.reg8.D), 4);
	case 0x6b:
		OPCYCLE(LD(Register.reg8.L, Register.reg8.E), 4);
	case 0x6c:
		OPCYCLE(LD(Register.reg8.L, Register.reg8.H), 4);
	case 0x6d:
		OPCYCLE(LD(Register.reg8.L, Register.reg8.L), 4);
	case 0x6e:
		OPCYCLE(LD(Register.reg8.L, Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0x6f:
		OPCYCLE(LD(Register.reg8.L, Register.reg8.A), 4);
	case 0x78:
		OPCYCLE(LD(Register.reg8.A, Register.reg8.B), 4);
	case 0x79:
		OPCYCLE(LD(Register.reg8.A, Register.reg8.C), 4);
	case 0x7a:
		OPCYCLE(LD(Register.reg8.A, Register.reg8.D), 4);
	case 0x7b:
		OPCYCLE(LD(Register.reg8.A, Register.reg8.E), 4);
	case 0x7c:
		OPCYCLE(LD(Register.reg8.A, Register.reg8.H), 4);
	case 0x7d:
		OPCYCLE(LD(Register.reg8.A, Register.reg8.L), 4);
	case 0x7e:
		OPCYCLE(LD(Register.reg8.A, Gameboy.ReadMemory(Register.reg16.HL)), 8);
	case 0x7f:
		OPCYCLE(LD(Register.reg8.A, Register.reg8.A), 4);
	case 0xf2:
		OPCYCLE(LD(Register.reg8.A, Gameboy.ReadMemory(0xFF00 + Register.reg8.C)), 8); //ok acc to gb manual
	case 0x70:
		OPCYCLE(LD(Register.reg16.HL, Register.reg8.B), 8);
	case 0x71:
		OPCYCLE(LD(Register.reg16.HL, Register.reg8.C), 8);
	case 0x72:
		OPCYCLE(LD(Register.reg16.HL, Register.reg8.D), 8);
	case 0x73:
		OPCYCLE(LD(Register.reg16.HL, Register.reg8.E), 8);
	case 0x74:
		OPCYCLE(LD(Register.reg16.HL, Register.reg8.H), 8);
	case 0x75:
		OPCYCLE(LD(Register.reg16.HL, Register.reg8.L), 8);
	case 0x77:
		OPCYCLE(LD(Register.reg16.HL, Register.reg8.A), 8);
	case 0xe2:
		OPCYCLE(LD(static_cast<uint16_t>(0xFF00 + Register.reg8.C), Register.reg8.A), 8);
	case 0xE0:
		OPCYCLE(LD(static_cast<uint16_t>(0xFF00 + Gameboy.ReadMemory(Register.pc++)), Register.reg8.A), 12);
	case 0xF0:
	{
		//if(Register.pc==10667) __debugbreak();
		const uint8_t val{ Gameboy.ReadMemory(Register.pc++) };
		OPCYCLE(LD(Register.reg8.A, Gameboy.ReadMemory(static_cast<uint16_t>(0xFF00 + val))), 12);
	}
	break;
	case 0xC1:
	{
		uint16_t temp;
		POP(temp);
		Register.reg16.BC = temp;
		cycles = 12;
	}
	break;
	case 0xD1:
	{
		uint16_t temp;
		POP(temp);
		Register.reg16.DE = temp;
		cycles = 12;
	}
	break;
	case 0xE1:
	{
		uint16_t temp;
		POP(temp);
		Register.reg16.HL = temp;
		cycles = 12;
	}
	break;
	case 0xF1:
	{
		uint16_t temp;
		POP(temp);
		Register.reg16.AF = temp;
		cycles = 12;
	}
	break;

	case 0xC5:
		OPCYCLE(PUSH(Register.reg16.BC), 16);
	case 0xD5:
		OPCYCLE(PUSH(Register.reg16.DE), 16);
	case 0xE5:
		OPCYCLE(PUSH(Register.reg16.HL), 16);
	case 0xF5:
		OPCYCLE(PUSH(Register.reg16.AF), 16);

#pragma endregion
#pragma region Rotates and Shifts
	case 0x07:
		/*
		Rotate the contents of register A to the left.
		That is, the contents of bit 0 are copied to bit 1,
		and the previous contents of bit 1 (before the copy operation) are copied to bit 2.
		The same operation is repeated in sequence for the rest of the register.
		The contents of bit 7 are placed in both the CY flag and bit 0 of register A.
		*/
		OPCYCLE(RLC(Register.reg8.A), 4); //Codeslinger: 8

		//case 0x0F:
		//	/*
		//	Rotate the contents of register A to the right.
		//	That is, the contents of bit 7 are copied to bit 6,
		//	and the previous contents of bit 6 (before the copy) are copied to bit 5.
		//	The same operation is repeated in sequence for the rest of the register.
		//	The contents of bit 0 are placed in both the CY flag and bit 7 of register A.
		//	*/
		//	OPCYCLE(RRC(Register.reg8.A), 4); //Codeslinger: 8

		//case 0x17:
		//	/*
		//	Rotate the contents of register A to the right, through the carry (CY) flag.
		//	That is, the contents of bit 7 are copied to bit 6,
		//	and the previous contents of bit 6 (before the copy) are copied to bit 5.
		//	The same operation is repeated in sequence for the rest of the register.
		//	The previous contents of the carry flag are copied to bit 7.
		//	*/
		//	OPCYCLE(RL(Register.reg8.A), 4); //Codeslinger: 8

	case 0x1F:
		/*
		Rotate the contents of register A to the right, through the carry (CY) flag.
		That is, the contents of bit 7 are copied to bit 6,
		and the previous contents of bit 6 (before the copy) are copied to bit 5.
		The same operation is repeated in sequence for the rest of the register.
		The previous contents of the carry flag are copied to bit 7.
		*/
		OPCYCLE(RR(Register.reg8.A), 4); //Codeslinger: 8

#pragma endregion
#pragma region Misc
	case 0x2F:
		OPCYCLE(CPL(), 4);
	case 0x3F:
		OPCYCLE(CCF(), 4);
	case 0x37:
		OPCYCLE(SCF(), 4);
	case 0x10:
		OPCYCLE(STOP(), 4);
	case 0x76:
		OPCYCLE(HALT(), 4);
	case 0xF3:
		OPCYCLE(DI(), 4);
	case 0xFB:
		OPCYCLE(EI(), 4);
#pragma endregion
#pragma region Calls n Jumps
	case 0xC4:
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), !Register.flags.ZF), 0);
	case 0xD4:
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), !Register.flags.CF), 0);
	case 0xCC:
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), Register.flags.ZF), 0);
	case 0xDC:
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), Register.flags.CF), 0);
	case 0xCD:
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), true), 0);;
	case 0xC2:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), !Register.flags.ZF), 0);
	case 0xD2:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), !Register.flags.CF), 0);
	case 0xC3:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), true), 0);
	case 0xE9:
		OPCYCLE(JP(Register.reg16.HL, true, false), 4);
	case 0xCA:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), Register.flags.ZF), 0);
	case 0xDA:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), Register.flags.CF), 0);;
	case 0x20:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), !Register.flags.ZF), 0);
	case 0x30:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), !Register.flags.CF), 0);
	case 0x18:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), true), 0);
	case 0x28:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), Register.flags.ZF), 0);
	case 0x38:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), Register.flags.CF), 0);

	case 0xC0:
		OPCYCLE(RET(!Register.flags.ZF), 0);
	case 0xD0:
		OPCYCLE(RET(!Register.flags.CF), 0);
	case 0xC8:
		OPCYCLE(RET(Register.flags.ZF), 0);
	case 0xD8:
		OPCYCLE(RET(Register.flags.CF), 0);
	case 0xC9:
		OPCYCLE(RET(true, false), 16); //CodeSlinger:8
	case 0xD9:
		OPCYCLE(RETI(), 16); //CodeSlinger:8

	case 0xC7:
		OPCYCLE(RST(0x0), 16); //CodeSlinger:32 (all)
	case 0xD7:
		OPCYCLE(RST(0x10), 16);
	case 0xE7:
		OPCYCLE(RST(0x20), 16);
	case 0xF7:
		OPCYCLE(RST(0x30), 16);
	case 0xCF:
		OPCYCLE(RST(0x08), 16);
	case 0xDF:
		OPCYCLE(RST(0x18), 16);
	case 0xEF:
		OPCYCLE(RST(0x28), 16);
	case 0xFF:
		OPCYCLE(RST(0x38), 16);
#pragma endregion
		//following opcodes were not in the original implementation of the emulator
#pragma region missing opcodes
	case 0x0F: OPCYCLE(RRC(Register.reg8.A), 4);
	case 0x17: OPCYCLE(RL(Register.reg8.A), 4);
#pragma endregion
#ifdef _DEBUG
	default:
		//TODO: cancel execution and write all missing opcodes to a log file.
		//assert(("Opcode not implemented", false));
		//assert(false);
		break;
#endif
	}

	//	uint8_t   x;
	//	switch (opcode)
	//	{
	//#include "lr35902/opc_main.hxx"
	//	default:
	//
	//		break;
	//	}
	Gameboy.AddCycles((uint8_t)cycles_per_instruction[opcode]);

	//std::cout << "cycles this opcode took: " << std::to_string(cycles) << std::endl;

	return cycles;
}

uint8_t LR35902::ExecuteOpcodeCB(uint8_t opcode)
{
	//assert(Gameboy.ReadMemory(Register.pc - 1) == opcode); //pc is pointing to first argument
	uint8_t cycles{ (uint8_t)cycles_per_instruction_cb[opcode] };

	switch (opcode)
	{
#pragma region Rotates and Shifts
	case 0x18:
		OPCYCLE(RR(Register.reg8.B), 8);
	case 0x19:
		OPCYCLE(RR(Register.reg8.C), 8);
	case 0x1A:
		OPCYCLE(RR(Register.reg8.D), 8);
	case 0x1B:
		OPCYCLE(RR(Register.reg8.E), 8);
	case 0x1C:
		OPCYCLE(RR(Register.reg8.H), 8);
	case 0x1D:
		OPCYCLE(RR(Register.reg8.L), 8);
	case 0x1E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RR(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x1F:
		OPCYCLE(RR(Register.reg8.A), 8);
	case 0x20:
		OPCYCLE(SLA(Register.reg8.B), 8);
	case 0x21:
		OPCYCLE(SLA(Register.reg8.C), 8);
	case 0x22:
		OPCYCLE(SLA(Register.reg8.D), 8);
	case 0x23:
		OPCYCLE(SLA(Register.reg8.E), 8);
	case 0x24:
		OPCYCLE(SLA(Register.reg8.H), 8);
	case 0x25:
		OPCYCLE(SLA(Register.reg8.L), 8);
	case 0x26:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SLA(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x27:
		OPCYCLE(SLA(Register.reg8.A), 8);
	case 0x38:
		OPCYCLE(SRL(Register.reg8.B), 8);
	case 0x39:
		OPCYCLE(SRL(Register.reg8.C), 8);
	case 0x3A:
		OPCYCLE(SRL(Register.reg8.D), 8);
	case 0x3B:
		OPCYCLE(SRL(Register.reg8.E), 8);
	case 0x3C:
		OPCYCLE(SRL(Register.reg8.H), 8);
	case 0x3D:
		OPCYCLE(SRL(Register.reg8.L), 8);
	case 0x3E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SRL(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x3F:
		OPCYCLE(SRL(Register.reg8.A), 8);
	case 0x00:
		OPCYCLE(RLC(Register.reg8.B), 8);
	case 0x01:
		OPCYCLE(RLC(Register.reg8.C), 8);
	case 0x02:
		OPCYCLE(RLC(Register.reg8.D), 8);
	case 0x03:
		OPCYCLE(RLC(Register.reg8.E), 8);
	case 0x04:
		OPCYCLE(RLC(Register.reg8.H), 8);
	case 0x05:
		OPCYCLE(RLC(Register.reg8.L), 8);
	case 0x06:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RLC(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x07:
		OPCYCLE(RLC(Register.reg8.A), 8);
#pragma endregion
#pragma region Bits
	case 0x40:
		OPCYCLE(BITop(0, Register.reg8.B), 8);
	case 0x41:
		OPCYCLE(BITop(0, Register.reg8.C), 8);
	case 0x42:
		OPCYCLE(BITop(0, Register.reg8.D), 8);
	case 0x43:
		OPCYCLE(BITop(0, Register.reg8.E), 8);
	case 0x44:
		OPCYCLE(BITop(0, Register.reg8.H), 8);
	case 0x45:
		OPCYCLE(BITop(0, Register.reg8.L), 8);
	case 0x46:
	{
		BITop(0, Gameboy.ReadMemory(Register.reg16.HL));
		cycles = 16;
	}
	break;
	case 0x47:
		OPCYCLE(BITop(0, Register.reg8.A), 8);
	case 0x48:
		OPCYCLE(BITop(1, Register.reg8.B), 8);
	case 0x49:
		OPCYCLE(BITop(1, Register.reg8.C), 8);
	case 0x4a:
		OPCYCLE(BITop(1, Register.reg8.D), 8);
	case 0x4b:
		OPCYCLE(BITop(1, Register.reg8.E), 8);
	case 0x4c:
		OPCYCLE(BITop(1, Register.reg8.H), 8);
	case 0x4d:
		OPCYCLE(BITop(1, Register.reg8.L), 8);
	case 0x4e:
	{
		BITop(1, Gameboy.ReadMemory(Register.reg16.HL));
		cycles = 16;
	}
	break;
	case 0x4f:
		OPCYCLE(BITop(1, Register.reg8.A), 8);
	case 0x50:
		OPCYCLE(BITop(2, Register.reg8.B), 8);
	case 0x51:
		OPCYCLE(BITop(2, Register.reg8.C), 8);
	case 0x52:
		OPCYCLE(BITop(2, Register.reg8.D), 8);
	case 0x53:
		OPCYCLE(BITop(2, Register.reg8.E), 8);
	case 0x54:
		OPCYCLE(BITop(2, Register.reg8.H), 8);
	case 0x55:
		OPCYCLE(BITop(2, Register.reg8.L), 8);
	case 0x56:
	{
		BITop(2, Gameboy.ReadMemory(Register.reg16.HL));
		cycles = 16;
	}
	break;
	case 0x57:
		OPCYCLE(BITop(2, Register.reg8.A), 8);
	case 0x58:
		OPCYCLE(BITop(3, Register.reg8.B), 8);
	case 0x59:
		OPCYCLE(BITop(3, Register.reg8.C), 8);
	case 0x5a:
		OPCYCLE(BITop(3, Register.reg8.D), 8);
	case 0x5b:
		OPCYCLE(BITop(3, Register.reg8.E), 8);
	case 0x5c:
		OPCYCLE(BITop(3, Register.reg8.H), 8);
	case 0x5d:
		OPCYCLE(BITop(3, Register.reg8.L), 8);
	case 0x5e:
	{
		BITop(3, Gameboy.ReadMemory(Register.reg16.HL));
		cycles = 16;
	}
	break;
	case 0x5f:
		OPCYCLE(BITop(3, Register.reg8.A), 8);
	case 0x60:
		OPCYCLE(BITop(4, Register.reg8.B), 8);
	case 0x61:
		OPCYCLE(BITop(4, Register.reg8.C), 8);
	case 0x62:
		OPCYCLE(BITop(4, Register.reg8.D), 8);
	case 0x63:
		OPCYCLE(BITop(4, Register.reg8.E), 8);
	case 0x64:
		OPCYCLE(BITop(4, Register.reg8.H), 8);
	case 0x65:
		OPCYCLE(BITop(4, Register.reg8.L), 8);
	case 0x66:
	{
		BITop(4, Gameboy.ReadMemory(Register.reg16.HL));
		cycles = 16;
	}
	break;
	case 0x67:
		OPCYCLE(BITop(4, Register.reg8.A), 8);
	case 0x68:
		OPCYCLE(BITop(5, Register.reg8.B), 8);
	case 0x69:
		OPCYCLE(BITop(5, Register.reg8.C), 8);
	case 0x6a:
		OPCYCLE(BITop(5, Register.reg8.D), 8);
	case 0x6b:
		OPCYCLE(BITop(5, Register.reg8.E), 8);
	case 0x6c:
		OPCYCLE(BITop(5, Register.reg8.H), 8);
	case 0x6d:
		OPCYCLE(BITop(5, Register.reg8.L), 8);
	case 0x6e:
	{
		BITop(5, Gameboy.ReadMemory(Register.reg16.HL));
		cycles = 16;
	}
	break;
	case 0x6f:
		OPCYCLE(BITop(5, Register.reg8.A), 8);
	case 0x70:
		OPCYCLE(BITop(6, Register.reg8.B), 8);
	case 0x71:
		OPCYCLE(BITop(6, Register.reg8.C), 8);
	case 0x72:
		OPCYCLE(BITop(6, Register.reg8.D), 8);
	case 0x73:
		OPCYCLE(BITop(6, Register.reg8.E), 8);
	case 0x74:
		OPCYCLE(BITop(6, Register.reg8.H), 8);
	case 0x75:
		OPCYCLE(BITop(6, Register.reg8.L), 8);
	case 0x76:
	{
		BITop(6, Gameboy.ReadMemory(Register.reg16.HL));
		cycles = 16;
	}
	break;
	case 0x77:
		OPCYCLE(BITop(6, Register.reg8.A), 8);
	case 0x78:
		OPCYCLE(BITop(7, Register.reg8.B), 8);
	case 0x79:
		OPCYCLE(BITop(7, Register.reg8.C), 8);
	case 0x7a:
		OPCYCLE(BITop(7, Register.reg8.D), 8);
	case 0x7b:
		OPCYCLE(BITop(7, Register.reg8.E), 8);
	case 0x7c:
		OPCYCLE(BITop(7, Register.reg8.H), 8);
	case 0x7d:
		OPCYCLE(BITop(7, Register.reg8.L), 8);
	case 0x7e:
	{
		BITop(7, Gameboy.ReadMemory(Register.reg16.HL));
		cycles = 16;
	}
	break;
	case 0x7f:
		OPCYCLE(BITop(7, Register.reg8.A), 8);

	case 0x80:
		OPCYCLE(RES(0, Register.reg8.B), 8);
	case 0x81:
		OPCYCLE(RES(0, Register.reg8.C), 8);
	case 0x82:
		OPCYCLE(RES(0, Register.reg8.D), 8);
	case 0x83:
		OPCYCLE(RES(0, Register.reg8.E), 8);
	case 0x84:
		OPCYCLE(RES(0, Register.reg8.H), 8);
	case 0x85:
		OPCYCLE(RES(0, Register.reg8.L), 8);
	case 0x86:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RES(0, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x87:
		OPCYCLE(RES(0, Register.reg8.A), 8);
	case 0x88:
		OPCYCLE(RES(1, Register.reg8.B), 8);
	case 0x89:
		OPCYCLE(RES(1, Register.reg8.C), 8);
	case 0x8a:
		OPCYCLE(RES(1, Register.reg8.D), 8);
	case 0x8b:
		OPCYCLE(RES(1, Register.reg8.E), 8);
	case 0x8c:
		OPCYCLE(RES(1, Register.reg8.H), 8);
	case 0x8d:
		OPCYCLE(RES(1, Register.reg8.L), 8);
	case 0x8e:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RES(1, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x8f:
		OPCYCLE(RES(1, Register.reg8.A), 8);
	case 0x90:
		OPCYCLE(RES(2, Register.reg8.B), 8);
	case 0x91:
		OPCYCLE(RES(2, Register.reg8.C), 8);
	case 0x92:
		OPCYCLE(RES(2, Register.reg8.D), 8);
	case 0x93:
		OPCYCLE(RES(2, Register.reg8.E), 8);
	case 0x94:
		OPCYCLE(RES(2, Register.reg8.H), 8);
	case 0x95:
		OPCYCLE(RES(2, Register.reg8.L), 8);
	case 0x96:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RES(2, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x97:
		OPCYCLE(RES(2, Register.reg8.A), 8);
	case 0x98:
		OPCYCLE(RES(3, Register.reg8.B), 8);
	case 0x99:
		OPCYCLE(RES(3, Register.reg8.C), 8);
	case 0x9a:
		OPCYCLE(RES(3, Register.reg8.D), 8);
	case 0x9b:
		OPCYCLE(RES(3, Register.reg8.E), 8);
	case 0x9c:
		OPCYCLE(RES(3, Register.reg8.H), 8);
	case 0x9d:
		OPCYCLE(RES(3, Register.reg8.L), 8);
	case 0x9e:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RES(3, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x9f:
		OPCYCLE(RES(3, Register.reg8.A), 8);
	case 0xa0:
		OPCYCLE(RES(4, Register.reg8.B), 8);
	case 0xa1:
		OPCYCLE(RES(4, Register.reg8.C), 8);
	case 0xa2:
		OPCYCLE(RES(4, Register.reg8.D), 8);
	case 0xa3:
		OPCYCLE(RES(4, Register.reg8.E), 8);
	case 0xa4:
		OPCYCLE(RES(4, Register.reg8.H), 8);
	case 0xa5:
		OPCYCLE(RES(4, Register.reg8.L), 8);
	case 0xa6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RES(4, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xa7:
		OPCYCLE(RES(4, Register.reg8.A), 8);
	case 0xa8:
		OPCYCLE(RES(5, Register.reg8.B), 8);
	case 0xa9:
		OPCYCLE(RES(5, Register.reg8.C), 8);
	case 0xaa:
		OPCYCLE(RES(5, Register.reg8.D), 8);
	case 0xab:
		OPCYCLE(RES(5, Register.reg8.E), 8);
	case 0xac:
		OPCYCLE(RES(5, Register.reg8.H), 8);
	case 0xad:
		OPCYCLE(RES(5, Register.reg8.L), 8);
	case 0xae:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RES(5, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xaf:
		OPCYCLE(RES(5, Register.reg8.A), 8);
	case 0xb0:
		OPCYCLE(RES(6, Register.reg8.B), 8);
	case 0xb1:
		OPCYCLE(RES(6, Register.reg8.C), 8);
	case 0xb2:
		OPCYCLE(RES(6, Register.reg8.D), 8);
	case 0xb3:
		OPCYCLE(RES(6, Register.reg8.E), 8);
	case 0xb4:
		OPCYCLE(RES(6, Register.reg8.H), 8);
	case 0xb5:
		OPCYCLE(RES(6, Register.reg8.L), 8);
	case 0xb6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RES(6, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xb7:
		OPCYCLE(RES(6, Register.reg8.A), 8);
	case 0xb8:
		OPCYCLE(RES(7, Register.reg8.B), 8);
	case 0xb9:
		OPCYCLE(RES(7, Register.reg8.C), 8);
	case 0xba:
		OPCYCLE(RES(7, Register.reg8.D), 8);
	case 0xbb:
		OPCYCLE(RES(7, Register.reg8.E), 8);
	case 0xbc:
		OPCYCLE(RES(7, Register.reg8.H), 8);
	case 0xbd:
		OPCYCLE(RES(7, Register.reg8.L), 8);
	case 0xbe:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RES(7, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xbf:
		OPCYCLE(RES(7, Register.reg8.A), 8);

	case 0xc0:
		OPCYCLE(SET(0, Register.reg8.B), 8);
	case 0xc1:
		OPCYCLE(SET(0, Register.reg8.C), 8);
	case 0xc2:
		OPCYCLE(SET(0, Register.reg8.D), 8);
	case 0xc3:
		OPCYCLE(SET(0, Register.reg8.E), 8);
	case 0xc4:
		OPCYCLE(SET(0, Register.reg8.H), 8);
	case 0xc5:
		OPCYCLE(SET(0, Register.reg8.L), 8);
	case 0xc6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SET(0, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xc7:
		OPCYCLE(SET(0, Register.reg8.A), 8);
	case 0xc8:
		OPCYCLE(SET(1, Register.reg8.B), 8);
	case 0xc9:
		OPCYCLE(SET(1, Register.reg8.C), 8);
	case 0xca:
		OPCYCLE(SET(1, Register.reg8.D), 8);
	case 0xcb:
		OPCYCLE(SET(1, Register.reg8.E), 8);
	case 0xcc:
		OPCYCLE(SET(1, Register.reg8.H), 8);
	case 0xcd:
		OPCYCLE(SET(1, Register.reg8.L), 8);
	case 0xce:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SET(1, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xcf:
		OPCYCLE(SET(1, Register.reg8.A), 8);
	case 0xd0:
		OPCYCLE(SET(2, Register.reg8.B), 8);
	case 0xd1:
		OPCYCLE(SET(2, Register.reg8.C), 8);
	case 0xd2:
		OPCYCLE(SET(2, Register.reg8.D), 8);
	case 0xd3:
		OPCYCLE(SET(2, Register.reg8.E), 8);
	case 0xd4:
		OPCYCLE(SET(2, Register.reg8.H), 8);
	case 0xd5:
		OPCYCLE(SET(2, Register.reg8.L), 8);
	case 0xd6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SET(2, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xd7:
		OPCYCLE(SET(2, Register.reg8.A), 8);
	case 0xd8:
		OPCYCLE(SET(3, Register.reg8.B), 8);
	case 0xd9:
		OPCYCLE(SET(3, Register.reg8.C), 8);
	case 0xda:
		OPCYCLE(SET(3, Register.reg8.D), 8);
	case 0xdb:
		OPCYCLE(SET(3, Register.reg8.E), 8);
	case 0xdc:
		OPCYCLE(SET(3, Register.reg8.H), 8);
	case 0xdd:
		OPCYCLE(SET(3, Register.reg8.L), 8);
	case 0xde:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SET(3, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xdf:
		OPCYCLE(SET(3, Register.reg8.A), 8);
	case 0xe0:
		OPCYCLE(SET(4, Register.reg8.B), 8);
	case 0xe1:
		OPCYCLE(SET(4, Register.reg8.C), 8);
	case 0xe2:
		OPCYCLE(SET(4, Register.reg8.D), 8);
	case 0xe3:
		OPCYCLE(SET(4, Register.reg8.E), 8);
	case 0xe4:
		OPCYCLE(SET(4, Register.reg8.H), 8);
	case 0xe5:
		OPCYCLE(SET(4, Register.reg8.L), 8);
	case 0xe6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SET(4, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xe7:
		OPCYCLE(SET(4, Register.reg8.A), 8);
	case 0xe8:
		OPCYCLE(SET(5, Register.reg8.B), 8);
	case 0xe9:
		OPCYCLE(SET(5, Register.reg8.C), 8);
	case 0xea:
		OPCYCLE(SET(5, Register.reg8.D), 8);
	case 0xeb:
		OPCYCLE(SET(5, Register.reg8.E), 8);
	case 0xec:
		OPCYCLE(SET(5, Register.reg8.H), 8);
	case 0xed:
		OPCYCLE(SET(5, Register.reg8.L), 8);
	case 0xee:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SET(5, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xef:
		OPCYCLE(SET(5, Register.reg8.A), 8);
	case 0xf0:
		OPCYCLE(SET(6, Register.reg8.B), 8);
	case 0xf1:
		OPCYCLE(SET(6, Register.reg8.C), 8);
	case 0xf2:
		OPCYCLE(SET(6, Register.reg8.D), 8);
	case 0xf3:
		OPCYCLE(SET(6, Register.reg8.E), 8);
	case 0xf4:
		OPCYCLE(SET(6, Register.reg8.H), 8);
	case 0xf5:
		OPCYCLE(SET(6, Register.reg8.L), 8);
	case 0xf6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SET(6, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xf7:
		OPCYCLE(SET(6, Register.reg8.A), 8);
	case 0xf8:
		OPCYCLE(SET(7, Register.reg8.B), 8);
	case 0xf9:
		OPCYCLE(SET(7, Register.reg8.C), 8);
	case 0xfa:
		OPCYCLE(SET(7, Register.reg8.D), 8);
	case 0xfb:
		OPCYCLE(SET(7, Register.reg8.E), 8);
	case 0xfc:
		OPCYCLE(SET(7, Register.reg8.H), 8);
	case 0xfd:
		OPCYCLE(SET(7, Register.reg8.L), 8);
	case 0xfe:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SET(7, hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0xff:
		OPCYCLE(SET(7, Register.reg8.A), 8);
#pragma endregion
#pragma region MISC
	case 0x30:
		OPCYCLE(SWAP(Register.reg8.B), 8);
	case 0x31:
		OPCYCLE(SWAP(Register.reg8.C), 8);
	case 0x32:
		OPCYCLE(SWAP(Register.reg8.D), 8);
	case 0x33:
		OPCYCLE(SWAP(Register.reg8.E), 8);
	case 0x34:
		OPCYCLE(SWAP(Register.reg8.H), 8);
	case 0x35:
		OPCYCLE(SWAP(Register.reg8.L), 8);
	case 0x36:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SWAP(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x37:
		OPCYCLE(SWAP(Register.reg8.A), 8);
#pragma endregion
#pragma region missing opcodes
	case 0x08: OPCYCLE(RRC(Register.reg8.B), 8);
	case 0x09: OPCYCLE(RRC(Register.reg8.C), 8);
	case 0x0A: OPCYCLE(RRC(Register.reg8.D), 8);
	case 0x0B: OPCYCLE(RRC(Register.reg8.E), 8);
	case 0x0C: OPCYCLE(RRC(Register.reg8.H), 8);
	case 0x0D: OPCYCLE(RRC(Register.reg8.L), 8);
	case 0x0E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RRC(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x0F: OPCYCLE(RRC(Register.reg8.A), 8);

	case 0x10: OPCYCLE(RL(Register.reg8.B), 8);
	case 0x11: OPCYCLE(RL(Register.reg8.C), 8);
	case 0x12: OPCYCLE(RL(Register.reg8.D), 8);
	case 0x13: OPCYCLE(RL(Register.reg8.E), 8);
	case 0x14: OPCYCLE(RL(Register.reg8.H), 8);
	case 0x15: OPCYCLE(RL(Register.reg8.L), 8);
	case 0x16:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		RL(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x17: OPCYCLE(RL(Register.reg8.A), 8);

	case 0x28: OPCYCLE(SRA(Register.reg8.B), 8);
	case 0x29: OPCYCLE(SRA(Register.reg8.C), 8);
	case 0x2A: OPCYCLE(SRA(Register.reg8.D), 8);
	case 0x2B: OPCYCLE(SRA(Register.reg8.E), 8);
	case 0x2C: OPCYCLE(SRA(Register.reg8.H), 8);
	case 0x2D: OPCYCLE(SRA(Register.reg8.L), 8);
	case 0x2E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.reg16.HL) };
		SRA(hlDeRef);
		Gameboy.WriteMemory(Register.reg16.HL, hlDeRef);
		cycles = 16;
	}
	break;
	case 0x2F: OPCYCLE(SRA(Register.reg8.A), 8);
#pragma endregion
#ifdef _DEBUG
	default:
		assert(("Opcode with prefix 0xCB not implemented", false));
		assert(false);
		break;
#endif
	}

	//	uint8_t   x;
	//	switch (opcode)
	//	{
	//#include "lr35902/opc_cb.hxx"
	//	default:
	//
	//		break;
	//	}
	Gameboy.AddCycles((uint8_t)cycles_per_instruction_cb[opcode]);

	//std::cout << "cycles this opcode took: " << std::to_string(cycles) << std::endl;

	return cycles;
}

void LR35902::ConfigureLCDStatus()
{
	if (!(Gameboy.GetLCDC() & 0x80))
	{
		//LCD Disabled
		Gameboy.GetLY() = LCDCycles = 0;
		Gameboy.GetLCDS() = ((Gameboy.GetLCDS() & 0xFC) | 1); //Set mode to vblank
	}

	uint8_t oldMode{ static_cast<uint8_t>(Gameboy.GetLCDS() & 3) };
	if (Gameboy.GetLY() >= 144)
	{
		//Mode 1 VBlank >4
		Gameboy.GetLCDS() &= 0xFC;
		Gameboy.GetLCDS() |= 0x1;
		oldMode |= (static_cast<bool>(Gameboy.GetLCDS() & 0x10) << 2); //can we interrupt?
	}
	else if (LCDCycles <= 80)
	{
		//Mode 2 OAM >5
		Gameboy.GetLCDS() &= 0xFC;
		Gameboy.GetLCDS() |= 0x2;
		oldMode |= (static_cast<bool>(Gameboy.GetLCDS() & 0x20) << 2);
	}
	else if (LCDCycles <= 252)
	{ //80+172
	 //Mode DataTransfer 3
		Gameboy.GetLCDS() |= 0x3;
	}
	else
	{
		//Mode Hblank 0
		Gameboy.GetLCDS() &= 0xFC;
		oldMode |= (static_cast<bool>(Gameboy.GetLCDS() & 8) << 2);
	}

	if (oldMode != (Gameboy.GetLCDC() & 3) && (oldMode & 8)) //Our mode changed and we can interrupt
		Gameboy.RequestInterrupt(lcdStat);

	if (Gameboy.GetLY() == Gameboy.GetRawMemory()[0xFF45])
	{
		//Compare LY to LYC to see if we're requesting the current line
		Gameboy.GetLCDS() |= 4;
		if (Gameboy.GetLCDS() & 0x40)
			Gameboy.RequestInterrupt(lcdStat);
	}
	else
		Gameboy.GetLCDS() &= ~(static_cast<unsigned>(1) << 2);
}

void LR35902::DrawLine() const
{
	DrawBackground();
	DrawWindow(); //window == ui
	DrawSprites();
}

void LR35902::DrawBackground() const
{
	assert(Gameboy.GetLCDC() & 0x80);

	const uint16_t tileSetAdress{ static_cast<uint16_t>(static_cast<bool>(Gameboy.GetLCDC() & 16) ? 0x8000 : 0x8800) };
	const uint16_t tileMapAddress{ static_cast<uint16_t>(static_cast<bool>(Gameboy.GetLCDC() & 8) ? 0x9C00 : 0x9800) };
	const uint8_t scrollX{ Gameboy.ReadMemory(0xFF43) }; //scroll

	const uint8_t yWrap{ static_cast<uint8_t>((Gameboy.ReadMemory(0xFF42) + Gameboy.GetLY()) % 256) };
	const uint8_t tileY{ static_cast<uint8_t>(yWrap % 8) }; //The y pixel of the tile
	const uint16_t tileMapOffset{ static_cast<uint16_t>(tileMapAddress + static_cast<uint16_t>(yWrap / 8) * 32) };
	const uint16_t fbOffset{ static_cast<uint16_t>((Gameboy.GetLY() - 1) * 160) };
	uint8_t colors[4];
	ConfigureColorArray(colors, Gameboy.ReadMemory(0xFF47));

	std::bitset<160 * 144 * 2>& fBuffer{ Gameboy.GetFramebuffer() };
	//TODO: Invesitgate Threading Bottleneck..
	/*DrawData data{colors, &Gameboy.GetFramebuffer(),tileSetAdress, tileMapOffset, fbOffset,scrollX, tileY, 0}; //DOUBLE DATA!

	DrawDataStruct = &data;
	//std::cout << "Starting Draw\n";
	ActivateDrawers.notify_all();
	std::unique_lock<std::mutex> mtx{ConditionalVariableMutex};

	ActivateDrawers.wait( mtx, [&data](){return (data.doneCounter&0x3FF)==0x3ff;} );
	//std::cout << "Draw DONE!\n";
	DrawDataStruct = nullptr;*/

#pragma loop(hint_parallel(10))
	for (uint8_t x{ 0 }; x < 160; ++x)
	{
		const uint8_t xWrap{ static_cast<uint8_t>((x + scrollX) % 256) };
		const uint16_t tileNumAddress{ static_cast<uint16_t>(tileMapOffset + (xWrap / 8)) };
		const uint16_t tileData{ static_cast<uint16_t>(tileSetAdress + Gameboy.ReadMemory(tileNumAddress) * 16) };

		const uint8_t paletteResult{ ReadPalette(tileData, static_cast<uint8_t>(xWrap % 8), tileY) };
		fBuffer[(fbOffset + x) * 2] = (colors[paletteResult] & 2);
		fBuffer[((fbOffset + x) * 2) + 1] = (colors[paletteResult] & 1);
	}
}

void LR35902::DrawWindow() const
{
	if (!(Gameboy.GetLCDC() & 32)) return; //window disabled

	const uint8_t wndX{ Gameboy.ReadMemory(0xFF4B) }; //scroll
	const uint8_t wndY{ Gameboy.ReadMemory(0xFF4A) };

	if (wndX < 7 || wndX >= (160 + 7) || wndY < 0 || wndY >= 144)
		return;
	if (wndY > Gameboy.GetLY())
		return;

	const uint16_t tileSetAdress{ static_cast<uint16_t>(static_cast<bool>(Gameboy.GetLCDC() & 16) ? 0x8000 : 0x8800) }; //If 0x8800 then the tile identifier is in signed TileSET
	const uint16_t tileMapAddress{ static_cast<uint16_t>(static_cast<bool>(Gameboy.GetLCDC() & 64) ? 0x9C00 : 0x9800) }; //TileMAP
	const uint8_t yWrap{ static_cast<uint8_t>((wndY + Gameboy.GetLY()) % 256) };
	const uint8_t tileY{ static_cast<uint8_t>(yWrap % 8) }; //The y pixel of the tile
	const uint16_t tileRow{ static_cast<uint16_t>(yWrap / 8) };
	const uint16_t tileMapOffset{ static_cast<uint16_t>(tileMapAddress + tileRow * 32) };
	const uint8_t fbOffset{ static_cast<uint8_t>((Gameboy.GetLY() - 1) * 160) };

	uint8_t colors[4];
	ConfigureColorArray(colors, Gameboy.ReadMemory(0xFF47));
#pragma loop(hint_parallel(20))
	for (uint8_t x{ static_cast<uint8_t>((wndX < 0) ? 0 : wndX) }; x < 160; ++x)
	{
		const uint8_t tileX{ static_cast<uint8_t>(x % 8) }; //The x pixel of the tile
		const uint16_t tileColumn{ static_cast<uint16_t>(x / 8) };

		const uint16_t tileNumAddress{ static_cast<uint16_t>(tileMapOffset + tileColumn) };
		const int16_t tileNumber{ Gameboy.ReadMemory(tileNumAddress) };

		const uint16_t tileAddress{ static_cast<uint16_t>(tileSetAdress + tileNumber * 16) }; //todo: Verify

		const uint8_t paletteResult{ ReadPalette(tileAddress, tileX, tileY) };
		Gameboy.GetFramebuffer()[(fbOffset + x) * 2] = (colors[paletteResult] & 2);
		Gameboy.GetFramebuffer()[((fbOffset + x) * 2) + 1] = (colors[paletteResult] & 1);
	}
}

void LR35902::DrawSprites() const
{
	if (Gameboy.GetLCDC() & 2)
	{
		//todo: support 8x16
#pragma loop(hint_parallel(20))
		for (uint16_t oamAddress{ 0xFE9C }; oamAddress >= 0xFE00; oamAddress -= 4)
		{
			const uint8_t yCoord{ static_cast<uint8_t>(Gameboy.ReadMemory(oamAddress) - 0x10) };
			const uint8_t xCoord{ static_cast<uint8_t>(Gameboy.ReadMemory(static_cast<int16_t>(oamAddress + 1)) - 0x8) };

			if (xCoord >= 160 || yCoord >= 144)
				continue;

			uint8_t tileY{ static_cast<uint8_t>(Gameboy.GetLY() - yCoord) };
			if (yCoord > Gameboy.GetLY() || tileY >= 8)
				continue;

			const uint16_t spriteNum{ Gameboy.ReadMemory(static_cast<uint16_t>(oamAddress + 2)) };
			const uint16_t optionsByte{ Gameboy.ReadMemory(static_cast<uint16_t>(oamAddress + 3)) };
			const bool xFlip{ (optionsByte & 0x20) != 0 };
			const bool yFlip{ (optionsByte & 0x40) != 0 };
			const bool priority{ (optionsByte & 0x80) != 0 };
			const uint16_t paletteAddress{ static_cast<uint16_t>(((optionsByte & 0x10) > 0) ? 0xFF49 : 0xff48) };
			const uint8_t palette{ Gameboy.ReadMemory(paletteAddress) };

			if (yFlip)
				tileY = (7 - tileY);
			const uint16_t tileAddress{ static_cast<uint16_t>(0x8000 + 0x10 * spriteNum) };
			for (uint8_t x{ 0 }; x < 8; ++x)
			{
				if (xCoord + x < 0 || xCoord + x >= 160) //outside range
					continue;

				const uint8_t tileX{ static_cast<uint8_t>(xFlip ? (7 - x) : x) };
				const uint8_t paletteIndex{ ReadPalette(tileAddress, tileX, tileY) };
				uint8_t spriteColors[4];
				uint8_t bgColors[4];

				ConfigureColorArray(spriteColors, palette);
				ConfigureColorArray(bgColors, Gameboy.ReadMemory(0xFF47));

				//White is transparent
				if (spriteColors[paletteIndex] > 0)
				{
					const int16_t i{ static_cast<int16_t>((Gameboy.GetLY() - 1) * 160 + xCoord + x) };

					//if the pixel has priority or the underlying pixel is not "black"
					if (priority || (Gameboy.GetPixelColor(i) != bgColors[3]))
					{
						Gameboy.GetFramebuffer()[i * 2] = spriteColors[paletteIndex] & 2;
						Gameboy.GetFramebuffer()[i * 2 + 1] = spriteColors[paletteIndex] & 1;
					}
				}
			}
		}
	}
}

uint8_t LR35902::ReadPalette(const uint16_t pixelData, const uint8_t xPixel, const uint8_t yPixel) const
{
	const uint8_t low{ Gameboy.ReadMemory(pixelData + (yPixel * 2)) };
	const uint8_t hi{ Gameboy.ReadMemory(pixelData + (yPixel * 2) + 1) };

	return ((hi >> (7 - xPixel) & 1) << 1) | ((low >> (7 - xPixel)) & 1);
}

void LR35902::ConfigureColorArray(uint8_t* const colorArray, uint8_t palette) const
{
	colorArray[0] = palette & 0x3;
	palette >>= 2;
	colorArray[1] = palette & 0x3;
	palette >>= 2;
	colorArray[2] = palette & 0x3;
	palette >>= 2;
	colorArray[3] = palette & 0x3;
}

//TODO: Investigate Threading Bottleneck..
/*void LR35902::ThreadWork( const uint8_t id, LR35902::DrawData **const drawData ) {
	std::unique_lock<std::mutex> mtx{ ConditionalVariableMutex };

	while (true) {
		ActivateDrawers.
			wait( mtx, [&drawData, id]()
			{
				return (*drawData && !((*drawData)->doneCounter & (1 << id)));
			} ); //needs mutex to not miss the signal/check condition; Checks if this worker's bit is already set
		//std::cout << int(id)<<" Started\n";
		mtx.unlock(); //Let the other threads do their thing

		const uint8_t fin{ uint8_t( id * 16 + 16 ) };
		std::bitset<160 * 144 * 2> *fBuffer{ (std::bitset<160 * 144 * 2>*)(*drawData)->bitset };
		for (uint8_t x{ uint8_t( id * 16 ) }; x < fin; ++x) {
			const uint8_t xWrap{ uint8_t( (x + (*drawData)->scrollX) % 256 ) };
			const uint16_t tileNumAddress{ uint16_t( (*drawData)->tileMapOffset + (xWrap / 8) ) };

			const uint16_t tileData{ uint16_t( (*drawData)->tileSetAdress + Gameboy.ReadMemory( tileNumAddress ) * 16 ) }; //todo: Verify

			const uint8_t paletteResult{ ReadPalette( tileData, uint8_t( xWrap % 8 ), (*drawData)->tileY ) };
			(*fBuffer)[((*drawData)->fbOffset + x) * 2] = ((*drawData)->colors[paletteResult] & 2);
			(*fBuffer)[(((*drawData)->fbOffset + x) * 2) + 1] = ((*drawData)->colors[paletteResult] & 1);
		}

		(*drawData)->doneCounter |= (1 << id);
		mtx.lock(); //If not, possible deadlock
		//std::cout << int(id)<<" Released\n";
		ActivateDrawers.notify_all();
	}
}*/

void LR35902::mycpu_init(size_t tester_instruction_mem_size, uint8_t* tester_instruction_mem)
{
	{
		instruction_mem_size = tester_instruction_mem_size;
		instruction_mem = tester_instruction_mem;

		/* ... Initialize your CPU here ... */

		std::cout << "Initializing the CPU ..." << std::endl;

		//Gameboy.InitROM();

		Reset();
	}
}

void LR35902::mycpu_set_state(state* state)
{
	/* ... Load your CPU with state as described (e.g., registers) ... */

	std::cout << "Setting state of the cpu ..." << std::endl;

	Gameboy.SetPaused(state->halted);
	InteruptsEnabled = state->interrupts_master_enabled;

	Register.pc = state->PC;
	Register.sp = state->SP;
	Register.reg8.A = state->reg8.A;
	Register.reg8.F = state->reg8.F;
	Register.reg8.B = state->reg8.B;
	Register.reg8.C = state->reg8.C;
	Register.reg8.D = state->reg8.D;
	Register.reg8.E = state->reg8.E;
	Register.reg8.H = state->reg8.H;
	Register.reg8.L = state->reg8.L;

	num_mem_accesses = 0;
}

void LR35902::mycpu_get_state(state* state)
{
	/* ... Copy your current CPU state into the provided struct ... */

	std::cout << "Getting the current state of the CPU ..." << std::endl;

	state->SP = Register.sp;
	state->PC = Register.pc;

	state->reg8.A = Register.reg8.A;
	state->reg8.F = Register.reg8.F;
	state->reg8.B = Register.reg8.B;
	state->reg8.C = Register.reg8.C;
	state->reg8.D = Register.reg8.D;
	state->reg8.E = Register.reg8.E;
	state->reg8.H = Register.reg8.H;
	state->reg8.L = Register.reg8.L;

	state->halted = Gameboy.GetPaused();
	state->interrupts_master_enabled = InteruptsEnabled;

	state->num_mem_accesses = num_mem_accesses;
	memcpy(state->mem_accesses, mem_accesses, sizeof(mem_accesses));
}

int LR35902::mycpu_step(uint8_t opCode)
{
	std::cout << "executing an opcode" << std::endl;

	u8 op;
	int cycles = 0;
	op = (uint8_t)mmu_read(Register.pc);
	if (op == 0xcb)
	{
		op = (uint8_t)mmu_read(Register.pc++);
		cycles = cycles_per_instruction_cb[op];
		ExecuteOpcodeCB(op);
	}
	else
	{
		cycles = cycles_per_instruction[op];
		ExecuteOpcode(opCode);
	}

	return cycles;
}

uint16_t LR35902::mmu_read(uint16_t addr)
{
	u8 ret;

	if (addr < instruction_mem_size)
		ret = instruction_mem[addr];
	else
		ret = 0xaa;

	return ret;
}

void LR35902::mmu_write(uint16_t addr, uint8_t val)
{
	struct mem_access* access = &mem_accesses[num_mem_accesses++];
	access->type = MEM_ACCESS_WRITE;
	access->addr = addr;
	access->val = val;
}