/*DISCLAIMER: Some notes have been copied from gbdev.gg8.se*/
#pragma once
#include <string>
#include "LR35902.h"
#include <vector>
#include <bitset>
#include "Register.h"

class FrameBuffer;

enum MBCs : uint8_t { none, mbc1, mbc2, mbc3, mbc4, mbc5, unknown };
enum Interupts : uint8_t
{
	vBlank = 0x40,
	lcdStat = 0x48,
	timer = 0x50,
	serial = 0x58,
	joypad = 0x60
};
enum Key : uint8_t { right, bButton, left, aButton, up, select, down, start };

struct GameHeader final
{
	char title[16 + 1]{}; //0x0134-0x0143 (depending on the cartridge, a manufacturer and cgb flag might be mixed in here)
	MBCs mbc{};
	uint8_t ramSizeValue{};
};

/**
 * \brief The Gameboy class, to be instantiated for every gameboy instance\n Can be seen as the bus..
 */
class GameBoy final
{
public:
	GameBoy() : IsPaused{ false }, IsRunning{ false }, IsCycleFrameBound{ false }, OnlyDrawLast{ false }, AutoSpeed{ false } {};
	GameBoy(const std::string& gameFile);
	~GameBoy() = default;

	void Update();
	void LoadGame(const std::string& gbFile);

	void SetRunningVariable(bool isRunning) { IsRunning = isRunning; }
	GameHeader ReadHeader();

	void TestCPU();
	void Disassemble();
	void AssignDrawCallback(const std::function<void(const FrameBuffer&)>& _vblank_callback);

	/**
	 * \brief Provides access to the raw memory array
	 * \return The Memory array (does not include rom(banks) or ram)
	 * \note With great power..
	 */
	uint8_t* GetRawMemory() noexcept { return Memory; };

	void WriteMemory(uint16_t address, uint8_t data);
	void WriteMemoryWord(const uint16_t pos, const uint16_t value);

	uint8_t ReadMemory(uint16_t pos);
	uint16_t ReadMemoryWord(uint16_t& pos);

	//uint8_t& GetIF() noexcept { return interrupt_flag.ref(); }
	//uint8_t GetIE() noexcept { return interrupt_enabled.ref(); }
	uint8_t& GetIF() const noexcept { return IF; }
	uint8_t GetIE() const noexcept { return IE; }

	ByteRegister interrupt_flag;
	ByteRegister interrupt_enabled;

	/**
	 *\brief LCDControl
	 *\note \code{.unparsed} Bit 7 - LCD Display Enable             (0=Off, 1=On)
 Bit 6 - Window Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF)
 Bit 5 - Window Display Enable          (0=Off, 1=On)
 Bit 4 - BG & Window Tile Data Select   (0=8800-97FF, 1=8000-8FFF)
 Bit 3 - BG Tile Map Display Select     (0=9800-9BFF, 1=9C00-9FFF)
 Bit 2 - OBJ (Sprite) Size              (0=8x8, 1=8x16)
 Bit 1 - OBJ (Sprite) Display Enable    (0=Off, 1=On)
 Bit 0 - BG/Window Display/Priority     (0=Off, 1=On)\endcode*/
	uint8_t& GetLCDC() const noexcept { return LCDC; }
	/**
	 * \brief LCDStatus
	 * \note \code{.unparsed} Bit 6 - LYC=LY Coincidence Interrupt (1=Enable) (Read/Write)
 Bit 5 - Mode 2 OAM Interrupt         (1=Enable) (Read/Write)
 Bit 4 - Mode 1 V-Blank Interrupt     (1=Enable) (Read/Write)
 Bit 3 - Mode 0 H-Blank Interrupt     (1=Enable) (Read/Write)
 Bit 2 - Coincidence Flag  (0:LYC<>LY, 1:LYC=LY) (Read Only)
 Bit 1-0 - Mode Flag       (Mode 0-3, see below) (Read Only)
		   0: During H-Blank
		   1: During V-Blank
		   2: During Searching OAM
		   3: During Transferring Data to LCD Driver\endcode
	 */
	uint8_t& GetLCDS() const noexcept { return LCDS; }
	uint8_t& GetLY() const noexcept { return LY; }
	/**
	 * \note\code{.unparsed} Bit 7-6 - Shade for Color Number 3
 Bit 5-4 - Shade for Color Number 2
 Bit 3-2 - Shade for Color Number 1
 Bit 1-0 - Shade for Color Number 0\endcode
	 */
	std::bitset<(160 * 144) * 2>& GetFramebuffer() noexcept { return frameBuffer; }
	const std::bitset<(160 * 144) * 2>& GetFramebuffer() const noexcept { return frameBuffer; }
	uint8_t GetPixelColor(const uint16_t pixel) const { return frameBuffer[pixel * 2] << 1 | static_cast<uint8_t>(frameBuffer[pixel * 2 + 1]); }

