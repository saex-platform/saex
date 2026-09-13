# Resource manifesti ve bağımlılık çözümü

Durum: JSON sözleşme taslağı; gerçek manifest dosyası üretilmez. [SDK](runtime-sdk.md) · [Asset kimliği](../assets/manifest-identity.md)

## Manifestin sorumluluğu

Resource'un ne çalıştırdığını, hangi servislere/asset'lere bağlı olduğunu ve ne kadar yetki istediğini açıklar. İstenen capability izin verildiği anlamına gelmez. Sunucu politikası ve client broker daha dar bir grant üretir.

```json
{
  "schemaVersion": 1,
  "id": "sandbox.destructibles",
  "version": "0.1.0",
  "sdk": { "major": 1, "minimumMinor": 0 },
  "runtime": "dotnet-10",
  "contractSchema": "sandbox.destructibles.contracts.v1",
  "entrypoints": {
    "server": "managed/server/Destructibles.dll",
    "client": "managed/client/Destructibles.Client.dll"
  },
  "dependencies": [],
  "requiredServices": ["world.entities.v1", "world.damage.v1"],
  "assetPackages": [
    { "id": "sandbox.props", "version": "1.0.0" }
  ],
  "requestedCapabilities": {
    "server": ["entity.spawn:sandbox.props", "damage.apply:owned"],
    "client": ["input.interaction", "ui.scene", "network.intent:impact"]
  },
  "networkSchemas": [
    {
      "id": "sandbox.destructibles.impact",
      "direction": "client-to-server",
      "payloadSchema": "sandbox.destructibles.impact.v1",
      "capability": "network.intent:impact",
      "reliability": "reliable",
      "requiresEntityState": true,
      "maxBytes": 256,
      "maxPerSecond": 20
    }
  ],
  "budgets": {
    "workerMemoryMiB": 128,
    "serverEntities": 128,
    "pendingCommands": 256
  },
  "lifecycle": {
    "stateSchema": 1,
    "stopPolicy": "release-resource-owned",
    "reloadPolicy": "transactional"
  }
}
```

Bu manifest davranış örneğidir; DLL'ler ve adı geçen paketler bu dizinde mevcut değildir. Byte uzunlukları, hash ve imzalar artifact indeksinde bulunur. Sayısal bütçeler örnek resource bütçeleridir, platformun ölçülmüş tüketimi değildir.

`payloadSchema` sürümlü registry tanımına referanstır; alanlar, tip/boyut ve geçersiz veri davranışı orada tanımlanır. `requiresEntityState` hedef entity'nin StateRevision'ına bağımlı dispatch ister; genel world revision nedeniyle private veri bekletilmez. Gönderen ActorContext payload'dan okunmaz, broker/oturumdan gelir. Bildirilmeyen remote event varsayılan kapalıdır.

## Alan kuralları

Kimlik normalize edilmiş namespace'tir. Version SemVer'dir; authoring bağımlılık aralığı kullanabilir ancak dağıtımda tam sürüm ve içerik özeti lock set ile sabitlenir. Entry path paket kökünün dışına çıkamaz; mutlak path ve case çakışması reddedilir.

Server-only assembly/artifact client dağıtım closure'ına girmez. Shared assembly public schema içerir; parola, bağlantı dizesi veya sunucu sırrı shared'da tutulmaz. “Client'ta kullanılmıyor” denilmesi dosyanın gönderilmesini güvenli yapmaz.

RequiredServices sürümlü arayüzlerdir, resource dosya adları değildir. Bir servis için birden fazla sağlayıcı varsa sunucu lock seti seçimi açıkça yapar. Runtime yükleme sırası kazananı belirlemez.

`contractSchema` şema registry tanımıdır; client/server payload, component mutator ve method izin/bütçe sözleşmelerini bağlar. Server-only schema/assembly'nin client closure'ına girmesi engellenir. Resource manifesti [WorldPlan](../architecture/world-plans.md) girdisidir; plan dünya çapında provider/mutator/schedule çakışmalarını da kontrol eder. İlan edilen statik capability runtime grant yerine geçmez.

## Çözümleme sırası

1. Manifest şemaları, platform ve runtime uyumunu doğrula.
2. Resource ve asset bağımlılık grafiklerini kur.
3. Sürüm aralıklarını kesin lock set'e çöz; döngü, eksik dependency ve çakışmayı reddet.
4. Sunucu policy ile capability grant ve bütçeyi hesapla; eski/yeni worker dahil toplam rezerv admission sınırını doğrula.
5. Asset closure ve engine capability'leri doğrula.
6. Topolojik sırada prepare/start; ters sırada stop.

Optional bağımlılık yüklenmemişse feature kapalıdır; resource bunu capability query ile görür. Optional diye ilan edilmiş bir paketin client'ta zorunlu collision sağlaması çelişkidir ve validation hatasıdır.

## Hata ve genişleme

`dependency_conflict`, `missing_service`, `unsupported_runtime`, `capability_denied`, `asset_not_ready` ayrı nedenlerdir. Hata dependency zinciri ve engelleyen grant ile raporlanır; sırf yüklenebilsin diye sürüm veya güven kuralı gevşetilmez.

Custom metadata `extensions` ad alanında eklenebilir. Engine-affecting alan opsiyonel metadata olarak saklanamaz; version ve capability anlaşmasına dahil olmalıdır. Kabul: AC-06, AC-07, AC-08, AC-09.

## Yaşayan dünya provider sözleşmesi

Population/Agent/Animals resource'ları versioned required/provided service, izinli species/prefab namespace, controller capability, nav/animation/hit closure ve schema tanımlarını ilan eder. Requested capability örnekleri population.policy.write:world, agent.task:owned-domain, animal.spawn:namespace, animal.ownership:domain olabilir; bunlar native capability veya verilmiş grant değildir.

Budgets; active/reserved agent, group member, perception query, path job, behavior transition, IPC ve worker toplamını kapsar. Stop policy; spawn durdurma, task cancellation, protected state adoption ve artifact lease'lerini belirtir. Birden fazla tekil PopulationService sağlayıcısı WorldPlan conflict'idir. Data-only tür paketi entrypoint gerektirmez; özel C# behavior ayrı resource, yeni native controller ayrı güvenilen platform release'idir. [Tanım yolu](../gameplay/animals-species.md), AC-34/56/62/65.

## RecoveryProfile ile zorunlu eşleşme

Manifest; provider lineage, transient/durable işlem sınıfları, clock-kind tanımlı deadline, stop sonrası state adoption ve gereken fallback capability'lerini bildirir. Dünya RecoveryProfile'ı per-resource değerleri process/global tavanla birleştirir; manifest kendi kendine daha geniş restart/queue hakkı vermez. Eksik timeout/aggregate cardinality veya korunacak committed state'e sahipsiz cleanup tanımı validation hatasıdır. Alanlar şema sürümünde açıkça tanımlanır; yukarıdaki kavramsal JSON tam production manifesti değildir.

[Kurtarma](../architecture/failure-recovery.md) circuit breaker ve admitted operation registry kurallarını verir. Data-only species paketinin çalışma kodu veya worker restart budget'ı yoktur; içerik decode/build bütçesi yine vardır. AC-73/82/83.
