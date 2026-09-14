# GTA motor uyarlaması ve kabiliyet sınırları

Durum: tasarım + N2 dosya/image, oyun dışı bootstrap DLL, ilk-create observer, statik native başlangıç ve sınırlı loader mapping alt kümeleri, v0.14/ADR-35–42. [Mimari](overview.md) · [Araştırma kaydı](../decisions/research-register.md) · [Native SDK entegrasyon sözleşmesi](native-sdk-integration.md)

Kod 0.1.4'ün [ayrı process observer'ı](../development/d1-suspended-process.md), exact dosya kapısından sonra yalnız kendi oluşturduğu askıdaki çocuğun ilk debug olayını okur. Event dosya kimliği/base/header/anchor eşleşmesi aranır, ana thread resume edilmez ve çıkış doğrulanır. Bu pre-user-code gözlemi initialized/unpacked engine profili, DLL yükleme veya function ABI desteği değildir; canAttach=false kalır.

## Seçilmiş native erişim yolu

Dryxio/plugin-sdk-sa, commit `b55e89b336a81448c1aa1a5b188431c9845ebaa9`, D1 için private x86 binding adayıdır. [İncelenen kaynaklar](../references/plugin-sdk-sa.md) sınıf/offset, pool, streaming ve event/hook erişimine dayanak sağlar; SDK'nin 23 dosyalı seçilmiş alt kümesi [D1-N1 private x86 probe'una](../development/d1-native-dependency.md) bağlandı; gerçek adapter henüz yoktur. C++23 gereksinimi yalnız bu bağımsız native hedefte uygulanır; core C++20'dir. Core/Host/server/C# tarafına native pointer veya SDK header'ı taşınmaz. Bağımlılık edinimi, C++20/build uyumu, profil ve hook önkoşulları [normatif sözleşmede](native-sdk-integration.md); ilk uygulama [D1-N1–N7 planındadır](../development/d1-engine-integration.md).

SDK'nin sürüm marker'ları EngineInspector'ın profil doğrulamasını karşılamaz. SDK bağımsız bootstrap hem dosya hem mapped-image önkoşullarını doğrular; global initializer/event kaydıyla bu kapıdan önce native erişim yapılamaz. Hook sahibi, rollback ve callback quiescence kaydedilir. Temiz session/process stop zorunlu; çalışan GTA'dan dinamik DLL unload yalnız ayrıca kanıtlandığında açılan capability'dir. N2'nin SDK bağımsız dosya/image gözlemi ve başlangıç DLL'si uygulanmıştır. DLL'nin ABI/ret/stop akışı oyun dışı host'ta test edilmiştir; gerçek GTA load/faz, hook ve gameplay lifecycle kanıtı henüz yoktur. [Modül raporu](../development/d1-bootstrap-module.md).

## Desteklenen executable yaklaşımı

İlk araştırma hedefi klasik PC GTA:SA 1.0 US profilidir. D1 foundation'da kullanıcının yerel gta_sa.exe dosyası salt okunur incelendi: I386/native PE32, 14.383.616 byte, SHA-256 `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`. N2'de Hoodlum marker'ı, bölüm düzeni ve dört kısa SDK adres anchor'ı dosyada ve çalıştırılmayan Windows image eşlemesinde doğrulandı. [N2 kanıtı](../development/d1-engine-preflight.md). Dosyanın runtime 1.0 US/hook/ABI uyumluluğu doğrulanmadı. [EngineInspector](../../managed/Saex.Tools/EngineInspector.cs) `recognizedProfile=false/canAttach=false` döndürür. R-01 hâlâ verified profil, gerekli hook seti, attach/başlatma/kapanış testi ister. Tanınmayan sürümde adres tahmin edilmez.

Uyarlama, dosyaların değiştirilmesini varsaymaz. Kullanıcının yerel oyun varlıkları kaynak katalogdan okunur; sunucu orijinal GTA dosyalarını dağıtmak zorunda değildir. Platform cache ve sanal mount'u orijinal kurulumdan ayrıdır.

