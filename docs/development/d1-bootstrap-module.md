# D1-N2 SDK bağımsız başlangıç modülü

Tarih: 13 Eylül 2026. Mimari v0.10 / kod 0.1.3. Durum: **x86 DLL ve oyun dışı lifecycle/ret alt kümesi uygulandı**. [Durum](status.md) · [N2 önceki dosya gözlemi](d1-engine-preflight.md) · [Uygulama sırası](d1-engine-integration.md) · [Native sözleşme](../architecture/native-sdk-integration.md) · [ADR-38](../decisions/architecture-decisions.md)

**Güncelleme 0.1.17:** Gerçek GTA'da üç C ABI export'u sekiz çağrıyla, 12/12 koşuda doğrulandı. [Güncel kanıt](d1-bootstrap-lifecycle.md). Aşağıdaki 0.1.3 ilk teslim açıklamaları tarihseldir; C ABI 1 ve DLL davranışı değişmedi. N2 engine fazı/symbol ABI ve N3 açıktır.

## Amaç ve kapsam

`saex_bootstrap.dll`, Plugin-SDK linklemeyen ilk yüklenebilir Windows x86 modülüdür. DLL'nin yüklenmesi GTA dosyası okuma veya hook kaydı başlatmaz. Açık `Initialize` çağrısı, DLL'nin **bulunduğu process'in ana executable'ını** OS'den bulur ve salt okunur denetler. Çağıran farklı dosya yolu, PID, image base, hook adresi veya doğrulama bayrağı veremez.

Bu teslimatta kendi x86 test executable'ımız modülü normal Windows loader ile yükler. Bu process GTA olmadığı için Initialize güvenli ret üretir. Gerçek GTA'ya DLL yükleme/injection, loader kurulumu, oyun başlatma ve oyun dosyası değişikliği yapılmadı. Doğru executable/image yolu kodlanmış olsa da gerçek GTA process'inde çalıştırılmadı; ABI/unpack/faz ve N3 hook kanıtı hâlâ açıktır.

## Kaynak ve veri akışı

| Kaynak | Sorumluluk |
|---|---|
| [C ABI](../../include/saex/engine/bootstrap_api.h) | Üç sürümlü export, fixed-width 192 byte status, açık sonuç/state/reason değerleri |
| [Session](../../src/engine/bootstrap/bootstrap_session.cpp) | Tek gözlem denemesi, terminal ret/stop, beklemeden BUSY, can_attach=0 |
| [Modül](../../src/engine/bootstrap/bootstrap_module.cpp) | Boş DllMain, constinit state, OS kaynaklı process kimliği, exception → internal error |
| [Ortak dosya incelemesi](../../src/engine/bootstrap/windows_file_observation.cpp) | Aynı read/share-read handle boyunca bounded okuma, PE parse, SHA-256 ve exact profil; exception dahil RAII close |
| [Artifact denetimi](../../tools/check_bootstrap.py) | PE32/x86 DLL, import/export, TLS/delay/CLR, linker map/CRT başlangıç alanları |
| [Session testleri](../../tests/engine/bootstrap_session_tests.cpp) | ABI/buffer, idempotence, terminal state ve concurrent BUSY |
| [Gerçek DLL testi](../../tests/engine/bootstrap_module_tests.cpp) | OS load/export çağrıları, yanlış host, stop/unload, 100 ölçülen tekrar |
| [Audit negatif testleri](../../tests/engine/test_bootstrap_audit.py) | Artifact/map bozulmaları ve bağımlılık/export/TLS/initializer drift reddi |

Akış: **Windows loader → boş SAEX DllMain → açık Initialize → OS host yolu → ortak file observation → actual module base → bounded image reader → rejected veya observed-unverified**. `Query` gözlem başlatmaz; `Stop` yeni başlangıçları kalıcı keser. SDK, binding veya gameplay aktivasyonu hiçbir dalda yoktur.

## C ABI 1 sözleşmesi