	void AddCycles(const unsigned cycles) { CurrentCycles += cycles; }
	unsigned int GetCycles() const noexcept { return CurrentCycles; }
	void RequestInterrupt(Interupts bit) noexcept;
	void SetKey(const Key key, const bool pressed);

	void SetPaused(const bool isPaused) noexcept { IsPaused = isPaused; }
	bool GetPaused() const noexcept { return IsPaused; }

	void SetSpeedMultiplier(const uint16_t speed) noexcept { SpeedMultiplier = speed; }
	uint16_t GetSpeedMultiplier() const noexcept { return SpeedMultiplier; }
	void SetAutoSpeed(const bool state) noexcept { AutoSpeed = state; }

	void SetCyclesToRun(const unsigned short cycles) noexcept
	{
		CyclesFramesToRun = cycles;
		IsCycleFrameBound = 2;
	}

	void SetFramesToRun(const unsigned short frames) noexcept
	{
		CyclesFramesToRun = frames;
		IsCycleFrameBound = 1;
	}

	/**
	 * \brief Whether we only want to draw the "last" frame
	 * \note Only works if we're Frame bound
	 */
	void SetOnlyDrawLastFrame(const bool state) noexcept { OnlyDrawLast = state; }

	void SetColor0(float* color);
	void SetColor1(float* color);
	void SetColor2(float* color);
	void SetColor3(float* color);

private:
	bool m_TestingOpcodes = false;
	std::string fileName{};

	LR35902 Cpu{ *this };

	unsigned int CurrentCycles{};
	unsigned int DivCycles{}, TIMACycles{};

	std::bitset<(160 * 144) * 2> frameBuffer{};

