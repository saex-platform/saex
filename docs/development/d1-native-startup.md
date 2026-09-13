# D1-N2 — Native başlangıç envanteri

Mimari v0.12 / kod 0.1.5, 13 Eylül 2026. [Durum](status.md) · [Motor planı](d1-engine-integration.md) · [Önceki süreç gözlemi](d1-suspended-process.md) · [Native sözleşme](../architecture/native-sdk-integration.md).

## Sorumluluk ve kapsam

GTA executable'ının aynı olması, yanında bulunan DLL/ASI dosyalarının aynı olduğu anlamına gelmez. Bu kesit, başlatma fazını ilerletmeden önce **oyun dizinindeki native başlangıç girdilerini** salt okunur inceler. C#/.NET 10 geliştirme aracına `engine startup` komutu eklenmiştir; C++20 çekirdek veya x86 bootstrap ABI'si değişmemiştir. Yeni NuGet/native bağımlılığı yoktur.

[PeStartupInspector](../../managed/Saex.Tools/PeStartupInspector.cs), BCL `PEReader` üzerinden native I386/PE32 EXE veya DLL metadata'sını okur. [NativeStartupInspector](../../managed/Saex.Tools/NativeStartupInspector.cs), executable'ın bulunduğu dizinin üst seviyesindeki `.dll`/`.asi` adaylarını ve extension işaretlerini toplar. Dosya/assembly yükleme, process oluşturma, breakpoint, injection veya kurulum dosyasını değiştirme yolu yoktur.

Önceki `engine inspect` dosya gözlemi, `saex_engine_image_probe` ve `saex_engine_process_probe` sözleşmeleri korunur. Bu komut ayrı bir statik scope üretir; ilk create-debug olayını ilerletme izni vermez. **CanAttach=false, CanAdvanceToLoader=false, RuntimeResolutionVerified=false ve DynamicLoadsEnumerated=false** her sonuçta sabittir. N2/AC-90/R-01a/b bütünü, unpack/ABI, gerçek DLL load sırası ve N3 hâlâ açıktır.

## Veri akışı ve arayüz

1. Executable yolu tam yola çevrilir; mevcut path/üst dizinlerde reparse point görülürse ret verilir. Dosya `FileAccess.Read/FileShare.Read` ile açılır. Aynı handle üzerinde PE metadata, SHA-256 ve gömülü engine profilinin dosya eşleşmesi çıkarılır. Bilinmeyen executable metadata'sı incelenebilir; `fileProfileMatched=false` kalır, çalıştırılmaz.
2. Yalnız üst dizinin en fazla 4096 girdisi listelenir; en fazla 128 yerel DLL/ASI adayı alınır. Case farkıyla aynı ada düşen adaylar reddedilir. CLEO/modloader/plugins/scripts dizin adları, ASI dosyaları ve exe `.local`/`.manifest` dosyaları işaret olarak kaydedilir; bu dizinlere girilmez. İşaret, yükleyicinin aktif olduğunun kanıtı değildir.
3. Modül başına 64 MiB, executable ve adayların toplamında 256 MiB okuma bütçesi vardır. Açık handle'ın gerçek boyutu parsing öncesi bütçeden düşer; bozuk modül de tükettiği bütçeyi geri kazanmaz. Adaylar sabit ada göre sıralı işlenir. Her dosya bitince handle bırakılır.
4. EXE/DLL türü, x86 native oluşu, PE bölüm/raw/image aralıkları, çakışma ve entry RVA kontrol edilir. Data-only DLL için sıfır entry temsil edilebilir; executable için sıfır entry reddedilir. Entry ve TLS callback hedefleri image içinde file-backed executable bölüme düşmelidir; bu hedefler çağrılmaz. Raw bölüm boyutunun file alignment'a tam bölünmemesi, güvenli dosya/image sınırları korunuyorsa `LayoutNotes` altında bölüm indeksiyle kaydedilir; uyumluluk kabulü değildir. Native C++ preflight'ın hizalama koşulu değişmez.
5. Normal ve delay import descriptor'larından basename DLL adları alınır. Delay import adlarında RVA ve eski VA biçimleri ayrı ele alınır. Her tablo en fazla 128 descriptor, her ad en fazla 127 ASCII karakter; NUL/descriptor sonlandırma, aralık, duplicate case ve path bileşeni denetimi vardır. Her data directory en fazla 1 MiB ve tek file-backed bölümle sınırlıdır.
6. TLS directory yokluğu ile mevcut fakat boş callback listesi ayrılır. PE32 TLS adresleri VA olarak image base'e göre çözülür; en fazla 32 callback, terminator ve raw/zero-fill bütçeleri kontrol edilir. Bound import directory yalnız varlık/aralık düzeyinde kaydedilir.
7. Bütün başarıyla parse edilen üst seviye modüllerin normal/delay bağımlılıkları edge listesine yazılır. Aynı ada sahip yerel dosya **LocalCandidate** olarak işaretlenir; yoksa null kalır. Bozuk bir adayın varlığı edge'den silinmez ve `Issues` içinde gösterilir. Döngülü import grafiği recursion gerektirmeden kaydedilir; DAG zorunluluğu konmaz.

