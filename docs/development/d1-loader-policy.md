# D1-N2 — İncelenmiş loader mapping profili

Mimari v0.14 / kod 0.1.7, 13 Eylül 2026. [Durum](status.md) · [Önceki loader](d1-loader-observation.md) · [Native sözleşme](../architecture/native-sdk-integration.md) · [ADR-42](../decisions/architecture-decisions.md). Bu profil yalnız ilk exception'a kadar bounded mapping gözlemi içindir; oyun initialization veya ABI izni değildir.

## İnceleme temeli

GTA'nın normal import grafiği, üç temel loader modülü ve önceki deneyin apphelp girdisi salt okunur incelendi. API set adları fiziksel dosya sayılmadı. Root girdileri vorbisfile.dll ve eax.dll yalnız first-exception mapping gözlemine dahil edildi; native proxy vorbisfile'ın initializer/dinamik ASI taraması onaylanmadı. Statik import aracı araştırma çıktısı otomatik izin değildir; isimler ayrıca seçilip gerekçesiyle profile kaydedildi.

Apphelp.dll SHA-256 `85d8b3d7a05378b778094dbb226f7bf1d929fe4a0b370b6e61b3a366eeda0aef`, dosya sürümü `10.0.26100.8457`, yerel Authenticode sonucu Valid, signer Microsoft Windows olarak gözlendi. HKCU/HKLM 32/64 görünümlerinde bu executable'a özel Layers/Custom/IFEO kaydı ve inherited __COMPAT_LAYER bulunmadı. Bu dar inceleme bütün SDB/policy/compatibility veritabanını taramaz; yokluk, hiçbir shim uygulanmadığını göstermez. Registry/environment değiştirilmedi. Microsoft [compatibility database açıklaması](https://learn.microsoft.com/en-us/windows/win32/devnotes/application-compatibility-database), eşleşmede executable öznitelikleri ve yan dosyaların etkisini açıklar; bu makinedeki exact shim seçimini tek başına kanıtlamaz.

## Kaynak, derleme ve profil sözleşmesi

[Loader policy JSON](../../contracts/engine/loader-policy.json), [generator](../../tools/loader_policy.py), [üretilmiş C++ verisi](../../include/saex/engine/loader_policy.generated.hpp), [pin hazırlayıcı](../../src/engine/loader/loader_policy.cpp) ve [arayüz](../../include/saex/engine/loader_policy.hpp) eklendi. Yeni dependency veya indirilen JSON policy yolu yoktur. Mevcut bootstrap C ABI, SDK source lock ve engine observation JSON'u değişmez.

Profil `schemaVersion=1`, `stage=first-exception-only`, exact engineSha256, sabit reviewDocument ve 22 modül taşır. ID `gta-sa.f01a00ce.windows-26200.loader-mapping-v1`, source digest `91bb9479a01a91fea9161939ed7ac063fce541c9bb9bd35e1a4c3c8e4dce49a8`'dir. Digest UTF-8 JSON dosyasının gerçek byte hash'idir; yeniden biçimleme bile yeni digest üretir. OS build adı açıklayıcıdır; runtime uygunluğu ilan etmez. OS güncellemesi/VC runtime/DLL değişikliği expected hash'i bozarsa gözlem durur; araç otomatik yeniden hash alıp yeni source policy yazmaz.

| Modül grubu | Kapsam ve karar |
|---|---|
| Loader sistemi | ntdll.dll, kernel32.dll, kernelbase.dll; önceki üç dosyalı deney temeli |
| Compatibility | apphelp.dll; imza/disk metadata ve önceki event kanıtıyla yalnız bounded deney girdisi |
| GTA root | eax.dll ve vorbisfile.dll; exact yerel hash, başka dizine fallback yok |
| Diğer doğrudan/transitif girdiler | advapi32, combase, gdi32, msvcp140, msvcp_win, msvcrt, ole32, rpcrt4, sechost, user32, vcruntime140, win32u, winmm, ws2_32 DLL'leri |
| API set host adayları | ucrtbase.dll ve gdi32full.dll; sonraki özel kopya deneyinde mapping'leri gözlendi. Bütün contract→host eşlemesi çözülmedi |

Toplam declared pin byte bütçesi **19.263.912**'dir. 20 system-x86 dosyanın yerel imzaları Valid/Microsoft olarak kaydedildi; iki game-root dosya için Microsoft imzası varsayılmaz. [Windows API sets](https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-apisets), sanal sözleşme adlarının dosya adlarıyla özdeş olmadığını açıklar. Araştırma grafiğinde 160 API set adı vardı; bunlar 160 disk DLL pini veya eksik dosya diye yazılmadı. [WOW64 yönlendirmesi](https://learn.microsoft.com/en-us/windows/win32/winprog64/file-system-redirector) nedeniyle x86 araç kendi OS sistem dizinini kullanır; x64 observer desteği eklenmedi.

