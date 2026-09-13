# İsteğe bağlı sandbox oyun sistemleri

Durum: oyun paketi taslakları; core gereksinimi değildir. [Vizyon](../vision.md) · [Tam dönüşüm](total-conversion.md)

## Paket sınırı

Platform world/entity, resource, asset, replication ve persistence araçlarını sunar. Aşağıdaki sistemler bu araçlarla geliştirilecek referans paketlerdir. Hiçbir sunucu bütün paketleri kurmak zorunda değildir. Core'da sabit para, polis faction'ı veya roleplay karakter kaydı bulunmaz.

| Paket | Sözleşme ve durum | Bağımlılık / hata davranışı |
|---|---|---|
| Inventory | ItemDefinition, ContainerId, atomic transfer | Persistence; tekrar istekte çift eşya yok |
| Economy | Ledger/transaction, fiyat, üretim-tüketim | Domain transaction; client bakiyesi authoritative değil |
| Ownership | Kullanıcı/grup hakkı, lease, erişim | Identity ve permissions; network owner'dan ayrı |
| Construction | Placement önerisi, collision, maliyet, commit | Map/prefab ve inventory; overlap reddi |
| Vehicles | Motor, yakıt, akü, aktarma, lastik, hasar ve bakım | VehicleDefinition; native alan yeteneği yoksa açık hata |
| Weapons | Silah profili, atış/şarjör/hasar ve cooldown | Combat authority; animation event tek karar değil |
| Agents / AI | Ortak algı, görev, navigasyon ve locomotion sözleşmesi | [AgentService](agents-navigation.md); bounded işler, tek accepted state |
| Population / Traffic | Ayrı yaya, kara/hava trafiği ve wildlife kanalları | [PopulationService](population-traffic.md); runtime policy, protected drain, kota |
| Animals | Species/Breed/Morphology/Rig/Animation/Behavior bileşimi | [Hayvanlar](animals-species.md); uygunluk kanıtı olmayan controller kapalı |
| Companion / Husbandry / Ecology / Mounts | Evcil takip, çiftlik, ileri yaşam ve binme ayrı paketler | Animals/ownership; R-19/20 alt kabiliyetleri ve atomik domain işlemleri |
| Needs / Schedules | Açlık, uyku, güvenlik, vardiya | AI; görünmeyen ajan için düşük frekans |
| Factions / Crime | Üyelik, ilişki, suç olayı, tepki politikası | Yetki/AI; sabit polis davranışı core'a girmez |
| Business / Housing | İşletme/ev, kiralama, erişim ve stok | Ownership/economy; finansal sonuç atomic |
| Missions | Hedef, koşul, aktör, ödül, başarısızlık | Instance ve transaction; idempotent ödül |
| Dynamic City | Elektrik ve bölgesel tepkiler; PopulationService'e izinli policy isteği | Environment/AI; ikinci bağımsız trafik üreticisi veya mutator oluşturmaz |
| Racing / Survival | Oyun döngüsü, checkpoint veya ihtiyaç kuralları | Yalnız gereken kit'ler |

## AI'ın üç katmanı

Karar katmanı server'da hedef ve izin üretir. Navigasyon katmanı collision/nav revision'a göre rota arar. Sunum/locomotion adapter'ı ped/animasyon hareketini gösterir. NPC'nin ekranda yürümesi kararın client'a bırakıldığı anlamına gelmez.

Örnek işçi ajanı; vardiya, görev hedefi, envanter referansı ve ihtiyaç düzeyi taşır. Oyuncu uzaktayken dakikalık soyut ilerleme, yakındayken nav ve animasyon kullanılabilir. Geçişte konum/amaç tutarlılığı doğrulanır; göz önünde rastgele ışınlanma olmaz.

Hayvan; skinned model, uygun skeleton/IFP, hit/collision profili ve locomotion controller gerektirir. DFF/TXD dağıtmak tek başına yeni canlı türünün hareket ve davranış desteğini tamamlamaz. [Species/Breed sözleşmesi](animals-species.md) tür, cins, görünüm ve kalıcı bireyi ayırır. Ground, flying, aquatic ve mounting ayrı capability'lerdir; genel NPC desteğinden türetilemez.

## Ekonomi ve yaşayan şehir

Üretim→stok→satış→tüketim akışı domain olaylarıyla ilerler. Yakıt azlığının fiyatı artırması configurable economy policy'dir. Oyuncu tanker teslimi ödülü ile stok artışı aynı işlem sınırına veya outbox garantisine bağlanır.

Crime artınca police response değişebilir; bu sürekli tüm NPC'leri her tick yeniden hesaplatmaz. Bölgesel toplulaştırma, sabit güncelleme aralığı ve bütçeli görev planlama kullanılır. Oyun dengesi için katsayılar veri paketidir.

## Mission generator

Template; önkoşul, objective graph, aktör/prefab havuzu, konum kısıtı, başarısızlık/timeout ve reward policy taşır. Üretilen görev instance kimliğiyle saklanır. Yeniden bağlanan oyuncu görev durumunu alır; yalnız “başladı” olayı yeterli değildir.

Görev rastgeleliği server seed'iyle içerik seçebilir. Bütün native fizik deterministik olmaz. Resource reload mission schema migration gerektirir; görev ödülü daha önce verildiyse restart sonrası tekrarlanmaz.

## Modlar arası genişleme

Her kit sürümlü servis ve component şemasını yayımlar. Ekonomi sağlayıcısını değiştirmek araç fiziği protokolünü değiştirmez. DB abstraction sadece bağlantı sarmalayıcısı değildir; atomic transfer gibi domain garantileri sağlayıcı testleriyle korunur.

Kabul: AC-21 sandbox inşa/ekonomi, AC-23 NPC/mission, AC-24 farklı oyun profilleri. Bu paketlerin varlığı yol haritasının çekirdek kanıt kapılarını atlamaz.

## Ortak aksiyon paketi

[Actions/Effects](actions-effects.md), kapı açma, onarım, eşya kullanma ve yakıt doldurma için typed koşul/süre/maliyet/cancel/cue tanımları sunacak isteğe bağlı kit'tir. Inventory veya economy gibi domain provider'larını yetki ve transaction sınırlarıyla çağırır. Bu paketi kullanmayan oyunlar doğrudan C# SDK komutlarıyla geliştirilebilir. Dynamic City ve AI follow-up'ları [ChangeSet](../networking/world-change-projections.md) bounded cause/dependency kurallarına uyar.
