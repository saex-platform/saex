# Native SDK entegrasyonu ve doğrulama sözleşmesi

Durum: **mimari v0.30 / kod 0.1.23; N1 build ve N2 sınırlı loader/ASI/bootstrap/frame adayı, doğal startup dönüşü, CRT sınırı, doğal uygulama girişi ve host-setting CALL öncesi platform başlangıcı ve ayrı sentetik dönüş/restore alt kanıtları mevcut. Initialized GTA, doğal frame ve N3 bekliyor.** [Motor adapter](engine-adapter.md) · [Uygulama sırası](../development/d1-engine-integration.md) · [Güncel durum](../development/status.md)

### Kod 0.1.4 süreç gözlemi sözleşmesi

[Ayrı CLI ve observer](../development/d1-suspended-process.md), SDK/DLL yüklemeden exact dosyayı kendi askıya alınmış child process'iyle file ID ve image base üzerinden eşler. İlk create-debug olayı tutulur; bounded header/anchor okumasından sonra owned child öldürülür, ardından debug exit tüketilip process çıkışı doğrulanır. Başka PID, ResumeThread, memory write, breakpoint veya injection girişi yoktur. Construct/read/stop/destroy aynı dedicated observer thread'indedir. Bu araç bootstrap modülüne linklenmez; mevcut C ABI değişmez. Gözlem sonucu runtime active değildir ve retained dosya profili observation-only kalır. AC-90/R-01a/b alt kapsamı ayrı scope'ta raporlanır.

## Amaç ve sorumluluk sınırı

Dryxio/plugin-sdk-sa, klasik GTA'nın C++ sınıf, fonksiyon ve hook eşlemeleri için D1 başlangıç bağımlılığı olarak seçilir. İlk build/profil/lifecycle doğrulaması D1-N1/N2/N3'ü; üretimde kullanım ayrıca kullanılan capability kanıtlarını ve ilgili release kapılarını bekler. Native erişim yalnız Windows x86 adapter'ın private uygulamasındadır. C++20 core, x64 Host/server, .NET 10 SDK/worker ve oyun protokolü Plugin-SDK tiplerini veya header'larını tüketmez.

Tasarlanan çağrı yolu: **yetkili Host komutu → sürümlü ve bounded IPC → adapter doğrulaması → güvenli frame → seçilmiş SDK binding'i → GTA**. Dönüşte kopyalanmış sonuç ve SAEX kimliği gönderilir. `CPed*`, `CVehicle*`, RenderWare pointer'ı, vtable, STL nesnesi ve raw oyun adresi süreç sınırını geçmez. `Native.Call(address)` veya sunucudan gönderilen hook tanımı SDK'ye eklenmez. GNS Host/server'da, asset HTTPS hattında kalır.

## Bağımlılık edinimi ve build sınırı

Başlangıç kaynak pini `b55e89b336a81448c1aa1a5b188431c9845ebaa9`'dur. D1-N1, mevcut [build akışına](../development/workflow.md) açık SDK edinme/build girişi ve [makine lock'u](../../contracts/engine/plugin-sdk.lock.json) ekledi. 23 dosyalı orijinal envanter, iki exact patch ve ayrı build cache doğrulanır; yalnız PluginBase.cpp ve CPool layout/link probe'u tüketilir. Normal core build'i SDK indirmez; opt-in native hedefi exact source/lock yoksa açık hata verir.

Lock kaydı en az upstream URL, tam commit, edinilen arşiv kullanılıyorsa SHA-256, kaynak envanteri digest'i, sıralı yerel patch digest'leri, build recipe digest'i, compiler/Windows SDK/generator sürümleri, x86 hedefi, CRT/packing/calling convention/define ayarları ve notice envanterini taşır. Arşiv hash'i ölçülmeden yazılmaz. Header, library ve symbol dosyaları aynı lock/build'den gelir; başka checkout'tan hazır `.lib` karıştırılmaz. Branch veya tag tek başına pin değildir.

SDK adapter'ın private build hedefidir. GTA-SA ve gerçekten kullanılan shared/transitif dosyalar envanterle alınır; bütün oyun dizinlerini veya örnek ASI'leri linklemek varsayılan değildir. Upstream installer otomatik çalıştırılmaz. Kaynaklar değiştirilmeden tutulur; gerekli uyarlamalar açık patch serisidir. İlk C++20/MSVC 19.44 denemesi SafetyHook header'ının std::expected gereksiniminde başarısız oldu. ADR-36 ile bağımsız private hedef C++23'e alındı; kilitli CMake/MSVC çifti /std:c++latest üretir. Core C++20 kalır. Injector private üye adları ve yalnız native bit-field union'ını kapsayan C4201 push/pop iki izlenen patch'tir; /W4 /WX ve /permissive- korunur. [Başarısız deney ve uyarlama kaydı](../development/d1-native-dependency.md). Upstream'in C++latest, statik CRT veya uyarı ayarları core'a yayılmaz; uyumsuzluk çözülmeden flag gevşeterek kapı geçirilmez. Farklı vendor dil/araç gereksinimi çıkarsa kapsamı ve ABI sınırı belgelenerek karar güncellenir.

