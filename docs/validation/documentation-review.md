# D0 v0.5 dokümantasyon inceleme raporu

İnceleme tarihi: 12 Eylül 2026. Bu rapor **D0 v0.5 teslimatının tarihsel envanteridir**; aşağıdaki “uygulanmadı/yalnız Markdown” ifadeleri o kesitin durumudur. Kullanıcı onayıyla başlayan v0.6 D1 kodlama ve güncel test sonuçları [uygulama durumunda](../development/status.md) ve [D1 kanıtında](../development/d1-foundation.md) tutulur. V0.2 [tutarlılık](consistency-audit.md), [v0.3](architecture-evolution.md), [v0.4](population-animal-integration.md) ve [v0.5 denetim](stability-audit.md) kayıtları korunur. [Ana indeks](../../README.md) · [Yol haritası](../roadmap.md)

## Teslimat envanteri

| Kalem | İçerik |
|---|---|
| Dosyalar | 59 Markdown belgesi; v0.5'te 39 mevcut belge güncellendi, 4 yeni belge eklendi; uygulama kaynak dosyası veya proje iskeleti yok |
| Diyagramlar | 16 Mermaid mimari/akış/durum/sıra diyagramı; v0.5'te 2 yeni |
| Veri örnekleri | 6 açıklamalı JSON blok; gerçek manifest/asset dosyaları değil |
| API örneği | C# kavramsal kullanım bloğu; SDK implementasyonu değil |
| Kabul şartnamesi | AC-01–AC-88, toplam 88 gelecek gameplay/operasyon senaryosu; v0.5'te 18 yeni |
| Kararlar | ADR-01–ADR-32, toplam 32 tasarım kararı |
| Araştırmalar | R-01–R-23, ayrıca R-11a/b/c alt kapıları; 26 kayıt satırı, tamamı uygulama kanıtı bekliyor |
| Kaynaklar | 31 birincil kaynak/grup kaydı; v0.5 zaman/transaction/kilit kaynakları dahil |
| Fikir eşlemesi | Önceki 49 kayıt ve stabilite isteğine ait ST-01–08, toplam 57 gereksinim kaydı |

## Gerçekten yapılan kontroller

- 59 Markdown dosyası UTF-8 okuma, başlık, encoding bozulma işareti, yinelenen/boş alt başlık ve kapalı/tanınan kod blokları açısından tarandı.
- Yerel Markdown bağlantılarının hedef dosyaları ve ana indeks kapsamı doğrulandı; tabloların sütun yapısı kontrol edildi. Kesin sayılar aşağıdaki son tarama kaydındadır.
- JSON kod blokları standart JSON parser ile ayrıştırıldı. Bu kontrol yalnız JSON sözdizimini doğrular; henüz uygulanmamış manifest şeması veya asset varlığı testi değildir.
- 16 Mermaid bloğu Mermaid 11.17.2 ile headless Microsoft Edge içinde parse/render edildi; tamamı başarılı. V0.5'in iki yeni işlem kurtarma/transfer diyagramı ayrıca görüntü olarak incelendi.
- AC/ADR/R/S kimlikleri, açık sayısal aralıklar ve ana indeks kapsamı kayıt tablolarına karşı kontrol edildi; tanımsız kayıt referansı bulunmadı. R-11a/b/c alt kapıları ayrıca tanımlandı.
- V0.2'de MTA kaynak akışları ve GNS/ENet API/tasarım kaynakları; v0.3'te OpenUSD, Epic GAS ve TUF belgeleri incelenmişti. V0.4'te OneSync, MTA ped kontrolü, model/animasyon replacement, IFP ve araç türü resmî sayfaları okundu. Yeni bulgular [MTA ekine](../references/mtasa-architecture.md) ve [kaynak envanterine](../sources.md) bağlandı. Bütün üçüncü taraf kaynak kodunun denetlendiği iddia edilmiyor.
- V0.2'deki 22 commit'e sabit dosya/satır atfının hedefleri bu revizyonda değiştirilmedi; önceki kaynak incelemesi tarihsel kanıttır. Bütün dış wiki adresleri veya eski C++ snapshot'ları bu revizyonda yeniden denetlenmedi.
- Çalışma dizininde yalnız Markdown çıktı bulunduğu doğrulandı. Geçici doğrulama runtime'ı ve render görüntüleri proje dışında tutuldu.
- V0.5'te Microsoft QPC/TimeProvider, commit belirsizliği ve PostgreSQL kilit açıklamaları okundu; S-28–31'e bağlandı. Yeni MTA binary, bütün eski dış bağlantılar veya başka platformların runtime performansı yeniden incelenmedi.
- V0.4 kopyası proje dışında korunarak metin değişimleri karşılaştırıldı. Alanı etkilenmeyen 16 mevcut belge korunmuştur; bütün 59 belge yapısal taramaya dahildir. Kalan araştırma yetenekleri kodlanmış gibi gösterilmedi.

