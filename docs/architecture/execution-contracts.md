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

## Kod 0.1.12 — Proxy dönüş sınırı

0.1.12’de [proxy recipe](../development/d1-proxy-return.md) ayrı build-time JSON sözleşmesidir: strict duplicate/key/type/size/RVA/alignment denetimi, entry source digest ve mevcut game-root module hash bağı. Runtime JSON yüklenmez; source değişikliği açık üretim ve --check gerektirir. Bu yerel gözlem tarifi production IPC/SDK/resource sözleşmesi veya yeni C ABI değildir.

## Kod 0.1.13 — Startup çağrı sınırı

[Startup policy](../development/d1-startup-call.md) strict build-time JSON’dur: exact proxy/profile digest, stage, x86-ff15-iat call form, 68-byte same-stack argument, dört sample üst sınırı ve zorunlu IAT watch. Boundary değerleri runtime seçeneklere dönüşmez; source değişikliği generator --check ve test ister. Core IPC, grants, resource veya bootstrap C ABI 1 değişmez.

## Kod 0.1.14 — Codec dönüş kesiti

Codec initializer kodu yalnız yeni açık deneyde çalışabilir; başlangıçtaki 23 pin yeni modda 26 olur. Dördüncü hit MOV EDI,EAX öncesinde terminaldir; C# process/sandbox ve gerçek SAEX loader-lock dışı C ABI kanıtı yerine geçmez. [Sözleşme ve doğrulama](../development/d1-codec-return.md).

## Kod 0.1.15 — Codec binding izni

