# Fizik malzemeleri, gövde profilleri ve native eşleme

Durum: sözleşme taslağı. [Fizik](../networking/physics.md) · [Yıkım](destruction.md)

## Ayrı tanımlar

RenderMaterial görünümü, PhysicsMaterial temas davranışını, BodyProfile kütle/şekil/hareket türünü, DestructionProfile hasar ve parça geçişlerini tanımlar. Bir TXD değiştirmek sürtünmeyi veya kırılma eşiğini değiştirmez.

| Profil alanı | Anlam | Uygulama sorumlusu |
|---|---|---|
| motionType | static, kinematic veya dynamic | Solver/adapter capability |
| massKg / centerOfMass | Kütle ve kütle merkezi | Body başına destek gerektirir |
| friction / restitution | Temas sürtünmesi ve sıçrama | Solver malzeme eşleme |
| linear/angular damping | Hız sönümlemesi | İlgili solver |
| buoyancyProfile | Suda davranış | Native veya custom capability |
| collisionLayer/mask | Hangi sınıflarla temas | Authoritative world policy |
| sleepPolicy | Uyuma ve uyanma eşikleri | Solver ve replication |
| attachment/joint | Sabitlenme, menteşe, kopma sınırı | Joint capability |
| damageResponse | Darbe/ateş/patlama profiline bağ | Gameplay hasar sistemi |
| impactAudio/effect | Görsel/işitsel tepki | Client presentation |

Birimler ve değer aralıkları solver adapter'ında dönüştürülür. GTA'nın native sayısal alanlarına fiziksel SI anlamı atamak doğrulama gerektirir. “Mass=10” her motorda aynı temas sonucunu garanti etmez.

## Malzeme örnekleri

| Malzeme | Tasarım amacı | Yıkım ve collision |
|---|---|---|
| glass | Hafif, darbede kırılabilir | Cam panel collision'ı gider; küçük parçalar cosmetic |
| wood | Hasar biriktirme, kıymık | Kasa state değişimi; büyük tahta gameplay body olabilir |
| metal | Deformasyon görünümü veya kopma | Direk sabitlemesi kopar; büyük gövde ağda kalır |
| concrete | Yüksek eşik, parçalı duvar | Hazırlanmış segmentler; keyfî mesh kesme yok |
| fabric | Hafif görsel tepki | İlk sürümde gerçek kumaş solver'ı garanti edilmez |

Bu kategoriler fiziksel doğruluk iddiası değil, içerik yazarının tutarlı profil oluşturması içindir. Her profil min/max clamp ve desteklediği damageType listesine sahip olur.

## Örnek BodyProfile

```json
{
  "schemaVersion": 1,
  "id": "sandbox.props:body/wood-crate",
  "simulationMode": "server-simulated",
  "motionType": "dynamic",
  "massKg": 12,
  "physicsMaterial": "sandbox.props:material/wood-contact",
  "collisionShape": "sandbox.props:crate/collision",
  "collisionLayer": "world-prop",
  "collisionMask": ["world-static", "vehicle", "character", "world-prop"],
  "sleepPolicy": "normal",
  "requiresCapabilities": ["physics.rigid-body.v1"]
}
```

`physicsMaterial` referansı friction/restitution gibi temas alanlarını taşır; BodyProfile mass/motion ve collision ilişkisini taşır. Bu referans da açıklamalı taslaktır, gerçek asset yaratılmaz. R-06 server solver capability geçmeden profil çalıştırılabilir ilan edilmez. Parametreler GTA objesi için doğrulanmış değerler değildir.

## Native gruplarla ilişki

MTA fizik grup API'sinde kütle, esneklik, devrilme/kopma ve çarpışma hasarı gibi ayarlar bulunması, native davranışların araştırılabilir olduğunu gösterir. Özellikler grup/model düzeyinde olabilir. SAEX bunları entity başına setter gibi sunmadan etkilerinin kapsamını doğrular. [MTA referansı](https://wiki.multitheftauto.com/wiki/EngineSetObjectGroupPhysicalProperty)

Uyarlama tablosu her alan için `exact`, `approximated`, `group-scoped` veya `unsupported` bildirir. Exact yalnız doğrulanmış eşleme için kullanılır. Grup paylaşımına müdahale eden instance override, izole profil oluşturulamıyorsa reddedilir.

## Runtime değişikliği

Sürtünme veya mass değişimi visual hot reload değildir. Sunucu yeni body profile revision'ını kilitler, affected body'leri quiesce eder, collision/profil bariyerini geçirir ve authority epoch ile uygular. Başka client'ın eski profile göre sürmeye devam etmesi kabul edilmez.

Constraint graph döngüsü gerekiyorsa solver bunu açıkça desteklemelidir; prefab attachment DAG kuralı fizik joint graph'ıyla aynı şey değildir. Dynamic compound/hinge desteği capability ile açılır.

Kabul: AC-16 cam/ahşap/metal/beton farkları, AC-17 onarım örtüşmesi, AC-13 impulse kötüye kullanımı. Ret nedeni mümkün değilse sessiz native approximation yapılmaz.

## Hayvan bedeni ve temas profilleri

MorphologyProfile; BodyProfile, hit proxy ve traversal ölçülerine bağlanır. Kürk materyali sürtünme/kütleyi dolaylı değiştiremez. Human ped capsule'ı custom hayvan gövdesine otomatik uygun sayılmaz; ped-backed/native ve platform controller temas eşlemesi R-19 kapsamındadır. Corpse gameplay engeliyse ortak collider state'i gerekir; kozmetik ragdoll farklı client'larda hasar üretemez.

Büyüme, yük veya beden varyantı kütle/hit/collision değiştiriyorsa gameplay geçişidir; occupied volume ve profil/epoch revalidation gerekir. Bindirilmiş sürücü, eyer veya ip attachment'ı tek başına fizik joint desteği sağlamaz. [Hayvan sözleşmesi](../gameplay/animals-species.md), AC-57/58/67.
