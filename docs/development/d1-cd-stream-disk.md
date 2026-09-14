# D1 — Disk sorgusu ve streaming bellek hazırlığı

Kod 0.1.38 / mimari v0.45, ADR-73, AC-90 alt kesiti. [Uygulama durumu](status.md) · [Önceki tablo sınırı](d1-cd-stream-tables.md) · [Yol haritası](../roadmap.md)

## Sorumluluk ve kapsam

`run_cd_stream_disk` / `--observe-cd-stream-disk <exe> <absolute-cwd>` tablo hazırlığından sonra **GetDiskFreeSpaceA çağrısını, dönüşünü ve ilk allocation çağrısının argüman hazırlığını** yürütür. Terminal **0x406BF4**, `CALL 0x72F4C0` önüdür. Bellek henüz ayrılmamıştır. Orijinal dosyalar değiştirilmez; gözlemci yalnız kendi oluşturduğu süreçte debugger sınırlarını yönetir ve çıkışı doğrular.

Eski `--observe-cd-stream-tables` 0x406BB4'te, `--observe-file-manager-ready` 0x53BB5F'te durmaya devam eder. Yeni komutun önceki checkpoint kayıtları tarihsel snapshot'tır; nihai disk sonucu `cdStreamDiskObservation` alanındadır. `tools/Run-SAEX.ps1` artık yeni komutu kullanır ve disk sonucu ile bellek hazırlığını raporda ayrı gösterir. Bootstrap export'ları çağrılmaz; `canAttach`, `initializationVerified`, `streamingReady` ve `allocationCallAllowed` false kalır.

## Veri akışı ve ABI

Exact executable SHA-256: `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`. Image base 0x400000; kod aralığı **0x406BB4–0x406BF8**, 69 byte, son allocation CALL yalnız şekil doğrulamasına dahildir. `contracts/engine/cd-stream-disk-policy.json`, önceki tablonun raw policy hash'ine bağlıdır; generator parent zincirini ve OS modüllerini de doğrular. Statik Ghidra çıktısı runtime izni vermez.

`C`, önceki CFileMgr dönüşündeki ESP olsun. Disk API argümanları NULL root, C−12 sektör/küme, C−24 byte/sektör, C−16 boş küme, C−20 toplam kümedir. Native local sırası bu mantıksal alan sırasından farklıdır. Hiçbir çıktı önceden sıfır varsayılmaz.

| Aşama | Durak | ESP | Denetim |
|---|---|---|---|
| Önkoşul | 0x406BB4 | C−48 | Tam tablo sonucu, kod/IAT/thunk, writable globals ve caller hazır |
| 1 | Kernel32 GetDiskFreeSpaceA thunk | C−52 | Dönüş adresi + beş argüman, korunacak register/flags |
| 2 | KernelBase GetDiskFreeSpaceA girişi | C−52 | Thunk slotu, gerçek implementation prefix'i ve aynı argümanlar |
| 3 | 0x406BBA, API dönüşü | C−28 | Stdcall stack, korunacak register'lar, BOOL; başarı halinde dört çıktı |
| 4 | 0x406BF4, allocation CALL önü | C−44 | 2048 byte / mantıksal sektör hizalaması / 0 argümanları ve saved ESI |

Kernel32 x86 export RVA **0x1EBA0**, altı byte `FF25` thunk; slot RVA **0x81354**. KernelBase implementation RVA **0x241BF0**, ilk 20 byte ve offset 15 HIGHLOW relocation doğrulanır. Kernel32'nin altı byte sonrasındaki komşu export kodu bu thunk'ın parçası sayılmaz. Aynı isimli dosya yeterli değildir: loader admission, etkin mapping, exact modül hash'i, IAT adresi, relocation ve kod eşleşmesi birlikte gerekir. Windows 26200.9445 pinleri değişmedi; bilinmeyen OS için otomatik yeni izin üretilmez.

## Sorgu sonucu ve geometri politikası

BOOL sıfırsa observer **0x406BBA'da** `cd_stream_disk_query_failed` ile durur. Çocuk TEB'inden LastError okunur; gözlemcinin kendi GetLastError değeri onun yerine kullanılmaz. Başarısız sorgunun dört çıktısı yorumlanmaz; çıktı modeli sıfırlanır ve GTA'nın kontrolsüz local okumasına izin verilmez. Başarı BOOL'ü herhangi bir sıfır dışı değer olabilir. Başarılı API'nin LastError'ı sıfır olmak zorunda değildir.

Başarı halinde admitted profile: byte/sektör 512–65536 arası ikinin kuvveti; sektör/küme sıfır dışı ikinin kuvveti; byte/küme hesabı DWORD taşırmaz; toplam küme sıfır dışıdır; boş küme toplamdan büyük değildir. **Boş küme sıfır olabilir.** Bunlar observer'ın açık destek aralığıdır; tüm Windows dosya sistemlerinin evrensel sınırı olarak sunulmaz. Aralık dışı sonuç `cd_stream_disk_geometry_rejected` verir; geometri dönüştürülmez veya başarılı sonuç enjekte edilmez. Kota etkili sayaçlar gözlemdir; sabit disk kapasitesi ya da disk sağlığı testi değildir.

