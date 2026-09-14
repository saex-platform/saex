# D1-N2 — CRT cwd girişi ve SEH kayıt kurulumu

Kod 0.1.29 / mimari v0.36. CRT girişi ve SEH kaydı kurulumu kesiti doğrulandı. [Durum](status.md) · [Önceki sınır](d1-file-manager-entry.md).

## Sorumluluk ve veri akışı

`--observe-cwd-seh <exe> <absolute-cwd>` önceki file-manager terminalinden CRT wrapper'a doğal CALL açar. Wrapper, 12-byte local alan ve scope adresini hazırlayıp 0x8286E0 SEH prologue yardımcısını çağırır. Yardımcı yığında hata kaydı kurar, önceki FS:[0] kaydını bağlar ve geri döner. Terminal 0x836E9D; sonraki PUSH 7 ve kilit yolu henüz çalıştırılmaz. Cwd sonucunu okumak, kayıt zincirini geri sökmek veya exception handler'ı yürütmek bu izin kapsamında değildir.

P, önceki 0x5386FB noktasındaki ESP, yani üst caller C−52'dir. Önkoşul EAX=EBX=ESI=0, EDI=24; ECX/EDX ve başlangıç EBP saklanır.

| Durak | Adres | ESP | EAX / EBP | FS:[0] |
|---|---|---|---|---|
| 1 — cwd wrapper entry | 0x836E91 | P−4 | 0 / saved EBP | Önceki head |
| 2 — SEH prologue entry | 0x8286E0 | P−16 | 0 / saved EBP | Önceki head |
| 3 — prologue döndü | 0x836E9D | P−48 | P−24 / P−8 | P−24 |

Yeni kayıt P−24'te dört uint32 taşır: önceki head, handler=0x825EA4, scope=0x88DF18, durum=0xFFFFFFFF. Scope tripleti `0xFFFFFFFF, 0, 0x836ECE`'dir; son adres cleanup yardımcısını gösterir. Handler ve cleanup kodu çalıştırılmaz; adreslerinin executable image içinde erişilebilir olması tam gövdelerinin onayı değildir. Gözlemci ilk beklenmeyen exception'da child'ı sonlandırır; exception dispatch/unwind test edilmez.

## ABI ve bellek sözleşmesi

[CwdSehSpec](../../include/saex/engine/cwd_seh.hpp), wrapper/prologue FrameTargetSpec'leri, scope/handler/cleanup RVA ve stop prefix taşır. [Portable doğrulayıcı](../../src/engine/loader/cwd_seh.cpp) 16-byte wrapper girişini, tam 59-byte prologue'u ve 12-byte scope'u admitted base üzerinden üretir. [Strict policy](../../contracts/engine/cwd-seh-policy.json) [üreticisi](../../tools/cwd_seh_policy.py), önceki manager policy SHA ve devamındaki executable/DLL zincirine bağlıdır. Bilinmeyen executable, yanlış parent/rel32/absolute operand, image aralığı, overlap veya bozuk gövde güvenli ret verir. Teknik veri ve komut adresleri CLI girdisi değildir; indirilen resource bu native observer API'sine erişmez.

ECX/EDX/EBX/ESI/EDI korunur. İlk iki durakta EFLAGS yalnız CPU breakpoint RF biti bakımından farklı olabilir. Son durakta `SUB ESP,EAX` için lhs=P−24, rhs=12, result=P−36 üzerinden CF/PF/AF/ZF/SF/OF hesaplanır; diğer bitler RF dışında korunur. TF/DF reddedilir. Son ESP=P−48 değeri, SUB sonucundan sonraki üç register PUSH'unu içerir. EAX bir başarı kodu değil, yeni kayıt adresidir.

80-byte yığın penceresi P−60'tan başlar ve P+20'de biter. İlk sekiz byte alt guard, sonraki 52 byte native yazım/ayrılmış alan, son 20 byte önceki buffer/maxlen/EDI/dönüş frame'idir. Başlangıç snapshot'ı alınır; local alanın sıfır olduğu varsayılmaz. Her durak için yalnız gerçekten yürütülen komutların yazımları değiştirilir; geri kalan bütün byte'lar aynı kalır.

