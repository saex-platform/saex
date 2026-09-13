# D1-N2 — Modül boşaltma ve yeniden eşleme

Tarih: 13 Eylül 2026. Kod 0.1.9 / mimari v0.16. Kapsam: bounded Windows loader gözleminin modül yaşam kayıtları. [Durum](status.md) · [Başlatma bağlamı](d1-launch-context.md) · [Loader gözlemi](d1-loader-observation.md) · [Normatif sözleşme](../architecture/native-sdk-integration.md)

## Sorumluluk ve gerekçe

0.1.8 gerçek özel GTA kopyasında UNLOAD_DLL_DEBUG_EVENT gördü ve desteklenmeyen olay olarak güvenli durdu. Artık bilinen aktif eşleme bu olayla emekli edilir. Bilinmeyen/sıfır adres veya tekrar unload ret verir. OS'nin unload bildirimi belleği okumayı gerektirmez; kayıtlı adresin yaşamı kapatılır. Her sonraki LOAD_DLL olayı, aynı adrese/dosyaya ait olsa da normal retained file ID/boyut/hash kapısından yeniden geçer.

Bu, native DLL hot unload veya SAEX hook sökme API'si değildir. Observer Windows'un yaptığı mapping/unmapping'i izler; GTA'ya LoadLibrary/FreeLibrary, injection, hook veya bellek yazımı yapmaz. İlk exception hâlâ terminaldir; bootstrap initialization ve N3 yetkisi açılmaz.

## Veri akışı ve arayüzler

LOAD_DLL → event dosyasından bounded hash/volume/file ID → retained pin karşılaştırması → LoaderMappingLedger.admit(base, eventIndex) → yeni mappingId ve tarihçe. UNLOAD_DLL → LoaderMappingLedger.retire(base, eventIndex) → aynı tarihçe kaydına unloadEventIndex → sonraki OS olayı. Exception → yalnız hâlâ aktif mappingId/base, pinli ntdll ve canlı MEM_IMAGE AllocationBase eşleşmesi üzerinden breakpoint adayı → owned child kill/exit.

[Ledger](../../include/saex/engine/loader_mapping_ledger.hpp), işletim sistemi/SDK bağımlılığı olmayan C++20 state yardımcı sınıfıdır. Fixed 64 kayıt, monoton gözlem içi kimlikler ve active/history ayrımı vardır. Kimlik gözlemden dışarı taşınan EntityRef, runtime symbol handle veya activation grant değildir. İki ayrı gözlemde mappingId=1 aynı modül demek değildir; raporun process/artifact/context kimliğiyle birlikte yorumlanır.

| İşlem | Invariant / hata |
|---|---|
| admit | Sıfır adres, aynı aktif base, geçersiz kapasite veya geriye/eşit event ret; kabulde daha önce kullanılmamış ID |
| retire | Yalnız bilinen aktif base; aynı base yeniden yüklenmişse en yeni aktif nesil emekli edilir |
| active(id, base) | Hem ID hem adres eşleşmeli; unload olmuş ID aynı base tekrar kullanılsa bile false |
| Başarısız geçiş | History/count/active/last-event değişmez; observer bu hatada terminal stop yapar |
| Budget | 64 yaşam boyu LOAD kaydı ve mevcut toplam hash-byte/event limitleri; unload kapasite veya byte iadesi yapmaz |

Ledger raw pointer kullanmaz; base sayısal bir gözlem anahtarıdır. load/unload event indeksleri strict artar, ardışık olmak zorunda değildir; thread vb. ara olaylar bulunabilir. Sayaç wrap kabul edilmez. Standalone metadata sınıfı per-observation load kapasitesini uygular; 128 debug-event/16 thread/256 MiB/5 saniye ve hash/pin sınırları Windows observer'ın sorumluluğunda kalır.

## Çıktı ve hata davranışı

Mevcut JSON schemaVersion=1 ve CLI exit 1/2/3 korunur. Additive alanlar:

