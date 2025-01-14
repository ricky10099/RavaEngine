#pragma once

#include "ravapch.h"

//namespace Rava {
static std::string GetPathWithoutFileName(const std::filesystem::path& path) {
	std::filesystem::path withoutFilename{std::filesystem::path(path.parent_path())};
	std::string pathWithoutFilename = withoutFilename.string();
	if (!pathWithoutFilename.empty()) {
		if (pathWithoutFilename.back() != '/') {
			pathWithoutFilename += '/';
		}
	}
	return pathWithoutFilename;
}

static bool FileExists(const std::string& filename) {
	std::ifstream infile(filename.c_str());
	return infile.good();
}

static bool IsDirectory(const std::string& filename) {
	bool isDirectory = false;
	std::filesystem::path path(filename);

	try {
		isDirectory = is_directory(path);
	} catch (...) {
		isDirectory = false;
	}
	return isDirectory;
}
//}  // namespace Rava