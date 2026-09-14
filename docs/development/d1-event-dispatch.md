# D1-N2 — Uygulama olayına geçiş sınırı

Kod 0.1.25 / mimari v0.32. Sınırlı olay aktarımı kesiti doğrulandı. [Durum](status.md) · [Önceki instance sınırı](d1-instance-startup.md).

## Sorumluluk ve veri akışı

Yeni `--observe-event-dispatch <exe> <absolute-cwd>` yalnız owned x86 child'da, önceki instance dönüşü doğrulandıktan sonra 0x748732 TEST/JNZ/PUSH dizisini yürütür. 0x748739 CALL önünde `[24,0]`; 0x619B60 dağıtıcı girişinde `[return,24,0]`; 0x619B6C uygulama işleyicisi CALL önünde `[24,0,savedEDI,savedESI,return,24,0]` doğrulanır. Üçüncü durakta CALL henüz çalışmamıştır. `RsEventHandler` adı kaynak karşılaştırmasına dayanır; oyuncu/resource event API'si değildir.

Caller ESP=C için üç durak sırasıyla C−8, C−12, C−28 bekler. Başlangıç EAX=EBX=0 olmalıdır. İlk iki durakta nonvolatile register'lar korunur; üçüncüde ESI=0, EDI=24 ve EBX/EBP eski değerdedir. ECX/EDX/EAX üç durakta değişmez; TF/DF reddedilir. Caller'ın yaşayan 156-byte yığını ve instance last-error değeri korunur. Named event kimliği her durakta yeniden doğrulanır. Bu işlemler debugger'ın uygulama register/yığın belleğini değiştirmesiyle yapılmaz; CPU doğal CALL/PUSH/MOV yürütür. Önceki platform bastırması kendi sentetik bağlam sözleşmesini korur.

## Arayüz ve kapsam

`EventDispatchSpec`, iki rel32 CALL/target bağı ve read-only 21-byte application prefix taşır. Sabit yedi byte caller dizisi ve 12-byte dağıtıcı prologue'u kaynakta tanımlıdır. `event-dispatch-policy.json` strict generator ile instance digest'ine, oradan exact engine/DLL zincirine bağlıdır. Uzak adres veya script girişi kabul edilmez. C++ trace/API kullanıcıları yeniden derlenir; bootstrap C ABI 1, GNS, otorite ve IPC aynı kalır. Kaldırılan özellik veya kalıcı migration yoktur.

DR0 üç durağa taşınır; DR1 startup IAT, DR2 ekstra ASI, DR3 önceki-instance pencere kolu korunur. Önceki instance modu 0x748732'de bitmeye devam eder; yeni modda bu ara kanıt `instanceStartupObservation.verified=true` olarak korunur. `eventDispatchObservation` eski modlarda null, yeni modda ayrı stage/reached/continued/shape/frame/stack/verified alanları taşır. Uygulama işleyicisi CALL izni ve renderer doğrulaması false'tur. Başarı `event_dispatch_boundary_verified`, exit 3; ret 1, kullanım 2. `canAttach=false`, `initializationVerified=false` değişmez.

## Hata davranışı ve genişleme

Yanlış parent/spec/rel32 bağı çalıştırmadan; yanlış caller dizisi, prologue veya uygulama prefix'i instance dönüşündeki önkoşulda reddedilir. Devam sonrası shape/yığın/register/last-error/nesne farkı, exception, owner veya kota sorunu başarı üretmez. Başarı ve ret sonunda owned child çıkışı ayrıca doğrulanır. Gerçek API sonrası eski CALL bağlamına rollback yapılmaz. Bu araç native güvenlik sandbox'ı veya production launcher değildir.

Yerel executable'da 0x53EC10 uygulama işleyicisinin girişinde 0x53EC1F → 0x4018CE JMP vardır. Bu durum sabit dosyanın disassembly'sinden görülür; jump hedefinin güvenli olduğu veya kaynak örneğindeki switch gövdesiyle aynı çalıştığı varsayılmaz. 21-byte örnek eşitliği sadece kimlik/kararlılık kanıtıdır. Sonraki iş uygulama işleyicisi yönlendirmesinin ve seçilen başlatma kolunun yan etkileri/ABI incelemesidir. Renderer/window/frame hâlâ sonraki kapılardır; AC-90/AC-91, N2/N3/D1/D2 bütünü kapanmaz.

## Kabul senaryoları

Normal üç durak ve `[24,nullptr]` aktarımı; bozuk rel32, prologue ve prefix; yanlış register/yığın/return/argüman; event kotası ve yanlış owner; önceki instance terminalinin korunması; mevcut event ile daha erken ret; uygulama gövdesi canary kontrolü ve 12 warm handle çevrimi. Portable frame kontrolleri gerçek Windows observer'ının kullandığı doğrulayıcıyı sınar; fixture başarısı GTA initialization kanıtı değildir.

## Kaynak ve doğrulama

GTA adresleri, hash'i `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac` olan yerel executable'ın PE/disassembly verisinden çıkarıldı. [gta-reversed AppEventHandler kaynağı](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/app/app.cpp) 0x53EC10'u uygulama olay işleyicisi olarak tanımlar ve initialize kolunu renderer başlatma kolundan ayırır. Bu referans açıklama içindir; SAEX'e kaynak kopyalama, yeni dependency veya native çağrı yetkisi getirmez. Statik dump'lar `event-dispatch-disassembly.txt`, `app-dispatch-disassembly.txt`; final sonuçlar aşağıda kayıtlıdır.

