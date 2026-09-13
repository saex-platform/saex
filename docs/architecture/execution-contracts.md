# ContractSchema, sistem zamanlaması ve tek mutasyon yolu

Durum: ADR-20; C++/C# sınırında normatif taslak. [Kimlik/commit](activation-contracts.md) · [WorldPlan](world-plans.md)

## D1-N1 dependency lock sınırı

[plugin-sdk.lock.json](../../contracts/engine/plugin-sdk.lock.json), genel ContractSchema generator girdisi değildir. Build aracı exact kaynak/patch/recipe kimliğini ve private x86 kapsamını doğrular; `runtimeEligible=false` kalır. [N1 raporu](../development/d1-native-dependency.md) patch formatını ve canonical envanter encoding'ini tanımlar. Bu ekleme wire ABI veya production IPC tanımlamaz.

## Sorumluluk ve veri sözleşmesi

ContractSchema, aynı alanın SDK'da başka, ağ decoder'ında başka, inspector'da başka anlam kazanmasını engellemek için tek şema kaynağıdır. Offline araçlar buradan C++/C# DTO ve broker proxy'lerini, bounded validator tablolarını, sürüm metadata'sını ve inspector alanlarını üretmeyi hedefler. D1 foundation'da [ContractGen](../../tools/ContractGen/Program.cs) kimlik tipleri/EntityRef ve yerel fixture sabitlerini üretir; tam schema/broker generator'ı değildir. [Uygulama sınırı](../development/d1-foundation.md); R-13 açık kalır.

| Şema parçası | Zorunlu anlam |
|---|---|
| Component | TypeId/schema, alan tür/boyut/birim, varsayılan, mutator domain, persistence ve visibility |
| Command | Parametre, ActorContext ihtiyacı, resource capability, idempotency, read/write set ve hata sonuçları |
| Event | Local/remote yönü, payload sınırı, state bağımlılığı, alıcı filtresi ve delivery policy |
| Service | Interface major/minor, provider seçimi, method bütçesi ve failure policy |
| Adapter capability | Engine profile, scope, live-set/recreate gereği ve exact/approximated/unsupported sonucu |

Girdi boyutu/depth/list uzunluğu bounded olmalıdır. Unknown field yalnız ilan edilmiş forward-compatible atlama kuralıyla kabul edilir. Schema major müzakereyle seçilir; ortak public schema bulunamıyorsa join reddedilir. Tek JSON alanı eklemek bütün client'ların onu güvenli anlayacağını göstermez. Özel serializer executable type adı, CLR object graph veya pointer kabul etmez.

## Tick fazları

| Faz | Core işi | Eşzamanlılık kuralı |
|---|---|---|
| Ingress | Bounded network/worker kuyruğunu al; caller ve phase doğrula | I/O thread'leri sadece immutable zarf kuyruğa koyar |
| Complete | Transient JobResult için epoch/read revision; CommitReceipt için core operation registry ve journal doğrula | Eski worker sonucu atılabilir; committed state kaybedilmez, core apply'da WorldRevision alır |
| Read snapshot | Sistemler için tutarlı state kesiti oluştur | Okuyucular bu kesiti değiştiremez |
| Evaluate | Yetkili command/reducer, AI query, physics hazırlığı | Bağımsız işler paralel olabilir; çıktı öneridir |
| Validate / reserve | Read/write set, domain grant, aggregate revision ve bütçe doğrula | Çakışan mutasyonlar sıralanır veya ret/yeniden değerlendirme |
| Commit / publish | Geçerli değişiklikleri mevcut transaction yoluna ver, hazır olanları yayımla | Tek authoritative writer; DB I/O bekleyen ayrı pending işlemdir |
| Project / replicate | Filtreli state, ChangeSet invalidation ve event delivery üret | Callback kendi write yolunu atlayamaz |
| Retire | Eski lease/handle/worker referanslarını bırak | Native frame/fence koşulları ayrıca korunur |

30 Hz logic, desteklenen physics için 60 Hz alt adım ve 20 Hz motion hedefleri değişmedi. WorldRevision sampled motion sayacı değildir. Server physics transform örnekleri state yapısını keyfî değiştiremez; body creation/destruction/profil değişimi transaction yolundan geçer.

## İş sırası ve çatışma çözümü

`SystemDescriptor` phase, reads/writes, before/after, max work ve deadline taşır. Schedule DAG'ı WorldPlan içinde çözülür. Sistemleri alfabetik sırada yüklemek gameplay önceliği değildir. Aynı phase'teki bağımsız read-only işler paralel planlanabilir; birbirine bağlı işler explicit sıraya alınır.

