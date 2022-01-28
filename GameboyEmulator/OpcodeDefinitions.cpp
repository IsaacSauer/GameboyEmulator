#include "pch.h"
#include "LR35902.h"
#include "GameBoy.h"
//Add the signature to LR35902.h

#pragma region ALU

FINLINE void LR35902::ADD(uint8_t toAdd)
{
	uint16_t result = m_Register.reg8.A + toAdd;
	m_Register.flags.ZF = (uint8_t)result == 0;
	m_Register.flags.NF = 0;
	m_Register.flags.HF = (m_Register.reg8.A ^ toAdd ^ result) & 0x10 ? 1 : 0;
	m_Register.flags.CF = result & 0x100 ? 1 : 0;
	m_Register.reg8.A = (uint8_t)result;
}

FINLINE void LR35902::SUB(uint8_t toSub)
{
	uint8_t result = m_Register.reg8.A - toSub;
	m_Register.flags.ZF = result == 0;
	m_Register.flags.NF = 1;
	m_Register.flags.HF = ((int32_t)m_Register.reg8.A & 0xf) - (toSub & 0xf) < 0;
	m_Register.flags.CF = m_Register.reg8.A < toSub;
	m_Register.reg8.A = result;
}

//Add Carry
//Add the contents of register A and the x flag to the contents of register x, and store the results in register A.
FINLINE void LR35902::ADC(uint8_t toAdd)
{
	u16 result = m_Register.reg8.A + toAdd + m_Register.flags.CF;
	m_Register.flags.ZF = (uint8_t)result == 0;
	m_Register.flags.NF = 0;
	m_Register.flags.HF = (m_Register.reg8.A ^ toAdd ^ result) & 0x10 ? 1 : 0;
	m_Register.flags.CF = result & 0x100 ? 1 : 0;
	m_Register.reg8.A = (uint8_t)result;
}

FINLINE void LR35902::ADDToHL(uint16_t toAdd)
{
	u32 toAddAddedToHL = m_Register.reg16.HL + toAdd;
	m_Register.flags.NF = 0;
	m_Register.flags.HF = (((m_Register.reg16.HL & 0xfff) + (toAdd & 0xfff)) & 0x1000) ? 1 : 0;
	m_Register.flags.CF = toAddAddedToHL > 0xffff;
	m_Register.reg16.HL = toAddAddedToHL;
}

FINLINE void LR35902::ADDToSP()
{
	int8_t off = (int8_t)m_Gameboy.ReadMemory(m_Register.pc);
	uint32_t res = m_Register.sp + off;
	m_Register.flags.ZF = 0;
	m_Register.flags.NF = 0;
	m_Register.flags.HF = (m_Register.sp & 0xf) + (m_Gameboy.ReadMemory(m_Register.pc) & 0xf) > 0xf;
	m_Register.flags.CF = (m_Register.sp & 0xff) + (m_Gameboy.ReadMemory(m_Register.pc) & 0xff) > 0xff;
	m_Register.sp = res;
	m_Register.pc++;
}

FINLINE void LR35902::ADD16(uint16_t toAdd)
{
	const int8_t toAddS{ static_cast<int8_t>(toAdd) };
	if (toAddS >= 0)
	{ //Positive
		m_Register.flags.HF = (m_Register.sp & 0xfff) + (toAdd & 0xfff) & 0x1000; //h is always 3->4 in the high byte
		m_Register.flags.CF = static_cast<uint32_t>(m_Register.sp) + toAdd > 0xFFFF;
	}
	else
	{ //Negative
		m_Register.flags.HF = static_cast<int16_t>(m_Register.sp & 0xf) + (toAddS & 0xf) <
			0; // Check if subtracting the last 4 bits goes negative, indicating a borrow //Could also do ((Register.sp+toAddS)^toAddS^Register.sp)&0x10
		m_Register.flags.CF = static_cast<uint32_t>(m_Register.sp) + toAddS < 0; //If we go negative, it would cause an underflow on unsigned, setting the carry
	}
	m_Register.sp += toAddS;
	m_Register.flags.ZF = 0;
	m_Register.flags.NF = 0; //even tho we might've done a subtraction...
}