Kaynak karşılaştırması `db11601c0897cf4197c972bb591fa83c8cd40da6` commit’ine sabitlendi; üç dosyanın hash kaydı `out/research/event-dispatch/sources.json` içindedir. [Platform dağıtıcısı](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/app/platform/platform.cpp) 0x619B60 eşlemesini verir. [RsEvent enum](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/game_sa/RenderWare/rw/skeleton.h) rsINITIALIZE=24 ve rsRWINITIALIZE=21 değerlerini ayırır. Bu kesit initialize olayını iletir; iki işleyicinin dönüşü veya renderer initialization sonucu değildir.

## Nihai doğrulama

Canonical kayıt `out/verification/engine/dispatch-gta-evidence.json`, derleme/hash özeti `dispatch-final-verification.json`. Yeni private dizinler `out/experiments/gta-dispatch-f01a00ce-{Debug,Release}-v1`; eski dizinler ve orijinal oyun kurulumu korunur. İlk keşif `dispatch-explore-Debug-1.json` final 12 koşuya katılmaz.

| Ölçüm | Sonuç |
|---|---|
| Gerçek GTA pozitif | Debug 6/6, Release 6/6; 12/12 |
| Doğal duraklar | 0x748739 CALL → 0x619B60 entry → 0x619B6C application CALL öncesi |
| Argüman / ABI | Üç yığın şekli, C−8/C−12/C−28, beklenen register ve dönüş adresi eşleşti |
| Caller koruma | Yaşayan 156-byte stack, last-error ve named event kimliği korundu |
| Uygulama örneği | 21 byte her durakta exact; 0x53EC1F yönlendirmesi yürütülmedi |
| Matris | 34/34 beklenen sonuç: 12 pozitif, 15 legacy mod, beş olumsuz koşul, iki event çakışması |
| Child ömrü | Oluşturulan 31/31 süreç kapandı; üç koşulda child oluşmadı |
| Dosyalar | 190/190 girdi hash'i korundu: önceki 175 + 14 private dosya + dispatch policy |
| x86 Debug / Release | Her profilde 29 native suite / 79 managed / 212 Python |
| x64 Debug / Release | Her profilde 13 native suite / 79 managed / 154 Python |
| Yeni testler | 81 portable spec/frame kontrolü; 11 x86 senaryo, application canary ve 12 warm çevrim |

Pozitiflerde 65–71 debug event, thread sayıları [4]; 128 event/16 thread/5 saniye kotaları korunur. Host foreground timeout yalnız SPI_GET ile 2147483647 → 2147483647 ölçüldü. Bu eşitlik tek başına sistem ayarı çağrısının çalışmadığı kanıtı değildir; önceki bastırma/bağlam/canary kanıtıyla birlikte değerlendirilir.

236 kaynak/yapılandırma girdisi ve iki profilin probe/DLL/map artifact'leri son kontrolde aynı kaldı. İlk derleme, beş policy testi ve targeted Debug testleri geçti; `dispatch-build-initial.log` / `dispatch-tests-initial.log` korunur. İlk fixture warm sayımı 139 → 139 idi; final genel akışlarda warm eşitliği test olarak tekrar geçti. Event fixture'ına diğer instance testinden ayrı nesne adı verildi. Linux/hosted CI/N1 SDK yeniden koşulmadı.

| Kimlik | SHA-256 |
|---|---|
| Event dispatch policy | `6265420804ada87df4beae0fc034fcb0478ad408dd05d04e9173d6d02f681058` |
| Debug probe | `2df0e7fd5faf417105bf0093ffc45d8818f0dc2dd967a4cb8db03235f9b8fcd6` |
| Debug dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release probe | `b6a1a512f31d5a0305f235bf1a8e0628b7da034c23a7a9ba533bfec0c27edb4b` |
| Release dll | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

GTA işleyicisinin çalışması veya başarılı initialize dönüşü bu kanıtın parçası değildir. Prefix eşitliği, reverse-engineered kaynak gövdesinin çalışacağı anlamına gelmez. Production SDK/IPC/sandbox, renderer ve doğal frame kapsamları açık kalır.

## Kod 0.1.26 bağlantısı

`--observe-application-routing` önceki event-dispatch kanıtından sonra yalnız rsINITIALIZE=24 rotasını yürütür: işleyici entry → executable detour → indirect JMP → ilk oyun initializer CALL öncesi. 39 index/11 hedef tablosu, rel32/absolute operand ve dört yığın/register sınırı doğrulanır. [Sözleşme ve sonuç](d1-application-routing.md). Önceki mod kendi AppEventHandler CALL öncesi terminalini korur. Yeni modda `eventDispatchObservation.applicationHandlerCallAllowed=true`, routing nesnesinde initializer çağrı izni false olur; önceki stage/verified ara kanıtı korunur. C++ trace/API yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni bağımlılık/kaldırılan özellik/kalıcı migration yoktur. Oyun initializer gövdesi, RsInitialize, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## 14 Eylül 2026 — Windows fixture taşınabilirliği

Startup-return ve devamındaki fixture zinciri, sistem DLL reçetelerini test makinesinin diskte tutulan PE dosyalarından çıkarır. Ortak okuyucu ve negatif doğrulamalar [test kapsamı notunda](d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği) açıklanır; üretim policy/hash kuralları ve bu belgedeki gerçek GTA kanıtının sınırları aynıdır.
