# Kalıcı dünya, journal ve kurtarma

Durum: sözleşme taslağı. [Entity modeli](../architecture/entities-worlds.md) · [Yıkım](../assets/destruction.md)

## Veri sınıfları

| Sınıf | Örnek | Başarı ve kurtarma |
|---|---|---|
| `none` | Efekt veya yalnız oturumda yaşayan replike state | Saklanmaz; replike olup olmaması ayrı karardır |
| `checkpointed` | Hareketli aracın konumu | Son checkpoint'e dönüş kabulü profile yazılır |
| `durable` | Kırılmış duvar, sahiplik, envanter değişimi | Başarı commit sonrasında; doğrulanmış backend/process-crash garantisi, disk/host-loss RPO ayrı |

`checkpointed` varsayılan aralık 5 saniyedir. Bu aradaki hareketin crash sonrası geri sarılması mümkündür. Durable kırılma için “önce herkese başarılı göster, sonra DB'ye yaz” kullanılmaz; client tahmini efekt gösterebilir ancak committed state ayrı kalır.

## Saklama modeli

Base map ve immutable paket kataloğu yeniden üretilebilir temeldir. World journal, placement farklarını ve dinamik entity aggregate değişikliklerini taşır. Periyodik snapshot kurtarmayı hızlandırır; tek başına sürekli tam dünya dosyası yazılmaz.

| Kayıt | Asgari içerik |
|---|---|
| WorldRecord | Kalıcı world kimliği, schema, catalog revision, gameplay profile |
| WorldTransaction | TransactionId, JournalSequence, OperationId/cause, actor, tick ve kalıcı aggregate değişiklikleri |
| PlacementOverride | Map namespace, PlacementId, damage/destruction state, transform farkı, state revision |
| DynamicEntityRecord | PersistentEntityId, prefab/version, kalıcı component alanları |
| SnapshotRecord | JournalSequence sınırı, checksum, catalog/schema sürümleri |
| OperationReceipt | Domain + actor + OperationId, payload özeti, sonuç, TransactionId ve JournalSequence |
| WorldWriterRecord | WorldPersistentId, ownerInstance, WriterTerm ve claim durumu; her transaction'da guard |
| TransferRecord | PersistentEntityId, TransferGeneration, source/target, reservation, phase ve canonical location |

Journal kalıcı kabul edilmiş değişiklikleri kaydeder; `none` state ve her motion örneği buraya yazılmaz. Canlı WorldRevision ve disk JournalSequence ayrı sayaçlardır. Geç katılım için canlı kesit, kurtarma için kalıcı kesit kullanılır. Native fiziği bütün input'lardan deterministik yeniden çalıştırmak gerekmez. Server restart sonrası yeni runtime EntityRef üretilir; persistent kimlikten runtime eşlemesi kurulur.

## Atomik değişiklik

Duvar kırılması parent state, tombstone/override, kalıcı debris ve varsa oyun ödülünü tek transaction sınırına alır. Ödül ayrı DB'deyse distributed atomiklik varsayılmaz: transaction outbox ve idempotent consumer kullanılır; UI ödülün beklemede olduğunu ayırır.

Tek writer aggregate revision'larını seri işler. Etkilenen aggregate pending iken aynı state'e yeni mutasyon sıraya alınır; DB I/O async yürür ve ilgisiz entity'leri/GTA frame'ini bloklamaz. Durable commit sonrasında core güvenli tick'te state'i yayımlar ve WorldRevision verir. DB commit ile core apply arasındaki crash, journal/receipt üzerinden kurtarılır. [İşlem sırası](../architecture/activation-contracts.md)

Backend en az atomic batch, koşullu revision, unique OperationId receipt, ordered journal ve transaction içi WriterTerm guard sözleşmesini karşılamalıdır. İlk referans backend PostgreSQL, lokal doğrulama backend'i SQLite olarak tasarlanır; oyun resource'larının kendi MySQL kullanması serbesttir, bu bağlantı core gameplay tablolarını guard dışında yazamaz. [Arıza/kurtarma](../architecture/failure-recovery.md) CommitReceipt/JobResult ayrımı, OutcomeUnknown, takeover ve backup closure'ını tanımlar. Provider garantileri ayrı kanıtlanır; SAEX v1 otomatik DB/world failover sunmaz.

## Kurtarma adımları

