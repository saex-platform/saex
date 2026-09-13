# Actions ve Effects: ortak etkileşim geliştirme paketi

Durum: ADR-22; **isteğe bağlı referans paket**, çekirdeğin zorunlu oyun kuralı değildir. [SDK](../resources/runtime-sdk.md) · [Sandbox kit'leri](sandbox-kits.md)

## Sorumluluk

Kapı açmak, duvar onarmak, yakıt doldurmak, eşya kullanmak, terminale bağlanmak veya özel yetenek çalıştırmak aynı araçlarla tanımlanabilsin. Her mod geliştiricisi cooldown, maliyet, iptal, animation ve tekrar isteği akışını ayrı ayrı eksik kurmak zorunda kalmasın. Bu paket kullanılmadan da typed C# komutlarla oyun yazılabilir.

Epic'in Gameplay Ability System belgelerinde ability, attribute ve effect ayrımı bulunur; bu ayrım tasarım referansıdır. SAEX'e Unreal runtime veya Blueprint sistemi eklenmiyor. [Epic resmî referansı](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine)

## Tanımlar

| Tanım | Sahip olduğu alan |
|---|---|
| ActionDefinition | İzin/target koşulu, start/progress/complete/cancel, süre ve interaction şekli |
| AttributeDefinition | Şemalı gameplay değeri, birim, sınır ve mutator domain |
| EffectDefinition | Attribute'a uygulanacak işlem, süre, stacking ve source ilişkisi |
| PresentationCue | Animasyon, ses, UI veya cosmetic efekt; gameplay sonucu üretmez |
| ActionInstance | Actor/target, ActionId, DefinitionRevision, server start/end tick, phase ve cause |
| CostPolicy | Ödeme/rezervasyon anı, başarılı/iptal/timeout sonucu ve gerekiyorsa telafi |

Tag'ler türe ve koşula yardımcıdır; `is.admin` gibi bir tag taşımak yetki vermez. Modifier önceliği/add/multiply/clamp sırası şemada sabittir; floating-point veya container iteration sırası oyun kuralı olamaz. Stacking en fazla instance sayısı ve replace/refresh/independent politikası içerir. Sonsuz status veya efekt zinciri default değildir.

## Sunucu akışı ve istemci hissi

UI/input → typed ActionIntent → ActorContext/target/world/revision doğrulama → koşul/cooldown/cost → pending/start transaction → ilerleme → complete/cancel transaction → filtreli state ve PresentationCue.

İstemci input anında nişan alma, progress gösterimi veya izin verilmiş animasyon tahmini yapabilir. Hasar, item tüketimi ve kalıcı collision tahminle kesinleşmez. Server action'ı reddederse cue durur veya uzlaştırılır. Native hareket input replay'i için yeni garanti eklenmez.

`ActionStarted` aksiyonun tamamlandığı anlamına gelmez. Uzun aksiyonda her kabul edilmiş adım `(ActionId, StepId)` cause'undan türetilen OperationId ile tekilleştirilir. Aynı step tekrar gelirse aynı receipt döner. Complete öncesi hedef generation/catalog/izin ve gerekiyorsa occupied volume yeniden kontrol edilir.

## Süre, maliyet ve iptal

Cooldown ve aktif süre sunucu simulation tick'iyle ölçülür. Offline süre ilerlemesi isteniyorsa ayrı persistence policy açık server timestamp ile tanımlanır; client duvar saati kabul edilmez. Varsayılan resource stop/disconnect davranışı action türüne göre `cancel`, `suspend` veya `continue-server` olarak WorldPlan'da seçilir; tanımsızsa cancel olur.

`continue-server`, sağlıklı ve yetkili bir provider'ın state ownership'ini devralabilmesini gerektirir; resource process'i ölürken aynı kod kendiliğinden core içinde çalışmaya devam etmez. Devralma yapılamazsa ilan edilmiş suspend/cancel fallback uygulanır ve alınmış ücretin akıbeti CostPolicy ile belirlenir.

CostPolicy `charge-on-start`, `reserve-then-charge` veya `charge-on-complete` olabilir. Rezervasyon gerçek para düşme ile aynı şey değildir. Start'ta alınmış bir ücret cancel sonrası geri verilecekse bu ayrı idempotent telafi transaction'ıdır; eski journal kaydı silinmez. Harici ekonomi sağlayıcısında outbox/pending sonuç kullanılır, otomatik distributed atomiklik iddia edilmez.

## İki referans aksiyon

| Aksiyon | Koşullar ve akış | Yarışan işlem davranışı |
|---|---|---|
| Duvar onarma | Yetki + malzeme rezervi → süre → hacmi yeniden kontrol → onarım ve maliyet commit | Hedef yıkılır/silinir/alan dolar ise complete ret; rezerv policy'ye göre bırakılır |
| Araç yakıt doldurma | Araç/pompa erişimi → session → ölçülmüş adımlar halinde transfer → stop | Her adım ayrı receipt; bağlantı kopunca son accepted litre/ücret korunur |

Tek tuş, basılı tutma, menü, controller veya erişilebilirlik girdisi aynı action schema'sını kullanabilir. Interaction prompt sunucuya ait gizli koşulları sızdırmaz; public availability hint oyuncu için bilgilendirmedir ve server kontrolünün yerine geçmez.

## Hot reload, genişleme ve hata

Devam eden action DefinitionRevision'a pinlenir. Uyumlu migration açıkça varsa yeni tanıma taşınır; aksi halde eski sürüm tamamlanana kadar tutulur veya ilan edilmiş cancel policy uygulanır. Yeni patch, çalışan aksiyonun ücretini/süresini sessizce değiştiremez. Effect sahibi resource ölürse core ownership ve domain provider sözleşmesi geçerlidir.

Başlangıç uygulaması C# paket ve veri tanımlarıdır. Görsel node editörü daha sonraki authoring yüzeyidir; oyuncudan yürütülebilir expression grafiği alınmaz. API aileleri `RequestAction`, `QueryAvailableActions`, `CancelAction`, `ApplyEffect` ve `InspectAction` olur; her biri typed schema/grant ile çalışır.

Hatalar koşul, cooldown, hedef kaybı, maliyet, deadline ve migration sebebiyle ayrılır. AC-39, AC-37 ve AC-17 kabulüdür. İlk iki örnek D3'te isteğe bağlı pakette doğrulanır; R-15 tamamlanmadan genel aksiyon kütüphanesi çalışıyor sayılmaz.

## Agent ve hayvan etkileşimleri

Araçtan sürücüyü çıkarma, hayvanı evcilleştirme/besleme, takip komutu, ısırma ve loot toplama aynı Action/Cost/Cancel ilkelerini kullanabilir. Actions kit'i seçilmemişse provider eşdeğer typed command/transaction sözleşmesini uygular; kit zorunlu dependency olmaz. Target/seat/owner/health revision'ları, izinli tür action'ı ve güncel collision sorgusu tamamlanmadan sonuç kesinleşmez.

Evcilleştirmede item debit, ownership, ControlClass ve population slot/kota disposition birlikte ele alınır. Population drain ile çakışma tek rezerv/kabul yolunda çözülür. Ölüm veya task cancel eski strike/complete'i geçersiz kılar; animation marker'ı doğrudan hasar veya ödül yazamaz. [Animals](animals-species.md), [Population](population-traffic.md), AC-49/58/59/63. İşlem kabulü sonrası worker crash durable ownership/death/loot sonucunu geri silmez.

## İşlem sonucu belirsizken aksiyon

Animation marker veya client timeout işlem sonucu değildir. Oyun cooldown ve cast süresi SimulationTick'te ilerler; external result/rezerv/lease bekleyişi ControlTime'a bağlıdır. Durable effect commit edilip callback kaybolduysa aynı OperationKey ile sonuç sorgulanır; yeni action başlatmak ikinci debit/ödül yaratamaz. Cancellation, kesinleşmiş effect'i silmez; telafi ayrı yetkili işlem ve yeni receipt'tir.

`OutcomeUnknown` sırasında etkilenen aggregate fenced kalır; unrelated aksiyonlar bütçe içinde sürebilir. Reload eski transient job'ı reddederken core-owned receipt'i kurtarır. [Arıza sözleşmesi](../architecture/failure-recovery.md), AC-73/74/83; harici provider outbox garantisi cross-world entity transfer desteği vermez.
