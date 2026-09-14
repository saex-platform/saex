# D1-N2 — Codec fonksiyon bağları

Kod **0.1.15**, mimari **v0.22**, 13 Eylül 2026, ADR-50. [Durum](status.md) · [Codec dönüşü](d1-codec-return.md) · [Native sözleşme](../architecture/native-sdk-integration.md).

## Sorumluluk ve kapsam

run_to_codec_bindings/--observe-codec-bindings, önceki dört duraktan sonra wrapper'ın sekiz GetProcAddress sonucunu kendi pointer tablosuna yazmasını ilerletir. Beşinci DR0 durağı son atama sonrasındadır; sonraki talimat, ASI taraması ve codec fonksiyonlarının kendileri çalıştırılmaz. Sonuç yalnız bağlanan adresler ve sınırlı target byte kararlılığıdır; ses decode, fonksiyon prototype/calling convention, tam LoadPlugins dönüşü veya SAEX bootstrap C ABI doğrulaması değildir. N2/D1/D2 açık; canAttach/initializationVerified false kalır.

Eski komutlar aynı durak ve izinleri korur. Yeni mod aynı 26 retained pini kullanır; ek DLL/ASI izni yoktur. Public C++ observer trace/API genişlediği için bağlı local çağıranlar birlikte yeniden derlenir. Bootstrap C ABI 1, engine profile/anchor, SDK lock, GNS/HTTPS ve otorite kararı değişmez. Kaldırılan özellik veya kalıcı state migration'ı yoktur.

## Veri kaynağı ve sözleşme

[binding-policy.json](../../contracts/engine/binding-policy.json), codec-policy source SHA-256'sına bağlıdır; codec→startup→proxy→entry→loader ve observed-profile bağı korunur. [Compiler](../../tools/binding_policy.py) yedi source için 64 KiB/file sınırı, strict JSON duplicate/unknown key, schema/stage/id/review, digest, RVA/slot alignment, 16-byte stop örneği ve exact sekiz isim/sıra kontrolü yapar. Duplicate slot, eksik/fazla entry ve bool/int karışımı reddedilir. Generated header elle değiştirilmez; standart Windows/Linux akışı --check ve ret testlerini çalıştırır. Runtime/server JSON veya otomatik export keşfi ile yürütme izni verilemez.

Exact yerel vorbisfile.dll disassembly ve literal okumaları, hedef vorbisHooked.dll export tablosuyla karşılaştırıldı. Kaynaklar `out/research/binding-wrapper-disassembly.txt`, `binding-codec-exports.txt`; önceki [linkage raporu](d1-native-linkage.md) ayrı statik kanıttır. RVA'lar yalnız hash'i sabit bu iki dosyaya aittir; genel GTA engine adresleri değildir.

| Export adı | Wrapper pointer slot RVA | Codec target RVA |
|---|---|---|
| ov_open_callbacks | 0x6408 | 0x1100 |
| ov_clear | 0x6428 | 0x1000 |
| ov_time_total | 0x6414 | 0x21C0 |
| ov_time_tell | 0x6404 | 0x33F0 |
| ov_read | 0x642C | 0x3560 |
| ov_info | 0x641C | 0x34E0 |
| ov_time_seek | 0x6400 | 0x31B0 |
| ov_time_seek_page | 0x6424 | 0x32B0 |

GetProcAddress import slot RVA 0x4024. Son pointer store RVA 0x1541, beşinci durak **RVA 0x1546**; beklenen sonraki 16 byte `c78554fdffff00000000c78558fdffff`. ASLR base'i runtime mapping'den alınır. [Microsoft GetProcAddress sözleşmesi](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress) isim/case eşitliği ve başarısızlıkta null dönüşünü tanımlar; non-null pointer tek başına doğru bağ değildir.

