# Kabul senaryoları ve gözlenecek kanıt

Durum: gelecekteki uygulama testlerinin şartnamesi. **Aşağıdaki gameplay testlerinin hiçbiri bu dokümantasyon teslimatında çalıştırılmadı.** [D0 incelemesi](documentation-review.md) ayrı kayıttır.

## Ortak test düzeni

Sürüm/fingerprint'i kaydedilmiş sunucu, en az iki gerçek GTA istemcisi ve late join için üçüncü oturum kullanılır. Özel Harbor sahnesi aynı lock set ile yüklenir. Testlerde server transaction log'u, client state revision, collider query ve gerektiğinde video kaydı karşılaştırılır.

Bu çok istemcili düzen gameplay/end-to-end senaryoları içindir. D1 native SDK alt senaryoları AC-89–96 kendi static/contract/tek GTA process düzenini aşağıda tanımlar; bunlar D2 kanıtı değildir.

Ağ profilleri [performans belgesindeki](performance.md) N0/N1/N2'dir. Rastgele kayıp testinin seed'i kaydedilir. Test oracle'ı “benim ekranımda doğru görünüyor” değildir. Kabul edilmiş gameplay state ve collision sorgusu temel kanıttır.

## Dünya, otorite ve yeniden katılım

| ID | Kurulum ve işlem | Beklenen sonuç / kanıt |
|---|---|---|
| AC-01 | Health 20 olan kasaya iki farklı oyuncudan aynı tick aralığında 15'er hasarlık geçerli darbe | Health 0, tek broken transition, tek ödül/fragment set; iki receipt ve bir transition log'u |
| AC-02 | Aynı ImpactIntent'i üç kez gönder; state/motion paketlerini ters sırala; N1 kayıp ekle | Hasar bir kez; state revision gerilemez; eski sequence/epoch atılır; bütün client'lar accepted state'e yetişir |
| AC-03 | Lamba dynamic iken owner bağlantısını kes; eski owner paketi daha sonra gelsin | Lease revoke/expiry, yeni owner epoch, son accepted snapshot'tan devam; eski owner yazamaz |
| AC-04 | Bir client bölgeden çıkıp girsin; üçüncüye baseline S gönderilirken nesne kırılsın | Stok sağlam nesne geri doğmaz; üçüncü JoinCommit sınırına yetişip JoinApplied verir; ilk gameplay anında collider/state aynı |
| AC-05 | Durable duvar yıkımını onayla, server'ı kapatıp restore et | Yeni world epoch, aynı kalıcı broken state, yeni runtime referanslar; başlangıçta intact flash/engel yok |
| AC-10 | Aynı koordinatta iki world, ayrı portal/voice routes; oyuncu geçiş yapsın | Yanlış world hit/voice/state yok; eski scope paketleri reddedilir; hedef hazır olmadan geçiş yok |
| AC-12 | Native/entity ID tekrar kullan; parent-child stream et; eski async işi tamamla | Generation/epoch stale sonucu reddeder; child dependency bozulmaz; yeni entity yanlışlıkla silinmez |
| AC-13 | Geçersiz hız/impulse, yanlış owner ve uydurulmuş client damageAmount gönder | Ret/düzeltme ve gerekçe; kalıcı hasar/ödül oluşmaz; normal jitter tek başına ceza nedeni olmaz |

## İçerik, güncelleme ve yalıtım

| ID | Kurulum ve işlem | Beklenen sonuç / kanıt |
|---|---|---|
| AC-06 | Eksik COL, yanlış artifact digest, aynı version altında farklı içerik ve kesilmiş download | Ayrı hata nedenleri; bozuk içerik aktive edilmez; doğrulanmış chunk'tan resume; yanlış collision ile join yok |
| AC-07 | Collision değişen release hazırla; bir client eski katalogda kalsın; migration hata versin | Commit öncesi eski world korunur; yarı eski/yeni collision yok; commit sonrası rollback sınırı açık |
| AC-08 | Resource start, drain, migrate ve commit sonrasında ayrı ayrı hata üret; 100 reload uygula | Tanımlı abort/stop/forward-fix; timer, event, worker ve asset lease birikimi yok |
| AC-09 | Client worker özel dosya/ağ/süreç erişimi ve başka resource adına IPC istesin | OS/broker ret; core çalışır; grant revoke sonrası eski worker komutu uygulanmaz |
| AC-11 | Cache cold, düşük VRAM/pool, hızlı teleport ve eksik zorunlu asset | Güvenli staging; yalnız cosmetic kalite azaltma; kritik collider client bazında silinmez |
| AC-14 | Path traversal/case çakışması, aşırı decompress, bozuk DFF/IFP, aşırı graph ve network payload | Parser/kota ret; beklenen diagnostic; process/worker bellek sınırı ve core sağlığı korunur |
| AC-15 | GTA adapter başlatma hatasını bağlantıdan önce, asset hatasını bağlantıdan sonra üret | Log aşamaları farklıdır; session yokken network/asset suçu uydurulmaz; redacted diagnostic yeterlidir |
| AC-19 | C# client/server resource UI ve input etkileşimi kursun; voice cihazı çıkarılsın | Server intent onayı ayrı; focus geri bırakılır; voice arızası gameplay session'ını düşürmez |
| AC-22 | Yetkisiz admin/API/debug çağrısı; geçerli request'i tekrar gönder | Scope/role ret; yetkili işlemin receipt'i tek; debug private state'i sızdırmaz |

## Yıkım, gameplay ve uzun oturum

