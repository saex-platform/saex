# SAEX v0.16 — Oyun üretimi, yaşayan dünya ve kurtarma mimarisi

Kod 0.1.11 [kontrollü entry sınırı](development/d1-entry-boundary.md) ekler: açık initialization izni, erken main-thread hardware breakpoint ve giriş byte’larının önce/sonra gözlemi. Bu safha tam oyun başlangıcı veya SAEX DLL/ABI kanıtı değildir.

Kod 0.1.10 [native sembol bağlantı denetimi](development/d1-native-linkage.md) ekler: açık aday DLL ile isim/ordinal gereksinimleri, export alias/hole/forwarder ve hash kanıtı karşılaştırılır. Statik tam eşleşme initialization/ABI yetkisi değildir.

Kod 0.1.9 [loader mapping lifecycle](development/d1-loader-lifecycle.md) ekler: eski eşlemeler unload ile emekli edilir, yeni LOAD tekrar doğrulanır ve yeni gözlem kimliği alır. Gerçek private GTA'da 12 kontrollü koşu breakpoint adayına ulaştı; 11 unload işlendi. Same-base remap yalnız metadata testlerinde doğrulandı. Initialization, SAEX DLL/hook ve multiplayer hâlâ açık kapılardır.

Kod 0.1.4 [askıda process observer'ı](development/d1-suspended-process.md) ekler. İlk create-debug olayı, file identity ve image eşleşmesi oyun kodu yürütülmeden incelenir; sonuç initialized GTA veya hook desteği değildir.

V0.7, [Plugin-SDK-SA kaynak incelemesini](references/plugin-sdk-sa.md), [native entegrasyon sözleşmesini](architecture/native-sdk-integration.md) ve [D1 motor uygulama planını](development/d1-engine-integration.md) ekler. Önceki kod 0.1.3 [SDK bağımsız başlangıç DLL'sini](development/d1-bootstrap-module.md) ve oyun dışı ret/stop testlerini ekler; gerçek GTA'ya DLL yüklemesi henüz yoktur. Önceki kod 0.1.2 [N2 dosya/image gözlemini](development/d1-engine-preflight.md) ekler; runtime/bootstrap kapısı açıktır. Önceki kod 0.1.1 [D1-N1 dependency build](development/d1-native-dependency.md) alt kümesini uygular; gerçek adapter yoktur. ADR-36 yalnız private vendor hedefinin C++23 ihtiyacını kaydeder. C++20 core, x86 GTA/x64 Host, .NET 10 worker ve GNS/HTTPS mimarisi korunur.

Durum: mimari yönü ve sözleşmeler; D1 foundation kodlaması başladı, multiplayer ürün henüz yok. [Ana indeks](../README.md) · [Gerçek uygulama kapsamı](development/status.md)

## Ürün farkını nasıl kuruyoruz?

SAEX'in geliştirme deneyimi; bir dünya profilini seçmek, paketleri birleştirmek, sonucu önceden denetlemek, ayrı bir test dünyasında denemek ve aynı sürümü yayımlamak etrafında kurulacak. İçerik yazarının tanımı, C# geliştiricisinin API'si, sunucunun kararı ve oyuncunun gördüğü durum izlenebilir bir zincir oluşturacak.

“Üst seviye” burada sınırsız feature anlamına gelmez. Özgünlük iddiası; hiçbir platformda benzeri olmayan tekil API'lere değil, GTA:SA için bu iş akışlarının birlikte ve ölçülebilir sözleşmelerle tasarlanmasına dayanır. Rakiplerden daha iyi performans veya hazır motor desteği henüz kanıtlanmış değildir.

## Tamamlayıcı yapılar

| Yapı | Geliştiricinin kazanımı | Karar / uygulama sınırı |
|---|---|---|
| WorldPlan | Harita, gameplay, resource, yetki, schema ve içerik tek çözümlenmiş planda | [Dünya planı](architecture/world-plans.md); yeni bir oyun motoru değil |
| ContractSchema | C# API, ağ doğrulama ve inspector aynı veri sözleşmesinden üretilir | [Çalışma sözleşmesi](architecture/execution-contracts.md); üretilmiş SDK henüz yok |
| Sıralı state yayımı | Paralel işler çalışırken dünyanın tek geçerli mutasyon yolu korunur | Tek world writer sürer; bütün core tek CPU thread'e hapsedilmez |
| Prefab bileşimi | Aynı objeden güvenli varyant üretme, alan kaynağını açıklama | [Bileşim](assets/composition-variants.md); sessiz son-paket-kazanır yok |
| Actions / Effects | Kapı açma, onarma, yakıt doldurma ve yetenekleri ortak araçlarla tanımlama | [Aksiyon paketi](gameplay/actions-effects.md); core için isteğe bağlı |
| ChangeSet projeksiyonları | Yıkımın collider, navigasyon, ses ve gameplay etkisini izleme | [Dünya değişimleri](networking/world-change-projections.md); her sistem aynı anda hesaplanmaz |
| Studio ve release provası | Ayrı test dünyasında değişiklik ve arıza denemesi, sonra incelenebilir yayın | [Studio iş akışı](platform/studio-release.md); üretim yan etkileri test dünyasına taşınmaz |
| Conformance kayıtları | Hangi motor/build/asset profilinin gerçekten çalıştığını bilme | [Uygunluk sözleşmesi](validation/conformance-contracts.md); “yükleniyor” başarı ölçütü değil |
| PopulationService | Yaya/kara/hava/wildlife kanalları, bölge/saat/yoğunluk ve güvenli canlı kapatma | [Nüfus ve trafik](gameplay/population-traffic.md); korunan birey ve kota kayıtları |
| AgentService | Ortak algı, görev, nav, controller ve davranış teşhisi | [AI ve navigasyon](gameplay/agents-navigation.md); karar server'da, native kapsam kanıtlı |
| Animals | Tür/cins/beden/rig/animasyon/behavior ayrı; yeni tanımı paketle ekleme | [Hayvan sözleşmesi](gameplay/animals-species.md); yeni beden/hareket ayrı capability |
| ControlTime ve RecoveryProfile | Oyun takılınca sona eren yetki, toplam kuyruk/restart sınırı ve görünür health | [Saat](architecture/time-fencing.md), [arıza sınırı](architecture/failure-recovery.md); runtime kanıtı gerekli |
| Core işlem kurtarması | Worker ölse de kesinleşmiş sonucu bulma; belirsiz commit'i tekrar etmeden çözme | CommitReceipt ve WriterTerm; v1 otomatik HA takeover yok |
| Onarım ve tekil transfer | Bozuk baseline/delta'yı sınırlı yenileme; pet/araç için tek canonical world | [Replikasyon/transfer](networking/consistency-recovery.md); v1 tek core/backend |

## Örnek: modlanabilir elektrikli kapı

İçerik üreticisi kapı mesh/collider'ını, menteşeyi, motor sesini ve bağlantı noktalarını tanımlar. Oyun geliştiricisi `door.open` aksiyonuna erişim koşulu, süre, kilit durumu ve isteğe bağlı güç servisi ekler. WorldPlan hangi resource'un bu alanları değiştirebileceğini, hangi paketin güç sağlayacağını ve gerekli native kabiliyetleri çözer.

Oyuncu düğmeye basınca istemci uygun yerel tepkiyi gösterir; sunucu erişim ve mevcut state'i doğrular. Kapının geçerli state'i ile collision geçişi ortak kritik işlem kapsamındadır. Navigasyon bağlantısı ancak ilgili collision revision'ına göre hazırsa açılır. Elektrik kesilince davranışı power policy belirler; bir visual effect keyfî biçimde güç veya hasar yazamaz. Test dünyasında kapı geçişinde paket kaybı, resource crash ve onarım çakışması denenir.

Kapının açık olması kalıcı state, motor sesi kısa event, collider native temsil, yol geçişi türetilmiş projeksiyondur. Bu ayrım sonradan katılan oyuncunun motor sesini tekrar oynatmadan açık kapıyı doğru kurmasını sağlar.

## Kodlama katmanları

| Katman | Sahip olduğu iş | Bağımlılık yönü |
|---|---|---|
| Contracts / schemas | Kimlik, typed command/event, capability ve veri kuralları | GTA, DB ve GNS tiplerine bağımlı olmaz |
| Native core C++20 | Scheduler, state registry, transaction, replication ve broker | Contracts kullanır; worker'ın CLR nesnesini paylaşmaz |
| Adapters | GTA x86, GNS, persistence, renderer, audio | Core portlarına bağlanır; desteklediği capability'yi bildirir |
| C# SDK / workers | Oyun paketleri, UI ve servis mantığı | Snapshot okur, yetkili komut verir; core belleğine yazmaz |
| Authoring / tooling | Asset cook, WorldPlan, inspector, test ve release | Aynı schema ve lock seti tüketir |
| Optional kits | Actions, inventory, economy, AI, inşa, görev vb. | Tür veya oyun modu gerektiren kurallar burada yaşar |

Bu tablo mimari sorumluluk sınırıdır. D1'de C++ core primitive'leri, C# araç/ortak tipler ve test kaynakları oluşturuldu; [kaynak eşlemesi](development/d1-foundation.md) gerçek alt kümeyi gösterir. C++20, C#/.NET10, x86 GTA/x64 host, Linux/Windows server ve GNS + HTTPS kararları değişmedi.

## Öncelik ve maliyet

D1'de WorldPlan doğrulayıcısının temel alt kümesi, ContractSchema ve bounded scheduler gerekir. D2'de tek hasar akışı ve projeksiyon revision'ları kanıtlanır. D3'te prefab bileşimi, SDK üretimi, isteğe bağlı ilk aksiyonlar ve headless release provası tamamlanır. Görsel Studio, geniş etkiler kataloğu ve ayrıntılı test araçları D4–D5'te büyür.

Bu ayrım, genel amaçlı görsel programlama editörünü ilk iki istemcinin önkoşulu yapmaz. Runtime geometri kesme, dağıtık aynı-world fizik, otomatik script çevirisi ve yeni işletim sistemi destekleri ayrı gelecek kararlardır. WorldPlan, schema üretimi ve test ortamı ek mühendislik maliyeti getirir; tekrar eden el yazımı doğrulama ve üretim hatalarının azalmasıyla değerlendirilecek.

## Başarı ölçütleri ve hata davranışı

Eksik sağlayıcı, alan mutator çakışması ve yanlış fizik varyantı oturum açılmadan açıklanmalıdır. Tek bir hata raporu ilgili asset, resource, schema, world revision ve native kabiliyete ulaşabilmelidir. Resource durduğunda bıraktığı iş, lease ve projeksiyon kuyruğu bulunabilmelidir. Dünya açılışı başarısızsa boş veya farklı kurallı dünya ile başarılı görünülmez.

AC-34–45 dünya üretimini, AC-46–70 yaşayan dünya eklerini, AC-71–88 zaman/işlem/ağ/transfer kurtarmasını sınar. Ölçüm ve uygulama kanıtı olmadan tasarlanmış özellik çalışıyor olarak işaretlenmez. [Yol haritası](roadmap.md), [v0.3 kaydı](validation/architecture-evolution.md), [v0.4 entegrasyonu](validation/population-animal-integration.md) ve [v0.5 denetimi](validation/stability-audit.md) kapsamın izidir.

## Örnek: yaşayan sokak ve yeni köpek cinsi

Operatör yaya ve kara trafiğini açar; hava trafiğini kapalı tutar. İki oyuncu aynı sokakta aynı sürücüyü/arabayı görür. Aracın alınması koltuk, NPC görev ve ControlClass değişimini ortak transaction'a bağlar. Traffic off yeni üretimi durdurur; kullanılan araç protected kalır. Rota yıkılmış çite veya yeni engele göre revision kontrollü güncellenir.

Sahiplenme kaydı DB'de commit olduktan sonra C# worker çökerse core sonucu kaybetmez; yeni worker güncel ownership state'ini alır. Hayvan world değiştirirken commit cevabı kaybolursa kaynakta yeniden spawn edilmez: sonuç receipt ile çözülene kadar geçiş bekler. Eski türün artifact'leri offline birey ve yedek restore kökleri boyunca tutulur. Ağda eksik state saptanırsa belirli kesit üzerinden repair yapılır; çözülmeyen kritik state ile etkileşim açılmaz.

İçerik üreticisi yeni bir köpek cinsini Species/Breed, morphology/rig, animation/locomotion ve görünümden oluşturur. Aynı uyumlu controller varsa paket/plan/closure kabulüyle eklenir; yeni beden topolojisi R-19 kapsamını genişletir. Sahiplenilen bireyin health/owner/state'i model güncellemesiyle sıfırlanmaz. Core C++20 portları, C# resource provider'ları, GNS + HTTPS ve tek authoritative writer bu akışlarda korunur.

## V0.12 native başlangıç çevresi

[Kod 0.1.5 aracı](development/d1-native-startup.md), motor dosyası ile yerel DLL/ASI ortamını ayrı kimliklerle inceler. Declared import/delay/TLS metadata ve yerel aday ilişkisi, gerçek Windows loaded-module seçiminden ayrılır. Bu çalışma yeni bir oynanış özelliği değildir; mevcut modların sonraki başlatma kanıtına etkisini görünür kılar ve N2'nin izlenebilirliğini artırır.

## V0.14 uygulama farkı

[İncelenmiş loader recipe](development/d1-loader-policy.md) başlangıçta engine ve DLL artifact eşleşmesini process yaratmadan doğrular. Aynı engine hash'i farklı Windows compatibility/yan dosya bağlamlarında farklı sonuç verebildiği için launch context ayrı kanıt olur. Yerel özel kopyanın breakpoint adayı initialization veya oynanış desteğine çevrilmez; üst seviye modüler mimarinin GNS/HTTPS, C# ve native yetki sınırları korunur.
