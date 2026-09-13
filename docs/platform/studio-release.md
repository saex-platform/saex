# Studio, test dünyası ve güvenilir release akışı

Durum: ADR-24; araç ve işletim taslağı. [Geliştirici araçları](developer-tools.md) · [WorldPlan](../architecture/world-plans.md)

## Sorumluluk

Geliştirici; bir asset alanının kaynağını, bir komutun reddedilme nedenini ve bir release'in dünyada ne değiştireceğini aynı kimlik zincirinden inceleyebilmelidir. Studio bu sözleşmelerin arayüzüdür. İlk araçlar CLI/inspector olabilir; tam görsel editör ilk multiplayer kanıtından önce zorunlu değildir.

## Üretimden ayrı test dünyası

`RehearsalWorld`, seçilmiş snapshot kesiti, anonimleştirilmiş domain fixture'ları, kesin WorldPlan ve catalog ile kurulan ayrı bir world/persistence namespace'idir. Runtime EntityRef'leri yeni WorldId/WorldEpoch alır. PlacementId mantıksal eşlemesi korunabilir; üretim entity referansı test dünyasında komut hedefi olarak kabul edilmez.

Varsayılan dış ağ/DB/webhook, gerçek ödül ve kullanıcı credential erişimi kapalıdır. Harici servisler kontrollü fixture adapter'larıyla temsil edilir. Üretim envanterine, ekonomisine, ses kayıtlarına veya private veriye yazma bağlantısı yoktur. Test worker'ının konfigürasyonla üretim endpoint'ine bağlanması OS/broker politikasıyla da engellenmelidir; yalnız uygulama etiketi yeterli değildir.

Test dünyasını kopyalamak git branch veya Codex task oluşturmak anlamına gelmez; burada ürünün gelecek world fixture özelliği tarif edilir. Aynı GTA sürecinde iki farklı aktif katalog aynı anda simüle ediliyor varsayılmaz. Native A/B testi ayrı kontrollü GTA süreçlerinde veya sırayla aynı profilde yapılır; headless state karşılaştırması native render kanıtı üretmez.

## Kayıt ve yeniden oynatma

Rehearsal kaydı engine/build/plan/catalog/schema, kabul edilmiş command/state/receipt kesiti, seçilmiş motion ve hata enjeksiyon zaman çizelgesini içerir. Private alanlar filtrelenir, ham voice varsayılan kaydedilmez. Kaynak fizik input'larından tüm GTA'yı deterministik yeniden oynatma iddiası yoktur.

Test iki tür olabilir: kayıtlı accepted state'i gösteren presentation replay; veya aynı kontrollü komutları yeni server build'ine verip invariant'ları karşılaştıran davranış provası. İkincisinde ağ sıra/seed, clock ve dış servis fixture'ları sabitlenir; native fizik farkları ayrıca ölçülür. Replay bir anti-cheat kararının tek kanıtı sayılmaz.

## Yayın akışı

| Aşama | Çıktı | Geçiş koşulu |
|---|---|---|
| Author | Kaynak asset/resource ve schema değişikliği | Alan provenance ve paket kaynağı belli |
| Resolve / build | WorldPlan, kesin artifact ve ContractDigest | Static validation; gerekli capability listesi |
| Diff | Kod, schema, gameplay, izin, bütçe ve migration farkı | Değişiklik sınıfı açık; yeni yetkiler mevcut policy'ye sığmalı |
| Rehearse | İzole world'de kabul ve hata senaryoları | İlgili AC sonuçları, unsupported ayrımı ve log |
| Stage | İçerik deposunda tam release kümesi | Hash, boyut, imzalı metadata ve closure tam |
| Activate | Mevcut prepare/Ready/commit/Applied akışı | Migration + katılımcı fencing; uygun world/epoch |
| Observe | Sağlık ve invariant izleme | Yeni hata eşiğinde rollout durur; rollback güvenliği kontrol edilir |

Kod, schema, server policy, asset lock ve WorldPlan tek ReleaseId altında ilişkilidir. Erişim anahtarlarının kendisi release'e paketlenmez. İzin artışı örneğin UI resource'una yeni HTTP hedefi ekliyorsa operator policy buna açık grant vermelidir; paketin kendini güncellemesi yetkiyi otomatik artırmaz.

Canary, ayrı test/üretim world'ü veya aynı uyumlu release'i kullanan bütün oyuncu grubudur. Collision/kurallar değişirken aynı etkileşim alanındaki bir oyuncu eski, diğeri yeni gameplay sürümünde tutulmaz. Cosmetic A/B testi ancak GameplayDigest ve görünür engel anlamı eşdeğerliği doğrulanmışsa mümkündür.

