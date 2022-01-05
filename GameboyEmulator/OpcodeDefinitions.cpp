#include "pch.h"
#include "LR35902.h"
#include "GameBoy.h"
//Add the signature to LR35902.h

#pragma region ALU

FINLINE void LR35902::ADD(uint8_t toAdd)
{
	uint16_t result = Register.reg8.A + toAdd;
	Register.flags.ZF = (uint8_t)result == 0;
	Register.flags.NF = 0;
	Register.flags.HF = (Register.reg8.A ^ toAdd ^ result) & 0x10 ? 1 : 0;
	Register.flags.CF = result & 0x100 ? 1 : 0;
	Register.reg8.A = (uint8_t)result;
}

FINLINE void LR35902::SUB(uint8_t toSub)
{
	uint8_t result = Register.reg8.A - toSub;
	Register.flags.ZF = result == 0;
	Register.flags.NF = 1;
	Register.flags.HF = ((int32_t)Register.reg8.A & 0xf) - (toSub & 0xf) < 0;
	Register.flags.CF = Register.reg8.A < toSub;
	Register.reg8.A = result;
}

//Add Carry
//Add the contents of register A and the x flag to the contents of register x, and store the results in register A.
FINLINE void LR35902::ADC(uint8_t toAdd)
{
	u16 result = Register.reg8.A + toAdd + Register.flags.CF;
	Register.flags.ZF = (uint8_t)result == 0;
	Register.flags.NF = 0;
	Register.flags.HF = (Register.reg8.A ^ toAdd ^ result) & 0x10 ? 1 : 0;
	Register.flags.CF = result & 0x100 ? 1 : 0;
	Register.reg8.A = (uint8_t)result;

	//if (addCarry && Register.flags.CF)
	//	++toAdd;

	//Register.flags.HF = (Register.reg8.A & 0xf) + (toAdd & 0xf) & 0x10;
	//Register.flags.CF = static_cast<uint16_t>(Register.reg8.A + toAdd) > 0xFF;

	//Register.reg8.A += toAdd;

	//Register.flags.ZF = !Register.reg8.A;
	//Register.flags.NF = 0;
}

FINLINE void LR35902::ADDToHL(uint16_t toAdd)
{
	u32 toAddAddedToHL = Register.reg16.HL + toAdd;
	Register.flags.NF = 0;
	Register.flags.HF = (((Register.reg16.HL & 0xfff) + (toAdd & 0xfff)) & 0x1000) ? 1 : 0;
	Register.flags.CF = toAddAddedToHL > 0xffff;
	Register.reg16.HL = toAddAddedToHL;
}

FINLINE void LR35902::ADDToSP()
{
	int8_t off = (int8_t)Gameboy.ReadMemory(Register.pc);
	uint32_t res = Register.sp + off;
	Register.flags.ZF = 0;
	Register.flags.NF = 0;
	Register.flags.HF = (Register.sp & 0xf) + (Gameboy.ReadMemory(Register.pc) & 0xf) > 0xf;
	Register.flags.CF = (Register.sp & 0xff) + (Gameboy.ReadMemory(Register.pc) & 0xff) > 0xff;
	Register.sp = res;
	Register.pc++;

}

FINLINE void LR35902::ADD16(uint16_t toAdd)
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
	Register.flags.NF = 0; //even tho we might've done a subtraction...
}

FINLINE void LR35902::SBC(uint8_t toSub)
{
	uint8_t result = Register.reg8.A - toSub - Register.flags.CF;
	Register.flags.ZF = result == 0;
	Register.flags.NF = 1;
	Register.flags.HF = ((int32_t)Register.reg8.A & 0xf) - (toSub & 0xf) - Register.flags.CF < 0;
	Register.flags.CF = Register.reg8.A < toSub + Register.flags.CF;
	Register.reg8.A = result;
}

FINLINE void LR35902::OR(const uint8_t toOr)
{
	Register.reg8.F = 0;
	Register.reg8.A |= toOr;
	Register.flags.ZF = !Register.reg8.A;
}

FINLINE void LR35902::INC(uint8_t& toInc)
{
	Register.flags.NF = 0;
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
	Register.reg8.F = 0;
	Register.flags.HF = 1; //Why tho?
	Register.reg8.A &= toAnd;
	Register.flags.ZF = !Register.reg8.A;

	//Register.reg8.F = (Register.reg8.A == 0) ? (Register.flags.HF | Register.flags.ZF) : Register.flags.HF;
}

FINLINE void LR35902::DEC(uint8_t& toDec)
{
	Register.flags.NF = 1;
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
	Register.flags.NF = 1;
	Register.flags.ZF = !(Register.reg8.A - toCompare);
	Register.flags.CF = Register.reg8.A < toCompare;
	Register.flags.HF = static_cast<int16_t>(Register.reg8.A & 0xF) - (toCompare & 0xF) < 0;
}

