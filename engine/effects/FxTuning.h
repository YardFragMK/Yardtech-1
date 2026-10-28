#pragma once
#include "FxCommon.h"

// Silah gorsel efektlerinin tum sayisal ayarlari, efekt bazinda gruplanmis.
//
// Birimler : dunya birimi, saniye. Renk kanallari 0..1 araliginda (r, g, b, a).
// "Span"       { en az, en cok }  : o aralikta her seferinde rastgele secilen deger.
// "ColorRamp"  { baslangic, bitis }: parcacigin omrunun basindaki ve sonundaki renk (rgba);
//                                    arada dogrusal gecis yapilir. Alfa 0'a inerek parcacik kaybolur.
// Efektlerin NASIL kuruldugu Fx*.cpp dosyalarinda, NE KADAR oldugu burada yazilidir.
namespace FxTuning {

    //--------------------------------------------------------------------------
    namespace Explosion {
        // --- Ana ates topu (boyutlar patlama yaricapinin katidir) ---
        constexpr float kMainFireballLife = 0.60f;           // omru (sn)
        constexpr float kMainFireballStartScale = 0.9f;      // dogdugundaki boyut = yaricap x 0.9
        constexpr float kMainFireballEndScale = 1.5f;        // omrun sonunda yaricap x 1.5'e buyur
        inline const Fx::ColorRamp kMainFireballColor = {
            { 1.00f, 0.85f, 0.50f, 1.0f },                   // baslangic: parlak sari-beyaz
            { 1.00f, 0.30f, 0.05f, 0.0f } };                 // bitis: turuncu-kirmizi, saydam

        // --- Yan ates parcalari (ana topun etrafinda savrulan kucuk toplar) ---
        constexpr int kSmallFireballCount = 4;               // kac adet
        constexpr float kSmallFireballOffsetScale = 0.3f;    // merkezden baslangic uzakligi = yaricap x 0.3
        constexpr float kSmallFireballSpeedScale = 0.4f;     // disari hizi (birim/sn) = yaricap x 0.4
        constexpr Fx::Span kSmallFireballLife = { 0.40f, 0.65f };   // omru (sn)
        constexpr float kSmallFireballStartScale = 0.4f;     // baslangic boyutu = yaricap x 0.4
        constexpr float kSmallFireballEndScale = 0.8f;       // bitis boyutu = yaricap x 0.8
        inline const Fx::ColorRamp kSmallFireballColor = {
            { 1.0f, 0.7f, 0.3f, 0.9f },
            { 0.9f, 0.2f, 0.05f, 0.0f } };

        // --- Merkez parlamasi ve sok dalgasi halkasi ---
        constexpr float kCoreGlowLife = 0.28f;               // parlamanin omru (sn): cok kisa bir flas
        constexpr float kCoreGlowStartScale = 1.4f;          // baslangic boyutu = yaricap x 1.4
        constexpr float kCoreGlowEndScale = 0.4f;            // daralarak yaricap x 0.4'e iner
        inline const Fx::ColorRamp kCoreGlowColor = {
            { 1.0f, 0.9f, 0.6f, 0.9f },
            { 1.0f, 0.5f, 0.2f, 0.0f } };

        constexpr float kShockwaveLife = 0.38f;              // halkanin omru (sn)
        constexpr float kShockwaveStartScale = 0.4f;         // halka kucuk baslar...
        constexpr float kShockwaveEndScale = 2.2f;           // ...ve yaricap x 2.2'ye kadar yayilir
        inline const Fx::ColorRamp kShockwaveColor = {
            { 1.0f, 0.8f, 0.5f, 0.7f },
            { 1.0f, 0.5f, 0.2f, 0.0f } };

        // --- Kivilcimlar: sayi patlamanin yaricapiyla artar ---
        constexpr float kMinSparks = 8.0f;                   // en az kivilcim sayisi
        constexpr float kMaxSparks = 40.0f;                  // en cok kivilcim sayisi
        constexpr float kRadiusPerExtraSpark = 20.0f;        // yaricap her 20 birim artinca +1 kivilcim
        constexpr float kSparkReferenceRadius = 200.0f;      // hizlar bu yaricap icin tanimli; buyuk patlamada orantili artar
        constexpr Fx::Span kSparkSpeed = { 200.0f, 600.0f }; // baslangic hizi (birim/sn)
        constexpr Fx::Span kSparkLife = { 0.5f, 1.1f };      // omru (sn)
        constexpr float kSparkStartSize = 9.0f;              // baslangic boyutu (birim)
        constexpr float kSparkEndSize = 2.0f;                // omrun sonunda ne kadar kuculur
        constexpr float kSparkGravity = 500.0f;              // asagi ivme (birim/sn^2): kivilcimlar yay cizerek duser
        constexpr float kSparkDrag = 0.8f;                   // hava surtunmesi (1/sn): hizlari zamanla azalir
        inline const Fx::ColorRamp kSparkColor = {
            { 1.0f, 0.8f, 0.4f, 1.0f },
            { 1.0f, 0.3f, 0.05f, 0.0f } };