- Her modules kaydında `mappingId`, `unloadEventIndex`, `activeAtObservationEnd`.
- Üst düzeyde `activeModuleCount`, `unloadCount`, `lastEventAddress`.
- lastEventAddress, LOAD/UNLOAD için o OS olayındaki base, exception için exception adresidir; diğer olaylarda sıfırdır. OS adresi runtime çağrı yetkisi veya symbol kanıtı değildir.

modules dizisi cumulative yükleme girişimleri tarihçesidir; reddedilmiş son dosya da teşhis için bulunabilir. `admitted` dosyanın pinle eşleşmiş olduğunu gösteren tarihsel alan olarak kalır. Ledger'a kabul edilmeyen girişin mappingId'si sıfırdır; unloadEventIndex sıfır ve activeAtObservationEnd false olur. Eski bir admitted satırı güncel mapping sayılmaz. Aktif alanlar **terminal cleanup başlamadan önceki son gözlemi** anlatır; çıktı yazıldığında owned child zaten kapatılmıştır.

`loader_mapping_invalid_limit/invalid_base/event_order/base_active/limit/not_active` state retlerini ayırır. History ile ledger eşleşmezse `loader_mapping_history_mismatch` terminaldir. Başarısız unload sonrası bilinmeyen adres okunmaz veya devam ettirilmez. Bilinen unload olayının devamı yalnız mevcut first-exception-only gözlem penceresindedir. EXIT_THREAD ve diğer desteklenmeyen olaylar bu kesitte sessizce serbest bırakılmaz.

## Genişleme, migration ve kabul

Kaldırılan özellik yoktur; eski üç CLI modu ve 22 modüllü compiled recipe aynıdır. Bir önceden desteklenmeyen olay için koşullu devam eklendiğinden ADR-44 ve AC-90/R-01b kanıtı yenilenir. Bootstrap C ABI 1, C# sözleşmeleri, core entity kimliği, GNS/HTTPS kararı ve SDK kaynak lock'u değişmez. Observer static C++ tüketicileri yeni struct layout için yeniden derlenir. Yeni CLI alanlarını okuyamayan analiz araçları eski modules/admitted alanından aktif modül sonucu çıkarmamalıdır.

Sıradaki yetenekler, proxy initialization/SAEX bootstrap ve unpack/symbol/ABI kanıtıdır. Thread lifecycle veya başka Windows olayı, dynamic symbol/module referansları ve hot unload ayrı kanıt ister. History kapasitesini artırmak veya unload başına budget iade etmek bu sözleşmeyi değiştirir.

AC-90 alt kabulü:

1. Load → unload → aynı base'e yeni load: eski ID pasif, yeni ID aktif; önceki kayıt ve unload zamanı korunur.
2. Farklı canlı base'ler bağımsızdır; duplicate load, unknown/duplicate unload ve stale ID sorgusu geçerli eşlemeyi bozmaz.
3. Event sırası/wrap, sıfır adres, invalid limit ve 64 kez aynı base'i tekrar yükleme: sınırlar negatif corpus ile doğrulanır; emeklilik budget iadesi yapmaz.
4. Windows fixture sonuçlarının mapping history/count/active/unload invariant'ları ve DLL/TLS/main canary sınırı birlikte sınanır. Hata girdisi child yaratmayan CLI'de yeni sayaç/adresler sıfırdır.
5. Ayrı gerçek GTA deneyi: bilinen unload varsa hangi kayıt emekli oldu, ardından ne oldu ve child çıkışı doğrulandı mı raporlanır. Gerçek reload/remap gözlenmezse salt metadata testi bu gerçek davranışı kanıtlamış sayılmaz.

## Doğrulama kaydı

Yeni native.loader_mappings suite'i on isimli portable negatif/pozitif senaryo içerir; mevcut x86 OS loader suite'i ve Python CLI corpus'u genişletilir. Standart build oyun başlatmaz. Gerçek OS unload kolu kendi deney çıktısında ayrıca kaydedilir; fixture kaynakları ilk loader breakpoint'ini geçip GTA initialization izni üretmez. Gerçek yeniden eşleme, canary ve metadata corpus sonuçları ayrı raporlanır. Tam build/deney sonuçları bu belgenin sonuna yazılır.

## Birincil kaynaklar

