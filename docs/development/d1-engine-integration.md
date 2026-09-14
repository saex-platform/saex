# D1 motor entegrasyonu uygulama planı

Tarih: 13 Eylül 2026. Mimari v0.24 / kod 0.1.17. Durum: **D1-N1 build ve N2 dosya/image/bootstrap/ilk-create/statik envanter uygulandı; sınırlı loader ve GTA içindeki SAEX C ABI yaşam döngüsü doğrulandı; engine fazı/native symbol ABI/tam initialization ve N3–N7 açık**. [N1 kod/kanıt raporu](d1-native-dependency.md). [Normatif SDK sözleşmesi](../architecture/native-sdk-integration.md) · [Kaynak incelemesi](../references/plugin-sdk-sa.md) · [Durum](status.md) · [Yol haritası](../roadmap.md)

## Başlangıç ve hedef

Mevcut foundation; Windows x86/x64 core, C# araçları ve yerel process fixture'ıdır. GTA engine profili ve hook'lar doğrulanmadı. Dryxio kaynak pini `b55e89b336a81448c1aa1a5b188431c9845ebaa9` seçildi; private SDK library, x86 build probe'u ve SDK bağımsız başlangıç DLL'si vardır; gameplay adapter/launcher yoktur. İlk hedef, tek gerçek GTA process'inde doğrulanmış başlatma, frame gözlemi, bounded komut uygulama ve temiz kapanış kanıtıdır.

Bu plan uygulayıcıya küçük, derlenen ve negatif testi bulunan kesitler verir. Kesit adları proje planı kimlikleridir; N1'in gerçek target/komutları [uygulama raporunda](d1-native-dependency.md) eşlenir. Süre tahmini verilmez; her kesitin tamamlanması çıktı ve kapıyla ölçülür.

## Uygulama sırası

| Kesit / sorumlu alan | Somut teslim | Geçiş ve negatif kanıt | Durum |
|---|---|---|---|
| D1-N1 Dependency/build | Exact commit/patch/source/recipe lock, dosya ve notice envanteri, SDK'yi private tüketen x86 compile/link probe; core build'inden ayrı opt-in giriş | R-02a, AC-89: x64 link/include sızıntısı, yanlış/missing source digest ve header/library karışımı ret; C++20 başarısız deneyi, private C++23 kararı, CRT/packing kayıtlı | Seçilmiş 23 dosyalı build kapsamı uygulandı; [kanıt](d1-native-dependency.md) |
| D1-N2 Engine profile | Salt okunur inspector genişletmesi; gerçek binary/symbol/hook evidence; SDK bağımsız bootstrap ve mapped-image kontrolü | R-01a/b, AC-90: unknown/truncated/wrong arch, benzer sürüm marker'ına rağmen yanlış hash, runtime hook uyuşmazlığı ret; ret öncesi SDK çağrısı/hook yazımı sıfır | [Dosya/image](d1-engine-preflight.md), [oyun dışı DLL](d1-bootstrap-module.md), [askıda süreç](d1-suspended-process.md), [statik native başlangıç](d1-native-startup.md) ve [sınırlı loader mapping](d1-loader-observation.md) kapsamları ve [GTA C ABI yaşam döngüsü](d1-bootstrap-lifecycle.md) ayrı; engine fazı/native symbol ABI/tam initialization açık |
| D1-N3 Lifecycle observer | Bir init/frame/stop gözlemi; hook ownership registry, kısmi kurulum geri alma ve callback drain; yerel teşhis | R-01c, AC-91/15: normal/başarısız başlatma ve stop; çakışan hook ve callback içindeyken stop. Her seçilen profil için en az 20 temiz process başlat/kapat koşusu hedefi | Planlandı |
| D1-N4 Host/frame bridge | Gerçek x86 adapter ↔ x64 Host production IPC ilk alt kümesi; no-op/read-only komut, session/epoch/sequence, kuyruk ve watchdog | R-02b, R-21, AC-92/93: yanlış ABI, bozuk/fazla veri, stale session, Host kill, stall/resume, yanlış thread ret; main thread IPC cevabı beklemez | Planlandı |
| D1-N5 Entity lifecycle | Bir doğrulanmış test entity türü için create/read/apply/retire, registry ve property capability matrisi | R-02c, AC-94/12: pool full, slot reuse, async stale iş, recursive destroy; en az 100 create/retire döngüsünde sahip olunan binding/ref sayacı başlangıca döner | Planlandı |
| D1-N6 Asset lease | Bir yerel stok model için request/readiness/bind/release; lease iptali ve collision readiness; ölçüm raporu | R-04a, AC-95/11/30: düşük bütçe, cancel/retire/reload sonrası late completion; en az 100 yükle/boşalt döngüsü ve main-thread maliyeti | Planlandı |
| D1-N7 Integration review | Exact build/engine/capability evidence seti, dependency upgrade/geri dönüş deneyi ve D1 kalan kapı raporu | AC-96/44; eski evidence yeni SDK/engine'e taşınmaz. R-03 OS sandbox ve R-11 GNS alt kanıtları dahil edilmeden D1/D2 hazır denmez | Planlandı |

