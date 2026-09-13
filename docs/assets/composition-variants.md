# Asset bileşimi, varyantlar ve açıklanabilir prefab'lar

Durum: ADR-21, authoring sözleşmesi. [Katalog](catalog.md) · [WorldPlan](../architecture/world-plans.md)

## Sorumluluk ve referans

İçerik üreticisi taban modeli çoğaltmadan aynı nesnenin malzeme, bağlantı, davranış ve görünüm varyantlarını hazırlayabilmelidir. Bir alanın neden belirli bir değer aldığı editörde görülebilmelidir. Runtime, çözülmemiş authoring zinciri yerine kesin ve sınırlı prefab tanımı tüketir.

OpenUSD'nin katman, referans ve varyant yaklaşımı authoring için referanstır. SAEX ilk runtime'ına USD/Hydra yükleme bağımlılığı eklemiyoruz; kendi şemamızın sınırlarını tanımlıyoruz. OpenUSD'nin namespace değişimlerinin override'ları etkileyebildiği açıklaması, SAEX'te kalıcı PlacementId ile authoring alan kimliğini ayrı tutma gerekçesidir. [OpenUSD açıklaması](https://openusd.org/release/intro.html)

## Bileşim kuralları

| Kavram | Anlam ve sınır |
|---|---|
| Base prefab | Component ve public customization noktaları için tek taban |
| Trait | Yeniden kullanılabilir component/ayar grubu; native kod değildir |
| Socket | Adlandırılmış attachment noktası, transform ve uyumlu child türleri |
| Variant | İlan edilmiş seçim ekseninde bir değer; ör. material, damage style, visual detail |
| Patch | Açık field/element kimliğine sınırlı değişiklik; kaynak ve gerekçe kaydı |
| Resolved prefab | Bileşim sonrası immutable component/asset closure |
| Field provenance | Son değerin taban/trait/variant/patch kaynağı ve doğrulama izi |

Authoring sırası base → trait birleşimi → seçili variant → açık world patch → izinli placement başlangıç override'ıdır. Bu sıra sınırsız son-yazan-kazanır değildir: bir önceki katmanın `customizable` ilan ettiği alanın override'ı ve aynı katman içindeki çatışma çözümü açık olmalıdır. İki trait aynı unique component'i farklı değerle getiriyorsa sonuç conflict'tir. Listeler indeksle ezilmez; öğe kimliğiyle add/remove/replace yapılır.

Composition DAG'ı döngüsüzdür. Başlangıç limitleri derinlik 8, prefab başına 32 trait ve çözülmüş component metadata 256 KiB deney sınırıdır; mesh/audio bytes bu metadata bütçesi değildir. Aşım build hatasıdır. Bir field'ı yeniden adlandırmak eski patch'in başka alanı değiştirmesine yol açmaz; schema migration veya açık hata gerekir.

## Statik tanım ve dünya durumu

Authoring variant bir eşyanın başlangıç malzemesini belirleyebilir; mevcut kırık durumunu sıfırlayamaz. Runtime hasar, envanter, ownership ve current animation yine entity state'idir. Onarım veya malzeme değiştirme gameplay işlemidir; aynı effect'i prefab patch'iyle yetkisiz taklit etmek mümkün değildir.

Socket bağlantısı kalıcı ownership veya mutator yetkisi taşımaz. Parent-child DAG ile constraint graph ayrıdır. Skinned attachment'ta socket kemik/skeleton revision'ına bağlanır; kemik bulunamazsa child sessizce world origin'e bırakılmaz. Collision/nonuniform scale desteği adapter capability'sine bağlıdır.

## Görsel kalite ve gameplay eşdeğerliği

QualityVariant; texture/mip, render LOD ve cosmetic effect bütçesini değiştirebilir. GameplayVariant ise collider, mass, damage, controller veya gameplay animation timing'i değiştirir; bu anlam katalog/planın GameplayDigest hesabına dahil edilir. Katalog birden fazla gameplay tanımını birlikte içerebilir; entity için izinli seçim server state'idir, client kalite seçimi değildir. Yeni gameplay tanımı veya değişmiş içerik yeni digest/release gerektirir; hazır tanımlar arasındaki yetkili state geçişi tek başına global katalog değiştirmez. İki kalite varyantı engelin görünürlüğü veya hit alanı üzerinden avantaj yaratamaz; compiler bunu metadata'dan tek başına kanıtlayamaz, conformance sahnesi gerekir.

Client istediği görsel kaliteyi seçebilir, sunucu gerekli gameplay compatibility grubunu zorunlu kılar. Aynı gameplay collision closure'ı cihaz bütçesine sığmıyorsa session admission kısıtlanır; gizli düşük kaliteli collider sürümü oluşturulmaz. Native model pool'u ve GPU residency ayrı limitler olarak korunur.

## Örnek: kasa ailesi

`crate.base`; mesh, collider, body ve interaction component'lerini verir. `trait.destructible` health/state/parça profilini, `trait.container` inventory servis bağını ekler. `wood` seçimi PhysicsMaterial/DestructionProfile referanslarını, `low-visual` seçimi yalnız render artifact'lerini değiştirir. World patch, yalnız public customization alanından kasa kapasitesini seçebilir.

Inventory kit'i kullanılmayan yarış profilinde container trait'i seçilmez. Aynı model resource çoğaltılmadan dekoratif, kırılabilir veya depolama işlevli prefab üretilebilir. İki trait'in Lifetime alanı çakışıyorsa tool kaynak yollarını gösterir ve açık çözüm ister; runtime sırası gizli tercih yapmaz.

## Arayüz, veri akışı ve hata

`ComposePrefab`, `ResolveVariant`, `ExplainField`, `ValidateSocketClosure` ve `CompareGameplayMeaning` offline arayüz taslaklarıdır. Kaynak → canonical composition → capability/closure validation → resolved artifact → WorldPlan → runtime lease akışı kullanılır.

Hatalar `composition_cycle`, `trait_conflict`, `unknown_patch_target`, `incompatible_socket`, `gameplay_variant_mismatch` ve `composition_budget_exceeded` olarak ayrılır. Normalize edilmiş alan yolları ve kaynak paket zinciri raporlanır. İleride DCC/USD/glTF import adapter'ı eklenebilir; import sonucu aynı kuralları geçer. AC-38 ve AC-41 kabulüdür; R-13/R-04 doğrulamaları gereklidir.

## Hayvan türü, cinsi ve appearance bileşimi

[AnimalDefinition](../gameplay/animals-species.md) tek Species tabanı + isteğe bağlı Breed + morphology/rig/locomotion/behavior + görünüm seçimini çözer. Tür veya cins ID'si runtime birey değildir. Aynı canine rig'deki renk değişimi uygun AppearanceVariant olabilir; büyük beden, farklı hız/hit proxy veya action timing gameplay varyantıdır.

Species'in izin vermediği boyut/rig/behavior override'ı, iki trait'in aynı gait/action alanını çelişkili yazması veya eksik mouth/back socket build hatasıdır. Görsel scale tek başına animal büyümesi değildir. Habitat/avcı-av ilişkileri karşılıklı hard dependency döngüsü açmaz. Yeni tür capability/ref closure ve conformance ile kabul edilir; AC-56/57/67.