| ID | Kurulum ve işlem | Beklenen sonuç / kanıt |
|---|---|---|
| AC-16 | Cam, kasa, çit, lamba ve duvarı tanımlı darbelerle kır | Profile uygun state/collider; ışık ve ses bir kez; bounded fragments; TTL parent state'i sağlam yapmaz |
| AC-17 | Onarılacak duvar hacminde oyuncu/araç tut; alanı boşalt ve onar | İlk deneme bekler/ret; ücret yanlış kesilmez; uygun anda atomik onarım ve yeni repairGeneration |
| AC-18 | Durable transaction commit öncesi ve onaydan hemen sonra ayrı crash; outbox yeniden teslim | Commit öncesi başarı yok; onaylanmış state restore olur; dış domain ödülü bir kez |
| AC-20 | P1/P2 load, 100 hücre giriş/çıkışı, 100 reload, 8 saat session | [Performans sınırları](performance.md); doğrulanmış leak yok; queue/pool trendi raporlu |
| AC-21 | İki oyuncu aynı alana yapı kursun; maliyet yetersiz/transfer tekrar gelsin | Collision/ownership kontrolü; tek accepted placement ve atomic debit; retry çift eşya üretmez |
| AC-23 | NPC yol ararken duvar yıkılsın; uzak/yakın AI geçişi; görev bitişini tekrar gönder | Stale nav işi reddi; görünür teleport yok; mission ödülü bir kez |
| AC-24 | Aynı build'de Harbor, Racing ve Survival world profillerini çalıştır | Gereksiz kit bağımlılığı yok; seçilen map/HUD/input/kurallar aktif; GTA varsayılanı istenmeden geri dönmez |

## V0.2 tutarlılık ve MTA referans kabulü

| ID | Kurulum ve işlem | Beklenen sonuç / kanıt |
|---|---|---|
| AC-25 | Spawn/state lane'ini geciktir; motion/resource event'i önce gelsin; CatalogCommit de ayrı lane'de geciksin | Bilinmeyen entity callback'e verilmez; requiresStateRevision/TransitionId kapısı; motion coalesce ve bounded resync; yanlış collider'a apply yok |
| AC-26 | Durable hasar/inşa ACK'ını kaybet; reconnect ile yeni RequestId, aynı actor/OperationId kullan; DB commit-core apply arasında crash et | Tek durable sonuç/ödül; restore edilmiş receipt; aynı kimliğe farklı payload operation_conflict; başka actor receipt okuyamaz |
| AC-27 | 1 MiB baseline ve pahalı prefab native hazırlığı; frame bind bütçesini 2 ms tut | Hazırlık çok kareye bölünür; staging collision/event üretmez; JoinApplied öncesi input kapalı; büyük bölünemez prefab ret/transition gerekir |
| AC-28 | Reload participant listesi kapandıktan sonra join; bir client Ready vermesin, biri Ready sonrası apply sırasında çöksün | İlk client commit öncesi fence/abort; yeni join uygun staging; postcommit client resync; eski katalogla etkin gameplay yok; durable state geri silinmez |
| AC-29 | Oyuncuyu AOI/syncer sınırında küçük ileri-geri hareket ettir; araç/römork grubunu geçir; owner kopmasını ekle | Hysteresis/minimum tutma gereksiz churn'ü engeller; disconnect anında istisna; closure eksikse admission yok; grup devrinde tek geçerli owner epoch |
| AC-30 | Resource crash, destroy callback'inde ikinci destroy, retire sonrası asset load completion, eski+yeni worker bellek baskısı | Broker registry bütün lease'leri bulur; çift free/yanlış handle bind yok; retirement ref/fence bekler; toplam bütçe aşan prepare reddedilir |
| AC-31 | Saklanmayan replike kapı state'i, sampled motion ve durable duvar değişimini aynı baseline kesitinde yakala; ardından restart | Canlı baseline none state'i içerir; disk restore yalnız ilan edilen persistence sınıfını getirir; WorldRevision/JournalSequence/ViewSequence boşlukları karışmaz |
| AC-32 | Doğru HTTPS bootstrap sonrası yanlış GNS server peer, iptal/eski anahtar ve aradaki saldırgan test fixture'ı | Üretim kimlik kapısı reddeder; credential/Join gönderilmez; şifreleme tek başına güvenli endpoint sayılmaz; R-11a kanıtı |
| AC-33 | Baseline, yoğun motion, voice, remote event ve kontrol spam'ini N1 bant sınırında birlikte çalıştır | 4 MiB reliable/64 KiB kontrol rezervi, oran sınırları; baseline tamamen aç bırakılmaz; gameplay p95/p99 ve queue peak raporlu; kota aşımı açık ret |

AC-25–31: [ortak sözleşme](../architecture/activation-contracts.md). AC-25/32/33: [taşıma kararı](../decisions/network-transport.md). MTA'dan alınan ilkelerin dayanağı [kaynak incelemesidir](../references/mtasa-architecture.md). Bunlar da gelecekte yürütülecek testlerdir.

## V0.3 dünya üretimi ve güvenilir işletim

