# MTA:SA kaynak mimarisi incelemesi ve SAEX'e aktarım

Durum: **kaynak kodundan gözlem + SAEX tasarım çıkarımı**, 12 Eylül 2026. [Ana indeks](../../README.md) · [Ağ kararı](../decisions/network-transport.md)

## İnceleme yöntemi ve sınırı

MTA'nın yalnız wiki API listesine değil, resmî `mtasa-blue` deposunun client/server resource, element, streaming, model yükleme, ped/araç sync ve obje köprülerine bakıldı. İncelenen snapshot: `2189a0dbffb57a3f5996b1e58357b2ed028cb386`. Aşağıdaki bağlantılar bu commit'e sabittir. Kaynaklar proje dışındaki geçici inceleme dizininde okundu; SAEX içine MTA kodu veya executable eklenmedi.

Bu inceleme bir MTA runtime benchmark'ı, bütün deponun denetimi veya protokol tersine mühendisliği değildir. Bir fonksiyonda kontrol görülmesi bütün uçtan uca güvenliği kanıtlamaz; kontrol görülmemesi de başka katmanda bulunmadığını kanıtlamaz. Her başlıkta **gözlem** ile **SAEX kararı/çıkarımı** ayrıdır.

## 1. Motor köprüsü ile mod mantığının ayrımı

**Gözlem:** MTA kaynak ağacında client oyun SDK'sı/native GTA uyarlaması, deathmatch mod mantığı ve server mantığı ayrı yer alır; ağ için ayrıca SDK arayüzü bulunur. Proje, kendi oyun framework'ü ile resource sistemini tanımlar. [MTA proje açıklaması](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/README.md), [CNetServer arayüzü](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Server/sdk/net/CNetServer.h)

**SAEX kararı:** GTA adapter yalnız motor kabiliyetleri ve güvenli frame işlemleri sunar. Core, resource lifecycle ve ağ şeması bu adapter'ın dışında kalır. MTA'nın dizin ayrımı SAEX'teki OS süreç yalıtımının kanıtı olarak gösterilmez: x64 host ve kısıtlı C# worker tasarımı SAEX'in ayrıca doğrulaması gereken seçimidir. [Mimari](../architecture/overview.md)

## 2. Resource başlatmada görünürlük sırası

