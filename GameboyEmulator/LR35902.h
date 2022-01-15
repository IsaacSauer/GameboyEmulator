#pragma once
#ifdef _DEBUG
#include <unordered_map>
#endif
#include <thread>
#include <condition_variable>
#include <atomic>

#include "opc_test\tester.h"
#include "Tile.h"

class GameBoy;
struct state;
struct mem_access;

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

struct Registers final
{
	//	enum Flags : uint8_t { zero = 7, subtract = 6, halfcarry = 5, carry = 4 };
	//
	//	uint8_t a{}, b{}, c{}, d{}, e{};
	//
	//	union
	//	{
	//		uint8_t f{}; //TODO: Lower nible is ALWAYS 0!!!
	//#ifndef _MSC_VER
	//#error	Compiler might not be supported
	//		/*
	//		 * Allocation of union variables is implementation-defined!
	//		 * So, the below MIGHT not be allocated correctly
	//		 * zeroF is supposed to be bit 7/7
	//		 */
	//#endif
	//		struct
	//		{
	//			uint8_t unusedF : 4;
	//			uint8_t carryF : 1;
	//			uint8_t halfCarryF : 1;
	//			uint8_t subtractF : 1;
	//			uint8_t zeroF : 1;
	//		};
	//	};
	//
	//	uint8_t h{}, l{};
	//
	//	uint16_t af() const { return static_cast<uint16_t>(a) << 8 | f; }
	//
	//	void af(uint16_t value)
	//	{
	//		a = value >> 8;
	//		f = value & 0xF0;
	//	}
	//
	//	uint16_t bc() const { return static_cast<uint16_t>(b) << 8 | c; }
	//
	//	void bc(uint16_t value)
	//	{
	//		b = value >> 8;
	//		c = value & 0xFF;
	//	}
	//
	//	uint16_t de() const { return static_cast<uint16_t>(d) << 8 | e; }
	//
	//	void de(uint16_t value)
	//	{
	//		d = value >> 8;
	//		e = value & 0xFF;
	//	}
	//
	//	uint16_t hl() const { return static_cast<uint16_t>(h) << 8 | l; }
	//
	//	void hl(uint16_t value)
	//	{
	//		h = value >> 8;
	//		l = value & 0xFF;
	//	}

	union
	{
		uint8_t regs[8];
		struct
		{
			uint16_t BC, DE, HL, AF;
		} reg16;
		struct
		{ /* little-endian of x86 is not nice here. */
			uint8_t C, B, E, D, L, H, F, A;
		} reg8;
		struct
		{
			char padding[6];
			uint8_t pad1 : 1;
			uint8_t pad2 : 1;
			uint8_t pad3 : 1;
			uint8_t pad4 : 1;
			uint8_t CF : 1;
			uint8_t HF : 1;
			uint8_t NF : 1;
			uint8_t ZF : 1;
		} flags;
	};

	uint16_t pc{}, sp{};
};

//VIDEO
enum class VideoMode
{
	ACCESS_OAM,
	ACCESS_VRAM,
	HBLANK,
	VBLANK,
};
const unsigned int GAMEBOY_WIDTH = 160;
const unsigned int GAMEBOY_HEIGHT = 144;
const unsigned int BG_MAP_SIZE = 256;
const int CLOCK_RATE = 4194304;

class LR35902 final
{
public:
	LR35902(GameBoy& gameboy);
	~LR35902() = default;

	LR35902(const LR35902& rhs) = delete; //Copy constructor
	LR35902(LR35902&& lhs) = delete; //Move Constructor
	LR35902& operator=(const LR35902& rhs) = delete; //Copy Assignment
	LR35902& operator=(LR35902&& lhs) = delete; //Move Assignment

	void Reset(const bool skipBoot = true);
	void ExecuteNextOpcode();
	void ExecuteNextOpcodeTest();

	void TestCPU();

	/**
	 * \note A Rudementary interupt handler
	 */
	void HandleInterupts();
	bool HandleInterrupt(u8 interrupt_bit, u16 interrupt_vector, u8 fired_interrupts);
	void HandleGraphics(const unsigned cycles, const unsigned cycleBudget, const bool draw) noexcept;

