# D1-N2 — DLL başlangıcından sonra PE girişinde kontrollü duruş

Mimari v0.18 / kod 0.1.11, 13 Eylül 2026. [Durum](status.md) · [N2 planı](d1-engine-integration.md) · [Native sözleşme](../architecture/native-sdk-integration.md) · [Statik linkage](d1-native-linkage.md) · [ADR-46](../decisions/architecture-decisions.md).

## Sorumluluk ve yürütme sınırı

[LoaderObservation::run_to_entry](../../src/engine/loader/loader_observation.cpp), owned x86 child'ın DLL/TLS başlangıcını açık deney kapsamında ilerletir; ana thread PE girişine ulaştığında execution hardware breakpoint ile durur ve child'ı kapatır. Bu, ilk exception'da duran eski `run()` davranışından ayrı bir yürütme iznidir. Eski üç CLI komutu ve statik araçların sınırları korunur.

Yeni açık komut:

```text
saex_engine_loader_probe --observe-entry-boundary <gta_sa.exe> <absolute-working-directory>
```

CLI yalnız mevcut exact engine preflight/hash/layout/anchor profili ve retained file ID/hash pinleri üzerinden child yaratır. Caller RVA, fonksiyon adresi, policy JSON veya arbitrary PID sağlayamaz. Entry RVA ve ilk 16 byte doğrulanmış dosyanın PE layout'undan alınır. C++ test API'si güvenilir yerel test caller'ına EntryStopSpec verir; worker/server API'si değildir.

`entryObservation.executionPolicy=gta-sa.f01a00ce.windows-26200.entry-boundary-v1`, [entry-policy.json](../../contracts/engine/entry-policy.json) üzerinden ayrı derlenen deney kararıdır; executionPolicySourceDigest ve probe artifact SHA-256 birlikte raporlanır. Mevcut `policy`/`policySourceDigest` alanları **mapping pin kaynağını** göstermeye devam eder. First-exception-only yazan eski JSON recipe, tek başına initializer izni vermez; yeni flag, ayrı scope ve ADR-46 bu sınırlı izni sağlar. 22 DLL'lik eski JSON/generator değişmez. Yeni izin bu exact base digest'ine bağlıdır; ayrıca incelenmiş imm32.dll system-x86 kaydıyla entry modunda 23 pin hazırlanır. Unknown/unpinned DLL otomatik eklenmez; ASI taraması veya oyun dosyası değişikliği yapılmaz.

## Veri akışı

1. Explicit cwd/environment ve tüm engine/DLL dosyaları child öncesi doğrulanır. İlk CREATE_PROCESS debug olayı tutulur; engine file ID, base/header/anchor eşleşir.
2. EntryStopSpec sıfır/aralık/16 byte bakımından doğrulanır. Hedef committed, executable, guard olmayan ve child engine'e ait MEM_IMAGE bölümünde olmalıdır. Image/girdi boyutları mevcut sınırlardadır.
3. **İlk CREATE_PROCESS olayı henüz tutulurken** owned main thread debug register durumu okunur. Kullanılmış DR0–DR3/DR7 slotları veya trap flag reddedilir. Yalnız DR0/DR6/DR7 ayarlanır; DR0 PE entry adresi, DR7 slot0 local execute/length0 olur. SetThreadContext sonrası readback zorunludur. IP, stack veya instruction byte'larına observer yazmaz.
4. Bounded loader döngüsü bütün LOAD için file ID/hash ve yeni mapping kimliği; known UNLOAD için emeklilik uygular. İlk exception yalnız mevcut koşullarla ana thread/pinned-active ntdll MEM_IMAGE breakpoint adayıysa değerlendirilebilir. Entry byte'ları henüz değişmemiş ve hardware register readback doğru olmalıdır; aksi halde initialization'a devam edilmez.
5. Bu kontrol sonrası bir ContinueDebugEvent başarılı olursa `initialBreakpointContinued=true` olur. Bu noktadan itibaren DLL/TLS/OS initializer kodu çalışabilir ve yan etki üretebilir. Her sonraki LOAD yine pin kontrolünden geçer. Kota, beklenmeyen olay veya exception terminaldir.
6. Beklenen son olay: main thread, first-chance EXCEPTION_SINGLE_STEP, exception address=entry, EIP=entry, DR0=entry, DR6 yalnız B0 sebebi ve DR7 yalnız gerekli execute slotu. Sıradan DebugBreak, single-step/trap nedeni veya başka thread/adres aynı sonuç sayılmaz.
7. Duruşta entry'nin 16 byte'ı yeniden okunur. Aynıysa `entry_boundary_reached`, değişmişse `entry_boundary_modified`; ikisi ayrı gözlem sonucudur. Değişen kod yürütülmez/restore edilmez. Child tutulurken TerminateProcess/job cleanup başlar; terminal debug olayı ancak kill istendikten sonra sürdürülür ve exit doğrulanır. DR durumunu restore edip process'i serbest bırakma yolu yoktur.

