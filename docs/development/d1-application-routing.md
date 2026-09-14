# D1-N2 — Uygulama başlatma dalının doğrulanması

Kod 0.1.26 / mimari v0.33. Sınırlı başlatma dalı kesiti doğrulandı. [Durum](status.md) · [Önceki dağıtıcı sınırı](d1-event-dispatch.md).

## Sorumluluk ve veri akışı

`--observe-application-routing <exe> <absolute-cwd>` owned x86 child'da önceki instance/event-dispatch kanıtlarını tamamlar. 0x619B6C CALL doğal yürür, 0x53EC10 AppEventHandler girişinde ilk durak alınır. Event 24 stack'ten okunur; unsigned 38 sınırı geçilmediği için JA alınmaz; iki NOP ve 0x53EC1F JMP ile 0x4018CE detour girişine gidilir. Burada MOVZX EAX,[EAX+0x53ED08] değeri 5 yapar, JMP 0x53EC24'e döner. Üçüncü durak indirect JMP önündedir; `[0x53ECDC+4*5]` 0x53EC2B'yi seçer. Dördüncü durak bu adresteki CALL önündedir. CALL hedefi 0x53BB50 CGame::InitialiseOnceBeforeRW olarak kaynakla eşlenir; CALL ve hedef gövdesi çalıştırılmaz.

| Durak | ESP | EAX | Anlam |
|---|---|---|---|
| 1 — 0x53EC10 | C−32 | 0 | Handler CALL doğal, ilk handler komutu henüz çalışmadı |
| 2 — 0x4018CE | C−32 | 24 | Event okundu ve bounded event kolu seçildi; tablo henüz okunmadı |
| 3 — 0x53EC24 | C−32 | 5 | Index okundu; indirect target JMP henüz çalışmadı |
| 4 — 0x53EC2B | C−32 | 5 | Indirect JMP doğru CALL adresine ulaştı; initializer henüz çağrılmadı |

C, instance doğal dönüşündeki caller ESP'dir. Sekiz kelime `[applicationReturn,24,0,savedEDI,savedESI,dispatcherReturn,24,0]` olmalıdır. ESI=0, EDI=24, EBX=0 ve ECX/EDX/EBP caller değerleri korunur; TF/DF reddedilir. Önceki yaşayan 156-byte caller stack, last-error ve named event kimliği her durakta eşleşir. Yeni kesit oyun register/IP/stack'ini sentetik olarak değiştirmez; CPU doğal CALL/MOV/CMP/branch yürütür. Önceki SystemParametersInfo bastırmasının sentetik bağlam işlemi ayrı kanıt olarak kalır; bu tümüyle değiştirilmemiş oyun başlangıcı iddiası değildir.

## Arayüzler ve kimlik

`ApplicationRoutingSpec` initializer FrameTargetSpec, detour/index/table RVA'ları, 39 index byte'ı ve 11 hedef RVA'sı taşır. `ApplicationRoutingCode` izinli opcode dizisini ve relocation operandlarını admitted image base'e göre üretir; child belleğine yazmaz. Entry 21-byte parent örneği, detour 12 byte, indirect JMP 7 byte, initializer CALL/prefix ve bütün iki tablo önkoşulda ve her durakta exact okunur. Yalnız selected index 5 ve event 24 kabul edilir. Unselected hedef gövdeleri incelenmiş veya çalıştırılabilir kabul edilmez. Aralık/taşma/çakışma kontrolü, CALL rel32 doğrulaması ve parent SHA zinciri strict JSON generator ile C++ doğrulayıcıya bağlıdır.

CLI kullanıcıdan RVA, opcode veya event numarası almaz. Policy, önceki event-dispatch digest'ine, oradan hash'i `f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac` olan executable ve pin'li DLL zincirine bağlıdır. C++ API test fixture'ları aynı sözleşmeyi farklı relocated image base ile kullanır; bu API indirilen resource'a açılmaz. Teknik tanım dosyaları [policy](../../contracts/engine/application-routing-policy.json), [header](../../include/saex/engine/application_routing.hpp), [portable doğrulayıcı](../../src/engine/loader/application_routing.cpp).

`applicationRoutingObservation` eski modlarda null'dır. Yeni modda policy/digest, sabit eventId=24, stage/stopAddress, selectedIndex/selectedTarget, continued ve reached/shape/frame/stack/verified alanları vardır. Seçilmiş hedef ancak üçüncü durakta EAX ve tablolar doğrulandıktan sonra raporlanır. `initializerCallAllowed=false` ve `rendererInitializationVerified=false` kalır. Üst modda önceki `eventDispatchObservation.applicationHandlerCallAllowed=true` olur; eski modda false ve 0x619B6C terminali korunur. Ara `verified=true` kayıtları önceki tamamlanmış kapıları gösterir, yeni kesitte hata varsa tüm süreç başarılı sayılmaz. Başarı `application_routing_boundary_verified`, exit 3; ret 1, kullanım 2. `canAttach=false`, `initializationVerified=false` değişmez.