Windows unload olayı yalnız base adresini verir; [Microsoft UNLOAD_DLL_DEBUG_INFO](https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-unload_dll_debug_info). Yeni yükleme olayının dosya handle'ı ayrıca incelenip kapatılır; [LOAD_DLL_DEBUG_INFO](https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-load_dll_debug_info). Bunlar SAEX test veya GTA initialize kanıtı değildir.

## Son doğrulama — 13 Eylül 2026

Üç standart build ilk koşuda geçti: Windows x86 Debug/Release için her birinde 8 native suite, 57 managed test ve 49 Python testi; Windows x64 Debug için 6 native suite, 57 managed ve 30 Python testi. Yeni native.loader_mappings suite'inin on senaryosu her üç koşuda geçti. Var olan bootstrap artifact audit, 100 oyun dışı DLL lifecycle çevrimi, held DLL/TLS/main canary'leri, 12 warm loader çevrimi ve context testleri de bu koşulara dahildir. X64 Release/Linux çalıştırılmadı; N1 kaynak/recipe değişmediği için SDK build tekrarlanmadı.

Build log'ları out/verification/engine/build-lifecycle-{x86-Debug,x86-Release,x64-Debug}.log; native ayrıntılar native-lifecycle-<architecture>-<configuration>.log. Bu kaynak/test kesitinde başarısız build/test olmadı; önceki sürümlerin başarısız kayıtları kendi raporlarında korunur.

### Gerçek GTA gözlemi

Önceki bit eşitliğinde özel kopya kullanıldı; yeniden oyun dosyası kopyalama/kurulum değiştirme yapılmadı. Windows 10.0.26200, workspace cwd, 60 environment entry/4132 UTF-16 code unit; environment digest `3725a091ed77f875b48ab336a8162328429aeebc82f2c4779f897ed54f62407e`. Her iki configuration için **önceden belirlenen altışar koşu** yapıldı; sonuç seçerek tekrarlama yapılmadı. Ayrı iki original-context ret kontrolü eklendi. Tüm 14 çıktı ve dosya hash'leri out/verification/engine/loader-lifecycle-gta-evidence.json'da kayıtlıdır.

| Bağlam | Koşu / gözlem | Sonuç |
|---|---|---|
| x86 Debug, private workspace | 6/6 koşuda 28 olay/22 lifetime mapping, 1 unload, 21 aktif; beşinde ole32.dll, birinde combase.dll emekli | 6/6 breakpoint adayı/exit 3 ve doğrulanmış child exit |
| x86 Release, private workspace | 5 koşuda 28 olay/22 mapping/1 ole32 unload; birinde 26 olay/21 mapping/unload yok; sonunda 21 aktif | 6/6 breakpoint adayı/exit 3 ve doğrulanmış child exit |
| x86 Release, original explicit context | 6 olay/5 yükleme girişimi; 4 aktif, AcLayers pin dışı | Beklenen loader_module_not_pinned/exit 1; child exit doğrulandı |
| x86 Release, original legacy 3-pin | 5 olay/4 girişim; 3 aktif, apphelp pin dışı | Beklenen loader_module_not_pinned/exit 1; child exit doğrulandı |

Böylece önceki UNLOAD kolu 11 gerçek koşuda görülüp bilinen eşleme emekliliğiyle devam etti. Aktif base tekilliği, ID doğrulaması, load/unload zaman sırası ve aktif/emekli/tarihçe sayaç ilişkileri 14 raporda ayrıca kontrol edildi. Emekli kaydın history'den silinmediği ve aktif sayıdan düşürüldüğü görüldü. Aynı base'e unload **sonrasında** yeniden eşleme bu koşul setinde gözlenmedi; bu davranışın kanıtı on testli metadata corpus'uyla sınırlıdır, gerçek GTA remap geçti denmez.

Cumulative hash yükü unload olmayan koşuda 18.553.632 byte, ek ole32 mapping olanlarda 19.953.960 byte, ek combase mapping olan koşuda 21.277.224 byte'tır. Dosya pinleri aynı kalsa da her mapping okumaya tekrar ücretlenir; history'den emeklilik bütçe iadesi yapmaz. Bunlar kapasite/performans benchmark'ı değildir.

