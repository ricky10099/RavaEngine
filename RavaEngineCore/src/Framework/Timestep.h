#pragma once

namespace Rava {
class Timestep {
   public:
	Timestep(std::chrono::duration<float, std::chrono::seconds::period> time);

	Timestep& operator=(const std::chrono::duration<float, std::chrono::seconds::period>& timestep);
	Timestep& operator-=(const Timestep& other);
	Timestep operator-(const Timestep& other) const;
	bool operator<=(const std::chrono::duration<float, std::chrono::seconds::period>& other) const;
	operator float() const { return m_timestep.count(); }

	void Print() const;
	static float Count() { return m_timestep.count(); }
	std::chrono::duration<float, std::chrono::seconds::period> GetSeconds() const;
	std::chrono::duration<float, std::chrono::milliseconds::period> GetMilliseconds() const;

   private:
	static std::chrono::duration<float, std::chrono::seconds::period> m_timestep;
};
}  // namespace Rava