FINLINE void LR35902::SBC(uint8_t toSub)
{
	uint8_t result = m_Register.reg8.A - toSub - m_Register.flags.CF;
	m_Register.flags.ZF = result == 0;
	m_Register.flags.NF = 1;
	m_Register.flags.HF = ((int32_t)m_Register.reg8.A & 0xf) - (toSub & 0xf) - m_Register.flags.CF < 0;
	m_Register.flags.CF = m_Register.reg8.A < toSub + m_Register.flags.CF;
	m_Register.reg8.A = result;
}

FINLINE void LR35902::OR(const uint8_t toOr)
{
	m_Register.reg8.F = 0;
	m_Register.reg8.A |= toOr;
	m_Register.flags.ZF = !m_Register.reg8.A;
}

FINLINE void LR35902::INC(uint8_t& toInc)
{
	m_Register.flags.NF = 0;
	//Register.flags.CF; //Does not affect the carry flag! The INC/DEC opcode is often used to control loops; Loops are often used for multiple precision arithmetic so, to prevent having to push the carry state after every loop they just made the instruction ignore it! :D
	m_Register.flags.HF = static_cast<uint8_t>((toInc & 0xF) == 0xF);
	m_Register.flags.ZF = !(++toInc);
}

FINLINE void LR35902::INC(uint16_t& toInc)
{
	//No flags for 16 bit variant
	++toInc;
}

FINLINE void LR35902::AND(uint8_t toAnd)
{
	m_Register.reg8.F = 0;
	m_Register.flags.HF = 1; //Why tho?
	m_Register.reg8.A &= toAnd;
	m_Register.flags.ZF = !m_Register.reg8.A;

	//Register.reg8.F = (Register.reg8.A == 0) ? (Register.flags.HF | Register.flags.ZF) : Register.flags.HF;
}

FINLINE void LR35902::DEC(uint8_t& toDec)
{
	m_Register.flags.NF = 1;
	//Register.flags.CF; //Does not affect the carry flag! The INC/DEC opcode is often used to control loops; Loops are often used for multiple precision arithmetic so, to prevent having to push the carry state after every loop they just made the instruction ignore it! :D
	m_Register.flags.HF = !(toDec & 0xF);
	m_Register.flags.ZF = !--toDec;
}

FINLINE void LR35902::DEC(uint16_t& toDec)
{
	//No flags for 16 bit variant
	--toDec;
}

FINLINE void LR35902::XOR(const uint8_t toXor)
{
	m_Register.reg8.F = 0;
	m_Register.reg8.A ^= toXor;
	m_Register.flags.ZF = !m_Register.reg8.A;
}

FINLINE void LR35902::CP(uint8_t toCompare)
{
	m_Register.flags.NF = 1;
	m_Register.flags.ZF = !(m_Register.reg8.A - toCompare);
	m_Register.flags.CF = m_Register.reg8.A < toCompare;
	m_Register.flags.HF = static_cast<int16_t>(m_Register.reg8.A & 0xF) - (toCompare & 0xF) < 0;
}

