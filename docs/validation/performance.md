# Performans profilleri ve ölçüm hedefleri

Durum: ölçüm şartnamesi. **Hiçbir kapasite veya gecikme sonucu ölçülmüş değildir.** [Yol haritası](../roadmap.md)

## Yük tanımı

Bağlantı sayısı, authoritative entity sayısı, ilgili entity sayısı ve client'ta materialize edilen native nesne sayısı ayrı sayılır. Tek sahnede yoğun fizik ile dağınık oyuncu workload'u aynı benchmark değildir.

| Profil | Bağlantı | Test edilen yerel yoğunluk | Aktif yük |
|---|---|---|---|
| P0 | 2 aktif + 1 late join | Tek test alanı | Beş yıkım prefabı, 16 dynamic body, temel hareket |
| P1-A | 64 | Yakın alanda 32 oyuncu | 24 araç, 32 ayrıntılı NPC, 64 dynamic prop/body |
| P1-B | 128 | Yakın alanda 64 oyuncu | 32 araç, 64 ayrıntılı NPC, 128 dynamic prop/body |
| P2-A | 256 | Yakın alanda 64 oyuncu | P1-B yerel yoğunluğu + dağınık bölgeler |
| P2-B | 512 | Yakın alanda 128 oyuncu stres deneyi | 64 araç, 64 ayrıntılı NPC, 128 dynamic prop/body |

Sayılar ayrı kategorilerin toplam yüküdür; araç veya NPC aynı prop sayacına tekrar eklenmez. NPC toplamı AI yaya, sürücü, pilot, yolcu ve hayvan bireylerini kapsar; oyuncular ayrıca sayılır. Araç sayısı sürücüyü içermez, her biri kendi native havuz/bütçesini tüketir. Uzak kayıtlı dünya büyüklüğü P1 için 10.000, P2 için 50.000 placement hedefidir; tamamı active dynamic body değildir. Gameplay parçaları prop/body bütçesine dahildir.

P2-B yerel yoğunluğu deneydir, destek vaadi değildir. Client havuzu aşılırsa yeni admission/aktif entity sınırı uygulanır ve hedef karşılanmadı raporlanır; collider'ları rastgele düşürerek “başarı” üretilmez.

## V0.4 yaşayan dünya yük karışımları

| Profil varyantı | Araç toplamının dağılımı | NPC toplamının dağılımı |
|---|---|---|
| P1-A living | 14 road + 10 oyuncu/görev = 24 | 14 AI sürücü + 10 yaya + 8 hayvan = 32 |
| P1-B living | 20 road + 2 air + 10 oyuncu/görev = 32 | 20 AI sürücü + 2 pilot + 18 yaya + 24 hayvan = 64 |
| P2-A living | P1-B yerel karışımı; uzak bölgeler ayrıca kayıtlı | P1-B yerel karışımı; server abstract nüfus ayrıca raporlu |
| P2-B living | 32 road + 4 air + 28 oyuncu/görev = 64 | 32 AI sürücü + 4 pilot + 12 yaya + 16 hayvan = 64 |

Bu fixture'larda oyuncu/görev araçlarına ek AI sürücü/yolcu konmaz; eklenirse NPC toplamı yeniden dağıtılır. Hayvan custom controller kullanıyorsa actual native ped/object/body havuz maliyeti ayrıca yazılır; mantıksal NPC sayacı fizik bütçesini gizlemez. Hava capability yoksa air satırı passed sayılmaz; açıkça etiketlenmiş ground-only varyant ayrı koşulur. Sayılar destek vaadi değildir, eski toplamlar üstüne gizli ek yük bindirilmez.

İlk Agent decision hedefleri interactive 10 Hz'e kadar, reduced 2 Hz, abstract 0,2 Hz'dir. 30 Hz logic/20 Hz motion hedefi korunur; decision frekansı fizik/animasyon FPS'i değildir. Algı event'leri, path işi ve sürü kararları da toplam execution kotasındadır. P0-agent fixture'ı iki aktif client + late join ile bir yaya, bir sürücülü araç ve bir ground creature kullanır; flight kabulü ayrı P0-air fixture'ında yapılır.

Ek metrikler: policy accepted→drain completion/protected count, duplicate slot sayısı, perception/query p95/p99, task deadline/cancel, stale nav, tier wake süresi, model/rig/clip çeşitliliği, attack query farkı, group fan-out, owner devir boşluğu ve offline catch-up backlog. Policy-off sonrası kabul edilmiş stale spawn, aynı bireyin iki dünyada aktifliği ve duplicate durable ödül/doğum için tolerans sıfırdır. Hareket/animasyon yakınsaması mevcut N1 state hedefi ile native görsel/contact ölçümlerinde ayrı raporlanır. AC-46–70; R-17–20.

