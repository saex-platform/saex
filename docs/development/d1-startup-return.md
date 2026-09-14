# D1-N2 — Doğal startup dönüşü

Kod 0.1.19 / mimari v0.26; 13 Eylül 2026. Doğal startup dönüşü özel GTA kopyasında doğrulandı; initialized motor ve doğal frame çağrısı açık. [Durum](status.md) · [Önceki frame adayı](d1-frame-target.md).

## Sorumluluk ve yürütme izni

`--observe-startup-return <exe> <absolute-working-directory>` ayrı deneydir. Exact profil, ASI ve frame reçetesi zinciri doğrulanır; aynı SAEX artifact'ı ASI yolundan yüklenir. Bu mod export çağırmaz; doğal loader yolunu izler. Önceki bootstrap deneyinin değiştirdiği EIP/ESP/FPU durumunu geri yükleyip devam ediyormuş gibi davranmaz. Eski bootstrap/frame/ASI komutları kendi duraklarında kalır. Yeni modda observer yalnız debug register değiştirir; yığın veya kod yazmaz. Yükleyicinin aşağıda tanımlanan EXE bellek koruması değişikliğini yapmasına ayrı izin verir.

## Kaynak ve veri akışı

Exact `vorbisfile.dll` SHA-256 `bdcf32fc3961eebffb4104327ca1396daf1cbd5e736930ed247836035148dafc` disassembly'si: ASI dönüşü RVA 0x1A4A → vector/tarama temizliği → 0x1B4D `CALL [0x400C]` → 0x1B53 dönüş → 0x1BB9 `JMP [0x4028]` → doğal GetStartupInfoA dönüşü. Bu zincir mevcut yerel binary'den incelendi; upstream kaynak benzerliği binary eşitliği sayılmadı. Yedi dosyalı özel deney dizini tek ASI içerir. ASI dönüşünden sonra DR2 aynı callsite'taki ikinci ASI çağrısını, aynı dosya zaten yüklü olsa da, durdurur; DR1 main-thread startup IAT yazımını izlemeyi sürdürür.

Üç yeni durak: VirtualProtect çağrısından **önce**, çağrı **dönüşünde**, sonra GTA'nın daha önce gözlenmiş GetStartupInfoA **dönüş adresinde**. DR0 yalnız bunlara taşınır. Event/byte/module/thread ve toplam beş saniye sınırları korunur; çocuk nihai durakta sonlandırılır. Başka PID veya adres girdisi alınmaz. [Makine reçetesi](../../contracts/engine/startup-return-policy.json) exact parent SHA'larına bağlıdır.

## Arayüz ve doğrulama

`StartupReturnSpec` C++ observer içi deney sözleşmesidir; production ABI değildir. `startupReturnObservation` eski CLI modlarında null; yeni modda stage, protect giriş/dönüş, argümanlar, sayfa denetimi, doğal dönüş ve iki frame örneği sonucu ayrıdır. Yeni `loaderProtectionChangeAllowed=true` yalnız belirtilen loader çağrısının bu owned EXE image'ına uygulanmasını kapsar. C ABI 1, GNS/HTTPS, otorite ve public SDK değişmez; C++ observer yeniden derlenir.

Windows x86 `kernel32.dll` hash/size pini mevcut loader politikasına bağlıdır. Disk export tablosunda VirtualProtect RVA 0x16B30, GetStartupInfoA RVA 0x64260 doğrudan export'tur. Her iki 20-byte prefix aktif pinned mapping'de denetlenir; VirtualProtect önekindeki +8 HIGHLOW tam dört byte normalize edilir. Bu, bütün Windows çağrı zincirini veya düşmanca native kodu doğrulayan bir attestation değildir.

Koruma çağrısında FF15 operandı, wrapper IAT hedefi ve sistem export'u eşleşir. Dört argüman: exact ana image base, exact SizeOfImage, PAGE_EXECUTE_READWRITE (0x40), mevcut thread stack allocation'ında writable 4-byte old-protect çıktısı. Bütün aralık committed nonguard MEM_IMAGE ve aynı EXE allocation içinde olmalıdır; en çok 128 region taranır. Başarılı dönüş nonzero EAX, stdcall ESP+16, ilk sayfanın önceki koruması ve bütün image region'larında exact 0x40 veya 0x80 sonucunu gerektirir. Bu loader davranışı SAEX'in üretim bellek koruma tercihi değildir.

Doğal startup dönüşünde giriş ESP+8, korunması gereken EBX/ESI/EDI/EBP, TF/DF, GTA FF15 çağrı yeri ve STARTUPINFOA.cb=68 kontrol edilir. STARTUPINFO içindeki başlık/desktop pointer'ları ve ortam değerleri yayımlanmaz. Frame adayının CALL/target baytları ASI ve doğal startup dönüşünde karşılaştırılır. Bu, CGame::Process çağrısının çalıştığını veya initialized render/world durumunu kanıtlamaz.

