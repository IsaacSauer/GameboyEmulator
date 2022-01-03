#include "pch.h"
#include "LR35902.h"
#include "GameBoy.h"
//Add the signature to LR35902.h

#pragma region ALU
//Add Carry
//Add the contents of register A and the x flag to the contents of register x, and store the results in register A.
FINLINE void LR35902::ADC(uint8_t toAdd, bool addCarry)
{
	if (addCarry && Register.flags.CF)
		++toAdd;

	Register.flags.HF = (Register.reg8.A & 0xf) + (toAdd & 0xf) & 0x10;
	Register.flags.CF = static_cast<uint16_t>(Register.reg8.A + toAdd) > 0xFF;

	Register.reg8.A += toAdd;

	Register.flags.ZF = !Register.reg8.A;
	Register.flags.ZF = 0;
}

/**
 * \param addToHL If False, it'll be added to SP and toAdd will be 8 bits signed
 */
FINLINE void LR35902::ADD16(bool addToHL, uint16_t toAdd)
{
	if (addToHL)
	{
		Register.flags.HF = (Register.reg16.HL & 0xfff) + (toAdd & 0xfff) & 0x1000; //h is always 3->4 in the high byte
		Register.flags.CF = static_cast<uint32_t>(Register.reg16.HL) + toAdd > 0xFFFF;

		Register.reg16.HL = Register.reg16.HL + toAdd;
	}
	else
	{
		const int8_t toAddS{ static_cast<int8_t>(toAdd) };
		if (toAddS >= 0)
		{ //Positive
			Register.flags.HF = (Register.sp & 0xfff) + (toAdd & 0xfff) & 0x1000; //h is always 3->4 in the high byte
			Register.flags.CF = static_cast<uint32_t>(Register.sp) + toAdd > 0xFFFF;
		}
		else
		{ //Negative
			Register.flags.HF = static_cast<int16_t>(Register.sp & 0xf) + (toAddS & 0xf) <
				0; // Check if subtracting the last 4 bits goes negative, indicating a borrow //Could also do ((Register.sp+toAddS)^toAddS^Register.sp)&0x10
			Register.flags.CF = static_cast<uint32_t>(Register.sp) + toAddS < 0; //If we go negative, it would cause an underflow on unsigned, setting the carry
		}
		Register.sp += toAddS;
		Register.flags.ZF = 0;
	}
	Register.flags.ZF = 0; //even tho we might've done a subtraction...
}

FINLINE void LR35902::SBC(uint8_t toSub, bool subCarry)
{
	if (subCarry && Register.flags.CF)
		++toSub;

	Register.flags.HF = ((Register.reg8.A - toSub) ^ toSub ^ Register.reg8.A) & 0x10;
	Register.flags.CF = (static_cast<int8_t>(Register.reg8.A) - toSub) < 0;
	Register.flags.ZF = !Register.reg8.A;
	Register.flags.ZF = 1;
	Register.reg8.A -= toSub;
}

FINLINE void LR35902::OR(const uint8_t toOr)
{
	Register.reg8.F = 0;
	Register.reg8.A |= toOr;
	Register.flags.ZF = !Register.reg8.A;
}

FINLINE void LR35902::INC(uint8_t& toInc)
{
	Register.flags.ZF = 0;
	//Register.flags.CF; //Does not affect the carry flag! The INC/DEC opcode is often used to control loops; Loops are often used for multiple precision arithmetic so, to prevent having to push the carry state after every loop they just made the instruction ignore it! :D
	Register.flags.HF = static_cast<uint8_t>((toInc & 0xF) == 0xF);
	Register.flags.ZF = !(++toInc);
}

FINLINE void LR35902::INC(uint16_t& toInc)
{
	//No flags for 16 bit variant
	++toInc;
}

FINLINE void LR35902::AND(uint8_t toAnd)
{
	//Register.reg8.F = 0;
	//Register.flags.HF = 1; //Why tho?
	//Register.reg8.A &= toAnd;
	//Register.flags.ZF = !Register.reg8.A;

	Register.reg8.F = (Register.reg8.A == 0) ? (Register.flags.HF | Register.flags.ZF) : Register.flags.HF;
}

FINLINE void LR35902::DEC(uint8_t& toDec)
{
	Register.flags.ZF = 1;
	//Register.flags.CF; //Does not affect the carry flag! The INC/DEC opcode is often used to control loops; Loops are often used for multiple precision arithmetic so, to prevent having to push the carry state after every loop they just made the instruction ignore it! :D
	Register.flags.HF = !(toDec & 0xF);
	Register.flags.ZF = !--toDec;
}

