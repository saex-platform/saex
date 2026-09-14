# D1-N2 — İlk oyun başlatma yardımcıları

Kod 0.1.27 / mimari v0.34. İki helper ve sınırlı veri yazımı kesiti doğrulandı. [Durum](status.md) · [Önceki routing sınırı](d1-application-routing.md).

## Sorumluluk ve veri akışı

`--observe-game-prelude <exe> <absolute-cwd>` owned x86 child'da önceki routing kanıtından 0x53EC2B CALL ile 0x53BB50 initializer girişine ilerler. 0x72F3B0 helper gövdesi yalnız RET'tir. Dönüşünün ardından 0x56D180 helper üç yerelleştirme bayrağını doğal CPU komutlarıyla yazar. Beşinci durak 0x53BB5A, CFileMgr::Initialise CALL önündedir. İlk initializer'ın tamamı veya dış çağırana dönüşü çalıştırılmaz.

| Durak | Adres | ESP | EAX | Veri penceresi |
|---|---|---|---|---|
| 1 — initializer entry / ilk helper CALL | 0x53BB50 | S−4 | 5 | Başlangıç ile aynı |
| 2 — boş helper entry | 0x72F3B0 | S−8 | 5 | Aynı; RET henüz yürütülmedi |
| 3 — boş helper döndü / localisation CALL | 0x53BB55 | S−4 | 5 | Aynı |
| 4 — localisation entry | 0x56D180 | S−8 | 5 | Aynı; veri yazımı henüz başlamadı |
| 5 — localisation döndü / CFileMgr CALL | 0x53BB5A | S−4 | 0 | Üç bayrak [1,0,0], çevre byte'lar aynı |

S, 0x53EC2B noktasındaki ESP, önceki instance caller C için C−32'dir. Dış dönüş adresi 0x53EC30; iç dönüş adresleri 0x53BB55/0x53BB5A'dır. EBX=ESI=0, EDI=24; ECX/EDX/EBP ve diğer korunması gereken register'lar değişmez. TF/DF reddedilir. CALL/RET öncesi bayraklar breakpoint'in RF biti dışında aynıdır. Son XOR AL,AL için CF/PF/ZF/SF/OF = 0/1/1/0/0 doğrulanır; AF tanımsız, RF CPU breakpoint alanıdır; diğer bitler korunur. Başlangıç EAX=5 şartı nedeniyle son EAX=0 beklenir; bu genel bir uint32→AL dönüş varsayımı değildir.

## Veri yan etkisi ve ABI

0x56D180–0x56D193 gövdesi 20 byte'tır: XOR AL,AL; [0xB9B7EC]=1; [0xB9B7ED]=AL; [0xB9B7EE]=AL; RET. Absolute operandlar admitted image base ile üretilir ve bütün gövde exact karşılaştırılır. Üç veri byte'ı dışındaki dört ön ve dokuz arka guard byte birlikte 16-byte pencere oluşturur. Başlangıç değerleri okunur; sıfır/BSS varsayımı yapılmaz. İlk dört durakta pencere tamamen aynı olmalıdır; beşincide yalnız tanımlı üç byte yeni değerini alabilir. Pencerenin MEM_IMAGE/COMMIT, doğru AllocationBase, erişilebilir ve yazılabilir tek region olması gerekir. PE policy BSS için .data virtualSize aralığını kabul eder; code aralıkları raw executable bölüm içinde olmalıdır.

32-byte mevcut application/dispatcher frame ile yaşayan 156-byte üst caller yığını ayrıca korunur. Last-error ve named event kimliği her durakta yeniden doğrulanır. Observer bu yeni kesitte uygulama verisi/IP/stack yazmaz; yalnız main-thread DR0 durağını taşır. Üç yazımı orijinal oyun komutları yapar. Önceki platform bastırmasının sentetik bağlam uyarlaması hâlâ ayrı kanıttır; bütün başlangıcın değiştirilmemiş olduğu iddia edilmez. Başarı/ret sonunda child sonlandırılır ve çıkışı doğrulanır; doğal yan etkiler sonrası eski bağlama veya eski veri byte'larına rollback yapılmaz.

## Arayüzler ve hata davranışı

[GamePreludeSpec](../../include/saex/engine/game_prelude.hpp) iki FrameTargetSpec, flags RVA ve sonraki CALL için read-only prefix taşır. [Portable doğrulayıcı](../../src/engine/loader/game_prelude.cpp) tam RET/padding, body operandları, nonoverlap, image/rel32 parent zinciri ve frame/veri geçişini doğrular. [Strict policy](../../contracts/engine/game-prelude-policy.json) application-routing SHA üzerinden exact executable/DLL zincirine bağlıdır. CLI uzak adres, event veya yazılacak değer almaz; indirilen resource bu C++ API'ye erişmez.