| Son duraktaki adres | Beklenen uint32 |
|---|---|
| P−52 | 0x836E9D; prologue RET'in tükettiği ama bellekte kalan dönüş adresi |
| P−48 / P−44 / P−40 | saved EDI / ESI / EBX |
| P−36 / P−28 | Başlangıçtaki iki kullanılmayan local word aynen korunur |
| P−32 | P−48; saved stack pointer |
| P−24 / P−20 / P−16 / P−12 | Önceki head / handler / scope / 0xFFFFFFFF |
| P−8 / P−4 | saved EBP / 0x538700 cwd dönüş adresi |
| P … P+16 | Önceki buffer, 128, saved EDI, 0x53BB5F, 0x53EC30 |

FS selector üzerinden önceki last-error doğrulayıcısının bulduğu aynı TEB kullanılır; WOW64 TEB için sabit başka bir offset varsayılmaz. NT_TIB'nin ilk yedi word'ü (28 byte) okunur. Self eşleşmeli; alan writable/COMMIT/MEM_PRIVATE ve erişilebilir olmalıdır. Derleme tarafında x86 NT_TIB ExceptionList/StackBase/StackLimit/Self offset'leri 0/4/8/24 için static_assert bulunur. P−60 … P+20 pencere tamamıyla StackLimit/StackBase içinde olmalıdır. Önceki head ya 0xFFFFFFFF sentinel'dır ya da hizalı, pencerenin üstünde, aynı stack sınırları içinde okunabilir sekiz-byte kayıttır. Zincirin tamamı takip edilmez; mevcut ilk kaydın iki word'ü her durakta korunur.

İlk iki durakta yedi NT_TIB word'ü aynı; üçüncüde yalnız ExceptionList=P−24 değişebilir. Ayrıca 32-byte application/dispatcher frame, yaşayan 156-byte caller, last-error, named event kimliği, 136-byte root buffer/guard ve 16-byte localisation penceresi korunur. Observer yeni kesitte FS/TEB, oyun verisi, IP veya stack yazmaz; yalnız DR0 durağını taşır. Native komutların FS:[0] yazımı gerçek yan etkidir. Önceki platform context uyarlaması ayrı kanıt olarak kalır. Sonuçta owned child kapatılır; eski TEB/context değerleriyle rollback denenmez.

## Arayüzler ve hata davranışı

CLI `cwdSehObservation` içinde policy/digest, stage/stopAddress/recordAddress, previousHead/currentHead, continued/wrapperEntryReached/prologueEntryReached/prologueReturned, shapeValid/frameValid/memoryRead/memoryValid/priorRecordPreserved/callerPreserved/bufferPreserved/localisationPreserved/verified taşır. Ham stack/buffer JSON'a yazılmaz. memoryRead=false iken sıfır alanlar kanıt sayılmaz. lockPathAllowed, directoryApiAllowed, cwdReturnVerified ve unwindVerified false kalır. Manager nesnesinde cwdCallAllowed yalnız yeni üst modda true olur; bunun anlamı wrapper/SEH sınırına kadar izindir. Önceki `--observe-file-manager-entry` terminali korunur; eski modlarda yeni nesne null'dır.

Başarı `cwd_seh_verified` / exit 3; ret 1, kullanım 2. Ara verified, geçmişte tamamlanan checkpoint'tir. canAttach=false ve initializationVerified=false devam eder. Yanlış spec yürütmeden; gövde/scope/prefix farkı wrapper CALL önünde; geçersiz TIB veya stack ayrı önkoşullarda reddedilir. Devamdan sonra frame, NT_TIB, prior record, guard/local, caller, buffer veya localisation farkı başarı üretmez. Beklenmeyen exception, owner/thread, mapping veya kota hatası child kapanışıyla sonuçlanır. DR1 startup IAT, DR2 ekstra ASI ve DR3 existing-instance pencere kolu korunur. Bu kod diğer native thread'lerin bütün yazmalarını engelleyen bir sandbox değildir.

