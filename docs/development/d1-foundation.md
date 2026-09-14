# D1 foundation: uygulanan sözleşmeler ve kanıt

Tarih: 12 Eylül 2026. Bu belge ilk foundation'ın mimari v0.6/kod 0.1.0 kanıtını korur. Kod 0.1.1'in bağımsız [native dependency kesiti](d1-native-dependency.md) 20 ek test tanımı ve ADR-36 içerir; aşağıdaki 46 foundation testi kendi kapsamıdır. [Durum](status.md) · [Derleme](workflow.md) · [Değişiklik kaydı](change-log.md)

Bu belge v0.6 foundation uygulama/kanıt kapsamını korur. Tarihsel v0.7 aşamasında [Plugin-SDK kaynak incelemesi](../references/plugin-sdk-sa.md) ve [D1-N1–N7 motor planı](d1-engine-integration.md) eklendi; SDK henüz linklenmedi, yeni AC-89–96 çalıştırılmadı. Aşağıdaki native core testleri GTA native adapter testi değildir. Yeni engine kaynakları ortak test/contract ailelerini etkilerse bu kanıt sınırı ve ilgili kaynak haritası aynı değişiklikte güncellenir.

## Sorumluluk ve kaynak haritası

| Yüzey | Gerçek kaynak | Sorumluluk |
|---|---|---|
| Ortak tanım | [foundation.schema.json](../../contracts/foundation.schema.json), [ContractGen](../../tools/ContractGen/Program.cs) | Non-interchangeable kimlik tipleri ve fixture sabitleri üretimi |
| C++ kimlik/codec | [StrongId](../../include/saex/contracts/strong_id.hpp), [codec](../../src/contracts/fixture_codec.cpp) | Checked increment, explicit little-endian ve bounded frame decode |
| Süre/lease | [ControlClock](../../src/core/control_clock.cpp), [LeaseAuthority](../../src/core/lease_authority.cpp) | Oyun tick'inden bağımsız süre ve stale/replay/permission ret |
| Ingress | [BoundedInbox](../../src/core/bounded_inbox.cpp) | Mutex korumalı item/byte admission, close sonrası drain |
| Araçlar | [WorldPlanValidator](../../managed/Saex.Tools/WorldPlanValidator.cs), [EngineInspector](../../managed/Saex.Tools/EngineInspector.cs) | Offline metadata çözümleme ve yerel binary gözlemi |
| Native fixture | [contract_probe](../../src/tools/contract_probe.cpp), [managed codec](../../managed/Saex.Contracts/FixtureCodec.cs) | Gerçek iki süreçte aynı EntityRef'i taşıma |
| Kabul alt testleri | [Native](../../tests/native/core_tests.cpp), [managed](../../tests/managed/Saex.Foundation.Tests/Program.cs), [doc gate](../../tests/tooling/test_doc_gate.py) | Alt invariant testleri; native GTA desteği kanıtı değil |

## C++ çekirdek davranışı

StrongId'nin farklı Tag tipleri WorldEpoch/OwnerEpoch gibi değerlerin yanlışlıkla karışmasını derleme zamanında engeller. EntityRef dört nonzero uint64 alanıyla temsil edilir; tam temsil geçerli olması entity'nin bir dünyada kayıtlı/yetkili olduğunu kanıtlamaz. `next` uint64 sınırında boş sonuç döndürür; sıfıra wrap yapmaz. Bu genişlik foundation tipidir, tüm oyun protokolünün wire ABI kararı değildir.

ControlClock monoton yerel süreyi ClockDomainId ile microsecond olarak verir; SteadyControlClock std::chrono::steady_clock kullanır. Bootstrap domain kimliğini güvenilen process seçer; global/kriptografik kimlik sağlayıcısı henüz uygulanmadı. Native fixture clock'u keyfî ilerletebilir; production clock override API'si yoktur.

LeaseAuthority tek authoritative thread için entity başına gate'tir. Core issuer geçerli session/catalog/state/alan maskesi ve participant Applied önkoşulunu sağlar. 2.000.000 µs fixed lease ve son geçerlilik anında ret vardır. Renew sequence tekrarları süre uzatmaz; expired/revoked grant yenilenemez. Entity/generation/session/OwnerEpoch/revision/alan maskesi ve motion sequence doğrulanır. Yeni owner epoch artar. Saatin gerilemesi/domain değişmesi veya explicit suspend/resume fence eder; trusted recovery/readiness sonrasında yeni grant gerekir. Gerçek OS resume bildirimi, GTA watchdog, connection heartbeat, collision/hit/hız doğrulaması ve scheduler henüz yoktur. Bool readiness parametresi network client beyanından alınacak bir yetki kanıtı değildir.

