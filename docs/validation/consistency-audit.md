# V0.2 doküman tutarlılık denetimi

Tarih: 12 Eylül 2026. Durum: doküman düzeltmeleri; çalışan platform doğrulaması değil. [İndeks](../../README.md) · [Teknik kontroller](documentation-review.md)

## Kapsam ve yöntem

V0.1'in 38 Markdown belgesi okundu; transport, kimlik/revision, lifecycle/commit, asset readiness, kalıcılık, hata politikası ve kabul senaryoları karşılaştırıldı. MTA, GNS ve ENet resmî kaynak snapshot'ları incelendi. Aşağıdaki kayıtların bir kısmı doğrudan çelişki, bir kısmı uygulanabilirliği etkileyen eksik sözleşmedir. Tamamı v0.2'de doküman düzeyinde ele alındı.

## Bulgular ve yapılan düzeltme

| ID / tür | Önceki sorun | V0.2 düzeltmesi ve iz |
|---|---|---|
| C-01 Belirsizlik | GNS “ilk tercih/adayı”, ENet ayrı taslakta | [GNS kararı](../decisions/network-transport.md): v1 tek transport, ENet fallback yok; sources/coverage/ADR/protokol aynı karar |
| C-02 Eksik güven sınırı | HTTPS bootstrap ve GNS şifrelemesi, UDP peer kimliğini yeterince ayırmıyordu | Aynı kararda R-11a açık production gate; desteklenen trust-root/certificate yolunun ayrıca kanıtlanması |
| C-03 Çelişki | Resource reload client readiness'i server commit'ten sonra; catalog dokümanı önce istiyordu | [Lifecycle](../resources/lifecycle-hot-reload.md): prepare/Ready önce, commit sonra, Applied ACK en son |
| C-04 Çelişki | Session/resource epoch RequestId ile restart aşan receipt anlatılıyordu | [Terimler](../glossary.md), SDK, destruction, admin ve persistence: kalıcı OperationId + payload hash; transient RequestId |
| C-05 Eksik atomiklik | Bütün baseline'ın atomik apply ifadesi 2 ms frame bütçesini açıklamıyordu | [Ortak sözleşme](../architecture/activation-contracts.md): çok kare staging, bounded aggregate swap, JoinApplied ve native capability sınırı |
| C-06 Çelişki | Journal sırası tüm canlı state'in snapshot revision'ı gibi kullanılıyordu | Canlı WorldRevision, disk JournalSequence, entity StateRevision ve scope ViewSequence ayrıldı; AC-31 |
| C-07 Eksik sıralama | Reliable state, motion ve resource event'in ayrı akışları arasında dependency belirsizdi | [Protokol](../networking/protocol.md): requiresStateRevision, catalog/transition ve bounded queue; AC-25 |
| C-08 Eksik hata akışı | World/catalog commit sonrası hazır olmayan client'ın eski dünyaya dönüşü belirsizdi | [Entity/world](../architecture/entities-worlds.md): precommit abort; postcommit fence/resync; dönüş ayrı geçiş, participant sınırı; AC-28 |
| C-09 Enum çelişkisi | none/checkpoint/checkpointed/Ephemeral farklı adlandırılmıştı | Component ve destruction sözleşmesi `none/checkpointed/durable` ile birleşti; `none` replike olabilir |
| C-10 Kavram çelişkisi | Asset catalog PhysicsMaterial içine body mass tanımı koyuyordu | [Katalog](../assets/catalog.md): body-profile ayrı asset; surface malzemesi gövde ayarından ayrıldı |
| C-11 Belirsiz profil | Lease için RTT uyarlaması söyleniyor fakat sınırlar tanımlanmıyordu | [Otorite](../networking/authority.md): sabit 2 s/500 ms; ayrı owner hysteresis, group epoch, Ready; performans aynı değerler |
| C-12 Eksik crash cleanup | Lease registry worker'ın bildirimine bağlı anlatılmıştı; callback silme sınırı açık değildi | Broker oluştururken kaydeder; önce retiring/revoke, sonra callback ve deferred free; [lifecycle](../resources/lifecycle-hot-reload.md), AC-30 |
| C-13 Eksik toplam bütçe | Resource başına 128 MiB verilmiş, toplam process/reload maliyeti sınırlandırılmamıştı | [Streaming bütçesi](../assets/streaming-budgets.md): runtime deney sınırı, toplam rezerv admission, eski+yeni worker birlikte sayılır |
| C-14 Yetersiz referans | MTA karşılaştırması çoğunlukla wiki API düzeyindeydi | [10 kaynak akışı incelemesi](../references/mtasa-architecture.md): commit/satır kanıtları, alınan ilke ve SAEX farkı ayrı; ENet varsayımı yapılmadı |
| C-15 Eksik dispatch sözleşmesi | Remote event manifest örneğinde tipli payload/bağımlılık ve ActorContext bağı tam değildi | [Manifest](../resources/manifest.md): schema/capability/reliability/state requirement; SDK'da server-derived ActorContext, varsayılan local event |

## İncelemede korunan kararlar

Windows x86 GTA adapter/x64 host, Linux/Windows x64 server, C++20, C#/.NET10, OS süreç yalıtımı, kademeli otorite, hazır parçalı yıkım ve 64–128 ardından 256–512 hedefleri korunmuştur. MTA'dan öğrenmek Lua veya MTA protokol uyumluluğunu v1 gereksinimine çevirmedi.

MTA kaynak bulguları SAEX native destek kanıtı değildir. GNS seçilmiş olması authenticated standalone session'ın uygulandığı anlamına gelmez. R-01–R-12 ve R-11a/b/c açık; AC-01–33 yürütülmemiş gelecek test şartnamesidir.

## Yeniden denetim arayüzü

Yeni bir alan sözleşmesi RequestId/OperationId, herhangi bir revision, Ready/Applied veya persistence enum'u tanımlıyorsa [ortak sözleşmeyle](../architecture/activation-contracts.md) eşleştirilir. Native veya DB davranışı yeni bir atomiklik sınırı gerektirirse ADR ve ilgili AC birlikte değişir. Kaynak snapshot'ı güncellenirse eski çıkarım yeni commit üzerinden yeniden doğrulanır; dosya adı aynı diye davranış sabit sayılmaz.

Doküman kontrolünün çalıştırılan sonuçları [inceleme raporunda](documentation-review.md) tutulur. Bu tablodaki “düzeltildi”, gameplay testinin geçtiğini veya bütün gelecekteki çelişkilerin yokluğunu kanıtlamaz.

## Sonraki kapsamın izi

Bu v0.2 bulgu tablosu tarihsel olarak korunur. V0.4'te nüfus/AI/hayvan gereksinimlerinin bütün mimari yüzeylerle eşlemesi ve yeni tutarlılık kuralları [entegrasyon kaydındadır](population-animal-integration.md). Güncel teslimat sayıları/kontrolleri [dokümantasyon raporu](documentation-review.md) belirler; buradaki eski R/AC aralıkları yeni kapsamın toplamı değildir.
