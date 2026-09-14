# D1-N2 ASI yolundan SAEX modülü yükleme

Tarih: 13 Eylül 2026. Mimari v0.23 / kod 0.1.16. [Durum](status.md) · [Önceki codec bağları](d1-codec-bindings.md) · [Bootstrap C ABI](d1-bootstrap-module.md) · [ADR-51](../decisions/architecture-decisions.md)

## Sorumluluk ve kapsam

`run_to_asi_return` ve `--observe-asi-return <absolute-exe> <absolute-cwd>`, incelenmiş wrapper'ın ASI taramasından **tek bir denetlenmiş SAEX DLL'sinin LoadLibraryA dönüşüne** ilerleyen ayrı geliştirme deneyidir. Bootstrap DLL'sinin bitleri değişmeden, yalnız özel deney klasöründe `saex_bootstrap.asi` adıyla kopyası kullanılır. Orijinal oyun dosyası ve kurulumu değiştirilmez; GTA/vendor binary'leri kaynak deposuna veya dağıtıma girmez.

DLL yüklenmesi ile açık platform başlangıcı farklıdır. Boş SAEX DllMain ve CRT başlangıcı yürüyebilir; **Initialize/Query/Stop çağrılmaz**, `bootstrapExportsCalled=false`, `initializationVerified=false`, `canAttach=false` kalır. Bu kesit native hook, frame, worker IPC, sandbox, oyun başlangıcı veya multiplayer sağlamaz. C ABI 1 ve 192 byte status değişmedi. C++ trace/API kullananlar yeniden derlenir; kaldırılan özellik veya kalıcı veri migration'ı yoktur.

## İncelenmiş motor yolu

Adresler yalnız SHA-256 `bdcf32fc3961eebffb4104327ca1396daf1cbd5e736930ed247836035148dafc` wrapper'ının yerel disassembly/PE incelemesinden gelir. Public [ASI-Loader kaynağı](https://github.com/GTAmodding/ASI-Loader/blob/main/vorbisFile.cpp) davranış referansıdır; exact yerel binary'nin build eşitliği varsayılmaz. Kaynakta dizin toplama, ada göre sıralama ve LoadLibraryA vardır; SAEX export'unu çağıran yol yoktur. [Microsoft loader kuralları](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices) gereği açık Initialize, DllMain'e taşınmayacaktır.

| Durak | Wrapper RVA | Yürütme sınırı |
|---|---|---|
| Önkoşul | 0x1546 | Önceki sekiz codec bağı doğrulanır |
| ASI çağrısı | 0x1A44 | `FF 15 [base+0x4020]` öncesi; stack'te tam dosya yolu |
| ASI dönüşü | 0x1A4A | LoadLibraryA sonrası, `INC ESI` öncesi; EAX modül handle |
| Boş tarama koruması | 0x1A5B | Plugin döngüsü çıkışı; cleanup ve sonraki bütün-image VirtualProtect öncesi |

`contracts/engine/asi-policy.json`, önceki binding policy digest'ine bağlıdır. Call RVA, 16 byte return/end örnekleri, sabit artifact adı ve exact Windows x86 bağımlılıkları strict generator ile derlenir. Sekiz JSON kaynak dosyası ayrı ayrı 64 KiB sınırındadır; parent drift, fazla/eksik anahtar, yinelenen JSON anahtarı, belirsiz stage, boolean sayı, geçersiz RVA/byte/hash veya genişletilmiş modül listesi reddedilir. Yeni bağımlılık otomatik keşifle izin almaz.

## Artifact kimliği ve üretim akışı

CMake önce mevcut `saex_bootstrap.dll` ve linker map'i üretir. `tools/bootstrap_load_artifact.py`, mevcut artifact audit'ini çalıştırır ve build klasöründe configuration'a özel `bootstrap_artifact.generated.hpp` üretir. Loader probe bu header'a bağımlıdır: DLL'nin boyutu/SHA-256'sı ve map digest'i probe'a bağlanır. Audit başarısızsa pin üretimi ve probe build'i başarısız olur. Bu header bir publisher imzası veya bütün transitif CRT için güvenlik kanıtı değildir; yerel source/build güveni varsayılır. Yeni build ile eski özel ASI kopyası uyuşmazsa dosya tekrar doğrulanıp açıkça yenilenmelidir; eski artifact sessiz kabul edilmez.

