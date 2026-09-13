# Ortak terimler ve sözleşme kuralları

Durum: sözleşme taslağı. [Ana indeks](../README.md)

| Terim | SAEX içindeki anlamı |
|---|---|
| Entity | Dünyada kimliği ve component durumu bulunan nesne; mutlaka görünür değildir |
| Component | Sürümlü veri ve mutasyon yetkisi sözleşmesi |
| Prefab | Entity/component başlangıç tanımı ve asset bağımlılıkları |
| Asset | Mantıksal içerik; model, collision, klip, malzeme veya davranış tanımı |
| Artifact | Belirli hedef için hazırlanmış, içeriği değişmeyen byte çıktısı |
| Resource | Yaşam döngüsü olan script/hizmet modülü |
| Package | Asset veya resource içeren sürümlü dağıtım birimi |
| Lock set | Bir dünya sürümünde kullanılacak kesin paket ve artifact özetleri |
| Catalog revision | Çözümlenmiş asset kataloğunun değişmeyen sürümü |
| World instance | Entity, otorite, zaman ve görünürlük kapsamı olan dünya |
| Interior/portal | Mekânsal geçiş tanımı; instance ile aynı şey değildir |
| Authority | Bir alanın geçerli değerine karar verme yetkisi |
| Simulation owner | Sunucunun geçici olarak fizik sonucu önermesine izin verdiği taraf |
| Gameplay owner | Sahiplik sistemindeki kullanıcı/kurum; ağ sahipliğiyle ilgisiz |
| Resource owner | Entity'nin yaşam döngüsünden sorumlu modül |
| Epoch | Oturum, dünya, resource veya sahiplik neslini ayıran değer |
| State revision | Bir entity aggregate'ının onaylanmış durum sürümü |
| World revision | WorldEpoch içindeki canlı state transaction sırası; sampled motion hariç |
| Journal sequence | Kalıcı dünya günlüğündeki committed kayıt sırası; WorldRevision ile aynı değildir |
| Scope epoch | İstemcinin dünyaya/kapsama abonelik nesli |
| Baseline | Belirli revision'a kadar kurulmuş tam başlangıç görüntüsü |
| Delta | Bilinen baseline üstündeki değişiklik |
| Tombstone | Silinen/kırılarak kaldırılan yerleşimin geri gelmesini önleyen kayıt |
| Interest/AOI | İstemcinin alması gereken bilgi kümesi; çizim mesafesinden geniştir |
| Prediction | Sunucu cevabını beklemeden uygulanan geçici yerel davranış |
| Reconciliation | Geçici durumun kabul edilmiş sunucu durumuyla uzlaştırılması |
| Cosmetic | Collision, hasar, görüş engeli veya ödül üretmeyen görsel/işitsel çıktı |
| Capability | Motorun sunduğu özellik veya resource'a verilen erişim yetkisi; bağlam belirtilir |
| Durable | Başarı onayı verilmeden önce kalıcı kayıt garantisi gerektiren veri |
| WorldPlan | Dünya profilinin kesin paket/provider/schema/mutator ve yürütme planı |
| ContractSchema / ContractDigest | SDK, doğrulayıcı ve inspector için ortak sürümlü şema / kesin özeti |
| GameplayDigest | Açık gameplay içerik ve schema compatibility anlamı; client dürüstlük kanıtı değil |
| ChangeSet | Accepted transaction'dan türetilen sınırlı değişiklik/bağımlılık bildirimi |
| CriticalSet | Aynı geçerli işlemde değişmesi gereken gameplay durum grubu |
| Projection | Kaynak state revision'larından hesaplanan, yeniden üretilebilir görünüm/cache |
| RepresentationSet | Yetkili state'in client native/GPU temsil kümesi; network AOI ile aynı değil |
| Trait / socket | Prefab bileşim parçası / adlandırılmış uyumlu attachment noktası |
| RehearsalWorld | Üretim etkilerinden izole, yeni kimliklerle kurulan test dünyası |
| ConformanceProfile | Capability için engine/build/schema/content kapsamlı kanıt tanımı |