Eski modlarda gamePreludeObservation null'dır. Yeni modda policy/digest, helperCallsAllowed=2, stage/stopAddress, flagsAddress ve flagsBeforeHex/flagsAfterHex (16-byte pencere), reached/returned/continued, shape/frame/stack/flags/verified alanları bulunur. Pencere okunmadan sıfır dolu alanlar kanıt sayılmaz; flagsRead/flagsValid ayrı değerlendirilir. fileManagerCallAllowed=false ve initializerReturnVerified=false kalır. Önceki routing nesnesinde initializerCallAllowed yalnız bu üst modda true olur; önceki mod 0x53EC2B terminalini korur. Ara verified kayıtları önceden tamamlanmış duraklardır, son işlem başarısı değildir. Başarı game_prelude_verified/exit 3; ret 1, kullanım 2. canAttach=false ve initializationVerified=false korunur.

Yanlış spec yürütmeden; yanlış helper/operand/stop prefix initializer CALL önünde; writable veri önkoşulu ayrı ret ile durur. Devam sırasında shape/frame/stack/last-error/event/flags farkı, beklenmeyen exception, owner veya kota hatası başarı üretmez. DR1 startup IAT, DR2 ekstra ASI ve DR3 existing-instance pencere kolu korunur. Bu thread gözlemi, başka kötü niyetli native thread'lerin bütün yazmalarını önleyen bir sandbox değildir; guard penceresi dışındaki bütün adreslerin değişmediğini ölçmez. İncelenen exact komutlar ile sınırlı readback birlikte kanıt oluşturur.

## Kabul senaryoları ve araştırma sınırı

Portable kontroller, yanlış RET/absolute adres/parent/stop, relocation, beş dönüş çerçevesi, register/flags bozulması ve 16 pencere byte'ının her birinin bozulmasını kapsar. Windows fixture gerçek CALL/RET/XOR/MOV dizisiyle [7,8,9] başlangıcını [1,0,0] yapar; böylece iki sıfır yazımı da ölçülür. Üç malformed spec ve üç runtime shape ret, event kotası, owner/recovery, eski routing terminali, mevcut named event ve 12 warm handle çevrimi kapsanır. File-manager canary normal gözlemde erişilmez; ayrı kontrol koşusunda marker üretip 92 ile çıkar. Ayrı nesne adı Local\SAEX.GamePreludeFixture.v1'dir.

İlk fixture testinde MSVC, RET sonrasında padding bulunduğu için `ret` komutunu C2 00 00 kodladı; exact sözleşme bunu yürütmeden reddetti. Fixture bir-byte C3 ile düzeltildi; üretim sözleşmesi gevşetilmedi. İlk başarısız test `out/verification/engine/prelude-tests-initial.log`, başarılı tekrar `prelude-tests-second.log` içinde korunur. Derleme başarılıydı; başarısızlık fixture spec kontrolündeydi.

Sonraki CFileMgr gövdesi 0x5386F0'da root-dir buffer/CRT cwd çağrısı ve string sonlandırma içerir. İleride cwd uzunluğu, CRT çağrı yolu ve buffer sınırları incelenecek; mevcut reverse-engineered FileMgr kaynağı UTF-8/wide-char iyileştirmeleri içerdiğinden yerel executable ile aynı gövde sayılmaz. 0x406B70 CdStreamInit(5) bellek/OS çağrıları, 0x541D90 CPad::Initialise alt çağrı/döngüleri daha sonraki kapılardır. Initializer'ın son AL=1/RET byte'ları void prototipten boolean başarı sonucu türetmek için kullanılmaz.

## Kaynaklar ve geçiş

Adresler hash'i f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac olan yerel PE/disassembly'den çıkarıldı. [Game.cpp](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/game_sa/Game.cpp) beş helper sırasını ve isimlerini, [Localisation.cpp](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/game_sa/Localisation.cpp) yerelleştirme davranışını açıklar. [FileMgr.cpp](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/game_sa/FileMgr.cpp) sonraki araştırmanın referansıdır. Commit ve üç dosya hash'i `out/research/game-prelude/sources.json` içindedir. İncelenen yerel alt çağrı dump'ları `out/verification/engine/prelude-helpers-disassembly.txt` kaydındadır. Kaynaklar isim/kapsam açıklamasıdır; yeni dependency veya kaynak kopyalama yoktur.

C++ observer/trace tüketicileri yeniden derlenir; bootstrap C ABI 1, GNS, otorite, IPC aynı kalır. Kaldırılan özellik veya kalıcı migration yoktur. Boş CMemoryMgr::Init dönüşü bellek yöneticisi kapasitesi/hazır heap kanıtı değildir. CFileMgr, streaming/pad, initializer dönüşü, RsInitialize, renderer/window/doğal frame ve production IPC/sandbox ile AC-90/AC-91/N2/N3/D1/D2 bütünü açık kalır.

## Nihai doğrulama