| ID | Kurulum ve işlem | Beklenen sonuç / kanıt |
|---|---|---|
| AC-34 | WorldProfile'a iki tekil service provider, aynı alan için çakışan mutator ve eksik collider capability ekle | WorldPlan oluşturma/production eligibility uygun aşamada reddedilir; dependency/field nedeni açık; ilk oyuncuya kadar saklanmaz |
| AC-35 | C++/C# schema sürümlerini farklılaştır; uzun listeler ve bilinmeyen zorunlu field gönder | Ortak ContractDigest anlaşması veya açıklamalı ret; bounded decode; private schema otomatik client dağıtımına girmez |
| AC-36 | Aynı kabul edilmiş ingress sırasına ait bağımsız job'ları farklı thread bitiş sırasıyla tamamla; ayrıca deadline'ı kaçıran eski sonucu gönder | Aynı hazır iş kümesinde ilan edilen schedule/command sırası korunur; stale read set/epoch ret; deadline kaybı ayrı raporlanır, native fizik determinism'i varsayılmaz |
| AC-37 | Yetkisiz resource güçlü inventory provider'ı üzerinden para/eşya değişimi istesin; iki domain aynı aggregate'a pending yazsın | ActorContext/delegation kesişimi ret; resource kimliğiyle yetki yükselmez; rezerv sırası bounded, harici DB için sahte atomiklik yok |
| AC-38 | Prefab trait'leri aynı alanı çelişkili yazsın; patch hedefini yeniden adlandır; socket skeleton'ını değiştir | Conflict/unknown target/incompatible socket build hatası; ExplainField kaynak zincirini gösterir; sessiz default yok |
| AC-39 | Uzun onarım/fuel action'ında duplicate step, disconnect, dolan onarım hacmi ve resource reload üret | Her accepted step tek sonuç; maliyet/rezerv/telafi policy'ye uygun; DefinitionRevision korunur; cue kalıcı hasar yazamaz |
| AC-40 | Duvar yıkımı sırasında eski nav/acoustic job'ı döndür, projector'ı çökert, follow-up görev olayını yinele | Kritik state/collider geçerli; stale projection ret/rebuild; NPC güvenli bekler veya güncel collider sorgular; ödül bir kez |
| AC-41 | Aynı world'de düşük/yüksek görsel varyant; bir client'ta interaction closure için native bütçe yetersiz | GameplayDigest/semantics eşdeğerliği sınanır; collider/hit avantajı yok; eksik closure admission/fence ile sonuçlanır |
| AC-42 | Üretimden anonim snapshot ile RehearsalWorld kur; test worker'ı production DB/webhook ve gerçek EntityRef kullansın | Yeni namespace/epoch; OS/broker ret; gerçek bakiye/ödül değişmez; test state'i publish ile üretime merge edilmez |
| AC-43 | Eski imzalı metadata, karışık release parçaları, sona ermiş timestamp ve içerik anahtarıyla native update dene; sonra yetkili rollback release'i sun | İlkleri ret; rollback yalnız yeni metadata + izinli artifact + uygun migration ile; GNS peer kimliği ayrıca doğrulanır |
| AC-44 | Geçen capability raporunu başka engine fingerprint, schema veya collision artifact'iyle kullan | Kanıt kapsam uyuşmazlığı; unsupported/experimental açık; production profile yanlış verified alamaz |
| AC-45 | Bir resource sürekli pahalı job/IPC üretirken kontrol, retire ve küçük gameplay komutlarını çalıştır | Ölçülen CPU/queue kotası; bounded erteleme/ret; grant revoke işler; kontrol/retire aç bırakılmaz; p95/p99 ve overload log'ları |

Bu senaryolar [WorldPlan](../architecture/world-plans.md), [execution](../architecture/execution-contracts.md), [bileşim](../assets/composition-variants.md), [actions](../gameplay/actions-effects.md), [projection](../networking/world-change-projections.md) ve [release](../platform/studio-release.md) sözleşmelerine bağlıdır. Yürütülmüş test sonucu değildir.

## V0.4 nüfus ve trafik kabulü

| ID | Kurulum ve işlem | Beklenen sonuç / kanıt |
|---|---|---|
| AC-46 | Aynı world'de all-off, pedestrians-only, road-only, air-only, city-all ve wildlife-only; zone/saat ayarlarını canlı değiştir | İzinli kanallar doğru; pedestrians off sürücü/pilotu silmez; görev NPC'si korunur; accepted policy ile drain tamamlanması ayrı |
| AC-47 | Örtüşen zone'lara eşit öncelikli çelişki ver; iki oyuncu aynı hücreye girsin; grup rezervi kotayı aşsın | Conflict ret, tek director/slot üretimi; oyuncu başına çift nüfus yok; driver/vehicle/animal toplam kota hesabı |
| AC-48 | Spawn prepare beklerken policy off; sonra eski aday ve duplicate policy isteği gelsin | Commit revalidation eski adayı reddeder; rezerv bırakılır; policy tek receipt; kapalı kanalda geç spawn yok |
| AC-49 | İki oyuncu aynı trafik aracının sürücü koltuğunu almaya çalışsın; eşzamanlı drain ve driver ölümü üret | Tek accepted seat/driver/task/ControlClass geçişi; stale cleanup korunan aracı silemez; native exit animasyonu hak vermez |
| AC-50 | Ped kaçış/görev ve animasyon değişiminde reliable state geciksin, motion/cue önce gelsin; N1 ve late join ekle | Task/StateRevision/catalog bağı kurulmadan apply yok; observer yeni AI kararı yazmaz; join ilk anda güncel life/task/seat state |
| AC-51 | Yaya, sürücülü araç, hayvan ve pilot owner'ını kopar; eski epoch sonuçlarını yeniden gönder | Her destekli controller'da tested fallback/devir; tek yazar; korunmuş yolcu; hava fallback kanıtı yoksa ilgili profil unsupported |
| AC-52 | Hızlı uçakla veya koşan hayvanla cache-cold/bütçesi düşük client'ın alanına yaklaş; N2 kesinti ekle | Hız/menzil/hazırlık tabanlı prefetch, closure admission/fence; görünmez collider/hasar yok; ölçülen uyanma ve apply gecikmesi |
| AC-53 | İki AI araç conflict zone'a gelsin; sinyal değişsin, oyuncu kırmızıda girsin, yol kaza ile kapansın | Ortak sinyal/rezervasyon state; güncel obstacle/temas, bounded stuck/replan; görünür teleport veya client başına farklı ışık kararı yok |
| AC-54 | Çit/duvar ekle/kaldır; küçük ve büyük hayvanla eski read set'li nav işlerini tamamla | Yanlış clearance/stale cell/controller sonucu reddedilir; güncel collider geçilmez; partial path varış sayılmaz |
| AC-55 | Uçak/helikopter seyir, kalkış, iniş, pist/helipad tıkanması, hasar ve pilot kaybını ayrı çalıştır | Her alt capability ayrı pass/fail; tek rezervasyon, bounded holding/abort; seyir geçişi otomatik iniş desteği sayılmaz |