BoundedInbox payload'ı item/byte sınırını kontrol ettikten sonra kendi belleğine kopyalar. ResourceEpoch zero reddi, close sonrası admission ret ve admitted işleri drain bulunur. Bu yalnız ingress primitive'idir; gerçek resource registry/permission/revoke kontrolü, process toplam bütçe, persistent CommitReceipt kuyruğu veya tam scheduler değildir. Charged bytes payload + sabit command yüküdür; allocator/deque overhead ve RSS ayrıca ölçülür. Durable committed sonuç burada kaybolabilir bir transient komut gibi kullanılmaz.

## Yerel conformance frame'i

ADR-34: tek istek/yanıt, stdin/stdout üzerinden process başına bir frame. Üretim worker IPC'si ve oyun ağı değildir. GNS/HTTPS seçimi değişmedi. Saex.Contracts yalnız DTO/fixture codec içerir; gerçek Saex.Client/Server resource SDK'sı değildir.

| Offset | Alan | Encoding |
|---|---|---|
| 0 | magic | uint32 LE, 0x58454153, ASCII SAEX |
| 4 | fixture version | uint16 LE, 1 |
| 6 | reserved | uint16 LE, 0 zorunlu |
| 8 | payload bytes | uint32 LE, tam 32 |
| 12 / 20 / 28 / 36 | WorldId / WorldEpoch / EntityId / Generation | Dört uint64 LE, nonzero |

Toplam 44 byte. Header doğru olmadan body kabul edilmez; bütün frame stack üzerinde sabit boyutludur. Kısa, uzun, trailing byte, wrong version, reserved bit ve zero identity reddedilir; ret stdout'ta entity üretmez. Native process tek frame sonrası stdin EOF bekler; test supervisor 5 saniye timeout ile kendi başlattığı process'i sonlandırabilir. Bu tasarım uzun yaşayan IPC'de blocking read kabul edildiği anlamına gelmez. Cross-process payload raw pointer/CLR object taşımaz.

## WorldPlan metadata alt kümesi

CLI girdi dosyası en çok 1 MiB, JSON depth 32, node sayısı 16.384; duplicate property, unknown field, missing required field ve null definition reddi vardır. En çok 64 resource, 256 asset, 128 capability adı. Bounded lower-case kimlik ve 64 karakter SHA-256 **biçimi** doğrulanır; artifact dosyaları mevcut/aynı hash diye doğrulanmaz.

Resource exact-lock metadata'sı dependency ve required service provider kenarlarından kararlı prepare sırası üretir. Unique provider ve tek field mutator zorunludur; missing ve cycle hataları açıklanır. Asset closure'da critical gereksinim transitif yayılır; dolaylı required collision optional olamaz. Declared required/available capability ve RecoveryProfile başlangıç tavanları kontrol edilir. Sınırlı JSON'un geçmesi native capability kanıtı değildir.

`sourceDigest` tam girdi byte'larının SHA-256'sıdır; canonical WorldPlanDigest değildir. Çıktı `valid=true` olsa bile `productionEligible=false` kalır. [Örnek](../../samples/foundation/world-plan.json) sentetik digest'li metadata fixture'ıdır; sample.harbor/saex.foundation adlı gerçek resource paketi veya crate asset'i yoktur. SemVer çözümleme, bütün ContractSchema, component schedule, trait/prefab flattening, content cook ve runtime activate sonraki R-13/R-14 işleridir.

## Gerçek GTA dosyasının gözlemi

Kullanıcının verdiği kurulumdaki gta_sa.exe salt okunur incelendi: **14.383.616 byte**, `I386`, native PE32 executable. SHA-256: `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`. `recognizedProfile=false`, `canAttach=false`, neden `unverified_engine_profile`.

Dosya adına/boyutuna dayanarak “1.0 US doğrulandı” denmedi. Hook adresi, executable davranışı, platform uyumluluğu, değiştirilmemiş oyun veya anti-cheat kanıtı çıkarılmadı. Orijinal dosyaya yazılmadı ve GTA başlatılmadı. R-01 profil/hook/attach/unload deneyleri açık kalır. PE parsing resmi .NET PEReader ile, hash SHA256 ile yapılır; kendi crypto algoritması yazılmadı.

