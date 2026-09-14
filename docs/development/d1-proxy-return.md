# D1-N2 — Proxy dönüş sınırı

Kod **0.1.12**, mimari **v0.19**, 13 Eylül 2026, ADR-47. [Durum](status.md) · [Önceki entry gözlemi](d1-entry-boundary.md) · [Native sözleşme](../architecture/native-sdk-integration.md).

## Sorumluluk ve kanıt sınırı

Bu kesit, ayrı ve açık bir geliştirme komutuyla ana thread’in proxy CALL gövdesini ilerletir; orijinal PE girişine dönen JMP talimatında durur. Giriş baytlarının geri yüklenmesi ve GetStartupInfoA IAT hedefinin değişmesi ölçülür. Native API çağıran production launcher, SAEX bootstrap yükleyicisi veya unpack edilmiş engine profili değildir. CanAttach ve initializationVerified daima false kalır. C++20 ve bootstrap C ABI 1 korunur.

Önceki --observe-entry-boundary ana thread’in ilk PE entry hitinde durmaya devam eder. Yeni komut bu sınırı sessizce genişletmez. run_to_proxy_return yeni local C++ API’sidir; LoaderTrace genişlediği için static library ve çağıranları birlikte yeniden derlenir. Kaldırılan özellik veya kalıcı state migration’ı yoktur.

## İncelenmiş dosya ve adreslerin kaynağı

GTA executable SHA-256: `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`. Yerel vorbisfile.dll SHA-256: `bdcf32fc3961eebffb4104327ca1396daf1cbd5e736930ed247836035148dafc`. Bunlar genel oyun sürümü/ABI kimliği olarak sunulmaz.

| Alan | Kanıtlı metadata / yerel disassembly | Kullanım |
|---|---|---|
| PE entry | EXE RVA 0x424570; mevcut preflight ile aynı | İlk DR0 hedefi |
| Main_DoInit şekli | DLL RVA 0x1D60: E8 rel32 → 0x1BE0; +5’te FF25 | CALL gövdesinden sonraki durağı tanımlar |
| Dönüş talimatı | DLL RVA 0x1D65, JMP [base+0x6418] | İkinci DR0 hedefi; bu JMP yürütülmez |
| Return slot | DLL RVA 0x6418 | İçerik ilk ve ikinci hitte original entry VA olmalı |
| GetStartupInfoA IAT | EXE RVA 4555248 (0x4581F0) | Önce pinned kernel32/kernelbase executable allocation; sonra DLL base+0x1BA0 |
| Proxy IAT hedefi | DLL RVA 0x1BA0 | Pointer eşleşmesi; fonksiyon çağrılmadı |

IAT RVA, mevcut engine linkage raporundaki exact `kernel32.dll / GetStartupInfoA` import kaydından alındı; proxy RVA’ları exact dosyanın MSVC dumpbin disassembly’siyle karşılaştırıldı. `out/research/vorbisfile-disassembly.txt` yerel araştırma çıktısıdır; dağıtım veya build girdisi değildir. GTA’nın native fonksiyon adresleri tahmin edilmedi.

