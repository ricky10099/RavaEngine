#pragma once

#include "ravapch.h"

namespace Rava {
std::string GetPathWithoutFileName(const std::filesystem::path& path) {
	std::filesystem::path withoutFilename{std::filesystem::path(path.parent_path())};
	std::string pathWithoutFilename = withoutFilename.string();
	if (!pathWithoutFilename.empty()) {
		if (pathWithoutFilename.back() != '/') {
			pathWithoutFilename += '/';
		}
	}
	return pathWithoutFilename;
}
}  // namespace Rava