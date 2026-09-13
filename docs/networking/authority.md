# Otorite, intent ve güven sınırları

Durum: sözleşme taslağı. [Mimari](../architecture/overview.md) · [Fizik](physics.md)

## Temel karar

Sunucu entity kimliğinin, ömrünün, kalıcı hasarın ve oyun sonuçlarının son karar merciidir. Kademeli otorite, ilk aşamada bütün GTA hareket/fiziğinin sunucuda yeniden çalıştırıldığı anlamına gelmez. Bir client önerisini kontrol etmek ile aynı fizik modelini bağımsız hesaplamak farklı güvence düzeyleridir.

| Alan | Üretici | Sunucunun kontrolü | Bilinen sınır |
|---|---|---|---|
| Spawn / silme | Yetkili resource | Capability, kota, dünya, yaşam döngüsü | Native client görüntüsü tek başına spawn kanıtı değil |
| Yaya hareketi | Oyuncu input ve hareket önerisi | Hız/ivme, zaman, dünya, izinli hareket modu, collision sorgusu | Tüm native görevler birebir doğrulanamaz |
| Araç hareketi | Süreli owner | Sürücü hakkı, profil, hız sınırı, collision ve epoch | Modifiye edilmiş handling yalnız client beyanından anlaşılamaz |
| Hasar / kırılma | Sunucu veya impact intent | Kaynak, menzil, cooldown, profil, current state | Yakınlık tek başına gerçek darbe kanıtı değil |
| Ekonomi / eşya | Server resource | Transaction, sahiplik ve idempotency | Client payload'daki ödül/miktar esas alınmaz |
| NPC hedefi | Server AI | Davranış ve görev yetkisi | Animasyon/motion client önerisi olabilir |
| Platform rigid body | Server solver veya lease owner | Moduna göre simülasyon/doğrulama | Native ve farklı solver sonuçları eşit varsayılmaz |
| Efekt / ses | Onaylı olay veya yerel kozmetik | Olay kimliği, bütçe, hedef kapsam | Kozmetik çıktı hasar veya collision üretmez |

## Intent işlem sırası

Session doğrulama → mesajın izin verilen oturum aşaması → boyut/şema → resource/oyuncu yetkisi → world/epoch/generation → sequence/replay → oran sınırı → mekânsal ve davranışsal kontrol → gameplay değerlendirme → commit → sonuç.

İstemci `playerId`, `damageAmount` veya `resourceName` yazarak yetki seçemez. Gönderen oyuncu sunucu oturumundan, resource kimliği broker bağlantısından alınır. Hasar miktarını kabul edilmiş silah/impact profili hesaplar.

Komutlar için iki eşzamanlılık şekli vardır: additive hasar sunucunun kabul sırasına göre güncel state üzerinde hesaplanır; “durumu X'ten Y'ye değiştir” komutu beklenen revision ile CAS benzeri önkoşul ister. Aynı OperationId/payload tekrar gelirse önceki sonuç döner, hasar ikinci kez uygulanmaz. RequestId yalnız bağlantı korelasyonudur. Durable pending aggregate'a ikinci mutasyon önceki işlem sonuçlanınca değerlendirilir. [İşlem sırası](../architecture/activation-contracts.md)

## Simülasyon sahipliği

Lease: entity/group reference, owner session, owner epoch, authority mode, sunucuya yerel monoton ControlDeadline, gerekli catalog/state revision ve izinli alanlar. İlk ölçüm profili sabit 2 saniye lease ve 500 ms yenileme kullanır; otomatik RTT uyarlaması v1 sözleşmesinde yoktur. Süreyi oyun tick'inden bağımsız ControlTime belirler; client saati uzatamaz. Ham monoton timestamp makineler arasında karşılaştırılmaz. Sunucu lease süresi dolmadan revoke edebilir. Stall/resume ve yerel watchdog kuralı [zaman sözleşmesindedir](../architecture/time-fencing.md); AC-71/72, R-21.

Atama yalnız mesafeye göre yapılmaz: Joined/Applied aşaması, ilgili world/scope, gerekli collision closure ve StateRevision, connection kalitesi, yük ve sürücü/etkileşim önceliği dikkate alınır. Lease hakları client tarafından üçüncü bir client'a aktarılamaz.