Referans [ASI-Loader kaynağı](https://github.com/GTAmodding/ASI-Loader/blob/main/vorbisFile.cpp), entry yönlendirmesi, PatchIAT ve geciktirilmiş LoadPlugins yolunu açıklamak için incelendi. Bu kaynak ile yerel DLL’nin exact build/commit eşitliği kanıtlanmadı; çalıştırma reçetesi yerel dosya hash’i ve gözlenen talimatlara dayanır. DLL adının/exports’un aynı olması bu davranışı kanıtlamaz. GetStartupInfoA slotunun işaret ettiği fonksiyonun prototype/calling convention doğrulaması bu ölçümün kapsamı dışındadır.

## Build-time policy

[proxy-policy.json](../../contracts/engine/proxy-policy.json), source digest üzerinden entry-policy.json’a; o da loader-policy.json ve engine hash’ine bağlanır. Yeni modül yoktur: önceki 23 pin korunur. Seçilen proxy mevcut game-root kaydının tam adı ve SHA’sı ile eşleşmelidir. Yeni proxy source digest: `20ff7fb664b56f3a43cded86faaa15743495e0cc1d9f17ccb675ccf6732fddcf`.

[Generator](../../tools/proxy_policy.py) duplicate/unknown JSON key, yanlış schema/stage/id/review, 64 KiB üstü source, parent drift, module/hash/origin uyuşmazlığı, bool/string/sıfır/negatif/taşan RVA ve hizasız dört byte slotları reddeder. RVA üst sınırı 256 MiB−16’dır; bu metadata sınırı tek başına executable region kanıtı değildir. Generated C++ static_assert entry digest bağını tekrar denetler. Normal Windows/Linux akışı yalnız --check kullanır; dosya keşfetmez, otomatik izin vermez, runtime JSON yüklemez. Policy değişimi kaynak incelemesi ve yeni kanıt ister.

## Veri akışı ve arayüz

```text
exact EXE / 23 retained DLL pin / explicit cwd+environment
  → CREATE_PROCESS: PE header/anchor ve erken main-thread DR0
  → pinned ntdll initial breakpoint: tekrar byte/register kontrolü
  → PE entry hit: E9 hedefi + suffix + proxy identity/thunk/return slot + IAT-before
  → DR0, proxy thunk+5’e taşınır; readback doğrulanır
  → ContinueDebugEvent: proxy CALL gövdesi ilerler
  → proxy dönüş hit: EIP/DR0/DR6/DR7 + aktif mapping/şekil/pointer
  → 16 entry byte restore + IAT-after
  → owned child kill, event cleanup, doğrulanmış exit
```

ProxyStopSpec yalnız tutulmuş LoaderFile pointer’ı ve beş RVA içerir. Pins listesinde olmayan/null module veya sınır dışı spec ilk continuation’dan önce reddedilir. CLI bu veriyi derlenmiş policy’den seçer; arbitrary PID/RVA veya server input girişi yoktur.

İlk entry hitinde yalnız ilk beş byte E9 rel32 olabilir; sonraki 11 byte dosya görüntüsüne eşit olmalıdır. Hedef tam olarak aktif ve admitted proxy mapping’in thunk RVA’sıdır. CALL rel32 ve JMP absolute operandı ASLR’lı gerçek base ile doğrulanır; return slot entry VA taşır. CALL ve IAT-target adreslerinin committed, nonguard executable MEM_IMAGE bölümü olduğu ayrıca kontrol edilir. Runtime remote read tek bölge ve en fazla 16 byte’tır; allocation base doğrulanır, pointer zinciri keyfî takip edilmez.

İlk IAT pointer’ı admitted ve active kernel32/kernelbase executable allocation’ına düşmelidir. Bu bölge kontrolü export adı veya çağrı ABI’si kanıtı değildir. İkinci hitte exact yeni pointer değeri, entry restorasyonu ve aynı proxy mapping/şekil tekrar aranır. Proxy mapping arada unload olursa terminal ret olur; remap eski izinle devam edemez.

Observer yalnız main-thread debug register’ını değiştirir; child code, IAT, IP, stack veya EFlags yazmaz. Entry/IAT yazımlarını incelenen proxy ve kendi test fixture’ımız yapar. İkinci hitte dönüş talimatı yürütülmeden child kapatılır. Debugger register erişimi ve event continuation Windows API’si ile yapılır. [Microsoft GetThreadContext](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext), [SetThreadContext](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadcontext). Hardware execution breakpoint semantiği için [Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) referans alınır; gerçek durma noktası fixture canary ve register kontrolleriyle ayrıca sınanır.

## CLI ve rapor sözleşmesi

```powershell
./out/windows-x86/Debug/saex_engine_loader_probe.exe --observe-proxy-return "<absolute-exe>" "<absolute-cwd>"
```

Bu komut native DLL/TLS/proxy kodu yürütür. Standart build yalnız kendi test programlarını çalıştırır; GTA deneyi ayrı çağrılır.

Scope `bounded-proxy-return-observation`, schemaVersion 1. Root policy önceki mapping kimliğidir; entryObservation entry izin kimliğini ve ilk hitin baytlarını tutar. ProxyObservation yeni yürütme izninin kimliğini/digest’ini taşır; eski komutlarda null’dır.

| Alan | Anlam |
|---|---|
| proxyExecutionAllowed | Yeni mod seçildi; gerçekleşmiş yürütme kanıtı değil |
| validated / breakpointArmed | İlk hit predicate’leri ve ikinci hedef readback başarılı |
| continued | İkinci hedef kurulduktan sonra ContinueDebugEvent başarılı |
| returnReached | Beklenen main-thread ikinci hardware hit kimliği doğrulandı |
| moduleBase / mappingId / returnAddress | Bu gözlemdeki canlı eşleme ve ikinci durak |
| entryRestored / entryAfterHex | İkinci durakta 16 byte okundu ve ilk görüntüyle karşılaştırıldı; false ise hex’i başarı kanıtı sayma |
| iatRva / iatBefore / iatAfter / iatVerified | Reçetedeki slot ve iki pointer; yalnız iatVerified olumluysa exact yeni hedef doğrulandı |

Exit 3 yalnız `reason=proxy_return_verified` ve childExitConfirmed birlikteyse döner. Entry/IAT/şekil uyuşmazlığı, beklenmedik exception veya budget/cleanup problemi exit 1; kullanım hatası exit 2. Eski entry CLI’da modified entry tanısal complete olabilir; yeni proxy CLI’da bu tek başına başarı değildir. Bütün pozitif raporlarda bile canAttach/initializationVerified false’tur.

## Hata davranışı ve güven sınırı

128 event, 64 lifetime module, 16 lifetime thread, 5 saniye observer bütçesi ve 256 MiB toplam dosya okuma sınırı korunur; unload bütçe iadesi yapmaz. I/O çağrıları hard wall-clock deadline değildir. Bilinmeyen debug olayı/exception terminaldir; EXIT_THREAD desteği sessizce eklenmedi. Foreign-owner çağrı pending olayı değiştirmez, owner cleanup yapabilir. Normal bitişte kill, pending continuation’dan önce gelir ve process exit doğrulanır; terminal child yeniden kullanılamaz.

Bu bir güvenlik sandbox’ı değildir. Native DLL’ler başka thread oluşturabilir, bellek/debug state değiştirebilir veya yan etki üretebilir. Main-thread durma kanıtı tüm thread’lerde motor kodu hiç yürütülmediğinin kanıtı değildir. Sayılan pin/byte predicate’leri bütün DLL gövdesinin runtime değişmezliğini veya orijinal Windows dispatch export’unu ispatlamaz. Kanıt exact artifact, OS, engine/DLL hash ve context kapsamındadır.

## Testler ve ilk başarısızlık

Sekiz own EXE/DLL varyantı: normal, no_restore, bad_iat, fault, stall, bad_return, bad_redirect, bad_suffix. DLL initializer entry’yi yönlendirir; proxy gövdesi ayrı marker üretir; main marker’ı hiçbir observer koşusunda oluşmamalıdır. Fixture diskteki export/import metadata’sından kendi RVA’larını okur; test runner içine DLL yüklenmez. Helper yalnız kendi fixture metadata’sı için private byte kopyasında DLL bitini temizleyerek mevcut EXE layout parser’ını kullanır; pinned dosya değişmez ve production parser genişletilmez.

19 senaryo; ayrıca 12 warm çevrim, handle sayısı önce/sonra eşitliği ve her koşuda terminal reuse/confirmed exit denetimi. Eksik restore, yanlış IAT, proxy fault/stall, değişen return pointer/redirect/suffix, null/eksik pin, yanlış thunk/call target, taşan slot, nonsystem IAT, event budget, eski entry modu ve foreign-owner geri dönüşü sınanır. Python’da yedi policy testi ve iki yeni CLI testi vardır; CLI corpus toplamı 13’tür.

İlk native Debug test runner `0xC00000FD` stack overflow ile başarısız oldu (`test-proxy-first.log`). MSVC /Od disassembly’sinde her finish dönüşünde büyük LoaderTrace geçicileri ayrılıyordu. İç finish helper const referans döndürecek biçimde düzeltildi; dış public API değer döndürür, stack limiti büyütülmedi. İncelenen run_impl frame rezervasyonu 0xAC1B4 byte’tan 0x4E5C byte’a düştü; diğer çağrı frame’leri buna dahil değildir. Sonraki `test-proxy-stack-fix.log` 16 eski senaryo+12 warm geçti; genişletilmiş `test-proxy-corpus.log` 19+12 geçti. İlk hata saklanır ve başarılı koşu diye sunulmaz.

## Doğrulama kaydı — tamamlanan yerel alt kapsam

Windows 10.0.26200; mevcut VS2022/MSVC19.44, CMake4.3.3, Windows SDK10.0.26100 ve .NET10 çalışma akışı kullanıldı. Dört standart tools/build.ps1 akışı geçti:

| Profil | Native CTest suite | Managed/entegrasyon | Python |
|---|---|---|---|
| Windows x86 Debug | 10/10 | 79/79 | 76/76 |
| Windows x86 Release | 10/10 | 79/79 | 76/76 |
| Windows x64 Debug | 6/6 | 79/79 | 53/53 |
| Windows x64 Release | 6/6 | 79/79 | 53/53 |

X86 akışları bootstrap artifact audit ve oyun dışındaki 100 DLL lifecycle çevrimini de içerir. Proxy suite 19 senaryo+12 warm çevrimdir; Debug genişletilmiş corpus’ta handle 133→133, Release standart corpus’ta 137→137 ölçüldü. Eski entry/loader/owner/context testleri de geçti. Loglar `out/verification/engine/build-proxy-<architecture>-<configuration>.log` biçiminde saklanır. Bu kesit için Linux işletim sistemi ve hosted CI sonucu ölçülmedi; N1 opt-in SDK yeniden derlenmedi.

Son gerçek deney kaydı `out/verification/engine/proxy-return-gta-evidence.json`: **6 Debug + 6 Release = 12/12** private GTA koşusu proxy_return_verified/exit 3 verdi. Her birinde yönlendirme doğrulandı, ikinci DR0 hedefi kuruldu, proxy gövdesi ilerledi, returnReached/entryRestored/iatVerified true oldu. Orijinal girişe dönen JMP ana thread üzerinde çalışmadan child kapatıldı.

İkinci durak her koşuda vorbisfile.dll base+0x1D65; IAT-after base+0x1BA0. Entry-after her koşuda ilk görüntünün aynısı: `6a606878808800e864410000bf940000`. İlk durağın entry bytesMatch=false verisi ayrı korundu. ASLR nedeniyle base/pointer değerleri farklıdır; karşılaştırma RVA ve canlı mapping kimliğiyle yapıldı. Altı koşuda 31, bir koşuda 33, beş koşuda 29 debug event; toplam sekiz bilinen unload işlendi ve bütün pozitif koşularda 22 aktif DLL vardı. 33-event koşusunda iki unload görülmesi yeni izin veya bütçe iadesi yaratmadı.

Aynı Release artifact’ıyla üç regresyon daha yapıldı: orijinal kurulum AcLayers mapping’inde loader_module_not_pinned/exit 1 ve proxy continued=false; private eski entry komutu entry_boundary_modified/exit 3; private eski context komutu loader_breakpoint_candidate/exit 3. İki legacy raporda proxyObservation=null. Toplam **15/15 child exit doğrulandı**. İlk tek Debug deneyi bu 15 koşuya ek ayrı kayıttır (`proxy-gta-Debug-first.json`).

Orijinal kurulumdaki 13 native dosya ve private deneydeki üç dosyanın SHA-256 değerleri başta/sonda eşitti. Özel kopya eski üç dosyalı deney dizinidir; ASI veya yeni codec/SAEX DLL eklenmedi, staging/rename yapılmadı. Oyun verisi içermediğinden oynanabilir kurulum değildir. Environment snapshot private koşularda 60 kayıt/4132 UTF-16 code unit; digest `3725a091ed77f875b48ab336a8162328429aeebc82f2c4779f897ed54f62407e`. Ham environment key/value kaydedilmedi.

| Probe artifact | SHA-256 |
|---|---|
| x86 Debug saex_engine_loader_probe.exe | 28f091df66303228445f29ec367483830222dd198d2ad9484541c79cfd3f46cc |
| x86 Release saex_engine_loader_probe.exe | 12ad29c4655b47a8372a7f353cc08ded191903a06544434d1b69430398f915e1 |

Retained pin/hash, gözlem source digest’leri ve artifact kimliği ayrı tutulur. Artifact veya işletim ortamı değişirse bu run kayıtları otomatik taşınmaz. SAEX bootstrap gerçek GTA içinde yüklenmedi; engine initialization/calling convention/unpack/WinMain/frame veya multiplayer doğrulanmadı.

## Sonraki genişleme ve kabul

GetStartupInfoA yönlendirmesinin çağrılma zamanı, dinamik vorbishooked/codec/ASI yolu ve unpack safhası sonraki N2 kesitidir. Ardından gerçek SAEX bootstrap C ABI’sinin loader lock dışında Initialize/Query/Stop akışı kanıtlanır. IAT’ye bakılması ilgili wrapper’ın veya LoadPlugins’in çalıştığı anlamına gelmez. Asıl PE girişine dönüş, WinMain/frame, SDK fonksiyon çağrısı ve indirilen C# sandbox bu kesitte açılmaz. AC-90/R-01 ve N2/D1 kapıları bu alt sonuçla kapanmaz.

## Kod 0.1.13 — Startup çağrı sınırı

[Yeni startup-call modu](d1-startup-call.md) yalnız ayrı API/CLI ve source izniyle ikinci duraktan devam eder. run_to_proxy_return ikinci hitte terminal kalır; proxyObservation önceki ölçümün anlamını korur. Kendi fixture PE/marker yardımcıları proxy_fixture_support.hpp’ye taşındı; proxy_iat_target test escape marker’ı + exit 90 üretir. Bu canary gerçek GetStartupInfoA implementasyonu değildir; eski proxy testleri gövdeye girmeden durmayı sürdürür.

## Kod 0.1.14 — Codec dönüş kesiti

Eski proxy komutu ikinci durakta kalır. Yeni codec modu üç eski kapıdan sonra farklı DR0 hedefine geçer; proxy mapping retirement ve entry/IAT doğrulamaları korunur. [Sözleşme ve doğrulama](d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

Proxy komutu ikinci durakta kalır; binding izni kendi parent zinciriyle ayrıca açılır. Proxy mapping/thunk ve EXE IAT eşitliği beşinci durak öncesi/sonrası yeniden denetlenir. [Sözleşme ve kanıt](d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Proxy restorasyon ve IAT denetimi yeni frame aday modunun da önkoşuludur. Yeni okuma hedefi proxyye ek yürütme izni vermez; eski proxy CLI davranışı korunur. [Aday ve doğrulama raporu](d1-frame-target.md).

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
