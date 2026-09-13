# Araştırma kayıtları ve geçiş koşulları

Durum: bütün ana kayıtlar açık. D1 foundation'ın çalıştırılmış alt testleri aşağıda ayrıca belirtilir; yeni native SDK deneyleri planlandı. [Yol haritası](../roadmap.md)

## Kanıt biçimi

Her araştırma gerçek build/ortam, asset manifest digest'i, giriş/çıkış logları, test adımları, ölçüm ve ret nedenleri üretir. Video destekleyicidir; kaynak varlığı runtime kanıtı değildir. Araştırma başarısızsa hangi kabiliyetin kapanacağı aşağıda bellidir.

| ID / sorumlu alan | Deney ve beklenen çıktı | Geçiş kriteri | Başarısızlıkta |
|---|---|---|---|
| R-01 Native integration | GTA 1.0 US gerçek fingerprint, attach/unload, bootstrap crash ayrımı | Tekrarlı başlatma/kapanışta temiz lifecycle; bilinmeyen binary güvenli ret | Multiplayer feature geliştirmesi bekler |
| R-02 Engine loop | Input/frame sırası, x86/x64 IPC, pool basıncı, birim/eksen dönüşümü | Main thread bloklamadan komut apply; stale handle reddi; bütçe içinde | Controller/capacity iddiası yapılmaz |
| R-03 Runtime security | Windows .NET worker + AppContainer/IPC; Linux server isolation | Dosya/ağ/süreç kaçışı testlerinin engellenmesi, kill/grant revoke, IPC bütçesi | İndirilen client C# kapalı; güvenilen yerel deneyler |
| R-04 Asset toolchain | DFF/TXD/COL/IFP cook/bind/release, hash, malformed corpus | Tekrarlı cook digest; leak olmayan yükle/boşalt; parser sınırları | Hatalı tür katalogdan çıkarılır |
| R-05 World adapter | Stok placement kimliği, streaming spawn bastırma, portal/interior | Kırık objenin 100 stream döngüsü ve late join'de geri gelmemesi | Yalnız doğrulanmış özel sahne/yerleşimler |
| R-06 Physics/destruction | Headless solver seçimi, native impact köprüsü, beş prefab | Tek transition, ortak collider, bounded fragments, tutarlı repair | Server-simulated capability daralır |
| R-07 Animation | Skeleton/IFP, clip timing, root motion, unload | Late join fazı, interrupt ve gameplay event tekliği | Uyumlu rig/kliplerle sınırlı |
| R-08 Vehicles/combat | Handling scope, native damage bastırma, hit query/controller | Çift hasar yok, doğrulama açıklanabilir, profil tutarlı | İlgili mod/server authority seviyesi kısıtlanır |
| R-09 Presentation | UI renderer ve audio/voice codec seçimi, cihaz/focus izolasyonu | Türkçe/DPI/input, bütçe, voice world izolasyonu, cihaz kaybı | Zorunlu olmayan presentation kabiliyetleri kapalı |
| R-10 AI/navigation | Path query, dinamik engel, native task ve uzaktaki simülasyon | Stale nav sonucu uygulanmaz; düşük/yüksek detay geçişi tutarlı | Karmaşık AI paketi ertelenir |
| R-11 Network | Seçilmiş GNS için R-11a kimlik, R-11b taşıma, R-11c codec alt kapıları | Üç alt kapının ayrı kanıtı; %3 kayıpta state yakınsaması | Üretim katılımı bekler; ENet'e otomatik geçilmez |
| R-12 Content provenance | Server collision hazırlama ve dağıtım kaynağı | Uygun kaynak/lisans kaydı ve client/server collision eşlemesi | Özel yeniden dağıtılabilir test sahnesiyle sınırlı |
| R-13 Plan/schema toolchain | WorldPlan resolver, ContractSchema C++/C# binding üretimi, composition provenance ve canonical digest | AC-34/35/38; aynı girdiye aynı çözüm; private schema client'a sızmaz | İlgili paket/plan üretim için eligible değil; temel statik doğrulama olmadan otomatik yükleme yok |
| R-14 Execution | Tick fazları, read/write DAG, async deadline, multi-aggregate reserve, caller delegation ve quota | AC-36/37/45; tek writer, bounded queue, stale job ret ve kontrolün ilerlemesi | Parallel sistemler kısıtlanır; authority/permission invariants geçmeden ilgili gameplay açılmaz |
| R-15 Gameplay projections | Actions/Effects ilk örnekleri, ChangeSet/nav/audio invalidation, visual kalite eşdeğerliği | AC-39/40/41; tekrar ödül yok, stale collider/path yok, cache rebuild izlenebilir | Opsiyonel action/projection türü kapanır; kritik gameplay state garantisi korunur |
| R-16 Release/conformance | İzole fixture world, metadata trust/freshness, seçilecek TUF entegrasyonu, scoped EvidenceRecord | AC-42/43/44; testten üretime erişim yok; metadata rollback ret; build kanıtı doğru kapsamda | Otomatik update/yayın ve ilgili test-world özelliği kapalı; manuel güvenilen geliştirme sürer |