## V0.4 hayvan, yaşam döngüsü ve birleşik yük

| ID | Kurulum ve işlem | Beklenen sonuç / kanıt |
|---|---|---|
| AC-56 | Mevcut canine rig'e iki cins/görünüm ekle; yeni species paketi release et; bir client gerekli IFP'yi indirmemiş olsun | Namespace/definition çözümü, ayrı bireyler; zorunlu closure tamamlanmadan spawn/interaction yok; veri ekleme ile native capability kanıtı ayrı |
| AC-57 | Yanlış bind pose, eksik action/socket, aşırı morphology, hatalı hit proxy ve desteklenmeyen uçan rig dene | Build veya native conformance uygun aşamada ret; human capsule/animasyonla gizli fallback yok; exact hata/bağımlılık izi |
| AC-58 | Aynı ısırma/tekme adımını tekrar gönder; marker'ı değiştir; saldıran ölürken target kaçsın; corpse/loot yarışsın | Server action/query/cancel kuralı; tek hasar/loot; client marker tek kanıt değil; corpse cleanup canlıyı diriltmez |
| AC-59 | İki oyuncu aynı hayvanı evcilleştirsin; item debit, ownership ve population drain çakışsın; ACK kaybolsun | Tek accepted owner/ControlClass/slot disposition; başarısız tarafa çift debit yok; reconnect receipt; korunan toplam kota |
| AC-60 | Sürü lideri ölsün/stream-out olsun; üyeler farklı owner'lara dağılsın; alarm spam'i üret | Server tek grup kararı; bounded fan-out/leader seçimi; sosyal grup fizik authority group sanılmaz; üyelik private alanları korunur |
| AC-61 | Sahipli hayvan durable ölüm/tame/owner değişiminden sonra server'ı kapat; restore ve late join yap | PersistentEntityId korunur, yeni EntityRef; ownership/death kaybı yok; motion yalnız checkpoint kadar dönebilir; eski task/lease restore edilmez |
| AC-62 | Behavior worker'ı çökert; eski+yeni rig/library reload bütçesini aş; aktif/persistent/corpse ref varken species kaldır | Registry cleanup, stale task ret, güvenli adoption/suspend; removal ret/migration; aktif IFP erken unload olmaz; postcommit state sıfırlanmaz |
| AC-63 | Wildlife/traffic off sırasında owned pet, farm group, mission NPC ve kullanılan araç tut; deadline dolsun | Yeni ambient üretim durur; protected sınıflar sürer; drain pending/blocked raporlu; force-retire ayrı yetki/koşul ister |
| AC-64 | Aynı bireyi interactive/reduced/abstract arasında geçir; oyuncu yaklaşırken nav/asset/owner eksikliği oluştur | Aktif saldırı/temas abstract'a düşmez; uyanmada güncel state ve safe position; client'lar farklı gameplay collider seçemez |
| AC-65 | Sahte owner/perception/hit intent, aşırı graph/group/path işi ve güçlü provider'a yetkisiz delegation isteği gönder | Grant/read set/kota ret; private blackboard/seed sızmaz; kontrol/retire ilerler; yeni tür asset'i native kod yetkisi vermez |
| AC-66 | Owner pet ile portal/world geçsin; hedefte tür/kota/izin eksik olsun; transfer commit çevresinde crash üret | Kalıcı birey tek world'de aktif; source stay/kennel veya hedef staging; yeni EntityRef/closure; yetkisiz takip/çift birey yok |
| AC-67 | Yavru-yetişkin veya büyük beden varyantını dar kapıda uygula; rig/controller major değişimi ve farklı LOD ekle | Occupied-volume ve gameplay barrier; aynı persistent birey; hit/mesh uyumu; unsupported major restart; kozmetik kalite avantaj üretmez |
| AC-68 | Slot spawn/retire/cooldown ve protected quota transferinde duplicate mesaj/restart; hücreyi tekrar yükle | Ledger/generation doğru, tek birey/cause; seed replay kopya üretmez; eski reservation yeni slotu tüketmez; refill toplam kotayı aşamaz |
| AC-69 | Ecology açık/kapalı; offline süreyi büyüt, saati geri al ve aynı doğum/loot adımını tekrarla | Varsayılan offline ilerleme yok; açık profilde bounded catch-up, tek offspring/ödül, kapasite/clock hatası görünür; sınırsız üretim yok |
| AC-70 | P1/P2 yaşayan dünya karışımı, N1/N2, cold/warm cache, 100 policy/reload/stream döngüsü ve 8 saat soak | Toplam NPC/driver/pilot/animal ve native/CPU/IPC yükü açık; mevcut frame/tick/state bütçesi; leak/queue/owner churn raporu; unsupported alt özellik pass sayılmaz |

