# D1-N2 — CRT başlatıcı sınırı

Kod 0.1.20 / mimari v0.27; 13 Eylül 2026. CRT başlatıcı çağrısı önündeki sınır özel GTA kopyasında doğrulandı. [Durum](status.md) · [Önceki doğal startup dönüşü](d1-startup-return.md).

## Sorumluluk ve veri akışı

Ayrı `--observe-crt-startup <exe> <absolute-cwd>` deneyi doğal startup dönüşünden I/O hazırlığı dönüşüne ve ardından statik başlatıcı çağrısının önüne ilerler. Observer yalnız debug register'ları değiştirir; startup-return deneyinin izinleri korunur. Bootstrap export'u, başlatıcılar ve uygulama girişinin gövdesi bu ek izinle çağrılmaz. Her çıkış owned child sonlandırmasıyla biter; production SDK ve güvenlik sandbox'ı değildir.

Yerel exact EXE disassembly'si: ilk GetStartupInfoA dönüşü VA 0x82C11C, 0x824664 CALL → 0x82C0B9 I/O hazırlığının içindedir. Bu hazırlığın doğal dönüşü 0x824669'dur. Sonraki komutlar komut satırı/ortam/argüman hazırlığına ilerler; 0x8246AC CALL → 0x823B76 statik başlatıcı döngülerine girmeden durulur. Ardından, bu modun yürütmediği 0x8246EC CALL → 0x748710 uygulama giriş adayı vardır. Bu isimler yerel çağrı şeklinin yorumudur; adresler SDK etiketi tahmin edilerek alınmadı. Üç rel32 CALL ve üç hedef öneki aynı EXE profilinde bağlıdır.

## Arayüz ve kontroller

[Makine reçetesi](../../contracts/engine/crt-startup-policy.json) startup-return parent hash'ine bağlıdır. `CrtStartupSpec` observer içi C++ sözleşmesidir; C ABI 1 değişmez. Aynı base/size, ayrık kod/tablo aralıkları, hizalama, stack offset ve toplam en çok 2048 slot doğrulanır. Diskteki pointer'lar sıfır veya EXE RVA olarak saklanır; runtime karşılaştırması gerçek image base eklenerek yapılır. Farklı image/harici pointer kabul edilmez.

Doğal startup dönüşünde çağrı yerleri/önekleri ve başlatıcı tabloları okunur. I/O hazırlığının incelenen yığın düzenindeki dönüş adresi ve dört saklanmış register değeri alınır. Bir sonraki durakta sıfır EAX, ESP'nin dönüş slotu+4 olması, EBX/EBP/ESI/EDI ve DF/TF doğrulanır. Son durakta üç aday çağrı, önekler ve tablo slotları tekrar karşılaştırılır. Nonzero hedeflerin MEM_IMAGE/committed/execute erişimi denetlenir; bu, hedef gövdelerinin güvenli veya çağrılmış olduğunu kanıtlamaz. DR1 main-thread IAT yazımını, DR2 aynı ASI callsite'ını izlemeyi sürdürür. Başka thread'lerin yazılarına karşı güvenlik sınırı iddia edilmez.

Başarı `crt_initializer_boundary_verified`, exit 3; `canAttach=false`, `initializationVerified=false`. Hata exit 1, CLI kullanım hatası 2. Eski modlarda `crtStartupObservation=null`; yeni modda continuation, I/O dönüşü, stack/register, incelenen/nonzero slot sayıları, ilk uyuşmazlığın RVA/hedefi ve terminal sınır ayrı kaydedilir. Yığın/ortam/komut satırı içeriği yayımlanmaz.

## Statik başlatıcı envanteri ve motor önkoşulları

Yerel 0x823B76 gövdesi önce 0x8E2B74 pointer'ını, ardından [0x8A5A14,0x8A5A30) ve [0x8A4000,0x8A5A10) aralıklarını kullanır. Sırasıyla 1/7/1668 slot ve 1/6/1667 nonzero kayıt vardır. Tekrarlanan hedefler slot sırasıyla korunur; sıfır slotlar atılarak kapsam daraltılmaz. Microsoft'un [CRT initialization açıklaması](https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-initialization?view=msvc-170) global başlatıcıların fonksiyon-pointer dizisinden çalıştırılmasını açıklar; bu belge GTA adresleri veya sürümü için kanıt değildir.

Uygulama girişinin statik aday öneki henüz renderer/world hazır durumu değildir. Erken uygulama kodunda sistem, pencere ve platform hazırlığı çağrıları görülür. Sonraki aşamada başlatıcıların callback/dinamik DLL etkileri, doğal uygulama girişi, dosya/pencere/renderer izinleri ve asset erişimi ayrıca doğrulanmalıdır. Yedi dosyalı mevcut özel deney kopyası tam oyun veri seti değildir; eksik veriyle çalışan motor veya frame kanıtı üretilemez. Asset/collision/world kapıları açılmaz.

## Hata davranışı, genişleme ve kabul

Bozuk spec/parent, rel32/önek/table drift, harici veya okunamayan hedef, hatalı yığın, sıfır dışı I/O sonucu, ABI bozulması, exception, süre/event bütçesi ve ikinci ASI çağrısı terminal rettir. Başlatıcı çağrısı önündeki breakpoint veya eski mod sınırı kaçırılırsa canary negatifleri testi düşürür. Portable metadata negatifleri, owned Windows normal/fault/stall/I/O failure/table/frame drift ve sıcak çevrimler geçti; gerçek GTA deneyi aşağıda ayrı kayıtlanır. Bir alt senaryo N2/AC-90 bütünü veya N3/AC-91'i kapatmaz.