## V0.4 nüfus ve hayvan araştırmaları

| ID / sorumlu alan | Deney ve çıktı | Geçiş kriteri | Başarısızlıkta |
|---|---|---|---|
| R-17 Population / ground AI | Native ambient spawn/retire/task bastırma; yaya, sürücülü araç, sinyal/şerit, slot ve policy | AC-46–54/68; ortak birey, çift üretim yok, protected drain, owner/late join ve N1 yakınsama | Stok şehir nüfusu kapalı; kanıtlı özel sahne/agent alt kümesi |
| R-18 Air traffic | Uçak ve helikopter seyir/kalkış/iniş ayrı; koridor/rezervasyon, hızlı AOI, owner kaybı ve etkileşim | AC-51/52/55; her uçuş görevinde tested loss/failure policy; yolcu ve collider tutarlılığı | Başarısız uçuş capability/görevi kapalı; seyir kanıtı iniş desteği sayılmaz |
| R-19 Creature adapter | Ped-backed/custom controller karşılaştırma; rig, body/hit proxy, gait, task, stream/reload ve tür ekleme | AC-56–67/70 temel alt kümesi; ground quadruped önce; flying/aquatic/mount/IK/ragdoll ayrı scoped kanıt | Uygun rig/beden/controller alt kümesi; model yüklendi diye genel hayvan desteği ilan edilmez |
| R-20 Animal life / ecology | Sahipli birey restore, life stage/corpse/loot, breeding/offline takvim, group maliyeti | AC-58–61/67/69/70 kullanılan alt küme; tek doğum/ödül, bounded catch-up, state kaybı yok | İleri ecology/üreme kapalı; basic Animals ve ownership için gereken durability şartları yine zorunlu |

R-17, R-02/05/08/10/11/14'ü kullanır; R-18 ayrıca road dışı flight/controller kanıtı ister. R-19; R-04/07/10/12/14 ve temel ağ kanıtına bağlıdır. R-20, R-19 ile persistence/Actions sağlayıcılarının kullanılan alt kümesini gerektirir. Tek R etiketi paket içindeki bütün alt özelliklerin geçtiğini göstermez. Doğum kapısı sonraya kalsa da sahipli hayvan açılmadan restore/idempotency doğrulanır.

## V0.5 stabilite deneyleri

| ID / sorumlu alan | Deney ve çıktı | Geçiş kriteri | Başarısızlıkta |
|---|---|---|---|
| R-21 Time / authority | Windows adapter/Host ve Windows/Linux server clock provider; stall/suspend/resume, checked sayaç, WriterTerm guard ve takeover fixture | AC-71/72/75/84/86; tick durunca lease uzamaz; eski process/term yeni dünya yazamaz; backend garantisi kayıtlı | İlgili engine/backend profile production kapalı; monotonic saat okuması tek başına tüm watchdog kanıtı değil |
| R-22 Recovery / durable coordinator | CommitUnknown/reload/crash, circuit/queue budget, shutdown, backup closure; aynı backend transfer phase fault injection | AC-73/74/76/77/82/83/85/86/87/88 kullanılan alt küme; tek etki ve birey, bounded kaynak; restore kanıtı | Durable world temel kapısı geçmez; transfer ayrıca kapalı; unsupported backend için garanti taklidi yok |
| R-23 Replication repair | Baseline assembler/digest/decode cap, ACK/base, bağımsız motion, slow join, logical/native repair | AC-78–81/88; kontrol ilerlemesi, sınırlı bellek/deneme, yanlış critical binding ile gameplay yok | İlgili codec/native capability kapalı; sınırsız retry veya collision düşürme başarı sayılmaz |

