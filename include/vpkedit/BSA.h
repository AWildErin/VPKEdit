#pragma once

#include <vpkedit/PackFile.h>

namespace vpkedit {

constexpr std::int32_t BSA_ID = 'B' + ('S' << 8) + ('A' << 16) + ('\0' << 24);

class BSA : public PackFileReadOnly {
	enum ArchiveFlags : std::uint32_t {
		IncludeDirectoryNames		= 0x1,
		IncludeFileNames			= 0x2,
		CompressedArchive			= 0x4,
		RetainDirectoryNames		= 0x8,
		RetainFileNames				= 0x10,
		RetainFileNameOffsets		= 0x20,
		X360Archive					= 0x40,
		RetainStringsDuringStartup	= 0x80,
		EmbedFileNames				= 0x100,
		XMemCodec					= 0x200,
	};

	enum FileFlags : std::uint16_t {
		Meshes = 0x1,
		Textures = 0x2,
		Menus = 0x4,
		Sounds = 0x8,
		Voices = 0x10,
		Shaders = 0x20,
		Trees = 0x40,
		Fonts = 0x80,
		Miscellaneous = 0x100,
	};

	struct Header {
		std::uint32_t fileId; // UESP specs this out as char[4], but that's a pain to manage
		std::uint32_t version; // Skyrim LE uses 104, Skyrim SE uses 105
		std::uint32_t folderRecordOffset;
		ArchiveFlags archiveFlags;
		std::uint32_t folderCount;
		std::uint32_t fileCount; 
		std::uint32_t totalFolderNameLength; // Total length of all folder names, including \0 but not including the prefixed length byte
		std::uint32_t totalFileNameLength; // Total length of all folder names, including \0
		FileFlags fileFlags;
		std::uint16_t padding;
	};
	
public:
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const override;

protected:
	BSA(const std::string& fullFilePath_, PackFileOptions options_);

	Header header{};
private:
	VPKEDIT_REGISTER_PACKFILE_EXTENSION(".bsa", &BSA::open);
};

} // namespace vpkedit