## Yaşayan dünya terimleri

| Terim | SAEX içindeki anlamı |
|---|---|
| PopulationService | Ambient üretim, politika, rezervasyon, kota ve drain koordinatörü |
| PopulationPolicy | WorldPlan'ın izin verdiği sınırlar içinde mutable world aggregate; mevcut StateRevision kullanır |
| PopulationOrigin | Doğuş kanalı, policy/zone/slot/generation ve cause; sahiplik değişse de audit kökeni korunur |
| ControlClass | ambient/mission/player-engaged/owned/service; Lifetime ve persistence'den ayrı yönetim sınıfı |
| SpawnSlotId / SpawnGeneration | Mantıksal üretim yeri / slotun üretim nesli; kalıcı birey kimliği değildir |
| Agent | Ortak görev/algı/nav/controller sözleşmesini kullanan insan veya hayvan aktör |
| TaskInstanceId | Tek görev yürütümü; native task pointer veya durable OperationId değildir |
| SimulationTier | interactive/reduced/abstract/suspended; simulation mode ve render LOD'dan ayrı |
| SpeciesDefinition / BreedDefinition | Tür / isteğe bağlı oyun cinsi-ırkı tanımı; AssetRef kullanır |
| MorphologyProfile | Beden boyutu/oranı ve collision/hit/traversal uyumluluğu |
| RigProfile / AnimationSet | İskelet/bind pose/socket eşlemesi / gait-action klip ve zamanlama kümesi |
| AnimalDefinition / AnimalInstance | Çözümlenmiş spawn tanımı / dünyadaki mutable birey |
| PersistentEntityId | Restart aşan birey kimliği; yeni world epoch'ta yeni EntityRef'e çözülür |
| Protected drain | Yeni ambient üretimi kapatıp aktif etkileşim/ownership'i koruyarak güvenli retirement |
| Tam sync | Ortak accepted kimlik/görev/etkileşim sonucu ve ölçülen yakınsama; sıfır gecikme/bit eşitliği değil |

Asıl tanımlar [Population](gameplay/population-traffic.md), [Agent](gameplay/agents-navigation.md) ve [Animals](gameplay/animals-species.md) belgelerindedir.

## Zaman ve kurtarma terimleri

| Terim | SAEX içindeki anlamı |
|---|---|
| SimulationTick / ControlTime / UtcInstant | Oyun sırası / monoton kontrol süresi / kalıcı takvim; birbirinin yerine geçmez |
| ClockDomainId / ControlDeadline | Yerel saat kaynağı yaşamı / o kaynağa bağlı süre sonu; başka makineyle ham çıkarım yok |
| RecoveryProfile | Actor/resource/world/process tavanları, clock/timeout türü, restart ve failure policy |
| OperationKey | `(domain, actor, OperationId)` bileşiminin kısa adı; yeni dördüncü işlem kimliği değil |
| JobResult / CommitReceipt | Atılabilir transient öneri / core registry üzerinden uzlaştırılan kesin durable sonuç |
| OutcomeUnknown | Commit submission sonrası sonuç belirsiz; rollback veya success varsayılmaz |
| WriterTerm | Kalıcı world'ün guarded writer nesli; runtime WorldEpoch veya OwnerEpoch değil |
| BaselineId / ViewCut | Tek scope staging denemesi / reliable catch-up başlangıç kesiti |
| StateAppliedAck | Client'ın uyguladığı state kesiti; GNS transport ACK'ından ayrı |
| TransferGeneration / canonical location | Kalıcı bireyin transfer nesli / tek geçerli world konumu |
| RPO / RTO | Belirli arıza türünde kabul edilen veri kaybı aralığı / kurtarma süresi hedefi |

Normatif tanımlar: [zaman](architecture/time-fencing.md), [arıza/kurtarma](architecture/failure-recovery.md), [replikasyon onarımı ve transfer](networking/consistency-recovery.md). `fenced` geçersiz veya belirsiz otoriteyi kullanıma kapatır; kendiliğinden state silmez.

