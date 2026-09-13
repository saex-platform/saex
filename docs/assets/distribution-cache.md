# Dağıtım, CDN, delta indirme ve cache

Durum: sözleşme taslağı. [Üretim](build-pipeline.md) · [Streaming](streaming-budgets.md)

## Merkezi hizmet zorunluluğu yok

Operatör immutable artifact'leri kendi HTTPS origin'inde veya seçtiği CDN'de barındırır. Platform manifest/katalog biçimini tanımlar, tek bir ticari CDN'ye bağımlı olmaz. HTTP yalnız açıkça seçilmiş yerel geliştirme profili içindir; üretim endpoint'leri HTTPS'tir.

Manifest sunucu/world kimliğine bağlanır. İmza doğrulaması içeriğin kaynağı ve bütünlüğü içindir; izin grant etmez ve parser güvenliğini kanıtlamaz. Public manifest kalıcı origin erişim anahtarı içermez; gerekirse scope ve süre sınırlı erişim belgesi kullanılır.

V0.3 release metadata doğrulaması [Studio/release sözleşmesine](../platform/studio-release.md) tabidir: güvenilen rol/namespace, metadata version/expiry ve snapshot kümesi doğrulanmadan yeni release aktive edilmez. Doğru hash, eski imzalı metadata replay'ini tek başına yakalayamaz. Mevcut aktif cache ile devam politikası yeni otomatik update için freshness atlama hakkı vermez. R-16 bu entegrasyonun uygulama kapısıdır.

## Chunk yaklaşımı

İlk yöntem artifact başına 1 MiB sabit chunk'lar ve her chunk için digest'tir; son chunk daha küçük olabilir. Artifact digest, birleştirilmiş exact artifact byte'ları için ayrıca kontrol edilir. Aynı chunk başka artifact'te de varsa yeniden kullanılabilir.

Bu sistem yalnız değişen chunk'ları indirir. Bir arşivin başındaki küçük değişiklik sonraki tüm byte'ları kaydırırsa kazanç azalabilir. “10 GB her zaman 30 MB'a iner” garantisi yoktur. Content-defined chunk veya binary patch ancak ölçümle sonraki optimizasyondur.

Paketin tamamını tek sıkıştırılmış akış yapmak küçük değişiklikte tüm içeriği yeniletebilir. Bu yüzden bağımsız artifact/chunk sıkıştırması tercih edilir. İndirilen encoded byte hash'i ile açılmış boyut/format doğrulaması farklı kontrollerdir.

## İndirme yolu

1. Release metadata güvenini/freshness'ini ve WorldOffer'daki CatalogRevision, ContractDigest, GameplayDigest ve minimum capability'leri doğrula.
2. Join için gereken asset closure'ını hesapla.
3. Cache'teki chunk/artifact bütünlüğünü kontrol et; yalnız eksik/bozuk parçaları iste.
4. İndirmeyi staging alanına yaz; boyut ve zaman sınırlarını uygula.
5. Chunk ve toplam artifact hash'ini, sonra parser/format sınırlarını doğrula.
6. Tam doğrulanmış artifact'i atomik rename ile content store'a al.
7. Gerekli artifact closure'ı hazır olduğunda runtime hazırlamayı başlat.

Download complete, runtime ready değildir. İstemci hash kontrolü yapmış olsa da gerekli collision ve model yüklenmeden AssetsReady/instance giriş bariyerini geçemez.

## Retry, kesinti ve origin davranışı

Resume kısmi dosyanın offset'ine güvenmez; doğrulanmış chunk sınırlarından sürer. Range yanıtı beklendiği gibi değilse chunk baştan alınır. CDN eski içerik gönderirse digest tutmaz ve quarantine edilir. Üç ardışık integrity hatası ilgili origin/artifact indirmesini durdurur; sonsuz retry yoktur.

İlk retry aralıkları 1, 2, 4, 8 saniye üstüne küçük jitter; oturum başına join indirme zaman bütçesi 5 dakikadır. Kullanıcı iptali her aşamada mümkündür. Sadece yavaş bağlantı nedeniyle background içerik indirmesini sonsuz aktif katılım önkoşulu yapmak yerine açık ilerleme/hata gösterilir.

Signed URL yenileme manifestin aynı digest'ine ulaşmayı sağlar; yeni digest almak yeni katalog teklifi gerektirir. Redirect farklı origin'e gidiyorsa allowlist/transport policy tekrar uygulanır.

## Cache politikası

İlk disk cache varsayılanı 10 GiB, kullanıcı tarafından ayarlanabilir. Aktif lease, hazırlanmakta olan world ve geri alma için pinli artifact'ler LRU'dan muaf tutulur. Disk dolarsa pinli gameplay içeriği silinmez; indirme/katılım açıklamalı durur.

Cache key artifact digest + format/target bilgisidir. Server adı tek başına cache key olamaz. Sunucu A'nın dosya yolu sunucu B'nin script storage'ına yazamaz. İndirilen içerik game root'a kopyalanmaz.

## Yayın ve geri dönüş

Önce bütün artifact'ler staging origin'e yüklenir ve örnek fetch doğrulanır. Sonra imzalı release manifesti yayımlanır. En son operatör world lock setini aktive eder. Eski katalog, bağlı client ve rollback retention bitene kadar erişilebilir kalır.

Kabul: AC-06 missing/mismatch, AC-07 eski-yeni katalog, AC-11 join bariyeri, AC-14 kötü origin. Metrikler downloaded/reused bytes, resume count, hash failure, cache pin size ve join'in download süresidir.

## Dinamik tür paketlerinin dağıtımı

Yeni Species/Breed paketinin çözülmüş artifact closure'ı aynı HTTPS/hash/chunk/metadata hattından gelir; oyun GNS mesajına model/IFP gömülmez. [Canlı tür ekleme](../gameplay/animals-species.md), catalog/plan activate ile tamamlanır. Yakın etkileşim için zorunlu rig/animasyon/hit/controller içeriği eksikse hayvan collision'sız aktive edilmez; spawn/katılım staging'de bekler veya açıklamalı ret alır.

Eski birey ve corpse/restore/rollback referanslarının pinlediği tür sürümü cache retention'a dahildir. Optional kozmetik ses eksikliği izinli sessiz sunum olabilir; gameplay stimulus sunucunun state/event'inden doğar. İndirilen davranış paketi asset imzasıyla native DLL yükleme hakkı kazanmaz. AC-06/11/56/62.

## Restore kökleri ve indirme kurtarması

Origin/yerel cache GC, yalnız bağlı client veya son release sayısına bakamaz. [Backup/operation closure](../architecture/failure-recovery.md) server snapshot, retained yedek, pending transfer, offline birey ve migration'ın pinlediği exact artifact'leri korur. İndirme tekrarları ve doğrulama işleri peer/resource/process bütçesine tabidir; content-address aynı olduğu için farklı dosya adları kapasiteyi sıfırlayamaz.

Download tamamlanması baseline/native Applied yerine geçmez. Yarım indirme aynı doğrulanmış chunk'larla devam eder; release metadata freshness ve content hash tekrar kontrol edilir. Eksik eski rig/IFP için varsayılan hayvan koyulmaz. AC-78/83/85.
