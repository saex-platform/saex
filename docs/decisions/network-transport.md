# Ağ taşıması kararı: GameNetworkingSockets

Durum: **kabul edilmiş tasarım kararı, ADR-07'nin ayrıntısı; v0.2, 12 Eylül 2026.** [Protokol](../networking/protocol.md) · [MTA incelemesi](../references/mtasa-architecture.md)

## Kesin karar ve sorumluluk

SAEX v1 oyun bağlantısı **GameNetworkingSockets (GNS) üzerinden doğrudan istemci–sunucu UDP bağlantısı** kullanacak. **ENet v1 bağımlılığı, ikinci taşıma sağlayıcısı veya otomatik fallback olmayacak.** Asset dağıtımı HTTPS üzerinden ayrı kalacak. Steam hesabı, Steam Datagram Relay veya merkezi bir SAEX hesabı zorunlu değildir. Voice paketleri GNS üzerinde ayrı mantıksal akış kullanır; codec seçimi R-09 kapsamındadır.

Bu karar kütüphane seçimini kapatır; GNS build'i, standalone sunucu kimliği entegrasyonu ve SAEX codec doğrulaması hâlâ R-11 uygulama kapısıdır. Başarısız bir deney sessizce ENet'e geçilmesine izin vermez: kanıt, etki ve yeni ADR gerekir. Kütüphanenin incelenen commit'i bir kaynak referansıdır; henüz oluşturulmamış üretim dependency lock'u değildir.

