# V0.3 mimari geliştirme kaydı

Tarih: 12 Eylül 2026. Durum: tasarım güncellemesi. [V0.2 denetimi](consistency-audit.md) tarihsel kayıttır; [güncel rapor](documentation-review.md) kontrolleri gösterir.

## Gereksinim ve seçim

Kullanıcı yapının daha güçlü ve yeni bir platform olarak tasarlanmasını istedi. V0.3, feature sayısını büyütmenin yanında paketlerin birlikte çalışması, geliştiricinin hata bulması ve yayının güvenle işletilmesi üzerinde ilerler. C++20/C#/.NET10, GNS + HTTPS, kademeli otorite ve hazırlanan parçalarla yıkım kararları korunmuştur.

| Önceki eksik | Eklenen sözleşme | Ölçülebilir sonuç / kapı |
|---|---|---|
| Dünya düzeyinde paket/provider/mutator birleşimi açık değildi | WorldPlan ve PlanCompiler | İlk oyuncudan önce conflict/missing provider ret; AC-34 |
| API/schema doğrulaması farklı yüzeylerde tekrarlanabilirdi | ContractSchema ve üretilecek binding/validator taslağı | SDK/wire/inspector sözleşmesi eşleşir; AC-35 |
| Core tek writer idi, paralel iş sırası yeterince ayrıntılı değildi | Fazlar, immutable read set, deadline ve command sırası | Tamamlanma sırası yetkisiz sonucu belirlemez; AC-36/45 |
| Resource bir başka güçlü provider üzerinden yetki yükseltebilirdi | ActorContext koruma ve açık domain delegation | Yetkisiz cross-domain komut reddi; AC-37 |
| Prefab tekrar kullanımı ve alan kaynağı sınırlıydı | Traits, sockets, varyant, provenance ve conflict kuralları | Yanlış override build'de bulunur; AC-38 |
| Ortak etkileşim örüntüleri her kit'te yeniden yazılacaktı | İsteğe bağlı Actions/Effects paketi | İptal, ücret, cooldown ve tekrar sonucu ortak test; AC-39 |
| Collider/nav/audio/domain tepkilerinin ortak sıra modeli eksikti | CriticalSet, ChangeSet ve revision bağlı projeksiyon | Stale yol/efekt ortak state'i bozmaz; AC-40 |
| Ağ state'i ile render kalite seçimi yalnız temel ayrımla anlatılmıştı | RepresentationSet ve GameplayDigest eşdeğerliği | Düşük kalite farklı gameplay üretmez; AC-41 |
| Test dünyası ve yayın provası ayrıntısızdı | RehearsalWorld, fixtures ve release diff | Test sonucu üretime para/ödül yazamaz; AC-42 |
| Paket imzasının eski metadata/release riskleri ayrılmamıştı | TUF uyumlu updater şartı, rol ve freshness ayrımı | Eski metadata ret; planlı rollback yeni release; AC-43 |
| Destek etiketi ile test kanıtı tam bağlanmamıştı | ConformanceProfile ve scoped EvidenceRecord | Eski engine/build kanıtı yanlış kapsama taşınamaz; AC-44 |

## Kaynakların kullanımı

MTA v0.2 incelemesi korunur. Yeni authoring referansı [OpenUSD](https://openusd.org/release/intro.html), aksiyon/etki ayrımı [Epic GAS](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine), update güveni [TUF](https://theupdateframework.github.io/specification/latest/) resmî belgeleridir. Bu projelerin runtime'ları SAEX'e eklenmedi. Scheduler ve WorldPlan bu belgelerin hazır ürünü gibi sunulmaz; SAEX tasarım kararlarıdır.

## Kapsam ve maliyet kararı

WorldPlan/schema/scheduler temel alt kümesi ilk uygulama aşamalarına bağlandı. Genel aksiyon paketi opsiyoneldir. Görsel Studio, geniş efekt editörü ve gelişmiş prova yönetimi sonraki aşamalara bırakıldı. Yeni platform yönü; GTA'yı yeniden yazma, sınırsız oyuncu, bütün fizik için deterministik replay veya otomatik uyumluluk vaadi getirmedi.

Ek araçlar test ve bakım maliyeti doğurur. R-13–16 ile parser, generator, scheduler, projection ve release çalışma maliyeti ayrı ölçülecek. Başarısızlıkta özellik experimental/unsupported kalacak veya kapsamı daralacak. D0 belgeleri güncellendi; gelecekteki uygulama kapıları geçilmiş sayılmadı.

## V0.4 ile ilişki

Bu belge v0.3 değişikliklerinin tarihsel kaydıdır. V0.4 eki; [Population/Agent/Animals entegrasyonunda](population-animal-integration.md) yer alır. WorldPlan'ın statik sözleşmesi ile izinli canlı PopulationPolicy alanları ayrılmış, üç ana gameplay sözleşmesi ve AC-46–70 eklenmiştir. V0.3 kanıt kayıtları yeni native trafik/hayvan yeteneklerinin doğrulandığı anlamına gelmez.