Önceki 26 pin yeni modda korunur. Release için `bcrypt.dll` ve o build'in ASI kopyası eklenir: **28 pin**. Debug ayrıca `msvcp140d.dll`, `vcruntime140d.dll`, `ucrtbased.dll` ekler: **31 pin**. Runtime dosyaları System32'nin x86 çözümünden alınır; Debug runtime dağıtımı tasarlanmamıştır. Eski CLI modları bu pinleri istemez. Her dosya child öncesinde aynı read/share-read handle ile boyut/hash/file ID kontrolünden geçer; pinler child çıkışı doğrulanana kadar tutulur.

PreparedLoaderPolicy varsayılanı yalnız `.dll` isimlerini kabul etmeye devam eder. Yeni explicit `allow_bootstrap_asi` yalnız game-root kökenindeki exact lowercase `saex_bootstrap.asi` adına izin verir; farklı ad, path traversal, büyük harfli recipe veya system-x86 kökeni reddedilir. Genel ASI allowlist değildir. Map SHA hesaplanırken CRLF dahil dosyanın ham byte'ları korunur; CMake Python yürütülebilir yolu PowerShell tarafından absolute olarak çözülür.

## Veri akışı ve arayüz

1. Exact executable profili ve explicit environment/cwd hazırlanır. ASI modunda cwd ile executable dizini aynı olmalıdır. Sabit `saex_bootstrap.asi` yolu ASCII, absolute drive path ve 260 byte'ın altındadır. Bu kısıt legacy wrapper'ın ANSI/MAX_PATH yoludur; platformun genel Unicode asset sözleşmesini daraltmaz.
2. Bütün compiled pinler hazırlanır. Unknown executable, eksik veya yanlış artifact/dependency child oluşturulmadan reddedilir.
3. Önceki entry/proxy/startup/codec/binding duraklarının denetimleri değişmeden geçilir. ASI modülü daha önce active ise ret.
4. DR0 ASI çağrısına, DR2 döngü çıkışına kurulur; DR1 main-thread IAT write watch korunur. Bu aralıkta yeni DLL yüklemesi, pinli olsa bile ret verir. Normal legacy dosya/dizin okumaları ilerler; dosya keşfi/INI içeriği güvenlik sandbox'ı sayılmaz.
5. Altıncı durakta exact call operand, LoadLibraryA hedefinin önceki aşamayla eşitliği, return/end byte'ları, codec mapping'leri ve sekiz bağ tekrar denetlenir. `[ESP]` pointer'ı ve istenen uzunluk+NUL kadarlık path, committed non-guard MEM_PRIVATE bölgesinden bounded okunur; tam yol ASCII case-insensitive eşleşmelidir. Child'ın verdiği yoldan gözlemci dosya açmaz ve program belleğine yazmaz.
6. DR0 dönüşe taşınır; DR2/DR1 korunur. Yeni LOAD_DLL olayları aynı handle/file ID/hash pin kapısından geçmeden initializer'lara devam edilmez. Root veya codec/proxy mapping emekli olursa ret.
7. Yedinci durakta EAX sıfır olmamalı; root, çağrı izninden sonra açılmış tekil active mapping olmalı, EAX onun base'i olmalıdır. ESP önceki değere göre +4 olmalı (stdcall tek argüman); önceki code/codec bağları tekrar eşleşmelidir. Başarı `asi_return_verified` olur; child pending hit devam ettirilmeden durdurulur ve confirmed exit beklenir.

DR7 ilgili bit maskesi ASI safhasında `0x00D00015` olur. Main-thread, first-chance SINGLE_STEP, EIP/ExceptionAddress/DR0 veya DR2 ve DR6 kimliği birlikte denetlenir. DR2'ye çağrı olmadan ulaşmak `asi_candidate_missing`; çağrı izninden sonra beklenen dönüşü atlamak `asi_return_skipped` ret verir. Bu gözlemci kendi child'ıyla sınırlıdır; keyfî PID attach API'si ve remote thread/code injection yoktur. Native güvenilir kodun debug register'larını kötü niyetle değiştirmesini önleyen sandbox iddiası yapılmaz.

## Hata davranışı ve çıktı

`asiObservation` eski CLI'larda null'dır. Yeni modda execution policy/source digest, artifact/map SHA, call/return arm-progress-hit, pathVerified, verified, call/return adresleri, moduleHandle/mappingId, armEvent ve stackBefore/After alanları vardır. Mode seçimi `asiExecutionAllowed=true` üretir; bu alan yürütmenin gerçekleştiğinin kanıtı değildir. Önceki `bindingObservation` beşinci durak kanıtını korur; sonraki tekrar okumalar onu değiştirmez.