[Microsoft VirtualProtect sözleşmesi](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect): çıktı ilk sayfanın eski korumasıdır; başarı nonzero dönüşle bildirilir ve işlem committed sayfaları kapsar. SAEX bu kuralları ayrı argüman, dönüş ve region denetimleriyle gözler. Başarısızlıkta uzak süreçte GetLastError çağrısı eklenmez.

## Hata davranışı ve kabul

Bozuk spec, parent/OS pin drift, önek/IAT/forward uyuşmazlığı, ek ASI çağrısı, hatalı koruma argümanı, okunamayan bellek, API failure, protection/result farkı, doğal dönüş ABI ihlali, frame drift, beklenmeyen exception, süre veya event kotası terminal rettir. Hangi durak yürütüldüğü korunur; koruma izni verilmiş olması çağrının çalıştığı veya başarılı olduğu anlamına gelmez. Ret sonraki capability'yi açmaz. Başarı exit 3 ve canAttach/initializationVerified=false; hata 1, kullanım 2.

Owned corpus doğal pozitif dönüş, fault/stall, aynı ASI'yi tekrar çağırma, yanlış boyut/flags/pointer, nonvolatile register bozma, frame drift, spec/prefix, owner-thread, event sınırı ve eski modun durma davranışını kapsar. Normal oyunun canary'den sonra ilerlememesi ve warm çevrimde handle korunumu ayrıca sınanır. Gerçek GTA kanıtı fixture sonucundan ayrı kaydedilir.

## Genişleme ve sonraki kapı

N2 startup dönüşü, frame fonksiyonuna doğal call/return kanıtından önceki sınırdır. Sonraki kesit CRT/oyun init yolu ve renderer/asset önkoşulları, ardından doğal CGame::Process çağrısının thread/faz/stack/register gözlemidir. Frame hook, native command drain, process'ten hot unload ve multiplayer kapıları açık kalır. Tam image RWX davranışını üretim launcher'ına taşıma veya kalıcı destek onayı bu deneyden çıkarılamaz.

## Koruma sonucu için Windows image ayrımı