## Mimari tutarlılık incelemesi

| İncelenen kural | Belgelerdeki sonuç |
|---|---|
| Client physics ile server authority | validated-native ile server-simulated ayrıldı; deterministik GTA replay vaat edilmedi |
| Kırılma ve kalıcılık | Hasar/fragment/collider transaction'ı, durable commit ve late join state'i birbirine bağlandı |
| Stok harita yeniden yüklemesi | PlacementId/override korunuyor; R-05 doğrulanmadan genel garanti verilmedi |
| Asset readiness | Download, decode, GPU/native bind ve world activation ayrı aşamalar |
| Resource sandbox | ALC güvenlik sayılmadı; OS/broker ve R-03 yayın kapısı tanımlandı |
| Canlı güncelleme | Commit öncesi abort ile commit sonrası ters migration/forward-fix ayrıldı |
| Ready / Applied | Resource ve asset akışları tek bariyer sırasına bağlandı; client ACK eksikse gameplay fence edilir |
| İstek / kalıcı işlem | RequestId korelasyon, OperationId durable idempotency; reconnect ve crash receipt'i açık |
| Snapshot / journal | Canlı WorldRevision ve disk JournalSequence ayrı; none state canlı baseline'a dahil |
| Çapraz lane | GNS'de lane'ler arası sıra yok; entity state ve catalog önkoşulu bounded queue ile korunur |
| Frame atomikliği | Çok kare staging ve bounded aggregate swap; bütün baseline tek kare işi değil |
| MTA lifecycle referansı | Broker-owned registry, deferred free, model lease ve syncer/streaming hysteresis sözleşmelere işlendi |
| Instance / interior | Portal modeli, world/scope epoch ve voice/hit filtreleri birlikte tanımlandı |
| Server collision | Görsel dosya yerine query/physics artifact ihtiyacı ve R-12 kaynağı açık |
| Ölçek | Toplam bağlantı, yerel yoğunluk ve gerçek client ölçümü ayrıldı |
| Taslakların kapsamı | Çekirdek, isteğe bağlı kit ve araştırma gereksinimi açıkça eşlendi |
| WorldPlan | Provider/mutator/schema/schedule çözümü runtime grant ve gameplay state'ten ayrıldı |
| Execution | Tek authoritative writer, paralel read-only işler ve bounded async completion birlikte tanımlandı |
| Prefab ve kalite | Bileşim/provenance, runtime hasar ve GameplayDigest eşdeğerliği ayrıldı |
| Actions / projection | İsteğe bağlı aksiyon paketi; CriticalSet ve gecikmeli derived/follow-up ayrı |
| Prova / update | Test world'ü üretimden yalıtılıyor; planlı rollback eski metadata replay'i değil |
| Capability kanıtı | Conformance sonucu engine/build/schema/artifact kapsamına bağlı; statik validation native kanıt değil |
| Canlı nüfus ayarı | Mutable policy mevcut WorldPlan sınırında state; yeni provider/tür/controller yeni release |
| Bağımsız trafik kanalları | Yaya off sürücü/pilotu silmez; wildlife ve mission/owned sınıfları ayrıdır |
| Spawn ve cleanup yarışları | Policy revision/slot/generation/kota commit'te revalidate; protected drain bounded |
| NPC ortak davranışı | Server goal/task, typed nav/controller, accepted animation/motion ve owner epoch birlikte |
| Hayvan bileşimi | Species/Breed/Morphology/Rig/Appearance ile AnimalInstance/PersistentEntityId ayrı |
| Rig/hit ve büyüme | Load success destek kanıtı değil; yeni beden/collider/action timing capability ve readiness gerektirir |
| Sürü ve offline yaşam | Sosyal grup fizik grubu değil; tek sahiplik/ölüm/loot/doğum ve bounded catch-up |
| Nüfus yükü | AI sürücü/pilot/yaya/hayvan toplam NPC sayacında; araç ve actual native/body maliyetleri ayrı |
| Zaman / resume | SimulationTick, monoton ControlTime ve UTC ayrı; stall lease'i uzatmaz; R-21 kanıtı gerekli |
| Durable completion | Transient JobResult atılabilir; eski worker'ın committed receipt'i core registry'de kurtarılır |
| Belirsiz commit | Timeout/cancel rollback değildir; outcome çözülene kadar affected aggregate fenced |
| Tek world writer | Backend WriterTerm guard; v1 otomatik HA takeover yok, eski process duruşu doğrulanır |
| Transfer | Aynı core/backend, canonical location ve phase-aware quota; iki etkileşimli birey yok |
| Replikasyon onarımı | Exact baseline cut/delta base, Applied ACK, bağımsız motion ve bounded resync |
| Native state doğruluğu | Logical digest doğru olması collider doğruluğu değildir; scoped rebind/fence ayrı |
| Toplam recovery yükü | Admission/queue/restart per-actor/resource/world/process tavanlarına bağlı |
| Restore closure | Persistent birey, backup, operation ve migration kökleri exact artifact'i korur |
| SDK örneği / fizik fallback | Pending sonuç ret yapılmaz; uçuşta genel dondurma varsayımı kaldırıldı |