## Arayüzler, hatalar ve bütçeler

Yeni scope `bounded-entry-boundary-observation`, schemaVersion=1. Entry olmayan komutlarda additive `entryObservation=null`; scope/reason/exit davranışları korunur. Yeni nesne executionPolicy/executionPolicySourceDigest, dllInitializationAllowed, breakpointArmed, initialBreakpointContinued, boundaryReached, address, bytesRead, bytesMatch, beforeHex/afterHex taşır. `dllInitializationAllowed=true` seçilen kapsamdır; fiilen geçiş için `initialBreakpointContinued` gerekir. `afterHex` yalnız bytesRead=true iken sonuç verisidir; sıfır varsayılanları kanıt sayılmaz. Diğer sayaçlar/mapping history mevcut sözleşmeyi korur.

Exit 3: entry donanım olayı doğrulandı, post byte'ları okundu ve owned exit onaylandı. Byte'ların değişmiş olması bu tanısal alt sonucu bozmaz; runtime uygunluğu anlamına da gelmez. Exit 1: preflight/context/pin/entry/register/olay/timeout/read/cleanup eksikliği; exit 2: yanlış CLI kullanımı. `canAttach=false`, `initializationVerified=false` bütün sonuçlarda sabittir. DLL/TLS başlangıcının ilerlediği gözlemi, initialized GTA/WinMain/unpack/oyun döngüsü/SAEX DLL/ABI kanıtı değildir.

128 event, 64 lifetime mapping, 16 oluşturulan thread, 256 MiB toplam hash okuması ve 5 saniye observation bütçesi; cleanup ayrıca 5 saniyedir. Testte stall bütçesi 1 saniyedir. Existing unknown/EXIT_THREAD olayları terminal ret kalır; thread exit desteği örtülü eklenmez. Senkron disk işlemleri katı duvar süresi garantisi vermez. DLL initializer kendi process'inde kod çalıştırdığı için bu deney bir sandbox değildir; kalıcı disk/registry yan etkisi rollback edilmez. Hardware breakpoint'i temizleyen/bypass eden düşmanca kod, initializer'ın başka thread'de engine fonksiyonu çağırması veya bütün CPU/OS varyantları engellenmiş sayılmaz. Başarı, kayıtlı engine/OS/probe/context ve fixture corpus'u için ölçülen ana-thread sınırıdır.

## Kaynak incelemesi ve deney gerekçesi

