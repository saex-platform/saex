# C# çalışma zamanı ve SDK taslağı

Durum: sözleşme taslağı; örnekler gerçek SDK kodu değildir. [Resource manifesti](manifest.md) · [Güvenlik](security-native.md)

## Runtime yerleşimi

C#/.NET 10 LTS, server ve client resource'larının ana dili olacaktır. Server core ve GTA adapter C++20 kalır. Worker kendi .NET bağımlılıklarını yükler; core ile object pointer paylaşmaz. Core protokol/SDK contract sürümünü worker başlatılırken doğrular.

İlk model resource başına worker sürecidir. Client worker x64 olduğundan büyük managed heap GTA'nın x86 adres alanını paylaşmaz; IPC, kopyalama ve toplam RAM maliyeti doğurur. Güven sınırı [R-03](../decisions/research-register.md) ile kanıtlanmalıdır.

## SDK yüzeyleri

| Paket ailesi | Sunulan yetenek | Sunulmayan yetenek |
|---|---|---|
| Saex.Shared | Kimlik, DTO, schema, vektör, hata ve lifecycle tipleri | Otomatik client-server bellek paylaşımı |
| Saex.Server | World/entity komutları, izinli persistence, session ve domain servisleri | GTA pointer veya render çağrısı |
| Saex.Client | Yerel input, kamera, UI, ses, görsel state ve intent gönderimi | Kalıcı state'i tek taraflı kesinleştirme |
| Saex.Tools | Paket doğrulama, katalog inceleme, editör metadata | Üretim oturumuna sınırsız admin erişimi |

## Asgari arayüz taslakları

| Arayüz | Girdi ve çıktı | Sıralama |
|---|---|---|
| World.CreateEntity | WorldRef, PrefabRef, initial state → CommandResult ve EntityRef | Sunucu commit |
| Entity.Submit | EntityRef, typed command, RequestId, durable OperationId, optional expectedRevision → sonuç | Tick kuyruğu |
| World.ReadSnapshot | Yetkili scope → immutable SnapshotView | Okuma zamanı belirtilir |
| Assets.Acquire | AssetRef, catalogRevision, purpose → lease veya hata | Async; artifact ready ayrı |
| Events.Subscribe | EventType, scope, handler → subscription lease | Resource epoch kapsamında |
| Network.SendIntent | İlan edilmiş intent schema ve payload → request receipt | Sunucu onayı değildir |
| Storage.Execute | İzinli provider işlemi → async sonuç | Oyun thread'ini bloklamaz |
| Ui.SubmitScene | Sahne değişiklik paketi → kabul/ret | Kare sınırında |

Örnek yalnız kullanım amacını gösterir:

```csharp
// Kavramsal sunucu SDK'sı; bu depoda uygulaması yoktur.
var result = await world.CreateEntityAsync(
    prefab: "sandbox.props:crate/wood",
    operationId: operation.Id,
    cancellationToken: lifetime.Stopping);

if (result.Pending)
    return InteractionReply.Pending(result.OperationId);

if (!result.Accepted)
    return InteractionReply.Rejected(result.Reason);

return InteractionReply.Created(result.Entity);
```

Metot adları ABI olarak sabitlenmiş değildir. Örnekte Accepted kesin authoritative oluşturma sonucudur; core kuyruğuna admission veya yerel proxy allocation başarı yerine geçmez. Pending ve timeout/cancellation sonrası OutcomeUnknown kesin ret sayılmaz; aynı OperationKey ile sorgulanır. Cancellation exception kullanacak SDK binding'i bu ayrımı çağırana görünür biçimde korumalıdır.

## Async ve thread davranışı

Her resource'un mantıksal bir callback kuyruğu vardır. Async I/O serbesttir, entity mutasyonu komut halinde core'a döner. Callback sonrasında entity silinmiş veya dünya değişmiş olabilir; referans/revision tekrar denetlenir.

`CancellationToken` işi iptal etmeyi ister; zaten commit edilmiş transaction'ı geriye almaz. Durable sonuç timeout/reconnect sonrasında OperationId ile sorgulanır; RequestId SDK'nın session/resource epoch kapsamında ürettiği korelasyon alanıdır. Async işler worker lifetime kaydına girer; broker kaynakları core tarafından oluşturuldukları anda kaydedilir. Resource stop iptal/drain uygular, takılan worker process olarak sonlandırılır.

Client'ta input/kamera gibi kare hassas işleri deklaratif controller profilleri ve toplu komutlarla ifade ederiz. C# handler'ını her native input okuması için senkron çağırmayız. Resource UI olaylarını bir frame sonra alabilir; izinli gecikme profilde görünür.

## Event bus ve network ayrımı