//DecimalAdjustA, implementation heavily inspired by Richeson's paper
FINLINE void LR35902::DAA()
{
	s8 add = 0;
	if ((!m_Register.flags.NF && (m_Register.reg8.A & 0xf) > 0x9) || m_Register.flags.HF) //if we did an initial overflow on the lower nibble or have exceeded 9 (the max value in BCD)
		add |= 0x6;
	if ((!m_Register.flags.NF && m_Register.reg8.A > 0x99) || m_Register.flags.CF)
	{
		add |= 0x60; //same overflow for the higher nibble
		m_Register.flags.CF = 1;
	}
	m_Register.reg8.A += m_Register.flags.NF ? -add : add;
	m_Register.flags.ZF = m_Register.reg8.A == 0;
	m_Register.flags.HF = 0;
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
FINLINE void LR35902::LD(const uint16_t destAddrs, const uint8_t data) { m_Gameboy.WriteMemory(destAddrs, data); }

FINLINE void LR35902::PUSH(const uint16_t data) //little endian
{
	--m_Register.sp;
	m_Gameboy.WriteMemory(m_Register.sp, data >> 8);
	--m_Register.sp;
	m_Gameboy.WriteMemory(m_Register.sp, data & 0xFF);
}

FINLINE void LR35902::POP(uint16_t& dest)
{
	u16 val = m_Gameboy.ReadMemoryWord(m_Register.sp);
	dest = val;
	m_Register.reg8.F = m_Register.reg8.F & 0xf0;
}
#pragma endregion

#pragma region Rotates and Shifts
//RotateLeftCarry
FINLINE void LR35902::RLC(uint8_t& toRotate)
{
	m_Register.reg8.F = 0;
	const bool msb{ static_cast<bool>(toRotate & 0x80) };

	toRotate <<= 1;
	toRotate |= static_cast<uint8_t>(msb);
	m_Register.flags.CF = msb;
	m_Register.flags.ZF = !toRotate;
}

FINLINE void LR35902::RLA(bool throughTheCarry)
{
	uint8_t rotatedA;
	if (throughTheCarry)
		rotatedA = m_Register.reg8.A << 1 | (m_Register.flags.CF ? 1 : 0);
	else
		rotatedA = m_Register.reg8.A << 1 | m_Register.reg8.A >> 7;

	m_Register.reg8.F = m_Register.reg8.A & 1 << 7 ? 0x10 : 0;
	m_Register.reg8.A = rotatedA;
}

//RotateLeft
FINLINE void LR35902::RL(uint8_t& toRotate)
{
	uint8_t res = (toRotate << 1) | (m_Register.flags.CF ? 1 : 0);
	m_Register.flags.ZF = res == 0;
	m_Register.flags.NF = 0;
	m_Register.flags.HF = 0;
	m_Register.flags.CF = toRotate >>= 7;
	toRotate = res;
}

//RotateRightCarry
FINLINE void LR35902::RRC(uint8_t& toRotate)
{
	m_Register.reg8.F = 0;
	const bool lsb{ static_cast<bool>(toRotate & 0x1) };
	toRotate = static_cast<uint8_t>((toRotate >> 1) | (lsb << 7));

	m_Register.flags.CF = lsb;
	m_Register.flags.ZF = !toRotate;
	m_Register.flags.HF = false;
	m_Register.flags.NF = false;
}

//RotateRightRegisterA
FINLINE void LR35902::RRCA()
{
	m_Register.reg8.F = m_Register.reg8.A & 1 ? 0x10 : 0;
	m_Register.reg8.A = m_Register.reg8.A >> 1 | (m_Register.reg8.A & 1) << 7;
}

//RotateRight
FINLINE void LR35902::RR(uint8_t& toRotate)
{
	const bool lsb{ static_cast<bool>(toRotate & 0x1) };
	toRotate >>= 1;
	toRotate |= m_Register.flags.CF << 7;

	m_Register.reg8.F = 0;
	m_Register.flags.CF = lsb;
	m_Register.flags.ZF = !toRotate;
}

//RotateRegisterARight
FINLINE void LR35902::RRA()
{
	const uint8_t resultingRotation = m_Register.reg8.A >> 1 | m_Register.flags.CF << 7;
	m_Register.flags.ZF = 0;
	m_Register.flags.NF = 0;
	m_Register.flags.HF = 0;
	m_Register.flags.CF = m_Register.reg8.A & 0x1;
	m_Register.reg8.A = resultingRotation;
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
	m_Register.flags.CF = toShift >> 7;
	toShift <<= 1;
	m_Register.flags.ZF = toShift == 0;
	m_Register.flags.NF = 0;
	m_Register.flags.HF = 0;
}

//ShiftRightArithmetic
FINLINE void LR35902::SRA(uint8_t& toShift)
{
	m_Register.flags.CF = toShift & 0x01;
	toShift = (toShift >> 1) | (toShift & (1 << 7));
	m_Register.flags.ZF = toShift == 0;
	m_Register.flags.NF = 0;
	m_Register.flags.HF = 0;
}

//ShiftRightLogical
FINLINE void LR35902::SRL(uint8_t& toShift)
{
	m_Register.reg8.F = 0;
	m_Register.flags.CF = toShift & 0x1;
	toShift >>= 1;
	m_Register.flags.ZF = !toShift;
}
#pragma endregion

#pragma region Bits
FINLINE void LR35902::BITop(const uint8_t bit, const uint8_t data)
{
	m_Register.flags.ZF = !((data >> bit) & 1);
	m_Register.flags.NF = 0;
	m_Register.flags.HF = 1;
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
	m_Register.reg8.F = 0;
	m_Register.flags.ZF = !data;
}

//ComplementCarryFlag
FINLINE void LR35902::CCF()
{
	m_Register.flags.CF = m_Register.flags.CF ? 0 : 1;
	m_Register.flags.NF = 0;
	m_Register.flags.HF = 0;
}

FINLINE void LR35902::SCF()
{
	m_Register.flags.NF = m_Register.flags.HF = 0;
	m_Register.flags.CF = 1;
}

/*
After a HALT instruction is executed, the system clock is stopped and HALT mode is entered.
Although the system clock is stopped in this status,
the oscillator circuit and LCD controller continue to operate.

In addition, the status of the internal RAM register ports remains unchanged.

HALT mode is cancelled by an interrupt or reset signal.

The program counter is halted at the step after the HALT instruction.
If both the interrupt request flag and the corresponding interrupt enable flag are set,
HALT mode is exited, even if the interrupt master enable flag is not set.

Once HALT mode is cancelled, the program starts from the address indicated by the program counter.

If the interrupt master enable flag is set,
the contents of the program coounter are pushed to the stack
and control jumps to the starting address of the interrupt.

If the RESET terminal goes LOW in HALT mode,
the mode becomes that of a normal reset.
*/
FINLINE void LR35902::HALT()
{
	//Until an interrupt
	//Gameboy.SetPaused(true);
	m_Halted = true;
	//--Register.pc;
}

/*
Execution of a STOP instruction stops both the system clock and oscillator circuit.
STOP mode is entered and the LCD controller also stops.
However, the status of the internal RAM register ports remains unchanged.

STOP mode can be cancelled by a reset signal.

If the RESET terminal goes LOW in STOP mode, it becomes that of a normal reset status.

The following conditions should be met before a STOP instruction is executed and stop mode is entered:

-All interrupt-enable (IE) flags are reset.
-Input to P10-P13 is LOW for all.
*/
FINLINE void LR35902::STOP()
{
	m_Halted = true;
	//Until button press
	//Gameboy.SetPaused(true);
	//--Register.pc;
}

FINLINE void LR35902::DI() { m_InteruptsEnabled = 0; }
FINLINE void LR35902::EI() { m_InteruptsEnabled = 1; }
FINLINE void LR35902::NOP() {}

//ComPlemenT
FINLINE void LR35902::CPL()
{
	m_Register.reg8.A = ~m_Register.reg8.A;
	m_Register.flags.NF = m_Register.flags.HF = 1;
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
		m_Gameboy.WriteMemoryWord(m_Register.sp -= 2, m_Register.pc);
		m_Register.pc = address;
		m_Gameboy.AddCycles(24); //CodeSlinger:12
	}
	else
		m_Gameboy.AddCycles(12);
}