FINLINE void LR35902::DEC(uint16_t& toDec)
{
	//No flags for 16 bit variant
	--toDec;
}

FINLINE void LR35902::XOR(const uint8_t toXor)
{
	Register.reg8.F = 0;
	Register.reg8.A ^= toXor;
	Register.flags.ZF = !Register.reg8.A;
}

FINLINE void LR35902::CP(uint8_t toCompare)
{
	Register.flags.ZF = 1;
	Register.flags.ZF = !(Register.reg8.A - toCompare);
	Register.flags.CF = Register.reg8.A < toCompare;
	Register.flags.HF = static_cast<int16_t>(Register.reg8.A & 0xF) - (toCompare & 0xF) < 0;
}

//DecimalAdjustA, implementation heavily inspired by Richeson's paper
FINLINE void LR35902::DAA()
{
	int newA{ Register.reg8.A };

	if (!Register.flags.ZF)
	{
		if (Register.flags.HF || (newA & 0xF) > 9) //if we did an initial overflow on the lower nibble or have exceeded 9 (the max value in BCD)
			newA += 6; //Overflow (15-9)
		if (Register.flags.CF || (newA & 0xF0) > 0x90)
			newA += 0x60; //same overflow for the higher nibble
	}
	else //The last operation was a subtraction, we need to honor this
	{
		if (Register.flags.HF)
		{
			newA -= 6;
			newA &= 0xFF;
		}
		if (Register.flags.CF)
			newA -= 0x60;
	}

	Register.flags.HF = false;
	Register.flags.CF = newA > 0xFF;
	Register.reg8.A = static_cast<uint8_t>(newA);
	Register.flags.ZF = !Register.reg8.A;
}
#pragma endregion

#pragma region Loads
/**
 * \note writes \b DIRECTLY
 */
FINLINE void LR35902::LD(uint8_t& dest, const uint8_t data)
{
	dest = data;
}

FINLINE void LR35902::LD(uint16_t* const dest, const uint16_t data) { *dest = data; }
FINLINE void LR35902::LD(const uint16_t destAddrs, const uint8_t data) { Gameboy.WriteMemory(destAddrs, data); }

FINLINE void LR35902::PUSH(const uint16_t data) //little endian
{
	--Register.sp;
	Gameboy.WriteMemory(Register.sp, data >> 8);
	--Register.sp;
	Gameboy.WriteMemory(Register.sp, data & 0xFF);
}

FINLINE void LR35902::POP(uint16_t& dest)
{
	dest = Gameboy.ReadMemory(Register.sp++);
	dest |= (static_cast<uint16_t>(Gameboy.ReadMemory(Register.sp++)) << 8);
}
#pragma endregion

#pragma region Rotates and Shifts
//RotateLeftCarry
FINLINE void LR35902::RLC(uint8_t& toRotate)
{
	Register.reg8.F = 0;
	const bool msb{ static_cast<bool>(toRotate & 0x80) };

	toRotate <<= 1;
	toRotate |= static_cast<uint8_t>(msb);
	Register.flags.CF = msb;
	Register.flags.ZF = !toRotate;
}

//RotateLeft
FINLINE void LR35902::RL(uint8_t& toRotate)
{
	const bool msb{ bool(toRotate & 0x80) };
	toRotate <<= 1;
	toRotate |= (Register.flags.CF << 0);

	Register.reg8.F = 7;
	Register.flags.CF = msb;
	Register.flags.ZF = !toRotate;
}

//RotateRightCarry
FINLINE void LR35902::RRC(uint8_t& toRotate)
{
	Register.reg8.F = 0;
	const bool lsb{ bool(toRotate & 0x1) };
	toRotate = static_cast<uint8_t>((toRotate >> 1) | (lsb << 7));

	//toRotate >>= 1;
	//toRotate |= uint8_t(lsb);
	Register.flags.CF = lsb;
	Register.flags.ZF = !toRotate;
	Register.flags.HF = false;
	Register.flags.ZF = false;
}

//RotateRight
FINLINE void LR35902::RR(uint8_t& toRotate)
{
	const bool lsb{ static_cast<bool>(toRotate & 0x1) };
	toRotate >>= 1;
	toRotate |= Register.flags.CF << 7;

	Register.reg8.F = 0;
	Register.flags.CF = lsb;
	Register.flags.ZF = !toRotate;
}

