# D1-N2 — Askıya alınmış gerçek süreç görüntüsü

Mimari v0.11 / kod 0.1.4, 13 Eylül 2026. [Durum](status.md) · [N2 planı](d1-engine-integration.md) · [Önceki dosya/image gözlemi](d1-engine-preflight.md) · [Başlangıç DLL'si](d1-bootstrap-module.md).

## Sorumluluk ve doğrulama sınırı

Yeni Windows geliştirme aracı, doğrulanmış yerel executable için **kendi oluşturduğu sürecin ilk create-debug olayındaki image** başlıklarını ve dört kısa anchor'ı okur. Ana thread askıda kalır; oyun kodu yürütülmeden süreç sonlandırılır. `scope=created-suspended-process-observation`, `canAttach=false`. Bu, `SEC_IMAGE_NO_EXECUTE` görünümünden ayrı bir gerçek process gözlemidir; unpack edilmiş/başlatılmış GTA, function ABI, DLL yükleme, frame veya hook kanıtı değildir. N2/R-01a/b ve N3 açık kalır.

Araç güvenilir yerel geliştirici kullanımına aittir. Çalışan GTA'ya bağlanma, kullanıcı PID'si alma, mod indirme, injection veya genel process yönetim API'si sunmaz. [Observer kaynağı](../../src/engine/observer/suspended_image.cpp), ayrı statik hedefte derlenir; `saex_bootstrap.dll` ve Plugin-SDK'ye eklenmez. GNS/HTTPS, C++20 core, private vendor C++23 ve bootstrap C ABI değişmez.

## Veri akışı ve arayüzler

1. [CLI](../../src/engine/observer/process_probe.cpp) yalnız `--observe-suspended <gta_sa.exe>` biçimini kabul eder. [WindowsFileObservation](../../src/engine/bootstrap/windows_file_observation.cpp), açık kalan salt okunur `FILE_SHARE_READ` handle üzerinden exact SHA-256/PE/layout/dört anchor profilini doğrular. Bilinmeyen dosya için child oluşturulmaz.
2. [SuspendedImage](../../include/saex/engine/suspended_image.hpp), explicit tam application path ve tırnaklı mutable command line ile `DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW` kullanır. Duraklatma ilk debug olayının Windows tarafından bekletilmesidir; `CREATE_SUSPENDED` eklenmez. Handle inheritance kapalıdır. Tek sahipli job, `KILL_ON_JOB_CLOSE` taşır; çocuk bu job'a atanır. Job kurulamazsa gözlem durur.
3. En fazla 5 saniyede ilk `CREATE_PROCESS_DEBUG_EVENT` alınır. Olayın PID'si oluşturulan child kimliğiyle aynı olmalıdır. Image base ve dosya handle'ı bu OS olayından gelir. `FILE_ID_INFO` volume/file ID, hâlâ açık verified file ile karşılaştırılır; aynı path metni yeterli değildir.
4. Preferred image base eşitliği aranır. `VirtualQueryEx` ve `ReadProcessMemory` ile yalnız bu owned image aralığı okunur: `MEM_COMMIT`, `MEM_IMAGE`, doğru allocation base, okunabilir protection, guard/no-access ve overflow reddi. Her copy en fazla 1024 byte, image en fazla 256 MiB. Başlıklar parça parça ve anchor'lar kendi RVA'larında mevcut profil kapısıyla karşılaştırılır.
5. İlk debug olayı gözlem bitene kadar bekletilir. `ResumeThread`, thread context değişimi, memory write, breakpoint veya DLL yükleme yoktur. Önce owned process'e `TerminateProcess` (başarısızsa owned job'a `TerminateJobObject`), **sonra** debug olayını continue yapılır. Debug duraklatması canlı oyun kodunu çalıştırmak için kaldırılmaz.
6. En fazla 5 saniye/64 debug olayı içinde exit olayı tüketilir ve sahip olunan process handle'ının signaled olması doğrulanır. Dosya, process/thread ve job handle'ları bırakılır. Debug olayının process/thread handle'larını Windows kapatır; event file handle'larını observer kapatır. Gözlem sonucu yalnız çıkış doğrulanmışsa başarılıdır.