Son probe SHA-256:

- Debug: `5cbe5a823605f05cd9e8d8fd9e7d1279f254c3a0c634b401d44e13f99ec364d8`
- Release: `19ea5490f04b89d6a79a88b23e86bc0fe4140b9a76277f645cebf3cac16464e1`

Engine/profile/policy kimlikleri önceki sürümle aynıdır. Bootstrap DLL hash'leri Debug `eb0cdf4c074bee945099103cf661ba3269a8e1b6f1a35f24d670d994ec41f353`, Release `cabf7e8225c4d3c60d7fbc655bee631042376bdcdd550a3140f624dfabc8e8ea` olarak değişmedi. Original 13 native girdi ve private 3 dosyanın önce/sonra hash'leri eşit; 14/14 owned child çıkışı doğrulandı. Registry, environment, oyun dosyası, remote/push/deployment değişimi yapılmadı.

Bu sonuç R-01b/AC-90'ın mapping-lifecycle alt kanıtıdır. Başlangıç breakpoint adayını geçme, proxy/TLS/DllMain/game initialization, gerçek SAEX bootstrap yükleme ve symbol/ABI kanıtları hâlâ yoktur. Orijinal kurulum AcLayers'da güvenli ret alır. N3/D1/D2 ve multiplayer hazır ilan edilmez.

0.1.9 son statik belge kontrolü: 76 Markdown, 1013 yerel bağlantı, 6 JSON örneği ve sıfır hata. Bu sonuç kaynak/belge eşlemesi ve statik yapı kanıtıdır; bütün mimarinin anlamsal kusursuzluğu veya GTA initialization başarısı değildir.

## Kod 0.1.11 — Aynı ledger, ayrı yürütme kapsamı

[Entry boundary](d1-entry-boundary.md) mapping ledger admission/retire veya kimlik ömrünü değiştirmez; aynı retained file ID/hash ve kümülatif kotaları kullanır. Ek entry flag/byte alanları yeni loader scope'unda geçerlidir. Önceki 12 private GTA mapping/unload sonucu tarihsel first-exception kapsamındadır; yeni initializer/hardware hit sonucuna otomatik dönüştürülmez.

## Kod 0.1.11 — Entry supplement bağı

Entry scope’undaki bir imm32 ek pini ledger kimlik/retire/hash/budget semantiğini değiştirmez. Eski history kanıtı yeni initializer/entry kanıtına dönüşmez. [Ayrıntı](d1-entry-boundary.md).

### Hosted corpus tamamlaması

İkinci hosted turda bütün native suite'ler geçti; ortak Python loader CLI corpus'undaki cwd string eşitliği kısa/uzun Windows adı nedeniyle hata verdi. Test artık pathlib.samefile ile aynı dizin kimliğini doğrular. Engine unknown-fingerprint reddi, environment redaction, childCreated=false, entry/loader policy ve lifecycle sınırları aynen kalır; önceki GTA artifact sonuçları yeni test kanıtı sayılmaz.

## Kod 0.1.12 — Proxy dönüş sınırı

[Proxy dönüş modu](d1-proxy-return.md), ilk hitte doğrulanan exact file ID/hash eşlemesinin mappingId/base çiftini tutar. Bu mapping aradaki UNLOAD ile emekliye ayrılırsa proxy_mapping_retired terminal olur; tekrar aynı adrese yükleme eski yetkiyi taşımaz. İkinci hitte aktif eşleme yeniden kontrol edilir. Kümülatif event/module/byte/thread kotaları ve eski mapping komutları korunur.

## Kod 0.1.13 — Startup çağrı sınırı

[Üçüncü durak](d1-startup-call.md) aynı proxy mappingId/base kimliğini kullanır; aradaki unload terminal kalır. Yeni runtime hedefi eski mapping kanıtından bağımsız keyfî pointer olarak takip edilmez. 128 event/64 lifetime module/16 thread/256 MiB ve 5 saniye bütçeler korunur; IAT write watch metadata’sı modül yetkisi veya remap izni vermez.
