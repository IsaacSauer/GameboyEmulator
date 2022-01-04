#pragma once
#ifdef _DEBUG
#include <unordered_map>
#endif
#include <thread>
#include <condition_variable>
#include <atomic>

#include "opc_test\tester.h"

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
		PACK(struct packed
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
		} flags);
	};

	uint16_t pc{}, sp{};
};

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
	void HandleGraphics(const unsigned cycles, const unsigned cycleBudget, const bool draw) noexcept;

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

	void LogRequiredOpcodes();
	std::vector<unsigned int> m_RequiredOpcodes;
#endif

	Registers Register{};
	std::thread DrawThreads[10];
	GameBoy& Gameboy;
	unsigned int LCDCycles{};

	bool InteruptsEnabled{ false }; ///< InteruptsMasterEnable
	bool InteruptChangePending{ false }; ///< When an interupt change is requested, it gets pushed after the next opcode\note lsb==Disable, msb==Enable

	uint8_t ExecuteOpcode(uint8_t opcode);
	uint8_t ExecuteOpcodeCB(uint8_t opcode);
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
	void ADD(uint8_t toAdd) { return ADC(toAdd, false); }
	void SUB(uint8_t toSub) { return SBC(toSub, false); }

	void ADC(uint8_t toAdd, bool addCarry = true);
	void ADD16(bool addToHL, uint16_t toAdd);
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
	void RL(uint8_t& toRotate);
	void RRC(uint8_t& toRotate);
	void RR(uint8_t& toRotate);
	void RST(const uint8_t address);
	void LD(uint16_t* const dest, const uint16_t data);
	void EI();
	void SCF();
	void SET(const uint8_t bit, uint8_t& data);
	void SLA(uint8_t& toShift);
	void SRA(uint8_t& toShift);
	void SBC(uint8_t toSub, bool subCarry = true);
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
};