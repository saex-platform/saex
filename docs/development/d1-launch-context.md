# D1-N2 — Açık çalışma klasörü ve ortam görüntüsü

Tarih: 13 Eylül 2026. Kod 0.1.8 / mimari v0.15. Kapsam: Windows geliştirme observer'ının başlatma girdileri. [Durum](status.md) · [N2 uygulama sırası](d1-engine-integration.md) · [Mapping policy](d1-loader-policy.md) · [Normatif sınır](../architecture/native-sdk-integration.md)

## Sorumluluk ve gerekçe

0.1.7'de aynı engine hash'i orijinal kurulumda ve üç dosyalı yerel kopyada farklı loader sonuçları verdi. Executable adı, yolu ve yan dosyaları yanında current directory ve environment da launch context'in parçasıdır. Yeni LaunchContext, seçilmiş klasörü ve ortamın o anda alınmış kopyasını açık CreateProcessW parametrelerine dönüştürür. Mevcut process/loader sahipliği, ilk exception'da kill-before-continue ve exact DLL pinleri korunur.

Bu kesit GTA initialization, injection, bootstrap DLL yükleme veya function ABI yetkisi eklemez. Aynı ortam digest'i bütün Windows/registry/AppCompat/yan dosyaların aynı olduğunu kanıtlamaz. Bir koşu içindeki environment görüntüsü sabittir; ham değerler saklanmadığından rapor tek başına başka bir gün aynı ortamı yeniden kuramaz.

## Veri akışı ve arayüzler

`--observe-context-loader <gta_sa.exe> <absolute-working-directory>` → LaunchContext capture/validation → engine için tek absolute path çözümü ve preflight → mevcut 22 pinli PreparedLoaderPolicy → açık Unicode environment/cwd ile owned SuspendedImage → bounded LoaderObservation → child exit → JSON metadata.

- [LaunchContext](../../include/saex/engine/launch_context.hpp) iki yerel C++ kurucusu sunar: current process ortamını bir defa yakalama ve test/yerel caller için açık string-view listesi. Sunucu/resource tarafından gönderilen environment veya DLL recipe okuma API'si değildir.
- Windows GetEnvironmentStringsW çıktısı bağımsız blok olarak alınır. Parent ortamını değiştirme, temizleme veya compatibility ayarını kaldırma yoktur. `__COMPAT_LAYER`, mevcutsa diğer değişkenler gibi korunur.
- Windows'a UTF-16, iki NUL ile sonlanan, anahtarları CompareStringOrdinal/ignore-case ile sıralanan blok verilir; CREATE_UNICODE_ENVIRONMENT zorunludur. `name=value` değerindeki ek `=` karakterleri korunur. Belgelenmiş `=C:=C:\...` drive-directory girdileri taşınır.
- 1024 entry, entry başına 32767 ve bütün blokta 65536 UTF-16 code unit yerel üst sınırdır; bunlar Windows'un genel limitleri diye sunulmaz. Boş ortam açık iki NUL'dur, nullptr/inheritance değildir.
- İlk geliştirme sürümü yalnız `C:\...` veya `C:/...` biçimindeki absolute drive cwd'yi ve MAX_PATH'ten kısa girdi/çözülmüş yolu kabul eder. Relative/drive-relative/UNC/extended yol güvenli ret verir. Girdi klasör olmalıdır; mevcut junction çözülmüş hedefe dönüştürülür. Hedef directory handle'ı FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES ve delete-share olmadan tutulur ve child öncesi volume/file ID yeniden karşılaştırılır.
- Directory pin'i klasör içeriğini veya bütün ata yollarını dondurmaz; dosya sistemi snapshot'ı/sandbox değildir. Windows path tabanlı CreateProcess çağrısındaki son yarış aralığı için atomik directory-handle launch garantisi verilmez. DLL event file ID/hash kontrolü bağımsız kalır.
- Snapshot önce hazırlanır; throwing allocation'lar child yaratılmadan tamamlanır. Context nesnesi observer çağrısı bitene kadar yaşar. SuspendedImage ortamın kendi kopyasını CreateProcess'e geçirir.

## Çıktı, hata ve migration

Yeni additive `launchContext` alanı explicit modda `mode`, `prepared`, çözülmüş `directory`, `environmentSha256`, `environmentEntries`, `environmentCodeUnits` taşır. SHA-256 sıralanmış bloğun bütün UTF-16LE byte'ları ve çift NUL'u üzerindedir; key/value yazımı aynen korunduğu için harf yazımı değişikliği digest'i de değiştirebilir. Ortam anahtar/değerleri loglanmaz; digest güvenlik yetkisi değildir. Cwd yerel deney yolu olarak görünür.