N1'in seçilmiş build alt kümesi tamamlandı; yeni native dosya eklenirse envanter/notice ve build kanıtı genişletilir. N2'nin dosya gözlemi dört SDK adres adayını kaydetti; sıradaki N2 runtime kısmı symbol/ABI ve başlatma fazını **araştırarak** doğrular; Bu gerçek runtime önkoşulları tamamlandıktan sonra N3 lifecycle deneyi için aday profile yalnız izole geliştirme modunda izin verilir; mevcut gözlem dosyası N3'ü açmaz ve production uygunluğu verilmez. N3 geçince N4, ardından N5/N6 açılır. N7 her kesitin belge ve kanıt güncellemesine ek son gözden geçirmedir; hataları sona biriktirme aşaması değildir.

N5, ilk entity yaşam deneyi için oyunda zaten hazır olduğu doğrulanmış yerel stok model alt kümesini kullanır; bu modelin deney boyunca hazır kalma önkoşulu ve referansı izlenir. Genel asenkron model edinimi/iptali N6'dadır. Hazırlığı veya yaşamı kanıtlanamayan modelle N5 başarısı raporlanmaz. Böylece create/retire deneyi henüz yazılmamış genel asset yükleyicisini var saymaz.

N1'de MSVC/C++20 uyumu başarısızsa derleyici hata ve kullanılan dosya listesi raporlanır; dar uyarlama patch'i değerlendirilir. Kanıtsız biçimde core standardı/uyarı politikası değiştirilmez. N3'te dinamik DLL unload kanıtlanamazsa bu capability kapalı ve modül process ömrüne bağlı kalır; temiz session/process stop zorunluluğu devam eder. N6'nın stok model sonucu özel asset parser, bütün R-04 veya genel yıkım desteği değildir.

## Kaynak yerleşimi ve sahiplik

Aşağıdakiler kaynak ailelerinin hedef sınırıdır. N1'de dependency kaynakları oluştu; N2'de `src/engine/bootstrap` altında SDK bağımsız parser/profile/reader ve CLI eklendi. Kod 0.1.3'te yüklenebilir x86 bootstrap modülü eklendi ve oyun dışı host'ta sınandı; 0.1.17 gerçek ASI load ve SAEX C ABI kanıtı eklendi; GTA native binding henüz yoktur. Eksik dosyalara çalışır komut veya link verilmez. [Component map](component-map.json) bu ailelerin belge sahiplerini şimdiden kaydeder; dosya oluşturmak ilgili sahip belgelerin aynı değişiklikte güncellenmesini gerektirir.

| Planlanan aile | Sorumluluk | Ana belge sahibi |
|---|---|---|
| `src/engine/bootstrap/`, `include/saex/engine/` | SDK bağımsız profil/başlatma ve SAEX'e ait arayüzler; public başlıklarda vendor type yok | Engine adapter, native SDK sözleşmesi, bu plan |
| `src/engine/observer/` | SDK/DLL dışı owned child image gözlemi; ana thread resume edilmez, cleanup doğrulanır | Askıda process raporu, engine/native sözleşmeleri |
| `src/engine/loader/` | Ayrı x86 bounded loader mapping deneyi; açık file ID/hash pinleri, terminal olayda kill/exit | Loader raporu, engine/native ve launcher sözleşmeleri |
| `src/engine/plugin_sdk/` | Private x86 binding ve hook lifecycle uygulaması | Engine adapter, native SDK sözleşmesi, bu plan |
| `contracts/engine/` | Profil/lock/IPC için ilk kesitte tanımlanacak bounded şemalar | Native SDK sözleşmesi, execution ve bu plan |
| `tools/native/` | Kaynak edinme/doğrulama ve tekrarlanabilir build recipe; build ile oyun başlatma ayrı | Workflow, native SDK sözleşmesi, bu plan |
| `tests/engine/` | Fake-image/bounded IPC negatif testleri ve ayrı gerçek GTA harness'i | Kabul senaryoları, conformance ve bu plan |