	bool GetHalted() { return Halted; }
private:
	struct DrawData
	{
		const uint8_t* const colors; //[4]
		void* const bitset;
		const uint16_t tileSetAdress;
		const uint16_t tileMapOffset;
		const uint16_t fbOffset;
		const uint8_t scrollX;
		const uint8_t tileY;
		std::atomic<uint16_t> doneCounter{};
	};

#ifdef _DEBUG
	std::unordered_map<unsigned char, std::string> LookUp{
		{0x0, "NOP"},
		{0x1, "LD BC,d16"},
		{0x2, "LD (BC),A"},
		{0x3, "INC BC"},
		{0x4, "INC B"},
		{0x5, "DEC B"},
		{0x6, "LD B,d8"},
		{0x7, "RLCA"},
		{0x8, "LD (a16),SP"},
		{0x9, "ADD HL,BC"},
		{0xa, "LD A,(BC)"},
		{0xb, "DEC BC"},
		{0xc, "INC C"},
		{0xd, "DEC C"},
		{0xe, "LD C,d8"},
		{0xf, "RRCA"},
		{0x10, "STOP 0"},
		{0x11, "LD DE,d16"},
		{0x12, "LD (DE),A"},
		{0x13, "INC DE"},
		{0x14, "INC D"},
		{0x15, "DEC D"},
		{0x16, "LD D,d8"},
		{0x17, "RLA"},
		{0x18, "JR r8"},
		{0x19, "ADD HL,DE"},
		{0x1a, "LD A,(DE)"},
		{0x1b, "DEC DE"},
		{0x1c, "INC E"},
		{0x1d, "DEC E"},
		{0x1e, "LD E,d8"},
		{0x1f, "RRA"},
		{0x20, "JR NZ,r8"},
		{0x21, "LD HL,d16"},
		{0x22, "LD (HL+),A"},
		{0x23, "INC HL"},
		{0x24, "INC H"},
		{0x25, "DEC H"},
		{0x26, "LD H,d8"},
		{0x27, "DAA"},
		{0x28, "JR Z,r8"},
		{0x29, "ADD HL,HL"},
		{0x2a, "LD A,(HL+)"},
		{0x2b, "DEC HL"},
		{0x2c, "INC L"},
		{0x2d, "DEC L"},
		{0x2e, "LD L,d8"},
		{0x2f, "CPL"},
		{0x30, "JR NC,r8"},
		{0x31, "LD SP,d16"},
		{0x32, "LD (HL-),A"},
		{0x33, "INC SP"},
		{0x34, "INC (HL)"},
		{0x35, "DEC (HL)"},
		{0x36, "LD (HL),d8"},
		{0x37, "SCF"},
		{0x38, "JR C,r8"},
		{0x39, "ADD HL,SP"},
		{0x3a, "LD A,(HL-)"},
		{0x3b, "DEC SP"},
		{0x3c, "INC A"},
		{0x3d, "DEC A"},
		{0x3e, "LD A,d8"},
		{0x3f, "CCF"},
		{0x40, "LD B,B"},
		{0x41, "LD B,C"},
		{0x42, "LD B,D"},
		{0x43, "LD B,E"},
		{0x44, "LD B,H"},
		{0x45, "LD B,L"},
		{0x46, "LD B,(HL)"},
		{0x47, "LD B,A"},
		{0x48, "LD C,B"},
		{0x49, "LD C,C"},
		{0x4a, "LD C,D"},
		{0x4b, "LD C,E"},
		{0x4c, "LD C,H"},
		{0x4d, "LD C,L"},
		{0x4e, "LD C,(HL)"},
		{0x4f, "LD C,A"},
		{0x50, "LD D,B"},
		{0x51, "LD D,C"},
		{0x52, "LD D,D"},
		{0x53, "LD D,E"},
		{0x54, "LD D,H"},
		{0x55, "LD D,L"},
		{0x56, "LD D,(HL)"},
		{0x57, "LD D,A"},
		{0x58, "LD E,B"},
		{0x59, "LD E,C"},
		{0x5a, "LD E,D"},
		{0x5b, "LD E,E"},
		{0x5c, "LD E,H"},
		{0x5d, "LD E,L"},
		{0x5e, "LD E,(HL)"},
		{0x5f, "LD E,A"},
		{0x60, "LD H,B"},
		{0x61, "LD H,C"},
		{0x62, "LD H,D"},
		{0x63, "LD H,E"},
		{0x64, "LD H,H"},
		{0x65, "LD H,L"},
		{0x66, "LD H,(HL)"},
		{0x67, "LD H,A"},
		{0x68, "LD L,B"},
		{0x69, "LD L,C"},
		{0x6a, "LD L,D"},
		{0x6b, "LD L,E"},
		{0x6c, "LD L,H"},
		{0x6d, "LD L,L"},
		{0x6e, "LD L,(HL)"},
		{0x6f, "LD L,A"},
		{0x70, "LD (HL),B"},
		{0x71, "LD (HL),C"},
		{0x72, "LD (HL),D"},
		{0x73, "LD (HL),E"},
		{0x74, "LD (HL),H"},
		{0x75, "LD (HL),L"},
		{0x76, "HALT"},
		{0x77, "LD (HL),A"},
		{0x78, "LD A,B"},
		{0x79, "LD A,C"},
		{0x7a, "LD A,D"},
		{0x7b, "LD A,E"},
		{0x7c, "LD A,H"},
		{0x7d, "LD A,L"},
		{0x7e, "LD A,(HL)"},
		{0x7f, "LD A,A"},
		{0x80, "ADD A,B"},
		{0x81, "ADD A,C"},
		{0x82, "ADD A,D"},
		{0x83, "ADD A,E"},
		{0x84, "ADD A,H"},
		{0x85, "ADD A,L"},
		{0x86, "ADD A,(HL)"},
		{0x87, "ADD A,A"},
		{0x88, "ADC A,B"},
		{0x89, "ADC A,C"},
		{0x8a, "ADC A,D"},
		{0x8b, "ADC A,E"},
		{0x8c, "ADC A,H"},
		{0x8d, "ADC A,L"},
		{0x8e, "ADC A,(HL)"},
		{0x8f, "ADC A,A"},
		{0x90, "SUB B"},
		{0x91, "SUB C"},
		{0x92, "SUB D"},
		{0x93, "SUB E"},
		{0x94, "SUB H"},
		{0x95, "SUB L"},
		{0x96, "SUB (HL)"},
		{0x97, "SUB A"},
		{0x98, "SBC A,B"},
		{0x99, "SBC A,C"},
		{0x9a, "SBC A,D"},
		{0x9b, "SBC A,E"},
		{0x9c, "SBC A,H"},
		{0x9d, "SBC A,L"},
		{0x9e, "SBC A,(HL)"},
		{0x9f, "SBC A,A"},
		{0xa0, "AND B"},
		{0xa1, "AND C"},
		{0xa2, "AND D"},
		{0xa3, "AND E"},
		{0xa4, "AND H"},
		{0xa5, "AND L"},
		{0xa6, "AND (HL)"},
		{0xa7, "AND A"},
		{0xa8, "XOR B"},
		{0xa9, "XOR C"},
		{0xaa, "XOR D"},
		{0xab, "XOR E"},
		{0xac, "XOR H"},
		{0xad, "XOR L"},
		{0xae, "XOR (HL)"},
		{0xaf, "XOR A"},
		{0xb0, "OR B"},
		{0xb1, "OR C"},
		{0xb2, "OR D"},
		{0xb3, "OR E"},
		{0xb4, "OR H"},
		{0xb5, "OR L"},
		{0xb6, "OR (HL)"},
		{0xb7, "OR A"},
		{0xb8, "CP B"},
		{0xb9, "CP C"},
		{0xba, "CP D"},
		{0xbb, "CP E"},
		{0xbc, "CP H"},
		{0xbd, "CP L"},
		{0xbe, "CP (HL)"},
		{0xbf, "CP A"},
		{0xc0, "RET NZ"},
		{0xc1, "POP BC"},
		{0xc2, "JP NZ,a16"},
		{0xc3, "JP a16"},
		{0xc4, "CALL NZ,a16"},
		{0xc5, "PUSH BC"},
		{0xc6, "ADD A,d8"},
		{0xc7, "RST 00H"},
		{0xc8, "RET Z"},
		{0xc9, "RET"},
		{0xca, "JP Z,a16"},
		{0xcb, "PREFIX CB"},
		{0xcc, "CALL Z,a16"},
		{0xcd, "CALL a16"},
		{0xce, "ADC A,d8"},
		{0xcf, "RST 08H"},
		{0xd0, "RET NC"},
		{0xd1, "POP DE"},
		{0xd2, "JP NC,a16"},
		{0xd3, "INVALID"},
		{0xd4, "CALL NC,a16"},
		{0xd5, "PUSH DE"},
		{0xd6, "SUB d8"},
		{0xd7, "RST 10H"},
		{0xd8, "RET C"},
		{0xd9, "RETI"},
		{0xda, "JP C,a16"},
		{0xdb, "INVALID"},
		{0xdc, "CALL C,a16"},
		{0xdd, "INVALID"},
		{0xde, "SBC A,d8"},
		{0xdf, "RST 18H"},
		{0xe0, "LDH (a8),A"},
		{0xe1, "POP HL"},
		{0xe2, "LD (C),A"},
		{0xe3, "INVALID"},
		{0xe4, "INVALID"},
		{0xe5, "PUSH HL"},
		{0xe6, "AND d8"},
		{0xe7, "RST 20H"},
		{0xe8, "ADD SP,r8"},
		{0xe9, "JP (HL)"},
		{0xea, "LD (a16),A"},
		{0xeb, "INVALID"},
		{0xec, "INVALID"},
		{0xed, "INVALID"},
		{0xee, "XOR d8"},
		{0xef, "RST 28H"},
		{0xf0, "LDH A,(a8)"},
		{0xf1, "POP AF"},
		{0xf2, "LD A,(C)"},
		{0xf3, "DI"},
		{0xf4, "INVALID"},
		{0xf5, "PUSH AF"},
		{0xf6, "OR d8"},
		{0xf7, "RST 30H"},
		{0xf8, "LD HL,SP+r8"},
		{0xf9, "LD SP,HL"},
		{0xfa, "LD A,(a16)"},
		{0xfb, "EI"},
		{0xfc, "INVALID"},
		{0xfd, "INVALID"},
		{0xfe, "CP d8"},
		{0xff, "RST 38H"}
	};

