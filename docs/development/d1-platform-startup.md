# D1-N2 — Platform başlangıcı ve sistem ayarı sınırı

Kod 0.1.22 / mimari v0.29; sınırlı platform başlangıç kesiti doğrulandı. [Durum](status.md) · [Önceki uygulama girişi](d1-application-entry.md).

## Sorumluluk, veri akışı ve ayrı izin

`--observe-platform-startup <exe> <absolute-cwd>` önceki doğal uygulama girişinden sonraki incelenmiş prologue'u çalıştırır; ilk OS çağrısının CALL komutundan önce durur. Exact EXE'de 0x748710 → 23-byte prologue → 0x748727 `CALL [0x85826C]` yoludur. Hedef pinned x86 user32.dll içindeki SystemParametersInfoA export'udur. Native gövdeye bu sınırlı ilerleme izni verilir; sistem çağrısına, geri kalan uygulama gövdesine veya pencere/renderer'a izin verilmez. Yığın/EIP/kod/IAT yazımı ve bootstrap export çağrısı yoktur; owned child kapatılır.

İlk çağrının argümanları `SPI_SETFOREGROUNDLOCKTIMEOUT (0x2001), 0, null, SPIF_SENDCHANGE (2)` şeklindedir. İstenen davranış sistem genelindeki foreground lock timeout'u sıfırlamak ve ayar bildirimini göndermektir; sonucu thread'in foreground yetkisine göre başarısız olabilir. Microsoft'un [SystemParametersInfoA sözleşmesi](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-systemparametersinfoa) bu semantiği açıklar; GTA adres kanıtı yerel EXE import/assembly kaydından gelir. Çağrı çalıştırılmadığından ayarın değiştiği veya API'nin başarılı olacağı iddia edilmez. Ayarı geçici değiştirip sonradan geri alma, süreçler arası yarış ve çöküş durumunda geri alamama nedeniyle bu kesitte seçilmedi.

## Arayüzler ve doğrulama

Strict platform policy önceki application policy hash'ine, dolayısıyla exact EXE ve loader pinlerine bağlıdır. C++ `PlatformStartupSpec` ve JSON kaynağı CALL/IAT, tam prologue ve dönüş öneki, 152-byte yığın değişimi ve user32 export'unun 20-byte relocation reçetesini taşır. Tercih edilen base'e göre HIGHLOW operandları tam dört byte olarak güncellenir; karşılaştırmadan byte çıkarılmaz. Ayrı yeni DLL pini yoktur. Unknown fingerprint/spec, body/IAT/export drift veya inactive mapping terminal rettir.

Uygulama girişinde tam prologue, CALL/dönüş ve user32 hedefi okunur; DR0 CALL önüne kurulur. Ana thread DR1 startup IAT watch ve DR2 ikinci ASI koruması korunur. Son durakta aynı byte'lar/aktif mapping ve dört argüman doğrulanır. ESP girişten 152 byte aşağıda, kaydedilen EBX `[ESP+16]` içinde; EBX=0, ESI/EDI/EBP değişmemiş ve TF/DF temiz olmalıdır. `[ESP+23]` yerel bayrağı 1; orijinal return address ve dört uygulama argümanı üst yığında korunmuş olmalıdır. Bu hazırlanan stdcall çerçevesidir, API dönüş ABI kanıtı değildir.

Yeni `platformStartupObservation` izin, continuation, shape/stack/register/argument ve terminal sonucu ayırır; eski modlarda null'dır. `applicationEntryObservation.applicationBodyExecuted` ve CRT içindeki `applicationEntryExecuted`, bu yeni modda CALL durağına ulaşıldığında true, devam verilmemişse false, devamdan sonra doğrulayıcı durağa ulaşılmamışsa null olur. Devam izni tek başına yürütme kanıtı değildir; eski application/CRT komutlarında false korunur. `hostSettingCallExecuted=false`, `canAttach=false`, `initializationVerified=false` korunur. Başarı `platform_startup_boundary_verified` ve exit 3; hata 1/kullanım 2. Bu gözlem bir production sandbox veya genel Windows API filtresi değildir.

