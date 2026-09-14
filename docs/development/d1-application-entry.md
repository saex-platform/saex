# D1-N2 — Doğal uygulama giriş sınırı

Kod 0.1.21 / mimari v0.28; sınırlı doğal giriş alt kesiti doğrulandı. [Durum](status.md) · [CRT sınırı](d1-crt-startup.md).

## Sorumluluk ve yürütme izni

`--observe-application-entry <exe> <absolute-cwd>` ayrı opt-in deneydir. Önceki CRT sınırından sonra seçilmiş EXE'nin statik başlatıcı yolu doğal olarak çalıştırılır. Başlatıcı dönüşü, ikinci GetStartupInfoA giriş/dönüşü, uygulama CALL yeri ve hedefin ilk komutu ayrı duraklardır. Hedefe CPU'nun doğal CALL ile girişi gözlenir; ilk komut yürütülmeden owned child sonlandırılır. Observer yalnız debug register değiştirir; yığın/EIP veya kod yazmaz ve SAEX bootstrap export'u çağırmaz. Eski CLI modları önceki terminal sınırlarında kalır.

Başlatıcıların native gövdelerine bu özel süreçte çalışma izni verilir. Bu izin başlatıcıları güvenli, salt okunur veya yan etkisiz yapmaz. EXE/wrapper/artifact güveni varsayılır; bu araç sandbox değildir. Yeni pin dışı DLL, beklenmeyen exception, ASI tekrar çağrısı veya süre/event/thread sınırı terminal rettir; bunlar otomatik allowlist güncellemesine yol açmaz. Pencere/renderer/gameplay için ek izin verilmez; uygulama gövdesine gelmeden durulur.

## Veri akışı ve kaynak

Exact EXE 0x8246AC CALL → 0x823B76 başlatıcı gövdesi, 0x8246B1 dönüşü; 0x8246C6 ikinci GetStartupInfoA CALL, 0x8246CC dönüş; 0x8246EC CALL → 0x748710 uygulama giriş adayı. Reçete önceki CRT policy hash'ine bağlıdır. 101-byte başlatıcı dispatcher gövdesi tamamıyla karşılaştırılır; 1676 slotun 1674 nonzero hedefi önceki bounded tablo sözleşmesinden gelir. Toplu fonksiyon dönüşü her callback'in gövdesi için ayrı ABI/yan etki kanıtı sayılmaz.

Yerel wrapper'ın RVA 0x1BA0 girişinde RVA 0x6430 bayrağı nonzero ise tarama atlanır; ilk tarama sonunda bayrak 1 olur ve orijinal GetStartupInfoA'ya JMP yapılır. 31 byte'ın tamamı karşılaştırılır; üç absolute operand (offset 5/19/27) gerçek wrapper base + incelenen slot RVA ile doldurulur. Bu alanlar karşılaştırmadan çıkarılmaz. Her yeni durakta bayrak tam 1 olmalıdır. İkinci ASI callsite koruması ve main-thread startup IAT write watch korunur.

Yerel statik örnekler FPU hazırlığı ve global float değer hesaplamaları içerir. Bütün callback'ler için eksiksiz statik çağrı grafiği çıkarıldığı iddia edilmez; dinamik modül/exception geçmişi runtime gözlemine dayanır. Disk örnekleri ile başarılı runtime sonuçları ayrı kayıtlanır.

## Arayüz ve doğrulama

`ApplicationEntrySpec` observer içi C++ sözleşmesidir; C ABI 1/SDK değişmez. Generator strict schema/parent/byte/range/overlap/guard denetimi yapar; portable spec aynı sınırları doğrular. Başlatıcı girişindeki ESP ve nonvolatile register'lar kaydedilir. Dönüşte EAX=0, aynı ESP, EBX/ESI/EDI/EBP ve TF/DF kontrol edilir. İkinci startup argümanı yığınla aynı allocation içindeki hizalı, en az 68 byte okunabilir committed private stack verisi olmalı; dönüşte CALL öncesine göre stdcall ESP+4, nonvolatile register'lar ve cb=68 doğrulanır. Startup flags/show değerlerinden beklenen nCmdShow hesaplanır; bu değer observer tarafından değiştirilmez.

