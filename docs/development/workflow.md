# Geliştirme, derleme ve doküman güncelleme akışı

Durum: D1'de kullanılabilir yerel iş akışı, ADR-33. [Uygulama durumu](status.md) · [Kök kurallar](../../AGENTS.md) · [D1 sözleşmesi](d1-foundation.md)

## Sorumluluk ve araçlar

Kod C++20 çekirdek ve C#/.NET 10 araç/ortak tipler olarak ayrılır. Başlangıçta harici runtime paket bağımlılığı yoktur; .NET BCL ve C++ standard library kullanılır. NuGet.Config remote kaynakları boş tutar. Yeni bağımlılık gerektiren işte kesin sürüm, gerekçe, lisans/kaynak, güvenlik ve lock politikası belgelenerek bu ayar değiştirilir; sessiz otomatik indirme eklenmez.

V0.7/ADR-35, Plugin-SDK-SA için [private x86 bağımlılık sözleşmesi](../architecture/native-sdk-integration.md) ve [D1-N1 build kesitini](d1-engine-integration.md) ekler. V0.8'de [D1-N1](d1-native-dependency.md) için ayrı opt-in build girişi eklendi. Native C++ bağımlılığı NuGet kaynaklarını açmayı gerektirmez. Kaynak/patch/recipe/notice lock'u artık uygulanır; normal core build'i SDK'den bağımsızdır. C++20 vendor denemesi std::expected nedeniyle başarısız oldu; yalnız private probe MSVC19.44/C++23 ve dinamik CRT ile derlenir. Ayrıntı ve dar uyarlamalar ADR-36/N1 raporundadır.

Windows doğrulama ortamı: CMake 4.3.3, Visual Studio 2022 Build Tools/MSVC 19.44, Windows SDK 10.0.26100.0, .NET SDK 10.0.300/runtime 10.0.8. global.json 10.0.300 feature band'inde patch güncellemesine izin verir. CMake minimum 3.25; Windows preset VS 2022, Linux preset Ninja kullanır. Visual Studio IDE'nin mevcut olması C++ workload'unun kurulu olduğu anlamına gelmez; kurulum keşfinde VC.Tools.x86.x64 bileşeni aranır.

## Standart derleme

Repository kökünde Python 3, CMake, .NET SDK ve C++ workload erişilebilirken:

```powershell
./tools/build.ps1 -Architecture x64 -Configuration Debug
./tools/build.ps1 -Architecture x86 -Configuration Debug
```

Python PATH'te değilse `-Python` parametresine gerçek python.exe yolu verilir. Script: belge eşleme/link/JSON kontrolü → contract generator ve engine profile generator --check → native configure/build/CTest → managed test ve native pipe fixture → tooling testleri → başarılı yerel hash baseline kaydı sırasını izler. Bir komut hata verirse durur. Release için aynı script `-Configuration Release` alır. Build çıktıları `out/`, .NET `bin/obj` altında ve Git dışındadır.

Bu makinedeki PATH Python kaydı WindowsApps alias'ıdır. Doğrulamada mevcut yerel runtime açık seçildi; makineye yeni Python kurulmadı. Kendi Python kurulum yolunuza uyarlayacağınız örnek komut:

```powershell
./tools/build.ps1 -Architecture x64 -Configuration Debug -Python "C:\Python312\python.exe"
```

Başka makinede aynı kullanıcıya özel yol varsayılmaz; yerel Python 3 yolu seçilir. Windows x64 Debug/Release ve x86 Debug bu kesitte test edilmiştir.

Şema bilinçli değiştiğinde önce generator çalışır; üretilmiş dosyalar elle düzenlenmez:

```powershell
dotnet run --project tools/ContractGen -- .
dotnet run --project tools/ContractGen -- . --check
dotnet run --project managed/Saex.Tools -- plan validate samples/foundation/world-plan.json
dotnet run --project managed/Saex.Tools -- engine inspect "C:\Program Files (x86)\Rockstar Games\GTA San Andreas\gta_sa.exe"
```

Plan CLI: 0 metadata-valid, 1 validation/input ret, 2 kullanım hatası. Engine inspect: 3 dosya incelendi ama destek/attach doğrulanmadı; 1 bozuk/girdi hatası, 2 kullanım hatası. Mevcut inspector hiçbir profile attach izni vermez. Dotnet run çıktısında build metni de olabilir; otomasyon için derlenmiş Saex.Tools.dll doğrudan `dotnet` ile çalıştırılır. Bu komutlar oyun başlatmaz.

