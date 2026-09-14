# D1 — CRT çalışma dizini sorgusu ve kopyalama önkoşulu

Kod 0.1.32 / mimari v0.39. Kod ve dört Windows test akışı tamamlandı; gerçek GTA query doğrulaması OS profil farkı nedeniyle açıktır. [Durum](status.md) · [Önceki kesit](d1-cwd-acquire.md).

## Sorumluluk ve veri akışı

`--observe-cwd-query <exe> <absolute-cwd>` kilit alındıktan sonra doğal wrapper/helper girişini ve drive-zero `GetCurrentDirectoryA(260, localBuffer)` çağrısını gözler. API dönüşünde durur; CRT hata dalları, strcpy, cookie denetiminin dönüş yolu, unlock ve SEH sökümü çalıştırılmaz. Root tamponu değişmez. ANSI sorgu çıktısı kopyalama için kontrol edilir; henüz kopyalanmış durum değildir.

## Arayüz, hata ve genişleme

Yeni komut eski acquire komutunu genişletmez; açık ayrı izindir. Bilinmeyen engine/OS/prefix, yanlış stack/SEH/cookie, izin dışı dal ve sınırlı tampon ihlali ret verir. Deneyin tüm child süreci kapatılır; kilit tutulurken process çıkışı temiz oyun kapanışı değildir. Production attach, IPC, sandbox ve D2 kapıları açılmaz. Sonraki kesit doğal kopya, helper dönüşü, kilit bırakma ve SEH sökümüdür.

### Yedi doğal durak ve ABI

T, acquire terminali 0x836EA4'teki ESP'dir; önceki S=T−12, wrapper EBP=T+44. Gövde adresleri yerel exact GTA executable'ın PE bölümünden ve dumpbin disassembly'sinden çıkarıldı. ASLR/fixture adresleri aynı RVA ilişkileri üzerinden doğrulanır.

| Durak | GTA adresi | ESP | Denetim |
|---|---|---|---|
| Wrapper CALL | 0x836EB1 | T−8 | POP ECX tamam, ECX=7; SEH state=0; drive=0, root pointer, max=128 |
| Helper entry | 0x836DA3 | T−12 | Doğal CALL dönüşü 0x836EB6 |
| Drive dalı | 0x836DBD | T−288 | EBX=0, ZF=1; EBP=T−16; cookie XOR dönüş kaydı |
| Sorgu yolu | 0x836E1E | T−288 | Sıfır-drive dalı doğal seçildi; alternatif drive yordamları çalışmadı |
| Win32 CALL | 0x836E2A | T−296 | 260 ve T−284 local buffer argümanları |
| API entry | admitted kernel32 + export RVA | T−300 | Doğal dönüş adresi 0x836E30, aynı argümanlar |
| API dönüşü | 0x836E30 | T−288 | stdcall cleanup, DWORD uzunluk ve NUL; CRT TEST/COPY henüz çalışmadı |

[CwdQuerySpec](../../include/saex/engine/cwd_query.hpp), [portable kurallar](../../src/engine/loader/cwd_query.cpp), [policy](../../contracts/engine/cwd-query-policy.json) ve [generator](../../tools/cwd_query_policy.py) ayrı aile oluşturur. Parent acquire SHA zinciri korunur. Wrapper 18 byte, helper prologue/dal 28 byte, query yolu 18 byte ve terminal 16 byte denetlenir; atlanan farklı-drive gövdesi çalıştırılmaz. Cookie globali 0x8E31BC'dir; değeri opaque olarak saklanır ve değişmezliği denetlenir.

GTA IAT 0x858244 → kernel32!GetCurrentDirectoryA → kernelbase!GetCurrentDirectoryA bağı, iki admitted/aktif mapping ve export prefix'leriyle doğrulanır. Reviewed kernel32 export RVA=0x30DA0, dolaylı slot RVA=0x81754; kernelbase export RVA=0x256CF0. İlk 20 byte'taki HIGHLOW alanları sırasıyla offset 8 ve 12'dir; tam DWORD yeniden konumlandırılır, karşılaştırmadan byte çıkarılmaz. Windows iç yardımcılarının tamamının denetlendiği iddia edilmez. Fixture bu export reçetelerini kendi host PE dosyalarından çıkarır; GTA CLI eski dosya pinlerini kullanır.

### Tampon, kayıt ve hata sözleşmesi

