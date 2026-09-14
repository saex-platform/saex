# D1 — CRT lock(7) kayıt seçimi

Kod 0.1.30 / mimari v0.37. Selector ve slot karşılaştırması kesiti doğrulandı. [Durum](status.md) · [Önceki sınır](d1-cwd-seh.md).

## Sorumluluk ve veri akışı

`--observe-cwd-lock <exe> <absolute-cwd>` doğal `PUSH 7 / CALL 0x82ADBE` akışını açar. CRT yardımcısı kendi EBP/ESI değerlerini saklar, argüman 7 üzerinden sekiz byte aralıklı tablodan slot seçer ve ilk word'ü sıfırla karşılaştırır. Gözlem **0x82ADCF JNE komutundan önce** durur. Boş ve dolu slot aynı terminale ulaşır; hiçbir dal veya kritik bölüm çağrısı çalıştırılmaz.

Q, önceki SEH terminali 0x836E9D'deki ESP'dir: Q=P−48=C−100. Başlangıç EAX=Q+24, EBP=Q+40, EBX=ESI=0, EDI=24, FS:[0]=Q+24. ECX/EDX saklanır.

| Durak | Adres | ESP | EAX | EBP | ESI |
|---|---|---|---|---|---|
| 1 — lock yardımcısına giriş | 0x82ADBE | Q−8 | saved EAX | saved EBP | saved ESI |
| 2 — CMP tamamlandı; JNE henüz çalışmadı | 0x82ADCF | Q−16 | 7 | Q−12 | 0x8E31F8 |

Tablo başlangıcı 0x8E31C0, stride 8, slot index 7, seçilen word adresi 0x8E31F8'dir. Yardımcının ilk 17 byte'ı tamamen incelenmiştir: `55 8b ec 8b 45 08 56 8d 34 c5 c0 31 8e 00 83 3e 00`. Son komut `CMP dword ptr [ESI],0`; duraktaki `75 13` kısa JNE ayrı doğrulanır. Terminalden başlayan toplam 16 byte da değişime karşı pinlenir; bu pin sonraki komutların yürütme izni değildir.

## Arayüz ve ABI

[CwdLockSpec](../../include/saex/engine/cwd_lock.hpp), selector FrameTargetSpec, tablo RVA ve stop prefix taşır. [Portable doğrulayıcı](../../src/engine/loader/cwd_lock.cpp) yürütülen gövdeyi admitted image base ile üretir. [Policy](../../contracts/engine/cwd-lock-policy.json) ve [generator](../../tools/cwd_lock_policy.py) önceki SEH policy SHA zincirine bağlanır; tür, duplicate key, boyut, PE section/izin, RVA/rel32, hizalama, overlap ve çağrı zinciri doğrulanır. Bilinmeyen executable yeni bir izin kazanmaz. Fixture kendi relocated base ve PE operandları üzerinden aynı sözleşmeyi kullanır.

İlk durakta flags RF dışında korunur. İkinci durakta `CMP value,0` CF/AF/OF bitlerini sıfırlar; SF/ZF/PF doğrudan gözlenen word üzerinden hesaplanır. Diğer flags RF dışında aynıdır; TF/DF reddedilir. Dolu word'ün geçerli `CRITICAL_SECTION` adresi olduğu varsayılmaz ve hedef adres okunmaz. `slotPresent=true`, yalnız karşılaştırılan değerin sıfır olmadığını belirtir; oluşturma, tutarlılık veya kilit alma kanıtı değildir.

## Bellek ve hata davranışı

Her iki durakta writable MEM_PRIVATE/COMMIT içindeki 100-byte stack penceresi [Q−32,Q+68) doğrulanır. Pencere TIB StackLimit/StackBase aralığında ve DWORD hizalı olmalıdır. Başlangıç değerleri saklanır; local alanların sıfır olması beklenmez.

