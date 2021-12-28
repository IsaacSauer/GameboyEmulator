#include "pch.h"
#include "Emulator.h"
#include <bitset>
#include "GameBoy.h"
#include <thread>

gbee::Emulator::Emulator(const std::string& gbfile, const uint8_t instances) : InstanceCount{ instances }
{
	Instances = new GameBoy[InstanceCount];
}

gbee::Emulator::~Emulator()
{
	for (int i{ 0 }; i < InstanceCount; ++i)
	{
		Instances[i].SetRunningVariable(false);
	}
	delete[] Instances;
}

void gbee::Emulator::LoadGame(const std::string& gbFile, bool testCPU) const
{
	for (uint8_t i{ 0 }; i < InstanceCount; ++i)
	{
		if(testCPU)
			Instances[i].TestCPU();

		Instances[i].LoadGame(gbFile);
		Instances[i].SetRunningVariable(true);
	}
}

void gbee::Emulator::Start() const
{
	for (long i{ 0 }; i < InstanceCount; ++i)
	{
		Instances[i].SetRunningVariable(true);
		std::thread t = std::thread{ &GameBoy::Update, std::ref(Instances[i]) };
		t.detach(); //We don't need to sync them, ever..
	}
}

std::bitset<160 * 144 * 2> gbee::Emulator::GetFrameBuffer(const uint8_t instanceID) const
{
	return Instances[instanceID].GetFramebuffer();
}

void gbee::Emulator::SetKeyState(const uint8_t key, const bool state, const uint8_t instanceID) const
{
	Instances[instanceID].SetKey((Key)key, state);
}

void gbee::Emulator::SetPauseState(const bool state, const uint8_t instanceID) const
{
	Instances[instanceID].SetPaused(state);
}

bool gbee::Emulator::GetPauseState(const uint8_t instanceID) const
{
	return Instances[instanceID].GetPaused();
}

void gbee::Emulator::SetSpeed(const uint16_t cycleMultiplier, const uint8_t instanceID) const
{
	Instances[instanceID].SetSpeedMultiplier(cycleMultiplier);
}

uint16_t gbee::Emulator::GetSpeed(const uint8_t instanceID) const
{
	return Instances[instanceID].GetSpeedMultiplier();
}

void gbee::Emulator::SetAutoSpeed(const bool onOff, const uint8_t instanceID) const
{
	Instances[instanceID].SetAutoSpeed(onOff);
}

void gbee::Emulator::RunForCycles(const unsigned short cycles, const uint8_t instanceID) const
{
	Instances[instanceID].SetCyclesToRun(cycles);
}

void gbee::Emulator::RunForFrames(const unsigned short frames, const bool onlyDrawLastFrame, const uint8_t instanceID) const
{
	Instances[instanceID].SetFramesToRun(frames);
	Instances[instanceID].SetOnlyDrawLastFrame(onlyDrawLastFrame);
}

void gbee::Emulator::Update(const float fps) const
{
	for (uint8_t i{ 0 }; i < InstanceCount; ++i)
	{
		Instances[i].Update();
	}
}