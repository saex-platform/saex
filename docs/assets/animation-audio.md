# Animasyon, skeleton, ses ve efekt asset'leri

Durum: sözleşme taslağı + R-07/R-09. [Katalog](catalog.md) · [UI/voice](../platform/ui-voice.md)

## Animasyon modeli

IFP kütüphanesi, klip kimliği, skeleton ve animasyon graph'ı ayrı asset'tir. MTA IFP yükleme ve özel namespace kullanımını destekler; bu, kendi adapter'ımız için araştırma referansıdır. Yükleme tek başına bütün oyuncuların aynı klip zamanında olduğu anlamına gelmez. [MTA EngineLoadIFP](https://wiki.multitheftauto.com/wiki/EngineLoadIFP)

| Tanım | Gerekli metadata |
|---|---|
| Skeleton | Kemik hiyerarşisi, bind pose ve compatibility ID |
| Clip | Library ref, clip name, süre, loop ve root motion mode |
| Graph | State, blend, transition koşulları, kesilme önceliği |
| Event marker | Klip zamanı ve olay türü; gameplay/cosmetic ayrımı |
| Attachment point | Kemik/soket kimliği, local transform |

ANPK/ANP2/ANP3 gibi girdi formatlarının kendi adapter desteği R-07'de sınanır. GTA dışı skeleton veya hayvan animasyonlarının sırf dosya uzantısı IFP olduğu için çalışacağı varsayılmaz.

## AnimationState aktarımı

EntityRef, clip/graph revision, state, server start tick, playback rate, loop, blend ve animation epoch taşınır. Geç gelen client o anki fazdan başlar. Bitmiş kısa klibin eski event'i yeni giren oyuncuda tekrar hasar yaratmaz.

Root motion kararları: `visual-only` klip entity transform'unu değiştirmez; `controller-driven` hareket controller tarafından hesaplanır; native root motion `validated-native` yolu üzerinden server'a önerilir. Aynı animasyon hem controller hem native root motion ile iki kat hareket üretemez.

Melee hit, eşya kullanımı veya silah doldurma sonucu yalnız client animation marker'ından kesinleşmez. Server zaman çizelgesi ve gameplay state koşulları onayı belirler. Cosmetic ayak sesi marker'ı frame hassasiyetinde yerel çalınabilir.

## Resource stop ve çakışma

IFP namespace'i package kimliği taşır. Stok animasyon override'ları açık world profilinden gelir; son yüklenen resource varsayılan kazanan değildir. Klipte aktif referans varken library unload edilmez; stop tanımlı güvenli idle/pose'a geçiş yapar.

Library sürümü değişirse eski entity referansı eski library lease'ini bitirene kadar yaşatır. Skeleton uyumsuz değişim restart/migration sınıfıdır. Eksik zorunlu klipte animasyon yerine rastgele stok klip kullanarak gameplay zamanlaması bozulmaz.

## Ses bankaları

Ses tanımı: asset ref, örnekleme/kanal metadata, stream/decode modu, loop noktaları, attenuation eğrisi, occlusion sınıfı, ses grubu ve eşzamanlı voice limiti. Büyük müzik/ambience streaming, kısa kırılma sesi preload sınıfında olabilir.

Kırılma sesi accepted transition event'ine bağlıdır. Aynı EventId duplicate geldiğinde tekrar çalmaz. Sonradan katılan client devam eden loop'u güncel faz/policy ile başlatır; geçmişte kırılmış bütün camların sesini yeniden duymaz.

Portal/interior ses geçirgenliği world verisidir. Client kalite seviyesi daha ucuz filtre kullanabilir, fakat başka instance sesini açamaz. Ses eventi private kanalın varlığını yetkisiz oyuncuya sızdırmamalıdır.

## Efekt ve ışık

Toz, kıvılcım ve darbe izi cosmetic olabilir; yangın hasarı gameplay sistemidir. Blackout state'i ışık kaynağını kapatır; “elektrik yok” ekonomi/AI tepkisi isteğe bağlı paket dinleyicisidir. Rastgele efekt seed'i görsel tekrarlanabilirliği artırır, fizik doğruluğu sağlamaz.

Kabul: AC-04 sonradan giriş, AC-08 unload, AC-16 yıkım sesi, AC-19 istemci modlama. Metrikler animation lease, aktif audio voice, decode queue ve event dedup sayılarıdır.

## Agent ve hayvan animasyon closure'ı

[Animals](../gameplay/animals-species.md) RigProfile/AnimationSet ile gait, dönüş, idle, reaction/death ve seçilmiş action'ları tanımlar. Breed görünümü aynı rig'i paylaşabilir; beden/kemik/timing farkı ayrı gameplay capability ve conformance gerektirir. Retarget, IK, ragdoll ve root motion desteği dosya adından çıkarılmaz. Model + human animasyon fallback'i desteklenmeyen hayvanı geçerli yapmaz.

MTA entity animasyon replacement'ını otomatik ağda eşitlemez ve partial animation sınırı belirtir. [Resmî API](https://wiki.multitheftauto.com/wiki/EngineReplaceAnimation). SAEX task/action ve animation phase/epoch'u state olarak taşır; late join ölüm/saldırı geçmişini yeniden hasar olarak oynatmaz. Havlama/ayak sesi cosmetic olabilir; AI'ın duyduğu stimulus server accepted olayından gelir, client mute/volume bağımsızdır. AC-50/56/57/58/62.
