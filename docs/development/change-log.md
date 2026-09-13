# Kaynak ve belge değişiklik kaydı

## Kod 0.1.13 / mimari v0.20 — 13 Eylül 2026

Ayrı run_to_startup_call/--observe-startup-call ile orijinal entry’den sonraki ilk proxy IAT çağrısı tutulur. DR0 fonksiyon başlangıcına taşınır, DR1 aynı main-thread’in IAT yazımlarını gözler. CALL biçimi/slot, stack parametre alanı, target byte kararlılığı ve dört sınırlı image örneği raporlanır. Eski entry/proxy durakları korunur; startupObservation eski modlarda null olur. Strict startup-policy source’u proxy ve observed profile digest’ine bağlıdır; yeni DLL pini veya otomatik native izin eklenmedi. Dokuz fixture, 19 senaryo + canary positive control + 12 warm, yedi policy ve iki CLI testi eklendi. Özel CRT entry denemesi unresolved CRT sembolleriyle başarısız oldu; normal CRT entry seçilerek düzeltildi. Ortak fixture PE helper taşındı; production parser/SDK/ABI/otorite değişmedi. Public C++ trace/API kullanıcıları birlikte yeniden derlenir; bootstrap C ABI 1 aynı, kaldırılan ürün özelliği/kalıcı migration yoktur. ADR-48, sahip belgeleri, status/roadmap/component-map ve Windows/Linux test girişleri güncellendi. [Doğrulama ve sınırlar](d1-startup-call.md). Son standart sonuç: x86 Debug/Release 11 native suite + 79 managed + 85 Python; x64 Debug/Release 6 suite + 79 managed + 60 Python geçti. Gerçek private GTA 12/12 startup hit, dört original/legacy regresyonla 16/16 confirmed exit; 16 input dosyası değişmedi. Linux/hosted/N1 bu kesit için yeniden doğrulanmadı.


## Kod 0.1.12 / mimari v0.19 — 13 Eylül 2026

Ayrı run_to_proxy_return/--observe-proxy-return, exact entry→proxy dönüş durağı, runtime thunk/return slot/aktif mapping ve entry/IAT doğrulaması eklendi. Yeni compiled proxy-policy entry digest ve game-root modül hash’ine bağlıdır; otomatik izin/pin genişlemesi yoktur. Sekiz own EXE/DLL fixture varyantı, 19 native senaryo/12 warm çevrim, yedi policy ve iki CLI testi eklendi. İlk Debug stack overflow büyük trace geçicileri kaldırılarak giderildi; yığın limiti artırılmadı. Eski CLI’lar durma sınırlarını korur; additive proxyObservation eski modlarda null olur. Public C++ observer tipi değiştiğinden çağıranlar birlikte yeniden derlenir; bootstrap C ABI 1 ve N1/GNS kararları aynı kalır. Kaldırılan özellik veya kalıcı veri migration’ı yoktur. ADR-47, normatif owner’lar, status/roadmap/component-map ve Windows/Linux test girişleri birlikte güncellendi. [Kanıt ve sınırlar](d1-proxy-return.md). Son doğrulama: Windows x86 Debug/Release 10 native suite + 79 managed + 76 Python; x64 Debug/Release 6 suite + 79 managed + 53 Python geçti. Gerçek private GTA 12/12 proxy dönüşü ve üç original/legacy regresyonda 15/15 confirmed exit; 16 girdi dosyası değişmedi. Linux/hosted/N1 yeniden doğrulaması bu kayıt kapsamında değildir.


Her kayıt davranış, kaynak, doküman, test ve kalan sınırı birlikte taşır. [Durum](status.md) · [İş akışı](workflow.md)

## 2026-09-13 — Hosted CI taşınabilirlik düzeltmesi

İlk GitHub Build başarısızlığından sonra bootstrap reason fallback açık uint32_t dönüşümüne çevrildi. Windows own fixture ham cwd metni yerine OS volume/file ID eşitliğini doğrular; eşdeğer yol, farklı mevcut klasör ve yanlış token regresyonları eklendi. Production LaunchContext, ABI/status layout, engine/policy/SDK hash girdileri ve GTA yetkileri değişmez; migration veya kaldırılan özellik yoktur. Bütün bootstrap/context normatif ve test sahibi belgeleri aynı değişiklikte güncellendi. İlk başarısız CI ve N1 uzun-yol denemesi yayın raporunda korunur; yeni yerel/hosted sonuçlar ayrıca kaydedilecektir.

## 2026-09-13 — İlk public GitHub yayını ve CI hazırlığı

Kullanıcı kurgunun uygulanmasını onayladı. SAEX marka görselleri, sade README ve belge indeksi, MIT lisansı, üçüncü taraf bildirimleri, katkı/destek/güvenlik kuralları, issue/PR şablonları, CODEOWNERS ve LF politikası eklendi. Önceki README açıklamaları bağlantıları düzeltilmiş tarihsel arşivde korundu. GitHub organizasyonu `saex-platform`, mevcut kullanıcının sahipliğinde ücretsiz planla oluşturuldu.