Asıl sözleşmeler [Population](../gameplay/population-traffic.md), [Agent](../gameplay/agents-navigation.md) ve [Animals](../gameplay/animals-species.md); kararlar ADR-25–28, araştırmalar R-17–20'dir. N0 temel correctness, N1 yakınsama ve N2 recovery profilleri uygulanır. Zorunlu capability olmadığı için çalıştırılamayan deney unsupported kalır; bu tablolar yürütülmüş test sonucu değildir.

## V0.5 stabilite ve arızadan kurtarma

| ID | Kurulum ve işlem | Beklenen sonuç / kanıt |
|---|---|---|
| AC-71 | Server logic tick'ini 3 s durdur; control izleyicisini çalışır bırak; eski owner motion/hit ve renewal gönder | 2 s lease uzamaz, yeni admission/grant kesilir; dönüşte deadline/epoch revalidate; gecikmiş hasar yok; kaçırılmış tick için sınırsız catch-up yok |
| AC-72 | Host/GTA ve server makinelerini ayrı suspend/resume et; clock domain reseti, gecikmiş grant ve sahte client zamanı ekle | İlk oynanış komutundan önce eski grant revoke; yeniden readiness; ham çapraz-makine timestamp çıkarımı yok; her controller'ın bounded fallback kanıtı |
| AC-73 | Durable tame/hasar commit olsun; core apply öncesi resource crash/reload; sonra eski transient job ve gerçek receipt gelsin | Stale öneri ret, committed sonuç core registry üzerinden tek apply; migration güncel state'ten; durmuş worker'a callback yok |
| AC-74 | Commit submission'dan sonra bağlantıyı kes; receipt sorgusunu geciktir; aynı/farklı payload retry ve cancel gönder | OutcomeUnknown görünür; affected aggregate fenced; receipt görünmüyor diye rollback varsayılmaz; tek etki, farklı payload conflict; çözüm sonrası tek yayın |
| AC-75 | Aynı persistent world için iki process claim etsin; DB partition, eski WriterTerm transaction'ı ve operator takeover dene | Tek guarded writer; eski terim ret; timeout otomatik takeover sağlamaz; eski process kesin durmadan yeni world açılmaz; DB ownership kaybı fenced |
| AC-76 | Sahipli hayvanı ve desteklenen araç/grubu aynı backend world'leri arasında taşı; her transfer phase'inde crash ve duplicate teslim ekle | Tek canonical location/TransferGeneration; precommit kesin abort veya postcommit target pending; iki etkileşimli kopya yok; restore quota ve yeni EntityRef doğru |
| AC-77 | Hedefte izin/species/collision/kota eksik; ayrıca başka server ve başka DB hedefi seç | Precommit açıklamalı ret/stay; unsupported_transfer_domain; kaynakta gereksiz silme yok; outbox cross-server destek olarak açılmaz |
| AC-78 | Baseline sayfalarını karıştır, eksilt, aynı index farklı digest yolla; açılmış boyut limitini aş; eski ScopeEpoch sayfası ekle | Tek cut/closure doğrulaması; staging atomik; bozuk deneme iptal ve pin cleanup; eksik native binding ile BaselineApplied yok |
| AC-79 | Reliable delta tabanı kaybolsun; eski/yanlış StateAppliedAck gönder; ardışık unreliable motion paketlerini düşür | Yanlış tabana delta apply yok; transport ACK tabanı ilerletmez; full repair; sonraki bağımsız motion örneği kullanılabilir; stale owner/state yine ret |
| AC-80 | Dünya sürekli değişirken düşük bantlı client'ı kat; catch-up history ve global staging kotasını doldur | 30 s deneme ve 60 s'de en çok 2 otomatik yeniden hazırlama; bounded bellek; küçülen scope critical closure'ı korur; yetişmeyen peer açık ret; aktif world ilerler |
| AC-81 | İzinli logical alanı sessizce boz; ayrı koşuda logical state doğruyken native collider binding'ini yanlış tut | View cut/digest mismatch bounded repair; private state sızmaz; native sapma ayrı query/binding kontrolünde yakalanır; güvenli rebind veya scope fence |
| AC-82 | Provider'ı tekrar tekrar çökert, sürüm/ad değiştir, aynı anda çok worker restart iste | 3/5 dk ve global restart/RAM sınırı; 1/2/4 s+jitter, cooldown/probe bütçesi; churn kota sıfırlamaz; core committed sonuçları korunur |
| AC-83 | Actor/resource/world ingress, job ve completion limitlerini ayrı/birlikte doldur; rastgele geçersiz OperationId spam'i üret | Admission öncesi kapasite/izin ret; limitsiz receipt/audit yok; committed kayıt kaybolmaz; kontrol/retire ilerler; ölçülen byte/item/age tavanları |
| AC-84 | Offline ecology açık/kapalı profilde UTC'yi ileri/geri al, DST ve tekrar işlenmiş pencere ekle | Varsayılan ilerleme yok; negatif/büyük fark politikası ve anomali; aynı pencere tek doğum/yaş etkisi; ControlTime lease değişmez; client saati etkisiz |
| AC-85 | Eski tür/rig içeren backup ve offline pet bırak; iki yeni release sonrası GC, disk/host kaybı ve restore provası yap | Exact artifact/schema closure pinli; backup manifest doğrulanır; eksik kritik world açılmaz; eksik birey açık karantina; process-crash ve disk-loss RPO ayrı raporlu |
| AC-86 | Küçültülmüş test sayaçlarını sınırına getir; pending durable commit sırasında graceful shutdown ve deadline kill uygula | Wrap/alias yok, kontrollü yeni epoch; claim erken bırakılmaz; unresolved sonuç rollback diye dönülmez; restart receipt/journal reconcile; eski callback ret |
| AC-87 | Transfer/spawn reserve TTL dolmasını commit submission ve protected quota geçişiyle yarıştır; restart'ta RAM sayacını kaybet | Kesin precommit rezerv bırakılır; uncertain/committed phase recovery'ye gider; persistent occupancy'den kota yeniden kurulur; owned birey silinmez/çift spawn yok |
| AC-88 | P1 living profilinde N1/N2, DB timeout, worker crash, owner kaybı, restore ve resync fault dizisini 8 saat çalıştır; native client'ı dahil et | Tek etki/tek birey/tek writer invariantlarında sıfır ihlal; bounded queue/restart/retention; recovery süresi ve unavailable scope raporlu; fault bitince tanımlı kapıyla açılır veya failed kalır |

