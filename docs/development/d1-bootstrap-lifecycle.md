# D1 — GTA içinde kontrollü bootstrap yaşam döngüsü

Kod 0.1.17 / mimari v0.24; 13 Eylül 2026. Durum: aşağıdaki sınırlı uygulama, dört Windows akışı ve özel GTA matrisi doğrulandı. [ASI önkoşulu](d1-asi-bootstrap-load.md), [C ABI 1](d1-bootstrap-module.md), [uygulama durumu](status.md).

## Sorumluluk ve güven sınırı

`--observe-bootstrap-lifecycle <exe> <absolute-cwd>` ayrı x86 geliştirme iznidir. Yalnız observer'ın oluşturduğu ve job ile sahip olduğu çocukta, önceki yedi durak doğrulanınca çalışır. ASI `LoadLibraryA` dönüşü tutulduğu için çağrı DllMain dışındadır; bu belirli çağrı zincirine ait kanıttır, genel loader-lock dedektörü değildir. CLI keyfî PID, export, RVA, hash veya indirilen modül kabul etmez. Kaynak/derleyici/yerel exact dosyalar güvenilir kabul edilir; OS sandbox sağlanmaz.

## Veri akışı ve arayüz

ASI recipe → aynı build artifact SHA/DLL/map audit → direct executable export RVAs ve 20 byte prefix → own main-thread call frame → C ABI 1 status → terminal child cleanup.

CMake üç export'u aynı audited artifact'ten üretir. Forwarder, çakışan veya executable olmayan target ve prefix sınırını kesen relocation reddedilir. Prefix içindeki tam dört byte HIGHLOW alanlarına gerçek base−preferred base farkı modulo 2^32 eklenir; diğer byte'lar değişmeden birebir karşılaştırılır. Her çağrı öncesi/sonrası aktif ASI mapping ve üç 20 byte prefix, wrapper/codec/IAT önkoşulları denetlenir. Windows crypto provider `bcryptprimitives.dll`, yalnız bu modun SHA/size kaynak pinine eklenir; diğer CLI modlarının kapanışı aynı kalır. Kaynak policy ASI parent SHA'sına bağlıdır; runtime keşfi izin oluşturmaz.

Sıra sabit sekiz çağrıdır: Query → Initialize → Query → Initialize → Stop → Query → Initialize → Stop. İlk Query discovered/attempts=0; ilk Initialize observed_unverified/reason=5 veya açık host ret/attempts=1; sonraki sorgu ve tekrar Initialize birebir aynı; son dört durum stopped ve önceki reason/profile/attempts korunmuş olmalıdır. `can_attach`, `bindings_loaded`, reserved her zaman sıfır; profile source digest 64 byte birebir; profile alanı kalan sıfır byte'larıyla doğrulanır. OK yalnız ABI çağrısının işlendiğini belirtir.

## Çağrı mekanizması

Yeni API sınırlı **stack veri yazımı ve EIP/ESP yönlendirmesi** yapar. Eski read-only gözlem yetkileri bunu kapsamaz. ASI dönüş ESP'sinin 256 byte altından 16'ya hizalanan scratch alanında 16 byte guard + 192 byte status + 16 byte guard kullanılır. Cdecl giriş ESP=bu alan−20 (mod16=12); return/abi/output/size dört kelimedir. Bölge committed PAGE_READWRITE, guard olmayan MEM_PRIVATE ve mevcut stack ile aynı allocation olmalıdır; aralık mevcut ESP altında kalır, exact yazma/readback gerekir. Çağrı dönüşünde ESP=frame+4 ve EBX/ESI/EDI/EBP, TF/DF kontrol edilir. Her çağrı aynı alanı yeniden kullanır.

DR0 mevcut ASI dönüşünde, DR1 main-thread startup IAT yazımında, DR2 wrapper tarama sonunda kalır. Çalıştırılabilir bellek veya oyun native adresi yazılmaz, shellcode/remote thread eklenmez. Sonuçta çocuk her zaman sonlandırılır: bütün FPU/SIMD/caller state restorasyonu ve gameplay'e devam sözleşmesi yoktur. Guard ve debug register kontrolleri düşmanca native kod için koruma kanıtı değildir.

## Hata davranışı ve genişleme

Yanlış spec/prefix, mapping emekliliği, ABI/size/state/digest/tail uyuşmazlığı, çıktı taşması, callee register/stack ihlali, busy/ret code, exception veya mevcut süre/event/thread/byte sınırları terminal rettir. Host ret durumunda kalan Query/Stop kontrolleri güvenilir SAEX code üzerinde tamamlanabilir; `verified=true` yalnız yaşam döngüsü tutarlılığıdır. `bootstrap_lifecycle_host_rejected` exit=1; yalnız observed_unverified ve sekiz doğru dönüş + confirmed exit durumunda `bootstrap_lifecycle_verified` exit=3. Root `initializationVerified=false`, `canAttach=false` korunur.

