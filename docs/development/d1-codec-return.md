# D1-N2 — Dinamik codec dönüş sınırı

Kod **0.1.14**, mimari **v0.21**, 13 Eylül 2026, ADR-49. [Durum](status.md) · [Önceki startup durağı](d1-startup-call.md) · [Native sözleşme](../architecture/native-sdk-integration.md).

## Sorumluluk ve izin

Bu geliştirme deneyi, incelenmiş startup wrapper gövdesindeki ilk LoadLibraryA çağrısını ilerletir. Çağrıdan sonraki ilk MOV EDI,EAX talimatı çalışmadan ana thread tutulur; EAX dönüş değeri ve yeni codec mapping kimlikleri karşılaştırılır. GetProcAddress ile codec export bağlama, ASI taraması, genel LoadPlugins dönüşü veya gerçek SAEX bootstrap yüklemesi bu kesitte yürütülmez. CanAttach/initializationVerified false; N2/D1/D2 açık kalır.

run_to_codec_return ve --observe-codec-return ayrı açık izinlerdir. Eski loader/context/entry/proxy/startup komutları önceki durak ve pin listelerini korur. Codec modu startup→proxy→entry→loader source digest zincirine bağlıdır; yalnız bu mod 23 yerine **26 retained pin** hazırlar. Public C++ observer trace/API değiştiğinden local çağıranlar birlikte derlenir. Bootstrap C ABI 1, engine profili/anchor'lar, SDK lock, GNS/HTTPS ve otorite kararı değişmez. Kaldırılan ürün özelliği veya kalıcı state migration'ı yoktur.

## İncelenmiş kaynak ve arayüz

[Codec policy](../../contracts/engine/codec-policy.json) exact startup digest'ine bağlıdır. [Compiler](../../tools/codec_policy.py) altı kaynağın her birini 64 KiB ile sınırlar; duplicate/unknown key, type, schema/stage/id, review document, RVA aralığı/hizalama, closure isim/sıra/boyut/hash ve parent drift retleri vardır. Bu recipe yalnız vorbishooked.dll→vorbis.dll→ogg.dll üçlüsünü kabul eder; server JSON'u, wildcard veya otomatik DLL izni yoktur. Header generator ile üretilir; Windows/Linux akışları --check çalıştırır.

| Ek game-root pin | Byte | SHA-256 |
|---|---|---|
| vorbishooked.dll | 65536 | a08923479000cec366967fb8259e0920b7aa18859722c7dda1415726bed4774f |
| vorbis.dll | 1060864 | fefda850b69e007fceba644483c7616bc07e9f177fc634fb74e114f0d15b0db0 |
| ogg.dll | 36864 | 4a4f65427e016b3c5ae0d2517a69db5f1cdc7a43d2c0a7957e8da5d6f378f063 |

İsimler işletim sistemi DLL araması için bir öneri değildir: CLI, EXE'nin canonical dizinindeki exact dosyaları child öncesinde aynı handle üzerinden boyut/hash/file ID ile pinler. Eksik/bozuk pin child öncesi reddedilir. Native wrapper'ın mevcut isimle arama davranışı değişmez; LOAD_DLL olayındaki gerçek dosya kimliği ayrıca bu pinlerle eşleşmelidir. İmza veya dosya ismi tek başına izin değildir.

Yerel vorbisfile.dll hash'i proxy-policy ile korunur. MSVC dumpbin disassembly, wrapper RVA **0x14AD** PUSH imm32, **0x14B2** FF15 abs32 ve **0x14B8** MOV EDI,EAX sırasını gösterir. Literal RVA **0x41C4** `vorbishooked\0`, LoadLibraryA import slot RVA **0x4020**. Bu adresler tahmin edilmiş motor fonksiyonları değildir; yalnız exact dosya ve ASLR base'iyle kullanılan laboratuvar recipe'sidir. [ASI-Loader kaynak akışı](https://github.com/GTAmodding/ASI-Loader/blob/main/vorbisFile.cpp) karşılaştırma kaynağıdır; yerel binary ile aynı build olduğu ileri sürülmez. Önceki [statik linkage raporu](d1-native-linkage.md) üç codec dosyasının import/export ilişkilerini ayrı tutar.