## Hata davranışı, genişleme ve kabul

Bozuk spec/parent/relocation, yanlış stack/argüman, değiştirilmiş code/IAT veya export öneki, sınırlı exception/timeout/event/owner ve eski mod sınırı test edilir. Test fixture'ında çağrı kaçırılırsa özel canary çalışır; host ayarı değiştiren gerçek API fixture tarafından çağrılmaz. Gerçek GTA gözlemi özel kopya, aynı retained pinler ve ayrı dosya hash matrisiyle kayıtlanır. Kaldırılan özellik/migration yok; C ABI 1, GNS/HTTPS ve otorite değişmez; C++ observer yeniden derlenir.

## Sonraki yol ve motor etkileri

İlk sistem ayarı çağrısından sonra yerel 0x7468E0 gövdesi CreateEventA/GetLastError ve önceki instance için FindWindowA/SetForegroundWindow yollarını içerir. Daha sonra uygulama olayı 0x619B60, platform başlangıcı 0x7486A0, argüman işleme ve 0x745560 pencere yolu gelir. Bunlar şu anda statik çağrı adaylarıdır; tam gövde/yan etki/ABI doğrulaması yapılmamıştır. Renderer ve tam GTA veri seti önkoşulları ayrıca incelenir.

Sonraki uygulama kesiti için exact çağrıya özgü, oyun süreciyle sınırlı sistem ayarı uyarlaması tasarlanmalı ve dönüş/last-error/stack/fonksiyon kimliği ile çöküş/rollback kanıtı alınmalıdır. Sunucudan adres/patch alma, OS ayarını sessizce değiştirme veya rastgele IAT hook açılmaz. Bu kesit genel API sanallaştırmasını uygulamaz. Ardından instance, dosya/pencere/renderer ve doğal frame kapıları gelir; N2/AC-90 bütünü, N3/AC-91, D1/D2 ve population/hayvan çalışma zamanı açık kalır.

## Nihai kanıt kaydı

Canonical matris `out/verification/engine/platform-gta-evidence.json`; tekil `platform-Debug-{1..6}.json`, `platform-Release-{1..6}.json` ve regresyon kayıtları aynı dizindedir. İlk `platform-explore-Debug-1.json` final tekrar sayısına dahil değildir. Yeni yedi dosyalı özel kopyalar `out/experiments/gta-platform-f01a00ce-{Debug,Release}-v1` içindedir. Önceki oyun kopyaları/kanıtlar ve orijinal kurulum korunur.

| Kanıt | Sonuç |
|---|---|
| Doğal prologue ve CALL önü | Debug 6/6 + Release 6/6; 12/12 |
| Terminal | VA 0x748727, FF15 komutu çalışmadan durdu |
| Çerçeve ve hedef | 12/12 ESP−152, saved EBX/local flag/orijinal giriş frame'i, register'lar ve exact relocated user32 öneki eşleşti |
| API isteği | Her koşuda [8193, 0, 0, 2]; hostSettingCallAllowed/Executed=false |
| Matris | 29/29 beklenen sonuç; oluşturulan 26/26 child çıkışı |
| Girdi korunumu | 145/145: önceki 130 + yeni özel dizinlerin 14 dosyası + platform policy |
| Host ayarı gözlemi | SPI_GETFOREGROUNDLOCKTIMEOUT önce/sonra 2147483647 → 2147483647; SET çağrısı yok |
| Bütçe | Pozitiflerde 58–63 event; lifetime thread değerleri [4]; mevcut 5 saniye/128 event/16 thread sınırları korundu |

On iki eski CLI aynı terminal sınırını ve yeni observation=null sonucunu korudu. Üç child öncesi ret (eksik ASI/cwd/artifact) ve iki child sonrası ret (devre dışı ASI/orijinal codec-bindings pin dışı modül) beklenen sonuçları verdi. Ayrı SPI_GET yalnız mevcut değeri okur; iki ölçümün eşitliği tek başına SET'in çağrılmadığını kanıtlamaz. CALL önündeki hardware breakpoint ve exact instruction/target/frame denetimi asıl yürütme sınırı kanıtıdır. Diğer host ayarlarının tümü ölçülmüş sayılmaz.

