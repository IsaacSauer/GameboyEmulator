#include "pch.h"
#include "LR35902.h"
#include "GameBoy.h"
#include "OpcodeDefinitions.cpp" //Inlines..

#include <fstream>
#include <vector>
#ifdef _DEBUG
//#define VERBOSE
#endif

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
		Register.af(0x01b0);
		Register.bc(0x0013);
		Register.de(0x00d8);
		Register.hl(0x014D);

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
	const uint8_t opcode{ Gameboy.ReadMemory(Register.pc++) };

#ifdef VERBOSE
	static bool ok{};
	if (ok)
	{
		static std::ofstream file{ "log.txt",std::ios::trunc };
		file << std::to_string(Register.pc) << ", " << std::hex << '[' << LookUp[opcode] << ']' << int(opcode) << ", " << std::to_string(Register.f) << ", " <<
			std::to_string(Register.a) + ", " <<
			std::to_string(Register.b) + ", " <<
			std::to_string(Register.c) + ", " <<
			std::to_string(Register.d) + ", " <<
			std::to_string(Register.e) + ", " <<
			std::to_string(Register.h) + ", " <<
			std::to_string(Register.l) + ", " <<
			"IF:" + std::to_string(Gameboy.GetIF()) + ", " <<
			"SP:" + std::to_string(Gameboy.ReadMemory(Register.sp)) + ':' + std::to_string(Gameboy.ReadMemory(Register.sp + 1)) << std::endl;
	}
#endif
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
#define BASICOPS(A, B, C, D, E, H, L, cycles, funcName, ...) \
	case A: OPCYCLE( funcName( Register.a __VA_ARGS__), cycles ); \
	case B: OPCYCLE( funcName( Register.b __VA_ARGS__), cycles ); \
	case C: OPCYCLE( funcName( Register.c __VA_ARGS__), cycles ); \
	case D: OPCYCLE( funcName( Register.d __VA_ARGS__), cycles ); \
	case E: OPCYCLE( funcName( Register.e __VA_ARGS__), cycles ); \
	case H: OPCYCLE( funcName( Register.h __VA_ARGS__), cycles ); \
	case L: OPCYCLE( funcName( Register.l __VA_ARGS__), cycles )