## Ortak veri kuralları

- Mantıksal `EntityRef`: `WorldId + WorldEpoch + EntityId + Generation`. Native pointer, GTA model ID'si veya array indeksi taşınmaz.
- `PlacementId`: kaynak harita paketinin ad alanında editörce atanmış, yeniden export'ta korunan kimlik. Konumdan veya sıralama indeksinden türetilmez.
- `AssetRef`: `namespace:path`; kesin artifact bir lock set ile çözülür. Aynı mantıksal kimlik farklı kataloglarda farklı artifact'e bağlanabilir.
- `RequestId`: session/resource epoch içindeki istek/cevap korelasyonu; yeniden bağlantı aşan kalıcı kimlik değildir.
- `OperationId`: yetkilendirilmiş actor ve domain kapsamında tek işlem amacı; durable receipt reconnect/restart boyunca bu kimlikle bulunur. Yeni RequestId aynı OperationId'yi taşıyabilir. Aynı OperationId farklı payload ile kullanılırsa `operation_conflict` döner.
- `TransactionId`, `WorldRevision`, `JournalSequence`, `StateRevision`, `ViewSequence` ve `MotionSequence` ayrı alanlardır; tam kapsamları [ortak sözleşmede](architecture/activation-contracts.md) tanımlıdır. Eski “transaction revision” ifadesi tek başına kullanılmaz.
- `TransitionId`: resource/catalog/world hazırlama ve aktivasyon denemesi; Ready/Applied mesajları bu kimlik ve kesin katalog/resource epoch'larıyla eşleşir.
- `Persistence` enum değerleri yalnız `none`, `checkpointed`, `durable`'dır. `none` runtime state yine replike olabilir; saklanmıyor olması cosmetic olduğu anlamına gelmez.
- Float alanlar sonlu olmalıdır; NaN/Infinity reddedilir. Dünya birimi tasarımda metre, süre saniye/tick, açı SDK'da radyandır; GTA dönüşümleri yalnızca adapter'da yapılır.
- Dünya eksenleri GTA uyarlamasında açık dönüşümle eşlenir; saklama/protokol biçiminde quaternion dönüşü kullanılır. Yön ve el sistemi R-02 kabulünde sabitlenir.
- UI metni UTF-8; teknik kimlikler ASCII küçük harf, sayı, nokta, tire, alt çizgi ve tanımlı ayırıcılarla sınırlıdır. Windows/Linux büyük-küçük harf farkı paket sonucunu değiştiremez.
- Boyut, sayaç ve kimliklerin mantıksal sınırları bellidir; wire byte genişlikleri protokol kodlaması seçilmeden “ABI” diye sunulmaz.

## Sonuç ve hata dili

`CommandResult` en az RequestId, varsa OperationId, `accepted/rejected/pending`, makinece okunabilir neden ve kabul edildiyse StateRevision/TransactionId taşır. Durable sonuç JournalSequence ile izlenir; canlı uygulama WorldRevision'ı ayrıca bulunabilir. `pending` başarı değildir. Ağdan alınmış olay, herhangi bir kalıcı komutun otomatik başarı kanıtı sayılmaz.

Ortak nedenler: `permission_denied`, `stale_entity`, `stale_epoch`, `revision_conflict`, `asset_not_ready`, `unsupported_capability`, `budget_exceeded`, `world_unavailable`, `dependency_conflict`, `durability_unavailable`, `overloaded`, `outcome_unknown`, `unsupported_transfer_domain`. PascalCase servis/phase etiketleriyle wire hata nedenleri karıştırılmaz; neden kodları snake_case kullanır.

## Belgeler arası öncelik

Kabul edilmiş ADR > ilgili alanın sözleşmesi > açıklamalı örnek. Çelişki bulunduğunda sessizce birini seçmek yerine doküman düzeltilir. Gerekçeli araştırma kayıtları gerçek uygulama kanıtının yerini tutmaz.
