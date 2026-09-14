# D1-N2 — Çalıştırılmayan image üzerinde Windows koruma deneyi

13 Eylül 2026; kod 0.1.19'a eşlik eden bağımsız native doğrulama aracı. [Durum](status.md) · [Doğal startup dönüşü](d1-startup-return.md) · [İş akışı](workflow.md).

## Amaç ve kapsam

[image_protection_probe.py](../../tools/native/image_protection_probe.py), kendi derlediğimiz x86 startup fixture'ını **çalıştırmadan** Windows `SEC_IMAGE` görünümüne eşler. `VirtualProtect(..., PAGE_EXECUTE_READWRITE)` isteğinin `VirtualQuery` tarafından hangi korumalarla bildirildiğini ve tek bir özel kopya yazımından sonraki değişimi ölçer. Bu ölçüm, startup observer'ın ilk testindeki `startup_return_protection_mismatch` hatasını açıklamak ve Windows/toolchain değişiminde tekrar sınamak içindir.

GTA başlatılmaz; loader, EXE giriş noktası, TLS veya DLL export'u çağrılmaz. Görünüm mevcut Python sürecine aittir. `LoadLibrary`, başka süreç/PID erişimi, DLL injection veya kaynak oyun dosyası yazımı yoktur. Çalıştırılabilir sayfa izni ile gerçek kod yürütmesi farklıdır. Bu araç GTA frame/ABI kanıtı, güvenlik sandbox'ı veya production launcher değildir; sonuçta `runtimeEligible=false` ve `canAttach=false` kalır.

## Girdi, sahiplik ve akış

CLI yalnız `--configuration Debug|Release` seçer. Girdi yolu depo köküne göre sabittir: `out/windows-x86/<configuration>/saex_startup_return_fixture_normal.exe`. Keyfî EXE, PID, adres veya çıktı yolu alınmaz; symlink/junction ve kök dışı yol kontrolü ortak native araç yordamını kullanır. Önkoşul güvenilir yerel derlemedir; fixture adı veya rapordaki hash bir imza/kaynak güveni kanıtı değildir.

Girdi `GENERIC_READ` ve yalnız `FILE_SHARE_READ` ile açık tutulur; eşleme bitene kadar başka yazma/silme açılışları engellenir. Aynı handle üzerinden en çok 16 MiB okunur ve SHA-256 hesaplanır. Tüketilen PE32/x86 EXE başlık alanları ve en çok 16 MiB, 4096 hizalı image boyutu denetlenir. Bu dar parser bütün PE dizinlerinin denetimi değildir; işletim sisteminin eşleme reddi de hata sayılır.

Akış: dosya kapısı → kendi image görünümü → bütün region'ların ilk taraması → 0x40 isteği ve ilk sayfanın eski koruması → ikinci tarama → varsa bir 0x80 region'ın tek baytını aynı değerle özel kopyaya yazma → üçüncü tarama → aynı dosya handle'ındaki verinin değişmediğini karşılaştırma → görünüm/mapping/dosya handle'larını kapatma. Başarılı JSON yalnız bu kapanışlar da başarılıysa üretilir. Kapanışlardan biri başarısızsa diğerleri yine denenir ve başarı verilmez.

Her tarama aynı allocation içinde committed `MEM_IMAGE`, kesintisiz image kapsamı ve en çok 128 region ister. Koruma bayrakları maskelenmez: değişim sonrası yalnız tam 0x40 veya 0x80 kabul edilir; guard, no-access, execute-only, salt okunur, execute içermeyen yazma ve ek modifier'lar reddedilir. Boyutsuz, ilerlemeyen, arada boşluk bırakan veya image sınırını aşan region reddedilir. Bu izinler yalnız bu deneyin kendi eşlemesi içindir.

## Windows sözleşmesi ve yorum sınırı