Uygulama CALL önündeki ESP, başlatıcı CALL öncesindeki ESP−16 olmalıdır. Uygulama çağrısında dört argüman denetlenir: hInstance=EXE base, hPrevInstance=0, en çok 32768 byte içinde sonlanan private command-line buffer, nCmdShow=ikinci startup sonucu. Komut satırı içeriği veya pointer'ı JSON'a yazılmaz; yalnız uzunluk raporlanır. Hedefin ilk komutunda ESP=CALL öncesi ESP−4, return address=CALL+5, aynı dört argüman ve aynı nonvolatile register'lar aranır. Bu çağrı giriş kanıtıdır; fonksiyon dönüşü veya initialized oyun dünyası kanıtı değildir.

Yeni `applicationEntryObservation` izin, ilerleme, durak, başlatıcı sonucu, ikinci startup ABI, argümanlar ve giriş doğrulamasını ayırır. Eski modlarda null'dır. `crtStartupObservation` önceki üç örneği korur; ek uygulama örnekleri eski sayacı artırmaz. Bu üst modda `initializerExecutionAllowed=true`, eski CRT komutunda false kalır. `applicationEntryExecuted=false` ilk komutun yürütülmediğini belirtir; yeni `entryReached=true` doğal CALL ile hedefe ulaşmayı gösterir. Başarı `application_entry_verified`, exit 3; `canAttach=false`, `initializationVerified=false`, `applicationBodyExecuted=false`. Hata 1, kullanım 2. CLR/C# sandbox, GNS veya asset aktivasyonu yoktur.

## Hata davranışı, genişleme ve kabul

Spec/parent/body/guard/table drift, başarısız başlatıcı, yığın/register bozulması, hatalı ikinci argüman, show/instance/command-line uyuşmazlığı, exception/hang ve kaynak kotası güvenli rettir. Owned negatif corpus ilk uygulama komutunun ve sonraki kodun çalışmadığını canary ile doğrular. Eski CRT komutu başlatıcıları çalıştırmamalıdır. Sıcak çevrimlerde handle korunumu ve süreç çıkışı ayrıca ölçülür.

Sonraki aşama uygulama gövdesinin sistem/pencere/renderer ve asset önkoşullarıdır. Yedi dosyalı özel kopya tam GTA verisi değildir. Doğal frame, pool/collision, SDK command drain, N2/AC-90 bütünü, N3/AC-91 ve D1/D2 açık kalır. Kaldırılan özellik veya kalıcı migration yok; C++ observer yeniden derlenir.

## Deney sırasında giderilen sorunlar

İlk fixture derlemesinde iki inline assembly komutunun aynı satıra yazılması MSVC sözdizimi hatası verdi; komutlar ayrıldı. Negatif EAX fixture'ındaki unreachable return, uyarıları kapatmadan koşullu dönüşle düzeltildi. Eski proxy fixture'ının dosya adıyla seçtiği frame/register bozma davranışı yeni negatifleri CRT sınırından önce tetikledi; application fixture bu eski seçim yolunu kullanmaz, bozulma EXE initializer gövdesinde yapılır. İlk build/CTest kayıtları `build-application-x86-Debug-first.log`, `build-application-fixtures-Debug*.log`, `ctest-application-Debug-{first,2,3,4}.log` içinde korunur.

İlk iki gerçek Debug giriş denemesinde observer `0xC00000FD` ile çıktı ve JSON üretemedi. CLI'deki büyük `LoaderTrace` sonuçlarını seçen zincir, MSVC /Od altında her dal için geçici değer ayırıyordu; tek dönüş slotlu dispatcher ile düzeltildi. Yığın rezervi artırılmadı ve istisna bastırılmadı. `application-explore-Debug-2-result.json` hata kodunu; `application-explore-Debug-3.json` düzeltilmiş doğal girişi kaydeder. Bu keşif koşuları aşağıdaki nihai tekrar matrisine dahil edilmez. Araç çöküşü başarılı child/initialization kanıtı sayılmaz.

## Kabul akışı ve sonraki kapı

| Durak | Başarı koşulu | Ret örneği |
|---|---|---|
| CRT initializer CALL önü | Aynı policy zinciri, tam dispatcher/reentry, once=1 | Bozuk spec veya prefix |
| Initializer CALL+5 | EAX=0, aynı ESP ve nonvolatile register'lar | Hata dönüşü, table/once/frame drift |
| İkinci startup CALL önü | FF15 aynı IAT, stack allocation içindeki 68-byte argüman | Null/yanlış pointer |
| İkinci startup CALL+6 | ESP+4, nonvolatile register ve cb=68 | ABI veya yapı uyuşmazlığı |
| Uygulama CALL önü | Dört argüman ve bounded NUL sonlandırıcı | Yanlış instance/show veya komut satırı |
| Uygulama hedefinin ilk komutu | ESP−4, CALL+5 dönüş slotu ve aynı argümanlar | Yığın veya hedef drift |

