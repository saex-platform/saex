# Ortak nüfus, yaya, kara ve hava trafiği

Durum: v0.4 normatif tasarım, ADR-25/28; uygulanmış özellik değildir. [Agent altyapısı](agents-navigation.md) · [Hayvanlar](animals-species.md) · [Otorite](../networking/authority.md)

## Sorumluluk ve ürün sınırı

PopulationService, sunucunun seçtiği dünyada yaşayan ortam nüfusunun üretimini, yoğunluğunu ve güvenli kaldırılmasını koordine eden isteğe bağlı hizmettir. PedestrianProvider, RoadTrafficProvider, AirTrafficProvider ve WildlifeProvider ayrı seçilir. Çekirdek kimlik, kota, transaction, yaşam döngüsü ve replikasyon sağlar; sabit şehir nüfusu veya tür listesi içermez. PopulationService yeni zorunlu mikroservis veya her NPC için ayrı süreç değildir.

Hedef; ortak yayalar, sürücülü trafik, uçak/helikopterler ve ileride hayvanlarla etkileşilebilmesidir. GTA'nın denetimsiz yerel rastgele üretimi, silmesi, görev ataması ve hasarı adapter tarafından kontrol altına alınmalıdır. R-17 geçmeden stok şehir nüfusu desteği; R-18 geçmeden gelişmiş uçuş desteği ilan edilmez. Native GTA davranışlarının tamamının otomatik taşınacağı varsayılmaz.

