#include <vpkedit/BSA.h>

#include <cstring>
#include <filesystem>

#include <vpkedit/detail/CRC.h>
#include <vpkedit/detail/Misc.h>
#include <vpkedit/detail/FileStream.h>

using namespace vpkedit;
using namespace vpkedit::detail;

BSA::BSA(const std::string& fullFilePath_, PackFileOptions options_) : PackFileReadOnly(fullFilePath_, options_) {
	this->type = PackFileType::BSA;
}

std::unique_ptr<PackFile> BSA::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* archive = new BSA{ path, options };
	auto packFile = std::unique_ptr<PackFile>(archive);

	FileStream reader{ archive->fullFilePath };
	reader.seekInput(0);
	reader.read(archive->header.fileId);

	if (archive->header.fileId != BSA_ID) {
		return nullptr;
	}

	reader.read(archive->header.version);
	reader.read(archive->header.folderRecordOffset);
	reader.read(archive->header.archiveFlags);
	reader.read(archive->header.folderCount);
	reader.read(archive->header.fileCount);
	reader.read(archive->header.totalFolderNameLength);
	reader.read(archive->header.totalFileNameLength);
	reader.read(archive->header.fileFlags);
	reader.read(archive->header.padding);

	for (int i = 0; i < archive->header.folderCount; i++) {
		FolderRecord record;

		reader.read(record.nameHash);
		reader.read(record.count);

		if (archive->header.version >= 105)
			reader.read(record.padding);

		reader.read(record.offset);

		if (archive->header.version >= 105)
			reader.read(record.padding2);

		archive->folderRecords.push_back(record);
	}

	for (int i = 0; i < archive->header.folderCount; i++) {
		FileRecordBlock block;

		if (archive->header.archiveFlags & ArchiveFlags::IncludeDirectoryNames) {
			std::uint8_t length = reader.read<std::uint8_t>();
			reader.read(block.name, length);
		}

		for (int j = 0; j < archive->folderRecords[i].count; j++) {
			FileRecord record;

			reader.read(record.nameHash);
			reader.read(record.size);
			reader.read(record.offset);

			if ((record.size & (1 << 30)) != 0) {
				// if the archive is compressed, this file isn't. idk why
				record.compressed = !(archive->header.archiveFlags & ArchiveFlags::CompressedArchive);
				record.size ^= 1 << 30;
			}

			block.fileRecords.push_back(record);
			archive->fileRecords.push_back(record);
		}

		archive->fileRecordBlocks.push_back(block);
	}

	if (archive->header.archiveFlags & ArchiveFlags::IncludeFileNames) {
		//for (int i = 0; i < archive->header.fileCount; i++) {
		//	std::string fileName;
		//	reader.read(fileName);
		//	archive->fileNames.push_back(fileName);
		//	archive->fileRecords[i].name = fileName;
		//}

		for (auto& recordBlock : archive->fileRecordBlocks)
		{
			for (auto& record : recordBlock.fileRecords)
			{
				record.folder = recordBlock.name;
				std::string fileName;
				reader.read(fileName);

				record.name = fileName;
			}
		}
	}

	for (auto& fileRecord : archive->fileRecordBlocks) {
		for (auto& record : fileRecord.fileRecords) {
			Entry entry = createNewEntry();

			// TODO: Handle archives that don't have a name list. I dont have any on hand to really test this.
			entry.path = record.folder + "/" + record.name;
			::normalizeSlashes(entry.path);

			entry.length = record.size;
			entry.offset = record.offset;

			auto parentDir = std::filesystem::path(entry.path).parent_path().string();
			if (!archive->entries.contains(parentDir)) {
				archive->entries[parentDir] = {};
			}
			archive->entries[parentDir].push_back(entry);
		}
	}

	return packFile;
}

std::optional<std::vector<std::byte>> BSA::readEntry(const Entry& entry) const {
	// Include this code verbatim - will likely be moved to a utility method soon
	if (entry.unbaked) {
		// Get the stored data
		for (const auto& [unbakedEntryDir, unbakedEntryList] : this->unbakedEntries) {
			for (const Entry& unbakedEntry : unbakedEntryList) {
				if (unbakedEntry.path == entry.path) {
					std::vector<std::byte> unbakedData;
					if (isEntryUnbakedUsingByteBuffer(unbakedEntry)) {
						unbakedData = std::get<std::vector<std::byte>>(getEntryUnbakedData(unbakedEntry));
					}
					else {
						unbakedData = ::readFileData(std::get<std::string>(getEntryUnbakedData(unbakedEntry)));
					}
					return unbakedData;
				}
			}
		}
		return std::nullopt;
	}

	// Use the contents of the entry to access the file data and return it
	// Return std::nullopt if there was an error during any step of this process - not an empty buffer!
	return std::nullopt;
}