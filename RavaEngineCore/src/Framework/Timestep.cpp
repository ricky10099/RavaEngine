#include "ravapch.h"

#include "Framework/Timestep.h"

namespace Rava {
std::chrono::duration<float, std::chrono::seconds::period> Timestep::m_timestep = 0ms;

Timestep::Timestep(std::chrono::duration<float, std::chrono::seconds::period> time)
	/*: m_timestep(time) */{}

Timestep& Timestep::operator=(const std::chrono::duration<float, std::chrono::seconds::period>& timestep) {
	this->m_timestep = timestep;
	return *this;
}

Timestep& Timestep::operator-=(const Timestep& other) {
	m_timestep = m_timestep - other.m_timestep;
	return *this;
}

Timestep Timestep::operator-(const Timestep& other) const {
	return m_timestep - other.m_timestep;
}

bool Timestep::operator<=(const std::chrono::duration<float, std::chrono::seconds::period>& other) const {
	return (m_timestep - other) <= 0ms;
}

std::chrono::duration<float, std::chrono::seconds::period> Timestep::GetSeconds() const {
	return m_timestep;
}

std::chrono::duration<float, std::chrono::milliseconds::period> Timestep::GetMilliseconds() const {
	return (std::chrono::duration<float, std::chrono::milliseconds::period>)m_timestep;
}

void Timestep::Print() const {
	auto inMilliSeconds = GetMilliseconds();
	ENGINE_INFO("timestep in milli seconds: {0} ms", inMilliSeconds.count());
	auto inSeconds = GetSeconds();
	ENGINE_INFO("timestep in seconds: {0} s", inSeconds.count());
}
}  // namespace Rava