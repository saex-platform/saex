# D1-N2 — Native import/export bağlantı incelemesi

Mimari v0.17 / kod 0.1.10, 13 Eylül 2026. [Durum](status.md) · [Motor planı](d1-engine-integration.md) · [Başlangıç envanteri](d1-native-startup.md) · [Native sözleşme](../architecture/native-sdk-integration.md) · [ADR-45](../decisions/architecture-decisions.md).

## Sorumluluk ve sınır

[PeLinkageInspector](../../managed/Saex.Tools/PeLinkageInspector.cs), bir executable/DLL'nin açıkça seçilmiş import modülü için istediği sembolleri, açıkça seçilmiş DLL dosyasının export metadata'sıyla karşılaştırır. Önceki envanter yalnız DLL adlarını çıkarıyordu; bu kesit normal/delay import thunk, isim/ordinal, export alias/boşluk, code/data bölümü ve forwarder bilgisini ekler. Dosya adlarının aynı olması gerekmez; bu, adayın Windows tarafından seçileceğinin veya yeniden adlandırılabileceğinin kanıtı değildir.

C#/.NET 10 geliştirme komutudur. Dosyalar salt okunur açılır, native DLL/assembly yüklenmez, oyun veya child process başlatılmaz. GNS, SDK, bootstrap C ABI 1 ve incelenmiş 22 pinli loader politikası değişmez. Statik isim eşleşmesinden function prototype, calling convention, DllMain/TLS davranışı, runtime resolution, dosya güvenilirliği veya clean-install sonucu çıkarılmaz. `CanAttach`, `CanInitialize`, `RuntimeResolutionVerified`, `CallingConventionVerified`, `DynamicLoadsEnumerated` daima false'tur. N2 initialization/gerçek SAEX DLL/unpack/ABI ve N3 açık kalır.

## Veri akışı ve arayüz

```text
dotnet managed/Saex.Tools/bin/Debug/net10.0/Saex.Tools.dll engine linkage <consumer> <imported-module> <candidate.dll>
```

1. Imported-module, en fazla 127 karakterlik ASCII basename olarak doğrulanır; dizin/drive/traversal biçimi ret alır. DLL adı case-insensitive normalize edilir; sembol adı case-sensitive kalır.
2. Yalnız iki açık dosya sırayla okunur; consumer native I386 PE32 EXE/DLL, candidate native I386 PE32 DLL olmalıdır. Bilinmeyen engine hash'i statik incelemeye engel değildir, çalışma yetkisi vermez. Her dosya en fazla 64 MiB, bu karşılaştırmanın toplamı en fazla 128 MiB'dir. Directory scan, bağımlılık indirme veya recursion yoktur.
3. Başlangıç envanterinin ortak PE layout/RVA/descriptor okuyucusu kullanılır. Header/section çakışması, aralık ve file backing denetlenir. Modül metadata'sı ile SHA-256 aynı açık read/share-read handle üzerinden çıkarılır. Kapsam import/export tablolarıdır; TLS/CRT initializer içeriği bu raporla doğrulanmaz.
4. Normal import için OriginalFirstThunk varsa okunur. Yoksa yalnız timestamp=0 olan unbound FirstThunk lookup olarak kabul edilir; bound IAT'den isim tahmin edilmez. Lookup ve IAT slotları file-backed olmalı, karşılıklı NUL sonlandırma korunmalıdır. IAT adres değerleri yürütülmez veya çözümlenmez. Hint tanısaldır, eksik ismi ordinal eşleşmesiyle kurtarmaz. Ordinal import sıfırı ve ayrılmış bitleri ret alır.
5. Delay import yalnız RvaBased=1 biçimiyle sembol düzeyinde desteklenir. Legacy VA descriptor `linkage_legacy_delay_unsupported` verir; önceki `engine startup` komutunun legacy DLL adı okuması korunur. Destek sınırı, dosyanın Windows'ta mutlaka bozuk olduğu iddiası değildir.
6. Export isim tablosu strict ordinal sıralı ve tekil olmalı; ordinal index EAT sınırında olmalıdır. Aynı export slotuna birden çok isim bağlanabilir. Sıfır EAT slotu ordinal/named hole'dur; hiçbir import bununla eşleşmez. Export base+index taşması ve tablo/hedef aralıkları denetlenir.
7. Export directory aralığındaki hedef forwarder metnidir; NUL aynı directory içinde ve string sınırında olmalıdır. Metin raporlanır, syntax/bağımlılık zinciri çalıştırılmaz; gerekli forwarder `forwarder_unresolved` üretir. Directory dışındaki hedef geçerli image bölümüne düşmelidir. Executable bölüm hedefi file-backed olmalı; data bölümünde zero-fill temsil edilebilir. Bölüm bayrağı gerçek function/data ABI türünün kanıtı değildir.
8. Seçilen modülün bütün normal/delay importları exact isim veya ordinal üzerinden eşleştirilir. Modül hiç import edilmemişse `requested_module_not_imported`; eksik sembolde `missing_export` vardır. Hiç gereksinim bulunmaması başarı sayılmaz. Candidate'in başka importlarının çözümü bu ikili karşılaştırmanın başarı ölçütü değildir.

