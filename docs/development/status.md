# Uygulama durumu ve doğrulama sınırı

Sürüm: mimari v0.38 / kod 0.1.31, D1 foundation + native dependency + N2 preflight, bootstrap, askıda process, statik envanter ve sınırlı loader gözlemi alt kümeleri. Tarih: 13 Eylül 2026. Kullanıcı kodlamayı ve her kaynak değişiminde belgelerin güncellenmesini onayladı. [Ana indeks](../../README.md) · [Çalışma akışı](workflow.md) · [Foundation](d1-foundation.md) · [N1 kanıtı](d1-native-dependency.md) · [Değişiklik kaydı](change-log.md)

## Gerçek durum

### Kod 0.1.31 kaynak eşitlemesi

14 Eylül 2026: Yerelde biriken 0.1.14–0.1.31 kaynakları ve bütün ilgili test/belgeler, 0.1.13 public tabanı üzerine yayın için hazırlanır. [Yayın kapsamı ve doğrulama kaydı](github-publication.md#kod-0131-kaynak-eşitlemesi--14-eylül-2026). Aşağıdaki 0.1.13 hosted sonuçları tarihsel kanıttır; güncel 0.1.31 kontrolleri ayrı kaydedilir. Ürün sürümü, D1/D2 kapıları ve gerçek GTA raporlarının kapsamı bu yayın çalışmasıyla değişmez.

### Public GitHub kaynak yayını

13 Eylül 2026: **SAEX — San Andreas Extended**, [saex-platform](https://github.com/saex-platform) organizasyonunda Owner `Rohatcengizhanbucak` yönetiminde yayımlandı. Public [saex](https://github.com/saex-platform/saex) kaynak monoreposu ve [.github](https://github.com/saex-platform/.github) profil/topluluk deposu; MIT, doküman/marka varlıkları, issue/PR şablonları, güvenlik bildirim kanalı ve [geliştirme panosu](https://github.com/orgs/saex-platform/projects/1) hazırdır. Her iki `main` dalında PR, linear/squash geçmişi, silme/force-push engeli uygulanır; ana depoda yedi GitHub Actions kontrolü zorunludur.

Kaynak commit'i `e5e2dd0d77e37447eca4b7e32c0b89306978c096` için **GitHub'daki yedi kontrol geçti**: Windows x64/x86 Debug/Release, Linux x64 Debug, doküman ve sır taraması. Her build'de 79 managed test; Windows x64 6 native suite/46 Python, x86 9 suite/67 Python, Linux 4 suite/37 Python başarılı. Ayrı kısa temiz checkout N1 SDK Debug/Release'te 1 probe + 17 dependency + 2 configure ret testi geçti; manuel hosted SDK işi çalıştırılmadı. İlk başarısızlıklar, düzeltmeler ve kanıt bağlantıları [yayın raporunda](github-publication.md) korunur.

Bu kaynak yayını D1'i kapatmaz; D2 oynanabilir multiplayer veya native binary release değildir. 0.1.12/0.1.13 proxy ve startup geliştirmesi bu ilk yayın kanıtının kapsamı dışındadır. Yeni kaynaklar yayımlanmış tabanla birleştirildi; bu sürümün GitHub doğrulaması [yayın raporunda](github-publication.md#kod-0113-kaynak-birleştirmesi) ayrıca izlenir.

### Kod 0.1.13 GitHub doğrulaması

[PR #9](https://github.com/saex-platform/saex/pull/9), `bd98c4a` kaynak commit’i: **yedi zorunlu hosted kontrol geçti**. Windows x64 Debug/Release 6 native suite/79 managed/60 Python; x86 Debug/Release 11 suite/79 managed/85 Python; Linux x64 Debug 4 suite/79 managed/51 Python başarılıdır. Ayrı belge kontrolü ve sır taraması geçti. Yerel x86 Debug birleştirme koşusu da geçti. [Koşu bağlantıları ve kapsam](github-publication.md#kod-0113-github-doğrulaması). N1 SDK ve gerçek GTA bu yayın çalışmasında tekrar çalıştırılmadı; aşağıdaki tarihsel runtime kanıtları kendi artifact kapsamındadır.

### Ürün bileşenleri

| Bileşen | Kodlanmış davranış | Doğrulama | Kalan sınır |
|---|---|---|---|
| CRT mevcut kilidi alma | Doğal JNE/Win32 CALL/selector dönüşü; nesne sahipliği, ABI ve caller/SEH korunumu | Dört Windows akışı; 141 portable kontrol/17 senaryo/iki canary/12 warm; GTA 12/12 ve 40/40 matris; [rapor](d1-cwd-acquire.md) | Kilit tutulurken terminal; unlock/cwd/SEH dönüşü açık |
| CRT lock(7) kayıt seçimi | Doğal CALL, tablo adresleme ve CMP; JNE önünde iki durak, slot/stack/SEH korunumu | Dört Windows akışı; 332 portable kontrol/13 senaryo/iki canary/12 warm; GTA 12/12 ve 39/39 matris; [rapor](d1-cwd-lock.md) | Slot içeriği opaque; kilit alma/OS/lazy yol açık |
| CRT cwd SEH kaydı | Wrapper/prologue entry ve doğal dönüş; FS:[0] zinciri, 80-byte stack ve 28-byte NT_TIB denetimi | Dört Windows akışı; 210 portable kontrol/11 senaryo/12 warm; GTA 12/12 ve 38/38 matris; [rapor](d1-cwd-seh.md) | Kilit/OS cwd, SEH sökümü ve cwd dönüşü açık |
| Dosya yöneticisi girişi | Doğal CFileMgr CALL ve üç PUSH; iki durak, buffer/maxlen ABI ve 136-byte korunum | Dört Windows akışı; 99 portable kontrol/12 senaryo/12 warm; GTA 12/12 ve 37/37 matris; [rapor](d1-file-manager-entry.md) | CRT cwd CALL/dönüşü, NUL/suffix ve manager dönüşü açık |
| İlk oyun başlatma yardımcıları | Boş Init dönüşü ve üç yerelleştirme bayrağının doğal yazımı; beş durak, ABI ve 13 guard byte denetimi | Dört Windows akışı; 215 portable kontrol/11 senaryo/12 warm; GTA 12/12 ve 36/36 matris; [rapor](d1-game-prelude.md) | CFileMgr, streaming/pad, initializer dönüşü ve N2/N3 açık |
| Uygulama başlatma dalı | Handler entry, executable detour, dolaylı tablo atlaması ve ilk oyun initializer CALL öncesi durak | Dört Windows akışı; 167 portable kontrol/11 senaryo/12 warm; GTA 12/12 ve 35/35 matris; [rapor](d1-application-routing.md) | Bu mod CALL öncesinde kalır; ilk iki yardımcının yürütmesi ayrı satırdadır |
| Uygulama olayına geçiş | Caller CALL, dağıtıcı entry ve AppEventHandler CALL önünde doğal argüman/yığın/register denetimi | Dört Windows akışı; 81 portable kontrol/11 senaryo/12 warm; GTA 12/12 ve 34/34 matris; [rapor](d1-event-dispatch.md) | Bu mod kendi CALL öncesi sınırında kalır; sınırlı handler rotası ayrı satırdadır |
| Instance başlangıcı | Gerçek CreateEvent/GetLastError ve yeni-instance helper dönüşü; mevcut event/nesne çakışmasında kontrollü ret | Dört Windows akışı; 129 portable kontrol/15 senaryo/12 warm; GTA 12/12 ve 33/33 matris; [rapor](d1-instance-startup.md) | Pencere/renderer, production instance politikası ve N2/N3 açık |
| Platform çağrı uyarlaması | Tek CALL için sentetik sonuç/stack/register/last-error ve bağlam rollback | Dört Windows akışı; 44 portable kontrol/20 senaryo/12 warm; GTA 12/12 ve 30/30 matris; [rapor](d1-platform-suppression.md) | Doğal API dönüşü/production shim değil; instance/pencere/renderer açık |
| Platform başlangıç sınırı | Uygulama prologue ve ilk host ayarı CALL önünde yığın/argüman/API denetimi | Dört Windows akışı; 20 portable kontrol, 17 senaryo/canary/12 warm; gerçek GTA 12/12 ve 29/29 matris; [rapor](d1-platform-startup.md) | Host-setting API çağrılmaz; uyarlama, pencere/renderer ve N2/N3 açık |
| Doğal uygulama girişi | Ayrı initializer/ikinci startup/app CALL-entry gözlemi | Dört Windows akışı; 24 portable kontrol, 23 senaryo + 12 warm; gerçek GTA 12/12 giriş ve 28/28 matris sonucu; [rapor](d1-application-entry.md) | Uygulama gövdesi/renderer/doğal frame ve N2/N3 açık |
| CRT başlatıcı sınırı | I/O dönüşü, 3 aday çağrı ve 1676 slotun denetimi | Dört Windows akışı; 15 portable kontrol, 16 senaryo + 12 warm; gerçek GTA 12/12 ve 27/27 beklenen matris sonucu; [rapor](d1-crt-startup.md) | Bu komut initializer CALL önünde durur; yeni giriş alt kanıtı üst satırda; initialized motor ve N3 açık |
| Çalıştırılmayan image koruma deneyi | Sabit SAEX fixture, bounded SEC_IMAGE taraması, 0x40/0x80 ve özel kopya ölçümü | 17 yeni test; x64 Debug 7 native/79 managed/115 Python; Windows Debug/Release eşleme ölçümü geçti; [rapor](d1-image-protection.md) | GTA/SDK/frame veya sandbox kanıtı değil; Linux/hosted bu görevde koşulmadı |
| Doğal startup dönüşü | Ayrı loader continuation, protect call/return ve startup dönüş denetimi | Gerçek GTA 12/12 doğal dönüş, 24/24 frame bayt örneği; 26/26 beklenen matris sonucu; [rapor](d1-startup-return.md) | Doğal frame/renderer/world ve N3 açık |
| Frame adayı ve faz örnekleri | Exact CALL/target reçetesi, üç read-only faz, fail-closed ret | Dört Windows akışı; 20 portable kontrol; 31 senaryo + 12 tekrar; gerçek GTA 12/12 ve 36/36 örnek; [rapor](d1-frame-target.md) | Doğal frame çağrısı/ABI/thread ve N3 açık |
| GTA içinde bootstrap yaşam döngüsü | Ayrı izin, sekiz C ABI çağrısı, yığın/status/export ve terminal durum denetimi | Dört Windows akışı; 24 senaryo + 12 tekrar; gerçek GTA 12/12, 96/96 dönüş; [rapor](d1-bootstrap-lifecycle.md) | observed_unverified; engine fazı/native symbol ABI, N3 frame/drain ve D1/D2 açık |
| ASI yolundan SAEX DLL yükleme | Ayrı call/return API/CLI, build artifact pini ve boş tarama sınırı | Dört Windows akışı; 27 senaryo/control/12 warm; gerçek GTA 12/12 load, 23/23 matris; [rapor](d1-asi-bootstrap-load.md) | Bu eski mod export çağırmaz; yeni C ABI kanıtı üst satırda; N2/D1/D2 açık |
| Codec fonksiyon bağları | Ayrı beşinci durak, sekiz pointer/target ve mapping sürekliliği | Dört Windows standart akışı; 24 senaryo/control/12 warm; özel GTA 12/12 koşu, 96/96 pointer bağı; sekiz regresyon; [rapor](d1-codec-bindings.md) | Codec fonksiyon çağrısı ve N2/D1 açık; SAEX C ABI üst satırda |
| Dinamik codec dönüşü | Ayrı API/CLI, 26 pin, dördüncü durak ve EAX/yeni modül kimliği denetimi | Dört Windows standart akışı; 24 senaryo + control + 12 warm; 12/12 özel GTA dönüşü, yedi ek regresyon; [rapor](d1-codec-return.md) | Bu komut codec load dönüşünde durur; ASI load ayrı üst satırdadır, C ABI ayrı üst satırda; N2 açık |
| Startup çağrı sınırı | Üçüncü durak, main-thread IAT watch, FF15 callsite/stack argument ve bounded anchor gözlemi | Dört Windows standart akışı; 19 senaryo + control + 12 warm; 12/12 private GTA startup hit ve dört original/legacy regresyon; [rapor](d1-startup-call.md) | Bu komut startup gövdesini çalıştırmaz; daha ileri codec/ASI alt kanıtları ayrı, C ABI ayrı üst satırda |
| Proxy dönüş sınırı | Ayrı iki-hit API/CLI, compiled entry/module bağı, thunk/return pointer ve entry restorasyonu ve IAT hedef kontrolü | Dört Windows standart akışı; 19 senaryo+12 warm; 12/12 private GTA proxy dönüşü ve üç gerçek legacy/original regresyon; [kanıt](d1-proxy-return.md) | Bu komut yalnız entry restorasyonu/IAT sonucunu gözler; tam initialization ve symbol ABI kapsam dışı |
| Kontrollü entry boundary | Ayrı run_to_entry/CLI, CREATE_PROCESS safhasında DR0 kurulumu, DLL/TLS izni ve PE giriş fault/16 byte gözlemi | x86 Debug/Release corpus ve 12/12 gerçek özel GTA entry hit; dokuz unload, aynı vorbisfile RVA 0x1D60 hedefi; [kanıt](d1-entry-boundary.md) | Sadece owned main thread giriş sınırı; sonraki DLL yükleme alt kanıtı ayrı, initialized symbol/ABI ve N3 açık |
| Native sembol bağlantı denetimi | C# engine linkage; normal/delay thunk, exact isim/ordinal, alias/hole, forwarder ve compact binding | 22 yeni corpus kaydı; x86 Debug/Release ve x64 Debug 79/79 managed test; 10 gerçek dosya karşılaştırması ve bağımsız dumpbin eşleşmesi; [rapor](d1-native-linkage.md) | Bu statik rapor runtime kanıtı değildir; dinamik codec raporu ayrı, tam initialization/ABI açık |
| Loader mapping lifecycle | Fixed history, gözlem içi mapping ID, known-active UNLOAD ve her LOAD için yeniden pin kontrolü | 10 metadata senaryosu + Windows corpus geçti; 12/12 private breakpoint adayı, 11 unload; [kanıt](d1-loader-lifecycle.md) | Bu bileşen mapping geçmişini izler; açık C ABI/initialization ve N3 kanıtı üretmez |
| Explicit launch context | Bounded UTF-16 environment snapshot, absolute cwd/directory identity ve ayrı context CLI | Windows x86 Debug/Release ve x64 Debug; ortam/cwd ve 12 warm çevrim geçti; gerçek mapping/UNLOAD retleri [raporda](d1-launch-context.md) | Context kimliği ve ortam snapshot kapsamıdır; açık C ABI veya sandbox kanıtı değildir |
| İncelenmiş loader profili | Compiled JSON→C++ recipe, exact engine/DLL hash bağı, iki origin ve child öncesi bütün pinleri hazırlama | Orijinal kurulumda AcLayers ret; üç dosyalı ayrı kopyada Debug 22/Release 21 DLL ve breakpoint adayı; eksik/değişmiş DLL ile child yok; [rapor](d1-loader-policy.md) | Eski komut mapping sınırında durur; yeni ASI pinleri ayrı explicit modda, C ABI ayrı üst satırda; N3 açık |
| Sınırlı loader gözlemi | Ayrı x86 CLI; aynı dosya ID/hash pinleri, bounded LOAD_DLL olayları, ilk exception'da durma ve owned child cleanup | DLL/TLS/main fixture canary'leri, 12 warm çevrim, kimlik/kota/CLI negatifleri; gerçek GTA apphelp mapping'inde beklenen ret; [kanıt](d1-loader-observation.md) | İlk exception sınırında durur; gerçek ASI load üst satırda, initialized GTA/ABI/N3 kapsamı açık |
| C++20 çekirdek temeli | StrongId/EntityRef, checked counter, ControlClock, LeaseAuthority, BoundedInbox | Windows x64 Debug/Release ve x86 Debug/Release, 16 native test/koşu | Tam scheduler/entity registry/server executable değil |
| Ortak şema | JSON kaynağından C++/C# kimlik tipleri ve fixture sürüm sabitleri | Deterministic generator ve stale-output kontrolü | Genel ContractSchema/SDK/proxy üretimi değil |
| Süreçler arası fixture | 44 byte sınırlı EntityRef frame, açık little-endian codec, version/length/zero-id ret | x64 C# → x64 ve x86 native process, pozitif/negatif test | Production IPC, GTA bridge, sandbox veya oyun protokolü değil |
| WorldPlan aracı | Sınırlı JSON, unique provider/mutator, dependency DAG, critical closure, capability/bütçe metadata | 26 managed/entegrasyon testi içindeki plan alt kümesi | SemVer resolver/cook/gerçek artifact/evidence/activation yok; productionEligible=false |
| EngineInspector ve native preflight | Ortak gözlem JSON'u; PE/hash/layout/anchor eşleşmesi; SDK bağımsız C++20 ve Windows çalıştırılmayan image okuması | Gerçek dosya: dört anchor eşleşti; sentetik PE/bozuk image/reader retleri; [N2 alt kanıtı](d1-engine-preflight.md) | Gözlem profili runtime desteği değildir; canAttach=false; bu satır dosya/image kanıtıdır; askıda process alt kapsamı aşağıda, initialized symbol/ABI açık |
| Askıda process observer | Exact dosya kapısı → owned suspended child → create-debug file ID/base/header/anchor → doğrulanmış çıkış | x64 Debug ve x86 Debug/Release fixture + gerçek GTA ilk image eşleşmesi; [kanıt](d1-suspended-process.md) | Oyun kodu yürütülmez; canAttach=false; unpack/ABI/DLL load kanıtı değildir |
| Native başlangıç envanteri | C# engine startup; bounded PE32 EXE/DLL import/delay/TLS/entry, yerel DLL/ASI aday grafiği ve digest | 57 managed test ve Debug/Release gerçek kurulum metadata'sı; [kanıt](d1-native-startup.md) | Statik metadata; Windows resolution/dinamik yükleme veya clean-install onayı değil; canAdvanceToLoader=false |
| SDK bağımsız bootstrap DLL | x86 C ABI 1, Initialize/Query/Stop, kendi host yolunu doğrulama, terminal ret/stop, BUSY | Oyun dışı 100 lifecycle çevrimi; gerçek GTA ASI load ve 0.1.17 C ABI alt kanıtları; [rapor](d1-bootstrap-module.md) | GTA içindeki çağrılar üst satırda doğrulandı; native binding/frame/drain yok, can_attach=0 |
| Plugin-SDK-SA dependency | 23 dosyalı exact source/patch/recipe/notice lock, açık edinme, private x86 library/probe; ADR-36 private C++23 uyarlaması | x86 Debug/Release opt-in build/probe; 17 dependency + 2 gerçek CMake ret testi geçti; [exact kanıt](d1-native-dependency.md) | Seçilmiş build alt kümesi; bootstrap/hook/production IPC/GTA yok; runtimeEligible=false |
| Belge kontrolü | Dosya/link/JSON, kaynak→belge eşlemesi, ekleme/değişme/silme takibi | Foundation ve yeni opt-in native build girişinde zorunlu; 5 tooling testi | Güncel statik sonuç out/verification/docs-check.json; anlamsal doğruluğu tek başına kanıtlamaz |
| Linux x64 | Portable core, preflight/bootstrap session ve managed fixture | 0.1.13 GitHub Ubuntu 24.04 Debug: 4 native suite, 79 managed, 51 Python geçti; 0.1.14–0.1.31 Linux yeniden koşulmadı; [yayın kanıtı](github-publication.md) | Windows/GTA adapter veya production server doğrulaması değil |
| GNS / worker runtime / GTA gameplay | Mimari kararları korunuyor | Çalışan oyun döngüsü veya multiplayer bu kesitte sınanmadı | GNS kimliği, sandbox, native frame/pool/collision ve multiplayer yok |

## Aşama kapıları

D0 v0.5 belge teslimatı tarihsel olarak tamamlandı. D1 **başladı ve henüz tamamlanmadı**. R-01 dosya incelemesi, R-13 metadata/kimlik üretimi, R-14 queue primitive ve R-21 lease/clock alt kanıtları oluştu. D1-N1/R-02a ve AC-89'un seçilmiş build alt kümesi eklendi; bütün ana R kayıtları açık kalır. AC-34/35/45/71/72/83/86 [foundation](d1-foundation.md), N1 ise [native dependency raporundaki](d1-native-dependency.md) sınırla yorumlanır. AC-90 dosya/image, oyun dışı DLL, ilk create-debug görüntü ve gerçek GTA ASI load alt kümeleri ayrı raporlarla sınandı; initialized runtime bölümü ve AC-91–96 çalıştırılmadı. 96 AC'nin topluca geçtiği söylenmez.

0.1.31 ile **D1-N2 CRT mevcut kilidi alma alt kesiti** doğrulandı. Beş doğal checkpoint, admitted ntdll hedefi, 24-byte kritik bölüm sahipliği ve 84-byte caller stack korunumu denetlenir. [Kanıt ve sınır](d1-cwd-acquire.md). Heap ve yalnız aynı GTA image içindeki nesneler denetlenir; boş/tutulmuş/geçersiz nesne reddedilir. Sonraki kesit cwd helper/OS/copy ve ardından unlock/SEH sökümüdür. Cwd/manager/initializer dönüşü, renderer/doğal frame ve N2/N3/D1/D2 açıktır.

## Kod 0.1.20 doğrulaması

Dört yerel Windows akışı geçti: x86 Debug/Release her profilde 19 native suite, 79 managed ve 170 Python; x64 Debug/Release 8 native, 79 managed ve 122 Python. Yeni portable spec 15 kontrol, x86 CRT corpus'u 16 senaryo + 12 tekrar; her iki x86 koşusunda handle 141 → 141. Gerçek GTA 12/12 sınır gözlemi, 36/36 tam tablo örneği; 27/27 beklenen matris sonucu, 24/24 child çıkışı ve 115/115 girdi korunumu. Başlatıcı gövdeleri ve uygulama girişi yürütülmedi. [Exact artifact'lar, ilk hatalar ve kapsam](d1-crt-startup.md). Linux/hosted CI/N1 SDK yeniden koşulmadı.

## Kod 0.1.19 doğrulaması

Dört yerel Windows akışı geçti: x86 Debug/Release her profilde 17 native suite, 79 managed ve 161 Python; x64 Debug/Release 7 native, 79 managed ve 115 Python. Portable frame/startup sözleşmesi 33 kontrol, doğal startup corpus'u 18 senaryo + 12 tekrar içerir; son iki x86 logunda handle 134 → 134. Gerçek GTA 12/12 doğal startup dönüşü ve 24/24 frame bayt örneği; 26/26 beklenen matris sonucu, 23/23 child çıkışı ve 100/100 girdi hash korunumu doğrulandı. Bu iki örnek doğal frame çağrısı değildir. [Artifact'lar, ilk hatalar, düzeltilen doküman kapısı ve tam loglar](d1-startup-return.md). Linux/hosted CI/N1 SDK yeniden koşulmadı.

## Kod 0.1.18 doğrulaması

Windows x86 Debug/Release her profilde 16 native suite, 79 managed, 135 Python; x64 Debug/Release 7 native, 79 managed, 91 Python geçti. Frame sözleşmesinde 20 portable kontrol; genişletilmiş bootstrap corpusunda 31 senaryo + 12 tekrar var. Gerçek GTA 12/12 koşuda 36/36 faz örneği ve 96/96 bootstrap dönüşü; regresyonlarla 25/25 beklenen sonuç, 22/22 child çıkışı ve 84 girdi hash korunumu doğrulandı. İlk generator/derleme sorunları ve exact artifact'lar [raporda](d1-frame-target.md). Linux/hosted CI/N1 SDK derlemesi yeniden çalıştırılmadı; N1 arşiv/source doğrulaması salt okunur geçti.

## Kod 0.1.17 doğrulaması

Windows x86 Debug/Release: her profilde 15 native suite, 79 managed ve 126 Python; x64 Debug/Release: 6 suite, 79 managed ve 84 Python geçti. Bootstrap corpus 24 senaryo + 12 tekrar ve beş HIGHLOW kontrolünü içerir. Gerçek GTA 12/12 koşuda 96/96 C ABI dönüşü; regresyonlarla 24/24 beklenen sonuç, 21/21 çocuk çıkışı ve 59 girdi hash korunumu doğrulandı. [Artifact kimlikleri, başarısız denemeler ve sınırlar](d1-bootstrap-lifecycle.md). `observed_unverified` durumu native binding/frame desteği değildir. Linux/hosted/N1 SDK tekrar çalıştırılmadı.

## Kod 0.1.16 doğrulaması

Windows x86 Debug/Release: her profilde 14 native suite, 79 managed ve 112 Python; x64 Debug/Release: 6 suite, 79 managed ve 78 Python geçti. ASI corpus 27 senaryo/control/12 warm. Gerçek GTA 12/12 başarılı SAEX DLL yükleme dönüşü; 11 regresyonla 23/23 beklenen sonuç, oluşturulan 20/20 child confirmed exit ve 44 girdi hash korunumu. [Exact artifact'lar, başarısız ilk denemeler ve kapsam](d1-asi-bootstrap-load.md). Initialize/Query/Stop GTA'da çağrılmadı; N2/D1 ve multiplayer açık. Linux/hosted CI/N1 SDK bu sürüm için yeniden koşulmadı.

## Kod 0.1.15 doğrulaması

Windows x86 Debug/Release: her profilde 13 native suite, 79 managed ve 101 Python; x64 Debug/Release: 6 suite, 79 managed ve 72 Python geçti. Binding corpus 24 senaryo + pozitif canary + 12 warm çevrimdir. Gerçek özel GTA 12/12 codec_bindings_verified, 96/96 doğru pointer bağı; sekiz regresyonla 20/20 beklenen sonuç, oluşturulan 18/18 child için confirmed exit. 22 original/private input hash'i aynı. [Ayrıntı ve artifact kimlikleri](d1-codec-bindings.md). 0.1.15 Linux/hosted/N1 SDK yeniden koşulmadı; gerçek SAEX bootstrap ve N2/D1/D2 açık.

## Kod 0.1.14 doğrulaması

Windows x86 Debug/Release: her profilde 12 native suite, 79 managed ve 93 Python; x64 Debug/Release: 6 suite, 79 managed ve 66 Python geçti. Codec corpus'u 24 senaryo + pozitif canary + 12 warm çevrim; ilk test varsayımı düzeltildi, başarısız kayıt saklandı. Gerçek özel GTA 12/12 codec_return_verified; yedi ek regresyonla 19/19 beklenen sonuç, oluşturulan 17/17 child için confirmed exit. Orijinal/tarihsel/yeni pozitif kopyadaki 22 input hash'i aynı. [Ayrıntı, artifact kimlikleri ve sınırlar](d1-codec-return.md). 0.1.14 Linux/hosted/N1 SDK yeniden çalıştırılmadı; SAEX DLL'si GTA'da yüklenmedi.

## Kod 0.1.13 doğrulaması

Windows x86 Debug/Release: her koşuda 11 native suite, 79 managed ve 85 Python testi geçti. X64 Debug/Release: her koşuda 6 suite, 79 managed ve 60 Python geçti. Startup corpus’u 19 senaryo + gözlemcisiz canary kontrolü + 12 warm çevrimdir. İlk custom CRT entry link hatası düzeltildi, başarısız kayıt saklandı.

Gerçek özel kopyada 6 Debug+6 Release startup_call_verified; dört original/legacy regresyonla **16/16 confirmed exit**. Her pozitif koşuda aynı callsite/return address, target ve stack alanı doğrulandı; dört mevcut anchor’ın 48 okuması eşleşti. Orijinal 13/private 3 input hash’i değişmedi. [Tam kanıt](d1-startup-call.md). Linux/hosted CI/N1 bu kesitte yeniden koşulmadı; gerçek SAEX DLL/dinamik wrapper gövdesi ve N2/D1 açık kalır.

## Kod 0.1.12 doğrulaması

Windows x86 Debug/Release: her koşuda 10 native suite, 79 managed ve 76 Python testi geçti. X64 Debug/Release: her koşuda 6 suite, 79 managed ve 53 Python geçti. Proxy corpus’u 19 senaryo+12 warm çevrim; ilk Debug stack overflow giderildi ve başarısız kayıt saklandı. Gerçek özel kopyada 6 Debug+6 Release proxy_return_verified; 3 original/legacy regresyonla toplam 15/15 confirmed exit. Orijinal 13 ve private 3 dosya hash’i değişmedi. [Ayrıntı ve artifact kimlikleri](d1-proxy-return.md).

Linux/hosted CI ve N1 opt-in SDK bu değişiklik için yeniden doğrulanmadı. Proxy sonrası oyun girişi, SAEX bootstrap yüklemesi ve D1/D2 ürün davranışı hazır değildir.

## Kod 0.1.11 doğrulaması

X86 Debug/Release son standart akışlarının her biri 9 native suite, 79 managed/entegrasyon ve 59 Python testiyle geçti. X64 Debug 6 native suite, 79 managed/entegrasyon ve 38 Python testiyle geçti. Yeni entry corpus'u 12 senaryo ve 12 warm çevrimdir; sekiz entry-policy testi ve iki CLI negatif testi eklendi. İlk geç DR0 kurulum hatası ve fixture Apphelp ret sonucunun nedenleri/düzeltmeleri [raporda](d1-entry-boundary.md) korunur. Son standart akışlar geçti; x64 Release/Linux çalıştırılmadı, N1 opt-in SDK build tekrarlanmadı.

Son gerçek matris: 6 Debug + 6 Release özel GTA koşusunun 12/12'si entry_boundary_modified/exit 3 verdi; 9 unload işlendi. Her atlama hedefi vorbisfile.dll+0x1D60 oldu. Orijinal entry komutu AcLayers'ta initializer öncesi ret verdi, private eski context komutu ilk breakpoint'te durdu. Son matriste 14/14 child exit ve orijinal 13 + private 3 native dosyada önce/sonra/0.1.10 hash eşitliği doğrulandı. DLL başlangıcının ilerlediği ölçüldü; oyun giriş atlaması, unpack ve SAEX bootstrap yürütülmedi.

## Önceki kod 0.1.10 doğrulaması

X86 Debug/Release standart akışlarının her biri 8 native suite, 79 managed/entegrasyon ve 49 Python testiyle geçti. X64 Debug 6 native suite, 79 managed/entegrasyon ve 30 Python testiyle geçti. Yeni 22 linkage kaydı bu 79'un içindedir. Üç standart akış da ilk denemede başarılı; x64 Release/Linux çalıştırılmadı. N1 SDK bağımlılığı değişmedi ve opt-in SDK build bu kesitte tekrarlanmadı.

Gerçek kurulum dosyalarıyla Debug/Release toplam 10 salt okunur karşılaştırma: 8 direct metadata complete, wrapper→hooked için 2 beklenen incomplete; toplam 94 direct binding. İki configuration'ın raporları aynı; wrapper 8/hooked 34 export slotu bağımsız dumpbin ile aynı. Orijinal 13 + private 3 native dosyanın önce/sonra ve 0.1.9 kanıtıyla hash eşitliği korundu. [Tam kanıt](d1-native-linkage.md). Gerçek GTA bu kesitte başlatılmadı; initialization, gerçek SAEX DLL ve ABI hâlâ açık. Önceki 0.1.9 OS loader sonuçları kendi tarihsel kapsamını korur.

## Önceki kod 0.1.9 doğrulaması

X86 Debug/Release standart akışlarının her biri 8 native suite, 57 managed ve 49 Python testiyle geçti. X64 Debug 6 native suite, 57 managed ve 30 Python testiyle geçti. Yeni on ledger senaryosu aynı-base reuse/stale ID, duplicate/unknown unload, sıra/wrap/kapasite ve budget iadesi olmamasını doğrular. [Tam sonuç](d1-loader-lifecycle.md).

Altışar Debug/Release gerçek private koşunun 12/12'si breakpoint adayına ulaştı; 11'inde ole32/combase mapping'i emekli edildi. Orijinal explicit context AcLayers, legacy 3-pin apphelp retlerini korudu. Toplam 14/14 child exit ve 13 original + 3 private dosya hash korunumu doğrulandı. Same-base remap gerçek GTA'da görülmedi; initialization/SAEX DLL/ABI ve N3/D1 açık. X64 Release/Linux çalıştırılmadı.

## Önceki kod 0.1.8 doğrulaması

X86 Debug/Release: her biri 7 native suite, 57 managed, 49 Python testi geçti. X64 Debug: 5 native suite, 57 managed, 30 Python testi geçti. Yeni context corpus'u Unicode environment freeze, cwd/PATH ile gerçek own DLL çözümü, child öncesi negatifler ve 12 warm handle çevrimini doğrular. İlk hatalı koşular ve düzeltmeler [raporda](d1-launch-context.md) saklanır; x64 Release/Linux çalıştırılmadı.

Son GTA artifact'ı Debug özel kopyada 21 DLL/breakpoint adayı; Release özel kopyada 19 DLL sonrası desteklenmeyen UNLOAD_DLL_DEBUG_EVENT ret; orijinal kurulumda AcLayers ret verdi. Aynı context'te daha önceki Release aday sonucu da korunur; build türüne göre deterministik başarı vaadi yoktur. Yedi deney çocuğu kapatıldı, 13 orijinal ve 3 kopya native dosya hash'i korundu. canAttach/initializationVerified false; bootstrap GTA'ya yüklenmedi.

## Önceki kod 0.1.7 doğrulaması

X86 Debug/Release standart akışları 6 native suite, 57 managed ve 47 Python testiyle geçti; x64 Debug 4 native suite, 57 managed ve 30 Python testiyle geçti. [Profil raporu](d1-loader-policy.md), original-context AcLayers reddini, private-context Debug 22/Release 21 mapping ve breakpoint adayını, child öncesi iki gerçek negatif senaryoyu ayırır. Release'te eski üç pinli modun apphelp reddi de korundu. Original 13 native girdi ve üç pozitif kopya hash'i değişmedi. Bütün deney çocukları kapandı; N2 initialization/SAEX DLL/ABI ve N3/D1 açık. Linux ve x64 Release bu kesitte yeniden çalıştırılmadı.

Kod 0.1.7 son belge kontrolü: 74 Markdown, 966 yerel bağlantı, 6 JSON örneği ve sıfır hata. İncelenmiş policy generator çıktısı güncel; kullanıcı çalışma ağacı ve oyun kurulumu korunur.

## Önceki kod 0.1.6 doğrulaması

X86 Debug/Release ve x64 Debug standart build/test geçti. X86: 6 native suite, 57 managed, 38 Python testi; x64: 4 native suite, 57 managed, 22 Python testi. Yeni x86 suite 12 warm çevrimde handle stabilitesi ve DLL/TLS/main canary sınırını doğruladı. [Tam loader raporu](d1-loader-observation.md). Debug ve Release gerçek GTA deneyleri ntdll/kernel32/kernelbase sonrasında apphelp.dll için aynı izin listesi reddini verdi; own child exit ve 13 native girdi hash'inin korunduğu doğrulandı. Bu ret, DLL'nin bozuk olduğu veya GTA başlangıcının geçtiği anlamına gelmez. Eski statik/ilk-create gözlemleri ve bootstrap C ABI kapsamı değişmedi; Linux/x64 Release bu kesitte yeniden çalıştırılmadı.

Kod 0.1.6 için son belge kontrolü 73 Markdown/934 yerel bağlantı/6 JSON örneği ve sıfır hatadır; N1'in 23 kaynak dosyası salt okunur verify kontrolünü geçti.

## Önceki kod 0.1.5 doğrulaması

Gerçek envanterdeki `bass.dll` raw padding farkı, güvenli okuma sınırları korunarak açık LayoutNotes kaydına ayrıldı. CLI InvalidDataException filtresi ve padding notunun runtime izni vermemesi regresyonlarla kontrol edilir; nihai koşu sonuçları aşağıdaki raporda kayıtlıdır.

X64 Debug ve x86 Release standart build/test geçti: x64 4/x86 5 native suite, 57 managed test (31 yeni), x64 22/x86 32 Python testi. Gerçek kurulumda executable + 12 DLL/ASI, 75 import ilişkisi ve bass.dll için bir padding notu kaydedildi. Debug/Release snapshot digest'leri eşit; 13 native dosyanın hash'i korundu. [Tam kanıt](d1-native-startup.md). Metadata alt kapsamı tamamlandı, loader fazı ilerletilmedi. N1 source verify geçti. Belge sonucu: 72 Markdown, 899 yerel bağlantı, 6 JSON örneği, sıfır hata.

## Önceki kod 0.1.4 doğrulaması

X64 Debug ve x86 Debug/Release standart build/test geçti: x64 4/x86 5 native suite, 26 managed ve x64 22/x86 32 Python testi. Her yapılandırmada 12 owned child lifecycle çevriminde handle sayısı sabit kaldı; canary, ret ve exception cleanup başarılı. Üç CLI gerçek GTA'nın ilk create-debug görüntüsünü file ID/base/header/dört anchor üzerinden eşleştirdi; ana thread resume edilmeden child exit doğrulandı, orijinal exe hash'i değişmedi. [Artifact ve deney raporu](d1-suspended-process.md). AC-90/R-01a/b'nin yalnız pre-user-code görüntü alt kapsamıdır; unpack/ABI/DLL load, N3 ve D1 açık kalır.

Kod 0.1.4 tarihsel belge kontrolü: 71 Markdown, 872 yerel bağlantı, 6 JSON örneği, sıfır hata. N1'in 23 dosyalı kaynak kilidi salt okunur verify kontrolünü yeniden geçti; SDK yeniden derlenmedi.

## Önceki kod 0.1.3 doğrulaması

X64 Debug ve x86 Debug/Release standart build akışları geçti. X64 üç, x86 dört native suite; 26 managed test; x64 17, x86 27 Python testi başarılı. X86 Debug/Release DLL'leri oyun dışı host'ta 100'er ölçülen load/init/stop/unload çevrimini geçti. Artifact/CRT/TLS denetimi temiz; kaynak kilidi verify edildi. Ortak dosya yordamının x86/x64 gerçek dosya regresyonu geçti, oyun executable'ı değişmedi. Statik sonuç: 70 Markdown, 834 yerel bağlantı, 6 JSON örneği, sıfır hata.

Yeni DLL, portable session ve artifact denetimi [başlangıç modülü raporunda](d1-bootstrap-module.md) kendi kapsamıyla kayıtlıdır. AC-90/R-01b'nin oyun dışı yükleme/ret/stop alt kanıtı oluştu; gerçek GTA load sırası ve R-01c hook kanıtı açık. Process-wide ilk handle artışı ayrıca raporlanır; warm çevrimlerin sabitliği bütün process kaynaklarının sıfır sızıntı kanıtı değildir.

## Önceki kod 0.1.2 doğrulaması

13 Eylül 2026: x64 Debug, x86 Debug ve x86 Release standart build akışları geçti. Aynı üç native probe gerçek dosyayı çalıştırmadan image olarak eşledi; başlık ve dört anchor eşleşti. C# dosya/profil digest eşleşmesi ve oyun dosyasının değişmediği doğrulandı. N1 dependency verify tekrar geçti. Son belge gate'i: 69 Markdown, 801 yerel bağlantı, 6 JSON örneği, sıfır hata.

N2 kod 0.1.2 doğrulama kapsamı: iki native CTest suite'i (16 foundation testi ve yeni PE/profile corpus), 26 managed/entegrasyon testi, 8 profil generator + 4 native CLI ret + 5 belge tooling testi. Platform/koşu sonuçları [N2 raporunda](d1-engine-preflight.md). N1'in x86 Debug/Release 20 test tanımlı eski kanıtı kendi kapsamıyla korunur; N2 için SDK source/recipe değişmedi. Tam lock/artifact kimlikleri ve gerçek GTA doğrulamasının sınırı [N1 raporundadır](d1-native-dependency.md). SDK private derleme sonucu engine/oynanış capability'si açmaz.

## Güncelleme zorunluluğu

Her kaynak ekleme/değiştirme/çıkarma bu tablo, [change-log](change-log.md) ve ilgili normatif sözleşmeyle aynı değişiklikte yapılır. D1'de olmayan bir özellik kodlanınca “planlanan” satırı ölçülen kapsamla değiştirilir; silinirse kullanıcı etkisi ve migration belirtilir. Test sonucu olmayan platform veya native yetenek verified olamaz.

0.1.8 ilk gerçek context koşusunda Debug özel kopya 19 DLL sonrası loader_unexpected_event ile güvenli durdu; iki Release özel kopya koşusu breakpoint adayına ulaştı, orijinal kurulum AcLayers'da ret verdi. Dört child'ın çıkışı ve native dosyaların hash korunumu doğrulandı. Tanı eksikliğini gidermek için additive lastEventCode/lastEventThreadId eklendi; sonuçlar initialization sayılmaz.

0.1.8 son statik belge kontrolü: 75 Markdown, 990 yerel bağlantı, 6 JSON örneği ve sıfır hata. Kaynak/belge eşlemesi başarılıdır; bu statik sonuç anlamsal kusursuzluk veya oyun içi doğrulama yerine geçmez.

0.1.9 son statik belge kontrolü: 76 Markdown, 1013 yerel bağlantı, 6 JSON örneği ve sıfır hata. Bu sonuç kaynak/belge eşlemesi ve statik yapı kanıtıdır; bütün mimarinin anlamsal kusursuzluğu veya GTA initialization başarısı değildir.

0.1.11 ara gerçek kanıt: ilk initializer koşusu imm32 unpinned ret verdi. Ayrı entry-policy.json + strict compiler, mevcut 22 pinin exact digest'ine bir incelenmiş system-x86 imm32 kaydı ekler. Sekiz generator testi geçti; ilk ek-pinli Debug özel kopya entry_boundary_modified sonucuna ulaştı. Son standart toplamlar ve 12/12 gerçek entry tekrarı yukarıdaki 0.1.11 bölümündedir; ilk ret kanıtı korunur.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.

## Kod 0.1.12

[Proxy dönüş sınırı](d1-proxy-return.md) uygulanmıştır; test ve gerçek GTA kanıtı ilgili raporda ayrı tutulur. N2/D1 ve oynanabilir multiplayer kapsamı açık kalır.

## Kod 0.1.13

[Startup çağrı raporu](d1-startup-call.md) implementasyon, fixture ve gerçek GTA kanıtını ayrı tutar. D1 kapıları henüz tamamlanmadı.

0.1.14 son statik kontrol: 92 Markdown, 1287 yerel bağlantı, 6 JSON örneği ve sıfır hata; --base HEAD kaynak/belge eşlemesi başarılı. Bu sonuç anlamsal kusursuzluk veya oyun initialization kanıtı değildir.

0.1.15 son statik kontrol: 93 Markdown, 1320 yerel bağlantı, 6 JSON örneği ve sıfır hata; --base HEAD kaynak/belge eşlemesi başarılı. Bu statik sonuç tüm mimarinin anlamsal doğruluğu veya GTA initialization kanıtı değildir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.21 doğrulaması

Dört yerel Windows akışı geçti: x86 Debug/Release 21 native suite/79 managed/179 Python; x64 Debug/Release 9/79/129. Portable spec 24 kontrol; x86 corpus 23 senaryo ve 12 warm çevrim. Debug corpusunda handle 136 → 136, Release 141 → 141. Gerçek GTA 12/12 giriş, 28/28 beklenen matris sonucu, 25/25 child çıkışı ve 130/130 girdi korunumu doğrulandı. Yeni CLI'nin Debug yığın taşması düzeltildi; ilk hatalar ve exact artifact'lar [raporda](d1-application-entry.md). İlk uygulama komutu/renderer/dünya henüz çalıştırılmadı; Linux, hosted CI ve N1 SDK bu kesitte tekrar koşulmadı.

## Kod 0.1.22 doğrulaması

Dört yerel Windows akışı geçti: x86 Debug/Release 23 native suite/79 managed/188 Python; x64 Debug/Release 10/79/136. Yeni 20 portable kontrol, 17 x86 senaryo + canary kontrolü + 12 warm çevrim geçti. Gerçek GTA 12/12 sınır, 29/29 beklenen matris, 26/26 child çıkışı ve 145/145 girdi korunumu; ayrıca okunan foreground timeout değeri değişmedi. Devam izni ile yürütme kanıtı ayrıldı; ara hata halinde body sonucu null olabilir. [Exact kanıtlar](d1-platform-startup.md). Sistem API'si/pencere/renderer henüz çalıştırılmadı; Linux/hosted/N1 SDK tekrar koşulmadı.

## Kod 0.1.23 doğrulaması

Dört Windows akışı geçti: x86 Debug/Release 25 native suite, 79 managed, 196 Python; x64 Debug/Release 11/79/142. Yeni 44 portable kontrol, 20 x86 senaryo + canary kontrolü + 12 warm çevrim geçti. Gerçek GTA 12/12 bastırılmış dönüş, 30/30 matris, 27/27 child çıkışı ve 160/160 girdi korunumu. 220 kaynak/yapılandırma girdisi build sonrası değişmedi. [Ayrıntı ve ilk hatalar](d1-platform-suppression.md). Linux/hosted/N1 SDK bu kesitte tekrar koşulmadı.

## Kod 0.1.24 doğrulaması

Dört Windows akışı geçti: x86 Debug/Release 27 native suite, 79 managed, 205 Python; x64 Debug/Release 12/79/149. Yeni 129 portable kontrol, 15 x86 senaryo + pencere canary kontrolü + 12 warm çevrim geçti. Gerçek GTA 12/12 instance dönüşü, 33/33 matris, 30/30 child çıkışı ve 175/175 girdi korunumu. 228 kaynak/yapılandırma girdisi ve altı x86 artifact son kontrolde aynı. [Ayrıntı ve ilk hatalar](d1-instance-startup.md). Linux/hosted/N1 SDK tekrar koşulmadı; N2/N3/D1/D2 açık.

## Kod 0.1.25 doğrulaması

Dört Windows akışı geçti: x86 Debug/Release 29 native suite, 79 managed, 212 Python; x64 Debug/Release 13/79/154. Yeni 81 portable kontrol, 11 x86 senaryo + uygulama canary kontrolü + 12 warm çevrim geçti. Gerçek GTA 12/12 dispatch sınırı, 34/34 matris, 31/31 child çıkışı ve 190/190 girdi korunumu. 236 kaynak/yapılandırma girdisi ve altı x86 artifact final kontrolde aynı. [Kanıt ve kapsam](d1-event-dispatch.md). Linux/hosted/N1 SDK tekrar koşulmadı; N2/N3/D1/D2 açık.

## Kod 0.1.26 doğrulaması

Dört Windows akışı geçti: x86 Debug/Release 31 native suite, 79 managed, 219 Python; x64 Debug/Release 14/79/159. Yeni 167 portable kontrol, 11 x86 senaryo + initializer canary kontrolü + 12 warm çevrim geçti. Gerçek GTA 12/12 routing sınırı, 35/35 matris, 32/32 child çıkışı ve 205/205 girdi korunumu. 244 kaynak/yapılandırma girdisi ve altı x86 artifact final kontrolde aynı. [Kanıt ve kapsam](d1-application-routing.md). Linux/hosted/N1 SDK tekrar koşulmadı; N2/N3/D1/D2 açık.

## Kod 0.1.27 doğrulaması

Dört Windows akışı geçti: x86 Debug/Release 33 native suite, 79 managed, 226 Python; x64 Debug/Release 15/79/164. Yeni 215 portable kontrol, 11 x86 senaryo + file-manager canary kontrolü + 12 warm çevrim geçti. Gerçek GTA 12/12 helper dönüşü/veri readback, 36/36 matris, 33/33 child çıkışı ve 220/220 girdi korunumu. 252 kaynak/yapılandırma girdisi ve altı x86 artifact final kontrolde aynı. [Kanıt ve kapsam](d1-game-prelude.md). Linux/hosted/N1 SDK tekrar koşulmadı; N2/N3/D1/D2 açık.

## Kod 0.1.28 doğrulaması

Dört Windows akışı geçti: x86 Debug/Release 35 native suite, 79 managed, 233 Python; x64 Debug/Release 16/79/169. Yeni 99 portable kontrol, 12 x86 senaryo + cwd canary kontrolü + 12 warm çevrim geçti. Gerçek GTA 12/12 manager girişi/buffer/maxlen readback, 37/37 matris, 34/34 child çıkışı ve 235/235 girdi korunumu. 260 kaynak/yapılandırma girdisi ve altı x86 artifact final kontrolde aynı. [Kanıt ve kapsam](d1-file-manager-entry.md). Linux/hosted/N1 SDK tekrar koşulmadı; N2/N3/D1/D2 açık.

## Kod 0.1.29 doğrulaması

Dört Windows akışı geçti: x86 Debug/Release 37 native suite, 79 managed, 240 Python; x64 Debug/Release 17/79/174. Yeni 210 portable kontrol, 11 x86 senaryo + post-SEH canary kontrolü + 12 warm çevrim geçti. Gerçek GTA 12/12 wrapper/prologue/kayıt kurulumu, 38/38 matris, 35/35 child çıkışı ve 250/250 girdi korunumu. 268 kaynak/yapılandırma girdisi ve altı x86 artifact final kontrolde aynı. [Kanıt ve kapsam](d1-cwd-seh.md). Linux/hosted/N1 SDK tekrar koşulmadı; N2/N3/D1/D2 açık.

## Kod 0.1.30 doğrulaması

Dört Windows akışı geçti: x86 Debug/Release 39 native suite, 79 managed, 247 Python; x64 Debug/Release 18/79/179. Yeni 332 portable kontrol, 13 x86 senaryo, iki dal için post-comparison canary ve 12 warm çevrim geçti. Gerçek GTA 12/12 selector/slot/CMP/SEH korunumu, 39/39 matris, 36/36 child çıkışı ve 265/265 girdi korunumu. 276 kaynak/yapılandırma girdisi ve altı x86 artifact final kontrolde aynı. [Kanıt ve kapsam](d1-cwd-lock.md). Linux/hosted/N1 SDK tekrar koşulmadı; N2/N3/D1/D2 açık.

## Kod 0.1.31 doğrulaması

Dört Windows akışı geçti: x86 Debug/Release 41 native suite, 79 managed, 254 Python; x64 Debug/Release 19/79/184. Yeni 141 portable kontrol, 17 x86 senaryo, iki canary ve 12 warm çevrim geçti. Gerçek GTA 12/12 doğal EnterCriticalSection/selector dönüşü ve thread sahipliği, 40/40 matris, 37/37 child çıkışı ve 280/280 girdi korunumu. 284 kaynak/yapılandırma girdisi ve altı x86 artifact final kontrolde aynı. [Kanıt ve kapsam](d1-cwd-acquire.md). İlk heap-only keşif reddi raporda korunur. Linux/hosted/N1 SDK tekrar koşulmadı; N2/N3/D1/D2 açık.