`JobResult` jobId, WorldEpoch, ResourceEpoch, read revision set ve cancellation generation içerir. Thread'in erken bitmesi authoritative öncelik sağlamaz; girişte verilen command sıra numarası, system order ve aggregate reservation kuralı kullanılır. Bir tick deadline'ını kaçıran transient iş sonraki tick'e devredilir veya tanımlı ret alır; sonra dönen stale öneri uygulanmaz. Bu ret kuralı backend'de kesinleşmiş CommitReceipt'i kapsamaz: [core-owned recovery](failure-recovery.md) sonucu uzlaştırır; reload snapshot'ı ilgili committed state uygulanmadan alınmaz. Bu kurallar GTA native fiziğinin deterministik olduğunu iddia etmez.

İlk scheduler, domain'ler arası birbirini bekleyen nested transaction açmaz. Multi-aggregate reserve sıralaması sabittir; aynı aggregate'a bağlı pending durable işlem bitmeden ikinci yazım yapılmaz. Kuyruk ve bekleme zamanı sınırlıdır. İlgisiz aggregate'lar ve network kontrolü ilerler. Bekleyen durable sonucu timeout nedeniyle körlemesine iptal edip yeni OperationId ile tekrar başlatmak yasaktır; [receipt sorgusu](activation-contracts.md) kullanılır.

## Resource ve domain yetkisi

Bir resource başka domain'in alanını doğrudan yazamaz. `construction` inventory azaltmak için inventory provider'a typed komut/transaction katkısı ister. Multi-domain transaction için her katılımcı domain kendi izinli write set'ini üretir; core yalnız bunları birleştirip doğrular. Tek persistence backend'de garantisi varsa atomik batch yapılır. Harici domain'de distributed atomiklik varsayılmaz, outbox ve pending iş akışı kullanılır.

Servis çağrısında `ActorContext` korunur. Güçlü yetkili provider, başka resource adına gelen isteği kendi sınırsız yetkisiyle otomatik yükseltemez. Açık delegation; actor, caller resource, hedef, işlem kapsamı, epoch ve expiry içerir; client tarafından üretilemez. Delegation yoksa provider kendi internal grant'i yerine caller için izinli kesişimi kullanır. Player yetkisi ile resource capability ikisi de gerektiğinde denetlenir.

## Zaman ve arıza bütçesi

Resource'un bildirdiği maliyet güvenilir değildir; host/core gerçek CPU/IPC/queue kullanımını ölçer. Başlangıçta client worker toplam rezervi için tanımlı 1 GiB deney profili ve tüm mevcut streaming sınırları korunur. Scheduler her resource'a world profilinde pay verir; kontrol ve retire işleri sürekli script yüküyle aç bırakılamaz.

Sınırsız C# callback tick içinde çalıştırılmaz. Transient worker sonucu deadline'a gelmezse core önceki kabul edilmiş state ve desteklenen fallback ile devam eder; capability/grant revoke veya bütçeli restart politikası devreye girer. Commit sonucu belirsiz aggregate fence edilir; yeni mutasyonla üstü örtülmez. Oyun deadline'ı ile kontrol timeout'u [ayrı saat alanıdır](time-fencing.md). Kritik komut geç çalıştı diye geçmişe yetkisiz hit/ödül yazılmaz. Prediction sadece izinli yerel sunumdur. AC-73/74/82/83.

## Genişleme ve kabul

Yeni dil aynı ContractSchema/broker üzerinden bağlanabilir; her runtime'ın kendi güvenlik ve maliyet kanıtı gerekir. Yeni native adapter aynı conformance profilini bildirir; doğrulanmamış capability auto-enable olmaz. AC-35/36/37/45; schema uyuşmazlığı, farklı job tamamlama sırası, confused-deputy ve resource açlığı durumlarını sınar. R-14 scheduler doğrulaması oyun akışı için zorunludur.

## Bütçeli AI ve üretim görevleri

[AgentService](../gameplay/agents-navigation.md) algı, karar ve nav işlerini immutable snapshot'tan üretir; çıktı TaskInstanceId, cancellation generation, EntityRef, epoch ve read revision seti taşır. Geç biten path/death/attack sonucu eski state'i diriltemez. Graph çevrimi ancak yield/max transition ile tick'ler arasında çalışır; aynı tick içinde sınırsız event recursion yoktur.