İlk owned koşuda API başarıyla döndü fakat ilk region PAGE_EXECUTE_WRITECOPY (0x80) raporlandığından yalnız 0x40 bekleyen kontrol reddetti. Tanı kaydı `test-startup-return-region-diagnostic.log` içinde region=0/protection=128 olarak korundu. [Microsoft belgesi](https://learn.microsoft.com/en-us/windows/win32/memory/memory-protection-constants), 0x80 mapped image sayfalarının yazıldığında özel kopyaya geçebileceğini açıklar; [VirtualQuery](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualquery) COW sayfalarının image türüyle raporlanabileceğini belirtir. Bu makinede gözlenen 0x40/0x80 sonuçları reçeteye alındı; requestedProtect hâlâ yalnız 0x40, MEM_IMAGE/aynı allocation/committed şartları zorunlu. Flag kombinasyonu, guard veya execute-read 0x20 kabul edilmez. Portable negatifler bu ayrımı sınar; son region alanları ve iki koruma türünün region sayıları JSON'da ayrı kaydedilir. [Çalıştırılmayan fixture deneyi](d1-image-protection.md) bu OS davranışını GTA'dan bağımsız ölçer.

## Gerçek GTA doğrulaması

Canonical kayıt `out/verification/engine/startup-return-gta-evidence.json`; tekil sonuçlar aynı dizindeki `startup-return-Debug-{1..6}.json`, `startup-return-Release-{1..6}.json` ve regresyon dosyalarıdır. Çıktılar yerel kanıttır; oyun/OS binary'leri depoya veya dağıtıma alınmaz. Ön exploratory `startup-return-Debug-first.json` bu 12 tekrara dahil değildir.

| Kontrol | Sonuç |
|---|---|
| Debug / Release doğal dönüş | 6/6 + 6/6; exit 3, `startup_return_verified` |
| GTA dönüş adresi | Her koşuda VA 0x82C11C; bu adresteki komut ilerletilmedi |
| Koruma çağrısı | Dört argüman uygun; dönüş 1; ilk sayfanın eski koruması 2 |
| EXE kapsamı | 18.313.216 byte; 11 region, 6 adet 0x80 ve 5 adet 0x40 |
| Startup ABI | 12/12 ESP ve nonvolatile register eşleşmesi; STARTUPINFOA.cb=68 |
| Frame bayt örnekleri | ASI ve doğal startup dönüşünde toplam 24/24; çağrı yürütme kanıtı değil |
| Regresyon matrisi | 26/26 beklenen sonuç; oluşturulan 23/23 child için çıkış doğrulandı |
| Girdi korunumu | Önce/sonra 100/100 SHA-256 aynı; önceki 84 girdi + yeni iki özel dizindeki 14 dosya + policy + kernel32 |

Dokuz eski gözlem komutu önceki terminal sınırında kaldı ve `startupReturnObservation=null` döndürdü. Eksik ASI, yanlış çalışma dizini ve Debug/Release artifact uyuşmazlığı child oluşturmadan reddedildi. ASI devre dışıyken `asi_candidate_missing`; orijinal kurulumdaki eski gözlemde pin dışı modül `loader_module_not_pinned` verdi. Bu retler başarı olarak yeniden yorumlanmaz; matrisin beklenen negatifleridir.

Pozitif koşularda 50–55 debugger event'i ve dört lifetime thread gözlendi. Her iki frame örneği owned main thread üzerindeydi. Sayfa korumaları yalnız özel kopyadan oluşturulan süreçte loader tarafından değiştirildi. Observer kod/yığın yazmadı, bootstrap export çağırmadı; tüm süreçler nihai durakta kapatıldı. Orijinal GTA kurulumu ve önceki deney dosyaları korundu.

### Kanıta bağlı artifact kimlikleri

| Girdi | SHA-256 |
|---|---|
| Yeni startup-return policy | `57d4d3867122c623e8132d61a0866c56e0055949074c3ea098f54a5fd087aeb7` |
| Debug observer EXE | `7e4c361824953c4966c7dbe39a0b2cd43119396cbae3715f6e4b31e84fc6ce94` |
| Release observer EXE | `797150560cf2e2cc2accff75625ac479de65b0c114581e896c6d8a1cfc60bde1` |
| Debug SAEX DLL / ASI | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Release SAEX DLL / ASI | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Windows x86 kernel32.dll | `4ef63ecfe99158e16a387028be295308b00618cec64d94f016f99b79ffe84e05` |

DLL ve map hash'leri önceki 0.1.18 artifact'larıyla aynıdır; map hash'leri canonical JSON'da ayrıca bulunur. OS export/RVA/relocation incelemesi `startup-return-system-symbols.json` içindedir. Başka executable, Windows güncellemesi veya artifact için bu sonuç otomatik geçerli sayılmaz.

## Derleme ve negatif test kanıtı

Dört standart yerel Windows akışı geçti. Python toplamları bağımsız image koruma aracının 17 taşınabilir testini de içerir; GTA deneyleri standart build sırasında çalıştırılmaz.

| Profil | Native suite | Managed test | Python test | Tam akış kaydı (`out/verification/engine/`) |
|---|---:|---:|---:|---|
| x86 Debug | 17 | 79 | 161 | `build-startup-return-x86-Debug-verified.log` |
| x86 Release | 17 | 79 | 161 | `build-startup-return-x86-Release-verified.log` |
| x64 Debug | 7 | 79 | 115 | `build-image-protection-x64-Debug.log` |
| x64 Release | 7 | 79 | 115 | `build-startup-return-x64-Release-verified.log` |

`native.frame_target` 33 portable kontrol içerir: önceki 20 kontrol + startup spec ve exact koruma bayrakları. `native.startup_return` 18 senaryo ve 12 warm çevrim içerir; son Debug/Release loglarında process handle sayısı 134 → 134 kaldı. Canary kontrolleri startup çağrısından sonraki uygulama koduna ilerlenmediğini, bootstrap export'u veya codec fonksiyonu çağrılmadığını sınar. Tam native çıktılar `ctest-startup-return-x86-{Debug,Release}-verified.log` içindedir. Yeni strict policy generator'ın yedi testi ve CLI'nin iki ek testi standart akışa bağlıdır.

Son kanıt denetimi `startup-return-final-verification.json`: canonical matristen 100 girdinin güncel hash'i, altı observer/DLL/map artifact'ı, dört başarılı build logu ve iki ayrı image ölçümü yeniden karşılaştırıldı. Doküman co-change ve link/şema kontrolü ile `git diff --check` geçti. Linux, hosted CI ve N1 SDK derlemesi bu kesitte yeniden çalıştırılmadı; yerel Windows sonucu bu kapsamları kapatmaz.

### İlk başarısızlıklar ve giderilen nedenler

- Generator ilk denemesinde parent listesinin son elemanı varsayılmıştı; loader policy kaynağı açık indeksinden seçildi. Strict source/parent denetimi korunur.
- `build-startup-return-x86-Debug-first.log`: Windows ULONG ile uintptr_t için `std::min` tür çıkarımı derlenmedi; açık uintptr_t türü verildi.
- `build-startup-return-x86-Debug-second.log`: aynı `auto` bildiriminin iki farklı türü çıkaramaması ayrı bildirimlerle düzeltildi.
- `build-startup-return-x86-Debug-third.log` ve region tanı kaydı: API başarılı olmasına rağmen 0x80 sonucu reddedilmişti; yukarıdaki OS deneyi, exact 0x40/0x80 sözleşmesi ve negatif testlerle giderildi. Ret nedeni gizlenmedi.
- İlk x86 Release tam akışı bütün testlerden sonra, ilk x64 Debug akışı testlerden önce, yeni image koruma aracının eksik doküman eşlemesinde durdu. Owner belgeleri/status/change-log ve test girişleri tamamlandı; tabloda verilen son akışlar ek 17 test ve doküman kapısıyla geçti. Önceki `*-complete.log` kayıtları korunur ve son başarı kaydı yerine kullanılmaz.

Bu kesitte kaldırılan özellik veya veri migration'ı yoktur. N2/AC-90'ın yalnız doğal startup dönüşü alt kapsamı doğrulandı; tam initialized motor, doğal frame çağrısı/AC-91 ve D1/D2 açık kalır.

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.

## 14 Eylül 2026 — Windows fixture taşınabilirliği

0.1.31 kaynak yayınının ilk hosted koşusunda Windows x86 Debug/Release, `startup_return_precondition_shape` ile 13 native grupta durdu; aynı commit temiz yerel Windows checkout'unda geçti. Testler fixture koduna ait alanları değiştirirken kernel32/kernelbase/ntdll alanlarında incelenmiş yerel Windows sürümünün RVA/prefix değerlerini bırakıyordu. [İlk başarısız koşu](https://github.com/saex-platform/saex/actions/runs/34800439726) bu ortam farkını kaydeder.

`tests/engine/system_fixture_support.hpp`, yalnız testler için tutulan sistem DLL dosyalarından adlandırılmış PE32 export RVA'sını, yürütülebilir 20-byte prefix'i ve HIGHLOW başlangıçlarını okur. Export tablo/bütçe/ordinal aralığı, hole/forwarder, relocation türü/blok sınırı ve kısmi/örtüşen fixup'lar doğrulanır. CreateEventA için kernel32 FF25 thunk/IAT slotu ayrıca kontrol edilir; exact prefix karşılaştıran gözlem sözleşmeleri için HIGHLOW düzeltmeleri test sürecinde zaten yüklü sistem modülünün tabanına uygulanır ve adrese bağlı test reçetesi üretilir. Child mapping/hash ve exact byte karşılaştırması korunur; child farklı adreste farklı kod içerirse yine reddedilir. Desteklenmeyen biçim testi durdurur.

13 runtime fixture grubunda startup-return reçetesi; ilgili alt gruplarda GetLastError, CreateEventA ve RtlEnterCriticalSection alanları bu test verisinden kurulur. Yanlış RVA/prefix senaryoları, child canary'leri, handle tekrarları ve gözlemci kontrolleri korunur. `native.startup_return`, beş prefix'i zaten yüklü modüllerin belleğiyle ASLR normalizasyonundan sonra karşılaştırır ve özel byte kopyalarında on bozuk PE metadata senaryosunu reddeder. Yardımcı DLL yüklemez, disk/system belleğine yazmaz. İlk yerel yardımcı koşusu ntdll'nin sıfır giriş noktasını EXE parser'ına verdiği için reddedildi. Yalnız ntdll metadata okumasında açıkça seçilen sıfır giriş modu, özel parse kopyasında yürütülebilir bölüm başlangıcını geçici kullanır ve sonuçta orijinal sıfır RVA'yı geri koyar; EXE kontrolleri ve diskteki DLL aynı kalır.

Üretim loader/entry/startup-return ve diğer canonical JSON/generated policy'leri bu düzeltmede değişmez. Gerçek GTA için yeni Windows sürümü onaylanmış sayılmaz; yeni GTA koşusu yapılmamıştır. Tam yayın ve kontrol sonucu [GitHub yayın raporunda](github-publication.md) tutulur.

İkinci hosted koşu, `system unrelocated export contract` ile sunucu DLL prefix'indeki HIGHLOW kaydını yakaladı. `system_absolute_export` ile yukarıdaki adres normalizasyonu eklendi; VirtualProtect üzerinde normalleştirilmiş reçete eşitliği ve geçersiz relocation maskesi de sınanır. Yerel ilk düzeltmenin tam akışı 41 native/79 managed/254 Python ile geçmişti; hosted ret bu yerel sonucun sunucu kanıtı olmadığını gösterir.

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
