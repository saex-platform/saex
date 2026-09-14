# D1 — Dosya yöneticisinin tamamlanması ve yerel doğrulama aracı

Kod 0.1.36 / mimari v0.43, 14 Eylül 2026. [Durum](status.md) · [Önceki helper dönüşü](d1-cwd-return.md) · [ADR-71](../decisions/architecture-decisions.md). Bu sözleşme `CFileMgr::Initialise` fonksiyonunun tamamını kapsayan açık gözlem iznini tanımlar.

## Sorumluluk ve sonuç

`--observe-file-manager-ready` önceki başlangıç, query, copy ve helper-return zincirini çalıştırır; ardından wrapper temizliği, CRT unlock(7), SEH epilogue ve CFileMgr yol sonlandırmasını doğal oyun komutlarıyla tamamlar. Son durak **0x53BB5F**, CFileMgr çağrısının dönüş adresidir. Sonraki `PUSH 5` henüz çalışmaz. Observer yolu kendisi yazmaz, kilidi kendisi bırakmaz ve SEH zincirini sentetik olarak değiştirmez.

Bu sonuç dosya yöneticisinin başlangıç alt sistemini doğrular. Bütün initializer, renderer, doğal frame, attach/detach, N2/N3/D1 veya D2 multiplayer hazır olduğu anlamına gelmez. Normal dönüş ve SEH kaydının kaldırılması sınanır; bir exception atarak handler/unwind çalıştırılması bu izne dahil değildir.

## Veri akışı ve dokuz birleşik kontrol

T önceki acquire terminalindeki ESP; R=0xB71AE0; L doğrulanmış ASCII dizinin NUL hariç uzunluğu (3–126). Dış caller ESP=T+68; wrapper EBP=T+44. Tek üst düzey komut dokuz kontrolü arka arkaya yürütür; bunlar ayrı kullanıcı adımları değildir.

| Kontrol | Exact GTA adresi | ESP | Gözlenen sonuç |
|---|---|---|---|
| LeaveCriticalSection CALL önü | 0x82AD1F | T−16 | Wrapper ADD ESP,12, sonuç kaydı, try-state=-1; unlock index=7 ve aynı kritik bölüm argümanı |
| RtlLeaveCriticalSection girişi | admitted ntdll base+0x53460 | T−20 | Doğru IAT, API return adresi ve sahip olunan kilit |
| OS dönüşü | 0x82AD25 | T−12 | Kilit ilk boş durumuna dönmüş; recursion=0 ve owner=0 |
| SEH epilogue girişi | 0x82871B | T | Wrapper sonucu EAX=R; kayıt ve eski FS head hâlâ doğru |
| SEH epilogue dönüşü | 0x836ECD | T+48 | NT_TIB ilk görüntüyle aynı; önceki record korunmuş, EBP/nonvolatile register'lar geri gelmiş |
| Cwd wrapper dönüşü | 0x538700 | T+52 | [R,128,saved EDI,manager return] caller argümanları |
| NUL taraması sonu | 0x538718 | T+60 | EDI=R+L; suffix henüz yazılmamış |
| Suffix yazımı sonrası | 0x538721 | T+60 | Tam dizin+backslash+NUL; buffer ve çevre guard'ları doğru |
| CFileMgr dönüşü | 0x53BB5F | T+68 | Dış caller stack/nonvolatile durumu; managerReturned ve verified |

API VOID semantiğindedir: OS dönüşündeki EAX/ECX/EDX veya aritmetik flags başarı kodu sayılmaz. Kilit sonucu aynı 24-byte nesnenin ilk boş görüntüsüyle karşılaştırılır. İlk iki durakta kilit alınmış görüntüdür; üçüncüden sonra boş görüntüdür. DebugInfo/semaphore/spin ve slot komşuları değişmez. TF/DF ve kontrol flags drift'i reddedilir; RF debugger bitidir.