R-21 D1 clock/adapter, D2 writer guard alt kanıtıyla ilerler. R-22 D2 durable core, D3 reload/backup; transfer sadece kullanılan ürün kolunda zorunludur. R-23 D2 temel baseline/motion, D3 bounded repair, D4 arıza yükü olarak aşamalanır. Air/animal seçilmemiş temel dünyada bu controller testleri zorunlu değildir; genel core recovery şartları yine geçerlidir. R-21/22/23 hiçbir runtime sonucu olmadan kapanmaz.

## V0.7 native SDK alt kapıları

[ADR-35](architecture-decisions.md) ve [native entegrasyon sözleşmesi](../architecture/native-sdk-integration.md), R-01/02/04'ü aşağıdaki ölçülebilir teslimlere ayırır. Kaynak incelemesi tamamlandı; bu tablodaki uygulama deneyleri çalıştırılmadı. Ana R kaydı tek alt testle kapanmaz.

| ID / kesit | Deney ve çıktı | Geçiş / başarısızlık sınırı |
|---|---|---|
| R-01a / D1-N2 | Gerçek executable fingerprint, kullanılan symbol/ABI ve mapped-image/hook önkoşulları | AC-90; benzer version marker'ı yanlış hash'i kabul ettirmez; adres/phase kanıtı eksikse profil kapalı |
| R-01b / D1-N2 | SDK bağımsız bootstrap, statik initializer ve loader sınırı denetimi | Ret öncesi SDK çağrısı/hook mutasyonu sıfır; file/process uyuşmazlığında active'e geçiş yok |
| R-01c / D1-N3 | Hook ownership, kısmi kurulum rollback'i, callback drain, temiz session/process stop | AC-91; en az 20 tekrarlı process koşusu hedefi; dinamik DLL unload ayrı sonuç/capability ve ancak tam unpatch/quiescence kanıtıyla |
| R-02a / D1-N1 | Pin/patch/recipe/notice lock; private x86 SDK compile/link, toolchain/CRT/layout deneyi | AC-89; core/Host'a vendor include/link sızıntısı veya karışık header/library ret; C++latest ihtiyacı C++20 uyumu sayılmaz |
| R-02b / D1-N4 | Gerçek adapter/Host ABI, bounded IPC, frame/input sırası, watchdog ve Host kaybı | AC-92/93 ve R-21; yanlış ABI/epoch/thread/stale komut ret, main loop'ta I/O bekleme yok; p50/p95/p99/max raporlu |
| R-02c / D1-N5 | Tek native türün pool/binding generation, create/apply/retire ve property matrisi | AC-94/12; pool full, slot reuse, geç async iş ve çift destroy; başarısız tür/property kapalı |
| R-04a / D1-N6 | Yerel stok model request/readiness/bind/release, lease iptali ve late completion | AC-95/11/30; en az 100 döngü hedefi, sahip olunan ref/lease sayıları ve bütçe raporlu; custom parser/cook ve bütün R-04 kapanmaz |

D1-N7, AC-96 ile lock/engine/adapter değişikliğinde evidence kapsamını sınar. SDK'nin fonksiyon listesi R-05/06/07/08/10/17/18/19 capability'lerini açmaz. [D1 planı](../development/d1-engine-integration.md) ve [güncel durum](../development/status.md) bu deneylerin iş sırasını ve uygulama sınırını taşır.

## Solver ve codec kararını somutlaştırma

R-11 kütüphane seçimi yarışması değildir; [GNS kararı](network-transport.md) kapalıdır. Aşağıdaki entegrasyon alt kapıları açıktır:

