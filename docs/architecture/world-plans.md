# WorldPlan: dünyayı çalıştırmadan önce çözümlemek

Durum: ADR-19, sözleşme taslağı. [Ürün yapısı](../platform-blueprint.md) · [Manifest](../resources/manifest.md)

## Sorumluluk

World profile insanın seçtiği oyun tanımıdır. WorldPlan bu tanımın kesin resource/asset sürümleri, servis sağlayıcıları, schema'lar, yetki sınırları ve yürütme kurallarıyla çözümlenmiş immutable sonucudur. PlanCompiler offline ve sunucu başlangıcında kullanılabilir; oyuncunun GTA sürecinde derleme yapılmaz.

“Compiler” burada paket/sözleşme çözümleme ve doğrulama aracıdır; C# veya C++ derleyicisinin yerine geçmez. İlk biçim JSON tabanlı şema taslağıdır, yeni bir betik dili tasarlanmıyor. Canonical serialization ve digest R-13'te sabitlenecek.

## Girdiler ve çıktı

| Girdi | Planın çözmesi gereken konu |
|---|---|
| WorldProfile | Map, controller, population, combat, environment, UI ve opsiyonel servis seçimi |
| Resource lock | Kesin package/artifact, taraf, SDK/schema ve bağımlılık sürümü |
| Asset lock / catalog | Kesin içerik, varyant, collision ve prefab closure'ı |
| ContractSchema registry | Alan, command/event, mutator, görünürlük ve persistence kuralları |
| PolicyProfile | Yetki üst sınırı, instance politikası, bütçe ve kabul sınırları |
| EngineSupport profile | Gereken adapter capability'leri ve yayın için gerekli conformance kanıtı |
| Migration set | Eski plan/catalog/schema'dan geçiş yolları ve restart sınıfı |
| RecoveryProfile | Clock provider, timeout türü, admission/queue/restart tavanı, health/fence kapsamı ve restore garantisi |

WorldPlan en az `planSchema`, `WorldPlanDigest`, `CatalogRevision`, `ContractDigest`, `GameplayDigest`, resource lock, provider map, mutator map, schedule graph, required capabilities, budget profile, recovery profile ve release compatibility alanlarını taşır. `ReleaseId` planı imzalı dağıtım kümesine bağlar; plan digest'i mutable bir “latest” adresi değildir.

GameplayDigest, istemci ve sunucunun paylaşması gereken **açık gameplay içerik/şema anlamını** tanımlar: collision compatibility, damage/body/controller tanımları ve zorunlu interaction sözleşmeleri. Server sırları, gerçek anahtarlar ve private domain verileri digest girdisi olarak dağıtılmaz. Server-only politika/assembly lock'u ayrı ServerPlanDigest ile denetlenir. İstemci, görmediği server implementasyonunu doğruladığını iddia etmez.

## Çözümleme akışı

1. Kimlik, sürüm aralığı ve girdi boyutlarını doğrula; package/asset dependency DAG'larını çöz.
2. Map/prefab varyantlarını düzleştir; her alanın kaynak izini koru.
3. Her required service için world'ün seçtiği sağlayıcıyı bul. Tek sağlayıcı beklenen hizmette iki aday varsa otomatik yükleme sırası seçme.
4. Her authoritative component alanı için bir mutator domain belirle. Birden fazla yetkili resource aynı alanı farklı kurallarla yazmak istiyorsa araya tek domain servisi veya ilan edilmiş sıralı reducer koy.
5. System read/write ve before/after bağımlılıklarını çöz; döngü, bilinmeyen sistem veya uyumsuz phase hatadır.
6. Network schema, private/public görünürlük ve client/server closure'ını ayır. Required collider'ı optional download sayan planı reddet.
7. Engine capability ve simulation-mode gereksinimlerini profil kanıtıyla karşılaştır. Plan oluşturulabilir ama sadece deneysel capability içeren dünya üretim için eligible sayılmaz.
8. Statik kaynak tahminini, recovery profile'ı ve admission tavanını kontrol et; bound olmayan script maliyetini “0” sayma. Peer/actor/resource/world/process limitlerinin birlikte uygulanabilirliği gerekir. Saat türü, zorunlu fallback veya toplam kuyruk tavanı eksikse ret; canlı cihaz bütçesi açılışta yeniden ölçülür.
9. Migration diff, plan digest'leri ve diagnostics üret. İmza veya activate işlemi otomatik yapılmaz.

## Hata açıklaması ve arayüzler