Generator unknown/duplicate JSON key, yanlış schema/stage, sıfır veya bozuk hash, traversal/ADS/case/uzun ad, unsupported origin/basis, duplicate basename, integer yerine bool, collection/toplam byte ve sıralama hatalarını reddeder. Source en fazla 64 KiB, modüller 1–64, basename en fazla 127 karakter, dosya en fazla 64 MiB, toplam en fazla 256 MiB'dir. İsimler ordinal ASCII sırasındadır; C++ data deterministiktir. --check stale output'u reddeder, standart build'e bağlandı. Generator kaynakları keşfetmez, DLL çalıştırmaz ve imza doğrulaması yapmaz; inceleme sonrası seçilen veriyi derler.

İlk hazırlamada PowerShell kültürel sıralaması ordinal sıra şartına uymadığı için generator `policy_module_order` verdi. Header oluşmadığından elle başlatılan ara CMake denemesi missing generated header ile durdu. JSON ordinal sıralandı, generator şartı korunarak build geçti; sıralama ret testi eklendi. Bu başarısız hazırlık nihai geçen test sonucu olarak sunulmaz.

## Çalışma akışı, yetki ve hata davranışı

Yeni açık giriş: `saex_engine_loader_probe --observe-reviewed-loader <exact-executable>`. Eski `--observe-loader` üç dosyalı politika ve ret davranışıyla kalır. JSON'a additive `policySourceDigest` ve `failedPolicyModule` alanları eklendi; eski modda digest boş string'dir. Schema 1 alanlarının önceki anlamı korunur. Statik C# engine startup hâlâ process yaratmaz ve canAdvanceToLoader=false verir.

1. CLI exact dosya/profile kapısını uygular; bilinmeyen executable için child/pin hazırlığı başlamaz.
2. PreparedLoaderPolicy engine hash bağı ve **bütün recipe** için basename/origin/hash/sayı/duplicate/toplam sınırlarını IO öncesi kontrol eder. İzin verilen origin yalnız system-x86 veya executable'ın game-root dizinidir; dosya adı mutlak yol/pattern olamaz.
3. Kaynak dizininden her dosya read/share-read handle ile açılır. Dosya başına ve kalan toplam okuma bütçesi SHA-256 öncesi uygulanır. Ad/boyut/hash compiled recipe ile karşılaştırılır. Eksik dosya `loader_policy_file_unavailable`, drift `loader_policy_file_mismatch` verir. İki origin arasında arama/fallback yoktur. Kısmi hazırlanmış pinler çağırana hiç sunulmaz.
4. Bütün pinler hazırsa child yaratılır ve mevcut create-debug executable ID/base/header/anchor kontrolleri uygulanır. Loader event'lerinde aynı canlı dosyanın volume/file ID/boyut/hash karşılaştırması sürer; hash'i aynı başka dosya bile aynı pin değildir.
5. İlk exception, beklenmeyen modül veya limitte durulur; terminal olayı ilerletmeden kill/exit uygulanır. Recipe süreç boyunca canlıdır; cleanup bitmeden pinler bırakılmaz. Pin hazırlığındaki exception için RAII handle'ları kapatır; hazırlık child öncesindedir.

**canAttach=false, initializationVerified=false** devam eder. Exit 1 ret/eksik gözlem, 3 breakpoint adayı + doğrulanmış exit, 2 kullanım hatasıdır. İlk Windows breakpoint'i normal akışa devam ettirilmez. Bu katman native code sandbox değildir: Windows/compatibility kodu olaylar arasında çalışabilir; disk hash'i mapped kodun tam bütünlüğü, OS imza kontrolü veya oyun initialization kanıtı değildir. IO senkrondur; önceki loader'ın 5 saniyelik event bütçesi katı disk I/O deadline'ı değildir. Pin hazırlama için ayrıca preemptive dış watchdog yoktur.

## Gerçek deney bağlamları