Her durakta kod/policy/IAT, aynı named event, localization, LastError ve 32/156-byte dış caller denetlenir. SEH silinene kadar önceki head/handler/scope ve try-state=-1, kaydedilmiş sonuç R olmalıdır. Silindikten sonra eski NT_TIB ve önceki record karşılaştırılır. Eski helper kaynağı retired'dır; stack yeniden kullanılabilir, bu bölgenin eski snapshot'ı yeni aşamada canlı veri olarak zorlanmaz.

136-byte hedef görüntüsünde (4 guard +128 buffer +4 guard) suffix öncesinde copy görüntüsü aynen korunur. Suffix sonrası yalnız eski NUL byte'ı backslash'a, sonraki byte NUL'a dönüşebilir. L=126 son yazılabilir iki byte'ı kullanır; L=127 parent query kapısında, kopya ve unlock öncesinde reddedilir. GTA'nın doğal davranışı aynen uygulanır: zaten backslash ile biten sürücü kökü için ayrıca normalizasyon yapılmaz.

## İncelenen kaynak ve izin zinciri

[Policy](../../contracts/engine/file-manager-ready-policy.json), parent return JSON SHA-256'sına bağlıdır. Parent zinciri exact GTA SHA-256 `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac` ve Windows 26200.9445 kimliklerini korur. Yeni fallback veya otomatik adres keşfi yoktur.

Salt okunur PE extraction ve `dumpbin /DISASM:BYTES` ile wrapper 24, cleanup 9, unlock 21, SEH epilogue 17 ve önceden incelenmiş CFileMgr 51 byte doğrulandı. Wrapper/cleanup CALL rel32'leri, CRT tablo mutlak adresi ve IAT operandı kod üretiminde yeniden hesaplanır. SEH scope cleanup hedefi ile normal cleanup çağrısı aynı olmalıdır. IAT 0x8580E8, executable import tablosunda `KERNEL32.dll!LeaveCriticalSection`'dır; çalışma zamanında pinned ntdll içindeki `RtlLeaveCriticalSection` RVA 0x53460'a çözülmesi ayrıca denetlenir. Export 20-byte prefix'i `8bff558bec83ec0853568b750857834608ff0f85`; relocation tablosunun taranması bu prefix içinde HIGHLOW olmadığını doğruladı. Aynı ntdll SHA-256'sı `7e15bd30890e9bf93b47fc894a68b2445618ee7528eb39584a263b21e112f9df` korunur.

İlk statik kayıttaki kaba caller adresi kanıt olarak kullanılmadı; final caller, mevcut manager policy'sindeki CALL RVA +5 üzerinden **0x53BB5F** olarak düzeltildi. Komşu fonksiyonlardan taşan extraction byte'ları izin gövdesine alınmadı. Statik kanıt `out/verification/engine/file-manager-ready-static.json` içindedir; runtime bunun dosyasından yetki üretmez.

## Arayüzler, hata ve uyumluluk

[FileManagerReadySpec ve observation](../../include/saex/engine/file_manager_ready.hpp), [portable doğrulayıcı](../../src/engine/loader/file_manager_ready.cpp), [strict generator](../../tools/file_manager_ready_policy.py) ve `LoaderObservation::run_file_manager_ready` yeni girişlerdir. Büyük trace kayıtlarının mevcut heap/tek dönüş düzeni korunur; stack rezervi büyütülmez.

CLI JSON'a `fileManagerReadyObservation` eklenir; eski modlarda null'dır. `lockReleased`, `sehRemoved`, `wrapperReturned`, `suffixWritten`, `managerReturned` ve `verified` ayrı kanıtlardır. `directoryHex` 136-byte hedef readback'idir. `checkpointRecordsAreSnapshots=true`: parent kayıtları kendi eski duraklarındaki görüntülerdir; örneğin acquire kaydındaki owner/recursion son terminalin kilit durumunu göstermez. Nihai durum yeni ready kaydından okunur. `canAttach` ve `initializationVerified` false kalır.