Export adları `.def` ile `SaexBootstrapInitialize`, `SaexBootstrapQuery`, `SaexBootstrapStop` olarak sabitlenir. Windows x86 çağrı kuralı `__cdecl`. Üçü de `uint32_t (abi_major, SaexBootstrapStatus*, out_bytes)` biçimindedir. Aynı process içindeki güvenilir çağıran, geçerli/yazılabilir ve tam **192 byte** bir buffer sağlar; API ham pointer geçerliliğini kötü niyetli process'e karşı güvenlik sınırı olarak sunmaz. Worker IPC veya C# SDK yüzeyi değildir.

| Alan / offset | Biçim ve anlam |
|---|---|
| 0–31 | Sekiz uint32: abi_major, struct_bytes, state, reason, observation_attempts, can_attach, bindings_loaded, reserved |
| 32–127 | observed_profile_id[96]; NUL sonlandırılmış; başarılı image gözlemine kadar boş |
| 128–191 | profile_source_digest[64]; tam 64 ASCII hex byte, NUL içermez |

Packing/alignment 4; native testler offset ve boyutları denetler. C++ nesnesi, exception, STL container, allocator sahipliği veya oyun pointer'ı taşınmaz. ABI major 1 ve tam buffer boyutu zorunlu; uyumsuz giriş çıktı/state/IO'yu değiştirmez. Return code **0** yalnız çağrının işlenmesi demektir: status içindeki ret/runtime-unverified sonucu ayrıca okunur; oyun desteği demek değildir.

| Return code | Anlam |
|---|---|
| 0 OK | Tutarlı status kopyalandı; state ayrıca değerlendirilir |
| 1 INVALID_ARGUMENT | Null output veya yanlış boyut; output değişmez |
| 2 ABI_MISMATCH | Bilinmeyen major; output değişmez |
| 3 BUSY | Başka export işlemde; bekleme/kuyruk/IO yok, output değişmez |

State sayıları: 0 discovered, 1 rejected, 2 observed-unverified, 3 stopped. Reason sayıları header'da: none=0, host_path=1, file_rejected=2, image_base=3, image_rejected=4, runtime_unverified=5, internal_error=6. Ayrıntılı dosya ret nedeni mevcut offline CLI ile incelenir; modül dosya hatalarını file_rejected altında toplar.

## Session ve kapanış kuralları

Initialize yalnız discovered durumda bir kez IO yapar. Tekrar Initialize aynı terminal sonucu döndürür; retry storm veya sayaç taşması oluşturmaz. Başarısız gözlem rejected olur. Dosya/image eşleşmesi bile yalnız observed-unverified olur; `can_attach=0` ve `bindings_loaded=0` sabittir. Unknown enum reason internal_error'a çevrilir; gizli başarı yoktur.

Stop, discovered/rejected/observed-unverified durumlarından stopped'a geçer; tekrar Stop aynı sonucu verir. Durdurulmuş session Initialize ile açılmaz; yeni modül ömrü gerekir. Son reason/observation sayısı teşhis için korunur. Session tek bir atomic_flag ile korunur; çakışan Initialize/Query/Stop BUSY verir ve buffer'a yazmaz. Başlatma sürerken Stop tamamlanmış sayılmaz.

Modül thread, callback, hook veya uzun ömürlü dosya handle'ı oluşturmaz. Başlatmanın dosya/view kaynakları export dönmeden bırakılır. DLL sahibi yeni çağrıları keser, kendi devam eden export çağrılarını tamamlatır, Stop sonucunu alır ve ancak sonra FreeLibrary çağırır. Stop sonrası başka thread export'a girerken DLL'yi boşaltmak bu ABI'nin dışında hatalı kullanımdır; modül kendi kodunu pin/unload yönetimiyle koruyan production loader değildir.

## Loader ve dosya/image sınırı

SAEX DllMain yalnız TRUE döndürür; global session constinit ile hazırlanır. CRT'nin kendi loader başlangıcı ve Debug RTC yardımcıları vardır; “DLL içinde hiçbir initializer yok” iddiası yapılmaz. Initialize **LoadLibrary döndükten sonra, loader lock dışında** çağrılır. Bu synchronous dosya başlangıcı frame callback'i değildir; IO süresine sert deadline garantisi yoktur. Gelecekteki launcher/başlatma fazı bu işi oyun frame'inden ayırmalıdır. [Microsoft loader kuralları](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).