[Microsoft GetCurrentDirectory sözleşmesi](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcurrentdirectory) doğrultusunda başarılı uzunluk sonlandırıcıyı içermez; sıfır hata, yetersiz tampon gerekli boyutu ifade eder. Bu kesit yalnız ASCII drive-absolute yolları kabul eder. ACP/DBCS/Unicode/UNC uyumluluğu ilan edilmez. Gerçek sonucun byte'ları explicit launch directory ile eşleşir; salt uzunluk eşleşmesi yeterli değildir. Sıfır/taşan uzunluk, erken/eksik NUL ve farklı dizin ayrık ret nedenleridir. 3–126 byte başarılı; 127–259 byte sorgu başarısı olsa da CFileMgr suffix kapasitesi reddidir. CFileMgr'nin 128 byte hedefine sonradan `backslash + NUL` eklenecektir; kesit bu kopyayı veya eki henüz yazmaz.

S'den başlayan 84 byte parent stack'te yalnız drive/root/max/return ve SEH state değişebilir; her word karşılaştırılır. Yeni helper EBP ve EBX saklamaları, 260 byte local buffer'ın üstündeki 8 byte (boşluk/cookie), NT_TIB/FS head, önceki ilk SEH kaydı, 32/156 byte dış caller, 136 byte root/guard, 16 byte localization ve alınmış kritik bölüm/slot korunur. API girişine kadar volatile register'lar da beklenen değerlere bağlıdır; API dönüşünde EAX sonuç, ECX/EDX/aritmetik flags volatile kabul edilir. TF/DF ve kontrol flags değişimi reddedilir. API LastError değeri başarıda sabit varsayılmaz; önce/sonra kaydedilir, API öncesinde değişmezlik aranır. Local buffer'ın NUL sonrası kullanılmayan 260 byte içindeki kısmı API için guard değildir.

Çalışma dizini process çapında değişebilir; bu gözlem immutable genel dosya sistemi veya çok thread'li path güvenliği sağlamaz. Fixture supervisor kendi cwd'sini değiştirmez; yalnız child'a başlangıç dizini verir. Üretim asset/resource dosya erişimi mutlak kimlik/handle sözleşmesini kullanmaya devam eder.

JSON'a `cwdQueryObservation` eklenir; eski modlarda null olur. Yedi checkpoint, API/hedef/buffer adresleri, dönüş uzunluğu, LastError ve korunma bayrakları ayrı görünür. Ham 260-byte local stack içeriği JSON'a dökülmez. `directoryApiAllowed=true` yalnız izin beyanıdır; `functionReturned`, `pathVerified` ve `verified` gözlemdir. Copy/unlock/SEH removal false kalır. Başarı exit 3 / cwd_query_verified; ret 1; kullanım 2. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, GNS, otorite ve kalıcılık biçimleri değişmez. Özellik kaldırma veya veri migration'ı yoktur.

### Windows profili değişikliği ve araç incelemesi

14 Eylül kontrolünde SysWOW64 kernel32/kernelbase/ntdll dosyaları eski loader pinlerinden farklı çıktı. İlk query generator denemesi bu farklı dosyaları parent ile birleştirmeyi `cwd_query_module` ile reddetti. Eski kernel32/kernelbase tam SHA-256 eşleşmeleri WinSxS `10.0.26100.9168` altında bulundu ve yalnız salt okunur export/prefix çıkarımı yapıldı; arşiv DLL'leri yüklenmedi veya sistem dosyaları değiştirilmedi. `cwd-query-static.json` güncel sistem gözlemi, `cwd-query-reviewed-system.json` eski pinle eşleşen statik kaynak kaydıdır. Bilinmeyen OS için yeni GTA yürütme izni verilmez; eski GTA pozitif sonuçları yeni kesite taşınmaz.

Kullanıcı önerisiyle [ReAgent incelendi](../references/reagent.md). Ghidra/bridge kurulumu hazır olmadığı için bu kesitte paket/LLM çalıştırılmadı; mevcut PE/dumpbin yolu kullanıldı. Gelecekteki fonksiyon grubu araştırması için isteğe bağlı rolü ve doğrulama sınırı kaydedildi.

## Kabul ve doğrulama

Hedef Debug build ilk denemede yeni okuyucu adındaki hata nedeniyle C3861 verdi; mevcut `prelude_read_flags` çağrısı düzeltildi. İlk native koşu fixture event adının eşleşmemesini `instance_precondition_shape` ile reddetti; query fixture'ına ayrı isim eklendi. İkinci hedef test koşusunda 583 portable kontrol, 12 native senaryo, bir post-query canary ve 12 warm çevrim geçti; handle 137→137. Yanlış policy/ABI/shape/dizin, eski acquire komutunun kendi terminali, owner thread ve event bütçesi test edildi. Güncel sistem export'ları gerçek Windows API'siyle kullanıldı; bu fixture sonucu GTA runtime sonucu değildir. Tam build/CLI/GTA ret kayıtları aşağıdadır.