SDK kaynak cache'i `out/dependencies/` altında yeniden edinilebilir build girdisi olarak tasarlanır; kaynak lock/recipe/patch'ler izlenen SAEX dosyalarıdır. `out/research/` inceleme kopyaları build girdisi olamaz. Test sonucu ve loglar `out/verification/engine/` altında tutulabilir; kalıcı kanıt özeti/kimlikler ilgili D1 raporuna işlenir. Engine profil kaydı oyun executable'ını veya asset binary'sini depoya eklemez.

## Her kesitin teslim kontrolü

Kod 0.1.5 metadata envanterinde noncanonical raw padding ayrı nottur; kaynak bytes/sınır kontrolleri veya C++ engine profil kapısı gevşetilmez. Statik startup sonuçları ve hata regresyonları [N2 başlangıç raporunda](d1-native-startup.md) tutulur.

1. İlgili R/AC ve normatif maddeleri belirle; var olan yerel çalışmayı koru.
2. En küçük davranışı ve hata yollarını uygula; SDK veya native erişim henüz gerekmiyorsa foundation testiyle kapsamı karıştırma.
3. Sahip belgeler, status, change-log ve gerekirse ADR'yi birlikte güncelle. Lock/profile/hook değişikliğinde kanıt kapsamını yeniden değerlendir.
4. `tools/build.ps1` ile etkilenen platform kontrolünü çalıştır; opt-in SDK gate'ini ayrıca çalıştır. Derleme GTA'yı otomatik başlatmaz veya oyun kurulumuna çıktı kopyalamaz.
5. Native deney gerekiyorsa açık deney girişinden, seçilmiş yerel GTA profili üzerinde çalıştır; artifact/fingerprint ve expected/observed sonuçlarını kaydet. Video yardımcı, log/query ve invariant sonuçları asıl kanıttır.
6. `implemented`, test edilmiş alt kapsam ve gerçek GTA sonucu status'ta ayrı yazılır; fail/unsupported veya yapılmayan deney açıklanır.

## D1 çıkışı ve sonraki ürün kapsamı

D1-N1–N6 başarıları yalnız kullanılan native alt kümeyi açar. OS sandbox (R-03), GNS kimlik/taşıma/codec (R-11a/b/c), gerçek grant revoke/time-fencing ve bounded asset deneyleri [ana D1 kapısında](../roadmap.md) ayrıca değerlendirilir. Fixture IPC'nin çalışması bu bağımsız şartları kapatmaz.

D2'de iki aktif client ve üçüncü late join ile ortak yıkım; server collision/persistence, authority ve baseline kanıtı gerekir. NPC, hava trafiği ve hayvanlar sırasıyla kendi R-17/18/19 kapılarına gider. SDK'ye yeni header eklenmesi bu ürün kollarını açmaz. Başarısız alt yetenek mevcut doğrulanmış core sonucunu silmez; ilgili native kapsam kapalı kalır ve sonraki adım hata raporuna göre seçilir.

## Kod 0.1.6 — N2 runtime mapping ara çıktısı

[src/engine/loader](../../src/engine/loader/loader_observation.cpp) yalnız x86 geliştirme observer hedefidir. Ayrı [rapor](d1-loader-observation.md), fixture DLL/TLS/main sınırı ve gerçek GTA apphelp mapping'inde beklenen ret sonucunu taşır. Önceki tabloda açık olan gerçek load sırası için artık kısmi OS mapping kaydı vardır; tam initialization ve SAEX bootstrap yüklemesi yoktur. N2'nin sonraki işi compatibility mapping kaynağı/erken yürütme ve kontrollü init ortamını incelemek, ardından unpack/symbol/ABI kanıtını üretmektir. N3–N7 sırası değişmez; eski src/engine/observer CLI'si ilk create olayını ilerletmez.