| Hata | Sonuç |
|---|---|
| Unknown engine, eksik/bozuk artifact veya CRT | Child öncesi mevcut profil/policy ret kodu |
| Başka tam yol, erken NUL, uzun/eksik NUL, bozuk pointer | `asi_path_mismatch` veya `asi_path_unreadable`, LoadLibrary çağrılmadan ret |
| ASI bulunmaması/ayarlarla atlanması | DR2 sınırında `asi_candidate_missing` |
| NULL handle / farklı veya olmayan active root | `asi_load_failed`, `asi_handle_mismatch`, `asi_mapping_missing` |
| DllMain FALSE ve unload | Mapping emekliliğinde terminal ret; NULL dönüş beklemek zorunlu değil |
| Fault, stall, IAT/code drift, unpinned dependency | Mevcut exception/deadline/pin kapısı veya `asi_return_shape` |
| Kota/owner/state hatası | Mevcut terminal ve owned cleanup kuralları |

128 debug event, 64 lifetime mapping, 16 thread, 256 MiB toplam okuma ve 5 saniye gözlem üst sınırları korunur; unload bütçe iadesi yapmaz. Çıkış kodu 3 yalnız `asi_return_verified` ve confirmed exit birlikteyse verilir. Ret 1, hatalı CLI kullanımı 2'dir. Cleanup başarısızlığını `loader_exit_unconfirmed` örter; başarı gibi sunulmaz.

## Kabul ve doğrulama

Own fixture, SAEX production DLL yerine ayrı canary ASI üretir. Normal load, ASI içinde aynı değeri tekrar yazan IAT watch, yanlış/unterminated path, okunamayan pointer, boş scan, tarama/initializer fault ve stall, FALSE initializer, return drift; invalid spec/path, preloaded root, yanlış kimlik, legacy stop, owner recovery ve budget retleri sınanır. Observer sonrasında ASI export/after-return/end canary'leri çalışmamalıdır. Gözlemcisiz pozitif kontrol, durak sonrası canary'nin gerçekten çalışabildiğini gösterir. On iki warm çevrim exact process handle eşitliği ister.

Policy testleri, artifact audit geçişinin atlanamaması, DLL overlay değişiminin yeni pin üretmesi ve CLI unknown/context retleri standart build'e bağlıdır. Bu corpus gerçek GTA testi yerine yazılmaz. AC-90/R-01b yükleme alt kümesidir; R-01a initialized symbol/ABI ve R-01c frame/hook/drain açık kalır. Güncel çalıştırma sonuçları aşağıda kaydedilir.

İleride açık C ABI invoker, LoadLibrary döndükten sonra Initialize/Query/Stop çağrılarını ayrıca kanıtlayacaktır. Wrapper yalnız LoadLibrary çağırdığı için bu üç export kendiliğinden çalışmış kabul edilmez. Daha geniş Unicode loader, başka executable/build, daha fazla ASI, production launcher veya C# sandbox ayrı capability/ADR ve negatif test ister.

## Bu sürümün çalıştırma kaydı

İlk native corpus ve eski loader suite geçti. İlk gerçek Debug denemesinde `asi_return_verified`, exact SAEX modülü ve dört ek Windows/Debug runtime mapping'i doğrulandı; 57 event sonunda owned child çıkışı onaylandı. Bu tek ön koşu, final matris yerine sayılmaz.

İlk geliştirme hataları saklandı: elle yazılmış return örneği 17 byte idi, pinned PE'den exact 16 byte ile düzeltildi; /WX yerel ad gölgeleme ve wchar→char örtük dönüşümünü yakaladı. Açık ASCII kontrolü/dönüşümü eklendi. Başarısız derleme sonrası ilk CTest artifact olmadığı için **Not Run** oldu; başarı sayılmaz. Sonraki normal native koşu, CMake'in `/` ayıracını üretmesi nedeniyle invalid_path verdi; fixture yolu Windows `\\` biçimine çevrildi, runtime ret gevşetilmedi. İlk gerçek preflight eski DLL-only isim kapısında `loader_policy_spec` verdi ve child oluşturmadı; yukarıdaki dar opt-in eklendi. İlk pozitif koşunun map digest'i text newline normalizasyonunu yansıtıyordu; üretici ham map byte'larına geçirildi, son matris yeni probe ile çalıştırılır. DLL hash ve o ilk koşunun module identity kanıtı bundan etkilenmedi.

