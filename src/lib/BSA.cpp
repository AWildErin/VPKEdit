#include <vpkedit/BSA.h>

#include <cstring>
#include <filesystem>

#include <vpkedit/detail/CRC.h>
#include <vpkedit/detail/Misc.h>
#include <vpkedit/detail/FileStream.h>

using namespace vpkedit;
using namespace vpkedit::detail;

BSA::BSA(const std::string& fullFilePath_, PackFileOptions options_) : PackFile(fullFilePath_, options_) {
	this->type = PackFileType::BSA;
}

std::unique_ptr<PackFile> BSA::open(const std::string& path, PackFileOptions options, const Callback& callback)
{
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
	reader.read(archive->header.archiveFlags);
	reader.read(archive->header.folderCount);
	reader.read(archive->header.fileCount);
	reader.read(archive->header.totalFolderNameLength);
	reader.read(archive->header.totalFileNameLength);
	reader.read(archive->header.fileFlags);
	reader.read(archive->header.padding);

	return packFile;
}

Entry& BSA::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_)
{
	return entry;
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

bool BSA::bake(const std::string& outputDir_, const Callback& callback)
{
	return false;
}
