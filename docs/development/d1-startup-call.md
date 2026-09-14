# D1-N2 — Startup çağrı sınırı

Kod **0.1.13**, mimari **v0.20**, 13 Eylül 2026, ADR-48. [Durum](status.md) · [Önceki proxy dönüşü](d1-proxy-return.md) · [Native sözleşme](../architecture/native-sdk-integration.md).

## Sorumluluk ve mevcut sınır

Bu kesit orijinal PE girişini ayrı izinle ilerletir; proxy’nin GetStartupInfoA yerine yerleştirdiği hedefin ilk talimatında ana thread’i durdurur. Çağıran kod, stack parametre alanı ve mevcut dört image anchor yeniden okunur. Fonksiyon gövdesine devam edilmeden owned child kapatılır. Bu bir debugger deneyi; production launcher, SAEX bootstrap yüklemesi, native fonksiyon ABI doğrulaması veya unpack edilmiş motor profili değildir. CanAttach/initializationVerified false kalır.

run_to_startup_call/--observe-startup-call yeni açık girişlerdir. run, run_to_entry ve run_to_proxy_return önceki birinci/ikinci duraklarını korur. Public C++ LoaderTrace ve arayüz değiştiği için bağlı local çağıranlar birlikte yeniden derlenir. Bootstrap C ABI 1, observed profile/dört anchor, N1 SDK lock ve GNS/HTTPS kararı değişmedi. Kaldırılan ürün özelliği veya kalıcı state migration’ı yoktur.

## Ayrı izin ve kaynak bağı

[startup-policy.json](../../contracts/engine/startup-policy.json) source’u proxy-policy digest’i ve observed-profile digest’ine bağlıdır; proxy→entry→loader-policy bağı aynen korunur. Mevcut 23 retained DLL pini kullanılır. Yeni DLL, codec veya ASI otomatik keşfedilip onaylanmaz.

| Bağ | SHA-256 |
|---|---|
| Startup execution source | ed2834167df81d2068197dd333e7f28602dc243adcd8d80290498c41508caf30 |
| Proxy parent source | 20ff7fb664b56f3a43cded86faaa15743495e0cc1d9f17ccb675ccf6732fddcf |
| Observed profile source | 0653426a913c3fdef809ea9e6a614fca7d26498e66cb63148d7073fe8d965436 |

[Generator](../../tools/startup_policy.py) en fazla 64 KiB/source, strict duplicate/unknown key, schema/stage/id/review, parent digest ve engine eşitliği denetler. Call form x86-ff15-iat, argument alanı 68 byte, sample üst sınırı dört ve IAT write watch zorunludur; bool/int karışımı veya sınırı gevşeten değer reddedilir. Generated C++ iki parent digest’i static_assert ile bağlar. Native x86 hedefinde sizeof(STARTUPINFOA)==68 derleme kontrolüdür. Source güncellemesi bilinçli üretim ister; normal Windows/Linux akışı --check çalıştırır. Runtime JSON/config veya server’dan native adres kabulü yoktur.

StartupStopSpec, caller’ın ömrünü koruduğu 1–4 ImageAnchor span’ıdır. Sample başına uzunluk 1–16 byte, geçerli bounded RVA, birbiriyle çakışmama ve CREATE_PROCESS anında expected byte eşitliği şarttır. CLI yalnız mevcut derlenmiş observed_anchors kullanır. Arbitrary PID/RVA seçimi veya tanınmayan EXE’ye yürütme izni yoktur.

## Veri akışı ve üç durak

```text
exact file/profile + 23 retained pin + explicit cwd/environment
  → CREATE_PROCESS: erken main-thread DR0, profile/anchor kontrolü
  → initial ntdll breakpoint: önceki doğrulamalar
  → PE entry hit: E9/proxy/return-slot/IAT-before kontrolü
  → proxy return hit: entry restore + IAT-after, önceki iki-hit sözleşmesi
  → yeni izin: DR0=IAT target; DR1=IAT slot (4 byte write)
  → orijinal entry ilerler
  → ilk startup target hit: register, mapping, IAT ve target byte kontrolü
  → stack return/argument, FF15 callsite, image sample gözlemi
  → gövdeye girmeden kill ve confirmed exit
```