//DecimalAdjustA, implementation heavily inspired by Richeson's paper
FINLINE void LR35902::DAA()
{
	s8 add = 0;
	if ((!Register.flags.NF && (Register.reg8.A & 0xf) > 0x9) || Register.flags.HF) //if we did an initial overflow on the lower nibble or have exceeded 9 (the max value in BCD)
		add |= 0x6;
	if ((!Register.flags.NF && Register.reg8.A > 0x99) || Register.flags.CF) {
		add |= 0x60; //same overflow for the higher nibble
		Register.flags.CF = 1;
	}
	Register.reg8.A += Register.flags.NF ? -add : add;
	Register.flags.ZF = Register.reg8.A == 0;
	Register.flags.HF = 0;
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

	u16 val = Gameboy.ReadMemory(Register.sp);
	Register.sp += 2;
	dest = val;
	Register.reg8.F = Register.reg8.F & 0xf0;

	//dest = Gameboy.ReadMemory(Register.sp++);
	//dest |= (static_cast<uint16_t>(Gameboy.ReadMemory(Register.sp++)) << 8);
	//Register.reg8.F = Register.reg8.F & 0xf0;
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

FINLINE void LR35902::RLA(bool throughTheCarry)
{
	uint8_t rotatedA;
	if (throughTheCarry)
		rotatedA = Register.reg8.A << 1 | (Register.flags.CF ? 1 : 0);
	else
		rotatedA = Register.reg8.A << 1 | Register.reg8.A >> 7;
	
	Register.reg8.F = Register.reg8.A & 1 << 7 ? 0x10 : 0;
	Register.reg8.A = rotatedA;
}

//RotateLeft
FINLINE void LR35902::RL(uint8_t& toRotate)
{
	uint8_t res = (toRotate << 1) | (Register.flags.CF ? 1 : 0);
	Register.flags.ZF = res == 0;
	Register.flags.NF = 0;
	Register.flags.HF = 0;
	Register.flags.CF = toRotate >>= 7;
	toRotate = res;

	//const bool msb{ bool(toRotate & 0x80) };
	//toRotate <<= 1;
	//toRotate |= (Register.flags.CF << 0);

	//Register.reg8.F = 7;
	//Register.flags.CF = msb;
	//Register.flags.ZF = !toRotate;
}

//RotateRightCarry
FINLINE void LR35902::RRC(uint8_t& toRotate)
{
	Register.reg8.F = 0;
	const bool lsb{ static_cast<bool>(toRotate & 0x1) };
	toRotate = static_cast<uint8_t>((toRotate >> 1) | (lsb << 7));

	//toRotate >>= 1;
	//toRotate |= uint8_t(lsb);
	Register.flags.CF = lsb;
	Register.flags.ZF = !toRotate;
	Register.flags.HF = false;
	Register.flags.NF = false;
}

//RotateRightRegisterA
FINLINE void LR35902::RRCA()
{
	Register.reg8.F = Register.reg8.A & 1 ? 0x10 : 0;
	Register.reg8.A = Register.reg8.A >> 1 | (Register.reg8.A & 1) << 7;
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

//RotateRegisterARight
FINLINE void LR35902::RRA()
{
	const uint8_t resultingRotation = Register.reg8.A >> 1 | Register.flags.CF << 7;
	Register.flags.ZF = 0;
	Register.flags.NF = 0;
	Register.flags.HF = 0;
	Register.flags.CF = Register.reg8.A & 0x1;
	Register.reg8.A = resultingRotation;
}

//ShiftLeftArithmetic (even though it's a logical shift..)
/*
Shift the contents of register toShift to the left.
That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2.
The same operation is repeated in sequence for the rest of the register.
The contents of bit 7 are copied to the CY flag, and bit 0 of register toShift is reset to 0.
*/
FINLINE void LR35902::SLA(uint8_t& toShift)
{
	Register.flags.CF = toShift >> 7;
	toShift <<= 1;
	Register.flags.ZF = toShift == 0;
	Register.flags.NF = 0;
	Register.flags.HF = 0;

	//Register.flags.CF = (toShift & 0x80) > 0;
	//toShift <<= 1;
	//toShift &= ~(1u << 0);
	//Register.flags.ZF = !toShift;
	//Register.flags.NF = 0;
	//Register.flags.HF = 0;
}

//ShiftRightArithmetic
FINLINE void LR35902::SRA(uint8_t& toShift)
{
	Register.flags.CF = toShift & 0x01;
	toShift = (toShift >> 1) | (toShift & (1 << 7));
	Register.flags.ZF = toShift == 0;
	Register.flags.NF = 0;
	Register.flags.HF = 0;

	//Register.reg8.F = 0;
	//Register.flags.CF = toShift & 0x1;
	//toShift >>= 1;
	//Register.flags.ZF = !toShift;
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
	Register.flags.NF = 0;
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
	Register.flags.CF = Register.flags.CF ? 0 : 1;
	Register.flags.NF = 0;
	Register.flags.HF = 0;
}

FINLINE void LR35902::SCF()
{
	Register.flags.NF = Register.flags.HF = 0;
	Register.flags.CF = 1;
}

FINLINE void LR35902::HALT() { Gameboy.SetPaused(true); } //Until an interrupt
FINLINE void LR35902::STOP() { Gameboy.SetPaused(true); }//Until button press
FINLINE void LR35902::DI() { InteruptsEnabled = 0; }
FINLINE void LR35902::EI() { InteruptsEnabled = 1; }
FINLINE void LR35902::NOP() {}

//ComPlemenT
FINLINE void LR35902::CPL()
{
	Register.reg8.A = ~Register.reg8.A;
	Register.flags.NF = Register.flags.HF = 1;
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