Yerel event otomatik ağ yayını değildir. Remote event için taraf, payload şeması, azami boyut, oran, alıcı ve capability manifestte ilan edilir. Handler birbirine doğrudan CLR nesnesi göndermez. Public event payload'una credential veya private component eklenmez.

Transactional gameplay olayı commit sonrasında yayınlanır. Cosmetic event state'in yerine geçmez. Durable yan etki üreten handler, EventId'yi cause olarak kullanıp ayrı OperationId/receipt ile idempotent komut gönderir.

Remote dispatch, transport actor kimliğinden üretilen salt okunur `ActorContext` taşır; payload'daki source entity oyuncu kimliği değildir. Varsayılan event yereldir, remote izin açık ilan edilir. Manifest şeması alan sayısı/türü, byte sınırı, direction, capability ve kota taşır. Entity/resource hazır olmadan callback teslimi [aktivasyon bariyerinde](../architecture/activation-contracts.md) bekler. MTA remote-trigger/caller ayrımı bu tasarımın [kaynak referansıdır](../references/mtasa-architecture.md).

## Genişleme, hata ve kabul

Yeni dil aynı command/event sözleşmelerine adapter yazar; C# mantığı protokole gömülmez. Schema major değişimi birlikte dağıtım veya açık adapter gerektirir. Worker'da dependency unload başarısız olursa süreç geri dönüştürülür.

Kabul: AC-08 lifecycle, AC-09 yetki, AC-12 stale async sonuç, AC-19 çift taraflı SDK. SDK reference'ında her çağrı taraf, capability, hata, thread davranışı ve idempotency bilgisiyle belgelenmelidir.

## Tek şemadan SDK ve araç üretimi

[ContractSchema](../architecture/execution-contracts.md), C++/C# DTO/broker proxy ve bounded validator/inspector tanımlarını üretecek ortak kaynaktır. D1 foundation'da Saex.Contracts kimlik tipleri ve yerel fixture codec'i, ContractGen ve Saex.Tools metadata/PE CLI alt kümesi kodlandı; burada tasarlanan Saex.Shared/Client/Server resource SDK'sı değildir. [Kaynak ve kanıt](../development/d1-foundation.md). C# resource hedefte immutable snapshot okur ve yetkili domain komutu gönderir. ActorContext korunur; provider yetkisi caller için otomatik yükselmez. [Actions/Effects](../gameplay/actions-effects.md) isteğe bağlı paket hedefidir.

## Population, Agent ve Animals SDK yüzeyleri

Saex.Shared typed PopulationPolicy/AgentTask/Species/Breed/Definition DTO ve snapshot şemalarını; Saex.Server PopulationService, AgentService ve Animals provider proxy'lerini; Saex.Client izinli interaction intent ve presentation yüzeyini sunmayı hedefler. Saex.Tools species/rig/route/behavior validation ve inspector sağlar. Bunlar kavramsal paket/API aileleridir, mevcut DLL değildir.

SetPopulationPolicy, RequestTask, QueryPath, Animals.Spawn/RequestInteraction/AssignOwner/TransferWorld/InspectDefinition komutları actor/grant, expected revision, budget ve gerekli OperationId kurallarını taşır. Spawn tanımı önceden WorldPlan'da çözülmüş olmalıdır. Per-agent senkron IPC veya native pointer yoktur. Snapshot'taki hayvan sahipliği client'a kendini owner ilan etme hakkı vermez. [Nüfus](../gameplay/population-traffic.md), [AI](../gameplay/agents-navigation.md), [hayvanlar](../gameplay/animals-species.md); AC-46/56/59/65/66.

## İptal, timeout ve kalıcı sonuç sorgusu

SDK `RequestCancel` ile “işlem geri alındı” sonucunu eşitlemez. JobResult eski ResourceEpoch/read set'te reddedilebilir; admitted durable OperationKey worker ömründen bağımsız core registry'de izlenir. Kavramsal `QueryOperation` actor/domain yetkisi altında `pending/outcome_unknown`, kesin ret veya committed sonucu döndürür. Receipt önceki worker öldü diye kaybolmaz; yeni worker state snapshot'ını core reconcile sonrasında alır. Ham DB callback'i native frame'i bekletmez.

API deadline'ları clock kind taşır. Oyun cooldown'ı SimulationTick, RPC/worker/lease bekleyişi ControlTime kullanır. Restart toplam bütçesi SDK çağrısıyla aşılamaz. `Animals.TransferWorld` v1'de aynı core/backend hedefiyle sınırlıdır. [Kurtarma](../architecture/failure-recovery.md), [transfer](../networking/consistency-recovery.md); AC-73/74/76/82/83.