## Yapılmayan doğrulamalar

GTA executable incelemesi, MTA/GNS/ENet binary build/benchmark'ı, SAEX adapter build'i, C# worker çalıştırması, güvenlik kaçış testi, gerçek ağ/voice/physics testi, server restart/restore deneyi veya asset cook yapılmadı. AC senaryoları çalıştırılmış test listesi değildir.

R-01–R-23 ve R-11a/b/c açık kalır. GNS peer identity/trust-root entegrasyonu özellikle üretim katılım kapısıdır; R-16 updater/prova kanıtı ayrıdır. WorldPlan compiler, schema generator, Actions/Effects, PopulationService, AgentService, Animals, türetilmiş dünya görünümleri ve Studio uygulanmadı. GTA native stok nüfusu, uçuş owner-loss davranışı, custom animal rig/hit/controller ve ecology için runtime kanıtı yoktur. V0.5 clock/watchdog, WriterTerm, recovery registry, transfer ve native repair de tasarım düzeyindedir. D0'ın tamamlanması D1–D5'in geçtiği anlamına gelmez. Sayısal limitler ölçüm hedefleridir.

## Son statik tarama kaydı

Yerel bağlantı: 590. Markdown tablo: 103. Dosya: 59. JSON: 6. Mermaid: 16. AC: 88. ADR: 32. R: 23 ana kayıt + 3 alt kapı. Kaynak: 31. Son kontrol hatası: 0. Sayılar yalnız belge envanteri ve yapısal kontroldür; 88 gameplay senaryosunun geçtiği anlamına gelmez.

## Yeniden inceleme koşulu

Entity/asset kimliği, authority, durable başarı, katalog migration veya worker güven sınırı değişirse ilgili ADR ve kabul senaryoları birlikte güncellenir. Yalnız yazım düzeltmesi için gameplay testleri uydurulmaz; link/örnek/diyagram değiştiyse uygun doküman kontrolü tekrar yapılır.