| Stack adresi | İzin verilen yazım |
|---|---|
| Q−4 | Argüman 7; iki durakta da |
| Q−8 | CALL dönüşü 0x836EA4; iki durakta da |
| Q−12 | saved EBP; yalnız ikinci durakta |
| Q−16 | saved ESI; yalnız ikinci durakta |
| Pencerenin diğer word'leri | Birebir korunur; canlı SEH kaydı, local alanlar ve cwd argümanları dahil |

Seçilen slotun [slot−4,slot+12) 16-byte penceresi writable MEM_IMAGE/COMMIT ve aynı AllocationBase içinde okunur. İlk word'ün önceki slot metadata'sı olması veya diğer metadata bitlerinin anlamı yürütme şartı değildir; **16 byte'ın tamamı korunur**. Pointer üzerindeki eşzamanlı değişim veya karşılaştırma flags uyumsuzluğu ret verir. Başka thread'lerin gelecekte slotu değiştirmeyeceğine ilişkin atomiklik/idari sahiplik garantisi üretilmez.

28-byte NT_TIB, önceki ilk SEH kaydı, 32-byte parent, 156-byte dış caller, root/guard 136 byte, localisation 16 byte, last-error ve named event kimliği korunur. Beklenmeyen exception, kod/slot/stack/SEH değişimi, owner-thread ihlali, süre/event/thread sınırı ve okuma/koruma hatasında child kapatılır. Yeni veri, kod, EIP/ESP veya FS yazımı yoktur; DR0 iki doğal durak arasında taşınır. Önceki host-setting bastırmasının dar bağlam müdahalesi kendi sözleşmesinde kalır. Oluşturulan child sonunda kapatılır; eski TEB/context rollback yapılmaz.

JSON `cwdLockObservation`: policy/digest, lockIndex=7, stage/stopAddress, slotAddress/slotValue/slotPresent, şekil/ABI/korunum sonuçları ve verified. `branchAllowed`, `lazyInitializationAllowed`, `criticalSectionCallAllowed`, `lockAcquiredVerified`, `cwdReturnVerified` false kalır. Üst modda `cwdSehObservation.lockPathAllowed=true` yalnız bu selector kesitini açar. Eski `--observe-cwd-seh` kendi terminalinde durur ve lockPathAllowed=false tutar; diğer modlarda cwdLockObservation null'dır. Başarılı gözlem exit 3 / cwd_lock_verified, ret exit 1, kullanım hatası exit 2. canAttach/initializationVerified=false.

## Genişleme ve açık kapılar

Statik executable incelemesinde JNE hedefi 0x82ADE4; sıfır dalı 0x82ADD1 ve lazy helper 0x82AD3F'tür. Başlangıç dosyasında slot pointer sıfırdır; çalışma zamanı değeri yalnız gerçek probe ile öğrenilir. Import tablosu 0x8580EC için KERNEL32.dll!EnterCriticalSection, 0x8580E8 için LeaveCriticalSection gösterir. Bu ad/adres eşlemesi canlı Windows hedefinin veya kritik bölüm iç yapısının onayı değildir. `out/verification/engine/cwd-seh-disassembly.txt` ve `cwd-lock-static.json` yerel inceleme çıktılarıdır, build girdisi değildir.

Sonraki kesit, gözlenen slot üzerinden mevcut/lazy dallar için ayrı durak ve reddetme politikasıdır. Pointer geçerliliği, kritik bölüm sahipliği ve OS target/module kimliği doğrulanmadan EnterCriticalSection açılmaz. Lazy yol allocator, recursive lock(10), InitializeCriticalSection ve hata/yarış yollarını kapsar; ayrı inceleme ister. Sonra cwd OS/copy dönüşü ve 126/127 ANSI-byte suffix sınırı gelir. Cwd/manager/initializer dönüşü, SEH sökümü/unwind, renderer/doğal frame ve N2/N3/D1/D2 açık kalır.

## Kabul senaryoları ve doğrulama

