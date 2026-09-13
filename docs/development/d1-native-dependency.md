# D1-N1: sabit native bağımlılık ve x86 derleme kanıtı

Tarih: 12 Eylül 2026. Mimari v0.8 / kod 0.1.1, ADR-36. Bu kesit **SDK'nin seçilmiş build alt kümesidir**; GTA bootstrap, hook, Host IPC veya native controller uygulaması değildir. [Durum](status.md) · [Uygulama sırası](d1-engine-integration.md) · [Normatif sözleşme](../architecture/native-sdk-integration.md)

## Sorumluluk ve kaynak haritası

| Kaynak | Uygulanan sorumluluk |
|---|---|
| [Lock](../../contracts/engine/plugin-sdk.lock.json) | Exact upstream commit, arşiv/source/patch/recipe digest'leri, toolchain, notice ve kapsam |
| [Dependency aracı](../../tools/native/dependency.py) | Açık edinme, salt okunur doğrulama, ayrı orijinal/uyarlanmış cache, uyuşmazlıkta ret |
| [Özel CMake hedefi](../../tools/native/CMakeLists.txt) | Windows x86 ve kilitli toolchain; kaynakların configure ve build öncesinde doğrulanması |
| [Build girişi](../../tools/native/build-sdk.ps1) | Belge kapısı, isteğe bağlı edinme, temiz library rebuild, test ve artifact/PDB kanıt kaydı |
| [SDK probe](../../src/engine/plugin_sdk/sdk_probe.cpp) | Private vendor header, x86/CPool boyut-offset kontrolü ve gerçek SDK library symbol link'i |
| [Bağımlılık testleri](../../tests/engine/test_dependency.py), [CMake retleri](../../tests/engine/test_sdk_configure.py) | Bozuk kaynak/patch/recipe/archive, ek header, path ve yanlış mimari/eksik kaynak retleri |

## Veri ve derleme akışı

`exact commit archive → archive SHA-256 → seçilmiş dosya envanteri → orijinal cache → sıralı exact patch → ayrı build cache → recipe/toolchain doğrulaması → private .lib → x86 probe → test → artifact kaydı`.

Kaynak: Dryxio/plugin-sdk-sa `b55e89b336a81448c1aa1a5b188431c9845ebaa9`. İndirilen arşiv **12.753.675 byte**, SHA-256 `3ff497b91744c189c38003e619ce5730b8998a634b4bef4c0d47ede996c758e9`. Arşivin tamamı build'e açılmaz: **23 kaynak/header/notice dosyası, toplam 788.235 byte** seçilir. Yalnız `shared/PluginBase.cpp` vendor translation unit olarak derlenir. `CPool.h` ve PluginBase'in header bağımlılıkları envanterdedir; diğer oyunlar, örnek ASI/installer, GameVersion.cpp, native adres kullanan kaynaklar, RenderWare/DXSDK ve SafetyHook implementation library'si dahil değildir.

Orijinal kaynak envanteri digest'i `68ae0550170492e70374d79f9a6b9737823d0fb3f164ed1672ae5bebd5358bee`; iki patch sonrası build envanteri `a393bada756ff618e2a9d7f98fe2b9e6cdea1ea5ba4f23efd9c130a312c4ced1`. Canonical envanter girdisi case-sensitive path sırasıyla UTF-8 `sha256 bytes path\n` satırlarıdır. Dosya hash'i ham byte üzerinden alınır; CRLF sessiz normalize edilmez. `DependencyLockDigest` lock dosyasının ham SHA-256'sıdır ve her build raporuna konur.

Normal `tools/build.ps1` SDK indirmez/linklemez. Açık native giriş `-Acquire` alırsa kilitli URL'den kaynak edinir; aksi halde hazır ve doğrulanabilir cache gerekir. Bozuk mevcut cache/arşiv otomatik silinmez veya üzerine yazılmaz. Yeni sürüm için yeni lock/cache kimliği gerekir; eski kaynak korunur. Orijinal ve patch'li dizinler ayrı doğrulanır. `out/research` build girdisi değildir.

## Derleyici bulgusu ve dar uyarlamalar

