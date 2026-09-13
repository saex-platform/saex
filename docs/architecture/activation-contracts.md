# Ortak işlem, revision ve aktivasyon sözleşmesi

Durum: normatif sözleşme taslağı, ADR-17. [Terimler](../glossary.md) · [Resource yaşam döngüsü](../resources/lifecycle-hot-reload.md) · [Replikasyon](../networking/replication.md)

## Sorumluluk ve sınır

Bu belge; resource reload, katalog geçişi, world geçişi, baseline ve yıkım işlemlerinde ortak olan kimlik, sıra ve hazırlık kurallarının tek tanımıdır. ServerCore geçerli gameplay state'ini belirler; persistence servisi durable commit'i; ClientHost ve GTA adapter yerel hazır olma/uygulama durumunu yönetir. Bütün makinelerin aynı duvar saati anında değişmesi vaat edilmez.

## Kimlikler ve revision alanları

| Alan | Kapsam / artış | Kullanım |
|---|---|---|
| RequestId | SessionEpoch + ResourceEpoch + çağrı kimliği | Tek bağlantı ömründeki istek/cevap korelasyonu |
| OperationId | Kalıcı domain + yetkilendirilmiş actor + işlem kimliği | Reconnect/restart aşan idempotency; RequestId değişebilir |
| TransactionId | Sunucunun tek kabul işlemi | State değişikliklerini ve receipt'i birleştirir |
| WorldRevision | WorldEpoch içinde core'un yayımladığı her state transaction'ında artar | Canlı tutarlı snapshot kesiti; sampled motion hariç |
| JournalSequence | Kalıcı world namespace içindeki committed kayıt sırası | Restore/compaction; her runtime state değişimi journal'a girmez |
| StateRevision | Entity aggregate'ının state değişiminde artar | CAS ve entity'ye bağımlı event/motion bariyeri |
| ViewSequence | ScopeEpoch içinde filtrelenmiş reliable transaction sırası | Client'ın gerçekten alması gereken kayıtlar arasındaki boşluk |
| MotionSequence | Entity/owner epoch örnek akışı | Eski unreliable örneği atma; state transaction değildir |
| TransitionId | Bir hazırlama/aktivasyon denemesi | Ready/commit/abort/ACK'ın yanlış denemeye bağlanmasını engeller |

WorldRevision ile JournalSequence eşit değildir. `none` state değişimi canlı snapshot'ta bulunabilir, disk journal'ında bulunmayabilir. Canlı snapshot kesiti `(WorldEpoch, WorldRevision, ServerTick)` ve o tick'te son kabul edilmiş motion örneklerini içerir. Disk snapshot sınırı `JournalSequence`'dır; yalnız saklanan component'ler vardır. Restart yeni WorldEpoch açar; eski runtime revision'lar yeni oturumda karşılaştırılmaz.

## Durable işlem akışı

1. Session/resource kimliğini ve işlem yapma yetkisini doğrula. `OperationId` başka actor'un receipt'ini okumaya yetki vermez.
2. `(domain, actor, OperationId)` için receipt veya in-flight kayıt ara. Aynı kimlik ve aynı kanonik payload özeti önceki sonucu döndürür; farklı payload `operation_conflict` verir.
3. Aggregate'lar üzerinde beklenen StateRevision ve gameplay önkoşullarını kontrol et; etkilenen aggregate'ları pending işlem için ayır. Aynı aggregate mutasyonları sıraya girer; ilgisiz entity'ler çalışabilir.
4. Kalıcı state, receipt, outbox ve JournalSequence kaydını tek backend transaction'ında commit et. I/O async'tir; GTA frame'i ve server tick'i DB cevabında bloklanmaz.
5. Core, commit tamamlanmasını güvenli tick noktasında işler; canlı state'i yayımlarken WorldRevision verir, sonra ACK/olay/replication üretir. Journal'ın kalıcı sonucu TransactionId ve JournalSequence ile tanınır; sonradan verilen runtime WorldRevision'ın disk receipt'te bulunması şart değildir.
6. DB commit olup core apply/ACK öncesi crash olursa restore journal'dan sonucu kurar. Yeniden gelen OperationId yeni hasar/ödül oluşturmaz; sonuç yeni runtime referanslarına çözülür.