## Kod 0.1.7 — N2 reviewed policy alt çıktısı

[Loader policy kesiti](d1-loader-policy.md), contracts/engine/loader-policy.json → tools/loader_policy.py → generated C++ → PreparedLoaderPolicy → bounded loader akışını uygular. Orijinal kuruluma ait AcLayers ret kanıtı ve bit eşitliğinde özel kopyanın Debug 22/Release 21 mapping/breakpoint adayı ayrı kaydedilir. Bu ara çıktı N2'nin tam initialization/SAEX DLL load/unpack/ABI kısmını kapatmaz. Sıradaki çalışma kontrollü context/dynamic proxy başlatma ve gerçek bootstrap girişi; N3 frame/hook sırası değişmez. SDK source/recipe envanteri bu kesitte genişlemedi.

## Kod 0.1.8 — N2 launch context alt çıktısı

[Yeni kesit](d1-launch-context.md), manuel ortam varsayımını explicit cwd ve immutable environment snapshot'ına dönüştürür. Girdi/kimlik/limit retleri, gerçek fixture DLL'sinin PATH/cwd çözümü ve held child yaşamı ayrı sınanır. Kontrollü bağlam artık API girdisidir; otomatik staging, proxy initializer incelemesi ve gerçek bootstrap girişi henüz kodlanmadı. N2 initialization/entry-unpack/symbol/ABI, ardından N3–N7 sırası korunur.

0.1.8 explicit context gözleminde beklenmeyen OS debug olayının tanısı için loader çıktısına lastEventCode/lastEventThreadId eklendi. Son alınan olay metadata'sıdır; initialization kanıtı veya olayın devamına izin değildir. Child öncesi retlerde ikisi de sıfırdır. Unknown event terminal ret davranışı ve incelenmiş DLL policy değişmedi.

0.1.8 son gerçek gözlem UNLOAD_DLL_DEBUG_EVENT (7) terminal ret verdi. Bu nedenle sıradaki N2 kesit, bounded module unload/remap yaşamını ve adres/kimlik emekliliğini test ederek tanımlamalıdır; ardından proxy initialization/SAEX bootstrap ve ABI çalışması sürer. Aynı context'te başarılı breakpoint adayı görülmesi bu ret yolunu veya N2 kapısını kapatmaz.

## Kod 0.1.9 — N2 unload/remap alt çıktısı

[Yeni kesit](d1-loader-lifecycle.md) known-active unload ve yeni load kimliğini bounded ledger ile modeller; Windows observer'a bağlanır. On metadata senaryosu, OS fixture history invariant'ları ve gerçek GTA unload kanıtı ayrı tutulur. Sıradaki N2 işi dynamic proxy/initialization/gerçek SAEX bootstrap ve entry-unpack/symbol/ABI; N3 sırası değişmez.

0.1.9 son kanıtı: 12 private GTA koşusu breakpoint adayına ulaştı, 11 bilinen unload işlendi; original AcLayers ret korundu. Gerçek same-base remap görülmediğinden bu başlığın kanıtı metadata corpus ile sınırlıdır. Sonraki N2 araştırması proxy/dinamik başlangıç, gerçek bootstrap ve entry-unpack/symbol/ABI'dir. Bu sonuç N3'ü açmaz.

## Kod 0.1.10 — N2 statik linkage alt çıktısı

[Yeni araç](d1-native-linkage.md) consumer/imported-module/candidate üçlüsünün normal/delay sembollerini export listesiyle eşler. GTA'nın vorbisfile gereksinimlerinin yedisi mevcut wrapper ve ayrı vorbisHooked dosyasında bulunur; wrapper→hooked statik kenarı yoktur. Alt kanıt dinamik proxy/initializer araştırmasını daraltır, DLL değiştirme veya initialization izni üretmez. Sonraki sıra: proxy initializer/gerçek dynamic load ve fonksiyon ABI incelemesi → ayrı incelenmiş bounded initialization → gerçek SAEX bootstrap/entry-unpack/symbol kanıtı → N3. SDK/header/lock ve bootstrap ABI genişlemedi.

