# D1 — CRT çalışma dizininin doğal kopyalanması

Kod 0.1.34 / mimari v0.41, 14 Eylül 2026. [Durum](status.md) · [Query](d1-cwd-query.md) · [OS profili](d1-system-profile-9445.md) · [ADR-69](../decisions/architecture-decisions.md). Ayrı native izin: `--observe-cwd-copy <exact-executable> <absolute-cwd>`.

## Sorumluluk ve veri akışı

Bu kesit, 0.1.33'te doğrulanan query dönüşünden sonra GTA'nın doğal kontrol dalları ve strcpy biçimindeki yordamını çalıştırır. Kaynak, helper'ın 260-byte local tamponundaki doğrulanmış dizindir; hedef `CFileMgr` için 128-byte GTA image tamponudur. Doğal cdecl yordam dönüşünde `0x836E8A` adresinde durulur. Argümanları POP etme, cookie/helper dönüşü, unlock(7), SEH sökümü, sona backslash ekleme ve CFileMgr/initializer dönüşü sonraki kesitlerdir.

Eski query komutu `0x836E30` adresinde durmaya devam eder. Yeni modda query'nin `verified=true` değeri önceki alt kanıttır; `copyAllowed=true` yeni izni belirtir. Yeni `cwdCopyObservation.copied/verified` ancak doğal kopya dönüşü ve bütün korunum denetimleri tamamlandığında true olur. Query'de görünen `rootPreserved` kendi API dönüş sınırına aittir; sonraki copy'nin hedefi değiştirmediğini söylemez.

### Beş durak ve ABI

T, önceki acquire terminalinin ESP'sidir. Query dönüşünde ESP=T−288, helper EBP=T−16, local kaynak=T−284; wrapper EBP=T+44'tür. N, NUL hariç dizin uzunluğudur; kabul edilen aralık 3–126 ASCII byte. R, GTA image hedefi `0x00B71AE0` adresidir.

| Durak | GTA adresi | ESP | Denetim |
|---|---|---|---|
| Non-null hedef JNE | `0x836E41` | T−288 | EAX=N+1, ECX=R; TEST ECX bayrakları, non-null dal |
| Kapasite JLE | `0x836E6B` | T−288 | EAX=N+1; CMP 128 signed/unsigned bayrakları; kabul edilmiş kapasite |
| Kopya CALL | `0x836E85` | T−296 | EAX=kaynak, ECX=R; stack=[R,kaynak] |
| Yordam entry | `0x826590` | T−300 | Doğal dönüş `0x836E8A`; stack=[dönüş,R,kaynak] |
| Kopya dönüşü | `0x836E8A` | T−296 | EAX=R; iki argüman hâlâ stack'te, EDI/EBX/ESI/EBP korunmuş |

Yordam içinde PUSH EDI nedeniyle en düşük kullanılan konum T−304'tür; private writable stack ve NT_TIB sınırı denetlenir. API dönüşünden ilk dört durağa kadar EDX korunur. Kopya dönüşünde ECX/EDX/aritmetik flags volatile kabul edilir; EAX dönüş hedefi, callee-saved register'lar ve kontrol flags denetlenir. TF/DF kabul edilmez; debugger resume flag'i hariç tutulur. İnceleme debugger checkpoint readback'idir, bütün makine komutlarının ayrı ayrı izlendiği iddiası değildir.

## Kaynak/policy ve native kapsam

[CwdCopySpec](../../include/saex/engine/cwd_copy.hpp), [portable doğrulama](../../src/engine/loader/cwd_copy.cpp), [strict policy](../../contracts/engine/cwd-copy-policy.json), [generator](../../tools/cwd_copy_policy.py) ve [observer](../../src/engine/loader/loader_observation.cpp) tek aile oluşturur. Policy, query JSON SHA-256'sına ve onun üzerinden 26200.9445 OS/GTA zincirine bağlanır. Yeni OS modülü veya runtime dependency eklenmez. Ayrı CALL/return izni, C ABI 1, GNS, otorite ve resource API'lerini değiştirmez; C++ observer yeniden derlenir. Kaldırılan davranış veya kalıcı veri migration'ı yoktur.

Exact `f01a00ce...` executable'dan salt okunur PE extraction ve `dumpbin /DISASM:BYTES` ile altı segment incelendi: kontrol 19 byte (`0x836E30`), kapasite 5 byte (`0x836E68`), argüman/CALL 13 byte (`0x836E7D`), copier entry 7 byte (`0x826590`), ortak copy gövdesi 131 byte (`0x826605`) ve dönüş prefix'i 16 byte (`0x836E8A`). Entry'nin kısa JMP'si aradaki farklı yordam girişini atlar. Oyun ve fixture aynı RVA/rel32 ilişkileriyle denetlenir; heap allocator veya errno/error dalları bu izin kapsamına alınmaz.