BindingStopSpec, trusted caller'ın tuttuğu 1–8 BindingSlotSpec span'ıdır: ad, wrapper slot RVA, root codec target RVA. Ad 1–31 ASCII harf/rakam/alt çizgi; slot dört-byte aligned, bounded, ad ve slot tekil olmalıdır. Stop RVA/16 expected byte ve GetProcAddress slot RVA ayrıca taşınır. CLI yalnız derlenmiş sekiz kaydı kullanır. Forwarder, başka DLL'ye export yönlendirme veya indirilen modülden adres kabulü bu recipe'nin kapsamı dışındadır.

## Beş duraklı akış

```text
exact engine/context + 26 retained pin
  → entry / proxy-return / startup-call / codec-return kapıları
  → dördüncü durak: aktif root/dependency kimlikleri ve root/EAX
  → stop byte'ları, GetProcAddress OS hedefi ve boş sekiz slot
  → her codec target'ın ilk 16 executable byte'ını örnekle
  → DR0 = wrapper base + 0x1546; mevcut DR1 IAT watch korunur
  → sekiz export adresi wrapper tablosuna yazılır
  → beşinci hit: aynı mapping'ler, stop/slot/target kontrolleri
  → sonraki talimat devam etmeden owned child kill + confirmed exit
```

Dördüncü durakta codec call/return 13-byte şekli, proxy mapping/thunk, EXE startup IAT, yeni stop'un 16 byte'ı ve GetProcAddress pointer'ının aktif pinli kernel32/kernelbase executable MEM_IMAGE bölgesinde bulunması denetlenir. GetProcAddress slot pointer'ının iki durakta eşitliği aranır; arada yazılıp geri alınması bu örneklemle saptanmaz. Target byte karşılaştırması da iki durak örneğiyle sınırlıdır. İsim/API bağı exact binary import incelemesine dayanır; tüm OS kodunun hash doğrulaması veya hook bulunmadığı kanıtı değildir.

Sekiz wrapper slot'u bounded MEM_IMAGE okumasıyla sıfır olmalıdır; önceden doldurulmuş tablo binding_preexisting_slot ret verir. Target başına ilk 16 byte aynı root codec mapping'in executable bölgesinden okunur. Beşinci durakta adres rootBase+targetRva ile eşit, ilk 16 byte sabit olmalıdır. Bütün slotlar okunabildiğinde tüm eşleşmeler raporlanır; herhangi biri null/yanlış hedef veya byte drift ise binding_slot_mismatch. Okuma başarısızsa binding_slot_unreadable; diğer alanlar partial olabilir.

Beşinci hit main thread, first-chance SINGLE_STEP, EIP/ExceptionAddress/DR0 ve yalnız DR6 B0 kimliği ister. DR7/DR1 önceki sözleşmede kalır; readback doğrulanır. Observer code/IAT/EIP/stack/EFlags yazmaz. İlk dört durak alanları önceki fazların kanıtlarıdır; özellikle codec_handle dördüncü durağın EAX değeridir, beşinci hitte tekrar EAX ile değiştirilmez. Startup samples hâlâ üçüncü durakta alınır.

## Hata ve yaşam döngüsü

Bağlama ilerlerken codec root/dependency mapping ID'lerinden biri emekliye ayrılırsa terminal binding_mapping_retired. Yeniden eşleme ile aynı base'i kullanmak önceki kimliği geri getirmez. Bu aşamada hiçbir yeni DLL mapping'ine devam izni yoktur: bilinen pinle eşleşse de binding_unexpected_load; bilinmeyen dosyada mevcut loader_module_not_pinned ret önceliklidir. Böylece bu izin sonraki ASI yüklemelerini kapsamaz.

Main-thread EXE IAT write watch korunur; yazımdan sonra tetiklenir, cross-thread sandbox değildir. Exception/stall, unknown event ve önceki 128 event/64 lifetime module/16 thread/256 MiB/5 saniye bütçeleri aynen geçerlidir. Senkron I/O hard wall-clock deadline vermez. Foreign-owner çağrı pending state/cleanup'ı değiştirmez; terminal child yeniden kullanılamaz. Kill pending talimattan önce; process exit ayrıca doğrulanır.

