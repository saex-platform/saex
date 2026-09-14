# D1-N2 — Tek platform çağrısının süreç içinde bastırılması

Kod 0.1.23 / mimari v0.30; sınırlı çağrı bastırma/restore kesiti doğrulandı. [Durum](status.md) · [Önceki doğal sınır](d1-platform-startup.md).

## Sorumluluk ve kapsam

Yeni `--observe-platform-suppression <exe> <absolute-cwd>` yalnız owned x86 child'ın doğrulanmış ana thread'inde, `SystemParametersInfoA` CALL önündeki 0x748727 durağında çalışır. Önceki platform modu aynı terminali korur. Sistem ayarı API'si yürütülmez; yeni yöntem kod/IAT/yığın belleği değiştirmeden EIP/ESP/EAX bağlamını uyarlayan yerel debugger deneyidir. Pencere, renderer ve oynanabilir multiplayer değildir. Bootstrap DLL/C ABI 1 değişmez; observer yeniden derlenir.

## Sözleşme ve veri akışı

1. Bütün eski EXE/pin/loader/ASI/CRT/application/platform kapıları geçer. CALL, tam prologue, return prefix, aktif user32 mapping/export, dört argüman `[8193,0,0,2]`, 152-byte prologue frame'i ve register'lar yeniden doğrulanır.
2. Ana thread register görüntüsü, 172 byte yığın ve thread'in last-error değeri alınır. GetLastError IAT'si 0x858094'tür. Pinned kernel32 export'unun exact 20 byte'ı karşılaştırılır; ilk yedi byte `64 A1 34 00 00 00 C3`, `MOV EAX,FS:[0x34]; RET` anlamındadır. Offset başka OS sürümüne genellenmez. `GetThreadSelectorEntry` ile FS tabanı okunur; SDK `NT_TIB.Self` bağı da eşleşmelidir. Yeni DLL kabulü, getter çağrısı veya TEB yazımı yapılmaz.
3. `SuppressionTransaction` tekrar okuduğu başlangıç görüntüsünün eşitliğini ister. EIP=CALL+6, ESP=önceki ESP+16, EAX=0 yapılır. CALL çalışmadığı için yığına return address itilmemiştir; yalnız dört stdcall argümanının tüketimi temsil edilir. ECX/EDX dahil diğer genel register'lar ve segment kimlikleri korunur. DR0 dönüş komutuna alınır; DR1/DR2 korumaları tutulur. DR6 event bitleri sıfırlanır, RF temizlenerek yeni execution breakpoint'in atlanması önlenir; TF/DF kabul edilmez.
4. Yazılan bağlam geri okunup tam ilgili alanlarla karşılaştırılır. Devamdan sonra 0x74872D'de ilk dönüş komutu çalışmadan durulur. EAX=0, ESP+16, korunan register/segment/flag'lar, eski 172-byte yığın ve last-error tekrar doğrulanır. CPU'nun breakpoint RF/DR6 değişimi ayrı ele alınır.
5. Başlangıç bağlamı geri yazılır ve tekrar okunur; hiçbir oyun komutu yürütülmeden child sonlandırılır. Dönüş komutu olan sonraki instance kontrol CALL'u bu kesitte çalıştırılmaz.

EAX=0 **SAEX'in bastırılmış istek sonucu**dur. Windows API'sinin gerçek başarısızlığı veya doğal CALL/RET sonucu diye sunulmaz; last-error korunması SAEX sözleşmesidir, Windows başarısızlık emülasyonu değildir. Exact sonraki komut bir CALL'dur; çağıran bu aralıkta EAX/last-error okumaz. İleride daha fazla gövde çalıştırmadan veri bağımlılıkları ayrıca incelenir. Genel API sanallaştırması veya production uyarlama DLL'si uygulanmadı.

## Hata davranışı ve geri alma