İlk build/test kayıtları `out/verification/engine/build-asi-first.log`–`build-asi-fifth.log` ve `test-asi-first.log`–`test-asi-fourth.log`; ilk gerçek ret `asi-Debug-first.json`, ilk yükleme `asi-Debug-first-load.json` içinde korunur. Nihai standart build ve gerçek matris sonuçları aşağıda tamamlanır.

## Nihai yerel doğrulama — 13 Eylül 2026

| Standart tools/build.ps1 profili | Native suite | Managed/entegrasyon | Python |
|---|---:|---:|---:|
| Windows x86 Debug | 14/14 | 79/79 | 112/112 |
| Windows x86 Release | 14/14 | 79/79 | 112/112 |
| Windows x64 Debug | 6/6 | 79/79 | 78/78 |
| Windows x64 Release | 6/6 | 79/79 | 78/78 |

Dört standart akış geçti. X86'daki yeni ASI corpus **27 senaryo + pozitif kontrol + 12 warm çevrim** içerir. Aynı-değere IAT yazımı yeni DR2 ekliyken `startup_iat_written` ile yakalanır. Warm handle eşitliği Debug 138→138, Release 142→142; bütün process kaynaklarının sıfır sızıntısı garantisi değildir. Mevcut bootstrap audit, oyun dışı 100 DLL lifecycle çevrimi ve tüm eski observer suite'leri geçti. Altı ASI policy, üç artifact-binding ve iki CLI test tanımı eklendi; eski loader suite içinde dar ASI isim/origin izinleri ayrıca sınandı. Loglar `out/verification/engine/build-asi-<architecture>-<configuration>.log` ve `ctest-asi-x86-<configuration>.log`.

`out/verification/engine/asi-bootstrap-gta-evidence.json`: **6 Debug + 6 Release = 12/12 asi_return_verified / exit 3**. Her koşuda aynı build'in SAEX DLL bitleri `saex_bootstrap.asi` adıyla gerçek GTA process'ine yüklendi; exact full-path, fresh active file ID/hash, EAX/base, ESP+4 ve önceki sekiz codec bağı eşleşti. Call base+0x1A44, return base+0x1A4A; bekleyen return hit devam ettirilmedi. Bootstrap export'u çağrılmadı; DLL lifecycle'ın LoadLibrary dönmüş alt kümesi kanıtlandı.

Debug'da dört koşu 50, iki koşu 52 event; sırasıyla 4/5 unload, tümünde 30 active DLL. Release'te dört koşu 45, bir koşu 47, bir koşu 48 event; 3/4 unload, beş koşu 27 ve bir koşu 28 active DLL. Bu lifetime/active sayıları compiled pin kapasitesiyle aynı kavram değildir; ASLR/mapping ID ve loader event sayısı ürün sabiti değildir. Ortam snapshot digest'i tüm pozitiflerde `3725a091ed77f875b48ab336a8162328429aeebc82f2c4779f897ed54f62407e`; ham ortam değerleri raporlanmadı.

On bir regresyon: altı eski komut kendi sınırında exit 3 ve `asiObservation=null`; eksik ASI, yanlış cwd ve diğer configuration'ın geçerli fakat farklı ASI artifact'ı child öncesi ret; original kurulumdaki eski binding komutu AcLayers mapping'inde ret; ayrı private klasörde `scripts/global.ini` ile `loadplugins=0` sınaması DR2'de `asi_candidate_missing`. Son durumda SAEX mapping oluşmadı ve wrapper'ın cleanup/whole-image VirtualProtect devamına geçilmedi. Toplam **23/23 beklenen sonuç; oluşturulan 20/20 child confirmed exit**, üç child-öncesi ret vardır.

Orijinal 13 native girdi + tarihsel üç dosya + önceki codec klasöründeki altı dosya + iki configuration için yedişer private dosya + kapalı-ASI klasöründeki yedi native dosya ve bir INI: **44 girdi hash'i** önce/sonra eşit; final rapor üretilirken diskten tekrar doğrulandı. Yeni deney klasörleri `out/experiments/gta-asi-f01a00ce-Debug-v1`, `gta-asi-f01a00ce-Release-v1`, `gta-asi-disabled-f01a00ce-v1`. Bunlar tam oyun asset'leri içeren playable kurulumlar değildir; dışa yayımlanmaz. İlk ret/ön yükleme koşuları 23'lü matristen ayrı tutulur.