/**
 * \note	<b>This function handles adding the cycles!</b>
 */
FINLINE void LR35902::JR(const int8_t offset, bool doJump)
{
	if (doJump)
	{
		m_Register.pc += offset;
		m_Gameboy.AddCycles(12); //CodeSlinger:8
	}
	else
		m_Gameboy.AddCycles(8);
}

FINLINE void LR35902::JP(const uint16_t address, const bool doJump, bool handleCycles)
{
	if (doJump)
	{
		m_Register.pc = address;
		m_Gameboy.AddCycles(handleCycles ? 16 : 0); //CodeSlinger:12:0
	}
	else
		m_Gameboy.AddCycles(handleCycles ? 12 : 0);
}

FINLINE void LR35902::RET(bool doReturn, bool handleCycles)
{
	if (doReturn)
	{
		m_Register.pc = m_Gameboy.ReadMemoryWord(m_Register.sp);
		m_Gameboy.AddCycles(handleCycles ? 20 : 0); //CodeSlinger:8
	}
	else
		m_Gameboy.AddCycles(handleCycles ? 8 : 0);
}

FINLINE void LR35902::RETI()
{
	m_Register.pc = m_Gameboy.ReadMemoryWord(m_Register.sp);
	m_InteruptsEnabled = true;
}

//same as call.. (apart from address size)
FINLINE void LR35902::RST(const uint8_t address)
{
	m_Gameboy.WriteMemoryWord(m_Register.sp -= 2, m_Register.pc);
	m_Register.pc = address;
}
#pragma endregion