MTA ped/vehicle syncer kararlılığı [referans alınarak](../references/mtasa-architecture.md) geçerli owner minimum 2 saniye tutulur; mesafe sebebiyle değiştirme ancak aday en az 20 metre daha yakınsa ve uygunluk 500 ms sürmüşse önerilir. Bunlar ilk ölçüm hedefidir. Disconnect, yanlış world, revoke, güven ihlali ve sürücü kontrol değişimi bu beklemeyi geçersiz kılar. Birbirine bağlı araç/römork veya desteklenen constraint grubu tek authority group epoch'u altında devredilir; split/merge sunucu state transaction'ıdır. Desteklenmeyen cross-owner constraint kabul edilmez.

Devir sırasında sunucu son kabul edilmiş snapshot'ı esas alır, owner epoch artırır, yeni tarafa durum verir. Yeni taraf ready onayı vermeden yazmaya başlayamaz. Eski owner paketleri epoch ve deadline ile reddedilir. Aradaki sürede controller'ın kanıtlanmış emniyet davranışı veya desteklenen server solver çalışır; uçuş/aktif temas için genel “son transform'u dondur” varsayımı yapılmaz. Güvenli ara davranış yoksa ilgili interaction scope fence edilir ve capability üretime açılmaz. World writer takeover bununla aynı işlem değildir; [WriterTerm ve kesin eski-process duruşu](../architecture/failure-recovery.md) gerekir.

## İtiraz, düzeltme ve yaptırım

Hatalı hareket birikimli ölçüm ve bağlamla değerlendirilir. Tek gecikme sıçraması otomatik hile hükmü değildir. Karşılıklar: state düzeltme, simülasyon yetkisi kaldırma, aksiyon reddi ve operatöre sinyal. Anti-cheat politika paketi bunlardan sonra oturum yaptırımına karar verebilir.

Kritik modlar `server-simulated` gerektirebilir. Oyun profili sadece `validated-native` sunabiliyorsa bu mod yüklenmez. Güven seviyesi capability anlaşmasında ve profiler'da görünür olmalıdır.

## Kabul

AC-01 aynı anda hasar; AC-02 tekrar/sıra; AC-03 sahip kopması; AC-09 izinsiz komut; AC-13 hileli hareket önerisi. Her sonuç request, actor, owner epoch, ret nedeni ve authoritative revision ile izlenebilir.

## NPC, trafik ve hayvan otoritesi

[PopulationService](../gameplay/population-traffic.md) policy/spawn/retire/slot/kotanın, [AgentService](../gameplay/agents-navigation.md) accepted goal/task/group state'inin server provider'ıdır. Animal ownership/bond/loot domain yetkisi ayrı kalır. Native locomotion owner'a süreli öneri yetkisi verir; owner hedefi, ödülü, türü veya gameplay sahibini değiştiremez.

Araç/sürücü ve gerekli attachment/koltuk grubunda tek owner epoch; sosyal sürüde ayrı üyeler için ayrı lease mümkündür. Görev, native kontrol/animation phase ve accepted motion birlikte taşınır; observer bağımsız AI sonucunu kesinleştirmez. Lease kaybında uçuş veya hayvan controller'ının kanıtlanmış fallback'i gerekir; yoksa ilgili capability kapalıdır. Tam sync ortak accepted state ve ölçülen yakınsamadır, client doğrulamasının yakalayamadığı native manipülasyonlar devam eden güven sınırıdır. AC-49/50/51/58/59.

## D1 lease gate kapsamı

[LeaseAuthority](../../src/core/lease_authority.cpp) entity/session/OwnerEpoch/catalog/StateRevision, izinli field mask, monotonic deadline ve motion/renewal sequence doğrulamasını kodlar. Issuer tek güvenilen core thread'idir; readiness bool'u client'ın doğrulanmamış paketinden yetki seçmez. Saat bozulması/explicit resume fence ve yeni grant'te artan owner epoch test edildi. Hareket/çarpışma/hasar doğrulaması, otomatik syncer seçimi, gerçek network ve native controller emniyeti henüz yoktur. [D1 kanıtı](../development/d1-foundation.md), AC-71/72 alt kümesi.
