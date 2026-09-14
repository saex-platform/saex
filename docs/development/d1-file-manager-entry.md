# D1-N2 — Dosya yöneticisi girişi ve cwd çağrı sınırı

Kod 0.1.28 / mimari v0.35. Dosya yöneticisi girişi ve çağrı parametreleri kesiti doğrulandı. [Durum](status.md) · [Önceki kesit](d1-game-prelude.md).

## Sorumluluk ve veri akışı

`--observe-file-manager-entry <exe> <absolute-cwd>` önceki iki helper'ın doğal dönüşünü doğrular; 0x53BB5A CALL ile CFileMgr::Initialise içine girer. CLI dışarıdan RVA, callback veya buffer içeriği almaz; indirilen resource bu native observer API'sine erişmez. İki yeni durak 0x5386F0 giriş ve 0x5386FB CRT cwd CALL önüdür. Bu kesitte yalnız PUSH EDI, PUSH 128, PUSH buffer yürür; 0x836E91 çağrısı, string taraması, suffix yazımı ve dosya yöneticisi dönüşü çalıştırılmaz.

T, prelude terminalindeki ESP'dir; önceki routing S için T=S−4, üst caller C için C−36. EAX=EBX=ESI=0, EDI=24; ECX/EDX/EBP ve EFLAGS (breakpoint RF hariç) korunur. TF/DF reddedilir.

| Durak | ESP | Yığındaki uint32 değerler |
|---|---|---|
| 1 — 0x5386F0 | T−4 | 0x53BB5F, 0x53EC30 |
| 2 — 0x5386FB | T−16 | 0xB71AE0, 128, saved EDI=24, 0x53BB5F, 0x53EC30 |

İkinci durakta CRT dönüş adresi henüz yığına itilmemiştir. Parametre sırası buffer/maxlen'dir; ileride çağrı açılırsa cdecl dönüş temizliği dosya yöneticisinin `ADD ESP,8` komutundadır. Bu kesitte temizlik veya dönüş kanıtlanmaz. 32-byte application/dispatcher frame, yaşayan 156-byte caller, last-error ve named-event kimliği her durakta yeniden denetlenir.

## Arayüzler, veri ve hata davranışı

[FileManagerEntrySpec](../../include/saex/engine/file_manager_entry.hpp), manager FrameTargetSpec, cwd/buffer/suffix RVA ve caller dönüş prefix'i taşır. [Portable sözleşme](../../src/engine/loader/file_manager_entry.cpp), [strict policy](../../contracts/engine/file-manager-entry-policy.json) ve [üretici](../../tools/file_manager_entry_policy.py) aynı 51-byte gövdeyi, absolute operandları ve rel32 çağrıyı doğrular. Prelude policy SHA üzerinden executable/DLL zincirine bağlanır; bilinmeyen executable güvenli ret verir. Code raw executable bölümde; buffer ve guard penceresi writable .data virtualSize içinde; suffix okunabilir raw bölümde olmalıdır. Yeni aralıklar birbirleri ve ilgili parent aralıklarıyla çakışamaz. Relocation, adresin sadece düşük byte'ını değiştirerek yapılmaz; admitted image base ile tüm operand yeniden üretilir.

0xB71ADC'den başlayan pencere dört ön guard + 128 buffer + dört arka guard = 136 byte'tır. Tek writable/COMMIT/MEM_IMAGE region, doğru AllocationBase, guard/noaccess olmaması ve bütün pencerenin region içinde kalması gerekir. En fazla 16-byte okumalarla alınır. Başlangıç sıfır veya NUL varsayılmaz; her iki durakta 136 byte başlangıçla aynı olmalıdır. Localisation'ın önceki 16-byte penceresi de korunur. Observer oyun verisini/IP/stack yazmaz; yalnız DR0 durağını taşır. Üst modların önceki platform context uyarlaması ayrı kanıt olarak kalır.

