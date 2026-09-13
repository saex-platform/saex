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