**Gözlem:** Server `CResource::Start`, resource element'lerini ve start mesajını yayınladıktan sonra client'ın element'leri bilmesine bağlı ertelenmiş callback'leri çalıştırır. Bu gönderim düzenidir; bütün client'lardan uygulanmış state ACK'ı beklediği anlamına gelmez. [CResource.cpp, 1222–1240](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Server/mods/deathmatch/logic/CResource.cpp#L1222-L1240)

**SAEX kararı:** Resource event'i, ilgili entity state'i ve yeni ResourceEpoch görünür olmadan C# callback'ine verilmez. GNS lane'leri arası sıra garantisi olmadığı için yalnız gönderim sırası yeterli değildir; `requiresStateRevision`, TransitionId ve bounded event kuyruğu gerekir. İlk prepare callback'i dış yan etki üretmez, activation bildirimi commit sonrası olur. [Aktivasyon](../architecture/activation-contracts.md), AC-25.

## 3. Resource bağımlılığı, durdurma ve indirme hazırlığı

**Gözlem:** Server manager restart işlemlerini kuyruklar ve dependent kaynakları ele alır. Stop yolu include ilişkilerini, resource dosyalarını, default element grubunu ve VM'yi temizler. [CResourceManager.cpp](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Server/mods/deathmatch/logic/CResourceManager.cpp#L799-L867), [CResource.cpp](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Server/mods/deathmatch/logic/CResource.cpp#L1282-L1359)

Client manager indirme grubu tamamlanınca bekleyen resource'ları değerlendirir; client resource başlangıcında ilk indirmelerin bitmesi kontrol edilir. [Client CResourceManager.cpp](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Client/mods/deathmatch/logic/CResourceManager.cpp#L103-L139), [Client CResource.cpp](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Client/mods/deathmatch/logic/CResource.cpp#L278-L319)

**SAEX kararı:** Reload tek dosya değiştirme değildir; dependency closure ve etkilenen resource grubu üzerinden yürür. Core, tüm broker lease'lerini oluşturduğu anda kaydeder; çöken worker'ın kendi cleanup bildirimine güvenmez. İndirmenin bitmesi worker ve gameplay asset readiness ile aynı şey sayılmaz. Client/server prepare, son state migration ve hazır katılımcı bariyeri **commit'ten önce** tamamlanır. [Yaşam döngüsü](../resources/lifecycle-hot-reload.md), AC-08/28/30.

## 4. Element state'i ve native nesnenin ömrü

**Gözlem:** `CClientObject::SetMass` native nesne varsa onu günceller ve client object'sindeki alanı da saklar. `SetBreakable`, native recreate işlemi için respawner kullanır; doğrudan recreate'in güvenli olmadığına dair yerel açıklama vardır. [CClientObject.cpp, 753–792](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Client/mods/deathmatch/logic/CClientObject.cpp#L753-L792)

**SAEX kararı:** Authoritative state/projection, GTA handle'ının yaşamasından bağımsızdır. Materialize yolu model → body/profile → damage stage → collision → animation/attachment → görünür aktivasyon sırasını kullanır. Recreate gerektiren alanlar adapter capability tablosunda işaretlenir; genel setter'ın her field'ı anında güvenle değiştirebildiği varsayılmaz. Property restore, sağlam varsayılanları görünür yapmadan hazırlanır. [Motor sınırı](../architecture/engine-adapter.md), [streaming](../assets/streaming-budgets.md).

## 5. Kırılabilirlik ile kalıcı yıkımı ayırmak

**Gözlem:** `CClientObject::Break` native nesnenin varlığını, kırılabilir model sınıfını ve breaking-disabled durumunu kontrol ederek native Break çağırır. Bu API'nin varlığı MTA'da kırılabilir obje desteğine somut kanıttır. [Break uygulaması](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Client/mods/deathmatch/logic/CClientObject.cpp#L747-L775)

**SAEX çıkarımı:** Bu fonksiyon tek başına arbitrary mesh fracture, durable dünya günlüğü veya reconnect state aktarımını göstermiyor. SAEX, break çağrısını yalnız görsel/native effect köprüsü olarak kullanabilecek; esas sonuç `DestructionState`, collider değişimi, fragment kimlikleri ve PlacementOverride'dır. Kırılma effect'i yeniden oynatılmadan mevcut broken stage kurulabilmelidir. Genel stok harita desteği R-05, custom davranış R-06 testine bağlıdır. [Yıkım](../assets/destruction.md), AC-04/05/16.

## 6. Streaming: hücre, eşik farkı ve grup bağımlılığı

**Gözlem:** Client streamer mekânsal sektörler, farklı giriş/çıkış mesafe eşikleri ve stream reference sayısı kullanır. Constructor'daki çıkış eşiği giriş mesafesinden 50 birim fazladır. Çıkış yolunda römorkun çekiciyle ele alınması ve referansı süren nesnenin tutulması görülür. [CClientStreamer.cpp başlangıç](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Client/mods/deathmatch/logic/CClientStreamer.cpp#L17-L33), [çıkış davranışı](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Client/mods/deathmatch/logic/CClientStreamer.cpp#L443-L492)

**SAEX kararı:** Önce world/yetki ve kaba mekânsal adaylar, sonra network interest, en son native materialization bütçesi hesaplanır. AOI ile GTA streaming aynı liste değildir. Giriş/çıkış eşiği farkı, minimum tutma süresi ve attachment closure rezervasyonu birlikte uygulanır. MTA'nın 50 değerini her SAEX world'ü için ölçülmüş ideal saymayız. Kritik closure bütçeye sığmıyorsa group'un bir yarısını collider'sız açmak yerine admission durur. [Replikasyon](../networking/replication.md), AC-29.

## 7. Model talebi ve iptal edilebilir asset lease

**Gözlem:** Model request manager hem blocking hem non-blocking yol içerir; entity'nin yeni model isteğinde eski modele ait referansı bırakır, henüz yüklenmeyen yeni modeli non-blocking ister. [CClientModelRequestManager.cpp](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Client/mods/deathmatch/logic/CClientModelRequestManager.cpp#L100-L179)

**SAEX kararı:** Yükleme isteği `EntityRef + ResourceEpoch + CatalogRevision + LeaseId` ile izlenir. İstek iptal veya entity retire olduktan sonra tamamlanan yükleme başka nesneye bağlanamaz. Background decode ile frame-bound native binding ayrı maliyetlerdir; steady-state GTA loop blocking model yükleme istemez. Hazırlık zamanı büyük iş yalnız geçiş politikasıyla bekletilir. Bu, MTA'nın tüm yüklemelerinin blocking olduğu iddiası değildir. [Streaming bütçeleri](../assets/streaming-budgets.md), AC-11/12/30.

## 8. Ped ve araç syncer seçiminden lease modeline

**Gözlem:** Ped sync, mevcut syncer uygun kaldığı sürece onu korur; yeni aday daha dar mesafeden seçilir ve joined/dimension koşulları denetlenir. Araç sync paketinde gönderenin syncer olması ve time context'in eşleşmesi aranır. [CPedSync.cpp](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Server/mods/deathmatch/logic/CPedSync.cpp#L106-L208), [CUnoccupiedVehicleSync.cpp](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Server/mods/deathmatch/logic/CUnoccupiedVehicleSync.cpp#L270-L289)

**SAEX kararı:** Geçerli owner'ı her tick en yakın oyuncuyla değiştirmeyiz. Aday dünya/scope, server rolü, asset closure, gecikme/yük ve sürücü ilişkisiyle değerlendirilir. OwnerEpoch ile stale paket reddi, kısa lease, minimum tutma/hysteresis, grup devri ve yeni owner Ready bariyeri birlikte tanımlanır. Owner olmak fizik sonucunun dürüstlüğü anlamına gelmez; validated-native kontrollerinin sınırı korunur. [Otorite](../networking/authority.md), AC-03/13/29.

## 9. Remote event ve element data yetkileri

**Gözlem:** Server event yolu `bAllowRemoteTrigger` kontrol eder ve gerçek packet caller'ı event çağrısına geçirir. Custom data yolunda client-change policy, boyut kontrolü ve broadcast/subscription ayrımı bulunur. [CGame.cpp](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Server/mods/deathmatch/logic/CGame.cpp#L2762-L2870)

**SAEX kararı:** Yerel event varsayılan olarak remote değildir. Typed intent manifesti, broker'ın doğruladığı ActorContext, field-level mutator/visibility ve oturum+resource toplam kotası zorunludur. Client payload'daki `source` entity'si yalnız hedef/context'tir, oyuncunun kimliği değildir. Public generic key/value broadcast varsayılanı yerine yalnız şemada ilan edilmiş replike alanlar gönderilir. MTA'nın subscription ve veri koruma yetenekleri yok sayılmaz. [SDK](../resources/runtime-sdk.md), [entity şeması](../architecture/entities-worlds.md), AC-09/25/33.

## 10. Ertelenmiş silme ve callback güvenliği

**Gözlem:** Server element deleter, silmeyi işaretleme/unlink ile gerçek nesne imhasını ayrı adımlarda ele alır ve silme listesi tutar. [CElementDeleter.cpp](https://github.com/multitheftauto/mtasa-blue/blob/2189a0dbffb57a3f5996b1e58357b2ed028cb386/Server/mods/deathmatch/logic/CElementDeleter.cpp#L18-L51)

**SAEX çıkarımı:** Ertelenmiş retirement yararlı olsa da bu source düzeni birebir kopyalanmayacak. SAEX önce retiring/reentrancy guard koyar, sonra callback üretir; end-of-tick/frame free sonrası generation değişir. Worker crash, callback sırasında destroy ve geç gelen asset yüklemesi aynı core registry üzerinden temizlenir. [Aktivasyon sözleşmesi](../architecture/activation-contracts.md), AC-12/30.

## MTA'nın transport'u hakkında çıkarım sınırı

Resmî derleme rehberi ağ modülünü `net.dll/net.so` olarak önceden derlenmiş dağıtım şeklinde tanımlar; sayfa derleme adımları için kontrol gerektirdiğini de belirtir. Açık kaynak CNet arayüzünden alttaki transport'un ENet olduğu çıkarılamaz. Bu inceleme **“MTA ENet kullanıyor” sonucuna varmaz** ve SAEX kararını böyle bir varsayıma bağlamaz. [MTA derleme rehberi](https://wiki.multitheftauto.com/wiki/Building_MTASA_Server_on_GNU_Linux)

## Aktarım tablosu ve kabul

| Referans ilkesi | SAEX'te somut sözleşme | Bedel / doğrulama |
|---|---|---|
| Resource ve element yaşamı | Core-owned lease registry, dependency-group reload | Süreç/RAM ve cleanup maliyeti; AC-08/30 |
| Başlangıç sırası | State hazır olmadan event yok; çapraz lane bariyeri | Kuyruk sınırı ve resync; AC-25 |
| Streaming | AOI/materialization ayrımı, hysteresis, closure reservation | Prefetch/RAM; AC-11/29 |
| Syncer | Lease epoch, ready, aday kararlılığı | Devir gecikmesi; AC-03/29 |
| Model/property | Materialize sırasında authoritative profil kurma | Native yetenek araştırması; R-02/04/05 |
| Event/data güveni | Typed schema, ActorContext, açık remote izin | SDK/manifest ayrıntısı; AC-09/33 |
| Obje kırılması | Kalıcı state, collision ve prepared fragments | Authoritative yıkım maliyeti; AC-04/05/16 |

Bu iyileştirmeler SAEX tasarımının daha açık ve denetlenebilir olmasını hedefler. MTA'nın tamamından üstün olduğuna ilişkin sonuç değildir. Ürün karşılaştırması aynı sahne, donanım, gecikme ve özellik koşullarında gelecekte ölçülür. Kod aktarımı kararı verilirse ilgili dosyanın lisansı ayrıca değerlendirilir; burada mimari öğrenme ve bağlantılı kaynak atfı yapıldı.

## V0.4: NPC kontrolü ve hayvan uyarlaması için ek referanslar

Bu ek 12 Eylül 2026'da okunan resmî API belgelerine dayanır; yukarıdaki commit snapshot'ına yeni C++ satır denetimi eklenmiş gibi yorumlanmamalıdır. MTA server ped kontrol girdilerini otomatik senkronize etmez; [setPedControlState](https://wiki.multitheftauto.com/wiki/SetPedControlState). Entity başına özel animasyon replacement'ı da otomatik ağ eşitlemesi sunmaz ve partial animation sınırlaması taşır; [engineReplaceAnimation](https://wiki.multitheftauto.com/wiki/EngineReplaceAnimation).

Model replacement ped'leri kapsar, fakat load success genel rig/controller/hit uyumunu kanıtlamaz; [engineReplaceModel](https://wiki.multitheftauto.com/wiki/EngineReplaceModel). SAEX bu bulgulardan task/controller/animation/motion'un ayrı sözleşmesini ve [hayvan conformance'ını](../gameplay/animals-species.md) çıkarır. MTA'nın bütün AI görevlerinin eksik olduğu veya hiçbir hayvan projesinin yapılamayacağı sonucu çıkarılmaz. FiveM population ownership ise ayrı [resmî OneSync](https://docs.fivem.net/docs/scripting-reference/onesync/) referansıdır; GTA Online motor mekanizması GTA:SA'ya hazır taşınmaz.