Sözleşme sahipleri: [zaman](../architecture/time-fencing.md), [arıza/işlem kurtarma](../architecture/failure-recovery.md), [replikasyon ve transfer](../networking/consistency-recovery.md). ADR-29–32 ve R-21–23 bu 18 yeni senaryoya bağlanır. Clock/DB/codec testleri headless yürütülebilir; suspend, controller fallback ve native binding kanıtı gerçek client ister. Bu tablo tam senaryo hedefidir; D1'deki sınırlı clock/lease/queue alt testleri aşağıdaki bölümde ayrıca belirtilir.

## V0.7 native SDK ve adapter kabulü

AC-89'un [D1-N1 build alt kümesi](../development/d1-native-dependency.md) uygulandı: x86 layout/link, source/patch/recipe ve archive retleri, private include kontrolü, gerçek CMake x64/eksik-source retleri. Native sınıflar genişledikçe header/library kanıtı yenilenir. AC-90'ın [dosya/image alt kümesi](../development/d1-engine-preflight.md) ve [oyun dışı bootstrap DLL](../development/d1-bootstrap-module.md) ABI/ret/stop alt testleri ve [ilk create-debug süreç görüntüsü](../development/d1-suspended-process.md) uygulandı; unpack/ABI/DLL load/hook önkoşulu kısmı açıktır. AC-91–96 **planlandı, çalıştırılmadı**. [SDK sözleşmesi](../architecture/native-sdk-integration.md), [D1 uygulama planı](../development/d1-engine-integration.md), ADR-35 ve R-01a/b/c, R-02a/b/c, R-04a ile eşlenir. Bütün native deneyler seçilmiş yerel executable ve exact build ile yapılır; bilinmeyen binary testi için GTA'ya tahmini adresle erişilmez.

| ID / katman | Kurulum ve işlem | Beklenen sonuç / kanıt |
|---|---|---|
| AC-89 / static-build | Pinlenmiş SDK ve private x86 compile/link probe; yanlış/missing digest, farklı checkout library'si, yanlış mimari ve public vendor include ekle | Lock/build gate reddeder; doğru varyantta header/layout/link kanıtı; core/Host/server bağımlılık grafiğinde SDK yok. Vendor compiler/CRT ayarları core'a taşınmaz. Probe başarısı GTA uyumu sayılmaz. |
| AC-90 / contract-native | Fake PE/profile corpus ve gerçek local preflight; version marker aynıyken dosya hash'i farklı, truncated/wrong arch, sınır dışı symbol ve değişmiş mapped hook site dene | SDK bağımsız ret; callback/native call/patch kaydı sıfır. Gerçek image fingerprint ve kullanılan symbol kanıtları eşleşmeden binding modülü çalışmaz. Fake image ve SEC_IMAGE_NO_EXECUTE testi gerçek native davranışın yerine geçmez. ADR-37: dört disk/image anchor eşleşmesi yalnız observation sonucudur, canAttach=false; marker aynı ve hash farklı fixture reddedilir. |
| AC-91 / native | Seçilen her profilde en az 20 process başlat/kapat; hook kurulumunun ortasında hata, dış hook çakışması ve aktif callback sırasında stop uygula | Kısmi kurulum güvenle geri alınır; başkasının patch'i ezilmez; bounded drain ve callback/quiescence log'u; freed modüle çağrı yok. Temiz session/process stop ile dinamik DLL unload sonuçları ayrı kaydedilir; ikincisi kanıtsızsa capability kapalıdır. |
| AC-92 / contract-native | x86 adapter/x64 Host gerçek IPC'sinde major mismatch, fazla/kesik frame, replay sequence, eski session/resource epoch ve yeniden açılmış endpoint dene | Boyut/kimlik/ABI denetimi native çağrıdan önce ret verir; payload adres/SDK nesnesi taşımaz; başlangıç/bitiş ve sonuç korelasyonu kayıtlı. Foundation stdin fixture'ı bu testi geçmiş sayılmaz. |
| AC-93 / native | No-op/read-only komut yükü; Host'u kapat, kuyrukları doldur, tick stall/suspend/resume ve yanlış thread çağrısı uygula | Güvenli faz/thread izi; p50/p95/p99/max süre ve queue/drop değerleri; main loop I/O beklemez. Eski lease/komut resume'da uygulanmaz; readiness yenilenene kadar yazım fenced. AC-71/72 controller alt testlerinin tamamı kapanmaz. |
| AC-94 / native | Tek native entity türünde en az 100 create/apply/retire; slot reuse, pool full, geciken job, recursive/double destroy ve binding generation sınırı | Eski EntityRef yeni nesneye bağlanmaz; double free yok; owned binding/ref sayıları başlangıca döner; native tür ve property capability sınırı kayıtlı. Diğer ped/araç/obje sınıfları otomatik açılmaz. |
| AC-95 / native | Bir yerel stok modelle en az 100 request/readiness/bind/release; cancel, entity retire, catalog/resource epoch değişimi ve düşük bütçe | Late completion yeni entity'ye bağlanmaz; owned lease/ref sayısı dengeli; kritik collision hazır olmadan görünür activation yok; bellek/frame eğilimi raporlu. Native cache'in her döngüde sıfır olması beklenmez; özel parser/cook ayrıca sınanır. |
| AC-96 / static-contract-native | Dependency commit/patch/recipe, adapter artifact, engine profile veya hook setini değiştir; eski EvidenceRecord'u sun. Önceki lock/artifact ile kontrollü yeni process başlatmayı dene | Eski kapsam yeni profile verified vermez; etkilenen testler yeniden istenir. Source/header/library/notice birbiriyle tutarlı eski set korunur; geri dönüş state/schema ve release güveni şartlarını aşmaz. Canlı DLL hot swap veya yalnız hash değiştirerek geçiş yok. |