`PlanCompiler.Resolve`, `PlanValidator.Check`, `PlanDiff.Compare` ve `PlanInspector.Explain` kavramsal arayüzlerdir. Girdi yolları paket ad alanlarına göredir. Hata; `code, worldProfile, fieldPath, sourcePackage, dependencyPath, requiredCapability, suggestedResolution` içerir. Öneri otomatik izin genişletmez.

Örnek: `door.open` power servisini zorunlu istiyor, fakat Racing profili power sağlayıcısı yüklemiyor. Çözüm compile aşamasında `missing_provider` olur; ilk oyuncu düğmeye basana kadar saklanmaz. `Explain(door.massKg)` ise seçilmiş prefab tabanını, uygulanan varyantı, kaynak dosyasını ve clamp kuralını gösterir.

## Runtime anlaşması

PopulationService provider map'i pedestrians/road-traffic/air-traffic/wildlife için bağımsız seçim taşır. Her kanal kapalı olabilir; hizmeti zorunlu isteyen consumer varsa eksik provider compile hatasıdır. Animals isteğe bağlıdır; Species/Breed/Morphology/Rig/Animation/Locomotion/Behavior closure'ı aynı resolver'dan geçer. Yeni türün mevcut controller ile çalışması metadata doğrulamasına ek R-19 kanıtı gerektirir.

Runtime PopulationPolicy aggregate'ı StateRevision taşır; dünya/bölge/saat yoğunluk değişimleri bu state'i günceller. Plan `mutableFields`, izinli preset/tür havuzu, min/max, quota tavanı ve persistence politikasını sınırlar. Aynı öncelikte çakışan zone patch'i veya kapanınca sahipsiz kalacak controller doğrulama hatasıdır. [Nüfus sözleşmesi](../gameplay/population-traffic.md), AC-46/47/48/56/65.

Sunucu planı doğruladıktan sonra mevcut activation/restore sözleşmesini yürütür. WorldOffer, CatalogRevision yanında açık client ContractDigest ve GameplayDigest'i içerir. Client kendi runtime/engine capability'si ve hazırlanmış artifact'leriyle uygunluk bildirir. Digest uyuşmazlığı değişmeyen paketi aynı isimle yükleyerek geçilemez.

WorldPlan'daki statik yetki sınırı runtime grant değildir. Oyuncunun hakkı, güncel resource epoch'u ve server policy her komutta tekrar değerlendirilir. Client'ın doğru digest bildirmesi dürüst fizik, değiştirilmemiş işlem veya güçlü anti-cheat kanıtı değildir.

## Değişiklik, genişleme ve kabul

Runtime state değişimi WorldPlan'ı değiştirmez. Kod/schema/statik kural sözleşmesi/asset mapping değişimi yeni release/plan üretir ve [aktivasyon sözleşmesini](activation-contracts.md) kullanır. Planın açıkça mutable ilan ettiği alanlar, izinli aralık ve havuzlar içinde runtime state olarak değişir; yeni provider, yetki, controller veya tavan ekleyemez. Kozmetik varyant değişebilir; aynı GameplayDigest'e sahip olma iddiası authoring validation ve native conformance ister.

Yeni servis veya mod, schema ve provider sözleşmesi yayımlar; core'a oyun türü özel bağımlılık eklemez. Tool plugin'leri build ortamında bile açık güven profiliyle çalışır. [Zaman](time-fencing.md) ve [kurtarma](failure-recovery.md) zorunlu recovery alanlarını tanımlar. Runtime admin ayarı sadece önceden izinli alt sınırda kapasiteyi azaltabilir; recovery garantisini zayıflatmak yeni plan/ADR ve conformance gerektirir. AC-34/35/38 ve AC-82/83/88; R-13/21/22/23. Plan derlenmesi gerçek native arıza testinin yerine geçmez.

## D1 metadata validator sınırı

[WorldPlanValidator](../../managed/Saex.Tools/WorldPlanValidator.cs), schemaVersion=1 foundation metadata'sını işler: bounded/strict JSON, unique provider/mutator, resource+service dependency DAG ve kararlı prepare sırası, asset DAG ve transitif critical optional ret, declared capability ve başlangıç RecoveryProfile tavanları. [Fixture](../../samples/foundation/world-plan.json) gerçek package/artifact içermez; SHA-256 alanlarının biçimi kontrol edilir, dosya içeriği doğrulanmaz.

Çıktıdaki sourceDigest raw input hash'idir; canonical WorldPlanDigest değildir. valid yalnız bu metadata alt kümesi için, productionEligible her zaman false'tur. Tam compiler, SemVer resolver, trait/schedule/schema üretimi, native EvidenceRecord ve runtime activation mevcut değildir. AC-34/R-13 bazı negatif fixture'larla sınandı; bütün kapı kapanmadı. [D1 açıklaması](../development/d1-foundation.md).
