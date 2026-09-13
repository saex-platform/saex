# Geliştirici araçları, gözlemlenebilirlik ve replay

Durum: araç sözleşmesi taslağı. [Performans](../validation/performance.md) · [Yol haritası](../roadmap.md)

## Geliştiricinin soruları

Araçlar “kaç entity var?” sorusundan fazlasını yanıtlamalıdır: Bu entity neden görünmüyor? Hangi asset'i bekliyor? Kırılma intent'i niçin reddedildi? Hangi resource tick'i geciktiriyor? Başka client neden eski state'te? Hot reload hangi referansa takıldı?

## Inspector yüzeyleri

| Araç | Gösterilecek veri | Yetki |
|---|---|---|
| Entity inspector | Platform/persistent/native kimlik, components, state revision, owner epoch | World debug scope |
| Interest inspector | Include/exclude nedeni, dependency pin, visibility filter | İzinli oyuncu/world |
| Asset inspector | Source→artifact→catalog→lease, CPU/GPU/native load state | Content debug |
| Resource profiler | CPU/RAM, GC, IPC, callback, queue, grants ve leak | Resource admin |
| Network profiler | Kanal byte, baseline/catch-up, retry, stale drop ve RTT | Network debug |
| Physics overlay | Collider, sleep, solver mode, owner ve contact adayı | World debug |
| Lifecycle debugger | Prepare/drain/migrate/commit/rollback aşaması | Deploy permission |

Önerilen gelecekteki CLI adları `saex inspect`, `saex resource validate`, `saex package build`, `saex world diff` ve `saex diagnostics collect` şeklindedir. Bu depoda bu komutların uygulaması yoktur.

## Kimlik zinciri

Trace RequestId ile tek çağrıyı, OperationId/TransactionId ile reconnect aşan işlemi, TransitionId ile asset/resource aktivasyonunu izler. Server validation, JournalSequence, WorldRevision, ViewSequence, client apply ve adapter binding ayrı alanlardır. Her aşama monotonic local zamanını ve gerektiğinde server tick eşlemesini taşır. Farklı makinelerin duvar saatleri doğrudan aynı doğrulukta sayılmaz.

MTA referansından gelen lifecycle/streaming kararları için inspector; asset → LeaseId → resource epoch → entity generation → native handle zincirini, pending retire ve gecikmiş load sonucunu gösterir. Network profiler GNS lane başına queue/age/bytes, bağımlı event bekleme, owner churn ve fence edilmiş katılımcıyı ayrı sayaçlarla sunar. [İnceleme](../references/mtasa-architecture.md)

Log sınıfları; session, world, asset, resource, network, physics, persistence ve adapter'dır. Log severity policy'si her motion paketi için disk yazımı gerektirmez; örnekleme ve bounded ring buffer kullanılır. Crash'ten önceki kısa diagnostik pencere korunabilir.

## Editör iş akışı

Asset explorer katalogdan tanım seçer. Prefab editörü component alanlarını şema ile gösterir. Harita editörü PlacementId'yi korur, collision/LOD/portal görünümü sunar. Yıkım önizlemesi her state'in mesh/collider/parça closure'ını gösterir.

Local preview değişiklikleri üretim world'e otomatik aktarılmaz. Publish işlemi content diff, validation raporu ve katalog release'i üretir. Ortak editörde mutation revision kontrolü yapılır; son yazan sessizce bütün map'i ezmez.

## Replay ve spectator

Replay; accepted snapshot/transaction ve seçilmiş motion örneklerini kaydeder. Native input'lardan deterministik bütün oyunu yeniden oluşturma iddiası yoktur. Artifact/catalog revision'ları pinlenir; farklı model sürümüyle aynı olayın yanlış gösterilmesi önlenir.

Private state ve ham ses varsayılan kayıt dışıdır. Spectator ayrı grant ile canlı world projection alır; oyuncu gibi proximity filtri dışında bilgi alması explicit yetki gerektirir. Replay görüntüsü audit'e yardımcıdır; eksik motion örnekleri yüzünden bütün mikrofizik olaylarını kanıtlamaz.

## Geliştirici deneyimi

Bir resource için schema reference, minimal C# kullanım örneği, hata listesi ve lifecycle rehberi aynı sürümde yayımlanır. SDK ile server uyumsuzluğunda anlamlı version hatası verilir. Profiler overhead'i ayrıca ölçülür; debug özellikleri kapalı üretim profilini temsil etmez.

Kabul: AC-08 reload teşhisi, AC-13 ret açıklaması, AC-20 leak eğrisi, AC-22 debug permission. Tool çıktısı source desteği, runtime etkinliği ve doğrulanmış behavior'u ayrı göstermelidir.

## Studio ve kanıt iş akışı

[Studio/release](studio-release.md) ve [conformance](../validation/conformance-contracts.md) bu araçların aynı release kimliği etrafındaki işleyişini tanımlar. ExplainField prefab alanının kaynağını, ExplainChange actor/domain/state/projection neden zincirini gösterir. RehearsalWorld üretim etkilerinden yalıtılır; headless replay native fizik kanıtı değildir. Görsel Studio D4–D5'te genişler, ilk CLI doğrulayıcı/inspector D1–D3 temel alt kümesidir.

## Nüfus, davranış ve tür inspector'ları

Population inspector effective policy/provenance, zone/slot/generation, active/reserved/protected/draining sayaç ve ret nedenini gösterir. Agent inspector goal/task phase, iptal/deadline, perception kaynağı, path read revisions, owner epoch ve SimulationTier zincirini izler. Animal inspector Species → Breed → Morphology → Rig → Animation/Locomotion → DefinitionRevision → birey/owner/corpse bağını açıklar.

Overlay'ler şerit/sinyal/conflict reservation, hava koridoru, habitat/clearance ve actual native hit/collider farkını yetkili scope'ta gösterir. Bir türün model-load başarısı ile grounded movement/sync kanıtı ayrı etiketlenir. Log/replay private blackboard'u varsayılan içermez. Araçlar [AC-46–70](../validation/scenarios.md) cause/receipt/revision ve pool trendi kanıtını üretmeye yardımcı olur; testin yerine geçmez.

## Kurtarma teşhis zinciri

Inspector OperationKey → transaction/receipt → WriterTerm/JournalSequence → WorldRevision/ViewSequence → BaselineId/AppliedBindingRevision zincirini ilişkilendirir. ClockDomainId, SimulationTick, ControlTime elapsed ve UTC farklı etiketlenir. Transfer inspector canonical location, TransferGeneration, reservation phase ve quota nedenini gösterir; sürekli retry butonu belirsiz commit'i tekrar etmez.

Metrikler admission ret, queue byte/item/age, unknown-commit age, scope fence süresi, recovery attempt, circuit state, retained artifact kökü ve old-term ret sayısını kapsar. Diagnostic paket private AI/credential/ses payload'u taşımaz. [Fault profilleri](../validation/performance.md) ve [recovery](../architecture/failure-recovery.md), AC-71–88; araç maliyeti de ölçüme dahildir.
