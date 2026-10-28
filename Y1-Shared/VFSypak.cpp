#include "VFSypak.h"
#include <miniz.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace {

	// Yolu sanal dosya sistemi anahtarina cevirir: kucuk harf, '/' ayirici, "." parcalari atilir,
	// ".." iceren yol reddedilir (bos string doner). Sondaki egik cizgi, dizin sorgulari icin korunur.
	std::string CleanPath(const std::string& path) {
		std::string joined;
		std::string segment;
		bool rejected = false;

		auto finishSegment = [&]() {
			if (segment.empty() || segment == ".") { segment.clear(); return; }
			if (segment == "..") { rejected = true; segment.clear(); return; }   // ust dizine cikis engellenir

			if (!joined.empty()) joined.push_back('/');
			joined += segment;
			segment.clear();
			};

		for (char c : path) {
			if (c == '/' || c == '\\') finishSegment();
			else segment.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		}
		finishSegment();

		if (rejected) return std::string();

		const bool endsWithSlash = !path.empty() && (path.back() == '/' || path.back() == '\\');
		if (endsWithSlash && !joined.empty()) joined.push_back('/');
		return joined;
	}

}  // namespace

VirtualFileSystem::~VirtualFileSystem() {
	for (auto& pair : m_archives) {
		mz_zip_archive* zip = static_cast<mz_zip_archive*>(pair.second);
		mz_zip_reader_end(zip);
		delete zip;
	}
}

// "nvs1/" gibi bir klasor adini, yollarin basindan atilacaklar listesine ekler.
void VirtualFileSystem::AddRootPrefix(const std::string& directoryPath) {
	std::string prefix = CleanPath(directoryPath);
	if (prefix.empty()) return;
	if (prefix.back() != '/') prefix.push_back('/');

	if (std::find(m_rootPrefixes.begin(), m_rootPrefixes.end(), prefix) == m_rootPrefixes.end()) {
		m_rootPrefixes.push_back(prefix);
	}
}

std::string VirtualFileSystem::Resolve(const std::string& path) const {
	std::string key = CleanPath(path);

	for (const std::string& prefix : m_rootPrefixes) {
		if (key.compare(0, prefix.size(), prefix) == 0) {
			key.erase(0, prefix.size());
			break;
		}
	}
	return key;
}

bool VirtualFileSystem::MountYPAK(const std::string& ypakPath) {
	if (m_archives.count(ypakPath) != 0) return true;   // zaten bagli

	mz_zip_archive* zip = new mz_zip_archive();
	std::memset(zip, 0, sizeof(*zip));

	if (!mz_zip_reader_init_file(zip, ypakPath.c_str(), 0)) {
		delete zip;
		return false;
	}

	// Paketin bulundugu klasorun adi ("nvs1/") yollarin basindan atilabilsin.
	AddRootPrefix(std::filesystem::path(ypakPath).parent_path().generic_string());

	const mz_uint fileCount = mz_zip_reader_get_num_files(zip);
	for (mz_uint i = 0; i < fileCount; ++i) {
		mz_zip_archive_file_stat fileStat;
		if (!mz_zip_reader_file_stat(zip, i, &fileStat)) continue;
		if (mz_zip_reader_is_file_a_directory(zip, i)) continue;

		const std::string key = CleanPath(fileStat.m_filename);
		if (key.empty()) continue;

		VFSFileEntry entry;
		entry.archivePath = ypakPath;
		entry.isPacked = true;
		entry.compressedSize = static_cast<size_t>(fileStat.m_comp_size);
		entry.uncompressedSize = static_cast<size_t>(fileStat.m_uncomp_size);
		entry.zipFileIndex = i;
		m_registry[key] = entry;
	}

	// Arsiv acik tutulur: her okumada merkezi dizini yeniden okumak gerekmez.
	m_archives[ypakPath] = zip;
	return true;
}

bool VirtualFileSystem::MountDirectory(const std::string& dirPath) {
	std::error_code error;
	if (!std::filesystem::exists(dirPath, error)) return false;

	AddRootPrefix(dirPath);

	std::filesystem::recursive_directory_iterator it(dirPath, error);
	const std::filesystem::recursive_directory_iterator end;
	for (; !error && it != end; it.increment(error)) {
		if (!it->is_regular_file(error)) continue;

		const std::string relative = std::filesystem::relative(it->path(), dirPath, error).generic_string();
		const std::string key = CleanPath(relative);
		if (key.empty()) continue;

		VFSFileEntry entry;
		entry.archivePath = it->path().string();
		entry.isPacked = false;
		entry.uncompressedSize = static_cast<size_t>(it->file_size(error));
		m_registry[key] = entry;
	}
	return true;
}

std::vector<uint8_t> VirtualFileSystem::ReadFile(const std::string& virtualPath) const {
	const auto found = m_registry.find(Resolve(virtualPath));
	if (found == m_registry.end()) return {};   // dosya VFS icinde yok

	const VFSFileEntry& entry = found->second;
	std::vector<uint8_t> buffer(entry.uncompressedSize);
	if (buffer.empty()) return buffer;

	if (!entry.isPacked) {
		std::ifstream file(entry.archivePath, std::ios::binary);
		if (!file) return {};

		file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
		// Dosya mount'tan sonra degismis olabilir: eksik okunduysa guvenilmez, bos don.
		if (static_cast<size_t>(file.gcount()) != buffer.size()) return {};
		return buffer;
	}

	const auto archive = m_archives.find(entry.archivePath);
	if (archive == m_archives.end()) return {};

	mz_zip_archive* zip = static_cast<mz_zip_archive*>(archive->second);
	if (!mz_zip_reader_extract_to_mem(zip, entry.zipFileIndex, buffer.data(), buffer.size(), 0)) {
		return {};   // cikarma basarisiz: sifirlarla dolu sahte bir tampon DONDURULMEZ
	}
	return buffer;
}

bool VirtualFileSystem::Exists(const std::string& virtualPath) const {
	return m_registry.find(Resolve(virtualPath)) != m_registry.end();
}

long long VirtualFileSystem::GetFileSize(const std::string& virtualPath) const {
	const auto found = m_registry.find(Resolve(virtualPath));
	if (found == m_registry.end()) return -1;
	return static_cast<long long>(found->second.uncompressedSize);
}

std::vector<std::string> VirtualFileSystem::ListFiles(const std::string& prefix, const std::string& extension) const {
	const std::string cleanPrefix = Resolve(prefix);
	const std::string cleanExtension = CleanPath(extension);

	std::vector<std::string> result;
	for (const auto& pair : m_registry) {
		const std::string& key = pair.first;
		if (key.compare(0, cleanPrefix.size(), cleanPrefix) != 0) continue;

		const bool extensionMatches = cleanExtension.empty()
			|| (key.size() >= cleanExtension.size()
				&& key.compare(key.size() - cleanExtension.size(), cleanExtension.size(), cleanExtension) == 0);
		if (extensionMatches) result.push_back(key);
	}

	std::sort(result.begin(), result.end());
	return result;
}