| Configuration / artifact | SHA-256 |
|---|---|
| Debug loader probe | `893f3ad761d72ab58a43996acfe9bc5ab2441451d118686e117fdd3f46255561` |
| Debug saex_bootstrap.dll / .asi kopyası | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug bootstrap linker map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release loader probe | `1946095cd173cbe7fdaa1cc56403514ebada387caf3a2ed5040bb54956151846` |
| Release saex_bootstrap.dll / .asi kopyası | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release bootstrap linker map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

ASI policy source SHA-256: `42947538ada61d7a163ff3a085c17b5218a8d702a7fe44445cfd2a2be154b1e8`. Probe içine derlenen DLL/map hash'leri final matris ve diskteki artifact'larla eşittir. Generated header yalnız build klasöründedir; remote hash/manifest kabul etmez. Yeni source yayını, Linux/hosted CI ve N1 opt-in SDK bu kesitte yürütülmedi.

**Sonraki kesit:** güvenilir açık invoker ile GTA içindeki Initialize → Query → Stop çağrılarını loader lock dışında, terminal/retry/cleanup davranışlarıyla doğrulamak. Bu wrapper export çağırmadığı için sadece ASI taramasını sürdürmek bu işi tamamlamaz. DllMain'e IO/başlatma eklemek seçilmedi. Çağırma yöntemi, faz/stack/thread sahipliği ve stop sınırı ayrıca ADR/kabul kaydı ister. Mevcut canAttach/initializationVerified false kalır; N2 bütünü, N3–N7, sandbox/GNS ve D2 açık kalır.

Son belge kontrolü: **94 Markdown, 1353 yerel bağlantı, 6 JSON örneği, sıfır hata**. `--base HEAD` kaynak/belge eşlemesi ve `git diff --check` geçti; bu statik denetim bütün mimarinin anlamsal doğruluğu veya N2 tamamlanması değildir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Yeni frame modu ASI return doğrulamasından hemen sonra ek EXE bayt örneği alır. Bu örnek başarısızsa ASI load kanıtı korunur, bootstrap export çağrıları başlamaz. Eski ASI CLI bu örneği almaz. [Aday ve doğrulama raporu](d1-frame-target.md).

## Kod 0.1.19 bağlantısı

Yeni mod ASI dönüşünden sonra DR2yi aynı ASI callsiteına taşır; ikinci çağrı öncesinde ret verir. VirtualProtect giriş/dönüşü ve doğal startup dönüşü ayrı DR0 duraklarıdır. Eski ASI-return komutu aynı yerde durur. [Sözleşme ve kanıt](d1-startup-return.md).

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.

## Kod 0.1.32 — Cwd query bağlantısı

Yeni query observer önceki terminal komutlarını korur ve ayrı çağrı/dönüş kapısı ekler. Paylaşılan loader/fixture test girişleri bu kapsam için güncellendi; bu belgedeki eski sonuçlar kendi artifact kapsamındadır. Windows OS dosya farkı, GTA yürütmesini güvenli reddeder. Copy/unlock/SEH removal, doğal frame ve D1/D2 kapıları açık kalır. [Sözleşme, kaynak ve güncel kanıt](d1-cwd-query.md).

## 0.1.33 — Windows profil bağı

[26200.9445 incelemesi](d1-system-profile-9445.md), bu bileşenin bağlı olduğu OS pinleri/API reçeteleri ve parent SHA-256 zincirini günceller. Bileşenin GTA durak/izin sınırı aynı kalır; önceki runtime sonuçları eski profil bağlamındadır. Yeni dosyalarla fixture ve gerçek GTA kanıtının kapsamı profil raporunda ayrıca izlenir. Eski OS pinlerine otomatik fallback yoktur; kaynak ve generated policy birlikte yeniden derlenir.

## 0.1.34 — Doğal cwd copy bağı

[Ayrı copy kesiti](d1-cwd-copy.md), query'den sonra native kontrol/CALL/dönüş ve gerçek hedef içerik kanıtını ekler. Önceki komutların terminali korunur; helper/unlock/SEH sökümü yeni izin kapsamına girmez. Source/guard, caller/SEH/kilit/cookie denetimleri ve yeni test/GTA kanıtının kapsamı ilgili rapordadır. C ABI 1, OS modül pinleri, GNS/otorite ve production kapıları aynı kalır; C++ observer yeniden derlenir.

## 0.1.35 — Cwd helper dönüş bağı

