# D1-N2 — Frame adayı ve fazlar arasında bayt denetimi

Kod 0.1.18 / mimari v0.25, 13 Eylül 2026. Dört Windows akışı ve gerçek GTA 12/12 koşuda 36/36 bayt örneği geçti. Bu rapor bir native hook yetkisi vermez.

## Sorumluluk ve sözleşme

`--observe-frame-target <exe> <absolute-working-directory>` açık bootstrap yaşam döngüsü deneyini kullanır. Ek davranış sadece okuma: exact EXE içindeki beş baytlık doğrudan CALL ve 16 baytlık hedef öneki CREATE_PROCESS, doğrulanmış ASI dönüşü ve sekizinci bootstrap çağrısının terminal Stop dönüşünde karşılaştırılır. Bunlar örnekleme noktalarıdır; motorun initialized olduğu anlamına gelmez. Son durakta owned çocuk sonlandırılır.

Sabit reçete [frame-target-policy.json](../../contracts/engine/frame-target-policy.json), gözlem profili ve bootstrap politika zincirine hash ile bağlıdır. CLI adres veya süreç kimliği kabul etmez. Eski CLI modlarının durma sınırı korunur; yeni rapor alanı bu modlarda `null` olur. Ek DLL izni, native çağrı, frame breakpoint, kod yazımı veya gameplay ilerletme yoktur. Önceden izinli SAEX C ABI yığın yazımları [bootstrap sözleşmesine](d1-bootstrap-lifecycle.md) tabidir.

## Kaynak ve adayın anlamı