Başarılı sınırlı gözlem ve child çıkışı CLI kodu 3; ret 1; kullanım hatası 2. Reçete retleri `file_manager_ready_invalid_spec`, yürütme öncesi byte/IAT retleri `file_manager_ready_precondition_shape`, sonraki drift `file_manager_ready_shape_drift` olur. ABI, kilit, SEH/önceki record, wrapper durumu, argüman, caller, hedef, LastError, localization ve event için ayrı ret nedenleri vardır. Hata ya da timeout halinde owned child sonlandırılır; sonlandırma doğal cleanup kanıtı sayılmaz. Şüpheli native kod için bu araç genel bir güvenlik sandbox'ı değildir.

Eski `--observe-cwd-return`, copy, query ve acquire terminal izinleri aynıdır. C++ tüketiciler yeniden derlenir; C ABI 1, C++20/.NET10, GNS/HTTPS, ağ kimliği/otorite ve kalıcı veri sözleşmeleri değişmez. Yeni dependency, veri migration'ı veya kaldırılan özellik yoktur.

## Kullanıcının çalıştıracağı somut araç

Depo kökünde, x86 Debug build hazırken:

```powershell
./tools/Run-SAEX.ps1
```

[Run-SAEX.ps1](../../tools/Run-SAEX.ps1) Python gerektirmez. Windows PowerShell 5.1 ve PowerShell 7 için script UTF-8 BOM, konsol ve rapor çıkışı UTF-8 kullanır. Varsayılan kurulum kullanıcının bildirdiği Rockstar Games klasörüdür; `-GameDirectory` ile değiştirilebilir. Release için `-Configuration Release`, varsayılan tarayıcıda rapor açmak için `-OpenReport` verilir. Derleme otomatik başlamaz; eksik artifact raporda açık hata olur. Standart [build komutu](workflow.md) önceden çalıştırılır.

Araç her seferinde `out/local-demo/<zaman>-<kimlik>/game` altında yedi dosyalı yeni özel kopya oluşturur. Gerekli oyun girdilerinin boyut/hash'leri policy ile, kopyalar kaynak hash'iyle doğrulanır; runtime OS/ASI admission'ını probe yapar. Orijinal oyun klasörüne yazılmaz. Sistem DLL'leri kopyalanmaz. Başka kurulum modları/ASI'lar alınmaz; bu sonuç orijinal modlu klasörün tümünün çalıştığı anlamına gelmez. Aynı adlı eski rapor/kopya silinmez. Yerel rapor dosyaları ve oyun kopyaları Git dışındadır ve yayımlanmaz.

Akış: dosya kimliği → ayrı kopya → tek native probe → evre kanıtları → owned child exit → izlenen kaynak/kopya hash'lerinin tekrar kontrolü → HTML/JSON rapor. Yol hazırlama veya admission hatasında kısmi rapor üretilir, çalışılmamış aşamalar başarılı gösterilmez. Rapor üretilemeyen filesystem hatası PowerShell hatasıdır. Wrapper kodu 0 ancak native kod 3, ready=true, childExitConfirmed=true ve inputHashesPreserved=true birlikteyse döner; ret kodu 1'dir. Rapor HTML'i çevrimdışı çalışır ve yerel yolları escape eder; ham JSON yerel yollar içerir.

Bu bir D1 motor doğrulama aracıdır. Oyun penceresi/oynanış, production launcher, server browser veya multiplayer gösterimi değildir. Rapor, tamamlanan evrelerle motor başlangıcı → doğal frame → iki istemcili örnek yolunu birlikte gösterir.

## Kabul ve test kapsamı

[Portable test](../../tests/engine/file_manager_ready_spec_tests.cpp) adres/range/overlap, dokuz ABI durumu, volatile/nonvolatile ayrımı, 3–126 uzunluklarında suffix ve guard bozulmalarını sınar. [Native fixture](../../tests/engine/file_manager_ready_tests.cpp) gerçek RtlLeaveCriticalSection ve elle kurulmuş SEH çerçevesinin doğal sökümünü yürütür; yanlış reçete/IAT/API/epilogue, bütçe, owner recovery, yanlış cwd, mevcut event, eski copy/query/acquire terminali ve warm handle dengesini sınar. Post-manager canary yeni terminalin sonrasındaki kodun çalışmadığını kontrol eder. Fixture bir gerçek exception unwind testi değildir.