Copy gövdesinde dış CALL/IAT yoktur; kaynak hizalanana kadar byte, ardından hizalı DWORD okunur. Sıfır byte tespiti sonrası son 1/2/3/4 byte yazılır; sonlandırıcı dahil hedefin dışına yazı kabul edilmez. DWORD okuması NUL'dan sonra en çok üç kaynak byte okuyabilir. Query snapshot'ındaki 260-byte committed private tampon ve 3–126-byte sınırı bu okumayı kapsar. GTA helper kaynağı 4-byte hizalıdır; ayrı fixture gövde kontrolü hizalanmamış kaynak/hedef varyantlarını da sınar. Bu ek kontrol GTA'nın bütün olası çağıranlarının desteklendiği anlamına gelmez.

Native aralıklar çakışamaz; parent query terminali copy kontrollerinin ilk 16 byte'ıyla aynı olmalıdır. Generator bütün admitted segment byte'larını ve rel32 bağıntısını doğrular. Canlı checkpoint'lerde aynı image/prefix/API kimliği tekrar denetlenir. Kanıt dosyasından otomatik izin üretimi yoktur.

## Tampon ve yaşam süresi sözleşmesi

- Önceki query doğrulanmadan yeni durak kurulmaz. 127 byte ve üzeri dizin `cwd_query_suffix_capacity` ile copy başlamadan reddedilir. 128-byte hedefin sonradan backslash+NUL taşıması için 126-byte sınırı korunur.
- Hedef image üzerinde committed, writable ve aynı allocation/range içindedir. Hedefin 4 byte öncesi + 128 byte içi + 4 byte sonrası toplam 136 byte okunur. Dönüşte yalnız `[R,R+N]` dizin+NUL ile değişebilir; geri kalan her byte eski değeriyle aynı kalmalıdır. NUL sonrası kullanılmayan hedef byte'ları temizlenmiş varsayılmaz.
- Kaynak 260 byte bütünüyle değişmeden kalır; NUL/uzunluk/dizin eşleşmesi query snapshot'ından gelir. Kaynağın üstündeki 8-byte guard, encoded/global cookie ve helper saved EBX/EBP korunur. Kaynak/hedef çakışması ve adres taşması reddedilir.
- 84-byte parent stack, 32/156-byte dış caller, NT_TIB/FS head ve önceki SEH kaydı, alınmış kritik bölüm/slot, localization ve named-event kimliği korunur. LastError, önceki query API dönüş değerinde kalır; bu copy gövdesi OS API çağırmaz.
- Runtime kodu kaynak/hedef/stack/TEB'ye sentetik kopya yazmaz. Değişiklik oyunun kendi yordamından gelir. Önceki platform ayarı bastırması aynı kalır; yalnız DR0 yeni duraklara taşınır, mevcut DR1/DR2/DR3 korumaları sürer. Sonunda owned child bütünüyle kapatılır; kilit tutulurken process sonlandırması normal unlock/SEH unwind değildir.

## Arayüzler ve hata davranışı

`LoaderObservation::run_cwd_copy` ayrı açık C++ girişidir. JSON'a additive `cwdCopyObservation` gelir; eski modlarda null'dır. Beş checkpoint, function/source/destination adresleri, `copiedBytes` (NUL dahil), `destinationCapacity=128`, korunma bayrakları ve `copied/verified` ayrıdır. Ham stack/root byte'ları CLI JSON'a dökülmez. `helperReturnVerified/unlockVerified/sehRemovalVerified=false` bu kesitin sınırıdır. Başarılı gözlem + doğrulanmış exit kodu 3, ret 1, kullanım hatası 2'dir; production oyun başarı kodu değildir.

Yanlış/çakışan recipe `cwd_copy_invalid_spec`; query sonrası kod uyuşmazlığı `cwd_copy_precondition_shape`; checkpoint sırasında drift `cwd_copy_shape_drift`; ABI/argüman, kaynak/hedef, cookie/guard, SEH/kilit/caller/localization/LastError uyuşmazlıkları ayrı nedenlerle durur. Yanlış owner thread devam veya cleanup yapamaz; kendi owner'ı tekrar çağırabilir. Bilinmeyen executable/OS ve hatalı dosya pinleri önceki güvenli ret kapılarını korur. Bu süreç ayrımı bir native kod sandbox'ı değildir; denetimler arası race'lerin tümünü önlediği iddia edilmez.