Önceki bütün kapılar ve kotalar korunur. Debug register koruması ana thread ile sınırlıdır; native initializer'lar için kötü niyetli kod yalıtımı değildir. Doğal app CALL gözlemi N2'nin bir alt çıktısıdır. Bir sonraki kesit uygulama gövdesinin dosya/sistem/pencere/renderer önkoşullarını exact build üzerinde inceleyerek yeni bir sınırlı doğal durak tanımlamalıdır. Tam veri seti ve motor fazı doğrulanmadan frame hook, entity havuzu veya population/hayvan sistemi çalışıyor ilan edilmez.

## Nihai doğrulama kaydı

Tam matris `out/verification/engine/application-gta-evidence.json`; tekil `application-Debug-{1..6}.json`, `application-Release-{1..6}.json` ve regresyon dosyaları aynı dizindedir. Yeni özel kopyalar `out/experiments/gta-application-f01a00ce-{Debug,Release}-v1` altında yedi dosya içerir; orijinal oyun, eski kopyalar ve kanıt dosyaları korunur.

| Kanıt | Sonuç |
|---|---|
| Gerçek doğal giriş | Debug 6/6 + Release 6/6, toplam 12/12 |
| Terminal konum | VA 0x748710; ilk komut çalışmadan durdu |
| Başlatıcı dönüşü | 12/12 EAX=0, aynı ESP ve nonvolatile register'lar; tekil callback sayımı/ABI kanıtı değil |
| İkinci startup | 12/12 doğal CALL/RET, cb=68 ve ESP+4; wrapper once=1 |
| Uygulama çerçevesi | 12/12 doğru dört argüman ve CALL+5 dönüş slotu; bu launch context'te nCmdShow=0, argüman metni uzunluğu 0 |
| Regresyon ve retler | On bir eski CLI durağı + eksik ASI/yanlış cwd/yanlış artifact/devre dışı ASI/orijinal eski mod: 16/16 beklenen sonuç |
| Toplam | 28/28 matris, oluşturulan 25/25 child çıkışı |
| Girdi korunumu | 130/130: önceki 115 + yeni dizinlerin 14 dosyası + application policy |
| Bütçe | Pozitiflerde 55–62 event, dört lifetime thread; mevcut 5 saniye/128 event/16 thread sınırları korundu |

Eski CRT komutunda initializer izni false ve yeni observation null kaldı. Üç child öncesi ret process oluşturmadı; ASI devre dışı ve orijinal codec-bindings negatifleri oluşturulan child'ı kapattı. Yeni OS/modül pini, EIP/yığın/kod yazımı veya bootstrap export çağrısı eklenmedi. Bu çalışmadaki nCmdShow=0 yalnız gizli deney launch context'inin ölçümüdür; sözleşme değeri sabit 0'a zorlamaz.

| Yerel Windows profili | Native suite | Managed test | Python test | Log (`out/verification/engine/`) |
|---|---:|---:|---:|---|
| x86 Debug | 21 | 79 | 179 | `build-application-x86-Debug-final.log` |
| x86 Release | 21 | 79 | 179 | `build-application-x86-Release-final.log` |
| x64 Debug | 9 | 79 | 129 | `build-application-x64-Debug-final.log` |
| x64 Release | 9 | 79 | 129 | `build-application-x64-Release-final.log` |

`native.application_entry_spec` 24 kontrol; `native.application_entry` 23 senaryo + 12 warm çevrim; yedi policy ve iki CLI testi eklendi. Debug ayrıntısı `ctest-application-Debug-4.log` (136 → 136 handle), final Release ayrıntısı `ctest-application-x86-Release-final.log` (141 → 141) içindedir. Final standard Debug akışı da aynı corpus'u geçti. Her iki profilin kendi başlangıç/son handle sayısı karşılaştırılır; profiller arası sayının eşit olması gerekmez. Linux/hosted CI/N1 SDK yeniden koşulmadı. Son hash/build denetimi `application-final-verification.json` içindedir.