| Bağlam | Sonuç | Yorum |
|---|---|---|
| Orijinal kurulum, reviewed mod | Apphelp eşleşti; **AcLayers.dll** 6. olayda unpinned ret; exit 1, child exit doğrulandı | Kullanıcı layer kaydı yokluğu built-in compatibility yolunun yokluğunu göstermedi; exact etkin shim/fix listesi çözülmedi |
| Aynı byte'lı ayrı yerel kopya / Debug | **22 DLL** eşleşti; 27 olay, 4 thread olayı, 19.263.912 byte, `0x80000003` breakpoint adayı; exit 3 ve child exit | Kontrollü bağlamın loader mapping alt kanıtı; GTA initialization veya tam normal oyun deneyimi değildir |
| Aynı yerel kopya / Release | **21 DLL** eşleşti; 26 olay, 4 thread olayı, 18.553.632 byte, aynı breakpoint adayı; exit 3 ve child exit | Bu koşuda apphelp mapping olayı yoktu; bütün 22 dosyanın pin hazırlığı yine tamamlandı |
| Kopyada değiştirilmiş vorbisfile.dll | `loader_policy_file_mismatch`, failedPolicyModule=vorbisfile.dll, childCreated=false | Geçerli GTA executable'ı tek başına başlangıcı yetkilendirmedi |
| Kopyada eksik vorbisfile.dll | `loader_policy_file_unavailable`, childCreated=false | Fallback veya eksik pinle ilerleme yok |

Özel kopya `out/experiments/gta-loader-f01a00ce-v1` altındadır: GTA executable byte'ları `saex-engine-observation.exe` adıyla, eax.dll ve vorbisfile.dll ayrı gerçek dosyalara kopyalandı. Üç SHA-256 kaynakla eşleşti; orijinal dosyalar yeniden adlandırılmadı/değiştirilmedi. Target workspace out/experiments altında doğrulandı, var olan dizin/dosya üzerine yazılmadı. Bu manuel yerel laboratuvar kurulumu, otomatik staging kodu veya dağıtılabilir oyun paketi değildir. Out Git dışında kalır; oyunun binary/asset'leri kaynak depoya/dağıtıma eklenmez. Tam oyun asset ağacı ve diğer ASI/mod dosyaları kopyalanmadı.

Executable adı, dizini ve yan dosya kümesi birlikte farklıdır. AcLayers gözlenmemesinin nedenini yalnız filename veya yalnız registry olarak açıklamak bu deneyden çıkarılamaz. Özel kopyanın Debug/Release koşuları arasında apphelp gözlenmesi de farklıydı; bunun cache, zaman veya build kaynaklı olduğu izole edilmedi. Recipe izin kümesidir; bütün izinli dosyaların her koşuda mapping olayı vermesi veya sabit sırayla gelmesi koşul değildir. **Aynı executable hash'i aynı launch context anlamına gelmez.** CWD proje dizini, environment inherited, Windows 10.0.26200'dır. Save/config/gameplay davranışı, full DLL initialization, unpack ve ABI test edilmedi. Dinamik proxy/ASI çözümü araştırılmadan özel kopya normal yürütmeye bırakılmaz. Exact engine hash'i veya source policy başlatma ortamını tek başına production için onaylamaz.

## Test kapsamı ve kanıt dosyaları

[Native policy/observer testleri](../../tests/engine/loader_observation_tests.cpp), exact hazırlanan dört pinle fixture breakpoint'ine ulaşmayı ve DLL/TLS/main canary'lerinin çalışmamasını kontrol eder. Engine mismatch, boş/aşırı liste, değişmiş hash/boyut, yanlış origin, traversal/duplicate/ad geçersizliği, sıfır hash, dosya/toplam okuma sınırı ve partial pin gizleme testleri vardır. Önceki 12 warm cycle/handle, owner thread, stop ve CLI ret testleri korunur. Testte policy hash'leri kendi fixture'ından alınır; bu production policy üretme yolu değildir.

[Sekiz generator testi](../../tests/engine/test_loader_policy.py), sözleşme reddi ve exact generated header'ı doğrular. [CLI corpus](../../tests/engine/test_loader_probe.py) yedi teste çıktı; yeni reviewed flag bilinmeyen hash ile policy/child aşamasına geçmez. Standart build gerçek GTA kurulumuna bağımlı değildir ve GTA'yı başlatmaz. Orijinal/özel kopya deneyleri ayrıca açık komutlarla yürütüldü.

Yerel kayıtlar `out/verification/engine/loader-policy-candidates.json` (araştırma, canAuthorize=false), `reviewed-loader-system-signatures.json`, `reviewed-loader-initial-evidence-Debug.json`, `reviewed-loader-private-copy-inputs.json`, `reviewed-loader-private-initial-Debug.json` ve `reviewed-loader-negative-Debug.json` altındadır. İlk Debug probe hash'i `0f2116e4a6ecb9925d552da3248e7e2825ef65107ba0cd4b86677b057862a8d8`; exact policy source digest yukarıda kayıtlıdır. Başlangıçtaki 13 original native dosya hash'i korundu.