Windows `CREATE_PROCESS_DEBUG_EVENT` olayını kullanıcı modu yürütülmeden üretir. Oluşturan debugger thread'i olay tüketimini ve continue çağrılarını yürütür. CreateProcess'in dönmesi oyun başlatmasının tamamlandığı anlamına gelmez. [Debug olayları](https://learn.microsoft.com/en-us/windows/win32/debug/debugging-events), [CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw), [ContinueDebugEvent](https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-continuedebugevent).

Sınıf güvenilir internal test seam'idir; kendi başına GTA fingerprint kapısı değildir. Üretim girişinde CLI'nin dosya kapısı zorunludur. Construct/read/stop/destroy aynı thread'de ve başka debug session yönetmeyen ayrı observer process'inde yapılır. Eşzamanlı çağrı ve process attach desteklenmez. Job son handle kapanışında owned process'i sonlandırma desteği sağlar; hiçbir ret başka PID'yi öldürme yetkisi vermez. [Job sınırları](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_extended_limit_information), [VirtualQueryEx](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualqueryex).

## Hata davranışı ve genişleme

Exit **3**, yalnız profil/dosya kimliği/image eşleşmesi ve child exit doğrulaması başarılı gözlemdir; runtime desteği kapalıdır. Exit **1**, dosya/OS/kimlik/image/cleanup reddidir; exit **2**, açık opt-in veya argüman hatasıdır. Exit 0 yolu yoktur. JSON; scope, observedProfileId, dosya SHA-256, profil digest, observer pointer genişliği, child PID/base ve matched/exit/resume alanlarını içerir. Ret çıktısı `childCreated` ve `childExitConfirmed` ile pre-create ret ve cleanup durumunu ayırır; `systemError` yalnız kaydedilen Windows oluşturma hatasını ifade eder.

Timeout, job reddi, farklı dosya/base, okunamayan sayfa veya anchor uyuşmazlığında child çalıştırılmadan cleanup denenir. Stop idempotenttir. Destructor scope/exception çıkışında stop'u tekrar dener; job kapanışı ek owned-child sonlandırma güvencesidir. `observer_exit_unconfirmed` başarıya çevrilmez; bounded cleanup sonucundan mutlak OS/driver arızası altında sıfır kalıntı garantisi çıkarılmaz. Windows API failure injection ve observer'ın zorla öldürülmesi bu kesitte ayrı ölçülmedi. Query/read atomik değildir; bu araç düşmanca process'e karşı anti-cheat değildir.

Sonraki faz, sadece yeni sözleşme ve ölçülen başlatma/ABI kanıtıyla eklenebilir. Mevcut `observationOnly` profilini değiştirmek, bir success bayrağı veya resume seçeneği eklemek N3'ü kendiliğinden açamaz. Bu kesitte kaldırılan özellik veya mevcut CLI/ABI'da geçiş gereksinimi yoktur.

## Kabul ve yeniden üretim

[Native test](../../tests/engine/suspended_image_tests.cpp), derlenen [kendi executable'ımızı](../../tests/engine/suspended_fixture.cpp) kullanır. Normal çalışan pozitif kontrol `.executed` canary dosyası üretir ve exit 73 verir; observer çevrimlerinde bu dosya oluşmamalıdır. Bilinen test çıktısı kontrol sonunda silinir; GTA dosyası kullanılmaz. Boş/eksik/yanlış boyut, farklı file identity, bounded/overflow read, yanlış thread, stop sonrası okuma, idempotent stop, 12 handle-stable lifecycle çevrimi ve exception unwind kapanışı sınanır. Canary yalnız fixture main'ine giriş göstergesidir; kullanıcı modu öncesi faz sınırının dayanağı Windows debug sözleşmesidir.

[CLI testleri](../../tests/engine/test_process_probe.py), aynı marker/yanlış hash, yanlış mimari, truncated image, bozuk başlık ve açık opt-in eksikliğini child oluşturmadan reddeder. [Ortak sentetik PE](../../tests/engine/pe_fixture.py), önceki image CLI testleriyle paylaşılır. Standart [build](workflow.md) yalnız kendi fixture executable'ımızı başlatır; GTA kurulumuna ihtiyaç duymaz.

Gerçek dosya üzerinde ayrı, açık deney:

```powershell
./out/windows-x64/Debug/saex_engine_process_probe.exe --observe-suspended "C:\Program Files (x86)\Rockstar Games\GTA San Andreas\gta_sa.exe"
# Beklenen exit 3; oyun kodu yürütülmez, owned child çıkışı ayrıca doğrulanır.
```

### Bu kesitin ölçüm kaydı

İlk x64 fixture koşusu, `CREATE_SUSPENDED` ayrıca kullanıldığında ilk debug olayının 5 saniyede gelmediğini gösterdi (`ERROR_SEM_TIMEOUT=121`). Child cleanup tamamlandı; canary oluşmadı. Bayrak çıkarıldı; Windows'un ilk debug olayında bütün thread'leri continue'a kadar duraklatan sözleşmesi kullanıldı. Ret/timeout başarı diye kaydedilmedi. [Süreç bayrakları](https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags).

13 Eylül 2026 yerel koşuları (UTC 12 Eylül 23:05 ve 23:07): **x64 Debug, x86 Debug ve x86 Release** standart build akışları başarılı. X64 4, x86 5 CTest suite; her koşuda 26 managed, x64 22/x86 32 Python testi geçti. Yeni Python kapsamı 5 process CLI ret testidir. Native testin her yapılandırmasında 12 lifecycle çevrimi, canary pozitif kontrolü, yanlış kimlik/sınır/thread ve exception cleanup geçti; bu çevrimler bağımsız AC sayısı veya AC-91'in GTA başlatma testi değildir.

| Yapılandırma | Observer test handle başlangıç → bitiş | Gerçek GTA sonucu | Observer artifact SHA-256 |
|---|---|---|---|
| x64 Debug | 121 → 121 | File ID, base, başlık + 4 anchor; exit 3 | `43b241cb83928771a12540c2d0449497736ea5efa3dcb9ae239f25cc43212ab0` |
| x86 Debug | 146 → 146 | File ID, base, başlık + 4 anchor; exit 3 | `0658dd7c79ca64a2c64003d3f70d80fe1472c338d3fab6800c864f39dcd4e979` |
| x86 Release | 147 → 147 | File ID, base, başlık + 4 anchor; exit 3 | `6d282b597ac508b183e7596620982a2c9ba9e9cae8005b7cc3df30ece0bbad1a` |

Kullanıcının yerel GTA dosyası her gözlem öncesi/sonrası aynı SHA-256'yı verdi: `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`. Gözlem profili `gta-sa.observed.f01a00ce950fa40c`, kaynak digest'i `0653426a913c3fdef809ea9e6a614fca7d26498e66cb63148d7073fe8d965436`; ikisi de değişmedi. Üç OS create olayında image base `0x400000`, file identity/header/anchor eşleşmesi true, `primaryThreadResumed=false`, `childExitConfirmed=true`, `canAttach=false`. Çağrı sonrasında gözlenen child PID'leri ayrıca mevcut değildi. Oyun kurulumuna çıktı yazılmadı; GTA DLL'si/SDK/hook yüklenmedi.

Yerel JSON kayıtları `out/verification/engine/process-observation-<architecture>-<configuration>.json`, CTest log kopyaları `process-tests-<architecture>-<configuration>.log` adlarıyla aynı doğrulama dizinindedir. Build/araç sürümleri: Windows 10.0.26200, Windows SDK 10.0.26100.0, MSVC 19.44.35228.0, CMake 4.3.3, .NET SDK 10.0.300. Bu kesitte Linux/x64 Release çalıştırılmadı. Artifact hash'leri bu koşuların kimliğidir; reproducible binary iddiası değildir.

Mevcut bootstrap DLL'nin x86 Debug/Release artifact hash'leri [0.1.3 raporuyla](d1-bootstrap-module.md) aynı kaldı; artifact denetimi ve oyun dışı 100'er warm lifecycle regresyonu geçti. Observer bu DLL'ye bağımlılık eklemedi. N1 kaynak kilidi/source/recipe değişmedi; 23 dosyalı salt okunur verify geçti, SDK yeniden derlenmedi. Statik belge/link/JSON gate'i 71 Markdown, 872 yerel bağlantı ve 6 JSON örneğinde sıfır hata verdi; bu gate anlamsal veya runtime doğruluğu kanıtlamaz. Güncel özet [durum tablosundadır](status.md).

## Kod 0.1.6 ile ortak process sahipliği

SuspendedImage özel durumuna ayrı LoaderObservation friend'i ve tek deney işareti eklendi. Bu rapordaki saex_engine_process_probe davranışı değişmez: ilk create-debug olayı öldürmeden devam ettirilmez. Yeni [loader CLI](d1-loader-observation.md) farklı hedef/scope'tur; açıkça ContinueDebugEvent kullanarak loader'ı ilerletebilir, ilk exception/unpinned mapping'de durur. Ortak stop artık hangi terminal olay tutuluyorsa önce kill sonra cleanup-continue uygular. Eski 0.1.4 binary hash'leri tarihsel kanıttır; değişmiş observer build'ine taşınmaz. Varsayılan observer'ın canary/lifetime regresyonu standart akışta yeniden çalışır.

## Kod 0.1.8 devamı — Opsiyonel LaunchContext

[Context parametresi](d1-launch-context.md), geçersiz environment/directory veya directory identity değişiminde child öncesi ret verir. Varsayılan nullptr eski inheritance yoludur. Açık parametre CREATE_UNICODE_ENVIRONMENT ve seçilmiş lpCurrentDirectory kullanır; ilk-create hold/stop/thread ownership kuralları aynıdır. Yeni context suite'i 12 yaşam çevrimi ve kendi child'ının aldığı gerçek cwd/environment değerini sınar; GTA initialization kanıtı değildir.