### Tamamlanan yerel doğrulama — 14 Eylül 2026

| Windows akışı | Native suite | Managed | Python |
|---|---:|---:|---:|
| x86-Debug | 43 | 79 | 261 |
| x86-Release | 43 | 79 | 261 |
| x64-Debug | 20 | 79 | 190 |
| x64-Release | 20 | 79 | 190 |

Bu dört standart `tools/build.ps1` akışı başarılıdır. Yeni kesitte 583 portable kontrol, 12 x86 senaryo, bir post-query canary ve 12 warm çevrim vardır. Policy için altı Python test metodu, CLI için unknown-input/argüman/null eski alan denetimi eklendi. Release ve Debug fixture kayıtları `cwd-query-x86-<configuration>-native.log` içindedir. Kontrol edilen eski native modlar regresyonları geçti; ayrı N1 SDK, Linux veya hosted CI bu görevde tekrar çalıştırılmadı.

GTA kabul girişinde Debug/Release × query/acquire olmak üzere **4/4 beklenen ret** vardır: `loader_policy_file_mismatch`, ilk farklı modül `advapi32.dll`. Dört denemede childCreated=false, loaderAdvanced=false; bu sayılar GTA API yürütme başarısı değildir. Toplam 17 sistem dosyası eski pinlerden farklıdır. Orijinal executable, özel deney dosyaları, araçlar, policy zinciri ve kullanılan OS girdileri dahil 61/61 hash korunmuştur. Eski DLL'ler yüklenmedi, sistem/oyun dosyaları değiştirilmedi. Gerçek GTA query sonucu **doğrulanmadı**.

Build boyunca native/araç/config girdilerinin son envanteri 293 dosyadır; bütün son hash'ler eşleşir. Native derleme sürerken henüz çalışmamış CLI testindeki Türkçe fixture dizin yazımı UTF-8 olarak eski haline döndürüldü; son envanter ve geçen tam CLI koşusu bu doğru byte'ları kapsar. Özet `cwd-query-final-verification.json`, ret kanıtı `cwd-query-gta-admission.json` dosyalarındadır. Policy SHA-256 `73dcc9c721a519001c25dd0d5905fd16875c7cdd8f857ac28e374d37db9e3167`; kaynak manifest SHA-256 `0e26ec049a4a71e833786137f5d84f88b128124f3a012a9313fdc4f0836c1402`. Build log/artifact kimlikleri makine kaydında tutulur.

Bu kesitin uygulanmış/test edilmiş kapsamı Windows fixture sorgusu ve kopya önkoşuludur. Sırada yeni Windows profilinin yeniden kabulü ve GTA query pozitif kanıtı, ardından doğal kopya/helper dönüşü, unlock ve SEH sökümü vardır. D1-N2 sürer; N3 veya D2 açılmaz.

## 0.1.33 — Windows profil bağı

[26200.9445 incelemesi](d1-system-profile-9445.md), bu bileşenin bağlı olduğu OS pinleri/API reçeteleri ve parent SHA-256 zincirini günceller. Bileşenin GTA durak/izin sınırı aynı kalır; önceki runtime sonuçları eski profil bağlamındadır. Yeni dosyalarla fixture ve gerçek GTA kanıtının kapsamı profil raporunda ayrıca izlenir. Eski OS pinlerine otomatik fallback yoktur; kaynak ve generated policy birlikte yeniden derlenir.

## 0.1.33 gerçek GTA sonucu

Yeni Windows 26200.9445 profilinde gerçek GTA query doğrulandı: **14 pozitif sorgu**, 126/127-byte sınırları, **50/50 matris**, **41/41 child çıkışı**, **134/134 girdi korunumu**. İlk Debug CLI stack overflow heap trace kayıtlarıyla düzeltildi. Dört Windows akışı geçti: x86 Debug/Release 43 native/79 managed/263 Python, x64 Debug/Release 20/79/192. 293 kaynak/config hash'i sabit kaldı. [Profil ve hata raporu](d1-system-profile-9445.md). Copy/helper dönüşü, unlock, SEH sökümü ve N2/N3/D1/D2 açıktır; Linux/hosted/N1 yeniden koşulmadı.

## Sonraki birleşik kesit — 0.1.36

Bu belgedeki terminal ve eski test kayıtları kendi sürümüne aittir. [File-manager-ready](d1-file-manager-ready.md) önceki zinciri tek komutta tamamlar: normal kilit bırakma, SEH sökümü, suffix ve CFileMgr dönüşü. Eski komut otomatik ilerletilmez; buradaki snapshot güncel final durum gibi yorumlanmaz. Genel initializer/renderer/frame ve D1/D2 açıktır.
