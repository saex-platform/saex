# Zaman alanları, yetki süresi ve duraklamadan dönüş

Durum: ADR-29 normatif sözleşmesi; v0.6 D1'de saat/lease gate alt kümesi kodlandı. R-21 genel OS/native kanıtı bekliyor. [Arıza/kurtarma](failure-recovery.md) · [Otorite](../networking/authority.md) · [Kabul](../validation/scenarios.md)

## Sorumluluk ve değişmezler

ServerCore zaman alanlarını ayırır; transport, broker ve adapter kendi süre denetimini yapar. Oyun tick'inin ilerlememesi bir simülasyon yetkisini, bekleyen isteği veya worker kullanım hakkını uzatamaz. İstemcinin ilettiği saat yetki vermez. Aynı fizik adımını tekrar çalıştırmak ile süresi dolan bir hakkı tekrar vermek farklı işlemlerdir.

| Alan | Kullanım ve arayüz | Kullanılmayacağı yer |
|---|---|---|
| SimulationTick / SimulationTime | Sabit adımlı oyun mantığı, oyun cooldown'ı, animasyon olayları; pause politikası WorldPlan'da | Lease, bağlantı liveness, DB/IPC/Ready timeout |
| ControlTime | Süreç içinde monoton elapsed time; ClockDomainId + başlangıç + süre/deadline | Kalıcı takvim, makineler arası mutlak saat karşılaştırması |
| UtcInstant | Denetim zamanı, paket/sertifika takvimi, izinli offline yaşam hesabı | Motion sırası, tick sırası, simülasyon sahibinin yetki süresi |
| ServerTickEstimate | İstemci interpolation/prediction hizalaması; hata payı ölçülür | Sunucunun kabul kararını geçersiz kılma |

Sözleşmedeki eski `ServerTick` simülasyon sırasıdır. Lease, hysteresis bekleyişi, Ready/Applied, indirme/IPC ve recovery denemesi gibi operasyonel saniye süreleri ControlTime kullanır; gameplay cast/cooldown/saat programı SimulationTime olarak açıkça işaretlenir. Yeni süre alanı `clockKind` taşır; bir alanın yorumu sessizce değiştirilemez. SimulationTick, ResourceEpoch, OwnerEpoch, WriterTerm ve bütün revision/sequence sayaçları farklı tiplerdir. Wire genişliği R-11c'de sabitlenecek; artış kontrol edilir. Üst sınıra ulaşmadan kontrollü drain ve yeni epoch gerekir; wrap ederek eski kimliğe eşitlenmek yasaktır. WorldEpoch değişimi, persistent identity veya OperationId kaydını sıfırlamaz.

## Lease ve watchdog veri akışı

1. Sunucu lease üretirken EntityRef/group, OwnerEpoch, izinli alanlar, gerekli revision ve kendi ControlDeadline'ını kaydeder. İlk ölçüm profili 2 saniye lease, 500 ms yenileme hedefidir; RTT'ye göre otomatik uzatma v1 kapsamında değildir.
2. Ağ mesajı grant kimliği, sıra, verilen azami süre ve bağlamı taşır. Sunucuya ait ham monoton timestamp başka makinenin saatiyle çıkarılmaz. Client saat tahmini veya gecikmiş yenileme sunucunun mevcut deadline'ını değiştiremez.
3. ClientHost ve adapter yeni grant bağlamında muhafazakâr yerel watchdog kurar. Bilinen gecikme/belirsizlik payı bütçeden düşülür; kalan süre güvenilir biçimde belirlenemiyorsa grant kullanılmaz. Eski/sırasız grant ve renewal tekrarları reddedilir. Yerel watchdog yalnız emniyet mekanizmasıdır; sunucunun yetki kanıtı değildir.
4. Sunucu her owner sonucu kabulünde epoch, ControlDeadline ve state önkoşullarını kontrol eder. Kuyruğa zamanında girmiş olmak, süresi dolduktan sonra uygulanma hakkı vermez. Gerekli hit geçmişi ayrı rewind politikasıdır; eski lease'i diriltmez.
5. Transport/broker izleyicisi tick heartbeat'inin kesildiğini görürse yeni owner grant ve gameplay ingress'i durdurur. Eşzamanlı başka bir dünya writer'ına dönüşmez; güvenli safepoint'te core recovery gerekir. Süreç bütünüyle durmuşsa uzaktan garanti verilemez; peer watchdog'ları ve yeniden başlatmada fencing bu boşluğu sınırlar.

Task için oyun içi bitiş tick'i ayrıca bulunabilir. `TaskInstanceId`, cancellation generation ve ControlDeadline ayrı denetlenir. Bir uçuşun veya ısırma animasyonunun görsel olarak sürmesi, yeni hasar/ödül kabul etme yetkisi vermez. Native emniyet tepkisi controller capability'sinin parçasıdır; uçaktaki oyuncunun collider'ını gelişigüzel silmek genel kurtarma yöntemi değildir.

## Pause, stall, suspend ve yeniden başlatma