//ShiftLeftArithmetic (even though it's a logical shift..)
FINLINE void LR35902::SLA(uint8_t& toShift)
{
	Register.reg8.F = 0;
	Register.flags.CF = toShift & 0x80;
	toShift <<= 1;
	Register.flags.ZF = !toShift;
}

//ShiftRightArithmetic
FINLINE void LR35902::SRA(uint8_t& toShift)
{
	Register.reg8.F = 0;
	Register.flags.CF = toShift & 0x1;
	toShift >>= 1;
	Register.flags.ZF = !toShift;
}

//ShiftRightLogical
FINLINE void LR35902::SRL(uint8_t& toShift)
{
	Register.reg8.F = 0;
	Register.flags.CF = toShift & 0x1;
	toShift >>= 1;
	Register.flags.ZF = !toShift;
}
#pragma endregion

#pragma region Bits
FINLINE void LR35902::BITop(const uint8_t bit, const uint8_t data)
{
	Register.flags.ZF = !((data >> bit) & 1);
	Register.flags.ZF = 0;
	Register.flags.HF = 1;
}

FINLINE void LR35902::RES(const uint8_t bit, uint8_t& data)
{
	data &= ~(1 << bit);
}

FINLINE void LR35902::SET(const uint8_t bit, uint8_t& data)
{
	data |= 1 << bit;
}
#pragma endregion

#pragma region Misc
FINLINE void LR35902::SWAP(uint8_t& data)
{
	data = data >> 4 | data << 4;
	Register.reg8.F = 0;
	Register.flags.ZF = !data;
}

//ComplementCarryFlag
FINLINE void LR35902::CCF()
{
	Register.flags.HF = Register.flags.ZF = 0;
	Register.flags.CF = !Register.flags.CF;
}

FINLINE void LR35902::SCF()
{
	Register.flags.ZF = Register.flags.HF = 0;
	Register.flags.CF = 1;
}

FINLINE void LR35902::HALT() { --Register.pc; } //Until an interrupt
FINLINE void LR35902::STOP() { --Register.pc; } //Until button press
FINLINE void LR35902::DI() { *(uint8_t*)&InteruptChangePending = 1; }
FINLINE void LR35902::EI() { *((uint8_t*)&InteruptChangePending) = 8; }
FINLINE void LR35902::NOP() {}

//ComPlemenT
FINLINE void LR35902::CPL()
{
	Register.reg8.A = ~Register.reg8.A;
	Register.flags.ZF = Register.flags.HF = 1;
}
#pragma endregion

#pragma region Calls and Jumps
/**
 * \note	<b>This function handles adding the cycles!</b>
 */
FINLINE void LR35902::CALL(const uint16_t address, bool doCall)
{
	if (doCall)
	{
		Gameboy.WriteMemoryWord(Register.sp -= 2, Register.pc);
		Register.pc = address;
		Gameboy.AddCycles(24); //CodeSlinger:12
	}
	else
		Gameboy.AddCycles(12);
}
/**
 * \note	<b>This function handles adding the cycles!</b>
 */
FINLINE void LR35902::JR(const int8_t offset, bool doJump)
{
	if (doJump)
	{
		Register.pc += offset;
		Gameboy.AddCycles(12); //CodeSlinger:8
	}
	else
		Gameboy.AddCycles(8);
}

FINLINE void LR35902::JP(const uint16_t address, const bool doJump, bool handleCycles)
{
	if (doJump)
	{
		Register.pc = address;
		Gameboy.AddCycles(handleCycles ? 16 : 0); //CodeSlinger:12:0
	}
	else
		Gameboy.AddCycles(handleCycles ? 12 : 0);
}

FINLINE void LR35902::RET(bool doReturn, bool handleCycles)
{
	if (doReturn)
	{
		Register.pc = Gameboy.ReadMemoryWord(Register.sp);
		Gameboy.AddCycles(handleCycles ? 20 : 0); //CodeSlinger:8
	}
	else
		Gameboy.AddCycles(handleCycles ? 8 : 0);
}

FINLINE void LR35902::RETI()
{
	Register.pc = Gameboy.ReadMemoryWord(Register.sp);
	InteruptsEnabled = true;
}

//same as call.. (apart from address size)
FINLINE void LR35902::RST(const uint8_t address)
{
	Gameboy.WriteMemoryWord(Register.sp -= 2, Register.pc);
	Register.pc = address;
}
#pragma endregion