Pinned Plugin-SDK-SA `b55e89b336a81448c1aa1a5b188431c9845ebaa9`: [Events.h](https://github.com/Dryxio/plugin-sdk-sa/blob/b55e89b336a81448c1aa1a5b188431c9845ebaa9/shared/Events.h), [CGame.cpp](https://github.com/Dryxio/plugin-sdk-sa/blob/b55e89b336a81448c1aa1a5b188431c9845ebaa9/plugin_sa/game_sa/CGame.cpp), [CGame.h](https://github.com/Dryxio/plugin-sdk-sa/blob/b55e89b336a81448c1aa1a5b188431c9845ebaa9/plugin_sa/game_sa/CGame.h). Arşiv SHA-256 `3ff497b91744c189c38003e619ce5730b8998a634b4bef4c0d47ede996c758e9` ve dosya hash'leri yerelde karşılaştırıldı; metadata reçetede kayıtlıdır. Normal build arşivi edinmez veya bu üç dosyayı linklemez; N1'in 23 dosyalık derleme kapsamı değişmedi.

| SDK olayı | CALL VA → disk hedefi | Kaynak yorumu / açık sınır |
|---|---|---|
| gameProcessEvent | 0x53E981 → 0x53BEE0 | Kaynakta CdeclEvent, PRIORITY_AFTER, void(); CGame::Process adresiyle örtüşür. Bu kesitin tek runtime bayt adayı. |
| initGameEvent | 0x748CFB → 0x53E580 | CdeclEvent, PRIORITY_AFTER. CGame::Initialise (0x53BC80, bool(char const*)) ile aynı hedef değildir; birbirinin yerine kullanılamaz. |
| shutdownRwEvent | 0x53D910 → 0x53BB80 | PRIORITY_BEFORE; CGame::ShutdownRenderWare ile disk adresi örtüşür. Bu kesitte runtime örneklenmez. |

Kaynak `PRIORITY_AFTER` kancasının event sıralamasını tarif eder; SAEX'in çağıranı, stack ABI'si, thread'i veya pause/menu/loading fazındaki sıklığını kanıtlamaz. 16 bayt bir fingerprint'tir; komut çözümleyici veya trampoline uzunluğu hesabı değildir. Detour uzunluğu olarak kullanılamaz. Preferred image base dışındaki eşleme reddedilir; relocation normalizasyonu yalnız önceki SAEX DLL export denetiminin kapsamındadır.

## Veri akışı, hata ve genişleme

Dosya kimliği/layout/anchor kapısı → parent modül pinleri → CREATE_PROCESS örneği → ASI dönüş örneği → sekiz SAEX C ABI dönüşü → terminal örnek → verified veya fail-closed ret → doğrulanmış child çıkışı. CALL rel32 signed ve taşmasız çözülür; hedef ile önek aralıkları image içinde ve birbirinden ayrıdır. Her okuma committed, nonguard, executable MEM_IMAGE ve aynı EXE allocation içinde olmalıdır. Okunamayan veri ile bayt uyuşmazlığı ayrı bayraklarla raporlanır. Hatalı recipe veya ilk örnek ret durumunda loader ilerlemez; ASI örneği ret durumunda bootstrap çağrılmaz; terminal örnek ret durumunda bootstrap kanıtı korunur fakat frame adayı verified olmaz.

Üç eşleşme hedefin bu anlarda beklenen baytları taşıdığını gösterir; aralarda değişip geri alınan baytları veya başka thread yürütmesini dışlamaz. Sürekli integrity/anti-cheat veya tam fonksiyon hash'i değildir. Örnek `threadId`, durdurulan owned main thread'in kimliğidir; frame fonksiyonunun bu thread'de çağrıldığı iddiası değildir. `nativeFunctionCalled=false`, observer'ın frame adayını çağırmadığını belirtir; bütün thread'lerin yürütme geçmişi ölçülmez.

## Kabul ve sonraki geçiş

Pozitif üç faz; bozuk opcode/displacement, overflow, çakışma, yanlış image base/size, okunamayan/non-executable hedef, ilk örnek ile ASI dönüşü arasında ve bootstrap sırasında hedef değişimi; eski bootstrap modunun frame kontrolü yapmaması sınanır. Test ve gerçek GTA sonuçları aşağıda aynı değişiklikte kaydedildi. N2/AC-90 tamamen kapanmaz; AC-91 ve N3 açık kalır.

Sonraki kesit, bootstrap sonrasındaki startup yolunun ayrı incelemesi ve doğal frame çağrısının call/return, thread, register/stack ve loading/menu/pause/gameplay faz kanıtıdır. Mevcut wrapper'ın ASI taraması sonrasında EXE korumasını değiştiren yolu yeni yürütme izni sayılmaz. Bu yol ve asset/renderer önkoşulları çözülmeden rastgele CGame::Process çağrılmaz veya N3 hook capability açılmaz.

## Doğrulama ve tekrar üretme kanıtı

| Windows profili | Native suite | Managed test | Python test |
|---|---:|---:|---:|
| x86/Debug | 16 | 79 | 135 |
| x86/Release | 16 | 79 | 135 |
| x64/Debug | 7 | 79 | 91 |
| x64/Release | 7 | 79 | 91 |

Portable `native.frame_target`: üretilmiş gerçek reçete dahil 20 kontrol; negatif/pozitif rel32, son-byte drift, okunmama bayrakları, overflow/range/overlap. x86 `native.bootstrap_lifecycle`: önceki 24 senaryoya ek yedi frame senaryosu, toplam 31 ve 12 warm çevrim; beş mevcut export HIGHLOW kontrolü de korunur. İlk sample ile ASI dönüşü arasındaki drift owned fixture main içinde, terminal drift owned fake bootstrap ilk Query içinde üretilir; GTA kodu test için değiştirilmez. Non-executable target negatifinde bellek okuması reddedilir. Warm handle sayıları Debug 143→143, Release 140→140. `test_frame_target_policy.py` yedi strict/generator testini, CLI corpusuna ek iki test unknown EXE/context/argument ve legacy-null davranışlarını kapsar.

Gerçek GTA özel kopyaları `out/experiments/gta-frame-f01a00ce-Debug-v1` ve `gta-frame-f01a00ce-Release-v1`: bit-identical yeniden adlandırılmış EXE, altı önceki codec dosyasının kapsamı ve SAEX ASI artifact'ı; her dizinde yedi dosya. Assets/renderer/world initialization aşamasına geçilmedi. Orijinal kurulum değiştirilmedi. Altı Debug ve altı Release koşuda üç örnek eşleşti; 96/96 C ABI dönüşü tekrar doğrulandı. Sekiz eski CLI modu eski durakta kaldı ve `frameTargetObservation=null` verdi. Eksik artifact, yanlış cwd, Debug/Release artifact uyuşmazlığı, devre dışı ASI taraması ve original kurulum legacy pin retiyle toplam **25/25 beklenen sonuç**, **22/22 oluşturulan child confirmed exit** elde edildi.

Üç örneğin baytları `e85ad5ffff` (CALL) ve `83ec0c535657e8e55e0000b97829b700` (target); diskteki exact hash ile ayrıca karşılaştırıldı. Debug ASI/terminal event indeksleri 50–52/59–61; Release 47–48/56–57. İlk örnek CREATE_PROCESS (indeks 0). Sayılar altı koşunun sınırlarıdır, zaman/performans garantisi değildir. Bütün örnekler durdurulan owned main thread kimliğini taşıdı. Ortam snapshot SHA-256 `3725a091ed77f875b48ab336a8162328429aeebc82f2c4779f897ed54f62407e`.

Yerel ham kanıtlar: `out/verification/engine/frame-target-gta-evidence.json`, `frame-final-summary.json`, `frame-Debug-1.json`…`frame-Debug-6.json`, `frame-Release-1.json`…`frame-Release-6.json` ve `frame-legacy-*.json`/negatif raporlar. Matriste önceki 59 girdinin üzerine 14 özel kopya dosyası, bir pinned SDK arşivi ve 10 reçete/gözlem JSON girdisi eklendi; **84 SHA-256 önce/sonra ve sonuçlandırma okumalarında aynı**. Artifact kimlikleri ayrı kontrol edildi. Eski 0.1.17 kanıtları üzerine yazılmadı.

| x86 artifact | SHA-256 |
|---|---|
| Debug probe | `3aff40706b6910ca40e2b35c91074fc528a19e32512ae69c0434f4775dec9b4a` |
| Debug dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` |
| Debug map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` |
| Release probe | `5aa734b1bbe19b1a02b45d751e459a2d7df47866e2282527784d428fbf48465a` |
| Release dll | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| Release map | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Frame politika digest'i `03b425ca10f2187fdc11a635798ccca269b8716c36bfe3f06a41e0f19315794d`. SAEX DLL/map hash'leri 0.1.17 ile aynıdır; bu kesit dış observer'ı değiştirir. Build logları `build-frame-x86-Debug-second.log`, `build-frame-x86-Release.log`, `build-frame-x64-Debug.log`, `build-frame-x64-Release.log`; ayrıntılı x86 CTest logları `ctest-frame-x86-Debug.log` ve `ctest-frame-x86-Release.log` altında saklıdır. Yeniden build normal workflow ile yapılır. Runtime matris scripti yalnız yerel, dağıtılmayan `out/verification/engine/run-frame-evidence.py` dosyasındadır; başka koşu yeni çıktı/kopya adlarıyla ve exact pinler tekrar denetlenerek hazırlanır.

Başarısız ilk denemeler saklandı: generator f-string kapanışı ve 17 yerine 16 bayt uzunluk girişi üretim öncesi düzeltildi; ilk x86 Debug build `build-frame-x86-Debug-first.log`, 32 baytlık hash formatter'ın kısa dizilerde boş `0x` ürettiğini yakaladı. Genel kısa dizi formatter ve üretilmiş reçeteyi de derleyen portable testle giderildi. İkinci x86 Debug ve diğer üç standart akış tamamen geçti. Linux veya hosted CI bu sürüm için çalıştırılmadı; N1 `dependency.py verify` arşiv/source/lock doğrulaması geçti, SDK yeniden derlenmedi.

## Kod 0.1.19 bağlantısı

Yeni doğal startup modunda frame adayı ASI dönüşü ve gerçek GetStartupInfoA dönüşünde iki ayrı örnekle gözlenir. Bu rapordaki üç fazlı bootstrap deneyi değişmez; yeni iki örnek frame fonksiyonu yürütüldü anlamına gelmez. [Sözleşme ve kanıt](d1-startup-return.md).

## Kod 0.1.20 bağlantısı

Ayrı CRT startup modu I/O hazırlığının doğal dönüşünü ve initializer CALL önünü denetler. Önceki modların durakları korunur; crtStartupObservation eski modlarda null olur. C ABI 1, GNS/HTTPS ve otorite değişmez; initialized motor/N3/D2 açık kalır. [Sözleşme ve kanıt](d1-crt-startup.md).

## Kod 0.1.21 bağlantısı

Ayrı `--observe-application-entry` modu başlatıcıların doğal dönüşünü, ikinci startup çağrısını ve uygulamanın ilk komutundan önce giriş yığınını denetler. Önceki CRT komutu başlatıcı CALL önünde durur; eski kanıtlar kendi artifact kapsamındadır. Yeni izin ve güncel doğrulama [uygulama giriş sözleşmesinde](d1-application-entry.md) izlenir. C ABI 1 korunur; initialized dünya, doğal frame/N3 ve D1/D2 kapıları açık kalır.

## Kod 0.1.22 bağlantısı

Ayrı platform-startup modu uygulama prologue'unu ilerletir ve sistem ayarı çağrısından önce durur. Dört argüman, yığın/register ve pinned API hedefi doğrulanır; host ayarı değişmez, eski CLI sınırları korunur. [Sözleşme ve sonuç](d1-platform-startup.md). C ABI 1/GNS/otorite aynı; sistem ayarı uyarlaması, pencere/renderer, doğal frame ve N2/N3/D1/D2 kapıları açık kalır.

## Kod 0.1.23 bağlantısı

Tek exact platform çağrısı için ayrı süreç içi bağlam uyarlaması eklendi: sentetik FALSE, last-error/yığın/register koruma ve bağlam geri alma; API çalıştırılmaz. Önceki doğal sınır komutları korunur. [Sözleşme ve sonuç](d1-platform-suppression.md). C ABI 1/GNS/otorite aynı; instance/pencere/renderer ve N2/N3/D1/D2 kapıları açık.

## Kod 0.1.32 — Cwd query bağlantısı

Yeni query observer önceki terminal komutlarını korur ve ayrı çağrı/dönüş kapısı ekler. Paylaşılan loader/fixture test girişleri bu kapsam için güncellendi; bu belgedeki eski sonuçlar kendi artifact kapsamındadır. Windows OS dosya farkı, GTA yürütmesini güvenli reddeder. Copy/unlock/SEH removal, doğal frame ve D1/D2 kapıları açık kalır. [Sözleşme, kaynak ve güncel kanıt](d1-cwd-query.md).

## 0.1.33 — Windows profil bağı

[26200.9445 incelemesi](d1-system-profile-9445.md), bu bileşenin bağlı olduğu OS pinleri/API reçeteleri ve parent SHA-256 zincirini günceller. Bileşenin GTA durak/izin sınırı aynı kalır; önceki runtime sonuçları eski profil bağlamındadır. Yeni dosyalarla fixture ve gerçek GTA kanıtının kapsamı profil raporunda ayrıca izlenir. Eski OS pinlerine otomatik fallback yoktur; kaynak ve generated policy birlikte yeniden derlenir.

## 0.1.34 — Doğal cwd copy bağı

[Ayrı copy kesiti](d1-cwd-copy.md), query'den sonra native kontrol/CALL/dönüş ve gerçek hedef içerik kanıtını ekler. Önceki komutların terminali korunur; helper/unlock/SEH sökümü yeni izin kapsamına girmez. Source/guard, caller/SEH/kilit/cookie denetimleri ve yeni test/GTA kanıtının kapsamı ilgili rapordadır. C ABI 1, OS modül pinleri, GNS/otorite ve production kapıları aynı kalır; C++ observer yeniden derlenir.

## 0.1.35 — Cwd helper dönüş bağı

[Ayrı return kesiti](d1-cwd-return.md) copy sonrasındaki iki POP, cookie checker eşitliği ve doğal LEAVE/RET'i açar. Önceki terminal izinleri korunur; wrapper/unlock/SEH işlemleri henüz açılmaz. Kaynak stack ömrü, CALL ile değişen saved slot, hedef/caller/kilit/SEH ve hata retleri sözleşmede açıklanır. C++ trace yeniden derlenir; C ABI 1, OS pinleri, GNS/otorite ve production kapıları aynı kalır. Yeni fixture/gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## 0.1.36 — Dosya yöneticisinin tamamlanması

Bu belgenin mevcut alt komut sınırı korunur. Yeni üst düzey `--observe-file-manager-ready` aynı önceki zinciri geçtikten sonra normal unlock(7), SEH epilogue, yol suffix ve CFileMgr dönüşünü birlikte doğrular. Eski checkpoint kanıtı final kilit/FS durumuyla karıştırılmaz; final sonuç ayrı ready kaydındadır. Renderer/doğal frame ve N2/N3/D1/D2 bu kesitte hazır sayılmaz. [Sözleşme, kullanıcı komutu ve doğrulama](d1-file-manager-ready.md).

## Kod 0.1.37 — Streaming tablo kesitiyle bağlantı

[CdStream tablo sözleşmesi](d1-cd-stream-tables.md) ortak observer/CLI ve fixture zincirine ayrı bir üst mod ekler. Bu belgenin eski komut ve checkpoint sınırı korunur; yalnız `--observe-cd-stream-tables` tam manager dönüşünden sonra iki tablo döngüsünü ve disk argüman hazırlığını açar. Sonuç yeni `cdStreamTablesObservation` alanında izlenir; eski kayıtlar final durum değil önceki checkpoint snapshot'ıdır. C++ trace tüketicileri yeniden derlenir; C ABI 1, GNS, OS pinleri ve production IPC sınırı değişmez. Yeni portable/native testler ile eski mod regresyonları standart build'e dahildir; gerçek GTA ve platform bazındaki final kanıt ana raporda tutulur.

## Kod 0.1.38 — Disk sonucu ve allocation önkoşulu

[Disk hazırlığı sözleşmesi](d1-cd-stream-disk.md) önceki native zincire ayrı `--observe-cd-stream-disk` modu ekler. BOOL başarısızsa dört output kullanılmadan ret; başarılı ve kabul edilen mantıksal geometride doğal bayrak/argüman hazırlığı, 0x406BF4 allocation CALL önünde doğrulanır. Eski modların terminal ve snapshot anlamı korunur. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS, network otoritesi ve sandbox kapsamı değişmez. Gerçek allocation, fiziksel hizalama, dosya okuma ve thread/renderer hazır kanıtı bu değişiklikten çıkarılamaz. Portable hata kararı ile native/gerçek GTA kanıtının ayrımı yeni raporun kabul tablosunda izlenir.

## Kod 0.1.39 — Hizalı tamponun doğal dönüşü

[Allocation sözleşmesi](d1-cd-stream-allocation.md) ayrı `--observe-cd-stream-allocation` API/CLI ile MallocAlign → CRT → HeapAlloc → back-pointer → 0x406BF9 doğal dönüşünü ekler. Heap modu/new-handler/SBH dalı yürütmeden önce denetlenir; NULL, taşma, metadata ve payload bütünlüğü guard'ları vardır. Eski alt modların terminalleri ve snapshot anlamı korunur; yeni mod 160, eskiler 128 olay üst sınırındadır. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS ve sandbox kapsamı değişmez. İlk gerçek GTA allocation geçti; güncel toplu kanıt yeni sözleşmede izlenir. Native free, I/O/thread, renderer ve D1/D2 hazır kabul edilmez.

## Kod 0.1.40 — Kanal belleği kesiti

[Yeni sözleşme](d1-cd-stream-channels.md) `run_cd_stream_channels` / `--observe-cd-stream-channels` ile SetLastError ve LocalAlloc doğal yolunu, 5 × 48 sıfır byte ve global pointer kaydını ekler. Terminal 0x406C34, arşiv CALL önüdür. Önceki allocation/parent kayıtları kendi duraklarının snapshot anlamını korur; canlı tabloda yalnız kanal sayısı/etkin sayı DWORD çifti değişebilir. C++ trace tüketicileri yeniden derlenir; C ABI 1 ve mevcut OS pinleri aynıdır. Allocation ve yeni mod 160, daha eski modlar 128 olay sınırındadır. Native free, dosya açma/okuma, thread, renderer ve D1/D2 kapıları açıktır. Güncel test ve GTA kanıtı yeni sözleşmede tutulur.