GTA'nın doğal dalı mantıksal sektör ≤2048 olduğunda `0x60000000` (overlapped + no-buffering), daha büyük olduğunda `0x40000000` (overlapped) hazırlar. Bu bayraklar henüz bir dosya açma çağrısına verilmez. 0x8E3FE0 flags, 0x8E3FE4 initialized=0, 0x8E3FE8 overlapped=1 olur. Bunları çevreleyen iki DWORD dahil 20 byte before/after penceresinde yalnız üç hedef değişebilir. EAX flags, ECX sektör; CALL stack'i `{2048, sector, 0, savedESI}` olmalıdır. Son OR'un CF/OF/SF/ZF/PF sonucu kontrol edilir; AF tanımsız kabul edilir, TF/DF reddedilir.

**Mantıksal sektör fiziksel sektör değildir.** `physicalAlignmentVerified=false`: bu sorgu fiziksel disk hizalamasını kanıtlamaz. İleride unbuffered dosya I/O için fiziksel hizalama ve I/O uzunluğu ayrıca doğrulanmalıdır. Windows'un NULL root davranışı current disk köküdür; bu kesit önceki explicit launch cwd ve CFileMgr yol doğrulamasına dayanır. [GetDiskFreeSpaceA sözleşmesi](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdiskfreespacea), [Microsoft dosya hizalama açıklaması](https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering).

## Korunacak durum ve hata davranışı

Her durakta 2192-byte streaming tablo snapshot'ı, CFileMgr'nin 136-byte root/guard penceresi, 32-byte initializer parent ve 156-byte application caller, nested return/streamCount=5, önceki SEH/TIB, bırakılmış critical section, localization ve named instance event tekrar denetlenir. API'nin kendi geçici stack/volatile register kullanımı ile kalıcı caller bozulması ayrılır. Dört output local yalnız başarılı API dönüşünde yorumlanır; sonraki evrede birebir korunur. LastError dönüşte kaydedilir, allocation önüne kadar aynı kalmalıdır.

DR0 aşama hedefi olur; startup IAT, extra ASI ve instance window guard'ları DR1–DR3 üzerinde kalır. Süre/olay/thread/modül/byte bütçeleri ve owner-thread şartı değişmez. Geçersiz policy başlatılan child'ı ilerletmeden sonlandırır; runtime kod/stack/IAT/global/parent sapması güvenli ret verir. Yanlış executable/OS/asset kimliği preflightta reddedilir. Exit 3 sadece bu alt kesit ve owned child çıkışı doğrulandı; exit 1 ret/eksik gözlem, exit 2 kullanım hatasıdır. API takılması süre bütçesi altında durdurulur; bu test Windows API'nin her storage sürücüsündeki davranışını kanıtlamaz.

## Arayüzler ve genişleme

`CdStreamDiskSpec`, `CdStreamDiskGeometry`, portable kod/frame/global doğrulayıcıları ve `CdStreamDiskQueryResult` hata/başarı sözleşmesini ayırır. Sonuç bool'larıyla birlikte ham API result, LastError, geometri, function/implementation/stop adresleri ve globals before/after hex'i kaydedilir. Heap üzerinde tutulan native fixture sonuçları ortak büyük trace'in x86 stack kullanımını sınırlı tutar. C++ tüketicileri yeniden derlenir; bootstrap C ABI 1, C# SDK, GNS, HTTPS, sandbox ve network otoritesi değişmez. Silinen API, dependency veya veri migration'ı yoktur.

İlk sonraki sınır `MallocAlign`/CRT allocation zinciridir. Exact native MallocAlign, `_malloc` başarısızlığını dereference öncesi kontrol etmiyor; null pointer, toplam boyut taşması, allocation başlangıç pointer'ı, payload/guard ve release sahipliği kanıtlanmadan çağrı açılmayacak. Ardından LocalAlloc kanalları, patched Open/Read hedefleri, GTA3.IMG izinli özel veri kopyası, thread/semaphore yaşam döngüsü, CPad ve initializer dönüşü gelir. Initialized bayrağı tek başına streaming hazır değildir.

## Kabul senaryoları ve doğrulama sınırı

| Senaryo | Beklenti / doğrulama türü |
|---|---|
| Başarılı gerçek sorgu | Dört aşama, doğal native global/stack hazırlığı ve 0x406BF4 terminali; Windows fixture ve gerçek GTA |
| BOOL 0, zehirli output | Sıfırlanmış çıktı, failed kararı; portable karar fonksiyonu. Gerçek Windows API failure enjeksiyonu yapılmadı |
| Sıfır dışı BOOL 2/UINT_MAX | Başarı kabul edilir; portable |
| 512–65536 sektörler ve iki bayrak dalı | Portable corpus. Gerçek disk yalnız bu makinenin gözlenen sektörünü doğrular |
| Sıfır/geçersiz sektör, küme çarpımı taşması, free>total | Allocation öncesi ret; portable |
| Boş disk alanı sıfır | Geometri geçer; portable; yazma kapasitesi sözü verilmez |
| Yanlış flags/allocator/thunk/slot/API prefix | Disk CALL açılmadan ret; native fixture |
| Eski tablo modu | Önceki durakta kalır, disk observation armed olmaz; native + gerçek GTA regresyonu |
| Yanlış cwd, instance çakışması, foreign owner, olay limiti | Ret/owner recovery/cleanup; native fixture |
| Allocation canary | Native terminal canary'yi çalıştırmaz; ayrı kontrol canary'nin çalışabildiğini gösterir |
| Tekrarlı çalıştırma | 12 warm çevrim, parent handle sayısı aynı; native |