Exact pin ve 16-byte örnekleri bütün DLL kodunun güvenliği, export tablosunun tümünün bütünlüğü, keyfî native müdahalelerin engellenmesi veya fonksiyon ABI'si anlamına gelmez. Native kod bu ayrı izin içinde çalışır. Bu bir development debugger deneyi; production IPC, indirilen C# sandbox veya mod loader değildir.

## CLI ve kanıt

```powershell
./out/windows-x86/Debug/saex_engine_loader_probe.exe --observe-codec-bindings "<absolute-exe>" "<absolute-working-directory>"
```

Scope bounded-codec-bindings-observation, schemaVersion 1. Additive bindingObservation eski modlarda null. Yeni nesne executionPolicy/digest, bindingExecutionAllowed, breakpointArmed/continued/reached/verified, stopAddress ve en fazla sekiz slot taşır. Slot ad/RVA'ları, before/after pointer'ları, targetStable ve match içerir; ham stack veya ortam değerleri yoktur. bindingExecutionAllowed yalnız mod seçimini, continued başarılı ContinueDebugEvent'i belirtir. verified=false iken partial alanlar complete sayılmaz. Exit 3 yalnız codec_bindings_verified + childExitConfirmed; ret/incomplete 1, usage 2.

## Kabul ve genişleme

Own root DLL'de sekiz export ve proxy'de gerçek GetProcAddress kullanan yeni fixture vardır. Export çağrılırsa ayrı canary; durak sonrası talimatlar çalışırsa başka canary oluşur. Gözlemcisiz pozitif kontrolde durak sonrası canary görülebilmeli; observer koşularında iki canary de oluşmamalıdır. Normal bağ, null/yanlış fonksiyon, target/stop drift, exception/stall, root unload, yeni DLL load, invalid spec/name/range/slot, duplicate, prepopulated slot, eski codec durağı, owner recovery ve bütçeler sınanır. Warm çevrimler ve strict policy/CLI retleri standart akışa dahildir. Bu bölüm test planıdır; sonuçlar çalıştırıldıkça aşağıda kaydedilir.

Bir sonraki izin ASI yükleme yolu ve gerçek SAEX bootstrap DLL'sini kapsayacak ayrı incelemedir. Initialize/Query/Stop loader lock dışında çağrılmadan N2 başlatma kanıtı tamamlanmaz. Frame/pool/collision, GNS ve iki istemcili D2 sonraki kapılardır.

## İlk doğrulama

İlk Debug derleme ve native.codec_bindings geçti: **24 senaryo + gözlemcisiz pozitif canary + 12 warm çevrim**, handle 141→141. Altı policy testi geçti. İlk doküman eşleme kontrolü binding-policy source'u için execution-contracts güncellemesinin eksikliğini buldu; ilgili normatif belge eklenince kontrol geçti. Native test başarısızlığı veya runtime ret gevşetmesi olmadı. Loglar `build-binding-first.log`, `test-binding-first.log`.

İlk gerçek özel GTA Debug koşusu codec_bindings_verified/exit 3, 45 event ve confirmed exit verdi. Sekiz slot'un before değeri 0, after değeri expected root+RVA; bütün targetStable/match true. Stop proxy base+0x1546. Kayıt `out/verification/engine/binding-gta-Debug-first.json`; orijinal/tarihsel/yeni pozitif private toplam 22 input hash'i ilk deneme öncesi/sonrası korunmuştur. Bu ilk deneme tam standart matris yerine geçmez.

Binding policy source SHA-256: **92856899cf2250668e1b644d9ec2c111b01e72eba46643bb4631285ee3de84eb**. Parent codec source SHA-256: **425272040c068e1e73b0cc1a9c43f9328b4dd13a2a00441efdbebe0a2372348d**.

## Tamamlanan standart ve gerçek GTA matrisi

| Profil | Native CTest suite | Managed/entegrasyon | Python |
|---|---|---|---|
| Windows x86 Debug | 13/13 | 79/79 | 101/101 |
| Windows x86 Release | 13/13 | 79/79 | 101/101 |
| Windows x64 Debug | 6/6 | 79/79 | 72/72 |
| Windows x64 Release | 6/6 | 79/79 | 72/72 |

