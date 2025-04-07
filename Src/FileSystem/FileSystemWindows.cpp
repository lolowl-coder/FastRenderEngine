#include "FileSystem/FileSystem.hpp"
#include "Engine.hpp"

#include <filesystem>

#include <windows.h>
#include <shlobj.h>

namespace fre
{
	FileSystem::FileSystem()
		: mAppDir(getCurrentDir())
	{
		auto docDir = getDocumentsDir();
		if(!std::filesystem::exists(std::filesystem::path(docDir)))
		{
			std::filesystem::create_directory(docDir);
		}
	}

	FileSystem& FileSystem::getInstance()
    {
        // Static local variable to ensure only one instance is created
        static FileSystem instance;
        return instance;
    }

	std::string FileSystem::openFileDialog(const char* dir, const char* filter)
	{
		OPENFILENAME ofn;       // common dialog box structure
		char szFile[260];       // buffer for file name

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = *(HWND*)fre::Engine::getWindowHandle();
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = dir;
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

	std::string FileSystem::saveFileDialog(const char* dir, const char* filter)
	{
		OPENFILENAME ofn;
		char szFile[260];

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = *(HWND*)fre::Engine::getWindowHandle();
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = dir;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

		if (GetSaveFileName(&ofn) == TRUE)
		{
			std::filesystem::current_path(mAppDir);
			return ofn.lpstrFile;
		}

		std::filesystem::current_path(mAppDir);
		return "";
	}

	std::string FileSystem::getCurrentDir()
	{
		return std::filesystem::current_path().generic_string().c_str();
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

	std::string FileSystem::getDocumentsDir()
	{
		std::filesystem::path result;
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

		std::filesystem::path appName;
		{
			char path[MAX_PATH];
			GetModuleFileNameA(NULL, path, MAX_PATH);
			appName = std::filesystem::path(path).filename().replace_extension();
		}

		return (result / appName).generic_string();
	}

	FileSystem::Entries FileSystem::listFiles(const std::string& dir)
	{
		Entries result;
		for(auto const& entry : std::filesystem::directory_iterator{dir}) 
		{
			result.push_back(entry.path().filename().generic_string());
		}

		return result;
	}

	std::string FileSystem::find(const std::string& fileName)
	{
		std::string result = fileName;

		for(const auto& p : mPath)
		{
			const std::string fullFilePath = p + '/' + fileName;
			if(std::filesystem::exists(fullFilePath))
			{
				result = fullFilePath;
				break;
			}
		}

		return result;
	}

	std::string FileSystem::trim(const std::string& path)
	{
		auto tmp = std::filesystem::path(path).generic_string();
		auto sub = getCurrentDir();
		size_t pos = tmp.find(sub);
		if(pos != std::string::npos)
		{
			tmp.erase(pos, sub.length() + 1);
		}

		return tmp;
	}

	std::string FileSystem::getExt(const std::string& fileName)
	{
		return std::filesystem::path(fileName).extension().generic_string();
	}
}