`DependencyLockDigest`, adapter artifact SHA-256 ve symbol/PDB referansı kanıt kaydına girer. Kaynak pinlemek imza/kaynak güveni veya runtime doğruluğunun kanıtı değildir; platform dağıtımı [release güven modelini](../platform/studio-release.md) ayrıca karşılar.

## EngineProfile ve symbol kanıtı

Yerel dosyanın mevcut SHA-256'sı ve ret durumu [engine-adapter](engine-adapter.md) içinde kayıtlıdır. Dosya adı/boyutu veya SDK'nin `GetGameVersion()` sonucu profile onay vermez. Compact ve Hoodlum ayrı executable adaylarıdır; biri geçince diğeri açılmaz. `EngineProfileId` GTA sürüm metninden ve SDK sürümünden ayrı kimliktir.

Her profil aşağıdaki alanlar tamamlanmadan deneyde aktif hale gelmez. [N2 gözlem kaydı](../development/d1-engine-preflight.md) yalnız dosya/hash/layout ve kısa anchor alanlarını uygular; runtime/symbol ABI/hook/build/capability tam profili hâlâ hedef sözleşmedir.

| Kayıt | Zorunlu içerik ve kontrol |
|---|---|
| Executable | Tam dosya SHA-256, byte boyutu, PE machine/format, image/section sınırları, beklenen yükleme düzeni |
| Native symbols | Kullanılan symbol adı, kaynak commit/dosya, kanıtlı RVA veya adres çözümleme kuralı, section/erişim türü, layout/calling convention ve araştırma gözlemi |
| Hook sites | Beklenen instruction/byte önkoşulu, kanıtlı relocation/unpack aşaması, patch kapsamı, original target, restore/quiescence gereksinimi ve sahiplik |
| Build | AdapterApiMajor, DependencyLockDigest, adapter artifact hash'i, compiler/CRT/packing ve kullanılan loader/bootstrap sürümü |
| Capabilities | Her operation/native sınıf için `declared/experimental/verified/unsupported`, test ve EvidenceRecord bağlantısı; gözleme/bastırma/uygulama ayrı |

Assembly/decompilation incelemesi kullanılan signature, stack/register ve adresin bu executable'a ait olduğunu göstermelidir. Pattern kullanılacaksa aranan image/section aralığı, exact match sayısı ve doğrulanan talimat koşulları kayıtlıdır; geniş taramada bulunan ilk benzer byte dizisi çağrılmaz. Sıfır adres, aralık dışı veya erişim türü uyumsuzluğu açık rettir. SDK'deki destek makrosu tek başına symbol kanıtı değildir.

Dosya preflight'ı ile process içindeki kontrol birbirini tamamlar: doğrulanan yerel dosya ile açılan image eşleşir; SDK'ye temas etmeden önce güvenli sınır/erişim kontrolleriyle gerekli mapped image ve hook önkoşulları tekrar sınanır. Değişiklik olmuşsa ret verilir. Self-modifying/unpack davranışı yalnız profil için kanıtlanan aşamada ele alınır. Bu uyumluluk denetimi düşmanca process'e karşı anti-cheat garantisi olarak sunulmaz.

### Uygulanmış N2 gözlem sözleşmesi

ADR-37 ile `ObservedProfileId` dosya gözlemini adlandırır; `EngineProfileId` gibi verified runtime yetkisi vermez. Aynı bounded JSON native constexpr veriye üretilir ve C# araca gömülür; kaynak digest'leri eşleşmelidir. Tam SHA-256, PE bölüm indeksleri ve kısa anchor kontrolleri marker tek başına kabulünü önler. Windows probe dosyayı `SEC_IMAGE_NO_EXECUTE` ile kendi sürecinde eşler; başlıklar ve dört RVA karşılaştırılır. Bu sonuç gerçek GTA'nın başlatılmış/unpack edilmiş image'ı veya aşağıdaki `mapped_image_verified` geçişi değildir. Native gözlem exit 3 ve `canAttach=false` verir; bilinmeyen dosya mapping öncesi reddedilir. SDK bağımlılığı veya yükleme çağrısı yoktur. [Kaynaklar, negatif testler ve hata sınıfları](../development/d1-engine-preflight.md).

## SDK çalışmadan önce bootstrap kapısı

