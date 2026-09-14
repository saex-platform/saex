# Mimari karar kayıtları

Durum: kabul edilen tasarım varsayılanları. Hiçbiri uygulama veya benchmark kanıtı değildir. [Araştırmalar](research-register.md)

## Kayıt biçimi

Bir ADR; karar, gerekçe, alternatif, sonuç ve yeniden değerlendirme koşulu içerir. Yeni major davranış eski kaydı silmez; yerine geçen ADR referansı eklenir. Kullanıcının kabul ettiği ürün kararları ile uygulama için araştırılan teknik ayrıntılar ayrı tutulur.

| ID | Karar ve gerekçe | Alternatif / sonuç | Yeniden değerlendirme |
|---|---|---|---|
| ADR-01 | Bağımsız platform ve protokol; tam dönüşüm alanı | MTA/open.mp fork'u ilk geliştirmeyi hızlandırabilir fakat taban sınırını taşır | Ürün kapsamı açıkça değişirse |
| ADR-02 | Klasik SA, Windows client; Linux/Windows x64 server | Android/DE farklı engine işleri | Klasik platform kanıtlandıktan sonra |
| ADR-03 | C++20 native core, x86 GTA adapter, x64 host/worker | Her şeyi GTA process'ine koymak adres alanı ve crash etkisini artırır | R-01/02 IPC/engine ölçümü |
| ADR-04 | İki tarafta C#/.NET 10 LTS | Çoklu runtime ilk SDK/test maliyetini artırır | LTS desteği veya workload gereği |
| ADR-05 | Client resource başına OS ile kısıtlı worker | ALC tek başına güvenlik değil; gruplama sert izolasyonu azaltır | R-03; otomatik kod çalıştırma kapısı |
| ADR-06 | Kademeli server authority, açık simulation mode | Tam native fizik reimplementation ilk teslimatı büyütür | R-06/08 yeni doğrulanmış body/controller |
| ADR-07 | V1 oyun taşıması GNS; asset HTTPS; [ayrıntılı karar](network-transport.md) | ENet v1'de yok; GNS peer kimliği ayrı yayın kapısı | R-11 başarısızlığında kanıtlı yeni ADR; sessiz fallback yok |
| ADR-08 | Entity identity, PlacementId, AssetRef ve native handle ayrı | Tek sayı kullanmak cache/reconnect/pool tekrarında hataya açıktır | Kimlik şeması major değişimi |
| ADR-09 | Immutable artifact, kesin lock set, açık override | Son yüklenen paketin kazanması öngörülemez | Uyumluluk gerekçeli yeni resolver sürümü |
| ADR-10 | Hazırlanmış state/fragment yıkımı | Keyfî gerçek zamanlı fracture ayrı araştırma | Kanıtlanmış solver/toolchain bütçesi |
| ADR-11 | Snapshot + journal; durable commit sonrası sonuç | Bütün native input'u event replay etmek deterministik değil | Persistence provider garantisi değişirse |
| ADR-12 | Transactional resource/catalog güncellemesi | Commit sonrası dış yan etkiler otomatik undo değil | Migration ve side-effect modeline yeni kanıt |
| ADR-13 | Oyun kit'leri optional; core genre bağımsız | Zorunlu economy/RP API'si platformu daraltır | Ürün yalnız tek oyun olursa |
| ADR-14 | İlk sunucu tek authoritative world writer | Dağıtık aynı-world physics ek consistency sorunu | 256–512 kapısı ve profil gereksinimi |
| ADR-15 | PostgreSQL referans persistence, SQLite lokal adapter | DB abstraction garanti farkını gizleyemez | Atomic batch/receipt/provider testleri |
| ADR-16 | UI retained scene sözleşmesi, renderer adapter'ı | Zorunlu browser ilk footprint/güven alanını büyütür | R-09 UI ve accessibility ölçümü |
| ADR-17 | [Ortak işlem/aktivasyon sözleşmesi](../architecture/activation-contracts.md): OperationId, ayrı revision'lar, prepare-before-commit ve apply ACK | Bütün makinelerin aynı karede değişmesi varsayılmaz; bounded staging/fencing gerekir | AC-25–31 veya backend/native apply garantisi değişirse |
| ADR-18 | [MTA kaynak ilkeleri](../references/mtasa-architecture.md): lifecycle registry, hysteresis, syncer bağlamı, typed remote veri | C++/Lua akışlarını birebir kopyalamak C# process güvenliği sağlamaz | R-02/03/04/11 ölçümleri ve lifecycle kabulü |
| ADR-19 | [WorldPlan](../architecture/world-plans.md): paket, provider, schema, mutator ve bütçeyi dünya açılmadan çöz | Dinamik yükleme sırası yerine compiler/diagnostics maliyeti | R-13 plan boyutu ve resolver kabulü |
| ADR-20 | [ContractSchema ve execution](../architecture/execution-contracts.md): üretilmiş binding hedefi, immutable okumalar, tek yayın yolu | Code generator/scheduler doğrulaması gerekir; native determinism vaadi yok | R-13/14 ve AC-35/36/37/45 |
| ADR-21 | [Bileşim ve varyant](../assets/composition-variants.md): trait/socket, açık patch ve provenance | Sınırlı authoring model; genel USD runtime zorunlu değil | AC-38/41, R-04/13 |
| ADR-22 | [Actions/Effects](../gameplay/actions-effects.md) isteğe bağlı referans paket | Core gameplay türünden bağımsız kalır; maliyet/iptal/retry ortak tanımlanır | R-15 ve farklı oyun profilleri |
| ADR-23 | [ChangeSet/projection](../networking/world-change-projections.md): kritik durum ve revision bağlı türevler | Tüm sistemler aynı frame'de hesaplanmaz; stale consumer politikası gerekir | AC-40/41, R-06/10/15 |
| ADR-24 | [Studio/release provası](../platform/studio-release.md) ve [conformance](../validation/conformance-contracts.md); TUF uyumlu update metadata | Headless test native kanıt sayılmaz; release güveni peer kimliğinden ayrı | R-16, AC-42/43/44 |