	std::unordered_map<unsigned char, std::string> LookUpCB{
		{0x0, "RLC B"},
		{0x1, "RLC C"},
		{0x2, "RLC D"},
		{0x3, "RLC E"},
		{0x4, "RLC H"},
		{0x5, "RLC L"},
		{0x6, "RLC (HL)"},
		{0x7, "RLC A"},
		{0x8, "RRC B"},
		{0x9, "RRC C"},
		{0xa, "RRC D"},
		{0xb, "RRC E"},
		{0xc, "RRC H"},
		{0xd, "RRC L"},
		{0xe, "RRC (HL)"},
		{0xf, "RRC A"},
		{0x10,"RL B"},
		{0x11,"RL C"},
		{0x12,"RL D"},
		{0x13,"RL E"},
		{0x14,"RL H"},
		{0x15,"RL L "},
		{0x16,"RL (HL)"},
		{0x17,"RL A"},
		{0x18,"RR B"},
		{0x19,"RR C"},
		{0x1a,"RR D"},
		{0x1b,"RR E"},
		{0x1c,"RR H"},
		{0x1d,"RR L"},
		{0x1e,"RR (HL)"},
		{0x1f,"RR A"},
		{0x20,"SLA B"},
		{0x21,"SLA C"},
		{0x22,"SLA D"},
		{0x23,"SLA E"},
		{0x24,"SLA H"},
		{0x25,"SLA L"},
		{0x26,"SLA (HL)"},
		{0x27,"SLA A"},
		{0x28,"SRA B"},
		{0x29,"SRA C"},
		{0x2a,"SRA D"},
		{0x2b,"SRA E"},
		{0x2c,"SRA H"},
		{0x2d,"SRA L"},
		{0x2e,"SRA (HL)"},
		{0x2f,"SRA A"},
		{0x30,"SWAP B"},
		{0x31,"SWAP C"},
		{0x32,"SWAP D"},
		{0x33,"SWAP E"},
		{0x34,"SWAP H"},
		{0x35,"SWAP L"},
		{0x36,"SWAP (HL)"},
		{0x37,"SWAP A"},
		{0x38,"SRL B"},
		{0x39,"SRL C"},
		{0x3a,"SRL D"},
		{0x3b,"SRL E"},
		{0x3c,"SRL H"},
		{0x3d,"SRL L"},
		{0x3e,"SRL (HL)"},
		{0x3f,"SRL A"},
		{0x40,"BIT 0 B"},
		{0x41,"BIT 0 C"},
		{0x42,"BIT 0 D"},
		{0x43,"BIT 0 E"},
		{0x44,"BIT 0 H"},
		{0x45,"BIT 0 L"},
		{0x46,"BIT 0 (HL)"},
		{0x47,"BIT 0 A"},
		{0x48,"BIT 1 B"},
		{0x49,"BIT 1 C"},
		{0x4a,"BIT 1 D"},
		{0x4b,"BIT 1 E"},
		{0x4c,"BIT 1 H"},
		{0x4d,"BIT 1 L"},
		{0x4e,"BIT 1 (HL)"},
		{0x4f,"BIT 1 A"},
		{0x50,"BIT 2 B"},
		{0x51,"BIT 2 C"},
		{0x52,"BIT 2 D"},
		{0x53,"BIT 2 E"},
		{0x54,"BIT 2 H"},
		{0x55,"BIT 2 L"},
		{0x56,"BIT 2 (HL)"},
		{0x57,"BIT 2 A"},
		{0x58,"BIT 3 B"},
		{0x59,"BIT 3 C"},
		{0x5a,"BIT 3 D"},
		{0x5b,"BIT 3 E"},
		{0x5c,"BIT 3 H"},
		{0x5d,"BIT 3 L"},
		{0x5e,"BIT 3 (HL)"},
		{0x5f,"BIT 3 A"},
		{0x60,"BIT 4 B"},
		{0x61,"BIT 4 C"},
		{0x62,"BIT 4 D"},
		{0x63,"BIT 4 E"},
		{0x64,"BIT 4 H"},
		{0x65,"BIT 4 L"},
		{0x66,"BIT 4 (HL)"},
		{0x67,"BIT 4 A"},
		{0x68,"BIT 5 B"},
		{0x69,"BIT 5 C"},
		{0x6a,"BIT 5 D"},
		{0x6b,"BIT 5 E"},
		{0x6c,"BIT 5 H"},
		{0x6d,"BIT 5 L"},
		{0x6e,"BIT 5 (HL)"},
		{0x6f,"BIT 5 A"},
		{0x70,"BIT 6 B"},
		{0x71,"BIT 6 C"},
		{0x72,"BIT 6 D"},
		{0x73,"BIT 6 E"},
		{0x74,"BIT 6 H"},
		{0x75,"BIT 6 L"},
		{0x76,"BIT 6 (HL)"},
		{0x77,"BIT 6 A"},
		{0x78,"BIT 7 B"},
		{0x79,"BIT 7 C"},
		{0x7a,"BIT 7 D"},
		{0x7b,"BIT 7 E"},
		{0x7c,"BIT 7 H"},
		{0x7d,"BIT 7 L"},
		{0x7e,"BIT 7 (HL)"},
		{0x7f,"BIT 7 A"},
		{0x80,"RES 0 B"},
		{0x81,"RES 0 C"},
		{0x82,"RES 0 D"},
		{0x83,"RES 0 E"},
		{0x84,"RES 0 H"},
		{0x85,"RES 0 L"},
		{0x86,"RES 0 (HL)"},
		{0x87,"RES 0 A"},
		{0x88,"RES 1 B"},
		{0x89,"RES 1 C"},
		{0x8a,"RES 1 D"},
		{0x8b,"RES 1 E"},
		{0x8c,"RES 1 H"},
		{0x8d,"RES 1 L"},
		{0x8e,"RES 1 (HL)"},
		{0x8f,"RES 1 A"},
		{0x90,"RES 2 B"},
		{0x91,"RES 2 C"},
		{0x92,"RES 2 D"},
		{0x93,"RES 2 E"},
		{0x94,"RES 2 H"},
		{0x95,"RES 2 L"},
		{0x96,"RES 2 (HL)"},
		{0x97,"RES 2 A"},
		{0x98,"RES 3 B"},
		{0x99,"RES 3 C"},
		{0x9a,"RES 3 D"},
		{0x9b,"RES 3 E"},
		{0x9c,"RES 3 H"},
		{0x9d,"RES 3 L"},
		{0x9e,"RES 3 (HL)"},
		{0x9f,"RES 3 A"},
		{0xa0,"RES 4 B"},
		{0xa1,"RES 4 C"},
		{0xa2,"RES 4 D"},
		{0xa3,"RES 4 E"},
		{0xa4,"RES 4 H"},
		{0xa5,"RES 4 L"},
		{0xa6,"RES 4 (HL)"},
		{0xa7,"RES 4 A"},
		{0xa8,"RES 5 B"},
		{0xa9,"RES 5 C"},
		{0xaa,"RES 5 D"},
		{0xab,"RES 5 E"},
		{0xac,"RES 5 H"},
		{0xad,"RES 5 L"},
		{0xae,"RES 5 (HL)"},
		{0xaf,"RES 5 A"},
		{0xb0,"RES 6 B"},
		{0xb1,"RES 6 C"},
		{0xb2,"RES 6 D"},
		{0xb3,"RES 6 E"},
		{0xb4,"RES 6 H"},
		{0xb5,"RES 6 L"},
		{0xb6,"RES 6 (HL)"},
		{0xb7,"RES 6 A"},
		{0xb8,"RES 7 B"},
		{0xb9,"RES 7 C"},
		{0xba,"RES 7 D"},
		{0xbb,"RES 7 E"},
		{0xbc,"RES 7 H"},
		{0xbd,"RES 7 L"},
		{0xbe,"RES 7 (HL)"},
		{0xbf,"RES 7 A"},
		{0xc0,"SET 0 B"},
		{0xc1,"SET 0 C"},
		{0xc2,"SET 0 D"},
		{0xc3,"SET 0 E"},
		{0xc4,"SET 0 H"},
		{0xc5,"SET 0 L"},
		{0xc6,"SET 0 (HL)"},
		{0xc7,"SET 0 A"},
		{0xc8,"SET 1 B"},
		{0xc9,"SET 1 C"},
		{0xca,"SET 1 D"},
		{0xcb,"SET 1 E"},
		{0xcc,"SET 1 H"},
		{0xcd,"SET 1 L"},
		{0xce,"SET 1 (HL)"},
		{0xcf,"SET 1 A"},
		{0xd0,"SET 2 B"},
		{0xd1,"SET 2 C"},
		{0xd2,"SET 2 D"},
		{0xd3,"SET 2 E"},
		{0xd4,"SET 2 H"},
		{0xd5,"SET 2 L"},
		{0xd6,"SET 2 (HL)"},
		{0xd7,"SET 2 A"},
		{0xd8,"SET 3 B"},
		{0xd9,"SET 3 C"},
		{0xda,"SET 3 D"},
		{0xdb,"SET 3 E"},
		{0xdc,"SET 3 H"},
		{0xdd,"SET 3 L"},
		{0xde,"SET 3 (HL)"},
		{0xdf,"SET 3 A"},
		{0xe0,"SET 4 B"},
		{0xe1,"SET 4 C"},
		{0xe2,"SET 4 D"},
		{0xe3,"SET 4 E"},
		{0xe4,"SET 4 H"},
		{0xe5,"SET 4 L"},
		{0xe6,"SET 4 (HL)"},
		{0xe7,"SET 4 A"},
		{0xe8,"SET 5 B"},
		{0xe9,"SET 5 C"},
		{0xea,"SET 5 D"},
		{0xeb,"SET 5 E"},
		{0xec,"SET 5 H"},
		{0xed,"SET 5 L"},
		{0xee,"SET 5 (HL)"},
		{0xef,"SET 5 A"},
		{0xf0,"SET 6 B"},
		{0xf1,"SET 6 C"},
		{0xf2,"SET 6 D"},
		{0xf3,"SET 6 E"},
		{0xf4,"SET 6 H"},
		{0xf5,"SET 6 L"},
		{0xf6,"SET 6 (HL)"},
		{0xf7,"SET 6 A"},
		{0xf8,"SET 7 B"},
		{0xf9,"SET 7 C"},
		{0xfa,"SET 7 D"},
		{0xfb,"SET 7 E"},
		{0xfc,"SET 7 H"},
		{0xfd,"SET 7 L"},
		{0xfe,"SET 7 (HL)"},
		{ 0xff,"SET 7 A" }
	};
#endif