## Ortam kaydı

Sunucu için ilk referans sınıfı 8 fiziksel x64 CPU çekirdeği, 32 GiB RAM ve SSD; client için 6 çekirdek CPU, 16 GiB RAM, 4 GiB GPU ve 1080p hedefidir. Bunlar kullanıcının mevcut cihazının ölçümü veya belirli bir işlemcinin performans iddiası değildir.

Her rapor exact CPU/GPU modelini, işletim sistemi, sürücü, saat/güç profili, build, engine/catalog fingerprint ve thermal durumunu kaydeder. Karşılaştırmalı koşular aynı donanımda yapılır. GPU'suz bot testi client performansı diye sunulmaz.

## Ağ profilleri

| Profil | RTT | Jitter | Paket kaybı | Amaç |
|---|---|---|---|---|
| N0 | 0–10 ms ölçülen LAN | Gerçek ortam | Enjekte edilmez | Temel correctness |
| N1 | 100 ms | ±20 ms | %3 bağımsız kayıp | Tipik bozulma altında yakınsama |
| N2 | 200 ms | ±40 ms | %5 + 1 saniyelik kopma | Recovery ve graceful degradation |

Jitter ve kayıp seed'i sabitlenir. Bant daraltma, asset download ve voice açık/kapalı koşuları ayrı çalıştırılır. Client readiness beyanının hileye dayanıklılığı hareket doğrulamasının yerine geçmez.

## İlk hedef eşikleri

| Metrik | Hedef | Ölçüm kapsamı |
|---|---|---|
| World logic tick | 30 Hz; p95 ≤25 ms, p99 ≤33,3 ms | Server core, physics/commit işi görünür |
| Server physics | Destekli solver'da 60 Hz alt adım | Native client frame'iyle eşit varsayılmaz |
| High priority motion | 20 Hz yayın hedefi | Queue/bandwidth altında effective rate ayrıca |
| Client render | 60 FPS hedef; frame p95 ≤20 ms | P1, warm cache, kaydedilmiş grafik ayarı |
| Streaming apply | Frame başına 2 ms hedef | Native bind/upload ayrı span |
| State yakınsama | N1'de server commit→ilgili client apply p95 ≤300 ms | Asset hazır accepted kritik state |
| Katılım | Warm cache, ≤1 MiB baseline için ≤5 s hedef | İndirme ve auth dışındaki world sync |
| State trafiği | Client başına ortalama ≤128 KiB/s down, ≤32 KiB/s up | Voice/asset/debug hariç; overhead ayrıca |
| Uzun koşu | 8 saat, ilk saat sonrası unexplained RAM/pool trendi yok | 100 reload ve 100 streaming döngüsü |
| Durable recovery | Onaylanmış durable transaction kaybı 0 | Doğrulanmış backend ile process crash; disk/host kaybı için ayrı RPO/RTO |

Tick süresi DB I/O bekleyişini içermez; async durable commit gecikmesi ve pending aggregate kuyruğu ayrı ölçülür. WorldRevision core apply sırası, JournalSequence disk sırasıdır. Rapor birini diğerinin latency ölçümü yerine koymaz.

Ek v0.2 ölçümleri: lane başına queue byte/age ve baseline ilerlemesi; candidate/owner churn; stream-in/out sayısı; iptal edilmiş late-load sonucu; retiring handle ve pinli asset sayısı; worker actual/reserved bellek ve reload tepe tüketimi. Lease 2 s/500 ms, owner minimum tutma 2 s ve aday iyileşme 20 m/500 ms ilk sabit deney profilidir. AOI giriş/çıkış eşik farkı 50 m ve minimum tutma 2 s'dir; yetki/world kaybı beklemez. [Otorite](../networking/authority.md), [streaming](../assets/streaming-budgets.md), AC-27–33.

Bu eşikler ilk kapı hedefidir, SLA değildir. Voice egress'i alıcı sayısıyla büyür ve ayrıca ölçülür. Örneğin 512 × 128 KiB/s yaklaşık 64 MiB/s uygulama egress'idir; transport/voice/CDN dahil değildir.

## Yük azaltma sırası