## V0.4 yaşayan dünya kararları

| ID | Karar ve gerekçe | Alternatif / sonuç | Yeniden değerlendirme |
|---|---|---|---|
| ADR-25 | [PopulationService](../gameplay/population-traffic.md): tek server director, dört bağımsız kanal, runtime policy ve grup rezervasyonu | Client başına bağımsız nüfus farklı dünyalar üretir; plan içindeki açık ayarlar canlı değişebilir | R-17/18, AC-46–55/68 |
| ADR-26 | [AgentService](../gameplay/agents-navigation.md): karar/görev/nav/controller/sunum ayrımı, bounded C# provider ve core state | Bütün native AI'ı sunucuda yeniden çalıştırma varsayımı yok; controller bazında kanıt gerekir | R-10/14/17/19, AC-50/51/54/64/65 |
| ADR-27 | [Animals](../gameplay/animals-species.md): tür/cins/morphology/rig/appearance/behavior ayrı, data-driven yeni tanım | Model değiştirme tek başına hayvan desteği değil; yeni beden/hareket native çalışma gerektirebilir | R-19/20, AC-56–62/67/69 |
| ADR-28 | Ortak sync, protected drain ve kalıcı birey/slot sözleşmesi; ControlClass ayrı, ownership sonucu durable | Sıfır gecikme/bit eşitliği yok; nüfus kapatılması aktif etkileşim veya kalıcı sahipliği silemez | AC-48/49/51/59/61/63/66/68/70; authority/lifetime değişirse |

## V0.5 stabilite kararları

| ID | Karar ve gerekçe | Alternatif / sonuç | Yeniden değerlendirme |
|---|---|---|---|
| ADR-29 | [Ayrı zaman alanları](../architecture/time-fencing.md): SimulationTick, monoton ControlTime ve UTC; resume fencing | Oyun stall veya takvim sıçraması lease'i uzatmaz; clock/controller deneyi gerekir | R-21, AC-71/72/84/86 |
| ADR-30 | [Core-owned recovery](../architecture/failure-recovery.md): CommitReceipt/JobResult ayrımı, OutcomeUnknown, WriterTerm, bounded supervisor ve restore closure | Belirsiz işlemlerde scope durabilir; v1 otomatik HA takeover içermez | R-21/22, AC-73–75/82–86/88 |
| ADR-31 | [Onarılabilir replikasyon](../networking/consistency-recovery.md): exact baseline/delta tabanı, Applied ACK, bağımsız motion ve bounded repair | Daha fazla state/diagnostic bütçesi; logical hash native collider kanıtı değil | R-23, AC-78–81/88 |
| ADR-32 | [Tekil entity transferi](../networking/consistency-recovery.md): tek core/atomik backend, canonical location ve phase-aware reservation | V1 cross-server/cross-DB transfer reddeder; outbox tek başına yeterli değil | R-22, AC-76/77/87; dağıtık transfer ayrı ADR |

## V0.6 uygulama başlangıcı

| ID | Karar ve gerekçe | Alternatif / sonuç | Yeniden değerlendirme |
|---|---|---|---|
| ADR-33 | [Kod–doküman ortak değişimi](../development/workflow.md): sahip belge, status ve change-log build kapısında denetlenir | Kullanıcının her ekleme/değiştirme/çıkarma talebi; mekanik gate anlamsal inceleme yerine geçmez | Yeni component veya geliştirme/CI yolu eklenirse |
| ADR-34 | [D1 foundation fixture](../development/d1-foundation.md): generated strong IDs, 44 byte bounded yerel frame; std/BCL dışında runtime dependency yok | x64 C# → x86/x64 native kanıtı; production IPC/GNS wire/sandbox değildir, gerçek SDK iddiası yok | R-03/11c/13'te production schema/broker/codec seçilirken |

## V0.7 native SDK entegrasyonu

| ID | Karar ve gerekçe | Alternatif / sonuç | Yeniden değerlendirme |
|---|---|---|---|
| ADR-35 | [Plugin-SDK-SA](../references/plugin-sdk-sa.md) commit `b55e89b336a81448c1aa1a5b188431c9845ebaa9`, D1 native erişim adayı; private x86 binding, SDK bağımsız bootstrap ve [exact build/engine kanıtı](../architecture/native-sdk-integration.md) | Bütün native erişimi sıfırdan yazmak veya upstream SDK alternatifleri. Seçilmiş fork araştırmayı hızlandırır; toolchain/lisans kapsamı ve hook lifecycle ayrıca doğrulanır. Core C++20, Host/server ve C# SDK bağımsız; R-03/GNS kapıları sürer. Kaynak pini üretim onayı değildir. | D1-N1 build/ABI uyumsuzluğu, R-01b/c güvenli başlatma/stop başarısızlığı, kullanılan kaynağın notice/kanıt eksikliği veya SDK/engine kapsam değişimi |

Bu karar SDK'yi dokümanlara entegre eder; derleme bağımlılığı ve native capability açmaz. İlk uygulama source/patch/recipe lock ve x86 compile/link probe'udur. ADR-03'teki süreç ayrımını ayrıntılandırır, ADR-34 foundation'ın bağımlılıksız test kapsamını değiştirmez. Temiz kapanış session/process stop olarak ölçülür; dinamik DLL unload ayrı kanıt ister. AC-89–96 ve R-01a/b/c, R-02a/b/c, R-04a yeni kabul alt kapsamıdır.

