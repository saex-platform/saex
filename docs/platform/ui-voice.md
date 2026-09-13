# Modlanabilir arayüz ve konumsal voice

Durum: tasarım + R-09. [Client SDK](../resources/runtime-sdk.md) · [Animasyon/ses](../assets/animation-audio.md)

## UI yaklaşımı

İlk arayüz sözleşmesi C# tarafından güncellenen retained sahne ağacıdır: layout, metin, görüntü, durum bağları ve action olayları. Çizim renderer'ı adapter arkasındadır. GTA'nın stok HUD'u `gta.base` tarafından açılan varsayılandır; total conversion profili kendi HUD'unu seçebilir.

Resource başına UI scene, z-order alanı, input focus lease ve asset quota vardır. UI değişiklikleri frame başına toplu uygulanır. Sınırsız node ekleme veya her mouse hareketinde senkron C# IPC çağrısı yapılmaz.

| API ailesi | Davranış |
|---|---|
| CreateScene / RemoveScene | Resource epoch'una bağlı sahne ömrü |
| SubmitSceneChanges | Bounded değişiklik paketi; asset hazır koşulu |
| AcquireInputFocus | İzinli input sınıfı, öncelik ve süre |
| ActionEvent | Kullanıcı isteği; server gameplay sonucu değildir |
| Theme / Locale | Renk, ölçek, font ve dil asset referansları |

Renderer seçimi R-09'da x86 uyum, frame süresi, Türkçe metin/font, DPI ve kaynak bırakma kriterleriyle yapılır. İlk SDK keyfî tarayıcı ortamına bağımlı değildir. Web tabanlı UI ileride ayrı sandbox/capability olarak eklenebilir; otomatik güvenilen OS bridge açılmaz.

## UI etkileşim güveni

“Satın al” butonu para düşüldüğünün kanıtı değildir. Client intent gönderir, UI pending state gösterir, accepted server sonucu uygular. Client gizledi diye bir admin action yetkisiz kullanılamaz; sunucu her action'da yetki kontrolü yapar.

Input focus world/menu/camera bağlamıyla sınırlıdır. Resource çökerse focus lease kaldırılır; oyuncu kalıcı klavye kilidinde kalmaz. IME/yerelleştirme, klavye erişimi ve kullanıcı font/ölçek ayarları UI conformance kapsamındadır.

## Voice akışı

Mikrofon platformun izinli voice hizmetindedir; resource ham cihaz erişimine sahip olmaz. Capture → encode → session'a bağlı server relay → yetkili dinleyiciler → decode/spatial mix. İlk topoloji server üzerinden relay'dir; zorunlu P2P yoktur.

Codec ve audio backend R-09'da CPU, gecikme, lisans ve cihaz değişimi testleriyle seçilir. Konumsal attenuation grafik/mesafe anlamı gameplay profilinde tanımlanır. Uzak voice paketleri reliable yeniden gönderimle eski konuşmayı yığmaz; jitter buffer ve kayıp toleransı kullanılır.

| Kanal | Sunucu yetkisi | Client sunumu |
|---|---|---|
| Proximity | World ve duyma mesafesi | 3D konum, portal/occlusion |
| Radio | Frekans/rol üyeliği | İlan edilmiş filtre ve gain |
| Phone | Görüşme session üyeliği | Konumsuz veya telefon cihazına bağlı |
| Vehicle | Yolcu/araç bağlamı | Kabin attenuation |
| Interior | Oda/portal ve world | Duvar/kapı geçirgenliği |

Radio ve phone proximity mesafe filtresini aşabilir, ancak üyelik yetkisini aşamaz. Bir oyuncunun birden fazla route üzerinden aynı konuşmayı alması mix policy ile tekilleştirilir veya açıkça istenmiş efekt olarak uygulanır.

## Gizlilik ve arıza

Yerel mute, platform mute ve sunucu moderation ayrı state'lerdir. Mikrofon kullanım göstergesi platform UI'sında resource tarafından gizlenemez. Ses kaydı varsayılan kapalıdır; replay varsayılan olarak ham konuşma saklamaz.

Cihaz çıkarsa voice durur, oyun oturumu sürer. Kanal grant'i revoke edilince yeni paketler iletilmez; eski session/route epoch paketleri reddedilir. Aşırı bitrate ve route fan-out ayrıca kotaya tabidir.

Kabul: AC-09 yetki, AC-10 world/voice izolasyonu, AC-19 UI/voice, AC-20 uzun oturum. Profilde encode/decode ms, relay egress, packet loss ve gecikme ayrı raporlanır.

## Nüfus yönetimi ve hayvan etkileşim sunumu

Operatör paneli yaya/kara/hava/wildlife kanalları için effective policy, kaynak zone/schedule, hedef/mevcut/korunan/draining nüfus ve pending/ret sonucunu gösterir. Checkbox değişimi server policy kabulü veya cleanup tamamlanması sayılmaz. Oyuncu UI'sı follow/stay/tame/feed gibi yalnız seçilmiş action'ların izinli ipuçlarını ve pending sonucunu gösterir; gizli AI hedefini açıklamaz.

Hayvan sesi audio-bank/cue yolundadır; mikrofon/voice relay yolu değildir. Client mute/volume, server AI işitme stimulus'unu değiştirmez; ham oyuncu voice içeriği perception girdisi yapılmaz. [Agent algısı](../gameplay/agents-navigation.md), [Animals etkileşimi](../gameplay/animals-species.md); AC-46/58/59/65.