CodecStopSpec, 1–3 retained module pointer span'ı taşır; ilk öğe beklenen dönüş modülüdür. Pointer'ların pins içinde olması, farklı file ID taşıması ve proxy olmaması gerekir. requested_name yalnız 1–15 ASCII küçük harf/rakam/alt çizgi/tire kabul eder; NUL ve yol karakterleri reddedilir. Span/pin ömrünü trusted local çağıran korur. Bu genel DLL yükleme API'si değildir.

## Veri akışı ve dördüncü durak

```text
exact engine + 26 pin + explicit cwd/environment
  → önceki entry / proxy-return / startup-call doğrulamaları
  → startup hit: codec üçlüsü henüz aktif olmamalı
  → 13 byte call/return şekli, literal, OS target kontrolü
  → DR0 = proxy base + sequenceRva + 11; DR1 aynı EXE IAT write watch
  → startup gövdesi ve LoadLibrary çağrısı ilerler
  → her yeni LOAD_DLL: same-file identity + lifetime budget + mapping ledger
  → return hit: EAX ve exact active/fresh module mapping'leri
  → MOV EDI,EAX / export bağlama / ASI taraması öncesi kill + confirmed exit
```

Üçüncü durakta 13 executable MEM_IMAGE byte'ın PUSH literal-address / FF15 slot-address / MOV EDI,EAX şekli ASLR operandlarıyla karşılaştırılır. NUL dahil literal en fazla 16 byte okunur. Slot pointer'ı aktif, pinlenmiş kernel32/kernelbase executable MEM_IMAGE bölgesinde bulunmalıdır. Aynı slot pointer'ı ve 13 byte dördüncü durakta korunmuş olmalıdır. Slotun LoadLibraryA adı exact dosya import incelemesine dayanır; runtime kontrol yalnız OS bölgesi/kararlılık kontrolüdür, tüm OS fonksiyon gövdesinin veya müdahale edilmemiş IAT'nin ispatı değildir.

DR0/DR1/DR7 Set/Get readback ve main-thread, first-chance EXCEPTION_SINGLE_STEP, EIP/adres, yalnız DR6 B0 kimliği gerekir. Main-thread EXE IAT yazımı önceki DR1/B1 terminal ret olarak kalır; aynı değerin yazımı da kapsanır. Bu watch yazımdan sonra tetiklenir; başka thread'lerin yazımlarını önlemez. Observer instruction/IAT/EIP/stack değiştirmez. Dördüncü hit için CONTEXT_INTEGER ile EAX okunur.

EAX sıfırsa codec_load_failed. Her beklenen codec modülü aynı file ID/hash/boyut ile aktif ve tek mapping olmalı, LOAD event'i codec_arm_event sonrasında gerçekleşmelidir. Önceden yüklenmiş modül başlangıçta codec_module_preloaded ile reddedilir; bu deney yeniden LoadLibrary çağırıp referans sayısı artırma senaryosunu doğrulamaz. İlk modül base'i EAX ile eşit değilse codec_handle_mismatch. Eksik/ambiguous/eski mapping veya shape drift terminaldir. Partial mapping ID'ler başarısız raporda bulunabilir; modulesVerified false iken başarı olarak yorumlanmaz.

[Microsoft LoadLibraryA sözleşmesi](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya), yeni DLL için başarılı PROCESS_ATTACH sonrası handle dönüşünü tanımlar. Bu dar gözlem DLL yükleme çağrısının dönüşüdür; codec decode fonksiyonları, ses sistemi, GetStartupInfoA'nın tamamlanması, GTA WinMain/frame veya tam unpack kanıtı değildir. Dört image örneği önceki startup durağında alınır, codec sonrası tekrar okunmuş sayılmaz.

## Hata, bütçe ve güven sınırı