## Metadata güveni ve geri dönüş

Hash doğru içerik byte'ını tanımlar; eski ama imzalı bir release'in tekrar sunulmasını tek başına engellemez. TUF; root/targets/snapshot/timestamp rolleri, sürüm ve geçerlilik kontrolleriyle update güveni için resmî referanstır. [TUF specification](https://theupdateframework.github.io/specification/latest/)

Platform updater için **TUF uyumlu metadata doğrulama** tasarım şartıdır; uygulanacak kütüphane ve entegrasyon R-16'da seçilir, özel kripto yapılmaz. Server asset/resource deposunda da aynı güven rolü ayrımı ve release tutarlılığı uygulanacak; sunucu içerik anahtarı platform native executable güncelleme yetkisi taşıyamaz. Güven kökü ilk kurulum/server trust akışından gelir, indirilen paket kendi kökünü kendiliğinden güvenilir yapamaz.

Yeni indirme/aktivasyonda imza, yetkili namespace, metadata version, expiry, snapshot tutarlılığı ve artifact hash/size kontrol edilir. Güvenilen son metadata sürümü saklanır; saat geri gitmesi ve offline freshness doğrulanamaması explicit hatadır. Mevcut doğrulanmış aktif içerikle devam edilip edilemeyeceği offline policy'ye bağlıdır; yeni otomatik güncelleme “cache var” diye freshness kontrolünü atlamaz.

Planlı rollback, eski metadata'yı tekrar kabul etmek değildir. Yeni ve yetkili release metadata'sı, izin verilen eski artifact kümesini açıkça hedefleyebilir; state migration ve güvenlik alt sürüm sınırı ayrıca geçmelidir. Vulnerable olduğu için yasaklanmış artifact'e rollback yapılmaz. Root/key değişimi TUF doğrulama kurallarına göre yürür; imza güvenilir kod, parser veya sandbox kanıtı değildir. GNS peer identity R-11a bu mekanizmadan ayrı kalır.

## Arayüz, hata ve kapsam

`PrepareRehearsal`, `CompareWorldState`, `ExplainChange`, `ValidateRelease`, `ActivateRelease`, `QueryOperation` ve `CollectDiagnostics` arayüzleri tasarlanır. World diff authoring değişikliğini taşır; test dünyasının canlı para/hasar state'i üretime otomatik merge edilmez.

Rollback ters migration gerektiriyorsa doğrulanmadan otomatik yapılmaz. Commit sonrası hata dünya state'ini silmez; rollout durdurma, affected resource stop, resync veya forward-fix uygulanır. Fixture eksikliği test başarısı değildir. AC-42 izolasyon, AC-43 update/rollback güveni ve AC-44 kanıt kapsamı; R-16 doğrulaması gerekir. İlk headless prova D3, görsel karşılaştırma ve Studio D4–D5 kapsamındadır.

## Yaşayan dünya authoring ve prova

Population editörü zone/schedule/kanal/slot kurallarını; traffic editörü şerit/sinyal ve air corridor rezervasyonlarını; Animals editörü species/breed/rig/gait/action/hit closure'ını aynı schema üzerinden doğrular. Model önizlemesi native locomotion kanıtı değildir. Definition diff; cosmetic kürk ile morphology/hit/timing/controller değişimini ayırır.

RehearsalWorld'de [AC-46–70](../validation/scenarios.md) kapsamına göre iki client/late join, hijack/tame yarışları, owner kaybı, bozuk rig, provider crash, population drain ve persistence denenir. Üretim hayvan owner/loot/offspring kayıtları anonimize fixture'lardır. Yeni tür veya davranış release'i yeni yetkiyi kendiliğinden açmaz; canlı kanal ayarı mevcut WorldPlan sınırında state değişimi olabilir. Native uçuş/hayvan kabulü R-18/19'da scoped kanıt ister.

## Yayın öncesi kurtarma provası

Release diff'i clock/schema/recovery policy değişimini ve eski snapshot'ların ihtiyaç duyduğu artifact closure'ını da gösterir. İlgili profile göre commit cevabı kaybı, worker crash, baseline sapması ve retained backup restore denenir; test dünyası üretim writer claim'i veya sırlarını kullanamaz. Clock override/counter wrap fixture'ı production build'e açık API olarak taşınmaz.

Commit sonucu belirsiz rollout otomatik rollback yapmaz; [core reconcile](../architecture/failure-recovery.md) sonucu beklenir. Postcommit eski artifact'e dönüş varsa yeni migration/metadata ve restore kökleri birlikte doğrulanır. [Stabilite kaydı](../validation/stability-audit.md), AC-73/74/82/85/86/88.
