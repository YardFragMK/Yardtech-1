#include "pch.h"
#include "SentenceBank.h"
#include <cctype>
#include <cstdlib>
#include <random>
#include <sstream>
#include <vector>

namespace {

    //--------------------------------------------------------------------------
    // Sabitler
    //--------------------------------------------------------------------------
    constexpr size_t kMaxSimultaneousSentences = 8;   // ayni anda en fazla cumle (ses sayisini sinirlar)
    constexpr float kPeriodPause = 0.45f;             // kelime '.' ile bitiyorsa sonraki kelimeden once bekleme (sn)
    constexpr float kCommaPause = 0.20f;              // kelime ',' ile bitiyorsa bekleme (sn)
    constexpr float kMinPitch = 0.25f;                // perde degisikliginin alt sinirii (p25 = 4 kat dusuk)
    constexpr float kMaxPitch = 4.0f;                 // ust siniri
    constexpr float kMaxWordVolume = 2.0f;            // kelime basina ses seviyesi carpaninin ust siniri
    constexpr float kFullVolumeFraction = 0.25f;      // konumlu cumle, yaricapinin %25'ine kadar tam siddette duyulur

    struct Word {
        std::string file;      // "dizin/kelime" (uzantisiz)
        float pitch = 1.0f;    // (p110) gibi degistiriciden: 1.10
        float volume = 1.0f;   // (v80) gibi degistiriciden: 0.80
        float pauseAfter = 0.0f;   // kelimeden sonraki sessizlik (sn)
    };

    struct Sentence {
        std::string name;
        std::vector<Word> words;
    };

    // Calmakta olan bir cumle.
    struct Speech {
        const Sentence* sentence = nullptr;
        size_t nextWord = 0;
        SoundHandle current = 0;       // su an calan kelimenin tutamaci
        float pendingPause = 0.0f;     // calan kelimeden sonra beklenecek sure
        float waitTimer = 0.0f;
        glm::vec3 position{ 0.0f };
        bool everywhere = false;
        float volume = 1.0f;
        float maxDistance = 800.0f;
    };

    IEngineClient* g_engine = nullptr;
    IEngineSound* g_sound = nullptr;
    std::vector<Sentence> g_sentences;
    bool g_sentencesLoaded = false;
    std::vector<Speech> g_speech;

    std::mt19937& RandomEngine() {
        static std::mt19937 engine{ std::random_device{}() };
        return engine;
    }

    float ClampF(float value, float low, float high) {
        return value < low ? low : (value > high ? high : value);
    }

    bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
        }
        return true;
    }

    std::string ReadTextFile(const char* path) {
        const int size = g_engine->GetFileSize(path);
        if (size <= 0) return std::string();

        std::string text(static_cast<size_t>(size), '\0');
        if (!g_engine->ReadFile(path, &text[0], size)) return std::string();
        return text;
    }

    //--------------------------------------------------------------------------
    // sentences.txt ayristirma
    //--------------------------------------------------------------------------
    // "(p105 v80)" gibi bir degistirici metnini perde ve ses carpanina cevirir.
    // p = perde yuzdesi, v = ses yuzdesi. Bilinmeyen harfler yok sayilir.
    void ApplyModifiers(const std::string& modifiers, float& pitch, float& volume) {
        std::string cleaned = modifiers;
        for (char& c : cleaned) if (c == ',') c = ' ';

        std::istringstream stream(cleaned);
        std::string token;
        while (stream >> token) {
            if (token.size() < 2) continue;

            const float value = static_cast<float>(std::atof(token.c_str() + 1));
            const char kind = static_cast<char>(std::tolower(static_cast<unsigned char>(token[0])));
            if (kind == 'p') pitch = ClampF(value / 100.0f, kMinPitch, kMaxPitch);
            else if (kind == 'v') volume = ClampF(value / 100.0f, 0.0f, kMaxWordVolume);
        }
    }

    // Satir bicimi: ADI dizin/kelime1 kelime2(p110) kelime3, ...
    //  - Ilk kelimedeki "dizin/" onekini sonraki oneksiz kelimeler de kullanir.
    //  - Kelimeden sonra '.' ya da ',' kisa bir sessizlik ekler.
    //  - Tek basina "(p105)" sonraki tum kelimelerin varsayilanini degistirir.
    void ParseSentenceLine(const std::string& line) {
        std::istringstream tokens(line);

        Sentence sentence;
        if (!(tokens >> sentence.name)) return;

        std::string directory;
        float defaultPitch = 1.0f;
        float defaultVolume = 1.0f;

        std::string token;
        while (tokens >> token) {
            // Parantez icindeki degistiriciler bosluk icerebilir ("(p105 v80)"): ')' gelene kadar birlestir.
            if (token.find('(') != std::string::npos && token.find(')') == std::string::npos) {
                std::string more;
                while (tokens >> more) {
                    token += " " + more;
                    if (more.find(')') != std::string::npos) break;
                }
            }

            std::string wordText = token;
            std::string modifiers;
            const size_t open = token.find('(');
            if (open != std::string::npos) {
                const size_t close = token.find(')', open);
                wordText = token.substr(0, open);
                modifiers = token.substr(open + 1, close == std::string::npos ? std::string::npos : close - open - 1);
            }

            // Kelimenin sonundaki noktalama bir bekleme ekler. Noktalama degistiricilerden sonra da gelebilir.
            float pause = 0.0f;
            const char last = token.back();
            if (last == '.') pause = kPeriodPause;
            else if (last == ',') pause = kCommaPause;
            while (!wordText.empty() && (wordText.back() == '.' || wordText.back() == ',')) wordText.pop_back();

            if (wordText.empty()) {
                // Yalnizca degistirici: sonraki kelimeler icin varsayilan olur.
                ApplyModifiers(modifiers, defaultPitch, defaultVolume);
                continue;
            }

            Word word;
            word.pitch = defaultPitch;
            word.volume = defaultVolume;
            word.pauseAfter = pause;
            ApplyModifiers(modifiers, word.pitch, word.volume);

            const size_t slash = wordText.rfind('/');
            if (slash != std::string::npos) {
                directory = wordText.substr(0, slash + 1);   // sonraki oneksiz kelimeler bu dizini kullanir
                word.file = wordText;
            }
            else {
                word.file = directory + wordText;
            }
            sentence.words.push_back(word);
        }

        if (!sentence.words.empty()) g_sentences.push_back(sentence);
    }

    void LoadSentencesOnce() {
        if (g_sentencesLoaded || g_engine == nullptr) return;
        g_sentencesLoaded = true;

        const std::string text = ReadTextFile("sound/sentences.txt");
        std::istringstream lines(text);

        std::string line;
        while (std::getline(lines, line)) {
            const size_t comment = line.find("//");
            if (comment != std::string::npos) line.erase(comment);   // yorum satiri
            ParseSentenceLine(line);
        }
    }

    // Tam ad eslesmesi; yoksa grup: sonundaki rakamlar atilinca ad eslesen cumlelerden rastgele biri.
    const Sentence* FindSentence(const std::string& requested) {
        LoadSentencesOnce();

        for (const Sentence& sentence : g_sentences) {
            if (EqualsIgnoreCase(sentence.name, requested)) return &sentence;
        }

        std::vector<const Sentence*> group;
        for (const Sentence& sentence : g_sentences) {
            std::string stem = sentence.name;
            while (!stem.empty() && std::isdigit(static_cast<unsigned char>(stem.back()))) stem.pop_back();
            if (EqualsIgnoreCase(stem, requested)) group.push_back(&sentence);
        }
        if (group.empty()) return nullptr;

        std::uniform_int_distribution<size_t> pick(0, group.size() - 1);
        return group[pick(RandomEngine())];
    }

}  // namespace

void SentenceBank::Init(IEngineClient* engine, IEngineSound* sound) {
    g_engine = engine;
    g_sound = sound;
    g_speech.clear();   // onceki haritanin cumleleri yarida kalmasin
}

void SentenceBank::Shutdown() {
    g_speech.clear();
    g_engine = nullptr;
    g_sound = nullptr;
}

bool SentenceBank::Play(const std::string& nameOrGroup, const glm::vec3& position, bool everywhere,
    float volume, float maxDistance) {

    if (g_sound == nullptr || g_speech.size() >= kMaxSimultaneousSentences) return false;

    const Sentence* sentence = FindSentence(nameOrGroup);
    if (sentence == nullptr) return false;

    Speech speech;
    speech.sentence = sentence;
    speech.position = position;
    speech.everywhere = everywhere;
    speech.volume = volume;
    speech.maxDistance = maxDistance;
    g_speech.push_back(speech);
    return true;
}

void SentenceBank::Update(float deltaTime) {
    if (g_sound == nullptr) return;

    for (size_t i = 0; i < g_speech.size();) {
        Speech& speech = g_speech[i];

        // Calan kelime bitene kadar beklenir; sonra kelimeye ozel sessizlik gelir.
        if (speech.current != 0) {
            if (g_sound->IsPlaying(speech.current)) { ++i; continue; }
            speech.current = 0;
            speech.waitTimer = speech.pendingPause;
        }
        if (speech.waitTimer > 0.0f) {
            speech.waitTimer -= deltaTime;
            ++i;
            continue;
        }

        if (speech.nextWord >= speech.sentence->words.size()) {
            g_speech.erase(g_speech.begin() + static_cast<std::ptrdiff_t>(i));   // cumle bitti
            continue;
        }

        const Word& word = speech.sentence->words[speech.nextWord++];
        const std::string path = "sound/" + word.file + ".wav";
        const float volume = speech.volume * word.volume;

        // Kelime dosyasi yoksa Play 0 doner; bir sonraki karede siradaki kelimeye gecilir.
        speech.current = speech.everywhere
            ? g_sound->Play2DEx(path.c_str(), volume, SOUND_CHANNEL_SFX, false, word.pitch)
            : g_sound->Play3DEx(path.c_str(), speech.position.x, speech.position.y, speech.position.z, volume, false,
                word.pitch, speech.maxDistance * kFullVolumeFraction, speech.maxDistance);
        speech.pendingPause = word.pauseAfter;
        ++i;
    }
}