[Ayrı return kesiti](d1-cwd-return.md) copy sonrasındaki iki POP, cookie checker eşitliği ve doğal LEAVE/RET'i açar. Önceki terminal izinleri korunur; wrapper/unlock/SEH işlemleri henüz açılmaz. Kaynak stack ömrü, CALL ile değişen saved slot, hedef/caller/kilit/SEH ve hata retleri sözleşmede açıklanır. C++ trace yeniden derlenir; C ABI 1, OS pinleri, GNS/otorite ve production kapıları aynı kalır. Yeni fixture/gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## 0.1.36 — Dosya yöneticisinin tamamlanması

Bu belgenin mevcut alt komut sınırı korunur. Yeni üst düzey `--observe-file-manager-ready` aynı önceki zinciri geçtikten sonra normal unlock(7), SEH epilogue, yol suffix ve CFileMgr dönüşünü birlikte doğrular. Eski checkpoint kanıtı final kilit/FS durumuyla karıştırılmaz; final sonuç ayrı ready kaydındadır. Renderer/doğal frame ve N2/N3/D1/D2 bu kesitte hazır sayılmaz. [Sözleşme, kullanıcı komutu ve doğrulama](d1-file-manager-ready.md).

## Kod 0.1.37 — Streaming tablo kesitiyle bağlantı

[CdStream tablo sözleşmesi](d1-cd-stream-tables.md) ortak observer/CLI ve fixture zincirine ayrı bir üst mod ekler. Bu belgenin eski komut ve checkpoint sınırı korunur; yalnız `--observe-cd-stream-tables` tam manager dönüşünden sonra iki tablo döngüsünü ve disk argüman hazırlığını açar. Sonuç yeni `cdStreamTablesObservation` alanında izlenir; eski kayıtlar final durum değil önceki checkpoint snapshot'ıdır. C++ trace tüketicileri yeniden derlenir; C ABI 1, GNS, OS pinleri ve production IPC sınırı değişmez. Yeni portable/native testler ile eski mod regresyonları standart build'e dahildir; gerçek GTA ve platform bazındaki final kanıt ana raporda tutulur.

## Kod 0.1.38 — Disk sonucu ve allocation önkoşulu

[Disk hazırlığı sözleşmesi](d1-cd-stream-disk.md) önceki native zincire ayrı `--observe-cd-stream-disk` modu ekler. BOOL başarısızsa dört output kullanılmadan ret; başarılı ve kabul edilen mantıksal geometride doğal bayrak/argüman hazırlığı, 0x406BF4 allocation CALL önünde doğrulanır. Eski modların terminal ve snapshot anlamı korunur. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS, network otoritesi ve sandbox kapsamı değişmez. Gerçek allocation, fiziksel hizalama, dosya okuma ve thread/renderer hazır kanıtı bu değişiklikten çıkarılamaz. Portable hata kararı ile native/gerçek GTA kanıtının ayrımı yeni raporun kabul tablosunda izlenir.

## Kod 0.1.39 — Hizalı tamponun doğal dönüşü

[Allocation sözleşmesi](d1-cd-stream-allocation.md) ayrı `--observe-cd-stream-allocation` API/CLI ile MallocAlign → CRT → HeapAlloc → back-pointer → 0x406BF9 doğal dönüşünü ekler. Heap modu/new-handler/SBH dalı yürütmeden önce denetlenir; NULL, taşma, metadata ve payload bütünlüğü guard'ları vardır. Eski alt modların terminalleri ve snapshot anlamı korunur; yeni mod 160, eskiler 128 olay üst sınırındadır. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS ve sandbox kapsamı değişmez. İlk gerçek GTA allocation geçti; güncel toplu kanıt yeni sözleşmede izlenir. Native free, I/O/thread, renderer ve D1/D2 hazır kabul edilmez.

## Kod 0.1.40 — Kanal belleği kesiti

[Yeni sözleşme](d1-cd-stream-channels.md) `run_cd_stream_channels` / `--observe-cd-stream-channels` ile SetLastError ve LocalAlloc doğal yolunu, 5 × 48 sıfır byte ve global pointer kaydını ekler. Terminal 0x406C34, arşiv CALL önüdür. Önceki allocation/parent kayıtları kendi duraklarının snapshot anlamını korur; canlı tabloda yalnız kanal sayısı/etkin sayı DWORD çifti değişebilir. C++ trace tüketicileri yeniden derlenir; C ABI 1 ve mevcut OS pinleri aynıdır. Allocation ve yeni mod 160, daha eski modlar 128 olay sınırındadır. Native free, dosya açma/okuma, thread, renderer ve D1/D2 kapıları açıktır. Güncel test ve GTA kanıtı yeni sözleşmede tutulur.