İlk bootstrap modülü Plugin-SDK linklemez. Global/static initializer, TLS callback veya `DllMain` yolunda oyun adresine erişim ve hook kurulumu bulunmaz. Loader lock altında ağır iş/IPC bekleme yapılmaz. SDK bağımlı modül ancak SDK'den bağımsız preflight ve process içi profil önkoşulları geçince kontrollü şekilde yüklenir; o modülün statik başlatmaları da kaynak/link envanterinde denetlenir. Yalnız `DllMain` içine if koymak daha erken initializer erişimini engelledi sayılmaz.

Hedef durum akışı `discovered → preflight_passed → mapped_image_verified → bindings_ready → observing → active → draining → stopped` şeklindedir. Her geçişin evidence ve neden kaydı vardır. Profil/ABI/hook uyuşmazlığı `rejected`, aktif oturumda bağlam kaybı `fenced` sonucuna gider. `observing` yalnız yerel deney içindir; gameplay kabulü veya production Join açmaz. Mevcut inspector bu akışı uygulamaz ve `canAttach=false` kalır.

İlk deney yalnız init/frame/kapanış gözlemi içerir. Event adına bakılarak uygun komut fazı seçilmez; thread ve önce/sonra sırası ölçülür. Bütün zorunlu hook siteleri mutasyondan önce doğrulanır; kayıt sırasında kısmi hata için geri alma listesi tutulur. Başka modun değiştirdiği site sessizce üzerine yazılmaz; ancak bu kombinasyon için ayrı kanıt varsa destek profiline alınabilir.

### Kod 0.1.3 başlangıç modülü

[Uygulanan x86 DLL](../development/d1-bootstrap-module.md) yalnız C ABI 1 Initialize/Query/Stop export'larını açar. DllMain boş, session constinit'tir; CRT loader yardımcıları ayrıca envanterlenir. Initialize kendi host executable yolunu OS'den bulur; path/PID/image base çağırandan alınmaz. Ortak dosya/hash/PE kapısından sonra actual ana image base/header/anchor kontrolü gelir; relocation/unpack farkına tolerans yoktur. Eşleşme yalnız observed-unverified sonucudur, can_attach=0 ve bindings_loaded=0 kalır.

Çağrı boyutu/major hatası output/state/IO'yu değiştirmez. Tek gözlem denemesi, terminal rejected/stopped ve beklemeden BUSY uygulanmıştır. Stop sırasında işlem varsa BUSY döner; DLL sahibi in-flight export'ları bitirmeden FreeLibrary yapamaz. Bu state machine yukarıdaki bütün hedef lifecycle'ı açmaz. Oyun dışı DLL unload testi gerçek GTA hook unload capability'si değildir; gerçek loader/phase/SDK initializer envanteri R-01a/b/c kapsamında açık kalır.

## Hook ömrü ve temiz kapanış

Her hook'un sahibi, kurulum kaydı, etkin callback sayacı, original bytes/target ve kaldırma koşulu vardır. Callback sırasında callback listesini değiştirme, recursive giriş ve destroy/reload davranışı açıkça sınanır. SDK'de callback remove çağrısının bulunması detour/trampoline'ın kaldırıldığı anlamına gelmez.

Kapanışta yeni komut kabulü kesilir; grants/epoch geçersizleştirilir; bounded işler tamamlanır veya stale olarak bırakılır; yerel lease ve sahip olunan entity'ler güvenli fazda retire edilir; callback erişimi kesilip quiescence doğrulanır. Hook yalnız hâlâ bizim kurduğumuz patch ise geri alınır. Başkasının sonradan değiştirdiği siteye eski byte'lar yazılmaz. Geri alınamayan hook'a ait kod belleği serbest bırakılmaz; sonuç ve gereken process kapanışı raporlanır.

D1 temel desteği kontrollü session stop ve oyun process'inin temiz kapanışını kanıtlar. **Çalışan GTA'dan native DLL'yi dinamik boşaltma ayrı capability'dir**; quiescence ve tam unpatch testi olmadan açılmaz. Process'in normal çıkması hot unload testi yerine yazılmaz. Bu ayrım eski genel attach/unload hedefini somutlaştırır; kapanış veya başarısız bootstrap temizliği atlanamaz. Orijinal oyun dosyaları değişmez; yalnız doğrulanmış process içinde geçici hook uygulaması bu sözleşmenin konusudur.

## Frame, IPC ve entity kuralları

Host/adapter arayüzü major sürüm, session/world/resource epoch, sequence, boyut/adet sınırı, readiness ve grant kontrolleri taşır. Foundation'ın 44 byte EntityRef fixture'ı üretim IPC değildir. Wire alanları sabit genişlik ve açık byte sırasıyla tanımlanır; pointer genişliği veya C++ struct packing'i wire formatı belirlemez. Adapter hataları typed sonuçtur; C++ exception/allocator sahipliği ABI sınırından geçirilmez.