Üçüncü DR0 hedefi ikinci durakta doğrulanan proxy IAT pointer’ıdır. İkinci durakta hedefin ilk 16 executable MEM_IMAGE byte’ı alınır; üçüncü durakta aynı aktif proxy mapping içinde tekrar okunup eşitlik aranır. Bu, iki durak arasındaki kararlılıktır; tüm DLL gövdesinin dosya eşitliğini veya güvenli olduğunu kanıtlamaz.

DR1 aynı main-thread üzerinde dört byte IAT slotuna write watch kurar: L0/L1, slot 0 execute/length1, slot 1 write/length4; seçilmiş DR7 bitleri 0x00D00005. Get/SetThreadContext readback gerekir. Üçüncü execution hitinde DR6 B0; write olayında yalnız B1 ve beklenen DR0/DR1/DR7 kimliği aranır. Aynı pointer değerinin tekrar yazılması da write olayıdır; sadece önce/sonra değer karşılaştırması yapılmaz. Watchpoint yazımdan **sonra** trap üretir, yazmayı engellemez. Context/register veya diğer exception uyuşmazlığı terminaldir. [Intel debug register belgeleri](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) temel alınır; gerçek write/hit davranışı ayrıca own fixture ile sınanır.

Observer code/IAT/EIP/stack/EFlags yazmaz. Native DLL/EXE kodu bu izin içinde çalışabilir; main-thread hardware register ayarı ve owned process cleanup debugger’ın işlemleridir. Başka thread’in IAT yazımını DR1 izlemez; o thread’deki olay beklenmedikse mevcut terminal olay kuralı uygulanır. Bu düzen bir güvenlik sandbox’ı veya bütün thread’lerin kontrol altında olduğunun kanıtı değildir.

## Çağıran ve parametre denetimi

İlk target hitinde ESP’nin committed, nonguard MEM_PRIVATE bölgesinden sadece sekiz byte okunur: return address ve ilk argument pointer. Return address main EXE image aralığında olmalıdır. Önceki altı executable byte, FF15 ve tam base+iatRva operandına eşit olmalıdır. Register call, tail call, başka modülden çağrı veya farklı slot desteklenmez; yeni inceleme ister. Return address’ten eksi altı hesaplaması aralık kontrolünden sonra yapılır.

Argument dört-byte aligned olmalı, ESP ile aynı AllocationBase içindeki writable MEM_PRIVATE bölgesinde tam 68 byte’a yer vermelidir. Null/heap/readonly/taşan veya farklı allocation bu dar deneyde reddedilir. Bu, genel GetStartupInfo API’sinin tüm geçerli kullanım biçimlerinin listesi değildir; deneyin incelenmiş stack kullanım sınırıdır. Struct field’ları ve içindeki pointer/string verileri okunmaz; cb veya standart handle değerleri hakkında sonuç üretilmez. [Microsoft STARTUPINFOA tanımı](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfoa) ve x86 compiler layout kontrolü birlikte kullanılır. Fonksiyon çalıştırılmadığı için gerçek output veya dönüşte stack temizleme/calling convention doğrulaması yapılmış sayılmaz.

## Image örnekleri ve unpack ayrımı

Dört mevcut profil anchor’ı, toplam 20 byte, üçüncü durakta yeniden okunur. Sample.read ve sample.match ayrıdır. Okunmuş ama değişmiş örnek, gözlemin başarılı bir çıktısı olabilir; sample.match=false tam initialization hatası veya yeni destek profili ilan etmez. Okunamayan sample startup_sample_unreadable ile incomplete olur. Genel image hash’i, section bütünlüğü, bütün unpack yolu veya engine symbol doğrulaması eklenmedi.