## Kabiliyet matrisi

“Dış kanıt”, benzer davranışın başka bir projede bulunduğudur; kendi adapter'ımızın tamamlanmış desteği değildir.

| Motor alanı | İlk SAEX davranışı | Durum ve sınır |
|---|---|---|
| Başlatma / kapanış | Profil doğrulama, güvenli attach, temiz unload | R-01; tanınmayan binary'de tahmini adres kullanılmaz |
| Frame / input | Kontrollü input sağlayıcısı, güvenli komut noktası | R-02; native hareket ve kamera sıralaması ölçülür |
| Entity pool | Platform kimliğini yerel handle'a bağlama | R-02; pool ve generation tükenmesi açık hata |
| Model / kaplama / collision | Ayrı yükleme, doğrulama, referans sayımı | R-04; sıraya ve thread'e özgü native sınırlar |
| Harita streaming | Hücre/placement takibi, varsayılan spawn bastırma | R-05; yerleşim bazlı değişiklik ayrımı zorunlu |
| Kırılma / devrilme | Native olay yakalama ve authoritative sonuç uygulama | R-06; efekt ile collision sonucu ayrılır |
| Fizik özellikleri | Native profile eşleme veya platform simülasyonu | R-06; grup/model düzeyi değişiklik entity düzeyi sanılmaz |
| Animasyon | IFP yükleme, klip kimliği, geçiş/iptal | R-07; skeleton ve root motion ayrı doğrulanır |
| Araç handling / hasar | Profil ve instance hasarı ayrımı | R-08; her parametre araç başına değişebilir varsayılmaz |
| Silah / combat | Intent, atış olayları, hit doğrulama adaptörü | R-08; native hasarın ikinci kez uygulanması engellenir |
| UI / render / ses | Sahne ve ses komutları, varsayılan HUD kapatma | R-09; shader/renderer izolasyonu tam garanti değildir |
| NPC / kara trafiği | Kontrollü spawn/task/sinyal/retire adaptörü | R-10/17; native AI'ın tüm davranışları deterministik değildir |
| Hava trafiği | Ayrı uçak/helikopter görevleri, loss policy ve hızlı streaming | R-18; seyir/kalkış/iniş ayrı capability |
| Hayvan / creature | Rig/beden/animasyon/controller/hit uyarlaması | R-19; model yükleme genel hayvan desteği değildir |