Telemetry örnekleme → opsiyonel efekt/texture kalite → uzak motion frekansı → uzak AI ayrıntısı → yeni noncritical entity kabulü → yeni oyuncu admission sınırı. Mevcut kritik collision veya durable sonuç client bazında düşürülmez.

Server-simulated ile validated-native modları ayrı grafiklerde raporlanır. Sonuç iyi görünsün diye bir koşuda solver değiştirilip aynı profil etiketi kullanılmaz. GC, IPC ve asset download maliyetleri ölçümden gizlenmez.

## V0.5 arıza yükü ve kurtarma ölçümü

F0 sağlıklı kontrol koşusu; F1 worker/owner kaybı ve gecikmiş completion; F2 DB commit cevabı kaybı/ownership erişim kesintisi; F3 clock stall/suspend; F4 baseline corruption/slow peer; F5 retained backup restore olarak etiketlenir. Her koşu fault başlangıç/bitişini, seed'ini, etkilenen scope'u ve recovery profile digest'ini kaydeder. Önce tek arızalar, sonra AC-88'de tanımlı birleşimler çalıştırılır; tüm olası arızalar test edilmiş sayılmaz.

| Metrik | İlk geçiş koşulu | Ölçüm sınırı |
|---|---|---|
| Tutarlılık ihlali | Duplicate durable etki/birey, iki writer ve stale yetkili hit: 0 | AC-71–88 invariant'ları; native manipulation güven sınırı ayrıca |
| Bounded kaynak | Byte/item/age ve process toplamı RecoveryProfile tavanını aşmaz | Core registry, staging, restart, AI ve durable completion birlikte |
| Lease / restart | 2 s/500 ms control lease; resource için 3 restart/5 dk bütçesi | [Saat](../architecture/time-fencing.md), [supervisor](../architecture/failure-recovery.md); OS dururken çalışma vaadi yok |
| Sync denemesi | 30 s/deneme; 60 s'de en çok 2 otomatik yeniden hazırlama | Asset download dışarıda; warm join 5 s hedefi ayrı |
| Kurtarma süresi | p50/p95/p99 ve en kötü değer raporlanır; seçilen deadline sonrası güvenli failed/fenced | Arıza sürerken otomatik başarı beklenmez; yeniden açılan scope ayrıca ölçülür |
| Backup geri dönüş | Doğrulanmış restore closure; failure envelope başına açıklanmış RPO/RTO | Process crash ile disk/host kaybı aynı sonuç etiketi almaz |

Fault sırasında kasıtlı fence/unavailable süresi, gecikme metriğinden çıkarılıp saklanmaz; availability ölçümüne girer. Sağlıklı N1 p95 ≤300 ms yakınsama hedefi, DB'nin erişilemediği süreye uygulanmış SLA gibi sunulmaz. Fault kaldırıldıktan sonra backlog drain, lease yeniden verme, baseline tamamlama ve bellek/pool tabanına dönüş ayrı zamanlanır. Açıklanamayan monoton büyüme başarısızlıktır. R-21–23 ve AC-88 gerçek client/backend koşulları olmadan kapanmaz.

## Tekrarlanabilir ölçüm

V0.3 profilinde plan resolve/build süresi ve tepe belleği, schema decode/validation maliyeti, tick phase süreleri, job deadline miss, quota ret, stale projection yaşı, pending action sayısı ve rehearsal overhead ayrıca ölçülür. İlk referans plan yükü 32 resource tanımı, 256 prefab ve 10.000 placement'tır; bunlar eşzamanlı client worker veya active native entity sayıları değildir. Mevcut worker/admission sınırları geçerlidir. İlk test fixture'ı 10 s plan çözümleme ve 512 MiB tool bellek hedefi kullanır; gameplay tick SLA'sı değildir.

World projection için 500 ms nav-cache ve 250 ms kozmetik akustik hedefleri [projection belgesindeki](../networking/world-change-projections.md) ilk deney değerleridir. Süre aşımı stale veriyi geçerli kılmaz. Actions/Effects ve conformance araçları kendi p95/p99 maliyetiyle raporlanır. AC-36/40/45 scheduler/projection açlığını denetler.

Warm-up 10 dakika, en az üç 30 dakikalık koşu ve ayrı 8 saat soak yapılır. p50/p95/p99, hata sayısı, dropped events, scope baseline süreleri ve queue peak raporlanır. Yeni değişiklik yokken sonsuz tekrar yerine gerekli kapı sonuçları ve kalan riskler yayımlanır.