## Sonuç, bütçe ve hata davranışı

JSON `schemaVersion=1`, `scope=static-native-linkage` taşır. `Consumer`/`Candidate` dosya adı, byte, SHA-256, import/export listeleri ve layout notlarını; `Bindings` seçilmiş gereksinim ile compact export hedefini; `Issues` tamamlanmamış eşleşmeleri verir. Alias listeleri yalnız modül export envanterinde bulunur; her binding içinde tekrar edilerek çıktıyı karesel büyütmez. CLI property adları camelCase'dir.

Her dosyada normal+delay toplamı en fazla 4096 import sembolü; export slot ve isim sayıları ayrı ayrı en fazla 4096'dır. Alias isimleri slot sayısını aşabilir. Her sembol/forwarder, NUL dahil en fazla 512 byte ve görünür ASCII aralığındadır; kontrol karakterli isimler desteklenmez. Her normal/delay descriptor tablosu 128 kayıt, data directory 1 MiB sınırını paylaşır. Export isimleri, adres tabloları ve stringleri kendi RVA sınırlarıyla ayrıca denetlenir. Header tabanlı tablolar ve desteklenmeyen PE varyantları açık ret alabilir. Sınırlar parser iş miktarını bağlar; senkron disk I/O için katı wall-clock veya genel Windows loader uyumluluğu garantisi değildir.

`StaticSymbolsComplete=true`, yalnız seçilen ikili için bütün gereksinimler doğrudan metadata hedefiyle eşleşti demektir. Exit **3** bu sonuç/runtime kapalı; **1** eksik/forwarded gereksinim, destek dışı veya bozuk/erişilemeyen girdi; **2** yanlış kullanımdır. Girdi hatası `input_rejected` ve detail ile döner; kısmi parse edilmiş modül başarılı rapor olarak dönmez. Exit 0 ve profile/policy yükseltme yolu yoktur.

İki dosya atomik dizin snapshot'ı değildir. Reparse kontrolü/open arasında yarış olasılığı ve iki okuma arasında çevre değişikliği korunur; hash çalışma anı dosya kimliği veya Authenticode/transitive trust kanıtı değildir. Gerçek loader gelecekte kendi live handle/file ID/hash kapısını uygulamalıdır. Kaldırılan özellik yoktur; mevcut inspect/startup komutları, digest ve snapshot şemaları korunur.

## Kabul senaryoları ve doğrulama

[LinkageTests](../../tests/managed/Saex.Foundation.Tests/LinkageTests.cs) 22 yeni managed test kaydı içerir: isim/ordinal ve alias, case/hint, named/ordinal holes, RVA delay/legacy ret, unbound fallback/bound IAT ret, tablo/isim/ordinal/aralık limitleri, 4096 import ve normal+delay ortak bütçesi, bozuk terminator, export overflow/index/sıra, zero-fill data, forwarder ve directory sonu, absent module/export, yanlış candidate/PE/path, hash korunumu ve CLI 3/1/2. Büyük alias listelerinin binding içinde yinelenmediği de denetlenir. Bu corpus Windows API resolution, calling convention veya gerçek initializer testi değildir.

