# Kaynaklar, karşılaştırma ve taşıma sınırı

Durum: resmî belge ve kaynak kodu incelemesi, erişim tarihi 12 Eylül 2026. Bulgular başka platformlara aittir; SAEX implementasyon kanıtı değildir. Wiki sayfaları zamanla değişebilir; yeni kaynak kodu atıfları commit'e sabitlenmiştir.

## İncelenen birincil kaynaklar

| ID | Kaynak | Kullanılan bulgu / sınır |
|---|---|---|
| S-01 | [MTA SetObjectBreakable](https://wiki.multitheftauto.com/wiki/SetObjectBreakable) | Kırılabilirlik API'si mevcut; “hiçbir GTA platformunda yok” denmez |
| S-02 | [MTA BreakObject](https://wiki.multitheftauto.com/wiki/BreakObject) | Kırılabilir objeyi kırma; özel SAEX persistence modelini kanıtlamaz |
| S-03 | [MTA fizik özellikleri](https://wiki.multitheftauto.com/wiki/EngineSetObjectGroupPhysicalProperty) | Native grup seviyesinde fizik davranışı araştırma dayanağı |
| S-04 | [MTA EngineRequestModel](https://wiki.multitheftauto.com/wiki/EngineRequestModel) | Dinamik model kimliği/lifecycle için referans |
| S-05 | [MTA EngineLoadIFP](https://wiki.multitheftauto.com/wiki/EngineLoadIFP) | Özel animasyon library/namespace; dosya yükleme ile network sync ayrı |
| S-06 | [MTA meta.xml](https://wiki.multitheftauto.com/wiki/Meta.xml) | Resource, client/server/shared script ve dosya metadata'sı |
| S-07 | [open.mp AddSimpleModel](https://open.mp/docs/scripting/functions/AddSimpleModel) | Custom basit model DFF/TXD indirme tanımı |
| S-08 | [open.mp AddCharModel](https://open.mp/docs/scripting/functions/AddCharModel) | Custom karakter model dağıtımı |
| S-09 | [FiveM OneSync](https://docs.fivem.net/docs/scripting-reference/onesync/) | Entity sync, culling, ownership ve server creation davranışları |
| S-10 | [FiveM state bags](https://docs.fivem.net/docs/scripting-manual/networking/state-bags/) | Entity/player/global state aktarımı ve yazma politikaları |
| S-11 | [FiveM resource manifest](https://docs.fivem.net/docs/scripting-reference/resource-manifest/) | Resource/script/file/runtime metadata'sı |
| S-12 | [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets/blob/a424b7db649438acafb60c99cae6667587c42732/README.md) | Seçilmiş v1 taşıması; güvenilir mesaj ve şifreleme; peer kimliği ayrı entegrasyon |
| S-13 | [.NET support policy](https://dotnet.microsoft.com/en-us/platform/support/policy) | .NET 10 LTS temel seçimi |
| S-14 | [AssemblyLoadContext](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.loader.assemblyloadcontext?view=net-10.0) | Assembly yükleme izolasyonu güvenlik özelliği değildir |
| S-15 | [.NET unloadability](https://learn.microsoft.com/en-us/dotnet/standard/assembly/unloadability) | Kooperatif unload; yaşayan referansların etkisi |
| S-16 | [AppContainer isolation](https://learn.microsoft.com/en-us/windows/win32/secauthz/appcontainer-isolation) | OS düzeyinde dosya/ağ/süreç sınırları |
| S-17 | [MTA kaynak snapshot'ı](https://github.com/multitheftauto/mtasa-blue/tree/2189a0dbffb57a3f5996b1e58357b2ed028cb386) | [Dosya/satır kaynak envanteri](references/mtasa-architecture.md): resource, streamer/model, syncer, object, event/data, deferred delete |
| S-18 | [GNS genel bağlantı/lane API](https://github.com/ValveSoftware/GameNetworkingSockets/blob/a424b7db649438acafb60c99cae6667587c42732/include/steam/isteamnetworkingsockets.h) | Peer identity uyarısı, lane priority/weight ve yalnız aynı lane reliable sıra güvencesi |
| S-19 | [ENet kaynak tasarımı](https://github.com/lsalzman/enet/blob/5a9c537fd464b3c6d3c55e1d3bd47588faf71b42/docs/design.dox) | Bağımsız channel, reliability, fragmentation, throttle; [karşılaştırma](decisions/network-transport.md) |
| S-20 | [MTA server build rehberi](https://wiki.multitheftauto.com/wiki/Building_MTASA_Server_on_GNU_Linux) | net.dll/net.so binary modül açıklaması; alt transport hakkında ENet çıkarımı yapılmadı |
| S-21 | [OpenUSD introduction](https://openusd.org/release/intro.html) | Katman/referans/varyant ve namespace değişimi sınırı; SAEX authoring tasarım referansı, runtime bağımlılığı değil |
| S-22 | [Epic Gameplay Ability System](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine) | Ability/attribute/effect ayrımı; isteğe bağlı SAEX Actions/Effects referansı, Unreal entegrasyonu değil |
| S-23 | [TUF specification](https://theupdateframework.github.io/specification/latest/) | Update metadata rolleri, freshness/rollback ve güven kökü; R-16 entegrasyon gereksinimi |
| S-24 | [MTA SetPedControlState](https://wiki.multitheftauto.com/wiki/SetPedControlState) | Server ped control state otomatik sync değildir; task/motion ayrımı için referans |
| S-25 | [MTA EngineReplaceAnimation](https://wiki.multitheftauto.com/wiki/EngineReplaceAnimation) | Entity başına replacement, otomatik sync olmaması ve partial animation sınırı |
| S-26 | [MTA EngineReplaceModel](https://wiki.multitheftauto.com/wiki/EngineReplaceModel) | Ped/model replacement; tek load sonucu genel rig/controller doğruluğunu kanıtlamaz |
| S-27 | [MTA GetVehicleType](https://wiki.multitheftauto.com/wiki/GetVehicleType) | Araç sınıfı ayrımı; plane/helicopter varlığı otonom uçuş AI kanıtı değildir |
| S-28 | [Microsoft QPC / zaman damgaları](https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps) | Yerel elapsed time ile UTC senkronizasyonu farklı; SAEX ControlTime tasarım dayanağı |
| S-29 | [.NET TimeProvider](https://learn.microsoft.com/en-us/dotnet/standard/datetime/timeprovider-overview) | UTC/timestamp ve test edilebilir saat soyutlaması; hazır SAEX watchdog değil |
| S-30 | [Microsoft commit/connection resiliency](https://learn.microsoft.com/en-us/ef/core/miscellaneous/connection-resiliency) | Commit sırasında bağlantı kopunca sonucun belirsizliği; EF Core bağımlılığı seçilmedi |
| S-31 | [PostgreSQL explicit locking](https://www.postgresql.org/docs/current/explicit-locking.html) | Advisory/session/transaction lock sınırları; her writer mutation'ında guard gereksinimi SAEX tasarımı |
| S-32 | [Microsoft PE formatı](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format) | D1 salt okunur executable/mimari/section incelemesi; hook adresi veya GTA sürüm kanıtı değil |
| S-33 | [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) | Configure/build/test preset ayrımı; gerçek yerel araç sürümleri workflow'da |
| S-34 | [Dryxio Plugin-SDK-SA snapshot](https://github.com/Dryxio/plugin-sdk-sa/tree/b55e89b336a81448c1aa1a5b188431c9845ebaa9) | [Dosya bazlı inceleme](references/plugin-sdk-sa.md): engine ABI/pool/streaming, sürüm marker'ı, event patch/remove, VS2026/C++latest build ve lisans kapsamı. ADR-35 D1 adayı; runtime doğrulaması yok |

S-17 bir kaynak grubu kaydıdır; incelenen C++ dosyalarının sabit commit/satır bağlantıları [MTA incelemesinde](references/mtasa-architecture.md) tek tek yer alır. Snapshot'lar shallow kaynak incelemesidir; binary derlenmedi veya çalıştırılmadı. Bunlar üretim dependency lock'u seçildiği anlamına gelmez.

S-34, 12 Eylül 2026'da seçilmiş kaynakların metin incelemesidir. Commit başlangıç kaynak pini olarak kabul edildi; bu ilk incelemede makine lock'u/build sonucu yoktu. V0.8 [D1-N1 uygulaması](development/d1-native-dependency.md) 23 dosya, iki patch ve C++23 private x86 build sonucunu ekledi; runtime doğrulaması hâlâ yoktur. Bakımcı README iddiaları, kaynakta gözlenen davranış ve SAEX tasarım kararı [inceleme kaydında](references/plugin-sdk-sa.md) ayrılır. SDK C++latest/VS2026 ayarları yerel C++20/MSVC uyumu kanıtı değildir; root lisansı transitif dosyaların tamamının envanteri sayılmaz.

S-21–23, v0.3'te 12 Eylül 2026 tarihinde okunan resmî belgelerdir. WorldPlan, execution ve projeksiyon sözleşmeleri SAEX tasarım çıkarımlarıdır; bu kaynakların SAEX için hazır uygulama veya performans kanıtı sağladığı iddia edilmez. [Geliştirme kaydı](validation/architecture-evolution.md)

S-28–31 v0.5 için 12 Eylül 2026'da okundu. [Zaman](architecture/time-fencing.md), [kurtarma](architecture/failure-recovery.md) ve [stabilite denetimi](validation/stability-audit.md) kaynak bulgularını SAEX kararlarından ayırır. S-32/33 v0.6 D1 başlangıcında incelendi; [gerçek implementasyon/test](development/d1-foundation.md) dış kaynak bulgusundan ayrıdır. Monoton süre, transaction belirsizliği ve advisory lock kapsamı dış kaynak bulgusudur; bütün SAEX recovery/transfer state machine'i henüz uygulanmadı.

## Kanıta dayalı karşılaştırma

V0.4 için S-09 ve S-05 yeniden, S-24–27 ayrıca 12 Eylül 2026'da incelendi. OneSync nüfus sahipliği ve bucket kontrolü GTA Online tabanlıdır; SAEX'teki PopulationPolicy/Agent/Animals sözleşmeleri kendi tasarımımızdır. Yeni MTA C++ snapshot denetimi, uçuş/hayvan prototipi veya rakip benchmark'ı yapılmadı. [Entegrasyon ve kaynak sınırı](validation/population-animal-integration.md)

| Alan | Mevcut platformlarda görülen | SAEX tasarım hedefi | Henüz iddia edilemeyen |
|---|---|---|---|
| Kırılma | MTA kırılma ve fizik grubu API'leri sunar (S-01–03) | State/collision/fragment/persistence ortak sözleşmesi | Rakiplerden daha hızlı veya daha doğru olduğu |
| Model dağıtımı | open.mp model/kaplama indirme yüzeyleri (S-07–08) | Çok türde closure, exact content, runtime readiness | Tüm native formatların başarıyla yüklendiği |
| Resource | MTA ve FiveM manifest ve taraf ayrımı (S-06/11) | Capability, worker isolation ve transaction reload | Hot reload'un bütün değişikliklerde kesintisiz olduğu |
| Replikasyon | FiveM entity/ownership/culling ve state bag modeli (S-09–10) | Açık otorite seviyeleri, late join ve durable state | Tam sunucu GTA fizik simülasyonu |
| Geliştirici araçları | Bu incelemede kapsamlı araç benchmark'ı yapılmadı | Kimlik zinciri, per-resource profiler ve inspector | Daha düşük maliyet veya daha iyi UX ölçümü |
| Ölçek | Burada eş koşullu rakip benchmark'ı yapılmadı | P1/P2 profillerinde tutarlı sonuç ve bütçe | MTA/open.mp/FiveM'i kapasitede geçtiği |
| Yaşayan nüfus | FiveM nüfus sahipliği/bucket kontrolü, MTA ped/model/animasyon araçları (S-09/24–27) | Kanal bazlı dinamik population, typed task/controller, modüler hayvan ve protected lifetime | Native GTA AI'ın tamamının taşındığı veya genel hayvan/flying controller desteği |

Belgedeki “SAEX tasarım hedefi” özgünlük patenti veya rakipte benzeri bulunmadığı iddiası değildir. FiveM GTA V tabanlıdır; GTA:SA ile farklı motor/hardware şartlarını tek oyuncu sayısına indirgemek anlamlı kıyas değildir.

## Taşıma rehberi taslağı

| Kaynak kavram | SAEX karşılığı | Taşıma işi |
|---|---|---|
| Pawn gamemode / filterscript | C# server resource ve servisler | Callback akışını async command/result'a dönüştür |
| MTA client/server/shared Lua | Ayrı C# worker ve shared DTO | Kod yeniden yazılır; shared otomatik ağ state'i değildir |
| MTA/FiveM resource manifest | Resource manifest + kesin asset lock | Bağımlılık, script dağıtımı ve capability açıklaştırılır |
| Sayısal model ID | AssetRef / native mapping | Paket namespace ve kalıcı kimlik ayrılır |
| Dimension/routing bucket | World instance + portal modeli | Visibility ve geçiş/persistence semantics yeniden ele alınır |
| Entity data/state bag | Şemalı component ve visibility policy | Alan yetkisi/boyutu/replication class belirlenir |
| MTA resource root / element group | Resource lease registry; attachment ağacı ayrı | Resource stop temizliği core kayıtlarından, parent olmak yetki devri değil |
| MTA syncer / time context | Owner lease + OwnerEpoch + Ready | Dünya/catalog/closure doğrulaması ve grup devri eklenir |
| MTA element stream in/out | Native materialization; network AOI ayrı | State handle'dan uzun yaşar; hysteresis/closure ve budget admission |
| MTA remote event caller | Salt okunur ActorContext + typed intent | Payload source entity ile gönderen kimliğini ayır |
| Native client fizik olayı | Impact intent ve server transaction | Güven sınırı ve geç katılım state'i eklenir |

Binary/protokol uyumluluğu ve otomatik script çevirisi ilk sürüm kapsamı değildir. Dosya biçiminin import edilebilir olması script semantics'inin taşındığını göstermez.

## Kaynak ve dağıtım kaydı

Her benimsenen dependency'nin sürümü, lisansı, kaynak adresi ve hangi artifact'e girdiği kaydedilir. Oyun asset'lerinin yerel kullanım, server hazırlama ve yeniden dağıtım izinleri ayrı envanter konusudur. Proje lisansı veya ticari model bu çalışmada seçilmemiştir.

## N2 PE/image gözlemi kaynakları

13 Eylül 2026'da [Microsoft PE biçimi](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format), [CreateFileMapping / SEC_IMAGE_NO_EXECUTE](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga), [VirtualQuery](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualquery), [ReadProcessMemory](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory), [BCryptHash](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcrypthash) ve [DLL başlatma kuralları](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices) incelendi. İlk beşi [N2 parser/reader/CLI](development/d1-engine-preflight.md) tasarımını destekler; son kaynak [0.1.3 bootstrap modülünün](development/d1-bootstrap-module.md) loader sınırına referanstır; oyun dışı modül testi ve gerçek GTA'ya yükleme ayrı kapsamdır. API belgesi veya dosya eşleşmesi GTA runtime/hook desteği kanıtı değildir.

## N2 ilk create-debug süreç gözlemi kaynakları

13 Eylül 2026'da [Windows debug olayları](https://learn.microsoft.com/en-us/windows/win32/debug/debugging-events), [CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw), [process creation flags](https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags), [ContinueDebugEvent](https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-continuedebugevent), [VirtualQueryEx](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualqueryex) ve [job limitleri](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_extended_limit_information) incelendi. [Kod 0.1.4 raporu](development/d1-suspended-process.md) bunları kendi fixture/gerçek dosya gözlemleriyle ilişkilendirir. İlk debug olayındaki duraklatma ve handle yaşamı OS sözleşmesidir; initialized GTA function ABI desteği değildir.

## Native startup metadata kaynakları

13 Eylül 2026'da [Microsoft PE formatı](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format), [.NET 10 PEHeader](https://learn.microsoft.com/en-us/dotnet/api/system.reflection.portableexecutable.peheader?view=net-10.0) ve [Windows DLL arama sırası](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order), [0.1.5 başlangıç raporu](development/d1-native-startup.md) için incelendi. Statik import adı/yerel dosya eşleşmesi, Windows loaded-module kimliği veya transitif güven kanıtı sayılmaz.

## Kod 0.1.6 Windows loader kaynakları

13 Eylül 2026'da [Microsoft Initial Breakpoint](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/initial-breakpoint), [Debugging Events](https://learn.microsoft.com/en-us/windows/win32/debug/debugging-events) ve [LOAD_DLL_DEBUG_INFO](https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-load_dll_debug_info) doğrulandı. Belgeler başlangıç debug durağı, olayların thread'leri durdurması ve file handle sahipliğine dayanak sağlar. File ID/hash politikası, sınırlı üç OS dosyası seçimi ve breakpointCandidate sınıflandırması SAEX tasarımıdır; Microsoft kaynakları GTA unpack/ABI/compatibility veya bizim uygulamanın hatasızlığını kanıtlamaz. [Deney sonucu ve kapsam](development/d1-loader-observation.md).

## Kod 0.1.7 compatibility ve API set kaynakları

13 Eylül 2026'da [Microsoft Application Compatibility Database](https://learn.microsoft.com/en-us/windows/win32/devnotes/application-compatibility-database), [Windows API sets](https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-apisets) ve [WOW64 File System Redirector](https://learn.microsoft.com/en-us/windows/win32/winprog64/file-system-redirector) doğrulandı. Kaynaklar compatibility matching girdileri, API sözleşmesi ile fiziksel modül ayrımı ve x86 dosya görünümüne dayanak olur. Exact 22 dosyalı policy ve first-exception sınırı SAEX tasarımı; yerel imza/hash/registry ve iki launch context sonucu [uygulama raporunun](development/d1-loader-policy.md) kanıtıdır. Registry yokluğundan shim yokluğu veya özel kopya başarısından orijinal kurulum desteği çıkarılmaz.