Test loader'ı tam artifact yolu ve `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32` kullanır; PATH/current-directory aramasına dayanmaz. Bunun artifact imzası, dependency güven zinciri veya production launcher olduğu iddia edilmez. [LoadLibraryExW](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw).

Host yolu `GetModuleFileNameW(nullptr)` ile en fazla 32.768 wchar buffer'a alınır; truncation ret. File observation eski CLI ile ortaklaştırıldı; 256 MiB sınırı, read/share-read handle, PE/hash/layout/anchor kontrolleri ve hata adları korunur. Construction sırasında allocation exception olsa da açılmış handle kapatılır. [GetModuleFileNameW](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamew).

Exact dosya profilinden sonra actual `GetModuleHandleW(nullptr)` tabanı preferred image base ile aynı olmalıdır. Reader, başlıkları ve dört anchor'ı actual ana image'dan sınar. Relocation veya unpack farkını kabul eden istisna eklenmedi; farklı byte'lar reddedilir. Bu yolu gerçek GTA'da denemeden runtime/profile desteği açılmaz. Caller-provided görüntüye ya da farklı path'teki sağlam GTA dosyasına güvenilmez.

## Artifact ve initializer denetimi

Audit, üretilmiş DLL ve aynı build'in linker map'ini okur. Girişler 16 MiB DLL ve 2 MiB map ile bounded'dır. PE32/x86 DLL, tam üç export, sınırlı normal imports, sıfır TLS directory/delay imports/CLR şarttır. İzinli doğrudan dependencies Windows kernel, bcrypt ve listelenmiş MSVC/CRT runtime'larıdır. Yeni dependency sessiz kabul edilmez. Plugin-SDK/Injector/SafetyHook map nesneleri reddedilir.

Sekiz `.CRT$XC/XI/XP/XT` başlangıç/bitiş işareti DLL'de sıfır pointer olmalıdır. Release linker'ın XTZ alanındaki ilave dört byte null padding'i ayrıca sıfır kontrolüyle kabul edilir. `.CRT$XCU`, dinamik initializer/destructor symbol'u veya non-null sentinel reddedilir. CRT ve Debug `.rtc` yardımcıları vendor/runtime kapsamıdır; tüm transitif Windows/CRT kodunu denetlediğimiz veya map dosyasının imzalı olduğu iddiası yoktur. Kaynak incelemesi, artifact audit ve gerçek DLL testi birbirini tamamlar.

## Doğrulama, hata bulgusu ve kanıt sınırı

Yeni portable session suite: ABI/null/short/oversize, output değişmezliği, Query'nin yan etkisizliği, duplicate init, ret nedeni, erken/tekrar Stop ve thread yarışında BUSY. Gerçek x86 DLL suite: undecorated exports, unknown host ret, init idempotence, terminal stop, 100 yükle/başlat/durdur/boşalt çevrimi. Test ana executable'ı GTA değildir; başarılı GTA başlangıcının veya hook unload'un yerine yazılmaz.

İlk testte cold process handle sayısının eski haline dönmesi beklentisi başarısız oldu. Ayrı **yalnız yükle/boşalt** kontrolü ilk artışın bir bölümünü Initialize çağrısı olmadan yeniden üretti; ilk dosya incelemesi ek artış gösterdi. Sonraki 100 tam çevrimde handle sayısı sabit kaldı. Test şimdi cold, load-only, warm ve final değerlerini ayrı raporlar; her ölçülen warm çevrimde tam eşitlik ister. Sabit toleransla artış gizlenmez. İlk kullanımda tutulan process-wide handle'ların tek tek sahipliği doğrulanmadı; bütün process kaynaklarının sıfır sızıntısı iddia edilmez. SAEX dosya kaynakları RAII ile scope sonunda bırakılır.

Artifact audit için 10 Python test tanımı: gerçek artifact, x64, TLS, dependency, export, initializer bucket/symbol, vendor object, eksik map, non-null CRT sentinel ve truncated artifact. Mevcut PE/profile/managed testleri ortak dosya kodunun refactor'u için regresyon kapısıdır.