Yeni binding-policy source'u codec digest'ine bağlıdır; dördüncü duraktan beşinciye ayrı yürütme izni verir. GetProcAddress sonuçlarının tabloya yazılması codec fonksiyon çağrısı/ABI kanıtı değildir. Aynı 26 pin, main-thread DR1 watch ve owned cleanup korunur; gerçek SAEX C ABI loader lock dışında ayrıca doğrulanır. [Sözleşme](../development/d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](../development/d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

0.1.16 kimlik zinciri: strict ASI source → önceki binding digest’i; CMake audit → aynı configuration DLL ve ham map digest’i → compiled artifact pin → retained same-file LOAD_DLL kimliği. İzin seçimi ile path/hit/verified sonuçları ayrıdır. Build/source güveni varsayılır; paket imzası, production loader veya güvenlik sandbox’ı değildir. ASI permit eski policy constructor’ında varsayılan kapalıdır.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](../development/d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Yeni observer API aynı bootstrap execution iznini kullanır; üç ek EXE okuması yeni native çağrı veya hook izni değildir. İlk/ASI örnek retleri sırasıyla loader/bootstrap ilerlemesini keser. Terminal ret durumunda tamamlanmış bootstrap kanıtı korunur fakat frame doğrulanmaz. [Aday ve doğrulama raporu](../development/d1-frame-target.md).

## Kod 0.1.19 bağlantısı

Ayrı startup-return deneyi ASI dönüşünden sonra loaderın exact EXE image aralığını 0x40 korumasına geçirme çağrısını ve doğal startup dönüşünü gözler. Observer yığın/EIP yazmaz, bootstrap exportlarını çağırmaz. Eski execution izinleri bu devamı kapsamaz. [Sözleşme ve kanıt](../development/d1-startup-return.md).

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](../development/d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](../development/d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](../development/d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

Yeni platform CLI'ında body yürütme alanları üç durum taşır: devam verilmediğinde false, CALL durağına ulaşılınca true, ilerletilip henüz doğrulanamayan hata durumunda null. Host-setting çağrısına izin false kalır. Eski CLI'ların boolean sonuçları değişmez; izin, ilerleme ve kanıt aynı anlamda kullanılmaz.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](../development/d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.

0.1.23 bağlam geri alması uygulama register/segment/flag ve breakpoint adres/izinlerini kapsar. EFLAGS sabit bit 1 normalize edilir; DR6 olay nedeni restore sırasında sıfırlanır. RF, return-site breakpoint olayından ayrı değerlendirilir. Bu özel ayrımlar dışındaki context farkı ret üretir; [ayrıntı](../development/d1-platform-suppression.md).

## Kod 0.1.24 bağlantısı

Ayrı instance-startup modu platform bastırma dönüşünden gerçek named event oluşturma/açma ve doğal helper dönüşüne ilerler. Mevcut event veya NULL handle durumunda pencere kolundan önce ret verilir. Önceki suppression modu restore ederek bitmeye devam eder; yeni mod doğal API sonrası eski CALL bağlamını geri yazmaz. [Sözleşme ve doğrulama](../development/d1-instance-startup.md). Oturumdaki ortak event ömrü process-private değildir; observer sinyal durumunu değiştirmez. C ABI 1/GNS/otorite aynı; pencere/renderer/doğal frame ve N2/N3/D1/D2 kapıları açıktır.

## Kod 0.1.25 bağlantısı

Ayrı event-dispatch modu, instance dönüşünden doğal olay dağıtıcısı CALL/entry ve uygulama işleyicisi CALL önüne ilerler. Üç durakta argüman, dönüş adresi, register ve yaşayan caller stack doğrulanır; uygulama işleyicisi çalıştırılmaz. [Sözleşme ve sonuç](../development/d1-event-dispatch.md). Eski instance terminali, C ABI 1/GNS/otorite aynı; yeni bağımlılık/kalıcı migration yoktur. AppEventHandler gövdesindeki executable yönlendirmesi, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.26 bağlantısı

`--observe-application-routing` önceki event-dispatch kanıtından sonra yalnız rsINITIALIZE=24 rotasını yürütür: işleyici entry → executable detour → indirect JMP → ilk oyun initializer CALL öncesi. 39 index/11 hedef tablosu, rel32/absolute operand ve dört yığın/register sınırı doğrulanır. [Sözleşme ve sonuç](../development/d1-application-routing.md). Önceki mod kendi AppEventHandler CALL öncesi terminalini korur. Yeni modda `eventDispatchObservation.applicationHandlerCallAllowed=true`, routing nesnesinde initializer çağrı izni false olur; önceki stage/verified ara kanıtı korunur. C++ trace/API yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni bağımlılık/kaldırılan özellik/kalıcı migration yoktur. Oyun initializer gövdesi, RsInitialize, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.27 bağlantısı

`--observe-game-prelude` ilk oyun initializer içine girer; exact boş Init ve üç yerelleştirme bayrağını yazan iki helper doğal olarak geri döner. Beş durak, stack/register/flags, yaşayan caller ve 16-byte veri penceresi denetlenir; yalnız üç veri byte değişebilir. CFileMgr CALL çalıştırılmaz. [Sözleşme ve sonuç](../development/d1-game-prelude.md). Önceki application-routing terminali korunur; yeni üst modda routing nesnesinin initializerCallAllowed alanı true, prelude nesnesinin fileManagerCallAllowed ve initializerReturnVerified alanları false olur. C++ observer yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni dependency/kaldırma/kalıcı migration yoktur. CFileMgr, streaming/pad, initializer dönüşü, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.28 bağlantısı

`--observe-file-manager-entry` CFileMgr içine doğal CALL ve ilk üç PUSH komutunu açar; 0x5386FB CRT cwd CALL önünde durur. İki durakta buffer/maxlen=128 ABI, nested return stack, register/flags, 136-byte root/guard ve localisation korunumu denetlenir. [Sözleşme ve sonuç](../development/d1-file-manager-entry.md). Önceki prelude terminali korunur; yeni üst modda prelude fileManagerCallAllowed=true, manager cwdCallAllowed=false/fileManagerReturnVerified=false olur. CRT lock/SEH/OS/copy yolu henüz açılmaz. Gelecekte suffix yazımından önce NUL en geç buffer offset 126, başarılı dönüş ve ANSI byte uzunluğu kanıtı gerekir. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Manager/initializer dönüşü, streaming/pad, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.29 bağlantısı

`--observe-cwd-seh` CRT wrapper ve SEH prologue içine doğal CALL açar; kayıt kurulup yardımcı döndüğünde 0x836E9D noktasında durur. Üç durakta 80-byte stack, 28-byte NT_TIB, önceki kayıt ve caller/buffer/localisation korunumu denetlenir. [Sözleşme ve sonuç](../development/d1-cwd-seh.md). Önceki manager terminali korunur; yeni üst modda manager cwdCallAllowed=true, cwdSeh lockPathAllowed/directoryApiAllowed/cwdReturnVerified/unwindVerified=false olur. Handler veya kilit/OS/copy yolu açılmaz; owned child sonunda kapatılır, eski TEB/context rollback yapılmaz. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Kilit/cwd, SEH sökümü, manager/initializer dönüşü, renderer/doğal frame ve N2/N3/D1/D2 açıktır.

## Kod 0.1.30 bağlantısı

`--observe-cwd-lock` lock(7) selector CALL ve ilk 17-byte gövdeyi doğal yürütür; CMP tamamlandığında 0x82ADCF JNE önünde durur. [Sözleşme ve sonuç](../development/d1-cwd-lock.md). 100-byte stack, 16-byte slot penceresi, NT_TIB/önceki kayıt/caller/buffer korunur; slot değeri dereference edilmez. SlotPresent yalnız sıfırdan farklı word demektir, kritik bölüm veya kilit alma kanıtı değildir. Üst modda cwdSeh.lockPathAllowed=true yalnız selector iznidir; yeni branchAllowed/lazyInitializationAllowed/criticalSectionCallAllowed/lockAcquiredVerified=false. Önceki SEH terminali korunur; DR0 dışında yeni observer müdahalesi ve TEB/context rollback yoktur. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Mevcut/lazy dal, OS kilidi, cwd/SEH dönüşü ve N2/N3/D1/D2 açıktır.

## Kod 0.1.31 bağlantısı

`--observe-cwd-acquire` mevcut/unowned lock(7) nesnesi için doğal dal, admitted ntdll API entry/return ve CRT selector dönüşünü açar; 0x836EA4 terminalinde durur. [Sözleşme ve sonuç](../development/d1-cwd-acquire.md). Beş durakta object/slot/84-byte caller/SEH korunumu ve API sonrası thread sahipliği doğrulanır. x86 24-byte kritik bölüm düzeni pinned Windows uygulamasına aittir; VOID dönüşte EAX başarı kodu sayılmaz. Heap veya aynı GTA image nesnesi için sınır/koruma denetimi vardır. Üst modda branch/criticalSectionCallAllowed=true, acquired readback ile ayrıdır; lazy/directory/unlock kapalı kalır. Önceki lock terminali korunur; kilit tutulurken bütün owned child kapatılır, observer veri/TEB/context rollback yapmaz. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Cwd/SEH/manager dönüşü ve N2/N3/D1/D2 açıktır.
