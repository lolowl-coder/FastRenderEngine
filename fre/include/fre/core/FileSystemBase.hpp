#pragma once

#include "fre/core/FileSystem.hpp"
#include "fre/macros/Member.hpp"

namespace fre
{
	class FileSystemBase : public IFileSystem
	{
	public:
		FileSystemBase();
		virtual Path getCurrentDir() override;
		virtual Path trim(const Path& path) override;
		virtual Path getExt(const Path& fileName) override;
		virtual bool exists(const Path& path) override;
		virtual Entries listFiles(const Path& dir) override;
		virtual std::vector<uint8_t> readFile(const Path& fileName) override;

		FIELD_NS(Path, AppDir, protected, public, public);
	};
}