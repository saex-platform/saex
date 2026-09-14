# SAEX dokümantasyonu

Kod 0.1.40 / mimari v0.47: [streaming kanal belleğinin oluşturulması](development/d1-cd-stream-channels.md). Run-SAEX 13 kontrolle 5 × 48 sıfır byte ve global kaydı doğrular; CdStreamOpen çağrısı önünde durur. [Ghidra bridge](references/ghidra-bridge.md) kaydı mevcut native kodla karşılaştırıldı. Arşiv I/O, thread, doğal bellek bırakma, CPad ve renderer açık; D1 sürüyor.

SAEX — San Andreas Extended. Belgeler Türkçedir; API ve kaynak kimlikleri İngilizcedir. Önce **uygulama durumu**, ardından ilgili normatif sözleşme okunmalıdır.

| İhtiyaç | Başlangıç |
|---|---|
| Ne çalışıyor? | [Durum ve kanıt sınırı](development/status.md) |
| Nasıl derlerim? | [Geliştirme akışı](development/workflow.md) |
| Mimariyi öğrenmek | [Vizyon](vision.md), [genel mimari](architecture/overview.md), [platform tasarımı](platform-blueprint.md) |
| GTA entegrasyonu | [Engine adapter](architecture/engine-adapter.md), [native SDK sözleşmesi](architecture/native-sdk-integration.md), [D1 sırası](development/d1-engine-integration.md) |
| Native OS deneyi | [Çalıştırılmayan fixture üzerinde Windows image koruması](development/d1-image-protection.md) |
| Ağ ve otorite | [GNS kararı](decisions/network-transport.md), [otorite](networking/authority.md), [replikasyon](networking/replication.md) |
| Resource ve C# | [Runtime/SDK](resources/runtime-sdk.md), [manifest](resources/manifest.md), [güvenlik](resources/security-native.md) |
| Asset ve dünya | [Katalog](assets/catalog.md), [WorldPlan](architecture/world-plans.md), [yıkım](assets/destruction.md) |
| Yaşayan dünya hedefleri | [Population/trafik](gameplay/population-traffic.md), [AI/navigasyon](gameplay/agents-navigation.md), [hayvanlar](gameplay/animals-species.md) |
| Kabul ve araştırma | [Senaryolar](validation/scenarios.md), [araştırma kayıtları](decisions/research-register.md), [ADR](decisions/architecture-decisions.md) |
| İlerleme | [Yol haritası](roadmap.md), [değişiklik kaydı](development/change-log.md) |
| GitHub ve katkı | [Yayın sözleşmesi/raporu](development/github-publication.md), [katkı](../CONTRIBUTING.md) |

[V0.18 README arşivi](development/readme-history-v0.18.md), önceki ayrıntılı açıklamalar ve tam belge indeksini korur. Tarihsel metinlerdeki planlanan davranışlar güncel uygulama kanıtının yerine geçmez.

## Kod 0.1.12

[Proxy dönüş sınırı](development/d1-proxy-return.md) uygulanmıştır; test ve gerçek GTA kanıtı ilgili raporda ayrı tutulur. N2/D1 ve oynanabilir multiplayer kapsamı açık kalır.

[0.1.13 startup çağrı sınırı](development/d1-startup-call.md): üçüncü durak, IAT write watch, çağıran/argument ve image örnekleri.

[0.1.14 codec dönüş sınırı](development/d1-codec-return.md): dinamik load dönüşü, yeni module mapping ve negatif fixture corpus.

[0.1.15 codec fonksiyon bağları](development/d1-codec-bindings.md): pointer tablosu, modül kimliği ve negatif corpus.

## Kod 0.1.16 — ASI bootstrap yükleme

[ASI yükleme sözleşmesi](development/d1-asi-bootstrap-load.md), iki ek call/return durağı ve boş tarama korumasını tanımlar. Yeni mod, build auditinden geçen aynı SAEX DLL bitlerini özel klasörde `.asi` adıyla yüklemeyi denetler; Release 28/Debug 31 exact pin, tam yol kontrolü ve owned child cleanup uygulanır. Önceki CLI durakları ve C ABI 1 korunur; `asiObservation` eski modlarda null olur. Initialize/Query/Stop çağrıları bu kesitte çalışmaz; N2/D1/D2 açık kalır. Test ve gerçek GTA kanıtı ilgili raporda ayrı izlenir.