Durable operasyonun server-generated hasarı varsa OperationId güvenilen cause zincirinden üretilir. Yeni OperationId oluşturarak tekrar ödül istemek gameplay kurallarını aşamaz: cooldown, tek-use action token veya domain unique kuralı ayrıca gerekir. Guest reconnect kimliği kurtarılamıyorsa eski operasyonları yeni guest actor'a mal etmek yerine belirsiz sonuç durumu sunulur.

Receipt saklama politikası operasyonun yeniden oynatılabilir ömrünü kapsar. Silinen receipt'in yerine kalıcı domain uniqueness kaydı veya eski operasyon aralığını reddeden watermark gerekir. Süresi dolmuş bilinmeyen işlem otomatik yeni işlem sayılmaz. [Kalıcılık](../networking/persistence.md)

## Ortak geçiş aşamaları

| Aşama | Önkoşul ve işlem | Hata sonucu |
|---|---|---|
| Plan | Etkilenen entity/resource grubu, eski/yeni catalog, migration ve katılımcı listesi | Uyumsuz değişiklik restart sınıfına alınır |
| Prepare | Yeni worker, asset closure ve server collider; client'larda görünmeyen staging; bütçe rezervasyonu | Eski sürüm çalışır, yeni kaynaklar bırakılır |
| Quiesce / revalidate | Etkilenen mutasyonlar ve eski owner lease'leri durdurulur; işler drain olur; committed sonuçlar core'a uygulanır, son state taşınır | Kesin commit submission öncesi timeout abort; belirsiz durable sonuçta fence/reconcile; stale migration yeniden hazırlanır |
| Ready barrier | Katılımcılar TransitionId, catalog/lock, resource epoch ve dependency digest ile hazır olduğunu bildirir | Hazır olmayan oyuncu fence edilir veya işlem iptal edilir |
| Commit | Gerekirse durable backend commit; ardından core routing/state/epoch değişimini tek mantıksal kararla yayımlar | Kesin rollback'te eski sürüm; cevap kaybında OutcomeUnknown ve fence; committed sonuç ters migration olmadan silinmez |
| Apply / ACK | Her client bounded native swap yapar, `Applied(TransitionId, revision)` döner | O client etkileşime kapalı staging/resync durumunda tutulur |
| Retire | Yeni state'i kullanan katılımcılar aktive edilir; eski ref/fence bitince kaynak bırakılır | Referansı süren eski artifact tutulur; lease sızıntısı raporlanır |

Ready bildirimi istemcinin dürüst olduğunu kanıtlamaz; otorite kontrolleri devam eder. Katılımcı listesi kapanınca yeni girişler bekletilir veya yeni katalog için ayrı staging'e alınır. Timeout'ta katılımcıyı listeden çıkarmak önce server tarafında input/lease/hit etkileşimlerini fence etmeyi ve scope'unu geçersiz kılmayı gerektirir. Salt listeden silip eski collision ile oynatmak geçerli değildir.

Commit sonrası her istemci aynı anda ACK vermeyebilir. Henüz apply etmemiş oyuncu hedef sahnede hareket/hasar/owner sonucu üretemez ve eski collision'la etkin görünmez; yükleme/geçiş görünümünde kalır. Client'ın prepare sonrasında çökmesi bütün durable dünyayı geriye döndürmez. Yetkili yeni katalogla resync veya açıklamalı bağlantı kapanışı uygulanır. Etkilenen fizik grubu serbest bırakılırken hâlâ fence edilmiş oyuncular simülasyon etkileşiminden ayrılmış olmalıdır.

## Baseline ve kare bütçesinde atomiklik

Baseline bütün dünyayı tek karede GTA'ya oluşturma talimatı değildir. Mantıksal görüntü ve dependency closure doğrulanır; native binding'ler çok kare boyunca staging'de hazırlanır. Staging entity'si render, collision, input, ses ve script gameplay callback'i üretmez. `BaselineApplied`, bu noktada mantıksal baseline'ın hazırlanmış yerel kopyasını bildirir; oyuncunun dünyada aktif olduğunu bildirmez.

Catch-up sonrasında `JoinCommit` hedef ViewSequence ve katalog sınırını belirtir. Client bu sınıra kadar state'i kurar, ilk etkileşim bölgesinin zorunlu native closure'ını hazırlar, aktivasyonu uygular ve `JoinApplied` yollar. Sunucu ancak bu ACK doğrulandıktan sonra oyuncu komutlarını açar. Sonraki state değişimleri normal sıralı akışa geçer; ACK gecikmesi eski world state'ini kalıcı dondurmaz.