Mevcut 128 debug event, 64 lifetime module, 16 lifetime thread, 256 MiB kümülatif file okuma ve beş saniye gözlem bütçesi değişmez. Bilinen unload mapping'i emekliye ayırır; yeniden load pin denetimi/bütçe harcaması gerektirir. Proxy unload terminaldir. DllMain FALSE ile load başarısızlığı, initializer exception/stall, bilinmeyen DLL veya beklenmedik olay ilerletilmez. I/O çağrıları hard wall-clock deadline değildir. Her bitiş owned child'ı pending talimat devamından önce kapatır; exit teyidi alınır. Foreign-owner çağrı owner'ın cleanup/state'ini değiştiremez; terminal child tekrar çalıştırılamaz.

İzinli native kod bu deneyde çalışır; exact hash güvenli kod veya OS sandbox kanıtı değildir. DllMain içindeki başka yan etkiler geri alınmış sayılmaz. Testlere özel marker dosyaları ve alt süreçler ürün eklenti mekanizması değildir. Orijinal oyun kurulumuna yazılmaz; codec deneyi yeni özel kopyada yapılır, eski üç dosyalı kanıt dizini korunur. Standart build GTA'yı başlatmaz.

## CLI ve kanıt alanları

```powershell
./out/windows-x86/Debug/saex_engine_loader_probe.exe --observe-codec-return "<absolute-exe>" "<absolute-working-directory>"
```

Scope bounded-codec-return-observation, schemaVersion 1. Additive codecObservation eski modlarda null. Root/entry/proxy/startup policy alanları önceki faz kimliklerini korur; yeni alan kendi executionPolicy/digest'ini taşır.

| Alan | Yorum |
|---|---|
| startupBodyExecutionAllowed | Mod seçildi; yürütmenin kanıtı değildir |
| shapeVerified | Üçüncü durakta recipe/literal/OS hedefi denetlendi |
| breakpointArmed / continued | DR readback; ardından gerçek ContinueDebugEvent başarısı |
| reached | Dördüncü execution hit kimliği doğrulandı |
| returnAddress / moduleHandle | Durulan adres ve EAX; engine API adresi olarak kullanılmaz |
| armEvent | Yeni codec mapping'leri bunun sonrasında oluşmalı |
| moduleCount / mappingIds | Doğrulanan root-first sayısı / üç fixed slot; kullanılmayan slot 0 |
| modulesVerified | Bütün beklenen mapping'ler ve root/EAX bağı doğrulandı |

Exit 3 yalnız codec_return_verified ve childExitConfirmed birlikteyse; ret/incomplete 1, kullanım 2. Başarısızlıkta root initializationVerified ve canAttach false kalır.

## Kabul senaryoları ve doğrulama kaydı

Own codec root→leaf DLL zinciri ve yedi EXE varyantı gerçek LoadLibraryA kullanır. Marker'lar startup gövdesini, iki DLL initializer'ını ve durak sonrasındaki canary'yi ayırır. Gözlemcisiz pozitif kontrolde son canary/exit 90 görülmeli; observer koşularında return sonrası marker hiç oluşmamalıdır. Bu fixture'ın proxy target'ı gerçek GetStartupInfoA yerine geçecek bir ürün implementasyonu değildir.

Normal dönüş, DllMain FALSE/exception/stall, dönüş byte drift, preloaded root/dependency, yanlış/eksik/duplicate pin, yanlış root handle, eksik required mapping, unpinned transitive DLL, invalid RVA/literal/slot/closure, bütçe, eski startup durağı, foreign-owner recovery ve terminal reuse sınanır. Ayrıca 12 warm çevrim handle eşitliği; strict policy ve child öncesi CLI ret testleri vardır. Sonuçlar çalıştırıldıktan sonra aşağıda kaydedilir; bu paragraf tek başına test başarısı iddiası değildir.

## Sonraki kapı