## V0.8 native dependency derleme uyumu

| ID | Karar ve gerekçe | Alternatif / sonuç | Yeniden değerlendirme |
|---|---|---|---|
| ADR-36 | [D1-N1](../development/d1-native-dependency.md): yalnız private x86 SDK probe'u C++23 gerektirir; C++20 deneyi std::expected yüzünden başarısız. Exact CMake/MSVC çifti, 23 dosyalı kaynak lock'u ve iki izlenen patch kullanılır. | Core C++20'yi yükseltmek veya hook bağımlılığını gizleyen sahte header üretmek yerine ayrı vendor build sınırı. MSVC C4458 private adlarla giderildi; C4201 sadece native union tanımında kapsamlı push/pop istisnasıdır. /W4 /WX ve /permissive- korunur; core'a vendor header/flag taşınmaz. | Yeni SDK source/patch/recipe/toolchain, public native ABI eklenmesi veya ilk gerçek bootstrap kapsamı. Bu build sonucu production/GTA onayı değildir. |

## V0.9 engine gözlemi ve runtime kapısı

| ID | Karar ve gerekçe | Alternatif / sonuç | Yeniden değerlendirme |
|---|---|---|---|
| ADR-37 | [N2 preflight](../development/d1-engine-preflight.md): exact dosya/layout/anchor gözlemi ortak bounded JSON'dan C++20 verisine üretilir ve C# araca gömülür. Windows SEC_IMAGE_NO_EXECUTE kontrolü ayrı observation scope'tur. | Disk hash'i veya sürüm marker'ından runtime desteği çıkarılmaz. `ObservedProfileId` desteklenen `EngineProfileId` değildir; canAttach=false, native başarı gözlemi exit 3. SDK bağımsız parser/reader gerçek initializer/unpack/callback kanıtının yerini almaz. | Gerçek process bootstrap, symbol/ABI/faz kanıtı, yeni executable, yeni profile schema veya runtime capability aktivasyonu. AC-90/R-01a/b bu alt sonuçla kapanmaz. |

## V0.10 başlangıç DLL'si

| ID | Karar ve gerekçe | Alternatif / sonuç | Yeniden değerlendirme |
|---|---|---|---|
| ADR-38 | [SDK bağımsız x86 bootstrap](../development/d1-bootstrap-module.md): boş DllMain, constinit session, C ABI 1/192 byte status ve açık Initialize/Query/Stop. Host dosyası/base OS'den alınır; terminal ret/stop ve beklemeden BUSY. | DLL load sırasında otomatik native başlangıç yerine açık dış-loader-lock çağrısı. Caller-provided sağlam dosya başka process'i yetkilendiremez. can_attach/bindings_loaded sıfır; gerçek GTA load/faz kanıtı beklenir. Oyun dışı unload ve warm handle stabilitesi GTA hook unload veya genel sıfır sızıntı kanıtı değildir. | ABI, worker/thread/hook, initializer/dependency, retry veya gerçek GTA load sırası eklendiğinde AC-90/R-01a/b/c ve artifact kanıtı yenilenir. |

## Temel gerekçelerin kaynakları