Microsoft, `PAGE_EXECUTE_WRITECOPY` (0x80) sayfasına yazmanın özel bir kopya oluşturduğunu ve kopyanın `PAGE_EXECUTE_READWRITE` (0x40) olduğunu belgeler. [Memory Protection Constants](https://learn.microsoft.com/en-us/windows/win32/memory/memory-protection-constants). Özel kopya oluşsa da sorgu türünün `MEM_IMAGE` kalabilmesi ayrıca belgelenmiştir. [VirtualQuery](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualquery).

0x40 isteği sonrası henüz yazılmamış image sayfalarının bu makinede 0x80 görünmesi **yerel deney sonucudur**; bütün Windows sürümleri için aynı region düzeni varsayılmaz. Bütün sayfalar doğrudan 0x40 görünürse sonuç bunu korur, `privateWrite.performed=false` ve `afterPrivateWrite=null` olur; görülmeyen copy-on-write geçişi doğrulanmış sayılmaz. 0x80 görüldüğünde yazılan sayfanın 0x40'a geçmemesi hatadır. Orijinal bayt değeri ve diskteki dosya değişmez.

## Kullanım ve hata sözleşmesi

Standart x86 derleme fixture'ı oluşturduktan sonra:

```powershell
python tools/native/image_protection_probe.py --configuration Debug
python tools/native/image_protection_probe.py --configuration Release
python tests/engine/test_image_protection_probe.py
```

PATH Python'u yoksa [iş akışındaki](workflow.md) gibi gerçek interpreter yolu seçilir. İlk iki komut açık Windows deneyidir; standart build onları çalıştırmaz. Son komutun 17 taşınabilir testi standart Windows build ve Linux CI akışına eklenmiştir. Linux testleri Windows API'si çalıştırmaz; gerçek ölçüm komutu Windows dışında reddedilir.

Başarı exit 0 ve `verified=true`; API/girdi/karşılaştırma/kapanış hatası exit 1 ve `verified=false`; CLI kullanım hatası exit 2. JSON fixture hash/boyut, Windows sürümü, image boyutu, üç fazın RVA/boyut/koruma değerleri, eski koruma, özel yazım ve kapanış sonucunu taşır. Ham process pointer'ı veya ortam içeriği yayımlanmaz. Rapor aktarımı otomatik dosya değiştirmez; stdout tüketicisi exit kodunu da kontrol etmelidir.

## Doğrulama kaydı

17 taşınabilir test geçti: x64/DLL/truncated/oversized PE retleri, region boşluğu/sınırı/ilerlememe/kota, yanlış allocation/state/type, geçersiz korumalar, eski koruma/özel kopya geçişi, girdi değişimi, query/kapanış hatası ve girdi yolunun sabitliği. Test double'ları gerçek Windows kanıtı değildir.

Windows 10.0 build 26200 üzerinde Debug fixture'ın gerçek çalıştırılmayan eşlemesi başarılı: 749568 byte image, istek 0x40 sonrası bütün görünüm 0x80; tek aynı-değer yazımından sonra ilk 4096 byte 0x40, kalan 745472 byte 0x80. Dosya SHA-256 `54fc67af1b0c2859009e534d0447d81d61355b887d06969cec282f593d39a629`, dosya boyutu 730112 byte; girdi korunumu ve üç kapanış başarılı.

Release ölçümü de geçti: 122880 byte image, istek sonrası tamamı 0x80; aynı-değer yazımı sonrası ilk 4096 byte 0x40 ve kalan 118784 byte 0x80. Dosya SHA-256 `43936ece3361b0cf03e9e8f063b36bf077a533ef04fa003f656a1de6051a36a2`, dosya boyutu 97792 byte; girdi korunumu ve üç kapanış başarılı. İki çıktı `out/verification/engine/image-protection-Debug.json` ve `image-protection-Release.json` konumlarında tutulur; yeniden derlemede bu artifact hash'leri değişebilir.

Standart `tools/build.ps1 -Architecture x64 -Configuration Debug` geçti: 7 native suite, 79 managed/entegrasyon testi ve yeni 17 test dahil 115 Python testi. Log `out/verification/engine/build-image-protection-x64-Debug.log`. `tools/check_docs.py --base HEAD` kaynak/belge kontrolü de geçti. Bu görevde x86 build, Linux/hosted CI ve gerçek GTA başlangıç/frame deneyi yeniden çalıştırılmadı; x86 Debug/Release için mevcut fixture çıktıları yalnız bu araçla eşlendi. Başka görevde yürütülen startup observer düzeltmesi ve oyun kanıtları bu aracın başarısı olarak sayılmaz.

Bu ekleme gözlem aracıdır; native C ABI, GNS, otorite, EngineProfile ve production SDK değişmez. Özellik kaldırma veya kalıcı veri migration'ı yoktur. N2/AC-90, N3/AC-91 ve D2 multiplayer bu deneyle kapanmaz.

### Startup kesitiyle birlikte son kontrol

0.1.19 startup kesitinin son kontrolünde Debug/Release image ölçümleri yeniden geçti: `out/verification/engine/image-protection-startup-final-{Debug,Release}.json`. Yukarıdaki hash/boyut/region sonuçları aynı kaldı; iki fixture'ın güncel hash'i, girdi korunumu ve üç kapanış doğrulandı. Aracın 17 portable testi dört Windows standart akışının toplamına dahildir. [Tam akış logları ve toplamlar](d1-startup-return.md#derleme-ve-negatif-test-kanıtı); GTA matrisi ayrı observer kanıtıdır.
