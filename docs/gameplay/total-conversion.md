# Tam oyun dönüşümü için üç referans taslak

Durum: kullanım senaryosu; çalıştırılabilir gamemode yoktur. [Kit'ler](sandbox-kits.md) · [Motor sınırları](../architecture/engine-adapter.md)

## Dönüşüm profili

World profile hangi map, controller, combat, population, UI, environment ve gameplay sağlayıcısının aktif olduğunu seçer. `gta.base` bütün davranışların mecburi sahibi değildir. Bir sağlayıcı kapatıldığında gerektirdiği hizmeti kim üstlenecek manifestte belirlenir; eksik sağlayıcıyla dünya active olmaz.

## Harbor Sandbox

İlk teknik dikey kesit için küçük özel liman alanı: cam, kasa, çit, lamba ve hazır segmentli duvar. İki oyuncu darbeyle parçalar; üçüncü oyuncu sonradan girer; world restart sonrası yıkım korunur. Harita koordinatları özel test sahnesidir, GTA stok şehrine bağımlı değildir.

| Alan | Profil seçimi |
|---|---|
| Map | Yeniden dağıtılabilir custom map/collision |
| Hareket | İlk aşama doğrulanmış GTA yaya/araç adaptörü |
| Kurallar | Etkileşim ve hasar; ekonomi zorunlu değil |
| UI | Özel interaction prompt, health/state debug panel |
| Yıkım | Beş prefab, durable state ve bounded gameplay debris |
| Voice | Test için opsiyonel proximity |

Bu kesit bütün platformu yaptığımız anlamına gelmez; asset, state, replication, persistence ve worker zincirinin birlikte ispatıdır.

## Racing

Harita, araç handling profili, başlangıç sırası, checkpoint, tur ve bitiş kuralları seçilir. Suç, inventory, housing veya NPC needs yüklenmez. HUD hız/tur/pozisyon gösterir. Server checkpoint sırasını ve zaman aralığını doğrular; client'ın “yarışı kazandım” mesajı sonucu belirlemez.

Handling uyumluluğu için tüm katılımcılar aynı gameplay catalog kullanır. Görsel araç varyantı collision/performans avantajı üretemez. Yarışta advanced server-simulated araç şart koşulursa native validation capability'si bunun yerine kabul edilmez.

## Survival / Construction

Stok HUD ve denetimsiz native population kapatılır; yeni map, inventory, needs, Agent/Animals, inşa ve görev paketleri seçilir. SAEX PopulationPolicy içinde pedestrians/road-traffic/air-traffic kapalı, wildlife istenirse açık seçilebilir. PopulationService'in kapatılması ile yalnız stok üretimin bastırılması farklıdır. Etkileşim input'u ve kamera modlanır; engine kapatma/override kabiliyetleri R-02/05/08/17 üzerinden doğrulanır.

Oyuncu duvar kurar: preview → server alan/ownership/kota/maliyet kontrolü → inventory debit + placement commit → collision/nav güncellemesi → tüm client'larda kabul edilmiş yapı. Biri duvarı yıkar: destruction state değişir, nav geçiş açılır, kalıcı kayıt güncellenir. Aynı anda onarım istenirse server revision/occupied volume denetimi uygulanır.

NPC'ler bütün dünyada full-rate çalışmaz. Görünür ajanlar ayrıntılı, uzak ajanlar düşük frekansta işlenir. Ateşten hasar, duman efekti ve ambient ses üç farklı sözleşme olsa da aynı cause kimliğiyle izlenir.

## Kabul ve hata sınırları

Yaşayan şehir varyantı aynı platformda yaya/kara/hava kanallarını seçer; survival aynı AgentService üzerinde yalnız wildlife ve sahipli hayvan kit'lerini seçebilir. Racing ambient kanalları kapalı tutabilir; scripted yarış NPC'leri mission sınıfında ayrıca tanımlanır. [Nüfus](population-traffic.md), [hayvanlar](animals-species.md), AC-46/63/66. Bu varyantlar yeni çalışan gamemode veya temel D2 yıkım kesitinin ek zorunluluğu değildir.

AC-24; aynı platform build'iyle bu üç profil dependency ve capability doğrulamasından geçmelidir. “GTA tabanı devre dışı” testi stok HUD, istenmeyen trafik spawn'ı ve eski input davranışının geri dönmediğini doğrular.

R-05 stok harita bastırmayı doğrulamazsa Harbor özel izole sahnede ilerler; tüm şehir dönüşümü deneysel kalır. R-08 araç/weapon override'ını doğrulamazsa ilgili profil gereksinimini karşılamayan oturum açılmaz. Belgelenmiş sınır, görünürde çalışan ama farklı client'larda farklı kuralları olan profil yerine tercih edilir.
