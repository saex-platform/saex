# Harita hücreleri, prefab ve yerleşim sistemi

Durum: sözleşme taslağı. [World modeli](../architecture/entities-worlds.md) · [Streaming](streaming-budgets.md)

## Haritanın yapısı

Map package; world bounds, hücreler, placement'lar, LOD grupları, iç mekânlar, portallar, yollar/nav-data ve environment bölgelerini taşır. Hücre fiziksel dağıtım/streaming birimidir; oyuncunun bulunduğu instance ile aynı şey değildir.

İlk açık dünya authoring grid'i 256 metre hücredir. Bu bir ağ mesafesi veya GTA native grid iddiası değil paketleme varsayılanıdır. Büyük yapı birden fazla hücreye temas edebilir; dependency pin'i bütün gerekli collision bölümünü kapsar.

## Yerleşim kaydı

```json
{
  "schemaVersion": 1,
  "mapId": "sandbox.harbor",
  "cellId": "harbor-east",
  "placements": [
    {
      "placementId": "lamp-east-entrance-01",
      "prefab": "sandbox.props:streetlamp/standard",
      "position": [120.0, 64.0, 8.0],
      "rotation": [0.0, 0.0, 0.0, 1.0],
      "scale": [1.0, 1.0, 1.0],
      "persistence": "durable"
    }
  ]
}
```

Koordinatlar örnektir; GTA haritasında doğrulanmış bir konum değildir. Collision, animasyon ve query verileri aynı transform dönüşümünü kullanır. Nonuniform scale desteklenmiyorsa importer reddeder veya önceden cook eder; engine'e belirsiz biçimde iletmez.

PlacementId yeri değişince değişmez. Stok import'ta stable ID kayıt tablosu üretilir ve korunur; sırf dosyadaki sıra veya en yakın koordinatla her export'ta yeniden kimlik verilmez.

## Prefab bileşimi

Prefab alt entity'ler ve component'ler içerebilir: kapı gövdesi, menteşe, etkileşim hacmi, ses, ışık ve erişim kontrolü. Authoring composition DAG'ı çözüldükten sonra immutable runtime tanımı elde edilir. Döngü ve sınırsız prefab nesting reddedilir.

Placement override başlangıç transform/etiket gibi ilan edilmiş alanları değiştirebilir. Physics profile'ı instance düzeyinde değiştirmenin native etkisi [malzeme kurallarına](physics-materials.md) tabidir. Runtime sağlık değişikliği authoring override değildir.

## Stok dünyayı dönüştürme

`gta.base` stok referansları içerir. Bir total conversion profili stok map/population sağlayıcılarını devreden çıkarıp özel map package'ını seçebilir. Native streaming yeniden yüklemesinde stok objelerin geri doğmasını önlemek R-05'in ana kanıtıdır.

Stok bina, lamba ve dummy object aynı native sınıfta olmayabilir. Remove/replace/destruction capability'si placement sınıfı bazında raporlanır. Desteklenmeyen sınıfta “tam harita override” başarılı gösterilmez.

## Portal ve içerik bariyeri

Portal giriş/çıkış transform'u, hedef world/interior, görünürlük bağı, akustik geçirgenlik ve gerekli asset closure taşır. Kapı açıkken arkasındaki collider henüz yüklenmemişse oyuncu geçişe alınmaz. Açık portal through-view için düşük detay render gerekebilir; bu da ayrı dependency'dir.

Interiors aynı world içinde oda grafiği olabilir. Ayrı instance'a geçiş gerekiyorsa source action'lar durur, target ready olur, world/scope epoch commit edilir. Ses ve hit ray'leri eski instance'ta kalmaz.

## Navigasyon ve yıkım

Nav-data collision sürümüne bağlıdır. Duvar yıkılınca nav obstacle revision değişir; NPC path işi eski nav revision için tamamlanırsa yeniden değerlendirilir. İlk yaklaşım dynamic obstacle ve sınırlı bağlantı güncellemesidir; tüm şehrin her darbede navmesh rebuild'i değildir.

Server henüz geçerli yol hesaplayamadıysa NPC güvenli bekler veya yerel avoidance kullanır; sağlam varsayılan duvarın içinden geçecek yeni rota uydurmaz. Büyük harita yeniden cook'u world release olarak yapılır.

## Runtime inşa ve kayıt

Oyuncu placement önerir; sunucu hak, bölge, çakışma, maliyet ve bütçeyi doğrular. Kabulde kalıcı dynamic placement kimliği atanır. Client editör preview'ı authoritative collider değildir.

Bir yapının silinmesi child ownership ve içindeki oyuncu/araç politikasıyla atomik değerlendirilir. Örtüşen yeni collider, güvenli onarım kuralıyla aynı şekilde engellenir. Kabul: AC-04, AC-05, AC-10, AC-17, AC-21.

## Katmanlı authoring ve izole deneme

Harita/prefab authoring varyantları [composition](composition-variants.md) ile çözülür; PlacementId bir field path veya liste indeksinden türetilmez. [Studio provası](../platform/studio-release.md) seçilmiş durumu yeni world namespace ile test eder. Authoring diff üretime yeni release olarak alınabilir; test dünyasının canlı bakiye/hasar state'i otomatik merge edilmez. Büyük bir harita farklı hücrelere bölünse de tek etkileşim grubunun gameplay kuralları ayrışamaz.

## Trafik şeritleri, habitat ve üretim yerleri

Map-cell; karayolu lane/connectivity/signal conflict zone, air corridor/runway/helipad ve habitat/spawn-set referanslarını taşıyabilir. [Population](../gameplay/population-traffic.md) SpawnSlotId'yi editörce korunmuş kimlikle kullanır; hücre sınırı veya export sırası iki nüfus üreticisi yaratmaz. Slot bir canlı bireyin kalıcı kimliği değildir.

[Agent navigation](../gameplay/agents-navigation.md) pedestrian, vehicle, quadruped ve uçuş/su traversal'ını ayrı ölçü/kısıt profilleriyle sorgular. Güncel collision/nav hücre generation'ı route validity'ye girer; hızlı uçuş daha erken closure hazırlığı ister. Stok path verisinin import/dağıtım kaynağı R-10/12, native population bastırma R-17, aircraft R-18, hayvan R-19 kapısıdır. AC-47/52/53/54/55/68.