20 ve 100 tekrar sayıları bu aşamanın **asgari test hedefleridir**, ölçülmüş başarı veya uzun soak garantisi değildir. Ret ve rollback oracle'ı source kontrolüyle birlikte gerçek diagnostic/callback/patch/registry sayaçlarıdır. Process normal çıkışı tek başına patch restore kanıtı değildir. Desteklenmeyen profile ait hook testi çalıştırılmaz ve `unsupported/not_run` sonucu açık tutulur.

## D1'de başlayan alt testler

AC-34/35/45/71/72/83/86 ile ilişkili metadata, yerel codec, queue ve saat/lease alt testleri [D1 foundation kanıtında](../development/d1-foundation.md) kaynak ve platformla eşlenmiştir. Bunlar AC'nin bütün kurulum/sonuçlarının geçtiği anlamına gelmez. İki process fixture'ı gerçek GTA client'ı veya GNS server değildir; R-01 ve native/OS/security kapıları açık kalır.

## Test uygulamasının sınırı

Headless network botu protokol ve sunucu yükünü ölçer; native render, collision, animation ve GPU kanıtı üretmez. AC-04/11/16/19 için gerçek client zorunludur. R-03 saldırı testleri controlled fixture üzerinde yapılır; başka kullanıcı cihazı veya harici sunucu hedeflenmez.

Her test raporu build/engine/catalog, hardware, network profile, seed, expected/observed, log referansları ve pass/fail içerir. Desteklenmeyen capability nedeniyle çalışmayan senaryo “geçti” sayılmaz; ilgili release kapsamı daraltılır.

## Ana sözleşmelere bağ