.NET 10 seçimi [LTS politikasına](https://dotnet.microsoft.com/en-us/platform/support/policy), ALC güven sınırı kararı [Microsoft API açıklamasına](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.loader.assemblyloadcontext?view=net-10.0), taşıma seçimi [GameNetworkingSockets projesine](https://github.com/ValveSoftware/GameNetworkingSockets) dayanır. Entity, yıkım ve kalıcılık kararları SAEX tasarımıdır; bu kaynaklar onların implementasyonunu kanıtlamaz.

## Değişiklik eşiği

Belgelendirilmiş hedefin başarısız testi, hedefi otomatik başarılıya çevirecek eşiğin gevşetilmesine gerekçe değildir. Önce ölçüm, sonra neden analizi, sonra ya optimizasyon ya kapsam/ADR revizyonu gerekir. Tasarım sürümü artırılır ve etkilenen kabul senaryoları yeniden gözden geçirilir.

ABI, codec byte düzeni, solver ve UI backend gibi henüz executable kanıt gerektiren seçimler araştırma kaydında belirli teslim ve stop koşullarıyla yönetilir. Dokümantasyon yazarına belirsiz “sonra bakılır” işi bırakılmaz; implementasyon deneyi o kararın girdisidir.

## ADR-39 — İlk create-debug olayında owned process gözlemi

| Kimlik | Karar | Gerekçe ve sınır | Yeniden değerlendirme |
|---|---|---|---|
| ADR-39 | [Ayrı Windows CLI](../development/d1-suspended-process.md): exact preflight, DEBUG_ONLY_THIS_PROCESS ve ilk debug olayının duraklatması, event file ID/base ve bounded image okuma; owned child kill-before-continue, exit doğrulaması | Dosya mapping ile initialized oyun arasına açık pre-user-code kapsamı ekler. Başka PID, resume, DLL/hook veya belirsiz cleanup başarısı yoktur; canAttach=false. Bootstrap DLL dependency/ABI değişmez. | Başlatma fazını ilerletme, breakpoint/memory write, DLL yükleme, API failure injection, yeni profile veya multi-child/thread desteği; AC-90/R-01a/b ve ardından N3 kanıtı gerekir. |

## ADR-40 — Native başlangıç ortamını executable'dan ayrı gözlemle

| Kimlik | Karar | Gerekçe ve sınır | Yeniden değerlendirme |
|---|---|---|---|
| ADR-40 | [C# statik startup envanteri](../development/d1-native-startup.md): üst seviye DLL/ASI, bounded PE32 import/delay/TLS/entry ve aday grafiği | Mevcut kurulumda native eklentiler var; aynı exe hash'i aynı loader ortamı değildir. Dosyalar değiştirilmez. Tam metadata bile runtime load/clean-install onayı vermez; canAdvanceToLoader=false. | İlk OS loader fazını ilerletme, loaded-module/hash doğrulama, recursive/dinamik dependency çözümü veya clean experiment politikasında yeni AC-90/R-01a/b kanıtı gerekir. |

## ADR-41 — Mapping gözlemini DLL initialization yetkisinden ayır

| Kimlik | Karar | Gerekçe ve sınır | Yeniden değerlendirme |
|---|---|---|---|
| ADR-41 | [Ayrı x86 loader observer](../development/d1-loader-observation.md): exact exe preflight, canlı tutulan volume/file ID/boyut/hash pinleri, bounded LOAD_DLL olayları ve ilk exception'da terminal stop | Default ilk-create CLI aynı kalır. Üç OS dosyalı mapping politikası TLS/DllMain/oyun initialization izni değildir; breakpointCandidate ve canAttach=false açık ayrılır. Statik inventory otomatik allowlist olmaz. Gerçek apphelp mapping'inde ret, fixture'da üç canary ve owned exit kanıtlanır. | Yeni OS/yerel DLL pinleri, compatibility/initialization yürütmesi, breakpoint symbol doğrulaması, cross-bitness veya dış watchdog; N2 evidence yenilenir. SAEX DLL load/unpack/ABI ve N3 ayrıca geçmelidir. |

## ADR-42 — İncelenmiş mapping recipe ve launch context ayrımı

| Kimlik | Karar | Gerekçe ve sınır | Yeniden değerlendirme |
|---|---|---|---|
| ADR-42 | [Exact source recipe](../development/d1-loader-policy.md): engine SHA, 22 DLL origin/ad/byte/hash, first-exception-only ve canonical generated C++; bütün pinler child öncesi hazırlanır | Statik araştırma grafiği otomatik izin olmaz. Yeni açık flag eski üç pinli yolu korur. Apphelp yalnız bounded deney girdisi olarak incelendi; AcLayers eklenmedi. Orijinal kurulum ve bit eşitliğinde özel kopya farklı loader sonuçları verdi; path/name/yan dosyalar değiştiğinden aynı engine hash'i aynı runtime kanıtı değildir. CanAttach/initializationVerified false kalır. | Policy/OS/runtime/module/context değişikliği, normal initialization, proxy/ASI çözümü veya SAEX bootstrap yüklemesi; N2 evidence yenilenir, N3 hâlâ kendi kapısını bekler. |

## ADR-43 — Başlatma ortamını açık ve sınırlı girdi yap

[LaunchContext](../development/d1-launch-context.md) yerel observer'da absolute cwd, retained directory identity ve bounded UTF-16 environment snapshot'ı kullanır. Ortam parent'tan bir defa kopyalanır; key/value loglanmaz, sadece digest/count ve cwd kaydedilir. Eski inherited CLI modları korunur. Aynı engine/policy SHA bütün AppCompat/registry/directory içeriğini tanımlamadığından context metadata ayrı tutulur. Yeni cwd/ortam üretimi, UNC/uzun yol, sanitization/staging veya initialization yürütmesi bu kararı ve AC-90 kanıtını yeniden değerlendirmeyi gerektirir; bootstrap C ABI 1 değişmez.

## ADR-44 — Mapping geçmişini güncel adres sahipliğinden ayır

[0.1.9 ledger](../development/d1-loader-lifecycle.md) fixed 64 lifetime kayıt ve monotonic gözlem içi ID ile unload/remap'i izler. Known-active UNLOAD'a koşullu devam eklenir; unknown/duplicate unload ve event sırası ret verir. Her remap dosya pin/hash denetiminden yeniden geçer; eski mappingId/base yetkisi taşınmaz. Unload budget iadesi yapmaz. İlk exception'ı geçme, budget politikası, thread lifecycle ve native hot unload/ABI kullanımı yeni kanıt gerektirir. Bootstrap ABI ve core entity/otorite kararları değişmez.

## ADR-45 — Statik sembol bağlantısını DLL seçimi ve ABI'den ayır

[0.1.10 karşılaştırması](../development/d1-native-linkage.md) açık iki dosyanın normal/RVA-delay import sembollerini case-sensitive isim veya ordinal üzerinden çözer; EAT holes hiçbir zaman eşleşmez, forwarder yalnız metindir. Her modülün alias listesi bir kez saklanır; compact binding çıktı büyümesini sınırlar. Legacy delay ve bound IAT'de lookup yokluğu açık ret verir. Statik pass yalnız exit 3'tür; canInitialize/canAttach ve calling convention/runtime resolution doğrulaması false kalır. Alternatif DLL'nin aynı isimleri sağlaması, o dosyanın değiştirme/dağıtım/ABI onayı değildir. Yeni dynamic dependency/forwarder resolver, legacy biçim, DLL seçimi/pin değişimi veya initialization yürütmesi bu kararı ve AC-90 kanıtını yeniden değerlendirmeyi gerektirir. Bootstrap C ABI 1 ve GNS kararı korunur.

## ADR-46 — Ayrı DLL initialization deneyi ve erken donanım entry durağı

[0.1.11](../development/d1-entry-boundary.md) mevcut mapping recipe'yi kimlik girdisi olarak korur; initializer yürütmesi için yeni açık --observe-entry-boundary ve compiled executionPolicy gerekir. Main-thread DR0 ilk CREATE_PROCESS olayında kurulur, ntdll initial breakpoint adayında byte/register tekrar doğrulanır. Geç ilk-breakpoint kurulumu fixture main'ini tutamadığından bu yaklaşım reddedildi; başarısız kanıt saklanır. Entry fault'unda byte mutation okunur ve child öldürülür; code patch/restore, IP/stack yönlendirme veya native injection yapılmaz. Başarılı alt gözlem initialized GTA/ABI/SAEX DLL/capability onayı vermez. Yeni OS/context, thread lifecycle, duraktan ileri yürütme, dinamik DLL/ABI çağrısı veya instruction patch bu ADR/AC-90 ve artifact kanıtını yeniden gerektirir; C ABI 1 ve GNS seçimi değişmez.

ADR-46 ek uygulama kararı: initialization izni ayrı entry-policy.json'dan derlenir; exact base/engine hash bağı, 1–8 system-only ek kayıt ve override yasağı zorunludur. İlk imm32 ret sonrası incelenen dosya source'ta hash/byte ile sabitlendi. Bu otomatik learn/allowlist değildir; executionPolicySourceDigest, base digest ve probe SHA ayrı tutulur.

## ADR-47 — Entry yönlendirmesini ayrı ikinci durakla sınırla

[0.1.12 proxy-return](../development/d1-proxy-return.md) yalnız exact entry execution digest + retained proxy module identity + incelenmiş thunk/slot/RVA reçetesiyle açılır. İlk entry hitinde E9 hedefi, 11 byte suffix, executable CALL/JMP şekli ve original-entry pointer doğrulanır. DR0 farklı adresteki dönüş JMP talimatına taşınır; code/IP/stack/EFlags yazımı yapılmaz. İkinci hitte mapping/şekil tekrar doğrulanır, 16 entry byte restorasyonu ve GetStartupInfoA IAT hedefi kontrol edilir, owned process terminal kapatılır. İzin ayrı CLI/API’dedir; eski entry modu ilk hitte durur. Bellek pin’i sandbox değildir; başka thread, hostile native code ve hard I/O deadline güvenliği vaat edilmez. Unpack, asıl engine girişi, dinamik plugin/SAEX DLL/ABI ve N3 ayrıca kanıt gerektirir. Yeni yürütme sınırı, modül hash’i, recipe RVA’sı, debug-register davranışı veya ABI değişimi yeni ADR/AC-90 kanıtı ister. C ABI 1, C++20 ve GNS korunur.

## ADR-48 — İlk startup çağrısını gövde çalışmadan gözle

[0.1.13](../development/d1-startup-call.md) ayrı compiled startup stage ile proxy dönüşünü ilerletir. Üçüncü DR0 hedefi önceki doğrulanmış IAT adresidir; main-thread DR1 aynı dört byte slotta write watch tutar. DR6/DR7/context kimliği doğrulanır; write watch yazımdan sonra terminal trap’tir, önleme/sandbox sayılmaz. İlk fonksiyon hitinde target byte’ları, aktif proxy mapping/IAT, main EXE’de FF15 çağıran ve stack allocation içinde 68-byte output parametre aralığı doğrulanır. Dört anchor örneği değişmiş olsa da okunmuş gözlem olarak raporlanabilir; unpack/initialized profile türetilmez. Gövdeye devam edilmeden owned child kapatılır. Eski modlar aynı kalır; yeni DLL/load/ASI/ABI işlemi henüz açılmaz. Call form, sample/argument kapsamı, farklı thread, breakpoint slotları, dynamic load veya sonraki dönüş sınırına geçiş yeni policy/ADR/AC-90 kanıtı gerektirir. Bootstrap C ABI 1 ve GNS/HTTPS seçimi korunur.

## ADR-49 — İlk dinamik codec çağrısı için ayrı dönüş izni

Durum: kabul edilen geliştirme deneyi, kod 0.1.14/mimari v0.21. Önceki üçüncü startup durağı korunur. Yeni compiled recipe startup parent digest'ine bağlı exact üç codec dosyasını pinler; LoadLibraryA dönüşündeki MOV EDI,EAX öncesinde DR0 durur. Root handle ile yeni aktif mapping kimlikleri eşleşmeden complete sonucu üretilemez. DLL initializer çalışması açık izindir; codec export/ASI/SAEX bootstrap için yetki vermez. Mevcut C++ observer trace/API yeniden derleme ister; bootstrap C ABI 1 ve otorite değişmez. Eski pin listelerini genişletmek yerine yeni modun 26 pin listesi kullanılır; bilinmeyen DLL otomatik kabul edilmez. Sonuçlar [codec sözleşmesinde](../development/d1-codec-return.md), AC-90/R-01 alt testleri [senaryolarda](../validation/scenarios.md); N2/D1 açık kalır.

## ADR-50 — Codec pointer bağlarını ayrı beşinci durakta doğrula

Kabul edilen geliştirme deneyi, 0.1.15/v0.22. Compiled binding policy önceki codec source digest'ine bağlı, aynı 26 pini kullanır. Dördüncü durakta boş slotlar ve bounded direct-export target örnekleri alınır; beşinci durakta exact root+RVA eşitliği ve kararlılık gerekir. Root/dependency unload veya yeni DLL bu izni terminal bitirir. Eski modların durakları ve C ABI 1 aynı kalır; public C++ observer birlikte derlenir. Forwarder/ASI/export invocation/gerçek SAEX bootstrap yeni kanıt ister; bu sonuç engine initialization veya sandbox değildir. [Normatif ayrıntı](../development/d1-codec-bindings.md), [AC-90 alt corpus](../validation/scenarios.md); N2/D1 açık.

## ADR-51 — Tek ASI dönüşü ve build artifact kimliği

Durum: kabul edilmiş geliştirme deneyi, 13 Eylül 2026. Önceki binding iznini genişletmeden ayrı ASI call/return API'si seçildi. Exact wrapper call operand'ı ve stack tam yolu yükleme öncesi; fresh active root/EAX, stdcall stack dönüşü ve önceki codec bağları yükleme sonrası denetlenir. DR2 boş taramada cleanup/whole-image VirtualProtect öncesinde durur. Dinamik artifact hash'i elle kaynak JSON'una taşınmaz: CMake aynı configuration DLL/map auditinden generated pin üretir, probe bu çıktıya bağımlıdır. CLI'ya arbitrary hash/DLL/PID yükleme seçeneği eklenmez. Release/Debug exact CRT pinleri ayrı; bağımlılık keşfi izin üretmez. Bu yaklaşım build/source güveni varsayar; imza, sandbox veya bütün loader lock davranışının kanıtı değildir. C ABI 1 aynı kalır, Initialize DllMain içine taşınmaz. Public C++ API/trace değiştiğinden caller yeniden derlenir; kalıcı migration yoktur. [Sözleşme, alternatiflerin sınırı ve kabul](../development/d1-asi-bootstrap-load.md). Gerçek C ABI invoker sonraki kesittir; N2 bütünü açık kalır.

## ADR-52 — Bootstrap çağrısını ayrı ve terminal deneyle doğrula

[Yeni sözleşme ve doğrulama kaydı](../development/d1-bootstrap-lifecycle.md) ayrı `--observe-bootstrap-lifecycle` iznini tanımlar. ASI dönüşünden sonra aynı build'in audited üç C ABI export'u sekiz sabit çağrıyla çalıştırılır; bounded stack veri yazımı, EIP/ESP yönlendirmesi, status/canary/cdecl kontrolü ve terminal child cleanup uygulanır. Önceki modların durakları korunur; C ABI 1, GNS/HTTPS, engine profile ve otorite değişmez. Bu alt kesit N2/D1 veya playable D2'yi kapatmaz. Dört yerel Windows akışı ve gerçek GTA 12/12 koşuda 96/96 C ABI dönüşü doğrulandı; engine fazı/N3 ve D1/D2 açık.

DllMain worker thread ve wrapper kod yaması tercih edilmedi: başlatma, sahip olunan debug-held main thread üzerinde LoadLibrary dönüşünden sonra yapılır. Deneme sonunda child öldürüldüğünden gameplay devamı veya tam register/FPU restorasyonu iddia edilmez. RVA/prologue kaynağı incelenmiş build artifact'idir; keyfî native adres kabul edilmez.

0.1.17 Release incelemesi: export prefix 20 byte olarak sabitlendi; yalnız PE32 HIGHLOW kayıtlarına göre preferred/actual base farkı uygulanarak tüm byte'lar karşılaştırılır. Kısmi/çakışan fixup, bilinmeyen relocation tipi ve bozuk blok reddedilir; byte maskeleme/atlama yapılmaz. [Kanıt ve sınır](../development/d1-bootstrap-lifecycle.md); C ABI 1 aynı, C++ recipe yeniden üretilir.

## ADR-53 — Frame adayını bayt kanıtından çağrılabilir capability'ye otomatik yükseltmeme

Karar: exact profil ve bootstrap zincirine bağlı ayrı read-only frame-target reçetesi üç mevcut durakta örneklenir. Olumlu sonuç yalnız bu anlarda CALL ve hedef önekinin beklenen değerlerini doğrular. SDK event imzası, faz/thread gözlemi ve doğal call/return ABI kanıtı birbirinden ayrıdır; canAttach=false ve runtimeVerified=false korunur. Eski CLI sınırları değişmez. Alternatif olan adres eşleşmesinden doğrudan hook kurma reddedildi: başlatılmamış subsystem ve yanlış callback fazı riski çözülmemiştir. C ABI 1/otorite değişikliği yok, C++ observer yeniden derlenir. [Sözleşme ve kabul senaryoları](../development/d1-frame-target.md).

## Kod 0.1.19 bağlantısı

0.1.19, aynı profil/pin ve owned child kapıları üzerinde ayrı doğal startup-return deneyi ekler. Mevcut durak davranışı korunur; koruma çağrısı ve dönüş ABI kanıtı yeni raporda izlenir. D1/N2/N3 ve oynanabilir multiplayer kapıları açık kalır. [Sözleşme ve kanıt](../development/d1-startup-return.md).

## ADR-54 — Doğal startup dönüşü için ayrı yürütme izni

Karar: ASI dönüşünden sonra explicit startup-return modu; loaderın exact EXE imageına VirtualProtect çağrısını önkoşul/dönüş/sayfa kontrolleriyle, ardından doğal GetStartupInfoA dönüşünü gözler. Zorlanmış sekiz bootstrap çağrısından sonra kaydedilmemiş FPU/caller state ile devam alternatifi kullanılmaz; yeni dal export çağırmaz ve observer stack/EIP yazmaz. Aynı ASI için ikinci call dahi durdurulur. Bütün-image RWX bu tarihsel loader davranışının deney iznidir, üretim koruma politikası değildir. Frame hook/oynanabilir capability açılmaz; eski CLI ve C ABI 1 korunur, C++ observer yeniden derlenir. [Ayrıntı ve kabul](../development/d1-startup-return.md).

## Kod 0.1.20 bağlantısı

0.1.20 ayrı CRT başlangıç sınırı ekler: doğal I/O dönüşü ve statik başlatıcı çağrısından önce tablo/çağrı/yığın denetimi. Önceki gözlem durakları ve C ABI 1 korunur; motor initialization, renderer, doğal frame ve multiplayer kanıtı açılmaz. [Sözleşme ve kanıt](../development/d1-crt-startup.md).

## ADR-55 — Statik başlatıcıları ayrı yürütme kapısında tutma

Karar: doğal startup dönüşünden sonra I/O hazırlığı dönüşü ve initializer CALL önünde durulur; üç çağrı adayı ve sıfır slotlar dahil bounded tablolar karşılaştırılır. 1674 nonzero slotun bulunması gövdelerinin güvenli veya initialized motorun hazır olduğu anlamına gelmez. Bütün tabloyu denetimsiz çalıştırıp uygulama girişine atlama alternatifi seçilmedi. Sonraki çalışmada initializer callback/dinamik DLL, pencere ve dosya izinleri ayrı incelenir. C ABI 1 aynı, C++ observer yeniden derlenir. [Sözleşme](../development/d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](../development/d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## ADR-56 — Başlatıcıların doğal dönüşünden uygulama girişine ayrı izin

Karar: CRT sınırı terminal davranışını korur; yeni opt-in modu native initializer dispatcher'ını doğal CALL/RET ile çalıştırır. Her callback için tekil ABI/yan etki doğrulaması iddia edilmez. İkinci GetStartupInfoA için wrapper once bayrağı ve bütün prefix doğrulanarak tekrar ASI taraması atlanır; DR2 ikinci ASI koruması korunur. Dört WinMain argümanı ve giriş yığını gözlenir, ilk uygulama komutu çalıştırılmaz. EIP atlatma/forced bootstrap sonrasında devam seçilmedi. N2/N3 açık; C ABI 1 değişmez, observer yeniden derlenir. [Sözleşme ve negatif kabul](../development/d1-application-entry.md).

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](../development/d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## ADR-57 — Host sistem ayarını başlangıç ilerlemesinden ayırma

Exact uygulamanın ilk çağrısı SystemParametersInfoA ile foreground timeout ayarını değiştirmeyi ister. Karar: yeni opt-in prologue modu CALL önünde durur; complete byte/IAT/export, 152-byte stack ve dört argümanı doğrular. Global ayarı değiştirip sonra geri alma; çöküş, eşzamanlı kullanıcı değişimi ve pencere bildirimi nedeniyle bu kesitin parçası değildir. Sonraki kesitte exact çağrıya özgü process-local uyarlama için ABI/last-error/rollback kanıtı gerekir; burada henüz uygulanmadı. C ABI 1/otorite değişmez. [Sözleşme ve kaynak](../development/d1-platform-startup.md).

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](../development/d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.

## ADR-58 — Exact platform isteğinde sentetik dönüşün açık kimliği

İlk SystemParametersInfoA isteği yeni opt-in modunda bağlam işlemiyle bastırılır. Sonuç FALSE ve last-error korunumu SAEX sözleşmesidir; doğal API dönüşü/Windows başarısızlık emülasyonu iddiası değildir. Exact IAT/export ve FS-reader kanıtı önkoşuldur. Kısmi write/readback hatasında aynı durakta rollback denenir; devam sonrası bilinmeyen fazda yalnız child sonlandırılır. Başarıda return-site breakpoint ve restore readback gerekir; bu duraktan oyun sürdürülmez. Genel hook, disk patch veya bootstrap ABI genişletmesi seçilmedi. [Sözleşme](../development/d1-platform-suppression.md).

0.1.23 bağlam geri alması uygulama register/segment/flag ve breakpoint adres/izinlerini kapsar. EFLAGS sabit bit 1 normalize edilir; DR6 olay nedeni restore sırasında sıfırlanır. RF, return-site breakpoint olayından ayrı değerlendirilir. Bu özel ayrımlar dışındaki context farkı ret üretir; [ayrıntı](../development/d1-platform-suppression.md).

## ADR-59 — Instance nesnesini doğal oluşturma, pencere kolunu kapalı tutma

Yeni bounded observer modu yalnız hash’i sabit named-event helper’ını yürütür. NULL handle başarı sayılmaz; handle türü ve isimle açılan event kimliği karşılaştırılır. Mevcut instance 183 sonucunda pencere arama/öne getirme kolundan önce durulur. Yeni event için exact profil last-error 0 ve doğal helper EAX=0/ESP/nonvolatile dönüşü ister. Host ayarı önceki sentetik sözleşmeyle bastırılır, ancak gerçek API yürüdükten sonra eski CALL bağlamı restore edilmez. Önceki restore modunun tam verified alanı bu modda false, bağımsız suppressionBoundaryValidated alanı true olur. Oturum ortak namespace etkisi, aynı isimde başka tür çakışması ve son handle ömrü açıkça kaydedilir; production çoklu-instance yeteneği çıkarılmaz. [Sözleşme](../development/d1-instance-startup.md).

## ADR-60 — Olay dağıtıcısı ile uygulama işleyicisini ayrı yürütme kapıları yapmak

Karar: instance sonrası `[24,nullptr]` çağrısı, dağıtıcı entry ve dağıtıcının 12-byte prologue aktarımı doğal olarak sınanır. AppEventHandler CALL çalıştırılmaz. Yerel executable bu işleyicide kaynak örneğinden farklı bir JMP içerdiği için sembol adı/prefix eşitliği initializer çalıştırma yetkisi değildir. İki CALL bağı ve branch/prologue exact; üç yığın şekli ve register değişimi portable doğrulayıcıya bağlıdır. Caller durumu ve named event kimliği korunur. Önceki mod sınırı değişmez; C++ observer yeniden derlenir, C ABI 1/otorite aynı. [Sözleşme ve sonraki araştırma](../development/d1-event-dispatch.md).

## ADR-61 — Executable yönlendirmesini ve indirect olay seçimini bağımsız doğrulamak

Karar: 0x53EC1F detour kodu yalnız yerel exact executable incelemesiyle tanımlanır. Event 24, 39-byte index tablosundan 5 değerini, 11 hedefli tablodan 0x53EC2B initializer CALL adresini seçer. Entry/detour/JMP/initializer CALL durakları arasında yalnız incelenmiş register işlemleri, bellek okumaları ve doğal CALL/branch çalışır. İlk initializer CALL yürütülmez. Bütün tablonun relocation kimliği, seçilen index/target, caller yığını ve parent kapıları yeniden denetlenir. Unselected tablo girdileri yürütme yetkisi değildir; başka event kullanımı ayrı inceleme/policy gerektirir. C++ observer yeniden derlenir, C ABI 1/otorite değişmez. Önceki terminal korunur; N2/N3 kapıları kapanmaz. [Sözleşme](../development/d1-application-routing.md).

## ADR-62 — İlk native veri yazımını sınırlı helper sözleşmesiyle gözlemek

Karar: CGame::InitialiseOnceBeforeRW içine doğal CALL sonrası yalnız 0x72F3B0 RET ve 0x56D180 yerelleştirme gövdesi yürütülür. Gövdenin üç absolute adresi relocation ile bağlanır; flags penceresi writable MEM_IMAGE olmalıdır. Beş durakta dönüş/stack/register/flags, caller ve last-error denetlenir. İlk dört durakta veri penceresi aynı; son durakta yalnız üç bayrak [1,0,0] olur. Observer oyun veri belleğini yazmaz veya eski değerlerle rollback yapmaz; owned child sonlandırılır. Bu doğal bellek yan etkisi initializer tamamlanması, memory manager kapasitesi veya production attach kanıtı değildir. Önceki terminal korunur; C++ observer yeniden derlenir, C ABI 1/otorite değişmez. [Sözleşme](../development/d1-game-prelude.md).

## ADR-63 — Cwd yürütmesini buffer ve CRT dönüş kapısından ayırmak

Karar: CFileMgr::Initialise içine doğal giriş ve üç PUSH yürütülür; CRT cwd CALL önünde durulur. Exact 51-byte gövde, root/suffix operandları, parent zinciri, iki ABI frame ve 136-byte writable image penceresi doğrulanır. Sonraki lock/SEH/OS/copy çağrıları bu izin kapsamına girmez. Yerel kod CRT dönüşünü kontrol etmeden NUL tarar ve iki-byte suffix yazar; gelecekte başarılı dönüş, en geç offset 126 NUL ve ANSI byte sınırı sağlanmadan döngü açılmaz. Önceki terminal korunur, C++ observer yeniden derlenir; C ABI 1/otorite aynı kalır. [Sözleşme](../development/d1-file-manager-entry.md).

## ADR-64 — CRT SEH kaydını doğrulamak, handler yürütmesini ayrı tutmak

Karar: Cwd wrapper ve tam 59-byte SEH prologue doğal yürütülür; FS:[0] yeni stack kaydına bağlandığında üç durakla kanıtlanır. 80-byte yığın, 28-byte NT_TIB, önceki ilk kayıt, caller ve buffer korunur. TIB adresi mevcut x86 FS/Self kanıtından gelir; internal layout genel Windows API sözleşmesi sayılmaz. Handler/cleanup, lock ve cwd OS yolu çalıştırılmaz; beklenmeyen exception child kapanışıyla sonuçlanır. Doğal kayıt yazımı unwind veya cwd dönüş kanıtı değildir; eski TEB/context rollback yapılmaz. C++ observer yeniden derlenir, C ABI 1/otorite aynı. [Sözleşme](../development/d1-cwd-seh.md).

## ADR-65 — CRT kilit kaydını seçmek ve kilit almayı ayrı doğrulamak

Karar: lock(7) selector doğal yürütülür; slot karşılaştırmasından sonra JNE önünde durulur. Boş/dolu slot iki dalı da açmadan aynı doğrulama sınırını kullanır. 100-byte stack, 16-byte slot, SEH/NT_TIB ve dış caller korunur. Pointer opaque tutulur; slotPresent kilit nesnesi/başlatma veya acquire kanıtı değildir. Böylece önceden okunan bir pointera dayanarak yarış sırasında başka dala kaçma izni verilmez. Mevcut/lazy dal, OS target ve kritik bölüm sahipliği sonraki sözleşmedir. Yeni observer yazımı yalnız DR0; eski mod ve C ABI 1/otorite aynı. [Sözleşme](../development/d1-cwd-lock.md).

## ADR-66 — Mevcut CRT kritik bölümünde doğal acquire ve sahiplik readback

Karar: boşta ve mevcut slot dalından admitted ntdll EnterCriticalSection hedefi doğal çağrılır; beş checkpoint ve kritik bölümün 24-byte before/after durumu ile ana thread sahipliği doğrulanır. EAX VOID API için başarı sinyali değildir. Heap ve aynı GTA image içindeki writable nesneler kabul edilir; başka image, stack/TIB, geçersiz/held/semaphore nesnesi ret verir. Internal LockCount -1/-2 beklentisi bu OS pinine özeldir. Windows iç yardımcılarının tamamı denetlendi iddiası yoktur; gerçek fonksiyon sözleşmesi ve pinned implementation kullanılır. Kilit tutulurken yalnız bütün owned child kapatılır; gameplay devamı/unlock/SEH unwind izni yoktur. C++ trace değişir, C ABI 1/otorite aynı. [Sözleşme](../development/d1-cwd-acquire.md).

## 14 Eylül 2026 — Windows fixture taşınabilirliği

Startup-return ve devamındaki fixture zinciri, sistem DLL reçetelerini test makinesinin diskte tutulan PE dosyalarından çıkarır. Ortak okuyucu ve negatif doğrulamalar [test kapsamı notunda](../development/d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği) açıklanır; üretim policy/hash kuralları ve bu belgedeki gerçek GTA kanıtının sınırları aynıdır.