## Hata davranışı ve genişleme

DR0 dört durağa taşınır. DR1 startup IAT yazımı, DR2 ekstra ASI ve DR3 existing-instance pencere kolu korunur. Yanlış parent/spec ön yürütmede; detour/table/initializer içeriği farkı handler CALL öncesi önkoşulda reddedilir. Devam sırasında shape, frame, stack, last-error, event kimliği, owner, debug event veya kota hatası child'ın sonlandırılmasıyla biter; çıkış ayrıca doğrulanır. Önceki API yan etkilerinden sonra eski stack/bağlama rollback yapılmaz. Child içindeki tüm kötü niyetli eşzamanlı native yazmaları önleyen bir sandbox iddiası yoktur; thread breakpoint'leri ve aşamalı readback bu kapsamı sağlamaz.

Başka event, farklı executable detour kodu veya initializer'ın çalıştırılması yeni inceleme/policy/kabul kanıtı gerektirir. İncelenen sonraki gövde 0x53BB50–0x53BB70 içinde beş CALL içerir: 0x72F3B0, 0x56D180, 0x5386F0, 0x406B70 (argüman 5), 0x541D90; AL=1 ve RET ile biter. Alt gövdelerin yan etkileri henüz bu kesitte doğrulanmadı. RsInitialize 0x619600 ayrıca sonraki çağrıdır. İsim veya ilk 16-byte eşitliği bu gövdeleri çalıştırma yetkisi değildir.

## Kabul ve kaynaklar

Portable testler range/overlap, index/target seçimi, relocation ve dört frame şeklinin bozulmasını sınar. Windows fixture gerçek doğal CALL/JA/JMP/MOVZX/indirect JMP zincirini kullanır; alt initializer canary'si pozitif run'da erişilmemiş olmalı, ayrı kontrol run'ında marker üretip 92 ile çıkmalıdır. Yanlış metadata/runtime tablo, hedef prefix'i, event kotası, owner, legacy terminal, mevcut named event ve 12 warm çevrim kapsanır. Fixture'a ayrı `Local\SAEX.ApplicationRoutingFixture.v1` nesne adı verilir. Fixture örneğinde MSVC'nin function-target JA'yı 6-byte near branch'e genişleten C4414 tanısı yalnız ilgili tanımda kapatılır; üretilen opcode ve rel32 ayrıca doğrulanır.

Adres ve baytların kaynağı yerel exact PE/disassembly'dir; statik çıktı `out/verification/engine/application-routing-detour-disassembly.txt`, `application-routing-initializer-disassembly.txt`, önceki `app-dispatch-disassembly.txt` içindedir. [AppEventHandler referansı](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/app/app.cpp), [dağıtıcı/RsInitialize](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/app/platform/platform.cpp) ve [event enum](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/game_sa/RenderWare/rw/skeleton.h) önceki `out/research/event-dispatch/sources.json` kaydıyla sabittir. Bu kaynaklar isim/kapsam açıklamasıdır; executable'a özgü detour yerel baytlardan çıkarılmıştır. Yeni dış dependency veya kaynak kopyalama yoktur.

C++ observer/trace tüketicileri yeniden derlenir. Bootstrap C ABI 1, GNS, otorite ve IPC sözleşmeleri değişmez; kaldırılan özellik veya kalıcı migration yoktur. Oyun initializer sonucu, renderer/window, doğal frame, production IPC/sandbox, AC-90/AC-91 bütünü ve N2/N3/D1/D2 kapıları açıktır.

## Nihai doğrulama

Canonical kayıt `out/verification/engine/routing-gta-evidence.json`, derleme/hash özeti `routing-final-verification.json`. Yeni private dizinler `out/experiments/gta-routing-f01a00ce-{Debug,Release}-v1`; önceki dizinler ve orijinal oyun kurulumu korunur. İlk keşif `routing-explore-Debug-1.json` final 12 koşuya katılmaz.