Yanlış policy, export, selector/TEB bağı veya başlangıç frame'i bağlam yazılmadan reddedilir. Yazma başarısız olsa bile kısmi etki olabileceği varsayılır: transaction, child aynı debug durağındayken özgün bağlamı geri yazmayı ve geri okumayı dener. Başarısız geri okuma başarı sayılmaz. İlk bağlam okunamadıysa yazma/rollback yoktur. Tek transaction tekrar uygulanamaz veya iki kez restore edilemez.

Devam izni `continued`, dönüş durağı `returnReached`, uygulama `applied`, geri alma `restored` ayrı alanlardır. Devam sonrası beklenmeyen exception, zaman/kota veya owner kaybında süreç sonlandırılır; bilinmeyen yürütme fazında eski bağlama dönüp oyun sürdürülmez. Bu yolda `restored=false` olabilir ve başarı verilmez. `childExitConfirmed` her ret ve başarıda ayrı kontrol edilir. Bağlam rollback'i dünya/OS işlemlerinin geri alınması değildir; sandbox veya hostile native code güvenlik sınırı değildir.

## Arayüz ve genişleme

`PlatformSuppressionSpec` strict JSON'dan üretilir; parent platform digest'i bütün exact profil zincirine bağlıdır. Sunucu/script/raw adres girişi yoktur. `SuppressionContextPort` yalnız yerel test edilebilir transaction arayüzüdür; SDK/IPC yüzeyi değildir. Yeni `platformSuppressionObservation` diğer komutlarda null olur. Başarı reason `platform_suppression_verified`, exit 3; ret 1, kullanım 2; `canAttach=false`, `initializationVerified=false`, `naturalApiReturn=false` korunur. Özellik kaldırılmadı; eski JSON alanlarının anlamı değişmedi.

## Kabul ve sonraki adım

Portable transaction testleri partial write, stale read, readback drift/failure ve başarısız rollback'i kapsar. x86 fixture, gerçek setter yerine pinned canary kullanır; gözlemsiz kontrol canary'nin çalışabildiğini kanıtlar. Yeni fixture son hata değerini `SetLastError(0x1234ABCD)` ile hazırlar; gözlem bu değerin korunmasını ister. Argüman/yığın/IAT/target/exception/owner/kota retleri ve eski komut sınırları tekrar sınanır. Gerçek GTA yalnız ayrı özel kopyalarda çalıştırılır; kaynak oyun ve önceki kanıtlar korunur. Ölçülmüş sonuçlar aşağıdadır.

Sonraki kapı, bastırılmış dönüşten sonraki instance denetiminin nesne adı, mevcut instance, yan etkiler ve dönüş ABI'sini doğrulamaktır; ardından dosya/pencere/renderer/asset ve doğal frame gelir. Bu kesit AC-90'ın bir alt kanıtıdır; N2, N3–N7, D1/D2, population ve hayvan çalışma zamanı açık kalır.

## Kaynaklar