Windows x64/x86 Debug/Release, portable Linux x64, belge eşlemesi ve hash-pinli Gitleaks için workflow'lar eklendi; SDK işi ayrı manuel opt-in kaldı. Yeni `tools/ci.py` event/base doğrulaması sekiz negatif/pozitif testle standart build'e bağlandı. Component map, yayın sözleşmesi, workflow, foundation/status/roadmap birlikte güncellendi. Python örnek yolları taşınabilir hale getirildi.

Ürün API/ABI/otorite ve GTA davranışı değişmedi; kaldırılan ürün özelliği veya migration yok. 189 kaynak + Git metadata'sının yerel yedeği doğrulandı. Temiz checkout x64/x86 Debug/Release akışları geçti: her koşuda 79 managed, x64 6 native suite/46 Python, x86 9 suite/67 Python testi. Actionlint/YAML/SVG ve Gitleaks kontrolleri geçti. GitHub CI sonuçları [yayın raporunda](github-publication.md) tamamlanacak; kaynak aktarımı oynanabilir release değildir.

Staging sırasında hash'li engine JSON ve SDK patch girdilerinin genel LF dönüşümünden etkileneceği saptandı. `.gitattributes` bu girdileri exact byte olarak koruyacak şekilde düzeltildi; hash kapıları veya eski lock/profil değiştirilmedi.

## 2026-09-13 — SAEX marka açılımı ve GitHub yayın kurgusu

Kullanıcının önce kurgu isteği ve ilk yayından itibaren public tercihi doğrultusunda [GitHub yayın planı](github-publication-plan.md) eklendi. Marka açılımı **San Andreas Extended** olarak netleştirildi; README, status ve roadmap birlikte bağlandı. İlk yapı organizasyon profili için `.github`, bütün mevcut kod/şema/test/belgeler için `saex` monorepo; bağımsız SDK/launcher/web depoları sonraki ürün kapılarına bağlıdır. Hedef sahiplik, envanter, lisans önerisi, CI, belge eşlemesi, branch düzeni ve ilk aktarımın doğrulama ölçütleri yazıldı.

Bu yalnız belge/marka kesitidir; kaynak/ABI/otorite veya ürün davranışı değişmedi, özellik kaldırılmadı ve migration gerekmez. GitHub'da oluşturma/yazma, Git commit/remote/push, lisans uygulaması veya release yapılmadı. Native/managed testler bu plan için yeniden çalıştırılmadı. `tools/check_docs.py`: 79 Markdown, 1100 yerel bağlantı, 6 JSON örneği, beş değişmiş belge ve sıfır hata; başarılı build baseline'ı yenilenmedi. 0.1.11 final runtime kanıtı bu değişiklikle tamamlanmış sayılmaz.

## 2026-09-13 — D1-N2 entry boundary / kod 0.1.11 / mimari v0.18

LoaderObservation::run_to_entry ve --observe-entry-boundary eklendi. Yeni explicit executionPolicy, mevcut mapping pin recipe'sinden ayrı DLL/TLS başlangıç izni verir; CREATE_PROCESS anında DR0 kurulur, ilk ntdll breakpoint'inde register/byte kontrolüyle devam edilir. Main-thread PE entry EXCEPTION_SINGLE_STEP kimliği ve 16 byte önce/sonra gözlemi, ardından owned kill/exit vardır. Byte mutation tamamlanmış tanısal sonuç olabilir; canAttach/initializationVerified false kalır. Eski üç komut ilk exception'da durur; additive entryObservation onlar için null olur. Native observer caller'ları yeniden derlenir; bootstrap C ABI 1, engine/profile/policy/SDK lock değişmez. Kaldırma veya dosya migration yoktur.

Yeni x86 fixture/test target'ları, 12 senaryo/12 warm çevrim ve iki Python CLI negatif testi, ADR-46 ve owner/status/roadmap/component-map birlikte güncellendi. İlk geç-register kurulumu başarısızlığı ve Apphelp pin ret sonucu [raporda](d1-entry-boundary.md) korunur. Son standart sonuçlar: x86 Debug/Release 9 native suite + 79 managed + 59 Python; x64 Debug 6 native suite + 79 managed + 38 Python geçti. Son 12 private GTA koşusu aynı vorbisfile+0x1D60 giriş yönlendirmesini hardware fault’unda gözledi; 9 unload işlendi. Original AcLayers ret ve eski context ilk-breakpoint sınırı korundu; son matris 14/14 exit ve 16 native dosya hash eşitliğini doğruladı. X64 Release/Linux ve N1 opt-in SDK tekrar çalıştırılmadı; initialized GTA/SAEX DLL/unpack/ABI açıktır.