Native GTA erişimi doğrulanmış oyun thread/fazıyla sınırlıdır. Hook callback'i bounded kopya/veri kuyruğu üretir; disk, HTTPS, socket, C# veya Host cevabını beklemez. Staging ve komut maliyeti ayrı ölçülür; [execution](execution-contracts.md) ve [zaman](time-fencing.md) bütçeleri geçerlidir. Host kaybı, lease expiry veya suspend dönüşü yeni yazımı fence eder; eski komutlar resume sonrasında tekrar oynatılmaz.

Adapter registry, EntityRef'i native tür/slot/referans ve kendi monoton binding generation'ıyla eşler. Her native kullanımda geçerlilik yeniden kontrol edilir; slot tekrar kullanılmışsa eski iş reddedilir. Native lifetime gözlenemiyorsa ilgili entity capability açılmaz. Native pointer yalnız geçerli frame kapsamındaki ödünç erişimdir; kalıcı kimlik veya worker verisi olmaz. Create/destroy/rebind, pool full ve generation exhaustion ayrı sonuçlar üretir. Property matrisi `live-set/recreate-required/unsupported` sınıflarını korur; SDK setter'ı shared handling/physics grubunu entity başına özellik yapmaz.

## Asset, dünya ve üst sistemler

Model isteği `EntityRef + ResourceEpoch + CatalogRevision + LeaseId` ile izlenir. İptal/reload/retire sonrası tamamlanan istek yeni nesneye bağlanmaz; referans bırakma dengeli ve ölçülüdür. Native request, yükleme tamamlanması, collision hazırlığı ve görünür aktivasyon ayrı adımlardır. Main loop'ta toplu yüklemenin gecikmesini saklayarak asenkron başarı raporlanmaz. İlk deney oyuncunun yerel stok modelini kullanabilir; DFF/TXD/COL/IFP özel parser ve dağıtım kanıtı ayrı R-04/R-12 kapsamıdır.

SDK'de physics/task/animation fonksiyonunun bulunması; sunucu otoritesi, stok spawn bastırma, yıkımın geri kurulması, ortak NPC veya hayvan rig/controller desteği değildir. R-05/06/07/08/10/17/18/19 kapıları ilgili native sınıf ve operation için ayrı kanıt ister. Sunucunun canonical state'i core'da kalır; GTA gözlemi doğrulanmadan accepted state olmaz.

## Kanıt, güncelleme ve geri dönüş

Her native EvidenceRecord; engine dosya hash'i, EngineProfileId, DependencyLockDigest, adapter artifact hash'i/API major, bootstrap/loader ve Host build'i, capability/operation/native sınıf, kullanılan asset digest'i, ortam, AC/R kimliği, expected/observed, log referansı ve sonucu taşır. Hook kurma/kaldırma sayısı, callback drain, queue peak/drop, frame p50/p95/p99/max ve entity/model referans sayaçları ilgili deneyde eklenir. `not_run`, `fail` ve `unsupported` pass'e çevrilmez. Oyuncu yolu sade hata gösterir; diagnostic sır veya oyun dosyasının kendisini içermez.

SDK commit/patch/recipe/CRT, engine hook seti veya adapter build'i değişince etkilenen eski kanıt kendiliğinden geçerli olmaz. Etki haritası belirliyse ilgili alt testler; belirsizse kullanılan native kapsam yeniden çalışır. Güncelleme aday branch/build'inde denenir; eski lock ve artifact korunur. Yerel bağımlılık geri dönüşü platform release güveni ve state/schema uyumluluğunu geçersiz kılamaz. Canlı DLL hot swap yerine yeni process başlangıcı kullanılır; otomatik indirilen server içeriği bu bağımlılığı değiştiremez.

Kabul: [AC-89–96](../validation/scenarios.md); temel R-01a/b/c, R-02a/b/c ve R-04a. D1-N1'in build sonucu AC-89'un seçilmiş alt kümesidir; hiçbir runtime kapısını kapatmaz.

## Kod 0.1.5 loader öncesi ortam sözleşmesi

Metadata-only parser, güvenli raw/image aralığı içindeki hizalama padding farkını LayoutNotes ile kaydedebilir; bu genel PE geçerliliği veya engine desteği değildir. C++ native preflight kuralları değişmez.

[Statik native başlangıç envanteri](../development/d1-native-startup.md), ayrı C# geliştirme aracıdır. Aynı EXE kimliğinin yanında bulunan DLL/ASI adaylarını, import/delay ilişkilerini, entry/TLS ve extension işaretlerini kaydeder. Bozuk adaylar sessizce atlanmaz; metadataComplete=false ve exit 1 verir. Null yerel aday Windows system resolution kanıtı değildir. İlk process görüntüsü doğrulandı diye mevcut modlu kurulumda yükleme fazı kendiliğinden ilerletilemez. Runtime modül kimliği ve başlatma politikası ayrıca kanıtlanır; hiçbir gözlem snapshot'ı native aktivasyon yetkisi değildir. Kullanıcının kurulumuna otomatik temizleme veya dosya değiştirme yoktur; bootstrap C ABI ve SDK kaynak kilidi değişmez.