        // --- Patlamadan sonra yukselen duman ---
        constexpr float kMinSmoke = 6.0f;                    // en az duman parcasi
        constexpr float kMaxSmoke = 16.0f;                   // en cok duman parcasi
        constexpr float kRadiusPerExtraSmoke = 40.0f;        // yaricap her 40 birim artinca +1 parca
        constexpr float kSmokeOffsetScale = 0.35f;           // merkezden dagilma = yaricap x 0.35
        constexpr Fx::Span kSmokeDrift = { 30.0f, 120.0f };  // yanlara savrulma hizi (birim/sn)
        constexpr Fx::Span kSmokeRise = { 30.0f, 90.0f };    // yukari cikma hizi (birim/sn)
        constexpr Fx::Span kSmokeLife = { 1.4f, 2.4f };      // omru (sn)
        constexpr float kSmokeStartScale = 0.35f;            // baslangic boyutu = yaricap x 0.35
        constexpr float kSmokeEndScale = 0.9f;               // omrun sonunda yaricap x 0.9'a yayilir
        constexpr float kSmokeDrag = 1.2f;                   // surtunme (1/sn): duman cabuk yavaslar
        constexpr float kSmokeSpin = 0.6f;                   // en fazla donus hizi (radyan/sn)
        constexpr float kSmokeFadeIn = 0.1f;                 // omrun ilk %10'unda yavasca belirir
        constexpr float kSmokeNearFade = 80.0f;              // kameraya 80 birimden yakinsa solar (ekrani kaplamasin)
        inline const Fx::ColorRamp kSmokeColor = {
            { 0.25f, 0.23f, 0.22f, 0.55f },                  // baslangic: koyu gri, yari saydam
            { 0.12f, 0.12f, 0.12f, 0.0f } };
    }

    //--------------------------------------------------------------------------
    namespace BfgExplosion {
        // --- Yesil ates topları (ilki merkezde, digerleri etrafinda) ---
        constexpr int kFireballCount = 5;                    // toplam top sayisi
        constexpr float kCenterFireballScale = 1.0f;         // merkezdeki topun boyut carpani
        constexpr float kSideFireballScale = 0.55f;          // yan toplarin boyut carpani
        constexpr float kSideFireballOffsetScale = 0.35f;    // yan toplarin merkezden uzakligi = yaricap x 0.35
        constexpr Fx::Span kFireballLife = { 0.9f, 1.2f };   // omru (sn)
        constexpr float kFireballStartScale = 0.9f;          // baslangic boyutu = yaricap x 0.9
        constexpr float kFireballEndScale = 1.5f;            // bitis boyutu = yaricap x 1.5
        inline const Fx::ColorRamp kFireballColor = {
            { 0.8f, 1.0f, 0.85f, 1.0f },                     // baslangic: beyaza yakin acik yesil
            { 0.1f, 0.9f, 0.3f, 0.0f } };                    // bitis: koyu yesil, saydam

        // --- Merkez parlamasi ve iki sok dalgasi ---
        constexpr float kGlowLife = 0.55f;                   // parlamanin omru (sn)
        constexpr float kGlowStartScale = 2.2f;              // baslangic boyutu = yaricap x 2.2 (dev flas)
        constexpr float kGlowEndScale = 0.6f;                // daralarak yaricap x 0.6'ya iner
        inline const Fx::ColorRamp kGlowColor = {
            { 0.7f, 1.0f, 0.8f, 1.0f },
            { 0.2f, 1.0f, 0.4f, 0.0f } };

        constexpr float kInnerRingLife = 0.6f;               // ic halkanin omru (sn)
        constexpr float kInnerRingStartScale = 0.3f;         // yaricap x 0.3'ten baslar
        constexpr float kInnerRingEndScale = 2.6f;           // yaricap x 2.6'ya yayilir
        inline const Fx::ColorRamp kInnerRingColor = {
            { 0.4f, 1.0f, 0.6f, 0.8f },
            { 0.2f, 1.0f, 0.4f, 0.0f } };

        constexpr float kOuterRingLife = 0.9f;               // dis halka daha yavas ve daha buyuk (sn)
        constexpr float kOuterRingStartScale = 0.2f;
        constexpr float kOuterRingEndScale = 3.2f;
        inline const Fx::ColorRamp kOuterRingColor = {
            { 0.5f, 1.0f, 0.7f, 0.5f },
            { 0.2f, 1.0f, 0.4f, 0.0f } };

