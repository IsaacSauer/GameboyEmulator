#include "pch.h"
#include "Emulator.h"
#include <bitset>
#include "GameBoy.h"
#include <thread>

gbee::Emulator::Emulator(const std::string& gbfile, const uint8_t instances) : m_InstanceCount{ instances }
{
	m_Instances = new GameBoy[m_InstanceCount];
}

gbee::Emulator::~Emulator()
{
	m_Instances[0].SetRunningVariable(false);
	delete[] m_Instances;
}

void gbee::Emulator::LoadGame(const std::string& gbFile) const
{
	m_Instances[0].LoadGame(gbFile);
}

void gbee::Emulator::Start()
{
	m_Instances[0].SetRunningVariable(true);
	m_Thread = std::thread{ &GameBoy::Update, std::ref(m_Instances[0]) };
}

void gbee::Emulator::Stop()
{
	m_Instances[0].SetRunningVariable(false);
}

void gbee::Emulator::AssignDrawCallback(const std::function<void(const std::vector<uint16_t>&)>&& _vblank_callback)
{
	m_Instances[0].AssignDrawCallback(_vblank_callback);
}

void gbee::Emulator::Reset()
{
	if (m_Instances)
	{
		m_Instances[0].SetRunningVariable(false);
		delete[] m_Instances;
		m_Instances = new GameBoy[m_InstanceCount];
	}
}

void gbee::Emulator::TestCPU() const
{
	for (uint8_t i{ 0 }; i < m_InstanceCount; ++i)
		m_Instances[i].TestCPU();
}

std::bitset<160 * 144 * 2> gbee::Emulator::GetFrameBuffer(const uint8_t instanceID) const
{
	return m_Instances[instanceID].GetFramebuffer();
}

void gbee::Emulator::SetKeyState(const uint8_t key, const bool state, const uint8_t instanceID) const
{
	m_Instances[instanceID].SetKey((Key)key, state);
}

void gbee::Emulator::SetPauseState(const bool state, const uint8_t instanceID) const
{
	m_Instances[instanceID].SetPaused(state);
}

bool gbee::Emulator::GetPauseState(const uint8_t instanceID) const
{
	return m_Instances[instanceID].GetPaused();
}

void gbee::Emulator::SetSpeed(const uint16_t cycleMultiplier, const uint8_t instanceID) const
{
	m_Instances[instanceID].SetSpeedMultiplier(cycleMultiplier);
}

uint16_t gbee::Emulator::GetSpeed(const uint8_t instanceID) const
{
	return m_Instances[instanceID].GetSpeedMultiplier();
}

void gbee::Emulator::SetAutoSpeed(const bool onOff, const uint8_t instanceID) const
{
	m_Instances[instanceID].SetAutoSpeed(onOff);
}

void gbee::Emulator::Join()
{
	m_Thread.join();
}

void gbee::Emulator::SetColor(int color, float* value)
{
	if (value && m_Instances)
		m_Instances[0].SetColor(color, value);
}

void gbee::Emulator::RunForCycles(const unsigned short cycles, const uint8_t instanceID) const
{
	m_Instances[instanceID].SetCyclesToRun(cycles);
}

void gbee::Emulator::RunForFrames(const unsigned short frames, const bool onlyDrawLastFrame, const uint8_t instanceID) const
{
	m_Instances[instanceID].SetFramesToRun(frames);
	m_Instances[instanceID].SetOnlyDrawLastFrame(onlyDrawLastFrame);
}

void gbee::Emulator::Update(const float fps) const
{
	for (uint8_t i{ 0 }; i < m_InstanceCount; ++i)
	{
		m_Instances[i].Update();
	}
}