## Kabul senaryoları ve genişleme noktaları

Portable kontroller invalid parent/RVA/operand, overlap, relocation, TIB head hizası/sentinel/stack sınırları, üç register/flags frame'i ve stack/TIB word bozulmalarını kapsar. Windows fixture, aynı 59-byte x86 dizisiyle gerçek FS:[0] kaydı oluşturur. Üç invalid spec, üç runtime shape ret, event kotası, owner/recovery, eski manager terminali, existing-instance, post-SEH canary sensitivity ve 12 warm handle çevrimi sınanır. Fixture event adı `Local\SAEX.CwdSehFixture.v1`; handler ve cleanup adresleri export sayısını artırmadan kendi PE'sinden çıkarılır.

Sonraki araştırma 0x82ADBE lock(7) seçimidir: slot doluysa EnterCriticalSection yoluna, boşsa allocation/initialization yoluna ayrılır. Bu yollar mevcut izne dahil değildir. Ardından cwd helper'ın OS API çağrısı, dönüş kodu, NUL/ANSI byte uzunluğu ve copy sınırları; daha sonra unlock/SEH sökümü ve CFileMgr suffix/dönüşü doğrulanacak. Önceki 126/127-byte metin eşiği geçerlidir; path okunmuş veya buffer doldurulmuş sayılmaz. Exception handler kayıtlı olması, exception/unwind davranışının güvenli/doğru olduğu kanıtı değildir.

C++ observer/trace tüketicileri yeniden derlenir; bootstrap C ABI 1, GNS/HTTPS, otorite ve production IPC aynı kalır. Yeni dependency, kaldırılan özellik veya kalıcı migration yoktur. CRT cwd dönüşü, lock/unlock, SEH epilog/unwind, file manager/initializer dış dönüşü, streaming/pad, RsInitialize, renderer/doğal frame, production SDK/IPC/sandbox ve AC-90/AC-91/N2/N3/D1/D2 bütünü açık kalır.

## Kaynak ve doğrulama

Yerel EXE SHA-256 `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`; wrapper/helper/lock disassembly kaydı `out/verification/engine/cwd-seh-disassembly.txt`. Komut şekli, adresler ve scope tripleti yerel PE'den çıkarıldı. [Microsoft SEH açıklaması](https://learn.microsoft.com/en-us/windows/win32/debug/structured-exception-handling), kayıt/handler sisteminin genel bağlamıdır. [Microsoft TEB notu](https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-teb), iç yapının sürümler arasında değişebileceğini belirtir; buradaki erişim pinned x86 debugger deneyine aittir, genel platform API'si veya tüm Windows sürümlerine destek iddiası değildir. Yerel Windows SDK NT_TIB tanımı ve exact FS:[0] komutları birlikte kullanılır.

İlk hedef build, fixture içindeki elle FS:[0] yazımı için C4733 uyarısını /WX nedeniyle reddetti (`cwd-seh-build-initial.log`). Yalnız fixture prologue çevresinde açıklamalı push/disable:4733/pop kullanıldı; genel /WX/SafeSEH ayarı değişmedi. Bu fixture handler-dispatch testi değildir. `cwd-seh-build-second.log` geçti. İlk portable testte P=0x100000 için beklenen PF biti yanlış yazıldığı için 158. kontrol başarısız oldu; gerçek native fixture aynı koşuda geçti. Beklenti 0x212 olarak düzeltildi, farklı ESP değerleri için AF/PF örnekleri eklendi. `cwd-seh-tests-initial.log` başarısızlığı ve `cwd-seh-tests-second.log` başarılı tekrarı korur; üretim hesaplaması değiştirilmedi.

## Nihai doğrulama

Canonical kayıt `out/verification/engine/cwd-seh-gta-evidence.json`, derleme/hash özeti `cwd-seh-final-verification.json`. Yeni private dizinler `out/experiments/gta-cwd-seh-f01a00ce-{Debug,Release}-v1`; önceki dizinler ve orijinal oyun kurulumu korunur. İlk keşif `cwd-seh-explore-Debug-1.json` final 12 koşuya katılmaz; keşifte 242 girdi hash'i korundu.