## 2026-09-13 — D1-N2 native sembol bağlantıları / kod 0.1.10 / mimari v0.17

PeLinkageInspector ve engine linkage komutu eklendi. İki açık native dosya, import thunk/name/ordinal ve bounded export/alias/hole/forwarder metadata'sıyla karşılaştırılır; isim/case ve ordinal ayrı kalır. Bound IAT lookup yoksa, legacy delay biçiminde, bozuk metadata/bütçede ret verilir. Eksik sembol/forwarder dinamik çözüm varmış gibi kabul edilmez; canInitialize/canAttach false. Shared startup Reader yalnız internal erişimle yeniden kullanılır; eski startup/inspect schema ve CLI davranışı korunur. Binding compact export hedefi taşır; alias listeleri her gereksinim için tekrar serialize edilmez.

22 yeni managed test kaydı, ADR-45, owner/status/roadmap ve component-map eklendi. X86 Debug/Release 8 native suite + 79 managed + 49 Python; x64 Debug 6 native suite + 79 managed + 30 Python ile ilk denemede geçti. Gerçek dosyalarda 10 CLI karşılaştırması 8 complete/2 beklenen incomplete verdi; 94 direct binding, Debug/Release aynı metadata, bağımsız dumpbin export eşleşmesi ve 13 original + 3 private native dosya hash korunumu doğrulandı. [Tam kanıt](d1-native-linkage.md). X64 Release/Linux ve N1 opt-in SDK yeniden çalıştırılmadı. Native bootstrap C ABI, policy/engine/SDK lock ve GNS kararı değişmedi; kaldırma veya migration yoktur. Orijinal oyun DLL'leri değiştirilmez, aday isim eşleşmesi DLL değiştirme onayı değildir.

## 2026-09-13 — D1-N2 loader unload/remap / kod 0.1.9 / mimari v0.16

Portable LoaderMappingLedger ve Windows UNLOAD_DLL kolu eklendi. Her kabul edilmiş mapping benzersiz gözlem içi kimlik taşır; bilinen aktif unload emekli edilir, stale ID aynı base yeniden yüklense de aktif olmaz. Yeni LOAD her seferinde retained file ID/hash kapısından geçer; lifetime load/byte/event bütçeleri iade edilmez. modules tarihçesi korunur; active/unload/mappingId/event-address alanları additive eklendi. On native ledger testi ve OS/CLI history invariant'ları eklendi. ADR-44 ve owner/status/roadmap/component-map birlikte güncellendi. Kaldırılan özellik yok; static observer tüketicileri yeniden derlenir. Bootstrap C ABI, engine/profile/policy/SDK lock ve core sözleşmeler değişmez. [Kanıt ve sınır](d1-loader-lifecycle.md).

0.1.9 sonuç: x86 Debug/Release 8 native suite + 57 managed + 49 Python; x64 Debug 6 native suite + 57 managed + 30 Python geçti. Her configuration için on ledger testi başarılı. Gerçek private kopyada 12/12 breakpoint adayı, 11 accepted unload; original-context AcLayers ve eski 3-pin apphelp retleri korundu. Toplam 14 child exit ve 16 native girdi hash'i doğrulandı. Gerçek same-base remap gözlenmedi; yalnız metadata corpus kanıtı var. Initialization/SAEX bootstrap/ABI açık kalır.

## 2026-09-13 — D1-N2 explicit launch context / kod 0.1.8 / mimari v0.15

LaunchContext ve --observe-context-loader eklendi: bounded/sorted Unicode environment snapshot, retained directory identity, child öncesi ret ve metadata-only environment digest. SuspendedImage opsiyonel context ile explicit CreateProcess parametreleri kullanır. Loader executable yolu tüm modlarda bir kez çözülür. Yeni native context suite, explicit PATH/cwd loader regresyonları ve iki Python CLI ret/redaction testi eklendi. Eski komutlar inherited davranışı korur; additive launchContext alanı onlar için null olur. Kaldırılan özellik yok; static C++ observer yeniden derlenir, bootstrap C ABI/profile/policy/SDK lock değişmez. ADR-43, owner belgeleri, status/roadmap/component map birlikte güncellendi. [Kanıt ve kalan sınır](d1-launch-context.md).

0.1.8 sonuç: x86 Debug/Release 7 native suite + 57 managed + 49 Python; x64 Debug 5 native suite + 57 managed + 30 Python geçti. Gerçek son Debug private gözlemi breakpoint adayı, Release private UNLOAD_DLL_DEBUG_EVENT güvenli ret, original AcLayers ret verdi; tüm 7 child çıkışı ve 16 native girdi hash korunumu doğrulandı. İlk cold/warm handle ve cwd/AppHelp test hataları raporda korunur. DLL unload/remap, initialization/SAEX bootstrap/ABI sıradaki açık N2 işleridir.