Population spawn adayı ayrı rezervdir; commit'te Policy StateRevision, slot generation, group closure ve quota tekrar kontrol edilir. Politikayı kapatma ile create yarışı aynı writer yolunda çözülür. Ortak sürü kararları bounded fan-out kullanır; her ajanı her ajanla her tick karşılaştırmaz. İlan edilmiş frekans CPU garantisi değildir. AC-48/54/60/65/70; R-14/17/19.

## D1 kod ve kabul sınırı

[BoundedInbox](../../src/core/bounded_inbox.cpp) item/charged-byte admission ve close/drain primitive'idir. Mutex ile concurrent producer tavanı sınandı; allocator overhead, gerçek scheduler fairness, process toplam bütçe ve durable completion registry henüz uygulanmadı. ResourceEpoch zero ret, resource yetkisinin doğrulandığı anlamına gelmez. AC-45/83 ve R-14 yalnız bu alt kanıtı alır.

Kimlikler dört uint64 EntityRef alanı ve explicit checked artış kullanır. [44 byte fixture](../development/d1-foundation.md) stdin/stdout üzerinden iki farklı mimaride test edildi; GNS wire ABI veya güvenli production worker IPC'si değildir. C# oyun worker'ına native pointer/veritabanı yetkisi verilmedi. Genel şema/schedule/private-public yüzeyleri sonraki kapıdır.

## D1-N2 profil verisi sınırı

[Observed engine profile](../../contracts/engine/observed-profile.json) offline uyumluluk gözlemidir; network ContractSchema, server grant veya runtime capability kaydı değildir. JSON kaynak digest'i native generator ve C# gömülü kaynak arasında eşleşir. Hash/layout/anchor eşleşse de `canAttach=false` kalır. `ImageReader` process içi sınırlı kopya arayüzüdür; raw GTA pointer'ı worker/protokol sözleşmesine eklemez. [N2 uygulama ve hata sözleşmesi](../development/d1-engine-preflight.md), ADR-37.

## D1-N2 bootstrap C ABI sınırı

[Kod 0.1.3 başlangıç ABI'si](../../include/saex/engine/bootstrap_api.h), üç __cdecl C export ve 192 byte fixed-width status kullanır. Major/buffer doğrulaması IO'dan önce; concurrent çağrı BUSY ile beklemeden döner. Output pointer'ı güvenilir aynı-process çağıran sağlar, remote payload değildir. Stop terminaldir; bekleyen Initialize varsa BUSY, işlem bittikten sonra yeni çağrıları kesen sahip Stop/FreeLibrary sırasını yürütür. Disk IO yalnız açık başlangıç çağrısında; frame callback veya production scheduler işi değildir. [Uygulama ve test sınırı](../development/d1-bootstrap-module.md), ADR-38/AC-90.

## Kod 0.1.7 policy verisinin yürütme sınırı

[Loader policy JSON](../development/d1-loader-policy.md) bir source/build sözleşmesidir; IPC/wire komutu veya indirilen server yetkisi değildir. C++'a derlenen veriden bütün pinler child öncesi hazırlanır; eksik/drift/partial durumda yürütme açılmaz. Bir modülün mapping için listelenmesi resource/native initializer ya da GTA fonksiyonu çalıştırma izni üretmez. Olaylar arasında OS/compatibility kodu çalışabileceğinden bu araç worker sandbox güvenlik sınırı olarak sunulmaz; mevcut C ABI ve bounded production IPC hedefi değişmez.

## Kod 0.1.11 — Entry supplement bağı

Entry initialization policy, core ControlClock/LeaseAuthority veya bootstrap C ABI değildir. Yalnız local debugger experiment iznidir; source build zamanında derlenir. Base pin/source digest ile executionPolicySourceDigest ayrı tutulur. Worker/SDK/IPC sözleşmeleri değişmez. [Ayrıntı](../development/d1-entry-boundary.md).

## GitHub kaynak yayını — taşınabilirlik düzeltmesi

Bootstrap reason fallback artık açık uint32_t dönüşümü kullanır; C ABI 1/status layout ve hata kodlarının sayısal değeri değişmez. GCC derleme uyumu IPC veya execution izni eklemez.

### Hosted corpus tamamlaması

Bootstrap session corpus artık geçersiz reason 0/999/UINT32_MAX için exact INTERNAL_ERROR değerini ve bilinen ret kodlarının korunmasını ayrıca sınar. Önceki test yalnız terminal ret durumunu ölçüyordu; bu ek assertion ABI/fallback değerini doğrudan kanıtlar. Production davranışı ve layout değişmez.
