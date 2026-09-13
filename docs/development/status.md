# Uygulama durumu ve doğrulama sınırı

Sürüm: mimari v0.20 / kod 0.1.13, D1 foundation + native dependency + N2 preflight, bootstrap, askıda process, statik envanter ve sınırlı loader gözlemi alt kümeleri. Tarih: 13 Eylül 2026. Kullanıcı kodlamayı ve her kaynak değişiminde belgelerin güncellenmesini onayladı. [Ana indeks](../../README.md) · [Çalışma akışı](workflow.md) · [Foundation](d1-foundation.md) · [N1 kanıtı](d1-native-dependency.md) · [Değişiklik kaydı](change-log.md)

## Gerçek durum

### Public GitHub kaynak yayını

13 Eylül 2026: **SAEX — San Andreas Extended**, [saex-platform](https://github.com/saex-platform) organizasyonunda Owner `Rohatcengizhanbucak` yönetiminde yayımlandı. Public [saex](https://github.com/saex-platform/saex) kaynak monoreposu ve [.github](https://github.com/saex-platform/.github) profil/topluluk deposu; MIT, doküman/marka varlıkları, issue/PR şablonları, güvenlik bildirim kanalı ve [geliştirme panosu](https://github.com/orgs/saex-platform/projects/1) hazırdır. Her iki `main` dalında PR, linear/squash geçmişi, silme/force-push engeli uygulanır; ana depoda yedi GitHub Actions kontrolü zorunludur.

Kaynak commit'i `e5e2dd0d77e37447eca4b7e32c0b89306978c096` için **GitHub'daki yedi kontrol geçti**: Windows x64/x86 Debug/Release, Linux x64 Debug, doküman ve sır taraması. Her build'de 79 managed test; Windows x64 6 native suite/46 Python, x86 9 suite/67 Python, Linux 4 suite/37 Python başarılı. Ayrı kısa temiz checkout N1 SDK Debug/Release'te 1 probe + 17 dependency + 2 configure ret testi geçti; manuel hosted SDK işi çalıştırılmadı. İlk başarısızlıklar, düzeltmeler ve kanıt bağlantıları [yayın raporunda](github-publication.md) korunur.

Bu kaynak yayını D1'i kapatmaz; D2 oynanabilir multiplayer veya native binary release değildir. 0.1.12/0.1.13 proxy ve startup geliştirmesi bu ilk yayın kanıtının kapsamı dışındadır. Yeni kaynaklar yayımlanmış tabanla birleştirildi; bu sürümün GitHub doğrulaması [yayın raporunda](github-publication.md#kod-0113-kaynak-birleştirmesi) ayrıca izlenir.

### Ürün bileşenleri

| Bileşen | Kodlanmış davranış | Doğrulama | Kalan sınır |
|---|---|---|---|
| Startup çağrı sınırı | Üçüncü durak, main-thread IAT watch, FF15 callsite/stack argument ve bounded anchor gözlemi | Dört Windows standart akışı; 19 senaryo + control + 12 warm; 12/12 private GTA startup hit ve dört original/legacy regresyon; [rapor](d1-startup-call.md) | Fonksiyon gövdesi, dinamik load/SAEX DLL ve N2/D1 açık |
| Proxy dönüş sınırı | Ayrı iki-hit API/CLI, compiled entry/module bağı, thunk/return pointer ve entry restorasyonu ve IAT hedef kontrolü | Dört Windows standart akışı; 19 senaryo+12 warm; 12/12 private GTA proxy dönüşü ve üç gerçek legacy/original regresyon; [kanıt](d1-proxy-return.md) | Asıl giriş/unpack, dinamik LoadPlugins, gerçek SAEX DLL/ABI ve N2/D1 açık |
| Kontrollü entry boundary | Ayrı run_to_entry/CLI, CREATE_PROCESS safhasında DR0 kurulumu, DLL/TLS izni ve PE giriş fault/16 byte gözlemi | x86 Debug/Release corpus ve 12/12 gerçek özel GTA entry hit; dokuz unload, aynı vorbisfile RVA 0x1D60 hedefi; [kanıt](d1-entry-boundary.md) | Sadece owned main thread sınırı; initialized GTA/SAEX DLL/ABI ve N3 açık |
| Native sembol bağlantı denetimi | C# engine linkage; normal/delay thunk, exact isim/ordinal, alias/hole, forwarder ve compact binding | 22 yeni corpus kaydı; x86 Debug/Release ve x64 Debug 79/79 managed test; 10 gerçek dosya karşılaştırması ve bağımsız dumpbin eşleşmesi; [rapor](d1-native-linkage.md) | Runtime DLL seçimi, dinamik proxy, initialization ve ABI doğrulanmadı |
| Loader mapping lifecycle | Fixed history, gözlem içi mapping ID, known-active UNLOAD ve her LOAD için yeniden pin kontrolü | 10 metadata senaryosu + Windows corpus geçti; 12/12 private breakpoint adayı, 11 unload; [kanıt](d1-loader-lifecycle.md) | Initialization/SAEX bootstrap/ABI ve N3 açık |
| Explicit launch context | Bounded UTF-16 environment snapshot, absolute cwd/directory identity ve ayrı context CLI | Windows x86 Debug/Release ve x64 Debug; ortam/cwd ve 12 warm çevrim geçti; gerçek mapping/UNLOAD retleri [raporda](d1-launch-context.md) | Tam initialization/SAEX DLL/ABI ve sandbox yok |
| İncelenmiş loader profili | Compiled JSON→C++ recipe, exact engine/DLL hash bağı, iki origin ve child öncesi bütün pinleri hazırlama | Orijinal kurulumda AcLayers ret; üç dosyalı ayrı kopyada Debug 22/Release 21 DLL ve breakpoint adayı; eksik/değişmiş DLL ile child yok; [rapor](d1-loader-policy.md) | Launch context'ler farklı; initialization/SAEX DLL/ABI ve N3 açık |
| Sınırlı loader gözlemi | Ayrı x86 CLI; aynı dosya ID/hash pinleri, bounded LOAD_DLL olayları, ilk exception'da durma ve owned child cleanup | DLL/TLS/main fixture canary'leri, 12 warm çevrim, kimlik/kota/CLI negatifleri; gerçek GTA apphelp mapping'inde beklenen ret; [kanıt](d1-loader-observation.md) | Windows loader kısmen ilerler; initializationVerified=false. Tam closure, initialized GTA, SAEX DLL yükleme ve N3 açık |
| C++20 çekirdek temeli | StrongId/EntityRef, checked counter, ControlClock, LeaseAuthority, BoundedInbox | Windows x64 Debug/Release ve x86 Debug/Release, 16 native test/koşu | Tam scheduler/entity registry/server executable değil |
| Ortak şema | JSON kaynağından C++/C# kimlik tipleri ve fixture sürüm sabitleri | Deterministic generator ve stale-output kontrolü | Genel ContractSchema/SDK/proxy üretimi değil |
| Süreçler arası fixture | 44 byte sınırlı EntityRef frame, açık little-endian codec, version/length/zero-id ret | x64 C# → x64 ve x86 native process, pozitif/negatif test | Production IPC, GTA bridge, sandbox veya oyun protokolü değil |
| WorldPlan aracı | Sınırlı JSON, unique provider/mutator, dependency DAG, critical closure, capability/bütçe metadata | 26 managed/entegrasyon testi içindeki plan alt kümesi | SemVer resolver/cook/gerçek artifact/evidence/activation yok; productionEligible=false |
| EngineInspector ve native preflight | Ortak gözlem JSON'u; PE/hash/layout/anchor eşleşmesi; SDK bağımsız C++20 ve Windows çalıştırılmayan image okuması | Gerçek dosya: dört anchor eşleşti; sentetik PE/bozuk image/reader retleri; [N2 alt kanıtı](d1-engine-preflight.md) | Gözlem profili runtime desteği değildir; canAttach=false; bu satır dosya/image kanıtıdır; askıda process alt kapsamı aşağıda, initialized symbol/ABI açık |
| Askıda process observer | Exact dosya kapısı → owned suspended child → create-debug file ID/base/header/anchor → doğrulanmış çıkış | x64 Debug ve x86 Debug/Release fixture + gerçek GTA ilk image eşleşmesi; [kanıt](d1-suspended-process.md) | Oyun kodu yürütülmez; canAttach=false; unpack/ABI/DLL load kanıtı değildir |
| Native başlangıç envanteri | C# engine startup; bounded PE32 EXE/DLL import/delay/TLS/entry, yerel DLL/ASI aday grafiği ve digest | 57 managed test ve Debug/Release gerçek kurulum metadata'sı; [kanıt](d1-native-startup.md) | Statik metadata; Windows resolution/dinamik yükleme veya clean-install onayı değil; canAdvanceToLoader=false |
| SDK bağımsız bootstrap DLL | x86 C ABI 1, Initialize/Query/Stop, kendi host yolunu doğrulama, terminal ret/stop, BUSY | Oyun dışı DLL yükleme ve 100 ölçülen lifecycle çevrimi; session ve artifact negatif testleri; [rapor](d1-bootstrap-module.md) | Gerçek GTA'da yüklenmedi; can_attach=0; hook, binding ve production loader yok |
| Plugin-SDK-SA dependency | 23 dosyalı exact source/patch/recipe/notice lock, açık edinme, private x86 library/probe; ADR-36 private C++23 uyarlaması | x86 Debug/Release opt-in build/probe; 17 dependency + 2 gerçek CMake ret testi geçti; [exact kanıt](d1-native-dependency.md) | Seçilmiş build alt kümesi; bootstrap/hook/production IPC/GTA yok; runtimeEligible=false |
| Belge kontrolü | Dosya/link/JSON, kaynak→belge eşlemesi, ekleme/değişme/silme takibi | Foundation ve yeni opt-in native build girişinde zorunlu; 5 tooling testi | Güncel statik sonuç out/verification/docs-check.json; anlamsal doğruluğu tek başına kanıtlamaz |
| Linux x64 | Portable core, preflight/bootstrap session ve managed fixture | GitHub Ubuntu 24.04 Debug: 4 native suite, 79 managed, 37 Python geçti; [yayın kanıtı](github-publication.md) | Windows/GTA adapter veya production server doğrulaması değil |
| GNS / worker runtime / GTA gameplay | Mimari kararları korunuyor | Çalışan oyun döngüsü veya multiplayer bu kesitte sınanmadı | GNS kimliği, sandbox, native frame/pool/collision ve multiplayer yok |

## Aşama kapıları

D0 v0.5 belge teslimatı tarihsel olarak tamamlandı. D1 **başladı ve henüz tamamlanmadı**. R-01 dosya incelemesi, R-13 metadata/kimlik üretimi, R-14 queue primitive ve R-21 lease/clock alt kanıtları oluştu. D1-N1/R-02a ve AC-89'un seçilmiş build alt kümesi eklendi; bütün ana R kayıtları açık kalır. AC-34/35/45/71/72/83/86 [foundation](d1-foundation.md), N1 ise [native dependency raporundaki](d1-native-dependency.md) sınırla yorumlanır. AC-90 dosya/image, oyun dışı DLL ve ilk create-debug görüntü alt kümeleri ayrı raporlarla sınandı; initialized runtime bölümü ve AC-91–96 çalıştırılmadı. 96 AC'nin topluca geçtiği söylenmez.

Bir sonraki kesit **D1-N2 dinamik codec/ASI yolu ve gerçek SAEX bootstrap/ABI**: 0.1.13 ile orijinal entry ilerletildi, GetStartupInfoA yönlendirmesinin ilk çağrısı 12/12 private GTA koşusunda gövde çalışmadan tutuldu. Çağıran/stack alanı ve dört anchor okuması doğrulandı; bunlar tam unpack/initialized engine değildir. Eski üç durak ve original AcLayers ret korundu. Sonraki iş kontrollü dinamik yükleme/SAEX bootstrap C ABI’sini loader lock dışında doğrulamaktır. Kullanıcının kurulum yolu biliniyor; tekrar istenmez. N2, N3–N7, OS sandbox/GNS ve iki istemcili D2 kapıları açıktır.

## Kod 0.1.13 doğrulaması

Windows x86 Debug/Release: her koşuda 11 native suite, 79 managed ve 85 Python testi geçti. X64 Debug/Release: her koşuda 6 suite, 79 managed ve 60 Python geçti. Startup corpus’u 19 senaryo + gözlemcisiz canary kontrolü + 12 warm çevrimdir. İlk custom CRT entry link hatası düzeltildi, başarısız kayıt saklandı.

Gerçek özel kopyada 6 Debug+6 Release startup_call_verified; dört original/legacy regresyonla **16/16 confirmed exit**. Her pozitif koşuda aynı callsite/return address, target ve stack alanı doğrulandı; dört mevcut anchor’ın 48 okuması eşleşti. Orijinal 13/private 3 input hash’i değişmedi. [Tam kanıt](d1-startup-call.md). Linux/hosted CI/N1 bu kesitte yeniden koşulmadı; gerçek SAEX DLL/dinamik wrapper gövdesi ve N2/D1 açık kalır.

## Kod 0.1.12 doğrulaması

Windows x86 Debug/Release: her koşuda 10 native suite, 79 managed ve 76 Python testi geçti. X64 Debug/Release: her koşuda 6 suite, 79 managed ve 53 Python geçti. Proxy corpus’u 19 senaryo+12 warm çevrim; ilk Debug stack overflow giderildi ve başarısız kayıt saklandı. Gerçek özel kopyada 6 Debug+6 Release proxy_return_verified; 3 original/legacy regresyonla toplam 15/15 confirmed exit. Orijinal 13 ve private 3 dosya hash’i değişmedi. [Ayrıntı ve artifact kimlikleri](d1-proxy-return.md).

Linux/hosted CI ve N1 opt-in SDK bu değişiklik için yeniden doğrulanmadı. Proxy sonrası oyun girişi, SAEX bootstrap yüklemesi ve D1/D2 ürün davranışı hazır değildir.

## Kod 0.1.11 doğrulaması

X86 Debug/Release son standart akışlarının her biri 9 native suite, 79 managed/entegrasyon ve 59 Python testiyle geçti. X64 Debug 6 native suite, 79 managed/entegrasyon ve 38 Python testiyle geçti. Yeni entry corpus'u 12 senaryo ve 12 warm çevrimdir; sekiz entry-policy testi ve iki CLI negatif testi eklendi. İlk geç DR0 kurulum hatası ve fixture Apphelp ret sonucunun nedenleri/düzeltmeleri [raporda](d1-entry-boundary.md) korunur. Son standart akışlar geçti; x64 Release/Linux çalıştırılmadı, N1 opt-in SDK build tekrarlanmadı.

Son gerçek matris: 6 Debug + 6 Release özel GTA koşusunun 12/12'si entry_boundary_modified/exit 3 verdi; 9 unload işlendi. Her atlama hedefi vorbisfile.dll+0x1D60 oldu. Orijinal entry komutu AcLayers'ta initializer öncesi ret verdi, private eski context komutu ilk breakpoint'te durdu. Son matriste 14/14 child exit ve orijinal 13 + private 3 native dosyada önce/sonra/0.1.10 hash eşitliği doğrulandı. DLL başlangıcının ilerlediği ölçüldü; oyun giriş atlaması, unpack ve SAEX bootstrap yürütülmedi.

## Önceki kod 0.1.10 doğrulaması

X86 Debug/Release standart akışlarının her biri 8 native suite, 79 managed/entegrasyon ve 49 Python testiyle geçti. X64 Debug 6 native suite, 79 managed/entegrasyon ve 30 Python testiyle geçti. Yeni 22 linkage kaydı bu 79'un içindedir. Üç standart akış da ilk denemede başarılı; x64 Release/Linux çalıştırılmadı. N1 SDK bağımlılığı değişmedi ve opt-in SDK build bu kesitte tekrarlanmadı.

Gerçek kurulum dosyalarıyla Debug/Release toplam 10 salt okunur karşılaştırma: 8 direct metadata complete, wrapper→hooked için 2 beklenen incomplete; toplam 94 direct binding. İki configuration'ın raporları aynı; wrapper 8/hooked 34 export slotu bağımsız dumpbin ile aynı. Orijinal 13 + private 3 native dosyanın önce/sonra ve 0.1.9 kanıtıyla hash eşitliği korundu. [Tam kanıt](d1-native-linkage.md). Gerçek GTA bu kesitte başlatılmadı; initialization, gerçek SAEX DLL ve ABI hâlâ açık. Önceki 0.1.9 OS loader sonuçları kendi tarihsel kapsamını korur.

## Önceki kod 0.1.9 doğrulaması

X86 Debug/Release standart akışlarının her biri 8 native suite, 57 managed ve 49 Python testiyle geçti. X64 Debug 6 native suite, 57 managed ve 30 Python testiyle geçti. Yeni on ledger senaryosu aynı-base reuse/stale ID, duplicate/unknown unload, sıra/wrap/kapasite ve budget iadesi olmamasını doğrular. [Tam sonuç](d1-loader-lifecycle.md).

Altışar Debug/Release gerçek private koşunun 12/12'si breakpoint adayına ulaştı; 11'inde ole32/combase mapping'i emekli edildi. Orijinal explicit context AcLayers, legacy 3-pin apphelp retlerini korudu. Toplam 14/14 child exit ve 13 original + 3 private dosya hash korunumu doğrulandı. Same-base remap gerçek GTA'da görülmedi; initialization/SAEX DLL/ABI ve N3/D1 açık. X64 Release/Linux çalıştırılmadı.

## Önceki kod 0.1.8 doğrulaması

X86 Debug/Release: her biri 7 native suite, 57 managed, 49 Python testi geçti. X64 Debug: 5 native suite, 57 managed, 30 Python testi geçti. Yeni context corpus'u Unicode environment freeze, cwd/PATH ile gerçek own DLL çözümü, child öncesi negatifler ve 12 warm handle çevrimini doğrular. İlk hatalı koşular ve düzeltmeler [raporda](d1-launch-context.md) saklanır; x64 Release/Linux çalıştırılmadı.

Son GTA artifact'ı Debug özel kopyada 21 DLL/breakpoint adayı; Release özel kopyada 19 DLL sonrası desteklenmeyen UNLOAD_DLL_DEBUG_EVENT ret; orijinal kurulumda AcLayers ret verdi. Aynı context'te daha önceki Release aday sonucu da korunur; build türüne göre deterministik başarı vaadi yoktur. Yedi deney çocuğu kapatıldı, 13 orijinal ve 3 kopya native dosya hash'i korundu. canAttach/initializationVerified false; bootstrap GTA'ya yüklenmedi.

## Önceki kod 0.1.7 doğrulaması

X86 Debug/Release standart akışları 6 native suite, 57 managed ve 47 Python testiyle geçti; x64 Debug 4 native suite, 57 managed ve 30 Python testiyle geçti. [Profil raporu](d1-loader-policy.md), original-context AcLayers reddini, private-context Debug 22/Release 21 mapping ve breakpoint adayını, child öncesi iki gerçek negatif senaryoyu ayırır. Release'te eski üç pinli modun apphelp reddi de korundu. Original 13 native girdi ve üç pozitif kopya hash'i değişmedi. Bütün deney çocukları kapandı; N2 initialization/SAEX DLL/ABI ve N3/D1 açık. Linux ve x64 Release bu kesitte yeniden çalıştırılmadı.

Kod 0.1.7 son belge kontrolü: 74 Markdown, 966 yerel bağlantı, 6 JSON örneği ve sıfır hata. İncelenmiş policy generator çıktısı güncel; kullanıcı çalışma ağacı ve oyun kurulumu korunur.

## Önceki kod 0.1.6 doğrulaması

X86 Debug/Release ve x64 Debug standart build/test geçti. X86: 6 native suite, 57 managed, 38 Python testi; x64: 4 native suite, 57 managed, 22 Python testi. Yeni x86 suite 12 warm çevrimde handle stabilitesi ve DLL/TLS/main canary sınırını doğruladı. [Tam loader raporu](d1-loader-observation.md). Debug ve Release gerçek GTA deneyleri ntdll/kernel32/kernelbase sonrasında apphelp.dll için aynı izin listesi reddini verdi; own child exit ve 13 native girdi hash'inin korunduğu doğrulandı. Bu ret, DLL'nin bozuk olduğu veya GTA başlangıcının geçtiği anlamına gelmez. Eski statik/ilk-create gözlemleri ve bootstrap C ABI kapsamı değişmedi; Linux/x64 Release bu kesitte yeniden çalıştırılmadı.

Kod 0.1.6 için son belge kontrolü 73 Markdown/934 yerel bağlantı/6 JSON örneği ve sıfır hatadır; N1'in 23 kaynak dosyası salt okunur verify kontrolünü geçti.

## Önceki kod 0.1.5 doğrulaması

Gerçek envanterdeki `bass.dll` raw padding farkı, güvenli okuma sınırları korunarak açık LayoutNotes kaydına ayrıldı. CLI InvalidDataException filtresi ve padding notunun runtime izni vermemesi regresyonlarla kontrol edilir; nihai koşu sonuçları aşağıdaki raporda kayıtlıdır.

X64 Debug ve x86 Release standart build/test geçti: x64 4/x86 5 native suite, 57 managed test (31 yeni), x64 22/x86 32 Python testi. Gerçek kurulumda executable + 12 DLL/ASI, 75 import ilişkisi ve bass.dll için bir padding notu kaydedildi. Debug/Release snapshot digest'leri eşit; 13 native dosyanın hash'i korundu. [Tam kanıt](d1-native-startup.md). Metadata alt kapsamı tamamlandı, loader fazı ilerletilmedi. N1 source verify geçti. Belge sonucu: 72 Markdown, 899 yerel bağlantı, 6 JSON örneği, sıfır hata.

## Önceki kod 0.1.4 doğrulaması

X64 Debug ve x86 Debug/Release standart build/test geçti: x64 4/x86 5 native suite, 26 managed ve x64 22/x86 32 Python testi. Her yapılandırmada 12 owned child lifecycle çevriminde handle sayısı sabit kaldı; canary, ret ve exception cleanup başarılı. Üç CLI gerçek GTA'nın ilk create-debug görüntüsünü file ID/base/header/dört anchor üzerinden eşleştirdi; ana thread resume edilmeden child exit doğrulandı, orijinal exe hash'i değişmedi. [Artifact ve deney raporu](d1-suspended-process.md). AC-90/R-01a/b'nin yalnız pre-user-code görüntü alt kapsamıdır; unpack/ABI/DLL load, N3 ve D1 açık kalır.

Kod 0.1.4 tarihsel belge kontrolü: 71 Markdown, 872 yerel bağlantı, 6 JSON örneği, sıfır hata. N1'in 23 dosyalı kaynak kilidi salt okunur verify kontrolünü yeniden geçti; SDK yeniden derlenmedi.

## Önceki kod 0.1.3 doğrulaması

X64 Debug ve x86 Debug/Release standart build akışları geçti. X64 üç, x86 dört native suite; 26 managed test; x64 17, x86 27 Python testi başarılı. X86 Debug/Release DLL'leri oyun dışı host'ta 100'er ölçülen load/init/stop/unload çevrimini geçti. Artifact/CRT/TLS denetimi temiz; kaynak kilidi verify edildi. Ortak dosya yordamının x86/x64 gerçek dosya regresyonu geçti, oyun executable'ı değişmedi. Statik sonuç: 70 Markdown, 834 yerel bağlantı, 6 JSON örneği, sıfır hata.

Yeni DLL, portable session ve artifact denetimi [başlangıç modülü raporunda](d1-bootstrap-module.md) kendi kapsamıyla kayıtlıdır. AC-90/R-01b'nin oyun dışı yükleme/ret/stop alt kanıtı oluştu; gerçek GTA load sırası ve R-01c hook kanıtı açık. Process-wide ilk handle artışı ayrıca raporlanır; warm çevrimlerin sabitliği bütün process kaynaklarının sıfır sızıntı kanıtı değildir.

## Önceki kod 0.1.2 doğrulaması

13 Eylül 2026: x64 Debug, x86 Debug ve x86 Release standart build akışları geçti. Aynı üç native probe gerçek dosyayı çalıştırmadan image olarak eşledi; başlık ve dört anchor eşleşti. C# dosya/profil digest eşleşmesi ve oyun dosyasının değişmediği doğrulandı. N1 dependency verify tekrar geçti. Son belge gate'i: 69 Markdown, 801 yerel bağlantı, 6 JSON örneği, sıfır hata.

N2 kod 0.1.2 doğrulama kapsamı: iki native CTest suite'i (16 foundation testi ve yeni PE/profile corpus), 26 managed/entegrasyon testi, 8 profil generator + 4 native CLI ret + 5 belge tooling testi. Platform/koşu sonuçları [N2 raporunda](d1-engine-preflight.md). N1'in x86 Debug/Release 20 test tanımlı eski kanıtı kendi kapsamıyla korunur; N2 için SDK source/recipe değişmedi. Tam lock/artifact kimlikleri ve gerçek GTA doğrulamasının sınırı [N1 raporundadır](d1-native-dependency.md). SDK private derleme sonucu engine/oynanış capability'si açmaz.

## Güncelleme zorunluluğu

Her kaynak ekleme/değiştirme/çıkarma bu tablo, [change-log](change-log.md) ve ilgili normatif sözleşmeyle aynı değişiklikte yapılır. D1'de olmayan bir özellik kodlanınca “planlanan” satırı ölçülen kapsamla değiştirilir; silinirse kullanıcı etkisi ve migration belirtilir. Test sonucu olmayan platform veya native yetenek verified olamaz.

0.1.8 ilk gerçek context koşusunda Debug özel kopya 19 DLL sonrası loader_unexpected_event ile güvenli durdu; iki Release özel kopya koşusu breakpoint adayına ulaştı, orijinal kurulum AcLayers'da ret verdi. Dört child'ın çıkışı ve native dosyaların hash korunumu doğrulandı. Tanı eksikliğini gidermek için additive lastEventCode/lastEventThreadId eklendi; sonuçlar initialization sayılmaz.

0.1.8 son statik belge kontrolü: 75 Markdown, 990 yerel bağlantı, 6 JSON örneği ve sıfır hata. Kaynak/belge eşlemesi başarılıdır; bu statik sonuç anlamsal kusursuzluk veya oyun içi doğrulama yerine geçmez.

0.1.9 son statik belge kontrolü: 76 Markdown, 1013 yerel bağlantı, 6 JSON örneği ve sıfır hata. Bu sonuç kaynak/belge eşlemesi ve statik yapı kanıtıdır; bütün mimarinin anlamsal kusursuzluğu veya GTA initialization başarısı değildir.

0.1.11 ara gerçek kanıt: ilk initializer koşusu imm32 unpinned ret verdi. Ayrı entry-policy.json + strict compiler, mevcut 22 pinin exact digest'ine bir incelenmiş system-x86 imm32 kaydı ekler. Sekiz generator testi geçti; ilk ek-pinli Debug özel kopya entry_boundary_modified sonucuna ulaştı. Son standart toplamlar ve 12/12 gerçek entry tekrarı yukarıdaki 0.1.11 bölümündedir; ilk ret kanıtı korunur.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.

## Kod 0.1.12

[Proxy dönüş sınırı](d1-proxy-return.md) uygulanmıştır; test ve gerçek GTA kanıtı ilgili raporda ayrı tutulur. N2/D1 ve oynanabilir multiplayer kapsamı açık kalır.

## Kod 0.1.13

[Startup çağrı raporu](d1-startup-call.md) implementasyon, fixture ve gerçek GTA kanıtını ayrı tutar. D1 kapıları henüz tamamlanmadı.