[Portable testler](../../tests/engine/cwd_lock_spec_tests.cpp) adres/rel32/gövde, ASLR, TIB/stack sınırları, bütün register/flags ve her stack word değişimini sınar. CMP flags boş değer, tek/çift parity ve yüksek sign biti dahil bağımsız beklenen örneklerle denetlenir. [Native fixture](../../tests/engine/cwd_lock_tests.cpp) dolu opaque word ve boş slotu ayrı executable'larla çalıştırır; salt okunur tablo initializer içinde dedicated page'e uygulanır. Böylece proxy'nin önceki image-writable davranışı yanlış pozitif yaratmaz. Fixture dışa aktarılan sembol sayısı 31, mevcut parser sınırı 32 olarak kalır.

Dolu ve boş fixture için iki canary kontrolü iki dalı serbest bırakmanın da algılanacağını gösterir; observer koşularında canary yoktur. Bozuk selector/stop/slot operandı, owner, event bütçesi, eski SEH terminali, mevcut event ve 12 warm handle çevrimi kapsanır. Yerel fixture gerçek kilit nesnesi veya sandbox kanıtı değildir. Policy ve CLI negatifleri standart build/CI girişine eklenmiştir. AC-90 yalnız bu alt senaryo kadar ilerler.

İlk hedef build fixture LEA ifadesindeki `offset` operandını MSVC C2415 ile reddetti (`cwd-lock-build-initial.log`). Adresleme `lea esi,[eax*8+cwd_lock_table]` olarak düzeltildi; gerçek byte sözleşmesi ve derleyici uyarı ayarları korundu. İkinci hedef build ve iki native suite geçti (`cwd-lock-build-second.log`, `cwd-lock-tests-initial.log`). Nihai dört Windows akışı ve gerçek GTA kanıtı aşağıdadır.

## Geçiş etkisi

C++ observer/CLI yeniden derlenir; trace'e yeni alan eklenir. Eski mode terminali, C ABI 1, oyun otoritesi, GNS/HTTPS ve IPC sözleşmeleri aynı kalır. Bağımlılık veya özellik kaldırma, kalıcı veri migration'ı, orijinal GTA değişikliği ve dağıtım yoktur. ADR-65 ve AC-90 alt senaryosu bu sınırı kaydeder.

## Nihai doğrulama

Canonical kayıt `out/verification/engine/cwd-lock-gta-evidence.json`, derleme/hash özeti `cwd-lock-final-verification.json`. Yeni private dizinler `out/experiments/gta-cwd-lock-f01a00ce-{Debug,Release}-v1`; önceki kopyalar ve orijinal kurulum korunur. İlk keşif `cwd-lock-explore-Debug-1.json` final 12 koşuya katılmaz; keşifte 258 girdi hash'i korundu.

| Ölçüm | Sonuç |
|---|---|
| Gerçek GTA pozitif | Debug 6/6, Release 6/6; 12/12 |
| Doğal duraklar | 0x82ADBE → 0x82ADCF; CALL ve ilk 17 byte yürüdü |
| Slot gözlemi | 0x8E31F8'de 12/12 dolu word; adres değişken, hedef dereference edilmedi |
| ABI/bellek | 100-byte stack ve 16-byte slot; register/CMP flags ve SEH/caller/buffer korundu |
| Matris | 39/39: 12 pozitif, 20 legacy mod, beş olumsuz koşul, iki event çakışması |
| Child ömrü | Oluşturulan 36/36 süreç kapandı; üç koşulda child oluşmadı |
| Dosyalar | 265/265 hash korundu: önceki 250 + 14 private dosya + cwd-lock policy |
| x86 Debug / Release | Her profilde 39 native suite / 79 managed / 247 Python |
| x64 Debug / Release | Her profilde 18 native suite / 79 managed / 179 Python |
| Yeni testler | 332 portable kontrol; 13 x86 senaryo, iki dal canary kontrolü ve 12 warm çevrim |

İlk başarılı warm sayımı 140 → 140 (`cwd-lock-fixture-initial-success.log`); iki canary eklenmiş nihai standart x86 akışları da geçti. Her profilin native ayrıntısı `cwd-lock-<architecture>-<configuration>-native.log` içinde saklanır. Linux/hosted CI/N1 SDK yeniden koşulmadı.