İlk x86 Debug hedefli iki suite ve sekiz Python policy testi geçti. İlk gerçek GTA koşusu BOOL=1, **512 byte/sektör**, **8 sektör/küme**, flags=0x60000000 ve terminal 0x406BF4 verdi; owned child çıktı, sekiz izlenen dosyanın hash'i aynı. `out/verification/engine/stream-disk-first-gta.json` özel ham kayıttır. İlk derlemedeki eksik public method bildirimi düzeltildi; `stream-disk-build-initial.log` korunur. Policy generator import hatası da giderildi; bir başarılı build gibi sunulmaz. Standart final matrisin sonucu aşağıda ayrı kaydedildi.

## Nihai doğrulama — kod 0.1.38

Canonical özel kayıtlar `out/verification/engine/stream-disk-final-verification.json` ve `stream-disk-gta-evidence.json` dosyalarıdır.

| Ölçüm | Sonuç ve sınır |
|---|---|
| x86 Debug standart akış | **54 native suite / 79 managed test / 313 Python test**, tek tam başarılı koşu |
| x86 Release standart akış | **54 / 79 / 313**, tek tam başarılı koşu; ön derleme ayrıca kayıtlı |
| Yeni portable corpus | **296 kontrol**, x86 Debug/Release ve ek x64 Debug hedefli test |
| Yeni native corpus | **14 senaryo + allocation canary kontrolü + 12 warm**; iki yapılandırma geçti |
| Gerçek GTA | **27/27** beklenen sonuç, **14** başarılı disk ve allocation hazırlığı |
| Süreç/girdiler | **22/22** oluşturulan child kapandı, **139/139** girdi hash'i aynı; beş preflight rette child yok |
| Yol ve önceki modlar | 126-byte cwd geçer, 127-byte ret; farklı host cwd altında explicit child cwd korunur; tables/ready eski terminali korur |
| Kimlik/instance retleri | Yanlış context/artifact, eksik ASI, bozuk codec, bilinmeyen exe, event/mutex çakışmaları beklendiği gibi durdu |
| Yerel PowerShell 5.1 raporları | Debug/Release **11/11** kontrol; her birinde **18/18** girdi korunumu |
| Kaynak/config freeze | **338/338** hash final native/GTA/runner koşuları boyunca aynı |

Yeni native suite'in parent handle sayısı Debug **136→136**, Release **143→143**. API dönüşünden sonraki native hazırlık EDX'i ve OR'un değiştirmediği EFLAGS bitlerini de API-return snapshot'ına göre korumalıdır. Başarısız BOOL/zehirli çıktı, 4K ve diğer sektör geometrileri portable corpus'ta test edildi; gerçek storage failure veya fiziksel hizalama deneyi yapılmadı. Gerçek gözlemler 512-byte mantıksal sektör/8 sektör-küme, 0x60000000 flags ve henüz çağrılmamış 2048-byte allocation hazırlığıyla sınırlıdır.

İlk eksik method bildirimi build hatası saklandı; final iki tam akış temiz geçti. Önceki 0.1.37 bootstrap stack regresyonu da mevcut kaynakla iki tam standart koşu içinde geçti. X64 Release, Linux, hosted CI ve N1 SDK yeniden çalıştırılmadı. Oyun dosyası/OS pini/remote değiştirilmedi; hiçbir oyun sunucusu yeniden başlatılmadı. Allocation, GTA3.IMG I/O, streaming thread, CPad, renderer/doğal frame ve D1/D2 açık.

| Artifact | Debug SHA-256 | Release SHA-256 |
|---|---|---|
| saex_engine_loader_probe.exe | `4e1feef8c1ca9167294dc00da052e795752c332c898a1b3ac5c8ef6b5028e9a7` | `20a708a281dc40b7489a1dca9457e1516c16c42cdf4c19b1ba44a09c4fedf967` |
| saex_bootstrap.dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| saex_bootstrap.map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Disk policy SHA-256: `64b5b07022d33654019d74ba6ec3deaa623f23bcac972280487962234782beb3`.

## Sonraki kesit — kod 0.1.39

[Hizalı allocation](d1-cd-stream-allocation.md) yeni ayrı modda 0x406BF9 doğal dönüşüne ilerler. Yukarıdaki 0.1.38 sonuçları eski disk komutunun tarihsel kapsamıdır; disk modu hâlâ 0x406BF4'te durur. Güncel Run-SAEX allocation sonucunu ayrı evrede raporlar. Native free ve I/O/thread/renderer açıktır.