1. Önceki writer'ın kesin durmuş olduğunu doğrula; guarded WriterTerm claim al, yeni world epoch aç; henüz client kabul etme.
2. Snapshot checksum ve schema/catalog erişimini doğrula.
3. Snapshot JournalSequence sınırından sonraki committed journal kayıtlarını uygula.
4. Asset bağımlılıklarını ve placement migration'larını çöz.
5. Kalıcı entity'lere yeni runtime kimlikler üret; referansları yeniden bağla.
6. Sağlam stok yerleşimlerini oluşturmadan önce override/tombstone katmanını hazırla.
7. Pending operation/transfer sonucunu receipt/journal ile uzlaştır; kota ve artifact pinlerini persistent köklerden kur. World sağlıklı duruma geçtiğinde join ve baseline hizmetini aç.

Kritik world closure veya zorunlu migration eksikse restore başarısız olur; sağlıklı görünmek için boş dünya açılmaz. Yalnız ayrı tutulması güvenli birey eksikleri açık karantina kaydıyla korunabilir; birey silinmez veya başka türle değiştirilmez. Operatör doğrulanmış eski katalog/snapshot'a kontrollü dönebilir. Geri dönüşün hangi committed kayıtları dışarıda bıraktığı, backend failure envelope ve RPO/RTO açık raporlanır; backup manifest artifact/schema closure'ı içerir. AC-85.

## Tombstone ve retention

Haritadaki kırılmış placement'ın fark kaydı client ilgisinden bağımsız yaşar. Map revision'ı veya kurallar restore sonucunu değiştirmeden tombstone silinmez. Journal compact edilirken en son state ve OperationId receipt gereksinimleri snapshot'a taşınır.

Network dedup penceresi bounded olabilir; kalıcı ekonomi/inşa komutlarında süre aşmış receipt yüzünden eski iş yeniden yapılmaz. Durable OperationId domain kaydında veya receipt'te korunur; silme politikası eski işlem aralığını reddeden watermark veya domain uniqueness garantisi gerektirir. RequestId session/resource epoch kapsamında kalır ve reconnect'te değişebilir. Aynı actor/OperationId farklı payload ile tekrar gelirse `operation_conflict` döner. Motion paketleri aynı uzun vadeli saklama gereksinimine sahip değildir.

## Hata ve kabul

DB yavaşsa yeni durable aksiyon admission'ı sınırlanır; limitte `durability_unavailable` dönülür. Kabul edilmiş işlemin cevabı kayıpsa `pending/outcome_unknown` korunur; başarısız gibi yeniden başlatılmaz. Writer ownership doğrulanamıyorsa persistent world'ün gameplay mutasyonları fence edilir; read-only gösterim devam edebilir. Backup doğrulaması ayrı restore provası gerektirir. AC-73/74/75/83/86.

Kabul: AC-05 server restart, AC-07 sürüm taşıma, AC-18 commit öncesi/sonrası crash. Ölçüm: commit p95/p99, pending durable sayısı, snapshot yaşı, restore süresi ve journal büyümesi.

## Population ledger ve kalıcı hayvan

Policy aggregate, spawn slot/generation/cooldown ve birey kayıtları [nüfus](../gameplay/population-traffic.md) ve [hayvan](../gameplay/animals-species.md) sözleşmelerini kullanır. Ambient none state canlı baseline'da bulunur; restart aynı bireyi getirmek zorunda değildir. Kalıcı hayvan PersistentEntityId, DefinitionRevision, life stage, owner ve ilan edilmiş vitals/state ile restore edilir. Motion checkpoint geri sarılması durable sahiplik/ölümü geri alamaz.

Tame/transfer/death/loot/offspring işlemleri cause + OperationId ve domain uniqueness taşır; aynı seed'den yeniden üretim durable ledger'ın yerine geçmez. Slot disposition/ControlClass/kota aktarımı sahiplenmeyle tutarlı kaydedilir. Yeni runtime EntityRef kurulur; eski owner/task pointer'ı kullanılmaz. Eksik species artifact restore hatasıdır; hayvan sessizce başka model veya sıfır sağlıkla açılmaz. Offline ilerleme varsayılan kapalı; açılırsa son işlenmiş zaman, bounded catch-up ve clock rollback kuralı saklanır. AC-59/61/66/68/69.
