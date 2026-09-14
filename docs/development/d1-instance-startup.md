# D1-N2 — Named event ile instance başlangıcı

Kod 0.1.24 / mimari v0.31. Sınırlı instance başlangıcı kesiti doğrulandı. [Durum](status.md) · [Önceki sınır](d1-platform-suppression.md).

## Sorumluluk ve veri akışı

`--observe-instance-startup <exe> <absolute-cwd>` owned x86 child üzerinde önceki exact pin zincirini ve platform CALL bastırmasını doğrular. 0x74872D'deki instance CALL'dan 0x7468E0 helper'ına doğal devam eder. Tam 91 byte gövde, dokuz image operandı, nesne adı pointer'ı ve NUL sonlu literal yeniden okunur. CreateEventA çağrı hedefi kernel32 altı byte `FF25` thunk ve kernelbase 20 byte prefix/aktif mapping kimliği üzerinden doğrulanır. Altı byte sınırı sonraki thunk'ın kısmi relocation operandını karşılaştırmaya katmaz. GetLastError kimliği ve TEB okuyucusu önceki sözleşmeyle aynıdır.

CreateEventA CALL 0x7468EC önünde `[nullptr,FALSE,TRUE,name]`, helper dönüş adresi ve nonvolatile register'lar doğrulanır. 0x7468F2'de NULL olmayan handle, ESP ve child last-error okunur; child handle'ı observer'a yalnız SYNCHRONIZE erişimiyle kopyalanır. Aynı adla OpenEvent ve CompareObjectHandles eşitliği şarttır. İki observer handle'ı hemen kapanır; child handle'ı kapatılmaz. 0x7468F8'de doğal GetLastError dönüşü karşılaştırılır. Hata 183 ise önceki instance algılandı olarak bitirilir; yeni event için bu exact profil hata 0 ister. Diğer başarı hata değerleri belirsiz sayılır. Windows'un bütün başarılı CreateEvent çağrılarında sıfır last-error garantisi olduğu iddia edilmez.

Yeni event yolunda doğal CMP/JNE/XOR/RET, 0x748732'de çağıranın sonraki komutu önünde biter. EAX=0, caller ESP/nonvolatile register'lar ve yaşayan 156-byte caller stack korunmalıdır. DR0 duraklar arasında taşınır; DR1 startup IAT, DR2 ek ASI ve DR3 0x7468FF mevcut-instance kolu korumaları tutulur. Child sonlandırılır; pencere/renderer/frame yürütülmez.

## Yan etkiler ve hata davranışı

`Grand theft auto San Andreas` adı Windows oturumunun ortak namespace'indedir; süreç özelinde değildir. Yeni event auto-reset ve başlangıçta signaled'dır, child ömrünce başka eşzamanlı oyun başlangıçlarını etkileyebilir. Mevcut event açılırken reset/initial-state argümanları Windows tarafından uygulanmaz. Observer event üzerinde Wait/Set/Reset çağırmaz. Son handle kapandığında nesne yok olur; başka sahip varsa yaşar. Aynı addaki farklı nesne türü NULL handle üretirse GetLastError sonucu kaydedilerek ret verilir; GTA'nın yalnız 183 kontrolüne güvenilmez. İsim eşitliği sahibin güvenilirliğini kanıtlamaz.

Önceki suppression komutu eski bağlamı geri alarak bitmeye devam eder. Yeni modda suppression dönüşü önce tam doğrulanır; ardından gerçek API yürüdüğü için eski EIP/ESP bağlamı geri yazılmaz. `platformSuppressionObservation.applied=true`, `restored=false`, tam `verified=false`; ayrı `instanceStartupObservation.suppressionBoundaryValidated=true` bu geçişi ifade eder. 156-byte yaşayan caller stack, önceki 172-byte snapshot'ın tüketilmiş 16 byte argümanları dışındaki bölümüdür. API'nin stack/OS yan etkileri rollback diye sunulmaz. Başarı ve ret sonunda owned child çıkışı ayrıca şarttır. Mevcut-instance kolu ve pencere odağı çağrıları izinli değildir; DR3'e beklenmeyen geliş terminal ret verir.

## Arayüzler ve genişleme

