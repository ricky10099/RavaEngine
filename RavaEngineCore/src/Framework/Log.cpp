#include "ravapch.h"

namespace Rava {
std::shared_ptr<spdlog::logger> Log::s_engineLogger;
std::shared_ptr<spdlog::logger> Log::s_logger;

void Log::Init() {
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Rava.log", true));

	logSinks[0]->set_pattern("[%T] [%^%l%$] %n: %v");
	logSinks[1]->set_pattern("[%T] [%l] %n: %v");

	s_engineLogger = std::make_shared<spdlog::logger>("ENGINE", begin(logSinks), end(logSinks));
	spdlog::register_logger(s_engineLogger);
	s_engineLogger->set_level(spdlog::level::trace);
	s_engineLogger->flush_on(spdlog::level::trace);

	s_logger = std::make_shared<spdlog::logger>("LOG", begin(logSinks), end(logSinks));
	spdlog::register_logger(s_logger);
	s_logger->set_level(spdlog::level::trace);
	s_logger->flush_on(spdlog::level::trace);
}
}  // namespace Rava