## 2026-09-13 — D1-N2 incelenmiş loader profili / kod 0.1.7 / mimari v0.14

Exact engine'e bağlı 22 modüllü JSON recipe, deterministic C++ generator, PreparedLoaderPolicy ve açık --observe-reviewed-loader girişi eklendi. Bütün origin/ad/hash/bütçe spec'leri IO öncesi, bütün gerçek dosya hash/boyutları child öncesi doğrulanır. LoaderFile kalan byte bütçesini hash öncesi uygular. Kısmi pin seti görünmez; eksik/drift dosyası için failedPolicyModule kaydedilir. Eski üç dosyalı --observe-loader korunur; JSON'a additive policySourceDigest/failedPolicyModule alanları gelir. Bootstrap ABI, observed engine JSON'u ve SDK kilidi değişmedi; kaldırma/migration yoktur.

[Rapor](d1-loader-policy.md) ve ADR-42 source policy/Windows compatibility/launch context ayrımını kaydeder. İlk generator kültürel sıralamayı reddetti; ordinal JSON düzeltildi, kontrol gevşetilmedi. Orijinal GTA Apphelp sonrasında AcLayers mapping'inde durdu. Ayrı, bit eşitliğinde üç dosyalı yerel kopyada Debug 22/Release 21 DLL ve başlangıç breakpoint adayı gözlendi; süreç kapatıldı. İki farklı launch context'in initialization kanıtı birleştirilmez. Eksik/değişmiş kopya DLL ile child yaratılmaması doğrulandı. Kaynak→normatif belge eşlemesi, status/plan/roadmap, AC-90/conformance, workflow/indeks/kaynaklar birlikte güncellendi; x86 Debug/Release 6 native suite, 57 managed ve 47 Python testi; x64 Debug 4 native suite, 57 managed ve 30 Python testiyle geçti. Release original/private/legacy deneyleri kendi farklı outcome türleriyle rapora kaydedildi; bütün owned child çıkışları ve native girdi hash korunumu doğrulandı.

## 2026-09-13 — D1-N2 sınırlı Windows loader gözlemi / kod 0.1.6 / mimari v0.13

SDK bağımsız x86 LoaderFile/LoaderObservation, ayrı saex_engine_loader_probe ve DLL/TLS/main canary fixture'ları eklendi. Açık dosya ID/boyut/hash pinleri, üç sistem dosyasıyla sınırlı CLI politikası, event/thread/module/byte/time bütçeleri, ilk exception'da durma ve terminal olay ilerletilmeden owned child sonlandırma uygulanır. SuspendedImage eski varsayılan davranışı korunarak ayrı friend üzerinden deney desteği aldı; bootstrap C ABI, core/GNS/HTTPS kararı ve N1 SDK kilidi değişmedi. Kaldırılan özellik veya migration yoktur.

[Rapor](d1-loader-observation.md), ADR-41, ilgili engine/native/launcher sözleşmeleri, eski observer/preflight kapsam açıklamaları, AC-90/conformance/R-01, status/plan/foundation/workflow/roadmap/README/blueprint/kaynaklar ve component-map birlikte güncellendi. Yeni x86 native suite ve altı CLI negatif testi standart build'e bağlandı. İlk Debug fixture 12 warm çevrim ve DLL/TLS/main kontrollerini geçti; gerçek GTA'da apphelp.dll mapping'i izin listesinde bulunmadığı için exit 1 ile duruldu, owned exit ve 13 native dosyanın değişmediği doğrulandı. X86 Debug/Release tam akışları 6 native suite, 57 managed ve 38 Python testiyle; x64 Debug 4 native suite, 57 managed ve 22 Python testiyle geçti. İki x86 gerçek GTA deneyi aynı apphelp ret/exit sonucunu verdi. Ret pass'e çevrilmez; initialization/N3/D1 açık kalır.

## 2026-09-13 — D1-N2 native başlangıç envanteri / kod 0.1.5 / mimari v0.12

Gerçek `bass.dll` girdisindeki raw padding farkı metadata notu olarak ayrıldı; dosya/image/overlap sınırları ve runtime kapıları korunur. Kaynağın loader uyumluluğu veya güvenilirliği bu gözlemle onaylanmaz.

Negatif testler, InvalidDataException'ın yerel aday ve CLI filtrelerine açıkça alınması gerektiğini yakaladı. Hata düzeltildi; mevcut engine inspect'in bu girdi hatalarında yakalanmamış exception yerine JSON/exit 1 döndürmesi de iki CLI regresyonuna bağlandı.