Linux için `cmake --preset linux-x64`, build/test preset'leri ve managed test runner'a `out/linux-x64/saex_contract_probe` yolu kullanılır. Linux'ta .NET 10.0.300 feature band, Ninja ve C++20 derleyici gerekir. Portable kapsam GitHub Ubuntu 24.04 üzerinde doğrulandı: `python tools/ci.py linux`; exact commit ve sonuçlar [yayın raporundadır](github-publication.md). Windows/GTA adapter bu Linux işinde sınanmaz.

## İsteğe bağlı D1-N1 SDK derlemesi

Native kaynaklar ilk kez edinilecekse açık `-Acquire` kullanılır. Sonraki koşu doğrulanmış cache ile ağsız ilerler:

```powershell
./tools/native/build-sdk.ps1 -Acquire -Configuration Debug -Python "C:\Python312\python.exe"
./tools/native/build-sdk.ps1 -Configuration Release -Python "C:\Python312\python.exe"
```

Bu hedef yalnız Windows x86, CMake 4.3.3 / VS 17 2022 / MSVC 19.44.35228.0 / Windows SDK 10.0.26100.0 kabul eder. SDK'ye özel C++23, MDd/MD, /Zp8 ve /Gd seçimi [ADR-36 ve N1 raporunda](d1-native-dependency.md) gerekçelidir. Core CMake hedefleri SDK'ye bağlanmaz. Toolchain değişimi için lock/kanıt güncellenir; hatayı geçmek için keyfî flag verilmez.

Akış: belge gate → isteğe bağlı acquire → source/patch/recipe verify → standalone configure → temiz library/probe build → CTest → 17 dependency ve 2 configure ret testi → tekrar verify → EXE/lib/PDB hash kaydı. `out/verification/engine/sdk-build-<Configuration>.json` en son başarılı koşuyu saklar. Opt-in script, ortak foundation testlerini çalıştırmış sayılmaz; kaynak değişiminde standart `tools/build.ps1` ayrıca yürütülür ve ortak baseline'ı ancak o başarılı koşu kaydeder.

Salt okunur manuel doğrulama `python tools/native/dependency.py verify`, açık edinme `python tools/native/dependency.py acquire` şeklindedir. Mevcut bozuk cache üzerine yazılmaz. CMake doğrudan çağrılsa da configure ve build öncesi verify zorunludur; production artifact kanıtı için script'in temiz build/test/record yolu gerekir. Oyun başlatma, kurulum dizinine dosya kopyalama veya SDK installer çalıştırma adımı yoktur.

## Belge ve kod birlikte değişir — kontrol adımları

1. İlgili normatif belgeyi ve status tablosunu oku; AC/R kapsamını belirle.
2. Davranışı ekle/değiştir/kaldır; pozitif ve gerekli negatif testleri güncelle.
3. [Component map](component-map.json) içindeki **bütün** sahip belgeleri, status ve change-log'u aynı değişiklikte güncelle. Yeni kaynak ailesi eşlemesiz bırakılamaz.
4. Yeni invariant/ABI/otorite yönü varsa ADR ekle; değişmez kararları sessizce yeniden yorumlama.
5. Standart build'i çalıştır; geçen platform/kapsamı ve kalan riski kaydet. Başarısız test completed/verified diye sunulmaz.

`tools/check_docs.py --base <git-ref>` Git farkındaki ekleme/değiştirme/silmeleri ve untracked dosyaları denetler; referans geçerli olmalıdır. Yerel başlangıçta Git commit'i gerektirmeden son başarılı build'in `out/verification/docs-baseline.json` hash'leri kullanılır. İlk koşu tüm kaynakları yeni kabul eder; sonraki koşu hash değişimi ve silinen yolları görür. `--record` yalnız başarılı build sonunda kullanılır. Baseline temizlenmesi denetimi ilk çalışma durumuna döndürür; güvenlik sınırı veya imzalı kanıt değildir. Paylaşılan incelemede Git referansı tercih edilir.

Bu kontrol sahip belgenin değiştiğini saptar; değişen cümlenin doğru/yeterli olduğunu insan incelemesi ve gerçek test kanıtı belirler. Bir tarihi değiştirerek anlamsal güncelleme şartı sağlanmış olmaz. Kaldırma için API kullanıcı etkisi, saklanan state ve migration yolu ayrıca yazılır.

V0.7 component map, gelecekteki `src/engine/`, `include/saex/engine/`, `contracts/engine/`, `tools/native/` ve `tests/engine/` ailelerini motor/SDK sözleşmesi, D1 planı ve ilgili kabul belgelerine bağlar. N1'de bu ailelerin dependency/probe/test alt kümesi oluştu; N2'de SDK bağımsız preflight parser/reader/CLI ve kod 0.1.3'te yüklenebilir x86 başlangıç DLL'si eklendi; gerçek GTA loader ve gameplay adapter hâlâ planlanmıştır. EngineInspector değişimleri de yeni SDK sözleşmesi ve D1 planının güncellenmesini gerektirir. Genel `contracts/*` ve `tests/*` sahipliği korunur; eşleşen bütün kurallar birlikte uygulanır. Bilinmeyen başka yeni kaynak yolu eşlemesiz bırakılırsa gate açık hata verir.

