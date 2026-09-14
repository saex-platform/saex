# D1 — CRT mevcut kilidi alma ve dönüş

Kod 0.1.31 / mimari v0.38. Mevcut kilit acquire ve CRT dönüşü kesiti doğrulandı. [Durum](status.md) · [Önceki sınır](d1-cwd-lock.md).

## Sorumluluk ve veri akışı

`--observe-cwd-acquire <exe> <absolute-cwd>` önceki CMP/JNE terminalinden dolu slot dalını açar, doğal EnterCriticalSection çağrısı ve CRT selector dönüşünü izler. Terminal 0x836EA4'tür; wrapper'ın `POP ECX`, SEH durum değişikliği ve cwd helper çağrısı henüz çalışmaz. Yeni izin yalnız mevcut, boşta ve wait/semaphore izi bulunmayan kritik bölüm için geçerlidir. Lazy initialization açılmaz.

S, önceki lock terminali 0x82ADCF'deki ESP'dir: S=Q−16=C−116. Başlangıç EAX=7, EBX=0, EDI=24, ESI=slot adresi, EBP=S+4, ZF=0. Önceki SEH EBP'si Q+40=S+56, FS:[0]=Q+24=S+40 olarak saklanır.

| Durak | Adres | ESP | Davranış |
|---|---|---|---|
| 1 | 0x82ADE4 | S | JNE doğal alındı; PUSH henüz çalışmadı |
| 2 | 0x82ADE6 | S−4 | Argüman slotta gözlenen nesne adresi; Win32 CALL öncesi |
| 3 | admitted ntdll base + 0x542D0 | S−8 | Doğal API entry; stack'te 0x82ADEC dönüşü ve nesne adresi |
| 4 | 0x82ADEC | S | API döndü, stdcall argümanı temizledi; sahiplik readback |
| 5 | 0x836EA4 | S+12 | POP ESI / POP EBP / RET tamamlandı; wrapper argüman 7 hâlâ stack'te |

Dalın 11-byte gövdesi `ff36 ff15 ec808500 5e 5d c3` olarak gerçek executable'dan alınır. İlk JNE ve öncesi selector kanıtı önceki moddan gelir. Win32 CALL dışında ek GTA fonksiyonu çağrılmaz. Slotun boş olduğu durumda JNE yürütülmeden ret verilir; flags ve pointer uyuşması önceki checkpoint'te doğrulanır. Her sonraki durakta slot penceresi tekrar karşılaştırılır.

## Arayüz ve Windows sınırı

[CwdAcquireSpec](../../include/saex/engine/cwd_acquire.hpp), IAT RVA, pinned Windows export prefix'i ve wrapper return prefix taşır. [Portable doğrulayıcı](../../src/engine/loader/cwd_acquire.cpp), admitted base ile callsite üretimi, non-overlap, ABI ve nesne geçişi sözleşmesini uygular. [Strict policy](../../contracts/engine/cwd-acquire-policy.json) ve [generator](../../tools/cwd_acquire_policy.py) lock policy SHA zincirine ve loader policy içindeki ntdll dosya kimliğine bağlanır. Bilinmeyen executable/Windows kimliği ret verir; yeni adresler CLI'dan alınmaz.

Statik PE incelemesi: GTA IAT 0x8580EC, KERNEL32.dll!EnterCriticalSection importudur. Bu sistemin kernel32 export forwarder'ı `NTDLL.RtlEnterCriticalSection`; ntdll RVA 0x542D0'dur. Runtime'da IAT target == aktif/admitted ntdll mapping base + RVA ve 20-byte giriş gövdesi eşleşmelidir. Mapping/file kimliği mevcut loader pinleriyle denetlenir; her durakta IAT/prefix yeniden kontrol edilir. İlk 20 byte'ın HIGHLOW fixup'ı yoktur. Statik inceleme kaydı `out/verification/engine/cwd-acquire-static.json` içindedir.

