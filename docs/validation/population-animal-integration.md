# V0.4 nüfus, trafik ve hayvan entegrasyon kaydı

Tarih: 12 Eylül 2026. Durum: v0.4 tarihsel dokümantasyon revizyonu; gameplay testleri çalıştırılmadı. V0.5 zaman/transfer/restore düzeltmeleri [stabilite denetimindedir](stability-audit.md). [Ana indeks](../../README.md) · [Güncel doğrulama raporu](documentation-review.md)

## Talep ve teslim sınırı

Kullanıcı; yaya, kara/hava trafiğinin ortak online dünyada etkileşimli çalışmasını, dinamik aç/kapat ve yoğunluk ayarlarını, ileride özel hayvan tür/cins/model/AI paketlerinin eklenmesini bütün mimariye işlemeyi istedi. V0.4 bu hedefin veri, süreç, hata ve doğrulama sözleşmesini tanımlar. Kod, executable, asset veya proje iskeleti üretilmedi.

Ana alan sahipleri [PopulationService](../gameplay/population-traffic.md), [AgentService](../gameplay/agents-navigation.md) ve [Animals](../gameplay/animals-species.md) belgeleridir. Ortak kimlik/commit kuralları [aktivasyon sözleşmesinde](../architecture/activation-contracts.md) kalır; yeni gameplay belgeleri ikinci protokol veya alternatif transaction sistemi açmaz.

## Gereksinim → sözleşme → kabul izi