Dört tools/build.ps1 akışı geçti; x86 bootstrap artifact denetimi, oyun dışındaki 100 DLL lifecycle çevrimi ve bütün eski entry/proxy/startup/codec corpus'ları dahil. Binding suite her x86 profilde 24 senaryo + gözlemcisiz pozitif kontrol + 12 warm çevrim içerir. Son Release handle 141→141; bu tüm process kaynakları için sıfır sızıntı garantisi değildir. Loglar `out/verification/engine/build-binding-<architecture>-<configuration>.log`.

`out/verification/engine/codec-bindings-gta-evidence.json`: **6 Debug + 6 Release = 12/12 codec_bindings_verified/exit 3**. Sekiz slot × 12 koşuda **96/96 doğru adres bağı**; before=0, after=rootBase+targetRva ve targetStable/match true. Sekiz target'ın toplam 128 byte'ı iki durakta örneklenir; tüm codec image'ı veya ABI sınanmış sayılmaz. Beşinci durak her koşuda proxy base+0x1546, root codec mapping kimliği korunmuş, EXE IAT write gözlenmemiştir. Bir koşuda 41, dördünde 43, yedisinde 45 debug event; sırasıyla 3/4/5 unload ve tümünde 25 aktif DLL. Mapping ID/ASLR adresleri sabit ürün kimlikleri değildir.

Sekiz gerçek regresyon: original AcLayers ret/exit 1; eski codec/startup/proxy/entry/context komutları kendi duraklarında exit 3 ve bindingObservation=null; eski üç dosyalı kopyada eksik codec child öncesi file_unavailable; önceki ayrı hash-ret kopyasında değiştirilmiş codec child öncesi file_mismatch. Toplam **20/20 beklenen sonuç, oluşturulan 18/18 child için confirmed exit**. İki preflight reddinde süreç hiç oluşturulmaz.

Önceki altı dosyalı `gta-codec-f01a00ce-v1` özel kopyası kullanıldı; yeni DLL/ASI veya oyun asset'i eklenmedi. Orijinal 13 native dosya + tarihsel üç dosya + pozitif codec kopyasındaki altı dosyanın **22 hash'i** matris öncesi/sonrası eşit ve önceki kayıtla uyumludur. Olumsuz hash kopyasındaki kasıtlı farklı codec bu 22 korunmuş girdi arasında değildir. Ortam snapshot digest'i bütün pozitif koşularda `3725a091ed77f875b48ab336a8162328429aeebc82f2c4779f897ed54f62407e`; ham ortam kaydedilmedi. İlk tek Debug denemesi bu matristen ayrı dosyadadır.

| x86 probe artifact | SHA-256 |
|---|---|
| Debug saex_engine_loader_probe.exe | 93eebf6d683f2a4ada5c8a210a97688e89fc072033d8eaa9201b07ebd4a36d6c |
| Release saex_engine_loader_probe.exe | ad44855dff05dfca066fa02b4d40b038076fdda346d01160e203deb197fd5f4d |

0.1.15 için Linux/hosted CI ve N1 opt-in SDK yeniden çalıştırılmadı. Sekiz codec fonksiyonunun adresleri bağlandı, fonksiyonlar çağrılmadı; ASI taraması, gerçek SAEX DLL yükleme/Initialize/Query/Stop, tam unpack/frame/pool/collision ve N2/D1/D2 açık kalır. Bir sonraki kesit ASI yolu ve loader lock dışında SAEX bootstrap C ABI kanıtıdır.

0.1.15 son statik kontrol: 93 Markdown, 1320 yerel bağlantı, 6 JSON örneği ve sıfır hata; --base HEAD kaynak/belge eşlemesi başarılı. Bu statik sonuç tüm mimarinin anlamsal doğruluğu veya GTA initialization kanıtı değildir.

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Frame aday denetimi, mevcut codec mapping ve sekiz pointer doğrulaması tamamlandıktan sonra çalışan ayrı bootstrap modundadır. Binding durma noktası ve codec fonksiyon çağrısı sınırı değişmedi. [Aday ve doğrulama raporu](d1-frame-target.md).

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