[Policy testleri](../../tests/engine/test_file_manager_ready_policy.py) schema, parent, byte, address, overlap, OS kimliği ve duplicate anahtar retlerini; [CLI testleri](../../tests/engine/test_loader_probe.py) yeni iznin bilinmeyen exe ve argüman retlerini kapsar. Standart build GTA başlatmaz; gerçek GTA ve Run-SAEX ayrı yerel kabul koşularıdır. AC-90'ın bu alt sonucu genel AC-90/R/N2/N3/D1/D2 kapısını kapatmaz.

## Bu kesitteki doğrulama kaydı

İlk derleme `naked` forward declaration ve DWORD/uint32_t auto-list tür farkında durdu. Forward declaration düz C++ bildirime, yerel adresler açık uint32_t tipine düzeltildi; ilk log korunur. İkinci hedefli Debug build ve iki native suite geçti. İlk gerçek GTA koşusu dokuz checkpoint, 0x53BB5F dönüşü, kilit bırakma/SEH kaldırma/suffix/caller doğrulaması ve owned child exit verdi. İlk Run-SAEX çalıştırmasında sekiz kontrol ve 17 izlenen girdi hash'i geçti. İlk runner negatif testinde Windows PowerShell konsol encoding farkı saptandı; UTF-8 BOM/çıktı ile düzeltildi, test beklentisi gevşetilmedi. Eksik klasör ve bilinmeyen exe için oyun başlatmadan başarısız rapor testleri standart x86 akışına eklendi. Windows PowerShell pozitif koşusunda Get-FileHash komut keşfi de başarısız oldu; hash okuma .NET SHA256/FileStream ile modül autoload bağımlılığından çıkarıldı. İlk standart Release çağrısı, Debug baseline sonrasındaki runner hash düzeltmesinin sahip belge güncellemelerini eksik buldu ve derleme başlamadan durdu; ilgili sekiz sahip belge güncellendi. Nihai sonuçlar aşağıdadır.

HTML dosyasını browser aracında açma girişimi, yerel file URL politikası tarafından reddedildi. Alternatif browser/HTTP dolaşımı denenmedi; bu teslimatta tarayıcı görüntüsü veya screenshot doğrulaması yoktur. Raporun üretilmesi, UTF-8 içeriği, JSON kanıt bağları ve başarısızlık davranışı doğrulandı.

## Sonraki iş paketi

Önceki [initializer incelemesinin](d1-application-routing.md) kalan iki CALL hedefi 0x406B70 (argüman 5, CdStreamInit) ve 0x541D90 (CPad) ile bunların kaynak eşlemesi/yan etkileri incelenir; mümkün olan fonksiyon grupları tek anlamlı kesitte uygulanır. Hedef önce bütün başlangıç/renderer, ardından doğal frame ve N3 yaşam döngüsü, daha sonra D1 kapıları kapanınca GNS üzerinden iki istemcili ortak dünya örneğidir. Her instruction için ayrı teslimat yapılması mimari zorunluluk değildir. Hedefli kontroller uygulama sırasında, standart build ve kabul matrisi tamamlanan davranış kesitinde çalıştırılır; aynı değişmemiş dört matrisi her ara düzeltmede tekrar etmek gerekmez.

## Nihai doğrulama — 14 Eylül 2026

| Akış | Native suite | Managed | Python | Sonuç |
|---|---|---|---|---|
| x86 Debug standart build | 50 | 79 | 285 | geçti |
| Debug runner ek doğrulaması | — | — | 2 | güncel Run-SAEX ile geçti |
| x86 Release standart build | 50 | 79 | 287 | geçti |
| x64 Debug hedefli portable build | 1 | çalıştırılmadı | çalıştırılmadı | geçti |

