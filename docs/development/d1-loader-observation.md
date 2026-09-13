# D1-N2 — Sınırlı Windows loader gözlemi

Mimari v0.13 / kod 0.1.6, 13 Eylül 2026. [Durum](status.md) · [Önceki statik envanter](d1-native-startup.md) · [Motor planı](d1-engine-integration.md) · [Native sözleşme](../architecture/native-sdk-integration.md) · [ADR-41](../decisions/architecture-decisions.md).

## Sorumluluk ve hipotez

Bu kesit, dosyadaki import adayları ile Windows'un gerçekten eşlediği DLL dosyaları arasındaki boşluğu ölçer. SDK bağımsız, yalnız Windows x86 geliştirme observer'ıdır. Kendi child process'inin ilk create-debug olayını açık deney girişiyle ilerletir; kabul edilen her DLL mapping'ini açık dosya kimliği/hash ile eşler; ilk exception veya izin verilmeyen olayda durur. SAEX DLL injection, oyun hook'u, native fonksiyon çağrısı, initialized engine veya production launcher uygulanmaz.

Hipotez: Windows başlangıç loader breakpoint'inde durmak, bize ait fixture'ın DLL DllMain/TLS/main kodunun başlamasını önleyebilir. [Microsoft initial breakpoint açıklaması](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/initial-breakpoint), statik DLL'ler yüklendikten ve initialization rutinlerinden önce bu durağı tanımlar. Bu varsayım üç ayrı canary ile yerel fixture'da sınanır; packed/modlu GTA'nın bütün erken yürütme yolları için genel güvenlik kanıtı sayılmaz.

## Arayüz ve süreç akışı

- [LoaderFile/LoaderObservation arayüzü](../../include/saex/engine/loader_observation.hpp), [uygulama](../../src/engine/loader/loader_observation.cpp) ve [CLI](../../src/engine/loader/loader_probe.cpp) C++20'dir. Yeni vendor kaynağı yoktur; SHA-256 için mevcut Windows BCrypt kullanılır.
- LoaderFile, `GENERIC_READ/FILE_SHARE_READ` ile açtığı dosyayı deney bitene kadar tutar. Aynı handle'dan boyut, volume/FileIdInfo, normalize edilmiş basename ve SHA-256 okunur. Dosya başına 64 MiB sınırı, 64 KiB hash parçası vardır. Kimlik çıktısı ancak bütün kontroller geçince atanır. Basename en fazla 127 ASCII harf/rakam/boşluk/nokta/alt çizgi/tire kabul eder; başka adlar metadata reddidir.
- CLI önce eski exact GTA dosya/profile kapısını uygular. Bilinmeyen hash, yanlış mimari veya bozuk PE için child bile yaratılmaz. Windows'un x86 sistem dizininden **yalnız ntdll.dll, kernel32.dll, kernelbase.dll** açılıp tutulur. Bunlar yerel Windows kurulumuna dayanan deney girdileridir; imza/OS güvenilirliği doğrulaması değildir. Yerel GTA DLL/ASI'leri veya dizin wildcard'ı otomatik listeye alınmaz.
- `SuspendedImage` kendi child'ını `DEBUG_ONLY_THIS_PROCESS` ile yaratır. İlk olayın executable file ID/base/header/dört anchor kontrolleri CLI'de yeniden yapılır. Önceden çalışan PID veya başka debugger process'i hedeflenmez; çağrılar aynı, başka debug oturumu olmayan owner thread'indedir.
- Ayrı friend `LoaderObservation.run`, bekleyen ilk olayı ilerletir. Her LOAD_DLL olayının **hFile** verisi okunur; pin ile volume/file ID/boyut/hash eşleşirse sonraki olaya geçilebilir. Aynı isim ve aynı byte hash'ine sahip farklı dosya bile kabul edilmez. Event file handle'ı her yolda kapatılır; debug process/thread handle'larını Windows yönetir. Caller pinleri çağrı boyunca canlı tutar.
- İlk exception **hiçbir zaman normal yürütmeye devam ettirilmez**. First-chance `EXCEPTION_BREAKPOINT`, ana thread, MEM_IMAGE ve kabul edilmiş ntdll allocation base eşleşmesi yalnız `breakpointCandidate=true` üretir. DbgBreakPoint export/symbol veya instruction signature kanıtı yoktur; initial breakpoint olduğu varsayımından runtime capability üretilmez.
- Terminal olay tutulurken owned child TerminateProcess/own-job fallback ile sonlandırılır; ancak bundan sonra cleanup için debug olayları tüketilir ve gerçek process exit beklenir. Aynı child ikinci kez ilerletilemez. Wrong-thread çağrı owner'ın olayını değiştirmez/öldürmez; cleanup owner'da kalır.