## Kod 0.1.6 — Ayrı loader deneyi

ADR-41 ve [loader sözleşmesi](../development/d1-loader-observation.md), ilk create olayından ilerlemeyi yalnız açık x86 deney yolunda açar. İzin, DLL mapping dosyasının canlı tutulan pinle volume/file ID/boyut/hash eşleşmesidir; DLL initializer veya native binding çalıştırma izni değildir. İlk exception tutulur; ana thread + first-chance breakpoint + kabul edilmiş ntdll MEM_IMAGE eşleşmesi yalnız aday fazdır. Unpinned/okunamayan modül ve budget/olay hatası terminaldir; kill-before-terminal-continue ve doğrulanmış exit zorunludur. Varsayılan SuspendedImage/process CLI ilk olayı ilerletmez; bootstrap DLL bu loader kodunu linklemez. Yerel GTA modları ve apphelp otomatik kabul edilmez. Statik envanter metadataComplete sonucu hâlâ canAdvanceToLoader=false kalır; yeni CLI'nin ayrı deney yetkisiyle karıştırılmaz.

Loader döngüsü 128 olay/64 modül/16 thread/256 MiB/5 saniye ile sınırlı, cleanup ayrı 5 saniyedir. Senkron file I/O katı wall-time garantisi vermez. Çalışan OS/shim koduna karşı sandbox ve tam mapped-image bütünlüğü kanıtı yoktur. Başka PID/thread debug oturumuna müdahale veya production Join/activation yolu bulunmaz.

## Kod 0.1.7 — Reviewed mapping recipe ve bağlam

ADR-42 ile [compiled recipe](../development/d1-loader-policy.md) exact engine hash'ine, 22 DLL'nin origin/ad/boyut/hash'ine ve first-exception-only sınıra bağlanır. PreparedLoaderPolicy bütün recipe'yi IO öncesi, tüm dosyaları child öncesi doğrular; partial pin seti açılamaz. System-x86/game-root dışı origin veya fallback yoktur. Drift/eksik DLL ve bütçe hatası child yaratmaz. Source JSON runtime'da veya server'dan okunmaz; generator strict/canonical --check kapısındadır. Eski üç dosyalı mod ayrı kalır; Apphelp yalnız yeni incelenmiş mapping profilinde yer alır. AcLayers onaylanmadı.

Sadece exe hash'i, launch context kanıtı değildir. Orijinal kurulumun AcLayers reddi ve aynı byte'lı üç dosyalı özel kopyanın Debug 22/Release 21 mapping/breakpoint adayı farklı evidence'dir; path/name/yan dosyalar aynı tutulmadığından tek değişken sonucu çıkarılamaz. Windows/shim kodu olaylar arasında çalışabilir. Oyun initializer/dinamik proxy/ASI, gerçek SAEX load, entry-unpack ve ABI araştırması tamamlanmadan observing/active native capability açılmaz.

## V0.15 — N2 explicit launch context

[LaunchContext sözleşmesi](../development/d1-launch-context.md), cwd ve Unicode environment görüntüsünü child öncesi doğrular; directory handle kimliği korunur ve mevcut file/hash mapping pinleri ayrıca uygulanır. Environment key/value telemetry değildir. Yeni açık CLI 22 pinli mapping recipe kullanır; inherited eski yollar korunur. Bu local C++ yardımcı ABI'si bootstrap C ABI 1 veya SDK runtime yetkisini değiştirmez; initialization/SAEX DLL/unpack/symbol/ABI ve N3 açık kalır.

0.1.8 explicit context gözleminde beklenmeyen OS debug olayının tanısı için loader çıktısına lastEventCode/lastEventThreadId eklendi. Son alınan olay metadata'sıdır; initialization kanıtı veya olayın devamına izin değildir. Child öncesi retlerde ikisi de sıfırdır. Unknown event terminal ret davranışı ve incelenmiş DLL policy değişmedi.

## V0.16 — Gözlem içi modül yaşamı

[0.1.9 lifecycle](../development/d1-loader-lifecycle.md) bilinen aktif UNLOAD_DLL olayını emekli eder. Her LOAD yeniden file ID/hash doğrular ve yeni yerel mappingId alır; eski ID/base breakpoint sınıflandırmasına katılamaz. History ve hash/event bütçesi unload ile sıfırlanmaz. Unknown/duplicate unload terminal ret; ilk exception ve canAttach=false sınırı aynıdır. Bu gözlem SAEX hook/DLL hot unload desteği değildir.

## V0.17 — Bağımlılık sembol incelemesi

