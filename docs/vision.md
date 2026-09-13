# Vizyon, kapsam ve başarı

Durum: tasarım kararı. [Ana indeks](../README.md) · [Kararlar](decisions/architecture-decisions.md)

## Ürün ve kullanıcılar

SAEX, mevcut bir multiplayer sunucusunun gamemode'u değil, bağımsız istemci/sunucu platformudur. Üç ana kullanıcı vardır: oyun tasarlayan geliştirici, model/harita üreten içerik üreticisi ve sunucu işleten yönetici. Oyuncunun beklentisi doğru içeriğe otomatik ulaşmak, dünyaya güvenli katılmak ve başkalarıyla tutarlı etkileşim kurmaktır.

“Her şey modlanabilir” hedefi; açık kabiliyet sözleşmeleri üzerinden mümkün olan tüm oyun katmanlarının değiştirilebilmesidir. Sunucudan gelen rastgele kodun oyuncunun işletim sistemi üzerinde sınırsız yetkiye sahip olması anlamına gelmez.

## Kabul edilmiş ürün sınırları

| Konu | İlk mimari | Sonraki genişleme |
|---|---|---|
| Oyun | Klasik PC GTA:SA; belirlenmiş executable profili | Ek klasik sürüm profilleri |
| İşletim sistemi | Windows x64 üzerinde x86 GTA; sunucuda Linux/Windows x64 | Android ve Definitive Edition ayrı uyarlama projeleri |
| Modlama dili | İstemci/sunucu C#, .NET 10 LTS | Sürümlü sözleşmeler üzerinden başka diller |
| Fizik | GTA hareketi doğrulaması ve platform objeleri için ayrı otorite | Kanıtlandıkça sunucu simülasyonu kapsamının artırılması |
| Yıkım | Hazırlanmış hasar aşamaları ve parçalar | Çalışma anında geometri kesme araştırması |
| Render | Mevcut motora uyarlama ve kontrollü malzeme/UI uzantıları | Render motorunu değiştirmek ayrı karar |
| Uyumluluk | Taşıma rehberi; yeni protokol | Talep ve ölçümle sınırlı uyumluluk adaptörleri |
| Barındırma | Kendi sunucusunda oyun, asset ve yönetim servisleri | İsteğe bağlı yönetilen hizmetler |

## Başarıyı gösteren davranışlar

1. Standart bir kasa ve özel bir duvar aynı yıkım sözleşmesiyle çalışır.
2. Harita, input, HUD ve kurallar değiştirildiğinde çekirdekte roleplay bağımlılığı bulunmaz.
3. Asset yüklemesi ağın kontrol mesajlarını veya GTA ana döngüsünü sınırsız süre bekletmez.
4. Resource güncellemesinin başarısızlığı tanımlı biçimde geri alınır; dünya yarım güncellenmez.
5. Farklı C# resource'larının hatası, yetkisi ve tüketimi ayrı izlenebilir.
6. Geliştirici bir entity'nin neden görünmediğini kimlik, dünya, kapsam, asset ve bütçe kararlarına kadar takip edebilir.

Somut eşikler [performans](validation/performance.md), gözlenebilir sonuçlar [senaryolar](validation/scenarios.md) belgesindedir. 512 bağlantılı boş bir sunucu, 512 oyuncunun tek kavşakta oynayabildiğinin kanıtı değildir.

## Çekirdek ve paket ayrımı

Çekirdek; zaman, dünya, kimlik, izin, komut, replikasyon ve içerik yaşam döngüsünü sağlar. Ekonomi, iş, suç, faction, konut, görev üretimi, araç bakımı ve NPC ihtiyaçları değiştirilebilir paketlerdir. Bunların kayıt biçimleri çekirdek protokolün özel mesajları olmayacak; sürümlü component ve servis sözleşmelerini kullanacaktır.

Varsayılan `gta.base` paketi GTA hissini sunar. `gta.base` devre dışıyken yerine gerekli hareket/karakter ve kamera sağlayıcılarını sunan bir profil olmalıdır. Sadece paketi kapatarak motorun bütün sabit davranışlarının kaybolduğu iddia edilmez; [motor kabiliyetleri](architecture/engine-adapter.md) kontrol edilir.

## Kapsam dışı ilk işler

Bu teslimatta uygulama, benchmark, executable araştırması, oyun binary'si değiştirme, ağ dinleyicisi, hesap sistemi veya çalışan sunucu oluşturulmaz. Platformun açık kaynak/ticari lisansı bu tasarımla seçilmez. Üçüncü taraf kodun benimsenmesi halinde kaynak ve lisans envanteri gerekir; resmî belgelerden davranış öğrenmek doğrudan kod kopyalama kararı değildir.

## Tasarım değişikliği

V0.3 geliştirme yönü [platform tasarımında](platform-blueprint.md) açıklanır: çözümlenmiş WorldPlan, şemadan SDK/validator üretimi, bileşimli asset authoring, isteğe bağlı Actions/Effects ve izole yayın provası. Ürün başarısı yalnız yeni API sayısıyla değil; world açılmadan bulunan çakışma, state kaybetmeyen update, teşhis edilebilen arıza ve kapsamı doğrulanmış capability ile ölçülür.

Yeni fikir önce ilgili kabiliyet ve senaryoya bağlanır. Public SDK, manifest, kimlik veya kalıcılık sözleşmesini değiştiren fikir ADR kaydı ve geçiş planı gerektirir. Bir araştırmanın başarısızlığı saklanmaz; kapsam daraltılır veya yeni karar yazılır. Sınırsız entity, sıfır gecikme ve tüm GTA fiziğinde tam sunucu doğruluğu bu sürümün vaatleri değildir.

## V0.4 yaşayan dünya kapsamı

Sunucu sahibi [PopulationService](gameplay/population-traffic.md) ile yaya, kara/hava trafiği ve wildlife kanallarını bağımsız açıp kapatır; yoğunluğu dünya/bölge/saat bazında değiştirir. Görev aktörleri, kullanılan araçlar ve sahipli hayvanlar ambient temizlikten korunur. Hedef ortak kimlik, etkileşim ve kabul edilmiş sonuçtur; sıfır gecikme veya her native AI davranışında birebir eşitlik değildir.

[AgentService](gameplay/agents-navigation.md) görev/algı/nav/controller sınırını, [Animals](gameplay/animals-species.md) tür/cins/beden/rig/animasyon/davranış bileşimini sağlar. Yeni model mevcut controller'a uyuyorsa paketle eklenebilir; yeni beden/hareket R-19 kanıtı ister. Hayvan temel altyapısı şimdi tasarlanır; ileri uçuş/yüzme/binme/üreme ayrı capability ve gelecek geliştirme kapısıdır. AC-46–70; R-17–20.