| ID | Somut deney / çıktı | Ret ve yayın sınırı |
|---|---|---|
| R-11a | GNS'nin desteklediği standalone sertifika/trust-root entegrasyonu; self-hosted server kimliği, provisioning, anahtar yenileme/iptal ve bootstrap→peer bağı | Yanlış peer, MITM, eski/iptal kimlik, geçersiz güven kökü ve replay token reddi; başarısızsa üretim Join kapalı |
| R-11b | Windows x64 host + Linux/Windows x64 server GNS build, bağımlılık lock'u, direct IPv4/IPv6, N0/N1/N2 ve lane basıncı | Kontrol rezervi, baseline ilerlemesi, bounded memory; istemcide GTA main thread socket beklememeli |
| R-11c | SAEX typed codec, schema negotiation, boyut/depth sınırı, fuzz corpus, çapraz lane nedensellik ve reconnect kimlikleri | Bozuk veri callback'e erişmez; AC-25/26/31/33; başarısız özellik üretim paketinde açılamaz |

R-02/R-04 kapsamına native property `live-set/recreate-required/unsupported` matrisi, frame bütçeli staging ve late-load iptali eklenmiştir. R-03; resource başına .NET baseline/JIT/heap, toplam worker admission ve eski+yeni reload bellek maliyetini ölçer. R-05 placement state'ini native kırılma efektini tekrar oynamadan kurmayı sınar. Bu işler MTA kaynak incelemesinden çıkan deney gereksinimleridir, tamamlanmış kabiliyetler değildir.

R-06 aday solver için aynı basit sahneyi kullanır: kutu yığını, menteşeli kapı, kopan lamba, 128 aktif body, native araç teması. Ölçütler: desteklenen constraint/query, sabit zaman adımı, sleeping, thread kontrolü, lisans ve hedef platform. GTA hissi ile farklar ayrı raporlanır. Seçim ADR ekidir; hiçbir adayın GTA'yı otomatik birebir simüle ettiği kabul edilmez.

R-11 codec için typed bounded schema, eski/yeni sürüm anlaşması, allocation üst sınırı, bozuk input corpus ve snapshot page overhead ölçülür. Başarılı adayla message ID/byte encoding sürümü sabitlenir. Bu belge sırf tam görünsün diye opcode veya hash uydurmaz.

R-09 UI için x86 binding/texture maliyeti, action latency ve erişilebilirlik; audio için codec CPU, end-to-end latency, loss ve cihaz değişimi raporlanır. Hazır kütüphane sürümü benimsenmeden lisans/bakım kaydı kaynak envanterine girer.

## D1 foundation ilk uygulama kanıtı

R-01 için kullanıcının gerçek executable'ının PE/SHA-256 gözlemi ve unverified profile ret; R-13 için metadata WorldPlan ve ortak kimlik generator'ı; R-14 için bounded concurrent ingress; R-21 için lease/time/checked counter alt testleri kodlandı. R-11c ile ilişkili yerel fixture codec'i GNS oyun codec seçimi değildir. [D1 kanıt tablosu](../development/d1-foundation.md) Windows x64/x86 sonuçlarını ve geçilmeyen native/OS/sandbox kısımlarını listeler.

Hiçbir R kaydı bütünüyle kapatılmadı. “GTA dosyası incelendi” verified hook/engine profili anlamına gelmez; “iki süreç haberleşti” OS izolasyonu veya ağ güveni kanıtı değildir. [Güncel uygulama durumu](../development/status.md).

## D1-N1 native dependency alt kanıtı