0.1.10 son doğrulama: x86 Debug/Release ve x64 Debug standart akışları geçti; 22 yeni testle managed toplam 79 oldu. Gerçek native dosyalarda 10 statik CLI koşusu ve bağımsız dumpbin export kontrolü başarılı, iki olmayan statik kenar beklenen incomplete sonucudur. Bu kesitte GTA process'i başlatılmadı; dinamik initializer/SAEX DLL/ABI için önceki açık sıra korunur.

## Kod 0.1.11 — N2 DLL/TLS initialization ve entry sınırı

[Yeni ayrı observer](d1-entry-boundary.md), DLL/TLS başlangıcından sonra ana thread PE girişini tutar. Eski first-exception davranışı ayrı kalır; source profile ve mapping pin recipe değişmeden compiled initialization permit yeni CLI'da açık seçilir. İlk hardware kurulum denemesi fixture main'ine geçti; kurulum CREATE_PROCESS aşamasına taşınarak düzeltildi. N2'nin sonraki gerçek safhası entry/proxy yönlendirmesi ve unpack, ardından loader lock dışında SAEX bootstrap C ABI; PE entry hit tek başına bunları kapatmaz. N3–N7 ve diğer D1 kapıları korunur.

## Kod 0.1.11 — Entry supplement bağı

Entry source ayrı entry-policy.json/generated header’a bağlıdır. İlk imm32 ret ve incelenmiş supplement sonrası gerçek Debug entry mutation ayrı gözlemlerdir. Sekiz strict generator testi ve --check eklendi; N2 unpack/SAEX DLL/ABI hâlâ açıktır. [Ayrıntı](d1-entry-boundary.md).

0.1.11 son kanıt: 12/12 private GTA koşusu giriş atlamasını yürütmeden vorbisfile.dll+0x1D60 yönlendirmesini gösterdi. Son matris 14 child exit ve 16 native dosya hash korunumu içerir. DLL/TLS başlangıcı/entry boundary alt kapsamı uygulanmış ve bu profilde ölçülmüştür; entry sonrası proxy/unpack, gerçek SAEX bootstrap ve ABI sıradadır. Standart x86 Debug/Release ve x64 Debug akışları geçti; sonuçların tam kapsamı entry raporundadır.

## GitHub kaynak yayını — taşınabilirlik düzeltmesi

Public CI ilk turda Linux bootstrap tür dönüşümü ve Windows fixture cwd metin eşitliğinde durdu. Açık uint32_t fallback ve OS dizin kimliğiyle doğrulayan fixture uygulanır; eşdeğer yol/farklı dizin/yanlış token regresyonları vardır. Bu düzeltmeler N2 unpack/bootstrap veya N3 kapısını kapatmaz; koşu sonuçları yayın raporunda kaydedilir.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.

## Kod 0.1.12 — Proxy dönüş sınırı

[0.1.12 proxy dönüş kesiti](d1-proxy-return.md) N2’de girişten sonraki tek CALL/geri dönüş yolunu sınar. Entry restorasyonu ve GetStartupInfoA IAT değişimi ayrı doğrulanır; eski entry durağının üstüne otomatik yürütme eklenmez. Sonraki iş bu IAT yönlendirmesinin çağrılma zamanı, unpack ve dinamik DLL yolu; ardından loader lock dışındaki gerçek SAEX bootstrap çağrısıdır. N2/N3 ve D1 ürün kapıları açık kalır.

## Kod 0.1.13 — Startup çağrı sınırı

[0.1.13 startup-call](d1-startup-call.md), N2’nin entry sonrası proxy çağrı zamanını ölçer. Mevcut proxy-return ikinci durakta bitmeye devam eder; yeni mod üçüncü hitte wrapper gövdesini çalıştırmadan kapatır. Sonraki iş wrapper’ın dinamik codec/ASI yükleme yolu ve loader lock dışında gerçek SAEX bootstrap çağrısıdır. Dört anchor örneği tüm unpack/motor durumunu kanıtlamaz; N2 ve N3–N7 açık kalır.

## Kod 0.1.14 — Codec dönüş kesiti