Microsoft [debug olayları](https://learn.microsoft.com/en-us/windows/win32/debug/debugging-events) ve [LOAD_DLL_DEBUG_INFO](https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-load_dll_debug_info) kaynakları, pending event'in process thread'lerini durdurmasını ve hFile'ın debugger tarafından kapatılmasını destekler. `lpImageName` pointer'ına veya process'in bildirdiği path metnine güvenilmez. File hash disk kimliğidir; mapped kodun tam bütünlüğünü veya DLL başlatma güvenliğini kanıtlamaz.

## Sınırlar, hata davranışı ve genişleme

| Alan | Üst sınır / terminal davranış |
|---|---|
| Gözlem | Create dahil 128 olay, 64 modül, create dahil 16 thread olayı; thread sayısı kümülatiftir |
| Dosya bütçesi | Modül başına 64 MiB, LOAD_DLL dosyaları toplam 256 MiB; root exe preflight ve önceden açılan pinler ayrı bütçedir |
| Zaman | İlerleme/event bekleme için 5 saniye; cleanup ayrıca önceki observer'ın 5 saniye/64 olay sınırını kullanır |
| Pin | En fazla 64 geçerli, caller tarafından canlı tutulan LoaderFile; CLI üç sabit sistem dosyasıyla sınırlı |
| Modül | Null hFile, okunamayan kimlik, kota, null base veya kimlik/hash uyuşmazlığı: terminal ret |
| Olay | İlk exception'da dur; erken process exit, unload, thread exit, debug string, RIP veya diğer beklenmeyen olay: terminal ret |
| Girdi/ömrü | Yanlış executable handle, tekrar çağrı, geçersiz bütçe/pin: ilerletme yok; owner cleanup uygular |
| Cleanup | Çıkış doğrulanmazsa reason=`loader_exit_unconfirmed`; başarılı gözlem raporlanmaz |

Dosya/hash ve OS çağrıları senkrondur; beş saniye katı I/O preemption veya toplam komut wall-time garantisi değildir. Kötü depolama/sistem çağrısında takılmaya karşı ayrı dış supervisor henüz yoktur. Bilinmeyen modül görülmesi sonraki fazı kapatır; daha önce hiçbir OS/shim kodu çalışmadığının kanıtı değildir. Bu geliştirme gözlemcisi indirilen native kod için sandbox, anti-cheat veya Windows loader güvenlik sınırı değildir.

Yeni bir sistem/yerel DLL'nin kabulü, kontrol edilen dosya kimliği ve init/compatibility davranışının ayrı incelemesini gerektirir. Mevcut sonuç dosyasını okuyup otomatik allowlist üreten seçenek yoktur. Apphelp/compatibility yolu, packed entry ve TLS farkları sonraki N2 araştırmasının girdileridir. Unpack/symbol/ABI ve gerçek SAEX bootstrap yüklemesi kanıtlanınca N3 açılır; bu araç bu kapıyı geçirmez.

## Çıktı ve komut

```powershell
./out/windows-x86/Debug/saex_engine_loader_probe.exe --observe-loader "C:\Program Files (x86)\Rockstar Games\GTA San Andreas\gta_sa.exe"
```

Bu açık komut child oluşturur ve Windows loader kodunu ilerletebilir; yalnız dosya okumak isteyen kullanıcı `engine startup` kullanır. Standart build GTA komutunu çalıştırmaz; yalnız SAEX fixture'ını çalıştırır. x64 probe hedefi bilinçli yoktur; bu kesitte cross-bitness loader davranışı desteklenmez.

SchemaVersion=1, scope=`bounded-loader-mapping-observation`, policy=`windows-loader-three-file-observation-v1`; engine hash, child PID/exit, loaderAdvanced, exception code/address, event/thread/byte sayacı, sıralı modüller ve reason taşır. Modüllerde `identityRead=false` iken sıfır alanlar gerçek dosya hash/kimliği sayılmaz. ASLR base/PID/run sırası evidence'dir; kalıcı artifact kimliği değildir.

**CanAttach=false ve initializationVerified=false** sabittir. Exit 3 yalnız tutulmuş breakpoint adayı + doğrulanmış child exit, exit 1 ret/eksik gözlem, exit 2 kullanım hatasıdır. Exit 0 yoktur. Başarılı dosya gözlemi veya statik startup envanteri otomatik loader yetkisine dönüştürülmez; eski CLI/ABI'ler korunur. `saex_engine_process_probe --observe-suspended` hâlâ ilk create olayını öldürmeden ilerletmez. Yeni yol yalnız bu ayrı x86 hedefe linklenir; bootstrap DLL'ye linklenmez.

## Kabul ve test kanıtı

[Native test](../../tests/engine/loader_observation_tests.cpp), [EXE/TLS fixture](../../tests/engine/loader_fixture.cpp), [DLL fixture](../../tests/engine/loader_fixture_dll.cpp) ve [CLI negatifleri](../../tests/engine/test_loader_probe.py) AC-90/R-01a/b'nin sınırlı mapping alt kapsamıdır. Fixtures x86 statik CRT kullanır; üretim bootstrap/core CRT ayarları değişmez. DllMain içindeki canary dosya yazımı yalnız test marker'ıdır; ürün loader tasarımı değildir.

1. Pozitif kontrolde fixture normal başlatılır: DLL DllMain, EXE TLS callback ve EXE main üç ayrı dosya üretir; exit 73 doğrulanır, yalnız testin marker'ları silinir.
2. Aynı fixture dört pinle gözlemlenir. ntdll/kernel32/kernelbase/fixture DLL sırası, ilk breakpoint adayı ve kill-before-terminal-continue doğrulanır. Üç canary de oluşmaz. 12 warm çevrim boyunca process handle sayısı aynı kalır; explicit/tekrar stop ve tekrar run reddi kontrol edilir.
3. Boş liste ilk DLL'yi, eksik fixture pini yerel DLL'yi durdurur. Aynı ad ve SHA-256, başka file ID ile kabul edilmez. Yanlış exe, missing/null pin, pin tutulurken write erişimi, metadata hata sonucunda output korunması test edilir.
4. Event/modül/byte limiti ve geçersiz event/module/thread/time/byte üst sınırları; başka thread'den çağrı ve owner cleanup sınanır. Altı Python testi bilinmeyen hash, yanlış mimari, truncated/header, missing file ve açık opt-in gereğini child oluşmadan doğrular.

İlk x86 Debug native ve altı Python denemesi geçti. İlk fixture trace 7 olay, 4 modül, `0x80000003` breakpoint verdi; 12 warm çevrimde handle 112→112 ölçüldü. Tam build/regresyon sonuçları son bölümde kayıtlıdır. Timeout/null event hFile/API failure injection/forced observer kill ve hostile filesystem yarışları bu yeni testte enjekte edilmedi; koddaki ret/cleanup yolu bütün OS hata çeşitleri için kanıt değildir. Fixture sonucu bütün GTA initialization yollarına taşınmaz.

## Gerçek GTA gözlemi ve sonraki kapı

X86 Debug ve Release açık CLI deneyleri exact `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac` executable ile yapıldı. Windows 10.0.26200, mevcut proje working directory ve inherited environment kullanıldı; oyun klasörünü CWD yapan launcher davranışı sınanmadı. Gözlenen sıra iki build'de aşağıdaki gibidir; bu **tam dependency closure veya initializer sırası değildir**.

| Olay | Dosya | Sonuç |
|---|---|---|
| 2 | ntdll.dll | Tutulan x86 sistem dosyasının ID/hash'i eşleşti |
| 3 | kernel32.dll | ID/hash eşleşti |
| 4 | kernelbase.dll | ID/hash eşleşti |
| 5 | apphelp.dll | ID/hash kaydedildi; pin listesinde bulunmadığı için ilerleme reddedildi |

Toplam 6.177.248 byte, 4 DLL mapping kaydı. Exit **1**, reason=`loader_module_not_pinned`, breakpointCandidate=false, childExitConfirmed=true. Bu beklenen politikanın gerçekten reddettiğinin kanıtıdır; başarılı GTA initialization diye raporlanmaz. Statik root import listesinde olmayan apphelp mapping'i, dosya aday grafiğinin Windows runtime seçiminin yerine geçmediğini somutlaştırdı. Apphelp dosya adı tek başına shim'in hangi API'yi değiştirdiğini veya aktif bir compatibility fix bulunduğunu kanıtlamaz.

Debug probe SHA-256: `72b1481a45310d53fdff613350bd3fb108ba6224a7591e31b52798ad21c6c3b5`. Apphelp event dosyası SHA-256: `85d8b3d7a05378b778094dbb226f7bf1d929fe4a0b370b6e61b3a366eeda0aef`. Loglar `out/verification/engine/loader-observation-Debug.json` ve `loader-evidence-Debug.json` altındadır; tam dört DLL kimliği ve executable + 12 yerel DLL/ASI'nin önce/sonra hash kontrolünü içerir. **13 native girdi değişmedi.** Bu deney diğer oyun asset/save/config dosyalarının tam filesystem audit'i değildir; uygulama oyun dosyalarına yazmaz.

Sıradaki N2 çalışma: bu compatibility mapping'inin kaynağını ve erken yürütme davranışını incele; kontrollü initialization deneyinin somut ortamını seç; ardından gerçek bootstrap ve unpack/ABI kanıtını oluştur. Apphelp'i otomatik kabul etmek, kullanıcı modlarını silmek veya profiler kaydını verified profile'a çevirmek yoktur. N3, R-01 ve D1 açık kalır.

## Son build ve artifact kaydı

| Yapılandırma | Standart build/test sonucu | Sınır |
|---|---|---|
| Windows x86 Debug | 6 native suite, 57 managed ve 38 Python testi geçti | Loader fixture ve gerçek apphelp ret deneyi; tam initialization yok |
| Windows x86 Release | Aynı suite/test sayıları geçti | Optimize edilmiş loader fixture ve gerçek apphelp ret deneyi |
| Windows x64 Debug | 4 native suite, 57 managed ve 22 Python testi geçti | Eski ilk-create observer regresyonu; yeni x86 loader target'ı yok |
| Windows x64 Release / Linux | Bu kesitte çalıştırılmadı | Önceki koşu sonucu yeni kanıt diye taşınmaz |

Standart build'in x86 bootstrap artifact denetimi ve 100 ölçülen oyun dışı DLL lifecycle çevrimi de geçti; C ABI ve SDK bağımsız DLL scope'u korunur. 38 Python toplamı, önceki 32 kontrole eklenen 6 loader CLI negatifidir. Native suite sayısı test senaryosu veya AC sayısı değildir. Tam loglar `out/verification/engine/build-loader-x86-Debug.log`, `build-loader-x86-Release.log`, `build-loader-x64-Debug.log` altındadır.

Release probe SHA-256: `84fe47f422d338a483c8fd6cb46478e84284a7239cf4462d47c86b5db92e7a08`. `loader-observation-Release.json` ve `loader-evidence-Release.json`, Debug ile aynı dört modül ID/hash sırasını, exit 1 ret/owned exit ve 13 native dosyanın hash korunmasını kaydeder. Windows SysWOW64 apphelp.dll dosyasının ayrıca alınan hash'i event hash'iyle eşleşti; Authenticode veya aktif shim listesi denetimi yapılmadı. Orijinal oyun kurulumu değiştirilmedi; SAEX modülü GTA'ya yüklenmedi.

Son belge kontrolü: **73 Markdown, 934 yerel bağlantı, 6 JSON örneği, sıfır hata**. N1 salt okunur verify yine 23 kaynak dosyası ve aynı dependencyLockDigest ile geçti; SDK yeniden derlenmedi. Bootstrap Debug `eb0cdf4c074bee945099103cf661ba3269a8e1b6f1a35f24d670d994ec41f353`, Release `cabf7e8225c4d3c60d7fbc655bee631042376bdcdd550a3140f624dfabc8e8ea` SHA-256 değerleri eski artifact'lerle aynı kaldı. Yeni kod bu DLL'nin dependency closure'ına girmedi. Test marker dosyaları kalmadı; kullanıcı çalışma ağacı korunarak commit/push/deployment yapılmadı.

## Kod 0.1.7 — Ayrı reviewed politika

Bu rapordaki üç pinli --observe-loader ve tarihsel apphelp ret sonuçları korunur. [Yeni --observe-reviewed-loader](d1-loader-policy.md), source-defined 22 modülün engine/ad/origin/boyut/hash kontrolünü child öncesi tamamlar; event file ID/hash kontrolü ve ilk exception'da stop aynı kalır. LoaderFile ek kalan byte bütçesini hash öncesi uygular. JSON'a additive policySourceDigest ve failedPolicyModule eklendi; eski modda digest boş, anlamlar aynı. Yeni original-context AcLayers reddi ve özel kopyadaki breakpoint adayı kendi raporunda tutulur. Bu rapordaki eski probe hash'leri yeni build hash'i değildir.

## Kod 0.1.8 devamı — Açık child context

[LaunchContext](d1-launch-context.md) yeni CLI yolunda child yaratılmadan hazırlanır; LoaderObservation'ın event/pin/limit/terminal stop algoritması değişmez. Yeni native corpus, kopyalanmış own EXE yanında bulunmayan pinli DLL'yi explicit PATH ve explicit cwd ile ayrı koşulda çözer; hiçbir koşulda DLL/TLS/main canary açılmaz. Eski loader yollarında launchContext null'dır.

0.1.8 explicit context gözleminde beklenmeyen OS debug olayının tanısı için loader çıktısına lastEventCode/lastEventThreadId eklendi. Son alınan olay metadata'sıdır; initialization kanıtı veya olayın devamına izin değildir. Child öncesi retlerde ikisi de sıfırdır. Unknown event terminal ret davranışı ve incelenmiş DLL policy değişmedi.

## Kod 0.1.9 devamı — Bounded unload kolu

[Yeni lifecycle sözleşmesi](d1-loader-lifecycle.md) bilinen aktif UNLOAD_DLL olayını işler. Olay adresi okunmadan ledger'da emekli edilir; retired ID breakpoint adayına katkı vermez. modules tarihçe olarak kalır, unload load/byte/event bütçesini iade etmez. Unknown/duplicate unload ve diğer desteklenmeyen olaylar terminaldir. Eski canary/cleanup sınırları korunur.

## Kod 0.1.11 — Eski gözlem ile yeni initialization ayrımı

`LoaderObservation::run()` ve önceki üç CLI first-exception-only sınırını korur. [Yeni run_to_entry](d1-entry-boundary.md) ortak pin/history/cleanup döngüsüne açık EntryStopSpec ile girer; sadece bu mod DLL/TLS başlangıcını ilerletir. Ortak LoaderTrace'e entry alanları eklenmiştir; eski JSON'da entryObservation=null olur. Trace değişimi nedeniyle local native caller yeniden derlenir; bootstrap C ABI etkilenmez. Known UNLOAD ve bütün lifetime kotaları her iki modda aynı çalışır.

## Kod 0.1.11 — Entry supplement bağı

run_to_entry birleşik base22 + ayrı imm32 supplement setini kullanır. run() ve legacy üç CLI aynı pin/faz sınırını korur. JSON executionPolicySourceDigest yeni source’u tanımlar. [Ayrıntı](d1-entry-boundary.md).

### Hosted corpus tamamlaması

İkinci hosted turda bütün native suite'ler geçti; ortak Python loader CLI corpus'undaki cwd string eşitliği kısa/uzun Windows adı nedeniyle hata verdi. Test artık pathlib.samefile ile aynı dizin kimliğini doğrular. Engine unknown-fingerprint reddi, environment redaction, childCreated=false, entry/loader policy ve lifecycle sınırları aynen kalır; önceki GTA artifact sonuçları yeni test kanıtı sayılmaz.