C# PeStartupInspector/NativeStartupInspector ve engine startup CLI eklendi. Bounded x86 PE EXE/DLL entry, import/delay VA-RVA, TLS callback metadata, upper-directory DLL/ASI aday grafiği, byte/modül/girdi limitleri, partial sorunlar ve inventory digest uygulanır. Native DLL/assembly yükleme ve process oluşturma yoktur; oyun dosyasına yazılmaz. Mevcut inspect başarılı JSON/exit sözleşmesi, native observer/bootstrap, C++20 core, C ABI ve SDK kaynak kilidi değişmedi; yeni bağımlılık veya kaldırılan özellik yoktur.

[Uygulama raporu](d1-native-startup.md), yeni managed corpus ve gerçek kurulum bulgularını ayrı kaydeder. ADR-40, engine/native/launcher sözleşmeleri, status/plan/workflow/foundation, AC-90/conformance/R-01, README/blueprint/roadmap/source-map/kaynaklar aynı kesitte güncellendi. Tam statik metadata bile canAdvanceToLoader=false; native runtime kapıları açık.

Sonuç: x64 Debug ve x86 Release standart akışları geçti; x64 4/x86 5 native suite, 57 managed test (31 yeni), x64 22/x86 32 Python testi. Debug/Release gerçek kurulum envanteri aynı digest ile executable + 12 DLL/ASI ve 75 import ilişkisini verdi; metadata issue sıfır, bass.dll padding notu açık. 13 native dosyanın hash'i değişmedi. N1 source verify geçti; SDK/native runtime fazı ilerletilmedi. 72 Markdown/899 yerel link/6 JSON örneği belge kontrolü temiz.

## 2026-09-13 — D1-N2 askıda süreç / kod 0.1.4 / mimari v0.11

SDK bağımsız ayrı SuspendedImage/CLI eklendi: exact preflight sonrası owned child, ilk create-debug olayının Windows tarafından duraklatılması, file ID/base/bounded header ve anchor okuma, kill-before-continue ve doğrulanmış çıkış. Oyun kodu resume edilmez; GTA DLL yükleme/hook yoktur. Canary kontrolü, 12 fixture lifecycle çevrimi, identity/bounds/thread/stop/unwind testleri ve 5 CLI ret testi build'e bağlandı. Sentetik PE iki CLI testi arasında ortaklaştırıldı. Mevcut bootstrap ABI, observed profile ve N1 SDK kilidi değişmedi; kaldırılan özellik yoktur.

ADR-39, engine/native sözleşmeleri, AC-90/R-01, uygulama planı/status/workflow/component map, foundation/preflight devam kayıtları, README/blueprint/roadmap birlikte güncellendi. [Ölçüm raporu](d1-suspended-process.md) tamamlanmış test ve gerçek süreç sonuçlarını ayrı kaydeder; initialized/unpacked GTA ve N3 kanıtı açık kalır.

Sonuç: x64 Debug ve x86 Debug/Release standart build/test, her yapılandırmada 12 fixture yaşam çevrimi ve üç gerçek GTA create-debug image gözlemi geçti. X64 4/x86 5 native suite, 26 managed, x64 22/x86 32 Python testi. Dosya hash'i korundu ve owned child exit doğrulandı; bootstrap DLL hash'leri değişmedi. İlk fixture denemesindeki CREATE_SUSPENDED kaynaklı event timeout gizlenmeden raporlandı; debug olayının OS duraklatmasıyla düzeltildi. Sonuç initialized GTA/ABI/hook desteği değildir.

## 2026-09-13 — D1-N2 başlangıç DLL / kod 0.1.3 / mimari v0.10

SDK bağımsız yüklenebilir x86 saex_bootstrap.dll, C ABI 1 Initialize/Query/Stop ve portable BootstrapSession eklendi. SAEX DllMain boş ve session constinit; başlangıç kendi host executable yolunu OS'den bulur. ABI/output doğrulaması, bir kez gözlem, terminal rejected/stopped ve concurrent BUSY uygulanır. can_attach ve bindings_loaded bütün yollarda sıfırdır; gerçek GTA loader/hook veya binding eklenmedi. [Modül raporu](d1-bootstrap-module.md).

Mevcut native image CLI'nin salt okunur dosya/hash/PE denetimi ortak WindowsFileObservation RAII yordamına taşındı; CLI JSON/exit ve gözlem profili değişmedi. Üretilmiş DLL'nin x86/import/export/TLS/CRT/map denetimi CTest load adımından önce standart x86 build'e bağlandı. Source map, execution/engine/native SDK sözleşmeleri, ADR-38, R-01/AC-90, N2 planı, status/workflow/README/roadmap/blueprint birlikte güncellendi. N1 SDK lock/source/recipe değişmedi; kaldırılan ürün özelliği yoktur.