FiveM OneSync, nüfus sahipliğini world grid üzerinden ele alır ve routing bucket nüfusunu kapatabilir. GTA Online tabanlı bu mekanizma tasarım referansıdır; GTA:SA uygulama kanıtı değildir. [Resmî OneSync açıklaması](https://docs.fivem.net/docs/scripting-reference/onesync/)

## Bağımsız kanallar ve kurallar

| Kanal | Ürettiği grup | Bağımsız ayarlar / sınır |
|---|---|---|
| pedestrians | Ortamda dolaşan insan NPC'ler | Aç/kapat, bölge, takvim, yoğunluk, model ve davranış havuzu |
| road-traffic | Araç + sürücü + isteğe bağlı yolcular | Aç/kapat, araç sınıfı, şerit, hız ve sinyal profili, yoğunluk |
| air-traffic | Uçak/helikopter + gereken pilot/yolcular | Aç/kapat, koridor, irtifa, görev ve kapasite; uçuş kabiliyetine bağlı |
| wildlife | Habitat kurallarına göre ortam hayvanları | Aç/kapat, tür havuzu, grup büyüklüğü, bölgesel kota |

`pedestrians=false` trafik sürücüsünü veya pilotunu kaldırmaz. Sürücü araçtan çıkınca kökeni road-traffic olarak kalır; başka kanala geçiş ayrı policy işlemidir. Görev aktörü, oyuncu tarafından kullanılan araç, sahipli hayvan ve çiftlik sürüsü ambient üretimden ayrı korunur. Polis/ambulans/taksi gibi hizmetlerin görev dispatch'i ayrı kit'tir; hangi spawn kotasını kullandığı açık seçilir. Tekne ve tren ileride kendi provider sözleşmesiyle eklenebilir, road/air desteğinden otomatik doğmaz.

`all-off`, `pedestrians-only`, `road-only`, `air-only`, `city-all` ve `wildlife-only` yalnız bu alanlara açılan isimli preset'lerdir. `city-all` ilk üç kanalı açar; wildlife ayrıca seçilir. `all-off` bütün ambient kanallarda yeni üretimi durdurur, görev/sahiplik kayıtlarına toplu silme hakkı vermez.

## Statik plan ve canlı politika

WorldPlan; provider, izinli tür/model havuzları, davranış sınıfları, kota tavanı, capability, kabul edilen fallback ve değiştirilebilir alanları sabitler. PopulationPolicy bu sınırlar içinde yaşayan sunucu state'idir. Yoğunluk/aç-kapat/zaman profili değişimi yeni binary veya plan gerektirmez. İlan edilmemiş tür, controller, provider, izin veya kota tavanı eklemek yeni plan/release ve gerektiğinde restart ister.

| Alan | Anlam |
|---|---|
| PolicyId / StateRevision | World'e ait politika aggregate'ı ve mevcut revision; ikinci global revision sayacı eklenmez |
| Channels | Kanal başına enabled, targetDensity, maxActive, spawnRate ve izinli tanım havuzu |
| ZoneOverrides | ZoneId, öncelik, kanal bazında açık alan patch'i; kalıtım kaynağı izlenir |
| Schedule | Sunucu simulation zamanı/takvim kaynağı ve izinli saat aralıkları |
| RetirementPolicy | drain varsayılanı, korunan sınıflar, deadline ve timeout sonucu |
| Persistence | Politika için none/checkpointed/durable; operasyon panelinde kapsam belirtilir |

Dünya varsayılanı → tek kazanan bölge patch'i → tanımlı zaman patch'i → yetkili süreli override sırası kullanılır. Örtüşen bölgelerde en yüksek açık öncelik kazanır; eşit öncelikte aynı alanı değiştiren patch'ler conflict'tir. Kimlik veya yükleme sırası kazanan seçmez. Değişmeyen alanlar world varsayılanından gelir. Hard-off güvenlik/bakım kuralı ve world kota tavanı bütün aşamalardan sonra uygulanır; bölge bunları yükseltemez. Sınır geçişinde hysteresis churn'ü azaltır, hard-off yeni üretimi hemen durdurur.

Panel/API, `SetPopulationPolicy(expectedRevision, patch, OperationId)` kavramsal komutunu kullanır; actor ve resource grant doğrulanır. Kabul edilmiş politika değişimi ile henüz bitmemiş nüfus temizliği ayrı sonuçlardır. `PopulationStatus`, effective policy, kaynağı, active/reserved/protected/draining sayıları ve blocked nedeni verir. Sayılar sadece yetkili kapsamda sorgulanır.

## Üretim ve kota akışı

1. Sunucu oyuncu interest alanlarının birleşimini ve bölge politikasını değerlendirir. Her oyuncu kendi kopya nüfusunu üretmez; aynı hücreye iki oyuncu girmesi hedef yoğunluğu ikiye katlamaz.
2. Director uygun spawn adayı ve tanım seçer. Kalıcı SpawnSlotId, SpawnGeneration ve CandidateId ile slot rezervasyonu yapılır; sınır hücresinin tek sorumlusu vardır. Dinamik encounter'lar da world registry'de tek üreticiye bağlanır.
3. World/kanal/zone kotaları, grup closure'ı, collision/nav uygunluğu ve gereken client hazırlığı kontrol edilir. Rezerve + active + draining gruplar birlikte sayılır. Araç, sürücü ve yolcular kendi native havuzlarına ayrı maliyet yazar.
4. Hazırlama async yürür. Commit'te Policy StateRevision, resource/world epoch, rezerv ve güncel occupied volume tekrar doğrulanır. Bu sırada kanal kapandıysa eski aday reddedilir.
5. Grup kimlikleri ve koltuk ilişkileri tek bounded transaction ile oluşturulur. Gerekli asset/controller/owner readiness tamamlanmadan etkileşim açılmaz. Aday üretmek başarı değildir.
6. Slot occupied olur; başarısız hazırlık kota rezervini bırakır. Spawn/retire/cooldown değişimleri server kayıtlarıdır; istemci görünürlüğü slotu yeniden boşaltmaz.

PopulationService görünür oyuncunun önünde rastgele spawn/despawn yapmamayı hedefler. Occlusion/güvenli uzaklık kesinleşmiyorsa üretim ertelenir; trafik yaratmak için collider içinde doğuş yapılmaz. Oyuncu sayısı veya mesafe dışında spawn rate, model çeşitliliği ve toplam native maliyet de bütçedir.

## Tam senkronizasyonun anlamı

| Ortak durum | Kabul kuralı |
|---|---|
| Entity kimliği, spawn, retire | Sunucu; client stream-out kimliği veya slotu silmez |
| Görev, rota hedefi, trafik sinyali | Yetkili server provider; sürümlü accepted state |
| Hareket ve yön | Seçilmiş simulation mode; tek geçerli owner/epoch ve sampled motion |
| Sağlık, ölüm, hasar, koltuklar | Sunucu doğrulaması ve transaction; duplicate sonuç yok |
| Etkileşim ve köken değişimi | İzin + revision + yaşam döngüsü/kota kontrolü |
| Yeni katılanın gördüğü durum | Baseline/catch-up; eski spawn veya görev olaylarını replay etmeden |

Ortak sonuç ve ölçülen yakınsama hedeflenir. Sıfır gecikme, her kare aynı animasyon kemiği veya bit düzeyinde native fizik eşitliği vaat edilmez. Uzak sunumda interpolation kullanılabilir; çarpışma, hasar ve görev sonucu farklı client'larda bağımsız kesinleştirilemez. Tüm şehir her client'a her frame gönderilmez. Gameplay state'i, izinli alıcı ve etkileşim closure'ına göre replike edilir.

## Etkileşim, köken ve kapatma

PopulationOrigin kanal, PolicyId, ZoneId, SpawnSlotId/generation ve doğuş cause'unu saklar. Ayrı ControlClass `ambient`, `mission`, `player-engaged`, `owned`, `service` olabilir. Bu sınıflar `resource/session/persistent-world` Lifetime veya `none/checkpointed/durable` persistence enum'larının yerine geçmez. Köken audit için korunur; mevcut kontrol sınıfı değişebilir.

Araç çalma: yetkili interaction → araç/sürücü/koltuk revision rezervi → sürücüyü çıkarma ve görev değişimi → koltuk hakkı ve ControlClass geçişi → accepted başlangıçtan motion sahipliği devri. Slot/kota disposition aynı işlemde kaydedilir. Korunan araç ambient aktif sayacından ayrı engaged kotasına aktarılır; toplam dünya/native kotasında sayılmaya devam eder. Böylece sınıf değiştirerek sınırsız spawn açılamaz. Native exit animasyonu sonucu tek başına koltuk hakkı vermez.

Varsayılan kapatma `drain`'dir: yeni üretim hemen kapanır; korumasız ambient gruplar güvenli noktada bounded retire edilir. Oyuncunun kullandığı araç, görev aktörü, havada etkileşilen uçak veya evcilleştirilmiş hayvan otomatik silinmez. Korunanlar için raporlanabilir pending/protected durumu kalır. Deadline aşılması korumayı sessizce kaldırmaz; policy `keep-protected` veya açıklamalı `blocked` seçer.

Yetkili zorla temizlik ayrı `RetirePopulation` işlemidir: hedef listesi/sınıfı, etkilenen etkileşimler ve sonuç gerekçesi görünür; gerekli tahliye/iptal ve readiness tamamlanmadan retire uygulanmaz. Bütün şehri tek transaction'a sıkıştırmak yerine her bounded grup yeniden doğrulanır. Kapatma ile sahiplenme yarışırsa aggregate rezerv/kabul sırası karar verir; stale cleanup sahiplenilmiş canlıyı silemez. Resource stop ve population off ayrı işlemlerdir.

## Kara trafiği ve hava sahası

RoadTrafficProvider; şerit grafiği, yön/hız sınırı, kavşak conflict zone'ları, sinyal planı, geçiş rezervasyonu ve takip davranışını kullanır. TrafficSignalState faz, server başlangıç tick'i ve süreyi taşır. Client rastgele başka ışık fazıyla AI kararı üretmez. Oyuncunun kırmızıda geçmesi fiziksel olarak mümkündür; AI buna güncel engel/temas bilgisiyle tepki verir. Rezervasyon kaza olmayacağının garantisi değildir. Tıkanmada bounded replan, bekleme ve stuck raporu vardır; görünür aracı ileri ışınlamak varsayılan çözüm değildir.

AirTrafficProvider, uçuş koridoru ve irtifa katmanına ek olarak kalkış/iniş alanı, yaklaşma, pist/helipad rezervasyonu, holding ve abort durumlarını tanımlar. Uçak ve helikopter farklı LocomotionProfile kullanır. Seyir, kalkış, iniş, hasarlı uçuş ve autorotation gibi ileri davranışlar ayrı capability'dir; biri geçince hepsi açılmaz. Kuyruk tıkanırsa yeni kalkış ertelenir; desteklenmeyen acil durum davranışı varmış gibi gösterilmez.

Hızlı hava aracının AOI/prefetch alanı hız, etkileşim menzili ve ölçülen hazırlık süresine bağlı ilerletilir; yerdeki sabit yarıçap otomatik yeterli sayılmaz. Çarpışabilir uçak yalnız dekoratif uzak efektle temsil edilerek etkileşime açılmaz. Pilot/araç birlikte devredilir; owner kaybındaki güvenli davranış [agent sözleşmesindeki](agents-navigation.md) test edilmiş controller fallback'ine bağlıdır.

## Kalıcılık ve toparlanma

Ambient nüfus varsayılan `none` olabilir; canlı baseline yine gerçek state'i taşır. Restart'ta aynı NPC bireyinin dönmesi yalnız seçilmiş persistence politikasıyla mümkündür. Kalıcı takvim/slot/cooldown ve PolicyId kayıtları restore edilir; eski runtime owner/koltuk referansları yeni EntityRef'lere çözülür. Persist edilmiş spawn ledger authoritative kayıttır; eski seed'i yeniden çalıştırıp ikinci canlı/ödül üretmek yasaktır.

Kalıcı sahiplik devrinde gereken dünya/domain kaydı ve slot disposition aynı backend transaction veya açık outbox akışına bağlanır. Tombstone/cooldown saklama süresi duplication penceresini kapsar. Slot, kullanılan araç uzaklaştı diye aynı bireyin yeni kopyasını üretmez; replacement ancak yeni SpawnGeneration ve izinli refill politikasıyla oluşur. Population provider yoksa native varsayılan trafiği otomatik açmak fallback değildir.

## Hata, genişleme ve kabul

Hatalar `policy_conflict`, `population_disabled`, `spawn_reservation_lost`, `population_budget_exceeded`, `protected_entity`, `route_unavailable`, `controller_unavailable`, `asset_not_ready` olarak ayrılır. Log; policy/slot/candidate, cause/transaction, görev, entity/owner epoch, kota ve bekleme nedenini taşır. Yeni sağlayıcı aynı typed PopulationService/AgentService sözleşmelerine uyar; farklı protokol açmaz.

AC-46–55, AC-63/68/70; [senaryolar](../validation/scenarios.md), [yük profilleri](../validation/performance.md), R-17/18. Native stok nüfus ve kara trafiği önce, hava kabiliyetleri ayrı kanıtla açılır. Bu PopulationService sözleşmesi tasarımdır; [D1 foundation](../development/status.md) kodunda henüz nüfus/AI controller implementasyonu yoktur.

## Zaman, rezervasyon ve arıza bütçesi

Gün/saat yoğunluk programı world'ün SimulationTime takvimini, oyun içi spawn cooldown'ı açıkça ilan edilen oyun zamanını kullanır. Kontrol lease'i, hazırlama rezervi ve drain deadline'ı monoton ControlTime'dır; pause/resume veya UTC değişimi gizlice hak uzatmaz. Kalıcı offline refill/yaşam açılmışsa son işlenmiş pencere ve bounded UTC politikası ayrıca gerekir. [Saat sözleşmesi](../architecture/time-fencing.md), AC-71/72/84.

Spawn/transfer rezervi TTL ile temizlenirken operation phase denetlenir. Kesin precommit aday bırakılabilir; CommitSubmitted/OutcomeUnknown/Committed kayıt recovery coordinator'a gider. Restart'ta protected occupancy, pending transfer ve slot ledger'ı kotayı yeniden kurar. Aşırı yükte yeni ambient admission azaltılır; owned pet/kullanılan araç silinmez. [Kurtarma](../architecture/failure-recovery.md) ve [transfer](../networking/consistency-recovery.md), AC-74/83/87/88.