	uint8_t& DIVTimer{ (Memory[0xFF04]) }; ///< DIVider\note Constant accumulation at 16384Hz, regardless\n Resets when written to
	uint8_t& TIMATimer{ (Memory[0xFF05]) }; ///< Timer Counter(Accumulator?)\note Incremented by the TAC frequency \n When it overflows, it is set equal to the TMA and an INT 50 is fired
	uint8_t& TMATimer{ (Memory[0xFF06]) }; ///< Timer Modulo\note Contents will be loaded into TIMA when TIMA overflows
	/**
	 * \brief Timer Control (TimerAccumulatorControl?)
	 * \note Controls the TIMA (not DIV)
	 * \note \code{.unparsed}Bit  2   - Timer Enable
Bits 1-0 - Input Clock Select
	00: CPU Clock / 1024 (DMG, CGB:   4096 Hz, SGB:   ~4194 Hz)
	01: CPU Clock / 16   (DMG, CGB: 262144 Hz, SGB: ~268400 Hz)
	10: CPU Clock / 64   (DMG, CGB:  65536 Hz, SGB:  ~67110 Hz)
	11: CPU Clock / 256  (DMG, CGB:  16384 Hz, SGB:  ~16780 Hz)\endcode
	 */
	uint8_t& TACTimer{ (Memory[0xFF07]) };
	uint8_t& IF{ (Memory[0xFF0F]) }; ///< Interrupt Flag
	uint8_t& IE{ (Memory[0xFFFF]) }; ///< Interrupts Enabled
	/**
	 *\brief LCDControl
	 *\note \code{.unparsed} Bit 7 - LCD Display Enable             (0=Off, 1=On)
 Bit 6 - Window Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF)
 Bit 5 - Window Display Enable          (0=Off, 1=On)
 Bit 4 - BG & Window Tile Data Select   (0=8800-97FF, 1=8000-8FFF)
 Bit 3 - BG Tile Map Display Select     (0=9800-9BFF, 1=9C00-9FFF)
 Bit 2 - OBJ (Sprite) Size              (0=8x8, 1=8x16)
 Bit 1 - OBJ (Sprite) Display Enable    (0=Off, 1=On)
 Bit 0 - BG/Window Display/Priority     (0=Off, 1=On)\endcode*/
	uint8_t& LCDC{ (Memory[0xFF40]) };
	/**
	 * \brief LCDStatus
	 * \note \code{.unparsed} Bit 6 - LYC=LY Coincidence Interrupt (1=Enable) (Read/Write)
 Bit 5 - Mode 2 OAM Interrupt         (1=Enable) (Read/Write)
 Bit 4 - Mode 1 V-Blank Interrupt     (1=Enable) (Read/Write)
 Bit 3 - Mode 0 H-Blank Interrupt     (1=Enable) (Read/Write)
 Bit 2 - Coincidence Flag  (0:LYC<>LY, 1:LYC=LY) (Read Only)
 Bit 1-0 - Mode Flag       (Mode 0-3, see below) (Read Only)
		   0: During H-Blank
		   1: During V-Blank
		   2: During Searching OAM
		   3: During Transferring Data to LCD Driver\endcode
	 */
	uint8_t& LCDS{ (Memory[0xFF41]) };
	uint8_t& LY{ (Memory[0xFF44]) }; ///< LY\note The LY indicates the vertical line to which the present data is transferred to the LCD Driver. The LY can take on any value between 0 through 153. The values between 144 and 153 indicate the V-Blank period.
	uint16_t CyclesFramesToRun{};
	uint16_t SpeedMultiplier{ 1 };

	uint8_t Memory[0x10000]; //The Entire addressable range; "memory" is not quite right.. //TODO: Improve (not all locations will be used)
	struct
	{
		uint8_t romBank = 5;
		uint8_t ramBank = 0;
		uint8_t ramOrRomBank = 2;
		bool isRam = true;

		uint8_t GetRomBank() const noexcept
		{
			return isRam ? romBank : ramOrRomBank << 5 | romBank;
		}

		uint8_t GetRamBank() const noexcept
		{
			return isRam ? ramBank : 0;
		}
	} ActiveRomRamBank{};
	MBCs Mbc{};
	std::vector<uint8_t> RamBanks{};
	std::vector<uint8_t> Rom{};
	bool RamBankEnabled = false;
	bool RamOverRtc = true;

	uint8_t JoyPadState{ 0xFF };///< States of all 8 keys\n 1==NOT pressed
	bool IsPaused : 1;
	bool IsRunning : 1;
	uint8_t IsCycleFrameBound : 2; ///< Whether we're cycle, frame or not bound\note Bit 0: Frame\n Bit 1: Cycle\n Can't be both
	bool OnlyDrawLast : 1;
	bool AutoSpeed : 1;
	bool UNUSED : 1;

	void HandleTimers(unsigned stepCycles, unsigned cycleBudget);
	uint8_t GetJoypadState() const;

	constexpr bool InRange(const unsigned int value, const unsigned int min, const unsigned int max) const noexcept
	{
		return value - min <= max - min;
	}

	//MEMORY BANK CONTROLLERS
private:
	void MBCWrite(const uint16_t& address, const uint8_t byte);
	uint8_t MBCRead(const uint16_t& address);

	void MBCNoneWrite(const uint16_t& address, const uint8_t byte);
	uint8_t MBCNoneRead(const uint16_t& address);

	void MBC1Write(const uint16_t& address, const uint8_t byte);
	uint8_t MBC1Read(const uint16_t& address);

	void MBC3Write(const uint16_t& address, const uint8_t byte);
	uint8_t MBC3Read(const uint16_t& address);

	void MBC5Write(const uint16_t& address, const uint8_t byte);
	uint8_t MBC5Read(const uint16_t& address);

	void SwitchRomBank(uint8_t bank);
	void SwitchRamBank(uint8_t bank);
};