        // --- Kivilcimlar ---
        constexpr int kSparkCount = 60;                      // sabit sayi (BFG patlamasi her zaman buyuk)
        constexpr Fx::Span kSparkSpeed = { 300.0f, 1100.0f };// baslangic hizi (birim/sn)
        constexpr Fx::Span kSparkLife = { 0.7f, 1.5f };      // omru (sn)
        constexpr float kSparkStartSize = 14.0f;             // baslangic boyutu (birim)
        constexpr float kSparkEndSize = 2.0f;                // bitis boyutu
        constexpr float kSparkGravity = 300.0f;              // asagi ivme (birim/sn^2): normal patlamadan hafif
        constexpr float kSparkDrag = 0.7f;                   // surtunme (1/sn)
        inline const Fx::ColorRamp kSparkColor = {
            { 0.6f, 1.0f, 0.7f, 1.0f },
            { 0.1f, 0.8f, 0.3f, 0.0f } };

        // --- Yesilimsi duman ---
        constexpr int kSmokeCount = 14;                      // parca sayisi
        constexpr float kSmokeOffsetScale = 0.4f;            // merkezden dagilma = yaricap x 0.4
        constexpr Fx::Span kSmokeDrift = { 40.0f, 160.0f };  // yanlara savrulma hizi (birim/sn)
        constexpr Fx::Span kSmokeRise = { 40.0f, 120.0f };   // yukari cikma hizi (birim/sn)
        constexpr Fx::Span kSmokeLife = { 2.0f, 3.5f };      // omru (sn): normal patlamadan uzun
        constexpr float kSmokeStartScale = 0.4f;             // baslangic boyutu = yaricap x 0.4
        constexpr float kSmokeEndScale = 1.0f;               // bitis boyutu = yaricap x 1.0
        constexpr float kSmokeDrag = 1.0f;                   // surtunme (1/sn)
        constexpr float kSmokeSpin = 0.5f;                   // en fazla donus hizi (radyan/sn)
        constexpr float kSmokeFadeIn = 0.1f;                 // omrun ilk %10'unda belirir
        constexpr float kSmokeNearFade = 120.0f;             // kameraya 120 birimden yakinsa solar
        inline const Fx::ColorRamp kSmokeColor = {
            { 0.1f, 0.28f, 0.14f, 0.5f },
            { 0.05f, 0.12f, 0.06f, 0.0f } };
    }

    //--------------------------------------------------------------------------
    namespace Fog {
        // Bulut, surekli yenilenen duman puf'larindan olusan bir hacimdir: ortaya ciktiginda
        // bir patlama ile dolar, omru boyunca yeni puf'lar eklenir.
        //
        // YOGUNLUK AYARI: kBurstPuffCount, kSteadyPuffInterval (kucuk = daha yogun),
        // kPuffColor'un alfasi ve puf boyutlari. Bulutun icinde FPS dusuyorsa once
        // kSteadyPuffInterval'i buyut.

        // --- Ilk patlama ---
        constexpr float kBurstWindow = 0.3f;                 // bulutun ilk 0.3 sn'sinde patlama uretilir
        constexpr int kBurstPuffCount = 140;                 // patlamadaki puf sayisi
        constexpr float kBurstSpreadScale = 0.45f;           // puf'lar merkezden bulut yaricapinin %45'ine kadar dagilir
        constexpr Fx::Span kBurstSpeed = { 60.0f, 180.0f };  // disari dogru hizlari (birim/sn)
        constexpr Fx::Span kBurstPuffLife = { 4.0f, 6.0f };  // omru (sn)

        // --- Omur boyunca surekli uretim ---
        constexpr float kSteadyPuffInterval = 1.0f / 80.0f;  // saniyede 80 yeni puf
        constexpr float kSteadySpreadScale = 0.85f;          // puf'lar yaricapin %85'i icinde dogar
        constexpr Fx::Span kSteadyDrift = { 2.0f, 14.0f };   // yavas surukleme hizi (birim/sn)
        constexpr Fx::Span kSteadyPuffLife = { 4.5f, 7.0f }; // omru (sn)
        constexpr float kStopRefillBeforeEnd = 5.5f;         // bulutun son 5.5 sn'sinde yeni puf eklenmez: bulut seyrelerek biter