Codec dönüşü geçtikten sonra export bağlama ve ASI/başlangıç modülü yolu incelenebilir. SAEX bootstrap DLL'sinin gerçek GTA'da yüklenmesi ve Initialize/Query/Stop C ABI'sinin LoadLibrary dönüşünden sonra loader lock dışında çağrılması hâlâ ayrı doğrulama ister. D1-N2 tamamlanmadan frame/pool/collision veya D2 multiplayer açılmaz.

## İlk yerel sonuç ve giderilen test hatası

İlk Debug derleme geçti. İlk native codec koşusunda normal dönüş, DllMain FALSE/fault/stall, shape drift ve root preloaded retleri geçti; testin sistem pin listesindeki ilk öğeyi önceden yüklenmiş sayması yanlıştı. Bu öğe aktif olmadığından doğru ret codec_mapping_missing oldu, fakat test codec_module_preloaded bekliyordu. Test artık adıyla seçilen kernel32 pinini dependency olarak kullanır; production ret gevşetilmedi. `test-codec-first.log` başarısız kayıt olarak korundu.

Düzeltilmiş corpus **24 senaryo + gözlemcisiz pozitif canary + 12 warm çevrim** geçti; Debug handle 139→139. Yedinci EXE, codec initializer içinde aynı EXE IAT pointer'ını tekrar yazar; DR1/B1 terminal ret ve codec return hit'inin oluşmaması doğrulanır. Unpinned transitive DLL olayında iki codec initializer marker'ı da oluşmaz. Kayıt `out/verification/engine/test-codec-fixture-fix.log`. Altı strict policy testi ayrıca geçti; standart bütün matris henüz bu kayıt değildir.

İlk özel GTA Debug deneyi codec_return_verified/exit 3 ve confirmed exit verdi. Üç codec mapping'i armEvent sonrasında oluştu; root base/EAX eşleşti. İlk kayıt `out/verification/engine/codec-gta-Debug-first.json`: 43 event, root-first mapping ID'ler 26/30/29; ogg/vorbis ilk eşlemeleri emekliye ayrılıp yeniden eşlendi. Ledger eski eşlemeleri aktif diye kullanmadı. Orijinal ve tarihsel deney input hash'leri korundu. Bu ilk sonuç tekrar matrisi veya SAEX bootstrap kanıtı değildir.

Recipe source SHA-256: **425272040c068e1e73b0cc1a9c43f9328b4dd13a2a00441efdbebe0a2372348d**. Ayrı static disassembly/import kayıtları `out/research/codec-call-disassembly.txt` ve `codec-wrapper-imports.txt` içinde tutulur; üçüncü taraf binary dosyaları repoya eklenmez.

## Tamamlanan standart ve gerçek GTA doğrulaması

| Profil | Native CTest suite | Managed/entegrasyon | Python |
|---|---|---|---|
| Windows x86 Debug | 12/12 | 79/79 | 93/93 |
| Windows x86 Release | 12/12 | 79/79 | 93/93 |
| Windows x64 Debug | 6/6 | 79/79 | 66/66 |
| Windows x64 Release | 6/6 | 79/79 | 66/66 |

Dört tools/build.ps1 akışı geçti; bootstrap artifact denetimi ve oyun dışı 100 DLL lifecycle çevrimi, eski native corpus'lar ve managed araç testleri dahil. Yeni codec suite her x86 profilde 24 senaryo, gözlemcisiz pozitif kontrol ve 12 warm çevrimi tamamladı. Son Release codec handle sayısı 143→143; ilk düzeltilmiş Debug koşusunun 139→139 sayısı farklı runner anına aittir. Process-wide ilk yükleme maliyeti veya bütün kaynak türleri için sıfır sızıntı sonucu çıkarılmaz. Loglar `out/verification/engine/build-codec-<architecture>-<configuration>.log`.

Yeni özel dizin `out/experiments/gta-codec-f01a00ce-v1`: exact EXE'nin saex-engine-observation.exe adlı byte-identical kopyası, eax.dll, vorbisfile.dll ve üç codec DLL; toplam altı dosya. Orijinal kurulum veya önceki üç dosyalı deney dizini düzenlenmedi. Bu dizinde oyun asset'leri veya SAEX/ASI bulunmaz; oynanabilir kurulum değildir.