İlk hedefli Debug koşusu 79/79 managed/entegrasyon testiyle geçti. Son artifact üzerinde tools/build.ps1 ile x86 Debug/Release akışlarının her biri 8 native CTest suite, 79 managed/entegrasyon ve 49 Python testiyle geçti. X64 Debug 6 native suite, 79 managed/entegrasyon ve 30 Python testiyle geçti. Üç tam akış da ilk denemede başarılıydı. Bootstrap artifact audit ve oyun dışı 100 warm lifecycle çevrimi standart kontrolün içindedir; yeni gerçek GTA initialization kanıtı değildir. X64 Release/Linux çalıştırılmadı; N1 SDK source/recipe değişmediği için opt-in SDK build yeniden yapılmadı.

Yerel loglar: `out/verification/engine/build-linkage-x86-Debug.log`, `build-linkage-x86-Release.log`, `build-linkage-x64-Debug.log`. Yeni testler sahte input/çıktı kopyalamak yerine isim/ordinal çözümü, kaynak bütçesi ve negatif metadata yollarını sınar. Source→owner eşlemeleri ve ilk belge kontrolü 77 Markdown/1037 yerel link/6 JSON örneğiyle hatasız geçti; son belge kanıtı `out/verification/docs-check.json` içindedir.

## Gerçek kurulumun statik bulguları

Setup: kullanıcının `C:\Program Files (x86)\Rockstar Games\GTA San Andreas` dizinindeki dosyalar salt okunur incelendi. Engine SHA-256 `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`; kullanılan C# artifact ve bütün aday dosya hash'leri yerel JSON kanıtına kaydedilir. Oyun başlatılmadı; özel deney kopyası veya orijinal native dosyalar değiştirilmedi.

| Consumer → logical import → açık aday | Doğrudan eşleşme | Sonuç |
|---|---:|---|
| gta_sa.exe → vorbisfile.dll → vorbisFile.dll | 7/7 | Statik alt kapsam tamamlandı, exit 3 |
| gta_sa.exe → vorbisfile.dll → vorbisHooked.dll | 7/7 | Alternatif dosyada aynı gerekli isimler var, exit 3; değiştirme/ABI onayı değil |
| vorbisFile.dll → vorbisHooked.dll → vorbisHooked.dll | 0 | Statik import yok, `requested_module_not_imported`, beklenen exit 1 |
| vorbisHooked.dll → vorbis.dll → vorbis.dll | 22/22 | Statik alt kapsam tamamlandı, exit 3 |
| vorbis.dll → ogg.dll → ogg.dll | 11/11 | Statik alt kapsam tamamlandı, exit 3 |

GTA'nın istediği yedi isim: `ov_open_callbacks`, `ov_clear`, `ov_time_total`, `ov_time_tell`, `ov_read`, `ov_info`, `ov_time_seek`. `vorbisFile.dll` sekiz, `vorbisHooked.dll` 34 export slotu içerir; ordinal numaraları farklı olduğundan sadece export sayısını karşılaştırmak yeterli değildir. `ov_time_seek_page` wrapper export'udur fakat bu executable'ın statik gereksinimleri arasında değildir.

Mevcut `vorbisFile.dll` importlarında `LoadLibraryA`, `GetProcAddress`, `FindFirstFileA`, `FindNextFileA`, `SetCurrentDirectoryA`, `VirtualProtect`, `GetPrivateProfileIntA` bulundu. Bağımsız yerel dumpbin ve byte-string incelemesinde `*.asi`, `vorbishooked`, `scripts`, `scripts\global.ini`, `loadplugins` ve Silent's ASI loader adını içeren PDB yolu görüldü. Bu bulgular proxy/eklenti yükleyicisi davranışını araştırmayı gerektirir; API'nin çağrıldığını, DLL'nin gerçekten seçildiğini, tam upstream kaynak kimliğini veya zararlı davranışı kanıtlamaz. Import/export aracı string tarayıcı veya initializer disassembler değildir; yardımcı inceleme ayrı kanıttır.