Kaldırılan özellik veya kalıcı migration yoktur. C++ observer yeniden derlenir; GNS/HTTPS, otorite ve C ABI 1 aynı kalır.

## Doğrulama kaydı

Tam matris `out/verification/engine/crt-gta-evidence.json`; tekil `crt-Debug-{1..6}.json`, `crt-Release-{1..6}.json` ve regresyon dosyaları aynı dizindedir. İlk exploratory `crt-Debug-first.json` bu tekrarlara dahil değildir. Eski özel kopyalar/kanıtlar korunur; yeni yedi dosyalı dizinler `out/experiments/gta-crt-f01a00ce-{Debug,Release}-v1` içindedir.

| Kanıt | Sonuç |
|---|---|
| Doğal I/O dönüşü ve initializer sınırı | Debug 6/6 + Release 6/6; EAX=0, ESP/register uygun |
| Terminal konum | Her koşuda VA 0x8246AC, CALL yürütülmeden durdu |
| Tablo karşılaştırması | Her koşuda 3/3 tam örnek; örnek başına 1676 slot, 1674 nonzero hedef |
| Toplam matris | 27/27 beklenen sonuç; oluşturulan 24/24 child'ın çıkışı doğrulandı |
| Girdi korunumu | 115/115 hash aynı: önceki 100 + yeni özel dizinlerin 14 dosyası + CRT policy |
| Gözlem bütçesi | Pozitif koşularda 52–57 event, dört lifetime thread; mevcut 5 saniye/128 event/16 thread kapıları içinde |

On eski mod aynı terminal sınırlarını korudu ve `crtStartupObservation=null` üretti. Eksik ASI, yanlış cwd ve Debug/Release artifact uyuşmazlığı child öncesi reddedildi. ASI devre dışıyken `asi_candidate_missing`; orijinal kurulumdaki eski codec-bindings komutunda pin dışı modül `loader_module_not_pinned` beklenen negatif sonuçlardır. Observer yığın/kod yazmadı; seçilmiş initializer gövdeleri ve uygulama girişi yürütülmedi. Orijinal GTA dosyaları değişmedi.

### Derleme ve testler

| Yerel Windows profili | Native suite | Managed test | Python test | Log (`out/verification/engine/`) |
|---|---:|---:|---:|---|
| x86 Debug | 19 | 79 | 170 | `build-crt-x86-Debug-first.log` |
| x86 Release | 19 | 79 | 170 | `build-crt-x86-Release-complete.log` |
| x64 Debug | 8 | 79 | 122 | `build-crt-x64-Debug-complete.log` |
| x64 Release | 8 | 79 | 122 | `build-crt-x64-Release-complete.log` |

Portable `native.crt_startup_spec`: 15 kontrol. x86 `native.crt_startup`: 16 senaryo + 12 warm çevrim; son Debug/Release koşularında handle 141 → 141. Yedi yeni Python policy testi ve iki yeni CLI testi standart akışlara bağlıdır. Native ayrıntılar `ctest-crt-x86-{Debug,Release}.log`; dört akış ve artifact/girdi karşılaştırması `crt-final-verification.json` içinde izlenir. Linux, hosted CI ve N1 SDK bu kesitte yeniden çalıştırılmadı.

İlk generator denemesinde f-string kapanış brace'i eksikti; syntax hatası üretim başlamadan yakalandı ve düzeltildi. İlk doküman kapısı owner belgeleri henüz güncellenmediği için reddetti; stderr/stdout birleşik kaydı `docs-crt-owner-gaps.log` içinde korundu. Owner belgeleri ve eşleme tamamlandıktan sonra standart derlemelere geçildi; tablodaki dört akış başarıyla tamamlandı. Hata geçmek için uyarı, adres veya profil denetimi gevşetilmedi.

### Exact kimlikler ve kaynak incelemesi

| Artifact | SHA-256 |
|---|---|
| CRT policy | `86d01866ee719968fb7f4349fd2bc51ea7a749f88a1db7dd6794502d9ec1f67b` |
| Debug observer EXE | `e2f0841ba8c7f87d238877dc549ac2cfe03ded2ccba12638f60e67bb76b6ba56` |
| Release observer EXE | `edea47e768eed8ca6029a82d22c3b0088be182f067b4a8e18c27b9cc047b2211` |

SAEX DLL/map kimlikleri 0.1.19 ile aynı kaldı; tam hash'ler canonical matristedir. Kaynak EXE `f01a00ce...` gözlem profiline bağlıdır. Yerel dumpbin kayıtları: `crt-startup-disassembly.txt`, `crt-entry-disassembly.txt`, `crt-initializers-disassembly.txt`, `application-entry-disassembly.txt`. Bu kayıtlar diskteki byte incelemesidir; runtime eşleşmesi ayrıca yukarıdaki matrisle doğrulandı. Policy üretimi bütün nonzero hedeflerin diskte executable section içinde olmasını; runtime okuma ise aktif committed MEM_IMAGE/execute erişimini ister.

Sonraki izin incelemesinde 0x823B76 başlatıcı gövdesi ve callback etkileri ele alınmalıdır. Ardından 0x8246C6'da ikinci GetStartupInfoA çağrısı vardır; aynı IAT yoluna yeniden girişin etkisi araştırılmadan mevcut ASI koruması kaldırılmaz. Uygulama girişi, pencere/renderer, asset hazırlığı ve doğal frame ayrı kapılardır. Bu alt çıktı motorun tam açılışı veya oynanabilir multiplayer değildir.

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.