        // --- Her puf'in gorunumu ---
        constexpr float kPuffStartScale = 0.38f;             // baslangic boyutu = bulut yaricapi x 0.38
        constexpr float kPuffEndScale = 0.60f;               // yavasca bulut yaricapi x 0.60'a buyur
        constexpr float kPuffSpin = 0.3f;                    // en fazla donus hizi (radyan/sn)
        constexpr float kPuffDrag = 1.5f;                    // surtunme (1/sn): puf'lar kisa surede durur
        constexpr float kPuffFadeIn = 0.2f;                  // omrun ilk %20'sinde yavasca belirir
        constexpr float kPuffNearFade = 90.0f;               // kameraya 90 birimden yakinsa solar
        inline const Fx::ColorRamp kPuffColor = {
            { 0.76f, 0.78f, 0.82f, 0.34f },                  // acik gri-mavi; 0.34 alfa: yogunluk icin ana deger
            { 0.70f, 0.72f, 0.76f, 0.0f } };
    }

    //--------------------------------------------------------------------------
    namespace Pool {
        // Lav ve asit havuzu ayni kodu kullanir, sayilari farkli olur.
        struct Style {
            glm::vec3 hotColor;           // parlak renk (kabarcik, damla, koz)
            glm::vec3 darkColor;          // koyu renk (carpma dumani)
            float bubbleIntervalScale;    // kabarcik araligi (sn) = bu sayi / havuz yaricapi (buyuk havuz = daha sik)
            float steamIntervalScale;     // buhar araligi icin ayni mantik
            float emberIntervalScale;     // koz/fisirti araligi icin ayni mantik
            Fx::ColorRamp steamColor;     // buharin rengi
            Fx::Span emberLife;           // kozun omru (sn)
            float emberSize;              // kozun baslangic boyutu (birim)
            Fx::Span emberSpeed;          // kozun yuzeyden firlama hizi (birim/sn)
            float emberLift;              // yuzey normali yonunde ivme: pozitif = yukselir, negatif = yere cekilir

            // --- Yuzey gorunumu (maske ya da shader yokken kullanilan cizim yolu) ---
            float flowSpeed;              // sivinin akisi: saniyede kac texture tekrari kayar
            glm::vec3 centerTint;         // havuz merkezinin renk carpani
            glm::vec3 edgeTint;           // kenarlarin renk carpani (lavda sogumus kabuk gibi koyu)
            float pulseAmount;            // parlakligin nabiz genligi (0.2 = +-%20)
        };

        inline const Style kLava = {
            { 1.00f, 0.50f, 0.10f },                         // hotColor: turuncu
            { 0.45f, 0.12f, 0.05f },                         // darkColor: koyu kirmizi
            8.0f, 25.0f, 18.0f,                              // kabarcik / buhar / koz araliklari
            { { 0.30f, 0.12f, 0.08f, 0.28f }, { 0.15f, 0.10f, 0.10f, 0.0f } },   // buhar: koyu kirmizi-gri
            { 1.0f, 1.8f },                                  // koz omru: uzun (yukselir)
            6.0f,                                            // koz boyutu
            { 40.0f, 120.0f },                               // koz hizi
            20.0f,                                           // koz yukari ivme
            0.03f,                                           // flowSpeed: lav yavas akar
            { 1.0f, 1.0f, 1.0f },                            // centerTint: dokunun kendi rengi
            { 0.35f, 0.15f, 0.10f },                         // edgeTint: kenarda koyu kabuk
            0.20f                                            // pulseAmount: +-%20 nabiz
        };

        inline const Style kAcid = {
            { 0.45f, 1.00f, 0.20f },                         // hotColor: parlak yesil
            { 0.12f, 0.35f, 0.06f },                         // darkColor: koyu yesil
            6.0f, 25.0f, 14.0f,
            { { 0.55f, 0.85f, 0.30f, 0.22f }, { 0.50f, 0.80f, 0.30f, 0.0f } },   // buhar: soluk yesil
            { 0.3f, 0.6f },                                  // fisirti omru: kisa
            5.0f,
            { 60.0f, 140.0f },
            -200.0f,                                         // negatif: fisirtilar yere geri duser
            0.06f,                                           // flowSpeed: asit lavdan hizli akar
            { 1.0f, 1.0f, 1.0f },
            { 0.75f, 1.0f, 0.55f },                          // edgeTint: kenarda acik yesil kopuk
            0.12f
        };

        constexpr float kSurfaceLift = 3.0f;                 // parcaciklar yuzeyin 3 birim ustunden baslar
        constexpr float kEmissionStopBeforeEnd = 1.0f;       // havuzun son 1 sn'sinde yeni parcacik uretilmez (soguma)