	uint8_t m_opcode;
	bool Halted = false;

	Registers Register{};
	std::thread DrawThreads[10];
	GameBoy& Gameboy;
	unsigned int LCDCycles{};

	bool InteruptsEnabled{ false }; ///< InteruptsMasterEnable
	bool InteruptChangePending{ false }; ///< When an interupt change is requested, it gets pushed after the next opcode\note lsb==Disable, msb==Enable

	uint8_t ExecuteOpcode();
	uint8_t ExecuteOpcodeCB();
	void ConfigureLCDStatus();
	void DrawLine() const;
	void DrawBackground() const;
	void DrawWindow() const;
	void DrawSprites() const;
	uint8_t ReadPalette(const uint16_t pixelData, const uint8_t xPixel, const uint8_t yPixel) const;
	void ConfigureColorArray(uint8_t* const colorArray, uint8_t palette) const;

	void ThreadWork(const uint8_t id, DrawData** const drawData);

	// ReSharper disable CppInconsistentNaming
#pragma region OPCODES
	void ADD(uint8_t toAdd);
	void SUB(uint8_t toSub);

	void ADC(uint8_t toAdd);
	void ADDToHL(uint16_t toAdd);
	void ADDToSP();
	void ADD16(uint16_t toAdd);
	void AND(uint8_t toAnd);
	void BITop(const uint8_t bit, const uint8_t data);
	void CALL(uint16_t address, bool doCall = true);
	void CCF();
	void CP(uint8_t toCompare);
	void CPL();
	void DEC(uint8_t& toDec);
	void DI();
	void HALT();
	void INC(uint8_t& toInc);
	void JP(const uint16_t address, const bool doJump = true, bool handleCycles = true);
	void JR(int8_t offset, bool doJump = true);
	void LD(uint8_t& dest, const uint8_t data);
	void NOP();
	void OR(const uint8_t toOr);
	void POP(uint16_t& dest);
	void PUSH(const uint16_t data);
	void RES(const uint8_t bit, uint8_t& data);
	void RET(bool doReturn = true, bool handleCycles = true);
	void RETI();
	void RLC(uint8_t& toRotate);
	void RLA(bool throughTheCarry);
	void RL(uint8_t& toRotate);
	void RRC(uint8_t& toRotate);
	void RRCA();
	void RR(uint8_t& toRotate);
	void RRA();
	void RST(const uint8_t address);
	void LD(uint16_t* const dest, const uint16_t data);
	void EI();
	void SCF();
	void SET(const uint8_t bit, uint8_t& data);
	void SLA(uint8_t& toShift);
	void SRA(uint8_t& toShift);
	void SBC(uint8_t toSub);
	void SRL(uint8_t& toShift);
	void STOP();
	void SWAP(uint8_t& data);
	void DEC(uint16_t& toDec);
	void XOR(const uint8_t toXor);
	void INC(uint16_t& toInc);
	void LD(const uint16_t destAddrs, const uint8_t data);
	void DAA();
#pragma endregion
	// ReSharper restore CppInconsistentNaming