İlk **C++20 deneyi başarısız** oldu: `PluginBase.h → Pattern.h → injector/assembly.hpp → safetyhook.hpp → <expected>` zinciri `std::expected` gerektiriyor; MSVC `STL4038/C2039` verdi. Bu sonuç saklandı: `out/dependencies/sdk-cxx20-build.log`. C++23 ile ikinci deneme injector üye gölgeleme uyarılarını `/WX` altında yakaladı; `out/dependencies/sdk-cxx23-build.log` bunu kaydeder.

ADR-36 yalnız bu bağımsız private SDK hedefinde **C++23 gereksinimini** kabul eder. CMake 4.3.3 + sabit MSVC 19.44.35228.0 kombinasyonu `/std:c++latest` üretir; gelecekteki derleyiciye kayan bir izin değildir. C++20 core, x64 Host/server ve .NET 10 seçimi değişmedi. Probe henüz public native ABI export etmez; ileride bu dil farkını aşan sınır SAEX'e ait sabit alanlı arayüz olur, STL/exception/allocator taşınmaz.

| Uyarlama | Gerekçe ve kapsam |
|---|---|
| [0001 injector üye adları](../../tools/native/patches/0001-injector-member-names.json) | `scoped_basic` içindeki üç private üye ve onlara erişimler yeniden adlandırıldı; C4458 gölgelemesi giderildi. Tip/sıra/çağrı değişmedi. |
| [0002 native union](../../tools/native/patches/0002-pool-native-union.json) | GTA'nın anonim bit-field düzeni korunur. C4201 yalnız `tPoolObjectFlags` tanımı çevresindeki push/pop ile izinlidir; global uyarı kapatma yoktur. |

Patch formatı `schemaVersion=1, target, beforeSha256, afterSha256, replacements[]`; her eski byte dizisi tam bir kez bulunmalıdır. SHA-256 ön/son koşulu sağlanmazsa uygulanmaz. Uyarlanmış kaynakta SAEX işareti ve upstream notice korunur. Bu patch'lerin hook davranışını doğruladığı söylenmez; patch'li hook fonksiyonları probe tarafından çağrılmaz.

Derleme: **Win32, MSVC 19.44.35228.0, Windows SDK 10.0.26100.0, VS 17 2022 generator, CMake 4.3.3, `/W4 /WX /permissive- /Zp8 /Gd /EHsc /utf-8`, Debug MDd / Release MD**, `GTASA/PLUGIN_SGV_10US/RW`. Son define gerçek executable profili onayı değildir. Vendor'ın kendi mevcut pragma'ları envanterde korunur; tüm upstream kodun bütün uyarılardan arındırıldığı iddia edilmez.

## Sınırlar, hata davranışı ve genişleme

İndirme 32 MiB, tek envanter dosyası 4 MiB, envanter 512 dosya ve zip 16.384 girişle sınırlıdır. Arşiv özeti doğrulanmadan extraction olmaz. Yalnız envanterdeki exact isimler okunur; yol kaçışı/ADS/device adları, case çakışması, symlink/junction, duplicate JSON key ve fazladan header reddedilir. Kaynak staging ayrı dizine hazırlanır; mevcut cache değiştirilmez. Doğrulama, düşmanca yerel yöneticiye karşı imza veya sandbox sınırı değildir; edinim ile derleme arasında yerel diski eşzamanlı değiştiren saldırgana karşı atomik filesystem snapshot sağlanmaz.

Header ve library aynı clean build'den gelir; keyfî `.lib` yolu girişi yoktur. Vendor include/define/feature yüzeyi `PRIVATE` ve CMake kontrolündedir; x64 configure açık `SAEX_SDK_X86_ONLY` hatası verir. Recipe veya kaynak değişirse configure ve custom build kontrolü durur. Artifact kaydı yalnız testler ve build sonrası tekrar doğrulama geçerse yazılır; `gtaEligible=false` kalır. Kayıt `out/verification/engine/sdk-build-Debug.json` veya `sdk-build-Release.json`; exact lock, ayarlar, EXE/.lib/PDB hash ve boyutlarını taşır. Bunlar en son **başarılı** yerel build kayıtlarıdır; sonraki hatalı denemeyi başarıya dönüştürmezler.

Yeni native sınıf/header/translation unit eklendiğinde envanter, patch/recipe ve notice kapsamı bilinçli güncellenir. Kapsam dışı RenderWare/DXSDK veya SafetyHook implementation'ı eski N1 sonucuyla kabul edilmez. Otomatik lock yenileme/bless komutu yoktur. Aynı commit üzerinde source genişletmek de yeni kanıt ister.