N2 startup alt kanıtını izleyen codec-return API/CLI ve own DLL zinciri eklendi. Yeni pinler sadece yeni mod içindir; tam LoadPlugins dönüşü, SAEX bootstrap, WinMain/frame ve N3–N7 kapıları açık kalır. [Sözleşme ve doğrulama](d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

N2 codec binding alt adımı, sekiz pointer adresini ve ilk 16 target byte kararlılığını beşinci durakta ölçer. N1 ABI/SDK lock değişmedi; ASI/SAEX bootstrap ve N3–N7 açık. [Sözleşme ve kanıt](d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

0.1.16 N2 ilerlemesi: gerçek SAEX DLL ASI load/return 12/12; x86/x64 Debug/Release standart kontrolleri geçti. Sonraki uygulanacak kesit loader lock dışında güvenilir invoker ve Initialize/Query/Stop kanıtıdır. Wrapper’ın ASI LoadLibrary döngüsü kendi başına bu export’ları çağırmaz; N2 kapanmadı.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

N2 frame adayının kaynak/adres ilişkisi ve üç yükleme durağındaki baytları için yeni kesit eklendi. Doğal game-loop çağrısı, ABI/thread ve faz sıklığı kanıtı eksik; N3 frame hook kapısı hâlâ açık. [Aday ve doğrulama raporu](d1-frame-target.md).

## Kod 0.1.19 bağlantısı

0.1.19, aynı profil/pin ve owned child kapıları üzerinde ayrı doğal startup-return deneyi ekler. Mevcut durak davranışı korunur; koruma çağrısı ve dönüş ABI kanıtı yeni raporda izlenir. D1/N2/N3 ve oynanabilir multiplayer kapıları açık kalır. [Sözleşme ve kanıt](d1-startup-return.md).

## Kod 0.1.19 — Bağımsız image koruma deneyi

N2 startup dönüşündeki koruma uyuşmazlığını bağımsız ölçmek için çalıştırılmayan fixture image deneyi eklendi. Bu araç GTA çağrısı/frame önkoşullarını ilerletmez; 17 taşınabilir negatif test ve ayrı Windows ölçümü vardır. [Sözleşme ve kanıt](d1-image-protection.md).

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.

## Kod 0.1.24 bağlantısı

Ayrı instance-startup modu platform bastırma dönüşünden gerçek named event oluşturma/açma ve doğal helper dönüşüne ilerler. Mevcut event veya NULL handle durumunda pencere kolundan önce ret verilir. Önceki suppression modu restore ederek bitmeye devam eder; yeni mod doğal API sonrası eski CALL bağlamını geri yazmaz. [Sözleşme ve doğrulama](d1-instance-startup.md). Oturumdaki ortak event ömrü process-private değildir; observer sinyal durumunu değiştirmez. C ABI 1/GNS/otorite aynı; pencere/renderer/doğal frame ve N2/N3/D1/D2 kapıları açıktır.

## Kod 0.1.25 bağlantısı

Ayrı event-dispatch modu, instance dönüşünden doğal olay dağıtıcısı CALL/entry ve uygulama işleyicisi CALL önüne ilerler. Üç durakta argüman, dönüş adresi, register ve yaşayan caller stack doğrulanır; uygulama işleyicisi çalıştırılmaz. [Sözleşme ve sonuç](d1-event-dispatch.md). Eski instance terminali, C ABI 1/GNS/otorite aynı; yeni bağımlılık/kalıcı migration yoktur. AppEventHandler gövdesindeki executable yönlendirmesi, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.26 bağlantısı

`--observe-application-routing` önceki event-dispatch kanıtından sonra yalnız rsINITIALIZE=24 rotasını yürütür: işleyici entry → executable detour → indirect JMP → ilk oyun initializer CALL öncesi. 39 index/11 hedef tablosu, rel32/absolute operand ve dört yığın/register sınırı doğrulanır. [Sözleşme ve sonuç](d1-application-routing.md). Önceki mod kendi AppEventHandler CALL öncesi terminalini korur. Yeni modda `eventDispatchObservation.applicationHandlerCallAllowed=true`, routing nesnesinde initializer çağrı izni false olur; önceki stage/verified ara kanıtı korunur. C++ trace/API yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni bağımlılık/kaldırılan özellik/kalıcı migration yoktur. Oyun initializer gövdesi, RsInitialize, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.27 bağlantısı

`--observe-game-prelude` ilk oyun initializer içine girer; exact boş Init ve üç yerelleştirme bayrağını yazan iki helper doğal olarak geri döner. Beş durak, stack/register/flags, yaşayan caller ve 16-byte veri penceresi denetlenir; yalnız üç veri byte değişebilir. CFileMgr CALL çalıştırılmaz. [Sözleşme ve sonuç](d1-game-prelude.md). Önceki application-routing terminali korunur; yeni üst modda routing nesnesinin initializerCallAllowed alanı true, prelude nesnesinin fileManagerCallAllowed ve initializerReturnVerified alanları false olur. C++ observer yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni dependency/kaldırma/kalıcı migration yoktur. CFileMgr, streaming/pad, initializer dönüşü, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.28 bağlantısı

`--observe-file-manager-entry` CFileMgr içine doğal CALL ve ilk üç PUSH komutunu açar; 0x5386FB CRT cwd CALL önünde durur. İki durakta buffer/maxlen=128 ABI, nested return stack, register/flags, 136-byte root/guard ve localisation korunumu denetlenir. [Sözleşme ve sonuç](d1-file-manager-entry.md). Önceki prelude terminali korunur; yeni üst modda prelude fileManagerCallAllowed=true, manager cwdCallAllowed=false/fileManagerReturnVerified=false olur. CRT lock/SEH/OS/copy yolu henüz açılmaz. Gelecekte suffix yazımından önce NUL en geç buffer offset 126, başarılı dönüş ve ANSI byte uzunluğu kanıtı gerekir. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Manager/initializer dönüşü, streaming/pad, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.29 bağlantısı

`--observe-cwd-seh` CRT wrapper ve SEH prologue içine doğal CALL açar; kayıt kurulup yardımcı döndüğünde 0x836E9D noktasında durur. Üç durakta 80-byte stack, 28-byte NT_TIB, önceki kayıt ve caller/buffer/localisation korunumu denetlenir. [Sözleşme ve sonuç](d1-cwd-seh.md). Önceki manager terminali korunur; yeni üst modda manager cwdCallAllowed=true, cwdSeh lockPathAllowed/directoryApiAllowed/cwdReturnVerified/unwindVerified=false olur. Handler veya kilit/OS/copy yolu açılmaz; owned child sonunda kapatılır, eski TEB/context rollback yapılmaz. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Kilit/cwd, SEH sökümü, manager/initializer dönüşü, renderer/doğal frame ve N2/N3/D1/D2 açıktır.

## Kod 0.1.30 bağlantısı

`--observe-cwd-lock` lock(7) selector CALL ve ilk 17-byte gövdeyi doğal yürütür; CMP tamamlandığında 0x82ADCF JNE önünde durur. [Sözleşme ve sonuç](d1-cwd-lock.md). 100-byte stack, 16-byte slot penceresi, NT_TIB/önceki kayıt/caller/buffer korunur; slot değeri dereference edilmez. SlotPresent yalnız sıfırdan farklı word demektir, kritik bölüm veya kilit alma kanıtı değildir. Üst modda cwdSeh.lockPathAllowed=true yalnız selector iznidir; yeni branchAllowed/lazyInitializationAllowed/criticalSectionCallAllowed/lockAcquiredVerified=false. Önceki SEH terminali korunur; DR0 dışında yeni observer müdahalesi ve TEB/context rollback yoktur. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Mevcut/lazy dal, OS kilidi, cwd/SEH dönüşü ve N2/N3/D1/D2 açıktır.

## Kod 0.1.31 bağlantısı

`--observe-cwd-acquire` mevcut/unowned lock(7) nesnesi için doğal dal, admitted ntdll API entry/return ve CRT selector dönüşünü açar; 0x836EA4 terminalinde durur. [Sözleşme ve sonuç](d1-cwd-acquire.md). Beş durakta object/slot/84-byte caller/SEH korunumu ve API sonrası thread sahipliği doğrulanır. x86 24-byte kritik bölüm düzeni pinned Windows uygulamasına aittir; VOID dönüşte EAX başarı kodu sayılmaz. Heap veya aynı GTA image nesnesi için sınır/koruma denetimi vardır. Üst modda branch/criticalSectionCallAllowed=true, acquired readback ile ayrıdır; lazy/directory/unlock kapalı kalır. Önceki lock terminali korunur; kilit tutulurken bütün owned child kapatılır, observer veri/TEB/context rollback yapmaz. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Cwd/SEH/manager dönüşü ve N2/N3/D1/D2 açıktır.