| Gereksinim / giderilen belirsizlik | Güncel sözleşme ve bağlantılar | Kanıt kapısı |
|---|---|---|
| Yaya, road, air ve wildlife bağımsızlığı | Population channels; [WorldPlan](../architecture/world-plans.md), [kit'ler](../gameplay/sandbox-kits.md), [operasyon](../platform/launcher-operations.md) | ADR-25; AC-46/63; R-17/18 |
| Statik plan ile canlı yoğunluk çelişkisi | İlan edilmiş mutable PopulationPolicy; StateRevision, izinli aralık/havuz ve yeni release sınırı | AC-46/48/65 |
| İki oyuncu aynı bölgede çift nüfus üretebilir | Tek director, AOI birleşimi, slot/generation rezervi ve grup kotası | AC-47/68 |
| Yaya off sürücüyü/pilotu silebilir | Kanal kökeni ve farklı ControlClass/Lifetime/persistence | AC-46/49/63 |
| Off sırasında spawn veya sahiplenme yarışı | Commit revalidation, protected drain ve bounded retire; [execution](../architecture/execution-contracts.md) | ADR-28; AC-48/49/59/68 |
| Sadece konum sync yeterli sanılabilir | Task/controller/animation/life/seat state ve nedensel motion; [protokol](../networking/protocol.md), [replikasyon](../networking/replication.md) | ADR-26/28; AC-50/51/61 |
| Native AI server'da çalışıyor sanılabilir | Karar/nav/controller ayrımı ve validated-native güven sınırı; [otorite](../networking/authority.md) | R-10/17/19; AC-50/51/65 |
| Hava trafiği road AI'ın aynı sınıfı sanılabilir | Ayrı koridor, runway/helipad, seyir/kalkış/iniş/loss capability | R-18; AC-51/52/55 |
| Model/IFP ekleme hayvan desteği sanılabilir | Species/Breed/Morphology/Rig/Animation/Locomotion/Behavior ve native conformance | ADR-27; R-19; AC-56/57 |
| Yeni tür için core switch listesi gerekebilir | AssetRef ve typed schema, provider extension; [SDK](../resources/runtime-sdk.md), [manifest](../resources/manifest.md) | AC-56/62/65 |
| Rig ile collider/hit uyuşmazlığı | [Build](../assets/build-pipeline.md), [animation](../assets/animation-audio.md), [body](../assets/physics-materials.md), creature capability | AC-57/58/67 |
| Birey ve tür/appearance karışabilir | PersistentEntityId ayrı; [kimlik](../assets/manifest-identity.md), [bileşim](../assets/composition-variants.md) | AC-56/61/67 |
| Farklı cinsler için ayrı world gerekir sanılabilir | Katalog birden fazla gameplay tanımı içerir; entity seçimi server state, grafik kalite seçimi eşdeğer sunumdur | AC-56/57/67 |
| Sürü, attachment ve ownership karışabilir | Server social group, bağımsız üye lease, gameplay owner ayrı | AC-59/60/66 |
| Ölüm/loot/doğum yeniden üretilebilir | Receipt/slot/cause/domain uniqueness, offline bounded catch-up; [kalıcılık](../networking/persistence.md) | R-20; AC-58/61/68/69 |
| Asset eksikliği farklı collider yaratabilir | HTTPS closure, native admission/Ready/Applied; [streaming](../assets/streaming-budgets.md), [dağıtım](../assets/distribution-cache.md) | AC-52/56/64 |
| Tür kaldırma/reload eski bireyi silebilir | Pinli DefinitionRevision, persistent referans taraması; [versioning](../assets/versioning-overrides.md), [lifecycle](../resources/lifecycle-hot-reload.md) | AC-61/62/67 |
| Uzak AI görünmez saldırı üretebilir | SimulationTier, interaction closure ve uyanma doğrulaması | AC-52/54/64 |
| Yıkım route/algı ile kopuk kalabilir | [ChangeSet](../networking/world-change-projections.md), farklı traversal ölçüsü ve server stimulus | AC-53/54/58 |
| Daha çok NPC mevcut kapasite gibi sunulabilir | [Living profilleri](performance.md), toplam sürücü/pilot/hayvan maliyeti, gerçek native ölçüm | AC-70; R-17–20 |
| Destek etiketi kanıttan geniş olabilir | [Conformance](conformance-contracts.md), [Studio](../platform/studio-release.md), engine/rig/controller kapsamı | AC-44/55/57/70 |

## Doküman grupları boyunca yapılan işlem

| Grup | Entegrasyon |
|---|---|
| Başlangıç/vizyon/terimler/kapsam | V0.4 ürün görünümü, 10 yeni kullanıcı gereksinimi ve ortak terimler |
| Mimari | Süreç/core-provider sınırı, WorldPlan mutable state, entity/control sınıfı, execution/activation |
| Ağ | GNS kararı korunarak typed agent state, authority/owner, baseline/closure, physics ve kalıcılık |
| Asset | Yeni tanım aileleri, kimlik, build/HTTPS/streaming, morphology/rig/animasyon, prefab ve migration |
| Resource | SDK ve manifest, izin/ActorContext, worker crash, adoption ve native extension sınırı |
| Platform | Policy/animal admin, UI ses ayrımı, debug overlay, authoring/release provası |
| Gameplay | Üç yeni ana sözleşme, existing kit/action/total-conversion bağları |
| Doğrulama/kararlar | AC-46–70, ADR-25–28, R-17–20, load karışımları ve conformance |
| Kaynak/geçmiş | Yeni resmî API bulguları; eski v0.2/v0.3 kayıtları tarihsel tutulup güncele bağlandı |
| Yol haritası | Temel D2 yıkımı büyütmeden ground population/animals, sahiplik ve air kolları; ileri ecology ayrı |

Eski belgelerin yalnız sonuna referans eklemek yeterli görülmedi: WorldPlan runtime kural cümlesi, engine capability tablosu, kit provider sahipliği, Survival population seçimi, performans NPC sayacı, aynı dünyada farklı gameplay varyantlarının birlikte seçilmesi ve ana ürün indeksi doğrudan düzeltildi. Her eski belge alanına özgü sözleşme veya güncel kapsam izi taşır. D0 kontrol sonuçları ve sayısal envanter [inceleme raporundadır](documentation-review.md).

## Doğruluk ve kalan araştırma

Birincil kaynaklar: [MTA ped kontrolü](https://wiki.multitheftauto.com/wiki/SetPedControlState), [animasyon replacement](https://wiki.multitheftauto.com/wiki/EngineReplaceAnimation), [model replacement](https://wiki.multitheftauto.com/wiki/EngineReplaceModel), [IFP yükleme](https://wiki.multitheftauto.com/wiki/EngineLoadIFP) ve [FiveM OneSync](https://docs.fivem.net/docs/scripting-reference/onesync/). Bulgular [kaynak envanterine](../sources.md) bağlıdır. İncelenen mekanizmalar SAEX'e hazır native AI/animal kodu vermez.

Bu revizyonda hiçbir R/AC gameplay kapısı geçmiş sayılmaz. Özellikle native nüfus bastırma, güvenli owner-loss uçuşu, farklı hayvan rig/controller/hit geometrisi, büyüme/üreme ve performance deney gerektirir. “Tam doğruluk”, tasarımda tek anlam/izlenebilir kabul hedefidir; gelecekte hatasız yazılım veya sınırsız motor kabiliyeti garantisi değildir.