| Ölçüm | Sonuç |
|---|---|
| Gerçek GTA pozitif | Debug 6/6, Release 6/6; 12/12 |
| Yeni doğal duraklar | 0x836E91 → 0x8286E0 → 0x836E9D |
| SEH kaydı | FS:[0] = P−24 = callerStack−76; önceki head, handler/scope/durum doğru |
| ABI/bellek | 80-byte stack, 28-byte NT_TIB ve eski kayıt denetlendi; yalnız tanımlı yazımlar kabul edildi |
| Korunan durum | 32-byte parent, 156-byte caller, register/flags, last-error, named event, root ve localisation pencereleri |
| Matris | 38/38: 12 pozitif, 19 legacy mod, beş olumsuz koşul, iki event çakışması |
| Child ömrü | Oluşturulan 35/35 süreç kapandı; üç koşulda child oluşmadı |
| Dosyalar | 250/250 hash korundu: önceki 235 + 14 private dosya + cwd-SEH policy |
| x86 Debug / Release | Her profilde 37 native suite / 79 managed / 240 Python |
| x64 Debug / Release | Her profilde 17 native suite / 79 managed / 174 Python |
| Yeni testler | 210 portable kontrol; 11 x86 senaryo, post-SEH canary ve 12 warm çevrim |

İlk derleme/test başarısızlıkları yukarıda ayrılmıştır. İlk başarılı warm sayımı 133 → 133; standart x86 akışlarında warm eşitliği tekrar geçti. `cwd-seh-fixture-initial-success.log` ilk başarılı test ayrıntısını korur. 268 kaynak/yapılandırma girdisi `cwd-seh-source-build-inputs.json` ve altı probe/DLL/map artifact hash'i final kontrolde aynı kaldı. Linux/hosted CI/N1 SDK yeniden koşulmadı.

Pozitiflerde 78–85 debug event, thread sayıları [4]; 128 event/16 thread/5 saniye kotaları korundu. Host foreground timeout yalnız SPI_GET ile 2147483647 → 2147483647 ölçüldü; bu eşitlik önceki bastırma/canary kanıtının yerine geçmez.

| Kimlik | SHA-256 |
|---|---|
| Cwd SEH policy | `995b9d5bc9223df40245583de1ba6226d04b372fc4585d046df039d7859cd198` |
| Debug probe | `a2ffa4bbb6872caee27ad0bce402e2ecfdb66e4d0f7c29ec4184c788ac5d0a90` |
| Debug dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release probe | `dd2869aa5aad9ba5e8dfef77db4c37c8e2befe71eefa1dbef01cfc995db97a1d` |
| Release dll | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

SEH kaydı kurulmuştur; handler yürütmesi, exception/unwind, kilit, dizin okuma veya CRT dönüşü doğrulanmış sayılmaz. Renderer/doğal frame ve multiplayer kapıları açık kalır.

## Kod 0.1.30 bağlantısı

`--observe-cwd-lock` lock(7) selector CALL ve ilk 17-byte gövdeyi doğal yürütür; CMP tamamlandığında 0x82ADCF JNE önünde durur. [Sözleşme ve sonuç](d1-cwd-lock.md). 100-byte stack, 16-byte slot penceresi, NT_TIB/önceki kayıt/caller/buffer korunur; slot değeri dereference edilmez. SlotPresent yalnız sıfırdan farklı word demektir, kritik bölüm veya kilit alma kanıtı değildir. Üst modda cwdSeh.lockPathAllowed=true yalnız selector iznidir; yeni branchAllowed/lazyInitializationAllowed/criticalSectionCallAllowed/lockAcquiredVerified=false. Önceki SEH terminali korunur; DR0 dışında yeni observer müdahalesi ve TEB/context rollback yoktur. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Mevcut/lazy dal, OS kilidi, cwd/SEH dönüşü ve N2/N3/D1/D2 açıktır.
