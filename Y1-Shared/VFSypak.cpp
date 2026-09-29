#include "VFSypak.h"
#include <miniz.h>
#include <filesystem>
#include <fstream>

void VirtualFileSystem::MountYPAK(const std::string& ypakPath) {
	mz_zip_archive zipArchive;
	memset(&zipArchive, 0, sizeof(zipArchive));

	if (!mz_zip_reader_init_file(&zipArchive, ypakPath.c_str(), 0)) {
		//Error message
		return;
	}

	mz_uint numFiles = mz_zip_reader_get_num_files(&zipArchive);
	for (mz_uint i = 0; i < numFiles; ++i) {
		mz_zip_archive_file_stat fileStat;
		if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) continue;
		if (mz_zip_reader_is_file_a_directory(&zipArchive, i)) continue;

		std::string virtualPath = fileStat.m_filename;

		VFSFileEntry entry;
		entry.archivePath = ypakPath;
		entry.isPacked = true;
		entry.compressedSize = (size_t)fileStat.m_comp_size;
		entry.uncompressedSize = (size_t)fileStat.m_uncomp_size;
		entry.zipFileIndex = i;

		fileRegistry[virtualPath] = entry;
	}
	mz_zip_reader_end(&zipArchive);
	searchPaths.push_back(ypakPath);
}

void VirtualFileSystem::MountDirectory(const std::string& dirPath) {
	if (!std::filesystem::exists(dirPath)) return; //Error message

	for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath)) {
		if (entry.is_regular_file()) {
			std::string relativePath = std::filesystem::relative(entry.path(), dirPath).string();
			std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

			VFSFileEntry fileEntry;
			fileEntry.archivePath = entry.path().string();
			fileEntry.isPacked = false;
			fileEntry.uncompressedSize = std::filesystem::file_size(entry.path());
			fileEntry.zipFileIndex = 0;

			fileRegistry[relativePath] = fileEntry;
		}
	}
}

std::vector<uint8_t> VirtualFileSystem::ReadFile(const std::string& virtualPath) {
	auto it = fileRegistry.find(virtualPath);
	if (it == fileRegistry.end()) {
		//Dosya VFS icinde bulunamadi
		return{};
	}
	const VFSFileEntry& entry = it->second;

	if (!entry.isPacked) {
		std::ifstream file(entry.archivePath, std::ios::binary);
		if (!file) return {};

		std::vector<uint8_t> buffer(entry.uncompressedSize);
		file.read(reinterpret_cast<char*>(buffer.data()), entry.uncompressedSize);
		return buffer;
	}

	mz_zip_archive zipArchive;
	memset(&zipArchive, 0, sizeof(zipArchive));
	if (!mz_zip_reader_init_file(&zipArchive, entry.archivePath.c_str(), 0)) return{};

	std::vector<uint8_t> buffer(entry.uncompressedSize);

	if (!mz_zip_reader_extract_to_mem(&zipArchive, entry.zipFileIndex, buffer.data(), buffer.size(), 0)) {
		//Error ypak ten dosya cikarilamadi
	}

	mz_zip_reader_end(&zipArchive);
	return buffer;

}