## Doğrulama kapsamı

Windows x64 Debug/Release ve x86 Debug build'leri MSVC 19.44 ile warnings-as-errors altında derlendi. Her konfigürasyonda 16 native test ve 25 managed/entegrasyon testi geçti. x64 C# runtime hem x64 hem x86 native fixture executable'ıyla konuştu. Native corpus 10.000 sabit seed'li bozuk giriş ve her geçerli frame prefix'ini içerir; bu kapsamlı fuzz/security denetimi değildir. Tooling testleri ayrı 5 senaryodur. Toplam 46 farklı test tanımı vardır; farklı mimarilerde tekrar çalıştırılmaları yeni AC sayısı olarak toplanmaz.

| AC / R bağı | Bu kesitte sınanan alt davranış | Geçilmiş sayılmayan kısım |
|---|---|---|
| AC-34 / R-13 | Provider/mutator, missing/cycle, critical optional, metadata budget ret | Tam compiler/engine production eligibility |
| AC-35 / R-11c/13 | Kimlik üretimi, bounded fixture version/length codec ve x86/x64 süreç farkı | Public/private bütün schema, GNS codec veya sandbox |
| AC-45/83 / R-14 | Concurrent ingress item/byte cap, close/drain | Scheduler fairness, process/global budget ve actual load |
| AC-71/72 / R-21 | Frozen simulation, monotonic expiry, domain/regression, explicit resume fence | Gerçek OS suspend, GTA controller/watchdog loss |
| AC-86 / R-21 | Checked uint64 artışı ve deadline taşması | World epoch dönüşü, durable shutdown/restore |
| R-01 | Gerçek dosyanın PE/hash gözlemi ve unknown profile ret | Verified GTA sürümü, hook/attach/unload |

Bu kodda sunucu dinleme portu, oyun launcher'ı, C# resource sandbox'ı, persistence transaction, population, hayvan controller'ı veya multiplayer simülasyonu yoktur. D1 tamamlanmadı; alt testler başarıyla kapanan küçük temeldir.

## Kod 0.1.2 ile gelen araç farkı

[N2 preflight](d1-engine-preflight.md) SDK bağımsız PE/profile/image alt kümesini ekler. Yukarıdaki ilk 46 tanımlı foundation kaydı tarihseldir; güncel managed runner 26 testtir. EngineInspector aynı gömülü gözlem profilini kullanır, `fileProfileMatched/observedProfileId/profileSourceDigest` alanlarını ekler; eski neden adı bilinen dosyada `observed_profile_runtime_unverified` olur. `recognizedProfile/canAttach` false kalır. Native parser C# raporundan izin devralmaz; kendi dosya/image kontrolünü çalıştırır. Bu fark source/CLI sözleşmesidir, çalışan GTA veya production IPC kanıtı değildir.

## Kod 0.1.3 bootstrap modül farkı

Kod 0.1.4 ayrıca [askıda process observer testini](d1-suspended-process.md) Windows native akışına ekler. Kendi canary executable'ımızın lifecycle testi ve 5 process CLI ret testi, foundation/GTA multiplayer kanıtı olarak sayılmaz. Mevcut 26 managed test, fixture ABI ve core davranışı değişmedi.

