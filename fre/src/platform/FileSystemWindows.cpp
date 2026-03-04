#include "fre/core/Log.hpp"
#include "platform/FileSystemWindows.hpp"

#include <filesystem>

#define NOMINMAX
#include <windows.h>
#include <shlobj.h>

namespace fre
{
	FileSystemWindows::FileSystemWindows()
		: FileSystemBase()
	{
		auto docDir = getDocumentsDir();
		if(!std::filesystem::exists(std::filesystem::path(docDir)))
		{
			std::filesystem::create_directory(docDir);
		}
	}

	void FileSystemWindows::createDirectory(const Path& path)
	{
		std::filesystem::create_directory(path);
	}

	IFileSystem::Path FileSystemWindows::openFileDialog(IWindow& mainWindow, const FileSystemWindows::Path& dir, const char* filter)
	{
		// Common dialog box structure
		OPENFILENAME ofn;
		// Buffer for file name
		char szFile[260];

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = *(HWND*)mainWindow.getNativeHandle().handle;
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = dir.string().c_str();
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Display the Open dialog box
		if (GetOpenFileName(&ofn) == TRUE)
		{
			std::filesystem::current_path(mAppDir);
			return ofn.lpstrFile;
		}

		std::filesystem::current_path(mAppDir);
		return "";
	}

	FileSystemWindows::Path FileSystemWindows::saveFileDialog(IWindow& mainWindow, const FileSystemWindows::Path& dir, const char* filter)
	{
		OPENFILENAME ofn;
		char szFile[260];

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = *(HWND*)mainWindow.getNativeHandle().handle;
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = dir.string().c_str();
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

		if (GetSaveFileName(&ofn) == TRUE)
		{
			std::filesystem::current_path(mAppDir);
			return ofn.lpstrFile;
		}

		std::filesystem::current_path(mAppDir);
		return "";
	}

	std::string WideToUTF8(LPCWSTR wideStr)
	{
		if(!wideStr)
			return "";

		int size_needed = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);
		if(size_needed <= 0)
			return "";

		std::string result(size_needed - 1, 0);  // -1 to exclude null terminator
		WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &result[0], size_needed, NULL, NULL);
    
		return result;
	}

	IFileSystem::Path FileSystemWindows::getDocumentsDir()
	{
		Path result;
		{
			PWSTR path = nullptr;
			HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
    
			if (SUCCEEDED(hr))
			{
				// Free allocated memory
				result = std::filesystem::path(WideToUTF8(path));
				CoTaskMemFree(path);
			}
			else
			{
				LOG_ERROR("Failed to get Documents folder path.");
			}
		}

		Path appName;
		{
			char path[MAX_PATH];
			GetModuleFileNameA(NULL, path, MAX_PATH);
			appName = std::filesystem::path(path).filename().replace_extension();
		}

		return (result / appName).generic_string();
	}
}