Doküman değişiminde `tools/check_docs.py` link/JSON/eşleme kontrolü kullanılır; component map değiştiğinde mevcut tooling negatif testleri ve standart build yolu da doğrulanır. Yerel Python PATH'te bulunmuyorsa `-Python` ile gerçek runtime yolu verilir. İnceleme cache'i `out/research/` test veya dağıtım girdisi sayılmaz. SDK'nin kendisini compile etmeden mevcut foundation build'inin geçmesi SDK entegrasyonu başarılı diye raporlanmaz.

## Doğrulama ve arıza çıktıları

`out/verification/docs-check.json` son statik kontrolü, CTest `out/<platform>/Testing/Temporary/LastTest.log` native koşuyu kaydeder. Native foundation runner 16 isimli test, managed runner 26 isimli test raporlar. CTest foundation'a ek PE/profile preflight ve portable bootstrap session suite'lerini içerir; x86'da gerçek bootstrap DLL suite'i de çalışır. X64'te üç, x86'da dört suite vardır. Python tarafında 5 belge, 8 engine profile ve 4 native CLI testi çalışır. Raporlarda suite sayısı ile senaryo sayısı karıştırılmaz. Tooling testleri belge eşlemesinin hatalı değişiklikleri reddettiğini sınar.

Build veya belge kapısı başarısızsa önce neden giderilir; bağımlılık/uyarı/izin kontrolü sırf yeşil sonuç için kaldırılmaz. Hook deneyi, server restart, dağıtım veya oyun dosyası yazma bu iş akışına gizlenmez. İlk foundation kesitinde remote/push yapılmamıştı. Güncel public GitHub yayını, protected main ve doğrulama kayıtları [yayın raporundadır](github-publication.md).

## Birincil kaynaklar

CMake preset dosyası standart configure/build/test ayrımını kullanır. [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html). Engine incelemesi Windows PE başlıklarının belgelenmiş mimari ve section alanlarını temel alır. [Microsoft PE formatı](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format). Bunlar kendi implementasyon/test kayıtlarımızın yerine geçmez.

## D1-N2 profile üretimi ve açık dosya gözlemi

`python tools/engine_profiles.py` gözlem JSON'undan native constexpr veriyi üretir; `--check` stale çıktıyı reddeder ve standart build girişinde zorunludur. C# aynı JSON'u assembly içine gömer. Profil bayrağını değiştirerek runtime izni verme yolu yoktur. [N2 raporu](d1-engine-preflight.md) gerçek CLI komutları, exit 1/2/3 ayrımı, Windows 10+ sınırı ve doğrulama kapsamını verir. Standart build oyun dosyası istemez/çalıştırmaz; `saex_engine_image_probe` yalnız açık dosya gözlem girişidir.

Component map'e N2 profile/generator/managed/preflight kaynak sahipliği eklendi. N1 raporu sahipliği, engine test ailesinin tamamı yerine mevcut `test_dependency.py` ve `test_sdk_configure.py` dosyalarına bağlandı; N2 testleri kendi raporu ve genel engine kabul belgelerini zorunlu günceller. Genel contracts/test kuralları korunur. Böylece yeni N2 testi N1 SDK recipe'si değişmiş gibi kaydedilmez.

## D1-N2 başlangıç DLL build ve audit

[Kod 0.1.3 modül raporu](d1-bootstrap-module.md) C ABI, loader kısıtları, state ve test kapsamını tanımlar. Standart x86 build saex_bootstrap.dll ile linker map üretir; CTest DLL'yi yüklemeden **önce** `tools/check_bootstrap.py` artifact denetimi çalışır. Yeni 10 audit negatif testi diğer tooling testlerinden sonra gelir. DLL yalnız x86 target'tır; C++20 session x64 testine de girer. Core/compiler standardı veya N1 source/recipe/patch değişmedi.

`out/verification/engine/bootstrap-audit-<Configuration>.json` yalnız artifact audit sonucu demektir; bütün testlerin veya gerçek GTA başlangıcının geçtiğini göstermez. Bütün standart akış başarılı olunca normal docs baseline kaydı yapılır. Component map yeni C ABI/session/module/ortak dosya gözlemi ve audit testlerini bootstrap raporu ve execution/engine belgelerine bağlar. Aynı-process güvenilir loader testinin modül yolu kullanıcı içeriği indirme veya production yükleme API'si olarak sunulmaz.