### Exact kimlik ve yerel kaynak

| Artifact | SHA-256 |
|---|---|
| Application policy | `d683a244ece6bcfeffcb78f461daeb1232b6fd7a33c6e7f7055fdbdb8bd5bca9` |
| Debug observer EXE | `658403339a4105fa7ee47ef1a67bd9e64f5d4b8893688f5632e3e2937b9d3bc4` |
| Release observer EXE | `dca91fa7f8f4a5ee2398bd493675b2a3e1e40bc4423337097c8379a65e7a5906` |

SAEX DLL/map kimlikleri 0.1.20 ile aynıdır; bütün değerler canonical matriste tutulur. Kaynak EXE SHA-256 `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`; yerel wrapper `bdcf32fc3961eebffb4104327ca1396daf1cbd5e736930ed247836035148dafc`. Bu adresler yalnız bu exact gözlem profili için geçerlidir.

Yerel dumpbin kayıtları: `application-wrapper-reentry-disassembly.txt`, `application-fp-disassembly.txt`, `application-constructors-sample.txt`, önceki `crt-initializers-disassembly.txt` ve `application-entry-disassembly.txt`. Wrapper'ın 31-byte guard gövdesi ve initializer'ın 101-byte dispatcher gövdesi kaynak reçeteyi destekler; runtime eşitlik ve doğal geçiş ayrıca yukarıdaki matrisle doğrulanır. İlk komuttan sonraki uygulama yürütmesi, frame ABI, renderer ve dünya hazırlığı bu sonucun dışında kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.

## Kod 0.1.24 bağlantısı

Ayrı instance-startup modu platform bastırma dönüşünden gerçek named event oluşturma/açma ve doğal helper dönüşüne ilerler. Mevcut event veya NULL handle durumunda pencere kolundan önce ret verilir. Önceki suppression modu restore ederek bitmeye devam eder; yeni mod doğal API sonrası eski CALL bağlamını geri yazmaz. [Sözleşme ve doğrulama](d1-instance-startup.md). Oturumdaki ortak event ömrü process-private değildir; observer sinyal durumunu değiştirmez. C ABI 1/GNS/otorite aynı; pencere/renderer/doğal frame ve N2/N3/D1/D2 kapıları açıktır.

## Kod 0.1.25 bağlantısı

Ayrı event-dispatch modu, instance dönüşünden doğal olay dağıtıcısı CALL/entry ve uygulama işleyicisi CALL önüne ilerler. Üç durakta argüman, dönüş adresi, register ve yaşayan caller stack doğrulanır; uygulama işleyicisi çalıştırılmaz. [Sözleşme ve sonuç](d1-event-dispatch.md). Eski instance terminali, C ABI 1/GNS/otorite aynı; yeni bağımlılık/kalıcı migration yoktur. AppEventHandler gövdesindeki executable yönlendirmesi, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.26 bağlantısı

`--observe-application-routing` önceki event-dispatch kanıtından sonra yalnız rsINITIALIZE=24 rotasını yürütür: işleyici entry → executable detour → indirect JMP → ilk oyun initializer CALL öncesi. 39 index/11 hedef tablosu, rel32/absolute operand ve dört yığın/register sınırı doğrulanır. [Sözleşme ve sonuç](d1-application-routing.md). Önceki mod kendi AppEventHandler CALL öncesi terminalini korur. Yeni modda `eventDispatchObservation.applicationHandlerCallAllowed=true`, routing nesnesinde initializer çağrı izni false olur; önceki stage/verified ara kanıtı korunur. C++ trace/API yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni bağımlılık/kaldırılan özellik/kalıcı migration yoktur. Oyun initializer gövdesi, RsInitialize, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.27 bağlantısı

`--observe-game-prelude` ilk oyun initializer içine girer; exact boş Init ve üç yerelleştirme bayrağını yazan iki helper doğal olarak geri döner. Beş durak, stack/register/flags, yaşayan caller ve 16-byte veri penceresi denetlenir; yalnız üç veri byte değişebilir. CFileMgr CALL çalıştırılmaz. [Sözleşme ve sonuç](d1-game-prelude.md). Önceki application-routing terminali korunur; yeni üst modda routing nesnesinin initializerCallAllowed alanı true, prelude nesnesinin fileManagerCallAllowed ve initializerReturnVerified alanları false olur. C++ observer yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni dependency/kaldırma/kalıcı migration yoktur. CFileMgr, streaming/pad, initializer dönüşü, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.28 bağlantısı