| Olay | Zorunlu davranış | Tekrar açma koşulu |
|---|---|---|
| Yönetici oyun zamanını durdurur | SimulationTick durabilir; control timeout, kimlik ve bütçe denetimi devam eder | Güncel grant ve ready state; oyun cooldown politikası korunur |
| Tek worker takılır | Resource grant revoke; worker IPC kesilir; core-owned committed işler kurtarılır | Yeni ResourceEpoch ve doğrulanmış state; restart bütçesi |
| Core uzun tick kaçırır | Yeni admissions ve grants kesilir; sonuçlar birikerek limitsiz catch-up yapmaz | Stall sonrası lease/revision revalidation ve ölçülmüş recovery bütçesi |
| Host veya makine suspend/resume | Adapter resume girişinde eski grantleri kullanılmaz sayar; yeniden handshake/readiness ister | Yeni Session/Owner bağlamı veya doğrulanmış resync |
| Monoton saat reseti / süreç restart | Eski ClockDomainId deadline'ları taşınmaz; eski lease'ler geçersiz | Yeni WorldEpoch/SessionId ve gerektiğinde WriterTerm |
| UTC ileri/geri atlar | ControlTime hakları değişmez; takvim tüketicisi anomaliyi kaydeder | Offline işlemde doğrulanmış ve sınırlandırılmış zaman aralığı |

Saat kaynağının suspend dahil/dışında ilerleme davranışı her Windows/Linux build'inde conformance ile kaydedilir. Resume tespiti ve grant geçersizleştirme, yalnız geçen süre sayacına güvenmez. Adapter'ın native frame girişinde kendi kontrolü bulunur; Host'tan bir sonraki komutun gelmesini beklemek yeterli değildir. İşlemci veya tüm OS durmuşken watchdog'un çalıştığı iddia edilmez. Sistem devam ettiğinde ilk oynanış komutundan önce fence uygulanması test edilir.

Uzun duruşta birikmiş AI/physics tick'leri sınırsız yürütülmez. RecoveryProfile azami catch-up adımını tanımlar; fazlası için simülasyon yavaşlatma, oturum resync veya kontrollü dünya duruşu seçilir. Geçmişte yapılmamış saldırı/doğum/loot olayları sırf saat ilerledi diye topluca üretilmez.

## Kalıcı offline yaşam ve takvim

Varsayılan offline progression kapalıdır. Açılan paket, server tarafından saklanan son güvenilir UTC, son işlenmiş zaman penceresi, azami catch-up süresi ve OperationId kuralını bildirir. Negatif fark sıfırlanır ve anomali işaretlenir; beklenmeyen büyük pozitif fark üst sınırda beklemeye alınır. Operatörün yeniden değerlendirmesi aynı pencereyi iki kez doğuramaz. Client saati, görüntülenen gün/saat veya sistemin yerel saat dilimi bu hesabın girdisi değildir. DST değişimi hayvanı yaşlandırmaz. Beden değişimi yine asset/controller readiness gerektirir.

## Hata davranışı, genişleme ve kabul

ClockProvider; `monotonicNow`, `utcNow`, `domainId`, resume/gap bildirimi ve test saati arayüzünü sağlar. Pakete ham OS saatini yetki belirlemek için kullanma hakkı verilmez. TimeProvider benzeri enjekte edilebilir saat yüzeyi hedeflenir; sahte saat yalnız izole conformance ortamındadır. Canlı bir lease'in deadline'ını debug aracıyla geriye almak yasaktır.

Microsoft QPC açıklaması monoton aralık ölçümünü UTC senkronizasyonundan ayırır; .NET TimeProvider farklı saat okumaları ve test edilebilir zaman soyutlaması sunar. Bunlar SAEX'in hazır watchdog implementasyonu değildir. [QPC](https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps) · [TimeProvider](https://learn.microsoft.com/en-us/dotnet/standard/datetime/timeprovider-overview)

AC-71/72 oyun stall, suspend ve sahte zamanı; AC-84 takvim sıçramasını; AC-86 sayaç sınırı ve shutdown'ı sınar. R-21 Windows adapter/Host ve Linux/Windows server için ayrı kanıt ister. Başarısız bir clock/controller profili world capability'sini açamaz.

## D1 uygulanan alt küme

[ControlClock](../../src/core/control_clock.cpp) std::chrono::steady_clock ile process-local microsecond üretir. [LeaseAuthority](../../src/core/lease_authority.cpp) 2 saniye deadline, exclusive expiry, replay renewal ret, domain/regression fence ve explicit recovery kullanır. SimulationTick ilerlemeden sürenin dolması fake clock ile; gerçek provider'ın monotonluğu ayrı küçük smoke testiyle sınandı. uint64 checked artış ve deadline addition overflow ret vardır.

ClockDomainId üretimini güvenilen bootstrap sağlayacaktır; örnek domain numarası production kimliği değildir. Gerçek suspend/resume bildirimi, OS dış watchdog, GTA frame kontrolü, UTC/offline ecology ve bounded catch-up scheduler uygulanmadı. Explicit test fence'i OS suspend kanıtı sayılmaz. AC-71/72/86 ve R-21 yalnız [D1 raporundaki](../development/d1-foundation.md) alt kapsamla ilerler.