void LR35902::ExecuteOpcode(uint8_t opcode)
{
	assert(Gameboy.ReadMemory(Register.pc - 1) == opcode); //pc is pointing to first argument
	uint8_t cycles{};

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
		OPCYCLE(ADC(Gameboy.ReadMemory(Register.hl()), true), 8);
	case 0xCE: //Add the contents of the 8-bit immediate operand d8 and the CY flag to the contents of register A, and store the results in register A.
		OPCYCLE(ADC(Gameboy.ReadMemory(Register.pc++), true), 8);

	case 0x80: //Add the contents of register B to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.b), 4);
	case 0x81: //Add the contents of register C to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.c), 4);
	case 0x82: //Add the contents of register D to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.d), 4);
	case 0x83: //Add the contents of register E to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.e), 4);
	case 0x84: //Add the contents of register H to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.h), 4);
	case 0x85: //Add the contents of register L to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.l), 4);
	case 0x87: //Add the contents of register A to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Register.a), 4);
	case 0x86: //Add the contents of memory specified by register pair HL to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Gameboy.ReadMemory(Register.hl())), 8);
	case 0xC6: //Add the contents of the 8-bit immediate operand d8 to the contents of register A, and store the results in register A.
		OPCYCLE(ADD(Gameboy.ReadMemory(Register.pc++)), 8);
	case 0xE8: //Add the contents of the 8-bit signed (2's complement) immediate operand s8 and the stack pointer SP and store the results in SP.
		OPCYCLE(ADD16(false, Gameboy.ReadMemory(Register.pc++)), 16); //Might have to add this value to the pc
	case 0x09: //Add the contents of register pair BC to the contents of register pair HL, and store the results in register pair HL.
		OPCYCLE(ADD16(true, Register.bc()), 8);
	case 0x19: //Add the contents of register pair DE to the contents of register pair HL, and store the results in register pair HL.
		OPCYCLE(ADD16(true, Register.de()), 8);
	case 0x29: //Add the contents of register pair HL to the contents of register pair HL, and store the results in register pair HL.
		OPCYCLE(ADD16(true, Register.hl()), 8);
	case 0x39: //Add the contents of register pair SP to the contents of register pair HL, and store the results in register pair HL.
		OPCYCLE(ADD16(true, Register.sp), 8);

	case 0xA0: //Take the logical AND for each bit of the contents of register B and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.b), 4);
	case 0xA1: //Take the logical AND for each bit of the contents of register C and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.c), 4);
	case 0xA2: //Take the logical AND for each bit of the contents of register D and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.d), 4);
	case 0xA3: //Take the logical AND for each bit of the contents of register E and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.e), 4);
	case 0xA4: //Take the logical AND for each bit of the contents of register H and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.h), 4);
	case 0xA5: //Take the logical AND for each bit of the contents of register L and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.l), 4);
	case 0xA6: //Take the logical AND for each bit of the contents of memory specified by register pair HL and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Gameboy.ReadMemory(Register.hl())), 8);
	case 0xA7: //Take the logical AND for each bit of the contents of register A and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Register.a), 4);
	case 0xE6: //Take the logical OR for each bit of the contents of the 8-bit immediate operand d8 and the contents of register A, and store the results in register A.
		OPCYCLE(AND(Gameboy.ReadMemory(Register.pc++)), 8);

	case 0xB8:
		//Compare the contents of register B and the contents of register A by calculating A - B, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.b), 4);
	case 0xB9:
		//Compare the contents of register C and the contents of register A by calculating A - C, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.c), 4);
	case 0xBA:
		//Compare the contents of register D and the contents of register A by calculating A - D, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.d), 4);
	case 0xBB:
		//Compare the contents of register E and the contents of register A by calculating A - E, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.e), 4);
	case 0xBC:
		//Compare the contents of register H and the contents of register A by calculating A - H, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.h), 4);
	case 0xBD:
		//Compare the contents of register L and the contents of register A by calculating A - L, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.l), 4);
	case 0xBE:
		//Compare the contents of memory specified by register pair HL and the contents of register A by calculating A - (HL), and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Gameboy.ReadMemory(Register.hl())), 8);
	case 0xBF:
		//Compare the contents of register A and the contents of register A by calculating A - A, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Register.a), 4);
	case 0xFE:
		//Compare the contents of register A and the contents of the 8-bit immediate operand d8 by calculating A - d8, and set the Z flag if they are equal.
		//The execution of this instruction does not affect the contents of register A.
		OPCYCLE(CP(Gameboy.ReadMemory(Register.pc++)), 8);

	case 0x05: //Decrement the contents of register B by 1.
		OPCYCLE(DEC(Register.b), 4);
	case 0x15: //Decrement the contents of register D by 1.
		OPCYCLE(DEC(Register.d), 4);
	case 0x25: //Decrement the contents of register H by 1.
		OPCYCLE(DEC(Register.h), 4);
	case 0x35: //Decrement the contents of memory specified by register pair HL by 1.
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		DEC(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 12;
		break;
	}
	case 0x0D: //Decrement the contents of register C by 1.
		OPCYCLE(DEC(Register.c), 4);
	case 0x1D: //Decrement the contents of register E by 1.
		OPCYCLE(DEC(Register.e), 4);
	case 0x2D: //Decrement the contents of register L by 1.
		OPCYCLE(DEC(Register.l), 4);
	case 0x3D: //Decrement the contents of register A by 1.
		OPCYCLE(DEC(Register.a), 4);
	case 0x0B: //Decrement the contents of register pair BC by 1.
	{
		uint16_t temp{ Register.bc() };
		DEC(temp);
		cycles = 8;
		Register.bc(temp);
		break;
	}
	case 0x1B: //Decrement the contents of register pair DE by 1.
	{
		uint16_t temp{ Register.de() };
		DEC(temp);
		cycles = 8;
		Register.de(temp);
		break;
	}
	case 0x2B: //Decrement the contents of register pair HL by 1.
	{
		uint16_t temp{ Register.hl() };
		DEC(temp);
		cycles = 8;
		Register.hl(temp);
		break;
	}
	case 0x3B: //Decrement the contents of register pair SP by 1.
		OPCYCLE(DEC(Register.sp), 8);

	case 0x04: //Increment the contents of register B by 1.
		OPCYCLE(INC(Register.b), 4);
	case 0x14: //Increment the contents of register D by 1.
		OPCYCLE(INC(Register.d), 4);
	case 0x24: //Increment the contents of register H by 1.
		OPCYCLE(INC(Register.h), 4);
	case 0x34: //Increment the contents of memory specified by register pair HL by 1.
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		INC(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 12;
		break;
	}
	case 0x0C: //Increment the contents of register C by 1.
		OPCYCLE(INC(Register.c), 4);
	case 0x1C: //Increment the contents of register E by 1.
		OPCYCLE(INC(Register.e), 4);
	case 0x2C: //Increment the contents of register L by 1.
		OPCYCLE(INC(Register.l), 4);
	case 0x3C: //Increment the contents of register A by 1.
		OPCYCLE(INC(Register.a), 4);
	case 0x03: //Increment the contents of register pair BC by 1.
	{
		uint16_t temp{ Register.bc() };
		INC(temp);
		cycles = 8;
		Register.bc(temp);
		break;
	}
	case 0x13: //Increment the contents of register pair DE by 1.
	{
		uint16_t temp{ Register.de() };
		INC(temp);
		cycles = 8;
		Register.de(temp);
		break;
	}
	case 0x23: //Increment the contents of register pair HL by 1.
	{
		uint16_t temp{ Register.hl() };
		INC(temp);
		cycles = 8;
		Register.hl(temp);
		break;
	}
	case 0x33: //Increment the contents of register pair SP by 1.
		OPCYCLE(INC(Register.sp), 8);

		BASICOPS(0xB7, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 4, OR);
	case 0xB6:
		//Take the logical OR for each bit of the contents of memory specified by register pair HL and the contents of register A, //and store the results in register A.
		OPCYCLE(OR(Gameboy.ReadMemory(Register.hl())), 8);
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
		OPCYCLE(SBC(Gameboy.ReadMemory(Register.hl()), true), 8);
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
		OPCYCLE(SUB(Gameboy.ReadMemory(Register.hl())), 8);
	case 0xD6:
		OPCYCLE(SUB(Gameboy.ReadMemory(Register.pc++)), 8);

		BASICOPS(0xAF, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 4, XOR);
	case 0xAE:
		OPCYCLE(XOR(Gameboy.ReadMemory(Register.hl())), 8);
	case 0xEE:
		OPCYCLE(XOR(Gameboy.ReadMemory(Register.pc++)), 8);

	case 0x27:
		OPCYCLE(DAA(), 4);
#pragma endregion
#pragma region Loads
	case 0x2a:
		OPCYCLE((LD(Register.a, Gameboy.ReadMemory(Register.hl())), Register.hl(Register.hl() + 1)), 8);
	case 0x3a:
		OPCYCLE((LD(Register.a, Gameboy.ReadMemory(Register.hl())), Register.hl(Register.hl() - 1)), 8);
	case 0xfa:
		OPCYCLE(LD(Register.a, Gameboy.ReadMemory(Gameboy.ReadMemoryWord(Register.pc))), 16);
	case 0x22:
		OPCYCLE((LD(Register.hl(), Register.a), Register.hl(Register.hl() + 1)), 8);
	case 0x32:
		OPCYCLE((LD(Register.hl(), Register.a), Register.hl(Register.hl() - 1)), 8);
	case 0x36:
		OPCYCLE(LD(Register.hl(), Gameboy.ReadMemory(Register.pc++)), 12);
	case 0xea:
	{
		const uint16_t adrs{ Gameboy.ReadMemoryWord(Register.pc) };
		OPCYCLE(LD(adrs, Register.a), 16);
	}
	case 0x08:
	{
		Gameboy.WriteMemoryWord(Gameboy.ReadMemoryWord(Register.pc), Register.sp);
		cycles = 20;
		break;
	}
	case 0x31:
		OPCYCLE(LD(&Register.sp, Gameboy.ReadMemoryWord(Register.pc)), 12);
	case 0xf9:
		OPCYCLE(LD(&Register.sp, Register.hl()), 8);
	case 0x02:
		OPCYCLE(LD(Register.bc(), Register.a), 8);
	case 0x12:
		OPCYCLE(LD(Register.de(), Register.a), 8);
	case 0x06:
		OPCYCLE(LD(Register.b, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x11:
	{
		uint16_t temp{ Register.de() };
		LD(&temp, Gameboy.ReadMemoryWord(Register.pc));
		cycles = 12;
		Register.de(temp);
		break;
	}
	case 0x01:
	{
		uint16_t temp{ Register.bc() };
		LD(&temp, Gameboy.ReadMemoryWord(Register.pc));
		cycles = 12;
		Register.bc(temp);
		break;
	}
	case 0x21:
	{
		uint16_t temp{ Register.hl() };
		LD(&temp, Gameboy.ReadMemoryWord(Register.pc));
		cycles = 12;
		Register.hl(temp);
		break;
	}
	case 0xf8:
	{
		uint16_t temp{ Register.hl() };
		LD(&temp, Register.sp + (int8_t)Gameboy.ReadMemory(Register.pc++));
		cycles = 12; //CodeSlinger:0
		Register.hl(temp);
		break;
	}
	case 0x0a:
		OPCYCLE(LD(Register.a, Gameboy.ReadMemory(Register.bc())), 8);
	case 0x0e:
		OPCYCLE(LD(Register.c, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x16:
		OPCYCLE(LD(Register.d, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x1a:
		OPCYCLE(LD(Register.a, Gameboy.ReadMemory(Register.de())), 8);
	case 0x1e:
		OPCYCLE(LD(Register.e, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x26:
		OPCYCLE(LD(Register.h, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x2e:
		OPCYCLE(LD(Register.l, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x3e:
		OPCYCLE(LD(Register.a, Gameboy.ReadMemory(Register.pc++)), 8);
	case 0x40:
		OPCYCLE(LD(Register.b, Register.b), 4);
	case 0x41:
		OPCYCLE(LD(Register.b, Register.c), 4);
	case 0x42:
		OPCYCLE(LD(Register.b, Register.d), 4);
	case 0x43:
		OPCYCLE(LD(Register.b, Register.e), 4);
	case 0x44:
		OPCYCLE(LD(Register.b, Register.h), 4);
	case 0x45:
		OPCYCLE(LD(Register.b, Register.l), 4);
	case 0x46:
		OPCYCLE(LD(Register.b, Gameboy.ReadMemory(Register.hl())), 8);
	case 0x47:
		OPCYCLE(LD(Register.b, Register.a), 4);
	case 0x48:
		OPCYCLE(LD(Register.c, Register.b), 4);
	case 0x49:
		OPCYCLE(LD(Register.c, Register.c), 4);
	case 0x4a:
		OPCYCLE(LD(Register.c, Register.d), 4);
	case 0x4b:
		OPCYCLE(LD(Register.c, Register.e), 4);
	case 0x4c:
		OPCYCLE(LD(Register.c, Register.h), 4);
	case 0x4d:
		OPCYCLE(LD(Register.c, Register.l), 4);
	case 0x4e:
		OPCYCLE(LD(Register.c, Gameboy.ReadMemory(Register.hl())), 8);
	case 0x4f:
		OPCYCLE(LD(Register.c, Register.a), 4);
	case 0x50:
		OPCYCLE(LD(Register.d, Register.b), 4);
	case 0x51:
		OPCYCLE(LD(Register.d, Register.c), 4);
	case 0x52:
		OPCYCLE(LD(Register.d, Register.d), 4);
	case 0x53:
		OPCYCLE(LD(Register.d, Register.e), 4);
	case 0x54:
		OPCYCLE(LD(Register.d, Register.h), 4);
	case 0x55:
		OPCYCLE(LD(Register.d, Register.l), 4);
	case 0x56:
		OPCYCLE(LD(Register.d, Gameboy.ReadMemory(Register.hl())), 8);
	case 0x57:
		OPCYCLE(LD(Register.d, Register.a), 4);
	case 0x58:
		OPCYCLE(LD(Register.e, Register.b), 4);
	case 0x59:
		OPCYCLE(LD(Register.e, Register.c), 4);
	case 0x5a:
		OPCYCLE(LD(Register.e, Register.d), 4);
	case 0x5b:
		OPCYCLE(LD(Register.e, Register.e), 4);
	case 0x5c:
		OPCYCLE(LD(Register.e, Register.h), 4);
	case 0x5d:
		OPCYCLE(LD(Register.e, Register.l), 4);
	case 0x5e:
		OPCYCLE(LD(Register.e, Gameboy.ReadMemory(Register.hl())), 8);
	case 0x5f:
		OPCYCLE(LD(Register.e, Register.a), 4);
	case 0x60:
		OPCYCLE(LD(Register.h, Register.b), 4);
	case 0x61:
		OPCYCLE(LD(Register.h, Register.c), 4);
	case 0x62:
		OPCYCLE(LD(Register.h, Register.d), 4);
	case 0x63:
		OPCYCLE(LD(Register.h, Register.e), 4);
	case 0x64:
		OPCYCLE(LD(Register.h, Register.h), 4);
	case 0x65:
		OPCYCLE(LD(Register.h, Register.l), 4);
	case 0x66:
		OPCYCLE(LD(Register.h, Gameboy.ReadMemory(Register.hl())), 8);
	case 0x67:
		OPCYCLE(LD(Register.h, Register.a), 4);
	case 0x68:
		OPCYCLE(LD(Register.l, Register.b), 4);
	case 0x69:
		OPCYCLE(LD(Register.l, Register.c), 4);
	case 0x6a:
		OPCYCLE(LD(Register.l, Register.d), 4);
	case 0x6b:
		OPCYCLE(LD(Register.l, Register.e), 4);
	case 0x6c:
		OPCYCLE(LD(Register.l, Register.h), 4);
	case 0x6d:
		OPCYCLE(LD(Register.l, Register.l), 4);
	case 0x6e:
		OPCYCLE(LD(Register.l, Gameboy.ReadMemory(Register.hl())), 8);
	case 0x6f:
		OPCYCLE(LD(Register.l, Register.a), 4);
	case 0x78:
		OPCYCLE(LD(Register.a, Register.b), 4);
	case 0x79:
		OPCYCLE(LD(Register.a, Register.c), 4);
	case 0x7a:
		OPCYCLE(LD(Register.a, Register.d), 4);
	case 0x7b:
		OPCYCLE(LD(Register.a, Register.e), 4);
	case 0x7c:
		OPCYCLE(LD(Register.a, Register.h), 4);
	case 0x7d:
		OPCYCLE(LD(Register.a, Register.l), 4);
	case 0x7e:
		OPCYCLE(LD(Register.a, Gameboy.ReadMemory(Register.hl())), 8);
	case 0x7f:
		OPCYCLE(LD(Register.a, Register.a), 4);
	case 0xf2:
		OPCYCLE(LD(Register.a, Gameboy.ReadMemory(0xFF00 + Register.c)), 8); //ok acc to gb manual
	case 0x70:
		OPCYCLE(LD(Register.hl(), Register.b), 8);
	case 0x71:
		OPCYCLE(LD(Register.hl(), Register.c), 8);
	case 0x72:
		OPCYCLE(LD(Register.hl(), Register.d), 8);
	case 0x73:
		OPCYCLE(LD(Register.hl(), Register.e), 8);
	case 0x74:
		OPCYCLE(LD(Register.hl(), Register.h), 8);
	case 0x75:
		OPCYCLE(LD(Register.hl(), Register.l), 8);
	case 0x77:
		OPCYCLE(LD(Register.hl(), Register.a), 8);
	case 0xe2:
		OPCYCLE(LD(static_cast<uint16_t>(0xFF00 + Register.c), Register.a), 8);
	case 0xE0:
		OPCYCLE(LD(static_cast<uint16_t>(0xFF00 + Gameboy.ReadMemory(Register.pc++)), Register.a), 12);
	case 0xF0:
	{
		//if(Register.pc==10667) __debugbreak();
		const uint8_t val{ Gameboy.ReadMemory(Register.pc++) };
		OPCYCLE(LD(Register.a, Gameboy.ReadMemory(static_cast<uint16_t>(0xFF00 + val))), 12);
	}

	case 0xC1:
	{
		uint16_t temp;
		POP(temp);
		Register.bc(temp);
		cycles = 12;
		break;
	}
	case 0xD1:
	{
		uint16_t temp;
		POP(temp);
		Register.de(temp);
		cycles = 12;
		break;
	}
	case 0xE1:
	{
		uint16_t temp;
		POP(temp);
		Register.hl(temp);
		cycles = 12;
		break;
	}
	case 0xF1:
	{
		uint16_t temp;
		POP(temp);
		Register.af(temp);
		cycles = 12;
		break;
	}

	case 0xC5:
		OPCYCLE(PUSH(Register.bc()), 16);
	case 0xD5:
		OPCYCLE(PUSH(Register.de()), 16);
	case 0xE5:
		OPCYCLE(PUSH(Register.hl()), 16);
	case 0xF5:
		OPCYCLE(PUSH(Register.af()), 16);

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
		OPCYCLE(RLC(Register.a), 4); //Codeslinger: 8
		break;
	//case 0x0F:
	//	/*
	//	Rotate the contents of register A to the right.
	//	That is, the contents of bit 7 are copied to bit 6,
	//	and the previous contents of bit 6 (before the copy) are copied to bit 5.
	//	The same operation is repeated in sequence for the rest of the register.
	//	The contents of bit 0 are placed in both the CY flag and bit 7 of register A.
	//	*/
	//	OPCYCLE(RRC(Register.a), 4); //Codeslinger: 8

	//case 0x17:
	//	/*
	//	Rotate the contents of register A to the right, through the carry (CY) flag. 
	//	That is, the contents of bit 7 are copied to bit 6, 
	//	and the previous contents of bit 6 (before the copy) are copied to bit 5. 
	//	The same operation is repeated in sequence for the rest of the register.
	//	The previous contents of the carry flag are copied to bit 7.
	//	*/
	//	OPCYCLE(RL(Register.a), 4); //Codeslinger: 8

	case 0x1F:
		/*
		Rotate the contents of register A to the right, through the carry (CY) flag. 
		That is, the contents of bit 7 are copied to bit 6, 
		and the previous contents of bit 6 (before the copy) are copied to bit 5. 
		The same operation is repeated in sequence for the rest of the register.
		The previous contents of the carry flag are copied to bit 7.
		*/
		OPCYCLE(RR(Register.a), 4); //Codeslinger: 8

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
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), !Register.zeroF), 0);
	case 0xD4:
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), !Register.carryF), 0);
	case 0xCC:
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), Register.zeroF), 0);
	case 0xDC:
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), Register.carryF), 0);
	case 0xCD:
		OPCYCLE(CALL(Gameboy.ReadMemoryWord(Register.pc), true), 0);;
	case 0xC2:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), !Register.zeroF), 0);
	case 0xD2:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), !Register.carryF), 0);
	case 0xC3:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), true), 0);
	case 0xE9:
		OPCYCLE(JP(Register.hl(), true, false), 4);
	case 0xCA:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), Register.zeroF), 0);
	case 0xDA:
		OPCYCLE(JP(Gameboy.ReadMemoryWord(Register.pc), Register.carryF), 0);;
	case 0x20:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), !Register.zeroF), 0);
	case 0x30:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), !Register.carryF), 0);
	case 0x18:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), true), 0);
	case 0x28:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), Register.zeroF), 0);
	case 0x38:
		OPCYCLE(JR(Gameboy.ReadMemory(Register.pc++), Register.carryF), 0);

	case 0xC0:
		OPCYCLE(RET(!Register.zeroF), 0);
	case 0xD0:
		OPCYCLE(RET(!Register.carryF), 0);
	case 0xC8:
		OPCYCLE(RET(Register.zeroF), 0);
	case 0xD8:
		OPCYCLE(RET(Register.carryF), 0);
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