`--observe-file-manager-entry` CFileMgr içine doğal CALL ve ilk üç PUSH komutunu açar; 0x5386FB CRT cwd CALL önünde durur. İki durakta buffer/maxlen=128 ABI, nested return stack, register/flags, 136-byte root/guard ve localisation korunumu denetlenir. [Sözleşme ve sonuç](d1-file-manager-entry.md). Önceki prelude terminali korunur; yeni üst modda prelude fileManagerCallAllowed=true, manager cwdCallAllowed=false/fileManagerReturnVerified=false olur. CRT lock/SEH/OS/copy yolu henüz açılmaz. Gelecekte suffix yazımından önce NUL en geç buffer offset 126, başarılı dönüş ve ANSI byte uzunluğu kanıtı gerekir. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Manager/initializer dönüşü, streaming/pad, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.29 bağlantısı

`--observe-cwd-seh` CRT wrapper ve SEH prologue içine doğal CALL açar; kayıt kurulup yardımcı döndüğünde 0x836E9D noktasında durur. Üç durakta 80-byte stack, 28-byte NT_TIB, önceki kayıt ve caller/buffer/localisation korunumu denetlenir. [Sözleşme ve sonuç](d1-cwd-seh.md). Önceki manager terminali korunur; yeni üst modda manager cwdCallAllowed=true, cwdSeh lockPathAllowed/directoryApiAllowed/cwdReturnVerified/unwindVerified=false olur. Handler veya kilit/OS/copy yolu açılmaz; owned child sonunda kapatılır, eski TEB/context rollback yapılmaz. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Kilit/cwd, SEH sökümü, manager/initializer dönüşü, renderer/doğal frame ve N2/N3/D1/D2 açıktır.

## Kod 0.1.30 bağlantısı

`--observe-cwd-lock` lock(7) selector CALL ve ilk 17-byte gövdeyi doğal yürütür; CMP tamamlandığında 0x82ADCF JNE önünde durur. [Sözleşme ve sonuç](d1-cwd-lock.md). 100-byte stack, 16-byte slot penceresi, NT_TIB/önceki kayıt/caller/buffer korunur; slot değeri dereference edilmez. SlotPresent yalnız sıfırdan farklı word demektir, kritik bölüm veya kilit alma kanıtı değildir. Üst modda cwdSeh.lockPathAllowed=true yalnız selector iznidir; yeni branchAllowed/lazyInitializationAllowed/criticalSectionCallAllowed/lockAcquiredVerified=false. Önceki SEH terminali korunur; DR0 dışında yeni observer müdahalesi ve TEB/context rollback yoktur. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Mevcut/lazy dal, OS kilidi, cwd/SEH dönüşü ve N2/N3/D1/D2 açıktır.

## Kod 0.1.31 bağlantısı

`--observe-cwd-acquire` mevcut/unowned lock(7) nesnesi için doğal dal, admitted ntdll API entry/return ve CRT selector dönüşünü açar; 0x836EA4 terminalinde durur. [Sözleşme ve sonuç](d1-cwd-acquire.md). Beş durakta object/slot/84-byte caller/SEH korunumu ve API sonrası thread sahipliği doğrulanır. x86 24-byte kritik bölüm düzeni pinned Windows uygulamasına aittir; VOID dönüşte EAX başarı kodu sayılmaz. Heap veya aynı GTA image nesnesi için sınır/koruma denetimi vardır. Üst modda branch/criticalSectionCallAllowed=true, acquired readback ile ayrıdır; lazy/directory/unlock kapalı kalır. Önceki lock terminali korunur; kilit tutulurken bütün owned child kapatılır, observer veri/TEB/context rollback yapmaz. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Cwd/SEH/manager dönüşü ve N2/N3/D1/D2 açıktır.

## 14 Eylül 2026 — Windows fixture taşınabilirliği

Startup-return ve devamındaki fixture zinciri, sistem DLL reçetelerini test makinesinin diskte tutulan PE dosyalarından çıkarır. Ortak okuyucu ve negatif doğrulamalar [test kapsamı notunda](d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği) açıklanır; üretim policy/hash kuralları ve bu belgedeki gerçek GTA kanıtının sınırları aynıdır.

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