Mevcut vorbisFile.dll SHA-256 `bdcf32fc3961eebffb4104327ca1396daf1cbd5e736930ed247836035148dafc`. Yerel dumpbin incelemesi RVA 0x1D70 çevresinde reason==1, ana modül header'ından entry hesaplama, VirtualProtect, beş byte yedekleme ve E9 ile RVA 0x1D60 yoluna yönlendirme gösterdi. Bu adresler SAEX çağrı/binding API'sine eklenmez; disassembly gözlemidir. Bağımsız [ASI-Loader kaynak yolu](https://github.com/GTAmodding/ASI-Loader/blob/main/vorbisFile.cpp) giriş/IAT uyarlaması ve sonraki GetStartupInfo üzerinden plugin yükleme düzenini açıklar. Kaynak dalı ve yerel DLL için exact build eşitliği kanıtlanmadığından upstream sürüm kimliği çıkarılmaz; üçüncü taraf kod SAEX'e kopyalanmadı.

Bu nedenle yalnız dosya export eşleşmesiyle tam başlangıca geçmek yeterli değildir. Entry gözlemi, başlangıç rutinleri sırasında executable'ın değişip değişmediğini ortaya koyar; proxy'nin dinamik vorbishooked/ASI yolu entry sonrasındaysa bu kesit onu yürütmez. DLL'yi değiştirme/rename, yeni codec ABI veya SAEX bootstrap injection kararı ayrıca kanıt gerektirir.

## Test corpus'u ve ilk başarısızlıklar

[Entry testleri](../../tests/engine/entry_observation_tests.cpp), kendi fixture EXE/DLL/TLS marker'larıyla 12 senaryo ve 12 warm çevrim çalıştırır. Normal DLL/TLS marker'ları oluşmalı, main marker'ı oluşmamalıdır. [Üç fixture varyantı](../../tests/engine/entry_fixture.cpp) entry'nin ilk byte'ını 0xCC yapar, TLS içinde DebugBreak verir veya TLS içinde süresiz bekler. Mutasyonda HW fault değişmiş 0xCC yürütülmeden yakalanmalı; hata ve stall başarı sayılmamalı, child çıkışı doğrulanmalıdır. Entry byte uyuşmazlığı, zero/end range, data section, eksik pin, event budget, yanlış owner thread, terminal tekrar çağrı ve eski ilk-exception modu ayrıca sınanır. Warm handle sayısı yalnız bu corpus kapsamında ölçülür.

İlk deneme başarısızdı: hardware register'ları ilk ntdll breakpoint'inde ayarlayan uygulama fixture main'ini tutamadı; sonra kernel.appcore unpinned olayında durdu. Bu sonuç GTA üzerinde denenmedi. Register kurulumu ilk CREATE_PROCESS olayına taşındı; ntdll noktasında yalnız readback/byte kontrolü bırakıldı. Aynı fixture bundan sonra entry noktasında EXCEPTION_SINGLE_STEP ile tutuldu. Geç kurulumun kaybolmasının tam OS iç nedeni bu kesitte kanıtlanmadı; NtContinue gibi bir özel symbol tahmin edilip patchlenmedi.

İkinci corpus koşusunda yeni fixture'ın apphelp mapping'i mevcut üç-system-pin seti dışında kaldı; güvenli ret oluştu. Test, zaten incelenmiş recipe'nin 20 system-x86 basename'ini açık set olarak ve fresh retained dosya kimlikleriyle hazırlayacak biçimde güncellendi. GTA CLI exact hash recipe'si genişletilmedi. Eksik kendi fixture DLL pini hâlâ initialization öncesi ret verir; wildcard/system-directory genel kabulü yoktur. Bu düzeltmeden sonraki hedefli entry corpus'u geçti; son standart akış ve gerçek GTA sonuçları aşağıda kayıtlıdır.

İlk loglar: `out/verification/engine/test-entry-first.log`, `test-entry-create-arm.log`, `test-entry-fixture-pins.log`. Başarısız sonuçlar tarihsel kanıt olarak korunur; ilk testleri geçmiş gibi gösteren yeniden yazım yapılmaz.

## Kabul, genişleme ve kalan kapılar

AC-90'ın DLL başlangıcı/entry-boundary alt kapsamı; R-01a/b/c araştırmasıyla birlikte değerlendirilir. Bütün AC-90, N2 ve D1 kapanmaz. Sonraki kontrollü çalışma, tespit edilen proxy başlangıç yönlendirmesini ve engine unpack safhasını ayrı ele almalı; gerçek SAEX bootstrap C ABI çağrısı loader lock dışında ve exact artifact/phase kanıtıyla yapılmalıdır. N3 frame/hook, N4 Host/IPC ve N5/N6 entity/asset sırası korunur. GNS/HTTPS ve C# sandbox kararları değişmez.

[Microsoft initial breakpoint](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/initial-breakpoint) statik DLL mapping ve initialization arasındaki debugger sınırını tanımlar. [Thread context işlemleri](https://learn.microsoft.com/en-us/windows/win32/debug/thread-functions-for-debugging), [SetThreadContext](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadcontext) ve [Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) register/execute fault düzeninin birincil kaynaklarıdır. Bunlar SAEX fixture veya GTA koşusunun geçtiği iddiasının yerine geçmez.

## İlk gerçek initializer deneyi ve ek pin sözleşmesi

İlk Debug özel-kopya koşusunda hardware guard kuruldu, ilk breakpoint devam ettirildi; başlangıç sırasında imm32.dll eski 22 pin dışında kaldı. Beklenen loader_module_not_pinned/exit 1 ve doğrulanmış child exit alındı; boundaryReached=false. Yerel `entry-gta-Debug-first.json` bu sonucu saklar; kontrol kaldırılmadı.

SysWOW64/imm32.dll: 148344 byte, sürüm 10.0.26100.8521, SHA-256 `c69ca53458deb7609f9a879b356f62330be743343aaa08afd055f15fa4fe2a95`. Yerel dumpbin import/delay incelemesi, User32/API-set ilişkileri ve Valid/Microsoft Windows Authenticode sonucu sınırlı initialization deneyi için değerlendirildi. İmza, initializer davranışının tam güvenlik kanıtı değildir.

[entry_policy.py](../../tools/entry_policy.py) en fazla 64 KiB source, schema 1/stage=pe-entry-boundary ve exact basePolicySha256/engineSha256/reviewDocument bağı ister. additionalModules 1–8 kayıt ve yalnız system-x86 olabilir; base modülünü override/duplicate edemez. Birleşimde mevcut isim/hash/byte/sıra ve 64 modül/256 MiB kuralları uygulanır. [Generated header](../../include/saex/engine/entry_policy.generated.hpp) bütün 23 pin ve iki source bağını taşır; base digest için constexpr static_assert vardır. Source runtime/server'dan okunmaz. CLI tüm birleşik dosyaları child öncesi doğrular; eski komutlar aynı 22/3 setleri kullanır.

[Sekiz generator testi](../../tests/engine/test_entry_policy.py) exact output, base/engine drift, stage/bool schema, unknown/duplicate keys/size, origin/override, path/hash/byte, count/order ve id/review retlerini sınar. Standart build iki generator'a --check yapar ve sekiz yeni testi çalıştırır. İlk ek-pinli Debug GTA koşusu entry_boundary_modified/exit 3 verdi; E9 gözlendi ve child exit doğrulandı. Eski base recipe SHA'sı değişmedi. Son tekrar/artifact kanıtı aşağıda kayıtlıdır.

## Son doğrulama — kod 0.1.11

| Standart akış | Native CTest suite | Managed/entegrasyon | Python | Sonuç |
|---|---:|---:|---:|---|
| Windows x86 Debug | 9 | 79 | 59 | Geçti |
| Windows x86 Release | 9 | 79 | 59 | Geçti |
| Windows x64 Debug | 6 | 79 | 38 | Geçti |

X86 corpus 12 entry senaryosu ve 12 warm çevrim içerir; warm handle sayısı değişmedi (son Release 136→136). Bootstrap'ın oyun dışı 100 lifecycle çevrimi/artifact audit ve eski loader/kimlik/kota/cleanup regresyonları da standart akışta geçti. İlk hedefli başarısızlıklar yukarıda ayrı tutulur. X64 Release/Linux koşulmadı; N1 SDK kaynak/recipe değişmediği için opt-in SDK build tekrarlanmadı. Loglar: `out/verification/engine/build-entry-x86-Debug.log`, `build-entry-x86-Release.log`, `build-entry-x64-Debug.log`.

Gerçek deney, önceki üç dosyalı private kopyada aynı explicit workspace cwd ile yapıldı. Final matris 6 Debug + 6 Release private entry koşusu, 1 Release original entry ret ve 1 Release private eski context kontrolüdür. `out/verification/engine/entry-boundary-gta-evidence.json` source/probe/hash/context/exit/target bilgilerini; her ayrı `entry-gta-<Configuration>-<n>.json` tam trace'i taşır.

| Gerçek gözlem | Sonuç |
|---|---|
| Private Debug 6/6 ve Release 6/6 | entry_boundary_modified, EXCEPTION_SINGLE_STEP, exit 3, ana-thread EIP/exception/DR0=PE entry |
| Entry adresi | 0x00824570 = image base 0x00400000 + dosyadan entry RVA 0x00424570 |
| Başlangıçtan önce 16 byte | `6a606878808800e864410000bf940000` |
| Başlangıçtan sonra | İlk 5 byte E9 rel32; son 11 byte aynı; rel32 ASLR'ye göre değişir |
| Atlama hedefi | 12/12 active ve pinned vorbisfile.dll base + RVA 0x1D60; yerel disassembly ile aynı |
| Mapping history | 9 koşuda 30 event/23 LOAD/1 UNLOAD; 3 koşuda 28 event/22 LOAD/0 UNLOAD; her koşuda 22 aktif DLL |
| Original Release | AcLayers unpinned ret; initialBreakpointContinued=false, exit 1 |
| Private legacy context Release | İlk breakpoint adayı/exit 3; entryObservation=null |
| Cleanup ve dosyalar | Son matriste 14/14 owned child exit; original 13 + private 3 native hash'i önce/sonra ve 0.1.10 kanıtıyla aynı |

Bu sonuç, seçilmiş DLL başlangıcı sırasında entry yönlendirmesinin gerçekten değiştiğini doğrular. Atlamanın kendisi, onun çağıracağı IAT/proxy yükleyicisi, vorbishooked/ASI taraması, oyun unpack/WinMain ve SAEX bootstrap bu deneyde çalıştırılmadı. Initializer'ların başka yan etkilerini eksiksiz izlediğimiz veya bütün thread'lerde engine kodunu engellediğimiz iddia edilmez. İlk imm32 ret ve ilk ek-pinli Debug örneği son 14 koşuluk matristen ayrı, aynı dizinde saklanır.

| Kanıt kimliği | SHA-256 |
|---|---|
| Base mapping source (değişmedi) | `91bb9479a01a91fea9161939ed7ac063fce541c9bb9bd35e1a4c3c8e4dce49a8` |
| Entry policy source | `56328c7da0b0de6f3584e526b5fae3484634af927ce3a2cc8ace311112166cb2` |
| Debug loader probe | `f23d88564699623c51cf47bbdb2304cb7ae73949cb8563fc793b11e6a8447f3b` |
| Release loader probe | `8ef37d29438a81b9816ff0e3db5f73208cd4c24170719c476b90bf19d36c25bd` |

Environment digest bütün private koşularda `3725a091ed77f875b48ab336a8162328429aeebc82f2c4779f897ed54f62407e`; raw değerler raporlanmadı. Entry source ve probe değişirse bu sonuç yeni artifact'a otomatik taşınmaz. N2 sonraki adımı entry sonrası proxy/unpack ve loader lock dışında gerçek SAEX bootstrap/ABI kanıtıdır; D1/N3 ve multiplayer hâlâ açık kapılardır.

### Hosted corpus tamamlaması

İkinci hosted turda bütün native suite'ler geçti; ortak Python loader CLI corpus'undaki cwd string eşitliği kısa/uzun Windows adı nedeniyle hata verdi. Test artık pathlib.samefile ile aynı dizin kimliğini doğrular. Engine unknown-fingerprint reddi, environment redaction, childCreated=false, entry/loader policy ve lifecycle sınırları aynen kalır; önceki GTA artifact sonuçları yeni test kanıtı sayılmaz.

## Kod 0.1.12 — Proxy dönüş sınırı

[Yeni proxy-return modu](d1-proxy-return.md) yalnız yeni API/CLI ve executionPolicy ile ilk entry hitini geçebilir. Mevcut run_to_entry ve --observe-entry-boundary burada tarif edilen ilk hitte bitmeye devam eder. Yeni mod ilk entry_before/entry_after alanlarını korur, restorasyonu ayrı proxy_entry_after alanında verir. CREATE_PROCESS’te erken DR0 kurulumu değişmez; yeni ikinci hedef call thunk’ının sonundaki JMP’dir.

## Kod 0.1.13 — Startup çağrı sınırı

[0.1.13 startup-call](d1-startup-call.md) ilk entry görüntüsünü ve erken CREATE_PROCESS DR0 kurulumunu yeniden kullanır. run_to_entry bu ek izin olmadan ilk hitte durur. Üçüncü modun entryExecutionAllowed bayrağı gerçekleşmiş başlangıç kanıtı değildir; continued ancak ikinci duraktan başarılı ContinueDebugEvent sonrası true olur.

## Kod 0.1.14 — Codec dönüş kesiti

Eski entry komutu PE entry durağını korur. Codec modu önce bu kapıyı, ardından proxy/startup kontrollerini geçer; yeni gövde yürütme izni entry moduna taşınmaz. [Sözleşme ve doğrulama](d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

Entry komutunun erken DR0 sınırı korunur. Beşinci binding durağı yeni komuttadır; ilk entry gözlemi codec veya ASI yürütme iznine dönüşmez. [Sözleşme ve kanıt](d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Frame modunun ilk EXE örneği CREATE_PROCESS beklerken, loader ilerlemeden alınır. Entry DR0 reçetesi değişmedi; native frame hedefinde breakpoint kurulmaz. [Aday ve doğrulama raporu](d1-frame-target.md).

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