	size_t instruction_mem_size;
	uint8_t* instruction_mem;
	int num_mem_accesses;
	mem_access mem_accesses[16];

public:
	/*
	Called once at startup. Here you should perform any initialization needed by your CPU.
	Additionally, the arguments passed in by the tester specify a memory area of instruction_mem_size bytes,
	which should be mapped read-only at address 0 for your CPU (see below).

	Called once during startup. The area of memory pointed to by
	tester_instruction_mem will contain instructions the tester will inject, and
	should be mapped read-only at addresses [0,tester_instruction_mem_size).
	*/
	void mycpu_init(size_t tester_instruction_mem_size,
		uint8_t* tester_instruction_mem);

	/*
	Reset your CPU to a specific state, as defined in the state struct (format can be found below).
	This function is called in between each different instruction and set of inputs per instruction.

	Resets the CPU state (e.g., registers) to a given state state.
	*/
	void mycpu_set_state(state* state);

	/*
	Load the current state of your CPU into the state struct (as defined below).
	This function is called after each test run for different instruction and input combinations.

	Query the current state of the CPU.
	*/
	void mycpu_get_state(state* state);

	/*
	Step a single instruction of your CPU, and return the number of cycles spent doing so.
	This means machine cycles (running at a 4.19 MHz clock), so for instance the NOP instruction should return 4.

	Step a single instruction of the CPU. Returns the amount of cycles this took
	(e.g., NOP should return 4).
	*/
	void mycpu_step();