Strict `instance-startup-policy.json`, suppression source digest'iyle tüm parent zincirine bağlıdır. Remote script/raw adres girişi yoktur. `InstanceStartupSpec`, `InstanceStartupObservation` ve yeni observer metodu C++ yüzeyine eklendi; tüketiciler yeniden derlenir. Bootstrap C ABI 1, GNS/otorite/IPC değişmez; kaldırılan özellik veya kalıcı migration yoktur. Başarı reason `instance_startup_verified`, exit 3; mevcut instance dahil ret 1, kullanım 2. `canAttach=false`, `initializationVerified=false` korunur. Eski modlarda yeni JSON alanı null'dır.

Sonraki araştırma çağıranın TEST/JNZ ve yeni-instance sonrası uygulama/renderer önkoşullarıdır. Bu kesit production çoklu-instance politikası, sandbox veya N2/N3/D1/D2 kapanışı değildir.

## Kabul ve doğrulama

Yeni event ve doğal helper dönüşü; mevcut nonsignaled manual-reset event'in değişmemesi; aynı adlı mutex çakışmasının NULL/error 6 ile reddi; yanlış ad/operand/thunk/target/spec; event kotası; yanlış owner; eski suppression terminali; pencere canary sensitivity kontrolü ve 12 warm handle döngüsü test kapsamındadır. Sonuçlar aşağıdaki nihai kanıta bağlıdır.

## Kaynaklar

Event oluşturma/açma, reset bayrakları, aynı isimde farklı nesne ve kapanış ömrü [Microsoft CreateEventA](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventa) sözleşmesine dayanır. Gözlemciye kopyalama ve kaynak handle'ı koruma [DuplicateHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle), kimlik karşılaştırması [CompareObjectHandles](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-compareobjecthandles) ile tanımlıdır. GTA adresleri yerel hash'i sabit executable'ın disassembly/PE verisinden çıkarılmıştır; genel GTA veya bütün Windows sürümlerine genellenmez.

## İnceleme ve ilk hata düzeltmeleri

İlk policy doğrulaması pencere kolunun iki global pointer'ını da dosyadaki raw bölümde aradığı için reddetti. Bunlar PE'nin sıfırla oluşturulan `.data` alanındadır. Yalnız bu iki yürütülmeyen pencere verisi operandında non-executable virtualSize kabul edildi; yürütülen kod, literal, isim pointer'ı ve IAT'ler raw bölüm koşulunu korur. Yeni gövde opcodları/branch offsetleri değiştirilemez. İlk generator reddinden sonra header bulunamadığı için başlayan keşif derlemesi de başarısız oldu; başarılı generator ardından tekrar derlendi.

Microsoft CompareObjectHandles için Kernelbase.lib belirtir; yerel SDK'de bu dosya bulunmadığından önce unresolved symbol, ardından eksik library hatası alındı. Observer aynı belgelenmiş API'yi zaten yüklenmiş kernelbase.dll üzerinden adla çözer. Export bulunamazsa event kimliği doğrulanmaz ve güvenli ret verilir; karşılaştırmayı atlayan yol eklenmedi. Child'a DLL yüklenmez ve pin listesi genişlemez.

ASLR kullanan fixture'ın helper prefix'i ilk testte diskteki mutlak operandları taşıdığı için spec reddedildi. Test beklenen operandları child image tabanına göre kuracak şekilde düzeltildi; gerçek GTA profili kendi exact image tabanı kapısını korur. IAT operandını değiştiren negatif senaryonun target-prefix bağı nedeniyle runtime'dan önce spec aşamasında reddedilmesi test beklentisine işlendi; hiçbir ret koşulu gevşetilmedi. İlk loglar `instance-build-{initial,second,third,fourth,fifth}.log` ve `instance-tests-{initial,second,third}.log` altında korunur.

Debug fixture'da 129 portable kontrol, 15 senaryo, pencere canary sensitivity kontrolü ve 12 warm döngü geçti; observer handle sayısı 132 → 132. Bu ön sonuç dört profil ve gerçek GTA final matrisinin yerine geçmez. `instance-explore-Debug-1.json` ilk özel GTA denemesidir; nihai 12 koşuya ayrıca katılmaz.