[Başlangıç DLL'si](d1-bootstrap-module.md), portable bootstrap session testini ortak native build'e ekler; x86 build ayrıca loadable DLL, oyun dışı host ve artifact audit üretir. Mevcut managed 26 test/fixture ABI'si değişmedi. Yeni 192 byte C ABI yalnız güvenilir aynı-process native çağıran içindir; foundation'ın 44 byte process fixture'ı veya production worker IPC'si değildir. DLL kendi test host'unu reddeder; GTA runtime desteği bundan çıkmaz.

## Kod 0.1.5 native başlangıç metadata'sı

Startup testlerine iki CLI'nin InvalidDataException için JSON/exit 1 davranışı ve noncanonical raw padding notunun loader izni vermemesi eklendi. Bütçe testleri bozuk dosyaları da tüketilen toplam boyuta dahil eder; normal temel sözleşmeler değişmedi.

[StartupTests](../../tests/managed/Saex.Foundation.Tests/StartupTests.cs), mevcut managed runner'a EXE/DLL import/delay/TLS, kota ve yerel aday grafiği testlerini ekler. Önceki 26 managed tanım korunur; 31 yeni tanımla toplam 57 test x64 Debug/x86 Release akışında geçti. Kapsam ve artifact'ler [raporda](d1-native-startup.md) kayıtlıdır. Bu statik C# araç çalışması C++20 core, fixture codec veya production launcher/IPC davranışı değiştirmez.

## Kod 0.1.6 ek doğrulama kapsamı

[Yeni native loader suite](d1-loader-observation.md) x86'ya özeldir: ayrı EXE/DLL/TLS canary, hold/deny/kimlik/bütçe/owner-thread/terminal cleanup ve 12 warm çevrim. Altı Python CLI testi unknown girdiyle child oluşmadığını doğrular. 57 managed testin kapsamı değişmedi. Fixtures yalnız bu deneyde static CRT ile derlenir; core ve production bootstrap CRT/ABI'si korunur. Bu sonuçlar GNS, OS worker sandbox veya initialized GTA kanıtı değildir.

## Kod 0.1.7 ek kapsam

[Loader policy](d1-loader-policy.md) C++20 native pin hazırlığı, strict JSON→C++ generator ve engine/context ayrımını ekler. Mevcut native loader suite'i exact hazırlama ve engine/hash/boyut/origin/partial/budget retleriyle genişledi; sekiz Python generator testi ve yedinci loader CLI testi eklendi. Managed 57 test ve foundation codec/clock/lease davranışı değişmez. Gerçek GTA ile eksik/tek byte değişmiş yerel kopya DLL deneyi ayrıca child oluşmadan ret verdi. Standart build oyun kurulumunu kullanmaz.

## Kod 0.1.8 — Observer context regresyonu

[Context testleri](d1-launch-context.md), Windows native suite ve x86 loader CLI corpus'una eklendi. Core lease/clock/inbox, 44-byte fixture codec ve 57 managed testin davranışları değişmez. Native suite sayısı ile senaryo sayısı farklı ölçümlerdir; yeni testler production IPC, worker sandbox veya GNS kanıtı sayılmaz.

0.1.8 x86 loader corpus'u terminal debug event kodu/thread kimliğini de sınar; child açılmayan CLI retlerinde bu alanlar sıfırdır. Ortak codec/managed davranışı değişmedi.

## Kod 0.1.9 — Portable mapping ledger corpus'u

[Yeni on testli native suite](d1-loader-lifecycle.md) C++20 metadata ledger'ını Windows x86/x64 build akışına ekler; Windows observer yalnız x86'da bu ledger ile UNLOAD işler. Core EntityRef/codec/lease, 57 managed test ve worker/GNS sınırları değişmez. Portable kaynak varlığı Linux test kanıtı değildir.

## Kod 0.1.10 — CLI ve managed test eki

[engine linkage](d1-native-linkage.md) yeni beş argümanlı CLI'dir; existing plan/engine inspect/startup komutlarını korur. 22 yeni LinkageTests kaydı standart managed runner'a kayıtlıdır; fixture'lar yalnız kendi geçici PE dosyalarını okur ve temizler. İlk hedefli Debug sonuç 79/79 managed/entegrasyon testidir; foundation, statik PE ve gerçek runtime kanıtları birleştirilmez. Oyun protokolü, public C# SDK veya process sandbox bu araçla uygulanmış sayılmaz.

0.1.10 son standart sonuç: x86 Debug/Release ve x64 Debug akışlarında 79/79 managed/entegrasyon testi geçti; yeni 22 kayıt bu sayının içindedir. Önceki foundation sonuçları tarihsel kapsamını korur; native suite/Python/platform ve gerçek dosya karşılaştırmalarının ayrıntısı [güncel rapordadır](d1-native-linkage.md).

## Kod 0.1.11 — Native test runner kapsamı

[Entry boundary corpus](d1-entry-boundary.md) ayrı x86 suite'tir; 12 senaryo ve 12 warm çevrim, iki ek CLI negatif testi getirir. C# managed toplamı 79 olarak kalır. Native C++ entry debug/OS fixture sonucu, public SDK/IPC/sandbox veya multiplayer temeli olarak sayılmaz; fixture başarısı ve gerçek GTA kanıtı ayrı raporlanır.

## Kod 0.1.11 — Entry supplement bağı

Sekiz entry-policy generator testi bütün standart akışlara eklendi. Yeni source dünya planı/ContractSchema/IPC protokolü değildir; managed toplamı 79 olarak kalır. [Ayrıntı](d1-entry-boundary.md).

## GitHub yayın tooling kesiti

13 Eylül 2026: [yayın iş akışı](github-publication.md), sekiz event/base SHA testini standart build'e ekler. İlk push, PR base, bozuk/eksik SHA ve privileged event reddi test edilir. Linux için portable core/managed fixture girişi eklendi. Bu tooling testleri mevcut core/ABI invariant sayısını veya GTA kabul kapsamını değiştirmez; yerel/GitHub sonuçları yayın raporunda ayrılır.

## GitHub kaynak yayını — taşınabilirlik düzeltmesi

GitHub taşınabilirlik düzeltmesi Windows native.launch_context suite içine alias/farklı dizin/yanlış token regresyonları ekler; native suite ve managed test toplamları değişmez. Portable bootstrap reason fallback açık uint32_t dönüşümüdür. Yerel yeniden doğrulama ve hosted CI sonucu ayrı kayıtlanır.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.

## Kod 0.1.12 — Proxy dönüş sınırı

[0.1.12](d1-proxy-return.md), x86 observer için iki-hit corpus ve yedi portable proxy-policy testi ekler. Ortak ContractGen/EntityRef/ControlClock/LeaseAuthority/BoundedInbox ve C# plan/linkage davranışı değişmez. Standart build GTA kurulumu gerektirmez; gerçek GTA koşusu ayrı opt-in deneydir. Foundation fixture başarısı server/client veya tam D1 hazır oluşu değildir.

## Kod 0.1.13 — Startup çağrı sınırı

[0.1.13](d1-startup-call.md) x86 native.startup_observation ve yedi portable policy testi ekler; C# fixture runner/çekirdek otorite davranışı değişmedi. Test helper taşınması production PE parser’ını genişletmez. Standart build kendi CRT/test programlarını çalıştırır, GTA gerektirmez. İlk özel CRT entry denemesi link hatası verdi; normal CRT entry ile düzeltildi ve başarısız kayıt saklandı.

## Kod 0.1.14 — Codec dönüş kesiti

Foundation çekirdeği/ortak şema aynı kalırken N2 observer public C++ trace/API genişledi; local çağıranlar birlikte yeniden derlenir. Yeni native codec corpus ve Python recipe retleri standart build akışına eklendi. [Sözleşme ve doğrulama](d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

Foundation ortak şema/otorite aynı kalır. Local C++ observer trace/API genişledi; birlikte derleme gerekir. Native binding corpus ve policy/CLI ret testleri standart akışa eklendi. [Sözleşme ve kanıt](d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.18 bağlantısı

Portable frame-target sözleşmesi ve negatif testleri foundation build kapsamına eklendi. Bu doğrulayıcı GNS/server simulation veya oyun içinde frame scheduler uygulaması değildir. [Aday ve doğrulama raporu](d1-frame-target.md).

## Kod 0.1.19 bağlantısı

0.1.19, aynı profil/pin ve owned child kapıları üzerinde ayrı doğal startup-return deneyi ekler. Mevcut durak davranışı korunur; koruma çağrısı ve dönüş ABI kanıtı yeni raporda izlenir. D1/N2/N3 ve oynanabilir multiplayer kapıları açık kalır. [Sözleşme ve kanıt](d1-startup-return.md).

## Kod 0.1.19 — Bağımsız image koruma deneyi

Standart build, image-protection deneyinin 17 taşınabilir testini de çalıştırır. Bunlar PE/region/girdi/kapanış sözleşmesini sınar; gerçek Windows eşleme deneyi açık CLI ile ayrı yürütülür. Foundation veya production IPC kapsamı genişlemez. [Sözleşme ve kanıt](d1-image-protection.md).

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Yeni uygulama giriş API/CLI ve fixture/policy/portable testleri, mevcut preflight ve owned child kapılarından sonra ayrı izinle çalışır. Eski terminal sınırlar ve C ABI 1 korunur; yeni sonuçlar [uygulama giriş raporunda](d1-application-entry.md) izlenir. N2/N3 ve D1/D2 açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.