| Ölçüm | Sonuç |
|---|---|
| Gerçek GTA pozitif | Debug 6/6, Release 6/6; 12/12 |
| Doğal duraklar | 0x53EC10 entry → 0x4018CE detour → 0x53EC24 indirect JMP → 0x53EC2B initializer CALL öncesi |
| Seçim / ABI | Event 24 → index 5 → 0x53EC2B; C−32 stack ve dört register durumu eşleşti |
| Caller koruma | Yaşayan 156-byte stack, last-error ve named event kimliği korundu |
| Matris | 35/35 beklenen sonuç: 12 pozitif, 16 legacy mod, beş olumsuz koşul, iki event çakışması |
| Child ömrü | Oluşturulan 32/32 süreç kapandı; üç koşulda child oluşmadı |
| Dosyalar | 205/205 girdi hash'i korundu: önceki 190 + 14 private dosya + routing policy |
| x86 Debug / Release | Her profilde 31 native suite / 79 managed / 219 Python |
| x64 Debug / Release | Her profilde 14 native suite / 79 managed / 159 Python |
| Yeni testler | 167 portable spec/code/frame kontrolü; 11 x86 senaryo, initializer canary ve 12 warm çevrim |

Pozitiflerde 68–75 debug event, thread sayıları [4]; 128 event/16 thread/5 saniye kotaları korunur. Host foreground timeout yalnız SPI_GET ile 2147483647 → 2147483647 ölçüldü. Bu eşitlik tek başına sistem ayarı çağrısının çalışmadığı kanıtı değildir; önceki bastırma/bağlam/canary kanıtıyla birlikte değerlendirilir.

244 kaynak/yapılandırma girdisi `routing-source-final-inputs.json` ile ve iki profilin probe/DLL/map artifact hash'leri final kontrolde aynı kaldı. İlk snapshot'tan sonra yalnız tools/ci.py portable CI wiring eklendi; bunun ardından 8 tooling CI testi ayrıca geçti (`routing-ci-final.log`). C++ kaynakları ilk snapshot ile de aynıdır. Linux/hosted CI/N1 SDK yeniden koşulmadı.

İlk fixture derlemelerinde MSVC cross-function JA C4414, yanlış naked forward declaration/near ptr yazımı ve DWORD/std::uint32_t array tür uyuşmazlığı görüldü. Fixture deklarasyonu, local C4414 açıklaması ve sabit genişlikli okuma düzeltildi; ilk üç başarısız log silinmedi (`routing-build-initial.log`, `routing-build-second.log`, `routing-build-third.log`). Dördüncü hedefli build, ardından iki hedefli test ve dört standart build geçti. İlk warm sayımı 138 → 138, targeted kayıt `routing-tests-initial.log`; final suite'lerde warm eşitliği tekrar geçti.

| Kimlik | SHA-256 |
|---|---|
| Application routing policy | `d2794efb7784cb53b20d693533f5ab67c6f05a8fbd4c2d77264942910c408f66` |
| Debug probe | `e577cfd48f2710cf205d32cd4848d3777fa93a32bd3a116104923ee3ad50d7af` |
| Debug dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release probe | `dc042f0086160e974bdea841126a9f2bf5b9d3a8ecd5e54edd149808fd194186` |
| Release dll | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Handler'ın yalnız belirtilen rota komutları çalıştırıldı; ilk oyun initializer CALL veya başarılı initialize dönüşü bu kanıtın parçası değildir. Production SDK/IPC/sandbox, renderer ve doğal frame kapsamları açık kalır.

## Kod 0.1.27 bağlantısı

`--observe-game-prelude` ilk oyun initializer içine girer; exact boş Init ve üç yerelleştirme bayrağını yazan iki helper doğal olarak geri döner. Beş durak, stack/register/flags, yaşayan caller ve 16-byte veri penceresi denetlenir; yalnız üç veri byte değişebilir. CFileMgr CALL çalıştırılmaz. [Sözleşme ve sonuç](d1-game-prelude.md). Önceki application-routing terminali korunur; yeni üst modda routing nesnesinin initializerCallAllowed alanı true, prelude nesnesinin fileManagerCallAllowed ve initializerReturnVerified alanları false olur. C++ observer yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni dependency/kaldırma/kalıcı migration yoktur. CFileMgr, streaming/pad, initializer dönüşü, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## 14 Eylül 2026 — Windows fixture taşınabilirliği

Startup-return ve devamındaki fixture zinciri, sistem DLL reçetelerini test makinesinin diskte tutulan PE dosyalarından çıkarır. Ortak okuyucu ve negatif doğrulamalar [test kapsamı notunda](d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği) açıklanır; üretim policy/hash kuralları ve bu belgedeki gerçek GTA kanıtının sınırları aynıdır.

## 0.1.33 — Windows profil bağı

[26200.9445 incelemesi](d1-system-profile-9445.md), bu bileşenin bağlı olduğu OS pinleri/API reçeteleri ve parent SHA-256 zincirini günceller. Bileşenin GTA durak/izin sınırı aynı kalır; önceki runtime sonuçları eski profil bağlamındadır. Yeni dosyalarla fixture ve gerçek GTA kanıtının kapsamı profil raporunda ayrıca izlenir. Eski OS pinlerine otomatik fallback yoktur; kaynak ve generated policy birlikte yeniden derlenir.