| Yerel Windows profili | Native suite | Managed test | Python test | Log (`out/verification/engine/`) |
|---|---:|---:|---:|---|
| x86 Debug | 23 | 79 | 188 | `build-platform-x86-Debug-complete.log` |
| x86 Release | 23 | 79 | 188 | `build-platform-x86-Release-complete.log` |
| x64 Debug | 10 | 79 | 136 | `build-platform-x64-Debug-complete.log` |
| x64 Release | 10 | 79 | 136 | `build-platform-x64-Release-complete.log` |

20 portable spec kontrolü, yedi policy ve iki CLI testi eklendi. x86 corpus 17 senaryo, bir gözlemsiz canary kontrolü ve 12 warm çevrim içerir. Gözlemsiz kontrol gerçek SystemParametersInfoA'ya gitmez; pinned fixture DLL'sindeki canary exit 96 ve marker ile çalıştığını kanıtlar. Gözlemli bütün koşularda canary kapalıdır. Genel hang/loader kotaları önceki application/loader corpuslarında da sınanır; yeni prologue corpusunda exception, event sınırı ve yanlış owner doğrudan sınandı.

Final Debug handle 134 → 134; Release 134 → 134. Ayrıntılar `ctest-platform-x86-{Debug,Release}-complete.log`; son hash/build kaydı `platform-final-verification.json`. Linux/hosted CI/N1 SDK bu kesitte tekrar çalıştırılmadı.

İlk Debug akışı 18 portable kontrol ile geçti; review sırasında sıfır function RVA/base retleri eklenerek 20'ye çıkarıldı. Sonrasında CLI body alanı, devamdan sonra ara hata halinde null olacak şekilde netleştirildi ve x86 akışları son kaynakla yeniden çalıştırıldı. İlk ve ara başarılı loglar korundu; nihai kanıt yukarıdaki complete loglara bağlıdır. Native uyarı/profil denetimi gevşetilmedi. JSON'daki bu üç durum yalnız yeni platform modunu etkiler; eski modların alanları false kalır.

### Kimlikler ve kaynak

| Artifact | SHA-256 |
|---|---|
| Platform policy | `a5c666a8b72a2e4576770419fc801c8210416a08e54440f983f4993195bd35c7` |
| Debug observer EXE | `4ce43e5b29d2adbfa5f9853b1fe821ea826635d07a883f8b624aa7920fe11c66` |
| Release observer EXE | `0552723a2811378e2c70a5113983c445de2d4f01d4cab10b43add2fb387169a8` |

SAEX DLL/map kimlikleri 0.1.21 ile aynıdır; tüm hash'ler canonical matriste bulunur. Yerel x86 user32.dll 1916288 byte, SHA-256 `b3cc6f999873061683021abd4aeea6551f0a56a642c6f90c77744e10488248d9`; SystemParametersInfoA RVA 0xB65B0, preferred base 0x10000000, tek HIGHLOW başlangıcı önekte offset 3'tür. Farklı OS güncellemesi aynı pin kabulü almaz.

Yerel kayıtlar `platform-startup-imports.txt`, `platform-import-bindings.json`, `platform-instance-disassembly.txt`, `platform-user32-export-disassembly.txt` ve önceki `application-entry-disassembly.txt` içindedir. EXE import tablosu IAT 0x85826C'yi USER32.dll/SystemParametersInfoA olarak eşler; export ve relocation incelemesi dosya üzerinden yapıldı, DLL gözlem aracına yüklenmedi. Microsoft API belgesi semantiği, exact binary/read-only disk kayıtları adresi, final runtime matrisi doğal sınıra ulaşmayı destekler. Sistem ayarı uyarlaması, API dönüşü, pencere/renderer, dünya ve doğal frame bu kesitin dışında kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.