## Genişleme ve sonraki N2 adımı

Seçilmiş module/system/VC runtime güncellenirse source policy, review ve generated output birlikte güncellenir; yeni Windows profili mevcut pinleri sessizce kabul ettirmez. AcLayers veya başka compatibility modülü yalnız listede görünmesi nedeniyle eklenmez. Loader kendi sonuç dosyasını izin kaynağına dönüştürmez.

Sonraki kesit controlled launch context'i sabitlemek, dynamic proxy/initialization önkoşullarını incelemek ve gerçek SAEX bootstrap/entry-unpack/symbol-ABI gözlemini geliştirmektir. Orijinal kurulumun compatibility kolu ile özel kopyanın farklı kanıtları ayrı tutulur. N2 bütünü, N3 hook lifecycle ve D1 multiplayer geçişi açık kalır.

## Son doğrulama ve artifact kaydı

| Build | Native CTest | Managed | Python | Sonuç |
|---|---|---|---|---|
| Windows x86 Debug | 6 suite | 57 | 47 | Standart akış geçti |
| Windows x86 Release | 6 suite | 57 | 47 | Standart akış geçti |
| Windows x64 Debug | 4 suite | 57 | 30 | Yeni policy generator ve mevcut observer regresyonu geçti; x86 loader target'ı yok |
| Windows x64 Release / Linux | Çalıştırılmadı | Çalıştırılmadı | Çalıştırılmadı | Bu kesitte yeni kanıt yok |

X86 Python toplamı önceki 38 + 8 generator + 1 reviewed CLI; x64 önceki 22 + 8 generator'dır. Native policy senaryoları mevcut loader suite'ine eklendi. X86 bootstrap artifact/100 oyun dışı lifecycle regresyonu geçti; Debug DLL `eb0cdf4c074bee945099103cf661ba3269a8e1b6f1a35f24d670d994ec41f353`, Release DLL `cabf7e8225c4d3c60d7fbc655bee631042376bdcdd550a3140f624dfabc8e8ea` önceki artifact'lerle aynı kaldı. Tam loglar `out/verification/engine/build-policy-x86-Debug.log`, `build-policy-x86-Release.log` ve `build-policy-x64-Debug.log` altındadır.

Release probe SHA-256 `3382f650639121a803ca75847cf61a2af6032006817ebdc04e4e10921e2d629a`. `reviewed-loader-evidence-Release.json` üç ayrı çalışmayı saklar: reviewed/original AcLayers ret (exit 1), reviewed/private breakpoint adayı (exit 3), legacy/original apphelp ret (exit 1). Üç owned child'ın çıkışı doğrulandı; 13 original native girdi ve üç pozitif private copy hash'i korundu. Bu 21–22 mapping alt sonucu GTA'ya SAEX DLL yükleme veya oynanabilir istemci diye sunulmaz.

Son statik belge kontrolü: **74 Markdown, 966 yerel bağlantı, 6 JSON örneği, sıfır hata**. Kaynaklar/üretilmiş veriler ve sahip belgeler aynı kesitte güncellendi. Oyun executable/DLL kopyaları yalnız Git dışındaki yerel deney alanındadır; commit/push, deployment veya kurulum dosyası değişikliği yapılmadı.

## Kod 0.1.8 devamı — Context policy'den ayrıdır

[Explicit context CLI](d1-launch-context.md) mevcut 22 pinli first-exception-only policy'yi aynen tüketir; module hash'leri ve source digest değişmedi. Environment/cwd kimliği yeni ayrı rapor alanıdır, DLL initialization izni değildir. Eski reviewed komut inherited davranışını korur. Root drive executable parent'ı artık C: yerine C:\ biçiminde korunur; bütün loader modlarında executable path bir kez çözülür.

0.1.8 explicit context gözleminde beklenmeyen OS debug olayının tanısı için loader çıktısına lastEventCode/lastEventThreadId eklendi. Son alınan olay metadata'sıdır; initialization kanıtı veya olayın devamına izin değildir. Child öncesi retlerde ikisi de sıfırdır. Unknown event terminal ret davranışı ve incelenmiş DLL policy değişmedi.

## Kod 0.1.9 devamı — Retained pin ve yeniden yükleme

[Lifecycle](d1-loader-lifecycle.md) eklenirken 22 modüllü recipe/digest değişmez. Unload yalnız aktif mapping'i emekli eder, dosya pinini kaldırmaz; aynı base'e yeni LOAD normal dosya handle/volume/file ID/hash/bütçe denetimini tekrar yapar. Yeni yükleme onayı önceki kayıttan kopyalanmaz. first-exception-only ve original AcLayers ret kuralı korunur.