`bootstrapObservation` eski modlarda null; yeni modda ayrı permission/progress, sekiz çağrı kaydı, ham 192 byte hex ve sonuç alanları bulunur. Geçersiz native metin NUL aranmadan yazdırılmaz. C ABI 1 değişmez; C++ observer caller'ları yeniden derlenir. Kaldırılan özellik yoktur. Yeni private artifact kopyası oluşturulur; eski kanıt klasörlerinin üzerine yazılmaz.

Sonraki genişleme engine phase kanıtı, güvenli frame callback ve kapatmada in-flight drain olacaktır; bu deney hook, üretim launcher, IPC, unload veya D2 kanıtı değildir. GTA profile/header/anchor ret kapıları gevşetilmez.

## Kabul ve kanıt

Own fixture'da doğru sıra, host ret, yanlış ABI/digest/profile tail/attach, tekrar Initialize ve terminal ihlali, guard taşması, busy, fault/stall, cdecl stack/register, prefix/spec, owner ve legacy sınırları test edilir. Aynı gerçek SAEX DLL ayrıca own non-GTA EXE içinde host ret ile sınanır. Gerçek GTA koşuları standart testlerden ayrı tutulur. Sonuçlar aşağıdadır; AC-90/R-01/N2 bütünü açık kalır.

## İlk denemeler ve düzeltmeler

Son eklenen prefix-drift fixture'ının forward declaration'ında `dllexport` eksikti; MSVC C2375 hatası verdi. Bildirim tanımla eşlendi. Başarısız kayıt `out/verification/engine/build-bootstrap-x86-Debug.log` altında korunur; production bootstrap veya GTA dosyası değiştirilmedi.

Ek negatifin ilk koşusu izlenmeyen fixture Query adresini değiştirdiği için beklenen prefix ret yerine status ret üretti. Test, çağrı sırasında hâlâ spec'te bulunan Stop prefix'ini değiştirecek şekilde düzeltildi; production kontrolü gevşetilmedi. Başarısız native kayıt `out/verification/engine/build-bootstrap-x86-Debug-final.log` içinde saklandı.

İlk x86 Debug standart akışı ve ilk özel GTA koşusu geçti: sekiz dönüş, observed_unverified/reason=5, attempts=1, terminal stopped ve confirmed exit. Son incelemede stack yazma girişimi ile yazılan byte kanıtı ayrıldı; çağrı sırasında export prefix değişimine negatif test eklendi. Bu ilk koşu aşağıdaki nihai matristen ayrı tutuldu. İlk kanıt: `out/verification/engine/bootstrap-Debug-first.json`; bu ilk koşu son progress alanı eklenmeden önce alınmıştır.

Release MSVC çıktısında her export'un +13 konumunda HIGHLOW vtable adresi vardır (`0x10004CB0`, tercih edilen base `0x10000000`); eski 16 byte kesit alanı yarım bıraktığı için reddedildi. 20 byte kesit bu alanı tam kapsar ve iki bounded read (16+4) ile okunur. İlk Release build reddi `build-bootstrap-x86-Release-verified.log` içinde korunur; inceleme `out/research/bootstrap-Release-disasm.txt`. [Microsoft PE HIGHLOW tanımı](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#base-relocation-types). Sonradan kod değiştirme veya bilinmeyen hook kabul edilmez. Beş saf normalization kontrolü (pozitif/negatif delta, overlap, taşan fixup, base) ve iki ek audit testi eklendi.

## Nihai doğrulama — 13 Eylül 2026

| Profil | Native suite | Managed/entegrasyon | Python |
|---|---:|---:|---:|
| x86-Debug | 15 | 79 | 126 |
| x86-Release | 15 | 79 | 126 |
| x64-Debug | 6 | 79 | 84 |
| x64-Release | 6 | 79 | 84 |

X86 yaşam döngüsü corpus'u beş saf HIGHLOW kontrolü, 24 senaryo + 12 warm çevrim içerir; sahte modül ve gerçek SAEX DLL/non-GTA host ret yolu birlikte sınandı. PASS bootstrap lifecycle: 24 scenarios and 12 warm cycles; handles 139 -> 139; PASS bootstrap lifecycle: 24 scenarios and 12 warm cycles; handles 143 -> 143.