	/*
	Example mock MMU implementation, mapping the tester's instruction memory
	read-only at address 0, and logging all writes.
	*/
	void mmu_write(uint16_t addr, uint8_t val);

	uint16_t mmu_read(uint16_t addr);

#pragma region VIDEO
	//Copyright (c) 2015-2021 Jonathan Gilchrist
	//All rights reserved.
	//Source: https://github.com/jgilchrist/gbemu/blob/master/src
public:
	using vblank_callback_t = std::function<void(const FrameBuffer&)>;

	void register_vblank_callback(const vblank_callback_t& _vblank_callback);

	ByteRegister scroll_y{};
	ByteRegister scroll_x{};

	/* LCDC Y-coordinate */
	ByteRegister ly_compare{};

	ByteRegister window_y{};
	ByteRegister window_x{}; /* Note: x - 7 */

	ByteRegister bg_palette{};
	ByteRegister sprite_palette_0{}; /* OBP0 */
	ByteRegister sprite_palette_1{}; /* OBP1 */

	/* TODO: LCD Color Palettes (CGB) */
	/* TODO: LCD VRAM Bank (CGB) */

	ByteRegister dma_transfer{}; /* DMA */
	FrameBuffer& GetBuffer() { return buffer; }
private:
	void ResetFrameBuffer();