## Kabul ve test kapsamı

[Portable corpus](../../tests/engine/cwd_copy_spec_tests.cpp) 3–126 byte uzunlukları, her hedef/guard byte'ının bozulmasını, NUL bozulması/erken NUL, adres/aralık/çakışma ve beş ABI durağının register/flags negatiflerini sınar. İlk koşuda **18.457 kontrol** geçti. [Native corpus](../../tests/engine/cwd_copy_tests.cpp): **13 senaryo**, bir gerçek post-copy canary kontrolü ve **12 warm çevrim**; ilk hedefli Debug koşusunda handle sayısı **143→143**. Ayrı fixture kopya gövdesi **1.984 uzunluk/kaynak-hizası/hedef-hizası kombinasyonu** için içerik, kaynak değişmezliği ve hedef guard'larını kontrol eder.

[Altı generator testi](../../tests/engine/test_cwd_copy_policy.py) her admitted code byte değişimini, bütün parent kaynak drift'lerini, scope/schema/duplicate/range retlerini kapsar. CLI bilinmeyen executable, eski modların null copy çıktısı/izin sınırı, yanlış argüman ve relative cwd retlerini ayrıca sınar. Standart build GTA başlatmaz. Gerçek GTA query/copy/regresyon ve 123–127-byte sınırları ayrı yerel matrisle doğrulandı; aşağıdaki gerçek GTA kanıtı bu fixture sonucundan ayrı tutulur.

İlk hedefli derlemede fixture testinde `code` yerel adı mevcut değişkenle çakıştı (C2373); `copy_code` olarak düzeltildi. İkinci hedefli derleme, üç yeni native suite, altı generator testi ve 53 CLI testi geçti. Ara loglar `out/verification/engine/cwd-copy-initial-build.log`, `cwd-copy-second-build.log`, `cwd-copy-initial-tests.log` içindedir. Nihai dört Windows akışı ve GTA sonuçları aşağıda ayrı kaydedilir; N2/N3/D1/D2 açık kalır.

## Tam akışta bulunan test yaşam süresi hatası

İlk tam x86 Debug akışında iki yeni suite başarısız oldu: gövde kontrolünü doğrudan fixture EXE ile çalıştırmak DLL başlangıcının `.dll-entered/.proxy-entered` dosyalarını geride bıraktı; sonraki observer testi `preexisting marker`, gövde testi ise DLL başlangıç reddi verdi. Kontrol, child ve marker ömrünü yöneten test runner'a taşındı; aynı gövde iki kez arka arkaya çalıştırılıp temizliğin tekrarlanabilirliği sınanır. İki suite ortak fixture için CTest resource lock kullanır. Başarısız akış `build-cwd-copy-x86-Debug-first-full.log` ve `cwd-copy-x86-Debug-first-full-native.log` ile saklandı; nihai dört akış bu düzeltmeden sonra yeniden koşuldu ve geçti. Üretim observer/CLI veya GTA izin kapsamı bu test düzeltmesiyle değişmedi.

## Genişleme ve sıradaki adım

Önce iki argümanın doğal temizliği ve cookie/helper dönüşü, sonra unlock(7), SEH epilogue ve CFileMgr dönüş/sonlandırıcı/backslash davranışı incelenecek. Bunlar tamamlanmadan çalışma dizini başlangıcının bütünü doğrulanmış sayılmaz. Daha sonra CdStream/CPad, initializer, renderer/doğal frame ve N3; ardından iki istemcili D2 sırası korunur.

## Nihai doğrulama — 14 Eylül 2026

| Windows akışı | Native suite | Managed | Python | Sonuç |
|---|---|---|---|---|
| x86 Debug | 46 | 79 | 270 | geçti |
| x86 Release | 46 | 79 | 270 | geçti |
| x64 Debug | 21 | 79 | 198 | geçti |
| x64 Release | 21 | 79 | 198 | geçti |

Dört standart akış **301/301 aynı kaynak/config hash'i** ile tamamlandı. Her iki x86 yapılandırmasında 18.457 portable kontrol, 13 native senaryo, post-copy canary, 12 warm çevrim ve **Debug 136→136, Release 143→143 handle** sonucu geçti. Aynı native copy gövdesi **1.984** uzunluk/kaynak-hizası/hedef-hizası kombinasyonunda kaynak ve hedef guard'larını korudu. İlk hedefli C2373 derleme hatası yukarıda kayıtlıdır; düzeltmeden sonraki dört tam akışta başarısız kontrol yoktur.

