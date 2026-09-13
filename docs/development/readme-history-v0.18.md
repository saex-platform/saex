# SAEX README arşivi — mimari v0.18

Bu belge yayın hazırlığı öncesindeki README açıklamalarını koruyan tarihsel arşivdir. Güncel başlangıç [ana README](../../README.md), durum [status](status.md) ve belge indeksi [docs](../README.md) üzerindedir.

GTA: San Andreas üzerinde kendi dünyanı ve oyun deneyimini geliştirmek için tasarlanan modüler platform. [GitHub organizasyonu ve ilk public yayın planı](../../docs/development/github-publication-plan.md), SAEX markasını ve mevcut kaynakların yayın düzenini tanımlar; kurgu henüz GitHub'a uygulanmadı.

**Durum: mimari v0.18, kod 0.1.11 — D1 foundation ve N2 motor/başlangıç doğrulama araçları, 13 Eylül 2026.** C++20 çekirdek temeli, C#/.NET 10 araçları ve çalışan conformance testleri mevcut. GTA multiplayer istemcisi/sunucusu henüz yoktur. [Uygulama durum tablosu](../../docs/development/status.md) kodlanmış, test edilmiş ve planlanan kapsamı ayırır; aşağıdaki mimari vaatler otomatik olarak uygulanmış özellik değildir.

**V0.7 native entegrasyon yönü:** Dryxio/plugin-sdk-sa, sabit kaynak commit'iyle x86 GTA adapter'ı için seçildi. [Kaynak incelemesi](../../docs/references/plugin-sdk-sa.md), [entegrasyon sözleşmesi](../../docs/architecture/native-sdk-integration.md) ve [D1 uygulama sırası](../../docs/development/d1-engine-integration.md); SDK bağımsız profil kontrolü, hook ömrü, frame/IPC, entity ve asset kapılarını tanımlar. V0.8'de [D1-N1 kaynak kilidi ve private x86 build](../../docs/development/d1-native-dependency.md) uygulandı. SDK'nin C++23 gereksinimi ADR-36 ile bu hedefe sınırlandı; gerçek GTA desteği açılmadı.

SAEX'in amacı klasik PC GTA: San Andreas üzerinde kendi haritasını, kurallarını, arayüzünü ve etkileşimlerini tanımlayabilen oyunlar üretmektir. Platform; Windows istemci, Linux/Windows x64 sunucu, C++20 çekirdek, iki tarafta C#/.NET 10 LTS resource'ları ve içerik odaklı bir asset sistemi etrafında tasarlanmıştır.

**Ağ kararı kesin:** v1 oyun trafiği **GameNetworkingSockets**, asset trafiği **HTTPS**; ENet bağımlılığı/fallback yok. [Ayrıntılı seçim](../../docs/decisions/network-transport.md). GNS karşı uç kimliği entegrasyonu R-11a uygulama/yayın kapısıdır; yalnız şifreleme üretim kimlik doğrulaması sayılmaz.

V0.2, [MTA kaynak kodu incelemesini](../../docs/references/mtasa-architecture.md) ve [15 maddelik tutarlılık düzeltmesini](../../docs/validation/consistency-audit.md) içerir. Ortak işlem kimliği, revision, hazır olma ve commit kuralları [aktivasyon sözleşmesinde](../../docs/architecture/activation-contracts.md) toplanmıştır.

