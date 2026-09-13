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