Her iki x86 yapılandırmasında **1.760 portable kontrol**, **14 native senaryo**, bir post-manager canary ve **12 warm çevrim** geçti. Handle dengesi Debug **139→139**, Release **139→139**. Debug standart build tamamlandıktan sonra runner hash uyarlaması yapıldığı için onun iki testi ayrıca güncel kaynakla çalıştırıldı; Release standart script bu iki testi de içerir. Native kaynak/artifact bu runner düzeltmesiyle değişmedi. X64 Release, Linux, hosted CI ve N1 SDK yeniden çalıştırılmadı; bu kesit için yeni portable kod x64 Debug'ta derlenip sınandı.

Gerçek GTA matrisi **58/58 beklenen sonuç**, **20 tam CFileMgr dönüşü**, **53/53 oluşturulan child için doğrulanmış çıkış** ve **177/177 izlenen girdi hash korunumu** verdi. Debug/Release normal dizinlerinde altışar ve 123/124/125/126-byte dizinlerde birer pozitif dönüş; 127-byte için iki erken kapasite reddi vardır. İki yapılandırmada eski return/copy/query terminalleri, Release'te diğer 22 eski mod ve Debug'ta acquire regresyonu geçti. Yanlış context/ASI yapılandırması, eksik ASI, bozuk codec, bilinmeyen exe ve named event/mutex çakışmaları beklenen retleri verdi. Deney event'i sinyallenmedi; host setting değişmedi.

Run-SAEX, Windows PowerShell 5.1 üzerinden Debug ve Release'te sekizer doğrulama ve 17 izlenen girdi hash kontrolü ile **0** döndü. Eksik klasör ve bilinmeyen exe için rapor **failed**, süreç hiç başlatılmadan **1** oldu; eksik trace dosyasına HTML link verilmedi. Önceki PowerShell 7 pozitif akışı da geçmiştir. Raporlarda genel oynanış/multiplayer hazır gösterilmez.

Son kaynak dondurmasındaki **319/319 source/config hash'i** korundu. Aynı gerçek GTA matrisinde kullanılan probe/bootstrap/OS/oyun girdileri final build sonunda tekrar aynı hash'leri verdi; kanıt eski artifact'a ait kalmadı. Bu tur 19 source/config ve ilgili belgeler değişti; 31 sahip belge co-change kontrolünden geçti, eski workspace dosyası kaldırılmadı. Orijinal oyun dosyaları değiştirilmedi; remote/push/deployment yoktur.

Kanıtlar `out/verification/engine/file-manager-ready-final-verification.json`, `file-manager-ready-gta-evidence.json`, `file-manager-ready-demo-evidence.json`, `file-manager-ready-final-inputs.json`, `file-manager-ready-review.json`, `build-file-manager-ready-x86-*-final*.log` ve `file-manager-ready-x86-*-native-final.log` altındadır. İlk başarısız build/runner/admission raporları korunmuştur. Private kopyalar ve HTML raporları `out/experiments/` ile `out/local-demo/` altındadır.

| Artifact | Debug SHA-256 | Release SHA-256 |
|---|---|---|
| saex_engine_loader_probe.exe | `742fd232b1c1c794115a51193a4863adf5fb18a6f5ceddcca550fd9cc5b90b59` | `36096e436cca032c513427fb9325696ce66aa26072fc9e8cea64530b8ea7e7fc` |
| saex_bootstrap.dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| saex_bootstrap.map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

## Kod 0.1.37 — Streaming tablo kesitiyle bağlantı

[CdStream tablo sözleşmesi](d1-cd-stream-tables.md) ortak observer/CLI ve fixture zincirine ayrı bir üst mod ekler. Bu belgenin eski komut ve checkpoint sınırı korunur; yalnız `--observe-cd-stream-tables` tam manager dönüşünden sonra iki tablo döngüsünü ve disk argüman hazırlığını açar. Sonuç yeni `cdStreamTablesObservation` alanında izlenir; eski kayıtlar final durum değil önceki checkpoint snapshot'ıdır. C++ trace tüketicileri yeniden derlenir; C ABI 1, GNS, OS pinleri ve production IPC sınırı değişmez. Yeni portable/native testler ile eski mod regresyonları standart build'e dahildir; gerçek GTA ve platform bazındaki final kanıt ana raporda tutulur.