## Kod 0.1.4 askıda process gözlemi

[Observer raporu](d1-suspended-process.md) yeni opt-in CLI ve child ownership/cleanup sözleşmesini tanımlar. Standart Windows build ayrı observer library/CLI, boşluk içeren ada sahip kendi fixture executable'ımız ve native.suspended_image testini üretir; fixture pozitif kontrol ve 12 lifecycle çevrimi GTA kurulumu gerektirmez. Ayrıca 5 process CLI ret testi çalışır; eski image CLI corpus'u ortak pe_fixture.py'ye taşındı. Observer bootstrap DLL'sine linklenmez. Gerçek GTA deneyi yalnız açık --observe-suspended çağrısıyla ayrı yapılır; standart build GTA'yı başlatmaz. Sonuç, artifact SHA-256 ve oyun dosyasının önce/sonra hash'i out/verification/engine/ altında, kalıcı özeti raporda tutulur.

## Kod 0.1.5 native başlangıç aracı

[engine startup raporu](d1-native-startup.md) yeni C# geliştirme komutunu ve upper-directory kapsamını tanımlar. Standart managed runner, StartupTests corpus'unu da çalıştırır; ek NuGet paketine veya GTA kurulumuna ihtiyaç duymaz. Ayrı yerel CLI: `dotnet managed/Saex.Tools/bin/Debug/net10.0/Saex.Tools.dll engine startup <gta_sa.exe>`. Exit 3 statik alt kapsam tamamlandı/runtime kapalı; 1 ret veya eksik modül metadata'sı, 2 kullanım hatasıdır. JSON ayrı dosyaya kaydedilebilir; komut kendi başına dosya yazmaz ve oyun başlatmaz. Mevcut inspect/native observer/bootstrap girişleri değişmez.

## Kod 0.1.6 loader hedefleri

X86 standart build artık saex_loader_observer, saex_engine_loader_probe ve owned EXE/DLL/TLS fixture hedeflerini derler; native.loader_observation ve altı testli test_loader_probe.py kontrolünü çalıştırır. Testler kendi fixture'larını başlatır; GTA otomatik başlatılmaz. Loader fixture EXE/DLL hedeflerine özel static CRT, dört modüllük test closure'ını sabitler; bootstrap/core CRT ayarı değişmez. X64 preset bu x86 loader hedeflerini oluşturmaz, ortak SuspendedImage regresyonunu çalıştırır. Yeni kaynaklar component-map ile [loader raporuna](d1-loader-observation.md), normatif engine/native/launcher ve kabul belgelerine bağlıdır.

Gerçek oyun deneyi ayrı açık komuttur: saex_engine_loader_probe --observe-loader tam-gta-exe-yolu. Bu komut dosya inspect'ten farklı olarak owned process oluşturur ve Windows loader'ı ilerletebilir. Initialization/attach açmaz. Exit 1 politika reddi dahil eksik gözlem, 3 breakpoint adayı ve doğrulanmış exit, 2 yanlış kullanım demektir. Çıktı rapora outcome türüyle kaydedilir; ret sonucu build failure veya initialization pass diye gizlenmez.

## Kod 0.1.7 policy üretim akışı

İnceleme sonrası contracts/engine/loader-policy.json bilinçli güncellenir; python tools/loader_policy.py C++ verisini üretir. Normal tools/build.ps1 yalnız --check çalıştırır; source/modül keşfi veya otomatik hash onayı yoktur. Basename sırası kültürden bağımsız ordinaldir. Yeni sekiz testli test_loader_policy.py her platformda, yedi testli test_loader_probe.py x86'da çalışır. CMake core standardı C++20 ve SDK private sınırı korunur. [Yeni rapor](d1-loader-policy.md) source, generated ve caller mapping'inin belge sahibidir.

Gerçek deney komutu --observe-reviewed-loader ile ayrıca çağrılır; standart build GTA'yı başlatmaz. Yerel out/experiments kopyası bu kesitte manuel laboratuvar hazırlığıdır; otomatik staging veya dağıtım aracı kodlanmadı. Original executable/ASIs değiştirilmez, kopya binary'ler Git dışında kalır.

## Kod 0.1.8 — Context build/deney girişi

[LaunchContext](d1-launch-context.md) Windows observer static library'sine eklenir; bootstrap DLL'sine linklenmez. Standart build native.launch_context fixture/test target'larını x86/x64'te çalıştırır; x86 loader CLI corpus'u dokuz testtir. Ayrı gerçek gözlem --observe-context-loader <gta_sa.exe> <absolute-working-directory> ile açılır, standart build GTA'yı çalıştırmaz. Source map bu helper/CLI/test ailesini yeni rapor ve normatif/kabul sahiplerine bağlar.