0.1.16 yerel doğrulama: dört Windows standart akışı ve gerçek GTA 12/12 ASI üzerinden SAEX DLL yükleme dönüşü geçti. Bu sürümün kapsamı export çağrılarını içermiyordu; 0.1.17 kanıtı aşağıdadır. N2 bütünü açık kalır.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](development/d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.24 bağlantısı

Ayrı instance-startup modu platform bastırma dönüşünden gerçek named event oluşturma/açma ve doğal helper dönüşüne ilerler. Mevcut event veya NULL handle durumunda pencere kolundan önce ret verilir. Önceki suppression modu restore ederek bitmeye devam eder; yeni mod doğal API sonrası eski CALL bağlamını geri yazmaz. [Sözleşme ve doğrulama](development/d1-instance-startup.md). Oturumdaki ortak event ömrü process-private değildir; observer sinyal durumunu değiştirmez. C ABI 1/GNS/otorite aynı; pencere/renderer/doğal frame ve N2/N3/D1/D2 kapıları açıktır.

## Kod 0.1.25 bağlantısı

Ayrı event-dispatch modu, instance dönüşünden doğal olay dağıtıcısı CALL/entry ve uygulama işleyicisi CALL önüne ilerler. Üç durakta argüman, dönüş adresi, register ve yaşayan caller stack doğrulanır; uygulama işleyicisi çalıştırılmaz. [Sözleşme ve sonuç](development/d1-event-dispatch.md). Eski instance terminali, C ABI 1/GNS/otorite aynı; yeni bağımlılık/kalıcı migration yoktur. AppEventHandler gövdesindeki executable yönlendirmesi, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.26 bağlantısı

`--observe-application-routing` önceki event-dispatch kanıtından sonra yalnız rsINITIALIZE=24 rotasını yürütür: işleyici entry → executable detour → indirect JMP → ilk oyun initializer CALL öncesi. 39 index/11 hedef tablosu, rel32/absolute operand ve dört yığın/register sınırı doğrulanır. [Sözleşme ve sonuç](development/d1-application-routing.md). Önceki mod kendi AppEventHandler CALL öncesi terminalini korur. Yeni modda `eventDispatchObservation.applicationHandlerCallAllowed=true`, routing nesnesinde initializer çağrı izni false olur; önceki stage/verified ara kanıtı korunur. C++ trace/API yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni bağımlılık/kaldırılan özellik/kalıcı migration yoktur. Oyun initializer gövdesi, RsInitialize, renderer/window ve doğal frame sonraki kapılardır; N2/N3/D1/D2 açık kalır.

## Kod 0.1.27 bağlantısı

`--observe-game-prelude` ilk oyun initializer içine girer; exact boş Init ve üç yerelleştirme bayrağını yazan iki helper doğal olarak geri döner. Beş durak, stack/register/flags, yaşayan caller ve 16-byte veri penceresi denetlenir; yalnız üç veri byte değişebilir. CFileMgr CALL çalıştırılmaz. [Sözleşme ve sonuç](development/d1-game-prelude.md). Önceki application-routing terminali korunur; yeni üst modda routing nesnesinin initializerCallAllowed alanı true, prelude nesnesinin fileManagerCallAllowed ve initializerReturnVerified alanları false olur. C++ observer yeniden derlenir; C ABI 1/GNS/otorite aynı, yeni dependency/kaldırma/kalıcı migration yoktur. CFileMgr, streaming/pad, initializer dönüşü, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.28 bağlantısı

`--observe-file-manager-entry` CFileMgr içine doğal CALL ve ilk üç PUSH komutunu açar; 0x5386FB CRT cwd CALL önünde durur. İki durakta buffer/maxlen=128 ABI, nested return stack, register/flags, 136-byte root/guard ve localisation korunumu denetlenir. [Sözleşme ve sonuç](development/d1-file-manager-entry.md). Önceki prelude terminali korunur; yeni üst modda prelude fileManagerCallAllowed=true, manager cwdCallAllowed=false/fileManagerReturnVerified=false olur. CRT lock/SEH/OS/copy yolu henüz açılmaz. Gelecekte suffix yazımından önce NUL en geç buffer offset 126, başarılı dönüş ve ANSI byte uzunluğu kanıtı gerekir. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Manager/initializer dönüşü, streaming/pad, RsInitialize, renderer ve doğal frame ile N2/N3/D1/D2 açık kalır.

## Kod 0.1.29 bağlantısı

`--observe-cwd-seh` CRT wrapper ve SEH prologue içine doğal CALL açar; kayıt kurulup yardımcı döndüğünde 0x836E9D noktasında durur. Üç durakta 80-byte stack, 28-byte NT_TIB, önceki kayıt ve caller/buffer/localisation korunumu denetlenir. [Sözleşme ve sonuç](development/d1-cwd-seh.md). Önceki manager terminali korunur; yeni üst modda manager cwdCallAllowed=true, cwdSeh lockPathAllowed/directoryApiAllowed/cwdReturnVerified/unwindVerified=false olur. Handler veya kilit/OS/copy yolu açılmaz; owned child sonunda kapatılır, eski TEB/context rollback yapılmaz. C++ observer yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Kilit/cwd, SEH sökümü, manager/initializer dönüşü, renderer/doğal frame ve N2/N3/D1/D2 açıktır.

## Kod 0.1.30 bağlantısı

`--observe-cwd-lock` lock(7) selector CALL ve ilk 17-byte gövdeyi doğal yürütür; CMP tamamlandığında 0x82ADCF JNE önünde durur. [Sözleşme ve sonuç](development/d1-cwd-lock.md). 100-byte stack, 16-byte slot penceresi, NT_TIB/önceki kayıt/caller/buffer korunur; slot değeri dereference edilmez. SlotPresent yalnız sıfırdan farklı word demektir, kritik bölüm veya kilit alma kanıtı değildir. Üst modda cwdSeh.lockPathAllowed=true yalnız selector iznidir; yeni branchAllowed/lazyInitializationAllowed/criticalSectionCallAllowed/lockAcquiredVerified=false. Önceki SEH terminali korunur; DR0 dışında yeni observer müdahalesi ve TEB/context rollback yoktur. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Mevcut/lazy dal, OS kilidi, cwd/SEH dönüşü ve N2/N3/D1/D2 açıktır.

## Kod 0.1.31 bağlantısı

`--observe-cwd-acquire` mevcut/unowned lock(7) nesnesi için doğal dal, admitted ntdll API entry/return ve CRT selector dönüşünü açar; 0x836EA4 terminalinde durur. [Sözleşme ve sonuç](development/d1-cwd-acquire.md). Beş durakta object/slot/84-byte caller/SEH korunumu ve API sonrası thread sahipliği doğrulanır. x86 24-byte kritik bölüm düzeni pinned Windows uygulamasına aittir; VOID dönüşte EAX başarı kodu sayılmaz. Heap veya aynı GTA image nesnesi için sınır/koruma denetimi vardır. Üst modda branch/criticalSectionCallAllowed=true, acquired readback ile ayrıdır; lazy/directory/unlock kapalı kalır. Önceki lock terminali korunur; kilit tutulurken bütün owned child kapatılır, observer veri/TEB/context rollback yapmaz. C++ trace yeniden derlenir; C ABI 1/GNS/otorite/IPC aynı, dependency/kaldırma/kalıcı migration yoktur. Cwd/SEH/manager dönüşü ve N2/N3/D1/D2 açıktır.

## Kod 0.1.32 bağlantısı

[Cwd sorgusu ve kopyalama önkoşulu](development/d1-cwd-query.md), Windows fixture doğrulaması ve güncel OS profilinde GTA kabul reddini ayrı kaydeder. [ReAgent incelemesi](references/reagent.md) isteğe bağlı native araştırma aracının rolünü açıklar; kurulum veya runtime kullanım kanıtı değildir.

## Güncel D1 çalıştırma kesiti — 0.1.36

[Dosya yöneticisinin tam dönüşü ve Run-SAEX](development/d1-file-manager-ready.md): tek komutla gerçek GTA başlangıç alt kesiti, çevrimdışı HTML/JSON raporu ve kalan iş sınırları.

## Kod 0.1.37

[Streaming tablo hazırlığı](development/d1-cd-stream-tables.md) · [Ghidra bridge kurulumu ve kullanımı](references/ghidra-bridge.md). Native izin ve static araştırma kanıtı ayrı izlenir.

Güncel D1 kesiti: [0.1.40 streaming kanal belleği](development/d1-cd-stream-channels.md); arşiv açma, okuma ve thread yaşam döngüsü sonraki kapılardır.