276 kaynak/yapılandırma girdisi `cwd-lock-source-build-inputs.json` ve altı probe/DLL/map artifact hash'i final kontrolde aynı kaldı. Pozitiflerde 80–87 debug event, thread sayıları [4]; 128 event/16 thread/5 saniye kotaları korundu. Host foreground timeout yalnız SPI_GET ile 2147483647 → 2147483647 ölçüldü; eşitlik önceki bastırma/canary kanıtının yerine geçmez.

| Kimlik | SHA-256 |
|---|---|
| Cwd lock policy | `098567bb8f34f0619e5a31f3c0f760e89d32af659ec4b9fec633245215ac6327` |
| Debug probe | `b1430a2284fa5ebd630a96dcad728fdb7fe30e7bc8be98809e43379726e7d1ee` |
| Debug dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release probe | `2cd0b7c26ad24bda1ae642301e2ff1d96ee25b4c5ace115dde7ea4d14fc094f5` |
| Release dll | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Bu sonuç kilit kaydının doğal seçimini kanıtlar. Acquire, kritik bölüm güvenliği, lazy initialization, cwd okuma/kopyalama ve unwind hâlâ doğrulanmamıştır; renderer ve multiplayer kapıları açık kalır.

## Kod 0.1.31 bağlantısı

`--observe-cwd-acquire` mevcut/unowned lock(7) nesnesi için doğal dal, admitted ntdll API entry/return ve CRT selector dönüşünü açar; 0x836EA4 terminalinde durur. [Sözleşme ve sonuç](d1-cwd-acquire.md). Beş durakta object/slot/84-byte caller/SEH korunumu ve API sonrası thread sahipliği doğrulanır. x86 24-byte kritik bölüm düzeni pinned Windows uygulamasına aittir; VOID dönüşte EAX başarı kodu sayılmaz. Heap veya aynı GTA image nesnesi için sınır/koruma denetimi vardır. Üst modda branch/criticalSectionCallAllowed=true, acquired readback ile ayrıdır; lazy/directory/unlock kapalı kalır. Önceki lock terminali korunur; kilit tutulurken bütün owned child kapatılır, observer veri/TEB/context rollback yapmaz. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Cwd/SEH/manager dönüşü ve N2/N3/D1/D2 açıktır.

## 14 Eylül 2026 — Windows fixture taşınabilirliği

Startup-return ve devamındaki fixture zinciri, sistem DLL reçetelerini test makinesinin diskte tutulan PE dosyalarından çıkarır. Ortak okuyucu ve negatif doğrulamalar [test kapsamı notunda](d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği) açıklanır; üretim policy/hash kuralları ve bu belgedeki gerçek GTA kanıtının sınırları aynıdır.

## 0.1.33 — Windows profil bağı

[26200.9445 incelemesi](d1-system-profile-9445.md), bu bileşenin bağlı olduğu OS pinleri/API reçeteleri ve parent SHA-256 zincirini günceller. Bileşenin GTA durak/izin sınırı aynı kalır; önceki runtime sonuçları eski profil bağlamındadır. Yeni dosyalarla fixture ve gerçek GTA kanıtının kapsamı profil raporunda ayrıca izlenir. Eski OS pinlerine otomatik fallback yoktur; kaynak ve generated policy birlikte yeniden derlenir.

## Sonraki birleşik kesit — 0.1.36

Bu belgedeki terminal ve eski test kayıtları kendi sürümüne aittir. [File-manager-ready](d1-file-manager-ready.md) önceki zinciri tek komutta tamamlar: normal kilit bırakma, SEH sökümü, suffix ve CFileMgr dönüşü. Eski komut otomatik ilerletilmez; buradaki snapshot güncel final durum gibi yorumlanmaz. Genel initializer/renderer/frame ve D1/D2 açıktır.