Yeni testler: portable session, oyun dışı gerçek DLL lifecycle ve 10 artifact audit negatif testi. İlk cold handle baseline beklentisi başarısız oldu; load-only kontrolü ve ilk Initialize ayrı ölçüldü. İlk artış ayrıca raporlanır, her sonraki 100 warm çevrimde tam eşitlik istenir. Bu düzeltme genel sıfır leak iddiası vermez. Release CRT XTZ null alignment padding'i artifact'te gözlenip yalnız sıfır byte şartıyla denetime alındı; dinamik initializer/callback toleransı eklenmedi. Güncel platform ve artifact kanıtları modül raporundadır; gerçek GTA'ya yükleme/çalıştırma yapılmadı.

Sonuç: standart x64 Debug ve x86 Debug/Release build/test geçti. X64 üç, x86 dört native suite; 26 managed; x64 17/x86 27 Python testi. Her x86 yapılandırmasında 100 ölçülen oyun dışı DLL çevrimi geçti; cold/warm handle değerleri ve artifact kimlikleri saklandı. X86/x64 gerçek dosya/image regresyonunda dört anchor ve kaynak/dosya digest'leri eşleşti; oyun dosyası değişmedi. N1 lock verify başarılı. Belge sonucu 70 Markdown, 834 yerel bağlantı, 6 JSON örneği, sıfır hata. N2 gerçek GTA/symbol/load sırası, N3 hook ve Linux kanıtı açık.

## 2026-09-13 — D1-N2 preflight alt kümesi / kod 0.1.2 / mimari v0.9

SDK bağımsız C++20 PE parser/profile gate/Windows reader ve `saex_engine_image_probe` eklendi. Exact gözlem JSON'u native constexpr veriye üretilir, C# EngineInspector'a gömülür; duplicate key, bounded anchor ve observation-only kontrolleri vardır. Bilinmeyen hash Windows image eşlemesinden önce reddedilir. Aynı salt okunur handle'dan CNG hash ve `SEC_IMAGE_NO_EXECUTE` görünümü elde edilir; başlıklar ve kısa RVA anchor'ları karşılaştırılır. Profil kaydı GTA executable'ını içermez.

Yerel dosyanın 11 bölümü ve dört anchor'ı eşleşti; Hoodlum marker'ı aday sürüm bulgusudur. `canAttach=false` ve `recognizedProfile=false` korunur. Yeni CLI/gözlem alanları ve neden adları [N2 raporunda](d1-engine-preflight.md) kayıtlıdır; kaldırılan ürün özelliği yoktur. Native image observation exit 3, girdi reddi exit 1 kullanır. Gerçek GTA çalıştırma/SDK yükleme/hook veya başlatılmış process image kanıtı yoktur; N2 runtime/bootstrap kısmı ve N3 açık kalır.

Negatif doğrulama: native PE mutation/truncation ve fake image corpus; 26 managed test; 8 generator, 4 gerçek CLI ret ve 5 belge tooling testi. İlk x86 derlemesinde `SIZE_T/uintptr_t` template tür farkı ve testte signed/unsigned karşılaştırma uyarısı düzeltildi; /W4 /WX gevşetilmedi. Güncel platform/koşu kanıtı N2 raporuna işlenir. N1 kaynak/recipe/patch envanteri değiştirilmedi.

Belgeler: N2 raporu, ADR-37, engine/native SDK/execution sözleşmeleri, AC-90/R-01 ilerlemesi, uygulama planı/status/workflow, component map, kaynak incelemesi, README/blueprint/roadmap birlikte güncellendi. Component map N2 testlerini kendi raporuna, N1 testlerini N1 raporuna bağlar. Genel source→belge zorunluluğu sürer.

Sonuç: x64 Debug, x86 Debug ve x86 Release standart build akışları başarılı. Üç gerçek dosya/image gözlemi, C#/native profil digest eşleşmesi ve öncesi/sonrası oyun dosyası hash'i doğrulandı; artifact kimlikleri N2 raporunda. N1 dependency verify başarılı; kaynak kilidi değişmedi. Statik belge sonucu 69 Markdown, 801 yerel bağlantı, 6 JSON örneği, sıfır hata. Linux/x64 Release yeni kesitte çalıştırılmadı; gerçek GTA process testi yapılmadı.

## 2026-09-12 — D1-N1 native dependency / kod 0.1.1 / mimari v0.8

Önceki doküman değişikliği korunarak gerçek [source/patch/recipe lock](../../contracts/engine/plugin-sdk.lock.json), bounded edinme/doğrulama, standalone private x86 SDK library/probe ve opt-in build/test/evidence girişi eklendi. 23 upstream dosyası seçildi; installer/örnek/GTA kodu çalıştırılmadı. Cache orijinal ve iki patch'li build kaynağı olarak ayrılır; hash/ek header/patch/recipe uyuşmazlığı derlemeyi durdurur. Kaynak envanteri ve notice kapsamı [N1 raporundadır](d1-native-dependency.md).

