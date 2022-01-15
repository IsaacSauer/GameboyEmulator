#pragma once
#include <bitset>
#include <functional>
#include <thread>

class FrameBuffer;
class GameBoy;

/**
 * \brief GameBoy Emulation Environment
 */
namespace gbee
{
	class Emulator final
	{
	public:
		Emulator(const std::string& gbfile, const uint8_t instances);
		~Emulator();

		Emulator(const Emulator& rhs) = delete; //Copy constructor
		Emulator(Emulator&& lhs) = delete; //Move Constructor
		Emulator& operator=(const Emulator& rhs) = delete; //Copy Assignment
		Emulator& operator=(Emulator&& lhs) = delete; //Move Assignment

		void LoadGame(const std::string& gbFile);
		void Start();
		void Stop();
		void AssignDrawCallback(const std::function<void(const FrameBuffer&)>&& _vblank_callback);
		void TestCPU() const;

		std::bitset<160 * 144 * 2> GetFrameBuffer(const uint8_t instanceID);
		void SetKeyState(const uint8_t key, const bool state, const uint8_t instanceID);
		//void RunForMs(const unsigned int ms, const uint8_t instanceID);		Not implemented, The user should do the timing..

		void RunForCycles(const unsigned short cycles, const uint8_t instanceID);
		void RunForFrames(const unsigned short frames, const bool onlyDrawLastFrame, const uint8_t instanceID);

		void SetPauseState(const bool state, const uint8_t instanceID);
		bool GetPauseState(const uint8_t instanceID) const;

		void SetSpeed(const uint16_t cycleMultiplier, const uint8_t instanceID);
		uint16_t GetSpeed(const uint8_t instanceID) const;
		void SetAutoSpeed(const bool onOff, const uint8_t instanceID); //Worthless really, when speed is set to MAXINT, it'll go as fast as possible..

		void Join();

	private:
		GameBoy* Instance = nullptr;
		//std::string GbFile;

		void Update(float fps) ;

		std::thread thread{};
	};
}
