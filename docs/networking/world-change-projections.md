# Bir dünya değişikliğinin bütün sistemlere yansıması

Durum: ADR-23, normatif sözleşme taslağı. [Yıkım](../assets/destruction.md) · [Aktivasyon](../architecture/activation-contracts.md)

## Sorumluluk

Duvar kırıldığında collider, yol bulma, görünüm ve ses geçişi birbirinden habersiz kalmamalıdır. Bunun için state transaction'dan türetilen `ChangeSet` ve projeksiyon sözleşmeleri tanımlanır. ChangeSet ikinci bir otorite veya yeni genel transaction numarası değildir; mevcut TransactionId/WorldRevision ile ilişkilidir.

Kural: anlık ortak gameplay sonucuyla gecikmeli hazırlanabilen türetilmiş bilgi ayrı sınıflandırılır. Tüm şehrin navmesh'ini tek DB transaction içinde yeniden hesaplamak gerekmez. Geçerli sonuç hazır değilken tüketicinin ne yapacağı açık olmalıdır.

## Kritik ve türetilmiş durum

| Sınıf | Örnek | Tutarlılık kuralı |
|---|---|---|
| CriticalSet | Damage state, collider varlığı, gameplay fragment'leri, anlık gameplay engeli | Aynı authoritative transaction ve bounded aktivasyon |
| Derived projection | Nav obstacle/route cache, akustik görünüm, render temsil önbelleği | Source revision setine bağlı; stale çıktı uygulanmaz |
| Domain follow-up | Güç şebekesi yeniden dengeleme, görev ilerleme, ekonomi tepkisi | Yetkili domain'de yeni idempotent transaction; bounded sıra |
| PresentationCue | Toz, kırılma sesi, opsiyonel ışık efekti | State sonrası veya ilan edilmiş tahmin; kaybı gameplay state'i bozmaz |

Işığın sönmesi gameplay görünürlüğünü veya etkileşim yetkisini değiştiriyorsa ilgili logical state CriticalSet'e girer. Sadece render parlama efekti gecikebilir. “Türetilmiş” etiketi authoritative hasar/kapı kilidi güncellemesini keyfî geciktirme izni değildir.

## ChangeSet ve projection kaydı

ChangeSet; TransactionId, WorldEpoch, WorldRevision, CauseId, etkilenen aggregate'lar, değişen public component alanları ve spatial bounds taşır. Private değişiklikler yetkisiz tüketiciye veya client'a aktarılmaz. Güvenilen server projector dahi ihtiyacı olan domain iznine sahiptir.

`ProjectionDescriptor` input component/domain, output schema, scope, exact/conservative/deferred policy, max staleness ve bütçe taşır. `ProjectionResult` kaynak EntityRef/StateRevision kümesi veya hücre projection generation'ı, CatalogRevision, projection schema ve çıktı digest'i içerir. Birden fazla entity'ye bağlı iş yalnız tek parent revision'ı kontrol ederek kabul edilmez.

WorldPlan dependency graph'ı döngüsüz kurar. Bir domain başka domain'e geri etki ediyorsa doğrudan sonsuz callback zinciri kurulmaz; tick'e ertelenmiş, cause/depth/work limitli follow-up kullanılır. Kontrolsüz feedback cycle validation hatasıdır. Structural çökme veya elektrik şebekesi döngüsü solver/domain kendi sınırlı algoritması içinde çözülebilir; keyfî event recursion değildir.

## Duvar yıkımı örneği