Gövdenin tamamı read-only exact örneklenir; suffix literal 0x859F7C'de `5c00` olmalıdır. Cwd hedefinde yalnız executable image erişilebilirliği kontrol edilir; bir-byte okuma CRT gövdesinin onayı değildir. Tam executable kimliği ayrıca pinlidir. Sonraki CRT CALL hiçbir başarılı yolda açılmaz. Yanlış spec yürütmeden, body/pointer/suffix/return prefix uyuşmazlığı manager CALL önünde, yazılamayan buffer ayrı önkoşulda reddedilir. Devamdan sonraki frame/buffer/localisation/caller/event drift, exception, kota veya owner hatası başarı üretmez. DR1 startup IAT, DR2 ekstra ASI, DR3 existing-instance pencere kolu korunur. Bu gözlem kötü niyetli native thread'leri yalıtan bir sandbox değildir; pencere dışındaki tüm belleği izlediği iddia edilmez.

CLI `fileManagerEntryObservation` içinde policy/digest, stage/stopAddress/bufferAddress/cwdAddress, bufferCapacity=128, guardBytes=8, continued/entryReached/cwdCallReached, shapeValid/frameValid/stackPreserved/bufferRead/bufferUnchanged/localisationPreserved/verified döndürür. Pencere string olarak çözülmez ve ham buffer içeriği JSON'a yazılmaz. cwdCallAllowed=false ve fileManagerReturnVerified=false; prelude nesnesinde fileManagerCallAllowed yalnız yeni üst modda true olur. Önceki `--observe-game-prelude` 0x53BB5A terminalini korur; eski modlarda yeni nesne null'dır. Ara verified alanı geçmişte tamamlanan checkpoint'tir, sonraki çağrının başarısı değildir. Başarı `file_manager_entry_verified`/exit 3; ret 1, kullanım 2. canAttach=false/initializationVerified=false devam eder; sonunda child sonlandırılır ve çıkış doğrulanır. Doğal yan etkilerden sonra eski context rollback yapılmaz.

## CRT araştırması ve sonraki kapı

Yerel disassembly, 0x836E91 wrapper'da 0x8286E0 SEH giriş yardımcısı, 0x82ADBE lock(7), 0x836DA3 cwd helper, 0x836ECE unlock yolu ve 0x82871B epilog çağrıları gösterir. Bunların adları davranışa dayalı araştırma etiketleridir; tam alt çağrı ağaçları henüz yürütme için onaylı değildir. Drive=0 yolu 0x858244 IAT üzerinden `KERNEL32.dll!GetCurrentDirectoryA` import'una gider; yerel 260-byte alan, uzunluk/failure yolları, errno ve 0x826590 copy helper içerir. Gerçek OS export gövdesi, kilit/SEH/TLS/copy alt yolları ve dönüş yan etkileri sonraki kesitte incelenecek. Örnekleme kaydı `out/verification/engine/file-manager-crt-disassembly.txt` içinde, kod hiçbir yeni OS cwd çağrısını açmaz.

Dosya yöneticisi, CRT'den dönüşte EAX hatasını kontrol etmeden buffer'da NUL arar ve sona iki byte `\\` + NUL yazar. 128-byte alanı korumak için gelecekte **son NUL en geç offset 126'da** olmalıdır: 126 byte metin + slash + NUL sığar; 127 byte metin CRT'ye sığabilse de ikinci suffix byte'ı alan dışına taşır. NUL olmaması, NULL dönüş veya kod sayfası dönüşümü belirsizliği bu döngünün açılmasına izin vermez. Windows UTF-16 path uzunluğu ANSI byte uzunluğunun kanıtı değildir. Root dizinin zaten slash ile bitmesi yerel kodun ayrıca slash eklemesini değiştirmez. Bunlar mevcut gözlem moduna uzun yol desteği eklemez; dönüş kapısının gelecekteki zorunlu koşullarıdır.