**V0.3 yönü:** [WorldPlan, şemalı SDK, bileşimli asset'ler, Actions/Effects ve yayın provası](../../docs/platform-blueprint.md). Dünya açılmadan paket/provider çakışmalarını bulma, değişikliğin nedenini açıklama ve izole test dünyasında doğrulama ana geliştirme iş akışıdır. [Mimari geliştirme kaydı](../../docs/validation/architecture-evolution.md) her ekin gerekçesini ve aşamasını gösterir.

**Temel vaat:** Bir oyuncu sokak lambasını devirdiğinde diğer oyuncular ve sonradan bağlananlar aynı geçerli dünya durumunu görür. Yıkım, model değiştirme çağrısından ibaret değildir: collision, hasar, parçalar, ışık, ses, navigasyon ve kalıcılık aynı işlemle ilişkilidir.

**V0.4 yaşayan dünya:** [Yaya, kara/hava trafiği ve dinamik nüfus](../../docs/gameplay/population-traffic.md), [ortak NPC/AI ve navigasyon](../../docs/gameplay/agents-navigation.md), [modüler hayvan türleri/cinsleri](../../docs/gameplay/animals-species.md). Sunucu kanalları ayrı yönetir; sahipli/etkileşilen bireyler korunur. Tür, beden, rig, animasyon ve davranış bağımsız tanımlanır. Tam sync, ortak accepted state ve ölçülen yakınsamadır; native davranışların tamamı için sıfır hata/sıfır gecikme vaadi değildir. [V0.4 entegrasyon kaydı](../../docs/validation/population-animal-integration.md)

**V0.5 stabilite:** [16 açık ve düzeltmesi](../../docs/validation/stability-audit.md); oyun durunca uzamayan yetki süresi, resource çökünce korunan durable sonuç, belirsiz commit için kurtarma, tek world writer, onarılabilir baseline/delta, bounded kuyruk/restart ve eski tür/yedek içeriğini koruyan retention. Kalıcı hayvan/araç transferi aynı core ve atomik backend sınırında tekil bireyi korur. Yeni AC-71–88 uygulamada kanıtlanacak şartlardır.

V0.9'da [D1-N2 dosya/image gözlemi](../../docs/development/d1-engine-preflight.md) kodlandı: exact PE/hash/anchor profili, SDK bağımsız C++20 denetimi ve Windows'ta çalıştırılmayan image okuması. C# aynı profili kullanır. Gerçek GTA process/bootstrap/hook doğrulaması sürer; gözlem kaydı attach izni vermez.

V0.10'da [SDK bağımsız x86 başlangıç DLL'si](../../docs/development/d1-bootstrap-module.md) eklendi: sürümlü C ABI, açık Initialize/Query/Stop, yanlış host/ABI reddi, BUSY ve terminal stop. Modül kendi test executable'ımızda yüklenip boşaltılır; gerçek GTA'ya yüklenmedi ve hook/oynanış açılmadı.

V0.11'de [askıya alınmış process observer](../../docs/development/d1-suspended-process.md) eklendi: ilk OS debug olayındaki dosya kimliği ve image doğrulanır; ana thread resume edilmeden owned child kapatılır. Unpack/başlatma, function ABI ve gerçek DLL yükleme kapıları hâlâ açıktır.

V0.12'de [native başlangıç envanteri](../../docs/development/d1-native-startup.md) eklendi: executable ve yerel DLL/ASI girdilerinin hash, import/delay import, entry ve TLS kayıtları çıkarılır. Yerel aday grafiği gerçek Windows yükleme sonucu değildir; loader fazına geçiş ve attach izni kapalıdır.

V0.15'te [açık başlatma bağlamı](../../docs/development/d1-launch-context.md) eklendi: absolute çalışma klasörü ve bir defa alınmış environment görüntüsü ayrı CLI modunda açık kullanılır; ortam değerleri raporlanmaz. DLL initialization/SAEX yükleme ve N3 hâlâ açıktır.

V0.18'de [kontrollü PE giriş durağı](../../docs/development/d1-entry-boundary.md) eklendi: ayrı komut DLL/TLS başlangıcını ilerletir, ana thread PE girişinde donanım breakpoint'iyle tutulur. Oyun girişini, unpack veya SAEX bootstrap'ı yürütme izni değildir; gerçek sonuçlar raporda ayrılır.

V0.17'de [native sembol bağlantı denetimi](../../docs/development/d1-native-linkage.md) eklendi: açık consumer/import/adayı için bounded import/export karşılaştırması, ordinal/alias/forwarder ayrımı ve hash kanıtı. Statik eşleşme initialization veya ABI izni değildir.

V0.16'da [loader modül yaşamı](../../docs/development/d1-loader-lifecycle.md) eklendi: bilinen unload emekli edilir, aynı adrese yeni yükleme yeni gözlem kimliği ve tekrar dosya doğrulaması gerektirir. Geçmiş kayıt aktif modül sayılmaz; ilk exception sınırı korunur.

## Kodlama ve ilk çalıştırma

V0.14'te [incelenmiş loader mapping profili](../../docs/development/d1-loader-policy.md) eklendi: engine ve 22 DLL'nin exact hash'leri child öncesi doğrulanır. Orijinal GTA kurulumu AcLayers uyumluluk modülünde ret alırken, aynı byte'ları taşıyan ayrı yerel kopya başlangıç breakpoint adayına ulaştı. Bu farklı bağlamlar initialization veya oynanabilir GTA desteği değildir; orijinal dosyalar değiştirilmedi.

V0.13'te [sınırlı x86 Windows loader gözlemi](../../docs/development/d1-loader-observation.md) eklendi. Kendi test programında DLL/TLS/main başlamadan breakpoint adayı tutulur. Gerçek GTA'da ilk izin verilmeyen apphelp.dll mapping'inde duruldu ve child çıkışı doğrulandı; bu tam initialization veya SAEX DLL yükleme kanıtı değildir.