Referans [ASI-Loader kaynağı](https://github.com/GTAmodding/ASI-Loader/blob/main/vorbisFile.cpp) bu wrapper çağrısı içinde LoadPlugins ve ardından gerçek GetStartupInfo yolunu gösterir. Kaynaktaki zamanlama açıklaması yerel EXE/DLL’nin tam unpack kanıtı olarak alınmaz: exact source/build eşitliği kanıtlanmadı ve dört anchor çok dar bir örneklemdir. Bu kesit wrapper gövdesini, dinamik vorbishooked/codec/ASI zincirini veya SAEX DLL’sini çalıştırmaz.

## CLI/evidence arayüzü

```powershell
./out/windows-x86/Debug/saex_engine_loader_probe.exe --observe-startup-call "<absolute-exe>" "<absolute-cwd>"
```

Bu açık komut DLL/TLS, proxy ve orijinal entry kodunu ilerletir. Standart tools/build.ps1 kendi fixture’larını çalıştırır; GTA kurulumu gerektirmez ve GTA’yı otomatik başlatmaz.

Scope bounded-startup-call-observation, schemaVersion 1; additive startupObservation eski komutlarda null’dır. Root mapping/entry/proxy kimlikleri önceki fazların anlamını korur. Startup nesnesi kendi executionPolicy/digest’ini taşır.

| Alan | Anlam |
|---|---|
| entryExecutionAllowed | Yeni mod seçildi; yürütmenin gerçekleştiği anlamına gelmez |
| breakpointArmed / continued | DR0/DR1 readback başarılı; ardından ikinci duraktan ContinueDebugEvent başarılı |
| reached | Beklenen main-thread execution fault kimliği doğrulandı |
| iatWriteObserved | İncelenmiş DR1/B1 write trap terminal olarak görüldü |
| targetStable | İki durak arasındaki 16 target byte aynı |
| callsiteVerified / argumentValid | FF15 exact slot ve bounded same-stack parametre alanı doğrulandı |
| address / returnAddress / argumentAddress | Bu gözlemdeki adres metadata’sı; kalıcı engine API adresi değildir |
| targetBeforeHex / targetAfterHex | İkinci ve üçüncü durak örneği; true doğrulama bayrağı olmadan başarı diye yorumlanmaz |
| samples | En fazla dört RVA/length/beforeHex/afterHex/read/match kaydı; ham stack içeriği yok |

Exit 3 yalnız startup_call_verified ve childExitConfirmed birlikteyse döner. Geçerli sample değişimi bu tanısal complete sonucunu engellemez. Watch trap, target/IAT/shape/argument/range uyuşmazlığı, okunamama, timeout/budget veya unknown exception exit 1; kullanım exit 2. Root initializationVerified/canAttach false kalır.

## Yaşam döngüsü ve hata davranışı

Mevcut proxy mapping emekliye ayrılırsa terminal ret sürer. 128 event, 64 lifetime module, 16 lifetime thread, 256 MiB kümülatif file read ve beş saniye observer bütçesi değişmez. I/O blocking çağrıları hard wall-clock deadline değildir. EXIT_THREAD gibi önce desteklenmeyen olaylar sessizce kabul edilmez. Foreign-owner çağrı pending event/cleanup’ı değiştirmez; owner doğru çağrıyla ilerleyebilir. Terminal child yeniden kullanılamaz. Bütün normal/ret bitişlerinde önce kill, ardından pending cleanup ve process exit kontrolü vardır.

## Testler, araştırma ve ilk başarısızlık

Native.startup_observation dokuz own EXE ve mevcut own proxy DLL’yi kullanır. Normal CRT entry’den sonra main marker oluşur; IAT target canary’si gövdeye girilirse startup marker + exit 90 üretir. Bu gerçek GetStartupInfoA implementasyonu değildir. Gözlemcisiz pozitif kontrolde exit 90 ve marker’lar oluşmalıdır; observer koşularında target marker’ı oluşmamalıdır. Fixture’lar dağıtım/güvenlik sandbox girdisi değildir.

19 senaryo: normal, entry fault/stall, null argument, register call, aynı IAT değerine write, target mutation, değişmiş/okunamayan image örneği, boş/aşırı/unaligned spec, sample length/precondition/range/overlap, event budget, eski proxy modu ve foreign-owner recovery. Ayrıca pozitif control ve 12 warm çevrimde handle eşitliği vardır; her observer koşusunda confirmed exit ve terminal reuse ret kontrol edilir. Yedi policy testi digest/schema/key/boundary/engine/review retlerini; iki yeni CLI testi unknown engine/invalid cwd child öncesi kapıyı denetler. Loader CLI corpus toplamı 15’tir.

Ortak fixture PE/marker helper proxy_fixture_support.hpp’ye taşındı; test runner DLL yüklemeden kendi fixture export/import RVA’larını okur. Production PE parser genişletilmedi. İlk custom /ENTRY:main denemesinde memset/CRT debug ve exception sembollerinde link hatası görüldü (`build-startup-first.log`). Ek bir CRT paketi veya compiler check gevşetmesiyle geçilmedi; standart CRT entry’ye dönüldü, /INCREMENTAL:NO ve /OPT:NOICF yalnız fixture’larda kaldı. `build-startup-crt.log` derlemesi ve `test-startup-first.log` 19 senaryo + positive control + 12 warm çevrimi geçti; Debug handle 140→140. İlk hata saklanır.

## Gerçek GTA gözlemi

İlk private GTA Debug koşusu startup_call_verified/exit 3 verdi ve child kapatıldı. IAT write gözlenmedi; targetStable/callsiteVerified/argumentValid true, dört sample.read/match true. Eski 13 original + üç private input dosyası SHA’ları değişmedi. Kayıt `out/verification/engine/startup-gta-Debug-first.json`.

Return address **0x0082C11C**, çağrı talimatı **0x0082C116**: FF15 F0 81 85 00, yani IAT VA 0x008581F0. Exact EXE dosyası MSVC dumpbin /DISASM /RANGE ile aynı altı talimat byte’ını gösterdi (`out/research/startup-caller-disassembly.txt`). Çağıranın sembol adı/oyun görevi bundan çıkarılmaz; kod entry sonrası bu noktaya gerçekten ulaştı. Target proxy base+0x1BA0’dır. Fonksiyon gövdesi tutulduğundan bir sonraki LoadPlugins henüz çağrılmadı.

Son matris `out/verification/engine/startup-call-gta-evidence.json` içinde: **6 Debug + 6 Release = 12/12** private GTA koşusu startup_call_verified/exit 3 verdi. Hepsinde üçüncü hit, targetStable/callsiteVerified/argumentValid true; IAT write gözlenmedi. Dört anchor’ın 12 koşudaki **48 okuması** başarılı ve mevcut byte’larla eşitti. Toplam 20 byte/run örneklenmesi tam unpack/engine initialization kanıtı değildir.

Her koşuda aynı callsite/return address ve proxy RVA 0x1BA0 ölçüldü; ASLR base’leri değişebilir. İki koşuda 30, on koşuda 32 debug event; toplam on bilinen unload ve her pozitif koşuda 22 aktif DLL vardı. Mevcut 23 pin listesi genişletilmedi.

Aynı Release artifact’ı ile dört gerçek regresyon: orijinal kurulum AcLayers mapping’inde ret/exit 1 ve startup continued=false; private eski proxy/entry/context komutları sırasıyla proxy_return_verified, entry_boundary_modified, loader_breakpoint_candidate/exit 3. Üç legacy raporda startupObservation=null. Toplam **16/16 child exit doğrulandı**. İlk tek Debug koşusu bu matristen ayrı saklandı.

Orijinal 13 native dosya ve private üç dosyanın SHA-256 değerleri başta/sonda eşit. Eski private deney dizinine codec/ASI/SAEX DLL eklenmedi; executable/DLL rename veya kurulum dosyası düzenlemesi yok. Private ortam snapshot digest’i `3725a091ed77f875b48ab336a8162328429aeebc82f2c4779f897ed54f62407e`; ham ortam değerleri saklanmadı. Üç dosyalı kopya oynanabilir GTA kurulumu değildir.

| Probe artifact | SHA-256 |
|---|---|
| x86 Debug saex_engine_loader_probe.exe | 62191ae4fae60708b4192f2544803c761b825b01f9aab7c31255c244b5fce40e |
| x86 Release saex_engine_loader_probe.exe | e154f559a2c167acd2eff7c5715e412888b320dbf029c5959cc6af255ad5cb45 |

## Tamamlanan standart doğrulama

| Profil | Native CTest suite | Managed/entegrasyon | Python |
|---|---|---|---|
| Windows x86 Debug | 11/11 | 79/79 | 85/85 |
| Windows x86 Release | 11/11 | 79/79 | 85/85 |
| Windows x64 Debug | 6/6 | 79/79 | 60/60 |
| Windows x64 Release | 6/6 | 79/79 | 60/60 |

Dört tools/build.ps1 akışı geçti. X86 bootstrap artifact audit, oyun dışındaki 100 DLL lifecycle çevrimi ve eski entry/proxy/loader/context corpus’ları da kapsamda. Yeni startup suite’in 19 senaryosu, pozitif kontrolü ve 12 warm çevrimi geçti; son Release handle sayısı 140→140. Loglar `out/verification/engine/build-startup-<architecture>-<configuration>.log` biçimindedir.

Bu kesit için Linux/hosted CI ve N1 opt-in SDK yeniden doğrulanmadı. Gerçek SAEX DLL’si GTA içinde yüklenmedi; startup wrapper gövdesi/dinamik LoadPlugins, fonksiyon dönüşü ve N2/D1/D2 kapıları açık kalır.

## Sonraki kesit

İlk çağrı konumu artık ölçülmüştür. Sonraki N2 işi wrapper gövdesinin dinamik vorbishooked/codec/ASI yolu için bounded izin/denetim ve gerçek SAEX bootstrap C ABI’sinin loader lock dışında çağrılmasıdır. Fonksiyon dönüşü, tam unpack/engine initialization, WinMain/frame/pool/collision ve indirilen C# sandbox ayrı kanıt ister. Bu alt sonuç AC-90/R-01’in tamamını veya N3/D2’yi kapatmaz.

## Kod 0.1.14 — Codec dönüş kesiti

Eski startup komutu üçüncü durakta gövdeye girmeden kalır. Yeni codec modunda aynı çağıran/argument/sample doğrulamalarından sonra yalnız ilk dinamik LoadLibraryA dönüşüne kadar ayrı izin verilir. Samples alanı hâlâ üçüncü durağın gözlemidir. [Sözleşme ve doğrulama](d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

Startup komutu üçüncü durakta kalır; yeni binding komutu aynı kapıyı izler. Startup sample alanı binding sonrası yeniden alınmış gibi yorumlanamaz; beşinci durak ayrı slot/target gözlemi taşır. [Sözleşme ve kanıt](d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Startup anchor örnekleri korunur. Frame adayının CALL/target eşleştirmesi ayrı üç duraklı gözlemde yapılır; mevcut startup CLI aynı noktada durur. [Aday ve doğrulama raporu](d1-frame-target.md).

## Kod 0.1.19 bağlantısı

Yeni doğal startup deneyi girişte gözlenen return address/stack/argüman ile dönüşteki stdcall ve nonvolatile durumu eşler; mevcut startup-call modu gövdeye girmeden durmaya devam eder. [Sözleşme ve kanıt](d1-startup-return.md).

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.
