# Fizik simülasyonu, tahmin ve uzlaştırma

Durum: tasarım + R-02/R-06/R-08 araştırması. [Otorite](authority.md) · [Malzemeler](../assets/physics-materials.md)

## Üç açık simülasyon modu

| Mod | Kullanım | Hesap ve sonuç |
|---|---|---|
| validated-native | İlk yaya/araç ve test edilmiş native body'ler | GTA client hesaplar, server sınır/çarpışma kontrolleri yapar |
| server-simulated | Uyumlu collision/profil kullanan platform body'leri | Headless server solver karar verir; client render/interpolation yapar |
| cosmetic-local | Toz, küçük kıymık, dekoratif efekt | Her client hesaplar; hasar, collision veya ödül etkisi yok |

Bir body aynı anda iki solver'ın authoritative kontrolünde olamaz. Mod değişimi controller'ın kanıtlanmış quiesce/emniyet davranışı, gerekli asset hazırlığı, owner epoch artırma ve yeni başlangıç snapshot'ı ile yapılır. Dondurma yalnız ilgili body/temas sınıfında güvenli olduğu doğrulanmışsa seçilir; yolcu taşıyan uçuş için genel çözüm değildir.

## Headless sunucunun ihtiyacı

Server-simulated veya hit doğrulama isteyen sunucu; görsel TXD'ye ihtiyaç duymasa da collision geometry, dünya placement'ları, dönüşümler, malzeme/hasar profilleri ve sorgu hızlandırma verisini edinmelidir. İstemcinin “duvar burada yok” beyanı collision kaynağı değildir.

Stok GTA collision verisinin sunucuya hukuken ve teknik olarak nasıl hazırlanacağı R-12 çıktısıdır. İlk bağımsız test sahnesi yeniden dağıtılabilir özel collision paketi kullanır. Bu veri yolu bulunmadan tüm San Andreas haritasında server collision doğruluğu vaat edilmez.

Solver kütüphanesi bu dokümanla uygulanmış sayılmaz. R-06; sabit adım, contact sorguları, sleeping, eklemler, performans ve lisans uygunluğu üzerinden seçim yapar. Başarısızlıkta desteklenmeyen body sınıfı server-simulated olarak ilan edilmez.

## Zamanlama varsayılanı

World logic 30 Hz, uyumlu server physics alt adımı 60 Hz, yüksek öncelikli motion yayın hedefi 20 Hz'dir. Render kare hızı bunlardan bağımsızdır. Bunlar [ölçüm hedefleridir](../validation/performance.md); mevcut performans değildir.

Uzun frame sonrası en çok iki ek telafi adımı çalıştırılır; birikmiş zaman sınırsız döngüye dönüştürülmez. Kalıcı taşma profiler uyarısı, yeni body reddi ve kontrollü yük azaltma başlatır. Fizik zamanının yavaşladığı anlar gameplay ve telemetry tarafından görülebilir olmalıdır.

## Native prediction sınırı

Yerel input anında GTA'ya uygulanır. Sunucu accepted transform/motion mode döndürür. Küçük hata görsel yumuşatma, büyük veya collision açısından geçersiz hata güvenli yeniden konumlandırma ile düzeltilir. Kamera yumuşatması collision state'i geciktirerek duvar içinden geçiş üretmemelidir.

Native task ve fizik simülasyonunun deterministik input replay'i kanıtlanmadığı için ilk sürüm “son N input'u birebir yeniden simüle ederek tam rollback” sözü vermez. Desteklenen custom controller'da replay ayrı capability olarak eklenebilir.

Uzak body'ler timestamp'li bağımsız motion snapshot'ları arasında interpolate edilir. Kısa extrapolation sınırı ilk profilde 100 ms'dir; sonrasında hareket sınırsız tahmin edilmez. Lease kaybı [monoton control zamanı](../architecture/time-fencing.md) ile denetlenir; desteklenen controller emniyeti veya server solver kullanılır. Güvenli fallback yoksa etkileşim alanı fence edilir. Son transform'u tutmak tüm native uçuş/temasları güvenli yapmaz. AC-51/71/72/79.

## Temaslar ve yıkım

Native aracın platform objesine çarpması özel köprüdür. İstemci impact adayı gönderir; sunucu sahiplik, hareket geçmişi, collision adayı ve profil sınırlarıyla tutarlılığı değerlendirir. Yalnızca client'ın impulse sayısı kabul edilmez. R-06 temas köprüsü güvenilir değilse bu eşleşme competitive-critical modlarda kapalıdır.

Bir impact'in source tick'i, hedef entity generation'ı ve owner epoch'u bulunur. Hasar transaction'ı bir kez uygulanır. Collision'ın değiştiği tick ile debris spawn tick'i aynıdır. Kozmetik rastgelelik ortak seed kullanabilir; bu seed farklı solver'ların aynı fizik sonucunu üreteceğinin garantisi değildir.

## Uyku, aktif olmayan bölgeler ve onarım

Uyuyan body transform/state tutar, sık motion göndermez. Görünmeyen NPC için soyut görev ilerlemesi yapılabilir; görünen collision'ı olan body için rastgele soyut konum ataması yapılmaz. Hiç client yokken validated-native body donar veya tanımlı checkpoint davranışına geçer.

Onarımda yeni collision hacmi oyuncu/araçla örtüşüyorsa işlem güvenli alan boşalana kadar bekler veya `occupied_volume` ile reddedilir. Duvarı oyuncunun içinden geçirerek yeniden oluşturmak kabul edilmez.

Kabul: AC-01/03/13/16/17. Collision ve native/solver geçiş davranışı görsel benzerlikle değil log ve temas sonuçlarıyla doğrulanır.

## Creature controller ve trafik geçişi

[AgentService](../gameplay/agents-navigation.md) içindeki SimulationTier, bu belgedeki physics simulation mode değildir. Native controller hiç client yokken kendiliğinden çalışmaz; explicit abstract/suspended geçiş ve dönüşte yeni accepted başlangıç/owner epoch gerekir. Gözlenen veya etkileşilen ajan soyut konum ilerlemesiyle collider üzerinden atlatılamaz.

Hayvanın hareket hacmi, görsel rig'i ve hit proxy'si ayrı doğrulanır; büyük bedeni insan capsule'ıyla eşdeğer ilan etmek yasaktır. Isırma/tekme veya araç-hayvan teması güncel target/state ve server query/hasar yolundan geçer. Ragdoll/IK, yüzme/uçuş/binme ayrı R-19 kabiliyetleridir. Owner kaybındaki aircraft control yalnız tested R-18 profiliyle açıktır. AC-51/55/57/58/64/67.