Tek destruction transaction'ı bütün parça/collider değişimleriyle bir bounded aggregate olarak uygulanır. Gerekli hazırlık önce yapılır; güvenli native swap noktası frame bütçesine sığmalıdır. Bir sahnenin tamamı bu küçük transaction sınırına zorlanmaz. Adapter aynı karede görünürlük/collision swap'ını güvenli sağlayamıyorsa etkilenen alan kısa geçiş durumunda etkileşime kapatılır; capability testi geçmeden kesintisiz apply desteği ilan edilmez. Fazla büyük tek prefab build'de reddedilir.

## Akışlar arası bağımlılık ve callback düzeni

Spawn/state bir reliable lane, motion başka lane, script event başka lane üzerinden gelebilir. Entity bilinmiyorsa event uygulamaya çağrılamaz. Her bağımlı mesajın ilgili entity StateRevision'ı, catalog/transition ve owner/scope epoch'u doğrulanır. Motion için en yeni geçerli aday tutulur; retire/generation değişiminde silinir. Bağımlı event kuyruğu scope başına en çok 256 kayıt/256 KiB ve en çok 2 saniye bekler; limit veya timeout'ta resync/ret, sonsuz buffering yoktur. Bu rakamlar R-11 ölçüm hedefidir.

Event'ler kayıtlı component/event şeması üzerinden filtrelenir. Resource'a teslim, entity görünümü kurulduktan ve resource epoch'u running olduktan sonra olur. Transactional callback yeni state'i okur. Reentrant silmede önce `retiring` işaretlenir ve mutasyon hakkı kapanır; ardından tek destruction bildirimi üretilir. Gerçek native free ve handle reuse, end-of-tick/frame retirement ve referans/fence tamamlanmasına bırakılır. Bu düzen callback içinde tekrar destroy çağrısını güvenli ret/idempotent sonuca çevirir.

## Genişleme ve kabul

Yeni migration, component veya backend bu kimlik ve bariyer alanlarını taşımak zorundadır. Harici ödeme/ödül gibi yan etkiler [outbox](../networking/persistence.md) sözleşmesine uyar; istemci ACK'ı DB atomikliği sağlamaz. Timeout ve cancellation rollback kanıtı değildir; [işlem kurtarma](failure-recovery.md) core registry, CommitReceipt ve WriterTerm'i tanımlar. [Kalıcı entity transferi](../networking/consistency-recovery.md) v1'de yalnız aynı core/atomik backend içinde desteklenir. AC-25–31 temel ayrımları, AC-73–77 kurtarma sınırlarını sınar.

## V0.3 plan ve projeksiyon sınırı

Release geçişi CatalogRevision yanında client ContractDigest/GameplayDigest ve server WorldPlanDigest değişimini taşır. Kaynak state transaction CriticalSet ile tutarlı kabul edilir; nav/acoustic gibi türevlerin aynı frame içinde tamamlanması şart değildir. [Projection sözleşmesi](../networking/world-change-projections.md) stale tüketici, invalidation ve domain follow-up kurallarını tanımlar. WorldRevision/JournalSequence ve OperationId anlamları değişmez. Tick fazı ve multi-domain reserve ayrıntısı [execution sözleşmesindedir](execution-contracts.md).

## PopulationPolicy ve hayvan geçişleri

Mevcut planın izin verdiği PopulationPolicy değişimi normal aggregate state transaction'ıdır; StateRevision artar. Aç/kapat için bütün dünyaya katalog reload yapılmaz. Commit'ten sonra eski policy revision ile hazırlanan spawn adayları revalidate edilir ve kapalı kanalda reddedilir. Policy kabulü ile devam eden drain ayrı sonuçlardır; retire bounded grup transaction'larıyla ilerler. [Nüfus sözleşmesi](../gameplay/population-traffic.md), AC-48/63/68.

Yeni Species/rig/controller/GameplayDigest eklemek release geçişidir. Büyüme, beden/hit değişimi veya provider migration'ı gereken closure ve son task/ownership state'ini hazırlar; quiesce/revalidate/Ready/commit/Applied sırası korunur. Uyumsuz native skeleton/controller major restart sınıfıdır. Evcilleştirme ile cleanup yarışında aynı aggregate reserve sırası uygulanır; postcommit failure sahiplik/ölüm/ödül kaydını geri silmez. AC-59/61/62/67.