[Microsoft _getcwd sözleşmesi](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/getcwd-wgetcwd?view=msvc-170), maxlen'in NUL dahil sınır olduğunu ve NULL/errno hata dönüşünü açıklar. Güncel CRT dokümanı bu eski statik CRT'nin byte karşılığı değildir. [Pinned FileMgr.cpp](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/game_sa/FileMgr.cpp) UTF-8/wide-char iyileştirmeleri içerir; yerel PE'deki çağrı yolu yerine yürütülmez. Kaynak hash'leri önceki `out/research/game-prelude/sources.json` kaydındadır. Yerel EXE SHA-256: `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`.

## Kabul senaryoları ve genişleme sınırı

Portable testler yanlış spec, root/cwd/suffix aralığı, body operandları, parent zinciri, data/code overlap, relocation, iki ABI frame, register/EFLAGS ve parametre/dönüş sırasını kapsar. Windows fixture doğal üç PUSH yürütür; 136-byte pencere readback'i, bozuk spec, üç runtime shape ret, ayrı read-only buffer fixture, kota, owner/recovery, eski prelude terminali, mevcut named event ve 12 warm handle çevrimi sınanır. Cwd canary normal modda erişilmez; kontrol koşusunda marker üretip 92 ile çıkar. Event adı `Local\SAEX.FileManagerFixture.v1`; eski fixture event adları korunur.

Gelecek ilerleme CRT dönüşünü, başarılı path/byte sınırını ve NUL sonlandırmayı ayrı kapıyla doğrulayacak; sonra suffix yazımı/manager dönüşü ve streaming/pad ele alınacak. C++ observer ve trace tüketicileri yeniden derlenir; bootstrap C ABI 1, GNS/HTTPS, otorite ve production IPC değişmez. Yeni dependency, kaldırılan özellik veya kalıcı migration yoktur. File manager tamamlanması, initializer dış dönüşü, RsInitialize, renderer/window/doğal frame, production SDK/IPC/sandbox ve AC-90/AC-91/N2/N3/D1/D2 bütünü açık kalır.

İlk fixture koşusunda salt okunur PE bölümündeki buffer, daha önceki startup proxy tüm image korumasını yazılabilir yaptığı için ret üretmedi. Bu test varsayımı `file-manager-tests-initial.log` içinde başarısız kaydedildi. Fixture artık application initializer aşamasında kendine ait 4096-byte sayfaya PAGE_READONLY uygular; gerçek runtime ret ve bütün yeni testler `file-manager-tests-second.log` içinde geçti. Üretim kapısı gevşetilmedi.

## Doğrulama kaydı

Canonical kayıt `out/verification/engine/file-manager-gta-evidence.json`, derleme/hash özeti `file-manager-final-verification.json`. Yeni private dizinler `out/experiments/gta-file-manager-f01a00ce-{Debug,Release}-v1`; önceki dizinler ve orijinal oyun kurulumu korunur. İlk keşif `file-manager-explore-Debug-1.json` final 12 koşuya katılmaz; keşifte 227 girdi hash'i korundu.

| Ölçüm | Sonuç |
|---|---|
| Gerçek GTA pozitif | Debug 6/6, Release 6/6; 12/12 |
| Yeni doğal duraklar | 0x5386F0 → 0x5386FB; CRT CALL önünde terminal |
| ABI/veri | Buffer=0xB71AE0, kapasite=128; üç PUSH, EDI ve iki return adresi doğru; 136 byte aynı |
| Caller koruma | 32-byte parent frame, yaşayan 156-byte caller, register/EFLAGS, last-error, named event ve localisation penceresi korundu |
| Matris | 37/37: 12 pozitif, 18 legacy mod, beş olumsuz koşul, iki event çakışması |
| Child ömrü | Oluşturulan 34/34 süreç kapandı; üç koşulda child oluşmadı |
| Dosyalar | 235/235 girdi hash'i korundu: önceki 220 + 14 private dosya + manager policy |
| x86 Debug / Release | Her profilde 35 native suite / 79 managed / 233 Python |
| x64 Debug / Release | Her profilde 16 native suite / 79 managed / 169 Python |
| Yeni testler | 99 portable kontrol; 12 x86 senaryo, cwd canary ve 12 warm çevrim |