## Kod 0.1.9 — Mapping ledger build/test

[Lifecycle](d1-loader-lifecycle.md) ayrı saex_loader_lifecycle C++20 static target ve on senaryolu native.loader_mappings suite'i ekler; OS/SDK bağımlılığı yoktur. X86 loader observer target'ı library'ye bağlanır; bootstrap DLL'si bağlanmaz. Var olan x86 OS/CLI corpus'una history/active invariants eklenir. Standart build GTA başlatmaz; gerçek unload ayrı explicit context komutuyla kaydedilir.

## Kod 0.1.10 — Linkage komutu ve corpus

[Statik linkage aracı](d1-native-linkage.md) standart C# proje derlemesine ve 22 test kaydıyla mevcut runner'a dahildir; ek NuGet paketi, GTA kurulumu veya native işlem çalıştırma gerektirmez. Açık komut: `dotnet managed/Saex.Tools/bin/Debug/net10.0/Saex.Tools.dll engine linkage <consumer> <imported-module> <candidate.dll>`. Exit 3 statik eşleşme/runtime kapalı, 1 incomplete veya girdi reddi, 2 kullanım hatasıdır. Çıktıyı kaydetmek çağıranın seçimidir. Yeni parser/corpus source→doc kuralı bu rapor, engine/native sözleşmeleri ve AC-90 sahiplerini zorunlu kılar; startup Reader yardımcı erişimi eski envanter sahibini de günceller. Standart tools/build.ps1 sırası ve son successful baseline kuralı korunur.

## Kod 0.1.11 — Entry observer build/test

