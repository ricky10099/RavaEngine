#pragma once

namespace Rava {
class Log {
   public:
	Log() { Init(); }
	~Log() { Shutdown(); }

	static void Init();
	static void Shutdown() { spdlog::shutdown(); }

	static std::shared_ptr<spdlog::logger>& GetEngineLogger() { return s_engineLogger; }

	static std::shared_ptr<spdlog::logger>& GetLogger() { return s_logger; }

   private:
	static std::shared_ptr<spdlog::logger> s_engineLogger;
	static std::shared_ptr<spdlog::logger> s_logger;
};
}  // namespace Rava

template <typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector) {
	return os << glm::to_string(vector);
}

template <typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix) {
	return os << glm::to_string(matrix);
}

template <typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion) {
	return os << glm::to_string(quaternion);
}

#define ENGINE_TRACE(...)    Rava::Log::GetEngineLogger()->trace(__VA_ARGS__)
#define ENGINE_INFO(...)     Rava::Log::GetEngineLogger()->info(__VA_ARGS__)
#define ENGINE_WARN(...)     Rava::Log::GetEngineLogger()->warn(__VA_ARGS__)
#define ENGINE_ERROR(...)    Rava::Log::GetEngineLogger()->error(__VA_ARGS__)
#define ENGINE_CRITICAL(...) Rava::Log::GetEngineLogger()->critical(__VA_ARGS__); \
							throw std::runtime_error("Critical Error");

#define LOG_TRACE(...)    Rava::Log::GetLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)     Rava::Log::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)     Rava::Log::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    Rava::Log::GetLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) Rava::Log::GetLogger()->critical(__VA_ARGS__)