Windows API, başarıda sahipliği çağıran thread'e verir ve **VOID** döner; EAX'ten başarı kodu türetilmez. [Microsoft EnterCriticalSection sözleşmesi](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-entercriticalsection). İncelenen GTA çağrı gövdesi ile güvenilen, dosya kimliği pinlenmiş OS uygulaması ayrıdır. Ntdll'nin bütün iç yardımcıları instruction-level denetlenmiş sayılmaz; bu Windows API yürütmesinin sınırıdır. Production hook, sandbox veya genel Windows uyumluluk vaadi değildir.

## Nesne ve ABI sözleşmesi

Windows SDK 10.0.26100.0 `winnt.h` içindeki x86 CRITICAL_SECTION düzeni static_assert ile doğrulanır: 24 byte, altı DWORD; DebugInfo/LockCount/RecursionCount/OwningThread/LockSemaphore/SpinCount. Bu alanlar taşınabilir protokol veya kalıcı asset verisi değildir. Özellikle LockCount -1 → -2 beklentisi bu pinned Windows uygulamasının uncontended davranışına aittir.

Nesne adresi DWORD hizalı, taşmasız, writable ve committed MEM_PRIVATE veya aynı admitted GTA AllocationBase içindeki MEM_IMAGE içinde olmalı; canlı stack veya TIB aralığıyla çakışmamalıdır. Okuma 24 byte ile sınırlıdır. Private nesne execute izni istemez; game image önceki startup koruma değişimi nedeniyle writable+execute olabilir. 24 byte tek VM region içinde kalır ve region türü/koruması tüm duraklarda aynı olmalıdır. Başka DLL image kabul edilmez; game image RVA/size ve sayfa koruması ayrı kontrol edilir. DebugInfo ve SpinCount opaque metadata olarak korunur, işaret ettikleri adresler takip edilmez. Bu tuple kontrolü tek başına geçmişte InitializeCriticalSection çağrıldığının matematiksel kanıtı değildir; güvenilen GTA/CRT başlangıç akışı, OS kimliği ve aşağıdaki canlı API dönüşü/readback birlikte değerlendirilir. Fixture nesnesi gerçek InitializeCriticalSectionAndSpinCount ile hazırlanır.

| Alan | Çağrı öncesi / API entry | API ve selector dönüşünden sonra |
|---|---|---|
| DebugInfo | Saklanan değer | Aynı |
| LockCount | 0xFFFFFFFF (-1) | 0xFFFFFFFE (-2) |
| RecursionCount | 0 | 1 |
| OwningThread | 0 | Owned child main thread ID |
| LockSemaphore | 0 | 0 |
| SpinCount | Saklanan değer | Aynı |

Boş slot, okunamayan/salt okunur/geçersiz nesne, önceden tutulmuş/recursive/contended nesne, yanlış IAT/prefix ve metadata değişimi ret verir. API girişine kadar bütün register ve flags RF dışında korunur. API dönüşünde EAX/ECX/EDX ve aritmetik flags serbesttir; EBX/ESI/EDI/EBP, kontrol flags ve stack cleanup doğrulanır. TF/DF reddedilir. Selector dönüşünde ESI=0 ve önceki SEH EBP'si geri gelmelidir.

84-byte [S,S+84) çağıran stack penceresi bütün duraklarda aynıdır; canlı SEH kaydı ve cwd parametrelerini içerir. API girişinde alt stack'teki argüman/dönüş ayrıca okunur. Windows işlevinin S altındaki çalışma yığınında yalnız PUSH izleri kaldığı varsayılmaz; bu bölgeye keyfî guard değişmezliği dayatılmaz. 28-byte NT_TIB, önceki ilk SEH kaydı, dış 32/156-byte caller pencereleri, root/guard 136 byte, localisation 16 byte, last-error ve named event kimliği korunur.

## Hata, durdurma ve JSON davranışı