[Microsoft SystemParametersInfoA](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-systemparametersinfoa) sistem ayarı çağrısının semantiğini açıklar. [GetThreadSelectorEntry](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getthreadselectorentry) descriptor tabanını okumayı; [debugger ana döngüsü](https://learn.microsoft.com/en-us/windows/win32/debug/writing-the-debugger-s-main-loop) debug olayı sırasında thread bağlamı inceleme/değiştirme sınırını destekler. Exact adres/export/offset kanıtı yerel binary'lerden gelir; `out/verification/engine/suppression-last-error-source.json` hash ve öneki kayıtlar. API başarısı bu kaynaklardan çıkarılmaz.

## Windows bağlam ayrıntıları ve ilk denemeler

EFLAGS bit 1 mimaride sabit 1'dir; bu WOW64 profilinin GetThreadContext geri okumasında 0 görülebildi. Yalnız bu sabit bit karşılaştırmada normalize edilir, her SetThreadContext yazısında 1 hazırlanır; aritmetik/kontrol flag'ları genel maskeyle saklanmaz. DR6 debug olayının nedenidir, uygulama register'ı değildir: apply ve restore'da sıfırlanır, geçmiş breakpoint nedeni tekrar yazılmaz. Başarıdaki restore; özgün EIP/ESP/tüm genel register/segment/uygulama flag'ları ve breakpoint adres/izinlerini geri doğrular; debug olay bitlerinin byte-byte tekrarını vaat etmez. Return hit'teki RF ayrıca CPU olayı olarak normalize edilir. [Intel SDM Volume 1, EFLAGS şekli](https://cdrdv2-public.intel.com/819711/253665-sdm-vol-1.pdf) sabit bit ayrımını; [SetThreadContext](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadcontext) bazı CPU context alanlarının işletim sistemi tarafından düzeltilebildiğini açıklar. Getter/selector/restore eşleşmezse güvenli ret devam eder.

İlk build değişken adı çakışması nedeniyle başarısızdı; isim ayrıldı. İkinci x86 Debug'da virtual destructor ile MSVC /Od'nin çok sayıda LoaderTrace dönüş kopyası üretmesi, run_impl stack rezervini 0x2F9858'e çıkarıp eski native corpuslarında da stack overflow oluşturdu. Borrowed context port'un protected trivial destructor'u ve static_assert ile düzeltildi; takip eden disassembly'de rezerv 0x87F0 oldu (sonraki tanı alanları nedeniyle final byte sayısı ayrıca değişebilir). Stack rezervi/linker kotası artırılmadı. Sonraki hedef testleri EFLAGS bit 1 ve DR6 restore uyuşmazlıklarını yakaladı; yukarıdaki açık karşılaştırma/restore sözleşmesi eklendi. İlk başarısız loglar `build-suppression-x86-Debug-{first,second,third}.log`, tanı disassembly'leri ve CTest kayıtlarında korunur. Son başarılı doğrulamalar aşağıdaki final kayıtlara bağlıdır.

Context transaction'ın ilk ret sonucu da terminaldir; reddedilmiş transaction ile yeniden yazma denenmez. `applyFailure` yalnız apply aşamasının hata adını taşır; rollback sonucu ayrı `restored` alanındadır. Model port arızaları native Win32 API arızalarının birebir enjeksiyonu değildir; gerçek API'nin başarı/geri okuma/restore yolu x86 corpus ve GTA'da ayrıca sınanır.

## Nihai doğrulama

Canonical kayıt `out/verification/engine/suppression-gta-evidence.json`; final derleme/hash özeti `suppression-final-verification.json`. Ön deneme `suppression-explore-Debug-1.json` 12 final koşuya dahil değildir. Yeni yedi dosyalı özel kopyalar `out/experiments/gta-suppression-f01a00ce-{Debug,Release}-v1`; önceki kopyalar ve orijinal GTA kurulumu korundu.

| Ölçüm | Sonuç |
|---|---|
| Gerçek GTA | Debug 6/6, Release 6/6; 12/12 |
| Sınırlı ilerleme | 0x748727 CALL bastırıldı; 0x74872D'de sonraki komut önünde durdu |
| Dönüş durumu | EAX=0 ve ESP+16 oluştu; 172-byte yığın, diğer genel register/segment/uygulama flag'ları ve last-error korundu |
| Geri alma | 12/12 CALL öncesi uygulama bağlamı/breakpoint adres ve izinleri geri okundu; DR6 neden bitleri temizlendi |
| Matris | 30/30 beklenen sonuç: 12 yeni pozitif, 13 eski komut, beş olumsuz koşul |
| Süreçler | Oluşturulan 27/27 child kapandı; üç olumsuz koşulda child oluşmadı |
| Girdiler | 160/160 hash korundu: önceki 145 + yeni özel kopyaların 14 dosyası + suppression policy |
| Kaynak/artifact | 220 kaynak/yapılandırma girdisi ve altı x86 artifact final kontrolde aynı |

Host foreground timeout yalnız SPI_GET ile okundu: 2147483647 → 2147483647. Pozitiflerde last-error değerleri [0]; fixture sentinel 0x1234ABCD ile sıfır olmayan değer ayrıca sınandı. Yalnız host ölçümlerinin eşitliği API'nin çalışmadığının kanıtı değildir; exact CALL/return breakpoint, bağlam ve canary kanıtıyla birlikte değerlendirilir.

Pozitiflerde 59–64 debug event; lifetime thread sayıları [4]. Mevcut 128 event/16 thread/5 saniye kotaları artırılmadı. API, sonraki instance CALL'u, pencere/renderer ve doğal frame çalıştırılmadı.

| Yerel Windows akışı | Native suite | Managed | Python | Log (`out/verification/engine/`) |
|---|---:|---:|---:|---|
| x86 Debug | 25 | 79 | 196 | `build-suppression-x86-Debug-complete.log` |
| x86 Release | 25 | 79 | 196 | `build-suppression-x86-Release-complete.log` |
| x64 Debug | 11 | 79 | 142 | `build-suppression-x64-Debug-complete.log` |
| x64 Release | 11 | 79 | 142 | `build-suppression-x64-Release-complete.log` |

44 portable transaction/spec kontrolü, altı strict policy ve iki CLI testi yeni kapsamdadır. x86 corpus 20 senaryo, gözlemsiz canary kontrolü ve 12 warm çevrim içerir. Model port read/write/rollback hataları gerçek Win32 fault injection sayılmaz. Yeni native corpus yanlış last-error IAT/export, bozuk frame/argüman/target, exception, kota ve owner retlerini kapsar. Linux/hosted CI/N1 SDK yeniden çalıştırılmadı.

Final Debug handle 134 → 134, Release 140 → 140; `ctest-suppression-x86-{Debug,Release}-complete.log`. İlk başarısız deneyler final kanıt yerine kullanılmaz.

| Artifact | SHA-256 |
|---|---|
| Suppression policy | `222da1c0f85ee4747b2f0010d3eadda50c85a560724201d5fab345368fa8acda` |
| Debug observer EXE | `d839fe942278e1a08cbde3cb4139ee9385e83744fcde905c3a9a7610a6dc709a` |
| Release observer EXE | `d307d6ac66ff2397a31cdbaca32ef171147db6cbcc28539190abd97f2ef5c93b` |

SAEX DLL/map kimlikleri önceki 0.1.22 ile aynı; tüm girdi ve artifact kimlikleri canonical matriste bulunur. GetLastError: pinned x86 kernel32.dll 683312 byte, SHA-256 `4ef63ecfe99158e16a387028be295308b00618cec64d94f016f99b79ffe84e05`, RVA 0x23440; API uygulaması çağrılmadan dosya/runtimedaki exact önek karşılaştırıldı.

## Kod 0.1.24 bağlantısı

Ayrı instance-startup modu platform bastırma dönüşünden gerçek named event oluşturma/açma ve doğal helper dönüşüne ilerler. Mevcut event veya NULL handle durumunda pencere kolundan önce ret verilir. Önceki suppression modu restore ederek bitmeye devam eder; yeni mod doğal API sonrası eski CALL bağlamını geri yazmaz. [Sözleşme ve doğrulama](d1-instance-startup.md). Oturumdaki ortak event ömrü process-private değildir; observer sinyal durumunu değiştirmez. C ABI 1/GNS/otorite aynı; pencere/renderer/doğal frame ve N2/N3/D1/D2 kapıları açıktır.

## 14 Eylül 2026 — Windows fixture taşınabilirliği

Startup-return ve devamındaki fixture zinciri, sistem DLL reçetelerini test makinesinin diskte tutulan PE dosyalarından çıkarır. Ortak okuyucu ve negatif doğrulamalar [test kapsamı notunda](d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği) açıklanır; üretim policy/hash kuralları ve bu belgedeki gerçek GTA kanıtının sınırları aynıdır.