	void WriteScanline(uint8_t currentLine);
	void WriteSprites();
	void Draw();
	void DrawBackgroundLine(unsigned int currentLine);
	void DrawWindowLine(unsigned int currentLine);
	void DrawSprite(unsigned int spriteN);
	static GBColor GetPixelFromLine(uint8_t byte1, uint8_t byte2, uint8_t pixelIndex);

	static bool IsOnScreen(uint8_t x, uint8_t y);
	static bool IsOnScreenX(uint8_t x);
	static bool IsOnScreenY(uint8_t y);

	bool DisplayEnabled() const;
	bool WindowTileMap() const;
	bool WindowEnabled() const;
	bool BgWindowTileData() const;
	bool BgTileMapDisplay() const;
	bool SpriteSize() const;
	bool SpritesEnabled() const;
	bool BgEnabled() const;

	TileInfo GetTileInfo(Address titleSetLocation, uint8_t titleId, uint8_t titleLine) const;

	static Color GetRealColor(uint8_t pixelValue);
	static Palette LoadPalette(ByteRegister& palette_register);
	static Color GetColorFromPalette(GBColor color, const Palette& palette);

	FrameBuffer buffer;
	FrameBuffer background_map;

	VideoMode current_mode = VideoMode::ACCESS_OAM;

	vblank_callback_t vblank_callback;

#pragma endregion
};

const unsigned int CLOCKS_PER_HBLANK = 204; /* Mode 0 */
const unsigned int CLOCKS_PER_SCANLINE_OAM = 80; /* Mode 2 */
const unsigned int CLOCKS_PER_SCANLINE_VRAM = 172; /* Mode 3 */
const unsigned int CLOCKS_PER_SCANLINE =
(CLOCKS_PER_SCANLINE_OAM + CLOCKS_PER_SCANLINE_VRAM + CLOCKS_PER_HBLANK);

const unsigned int CLOCKS_PER_VBLANK = 4560; /* Mode 1 */
const unsigned int SCANLINES_PER_FRAME = 144;
const unsigned int CLOCKS_PER_FRAME = (CLOCKS_PER_SCANLINE * SCANLINES_PER_FRAME) + CLOCKS_PER_VBLANK;
