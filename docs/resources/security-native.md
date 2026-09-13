# Resource güven sınırları ve native genişleme

Durum: tasarım + R-03 yayın engeli. [Runtime](runtime-sdk.md) · [Motor adapter](../architecture/engine-adapter.md)

## Güven modeli

Sunucu geliştiricisinin yazdığı client C# kodu oyuncunun cihazı açısından güvenilmeyen koddur. İmza kaynağı tanımlar ve bütünlüğe yardımcı olur; kodun güvenli olduğunu ispatlamaz. Asset dosyaları da native parser, shader veya decoder hatasını tetikleyebilir.

`AssemblyLoadContext` bağımlılık yükleme alanıdır; güvenlik izolasyonu sağlamaz. .NET API'lerine kısıt koyan yüzeysel wrapper veya kaynak kod taraması güven sınırı yerine geçmez. [Microsoft API açıklaması](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.loader.assemblyloadcontext?view=net-10.0)

## Client worker politikası

| Yüzey | Varsayılan |
|---|---|
| Dosya sistemi | Resource'a ait kota sınırlı alan; paket içeriği salt okunur |
| Oyun kurulumu / kullanıcı belgeleri | Worker'a doğrudan erişim yok |
| Ağ | Doğrudan serbest socket yok; ilan edilmiş broker işlemleri |
| Diğer süreçler / native pointer | Erişim yok; GTA komutları doğrulanan IPC üzerinden |
| Kamera / mikrofon | Resource doğrudan açamaz; platform izinli voice/UI hizmeti |
| CPU / bellek / süreç oluşturma | OS kotaları, job/supervisor ve watchdog |
| IPC | Resource/session epoch, capability, schema ve boyut kontrolü |