Beklenmeyen exception, bekleme/süre veya event/thread kotası, hedef/object/stack/SEH drift ve debugger owner hatası owned child'ın kapatılmasıyla sonuçlanır. EnterCriticalSection bloke olabilir; önkoşul sonraki yarışları imkânsız kılmaz. Mevcut 5 saniye/128 event/16 thread sınırı korunur. Thread'i tek başına sonlandırıp başka oyun thread'lerinin devam etmesine izin verilmez; **deneyin tüm child süreci** kapatılır. Kilit başka sürece ortak değildir. [Microsoft kritik bölüm kapsamı](https://learn.microsoft.com/en-us/windows/win32/sync/critical-section-objects).

Bu kesit kilit tutulurken durur; LeaveCriticalSection veya SEH unwind çağırmaz. Oyuna geri bağlanma/temiz shutdown kanıtı değildir. Eski TEB/context rollback yapılmaz. Yeni observer müdahalesi yalnız DR0'dır; önceki host-setting bastırması kendi sözleşmesinde kalır. API/CRT yürütmesi doğal olarak nesne ve stack'i değiştirir; observer bunları yazmaz.

JSON `cwdAcquireObservation`, beş checkpoint, function/object adresi, thread ID, lockCount/recursion/owner önce/sonra ve korunum bayrakları taşır. `acquired`, API dönüşü ve nesne sahipliği eşleştiğinde true olur; bütün korunum/selector dönüşü için ayrıca `verified` gerekir. `cwdLockObservation.lockAcquiredVerified` bu alt kanıtı yansıtır. Üst modda branchAllowed/criticalSectionCallAllowed=true; lazyInitializationAllowed, directoryApiAllowed, unlockVerified ve cwdReturnVerified=false. Eski `--observe-cwd-lock` kendi CMP terminalinde durur ve acquire izni kazanmaz. Eski modlarda yeni JSON alanı null'dır. Başarı exit 3 / cwd_acquire_verified, ret 1, kullanım 2; canAttach/initializationVerified=false.

## Genişleme ve kabul senaryoları

Sonraki kesit wrapper'ın POP ECX/SEH durum değişikliği ve cwd helper'ıdır; OS GetCurrentDirectoryA, hata dönüşü, strcpy benzeri kopya, 126/127 ANSI-byte suffix ve buffer NUL sınırı ayrı doğrulanacaktır. Cwd helper dönüşü sonrası unlock(7), SEH sökümü, manager/initializer dönüşü, renderer/doğal frame ve N2/N3/D1/D2 açıktır. Lazy allocation ve recursive lock(10) yolu bu izinle açılmaz.

[Portable testler](../../tests/engine/cwd_acquire_spec_tests.cpp) sınır/hizalama/overlap, ASLR, beş ABI durumu, volatile/nonvolatile ayrımı, flags ve her nesne word değişimini sınar. [Native fixture](../../tests/engine/cwd_acquire_tests.cpp) gerçek Windows kritik bölümü üzerinde başarı, boş/tutulmuş/salt okunur/geçersiz nesne, yanlış IAT/prefix, eski mod, owner/kota/event, iki canary ve 12 warm handle çevrimini kapsar. Boş dal canary'si ile başarılı acquire sonrası canary ayrı kontrol edilir. Bütün fixture varyantlarında export sınırı artırılmaz. Policy/CLI negatifleri standart build/CI içindedir.

İlk hedef build ve iki native suite geçti (`cwd-acquire-build-initial.log`, `cwd-acquire-tests-initial.log`). İnceleme sırasında API dönüşündeki kontrol flags değişmezliği sıkılaştırıldı; yalnız aritmetik flags/RF değişebilir. İkinci hedef build/test geçti (`cwd-acquire-build-second.log`, `cwd-acquire-tests-second.log`). İlk gerçek GTA keşfi `cwd-acquire-explore-Debug-1.json` içinde, API çağrılmadan `cwd_acquire_precondition_object` reddi verdi: 0x00C9ADF0 nesnesi GTA image alanındaydı, ilk okuyucu yalnız heap kabul ediyordu. Aynı admitted GTA image için sınır/koruma kontrolü ve ayrı image fixture eklendi. 273 keşif girdisi değişmedi. Başarısız keşif final pozitif sayıya dahil edilmez. Image desteğinin üçüncü hedef build/test koşusu ve ikinci GTA keşfi geçti. Nihai dört Windows akışı ve gerçek GTA kanıtı aşağıdadır.