`Run-SAEX.ps1` artık streaming tablo terminaline ulaşır ve dokuz kontrol sunar; disk okuma/thread başlatma bu kullanıcı komutunun kapsamında değildir. [Yerel Ghidra aracı](../references/ghidra-bridge.md) opt-in araştırmadır; normal build ek runtime kurmaz.

## Kod 0.1.38 — Disk sonucu ve allocation önkoşulu

[Disk hazırlığı sözleşmesi](d1-cd-stream-disk.md) önceki native zincire ayrı `--observe-cd-stream-disk` modu ekler. BOOL başarısızsa dört output kullanılmadan ret; başarılı ve kabul edilen mantıksal geometride doğal bayrak/argüman hazırlığı, 0x406BF4 allocation CALL önünde doğrulanır. Eski modların terminal ve snapshot anlamı korunur. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS, network otoritesi ve sandbox kapsamı değişmez. Gerçek allocation, fiziksel hizalama, dosya okuma ve thread/renderer hazır kanıtı bu değişiklikten çıkarılamaz. Portable hata kararı ile native/gerçek GTA kanıtının ayrımı yeni raporun kabul tablosunda izlenir.

Run-SAEX yeni en ileri terminali kullanır; disk geometri ve allocation argümanı iki ayrı kontrol, source/probe policy digest eşleşmesi rapor önkoşuludur. Yedi dosyalı özel kopya, orijinal dosya korunumu, eski çıktıların saklanması ve kısmi hata raporu devam eder.

## Kod 0.1.39 — Hizalı tamponun doğal dönüşü

[Allocation sözleşmesi](d1-cd-stream-allocation.md) ayrı `--observe-cd-stream-allocation` API/CLI ile MallocAlign → CRT → HeapAlloc → back-pointer → 0x406BF9 doğal dönüşünü ekler. Heap modu/new-handler/SBH dalı yürütmeden önce denetlenir; NULL, taşma, metadata ve payload bütünlüğü guard'ları vardır. Eski alt modların terminalleri ve snapshot anlamı korunur; yeni mod 160, eskiler 128 olay üst sınırındadır. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS ve sandbox kapsamı değişmez. İlk gerçek GTA allocation geçti; güncel toplu kanıt yeni sözleşmede izlenir. Native free, I/O/thread, renderer ve D1/D2 hazır kabul edilmez.

Run-SAEX 12 sonuç kontrolü ve 19 hash girdisiyle allocation modunu raporlar. Disk/allocation policy digest eşleşmeleri zorunludur. Boş veya sonuç alanı eksik probe JSON durumunda exit code içeren açık hata üretilir.

## Kod 0.1.40 — Kanal belleği kesiti

[Yeni sözleşme](d1-cd-stream-channels.md) `run_cd_stream_channels` / `--observe-cd-stream-channels` ile SetLastError ve LocalAlloc doğal yolunu, 5 × 48 sıfır byte ve global pointer kaydını ekler. Terminal 0x406C34, arşiv CALL önüdür. Önceki allocation/parent kayıtları kendi duraklarının snapshot anlamını korur; canlı tabloda yalnız kanal sayısı/etkin sayı DWORD çifti değişebilir. C++ trace tüketicileri yeniden derlenir; C ABI 1 ve mevcut OS pinleri aynıdır. Allocation ve yeni mod 160, daha eski modlar 128 olay sınırındadır. Native free, dosya açma/okuma, thread, renderer ve D1/D2 kapıları açıktır. Güncel test ve GTA kanıtı yeni sözleşmede tutulur.

Run-SAEX güncel üst modla 13 kontrol, 20 hash girdisi ve üç policy/probe digest eşitliğini raporlar.

## 14 Eylül 2026 — Linux fixture derleme düzeltmesi

0.1.40 GitHub yayınında GCC strict uyarısı, file-manager portable testindeki tek satırlık döngü/terminator yazımını reddetti. Döngü gövdesi süslü parantezle açıklaştırıldı ve terminator ayrı satıra alındı; test koşulları, üretim davranışı ve strict -Werror aynı kaldı. İlk hosted ret [yayın raporunda](github-publication.md) tutulur.