Windows'ta C++ Build Tools, CMake, .NET 10.0.300 feature band ve Python 3 ile `./tools/build.ps1` çalıştırılır. Bu komut native/managed testleri ve zorunlu belge eşleme kontrolünü yürütür. [Kurulum/derleme](../../docs/development/workflow.md) · [D1 kaynakları ve kanıt](../../docs/development/d1-foundation.md) · [Değişiklik kaydı](../../docs/development/change-log.md) · [Geliştirme kuralları](../../AGENTS.md)

Her kaynak ekleme, değişiklik ve çıkarma ilgili normatif belge, status ve change-log ile birlikte yapılır. Oyun taşıması GNS ve asset HTTPS kararı korunur; ilk fixture yerel process/codec deneyidir. `gta_sa.exe` salt okunur incelendi; doğrulanmış hook profili olmadığından attach kapalıdır.

## Önce bunları okuyun

1. [Vizyon ve sınırlar](../../docs/vision.md)
2. [Mimari ve süreçler](../../docs/architecture/overview.md)
3. [GTA entegrasyonunun gerçek sınırları](../../docs/architecture/engine-adapter.md)
4. [Asset kataloğu](../../docs/assets/catalog.md) ve [yıkım sözleşmesi](../../docs/assets/destruction.md)
5. [Otorite modeli](../../docs/networking/authority.md) ve [sonradan katılım](../../docs/networking/replication.md)
6. [Geliştirme yol haritası](../../docs/roadmap.md)

Güncel yapının toplu açıklaması için [platform tasarımını](../../docs/platform-blueprint.md), gerçek kod sınırı için [D1 durumunu](../../docs/development/status.md) okuyun.

## Tam doküman indeksi