Taşıma yalnız mesaj teslimini sağlar. Entity, snapshot/delta, state revision, AOI, izin, kalıcılık, hasar ve hile doğrulaması SAEX core sorumluluğudur. GNS de bu üst düzey oyun sistemlerini sağlamadığını açıklar. [GNS kapsamı](https://github.com/ValveSoftware/GameNetworkingSockets/blob/a424b7db649438acafb60c99cae6667587c42732/README.md)

## ENet ile karşılaştırma

| Ölçüt | GNS | ENet | SAEX kararı üzerindeki etkisi |
|---|---|---|---|
| Reliable / unreliable mesaj | Var | Var | İkisi de temel oyun paketlerini taşıyabilir |
| Bağımsız akış | Lane ve lane başına reliable sıralama | Bağımsız sıralı channel | Bir kanaldaki gecikmenin diğerini engellemesini ikisi de azaltır |
| Trafik planlama | Lane priority ve aynı priority içinde ağırlık | Channel ayrımı, bandwidth/throttle mekanizmaları | SAEX sınıflarını GNS lane ağırlıklarına eşlemek daha doğrudan |
| Şifreleme ve kimlik | Yerleşik şifreleme ve sertifika arayüzleri; endpoint güveni ayrıca kurulmalı | İncelenen genel API'de yerleşik sertifika/kimliği doğrulanmış şifreleme yüzeyi yok | ENet seçimi ayrıca güvenli kanal entegrasyonu gerektirir |
| Bağımlılık ve sadelik | Daha geniş bağımlılık ve yapılandırma yüzeyi | Daha küçük C kütüphanesi | GNS bakım/build maliyeti kabul ediliyor |
| Oyun state'i / otorite | SAEX tasarlamalı | SAEX tasarlamalı | Taşıma seçimi dünya tutarlılığını kendiliğinden çözmez |
| Gerçek SAEX performansı | Ölçülmedi | Ölçülmedi | Hız veya oyuncu kapasitesi üstünlüğü iddiası yok |

ENet'in channel, ACK/retry, fragmentation ve congestion yaklaşımı incelenmiştir; onu yalnız “ham UDP” saymak yanlış olur. [ENet tasarım belgesi](https://github.com/lsalzman/enet/blob/5a9c537fd464b3c6d3c55e1d3bd47588faf71b42/docs/design.dox), [ENet genel API](https://github.com/lsalzman/enet/blob/5a9c537fd464b3c6d3c55e1d3bd47588faf71b42/include/enet/enet.h). GNS'nin lane öncelik/ağırlık sözleşmesi seçimdeki somut avantajdır. [ConfigureConnectionLanes](https://github.com/ValveSoftware/GameNetworkingSockets/blob/a424b7db649438acafb60c99cae6667587c42732/include/steam/isteamnetworkingsockets.h#L419-L486)

## Süreç ve veri akışı

Client resource intent → izinli ClientHost broker → SAEX codec → GNS bağlantısı → ServerCore giriş kuyruğu → kimlik/şema/otorite kontrolü → gameplay transaction → filtrelenmiş state → GNS → ClientHost staging → x86 adapter apply.

GNS client host'ta, server tarafı ağ servisi içinde yer alır. GTA ana thread'i socket polling, bağlantı callback'i, sıkıştırma veya indirme beklemez. I/O thread'i GTA pointer'ına erişmez. Transport callback'leri bounded kuyruk üzerinden core'a aktarılır; callback içinde script veya kalıcı DB işlemi çalıştırılmaz.

TransportAdapter taslağı yalnız `Connect`, `Accept`, `Close`, `SendMessage`, `PollReceived`, `GetMetrics` ve lane yapılandırmasını kapsar. GNS handle veya enum'ları C# SDK ve kalıcı veri biçimine sızmaz. Bu sınır v1'de birden fazla transport implementasyonu yazma taahhüdü değildir.

## Lane eşlemesi ve nedensel sıralama

Aşağıdaki değerler ilk ölçüm profilidir; üretimde doğrulanmış throughput değildir. Her iki uç kendi gönderim yönü için aynı profili kurar.

| Lane | Trafik | GNS priority / weight | SAEX davranışı |
|---|---|---|---|
| 0 | Kimlik, izin, lease, geçiş kontrolü | 0 / 1 | Reliable, sıkı oran sınırı; küçük kontrol rezervi |
| 1 | Spawn, retire ve world transaction | 10 / 8 | Reliable; ScopeEpoch ve ViewSequence |
| 2 | Motion ve physics samples | 10 / 8 | Unreliable; yalnız en yeni geçerli örnek |
| 3 | Voice | 10 / 4 | Unreliable; geç kalmış ses düşer |
| 4 | İlan edilmiş remote resource mesajı | 10 / 2 | Şemanın seçtiği reliability; byte/çağrı kotası |
| 5 | Baseline sayfaları | 10 / 1 | Reliable, sayfalama ve backpressure |
| 6 | Opsiyonel telemetry | 20 / 1 | Best effort; açlıkta düşürülebilir |

GNS'de küçük priority sayısı önce gelir; farklı priority sınıfları katı önceliklidir. Aynı priority içindeki weight yalnız gönderim paylaşımıdır. Reliable sıralama **yalnız aynı lane içinde** güvence altındadır; lane'ler arası varış sırası garanti edilmez. Bu nedenle kontrol kotası gereklidir ve baseline için ağırlıklı pay bırakılmıştır. [GNS sıralama sözleşmesi](https://github.com/ValveSoftware/GameNetworkingSockets/blob/a424b7db649438acafb60c99cae6667587c42732/include/steam/isteamnetworkingsockets.h#L419-L486)

`CatalogCommit` önce gönderilmiş diye motion'ın ondan sonra geleceği varsayılmaz. Motion ve resource event'i ilgili `TransitionId`, `CatalogRevision`, `EntityRef`, `requiresStateRevision` ve owner/scope epoch koşulları sağlanmadan uygulanmaz. Bağımlı reliable event bounded bekletilir; motion için entity başına en yeni örnek tutulur. Global world revision'a göre private veri beklenmez. Ayrıntı: [ortak aktivasyon sözleşmesi](../architecture/activation-contracts.md).

Toplam pending reliable limiti bağlantı başına 4 MiB'dir; bunun 64 KiB'ı kontrol için ayrılır, diğer reliable akışlar toplam en çok 4032 KiB kullanır. Bu yalnız çıkış reliable kuyruğudur; baseline staging ve giriş kuyrukları ayrıca bütçelenir. Resource başına rate limit ve oturum toplamı birlikte uygulanır. Kontrol taşması bağlantı sağlığı hatasıdır; sınırsız reliable kuyrukla gizlenmez.

## Şifreleme, endpoint kimliği ve açık yayın kapısı

GNS'nin varsayılan şifrelemesi tek başına karşı uç kimliğini doğrulamaz. Resmî header, sertifika veya başka kanaldan dağıtılmış uygun güven bilgisi olmadan aradaki saldırgana karşı kimlik güvencesi bulunmadığını belirtir. `ConnectByIPAddress` başarısı, SAEX `Authenticated` durumu değildir. [Bağlantı güveni](https://github.com/ValveSoftware/GameNetworkingSockets/blob/a424b7db649438acafb60c99cae6667587c42732/include/steam/isteamnetworkingsockets.h#L67-L84)

HTTPS bootstrap endpoint ve beklenen server kimliğini keşfetmek içindir. Sıradan HTTPS sertifikasını GNS'ye vermenin yeterli olduğu, `SetCertificate` çağrısının bütün güven politikasını tamamladığı veya belgelenmemiş bir PSK API'si bulunduğu varsayılmaz. R-11a; seçilen GNS sürümünün desteklediği sertifika/güven kökü yolunu, self-hosted server provisioning ve iptal/anahtar yenilemesini somutlaştıracak. Entegrasyon ek adapter ya da upstream değişikliği gerektirirse ayrıca kayda alınacak; özel kriptografik protokol tasarlanmayacak.

Üretim akışı: güvenilen server kimliğini çöz → GNS peer kimliğini o kimliğe kriptografik olarak bağla ve doğrula → uygulama Hello/capability → account/guest session challenge → asset readiness → baseline → oyun katılımı. Uygulama token'ı kimliği doğrulanmamış oyun bağlantısına gönderilmez. Oyuncu kimliği ve resource yetkisi bu kontrolden ayrı kalır.

**R-11a geçmeden dış kullanıma açık üretim katılımı kapalıdır.** Yerel geliştirme modu ancak açıkça işaretlenmiş test profiliyle çalışabilir; üretimde doğrulama başarısızlığını “şifreli zaten” gerekçesiyle kabul eden fallback yoktur. Bu açıklık transport seçiminin açık bırakılması değil, seçimin güvenli uygulanmasının henüz kanıtlanmamış olmasıdır.

## Hata, genişleme ve kabul

Disconnect/reconnect yeni session epoch üretir; GNS güvenilir teslimi process crash sonrasında idempotency sağlamaz. Durable tekrarlar [OperationId](../glossary.md) ve journal receipt ile çözülür. Mesaj parse hatası resource callback'ine ulaşmaz. Loss/bandwidth baskısında motion coalesce, baseline pacing ve kota uygulanır; kritik state düşürülüp başarılı görünülmez.

R-11a kimlik/yanlış anahtar/yenileme, R-11b hedef platform build ve lane basıncı, R-11c bounded codec/fuzz kabulü ayrı raporlanır. AC-25 çapraz lane sırası, AC-26 reconnect idempotency, AC-32 karşı uç kimliği ve AC-33 kuyruk basıncı bu kararı sınar. Hiçbiri bu doküman çalışmasında yürütülmedi.

## Population ve hayvan yükünün taşıma kararıyla ilişkisi

V0.4 [Population/Agent/Animals](../gameplay/population-traffic.md) trafiği mevcut GNS lane/queue/identity sözleşmesini kullanır; ENet veya ikinci oyun transport'u eklenmez. Policy/task/koltuk/life-state reliable transaction, motion sampled unreliable, görsel cue ilan edilmiş event yolundadır. Rig/model/IFP aynı HTTPS artifact hattından gelir.

Çok sayıda NPC, bütün davranış ağacını veya her kemiği her tick ağdan göndermeyi gerektirmez. Private blackboard/seed server'da kalır; gereken task/animation state ve motion filtrelenir. Mixed-load kabulü mevcut kontrol rezervi ve N1/N2 sınırlarında AC-50/52/65/70 ile genişler; daha çok entity sınıfı yeni ölçüm olmadan kapasiteyi artırmaz.