`out/verification/engine/codec-return-gta-evidence.json` matrisinde **6 Debug + 6 Release = 12/12 codec_return_verified/exit 3**. Tüm koşularda dördüncü hit proxy base+0x14B8, EAX=root base, üç fresh active mapping ve shape kontrolü başarılı; IAT write gözlenmedi. Dört koşuda 44, sekiz koşuda 42 event; sırasıyla 5 veya 4 unload, tümünde 25 aktif DLL. İlk tek Debug koşusunun 43 event'i bu matristen ayrı kayıttır. DLL eşleme/emeklilik sırası değişebilir; fixed mapping ID numaraları runtime sözleşmesi değildir.

Yedi ek regresyon: original kurulum AcLayers mapping'inde loader_module_not_pinned/exit 1; dört eski private startup/proxy/entry/context komutu kendi duraklarında exit 3 ve codecObservation=null; eski üç dosyalı kopyada eksik vorbishooked pin'i child öncesi loader_policy_file_unavailable; ayrı olumsuz kopyada yalnız codec dosyasının bir byte'ı değiştirilince child öncesi loader_policy_file_mismatch. Son iki durumda PID oluşmadı. Toplam **19/19 beklenen sonuç, oluşturulan 17/17 child için confirmed exit**.

Orijinal 13 native dosya, tarihsel private üç dosya ve yeni pozitif private altı dosyanın toplam **22 hash'i** matris öncesi/sonrası eşitti; ilk deneyden önceki original/tarihsel snapshot da doğrulandı. Hash ret deneyi ayrı `gta-codec-hash-rejection-v1` kopyasında kasıtlı değişikliktir; bu dosya korunmuş 22 girdi arasında sayılmaz. Ortam snapshot digest'i tüm pozitif koşularda `3725a091ed77f875b48ab336a8162328429aeebc82f2c4779f897ed54f62407e`; ham ortam değerleri kaydedilmedi.

| x86 probe artifact | SHA-256 |
|---|---|
| Debug saex_engine_loader_probe.exe | eca08def074e6bc84041a6bbac9c1cf72a04f16984907b64f7000c9c5fb8e097 |
| Release saex_engine_loader_probe.exe | 48308a554a8bf5e530f8b335b15c2d8c8dd4ef551ec88510057630a3d87bdaa0 |

0.1.14 için Linux/hosted CI ve N1 opt-in SDK yeniden çalıştırılmadı; önceki 0.1.13 hosted kanıtı bu artifact'ları kapsamaz. Gerçek SAEX DLL'si GTA'ya yüklenmedi; codec export çağrısı/ses, tam wrapper dönüşü, loader-lock dışı bootstrap C ABI, tam unpack/frame/pool/collision ve N2/D1/D2 açık kalır.

0.1.14 son statik kontrol: 92 Markdown, 1287 yerel bağlantı, 6 JSON örneği ve sıfır hata; --base HEAD kaynak/belge eşlemesi başarılı. Bu sonuç anlamsal kusursuzluk veya oyun initialization kanıtı değildir.

## Kod 0.1.15 — Codec fonksiyon bağları

Eski codec-return modu dördüncü durakta kalır. Yeni codec-bindings modu aynı kimlik kapısından sonra yalnız sekiz export adresinin tabloya atanmasını ilerletir. Eski codec_handle/armEvent/mappingIds alanları dördüncü durağın kanıtıdır. Own codec DLL fixture sekiz test exportu kazandı; production codec dosyaları değişmedi. [Sözleşme ve kanıt](d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Frame aday modu mevcut codec dönüş kontrolünü aynen kullanır; yeni codec izni veya fallback eklemez. Bu tarihsel dönüş raporu yeni frame/runtime ABI kanıtı sayılmaz. [Aday ve doğrulama raporu](d1-frame-target.md).

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