C++20 denemesi SafetyHook std::expected gereksiniminde başarısız oldu. ADR-36 yalnız private SDK hedefinde kilitli C++23 derlemesini kabul eder; core C++20 kalır. C4458 için private üye adları düzenlendi; C4201 native union'a dar push/pop istisnasıdır. /W4 /WX ve /permissive- korunur. Doğru symbol link'i ve CPool layout'u GTA erişimi olmadan sınanır; dışarıdan hazır library kabul edilmez.

20 yeni test tanımı: bir SDK probe'u, 17 dependency testi, iki gerçek CMake x64/eksik-source reddi. x86 Debug/Release opt-in SDK build ve 20 yeni test tanımı geçti; x64 Debug/x86 Debug foundation regresyonu da 16 native + 25 managed/entegrasyon + 5 tooling ile geçti. Toplam 66 farklı test tanımı. İzole cold acquire ve EXE/lib/PDB hash eşleşmesi ayrıca doğrulandı. Bu bölümdeki önceki v0.7 kaydının eşzamanlı kod eklenmesine ilişkin co-change uyarısı bu kesitin sahip belge güncellemeleriyle giderildi; tarihsel kaydı değiştirilmedi.

Belgeler: engine/SDK/execution sözleşmeleri, N1 planı/kanıtı, status/workflow/README/roadmap/blueprint, ADR, research ve acceptance/conformance güncellendi. Overview'deki stale transient iş ile committed receipt ve persistence ret ile OutcomeUnknown ayrımları mevcut v0.5 kurallarıyla düzeltildi; bu düzeltmeler yeni persistence kodu değildir. Kaldırılan ürün özelliği yoktur. Sıradaki iş N2 executable/symbol/SDK bağımsız bootstrap; GTA attach, production IPC, GNS ve sandbox açık kalır.

## 2026-09-12 — Plugin-SDK doküman entegrasyonu / mimari v0.7 (tarihsel kayıt)

Kullanıcının Plugin-SDK-SA'yı üst düzeyde proje dokümanlarına entegre etme ve uygulamaya yol gösterme talebi işlendi. Dryxio/plugin-sdk-sa kaynak pini `b55e89b336a81448c1aa1a5b188431c9845ebaa9` seçildi; seçilmiş kaynak/build/lisans dosyaları salt okunur incelendi. SDK çalıştırılmadı veya build'e bağlanmadı. Kod sürümü 0.1.0 olarak kaldı.

Eklenenler: [kaynak bulguları](../references/plugin-sdk-sa.md), [normatif native SDK sözleşmesi](../architecture/native-sdk-integration.md) ve [D1-N1–N7 uygulama planı](d1-engine-integration.md). ADR-35, R-01a/b/c, R-02a/b/c, R-04a ve AC-89–96 ile kaynak/build lock, SDK bağımsız profil/bootstrap, hook ownership/drain, ABI/IPC, frame watchdog, pool generation, asset lease ve upgrade evidence şartları tanımlandı. 20 başlat/kapat ve 100 entity/asset döngüsü asgari deney hedefleridir; ölçülmüş sonuç değildir.

Bulgu etkisi: SDK sürüm marker'ı executable doğrulaması sayılmaz; callback remove hook restore değildir; upstream VS2026/C++latest/statik CRT ayarlarının yerel C++20 build'ine uyumu açık kapıdır. Genel attach/unload hedefi temiz session/process stop ve ayrıca kanıt isteyen dinamik DLL unload olarak ayrıldı. Kaynak pin/patch/recipe veya engine değişimi eski kanıtı otomatik taşımaz.

Güncellenen bağlantılar: README/blueprint, status/roadmap/workflow, engine/overview, research/ADR/sources, kabul/conformance, native güvenlik ve launcher. Component map yeni engine/tool/test kaynak ailelerini sahip belgelere bağlar; gelecekte EngineInspector değişiminde SDK sözleşmesi/planı da güncellenir. Tarihsel D0 ve foundation test kayıtları geçmiş kapsamlarıyla korunur.

Doğrulama: `tools/build.ps1 -Architecture x64 -Configuration Debug` gerçek Python runtime yolu verilerek geçti; generated contracts kontrolü, **16/16 native**, **25/25 managed/entegrasyon**, **5/5 tooling** testi başarılı. Belge kontrolü **67 Markdown, 724 yerel bağlantı, 6 JSON örneği, sıfır hata** raporladı. 96 AC ve 35 ADR tanımı tekil/eksiksiz; yedi temsilî yeni/genişletilmiş kaynak yolu için eksik sahip belge güncellemesinin gate tarafından reddedildiği ayrıca kontrol edildi. Başlangıç kopyasına göre 20 dosya değişti; yerel inceleme farkı `out/verification/plugin-sdk-doc-review.diff`, yapısal rapor `out/verification/plugin-sdk-doc-review.json` altındadır. Git deposunda henüz commit olmadığından boş Git diff'i doğrulama yerine kullanılmadı.