## Notice envanteri

| Dosya | Bu kesitteki kapsam |
|---|---|
| Upstream `LICENSE` | Plugin-SDK shared/SA dosyaları; her kaynak başlığı korunur |
| `injector/injector.hpp` ve injector dosyalarının başlıkları | LINK/2012'nin inline Zlib bildirimleri |
| `hooking/LICENSE.md` | `Hooking.Patterns.h` için MIT bildirimi |
| `safetyhook/LICENSE.txt` | Dahil edilen SafetyHook header'ı için Boost 1.0 bildirimi |

Bu dosyalar kaynak cache'inin hash envanterinde bulunur; [kaynak incelemesindeki sabit bağlantılar](../references/plugin-sdk-sa.md) ile izlenir. Bu, bütün SDK ağacının veya GTA varlıklarının yeniden dağıtım değerlendirmesi değildir. Henüz yayımlanmış SAEX paketi yoktur.

## Kabul ve kanıt kapsamı

Son lock SHA-256: `eec9cbc16685563a08c4a121ddb885fcfe554dff99b055a4d516d810fe5d7670`. **Windows x86 SDK Debug ve Release** opt-in akışı başarıyla tamamlandı; her koşuda 1/1 probe, 17/17 dependency ve 2/2 gerçek CMake ret testi geçti. Ayrı foundation regresyonu **Windows x64 Debug ve x86 Debug** için 16/16 native, 25/25 managed/entegrasyon ve 5/5 tooling sonucu verdi. Toplam **66 farklı test tanımı** vardır; farklı konfigürasyon koşuları ayrıca eklenmez.

| Artifact | Son doğrulanmış SHA-256 |
|---|---|
| Debug saex_sdk_probe.exe | `98374b77f09547e33f0af7339bc81f658cb2a74020137745c75b37996d5930e4` |
| Release saex_sdk_probe.exe | `6f6e49a173d64baf9817211d4d29f85e4fde3b096c600655adba36acd2d1f51a` |

EXE/lib/PDB dosyalarının tümü build kayıtlarındaki hash ve boyutlarla tekrar karşılaştırıldı. Kilitli arşiv kullanılarak izole boş bir dizinde **cold acquire** ayrıca çalıştırıldı: orijinal ve patch'li cache bağımsız oluşturuldu ve tekrar doğrulandı. Bu ek elle yürütülen entegrasyon kontrolü yeni otomatik test tanımı diye sayılmadı. Byte-identical PE/PDB yeniden üretimi kanıtlanmış değildir; bunlar kullanılan artifact'in kimlikleridir. Yerel GTA inspector tekrarında aynı dosya hash'i, `recognizedProfile=false`, `canAttach=false` ve beklenen CLI exit 3 görüldü.

N1 testi SDK'nin gerçek `.lib` içindeki `plugin::Core::GetVersion()` fonksiyonunu linkleyip yalnız sabit `0x10` sonucunu çağırır. `sizeof(void*)=4`, `sizeof(CPool<int>)=0x14`, size/ownership offset'leri compile-time doğrulanır. **GTA sürüm algılaması, pattern taraması, event kaydı ve hook yazımı yapılmaz.** CPool yerel nesne ömrü veya oyun pool'u bu testte çalıştırılmaz.

17 bağımlılık testi; bozuk/eksik/ek kaynak, mevcut cache'i koruma, Windows tehlikeli yolları, case/JSON çakışması, patch ön/son koşulu, tekrarlı patch context, değişmiş recipe, seçili archive extraction, bozuk/yanlış içerikli archive, archive symlink, source lock sapması ve gerçek cache doğrulamasını kapsar. İki ayrı **gerçek CMake** testi x64 hedefini ve izole kopyada eksik kaynakları reddeder. Bir SDK compile/link testiyle **20 yeni test tanımı** vardır; Debug/Release tekrarları yeni test sayısı değildir.

Bu alt sonuç D1-N1 / R-02a'nın seçilmiş build kapsamını ve AC-89'un buna ait bölümünü destekler. **D1-N2–N7 ve AC-90–96 çalıştırılmadı.** EngineInspector hâlâ `canAttach=false`; GTA dosyası değiştirilmedi/başlatılmadı. Gerçek profile, bootstrap, hook drain, IPC, entity/asset ve OS/GNS kapıları açık kalır.