Gerçek özel GTA: Debug 6/6, Release 6/6; **96/96 C ABI dönüşü** doğrulandı. Yedi eski mod sınırı ve beş hata/regresyonla 24/24 beklenen sonuç; 21/21 oluşturulan çocukta confirmed exit. Eksik artifact, yanlış cwd ve configuration/artifact karışımı child öncesi ret; ASI kapalı tarama DR2 ret; original legacy AcLayers pini dışında yüklemeyi reddeder. Önce/sonra ve son tekrar okumada 59 dosyanın SHA'sı aynı: önceki 44 native/INI girdi, yeni iki özel klasörde 14 dosya ve crypto provider. Orijinal GTA kurulumu değiştirilmedi.

Durum sırası her koşuda `0,2,2,2,3,3,3,3`; attempts `0,1,1,1,1,1,1,1`; Initialize reason=5 (runtime_unverified). Ana image header ve dört anchor eşleşti; hiçbir çağrı binding/hook/attach açmadı. Sekiz dönüş mevcut beş saniyelik toplam gözlem bütçesinde tamamlandı; bu bir frame/FPS/oyuncu kapasitesi benchmark'ı değildir.

Tercih edilen image base dışında yüklenen koşular: Debug 6/6, Release 6/6. Release'te üç export'un +13 HIGHLOW alanı gerçek yükleme adresine göre kontrol edildi. Bu sonuç reboot'lar arası ASLR çeşitliliği testi değildir.

| Configuration | Observer SHA-256 | Bootstrap DLL SHA-256 | Map SHA-256 |
|---|---|---|---|
| Debug | `08c90b6db70bff1dfd47403935baccb636ad6270a2702373db98549a89a94fb7` | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release | `bbc2d9583543ba392f014dcfa88d4b726702cd516886f4f3c5ca4f19bacf18df` | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Lifecycle policy SHA-256: `cd6f7822e4916a1082ec502cc24e45915cefd0e78f5e17eb894655f17443948e`. Yeni mod Release 29/Debug 32 dosya pini kullanır. ASI parent policy ve engine profile kaynak hash'leri değişmedi.

Ham kanıtlar: `out/verification/engine/bootstrap-lifecycle-gta-evidence.json`, `bootstrap-final-summary.json`, `bootstrap-Debug-1.json` … `bootstrap-Release-6.json`, `bootstrap-legacy-*.json`, `build-bootstrap-*-*-complete.log`, `ctest-bootstrap-complete-x86-*.log`. Bireysel prefix/ABI/guard/return çıktı kayıtları LastTest kopyalarında; failed fixture koşuları yukarıda korunur.

Debug ölçümleri: {"eventCount": {"61": 1, "57": 2, "59": 3}, "unloadCount": {"5": 1, "3": 2, "4": 3}, "activeModuleCount": {"31": 6}}.

Release ölçümleri: {"eventCount": {"55": 1, "56": 3, "58": 2}, "unloadCount": {"3": 1, "4": 3, "5": 2}, "activeModuleCount": {"29": 1, "28": 5}}.

Standart Windows kontrolleri yerelde çalıştı. Linux/hosted CI ve opt-in N1 SDK bu değişiklikte yeniden çalıştırılmadı; mevcut SDK kodu/lock değişmedi. Tam unload/drain, engine fazı, native symbol ABI, N3 frame/hook, üretim IPC/sandbox, GNS ve D2 açık. Bir sonraki iş engine fazı ve frame hedefinin exact executable üzerinde kanıtını hazırlamaktır; adres tahmini veya gözlem profilini doğrudan destek profiline yükseltme yapılmaz.

## Kaynaklar

X86 argüman sırası ve caller cleanup: [Microsoft cdecl](https://learn.microsoft.com/en-us/cpp/cpp/cdecl?view=msvc-170). Duraklatılmış thread context işlemleri: [SetThreadContext](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadcontext). DllMain dışına başlatma taşıma gerekçesi: [DLL best practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices). Bu kaynaklar yerel binary çağrı sınırının deneyle doğrulanmasının yerine geçmez.

## Kod 0.1.18 bağlantısı

Yeni run_frame_target_observation aynı sekiz C ABI çağrısını ve terminal child çıkışını koruyarak EXE frame adayını üç noktada okur. Eski run_bootstrap_lifecycle ve CLI sonuçları değişmez. Frame kontrolü bootstrap DLL içine linklenmez; C ABI 1 değişmedi. [Aday ve doğrulama raporu](d1-frame-target.md).

## Kod 0.1.19 bağlantısı

Yeni startup-return modu C ABI lifecycle deneyinin devamı değildir. Aynı ASI yüklemesinden doğal yola ayrı dallanır; sekiz zorlanmış çağrıdan sonra FPU/stack restorasyonu yapılmış iddiası yoktur. Önceki terminal Stop/child cleanup kanıtı korunur. [Sözleşme ve kanıt](d1-startup-return.md).

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.