ADR-45 ve [0.1.10 linkage](../development/d1-native-linkage.md), native başlangıçta DLL adından daha dar bir statik gereksinim kaydı üretir. Schema 1 consumer/candidate hash, normal/delay thunk, exact case isim/ordinal ve compact export hedefini içerir. EAT holes eşleşmez; forwarder metni çözülmeden kalır. İki dosyanın doğrudan statik eşleşmesi runtime file ID, DllMain/TLS veya prototype/calling convention kanıtı değildir. Bootstrap C ABI 1, 22 pinli mapping policy ve first-exception-only davranışı değişmez. Gerçek kurulumdaki wrapper'ın dinamik load yolu ve ardından bounded initialization/SAEX DLL/ABI hâlâ N2 işi; N3 açılmaz.

## V0.18 — Ayrı initialization izni ve entry boundary

[0.1.11 sözleşmesi](../development/d1-entry-boundary.md), retained mapping pins ile yürütme iznini ayırır. Mevcut first-exception-only JSON/üç CLI aynı kalır; yeni --observe-entry-boundary, compiled executionPolicy ve artifact SHA ile ayrı sınırlı DLL/TLS başlangıç deneyi açar. DR0 ilk CREATE_PROCESS olayında kurulur, ntdll noktasında byte/register tekrar kontrol edilir, main-thread PE entry fault'u doğrulanır ve child öldürülür. Bu local debugger register yazımıdır; bootstrap ABI, IP/stack veya engine talimat byte'ı yazılmaz. Aynı/different entry byte outcome'ları initializationVerified veya canAttach açmaz. Native runtime/ABI ve gerçek SAEX DLL çağrısı N2'de ayrıca kanıtlanır.

## Kod 0.1.11 — Entry supplement bağı

Entry izin source’u entry-policy.json olarak somutlaştı: exact base/engine digest ve system-only ek pin. Imm32 ile 23 birleşik kayıt vardır; eski 22 pin/first-exception komutları değişmez. Runtime JSON, override veya otomatik hash kabulü bulunmaz. [Ayrıntı](../development/d1-entry-boundary.md).

## GitHub kaynak yayını — taşınabilirlik düzeltmesi

Bootstrap uint32_t fallback dönüşümü ve Windows own fixture dizin kimliği düzeltmesi kaynak taşınabilirliği kapsamındadır. Fixture eşdeğer yol yazımını kabul eder, farklı dizin/yanlış token sonucunu reddeder. Production directory pin, exact SDK/profile/recipe digest ve C ABI 1 aynı kalır; GTA entegrasyon kanıtı eklenmez.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.

## Kod 0.1.12 — Proxy dönüş sınırı

[ProxyStopSpec](../development/d1-proxy-return.md) yalnız güvenilir local C++ observer çağıranının ödünç pin ve derlenmiş RVA tarifidir. İlk hitten sonra DR0 dönüş talimatına taşınır; IP/stack/EFlags veya child kodu observer tarafından yazılmaz. İkinci hit terminaldir. Bootstrap C ABI 1 ve N1 toolchain/SDK lock değişmez; gerçek SAEX Initialize/Query/Stop çağrısı hâlâ kanıtlanmadı.

## Kod 0.1.13 — Startup çağrı sınırı

[StartupStopSpec](../development/d1-startup-call.md) yalnız güvenilir local C++ çağıranın tuttuğu 1–4 ImageAnchor span’ıdır. Üçüncü hedefte DR0 execution, main-thread IAT slotunda DR1 dört byte write watch kurulur; EIP/stack/EFlags veya code/IAT observer tarafından yazılmaz. Watchpoint yazımdan sonra tuzak verir; cross-thread güvenlik sınırı değildir. Bootstrap C ABI 1 ve N1 SDK bağı/standardı değişmez.

## Kod 0.1.14 — Codec dönüş kesiti