#pragma region Extended Opcodes
	case 0xCB:
		opcode = Gameboy.ReadMemory(Register.pc++);
		switch (opcode)
		{
#pragma region Rotates and Shifts
		case 0x18:
			OPCYCLE(RR(Register.b), 8);
		case 0x19:
			OPCYCLE(RR(Register.c), 8);
		case 0x1A:
			OPCYCLE(RR(Register.d), 8);
		case 0x1B:
			OPCYCLE(RR(Register.e), 8);
		case 0x1C:
			OPCYCLE(RR(Register.h), 8);
		case 0x1D:
			OPCYCLE(RR(Register.l), 8);
		case 0x1E:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RR(hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0x1F:
			OPCYCLE(RR(Register.a), 8);
		case 0x20:
			OPCYCLE(SLA(Register.b), 8);
		case 0x21:
			OPCYCLE(SLA(Register.c), 8);
		case 0x22:
			OPCYCLE(SLA(Register.d), 8);
		case 0x23:
			OPCYCLE(SLA(Register.e), 8);
		case 0x24:
			OPCYCLE(SLA(Register.h), 8);
		case 0x25:
			OPCYCLE(SLA(Register.l), 8);
		case 0x26:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SLA(hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0x27:
			OPCYCLE(SLA(Register.a), 8);
		case 0x38:
			OPCYCLE(SRL(Register.b), 8);
		case 0x39:
			OPCYCLE(SRL(Register.c), 8);
		case 0x3A:
			OPCYCLE(SRL(Register.d), 8);
		case 0x3B:
			OPCYCLE(SRL(Register.e), 8);
		case 0x3C:
			OPCYCLE(SRL(Register.h), 8);
		case 0x3D:
			OPCYCLE(SRL(Register.l), 8);
		case 0x3E:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SRL(hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0x3F:
			OPCYCLE(SRL(Register.a), 8);
		case 0x00:
			OPCYCLE(RLC(Register.b), 8);
		case 0x01:
			OPCYCLE(RLC(Register.c), 8);
		case 0x02:
			OPCYCLE(RLC(Register.d), 8);
		case 0x03:
			OPCYCLE(RLC(Register.e), 8);
		case 0x04:
			OPCYCLE(RLC(Register.h), 8);
		case 0x05:
			OPCYCLE(RLC(Register.l), 8);
		case 0x06:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RLC(hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0x07:
			OPCYCLE(RLC(Register.a), 8);
#pragma endregion
#pragma region Bits
		case 0x40:
			OPCYCLE(BIT(0, Register.b), 8);
		case 0x41:
			OPCYCLE(BIT(0, Register.c), 8);
		case 0x42:
			OPCYCLE(BIT(0, Register.d), 8);
		case 0x43:
			OPCYCLE(BIT(0, Register.e), 8);
		case 0x44:
			OPCYCLE(BIT(0, Register.h), 8);
		case 0x45:
			OPCYCLE(BIT(0, Register.l), 8);
		case 0x46:
		{
			BIT(0, Gameboy.ReadMemory(Register.hl()));
			cycles = 16;
			break;
		}
		case 0x47:
			OPCYCLE(BIT(0, Register.a), 8);
		case 0x48:
			OPCYCLE(BIT(1, Register.b), 8);
		case 0x49:
			OPCYCLE(BIT(1, Register.c), 8);
		case 0x4a:
			OPCYCLE(BIT(1, Register.d), 8);
		case 0x4b:
			OPCYCLE(BIT(1, Register.e), 8);
		case 0x4c:
			OPCYCLE(BIT(1, Register.h), 8);
		case 0x4d:
			OPCYCLE(BIT(1, Register.l), 8);
		case 0x4e:
		{
			BIT(1, Gameboy.ReadMemory(Register.hl()));
			cycles = 16;
			break;
		}
		case 0x4f:
			OPCYCLE(BIT(1, Register.a), 8);
		case 0x50:
			OPCYCLE(BIT(2, Register.b), 8);
		case 0x51:
			OPCYCLE(BIT(2, Register.c), 8);
		case 0x52:
			OPCYCLE(BIT(2, Register.d), 8);
		case 0x53:
			OPCYCLE(BIT(2, Register.e), 8);
		case 0x54:
			OPCYCLE(BIT(2, Register.h), 8);
		case 0x55:
			OPCYCLE(BIT(2, Register.l), 8);
		case 0x56:
		{
			BIT(2, Gameboy.ReadMemory(Register.hl()));
			cycles = 16;
			break;
		}
		case 0x57:
			OPCYCLE(BIT(2, Register.a), 8);
		case 0x58:
			OPCYCLE(BIT(3, Register.b), 8);
		case 0x59:
			OPCYCLE(BIT(3, Register.c), 8);
		case 0x5a:
			OPCYCLE(BIT(3, Register.d), 8);
		case 0x5b:
			OPCYCLE(BIT(3, Register.e), 8);
		case 0x5c:
			OPCYCLE(BIT(3, Register.h), 8);
		case 0x5d:
			OPCYCLE(BIT(3, Register.l), 8);
		case 0x5e:
		{
			BIT(3, Gameboy.ReadMemory(Register.hl()));
			cycles = 16;
			break;
		}
		case 0x5f:
			OPCYCLE(BIT(3, Register.a), 8);
		case 0x60:
			OPCYCLE(BIT(4, Register.b), 8);
		case 0x61:
			OPCYCLE(BIT(4, Register.c), 8);
		case 0x62:
			OPCYCLE(BIT(4, Register.d), 8);
		case 0x63:
			OPCYCLE(BIT(4, Register.e), 8);
		case 0x64:
			OPCYCLE(BIT(4, Register.h), 8);
		case 0x65:
			OPCYCLE(BIT(4, Register.l), 8);
		case 0x66:
		{
			BIT(4, Gameboy.ReadMemory(Register.hl()));
			cycles = 16;
			break;
		}
		case 0x67:
			OPCYCLE(BIT(4, Register.a), 8);
		case 0x68:
			OPCYCLE(BIT(5, Register.b), 8);
		case 0x69:
			OPCYCLE(BIT(5, Register.c), 8);
		case 0x6a:
			OPCYCLE(BIT(5, Register.d), 8);
		case 0x6b:
			OPCYCLE(BIT(5, Register.e), 8);
		case 0x6c:
			OPCYCLE(BIT(5, Register.h), 8);
		case 0x6d:
			OPCYCLE(BIT(5, Register.l), 8);
		case 0x6e:
		{
			BIT(5, Gameboy.ReadMemory(Register.hl()));
			cycles = 16;
			break;
		}
		case 0x6f:
			OPCYCLE(BIT(5, Register.a), 8);
		case 0x70:
			OPCYCLE(BIT(6, Register.b), 8);
		case 0x71:
			OPCYCLE(BIT(6, Register.c), 8);
		case 0x72:
			OPCYCLE(BIT(6, Register.d), 8);
		case 0x73:
			OPCYCLE(BIT(6, Register.e), 8);
		case 0x74:
			OPCYCLE(BIT(6, Register.h), 8);
		case 0x75:
			OPCYCLE(BIT(6, Register.l), 8);
		case 0x76:
		{
			BIT(6, Gameboy.ReadMemory(Register.hl()));
			cycles = 16;
			break;
		}
		case 0x77:
			OPCYCLE(BIT(6, Register.a), 8);
		case 0x78:
			OPCYCLE(BIT(7, Register.b), 8);
		case 0x79:
			OPCYCLE(BIT(7, Register.c), 8);
		case 0x7a:
			OPCYCLE(BIT(7, Register.d), 8);
		case 0x7b:
			OPCYCLE(BIT(7, Register.e), 8);
		case 0x7c:
			OPCYCLE(BIT(7, Register.h), 8);
		case 0x7d:
			OPCYCLE(BIT(7, Register.l), 8);
		case 0x7e:
		{
			BIT(7, Gameboy.ReadMemory(Register.hl()));
			cycles = 16;
			break;
		}
		case 0x7f:
			OPCYCLE(BIT(7, Register.a), 8);

		case 0x80:
			OPCYCLE(RES(0, Register.b), 8);
		case 0x81:
			OPCYCLE(RES(0, Register.c), 8);
		case 0x82:
			OPCYCLE(RES(0, Register.d), 8);
		case 0x83:
			OPCYCLE(RES(0, Register.e), 8);
		case 0x84:
			OPCYCLE(RES(0, Register.h), 8);
		case 0x85:
			OPCYCLE(RES(0, Register.l), 8);
		case 0x86:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RES(0, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0x87:
			OPCYCLE(RES(0, Register.a), 8);
		case 0x88:
			OPCYCLE(RES(1, Register.b), 8);
		case 0x89:
			OPCYCLE(RES(1, Register.c), 8);
		case 0x8a:
			OPCYCLE(RES(1, Register.d), 8);
		case 0x8b:
			OPCYCLE(RES(1, Register.e), 8);
		case 0x8c:
			OPCYCLE(RES(1, Register.h), 8);
		case 0x8d:
			OPCYCLE(RES(1, Register.l), 8);
		case 0x8e:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RES(1, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0x8f:
			OPCYCLE(RES(1, Register.a), 8);
		case 0x90:
			OPCYCLE(RES(2, Register.b), 8);
		case 0x91:
			OPCYCLE(RES(2, Register.c), 8);
		case 0x92:
			OPCYCLE(RES(2, Register.d), 8);
		case 0x93:
			OPCYCLE(RES(2, Register.e), 8);
		case 0x94:
			OPCYCLE(RES(2, Register.h), 8);
		case 0x95:
			OPCYCLE(RES(2, Register.l), 8);
		case 0x96:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RES(2, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0x97:
			OPCYCLE(RES(2, Register.a), 8);
		case 0x98:
			OPCYCLE(RES(3, Register.b), 8);
		case 0x99:
			OPCYCLE(RES(3, Register.c), 8);
		case 0x9a:
			OPCYCLE(RES(3, Register.d), 8);
		case 0x9b:
			OPCYCLE(RES(3, Register.e), 8);
		case 0x9c:
			OPCYCLE(RES(3, Register.h), 8);
		case 0x9d:
			OPCYCLE(RES(3, Register.l), 8);
		case 0x9e:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RES(3, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0x9f:
			OPCYCLE(RES(3, Register.a), 8);
		case 0xa0:
			OPCYCLE(RES(4, Register.b), 8);
		case 0xa1:
			OPCYCLE(RES(4, Register.c), 8);
		case 0xa2:
			OPCYCLE(RES(4, Register.d), 8);
		case 0xa3:
			OPCYCLE(RES(4, Register.e), 8);
		case 0xa4:
			OPCYCLE(RES(4, Register.h), 8);
		case 0xa5:
			OPCYCLE(RES(4, Register.l), 8);
		case 0xa6:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RES(4, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xa7:
			OPCYCLE(RES(4, Register.a), 8);
		case 0xa8:
			OPCYCLE(RES(5, Register.b), 8);
		case 0xa9:
			OPCYCLE(RES(5, Register.c), 8);
		case 0xaa:
			OPCYCLE(RES(5, Register.d), 8);
		case 0xab:
			OPCYCLE(RES(5, Register.e), 8);
		case 0xac:
			OPCYCLE(RES(5, Register.h), 8);
		case 0xad:
			OPCYCLE(RES(5, Register.l), 8);
		case 0xae:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RES(5, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xaf:
			OPCYCLE(RES(5, Register.a), 8);
		case 0xb0:
			OPCYCLE(RES(6, Register.b), 8);
		case 0xb1:
			OPCYCLE(RES(6, Register.c), 8);
		case 0xb2:
			OPCYCLE(RES(6, Register.d), 8);
		case 0xb3:
			OPCYCLE(RES(6, Register.e), 8);
		case 0xb4:
			OPCYCLE(RES(6, Register.h), 8);
		case 0xb5:
			OPCYCLE(RES(6, Register.l), 8);
		case 0xb6:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RES(6, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xb7:
			OPCYCLE(RES(6, Register.a), 8);
		case 0xb8:
			OPCYCLE(RES(7, Register.b), 8);
		case 0xb9:
			OPCYCLE(RES(7, Register.c), 8);
		case 0xba:
			OPCYCLE(RES(7, Register.d), 8);
		case 0xbb:
			OPCYCLE(RES(7, Register.e), 8);
		case 0xbc:
			OPCYCLE(RES(7, Register.h), 8);
		case 0xbd:
			OPCYCLE(RES(7, Register.l), 8);
		case 0xbe:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			RES(7, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xbf:
			OPCYCLE(RES(7, Register.a), 8);

		case 0xc0:
			OPCYCLE(SET(0, Register.b), 8);
		case 0xc1:
			OPCYCLE(SET(0, Register.c), 8);
		case 0xc2:
			OPCYCLE(SET(0, Register.d), 8);
		case 0xc3:
			OPCYCLE(SET(0, Register.e), 8);
		case 0xc4:
			OPCYCLE(SET(0, Register.h), 8);
		case 0xc5:
			OPCYCLE(SET(0, Register.l), 8);
		case 0xc6:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SET(0, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xc7:
			OPCYCLE(SET(0, Register.a), 8);
		case 0xc8:
			OPCYCLE(SET(1, Register.b), 8);
		case 0xc9:
			OPCYCLE(SET(1, Register.c), 8);
		case 0xca:
			OPCYCLE(SET(1, Register.d), 8);
		case 0xcb:
			OPCYCLE(SET(1, Register.e), 8);
		case 0xcc:
			OPCYCLE(SET(1, Register.h), 8);
		case 0xcd:
			OPCYCLE(SET(1, Register.l), 8);
		case 0xce:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SET(1, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xcf:
			OPCYCLE(SET(1, Register.a), 8);
		case 0xd0:
			OPCYCLE(SET(2, Register.b), 8);
		case 0xd1:
			OPCYCLE(SET(2, Register.c), 8);
		case 0xd2:
			OPCYCLE(SET(2, Register.d), 8);
		case 0xd3:
			OPCYCLE(SET(2, Register.e), 8);
		case 0xd4:
			OPCYCLE(SET(2, Register.h), 8);
		case 0xd5:
			OPCYCLE(SET(2, Register.l), 8);
		case 0xd6:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SET(2, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xd7:
			OPCYCLE(SET(2, Register.a), 8);
		case 0xd8:
			OPCYCLE(SET(3, Register.b), 8);
		case 0xd9:
			OPCYCLE(SET(3, Register.c), 8);
		case 0xda:
			OPCYCLE(SET(3, Register.d), 8);
		case 0xdb:
			OPCYCLE(SET(3, Register.e), 8);
		case 0xdc:
			OPCYCLE(SET(3, Register.h), 8);
		case 0xdd:
			OPCYCLE(SET(3, Register.l), 8);
		case 0xde:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SET(3, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xdf:
			OPCYCLE(SET(3, Register.a), 8);
		case 0xe0:
			OPCYCLE(SET(4, Register.b), 8);
		case 0xe1:
			OPCYCLE(SET(4, Register.c), 8);
		case 0xe2:
			OPCYCLE(SET(4, Register.d), 8);
		case 0xe3:
			OPCYCLE(SET(4, Register.e), 8);
		case 0xe4:
			OPCYCLE(SET(4, Register.h), 8);
		case 0xe5:
			OPCYCLE(SET(4, Register.l), 8);
		case 0xe6:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SET(4, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xe7:
			OPCYCLE(SET(4, Register.a), 8);
		case 0xe8:
			OPCYCLE(SET(5, Register.b), 8);
		case 0xe9:
			OPCYCLE(SET(5, Register.c), 8);
		case 0xea:
			OPCYCLE(SET(5, Register.d), 8);
		case 0xeb:
			OPCYCLE(SET(5, Register.e), 8);
		case 0xec:
			OPCYCLE(SET(5, Register.h), 8);
		case 0xed:
			OPCYCLE(SET(5, Register.l), 8);
		case 0xee:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SET(5, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xef:
			OPCYCLE(SET(5, Register.a), 8);
		case 0xf0:
			OPCYCLE(SET(6, Register.b), 8);
		case 0xf1:
			OPCYCLE(SET(6, Register.c), 8);
		case 0xf2:
			OPCYCLE(SET(6, Register.d), 8);
		case 0xf3:
			OPCYCLE(SET(6, Register.e), 8);
		case 0xf4:
			OPCYCLE(SET(6, Register.h), 8);
		case 0xf5:
			OPCYCLE(SET(6, Register.l), 8);
		case 0xf6:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SET(6, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xf7:
			OPCYCLE(SET(6, Register.a), 8);
		case 0xf8:
			OPCYCLE(SET(7, Register.b), 8);
		case 0xf9:
			OPCYCLE(SET(7, Register.c), 8);
		case 0xfa:
			OPCYCLE(SET(7, Register.d), 8);
		case 0xfb:
			OPCYCLE(SET(7, Register.e), 8);
		case 0xfc:
			OPCYCLE(SET(7, Register.h), 8);
		case 0xfd:
			OPCYCLE(SET(7, Register.l), 8);
		case 0xfe:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SET(7, hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0xff:
			OPCYCLE(SET(7, Register.a), 8);
#pragma endregion
#pragma region MISC
		case 0x30:
			OPCYCLE(SWAP(Register.b), 8);
		case 0x31:
			OPCYCLE(SWAP(Register.c), 8);
		case 0x32:
			OPCYCLE(SWAP(Register.d), 8);
		case 0x33:
			OPCYCLE(SWAP(Register.e), 8);
		case 0x34:
			OPCYCLE(SWAP(Register.h), 8);
		case 0x35:
			OPCYCLE(SWAP(Register.l), 8);
		case 0x36:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			SWAP(hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 16;
			break;
		}
		case 0x37:
			OPCYCLE(SWAP(Register.a), 8);
#pragma endregion
#ifdef _DEBUG
		default:
			//assert(("Opcode with prefix 0xCB not implemented", false));
			//assert(false);
			break;
#endif
		}
		break;
#pragma endregion
#ifdef _DEBUG
	default:
		//TODO: cancel execution and write all missing opcodes to a log file.
		//assert(("Opcode not implemented", false));
		//assert(false);
		break;
#endif
	}

	Gameboy.AddCycles(cycles);
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