Standart `tools/build.ps1 -Architecture x86 -Configuration Debug|Release` DLL'yi, dört CTest suite'ini, mevcut managed/tooling testlerini ve yeni artifact audit testlerini çalıştırır. x64 build DLL üretmez; portable session üçüncü native suite olarak sınanır. `out/verification/engine/bootstrap-audit-<Configuration>.json` DLL/map digest, imports/exports ve audit sonucunu taşır. Kaynak/dependency kilidi değişmedi; N1 SDK modülü burada yüklenmez.

## Son yerel doğrulama — 13 Eylül 2026

Standart build akışları **x64 Debug, x86 Debug ve x86 Release** için başarılıdır. X64'te 3 native suite, 26 managed ve 17 Python testi; x86'da 4 native suite, 26 managed ve 27 Python testi geçti. Tekrar koşular yeni test tanımı sayılmaz. Windows 10.0.26200, MSVC 19.44.35228.0 / Windows SDK 10.0.26100.0, CMake 4.3.3 ve .NET SDK 10.0.300 kullanıldı. Bu kesitin Linux ve x64 Release koşusu yapılmadı.

| Artifact | SHA-256 |
|---|---|
| x86 Debug saex_bootstrap.dll | `eb0cdf4c074bee945099103cf661ba3269a8e1b6f1a35f24d670d994ec41f353` |
| x86 Debug linker map | `8abdf5278e65922329b1bbbae258d0c93bab301dbd9ac66547d3791da017d8df` |
| x86 Release saex_bootstrap.dll | `cabf7e8225c4d3c60d7fbc655bee631042376bdcdd550a3140f624dfabc8e8ea` |
| x86 Release linker map | `65c324e7386bcbb82975d929477d54afff62bd976da0476013d2062180f94c80` |

Bu kimlikler bu build'e aittir; yeniden derlemenin aynı hash'i üretmesi garanti edilmez. `out/verification/engine/bootstrap-module-Debug.json` ve `bootstrap-module-Release.json` tekrar deneyini module/harness hash ve cold/warm handle değerleriyle saklar. Kaydedilen son doğrudan koşuda her iki yapılandırma için cold=83, load-only=95, warm=99, final=99 gözlendi. İlk artışın kaynağı hakkında yukarıdaki sınırlama geçerlidir; 100 ölçülen çevrimin her birinde warm değer değişmedi.

Ortak file observation refactor'u için x86/x64 Debug CLI, gerçek dosyayı çalıştırmadan image görünümünde tekrar sınadı: aynı SHA-256, aynı profil kaynak digest'i, aynı dört anchor ve canAttach=false. `bootstrap-file-regression-x86.json` ve `bootstrap-file-regression-x64.json` ayrı kayıtlardır. Orijinal dosyanın öncesi/sonrası hash'i aynı. N1 source/patch/recipe verify yeniden geçti; SDK lock/artifact değiştirilmedi. Son belge kontrolü **70 Markdown, 834 yerel bağlantı, 6 JSON örneği, sıfır hata** raporladı.

## Genişleme ve kalan kapılar

Yeni ABI alanı, export, thread/hook, global initializer, dependency veya retry davranışı ADR-38, bu belge, source map ve AC-90 güncellemesi ister. Bu başlangıç state machine'i üretim adapter'ın bütün lifecycle durumlarının implementasyonu değildir; binding/active yolu yoktur.

R-01b'nin SDK bağımsız DLL, açık başlatma ve oyun dışı ret/stop alt kanıtı oluştu. R-01a gerçek symbol ABI/unpack/faz, gerçek GTA'ya yükleme sırası ve R-01c hook callback/drain henüz kanıtlanmadı. N2 bütünü açık, N3 başlatılmadı. Bir sonraki adım, exact aday executable için gerçek process/symbol faz deneyini ve loader girişini hazırlamaktır; gözlem profili otomatik açılmayacak.

## Kod 0.1.4 devamı

