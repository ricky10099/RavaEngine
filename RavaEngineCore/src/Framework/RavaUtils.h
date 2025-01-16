#pragma once

#include "ravapch.h"

// namespace Rava {
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

static bool OpenFileDialog(std::string& filePath) {
	//  CREATE FILE OBJECT INSTANCE
	HRESULT f_SysHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(f_SysHr)) {
		return FALSE;
	}

	// CREATE FileOpenDialog OBJECT
	IFileOpenDialog* f_FileSystem;
	f_SysHr =
		CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&f_FileSystem));
	if (FAILED(f_SysHr)) {
		CoUninitialize();
		return FALSE;
	}

	IShellItem* pCurFolder = NULL;
	LPCWSTR deafultPath    = L"Assets/";
	f_SysHr                = SHCreateItemFromParsingName(deafultPath, NULL, IID_PPV_ARGS(&pCurFolder));
	if (SUCCEEDED(f_SysHr)) {
		f_FileSystem->SetDefaultFolder(pCurFolder);
		pCurFolder->Release();
	}

	//  SHOW OPEN FILE DIALOG WINDOW
	f_SysHr = f_FileSystem->Show(NULL);
	if (FAILED(f_SysHr)) {
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	//  RETRIEVE FILE NAME FROM THE SELECTED ITEM
	IShellItem* f_Files;
	f_SysHr = f_FileSystem->GetResult(&f_Files);
	if (FAILED(f_SysHr)) {
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	//  STORE AND CONVERT THE FILE NAME
	PWSTR f_Path;
	f_SysHr = f_Files->GetDisplayName(SIGDN_FILESYSPATH, &f_Path);
	if (FAILED(f_SysHr)) {
		f_Files->Release();
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	//  FORMAT AND STORE THE FILE PATH
	std::wstring path(f_Path);
	std::string c(path.begin(), path.end());
	filePath = c;

	////  FORMAT STRING FOR EXECUTABLE NAME
	// const size_t slash = sFilePath.find_last_of("/\\");
	// sSelectedFile      = sFilePath.substr(slash + 1);

	//  SUCCESS, CLEAN UP
	CoTaskMemFree(f_Path);
	f_Files->Release();
	f_FileSystem->Release();
	CoUninitialize();
	return TRUE;
}
//}  // namespace Rava