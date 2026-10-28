#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Sanal dosya sistemindeki bir dosya: diskteki serbest dosya ya da bir .ypak (zip) icindeki kayit.
struct VFSFileEntry {
	std::string archivePath;       // serbest dosyanin disk yolu ya da dosyayi tasiyan .ypak'in yolu
	bool isPacked = false;         // true: .ypak icinde
	size_t compressedSize = 0;     // sikistirilmis boyut (yalnizca paketli dosyalar)
	size_t uncompressedSize = 0;   // acilmis boyut
	uint32_t zipFileIndex = 0;     // arsiv icindeki sira (yalnizca paketli dosyalar)
};

// Oyunun tum varliklari (doku, model, ses, shader, harita) bu sinif uzerinden okunur.
//
// Kurallar:
//  - Yollar sanal yollardir ("sprites/glow.tga"); buyuk/kucuk harf ve '\' / '/' farki yoktur.
//  - Bagli klasorun adi basa yazilirsa ("nvs1/sprites/glow.tga") otomatik atilir.
//  - Sonradan baglanan kaynak, ayni yolu tasiyan oncekinin yerini alir. Engine once paketleri,
//    en son klasoru baglar: serbest dosyalar paketin icindekileri ezer (gelistirme kolayligi).
class VirtualFileSystem {
public:
	VirtualFileSystem() = default;
	~VirtualFileSystem();
	VirtualFileSystem(const VirtualFileSystem&) = delete;
	VirtualFileSystem& operator=(const VirtualFileSystem&) = delete;

	// Basari durumunu dondurur (eskiden void'di; sonucu yok sayan cagrilar etkilenmez).
	bool MountDirectory(const std::string& path);
	bool MountYPAK(const std::string& ypakPath);

	// Dosyanin icerigi; yoksa ya da okunamazsa bos vektor.
	std::vector<uint8_t> ReadFile(const std::string& virtualPath) const;

	bool Exists(const std::string& virtualPath) const;
	long long GetFileSize(const std::string& virtualPath) const;   // yoksa -1

	// Verilen dizin onekiyle baslayan (ve istenirse uzantisi eslesen) dosyalarin sirali listesi.
	std::vector<std::string> ListFiles(const std::string& prefix, const std::string& extension = "") const;

private:
	std::string Resolve(const std::string& path) const;
	void AddRootPrefix(const std::string& directoryPath);

	std::unordered_map<std::string, VFSFileEntry> m_registry;   // temiz yol -> kayit
	std::unordered_map<std::string, void*> m_archives;          // ypak yolu -> acik arsiv (mz_zip_archive*)
	std::vector<std::string> m_rootPrefixes;                    // basa yazilirsa atilan klasor adlari ("nvs1/")
};