[Ayrı process observer](d1-suspended-process.md) eklendi; bootstrap DLL kaynakları/C ABI ve Debug/Release DLL artifact hash'leri değişmedi. Her iki yapılandırmada mevcut audit ve oyun dışı 100 warm lifecycle regresyonu geçti. Yeni gerçek GTA ilk-image gözlemi DLL'nin GTA'ya yüklendiği veya Initialize export'unun GTA'da çalıştığı anlamına gelmez. Bir sonraki faz unpack/ABI ve gerçek DLL load sırasıdır.

## GitHub kaynak yayını — taşınabilirlik düzeltmesi

GitHub Linux GCC derlemesi, uint32_t reason ile enum fallback arasındaki örtük koşullu dönüşümü -Werror altında reddetti. Fallback açık std::uint32_t dönüşümüdür; mevcut geçersiz reason → INTERNAL_ERROR regresyonu korunur. Export, 192 byte status, C ABI 1 ve terminal durumlar değişmez.

### Hosted corpus tamamlaması

Bootstrap session corpus artık geçersiz reason 0/999/UINT32_MAX için exact INTERNAL_ERROR değerini ve bilinen ret kodlarının korunmasını ayrıca sınar. Önceki test yalnız terminal ret durumunu ölçüyordu; bu ek assertion ABI/fallback değerini doğrudan kanıtlar. Production davranışı ve layout değişmez.

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

0.1.18 frame aday denetimi dış observer içinde kalır; DLL Initialize/Query/Stop, status boyutu ve observed_unverified sınırı değişmedi. Terminal bootstrap durumundan GTA gameplay devam ettirilmez. [Aday ve doğrulama raporu](d1-frame-target.md).

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

## Kod 0.1.32 — Cwd query bağlantısı

Yeni query observer önceki terminal komutlarını korur ve ayrı çağrı/dönüş kapısı ekler. Paylaşılan loader/fixture test girişleri bu kapsam için güncellendi; bu belgedeki eski sonuçlar kendi artifact kapsamındadır. Windows OS dosya farkı, GTA yürütmesini güvenli reddeder. Copy/unlock/SEH removal, doğal frame ve D1/D2 kapıları açık kalır. [Sözleşme, kaynak ve güncel kanıt](d1-cwd-query.md).

## 0.1.33 — Windows profil bağı

[26200.9445 incelemesi](d1-system-profile-9445.md), bu bileşenin bağlı olduğu OS pinleri/API reçeteleri ve parent SHA-256 zincirini günceller. Bileşenin GTA durak/izin sınırı aynı kalır; önceki runtime sonuçları eski profil bağlamındadır. Yeni dosyalarla fixture ve gerçek GTA kanıtının kapsamı profil raporunda ayrıca izlenir. Eski OS pinlerine otomatik fallback yoktur; kaynak ve generated policy birlikte yeniden derlenir.

## 0.1.34 — Doğal cwd copy bağı

[Ayrı copy kesiti](d1-cwd-copy.md), query'den sonra native kontrol/CALL/dönüş ve gerçek hedef içerik kanıtını ekler. Önceki komutların terminali korunur; helper/unlock/SEH sökümü yeni izin kapsamına girmez. Source/guard, caller/SEH/kilit/cookie denetimleri ve yeni test/GTA kanıtının kapsamı ilgili rapordadır. C ABI 1, OS modül pinleri, GNS/otorite ve production kapıları aynı kalır; C++ observer yeniden derlenir.

## 0.1.35 — Cwd helper dönüş bağı

[Ayrı return kesiti](d1-cwd-return.md) copy sonrasındaki iki POP, cookie checker eşitliği ve doğal LEAVE/RET'i açar. Önceki terminal izinleri korunur; wrapper/unlock/SEH işlemleri henüz açılmaz. Kaynak stack ömrü, CALL ile değişen saved slot, hedef/caller/kilit/SEH ve hata retleri sözleşmede açıklanır. C++ trace yeniden derlenir; C ABI 1, OS pinleri, GNS/otorite ve production kapıları aynı kalır. Yeni fixture/gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## 0.1.36 — Dosya yöneticisinin tamamlanması