Canonical kayıt `out/verification/engine/prelude-gta-evidence.json`, derleme/hash özeti `prelude-final-verification.json`. Yeni private dizinler `out/experiments/gta-prelude-f01a00ce-{Debug,Release}-v1`; önceki dizinler ve orijinal oyun kurulumu korunur. İlk keşif `prelude-explore-Debug-1.json` final 12 koşuya katılmaz.

| Ölçüm | Sonuç |
|---|---|
| Gerçek GTA pozitif | Debug 6/6, Release 6/6; 12/12 |
| Doğal duraklar | 0x53BB50 → 0x72F3B0 → 0x53BB55 → 0x56D180 → 0x53BB5A |
| Yan etki / ABI | İki helper döndü; üç bayrak [1,0,0], 13 guard byte, register/flags ve nested return stack eşleşti |
| Caller koruma | 32-byte parent frame ve yaşayan 156-byte caller, last-error ve named event kimliği korundu |
| Matris | 36/36 beklenen sonuç: 12 pozitif, 17 legacy mod, beş olumsuz koşul, iki event çakışması |
| Child ömrü | Oluşturulan 33/33 süreç kapandı; üç koşulda child oluşmadı |
| Dosyalar | 220/220 girdi hash'i korundu: önceki 205 + 14 private dosya + prelude policy |
| x86 Debug / Release | Her profilde 33 native suite / 79 managed / 226 Python |
| x64 Debug / Release | Her profilde 15 native suite / 79 managed / 164 Python |
| Yeni testler | 215 portable spec/body/frame/flags kontrolü; 11 x86 senaryo, file-manager canary ve 12 warm çevrim |

Pozitiflerde 74–82 debug event, thread sayıları [4]; 128 event/16 thread/5 saniye kotaları korunur. Host foreground timeout yalnız SPI_GET ile 2147483647 → 2147483647 ölçüldü. Bu eşitlik tek başına sistem ayarı çağrısının çalışmadığı kanıtı değildir; önceki bastırma/bağlam/canary kanıtıyla birlikte değerlendirilir.

GTA veri penceresi örneği: `6a0bb63b000000000000000000000000` → `6a0bb63b010000000000000000000000`; final koşularının tamamında üç hedef byte [1,0,0] ve 13 guard byte eşitliği ayrı ayrı doğrulandı. Fixture [7,8,9] başlangıcıyla her üç doğal store etkisini sınar; GTA başlangıcında zaten sıfır olan iki byte için sadece son değer/readback eşitliği ölçülür.

252 kaynak/yapılandırma girdisi `prelude-source-build-inputs.json` ve altı probe/DLL/map artifact hash'i final kontrolde aynı kaldı. İlk başarılı build `prelude-build-initial.log`, fixture düzeltmesi sonrası build `prelude-build-second.log`; önceki bölümde ilk başarısız test ve başarılı tekrar ayrılmıştır. İlk başarılı warm sayımı 131 → 131; dört standart akışta warm eşitliği tekrar geçti. Linux/hosted CI/N1 SDK yeniden koşulmadı.

| Kimlik | SHA-256 |
|---|---|
| Game prelude policy | `eee50036b97098d00b2c3be58df01fa3724c04123884a3fdc4a75e4305ccab4d` |
| Debug probe | `4b827a5a1cfc52e1846210a71a3e23ea54172c5e16b4ea9fb8dedb21e5a17ee4` |
| Debug dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release probe | `d60dbda37e7ef265817b6244b5880a485bdd81e4786d4423de6eed056428eb62` |
| Release dll | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

İki helper'ın dönüşü ve tanımlı RAM yan etkisi doğrulandı; initializer dış dönüşü veya initialize başarı sonucu kanıtlanmadı. Production SDK/IPC/sandbox, renderer ve doğal frame kapsamları açık kalır.

## Kod 0.1.28 bağlantısı

`--observe-file-manager-entry` CFileMgr içine doğal CALL ve ilk üç PUSH komutunu açar; 0x5386FB CRT cwd CALL önünde durur. İki durakta buffer/maxlen=128 ABI, nested return stack, register/flags, 136-byte root/guard ve localisation korunumu denetlenir. [Sözleşme ve sonuç](d1-file-manager-entry.md). Önceki prelude terminali korunur; yeni üst modda prelude fileManagerCallAllowed=true, manager cwdCallAllowed=false/fileManagerReturnVerified=false olur. CRT lock/SEH/OS/copy yolu henüz açılmaz. Gelecekte suffix yazımından önce NUL en geç buffer offset 126, başarılı dönüş ve ANSI byte uzunluğu kanıtı gerekir. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Manager/initializer dönüşü, streaming/pad, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## 14 Eylül 2026 — Windows fixture taşınabilirliği

Startup-return ve devamındaki fixture zinciri, sistem DLL reçetelerini test makinesinin diskte tutulan PE dosyalarından çıkarır. Ortak okuyucu ve negatif doğrulamalar [test kapsamı notunda](d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği) açıklanır; üretim policy/hash kuralları ve bu belgedeki gerçek GTA kanıtının sınırları aynıdır.
