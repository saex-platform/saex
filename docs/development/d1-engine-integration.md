# D1 motor entegrasyonu uygulama planı

Tarih: 13 Eylül 2026. Mimari v0.16 / kod 0.1.9. Durum: **D1-N1 build ve N2 dosya/image/bootstrap/ilk-create/statik envanter uygulandı; sınırlı loader mapping gözlemi eklendi; unpack/symbol/ABI/tam initialization ve N3–N7 açık**. [N1 kod/kanıt raporu](d1-native-dependency.md). [Normatif SDK sözleşmesi](../architecture/native-sdk-integration.md) · [Kaynak incelemesi](../references/plugin-sdk-sa.md) · [Durum](status.md) · [Yol haritası](../roadmap.md)

## Başlangıç ve hedef

Mevcut foundation; Windows x86/x64 core, C# araçları ve yerel process fixture'ıdır. GTA engine profili ve hook'lar doğrulanmadı. Dryxio kaynak pini `b55e89b336a81448c1aa1a5b188431c9845ebaa9` seçildi; private SDK library, x86 build probe'u ve SDK bağımsız başlangıç DLL'si vardır; gameplay adapter/launcher yoktur. İlk hedef, tek gerçek GTA process'inde doğrulanmış başlatma, frame gözlemi, bounded komut uygulama ve temiz kapanış kanıtıdır.

Bu plan uygulayıcıya küçük, derlenen ve negatif testi bulunan kesitler verir. Kesit adları proje planı kimlikleridir; N1'in gerçek target/komutları [uygulama raporunda](d1-native-dependency.md) eşlenir. Süre tahmini verilmez; her kesitin tamamlanması çıktı ve kapıyla ölçülür.

## Uygulama sırası

| Kesit / sorumlu alan | Somut teslim | Geçiş ve negatif kanıt | Durum |
|---|---|---|---|
| D1-N1 Dependency/build | Exact commit/patch/source/recipe lock, dosya ve notice envanteri, SDK'yi private tüketen x86 compile/link probe; core build'inden ayrı opt-in giriş | R-02a, AC-89: x64 link/include sızıntısı, yanlış/missing source digest ve header/library karışımı ret; C++20 başarısız deneyi, private C++23 kararı, CRT/packing kayıtlı | Seçilmiş 23 dosyalı build kapsamı uygulandı; [kanıt](d1-native-dependency.md) |
| D1-N2 Engine profile | Salt okunur inspector genişletmesi; gerçek binary/symbol/hook evidence; SDK bağımsız bootstrap ve mapped-image kontrolü | R-01a/b, AC-90: unknown/truncated/wrong arch, benzer sürüm marker'ına rağmen yanlış hash, runtime hook uyuşmazlığı ret; ret öncesi SDK çağrısı/hook yazımı sıfır | [Dosya/image](d1-engine-preflight.md), [oyun dışı DLL](d1-bootstrap-module.md), [askıda süreç](d1-suspended-process.md), [statik native başlangıç](d1-native-startup.md) ve [sınırlı loader mapping](d1-loader-observation.md) kapsamları ayrı; unpack/symbol/ABI/tam initialization açık |
| D1-N3 Lifecycle observer | Bir init/frame/stop gözlemi; hook ownership registry, kısmi kurulum geri alma ve callback drain; yerel teşhis | R-01c, AC-91/15: normal/başarısız başlatma ve stop; çakışan hook ve callback içindeyken stop. Her seçilen profil için en az 20 temiz process başlat/kapat koşusu hedefi | Planlandı |
| D1-N4 Host/frame bridge | Gerçek x86 adapter ↔ x64 Host production IPC ilk alt kümesi; no-op/read-only komut, session/epoch/sequence, kuyruk ve watchdog | R-02b, R-21, AC-92/93: yanlış ABI, bozuk/fazla veri, stale session, Host kill, stall/resume, yanlış thread ret; main thread IPC cevabı beklemez | Planlandı |
| D1-N5 Entity lifecycle | Bir doğrulanmış test entity türü için create/read/apply/retire, registry ve property capability matrisi | R-02c, AC-94/12: pool full, slot reuse, async stale iş, recursive destroy; en az 100 create/retire döngüsünde sahip olunan binding/ref sayacı başlangıca döner | Planlandı |
| D1-N6 Asset lease | Bir yerel stok model için request/readiness/bind/release; lease iptali ve collision readiness; ölçüm raporu | R-04a, AC-95/11/30: düşük bütçe, cancel/retire/reload sonrası late completion; en az 100 yükle/boşalt döngüsü ve main-thread maliyeti | Planlandı |
| D1-N7 Integration review | Exact build/engine/capability evidence seti, dependency upgrade/geri dönüş deneyi ve D1 kalan kapı raporu | AC-96/44; eski evidence yeni SDK/engine'e taşınmaz. R-03 OS sandbox ve R-11 GNS alt kanıtları dahil edilmeden D1/D2 hazır denmez | Planlandı |

N1'in seçilmiş build alt kümesi tamamlandı; yeni native dosya eklenirse envanter/notice ve build kanıtı genişletilir. N2'nin dosya gözlemi dört SDK adres adayını kaydetti; sıradaki N2 runtime kısmı symbol/ABI ve başlatma fazını **araştırarak** doğrular; Bu gerçek runtime önkoşulları tamamlandıktan sonra N3 lifecycle deneyi için aday profile yalnız izole geliştirme modunda izin verilir; mevcut gözlem dosyası N3'ü açmaz ve production uygunluğu verilmez. N3 geçince N4, ardından N5/N6 açılır. N7 her kesitin belge ve kanıt güncellemesine ek son gözden geçirmedir; hataları sona biriktirme aşaması değildir.

N5, ilk entity yaşam deneyi için oyunda zaten hazır olduğu doğrulanmış yerel stok model alt kümesini kullanır; bu modelin deney boyunca hazır kalma önkoşulu ve referansı izlenir. Genel asenkron model edinimi/iptali N6'dadır. Hazırlığı veya yaşamı kanıtlanamayan modelle N5 başarısı raporlanmaz. Böylece create/retire deneyi henüz yazılmamış genel asset yükleyicisini var saymaz.

N1'de MSVC/C++20 uyumu başarısızsa derleyici hata ve kullanılan dosya listesi raporlanır; dar uyarlama patch'i değerlendirilir. Kanıtsız biçimde core standardı/uyarı politikası değiştirilmez. N3'te dinamik DLL unload kanıtlanamazsa bu capability kapalı ve modül process ömrüne bağlı kalır; temiz session/process stop zorunluluğu devam eder. N6'nın stok model sonucu özel asset parser, bütün R-04 veya genel yıkım desteği değildir.

## Kaynak yerleşimi ve sahiplik

Aşağıdakiler kaynak ailelerinin hedef sınırıdır. N1'de dependency kaynakları oluştu; N2'de `src/engine/bootstrap` altında SDK bağımsız parser/profile/reader ve CLI eklendi. Kod 0.1.3'te yüklenebilir x86 bootstrap modülü eklendi ve oyun dışı host'ta sınandı; gerçek GTA binding/loader henüz yoktur. Eksik dosyalara çalışır komut veya link verilmez. [Component map](component-map.json) bu ailelerin belge sahiplerini şimdiden kaydeder; dosya oluşturmak ilgili sahip belgelerin aynı değişiklikte güncellenmesini gerektirir.

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