        // --- Carpma ani: sivi sicramasi ve duman (yalnizca havuz ilk ortaya ciktiginda) ---
        constexpr float kImpactWindow = 0.3f;                // havuzun ilk 0.3 sn'sinde uretilir
        constexpr int kDropletCount = 22;                    // sicrayan damla sayisi
        constexpr Fx::Span kDropletNormalWeight = { 0.7f, 1.6f };  // damlalarin yuzeyden yukari dogru agirligi
        constexpr float kDropletRandomWeight = 0.7f;         // rastgele yanlara savrulma agirligi (yuksekse daha yayvan)
        constexpr Fx::Span kDropletSpeed = { 200.0f, 460.0f };     // firlama hizi (birim/sn)
        constexpr Fx::Span kDropletLife = { 0.5f, 0.95f };   // omru (sn)
        constexpr float kDropletStartSize = 10.0f;           // baslangic boyutu (birim)
        constexpr float kDropletEndSize = 4.0f;              // bitis boyutu
        constexpr float kDropletGravity = 900.0f;            // asagi ivme (birim/sn^2): damlalar hizla geri duser

        constexpr int kImpactSmokeCount = 5;                 // carpma dumani parca sayisi
        constexpr float kImpactSmokeSpread = 0.5f;           // havuz yaricapinin %50'si icinde dogar
        constexpr Fx::Span kImpactSmokeRise = { 40.0f, 90.0f };    // yukselme hizi (birim/sn)
        constexpr Fx::Span kImpactSmokeLife = { 1.2f, 1.9f };      // omru (sn)
        constexpr float kImpactSmokeStartScale = 0.3f;       // baslangic boyutu = havuz yaricapi x 0.3
        constexpr float kImpactSmokeEndScale = 0.7f;         // bitis boyutu = havuz yaricapi x 0.7
        constexpr float kImpactSmokeAlpha = 0.5f;            // baslangic opakligi
        constexpr float kImpactSmokeDrag = 1.3f;             // surtunme (1/sn)
        constexpr float kImpactSmokeFadeIn = 0.1f;           // omrun ilk %10'unda belirir
        constexpr float kImpactSmokeNearFade = 70.0f;        // kameraya 70 birimden yakinsa solar

        // --- Havuz yasarken surekli uretilenler ---
        constexpr float kBubbleSpread = 0.7f;                // kabarciklar yaricapin %70'i icinde dogar
        constexpr Fx::Span kBubbleRise = { 4.0f, 14.0f };    // yukselme hizi (birim/sn): yavas
        constexpr Fx::Span kBubbleLife = { 0.5f, 0.95f };    // omru (sn)
        constexpr float kBubbleStartSize = 5.0f;             // baslangic boyutu (birim)
        constexpr Fx::Span kBubbleEndSize = { 18.0f, 30.0f };// patlamadan once sisip ulastigi boyut
        constexpr float kBubbleAlpha = 0.8f;                 // baslangic opakligi

        constexpr float kSteamSpread = 0.8f;                 // buhar yaricapin %80'i icinde dogar
        constexpr Fx::Span kSteamRise = { 35.0f, 75.0f };    // yukselme hizi (birim/sn)
        constexpr float kSteamSideways = 12.0f;              // yanlara rastgele savrulma hizi (birim/sn)
        constexpr Fx::Span kSteamLife = { 2.2f, 3.2f };      // omru (sn)
        constexpr float kSteamStartSize = 28.0f;             // baslangic boyutu (birim)
        constexpr float kSteamEndSize = 85.0f;               // buhar yukselirken bu boyuta kadar yayilir
        constexpr float kSteamSpin = 0.4f;                   // en fazla donus hizi (radyan/sn)
        constexpr float kSteamFadeIn = 0.2f;                 // omrun ilk %20'sinde belirir
        constexpr float kSteamNearFade = 60.0f;              // kameraya 60 birimden yakinsa solar

        constexpr float kEmberSpread = 0.75f;                // koz yaricapin %75'i icinde dogar
        constexpr float kEmberSideways = 20.0f;              // yanlara rastgele savrulma hizi (birim/sn)
        constexpr float kEmberEndSize = 1.5f;                // omrun sonunda boyutu (birim)

        // --- Yere cizilen yuzey ---
        constexpr float kGrowTime = 0.45f;                   // havuzun tam boyuta acilma suresi (sn)
        constexpr float kFadeOutTime = 1.6f;                 // omrun son 1.6 sn'sinde havuz yavasca soner
        constexpr float kMinVisibleGrowth = 0.01f;           // bundan kucuk acilma orani cizilmez
        constexpr float kMaskAverageRadius = 0.78f;          // maske dokusundaki sekil, karenin ortalama %78'ini kaplar
        constexpr float kDrawLift = 1.5f;                    // yuzey, zeminin 1.5 birim ustune cizilir
        constexpr float kPolygonOffset = -2.0f;              // zeminle cakisip titremesin diye derinlik kaydirmasi

