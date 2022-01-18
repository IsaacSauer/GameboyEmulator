#pragma once
#include <chrono>
class Measure {
public:
	Measure(const std::string& name)
		: m_Name(name)
	{
		//m_StartTimepoint = std::chrono::high_resolution_clock::now();
	}

	void Start()
	{
		m_StartTimepoint = std::chrono::high_resolution_clock::now();
	}

	//Return microseconds
	int64_t Stop()
	{
		const auto endTimepoint = std::chrono::high_resolution_clock::now();

		const int64_t begin = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
		const int64_t end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

		return end - begin;
	}

private:
	std::string m_Name;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
};

//#define STR(s) #s
//#define XSTR(s) STR(s)
//
//#define MEASURE 1
//#if MEASURE
//#define START_MEASUREMENT() dae::Measure Time{__FUNCTION__ "-" XSTR(__LINE__)}
//#else
//#define START_MEASUREMENT()
//#endif