Gerçek GTA matrisi **54/54 beklenen sonuç**, **49/49 yaratılan child için doğrulanmış çıkış**, **175/175 girdi hash'i korunumu** verdi. Debug ve Release normal dizinlerinde altışar, 123/124/125/126-byte dizinlerde birer olmak üzere **20 pozitif doğal kopya** doğrulandı. Bu dört sınır dizini NUL'un DWORD içindeki dört konumunu kapsar. Normal Debug dizini 90, Release 92 byte; hedefe sonlandırıcı dahil 91/93 byte yazıldı. 127-byte dizinler iki yapılandırmada `cwd_query_suffix_capacity` ile kopyalama başlamadan reddedildi; copy stage=0 ve copied/verified=false kaldı.

Pozitif koşulların tamamında beş durak geçildi; doğal yordam `0x826590` → `0x836E8A`, hedef `0x00B71AE0`, EAX=hedef ve `copiedBytes=N+1` doğrulandı. Query kaynağı 260 byte boyunca aynı, 136-byte hedef penceresinde yalnız dizin+NUL değişmiş, diğer byte/guard'lar korunmuştu. Stack/caller/SEH/önceki kayıt, CRT kilit nesnesi ve cookie, localization, LastError ve named-event kimliği denetimleri geçti. Bu kanıt yalnız pinned `f01a00ce...` GTA / Windows 26200.9445 profiline aittir.

Eski query her iki yapılandırmada `0x836E30` terminalinde copyAllowed=false ve null copy kaydıyla geçti. Diğer **22 önceki mod** Release'te kendi terminalinde, acquire ayrıca Debug'ta geçti. Yanlış cwd context, yanlış ASI yapılandırması, eksik ASI, bozuk codec ve bilinmeyen executable child öncesi reddedildi. Mevcut named event `183`, aynı adlı mutex `6` ile copy/query başlamadan reddedildi. Deneyin manual-reset event'i sinyallenmedi; host setting önce/sonra aynı kaldı.

Orijinal oyun native dosyaları, OS dosyaları ve policy girdileri değişmedi. Bootstrap DLL/map hash'leri 0.1.33 ile aynı; probe'un varsayılan stack rezervi **1 MiB** olarak korundu. İlk Debug/Release keşif koşuları ayrıca başarılıydı; 54 koşuluk matrise dahil edilmedi. Yerel private kopyalar `out/experiments/gta-cwd-copy-f01a00ce-*-v1`, ham sonuçlar `out/verification/engine/cwd-copy-gta-evidence.json`, `cwd-copy-final-verification.json`, `cwd-copy-initial-*.json`, `cwd-copy-build-inputs.json`, `build-cwd-copy-*-final.log` ve `cwd-copy-*-native-final.log` içindedir.

| Artifact | Debug SHA-256 | Release SHA-256 |
|---|---|---|
| saex_engine_loader_probe.exe | `b111a5b890e560cc87ad0cf58b1a79b3c711981df7b8bf47c6503a1e95275d4a` | `88d588673067871731b3b1a359d2d250b0abd12d00d611d531e5a35adbde9aa5` |
| saex_bootstrap.dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| saex_bootstrap.map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Bu sonuç argüman POP/cookie/helper dönüşünü, unlock(7), SEH sökümünü veya CFileMgr suffix/dönüşünü kapsamaz. Owned process çıkışı kilidin normal bırakıldığı anlamına gelmez. Linux/hosted CI/N1 SDK bu kesitte yeniden çalıştırılmadı; N2/N3/D1/D2 ve production oyun başlangıcı açık kalır.

Son belge kontrolü **114 Markdown / 1.995 yerel bağlantı / 6 JSON örneği**, sıfır hata verdi. `git diff --check` geçti; çalışan gözlem veya copy fixture süreci kalmadı.

## Sonraki birleşik kesit — 0.1.36

Bu belgedeki terminal ve eski test kayıtları kendi sürümüne aittir. [File-manager-ready](d1-file-manager-ready.md) önceki zinciri tek komutta tamamlar: normal kilit bırakma, SEH sökümü, suffix ve CFileMgr dönüşü. Eski komut otomatik ilerletilmez; buradaki snapshot güncel final durum gibi yorumlanmaz. Genel initializer/renderer/frame ve D1/D2 açıktır.