Bu koşu x64 foundation regresyonudur; SDK compile/link, yeni AC-89–96, bootstrap ve gerçek GTA testleri yapılmadı. X86 foundation'ın eski kanıtı kendi D1 kaydındadır; bu doküman değişikliğinde yeniden çalıştırılmadı. Kaldırılan ürün özelliği yoktur; runtime davranışı ve mevcut `canAttach=false` sonucu değişmedi. İlk kod işi D1-N1'dir; D1/D2 açık kalır.

Son kontrol sınırı: başarılı build'in 23:31 yerel kayıt zamanından sonra, eşzamanlı başka çalışma `src/engine/plugin_sdk/sdk_probe.cpp` ve `tools/native/CMakeLists.txt` dosyalarını ekledi. Son ortak çalışma dizini kontrolü bu eklemeler için status/engine-adapter/workflow güncellemelerini eksik buldu. Bu kaynaklar doküman teslimatının değişiklik/test kapsamına alınmadı; ortak dizinin son gate sonucu başarılı olarak sunulmaz. Önceki başarılı build ve bu sonraki co-change hatası ayrı sonuçlardır; uygulama ilerledikçe kendi kaynak sahibi dokümanlarıyla kapanır.

## 2026-09-12 — D1 foundation / kod 0.1.0 / mimari v0.6

Kullanıcının kodlama başlangıcı ve eşzamanlı belge güncelleme talebi uygulandı. D0 v0.5 tasarım seti korunarak C++20/.NET 10 build temeli, ortak kimlik generator'ı, native lease/clock ve bounded inbox, metadata WorldPlan aracı, salt okunur engine inspector ve gerçek iki süreçli conformance fixture eklendi. Yerel Git deposu başlatıldı; remote/commit/push yapılmadı.

Davranış: tick donması lease'i uzatmaz; expired/revoked grant yenilenmez; stale owner/revision/sequence/alan maskesi reddedilir. Provider/mutator ve dependency/critical closure çelişkisi plan metadata'sında bulunur. Frame'ler explicit little-endian ve bounded'dır; x86/x64 farkı raw pointer paylaşmaz. GTA dosyası incelenir ama doğrulanmamış profile attach verilmez.

Belgeler: [execution](../architecture/execution-contracts.md), [time](../architecture/time-fencing.md), [authority](../networking/authority.md), [WorldPlan](../architecture/world-plans.md), [engine](../architecture/engine-adapter.md), [SDK](../resources/runtime-sdk.md), README/blueprint/roadmap ve araştırma durum bağlantıları güncellendi. ADR-33/34 ile kod–belge kapısı ve fixture kapsamı kaydedildi. Eski D0 doğrulama raporu tarihsel kapsamıyla korundu.

Test: Windows x64 Debug/Release ve x86 Debug native/managed entegrasyon testleri; invalid plan/PE/frame, zaman/kimlik/kota negatif senaryoları. 16 native + 25 managed/entegrasyon + 5 tooling, 46 farklı test tanımı. İsimli testler ve kanıt sınırı [D1 raporunda](d1-foundation.md). `tools/build.ps1` belge ve generated-code drift kontrolünü build girişine bağlar; kod silinmesi de sahip belge güncellemesi ister. 64 Markdown ve 665 yerel bağlantı denetimi temiz; kullanıcıya özel Python runtime komutu workflow'a kaydedildi.

Kaldırılan ürün özelliği yoktur. “Yalnız Markdown çalışma dizini” durumu kullanıcı onayıyla D1 kodlamaya geçiş olarak güncellendi. GNS/HTTPS, C++20/.NET 10, x86 GTA/x64 Host ve sunucu otoritesi kararları korunur. Native GTA entegrasyonu, Linux runtime kanıtı, güvenlik yalıtımı, GNS kimlik entegrasyonu ve D2 multiplayer açık kalır.

0.1.8 gerçek context deneyi, loader_unexpected_event için event türünün raporda eksik olduğunu gösterdi. lastEventCode/lastEventThreadId additive alanları ve regresyonları eklendi; bilinmeyen olay hâlâ terminal ret alır. Başarısız Debug deneyini Release başarısıyla gizleme veya mevcut policy'yi genişletme yoktur.

0.1.11 aynı kesitin ek-pini: ilk gerçek initializer deneyi imm32.dll'de ret verdi. Yerel metadata/hash/import ve Microsoft imzası incelemesinden sonra ayrı entry-policy.json/compiler/header eklendi. Base source/hash değişmedi; ek tablo 1–8 system-only kayıt, base/engine digest ve override yasağıyla sınırlıdır. Sekiz generator testi ve build --check eklendi; ilk ek-pinli Debug özel kopyada entry byte mutation ve owned exit doğrulandı.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.