Geçersiz context, unknown executable ve eksik/drift policy dosyası child öncesi ret verir. Hatalar sırasıyla `launch_directory_input/unavailable/resolution`, `launch_environment_entry/duplicate/limit/capture_failed/hash_failed`, observer tarafında `observer_invalid_context/context_directory_changed` olarak ayrılır. Hatalı nesne kısmi environment block yayınlamaz. Exit 1 ret, 2 kullanım, 3 breakpoint adayı ve doğrulanmış exit'tir.

Eski `--observe-loader` ve `--observe-reviewed-loader` çağrıları inherited cwd/environment davranışını korur ve `launchContext: null` raporlar. Loader CLI executable yolunu bütün modlarda yalnız bir kez absolute çözer; dosya kapısı, yerel policy kökü ve child aynı yolu kullanır. Bootstrap C ABI 1, observed profile ve mapping policy digest'i değişmez. Native SuspendedImage C++ kurucusuna opsiyonel context parametresi eklenmiştir; kaynak çağrıları uyumludur, static library tüketicileri yeniden derlenir. C ABI veya dağıtılan binary SDK değişimi yoktur.

## Genişleme ve kabul senaryoları

AC-90/R-01b'nin başlatma bağlamı alt kapsamı; AC-91 veya D1 tamamlandı sayılmaz. UNC/uzun yol, izinli environment üretimi, registry/AppCompat profili, otomatik staging ve DLL initialization sonraki ayrı kararlardır.

1. Parent environment snapshot alındıktan sonra değişir: kendi fixture child'ı snapshot değerini almalı, Unicode ve `=` bozulmamalı, kendi relative canary'sini seçilen cwd'ye yazmalıdır.
2. Unknown executable, olmayan/file/relative cwd, aynı anahtarın farklı harf yazımı, embedded NUL ve entry/byte sınırı: ret ve childCreated=false.
3. EXE'nin yanında bulunmayan fixture DLL: bir koşuda yalnız explicit PATH, bir koşuda yalnız explicit cwd üzerinden pinli dosyaya çözülmeli; observer ilk breakpoint adayında tutmalı, DLL/TLS/main canary'leri oluşmamalıdır.
4. Context ile 12 owned child çevriminde exit doğrulanmalı; warm handle sayısı büyümemeli. Yanlış context hiçbir debug child yaratmamalıdır.
5. CLI sentetik gizli environment değerini/key'ini stdout/stderr'e taşımamalı; unknown engine kapısı ve eski iki mod korunmalıdır.

## Derleme ve doğrulama kaydı

Standart build yeni native.launch_context suite'ini Windows x86/x64'te çalıştırır. X86 loader suite'i iki explicit dependency resolution deneyini, Python loader CLI corpus'u iki yeni ret/redaction testini kapsar. Fixture'lar yalnız SAEX'e ait test executable/DLL'leridir; standart build GTA'yı başlatmaz. Test sonuçları ve ayrı gerçek GTA gözlemi bu bölümün sonuna kaydedilir; henüz çalıştırılmayan koşu pass sayılmaz.

## Birincil API kaynakları