MTA'nın [fizik grubu API'si](https://wiki.multitheftauto.com/wiki/EngineSetObjectGroupPhysicalProperty) grup düzeyinde özellik açar. SAEX entity düzeyi ayar istediğinde shared native grubu değiştirmek diğer entity'leri de etkileyebilir. İzole profil oluşturma veya platform simülasyon yolu yoksa `unsupported_capability` dönülür; bütün dünyaya yan etki uygulanmaz.

MTA'nın [dinamik model API'si](https://wiki.multitheftauto.com/wiki/EngineRequestModel) ve [IFP yükleyicisi](https://wiki.multitheftauto.com/wiki/EngineLoadIFP) araştırmaya referanstır. Yükleme desteği, bu varlıkların ağda doğru davranışa sahip olduğunu tek başına göstermez.

Kaynak seviyesinde [MTA object/model/streamer incelemesi](../references/mtasa-architecture.md) doğrultusunda her property için adapter eşleme kaydı tutulur: `live-set`, `recreate-required`, `unsupported`. Materialize, server state'ini görünmeyen hazırlıkta model/body/damage/collision/attachment sırasıyla kurar. Geç gelen yükleme sonucu EntityRef/ResourceEpoch/CatalogRevision kontrolü olmadan bağlanmaz. Native recreate güvenliği ve 2 ms hazırlık bütçesi R-02/R-04 ölçümüdür; setter adı tek başına kesintisiz güncelleme garantisi değildir.

## Tek yazıcı ve native davranışların denetimi

Oynanışı etkileyen objede hangi sistemin transform, collision ve hasar ürettiği bellidir. GTA ile SAEX aynı değeri aynı frame'de bağımsız yazmaz. Native olayı gözlemek, engellemek ve accepted state uygulamak ayrı kabiliyetlerdir; sadece gözleme hook'u varsa tam otorite iddia edilmez.

Örneğin yerel GTA lambayı devirmiş olabilir. Adapter bu hareketi tahmini gösterim olarak ele alır; sunucu kabul etmezse eski collision ve görsel durum birlikte geri kurulur. Bu geri kurma doğrulanamıyorsa o model ilk authoritative yıkım kataloğuna alınmaz.

## Sürümlü adapter sözleşmesi

`EngineProfileId`, `AdapterApiMajor`, `CapabilitySet`, `PoolLimits`, `BuildFingerprint` handshake'in yerel motor bilgisine girer. Client'ın bildirdiği hash anti-cheat kanıtı değildir; uyumluluk denetimi girdisidir. Sunucu kritik davranışın kabul edilmiş capability setine uygunluğunu denetler.

Native entity için create/destroy/rebind, asset için acquire/release ve her komut için durum/ret nedeni gerekir. Pointer yalnız adapter'ın içinde yaşar. C# tarafında `Native.Call(address)` bulunmaz.

## Genişleme ve başarısızlık

Yeni motor profili ayrı conformance senaryolarından geçer. Desteklenmeyen shader, pool artırımı veya kural override'ı sessizce yaklaşık uygulanmaz. Asset, minimum capability tanımlar; alternatif varsa sunucunun aynı oynanış anlamına sahip olduğunu ilan ettiği varyant seçilir.

R-01/02 başarısızsa multiplayer katmanına geçilmez. R-05/06 başarısızsa dünya çapında yıkım sözü daraltılır: test edilmiş özel entity'lerle çalışma sürer ve stok harita dönüşümü deneysel kalır. Kabul: AC-01, AC-04, AC-06, AC-15, AC-16.

## Population ve creature capability profilleri

R-17; native ambient spawn, retire, görev, trafik sinyali ve otomatik hasarın ayrı ayrı gözlenmesini/bastırılmasını/accepted state ile uygulanmasını sınar. Salt spawn hook'u tam nüfus otoritesi değildir. R-18; uçak/helikopter seyir, kalkış, iniş, hasarlı uçuş ve owner kaybı davranışını ayrı test eder. Native görünürlükten uzaklaşma platform entity'sini silme yetkisi vermez.

R-19; ped-backed creature ile özel platform controller yolunu rig/beden bazında karşılaştırır. Human ped capsule, root/step/turn varsayımları, skinned mesh, hit proxy, ragdoll/IK, flying/aquatic ve binme ayrı capability kapsamıdır. DFF/TXD/IFP yüklemek bunları doğrulamaz. Her eşleme exact/approximated/unsupported ve live-set/recreate-required sınıfı taşır; gerek duyulan approximation'ı world profili açıkça kabul etmelidir. [Hayvan sözleşmesi](../gameplay/animals-species.md), AC-55/56/57/67. Hiçbir yeni engine capability bu teslimatta verified değildir.

## Stall, resume ve native state onarımı

Adapter güvenli native frame girişinde Host/lease bağlamını ve monoton watchdog'u kontrol eder; suspend dönüşünde eski komutları oynatmadan grant/readiness yeniler. Native controller her desteklenen araç/rig için loss policy bildirir. Havada yolcu taşıyan uçağın transform'unu koşulsuz dondurmak veya collision'ı silmek ortak fallback sayılamaz. [Zaman](time-fencing.md) ve [arıza](failure-recovery.md) şartları AC-71/72 ve R-21 ile kanıtlanır.

State digest ile actual native binding ayrı denetlenir: beklenen asset/body generation, AppliedBindingRevision ve capability'nin sunduğu sorgular karşılaştırılır. Geçersiz collider rebind edilene kadar ilgili interaction scope fence edilir. Güvenli sorgu/rebind kanıtı olmayan alan verified açılmaz. [Onarım akışı](../networking/consistency-recovery.md), AC-81; runtime testi henüz yapılmadı.

## Native başlangıç girdilerinin statik envanteri

Bu metadata okuyucusunda raw boyutun hizalama padding farkı, bütün dosya/image sınırları korunarak açık LayoutNotes kaydı olabilir. Bu dar gözlem desteği native parser veya runtime loader hizalama kapısını gevşetmez.

Kod 0.1.5 [başlangıç envanteri](../development/d1-native-startup.md), exe ile yanındaki üst seviye DLL/ASI girdilerini birlikte raporlar. Aynı exe hash'inden aynı loader ortamı çıkarılamaz. EXE/DLL entry, normal/delay import ve TLS callback metadata'sı file-backed/aralık/kota kontrollerinden geçer; callback gövdeleri çalıştırılmaz. Yerel adaylar gerçek loaded-module değildir. CanAdvanceToLoader ve CanAttach daima false; unknown exe dosya eşleşmesi false olarak raporlanabilir. Unpack/ABI ve OS loader gözlemi ayrı kanıt gerektirir.

## Kod 0.1.6 — Loader mapping kanıtının sınırı

[Yeni x86 observer](../development/d1-loader-observation.md) SDK olmadan Windows loader'ın seçtiği dosyaları event hFile ve tutulan file ID/hash pinleriyle karşılaştırır. İlk exception duraktır; ntdll içindeki breakpoint adayı initialized engine profili değildir. Test DLL/TLS/main canary'leri bu sınırı kendi fixture'ında doğrular. Gerçek GTA'da apphelp.dll mapping'inde ret ve process çıkışı gözlendi; tam DLL initialization sırası, unpack, oyun function ABI ve SAEX DLL load hâlâ açıktır. Mevcut dört anchor/profile ve canAttach=false değişmedi.

## Kod 0.1.7 — Ortamı da kapsayan loader önkoşulları

[Yeni prepared policy](../development/d1-loader-policy.md) hash'i doğru exe'nin yanındaki DLL'yi de child öncesi sabitler; event'te canlı file ID/hash ile aynı dosya aranır. Yanlış hash/boyut, eksik veya farklı origin'deki DLL güvenli ret verir. İncelenen 22 modüllü özel kopya loader breakpoint adayına ulaştı; orijinal kurulum AcLayers kolunda ret aldı. Bu iki launch context için initialized engine/SDK ABI kanıtı ortaklaştırılmaz. ObservedProfile ve dört anchor aynı kalır; gerçek bootstrap yüklemesi/oyun property yetenekleri henüz açılmadı.

## V0.15 — Başlatma girdilerinin sabitlenmesi

[Explicit context](../development/d1-launch-context.md) executable yolunu tek kez çözer, absolute cwd ve bounded environment kopyasını CreateProcessW'ye açık geçirir. Aynı engine SHA tek başına aynı Windows launch context değildir. Directory pin'i içeriği dondurmaz; read-only image/anchor kapısı ve loader dosya kimlikleri korunur. Oyun hook/başlatma capability'si açılmaz.

0.1.8 explicit context gözleminde beklenmeyen OS debug olayının tanısı için loader çıktısına lastEventCode/lastEventThreadId eklendi. Son alınan olay metadata'sıdır; initialization kanıtı veya olayın devamına izin değildir. Child öncesi retlerde ikisi de sıfırdır. Unknown event terminal ret davranışı ve incelenmiş DLL policy değişmedi.

## V0.16 — Aktif eşleme ve geçmiş ayrımı

[Mapping ledger](../development/d1-loader-lifecycle.md), retired base adresinin yeni DLL tarafından kullanıldığı durumda eski doğrulamanın taşınmasını engeller. Her eşleme olayına gözlem içi yeni kimlik verilir; stale ID ile ntdll/breakpoint adayı tanımlanamaz. Gözlem sonu active alanları cleanup öncesini anlatır; GTA DLL'si veya initializer'ı çağrılmaz.

## V0.17 — Statik sembol bağı bir runtime capability değildir

[Native linkage sözleşmesi](../development/d1-native-linkage.md) yalnız açık consumer/logical-import/candidate üçlüsünün isim veya ordinal gereksinimlerini karşılaştırır. Alias/ordinal hole, forwarder, executable/data bölüm hedefi ve iki dosya hash'i ayrı taşınır. Tüm gereksinimler eşleşse de CanAttach/CanInitialize/CallingConventionVerified false kalır; initializer, engine symbol ve fonksiyon ABI'si ayrıca kanıtlanır. Aday dosya adını veya loader pinlerini otomatik değiştirmek bu API'nin görevi değildir. Legacy delay sembolü ve çözümlenmeyen forwarder destek sınırı olarak açık kalır.

## V0.18 — PE entry ile initialized runtime ayrımı

[Entry boundary](../development/d1-entry-boundary.md), dosyadan kanıtlı RVA/16 byte ve owned MEM_IMAGE executable bölümü kullanır. Aynı DLL isimleri/proxy export'ları, giriş kodunun değişmediği anlamına gelmez; başlangıç sonrası bytesMatch ayrı ölçülür. Değişiklik bir native patch izni veya temiz/unpacked engine profili yaratmaz. Başka thread'deki engine yürütmesi/düşmanca breakpoint temizleme için güvenlik izolasyonu vaat edilmez; bu exact deneyin ana-thread hit/exit kanıtıdır. Bilinmeyen engine/pin/range/register/exception güvenli ret, canAttach=false olarak korunur.

## Kod 0.1.11 — Entry supplement bağı

İlk imm32 ret sonrasında ayrı entry supplement incelenmiş x86 dosyayı hash/byte ile sabitler. Engine hash ve base recipe digest birlikte bağlanır; 23 dosya child öncesi doğrulanır. Entry mutation runtime capability veya ABI açmaz. [Ayrıntı](../development/d1-entry-boundary.md).

## GitHub kaynak yayını — taşınabilirlik düzeltmesi

GitHub Linux derlemesi için bootstrap fallback türü açıklaştırıldı. Windows own context fixture dizin adını metin yerine volume/file ID ile doğrular. Engine profili, canAttach=false ve native adres/initialization sınırları değişmez.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.

## Kod 0.1.12 — Proxy dönüş sınırı

ADR-47 ile [proxy dönüş gözlemi](../development/d1-proxy-return.md), doğrulanmış entry E9 hedefindeki CALL gövdesini ilerletir ve dönüş JMP talimatında durur. Exact modül kimliği, executable MEM_IMAGE, thunk/return pointer ve entry suffix şarttır. Geri yüklenen 16 byte ve tek IAT slotu ayrı ölçülür; unpack/WinMain/SDK symbol veya gameplay capability türetilmez. Yeni RVA’lar bu tek hash’in incelenmiş gözlem reçetesidir; genel GTA adresleri değildir.

## Kod 0.1.13 — Startup çağrı sınırı

[Startup çağrı sınırı](../development/d1-startup-call.md), ayrı izinle orijinal entry’yi ilerletir ve proxy IAT hedefinin ilk talimatında durur. Çağıranın main EXE içinde FF15 [exact IAT] biçimi, aynı stack allocation’ındaki 68 byte parametre alanı ve iki durak arasındaki target byte kararlılığı denetlenir. Dört mevcut anchor’ın yeniden okunması tanısal örneklemedir; matched sample unpack, engine symbol veya fonksiyon ABI’si açmaz.

## Kod 0.1.14 — Codec dönüş kesiti

Adapter henüz codec export çağrısı veya engine capability açmaz. Yeni dördüncü durak yalnız ilk LoadLibraryA dönüşünü ve root/dependency mapping kimliğini kontrol eder; image anchor örnekleri önceki startup fazına aittir. [Sözleşme ve doğrulama](../development/d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

Adapter capability açmadan wrapper pointer tablosunun root codec export adresleriyle eşitliğini doğrulayan beşinci durak eklendi. Ses/codec fonksiyonları ve SAEX bootstrap henüz çağrılmaz. [Sözleşme ve kanıt](../development/d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](../development/d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](../development/d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Frame adayı CREATE_PROCESS, ASI dönüşü ve terminal bootstrap duraklarında exact CALL/16 bayt hedef önekiyle denetlenir. Örnek thread kimliği, frame fonksiyonunun çağıran thread kanıtı değildir. Observation profili ve canAttach=false korunur; sürekli integrity veya trampoline kurulumu yoktur. [Aday ve doğrulama raporu](../development/d1-frame-target.md).

## Kod 0.1.19 bağlantısı

0.1.19, aynı profil/pin ve owned child kapıları üzerinde ayrı doğal startup-return deneyi ekler. Mevcut durak davranışı korunur; koruma çağrısı ve dönüş ABI kanıtı yeni raporda izlenir. D1/N2/N3 ve oynanabilir multiplayer kapıları açık kalır. [Sözleşme ve kanıt](../development/d1-startup-return.md).

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](../development/d1-crt-startup.md).

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

Native query yalnız tanınmış game/OS dosya kimlikleriyle, doğru helper/Win32 çağrı sınırlarında açılır. ASCII path/readback ve hedef kapasitesi sağlanmadan CFileMgr kopya/suffix izni verilmez. Yeni OS farkında ret korunur; entity/frame desteği değişmez. [Sözleşme, kaynak ve güncel kanıt](../development/d1-cwd-query.md).

## 0.1.33 — Windows profil bağı

[26200.9445 incelemesi](../development/d1-system-profile-9445.md), bu bileşenin bağlı olduğu OS pinleri/API reçeteleri ve parent SHA-256 zincirini günceller. Bileşenin GTA durak/izin sınırı aynı kalır; önceki runtime sonuçları eski profil bağlamındadır. Yeni dosyalarla fixture ve gerçek GTA kanıtının kapsamı profil raporunda ayrıca izlenir. Eski OS pinlerine otomatik fallback yoktur; kaynak ve generated policy birlikte yeniden derlenir.

## 0.1.34 — Doğal cwd copy bağı

[Ayrı copy kesiti](../development/d1-cwd-copy.md), query'den sonra native kontrol/CALL/dönüş ve gerçek hedef içerik kanıtını ekler. Önceki komutların terminali korunur; helper/unlock/SEH sökümü yeni izin kapsamına girmez. Source/guard, caller/SEH/kilit/cookie denetimleri ve yeni test/GTA kanıtının kapsamı ilgili rapordadır. C ABI 1, OS modül pinleri, GNS/otorite ve production kapıları aynı kalır; C++ observer yeniden derlenir.

## 0.1.35 — Cwd helper dönüş bağı

[Ayrı return kesiti](../development/d1-cwd-return.md) copy sonrasındaki iki POP, cookie checker eşitliği ve doğal LEAVE/RET'i açar. Önceki terminal izinleri korunur; wrapper/unlock/SEH işlemleri henüz açılmaz. Kaynak stack ömrü, CALL ile değişen saved slot, hedef/caller/kilit/SEH ve hata retleri sözleşmede açıklanır. C++ trace yeniden derlenir; C ABI 1, OS pinleri, GNS/otorite ve production kapıları aynı kalır. Yeni fixture/gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## 0.1.36 — Dosya yöneticisinin tamamlanması

Adapterın gerçek GTA doğrulaması CFileMgr::Initialise fonksiyonunun sonuna genişletildi: cwd kopyası ve cookie kontrolü sonrasında kilit bırakma, normal SEH sökümü ve yol suffix/dönüşü birlikte denetlenir. `canAttach` ve `initializationVerified` false kalır; renderer ve doğal frame için bu alt kanıt yeterli değildir. [Sözleşme, kullanıcı komutu ve doğrulama](../development/d1-file-manager-ready.md).

## Kod 0.1.37 — Streaming tablo kesitiyle bağlantı

[CdStream tablo sözleşmesi](../development/d1-cd-stream-tables.md) ortak observer/CLI ve fixture zincirine ayrı bir üst mod ekler. Bu belgenin eski komut ve checkpoint sınırı korunur; yalnız `--observe-cd-stream-tables` tam manager dönüşünden sonra iki tablo döngüsünü ve disk argüman hazırlığını açar. Sonuç yeni `cdStreamTablesObservation` alanında izlenir; eski kayıtlar final durum değil önceki checkpoint snapshot'ıdır. C++ trace tüketicileri yeniden derlenir; C ABI 1, GNS, OS pinleri ve production IPC sınırı değişmez. Yeni portable/native testler ile eski mod regresyonları standart build'e dahildir; gerçek GTA ve platform bazındaki final kanıt ana raporda tutulur.

## Kod 0.1.38 — Disk sonucu ve allocation önkoşulu

[Disk hazırlığı sözleşmesi](../development/d1-cd-stream-disk.md) önceki native zincire ayrı `--observe-cd-stream-disk` modu ekler. BOOL başarısızsa dört output kullanılmadan ret; başarılı ve kabul edilen mantıksal geometride doğal bayrak/argüman hazırlığı, 0x406BF4 allocation CALL önünde doğrulanır. Eski modların terminal ve snapshot anlamı korunur. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS, network otoritesi ve sandbox kapsamı değişmez. Gerçek allocation, fiziksel hizalama, dosya okuma ve thread/renderer hazır kanıtı bu değişiklikten çıkarılamaz. Portable hata kararı ile native/gerçek GTA kanıtının ayrımı yeni raporun kabul tablosunda izlenir.

## Kod 0.1.39 — Hizalı tamponun doğal dönüşü

[Allocation sözleşmesi](../development/d1-cd-stream-allocation.md) ayrı `--observe-cd-stream-allocation` API/CLI ile MallocAlign → CRT → HeapAlloc → back-pointer → 0x406BF9 doğal dönüşünü ekler. Heap modu/new-handler/SBH dalı yürütmeden önce denetlenir; NULL, taşma, metadata ve payload bütünlüğü guard'ları vardır. Eski alt modların terminalleri ve snapshot anlamı korunur; yeni mod 160, eskiler 128 olay üst sınırındadır. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS ve sandbox kapsamı değişmez. İlk gerçek GTA allocation geçti; güncel toplu kanıt yeni sözleşmede izlenir. Native free, I/O/thread, renderer ve D1/D2 hazır kabul edilmez.

## Kod 0.1.40 — Kanal belleği kesiti

[Yeni sözleşme](../development/d1-cd-stream-channels.md) `run_cd_stream_channels` / `--observe-cd-stream-channels` ile SetLastError ve LocalAlloc doğal yolunu, 5 × 48 sıfır byte ve global pointer kaydını ekler. Terminal 0x406C34, arşiv CALL önüdür. Önceki allocation/parent kayıtları kendi duraklarının snapshot anlamını korur; canlı tabloda yalnız kanal sayısı/etkin sayı DWORD çifti değişebilir. C++ trace tüketicileri yeniden derlenir; C ABI 1 ve mevcut OS pinleri aynıdır. Allocation ve yeni mod 160, daha eski modlar 128 olay sınırındadır. Native free, dosya açma/okuma, thread, renderer ve D1/D2 kapıları açıktır. Güncel test ve GTA kanıtı yeni sözleşmede tutulur.