1. Sunucu DamageState, açıklık/collider ve gerekli fragment'leri aynı kabul işleminde değiştirir; snapshot yeni gerçeği taşır.
2. İlgili nav hücresi, ses portalı ve görsel görünüm için invalidation üretilir.
3. NPC eski nav sonucunu kullanacaksa path'in mevcut collision ile geçerliliği yeniden sorgulanır. Yeni bir engel eklendiyse güncel collision hemen durdurur; kaldırılan engelin açtığı rota hazır değilse kısa bekleme kabul edilir.
4. Client gameplay açısından doğru collider/mesh grubunu hazırlar ve uygular; büyük native swap sığmıyorsa mevcut activation fence kuralı kullanılır.
5. Akustik model gecikmişse yeni yetkisiz voice route açılmaz. World/üyelik filtreleri her durumda server'da geçerlidir; acoustics özel konuşma yetkisi vermez.
6. İlgili görev resource'u accepted cause'dan kendi OperationId'sini üretir; aynı ChangeSet yeniden işlense de görev ödülü tekrarlanmaz.

Bu akış bütün projeksiyonların aynı frame'de hazır olmasını vaat etmez. Max staleness world profilinde seçilir; ilk deneyde nav-cache 500 ms, kozmetik akustik güncelleme 250 ms hedefidir. Gameplay açısından kullanılamayan stale sonuç bekleme/ret getirir; süre aşımı eski sonucu doğru saymaz.

## Scope, kalite ve admission

Network AOI authoritative state alıcılarını; RepresentationSet client'ın native/GPU varlıklarını belirler. RepresentationSet yalnız yetki verilmiş state'den üretilebilir. Yetkisiz veri “belki sonra lazım olur” diye düşük detayla gönderilmez.

Player/vehicle/attachment/collision interaction closure'ı bütçe için birlikte değerlendirilir. Native nesneyi tam ayrıntı, görsel uzak temsil veya yalnız mantıksal state düzeyinde tutmak mümkün olabilir. Görsel uzak temsil collision gerektiren yakın etkileşime geçerken readiness tamamlanmalıdır. Server/client hangi entity'nin gerçekten etkileşime açık olduğunu bilir.

Kalite azaltma texture/LOD/cosmetic ve uzak örnekleme ile başlar. Kritik collider'ı bir client'ta düşürerek farklı oyun kuralı üretmek geçerli değildir. World kapasitesi tüm zorunlu closure'ı taşıyamıyorsa yeni entity/oyuncu admission sınırı uygulanır; ölçülen hedef karşılanmadı diye raporlanır. [Streaming bütçesi](../assets/streaming-budgets.md)

## Arıza, rebuild ve kabul

Projector crash sonrası türetilmiş cache geçerli state'den yeniden üretilebilir. Durable primary state geri alınmaz. Gerekli domain follow-up kalıcıysa outbox/receipt ile tekrar teslim edilir; yalnız RAM event'i ile garanti edilmez. ProjectionDefinition değişimi ilgili cache generation'ını geçersiz kılar; eski sonuç yeni schema'ya uygulanmaz.

API aileleri `RegisterProjection`, `Invalidate`, `ScheduleRebuild`, `QueryReadiness` ve `ExplainDependency` olarak tasarlanır. Projection hattı raw GTA pointer taşımaz. AC-40 stale nav ve follower, AC-41 kalite/closure, AC-23 AI, AC-25 çapraz lane ve AC-45 bounded işler kabulüdür. R-15 entegrasyonu geçmeden genel dünya tepki grafiği destekleniyor sayılmaz.

## AI, trafik ve habitat tüketicileri

Duvar/çit/kapı değişimi road lane ve animal traversal cache'lerini ilgili cell/collider revision setiyle invalid eder. Her bedenin clearance/slope/step sınıfı farklı olabilir. Güncel engel collision'da derhal dikkate alınır; route rebuild beklerken güvenli bekleme/validated avoidance vardır. NPC hedefi veya sürü alarmı yeni bounded domain follow-up'tır; collider transaction'ında bütün şehrin AI'ı hesaplanmaz.

Gameplay ses olayı Perception stimulus üretebilir, client volume/mute ayarı server algısını değiştiremez. Private perception hafızası diagnostic veya projection yoluyla public'e sızmaz. Animal collider/life-state/loot eligibility kritik domain state'idir; kozmetik kürk/ayak sesi aynı durability gereksinimini taşımaz. AC-53/54/58/60/65; [Agent sözleşmesi](../gameplay/agents-navigation.md).