R-02a / AC-89 için [seçilmiş kaynak build'i](../development/d1-native-dependency.md) uygulandı: exact commit/arşiv/23 dosya, sıralı iki patch, recipe/notice/toolchain kilidi, private x86 link/layout ve negatif doğrulama. C++20 deneyi başarısız; ADR-36 ile yalnız vendor hedefi C++23 kullanır. Bu sonuç N2'nin gerçek symbol/engine ve bootstrap, N3'ün hook/stop, N4'ün IPC/watchdog ölçümlerini karşılamaz. R-02 ana kaydı ve bütün runtime kapıları açık kalır.

## Karar ve teslim ölçütleri

Araştırma raporu `hypothesis, setup, observations, pass/fail, limitations, affected capabilities, next ADR` alanlarını içerir. Araştırma bittiğinde “kabiliyet doğrulandı” etiketi yalnız geçen engine/build/profile için verilir. Başka oyun sürümüne otomatik aktarılmaz.

Doğrulanmamış engine kabiliyeti gerekli olan release kapısı açık kalır. R-01/02/03/04/05/06/11/12 ve R-13/14 temel alt kümesi ilk dünyaya güvenli katılım ve yıkım için ana engellerdir. R-07/08/09/10/15 ilgili gelişmiş özellik kapılarında, R-16 otomatik update/yayın ve izole prova katmanında zorunludur. Ayrıntılı bağımlılıklar [yol haritasındadır](../roadmap.md).

## D1-N2 dosya/image gözlem ilerlemesi — 13 Eylül 2026

R-01a/AC-90 için [N2 alt kanıtı](../development/d1-engine-preflight.md) eklendi: exact yerel dosya, index/RVA bölüm düzeni, dört SDK kaynak adresinin kısa disk byte'ları ve SEC_IMAGE_NO_EXECUTE görünüm eşleşmesi. Aynı version marker/yanlış hash, bozuk PE ve unreadable image retleri sınandı. `ObservedProfileId` runtime profil değildir. R-01a function ABI/instruction ve unpack/başlatma fazı ile gerçek GTA image önkoşullarını bekler; R-01b yüklenebilir bootstrap/initializer denetimi ve R-01c lifecycle gözlemi yapılmadı. N2/N3 geçişi ve ana R-01 açık; ADR-37.

## D1-N2 başlangıç DLL ilerlemesi — 13 Eylül 2026

R-01b/AC-90 için [x86 bootstrap modülü](../development/d1-bootstrap-module.md) eklendi: SDK bağımlılığı olmadan OS load, sürümlü C ABI, açık host doğrulama, ret/stop/BUSY ve 100 ölçülen oyun dışı unload çevrimi. Artifact PE/import/export/TLS/CRT map kontrolleri ve negatif testler vardır. Gerçek GTA'da Initialize/image yolu çalıştırılmadı; R-01a function ABI/unpack/faz ve R-01b gerçek load sırası hâlâ açık. İlk process-wide handle artışının tekil kaynak sahipliği doğrulanmadı; warm stabilite sonucu dar kapsamlıdır. SDK bağlama/initializer envanteri ve R-01c hook/stop deneyi sonraki aşamadır. ADR-38 gözlemden runtime aktivasyonu çıkarmaz.

## Kod 0.1.4 — İlk process image alt kapsamı

R-01a/b ve AC-90 için [askıda process observer](../development/d1-suspended-process.md) eklendi: ilk create-debug olayı, verified dosyayla OS file ID eşleşmesi ve bounded image okuması. Owned child oyun kodu resume edilmeden sonlandırılır; exit doğrulanmadan gözlem başarı sayılmaz. Bu kapsam önceki SEC_IMAGE_NO_EXECUTE ve oyun dışı DLL sonuçlarından ayrıdır. Unpack/başlatma, function ABI ve gerçek DLL yükleme sırası açık; R-01c/N3 ve ana R-01 kapanmaz. ADR-39.

## Kod 0.1.5 — Native başlangıç çevresi

R-01a/b ve AC-90'a [statik startup envanteri](../development/d1-native-startup.md) eklendi. Dosya kimliği yanında yerel DLL/ASI adayları, import/delay ilişkileri, entry/TLS ve bozuk metadata açıkça kaydedilir. Mevcut kurulumda extension dosyaları bulunduğu için executable hash'i loader temizliği varsayımı vermez. Gerçek loaded-module seçimi, unpack/ABI ve DLL load sırası araştırması sürer; ADR-40. Eski dosya/ilk-debug kanıtları yalnız kendi fazları için geçerlidir.

## Kod 0.1.6 — R-01a/b loader mapping ara kanıtı

[Bounded x86 observer](../development/d1-loader-observation.md), gerçek LOAD_DLL olaylarını pinlenmiş file ID/hash ile izler. Fixture'da init öncesi breakpoint sınırı üç canary ile ölçüldü. Gerçek GTA ntdll/kernel32/kernelbase sonrasında apphelp.dll mapping'inde açık ret aldı; hash korunumu ve owned exit doğrulandı. Sonraki somut araştırma girdisi compatibility mapping kaynağı/erken yürütme ve kontrollü initialization ortamıdır; sonra unpack/symbol/ABI ve SAEX DLL load gelir. Bu kayıt tam closure/initializer sırası değildir; R-01/N3 açık, ADR-41.

## Kod 0.1.7 — R-01 launch context ayrımı

[İncelenen recipe](../development/d1-loader-policy.md) apphelp'i mapping kapsamına aldı; orijinal kurulum AcLayers'ta durdu. Yerel Microsoft signature ve dar Layers/Custom/IFEO/__COMPAT_LAYER kontrolleri compatibility'nin tamamının yokluğunu kanıtlamadı. Bit eşitliğinde üç dosyalı ayrı kopyada Debug 22/Release 21 DLL ve breakpoint adayı gözlendi. Filename/path/yan dosyalar birlikte değiştiğinden causality ayrı araştırmadır. Sonraki somut çıktı controlled context, dynamic proxy/init ve gerçek SAEX bootstrap/entry-unpack/ABI; N2/N3 kapıları açık, ADR-42.

## R-01b — Kod 0.1.8 context alt araştırması

[Explicit context](../development/d1-launch-context.md) bir koşuda environment/cwd girdilerini açıklaştırır. Registry/AppCompat, dynamic proxy başlangıcı ve arbitrary native initializer davranışı hâlâ ayrıca incelenir. Fixture PATH/cwd çözümlemesi gerçek GTA DLL initialization veya R-01b'nin bütünü için sonuç değildir.

## R-01b — Kod 0.1.9 unload/remap alt araştırması

[Lifecycle kesiti](../development/d1-loader-lifecycle.md) retired adreslerin gözlem kanıtına tekrar katılmasını engeller. Portable metadata corpus, OS canary ve gerçek UNLOAD kolu ayrı raporlanır; gerçek same-base remap görülmediğinde bu durum açık kalır. Initialization/dinamik proxy ve gerçek SAEX bootstrap araştırması sürer.

## Kod 0.1.10 — R-01a/b/c ve R-02 kapsamına statik sembol kanıtı

[Linkage araştırması](../development/d1-native-linkage.md) GTA→vorbisfile için iki adayda 7/7 direct isim eşleşmesi; hooked→vorbis 22/22, vorbis→ogg 11/11; wrapper→hooked statik kenarı bulunmamasını ayrı outcome olarak kaydeder. Wrapper import API'leri ve loader string işaretleri initializer/dinamik load araştırmasını gerekli kılar; yürütülmüş çağrı veya doğrulanmış upstream source/build kimliği değildir. Sonraki bounded initialization deneyi bu kod yolunu ve fonksiyon ABI'sini incelemeli; alınan isim listesi policy pinlerine otomatik aktarılmaz. ADR-45, N2/AC-90 initialization ve N3 kapılarını açık tutar.

## Kod 0.1.11 — R-01a/b/c giriş safhası

[Entry-boundary araştırması](../development/d1-entry-boundary.md) actual vorbisFile DLL'deki entry rewrite byte'larını kaynak yolu ile karşılaştırır; exact upstream build kimliği çıkarmadan post-DLL entry mutation ölçümü tasarlar. İlk geç DR0 kurulumu failed; erken CREATE_PROCESS kurulum corpus'u olumlu sonuç verdi. Gerçek GTA, farklı phase/context ve artifact SHA ile ayrıca sınanır. Entry thunk'ından ileri proxy/IAT/dynamic vorbishooked yolu, unpack, SAEX bootstrap/ABI hâlâ açıktır; ADR-46 geçiş sınırını tanımlar.

0.1.11 R-01a/b/c son gözlem: 6 Debug + 6 Release private entry durağında ilk beş byte'ın vorbisfile+0x1D60'a E9 olması doğrulandı; atlama devam ettirilmedi. İlk imm32 ret sonrası ayrı, exact base digest'e bağlı system-only supplement incelendi. Orijinal AcLayers ret ve legacy ilk-breakpoint sınırı korundu. Bu sonuç gerçek entry yönlendirmesi kanıtıdır; proxy yükleme gövdesi/unpack/SAEX DLL/ABI araştırması açık kalır.