Hypothesis: aday DLL aynı statik sembolleri sağlayabilir. Observed: yukarıdaki ikili eşleşmeler bunu yalnız belirtilen dosya byte'ları için doğruladı. Pass/fail: dört doğrudan ikili statik pass, statik olmayan wrapper→hooked için beklenen incomplete sonucu. Affected capabilities: yalnız geliştirme metadata aracı; hiçbir runtime capability açılmadı. Sonraki deney, proxy initializer/dinamik dependency davranışını ve bağlanacak fonksiyon ABI'sini ayrı incelemeli, sonra incelenmiş bounded initialization ve gerçek SAEX bootstrap kanıtını üretmelidir. Yeni DLL seçimi otomatik staging/rename/pin güncellemesi olamaz; ADR-45 yeniden değerlendirilir.

## Kaynaklar ve genişleme noktaları

Import thunk/name/ordinal ve export/forwarder alanları için [Microsoft PE biçimi](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format) kullanıldı. [GetProcAddress API](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress) isimlerin birebir eşleşmesi ve ordinal boşlukları konusunda ek sınırı açıklar. [C++ GetProcAddress açıklaması](https://learn.microsoft.com/en-us/cpp/build/getprocaddress?view=msvc-170), çağrı için doğru prototype/type bilgisi gerektiğini belirtir. SAEX limitleri, compact binding ve fail-closed kuralları bu projenin tasarım kararıdır.

Legacy delay symbol corpus'u, forwarder resolver, API-set/SxS kimliği, dynamic LoadLibrary izlemesi ve initializer/ABI doğrulaması sonraki ayrı kesitlerdir. Her ek, budget/negatif test/ADR/AC-90 ve bu raporla birlikte değişir. Statik çıktıyı production loader veya indirilen C# sandbox kanıtı olarak kullanmak sözleşme ihlalidir.

## Son yerel kanıt — 13 Eylül 2026

`out/verification/engine/native-linkage-evidence.json` 10 CLI koşusunu, expected/observed exit, ham rapor hash'leri, araç hash'leri ve 13 original + 3 private dosyanın before/after kayıtlarını taşır. Dört direct case, her configuration için toplam 47, ikisi için 94 binding verdi. Wrapper→hooked iki koşuda beklenen exit 1; diğer sekiz koşu exit 3 verdi. Her sonuçta beş runtime flag false kaldı. Debug ve Release içerik raporları birebir aynı; native input hash'leri 0.1.9 lifecycle kanıtıyla da eşleşti. Oyun process'i oluşturulmadı.

| Saex.Tools.dll configuration | SHA-256 |
|---|---|
| Debug | `02d797f3221161281a2030209539e72e4d3dcf9a3badf1a75d5cd9638c2ebb60` |
| Release | `a74f82c171398955895c51e7f6a0d2e5f17b326ba421434ac67d4e2004d93d34` |

MSVC 14.44.35207 dumpbin ile wrapper'ın 8, hooked dosyasının 34 export ordinal/RVA/ismi parser çıktısıyla bağımsız karşılaştırıldı ve tamamı eşleşti. `linkage-dumpbin-vorbisFile.dll.txt` / `linkage-dumpbin-vorbisHooked.dll.txt` ve byte-string offset kayıtları aynı evidence dizinindedir. Bu kontrol disassembly'den çağrı ABI'si türetmez ve hiçbir native kodu çalıştırmaz. Bir sonraki kesit için dinamik initializer yolu ve fonksiyon ABI kanıtı eksiktir; statik alternatif adayın bulunması üzerine dosya yeniden adlandırılmadı veya loader policy genişletilmedi.