İlk read-only fixture başarısızlığı ve düzeltmesi yukarıda ayrılmıştır. İlk başarılı warm sayımı 133 → 133; standart x86 akışlarında warm eşitliği tekrar geçti. `file-manager-fixture-initial-success.log` ilk başarılı test ayrıntısını korur. 260 kaynak/yapılandırma girdisi `file-manager-source-build-inputs.json` ve altı probe/DLL/map artifact hash'i final kontrolde aynı kaldı. Linux/hosted CI/N1 SDK yeniden koşulmadı.

Pozitiflerde 75–82 debug event, thread sayıları [4]; 128 event/16 thread/5 saniye kotaları korundu. Host foreground timeout yalnız SPI_GET ile 2147483647 → 2147483647 ölçüldü; bu eşitlik önceki bastırma/canary kanıtının yerine geçmez.

| Kimlik | SHA-256 |
|---|---|
| File manager policy | `0e65b724069c440804e61d3aad88fa781ff490ffd757a7928c62c318e63cc2a4` |
| Debug probe | `76a1e0793bae25dd3a46209fab0f5fa9774e11721737f4820af8b1acdbb36739` |
| Debug dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release probe | `c517115c35544caeab10ae028a73d281425218081ae2382c28c01752d539bbf8` |
| Release dll | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Bu sonuç çalışma dizininin CRT tarafından okunup yazıldığı, dosya yöneticisinin döndüğü veya oyun initialize işleminin tamamlandığı anlamına gelmez. Renderer/doğal frame ve multiplayer kapıları açık kalır.

## Kod 0.1.29 bağlantısı

`--observe-cwd-seh` CRT wrapper ve SEH prologue içine doğal CALL açar; kayıt kurulup yardımcı döndüğünde 0x836E9D noktasında durur. Üç durakta 80-byte stack, 28-byte NT_TIB, önceki kayıt ve caller/buffer/localisation korunumu denetlenir. [Sözleşme ve sonuç](d1-cwd-seh.md). Önceki manager terminali korunur; yeni üst modda manager cwdCallAllowed=true, cwdSeh lockPathAllowed/directoryApiAllowed/cwdReturnVerified/unwindVerified=false olur. Handler veya kilit/OS/copy yolu açılmaz; owned child sonunda kapatılır, eski TEB/context rollback yapılmaz. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Kilit/cwd, SEH sökümü, manager/initializer dönüşü, renderer/doğal frame ve N2/N3/D1/D2 açıktır.

## 14 Eylül 2026 — Windows fixture taşınabilirliği

Startup-return ve devamındaki fixture zinciri, sistem DLL reçetelerini test makinesinin diskte tutulan PE dosyalarından çıkarır. Ortak okuyucu ve negatif doğrulamalar [test kapsamı notunda](d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği) açıklanır; üretim policy/hash kuralları ve bu belgedeki gerçek GTA kanıtının sınırları aynıdır.

## 0.1.33 — Windows profil bağı

[26200.9445 incelemesi](d1-system-profile-9445.md), bu bileşenin bağlı olduğu OS pinleri/API reçeteleri ve parent SHA-256 zincirini günceller. Bileşenin GTA durak/izin sınırı aynı kalır; önceki runtime sonuçları eski profil bağlamındadır. Yeni dosyalarla fixture ve gerçek GTA kanıtının kapsamı profil raporunda ayrıca izlenir. Eski OS pinlerine otomatik fallback yoktur; kaynak ve generated policy birlikte yeniden derlenir.

## Sonraki birleşik kesit — 0.1.36

Bu belgedeki terminal ve eski test kayıtları kendi sürümüne aittir. [File-manager-ready](d1-file-manager-ready.md) önceki zinciri tek komutta tamamlar: normal kilit bırakma, SEH sökümü, suffix ve CFileMgr dönüşü. Eski komut otomatik ilerletilmez; buradaki snapshot güncel final durum gibi yorumlanmaz. Genel initializer/renderer/frame ve D1/D2 açıktır.