N2 için ayrı run_to_codec_return izni, startup parent digest ve üç exact game-root codec pini eklendi. LoadLibraryA dönüşündeki EAX aktif root mapping ile eşleşmeli; codec export/ASI/gerçek SAEX C ABI çağrısı henüz kapsamda değildir. N1 SDK lock ve bootstrap ABI 1 aynı kalır. [Sözleşme ve doğrulama](../development/d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

Ayrı run_to_codec_bindings izni aynı 26 pin ve codec source digest bağıyla sekiz direkt export pointer bağını gözler. Null veya yanlış root+RVA hedefi, byte drift, retired codec mapping ve yeni DLL ret verir. Bootstrap C ABI 1/SDK lock aynı kalır. [Sözleşme ve kanıt](../development/d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](../development/d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

0.1.16 runtime sınırı güncellendi: SDK bağımsız SAEX DLL’si gerçek GTA’ya yüklendi, LoadLibrary döndü. Plugin-SDK-SA library/probe kaynak kilidi ve bootstrap C ABI değişmedi. GTA içindeki açık bootstrap çağrıları ve N3 frame/hook capability ayrı kanıt bekler; bu yükleme SDK binding/hook aktivasyonu değildir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](../development/d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Pinned SDK gameProcessEvent CALL 0x53E981, CGame::Process 0x53BEE0 ile disk üzerinde örtüşür. PRIORITY_AFTER/void() kaynak bilgisi runtime ABI kabulü değildir. initGameEvent hedefi 0x53E580, CGame::Initialise 0x53BC80 ile aynı değildir. N1 kaynak/link kapsamı değişmedi; N2 ve N3 açık. [Aday ve doğrulama raporu](../development/d1-frame-target.md).

## Kod 0.1.19 bağlantısı

Doğal startup dönüşü, N2 içinde ayrı capability deneyidir: Windows export/IAT/önek, koruma argümanları/sonucu ve stdcall/nonvolatile dönüşü doğrulanır. CGame::Process hâlâ yalnız adaydır; doğal frame thread/faz/ABI ve N3 hook kapısı açık kalır. [Sözleşme ve kanıt](../development/d1-startup-return.md).

## Kod 0.1.19 — Bağımsız image koruma deneyi

Ayrı image-protection aracı yalnız derlenmiş SAEX fixture'ını kendi Python sürecinde SEC_IMAGE olarak eşler; engine binding, SDK link kapsamı veya runtime capability açmaz. 0x40/0x80 ölçümü ve tek özel kopya yazımı OS davranışını tekrar üretir; orijinal dosya yazılmaz. [Sözleşme ve kanıt](../development/d1-image-protection.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](../development/d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](../development/d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](../development/d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.

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

## 14 Eylül 2026 — Windows fixture taşınabilirliği

Startup-return ve devamındaki fixture zinciri, sistem DLL reçetelerini test makinesinin diskte tutulan PE dosyalarından çıkarır. Ortak okuyucu ve negatif doğrulamalar [test kapsamı notunda](../development/d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği) açıklanır; üretim policy/hash kuralları ve bu belgedeki gerçek GTA kanıtının sınırları aynıdır.

## Kod 0.1.32 — Cwd query bağlantısı

Drive-zero cwd sorgusu Plugin-SDK bağımsız observer kesitidir. Reviewed kernel32/kernelbase export HIGHLOW alanları tam DWORD normalize edilir. Fixture host metadata kullanabilir; GTA CLI eski loader pinlerini gevşetmez. ReAgent referansı native SDK veya runtime dependency değildir. [Sözleşme, kaynak ve güncel kanıt](../development/d1-cwd-query.md).

## 0.1.33 — Windows profil bağı

[26200.9445 incelemesi](../development/d1-system-profile-9445.md), bu bileşenin bağlı olduğu OS pinleri/API reçeteleri ve parent SHA-256 zincirini günceller. Bileşenin GTA durak/izin sınırı aynı kalır; önceki runtime sonuçları eski profil bağlamındadır. Yeni dosyalarla fixture ve gerçek GTA kanıtının kapsamı profil raporunda ayrıca izlenir. Eski OS pinlerine otomatik fallback yoktur; kaynak ve generated policy birlikte yeniden derlenir.

## 0.1.34 — Doğal cwd copy bağı

[Ayrı copy kesiti](../development/d1-cwd-copy.md), query'den sonra native kontrol/CALL/dönüş ve gerçek hedef içerik kanıtını ekler. Önceki komutların terminali korunur; helper/unlock/SEH sökümü yeni izin kapsamına girmez. Source/guard, caller/SEH/kilit/cookie denetimleri ve yeni test/GTA kanıtının kapsamı ilgili rapordadır. C ABI 1, OS modül pinleri, GNS/otorite ve production kapıları aynı kalır; C++ observer yeniden derlenir.

## 0.1.35 — Cwd helper dönüş bağı

[Ayrı return kesiti](../development/d1-cwd-return.md) copy sonrasındaki iki POP, cookie checker eşitliği ve doğal LEAVE/RET'i açar. Önceki terminal izinleri korunur; wrapper/unlock/SEH işlemleri henüz açılmaz. Kaynak stack ömrü, CALL ile değişen saved slot, hedef/caller/kilit/SEH ve hata retleri sözleşmede açıklanır. C++ trace yeniden derlenir; C ABI 1, OS pinleri, GNS/otorite ve production kapıları aynı kalır. Yeni fixture/gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## 0.1.36 — Dosya yöneticisinin tamamlanması

`run_file_manager_ready` ve `--observe-file-manager-ready` ayrı izinle doğal CFileMgr dönüşüne kadar ilerler. Son terminal 0x53BB5F; oyun ana döngüsü henüz açılmaz. Yeni ntdll LeaveCriticalSection hedefi exact mevcut OS kimliğine bağlıdır. Adresler review/generator/C++ doğrulamasıyla bağlı, C ABI 1 ve SDK görünürlüğü aynıdır. [Sözleşme, kullanıcı komutu ve doğrulama](../development/d1-file-manager-ready.md).

0.1.36 araç doğrulama düzeltmesi: Run-SAEX hash okuması .NET SHA256/FileStream kullanır; Windows PowerShell Get-FileHash modül keşfine bağlı değildir. UTF-8 BOM/konsol ve kısmi hata raporu ile Windows PowerShell 5.1 üzerinde Debug/Release pozitif akış ve eksik klasör/bilinmeyen exe retleri geçti. Native izin ve başarı koşulları değişmedi.

## Kod 0.1.37 — Streaming tablo kesitiyle bağlantı

[CdStream tablo sözleşmesi](../development/d1-cd-stream-tables.md) ortak observer/CLI ve fixture zincirine ayrı bir üst mod ekler. Bu belgenin eski komut ve checkpoint sınırı korunur; yalnız `--observe-cd-stream-tables` tam manager dönüşünden sonra iki tablo döngüsünü ve disk argüman hazırlığını açar. Sonuç yeni `cdStreamTablesObservation` alanında izlenir; eski kayıtlar final durum değil önceki checkpoint snapshot'ıdır. C++ trace tüketicileri yeniden derlenir; C ABI 1, GNS, OS pinleri ve production IPC sınırı değişmez. Yeni portable/native testler ile eski mod regresyonları standart build'e dahildir; gerçek GTA ve platform bazındaki final kanıt ana raporda tutulur.

## Kod 0.1.38 — Disk sonucu ve allocation önkoşulu

[Disk hazırlığı sözleşmesi](../development/d1-cd-stream-disk.md) önceki native zincire ayrı `--observe-cd-stream-disk` modu ekler. BOOL başarısızsa dört output kullanılmadan ret; başarılı ve kabul edilen mantıksal geometride doğal bayrak/argüman hazırlığı, 0x406BF4 allocation CALL önünde doğrulanır. Eski modların terminal ve snapshot anlamı korunur. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS, network otoritesi ve sandbox kapsamı değişmez. Gerçek allocation, fiziksel hizalama, dosya okuma ve thread/renderer hazır kanıtı bu değişiklikten çıkarılamaz. Portable hata kararı ile native/gerçek GTA kanıtının ayrımı yeni raporun kabul tablosunda izlenir.

## Kod 0.1.39 — Hizalı tamponun doğal dönüşü

[Allocation sözleşmesi](../development/d1-cd-stream-allocation.md) ayrı `--observe-cd-stream-allocation` API/CLI ile MallocAlign → CRT → HeapAlloc → back-pointer → 0x406BF9 doğal dönüşünü ekler. Heap modu/new-handler/SBH dalı yürütmeden önce denetlenir; NULL, taşma, metadata ve payload bütünlüğü guard'ları vardır. Eski alt modların terminalleri ve snapshot anlamı korunur; yeni mod 160, eskiler 128 olay üst sınırındadır. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS ve sandbox kapsamı değişmez. İlk gerçek GTA allocation geçti; güncel toplu kanıt yeni sözleşmede izlenir. Native free, I/O/thread, renderer ve D1/D2 hazır kabul edilmez.

## Kod 0.1.40 — Kanal belleği kesiti

[Yeni sözleşme](../development/d1-cd-stream-channels.md) `run_cd_stream_channels` / `--observe-cd-stream-channels` ile SetLastError ve LocalAlloc doğal yolunu, 5 × 48 sıfır byte ve global pointer kaydını ekler. Terminal 0x406C34, arşiv CALL önüdür. Önceki allocation/parent kayıtları kendi duraklarının snapshot anlamını korur; canlı tabloda yalnız kanal sayısı/etkin sayı DWORD çifti değişebilir. C++ trace tüketicileri yeniden derlenir; C ABI 1 ve mevcut OS pinleri aynıdır. Allocation ve yeni mod 160, daha eski modlar 128 olay sınırındadır. Native free, dosya açma/okuma, thread, renderer ve D1/D2 kapıları açıktır. Güncel test ve GTA kanıtı yeni sözleşmede tutulur.

## 14 Eylül 2026 — Linux fixture derleme düzeltmesi

0.1.40 GitHub yayınında GCC strict uyarısı, file-manager portable testindeki tek satırlık döngü/terminator yazımını reddetti. Döngü gövdesi süslü parantezle açıklaştırıldı ve terminator ayrı satıra alındı; test koşulları, üretim davranışı ve strict -Werror aynı kaldı. İlk hosted ret [yayın raporunda](../development/github-publication.md) tutulur.