Explicit Unicode environment, çift NUL, drive-directory girdileri ve current directory davranışı [Microsoft CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw) ve [Environment Variables](https://learn.microsoft.com/en-us/windows/win32/procthread/environment-variables) üzerinden kontrol edildi. Bu kaynaklar SAEX test kanıtının yerine geçmez.

## İlk doğrulamada bulunan ve giderilen hatalar

İlk x86 Debug koşusu iki corpus beklentisinde durdu: CMake'in absolute `C:/...` çıktısı sadece backslash kabul eden cwd doğrulamasına takıldı; geçici konuma kopyalanan own EXE Windows'ta apphelp.dll mapping'i oluşturdu. Absolute drive slash biçimi de kabul edildi ve resolved directory yine Windows'tan alınır. Yalnız bu own fixture'ın pin setine exact açık system apphelp dosyası eklendi; gerçek engine'in 22 modüllü recipe'si değişmedi. Gözlenen mapping sayısının sabit olması yerine gerekli fixture DLL'nin aynı volume/file ID ile admitted olması sınanır.

İkinci koşu, FILE_READ_ATTRIBUTES erişiminin tek başına directory rename'i engellemediğini gösterdi. FILE_LIST_DIRECTORY eklenince mevcut directory'yi rename etme negatif testi ret verdi. Üçüncü koşuda cold handle ile warm handle eşitliği başarısız oldu. İlk owned debug child sonrası bir process handle'ı arttı (135→136); sonraki 12 warm çevrimde 136→136 sabit kaldı. Corpus cold/warm/final değerlerini açık raporlar; bu sonuç bütün process kaynaklarının sıfır sızıntı kanıtı değildir. Windows debugger'ın ilk kullanım kaynakları olası açıklamadır, handle nesnesinin türü ayrıca kanıtlanmadı.

Başarısız üç standart build log'u out/verification/engine/build-context-x86-Debug-{first,second,third}-failed.log dosyalarında korunur. Dördüncü standart x86 Debug build bütünüyle geçti: 7 native suite, 57 managed test, 49 Python testi. Bootstrap artifact audit ve 100 oyun dışı warm DLL çevrimi de geçti; DLL hash'i değişmedi. Diğer yapılandırmalar ve gerçek GTA context gözlemi aşağıda kendi sonuçlarıyla kaydedilir.

İlk gerçek GTA context gözleminde Debug özel kopya 24. olay/19 DLL sonrası loader_unexpected_event verdi; o artifact event türünü kaydetmediği için nedeni tahmin edilmez. İki Release özel kopya koşusu 26 olay/21 DLL ile breakpoint adayına ulaştı, orijinal kurulum AcLayers'da ret verdi. Dört child çıkışı ve 13 orijinal/3 kopya hash'i korundu. Bu bulguyla lastEventCode/lastEventThreadId additive tanı alanları eklendi. İlk sonuç launch-context-gta-evidence.json'da korunur; yeni artifact ayrı kaydedilir.

## Sonuç — 0.1.8 doğrulama ve gerçek GTA sınırı

Standart akışlar geçti: Windows x86 Debug ve Release için her koşuda 7 native suite, 57 managed test ve 49 Python testi; Windows x64 Debug için 5 native suite, 57 managed ve 30 Python testi. Native suite sayısı senaryo sayısı değildir. X64 Release/Linux bu kesitte çalıştırılmadı; N1 SDK recipe/source değişmedi ve SDK build tekrarlanmadı. Sonradan eklenen terminal event alanları yalnız x86 loader hedefini etkiler; aynı iki x86 akışı bu değişiklikten sonra yeniden geçti.

Own context fixture cold/warm/final handle sayıları x86 Debug/Release'te 135/136/136, x64 Debug'ta 105/106/106'dır. 12 warm çevrimde artış yoktur. Own imported DLL explicit PATH ve cwd ile ayrı ayrı çözüldü; bu koşullar DLL/TLS/main canary'sini yürütmedi. CLI ortam değeri/key sızıntısı ve input gate testleri geçti. Hatalı testin oluşturduğu tek boş renamed temp klasörü yalnız boşluğu doğrulandıktan sonra kaldırıldı; başka dosya temizliği yapılmadı.

Gerçek gözlem Windows 10.0.26200 üzerinde yapıldı. Yedi çağrının tamamında 60 entry/4132 UTF-16 code unit environment görüntüsü ve `3725a091ed77f875b48ab336a8162328429aeebc82f2c4779f897ed54f62407e` digest'i aynıydı. Ham key/value kaydı alınmadı. İlk dört çağrı launch-context-gta-evidence.json, terminal olay teşhisi eklenen son üç çağrı launch-context-gta-final.json dosyasında saklıdır.

| Son artifact / bağlam | Gözlem | Sonuç |
|---|---|---|
| x86 Debug / özel kopya, workspace cwd | 26 olay, 21 DLL; lastEventCode=1, first-chance breakpoint adayı | Exit 3; child çıkışı doğrulandı |
| x86 Release / özel kopya, workspace cwd | 24 olay, 19 DLL; lastEventCode=7, UNLOAD_DLL_DEBUG_EVENT | Desteklenmeyen lifecycle olayı için loader_unexpected_event/exit 1; child çıkışı doğrulandı |
| x86 Release / orijinal kurulum, workspace cwd | 6 olay, 5 DLL; lastEventCode=6, AcLayers.dll mapping | loader_module_not_pinned/exit 1; child çıkışı doğrulandı |

İlk artifact'ta Release özel kopya hem workspace hem private-directory cwd ile 21 DLL/breakpoint adayına ulaştı; Debug beklenmeyen olayla durdu. Son artifact'ta tersi görüldü. Bu nedenle sonuç Debug/Release farkına bağlanmaz; aynı hash/context ile mapping lifecycle değişebilir. İlk başarısız artifact olay türünü kaydetmediğinden onun da UNLOAD olduğu ileri sürülmez. DLL unload henüz desteklenmiyor; pin kaldırma/yeniden eşleme gibi lifecycle davranışı ayrıca test edilmeden unknown event devam ettirilmez. Bu, sıradaki N2 çalışmasının somut girdisidir.

Final probe SHA-256:

- Debug: `295017c7546b7c738d40da341a216994af5996376b5843d78cc69adf5fa1e0b3`
- Release: `ff5a5e697f0f01db2ff203cdbc5960b749d2e1ecc94365e10f372d8d7b75b73e`

Engine/policy source kimlikleri 0.1.7 ile aynıdır. Original 13 native girdi ve private 3 kopyanın hash'leri bütün çağrılardan sonra korundu; 7/7 owned child çıkışı doğrulandı. SAEX bootstrap DLL'lerinin Debug/Release hash'leri de önceki sürümle aynıdır. Otomatik staging, oyun dosyası yazma, registry/env değiştirme veya production deployment yapılmadı. N2 initialization/SAEX DLL yükleme/unpack/symbol/ABI ve N3–N7 açık; bu sonuç multiplayer hazırlığı değildir.

0.1.8 son statik belge kontrolü: 75 Markdown, 990 yerel bağlantı, 6 JSON örneği ve sıfır hata. Kaynak/belge eşlemesi başarılıdır; bu statik sonuç anlamsal kusursuzluk veya oyun içi doğrulama yerine geçmez.

## Kod 0.1.9 devamı — Önceki UNLOAD reddinin ele alınması

Bu rapordaki 0.1.8 UNLOAD retleri tarihsel olarak korunur. [0.1.9 lifecycle](d1-loader-lifecycle.md) yalnız bilinen aktif eşlemeyi emekli ederek devam eder; unknown/duplicate unload hâlâ ret alır. LaunchContext capture/cwd/environment davranışı değişmedi. Yeni mapping alanları snapshot'ı initialization veya aynı registry/OS durumu kanıtına dönüştürmez.

## Kod 0.1.11 — Context yeni initializer deneyinde de açık

[--observe-entry-boundary](d1-entry-boundary.md) aynı absolute cwd/retained directory/immutable environment sözleşmesini kullanır. Yeni flag DLL/TLS kodunu ilerletebilir; --observe-context-loader ilk exception sınırını korur. Context snapshot'ı DLL yan etkilerini izole etmez; aynı directory/env/hash başlı başına initializer/ABI kanıtı değildir. Yeni CLI'nin relative cwd ve unknown engine retleri ayrı test edilir.

## Kod 0.1.11 — Entry supplement bağı

Ek entry policy imm32 kaydı kullansa da cwd/environment aynı explicit snapshot sözleşmesidir. Context ve tüm birleşik pinler child öncesi denetlenir; ham environment veya registry snapshot’ı eklenmedi. [Ayrıntı](d1-entry-boundary.md).

## GitHub kaynak yayını — taşınabilirlik düzeltmesi

GitHub Windows runner ilk koşusunda own fixture ham cwd metnini, LaunchContext ise Windows tarafından çözümlenmiş dizini kullanıyordu. Aynı klasörün kısa/uzun veya `\.` yazımları metinsel eşitlik garantisi vermez. Fixture artık açılmış cwd ve beklenen klasörün volume/file ID bilgisini karşılaştırır. Eşdeğer yol pozitif; farklı mevcut klasör ve yanlış environment token negatif kontrolleri eklenmiştir. Production LaunchContext, directory pin ve snapshot davranışı değişmez.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.

## Kod 0.1.12 — Proxy dönüş sınırı

[0.1.12 proxy-return](d1-proxy-return.md) entry moduyla aynı explicit cwd ve retained environment snapshot akışını kullanır; yeni environment değeri, registry kuralı veya arama dizini eklemez. Aynı orijinal/private dosya hash’i launch context eşitliği değildir. Gerçek deneyler input önce/sonra hash ve context digest ile ayrı raporlanır; dizin içeriği bu helper tarafından dondurulmaz.

## Kod 0.1.13 — Startup çağrı sınırı

[Startup-call CLI](d1-startup-call.md) proxy-return ile aynı retained directory ve tek bounded environment snapshot’ını kullanır. Entry yürütme izni context’ten türetilmez, ayrı compiled stage gerekir. Ham ortam/stack içerikleri raporlanmaz; yalnız call return/argument adresleri ve seçilmiş image baytları vardır. Original/private aynı hash’e sahip olsa da loader ortamları ayrı kanıttır.

## Kod 0.1.14 — Codec dönüş kesiti

Yeni codec komutu explicit canonical cwd/environment snapshot ister. Wrapper isimle arama yapmaya devam eder; gerçek mapping dosyası retained pin ile karşılaştırılır. Codec deneyi ayrı özel dizinde yapılır, eski üç dosyalı deney dizinine dosya eklenmez. [Sözleşme ve doğrulama](d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

Binding komutu explicit canonical cwd/environment ve exact input pinleriyle çalışır. Deney önceki özel altı dosyalı dizini kullanabilir; orijinal kurulum ve tarihsel kopyaların dosya hash korunumu ayrıca sınanır. [Sözleşme ve kanıt](d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

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