Windows için AppContainer, sınırlı ACL'ler ve Job Object gözetimi tasarım temelidir. AppContainer dosya, ağ ve süreç yetkilerini sınırlamak için resmî bir mekanizma sunar; .NET runtime ve gerekli IPC'nin bu profilde çalışması ayrıca kanıtlanmalıdır. [AppContainer isolation](https://learn.microsoft.com/en-us/windows/win32/secauthz/appcontainer-isolation)

Linux server worker için eşdeğer process/namespace, filesystem, syscall ve kaynak sınırlama profili R-03 kapsamındadır. Host'taki genel .NET sürecini sadece başka PID'ye koymak güvenlik garantisi değildir.

## Broker yetkisi

Her IPC endpoint tek resource epoch'una bağlıdır. Caller tarafından gönderilen resource adı yetkilendirme girdisi değildir. Broker dosya yolunu normalize eder, grant edilen kökü ve boyutu doğrular; HTTP isteklerinde destination politikasını uygular. Server secrets worker'a gerektiği kadar ve server tarafında sağlanır.

Yetki örnekleri: yalnız owned entity'ye hasar uygulama, belirli prefab namespace'inde spawn, public UI sahnesi yaratma, bir schema ile intent gönderme. Yönetici oyuncu hakkı ile resource capability iki ayrı denetimdir; ikisi de gerekebilir.

## Native uzantı politikası

GTA sürecine alınan C++ uzantı tam güven alanındadır. İlk sürümde yalnız platform dağıtımının gözden geçirilmiş native modülleri yüklenir. Genel sunucu katılımı arbitrary DLL/ASI indirip GTA'ya yükleyemez.

ADR-35 ile seçilen Plugin-SDK-SA, platformun private x86 adapter bağımlılığıdır; resource native API'si değildir. SDK/header/library güncellemesi sunucu manifestinden yapılamaz. Executable ve mapped-image kontrolü SDK bağımsız bootstrap'ta, SDK global initializer/event kaydından önce tamamlanır. Kaynak pin/patch/build/notice ve hook ownership kanıtı [native SDK sözleşmesine](../architecture/native-sdk-integration.md) tabidir. Hash veya kaynak incelemesi OS sandbox/anti-cheat kanıtı sayılmaz; R-03 ve GNS kimlik sınırı sürer. Bu bootstrap henüz uygulanmadı.

İleride ayrıca kurulmuş güvenilen native modlar için sürümlü C ABI ve capability bildirimi tasarlanabilir. DLL'nin aynı process içindeki zararlı davranışını capability bildirimi engellemez; bu nedenle genel mod sandbox'ı sayılmaz. Kernel seviyesinde anti-cheat bu planın parçası değildir.

## Hata ve saldırı yüzeyi

Loop yapan worker kill/restart edilir, grants revoke olur, dünya persistent kayıtları korunur. Aşırı IPC veya entity oluşturma kotada reddedilir. Bozuk asset build/validation katmanında elenir; native decode katmanı için fuzz corpus gerekir. Shader maliyet limiti her sürücü kilitlenmesini önleyebildiğimiz anlamına gelmez.

R-03 başarısızsa otomatik indirilen client C# özelliği kapalı kalır; geliştirme profili yalnız açıkça güvenilen yerel kaynaklarla ilerler. Yalıtım testi geçmeden “güvenli mod çalıştırma” yayımlanmaz.

Kabul: AC-09, AC-14, AC-19. Testler dosya/ağ/süreç kaçışı, resource'lar arası veri sızıntısı, kota taşması ve grants revoke sonrasındaki eski IPC'yi kapsar.

## Servis çağrısında yetki koruma

Bir provider, başka resource adına gelen komutu kendi geniş yetkisiyle otomatik çalıştıramaz. ActorContext, caller resource epoch, hedef domain ve işlem kapsamı korunur; açık ve sunucuca verilmiş delegation yoksa yetkilerin izinli kesişimi kullanılır. [Execution sözleşmesi](../architecture/execution-contracts.md), AC-37. WorldPlan üst sınırdır; runtime grant her çağrıda denetlenir. [Test dünyası](../platform/studio-release.md) üretim DB/HTTP/credential erişiminden ayrıca OS/broker seviyesinde yalıtılır.

## Davranış paketi ve tür ekleme güven sınırı

Species/Behavior metadata'sı yerel makineye yeni dosya/ağ/native izni vermez. Data graph düğümleri typed bounded schema ve action allowlist'tir; native adres, executable expression veya sınırsız recursion kabul edilmez. C# behavior provider mevcut worker izolasyonunu kullanır; güçlü Animals/Ownership provider'ına çağrıda ActorContext/delegation korunur.

Client algı/hit/konum önerileri server query/epoch/permission kontrolünü geçer. Hayvan sahibinin pet kontrol hakkı simülasyon lease'i değildir; lease de sahiplik/tame/loot hakkı değildir. Private blackboard, player ilişkileri ve server seed istemciye veya yetkisiz debug'a gönderilmez. Yeni rig parser/controller native platform dağıtımı olmadan açılamaz. [Hayvan genişleme düzeyleri](../gameplay/animals-species.md), AC-57/59/65.

## Arıza sırasında yetki sınırı

Supervisor yalnız health/lifecycle işlemi yapar; ikinci gameplay writer olamaz. Resource kimliği değişimi, process yeniden başlatma veya farklı OperationId spam'i world/process admission sınırını sıfırlamaz. Core CommitReceipt'i yalnız guarded backend/operation registry üzerinden kabul eder; worker'ın sahte completion bildirimi durable kanıt değildir. QueryOperation/QueryTransfer sonuçları domain/actor yetkisiyle filtrelenir.

Fault injector, sahte clock ve sayaç küçültme yalnız izole conformance build'inde bulunur; production resource grant'i olarak açılmaz. [Recovery sözleşmesi](../architecture/failure-recovery.md), AC-75/82/83/86. Bu kurallar OS sandbox'ın gerçekten doğrulandığı iddiasını içermez.