        // --- Dokusal cizim yolu (poolmask.tga ya da shader yokken) ---
        constexpr int kRimSegments = 40;                     // kenarin kac kose noktasindan olustugu (cok = daha yuvarlak)
        constexpr float kRimWobbleLow = 0.12f;               // kenar sekli: 3 loblu buyuk dalga genligi (yaricapin %12'si)
        constexpr float kRimWobbleMid = 0.06f;               // 7 loblu orta dalga genligi
        constexpr float kRimWobbleHigh = 0.03f;              // 13 loblu ince dalga genligi
        constexpr float kEdgeAlpha = 0.0f;                   // kenardaki opaklik: 0 = yumusak solarak biter
        constexpr float kLiquidTileSize = 128.0f;            // sivi dokusunun bir tekrari kac dunya birimini kaplar
        constexpr float kSecondLayerScale = 1.7f;            // ikinci katman dokuyu 1.7 kat sik tekrarlar
        constexpr float kSecondLayerAlpha = 0.4f;            // ikinci katmanin opakligi (derinlik/akis hissi verir)
        constexpr float kPulseSpeed = 2.0f;                  // parlaklik nabzinin hizi (radyan/sn)
    }

    //--------------------------------------------------------------------------
    namespace Trail {
        namespace Rocket {
            constexpr float kSmokeInterval = 0.012f;         // duman izi araligi (sn): sik = surekli iz
            constexpr float kSmokeBackOffset = 10.0f;        // duman roketin 10 birim arkasinda dogar
            constexpr float kSmokeJitter = 3.0f;             // dogus noktasina rastgele sapma (birim)
            constexpr float kSmokeBackSpeed = 20.0f;         // roketin ters yonunde suruklenme hizi (birim/sn)
            constexpr float kSmokeScatterSpeed = 18.0f;      // rastgele yanlara savrulma hizi (birim/sn)
            constexpr Fx::Span kSmokeLife = { 0.9f, 1.3f };  // omru (sn)
            constexpr float kSmokeStartSize = 12.0f;         // baslangic boyutu (birim)
            constexpr float kSmokeEndSize = 34.0f;           // yayilarak ulastigi boyut
            constexpr float kSmokeFadeIn = 0.1f;             // omrun ilk %10'unda belirir
            constexpr float kSmokeNearFade = 50.0f;          // kameraya 50 birimden yakinsa solar
            inline const Fx::ColorRamp kSmokeColor = {
                { 0.55f, 0.55f, 0.55f, 0.35f },              // gri, hafif seffaf
                { 0.20f, 0.20f, 0.20f, 0.0f } };

            constexpr float kFlameInterval = 0.02f;          // egzoz alevi araligi (sn)
            constexpr float kFlameBackOffset = 16.0f;        // alev roketin 16 birim arkasinda
            constexpr float kFlameLife = 0.1f;               // alev cok kisa omurlu (sn)
            constexpr float kFlameStartSize = 26.0f;         // baslangic boyutu (birim)
            constexpr float kFlameEndSize = 8.0f;            // bitis boyutu
            inline const Fx::ColorRamp kFlameColor = {
                { 1.0f, 0.7f, 0.3f, 0.9f },
                { 1.0f, 0.2f, 0.0f, 0.0f } };
        }

        namespace EnergyBall {
            // Stilar (bot) topu ve staplegun topu kisa bir kivilcim izi birakir.
            struct Style {
                float interval;        // kivilcim araligi (sn)
                float startSize;       // kivilcimin baslangic boyutu (birim)
                glm::vec3 color;       // kivilcim rengi
            };

            inline const Style kBot = { 0.015f, 10.0f, { 1.0f, 0.5f, 0.2f } };     // turuncu
            inline const Style kStaple = { 0.02f, 7.0f, { 0.5f, 0.9f, 1.0f } };    // acik mavi

            constexpr float kJitter = 4.0f;                  // dogus noktasina rastgele sapma (birim)
            constexpr float kScatterSpeed = 40.0f;           // rastgele savrulma hizi (birim/sn)
            constexpr Fx::Span kLife = { 0.25f, 0.5f };      // omru (sn)
            constexpr float kEndSize = 1.0f;                 // omrun sonunda boyutu (birim)
            constexpr float kFadedColorScale = 0.4f;         // bitisteki renk, baslangicin %40'i kadar parlak
        }