| Grup | Belgeler |
|---|---|
| Temel | [Vizyon](../../docs/vision.md), [terimler](../../docs/glossary.md), [fikir kapsamı](../../docs/coverage.md) |
| Geliştirme | [Uygulama durumu](../../docs/development/status.md), [iş akışı](../../docs/development/workflow.md), [D1 foundation](../../docs/development/d1-foundation.md), [native dependency kanıtı](../../docs/development/d1-native-dependency.md), [N2 preflight kanıtı](../../docs/development/d1-engine-preflight.md), [başlangıç DLL'si](../../docs/development/d1-bootstrap-module.md), [askıda process gözlemi](../../docs/development/d1-suspended-process.md), [native başlangıç envanteri](../../docs/development/d1-native-startup.md), [sembol bağlantı denetimi](../../docs/development/d1-native-linkage.md), [değişiklik kaydı](../../docs/development/change-log.md) |
| Native entegrasyon | [Plugin-SDK kaynak incelemesi](../../docs/references/plugin-sdk-sa.md), [normatif entegrasyon](../../docs/architecture/native-sdk-integration.md), [D1 motor uygulama planı](../../docs/development/d1-engine-integration.md) |
| Dünya üretimi | [Platform tasarımı](../../docs/platform-blueprint.md), [WorldPlan](../../docs/architecture/world-plans.md), [şema ve execution](../../docs/architecture/execution-contracts.md) |
| Bileşim ve davranış | [Prefab/trait/variant](../../docs/assets/composition-variants.md), [Actions/Effects](../../docs/gameplay/actions-effects.md), [ChangeSet/projection](../../docs/networking/world-change-projections.md) |
| Üretim araçları | [Studio ve release provası](../../docs/platform/studio-release.md), [conformance sözleşmesi](../../docs/validation/conformance-contracts.md), [v0.3 geliştirme kaydı](../../docs/validation/architecture-evolution.md) |
| Mimari | [Genel yapı](../../docs/architecture/overview.md), [motor uyarlaması](../../docs/architecture/engine-adapter.md), [entity ve dünyalar](../../docs/architecture/entities-worlds.md), [ortak aktivasyon](../../docs/architecture/activation-contracts.md) |
| Stabilite ve kurtarma | [Zaman ve yetki süresi](../../docs/architecture/time-fencing.md), [arıza/işlem kurtarma](../../docs/architecture/failure-recovery.md), [replikasyon onarımı ve entity transferi](../../docs/networking/consistency-recovery.md), [v0.5 denetim kaydı](../../docs/validation/stability-audit.md) |
| Ağ ve dünya | [Otorite](../../docs/networking/authority.md), [protokol](../../docs/networking/protocol.md), [replikasyon](../../docs/networking/replication.md), [fizik](../../docs/networking/physics.md), [kalıcılık](../../docs/networking/persistence.md) |
| Modlama | [C# SDK](../../docs/resources/runtime-sdk.md), [resource manifesti](../../docs/resources/manifest.md), [yaşam döngüsü](../../docs/resources/lifecycle-hot-reload.md), [güvenlik ve native sınırlar](../../docs/resources/security-native.md) |
| Asset temeli | [Katalog](../../docs/assets/catalog.md), [manifest ve kimlik](../../docs/assets/manifest-identity.md), [üretim hattı](../../docs/assets/build-pipeline.md), [dağıtım ve cache](../../docs/assets/distribution-cache.md), [streaming ve bütçeler](../../docs/assets/streaming-budgets.md) |
| Asset davranışı | [Fizik malzemeleri](../../docs/assets/physics-materials.md), [yıkım](../../docs/assets/destruction.md), [animasyon ve ses](../../docs/assets/animation-audio.md), [harita ve prefab](../../docs/assets/maps-prefabs.md), [sürüm ve override](../../docs/assets/versioning-overrides.md) |
| Platform | [UI ve voice](../../docs/platform/ui-voice.md), [launcher ve işletim](../../docs/platform/launcher-operations.md), [geliştirici araçları](../../docs/platform/developer-tools.md) |
| Oyun geliştirme | [İsteğe bağlı oyun sistemleri](../../docs/gameplay/sandbox-kits.md), [tam dönüşüm örneği](../../docs/gameplay/total-conversion.md) |
| Yaşayan dünya | [Population ve trafik](../../docs/gameplay/population-traffic.md), [Agent/AI/navigasyon](../../docs/gameplay/agents-navigation.md), [hayvan/tür/cins](../../docs/gameplay/animals-species.md), [v0.4 entegrasyon ve kapsam](../../docs/validation/population-animal-integration.md) |
| Doğrulama | [Kabul senaryoları](../../docs/validation/scenarios.md), [performans profilleri](../../docs/validation/performance.md), [dokümantasyon incelemesi](../../docs/validation/documentation-review.md), [v0.2 tutarlılık denetimi](../../docs/validation/consistency-audit.md) |
| Kararlar | [Mimari karar kayıtları](../../docs/decisions/architecture-decisions.md), [GNS/ENet kararı](../../docs/decisions/network-transport.md), [araştırma kayıtları](../../docs/decisions/research-register.md), [yol haritası](../../docs/roadmap.md), [kaynaklar ve karşılaştırma](../../docs/sources.md), [MTA kaynak incelemesi](../../docs/references/mtasa-architecture.md) |

## Belgeler nasıl yorumlanmalı?

- **Karar:** Bu tasarımın kabul edilmiş varsayılanı; değişikliği mimari karar kaydı gerektirir.
- **Sözleşme taslağı:** Uygulayıcının uyması beklenen davranış. Gerçek API veya sabitlenmiş binary ABI değildir.
- **Araştırma:** Oyun motorunda veya çalışma ortamında deney gerektiren kabiliyet. İlgili R kimliği ve geçiş koşulu bulunur.
- **Ölçüm hedefi:** Gelecekte test edilecek eşik. Bugün yapılmış benchmark değildir.
- **Dış kaynak bulgusu:** Başka bir projenin resmî belgesinde görülen özellik. SAEX desteğini kanıtlamaz.

Eski mimari API/manifest örnekleri, uygulama durum tablosunda eşleşen kod gösterilmedikçe taslaktır. `contracts/`, `include/`, `src/`, `managed/`, `tools/` ve `tests/` D1 kaynaklarını içerir. `samples/foundation` sentetik metadata fixture'ıdır; gerçek GTA asset'i veya çalışan oyun resource'u değildir. Oyun dosyaları dağıtılmaz/değiştirilmez.

## Değişmez ilkeler

Sunucu geçerli dünya durumunu belirler. Gecikmesiz yerel geri bildirim ile sunucu kararı ayrı kavramlardır. Her fizik adımının tüm makinelerde bit düzeyinde aynı çıkacağı varsayılmaz. GTA'nın sayısal model havuzları platform kimliği değildir. Resource yetkisi, oyuncu yetkisi ve simülasyon sahipliği birbirine karıştırılmaz.

İlk ölçek hedefi 64–128 bağlantıdır; 256–512 sonraki kapıdır. Aynı yerdeki yoğunluk, toplam bağlantıdan ayrıca ölçülür. Ekonomi veya roleplay paketini kullanmadan yarış ya da survival sunucusu kurmak mümkün olmalıdır.

0.1.11 gerçek kanıt: 12/12 özel GTA koşusunda DLL başlangıcının oyun girişini vorbisfile.dll içindeki aynı RVA'ya yönlendirdiği ölçüldü; atlama yürütülmeden child kapatıldı. X86 Debug/Release ve x64 Debug kontrolleri geçti. [Kanıt ve sınırlar](../../docs/development/d1-entry-boundary.md).
