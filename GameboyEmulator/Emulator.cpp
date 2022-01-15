#include "pch.h"
#include "Emulator.h"
#include <bitset>
#include "GameBoy.h"
#include <thread>

gbee::Emulator::Emulator(const std::string& gbfile, const uint8_t instances)
{
}

gbee::Emulator::~Emulator()
{
	Instance->SetRunningVariable(false);
	delete Instance;
}

void gbee::Emulator::LoadGame(const std::string& gbFile) 
{
	if (Instance)
	{
		Instance->SetRunningVariable(false);
		delete Instance;
	}

	Instance = new GameBoy(gbFile);

	Instance->LoadGame(gbFile);
}

void gbee::Emulator::Start()
{
	Instance->SetRunningVariable(true);
	thread = std::thread{ &GameBoy::Update, Instance };
	thread.detach(); //We don't need to sync them, ever..
}

void gbee::Emulator::Stop()
{
	Instance->SetRunningVariable(false);
}

void gbee::Emulator::AssignDrawCallback(const std::function<void(const FrameBuffer&)>&& _vblank_callback)
{
	Instance->AssignDrawCallback(_vblank_callback);
}

void gbee::Emulator::TestCPU() const
{
	Instance->TestCPU();
}

std::bitset<160 * 144 * 2> gbee::Emulator::GetFrameBuffer(const uint8_t instanceID) 
{
	return Instance->GetFramebuffer();
}

void gbee::Emulator::SetKeyState(const uint8_t key, const bool state, const uint8_t instanceID) 
{
	Instance->SetKey((Key)key, state);
}

void gbee::Emulator::SetPauseState(const bool state, const uint8_t instanceID) 
{
	Instance->SetPaused(state);
}

bool gbee::Emulator::GetPauseState(const uint8_t instanceID) const
{
	return Instance->GetPaused();
}

void gbee::Emulator::SetSpeed(const uint16_t cycleMultiplier, const uint8_t instanceID) 
{
	Instance->SetSpeedMultiplier(cycleMultiplier);
}

uint16_t gbee::Emulator::GetSpeed(const uint8_t instanceID) const
{
	return Instance->GetSpeedMultiplier();
}

void gbee::Emulator::SetAutoSpeed(const bool onOff, const uint8_t instanceID) 
{
	Instance->SetAutoSpeed(onOff);
}

void gbee::Emulator::Join()
{
	thread.join();
}

void gbee::Emulator::RunForCycles(const unsigned short cycles, const uint8_t instanceID) 
{
	Instance->SetCyclesToRun(cycles);
}

void gbee::Emulator::RunForFrames(const unsigned short frames, const bool onlyDrawLastFrame, const uint8_t instanceID) 
{
	Instance->SetFramesToRun(frames);
	Instance->SetOnlyDrawLastFrame(onlyDrawLastFrame);
}

void gbee::Emulator::Update(const float fps) 
{
	Instance->Update();
}