Bu belgenin mevcut alt komut sınırı korunur. Yeni üst düzey `--observe-file-manager-ready` aynı önceki zinciri geçtikten sonra normal unlock(7), SEH epilogue, yol suffix ve CFileMgr dönüşünü birlikte doğrular. Eski checkpoint kanıtı final kilit/FS durumuyla karıştırılmaz; final sonuç ayrı ready kaydındadır. Renderer/doğal frame ve N2/N3/D1/D2 bu kesitte hazır sayılmaz. [Sözleşme, kullanıcı komutu ve doğrulama](d1-file-manager-ready.md).

## Kod 0.1.37 — Streaming tablo kesitiyle bağlantı

[CdStream tablo sözleşmesi](d1-cd-stream-tables.md) ortak observer/CLI ve fixture zincirine ayrı bir üst mod ekler. Bu belgenin eski komut ve checkpoint sınırı korunur; yalnız `--observe-cd-stream-tables` tam manager dönüşünden sonra iki tablo döngüsünü ve disk argüman hazırlığını açar. Sonuç yeni `cdStreamTablesObservation` alanında izlenir; eski kayıtlar final durum değil önceki checkpoint snapshot'ıdır. C++ trace tüketicileri yeniden derlenir; C ABI 1, GNS, OS pinleri ve production IPC sınırı değişmez. Yeni portable/native testler ile eski mod regresyonları standart build'e dahildir; gerçek GTA ve platform bazındaki final kanıt ana raporda tutulur.

## Kod 0.1.38 — Disk sonucu ve allocation önkoşulu

[Disk hazırlığı sözleşmesi](d1-cd-stream-disk.md) önceki native zincire ayrı `--observe-cd-stream-disk` modu ekler. BOOL başarısızsa dört output kullanılmadan ret; başarılı ve kabul edilen mantıksal geometride doğal bayrak/argüman hazırlığı, 0x406BF4 allocation CALL önünde doğrulanır. Eski modların terminal ve snapshot anlamı korunur. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS, network otoritesi ve sandbox kapsamı değişmez. Gerçek allocation, fiziksel hizalama, dosya okuma ve thread/renderer hazır kanıtı bu değişiklikten çıkarılamaz. Portable hata kararı ile native/gerçek GTA kanıtının ayrımı yeni raporun kabul tablosunda izlenir.

## Kod 0.1.39 — Hizalı tamponun doğal dönüşü

[Allocation sözleşmesi](d1-cd-stream-allocation.md) ayrı `--observe-cd-stream-allocation` API/CLI ile MallocAlign → CRT → HeapAlloc → back-pointer → 0x406BF9 doğal dönüşünü ekler. Heap modu/new-handler/SBH dalı yürütmeden önce denetlenir; NULL, taşma, metadata ve payload bütünlüğü guard'ları vardır. Eski alt modların terminalleri ve snapshot anlamı korunur; yeni mod 160, eskiler 128 olay üst sınırındadır. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS ve sandbox kapsamı değişmez. İlk gerçek GTA allocation geçti; güncel toplu kanıt yeni sözleşmede izlenir. Native free, I/O/thread, renderer ve D1/D2 hazır kabul edilmez.

## Kod 0.1.40 — Kanal belleği kesiti

[Yeni sözleşme](d1-cd-stream-channels.md) `run_cd_stream_channels` / `--observe-cd-stream-channels` ile SetLastError ve LocalAlloc doğal yolunu, 5 × 48 sıfır byte ve global pointer kaydını ekler. Terminal 0x406C34, arşiv CALL önüdür. Önceki allocation/parent kayıtları kendi duraklarının snapshot anlamını korur; canlı tabloda yalnız kanal sayısı/etkin sayı DWORD çifti değişebilir. C++ trace tüketicileri yeniden derlenir; C ABI 1 ve mevcut OS pinleri aynıdır. Allocation ve yeni mod 160, daha eski modlar 128 olay sınırındadır. Native free, dosya açma/okuma, thread, renderer ve D1/D2 kapıları açıktır. Güncel test ve GTA kanıtı yeni sözleşmede tutulur.