## Nihai doğrulama

Canonical kayıt `out/verification/engine/instance-gta-evidence.json`, derleme/hash özeti `instance-final-verification.json`. İlk keşif koşusu final 12 koşuya eklenmez. Yeni yedi dosyalı private kopyalar `out/experiments/gta-instance-f01a00ce-{Debug,Release}-v1`; eski kopyalar ve orijinal oyun kurulumu korunmuştur.

| Ölçüm | Sonuç |
|---|---|
| Yeni GTA instance yolu | Debug 6/6, Release 6/6; toplam 12/12 |
| Son doğal durak | 0x748732; caller TEST çalışmadan önce EAX=0, caller ESP/nonvolatile ve yaşayan 156-byte stack eşit |
| OS nesnesi | Child CreateEvent handle'ı adıyla açılan event ile aynı; GetLastError=0 |
| Önceki instance | Aynı isimli nonsignaled manual-reset event açıldı, 183 alındı; 0x7468F8'de ret, sinyal durumu korundu |
| Farklı nesne türü | Aynı isimli mutex; NULL handle/error 6, 0x7468F2'de ret |
| Matris | 33/33 beklenen sonuç: 12 pozitif, 14 eski mod, beş önceki olumsuz koşul, iki event çakışması |
| Child ömrü | Oluşturulan 30/30 child kapandı; üç koşulda child oluşmadı |
| Dosyalar | 175/175 girdi hash'i korundu: önceki 160 + 14 yeni kopya dosyası + instance policy |
| x86 Debug / Release | Her birinde 27 native suite / 79 managed / 205 Python |
| x64 Debug / Release | Her birinde 12 native suite / 79 managed / 149 Python |
| Yeni test kesiti | 129 portable kontrol, 15 x86 senaryo, pencere canary kontrolü ve 12 warm çevrim |

Debug final verbose kaydında observer handle sayısı 132 → 132. Gerçek pozitiflerde 61–67 debug event; thread sayıları [4]. Mevcut 128 event/16 thread/5 saniye kotaları artırılmadı. Host foreground timeout yalnız SPI_GET ile 2147483647 → 2147483647 ölçüldü. Bu eşitlik tek başına API'nin çalışmadığı kanıtı değildir; önceki CALL bastırma/bağlam/canary kanıtıyla birlikte değerlendirilir.

228 kaynak/yapılandırma girdisi ve iki profilin probe/DLL/map artifact'leri final kontrolde aynı kaldı. Her pozitif child sonrasında event artık açılamadı; mevcut-event testinde yalnız testin sahip olduğu manual-reset event üzerinde sıfır süreli durum okuması yapıldı. Observer'ın kendisi event sinyalini tüketmez.

| Kimlik | SHA-256 |
|---|---|
| GTA executable | `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac` |
| Instance policy | `26c06199eaa5d4e14c1e693ac60bd6df5c69cb3557e80b1baad320d7fc46f7e9` |
| Debug probe | `f45a0745d6c4cabac400d3b151ce3d815ea572e822ebc272873e4da5836e45a5` |
| Debug dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release probe | `329ec77c8e99783e814b978f5ff84f32b2fc54a6bc958b0fbb03661f15cddf22` |
| Release dll | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Linux/hosted CI/N1 SDK bu kesitte yeniden koşulmadı. Native kaynak ve fixture başarısı production IPC, sandbox, pencere/renderer veya doğal game frame kanıtı değildir. Bu kesit AC-90 alt sonucudur; bütün N2/N3/D1/D2 kapılarını kapatmaz.

## Kod 0.1.25 bağlantısı

Ayrı event-dispatch modu, instance dönüşünden doğal olay dağıtıcısı CALL/entry ve uygulama işleyicisi CALL önüne ilerler. Üç durakta argüman, dönüş adresi, register ve yaşayan caller stack doğrulanır; uygulama işleyicisi çalıştırılmaz. [Sözleşme ve sonuç](d1-event-dispatch.md). Eski instance terminali, C ABI 1/GNS/otorite aynı; yeni bağımlılık/kalıcı migration yoktur. AppEventHandler gövdesindeki executable yönlendirmesi, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.