        namespace LastBall {
            constexpr float kSparkInterval = 0.008f;         // kivilcim araligi (sn): cok sik, yogun bir iz
            constexpr float kSparkJitter = 30.0f;            // topun etrafinda dogma yaricapi (birim)
            constexpr float kSparkScatterSpeed = 80.0f;      // savrulma hizi (birim/sn)
            constexpr Fx::Span kSparkLife = { 0.5f, 0.9f };  // omru (sn)
            constexpr float kSparkStartSize = 14.0f;         // baslangic boyutu (birim)
            constexpr float kSparkEndSize = 2.0f;            // bitis boyutu
            inline const Fx::ColorRamp kSparkColor = {
                { 0.6f, 1.0f, 0.7f, 1.0f },
                { 0.1f, 0.8f, 0.3f, 0.0f } };

            constexpr float kGlowInterval = 0.03f;           // yesil hale parlamasi araligi (sn)
            constexpr float kGlowLife = 0.35f;               // parlamanin omru (sn)
            constexpr float kGlowStartSize = 120.0f;         // baslangic boyutu (birim)
            constexpr float kGlowEndSize = 30.0f;            // daralarak ulastigi boyut
            inline const Fx::ColorRamp kGlowColor = {
                { 0.4f, 1.0f, 0.6f, 0.35f },
                { 0.1f, 0.8f, 0.3f, 0.0f } };
        }

        namespace Bomb {
            // Lav ve asit bombasi, havada giderken yercekimiyle dusen parlak damlalar birakir.
            constexpr float kInterval = 0.02f;               // damla araligi (sn)
            constexpr float kJitter = 4.0f;                  // dogus noktasina rastgele sapma (birim)
            constexpr float kScatterSpeed = 30.0f;           // savrulma hizi (birim/sn)
            constexpr Fx::Span kLife = { 0.4f, 0.8f };       // omru (sn)
            constexpr float kStartSize = 8.0f;               // baslangic boyutu (birim)
            constexpr float kEndSize = 1.5f;                 // bitis boyutu
            constexpr float kGravity = 250.0f;               // asagi ivme (birim/sn^2)
            constexpr float kFadedColorScale = 0.4f;         // bitisteki renk, baslangicin %40'i kadar parlak
            inline const glm::vec3 kLavaColor = { 1.0f, 0.5f, 0.1f };
            inline const glm::vec3 kAcidColor = { 0.45f, 1.0f, 0.2f };
        }

        namespace Sawblade {
            constexpr float kInterval = 0.03f;               // kivilcim araligi (sn)
            constexpr float kJitter = 14.0f;                 // testerenin kenarina yakin dogma yaricapi (birim)
            constexpr float kScatterSpeed = 120.0f;          // savrulma hizi (birim/sn)
            constexpr Fx::Span kLife = { 0.15f, 0.35f };     // omru (sn): kisa metal kivilcimi
            constexpr float kStartSize = 6.0f;               // baslangic boyutu (birim)
            constexpr float kEndSize = 1.0f;                 // bitis boyutu
            constexpr float kGravity = 300.0f;               // asagi ivme (birim/sn^2)
            inline const Fx::ColorRamp kColor = {
                { 1.0f, 0.9f, 0.6f, 1.0f },
                { 1.0f, 0.5f, 0.2f, 0.0f } };
        }
    }

    //--------------------------------------------------------------------------
    namespace RayBeam {
        // --- Namlu ve isabet noktasi parlamalari ---
        constexpr float kMuzzleFlashLife = 0.2f;             // namlu parlamasinin omru (sn)
        constexpr float kMuzzleFlashStartSize = 90.0f;       // baslangic boyutu (birim)
        constexpr float kMuzzleFlashEndSize = 12.0f;         // daralarak ulastigi boyut
        inline const Fx::ColorRamp kMuzzleFlashColor = {
            { 0.7f, 0.95f, 1.0f, 1.0f },
            { 0.2f, 0.7f, 1.0f, 0.0f } };

        constexpr float kImpactFlashLife = 0.35f;            // isabet parlamasinin omru (sn)
        constexpr float kImpactFlashStartSize = 130.0f;
        constexpr float kImpactFlashEndSize = 20.0f;
        inline const Fx::ColorRamp kImpactFlashColor = {
            { 0.8f, 1.0f, 1.0f, 1.0f },
            { 0.2f, 0.7f, 1.0f, 0.0f } };

        constexpr float kImpactRingLife = 0.35f;             // isabet halkasinin omru (sn)
        constexpr float kImpactRingStartSize = 20.0f;        // kucuk baslar...
        constexpr float kImpactRingEndSize = 150.0f;         // ...ve bu boyuta yayilir
        inline const Fx::ColorRamp kImpactRingColor = {
            { 0.5f, 0.9f, 1.0f, 0.7f },
            { 0.2f, 0.6f, 1.0f, 0.0f } };

