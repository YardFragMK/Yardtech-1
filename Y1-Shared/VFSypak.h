#include <string>
#include <unordered_map>
#include <vector>

struct VFSFileEntry {
	std::string archivePath; //dosya yolu
	bool isPacked;
	size_t compressedSize;
	size_t uncompressedSize;
	uint32_t zipFileIndex;
};

class VirtualFileSystem {
private:
	std::unordered_map<std::string, VFSFileEntry> fileRegistry;
	std::vector<std::string> searchPaths;

public:
	void MountDirectory(const std::string& path);
	void MountYPAK(const std::string& ypakPath);
	std::vector<uint8_t> ReadFile(const std::string& virtualPath);

};