X86 standart build [entry corpus'unu](d1-entry-boundary.md) native.entry_observation olarak ekler: üç own EXE varyantı (mutate/fault/stall), existing DLL/TLS marker, 12 senaryo ve 12 warm çevrim. Yeni hedefler yalnız test fixture'ı için static CRT kullanır. Python loader CLI corpus'u 11 testtir. X64 kendi eski alt hedeflerini korur; GTA otomatik başlatılmaz. Gerçek --observe-entry-boundary komutu ayrı açık deneydir ve DLL/TLS kodunu ilerletebilir. Bütün owner belgeler/ADR/status güncellenir; başarısız ilk testler out/verification/engine/ altında saklanır.

## Kod 0.1.11 — Entry supplement bağı

Source bilinçli değişince python tools/entry_policy.py çalışır. Standart build yalnız --check ile drift/stale output reddeder ve test_entry_policy.py’nin sekiz testini yürütür. Normal build GTA başlatmaz. [Ayrıntı](d1-entry-boundary.md).

## GitHub yayın ve CI akışı

[Kaynak yayını sözleşmesi](github-publication.md) yeni workflow, lisans, marka ve topluluk dosyalarının sahibidir. Standart Windows build sekiz GitHub event/base testi daha çalıştırır. `python tools/ci.py docs` gerçek PR base/push-before SHA ile doküman eşlemesini doğrular; sıfır ilk-push ve bozuk event ayrı ele alınır. `python tools/ci.py linux` portable C++/C# fixture ve generator corpus'unu yürütür; Windows/GTA runtime kapsamını taşımaz.

GitHub hosted Windows matrisi x64 Debug/Release için `windows-2022`, x86 Debug/Release için `windows-2025`; Linux matrisi x64 Debug'dır. x86 fixture zinciri, incelenmiş inline kernel32 GetLastError komut biçimini gerektirir; Server 2022'nin farklı API uygulaması destek kabul edilmez. Windows 2025 ortamındaki sonuç ayrıca doğrulanır; bu runner seçimi gerçek GTA için yeni OS/hash onayı vermez. Sır taraması Gitleaks 8.30.1'in SHA-256 ile doğrulanmış binary'siyle Git geçmişini redacted olarak tarar. Opt-in SDK işi exact CMake/MSVC/SDK lock'u gevşetmez. Her platformun yerel/GitHub sonuçları yayın kaydında ayrı tutulur. Başarı rozeti veya required check ancak gerçek workflow sonucuna bağlanır.

Kullanıcıya özgü Python yolları genel kurulum örneklerinden çıkarıldı. `C:\Python312\python.exe` yalnız örnek yoldur; gerçek kurulu Python 3 yolu `-Python` ile seçilir. Kaynak/ABI/GTA davranışı değişmedi.

Genel kaynak metni LF kullanır; raw SHA-256 ile bağlı `contracts/engine/*.json` ve `tools/native/patches/*.json` için `.gitattributes` dönüşümü kapatır. Bu istisna mevcut kanıt digest'lerini korur; yeni clone'da generator/SDK verify kapıları ayrıca geçmelidir.

Temiz N1 SDK derlemesi için kısa bir checkout yolu kullanılır: uzun kullanıcı/OneDrive/alt-checkout yollarında MSBuild compiler-ID tlog oluşturma adımı hata verebilir. İlk uzun-yol başarısızlığı ve kısa checkout Debug/Release başarıları [yayın raporunda](github-publication.md) ayrı kaydedilir. Lock/toolchain kontrolü bu hata için gevşetilmez.


Yayın sonrası değişiklikler kısa ömürlü dal → PR → yedi zorunlu kontrol → squash merge sırasını izler. `main` güncel base'i ister; başka onaylayıcı zorunlu değildir (başlangıçta tek Owner). PR base doküman eşlemesini ve bütün standart matrisi geçmeden merge yapılmaz. `.github` topluluk deposunun `main` dalı da PR/linear history ile korunur; uygulama build kontrolleri yalnız `saex` deposundadır.

## Kod 0.1.12 — Proxy dönüş sınırı

[Proxy dönüş kesiti](d1-proxy-return.md) için source bilinçli değişince python tools/proxy_policy.py çalıştırılır; normal Windows ve Linux CI yalnız --check ve yedi generator negatif testini yürütür. X86 native.proxy_observation sekiz EXE/DLL varyantı, 19 senaryo ve 12 warm çevrim içerir; kod yalnız kendi canary dosyalarını kullanır. Windows CLI corpus’u 13 testtir. Standart build GTA başlatmaz. Component-map yeni policy/header/generator/corpus ve eski ortak observer owner’larını birlikte denetler.

## Kod 0.1.13 — Startup çağrı sınırı

[Startup-call kesiti](d1-startup-call.md) source değişince python tools/startup_policy.py üretimi, normal Windows/Linux akışında --check + yedi policy testi kullanır. X86 native.startup_observation dokuz standart CRT EXE, tek mevcut proxy DLL, 19 senaryo + canary control + 12 warm çevrimdir. CLI corpus 15 testtir. Yeni kaynaklar/component-map bu rapor ve bütün örtüşen sahip belgeleri denetler; normal build GTA çalıştırmaz.

## Kod 0.1.14 — Codec dönüş kesiti

Standart Windows ve portable Linux kaynak akışı codec_policy.py --check ile test_codec_policy.py çalıştırır. X86 CTest yeni native.codec_observation suite ve own root/leaf DLL zincirini çalıştırır; standart build GTA başlatmaz. Gerçek denemeler ayrı explicit CLI ile yapılır. [Sözleşme ve doğrulama](d1-codec-return.md).

## Kod 0.1.15 — Codec fonksiyon bağları

binding_policy.py üretimi source değişiminde bilinçli çalıştırılır; Windows/Linux standart akışları --check ve altı policy testi kullanır. X86 native.codec_bindings own fixture suite eklendi; standart build GTA başlatmaz. [Sözleşme ve kanıt](d1-codec-bindings.md).

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

0.1.16 x86 CMake artık aynı configuration bootstrap DLL/map auditinden artifact header üretir; Python executable build.ps1 içinde absolute çözülür. Standart akışlar yeni altı policy, üç x86 artifact-binding ve iki x86 CLI testiyle güncellendi. Son dört Windows akışı geçti; gerçek GTA matrisinin explicit/private koşuları standart CI’ye dahil değildir. Yeni binary, eski özel ASI kopyasıyla eşleşmeyebilir; immutable kanıt klasörü üzerine otomatik kopyalama yapılmaz.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

Yeni policy generator ve altı Python testi Windows/Linux akışlarına bağlı; x86 native.bootstrap_lifecycle ve altı ek export audit testi, iki CLI testi standart Windows build'dedir. `stackWriteAttempted`, yazma izni/girişimi/ilerlemesi ayrımını korur; canonical full logs `build-bootstrap-*-*-complete.log`. Standart CI GTA açmaz. Linux/hosted/N1 SDK bu kesitte tekrar koşulmadı.

## Kod 0.1.18 bağlantısı

Standart build ve CI girişleri frame_target_policy --check ve Python negatiflerini; CTest portable frame sözleşmesini ve x86 bootstrap faz corpusunu çalıştırır. Reçete adresleri CLI girdisi değildir. Yeni runtime deneyleri yalnız yeni out kanıt adlarına yazılır; önceki matrisler korunur. [Aday ve doğrulama raporu](d1-frame-target.md).

## Kod 0.1.19 bağlantısı

Standart build/CI startup_return_policy --check ve yedi Python negatifini çalıştırır; x86 CTest doğal startup corpusunu, portable frame suite ek startup spec kontrollerini içerir. Runtime deneyleri yalnız ayrı özel kopya ve yeni evidence adlarıyla yapılır. [Sözleşme ve kanıt](d1-startup-return.md).

## Kod 0.1.19 — Bağımsız image koruma deneyi

Standart build yalnız yeni aracın 17 taşınabilir testini yürütür. Gerçek Windows ölçümü, x86 fixture derlendikten sonra `python tools/native/image_protection_probe.py --configuration Debug` (veya Release) ile açıkça çağrılır. Sabit fixture dışında EXE/PID kabul edilmez; çıktı stdout JSON, başarı 0/hata 1/kullanım 2. Windows dışı gerçek ölçüm reddedilir; yeni paket bağımlılığı yoktur. [Sözleşme ve kanıt](d1-image-protection.md).

## Kod 0.1.20 bağlantısı

Standart Windows/Linux generator ve test listelerine CRT policy kontrolü/yedi negatif test eklendi. CTest portable spec ve x86 CRT corpusunu çalıştırır; gerçek GTA yalnız ayrı opt-in deneydir. Kaynak ve owner dokümanları aynı değişiklikte denetlenir. [Sözleşme ve kanıt](d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

0.1.21 standart akışında `application_entry_policy.py --check`, yedi policy testi ve portable spec suite çalışır; x86 profilde application corpus ve CLI testleri de koşulur. Gerçek GTA opt-in komutu `saex_engine_loader_probe --observe-application-entry <exe> <absolute-working-directory>` şeklindedir. Özel kopyadaki exact EXE/beş yerel bağımlılık ve profile uygun SAEX artifact aynı build kanıtına bağlanır; önceki rapor/artifact dosyaları üzerine yazılmaz.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

0.1.22 standart akışı `platform_startup_policy.py --check`, yedi policy testi, portable spec, x86 platform corpus/canary ve CLI testlerini kapsar. Gerçek GTA komutu `saex_engine_loader_probe --observe-platform-startup <exe> <absolute-working-directory>` şeklindedir. Host ayarı için yalnız ayrı SPI_GET gözlemi kayıtlanabilir; gözlenen SystemParametersInfoA SET çağrısı yürütülmez. Önceki raporlar üzerine yazılmaz; build artifact'ı değiştiğinde yeni matris gerekir.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.

Yeni strict kaynak `tools/platform_suppression_policy.py` ile üretilir, `--check` standart build/CI'da zorunludur. `saex_engine_loader_probe --observe-platform-suppression <gta_sa.exe> <absolute-working-directory>` yalnız özel kopyada açık context-write deneyidir; genel build GTA'yı çalıştırmaz. Exit 3 sınırlı deney sonucu, 1 ret, 2 kullanımdır; initialized/attach izni vermez.

## Kod 0.1.24 bağlantısı

Ayrı instance-startup modu platform bastırma dönüşünden gerçek named event oluşturma/açma ve doğal helper dönüşüne ilerler. Mevcut event veya NULL handle durumunda pencere kolundan önce ret verilir. Önceki suppression modu restore ederek bitmeye devam eder; yeni mod doğal API sonrası eski CALL bağlamını geri yazmaz. [Sözleşme ve doğrulama](d1-instance-startup.md). Oturumdaki ortak event ömrü process-private değildir; observer sinyal durumunu değiştirmez. C ABI 1/GNS/otorite aynı; pencere/renderer/doğal frame ve N2/N3/D1/D2 kapıları açıktır.

## Kod 0.1.25 bağlantısı

Ayrı event-dispatch modu, instance dönüşünden doğal olay dağıtıcısı CALL/entry ve uygulama işleyicisi CALL önüne ilerler. Üç durakta argüman, dönüş adresi, register ve yaşayan caller stack doğrulanır; uygulama işleyicisi çalıştırılmaz. [Sözleşme ve sonuç](d1-event-dispatch.md). Eski instance terminali, C ABI 1/GNS/otorite aynı; yeni bağımlılık/kalıcı migration yoktur. AppEventHandler gövdesindeki executable yönlendirmesi, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.26 bağlantısı

`--observe-application-routing` önceki event-dispatch kanıtından sonra yalnız rsINITIALIZE=24 rotasını yürütür: işleyici entry → executable detour → indirect JMP → ilk oyun initializer CALL öncesi. 39 index/11 hedef tablosu, rel32/absolute operand ve dört yığın/register sınırı doğrulanır. [Sözleşme ve sonuç](d1-application-routing.md). Önceki mod kendi AppEventHandler CALL öncesi terminalini korur. Yeni modda `eventDispatchObservation.applicationHandlerCallAllowed=true`, routing nesnesinde initializer çağrı izni false olur; önceki stage/verified ara kanıtı korunur. C++ trace/API yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni bağımlılık/kaldırılan özellik/kalıcı migration yoktur. Oyun initializer gövdesi, RsInitialize, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.27 bağlantısı

`--observe-game-prelude` ilk oyun initializer içine girer; exact boş Init ve üç yerelleştirme bayrağını yazan iki helper doğal olarak geri döner. Beş durak, stack/register/flags, yaşayan caller ve 16-byte veri penceresi denetlenir; yalnız üç veri byte değişebilir. CFileMgr CALL çalıştırılmaz. [Sözleşme ve sonuç](d1-game-prelude.md). Önceki application-routing terminali korunur; yeni üst modda routing nesnesinin initializerCallAllowed alanı true, prelude nesnesinin fileManagerCallAllowed ve initializerReturnVerified alanları false olur. C++ observer yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni dependency/kaldırma/kalıcı migration yoktur. CFileMgr, streaming/pad, initializer dönüşü, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.28 bağlantısı

`--observe-file-manager-entry` CFileMgr içine doğal CALL ve ilk üç PUSH komutunu açar; 0x5386FB CRT cwd CALL önünde durur. İki durakta buffer/maxlen=128 ABI, nested return stack, register/flags, 136-byte root/guard ve localisation korunumu denetlenir. [Sözleşme ve sonuç](d1-file-manager-entry.md). Önceki prelude terminali korunur; yeni üst modda prelude fileManagerCallAllowed=true, manager cwdCallAllowed=false/fileManagerReturnVerified=false olur. CRT lock/SEH/OS/copy yolu henüz açılmaz. Gelecekte suffix yazımından önce NUL en geç buffer offset 126, başarılı dönüş ve ANSI byte uzunluğu kanıtı gerekir. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Manager/initializer dönüşü, streaming/pad, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.29 bağlantısı

`--observe-cwd-seh` CRT wrapper ve SEH prologue içine doğal CALL açar; kayıt kurulup yardımcı döndüğünde 0x836E9D noktasında durur. Üç durakta 80-byte stack, 28-byte NT_TIB, önceki kayıt ve caller/buffer/localisation korunumu denetlenir. [Sözleşme ve sonuç](d1-cwd-seh.md). Önceki manager terminali korunur; yeni üst modda manager cwdCallAllowed=true, cwdSeh lockPathAllowed/directoryApiAllowed/cwdReturnVerified/unwindVerified=false olur. Handler veya kilit/OS/copy yolu açılmaz; owned child sonunda kapatılır, eski TEB/context rollback yapılmaz. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Kilit/cwd, SEH sökümü, manager/initializer dönüşü, renderer/doğal frame ve N2/N3/D1/D2 açıktır.

## Kod 0.1.30 bağlantısı

`--observe-cwd-lock` lock(7) selector CALL ve ilk 17-byte gövdeyi doğal yürütür; CMP tamamlandığında 0x82ADCF JNE önünde durur. [Sözleşme ve sonuç](d1-cwd-lock.md). 100-byte stack, 16-byte slot penceresi, NT_TIB/önceki kayıt/caller/buffer korunur; slot değeri dereference edilmez. SlotPresent yalnız sıfırdan farklı word demektir, kritik bölüm veya kilit alma kanıtı değildir. Üst modda cwdSeh.lockPathAllowed=true yalnız selector iznidir; yeni branchAllowed/lazyInitializationAllowed/criticalSectionCallAllowed/lockAcquiredVerified=false. Önceki SEH terminali korunur; DR0 dışında yeni observer müdahalesi ve TEB/context rollback yoktur. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Mevcut/lazy dal, OS kilidi, cwd/SEH dönüşü ve N2/N3/D1/D2 açıktır.

## Kod 0.1.31 bağlantısı

`--observe-cwd-acquire` mevcut/unowned lock(7) nesnesi için doğal dal, admitted ntdll API entry/return ve CRT selector dönüşünü açar; 0x836EA4 terminalinde durur. [Sözleşme ve sonuç](d1-cwd-acquire.md). Beş durakta object/slot/84-byte caller/SEH korunumu ve API sonrası thread sahipliği doğrulanır. x86 24-byte kritik bölüm düzeni pinned Windows uygulamasına aittir; VOID dönüşte EAX başarı kodu sayılmaz. Heap veya aynı GTA image nesnesi için sınır/koruma denetimi vardır. Üst modda branch/criticalSectionCallAllowed=true, acquired readback ile ayrıdır; lazy/directory/unlock kapalı kalır. Önceki lock terminali korunur; kilit tutulurken bütün owned child kapatılır, observer veri/TEB/context rollback yapmaz. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Cwd/SEH/manager dönüşü ve N2/N3/D1/D2 açıktır.