AC-01–05/12/13/16–18: [otorite](../networking/authority.md), [yıkım](../assets/destruction.md), [kalıcılık](../networking/persistence.md). AC-06/07/11/14: [asset hattı](../assets/build-pipeline.md), [catalog geçişi](../assets/versioning-overrides.md). AC-08/09/19: [resource lifecycle](../resources/lifecycle-hot-reload.md), [security](../resources/security-native.md). AC-21/23/24: [kit'ler](../gameplay/sandbox-kits.md).

## AC-90 — Kod 0.1.4 pre-user-code alt testi

[Askıda süreç raporu](../development/d1-suspended-process.md): exact dosya eşleşmeden child oluşturulmaz; doğru dosyada create-debug event file ID/base/header/dört anchor eşleşmesi aranır. Farklı file identity, boş/taşan read, yanlış thread, stop sonrası read ve opt-in eksikliği ret verir. Kendi fixture'ımızın normal koşusu canary üretirken 12 observer çevriminde main canary oluşmaz, exit doğrulanır ve handle sayısı sabit kalır; exception unwind da child'ı kapatır. Gerçek GTA sonuçları ayrı scope/artifact ile kayıtlıdır. İlk debug olayı gözlemi unpack/ABI/DLL/hook veya AC-91'in 20 initialized process koşusu yerine sayılmaz.

## AC-90 — Kod 0.1.5 native başlangıç envanteri alt testi

Raw padding farkı için file-backed sınırlar içindeki metadata notu kabul edilir; overlap/taşma retleri sürer ve loader izni false kalır. İlk CLI exception-filter hatası ayrıca regresyonla kapatılır.

[Yeni corpus ve rapor](../development/d1-native-startup.md); native EXE/DLL ayrımı, yanlış mimari, entry/TLS hedefinin data veya image dışına çıkması, VA underflow, import path/case/terminator/kota hatası, bozuk yerel aday, cycle ve değişen dosya hash'ini sınar. Bozuk aday metadataComplete=false/exit 1 üretir; tam statik alt kapsam exit 3 verir; canAdvanceToLoader/canAttach false kalır. Gerçek Windows resolution, runtime DLL başlangıcı ve oyun modu çalışması bu alt testlerden geçmiş sayılmaz. N2/R-01a/b ve AC-91 açık.

## AC-90 — Kod 0.1.6 loader mapping alt senaryoları

[Loader kanıtı](../development/d1-loader-observation.md): normal fixture'da DLL/TLS/main üç canary oluşur; aynı fixture bounded observer'da breakpoint adayına gelince hiçbiri oluşmaz ve child exit doğrulanır. Boş/eksik pin, aynı ad/hash fakat farklı file ID, yanlış executable, invalid/missing pin, byte/event/module budget ve yanlış owner thread ret alır. Tekrar run ileri gitmez; 12 warm çevrim handle artışı oluşturmaz. Altı CLI negatif testi bilinmeyen hash/mimari/header/eksik dosya/opt-in için child oluşmadığını doğrular. Gerçek GTA'daki apphelp mapping ret sonucu kabul politikasının alt kanıtıdır; AC-90 initialized kısmı ve AC-91–96 geçmez. Null event handle/API timeout/kill failure injection bu yeni suite'in ölçülmüş kapsamına girmez.

## AC-90 — Kod 0.1.7 prepared mapping policy

[Policy kabulü](../development/d1-loader-policy.md): exact recipe fixture breakpoint adayını açar; engine/hash/boyut/origin/duplicate/path/bütçe uyuşmazlığında hiçbir partial pin seti verilemez. Yeni reviewed CLI bilinmeyen exe'yi child öncesi reddeder. Gerçek, hash'i bilinen GTA kopyasında eksik veya bir byte değişmiş vorbisfile.dll de child öncesi ret verir. Original-context AcLayers ret ve farklı private-context Debug 22/Release 21 mapping/breakpoint adayı ayrı outcome'dur. DLL/TLS/main canary ve 12 warm observer regresyonları korunur. AC-90 initialization alt kısmı, AC-91–96 ve N3 bu sonuçla geçmez.

## AC-90 — Kod 0.1.8 launch context alt kabulü

[Context corpus](../development/d1-launch-context.md): environment freeze/Unicode/equals/drive entry; duplicate/NUL/limit ve relative/missing/file cwd reddi; hatalı context child yaratmama; own DLL'nin explicit PATH ve cwd'den çözülmesi; canary'siz held start ve 12 warm cycle; CLI secret redaction/unknown engine reddi. Bunların geçmesi yalnız başlatma girdisi alt kanıtıdır; gerçek GTA initialization, SAEX DLL, hook/ABI ve bütün AC-90 kapanmaz.

0.1.8 explicit context gözleminde beklenmeyen OS debug olayının tanısı için loader çıktısına lastEventCode/lastEventThreadId eklendi. Son alınan olay metadata'sıdır; initialization kanıtı veya olayın devamına izin değildir. Child öncesi retlerde ikisi de sıfırdır. Unknown event terminal ret davranışı ve incelenmiş DLL policy değişmedi.

## AC-90 — Kod 0.1.9 modül lifecycle alt kabulü

[Ledger corpus](../development/d1-loader-lifecycle.md): load/unload/same-base reuse, eski ID reddi, duplicate-active ve unknown/duplicate unload atomik ret, event/wrap/base/limit corpus'u, 64 lifetime mapping ve budget iadesi olmaması. OS fixture metadata ve canary'leri ile gerçek GTA unload ayrı kanıtlardır. Gerçek remap gözlenmemişse metadata başarısı bunu doğrulanmış yapmaz; initialization/SAEX DLL/ABI ve AC-90 bütünü açık kalır.

## AC-90 — Kod 0.1.10 statik linkage alt kabulü

[22 kayıtlı corpus](../development/d1-native-linkage.md): exact isim/case/hint ve ordinal; named/ordinal hole; sorted alias; RVA delay ve legacy ret; bound IAT lookup yokluğu; yanlış PE/path/candidate; toplam 4096 normal+delay limit; bozuk table/string/termination/overflow; zero-fill data; çözümlenmeyen name/ordinal forwarder ve directory sonu; absent module/export; değişen hash ve byte korunumu; compact binding/CLI 3/1/2. Başarı statik ikili metadata'ya aittir. Gerçek DLL selection/initialization, prototype/ABI ve bütün AC-90 açık kalır. Gerçek kurulum gözlemi ve sentetik negatifler ayrı raporlanır.

## AC-90 — Kod 0.1.11 entry-boundary alt kabulü

[12 senaryo + 12 warm çevrim](../development/d1-entry-boundary.md): DLL/TLS marker'ı var, main marker'ı yok; TLS entry mutation 0xCC yürütülmeden HW fault; initializer DebugBreak ve stall terminal ret; 16 byte/zero/end/data-section retleri; eksik fixture pin/event budget; yanlış owner ve terminal tekrar; eski ilk-exception davranışı; warm handle korunumu. CLI unknown engine ve invalid context child öncesi ret verir. Geç register kurulumu başarısızlığının ardından CREATE_PROCESS kurulumu ölçülür; hiçbir failed koşu pass diye yazılmaz. Gerçek GTA entry hit/byte mutation ayrı kaydedilir; unpack, SAEX DLL/ABI ve bütün AC-90 açık kalır.

## Kod 0.1.11 — Entry supplement bağı

Sekiz entry-policy testi base/engine drift, scope/schema/keys/size, system-only origin/duplicate override, hash/path/byte, count/order ve review/id sınırlarını denetler. İlk gerçek imm32 ret negatif kanıt, supplement sonrası entry mutation ayrı alt sonuçtur. [Ayrıntı](../development/d1-entry-boundary.md).

## GitHub kaynak yayını — taşınabilirlik düzeltmesi

Launch-context own fixture için üç ek regresyon: aynı dizinin `\.` yazımı başarılı ve marker doğru cwd içinde; başka mevcut dizin beklentisi exit 72 ve marker yok; snapshot sonrası değişen token ile exit 72 ve marker yok. Mevcut snapshot freeze, retained directory ve 12 warm çevrim kontrolleri kalır. Bootstrap geçersiz reason testi GCC için açık dönüşümden sonra da INTERNAL_ERROR bekler.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.