        constexpr int kImpactSparkCount = 14;                // isabette saçilan kivilcim sayisi
        constexpr Fx::Span kImpactSparkSpeed = { 150.0f, 520.0f };   // baslangic hizi (birim/sn)
        constexpr Fx::Span kImpactSparkLife = { 0.4f, 0.9f };        // omru (sn)
        constexpr float kImpactSparkStartSize = 9.0f;
        constexpr float kImpactSparkEndSize = 1.5f;
        constexpr float kImpactSparkGravity = 400.0f;        // asagi ivme (birim/sn^2)
        constexpr float kImpactSparkDrag = 0.6f;             // surtunme (1/sn)
        inline const Fx::ColorRamp kImpactSparkColor = {
            { 0.6f, 0.95f, 1.0f, 1.0f },
            { 0.2f, 0.6f, 1.0f, 0.0f } };

        // --- Isin gövdesi: yumusak hale + parlak cekirdek ---
        constexpr float kCoreLife = 0.30f;                   // cekirdek 0.3 sn'de tamamen soner
        constexpr float kGlowLife = 0.55f;                   // hale 0.55 sn'de tamamen soner
        constexpr float kGlowBaseWidth = 12.0f;              // halenin yari genisligi (birim)
        constexpr float kGlowWidening = 2.5f;                // hale yaslandikca genislik carpani (saniye basina +2.5 kat)
        constexpr float kGlowAlpha = 0.75f;                  // halenin baslangic opakligi
        inline const glm::vec3 kGlowColor = { 0.25f, 0.7f, 1.0f };    // mavi
        constexpr float kCoreBaseWidth = 3.0f;               // cekirdegin yari genisligi (birim)
        constexpr float kCoreMinWidthFraction = 0.4f;        // cekirdek sonmeden once genisliginin %40'ina daralir
        inline const glm::vec3 kCoreColor = { 0.9f, 1.0f, 1.0f };     // neredeyse beyaz

        // --- Sarmal: isini saran iki sira parlayan nokta ---
        constexpr float kSpiralMinDotSpacing = 16.0f;        // iki nokta arasi en az mesafe (birim)
        constexpr float kSpiralMaxDots = 320.0f;             // bir sarmalda en fazla nokta (uzun isinda aralik acilir)
        constexpr float kSpiralTurnsPerUnit = 0.07f;         // isin boyunca her birimde sarmal ne kadar doner (radyan)
        constexpr float kSpiralSpinPerSecond = 14.0f;        // sarmalin zamanla donus hizi (radyan/sn)
        constexpr float kSpiralStartRadius = 5.0f;           // sarmalin ilk yaricapi (birim): isina yakin baslar
        constexpr float kSpiralExtraRadius = 24.0f;          // zamanla ne kadar genisledigi (birim)
        constexpr float kSpiralSpreadEnd = 0.8f;             // genisleme, isin omrunun %80'inde tamamlanir
        constexpr float kSpiralDotSize = 6.0f;               // noktanin yari boyutu (birim)
        constexpr float kSpiralTwinkleFrequency = 0.05f;     // parildama dalgasinin isin boyunca sikligi
        constexpr float kSpiralTwinkleSpeed = 30.0f;         // parildama dalgasinin zamanla hizi (radyan/sn)
        constexpr float kSpiralPointPixels = 5.0f;           // sprite yokken noktalarin ekrandaki capi (piksel)
        inline const glm::vec3 kSpiralColorA = { 0.35f, 0.85f, 1.0f };   // birinci sarmal: acik mavi
        inline const glm::vec3 kSpiralColorB = { 0.8f, 0.95f, 1.0f };    // ikinci sarmal: beyaza yakin
    }

    //--------------------------------------------------------------------------
    namespace Orb {
        // "orb" shader'iyla cizilen parildayan enerji topu.
        struct Style {
            float size;           // dortgenin kenar uzunlugu (birim); parlama bunun icinde kalir
            glm::vec3 color;      // topun rengi
            float spikes;         // yildiz isini sayisi
            int seedModulus;      // 0: sabit tohum; >0: tohum entity kimliginden gelir (her top farkli parildar)
        };

        inline const Style kStapleBall = { 44.0f, { 0.35f, 0.85f, 1.0f }, 8.0f, 97 };    // acik mavi, 8 isin
        inline const Style kBotBall = { 64.0f, { 1.0f, 0.35f, 0.15f }, 6.0f, 89 };       // kirmizi-turuncu, 6 isin
        inline const Style kLastBall = { 300.0f, { 0.4f, 1.0f, 0.6f }, 10.0f, 0 };       // dev yesil top, 10 isin
    }

}  // namespace FxTuning