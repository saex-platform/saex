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