## Kod 0.1.11 — Pin recipe, initializer izni değildir

Bu dosyada tanımlı JSON stage=first-exception-only, source digest ve 22 exact pin değişmedi. [Yeni entry deneyi](d1-entry-boundary.md) aynı kimlik listesini yalnız mapping pin kaynağı olarak kullanır; initialization izni ayrı compiled executionPolicy/CLI scope/ADR-46 ve probe SHA'sına aittir. Eski üç CLI ve prepared-policy retleri değişmez. Fixture'daki 20 system basename seti fresh local kimliklerle yalnız test caller'ının girdisidir; GTA CLI exact hash şartlarını genişletmez. Yeni DLL/policy hash'i runtime'dan otomatik onaylanamaz.

## Kod 0.1.11 — Entry supplement bağı

entry-policy.json içindeki basePolicySha256 bu değişmeyen JSON’un exact SHA’sıdır. Yalnız entry seti 23’e çıktı; 22 kayıtlı base recipe ve first-exception-only stage değiştirilmedi. [Ayrıntı](d1-entry-boundary.md).

### Hosted corpus tamamlaması

İkinci hosted turda bütün native suite'ler geçti; ortak Python loader CLI corpus'undaki cwd string eşitliği kısa/uzun Windows adı nedeniyle hata verdi. Test artık pathlib.samefile ile aynı dizin kimliğini doğrular. Engine unknown-fingerprint reddi, environment redaction, childCreated=false, entry/loader policy ve lifecycle sınırları aynen kalır; önceki GTA artifact sonuçları yeni test kanıtı sayılmaz.

## Kod 0.1.12 — Proxy dönüş sınırı

[Proxy execution recipe](d1-proxy-return.md), mevcut mapping JSON’unu değiştirmeden entry digest üzerinden ona bağlanır. Yeni modül/pin veya override eklenmez; mevcut game-root vorbisfile.dll hash’i ve incelenmiş RVA’lar source’ta sabittir. PreparedLoaderPolicy 23 dosyayı child öncesi tutar; mapping kabulü tek başına proxy yürütme izni olmaz.

## Kod 0.1.13 — Startup çağrı sınırı

[Startup execution source](d1-startup-call.md) proxy→entry→mapping source digest zincirine ve observed profile digest’ine bağlanır. Mapping listesi/origin/hash değişmedi; 23 dosya child öncesi doğrulanır. Dinamik vorbishooked/ASI yolu henüz yürütülmez, yeni DLL keşfi veya otomatik pin ekleme bulunmaz.

## Kod 0.1.14 — Codec dönüş kesiti

Base 22 ve entry/startup 23 pin listesi değişmez. Codec policy ayrı startup digest bağıyla üç game-root dosya ekler; PreparedLoaderPolicy aynı exact hash/file ID preflight kapısıyla toplam 26 pini hazırlar. Eski komutlara codec izni aktarılmaz. [Sözleşme ve doğrulama](d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

Binding policy, codec-policy digest zincirine stop/slot/target RVA tarifini ekler; 26 pin aynı, yeni modül izni yok. Runtime JSON veya otomatik export adresi onayı bulunmaz. [Sözleşme ve kanıt](d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

0.1.16 constructor genişlemesi: `allow_bootstrap_asi=false` varsayılanı eski DLL-only kuralını korur. Yalnız yeni ASI modu `true` verir; game-root exact `saex_bootstrap.asi` dışında ad/origin izni yoktur. Hash/size/file ID, tam recipe doğrulaması ve child öncesi pin hazırlığı değişmez. İlk gerçek preflight bu eski kuralda durmuş; narrow opt-in ve negatif testlerle giderilmiştir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Yeni frame-target CLI aynı absolute working-directory/environment ve pin hazırlama kapılarını kullanır. Ek DLL pini veya context genişletmesi yoktur; yalnız üç EXE okuması eklenir. [Kapsam ve doğrulama](d1-frame-target.md).

## Kod 0.1.19 bağlantısı

0.1.19, aynı profil/pin ve owned child kapıları üzerinde ayrı doğal startup-return deneyi ekler. Mevcut durak davranışı korunur; koruma çağrısı ve dönüş ABI kanıtı yeni raporda izlenir. D1/N2/N3 ve oynanabilir multiplayer kapıları açık kalır. [Sözleşme ve kanıt](d1-startup-return.md).

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.