Import descriptor/VA-RVA/TLS alanları için [Microsoft PE biçimi](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format), header erişimi için [.NET PEHeader](https://learn.microsoft.com/en-us/dotnet/api/system.reflection.portableexecutable.peheader?view=net-10.0) kullanıldı. Bu parser bütün PE çeşitlerini kabul eden Windows loader doğrulayıcısı değildir; header tabanlı directory veya destek sınırı dışındaki biçimler açık ret alabilir.

## Sonuç ve hata davranışı

`StartupInventory`, `Snapshot` ve `InventoryDigest` döndürür. Snapshot schemaVersion=1, scope=`static-native-startup-inventory`; engine gözlemi, modül boyut/hash/entry/import/TLS kayıtları, aday edge'ler, extension işaretleri, sorunlar ve harcanan byte sayısını taşır. JSON CLI çıktısı camelCase'dir. Digest, snapshot nesnesinin varsayılan .NET JSON property adları ve sabit property sırasıyla UTF-8 serialization'ının SHA-256'sıdır; pretty-printed CLI çıktısının hash'i değildir. Timestamp ve mutlak kullanıcı yolu snapshot kimliğine girmez.

Başarıyla parse edilen dosyanın byte'ı değişirse modül hash'i ve inventory digest değişir. Bozuk/erişilemeyen adayın hash'i her durumda üretilemez; böyle sonuçlarda `metadataComplete=false`, isim/hata/bütçe kaydı vardır. Bu digest atomik dizin snapshot'ı, imza, release izni veya capability evidence değildir. Reparse kontrolü ile dosya açma arasında yarış olabilir; farklı dosyalar aynı anda kilitlenmez. İleride process fazı ilerletilmeden gerçek handle/module kimliği ayrıca doğrulanmalıdır.

CLI **exit 3**, bu tanımlı statik metadata alt kümesi tamamlandı ama runtime kapalı demektir. **Exit 1**, root girdi/PE/kota hatası veya herhangi bir yerel adayın eksik/bozuk/erişilemez metadata'sıdır. Yerel aday hataları snapshot içinde bırakılır; toplam kota aşımı tüm işlemi sonlandırır. **Exit 2**, kullanım hatasıdır. Exit 0 veya otomatik profile yükseltme yoktur. Kaldırılan özellik veya mevcut CLI kullanıcıları için zorunlu migration yoktur.

## Genişleme ve gerçek Windows yükleme sınırı

Üst dizindeki dosyaya bakmak, Windows'un gerçekten o dosyayı seçeceğini kanıtlamaz. KnownDLLs, API sets, SxS/manifest, redirection, OS dizinleri, process ayarları ve dinamik `LoadLibrary` çağrıları ayrıca etkilidir. Null aday “sistem DLL'si bulundu” veya “dosya eksik” diye çevrilmez. Import thunk/function/forwarder çözümü, Authenticode/transitive trust, delay bound tabloları, CRT initializer gövdeleri ve modların dinamik taraması bu kesitte uygulanmadı. [Windows DLL arama sırası](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order).

Gelecek N2 fazı, seçilen kurulumun bu girdilerini hesaba katan kontrollü başlatma deneyi ve loaded-module kimlik doğrulamasıdır. Doğrulanmamış ASI/DLL'leri silmek, yeniden adlandırmak veya bulundukları dizini sessiz temizlemek çözüm değildir. Ayrı deney kurulumu/başlatma politikası ve kullanılan native kapsamın kanıtı gerekir; mevcut envanter otomatik clean-install onayı vermez.

## Kabul ve yeniden üretim

[StartupTests](../../tests/managed/Saex.Foundation.Tests/StartupTests.cs), standart managed test runner'a eklenir. Sentetik EXE/DLL, normal/delay import, VA underflow, duplicate/path/string/descriptor limitleri, TLS present/empty/target/count/data limitleri, zero-fill/raw ayrımı, yerel cycle/aday/bozuk modül, değişen hash ve CLI exit sınırları kontrol edilir. Dosyalar yalnız testin oluşturduğu geçici dizindedir ve çalıştırılmaz. Windows reparse-race, SxS/OS çözümü veya gerçek oyun modu davranışı bu testlerce kanıtlanmaz.

```powershell
dotnet managed/Saex.Tools/bin/Debug/net10.0/Saex.Tools.dll engine startup "C:\Program Files (x86)\Rockstar Games\GTA San Andreas\gta_sa.exe"
# Exit 3: statik metadata tamamlandı/runtime kapalı. Exit 1: tamamlanmamış/ret.
```

### Uygulama sonucu

İlk koşuda 55 managed testin üçü, `InvalidDataException` yerel modül/CLI hata filtresinde açıkça sayılmadığı için başarısız oldu. Filtre düzeltildi; bozuk aday partial metadata olarak kalır, root hatası JSON/exit 1 üretir. Önceden mevcut `engine inspect` için de aynı hata türünün dışarı taşması giderildi ve iki CLI'yi kapsayan regresyon eklendi. Başarılı eski inspect JSON/exit sözleşmesi değişmez.

İlk gerçek envanterde `bass.dll` bölüm 2 (`petite`) raw boyutu 3888 byte, file alignment 512 gözlendi ve katı padding koşulu metadata'yı eksik bıraktı. Güvenli okuma için gerekli raw/image sınırları ve çakışma retleri korundu; eksik hizalama padding'i gözlemde `section_raw_size_unaligned:2` notu olarak temsil edildi. Yeni negatif/pozitif regresyon bu notun loader iznine dönüşmediğini denetler. Bu dosyanın Windows loader davranışı, paketleyicisi veya çalışma güvenliği doğrulanmış sayılmaz.

13 Eylül 2026: **x64 Debug ve x86 Release** standart build akışları geçti. X64 4/x86 5 native suite, her koşuda **57 managed test** (önceki 26 + yeni 31), x64 22/x86 32 Python testi başarılı. X86 Release bootstrap artifact denetimi ve önceki oyun dışı lifecycle testi de geçti. C++ kaynakları/DLL ABI'si değişmedi; bu kesitte x86 Debug/x64 Release ve Linux yeniden çalıştırılmadı. N1'in 23 dosyalı kaynak/patch/recipe verify kontrolü başarılı; SDK yeniden derlenmedi.

Yerel kurulumda **1 executable + 12 DLL/ASI**, toplam **17.922.424 byte** incelendi. Sonuçta **75 normal/delay import ilişkisi**, sıfır metadata issue ve bir raw padding notu var. Debug ve Release araçları exit 3 verdi; inventory digest ikisinde de `9563f60b0e27c8348e509719128bd71a96945483eb6c3778d2785064d0895274`. MetadataComplete=true yalnız yukarıdaki tanımlı statik alanlar içindir; runtime resolution ve loader/attach izni false kaldı.

| Yerel bulgu | Kaydedilen kapsam |
|---|---|
| GTA dosyası | SHA-256 `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`; aynı gömülü dosya profili eşleşti |
| GTA entry | RVA `0x424570`; fonksiyon çağrı sözleşmesi veya unpack fazı kanıtı değil |
| GTA normal import | 9 isim; yerel adaylar `eax.dll` ve `vorbisfile.dll` |
| GTA delay import | `d3d9.dll`, `ddraw.dll`, `dinput8.dll`, `dsound.dll`; üst dizinde eş ad yok, Windows seçimi yapılmadı |
| GTA TLS/bound import | Bu executable metadata'sında iki directory de yok; transitif DLL initializer davranışı çıkarılmaz |
| Eklenti işaretleri | `_nodep.asi`, `cleo.asi`, `crashinfo.sa.asi`, `modloader.asi`, `cleo` ve `modloader` |
| Yerel graph örnekleri | CLEO.asi → bass.dll; CrashInfo.SA.asi → libcurl.dll → zlib1.dll; vorbisHooked.dll → vorbis.dll → ogg.dll |
| bass.dll notu | Bölüm 2 raw size=3888, file alignment=512; SHA-256 `6e1bf8ea63f9923687709f4e2f0dac7ff558b2ab923e8c8aa147384746e05b1d`; padding farkı kayıtlı |

**Kurulumdaki 13 native dosyanın SHA-256'sı öncesi/sonrası korundu.** Game process oluşturulmadı ve mevcut mod dosyalarına dokunulmadı. Salt gözlem sırasında native DLL yüklemek bu araçta yoktur. Özellikle `vorbisFile.dll` yanında `vorbisHooked.dll` bulunması, ikisinin runtime akışının statik import tablosundan bütünüyle çıkarıldığı anlamına gelmez.

| Saex.Tools.dll artifact | SHA-256 | Gerçek kurulum koşusu (UTC) |
|---|---|---|
| Debug | `d63a70cae95ad478c512b448726b43106360939a33ec963b19ff45cbf3069ec3` | 12 Eylül 23:29 |
| Release | `88cf2cd5f404be82585ca75c93d5e7c6a560fc50c3f872c2b40cd35c75554f09` | 12 Eylül 23:31 |

.NET SDK 10.0.300 kullanıldı. Yerel `out/verification/engine/native-startup-Debug.json` ve `native-startup-Release.json` snapshot'ları, `native-startup-evidence-<Configuration>.json` dosyaları araç/hash/önce-sonra bilgilerini taşır. İlk padding-reddedilmiş gözlem `native-startup-initial-Debug.json` olarak korundu. X86 Release tam akış log'u `build-startup-x86-Release.log` dosyasındadır. Bu hash'ler bu koşuların artifact kimlikleridir; tekrar derlemede aynı binary garantisi değildir.

Belge gate'i 72 Markdown, 899 yerel bağlantı ve 6 JSON örneğinde sıfır hata verdi. Yeni source-map sahiplik kuralı startup kaynaklarını normatif belgelere bağlar; kontrol sonrası ek padding regresyonunda eksik status/foundation güncellemesi gate tarafından yakalandı ve build'den önce tamamlandı. Bu statik kapı anlamsal veya native runtime doğruluğu iddiası değildir.

## Kod 0.1.6 sonraki tüketici sınırı

[Ayrı loader deneyi](d1-loader-observation.md) gerçek LOAD_DLL dosyalarını gözlemler; bu C# snapshot'ını bir izin manifesti olarak tüketmez. CLI'nin yalnız üç OS dosyalı mapping politikası canlı tutulan file ID/hash pinlerine dayanır. Gerçek GTA'da statik root import listesinde bulunmayan apphelp.dll mapping'inde duruldu. MetadataComplete/CanAdvanceToLoader sözleşmesi ve önceki kod 0.1.5 snapshot/artifact hash'leri bu ekle değişmez.

## Kod 0.1.10 — Ayrı sembol incelemesi

[engine linkage](d1-native-linkage.md) iki açık dosyada import thunk/export sembol karşılaştırmasını ekler. PeStartupInspector.Reader'ın layout/RVA/descriptor yardımcıları internal paylaşılır; startup snapshot/digest/CLI ve legacy delay DLL-adı davranışı aynıdır. Yeni araç yalnız RvaBased delay sembollerini destekler, legacy sembolde açık ret verir. Startup envanterinin genel Windows resolution, forwarder zinciri, dinamik load ve initializer sınırları bu ekle kapanmaz.