## Geçiş etkisi

C++ observer/CLI yeniden derlenir, trace'e yeni alan eklenir. C ABI 1, GNS/HTTPS, oyun otoritesi ve production IPC sözleşmeleri aynı kalır. Yeni bağımlılık, özellik kaldırma, kalıcı migration, orijinal GTA yazımı, remote/push veya dağıtım yoktur. ADR-66 / AC-90 alt senaryosu bu sınırı izler.

## Nihai doğrulama

Canonical kayıt `out/verification/engine/cwd-acquire-gta-evidence.json`, derleme/hash özeti `cwd-acquire-final-verification.json`. Yeni private dizinler `out/experiments/gta-cwd-acquire-f01a00ce-{Debug,Release}-v1`; önceki kopyalar ve orijinal kurulum korunur. Başarısız ilk keşif ve başarılı ikinci keşif final 12 koşuya katılmaz. İki keşifte 273 girdi hash'i korundu.

| Ölçüm | Sonuç |
|---|---|
| Gerçek GTA pozitif | Debug 6/6, Release 6/6; 12/12 |
| Doğal akış | 0x82ADE4 → 0x82ADE6 → ntdll API entry → 0x82ADEC → 0x836EA4 |
| Nesne | GTA image 0x00C9ADF0; MEM_IMAGE 0x1000000 / PAGE_EXECUTE_READWRITE 0x40 |
| Sahiplik | LockCount -1→-2, recursion 0→1, owner 0→child main thread; metadata korundu |
| ABI/bellek | 84-byte caller, 24-byte nesne, slot penceresi, NT_TIB/önceki SEH/caller/buffer denetlendi |
| Matris | 40/40: 12 pozitif, 21 legacy mod, beş olumsuz koşul, iki event çakışması |
| Child ömrü | Oluşturulan 37/37 süreç kapandı; üç koşulda child oluşmadı |
| Dosyalar | 280/280 hash korundu: önceki 265 + 14 private dosya + cwd-acquire policy |
| x86 Debug / Release | Her profilde 41 native suite / 79 managed / 254 Python |
| x64 Debug / Release | Her profilde 19 native suite / 79 managed / 184 Python |
| Yeni testler | 141 portable kontrol; 17 x86 senaryo, iki canary ve 12 warm çevrim |

Image fixture içeren ilk başarılı warm sayımı 136 → 136 (`cwd-acquire-fixture-third.log`); nihai standart x86 akışlarında da eşitlik geçti. Her profilin native ayrıntısı `cwd-acquire-<architecture>-<configuration>-native.log` içinde saklanır. Linux/hosted CI/N1 SDK yeniden koşulmadı.

284 kaynak/yapılandırma girdisi `cwd-acquire-source-build-inputs.json` ve altı probe/DLL/map artifact hash'i final kontrolde aynı kaldı. Pozitiflerde 87–92 debug event, thread sayıları [4]; 128 event/16 thread/5 saniye kotaları korundu. Host foreground timeout yalnız SPI_GET ile 2147483647 → 2147483647 ölçüldü.

| Kimlik | SHA-256 |
|---|---|
| Cwd acquire policy | `de8ff2899242f67396333ad305102eaf91a2afc9f21034962b2b1c0837dd3fd9` |
| Debug probe | `23e118716e21b915601f3ad50ac9f59b37620128a710dcd707accef418f19d9e` |
| Debug dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release probe | `f08ea7cd6e779af2a2b2df56c391c31f68d818fa1dc62a38f19df4b8a13fcc03` |
| Release dll | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Bu sonuç mevcut kritik bölümün doğal alınmasını ve selector dönüşünü kanıtlar. Unlock, cwd okuma/kopyalama, SEH sökümü ve temiz oyun kapanışı doğrulanmamıştır; renderer ve multiplayer kapıları açık kalır.
