# Kaynak ve belge değişiklik kaydı

## 14 Eylül 2026 — GitHub 0.1.40 ve teslim akışı

[PR #11](https://github.com/saex-platform/saex/pull/11) tesliminde tamamlanan tüm kaynaklar korundu; eski publish dalıyla güncel main arasındaki fark, OS/policy exact byte'ları ve yeni davranışları koruyarak birleştirildi. Önceki CI runner/VS/Python düzeltmeleri geri taşındı. AGENTS ve workflow tamamlanan kesitlerin aynı teslimatta PR/yedi kontrol/squash ile yayımlanmasını ve temiz main'e dönüşü tanımlar. Ham kaynak/Git yedekleri alındı; [yayın raporu](github-publication.md) gerçek GTA ve yeni kaynak kontrollerini ayrı tutar. Ürün davranışı/ABI/otorite izni genişlemez.

## Kod 0.1.40 / mimari v0.47 — 14 Eylül 2026

Streaming kanal belleği için C++ API/CLI, parent-hash policy ve generated header eklendi. Dokuz evrede SetLastError/LocalAlloc gerçek thunk/implementation/return ilişkisi, 240 byte sıfırlama, global yayın ve parent allocation korunumu denetlenir. Üç native fixture, portable mutasyon corpus'u, sekiz policy testi ve CLI gate testi eklendi. Run-SAEX 13 kontrol/20 hash girdisiyle güncellendi; yeni terminal 0x406C34. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS ve SDK davranışı aynı. Kaldırılan özellik, veri migration'ı veya yeni runtime bağımlılığı yok. ADR-75, AC-90 alt kesiti ve component sahipleri aynı değişiklikte güncellendi. Native free/I/O/thread kapıları açık. [Ayrıntı ve sonuçlar](d1-cd-stream-channels.md).

Nihai sonuç: x86 Debug/Release 58 native / 79 managed / 331 Python; yeni 2585 portable kontrol, 18 native senaryo, bir canary ve 12 warm; GTA 27/27, 14 pozitif, 22/22 child çıkışı, 141/141 girdi korunumu; iki PowerShell 5.1 raporu 13/13. Final 354 kaynak/config hash korunumu doğrulandı.

## Kod 0.1.39 / mimari v0.46 — 14 Eylül 2026

Gerçek hizalı streaming allocation için C++ model/API/CLI, strict parent-hash policy, CRT normal yol code reçeteleri, branch/SEH/ABI/metadata/hash denetimi, dört native fixture ve portable/negatif testler eklendi. Yeni modun 160 olay sınırı eski modların 128 sınırını değiştirmez. Run-SAEX allocation policy doğrulaması ve 12 sonuç kontrolüyle güncellendi. CLI dispatch'inde Debug stack taşması ayrı çağrı frame'leriyle giderildi; boş JSON raporu açık exit code verir. İlk scope cleanup taslağı executable'daki 0x82421F ile düzeltildi. C++ tüketicileri yeniden derlenir; C ABI 1, GNS/HTTPS ve OS pinleri aynı. Kaldırılan özellik, yeni runtime bağımlılığı veya veri migration'ı yok. Native free/streaming hazır iddiası yok. ADR-74, AC-90 alt kesiti ve component sahipleri birlikte güncellendi. [Kanıt ve açık işler](d1-cd-stream-allocation.md).

Nihai sonuç: x86 Debug/Release 56 native/79 managed/322 Python; GTA 27/27, 14 allocation, 22/22 child çıkışı ve 140/140 girdi korunumu. PowerShell 5.1 iki rapor 12/12; final 346 kaynak/config hash'i aynı. CMake sürüm metadata'sı eşitlendi; Debug 216 artifact hash'i aynı kaldı. İlk başarısız denemeler son başarılı matristen ayrı tutulur.

## Kod 0.1.38 / mimari v0.45 — 14 Eylül 2026

Disk sorgusu ve allocation hazırlığı için ayrı C++ API/CLI, parent-hash policy/generator, BOOL/geometri kararı, dört native durak, guards ve negatif testler eklendi. Gerçek GTA ilk koşuda 0x406BF4'e ulaştı. API failure output'u kullanılmaz; mantıksal/fiziksel hizalama ayrılır. Run-SAEX en ileri moda taşındı, iki kontrol ve policy/probe digest eşleşmesi eklendi. C++ tüketicileri yeniden derlenir; C ABI 1, OS pinleri ve GNS/HTTPS aynı; kaldırma, dependency, veri migration'ı yok. İlk public method declaration ve generator import hataları düzeltildi; ilk derleme log'u saklandı. ADR-73, AC-90, component map ve sahip belgeler güncellendi. Final: x86 Debug/Release tam 54/79/313; GTA 27/27, 14 pozitif, 22/22 child ve 139/139 girdi; iki runner 11/11, x64 Debug 296 portable kontrol; 338 kaynak/config aynı. [Sözleşme ve nihai sonuç](d1-cd-stream-disk.md).

## Kod 0.1.37 / mimari v0.44 — 14 Eylül 2026

CdStreamInit tablo hazırlığı için ayrı C++ API/CLI, parent-bound policy/generator, 2.192-byte before/after kontrolü, iki checkpoint ve negatif testler eklendi. Run-SAEX varsayılanı en ileri doğrulanmış terminale taşındı; eski CLI sınırları korundu. Ghidra/Java/PyGhidra/bridge yalnız yerel araştırma bağımlılığı olarak kuruldu; version/source lock, bounded export/verify aracı ve offline negatif testler eklendi. 15 fonksiyon analizinde bir eksik root ve patched Open/Read gövdeleri açık bulundu. ReAgent agent/model pipeline'ı çalıştırılmadı. C++ yeniden derlenir; C ABI 1/GNS/OS pinleri aynı, kaldırma veya veri migration'ı yok. İlk fixture ad çakışması ve yanlış metin değiştirme kaynaklı iki derleme hatası düzeltildi; log'lar saklandı. ADR-72, AC-90 ve sahip belgeler birlikte güncellendi. Sonuç: GTA 60/60, 20 pozitif, 55/55 child ve 178/178 girdi; Release tam 52/79/304. Debug ilk 51/52 + bootstrap testinin heap onarımı 1/1 + kalan 79/304 kontrollerle birleşik doğrulandı. 330 kaynak/config hash'i aynı kaldı. İki PowerShell 5.1 raporu 9/9, x64 Debug portable suite geçti. [Native kanıt](d1-cd-stream-tables.md), [araç kararı](../references/ghidra-bridge.md).

## Kod 0.1.36 / mimari v0.43 — 14 Eylül 2026

CFileMgr::Initialise doğal dönüşüne kadar wrapper cleanup, unlock(7), SEH epilogue ve suffix akışı tek explicit izin/CLI altında tamamlandı. Dokuz checkpoint, bağımlı policy, API/stack/kilit/FS/caller/buffer denetimleri ve negatif testler eklendi. Yeni `fileManagerReadyObservation` nihai sonucu taşır; parent kayıtları checkpoint snapshot'ıdır. `tools/Run-SAEX.ps1` yedi dosyalı özel kopya ve çevrimdışı HTML/JSON sonucu üretir; orijinal oyun girdilerini değiştirmez. İlk iki derleyici hatası düzeltildi ve kayıtları korundu. ADR-71/AC-90, eşleme ve sahip belgeler güncellendi. C++ yeniden derlenir; C ABI 1/GNS/OS pinleri aynı, dependency/kaldırma/migration yok. x86 Debug/Release 50 native/79 managed; 285+2 ve 287 Python, x64 Debug portable suite geçti. GTA 58/58 matris, 20 tam dönüş ve 53/53 child çıkışı; Run-SAEX iki yapılandırmada 8/8 kontrol verdi. 319 source/config hash aynı kaldı. [Nihai kanıt](d1-file-manager-ready.md). D1/D2 açık.

## Kod 0.1.35 / mimari v0.42 — 14 Eylül 2026

Ayrı `run_cwd_return` / `--observe-cwd-return`, altı durak, native cookie checker eşitlik yolu ve helper LEAVE/RET gözlemi eklendi. Parent copy sonuçları korunur; `helperReturnAllowed/Verified` ve yeni JSON kaydı ayrıdır. Kaynak retired durumu, eski saved-EBX slotunun CALL ile değişmesi ve caller argümanları açık denetlenir. Strict parent-bound policy/generator, ABI/flags/range negatifleri, native canary/owner/warm ve CLI retleri eklendi. İlk CLI testindeki relative cwd için 2 beklentisi, mevcut `launch_directory_input`/1 sözleşmesine göre düzeltildi; üretim davranışı değişmedi. ADR-70/AC-90, eşleme ve sahip belgeler güncellendi. C++ yeniden derlenir; C ABI 1/GNS/otorite/OS pinleri aynı, kaldırma/dependency/migration yok. Dört Windows akışı ve GTA 56/56 matris geçti; 20 pozitif helper dönüşü, 51/51 child çıkışı doğrulandı. 309 kaynak/config hash'i aynı kaldı. Wrapper/unlock/SEH açık. [Kanıt ve kapsam](d1-cwd-return.md).

## Kod 0.1.34 / mimari v0.41 — 14 Eylül 2026

İlk tam testte saptanan gövde kontrolü marker sızıntısı, kontrolün owned child/marker runner'ına taşınmasıyla düzeltildi. İki tekrar temizliği ve ortak fixture için CTest resource lock eklendi; başarısız ilk koşu copy raporunda korunur.

Ayrı `--observe-cwd-copy` ve `run_cwd_copy` izni, beş checkpoint, exact native strcpy gövdesi ve hedef/source/stack/SEH/kilit/cookie korunumları eklendi. Önceki query durağı korunur; copy dönüşünde argümanlar hâlâ stack üzerindedir. Strict parent-bound policy/generator, byte/ABI/guard negatifleri, native fixture/gövde kontrolü ve CLI retleri eklendi. ADR-69/AC-90, sahip belgeler/component map birlikte güncellendi. C++ yeniden derlenir; C ABI 1, OS pinleri, GNS/otorite aynı, yeni dependency/kaldırma/kalıcı migration yoktur. Dört Windows akışı ve gerçek GTA matrisi geçti: 20 pozitif kopya, 54/54 beklenen sonuç, 49/49 child çıkışı. 301 kaynak/config hash'i aynı kaldı. Argüman/helper/unlock/SEH dönüşü açık. [Kaynak ve doğrulama](d1-cwd-copy.md).

## Kod 0.1.33 / mimari v0.40 — 14 Eylül 2026

İlk GTA Debug kabul yolunda saptanan CLI stack overflow düzeltildi: büyük trace/hata kayıtları heap üzerinde yönetilir; stack rezervi ve gözlem izinleri büyütülmedi. Windows 26200.9445 için 26 sistem pini, API RVA/operand/HIGHLOW verileri ve 23 policy/header digest zinciri yenilendi. Modül kümesi, GTA native stage/terminal, C ABI 1 ve GNS/otorite aynı kaldı. Eski profil bu derlemede aktif değildir; otomatik fallback yoktur, yeniden derleme gerekir. Eski kaynak snapshot/hash ve runtime kanıtı korunur. Ancestor drift ve karma OS modülü ret testleri eklendi; ADR-68/AC-90 ve sahip belgeleri güncellendi. [Statik inceleme ve runtime sonuçları](d1-system-profile-9445.md).

## Kod 0.1.32 / mimari v0.39 — 14 Eylül 2026

`--observe-cwd-query`, strict JSON/generator ve yedi duraklı doğal directory API gözlemi eklendi. 260-byte local API buffer ile 128-byte CFileMgr hedefi ayrıdır; NUL/uzunluk/dizin eşleşmesi ve 126-byte suffix sınırı denetlenir. Stack/SEH/kilit/cookie/çevre korunumları ve hata nedenleri kaydedilir; eski acquire komutunun terminali korunur. C++ trace yeniden derlenir; C ABI 1/GNS/otorite değişmez, kaldırma/migration yoktur. ADR-67/AC-90 alt kapsamı ve tüm sahip belgeler güncellenir. Mevcut Windows dosyalarının eski pinlerden farklı olduğu saptandı; eski statik export'lar WinSxS hash eşleşmesiyle incelendi, OS pinleri gevşetilmedi. ReAgent isteğe bağlı araştırma aracı olarak [değerlendirildi](../references/reagent.md), kurulmadı/çalıştırılmadı. Build ve doğrulama ayrıntıları [query raporundadır](d1-cwd-query.md).


## 14 Eylül 2026 — 0.1.31 CI fixture düzeltmesi

İlk hosted x86 Debug/Release koşusu, yerel Windows export adreslerini devralan 13 fixture grubunda reddedildi. Sistem export/HIGHLOW reçetelerini tutulan host PE dosyalarından okuyan test yardımcısı, beş canlı prefix karşılaştırması ve on bozuk metadata reddi eklendi; ilgili fixture alanları buna bağlandı. İkinci hosted HIGHLOW reddi sonrası exact prefix reçeteleri host modül tabanına normalleştirildi; normalleştirme eşitliği ve hatalı maskenin reddi eklendi. Üretim JSON/generated policy, ABI/otorite ve GTA destek kapsamı aynı kaldı. Sahip belgeleri/component map birlikte güncellendi; ayrıntılar [startup-return raporunda](d1-startup-return.md#14-eylül-2026--windows-fixture-taşınabilirliği), kontrol durumu [yayın raporunda](github-publication.md).

## 14 Eylül 2026 — 0.1.14–0.1.31 kaynaklarının GitHub yayını

Kullanıcının tüm birikmiş kaynakları yayımlama isteğiyle 0.1.31 / mimari v0.38 anlık görüntüsü hazırlandı. Kaynak/test/policy/generator, bütün sahip belgeleri ve güncel marka varlıkları tek PR kapsamındadır. Kaynak ZIP/manifest ve Git bundle yedekleri alındı; temiz checkout ve mevcut yedi hosted kontrol yayın kapısıdır. Bu kayıt yeni ürün davranışı, ABI/otorite değişimi veya migration eklemez. [Yayın raporu](github-publication.md) geçmiş gerçek GTA ve yeni kaynak CI kanıtını ayrı tutar.

## Kod 0.1.31 / mimari v0.38 — 13 Eylül 2026

CRT mevcut kilidi alma API/CLI, Windows target/nesne ve beş checkpoint sözleşmesi eklendi. Doğal API/selector dönüşü, thread sahipliği, slot/stack/SEH korunumu denetlenir; lazy/cwd/unlock açılmaz. İlk GTA keşfi heap varsayımında güvenli ret verdi; aynı GTA image için kontrollü destek ve fixture eklendi. C++ trace yeniden derlenir; C ABI 1/GNS/otorite aynı, dependency/kaldırma/kalıcı migration yoktur. ADR-66/AC-90, mimari, durum, roadmap ve CI güncellendi. [Doğrulama](d1-cwd-acquire.md) tamamlandı: x86 Debug/Release 41 native/79 managed/254 Python; x64 Debug/Release 19/79/184. Yeni 141 portable kontrol/17 senaryo/iki canary/12 warm, gerçek GTA 12/12 ve 40/40 matris, 37/37 child çıkışı ve 280/280 girdi korunumu geçti. İlk heap-only keşif reddi ve image desteği düzeltmesi raporda korunur. Linux/hosted/N1 tekrar koşulmadı.

## Kod 0.1.30 / mimari v0.37 — 13 Eylül 2026

CRT lock(7) selector API/CLI ve strict policy eklendi; iki doğal durakta register/CMP flags, slot, stack ve SEH korunumu doğrulanır. Boş/dolu kayıt desteklenir; JNE ve iki kilit yolu kapalıdır. C++ trace yeniden derlenir; C ABI 1/GNS/otorite aynı, dependency/kaldırma/kalıcı migration yoktur. ADR-65/AC-90, mimari, durum, roadmap ve CI birlikte güncellendi. [Doğrulama](d1-cwd-lock.md) tamamlandı: x86 Debug/Release 39 native/79 managed/247 Python; x64 Debug/Release 18/79/179. Yeni 332 portable kontrol/13 senaryo/iki canary/12 warm, gerçek GTA 12/12 ve 39/39 matris, 36/36 child çıkışı ve 265/265 girdi korunumu geçti. İlk fixture C2415 derleme reddi ve düzeltmesi raporda korunur. Linux/hosted/N1 tekrar koşulmadı.

## Kod 0.1.29 / mimari v0.36 — 13 Eylül 2026

Cwd SEH API/CLI, strict policy zinciri, üç doğal durak, 59-byte prologue/12-byte scope, NT_TIB ve stack kayıt readback eklendi. Kilit/OS/copy kapalı, önceki manager terminali korunur. C++ trace yeniden derlenir; C ABI 1/GNS/otorite aynı, dependency/kaldırma/kalıcı migration yoktur. ADR-64/AC-90, mimari, durum, roadmap ve CI birlikte güncellendi. [Doğrulama](d1-cwd-seh.md) tamamlandı: x86 Debug/Release 37 native/79 managed/240 Python; x64 Debug/Release 17/79/174. Yeni 210 portable kontrol/11 senaryo/canary/12 warm, gerçek GTA 12/12 ve 38/38 matris, 35/35 child çıkışı ve 250/250 girdi korunumu geçti. İlk fixture C4733 derleme reddi ve portable PF beklentisi hatası raporda korunur. Linux/hosted/N1 tekrar koşulmadı.

## Kod 0.1.28 / mimari v0.35 — 13 Eylül 2026

File manager entry API/CLI, strict policy zinciri, iki doğal durak, buffer/maxlen ABI ve 136-byte readback eklendi. Cwd CALL kapalı, önceki prelude terminali korunur. Tam 51-byte gövde/suffix örneklemesi ve CRT araştırması, gelecekteki 126-byte metin/suffix sınırı belgelendi. C++ trace yeniden derlenir; C ABI 1/GNS/otorite aynı, dependency/kaldırma/kalıcı migration yoktur. ADR-63/AC-90, mimari, durum, yol haritası ve CI birlikte güncellendi. [Doğrulama](d1-file-manager-entry.md) tamamlandı: x86 Debug/Release 35 native/79 managed/233 Python; x64 Debug/Release 16/79/169. Yeni 99 portable kontrol/12 senaryo/canary/12 warm, gerçek GTA 12/12 ve 37/37 matris, 34/34 child çıkışı ve 235/235 girdi korunumu geçti. İlk read-only fixture varsayımı düzeltildi ve başarısız koşu korundu. Linux/hosted/N1 tekrar koşulmadı.

## Kod 0.1.27 / mimari v0.34 — 13 Eylül 2026

Game prelude API/CLI ve strict policy zinciri, iki doğal helper çağrısı, beş durak ve üç veri yazımının readback denetimi eklendi. Eski routing terminali korundu; C++ trace/API yeniden derlenir, C ABI 1/GNS/otorite aynı. Kaldırılan özellik/dependency/kalıcı migration yoktur. Mimari, durum, ADR-62, AC-90, roadmap ve Windows/portable CI eşlemeleri birlikte güncellendi. [Doğrulama](d1-game-prelude.md) tamamlandı: x86 Debug/Release 33 native/79 managed/226 Python; x64 Debug/Release 15/79/164. Yeni 215 portable kontrol/11 senaryo/canary/12 warm, gerçek GTA 12/12 ve 36/36 matris, 33/33 child çıkışı ve 220/220 girdi korunumu geçti. İlk fixture RET kodlama uyuşmazlığı düzeltildi ve raporlandı. Linux/hosted/N1 tekrar koşulmadı.

## Kod 0.1.26 / mimari v0.33 — 13 Eylül 2026

Application routing API/CLI, strict parent policy, executable detour ve iki tablo için relocated operand doğrulaması, dört doğal durak ve negatif testler eklendi. İlk oyun initializer CALL önünde durulur. Önceki event-dispatch terminali korunur; üst modun JSON CALL izni gerçek kapsamı gösterir. C++ observer yeniden derlenir; C ABI 1/GNS/otorite aynı, kaldırılan özellik/dependency/migration yoktur. Mimari, durum, ADR-61, AC-90, roadmap ve build eşlemeleri birlikte güncellendi. [Doğrulama](d1-application-routing.md) tamamlandı: x86 Debug/Release 31 native/79 managed/219 Python; x64 Debug/Release 14/79/159. Yeni 167 portable kontrol/11 senaryo/canary/12 warm, gerçek GTA 12/12 ve 35/35 matris, 32/32 child çıkışı ve 205/205 girdi korunumu geçti. İlk fixture derleme hataları ve düzeltmeleri raporda tutuldu. Linux/hosted/N1 tekrar koşulmadı.

## Kod 0.1.25 / mimari v0.32 — 13 Eylül 2026

Olay dağıtıcısı API/CLI, strict parent policy, üç doğal durak, portable frame doğrulayıcısı ve negatif testler eklendi. Uygulama olay işleyicisi CALL önünde durulur; 21-byte işleyici örneği yürütme yetkisi değildir. Mevcut instance terminali ve C ABI 1/GNS/otorite korundu; C++ observer yeniden derlenir, kaldırılan özellik/migration yoktur. Mimari, durum, ADR-60, AC-90, roadmap ve build/CI eşlemeleri birlikte güncellendi. [Doğrulama](d1-event-dispatch.md) tamamlandı: x86 Debug/Release 29 native/79 managed/212 Python; x64 Debug/Release 13/79/154. Yeni 81 portable kontrol/11 senaryo/canary/12 warm, gerçek GTA 12/12 ve 34/34 matris, 31/31 child çıkışı ve 190/190 girdi korunumu geçti. Linux/hosted/N1 tekrar koşulmadı.

## Kod 0.1.24 / mimari v0.31 — 13 Eylül 2026

Named-event instance API/CLI, strict parent policy, tam helper/argüman/aktif sistem hedefi denetimi, event kimlik karşılaştırması ve doğal dönüş kesiti eklendi. NULL handle ve mevcut event fail-closed; pencere kolu DR3 ile korunur. Yeni devam modu eski suppression terminalini değiştirmez; gerçek API sonrası restore yapılmaz. Portable/fixture/policy/CLI negatif testleri build/CI akışına bağlandı; mimari sözleşme, ADR-59, AC-90 alt senaryo, durum ve roadmap birlikte güncellendi. Kaldırılan özellik/migration yok; C++ observer yeniden derlenir, C ABI 1/GNS/otorite aynı. [Kanıt ve ilk hata düzeltmeleri](d1-instance-startup.md). Dört Windows akışı geçti: x86 27/79/205, x64 12/79/149 native/managed/Python. 129 portable kontrol/15 senaryo/canary/12 warm, gerçek GTA 12/12 ve 33/33 matris, 30/30 child çıkışı ve 175/175 girdi korunumu doğrulandı. Linux/hosted/N1 tekrar çalıştırılmadı.

## Kod 0.1.23 / mimari v0.30 — 13 Eylül 2026

Ayrı platform-suppression API/CLI ve strict policy, context transaction/rollback, exact GetLastError export/FS okuması ve negatif testleri eklendi. EIP/ESP/EAX sentetik dönüşü yalnız doğrulanmış owned child CALL'unda açılır; kod/IAT/yığın belleği ve orijinal oyun dosyaları değişmez. Eski mod sınırları ve C ABI 1 korunur. [Kanıt ve sınır](d1-platform-suppression.md). Dört Windows akışı geçti (x86 25/79/196; x64 11/79/142); 44 portable kontrol/20 senaryo/canary/12 warm, gerçek GTA 12/12 ve 30/30 matris, 27/27 child çıkışı ve 160/160 girdi korunumu. İlk derleme/Debug stack/EFLAGS/DR6 hataları ve düzeltmeleri raporda korunur. Linux/hosted/N1 SDK tekrar koşulmadı. Kaldırılan özellik yok; observer yeniden derlenir.

## Kod 0.1.22 / mimari v0.29 — 13 Eylül 2026

İlk uygulama prologue'undan host sistem ayarı CALL önüne ayrı API/CLI, strict policy ve pinned user32 export reçetesi eklendi. Negatif yığın/register/argüman/IAT ve pozitif canary kontrolü, portable/policy/CLI testleri build akışına bağlandı. İlk çağrının sistem genelindeki foreground ayarını değiştirme isteği kaynak incelemesiyle belirlendi; çağrı çalıştırılmaz. C ABI/GNS/otorite ve eski duraklar korunur; kaldırılan özellik/kalıcı migration yok, C++ observer yeniden derlenir. [Sonuç ve sınır](d1-platform-startup.md). Dört Windows akışı geçti: x86 23/79/188, x64 10/79/136 native/managed/Python. 20 portable kontrol, 17 senaryo/canary/12 warm; gerçek GTA 12/12 ve 29/29 matris, 26/26 child çıkışı, 145/145 hash korunumu doğrulandı. Body yürütme sonucu yeni CLI'da false/true/null ile kanıt düzeyini ayırır; eski CLI boolean davranışı korunur. Linux/hosted/N1 SDK tekrar koşulmadı.

## Kod 0.1.21 / mimari v0.28 — 13 Eylül 2026

Doğal başlatıcı dönüşü, ikinci startup ABI ve uygulama giriş çerçevesi için ayrı API/CLI/reçete eklendi. Wrapper'ın tek tarama bayrağı, 101-byte dispatcher ve mevcut tablolar/adaylar her yeni durakta doğrulanır. Negatif fixture, portable/policy/CLI testleri ve build/CI eşlemesi birlikte güncellendi; [sonuç ve kalan sınır](d1-application-entry.md). Eski CLI durakları/C ABI 1/GNS/otorite değişmez; kaldırılan özellik veya kalıcı migration yok, C++ observer yeniden derlenir. Dört Windows akışı geçti: x86 Debug/Release 21 native/79 managed/179 Python; x64 Debug/Release 9/79/129. 24 portable kontrol, 23 x86 senaryo + 12 warm; gerçek GTA 12/12 giriş, 28/28 matris, 25/25 child çıkışı, 130/130 girdi korunumu. CLI dispatcher büyük sonuçlar için tek dönüş slotu kullanır; Debug yığın taşması giderildi. Linux/hosted/N1 SDK tekrar koşulmadı.

## Kod 0.1.20 / mimari v0.27 — 13 Eylül 2026

Ayrı CRT startup API/CLI, bounded initializer tablosu, rel32 ve enclosing stack denetimi eklendi. C++/Python corpus ve standart build girişleri, ADR-55 ve owner belgeleri birlikte güncellendi. C ABI/otorite/GNS değişmez; kaldırılan özellik veya kalıcı migration yok. [Uygulama ve kanıt](d1-crt-startup.md).

Dört yerel Windows akışı geçti: x86 Debug/Release 19 native suite/79 managed/170 Python; x64 Debug/Release 8/79/122. 15 portable spec kontrolü, 16 x86 senaryo + 12 warm çevrim ve yedi yeni policy/iki CLI testi eklendi. Gerçek GTA 12/12 CRT sınırı, 27/27 beklenen matris sonucu, 24/24 child çıkışı ve 115/115 girdi korunumu; her koşuda 1676 slot üç kez karşılaştırıldı. İlk generator f-string syntax hatası ve eksik owner dokümanları build öncesi giderildi. İlk hatalar ve loglar raporda korunur; başlatıcı gövdeleri/uygulama girişi, Linux/hosted CI/N1 SDK bu kesitte çalıştırılmadı.

## Kod 0.1.19 / mimari v0.26 — 13 Eylül 2026

Bağımsız `image_protection_probe.py` ve 17 taşınabilir test eklendi. Çalıştırılmayan kendi fixture image'ında 0x40 isteği sonrası 0x80 ve tek özel kopya yazımında 0x40 geçişi ölçülür; bounded PE/region, girdi korunumu ve kapanış kontrolü vardır. Windows/Linux standart test girişleri ve belge eşlemesi güncellendi. C ABI/SDK/dependency/otorite değişmez; kaldırılan özellik/migration yok. X64 Debug standart build 7 native/79 managed/115 Python ve ayrı Debug/Release Windows eşleme ölçümleri geçti; bu görevde GTA veya hosted/Linux koşusu yapılmadı. [Kapsam ve doğrulama](d1-image-protection.md).

Ayrı doğal startup-return API/CLI, exact Windows export önekleri, loader VirtualProtect argüman/dönüş/region ve doğal GetStartupInfoA stack/register denetimi eklendi. Bootstrap ABI değişmedi; bu mod export çağırmadan doğal yola ayrılır. Eski modlar aynı duraklarda kalır; C++ caller yeniden derlenir, kaldırılan özellik/kalıcı migration yok. ADR-54, corpus, build/CI ve owner belgeleri birlikte güncellendi. Gerçek GTA 12/12 doğal dönüş, 24/24 frame bayt örneği; 26/26 beklenen matris sonucu, 23/23 child çıkışı ve 100/100 girdi korunumu doğrulandı. [Test, ilk hatalar ve GTA kanıtı](d1-startup-return.md).

Son dört Windows akışı geçti: x86 Debug/Release 17 native suite/79 managed/161 Python; x64 Debug/Release 7/79/115. Doğal startup corpus'u 18 senaryo + 12 tekrar, portable frame/startup sözleşmesi 33 kontrol içerir. İlk C++ tür çıkarımı ve parent seçim hataları düzeltildi. Başarılı VirtualProtect sonrası 0x80 image sonucu Microsoft sözleşmesi ve bağımsız Windows Debug/Release ölçümüyle incelendi; exact 0x40/0x80 kabulü ve negatifleri eklendi. Yeni tanı aracının eksik doküman eşlemesinde duran akışlar owner belgeleri güncellendikten sonra geçti. Başarısız ilk loglar korunur; Linux/hosted CI/N1 SDK yeniden çalıştırılmadı.

## Kod 0.1.18 / mimari v0.25 — 13 Eylül 2026

Read-only frame adayı reçetesi, portable rel32/range/match sözleşmesi, ayrı API/CLI ve üç duraklı observer örnekleri eklendi. SDK event ile CGame::Process ilişkisi belgelendi; initGameEvent/Initialise ayrımı düzeltildi. Negatif corpus, build/CI, ADR-53 ve owner eşlemesi aynı değişiklikte güncellendi. GNS, otorite ve bootstrap C ABI 1 değişmedi; C++ observer yeniden derlenir, kaldırılan özellik/kalıcı migration yok. Dört Windows akışı geçti: x86 Debug/Release 16 native/79 managed/135 Python, x64 Debug/Release 7/79/91. Gerçek GTA 12/12 koşuda 36/36 faz örneği; 25/25 regresyon matrisi, 22/22 child çıkışı, 84 girdi korunumu. Kısa bayt dizisine 32 baytlık hash formatter uygulanması ilk derlemede yakalanıp düzeltildi; üretilmiş reçete portable C++ testine alındı. [Kanıt ve sınırlar](d1-frame-target.md).

## Kod 0.1.17 / mimari v0.24 — 13 Eylül 2026

Ayrı bootstrap yaşam döngüsü izni, sekiz çağrı/status denetimi, executable export prefix audit ve crypto-provider pini eklendi. Own fake/gerçek DLL fixture ve negatif corpus, Windows/Linux policy kontrolleri ve ADR-52/owner belgeleri birlikte güncellendi. C ABI 1 değişmedi; C++ caller yeniden derlenir, kaldırılan özellik/kalıcı migration yoktur. Dört Windows akışı geçti: x86 Debug/Release 15 native/79 managed/126 Python; x64 Debug/Release 6/79/84. Gerçek GTA 12/12, 96/96 C ABI dönüşü; 24/24 matris, 21/21 child exit ve 59 input hash korunumu. İki fixture hatası ve Release prologue relocation uyumsuzluğu giderilip kayıtları korundu. Ayrıntılar [raporda](d1-bootstrap-lifecycle.md).

## Kod 0.1.16 / mimari v0.23 — 13 Eylül 2026

Ayrı ASI call/return gözlemi, DR2 boş tarama koruması, exact full-path ve fresh module/EAX/stack denetimi eklendi. CMake denetlenmiş bootstrap DLL/map çıktısını configuration'a özel observer pinine bağlar; yeni mod Release 28/Debug 31 pin ister. Own ASI corpus, strict policy ve artifact-binding testleri eklendi. Bootstrap export/C ABI, GNS/otorite ve engine profili değişmedi; kaldırılan özellik yoktur. C++ observer yeniden derlenir; eski özel ASI kopyası yeni artifact ile eşleşmiyorsa tekrar hazırlanır. ADR-51 ve owner belgeleri aynı değişiklikte güncellendi. İlk policy kontrolü elle yazılmış return örneğinin 16 yerine 17 byte olduğunu yakaladı; exact pinned PE'den 16 byte alınarak düzeltildi. [Doğrulama kaydı](d1-asi-bootstrap-load.md). İlk derleme/fixture-path/preflight sorunları ve map newline hash düzeltmesi raporda saklandı. Son dört Windows akışı: x86 Debug/Release 14 native/79 managed/112 Python; x64 Debug/Release 6/79/78 geçti. Gerçek GTA 12/12 SAEX DLL load dönüşü, 11 regresyonla 23/23 beklenen sonuç; 20/20 created child çıkışı ve 44 input hash korunumu. ASI isim izni default kapalı, yalnız game-root exact ad için açık. C ABI çağrısı henüz yok; Linux/hosted/N1 yeniden koşulmadı.


## Kod 0.1.15 / mimari v0.22 — 13 Eylül 2026

Ayrı codec-bindings API/CLI ve codec parent digest'ine bağlı sekiz isim/slot/target recipe eklendi. Beşinci durakta boş tablonun exact root+RVA adresleriyle dolması, ilk 16 target byte kararlılığı ve retained mapping sürekliliği denetlenir. Null/yanlış bağ, retired mapping, yeni DLL, stop/target drift ve eski durak regresyonları için own fixture/corpus eklendi. Yeni modül pini, bootstrap C ABI/engine profile/SDK/otorite/GNS değişikliği yoktur. C++ observer çağıranları yeniden derlenir; kaldırılan özellik veya kalıcı migration yoktur. ADR-50, owner sözleşmeleri, status/roadmap/component-map ve Windows/Linux generator/test akışı birlikte güncellendi. [Doğrulama kaydı](d1-codec-bindings.md); ilk Debug native 24 senaryo/control/12 warm, altı policy ve ilk özel GTA sekiz bağ kontrolü geçti. İlk eksik execution-contracts eşlemesi tamamlandı; son matris: x86 Debug/Release 13 native/79 managed/101 Python; x64 Debug/Release 6/79/72 geçti. Özel GTA 12/12 koşuda 96/96 pointer bağı, sekiz regresyonla 20/20 beklenen sonuç; 18/18 created child çıkışı ve 22 input hash korunumu. Linux/hosted/N1 SDK bu sürüm için yeniden koşulmadı.


## Kod 0.1.14 / mimari v0.21 — 13 Eylül 2026

Ayrı codec-return API/CLI ve startup digest'ine bağlı üç exact codec pini eklendi. Dördüncü DR0 hit ilk LoadLibraryA dönüşünde EAX/root ve yeni dependency mapping'lerini denetler; DR1 IAT watch, eski modların durakları/pinleri ve owned cleanup korunur. Yedi own EXE, root/leaf DLL zinciri, pozitif canary, negatif corpus, warm çevrim ve strict policy/CLI ret testleri eklendi. C++ observer kullanıcıları yeniden derlenir; bootstrap C ABI 1, SDK lock, profile/anchor, GNS/HTTPS ve otorite aynı. Kaldırılan özellik/kalıcı migration yoktur. Owner sözleşmeleri, component-map, Windows/Linux akışı ve ADR-49 birlikte güncellendi. [Doğrulama kaydı](d1-codec-return.md); ilk Debug native corpus 24 senaryo + control + 12 warm ve ilk özel GTA codec dönüşü geçti. İlk testin preloaded sistem DLL varsayımı kernel32 seçimiyle düzeltildi; başarısız kayıt saklandı. Son standart sonuç: x86 Debug/Release 12 native/79 managed/93 Python, x64 Debug/Release 6/79/66 geçti. Gerçek özel GTA 12/12 codec dönüşü; yedi ek regresyonla 19/19 beklenen sonuç, 17/17 created child exit ve 22 input hash korunumu. Eksik/değişmiş codec child öncesi reddedildi. Linux/hosted/N1 bu sürüm için koşulmadı.


## 2026-09-13 — 0.1.13 hosted yayın kanıtı

`bd98c4a` kaynakları için PR #9’daki beş Windows/Linux build, doküman ve sır taramasının tamamı geçti. Windows x64 6 native/79 managed/60 Python, x86 11 native/79 managed/85 Python; Linux 4 native/79 managed/51 Python. Yerel x86 Debug birleştirme koşusu ve altı commit’li geçmişin Gitleaks taraması da geçti. Status ve yayın raporu güncellendi; bu sonuç kaydı yalnız belgedir. Kaynak/ABI/otorite, raw hash girdileri ve önceki gerçek GTA kanıtının sınırı değişmedi; N1 hosted ve GTA yeniden çalıştırılmadı.

## 2026-09-13 — 0.1.13 kaynaklarının yayın tabanıyla birleştirilmesi

0.1.12/0.1.13 kaynakları, önceki public yayın ve hosted CI düzeltmeleriyle birleştirildi. Çakışan belge ekleri iki tarafın kanıtını korur; kaynak/ABI/otorite ve raw hash girdileri bu birleştirmede değişmedi. Yerel başlangıç commit’i güvenlik dalı ve tam Git bundle ile korundu. Yeni yayın PR ve mevcut yedi zorunlu kontrol üzerinden ilerler; sonuç [yayın raporunda](github-publication.md) ayrı kaydedilir.

## Kod 0.1.13 / mimari v0.20 — 13 Eylül 2026

Ayrı run_to_startup_call/--observe-startup-call ile orijinal entry’den sonraki ilk proxy IAT çağrısı tutulur. DR0 fonksiyon başlangıcına taşınır, DR1 aynı main-thread’in IAT yazımlarını gözler. CALL biçimi/slot, stack parametre alanı, target byte kararlılığı ve dört sınırlı image örneği raporlanır. Eski entry/proxy durakları korunur; startupObservation eski modlarda null olur. Strict startup-policy source’u proxy ve observed profile digest’ine bağlıdır; yeni DLL pini veya otomatik native izin eklenmedi. Dokuz fixture, 19 senaryo + canary positive control + 12 warm, yedi policy ve iki CLI testi eklendi. Özel CRT entry denemesi unresolved CRT sembolleriyle başarısız oldu; normal CRT entry seçilerek düzeltildi. Ortak fixture PE helper taşındı; production parser/SDK/ABI/otorite değişmedi. Public C++ trace/API kullanıcıları birlikte yeniden derlenir; bootstrap C ABI 1 aynı, kaldırılan ürün özelliği/kalıcı migration yoktur. ADR-48, sahip belgeleri, status/roadmap/component-map ve Windows/Linux test girişleri güncellendi. [Doğrulama ve sınırlar](d1-startup-call.md). Son standart sonuç: x86 Debug/Release 11 native suite + 79 managed + 85 Python; x64 Debug/Release 6 suite + 79 managed + 60 Python geçti. Gerçek private GTA 12/12 startup hit, dört original/legacy regresyonla 16/16 confirmed exit; 16 input dosyası değişmedi. Linux/hosted/N1 bu kesit için yeniden doğrulanmadı.


## Kod 0.1.12 / mimari v0.19 — 13 Eylül 2026

Ayrı run_to_proxy_return/--observe-proxy-return, exact entry→proxy dönüş durağı, runtime thunk/return slot/aktif mapping ve entry/IAT doğrulaması eklendi. Yeni compiled proxy-policy entry digest ve game-root modül hash’ine bağlıdır; otomatik izin/pin genişlemesi yoktur. Sekiz own EXE/DLL fixture varyantı, 19 native senaryo/12 warm çevrim, yedi policy ve iki CLI testi eklendi. İlk Debug stack overflow büyük trace geçicileri kaldırılarak giderildi; yığın limiti artırılmadı. Eski CLI’lar durma sınırlarını korur; additive proxyObservation eski modlarda null olur. Public C++ observer tipi değiştiğinden çağıranlar birlikte yeniden derlenir; bootstrap C ABI 1 ve N1/GNS kararları aynı kalır. Kaldırılan özellik veya kalıcı veri migration’ı yoktur. ADR-47, normatif owner’lar, status/roadmap/component-map ve Windows/Linux test girişleri birlikte güncellendi. [Kanıt ve sınırlar](d1-proxy-return.md). Son doğrulama: Windows x86 Debug/Release 10 native suite + 79 managed + 76 Python; x64 Debug/Release 6 suite + 79 managed + 53 Python geçti. Gerçek private GTA 12/12 proxy dönüşü ve üç original/legacy regresyonda 15/15 confirmed exit; 16 girdi dosyası değişmedi. Linux/hosted/N1 yeniden doğrulaması bu kayıt kapsamında değildir.


Her kayıt davranış, kaynak, doküman, test ve kalan sınırı birlikte taşır. [Durum](status.md) · [İş akışı](workflow.md)

## 2026-09-13 — Public yayın doğrulaması ve korumalı katkı akışı

Kaynak `e5e2dd0` için beş hosted build ve doküman/sır kontrollerinin tamamı geçti. README gerçek Actions rozetlerine ve public proje panosuna bağlandı; status/workflow Linux portable kanıtını kaydetti. İki public repo, MIT/notice belgeleri, Owner sahipliği, main PR/linear/squash koruması, ana depoda yedi zorunlu check, özel güvenlik bildirimleri, secret/push taraması, Türkçe issue formları, üç milestone ve yedi planlı iş doğrulandı. İlk başarısız koşular ve giderilen taşınabilirlik sorunları [yayın raporunda](github-publication.md) korunur. Son özet yalnız doküman/README değişimidir; D1/D2 veya GTA runtime yetkisini ilerletmez. Avatar kaynak varlığı hazır, tarayıcı yükleme izni bekleniyor.

## 2026-09-13 — Hosted CI taşınabilirlik düzeltmesi

İlk GitHub Build başarısızlığından sonra bootstrap reason fallback açık uint32_t dönüşümüne çevrildi. Windows own fixture ham cwd metni yerine OS volume/file ID eşitliğini doğrular; eşdeğer yol, farklı mevcut klasör ve yanlış token regresyonları eklendi. Production LaunchContext, ABI/status layout, engine/policy/SDK hash girdileri ve GTA yetkileri değişmez; migration veya kaldırılan özellik yoktur. Bütün bootstrap/context normatif ve test sahibi belgeleri aynı değişiklikte güncellendi. İlk başarısız CI ve N1 uzun-yol denemesi yayın raporunda korunur; yeni yerel/hosted sonuçlar ayrıca kaydedilecektir.

## 2026-09-13 — İlk public GitHub yayını ve CI hazırlığı

Kullanıcı kurgunun uygulanmasını onayladı. SAEX marka görselleri, sade README ve belge indeksi, MIT lisansı, üçüncü taraf bildirimleri, katkı/destek/güvenlik kuralları, issue/PR şablonları, CODEOWNERS ve LF politikası eklendi. Önceki README açıklamaları bağlantıları düzeltilmiş tarihsel arşivde korundu. GitHub organizasyonu `saex-platform`, mevcut kullanıcının sahipliğinde ücretsiz planla oluşturuldu.

Windows x64/x86 Debug/Release, portable Linux x64, belge eşlemesi ve hash-pinli Gitleaks için workflow'lar eklendi; SDK işi ayrı manuel opt-in kaldı. Yeni `tools/ci.py` event/base doğrulaması sekiz negatif/pozitif testle standart build'e bağlandı. Component map, yayın sözleşmesi, workflow, foundation/status/roadmap birlikte güncellendi. Python örnek yolları taşınabilir hale getirildi.

Ürün API/ABI/otorite ve GTA davranışı değişmedi; kaldırılan ürün özelliği veya migration yok. 189 kaynak + Git metadata'sının yerel yedeği doğrulandı. Temiz checkout x64/x86 Debug/Release akışları geçti: her koşuda 79 managed, x64 6 native suite/46 Python, x86 9 suite/67 Python testi. Actionlint/YAML/SVG ve Gitleaks kontrolleri geçti. GitHub CI sonuçları [yayın raporunda](github-publication.md) tamamlanacak; kaynak aktarımı oynanabilir release değildir.

Staging sırasında hash'li engine JSON ve SDK patch girdilerinin genel LF dönüşümünden etkileneceği saptandı. `.gitattributes` bu girdileri exact byte olarak koruyacak şekilde düzeltildi; hash kapıları veya eski lock/profil değiştirilmedi.

## 2026-09-13 — SAEX marka açılımı ve GitHub yayın kurgusu

Kullanıcının önce kurgu isteği ve ilk yayından itibaren public tercihi doğrultusunda [GitHub yayın planı](github-publication-plan.md) eklendi. Marka açılımı **San Andreas Extended** olarak netleştirildi; README, status ve roadmap birlikte bağlandı. İlk yapı organizasyon profili için `.github`, bütün mevcut kod/şema/test/belgeler için `saex` monorepo; bağımsız SDK/launcher/web depoları sonraki ürün kapılarına bağlıdır. Hedef sahiplik, envanter, lisans önerisi, CI, belge eşlemesi, branch düzeni ve ilk aktarımın doğrulama ölçütleri yazıldı.

Bu yalnız belge/marka kesitidir; kaynak/ABI/otorite veya ürün davranışı değişmedi, özellik kaldırılmadı ve migration gerekmez. GitHub'da oluşturma/yazma, Git commit/remote/push, lisans uygulaması veya release yapılmadı. Native/managed testler bu plan için yeniden çalıştırılmadı. `tools/check_docs.py`: 79 Markdown, 1100 yerel bağlantı, 6 JSON örneği, beş değişmiş belge ve sıfır hata; başarılı build baseline'ı yenilenmedi. 0.1.11 final runtime kanıtı bu değişiklikle tamamlanmış sayılmaz.

## 2026-09-13 — D1-N2 entry boundary / kod 0.1.11 / mimari v0.18

LoaderObservation::run_to_entry ve --observe-entry-boundary eklendi. Yeni explicit executionPolicy, mevcut mapping pin recipe'sinden ayrı DLL/TLS başlangıç izni verir; CREATE_PROCESS anında DR0 kurulur, ilk ntdll breakpoint'inde register/byte kontrolüyle devam edilir. Main-thread PE entry EXCEPTION_SINGLE_STEP kimliği ve 16 byte önce/sonra gözlemi, ardından owned kill/exit vardır. Byte mutation tamamlanmış tanısal sonuç olabilir; canAttach/initializationVerified false kalır. Eski üç komut ilk exception'da durur; additive entryObservation onlar için null olur. Native observer caller'ları yeniden derlenir; bootstrap C ABI 1, engine/profile/policy/SDK lock değişmez. Kaldırma veya dosya migration yoktur.

Yeni x86 fixture/test target'ları, 12 senaryo/12 warm çevrim ve iki Python CLI negatif testi, ADR-46 ve owner/status/roadmap/component-map birlikte güncellendi. İlk geç-register kurulumu başarısızlığı ve Apphelp pin ret sonucu [raporda](d1-entry-boundary.md) korunur. Son standart sonuçlar: x86 Debug/Release 9 native suite + 79 managed + 59 Python; x64 Debug 6 native suite + 79 managed + 38 Python geçti. Son 12 private GTA koşusu aynı vorbisfile+0x1D60 giriş yönlendirmesini hardware fault’unda gözledi; 9 unload işlendi. Original AcLayers ret ve eski context ilk-breakpoint sınırı korundu; son matris 14/14 exit ve 16 native dosya hash eşitliğini doğruladı. X64 Release/Linux ve N1 opt-in SDK tekrar çalıştırılmadı; initialized GTA/SAEX DLL/unpack/ABI açıktır.

## 2026-09-13 — D1-N2 native sembol bağlantıları / kod 0.1.10 / mimari v0.17

PeLinkageInspector ve engine linkage komutu eklendi. İki açık native dosya, import thunk/name/ordinal ve bounded export/alias/hole/forwarder metadata'sıyla karşılaştırılır; isim/case ve ordinal ayrı kalır. Bound IAT lookup yoksa, legacy delay biçiminde, bozuk metadata/bütçede ret verilir. Eksik sembol/forwarder dinamik çözüm varmış gibi kabul edilmez; canInitialize/canAttach false. Shared startup Reader yalnız internal erişimle yeniden kullanılır; eski startup/inspect schema ve CLI davranışı korunur. Binding compact export hedefi taşır; alias listeleri her gereksinim için tekrar serialize edilmez.

22 yeni managed test kaydı, ADR-45, owner/status/roadmap ve component-map eklendi. X86 Debug/Release 8 native suite + 79 managed + 49 Python; x64 Debug 6 native suite + 79 managed + 30 Python ile ilk denemede geçti. Gerçek dosyalarda 10 CLI karşılaştırması 8 complete/2 beklenen incomplete verdi; 94 direct binding, Debug/Release aynı metadata, bağımsız dumpbin export eşleşmesi ve 13 original + 3 private native dosya hash korunumu doğrulandı. [Tam kanıt](d1-native-linkage.md). X64 Release/Linux ve N1 opt-in SDK yeniden çalıştırılmadı. Native bootstrap C ABI, policy/engine/SDK lock ve GNS kararı değişmedi; kaldırma veya migration yoktur. Orijinal oyun DLL'leri değiştirilmez, aday isim eşleşmesi DLL değiştirme onayı değildir.

## 2026-09-13 — D1-N2 loader unload/remap / kod 0.1.9 / mimari v0.16

Portable LoaderMappingLedger ve Windows UNLOAD_DLL kolu eklendi. Her kabul edilmiş mapping benzersiz gözlem içi kimlik taşır; bilinen aktif unload emekli edilir, stale ID aynı base yeniden yüklense de aktif olmaz. Yeni LOAD her seferinde retained file ID/hash kapısından geçer; lifetime load/byte/event bütçeleri iade edilmez. modules tarihçesi korunur; active/unload/mappingId/event-address alanları additive eklendi. On native ledger testi ve OS/CLI history invariant'ları eklendi. ADR-44 ve owner/status/roadmap/component-map birlikte güncellendi. Kaldırılan özellik yok; static observer tüketicileri yeniden derlenir. Bootstrap C ABI, engine/profile/policy/SDK lock ve core sözleşmeler değişmez. [Kanıt ve sınır](d1-loader-lifecycle.md).

0.1.9 sonuç: x86 Debug/Release 8 native suite + 57 managed + 49 Python; x64 Debug 6 native suite + 57 managed + 30 Python geçti. Her configuration için on ledger testi başarılı. Gerçek private kopyada 12/12 breakpoint adayı, 11 accepted unload; original-context AcLayers ve eski 3-pin apphelp retleri korundu. Toplam 14 child exit ve 16 native girdi hash'i doğrulandı. Gerçek same-base remap gözlenmedi; yalnız metadata corpus kanıtı var. Initialization/SAEX bootstrap/ABI açık kalır.

## 2026-09-13 — D1-N2 explicit launch context / kod 0.1.8 / mimari v0.15

LaunchContext ve --observe-context-loader eklendi: bounded/sorted Unicode environment snapshot, retained directory identity, child öncesi ret ve metadata-only environment digest. SuspendedImage opsiyonel context ile explicit CreateProcess parametreleri kullanır. Loader executable yolu tüm modlarda bir kez çözülür. Yeni native context suite, explicit PATH/cwd loader regresyonları ve iki Python CLI ret/redaction testi eklendi. Eski komutlar inherited davranışı korur; additive launchContext alanı onlar için null olur. Kaldırılan özellik yok; static C++ observer yeniden derlenir, bootstrap C ABI/profile/policy/SDK lock değişmez. ADR-43, owner belgeleri, status/roadmap/component map birlikte güncellendi. [Kanıt ve kalan sınır](d1-launch-context.md).

0.1.8 sonuç: x86 Debug/Release 7 native suite + 57 managed + 49 Python; x64 Debug 5 native suite + 57 managed + 30 Python geçti. Gerçek son Debug private gözlemi breakpoint adayı, Release private UNLOAD_DLL_DEBUG_EVENT güvenli ret, original AcLayers ret verdi; tüm 7 child çıkışı ve 16 native girdi hash korunumu doğrulandı. İlk cold/warm handle ve cwd/AppHelp test hataları raporda korunur. DLL unload/remap, initialization/SAEX bootstrap/ABI sıradaki açık N2 işleridir.

## 2026-09-13 — D1-N2 incelenmiş loader profili / kod 0.1.7 / mimari v0.14

Exact engine'e bağlı 22 modüllü JSON recipe, deterministic C++ generator, PreparedLoaderPolicy ve açık --observe-reviewed-loader girişi eklendi. Bütün origin/ad/hash/bütçe spec'leri IO öncesi, bütün gerçek dosya hash/boyutları child öncesi doğrulanır. LoaderFile kalan byte bütçesini hash öncesi uygular. Kısmi pin seti görünmez; eksik/drift dosyası için failedPolicyModule kaydedilir. Eski üç dosyalı --observe-loader korunur; JSON'a additive policySourceDigest/failedPolicyModule alanları gelir. Bootstrap ABI, observed engine JSON'u ve SDK kilidi değişmedi; kaldırma/migration yoktur.

[Rapor](d1-loader-policy.md) ve ADR-42 source policy/Windows compatibility/launch context ayrımını kaydeder. İlk generator kültürel sıralamayı reddetti; ordinal JSON düzeltildi, kontrol gevşetilmedi. Orijinal GTA Apphelp sonrasında AcLayers mapping'inde durdu. Ayrı, bit eşitliğinde üç dosyalı yerel kopyada Debug 22/Release 21 DLL ve başlangıç breakpoint adayı gözlendi; süreç kapatıldı. İki farklı launch context'in initialization kanıtı birleştirilmez. Eksik/değişmiş kopya DLL ile child yaratılmaması doğrulandı. Kaynak→normatif belge eşlemesi, status/plan/roadmap, AC-90/conformance, workflow/indeks/kaynaklar birlikte güncellendi; x86 Debug/Release 6 native suite, 57 managed ve 47 Python testi; x64 Debug 4 native suite, 57 managed ve 30 Python testiyle geçti. Release original/private/legacy deneyleri kendi farklı outcome türleriyle rapora kaydedildi; bütün owned child çıkışları ve native girdi hash korunumu doğrulandı.

## 2026-09-13 — D1-N2 sınırlı Windows loader gözlemi / kod 0.1.6 / mimari v0.13

SDK bağımsız x86 LoaderFile/LoaderObservation, ayrı saex_engine_loader_probe ve DLL/TLS/main canary fixture'ları eklendi. Açık dosya ID/boyut/hash pinleri, üç sistem dosyasıyla sınırlı CLI politikası, event/thread/module/byte/time bütçeleri, ilk exception'da durma ve terminal olay ilerletilmeden owned child sonlandırma uygulanır. SuspendedImage eski varsayılan davranışı korunarak ayrı friend üzerinden deney desteği aldı; bootstrap C ABI, core/GNS/HTTPS kararı ve N1 SDK kilidi değişmedi. Kaldırılan özellik veya migration yoktur.

[Rapor](d1-loader-observation.md), ADR-41, ilgili engine/native/launcher sözleşmeleri, eski observer/preflight kapsam açıklamaları, AC-90/conformance/R-01, status/plan/foundation/workflow/roadmap/README/blueprint/kaynaklar ve component-map birlikte güncellendi. Yeni x86 native suite ve altı CLI negatif testi standart build'e bağlandı. İlk Debug fixture 12 warm çevrim ve DLL/TLS/main kontrollerini geçti; gerçek GTA'da apphelp.dll mapping'i izin listesinde bulunmadığı için exit 1 ile duruldu, owned exit ve 13 native dosyanın değişmediği doğrulandı. X86 Debug/Release tam akışları 6 native suite, 57 managed ve 38 Python testiyle; x64 Debug 4 native suite, 57 managed ve 22 Python testiyle geçti. İki x86 gerçek GTA deneyi aynı apphelp ret/exit sonucunu verdi. Ret pass'e çevrilmez; initialization/N3/D1 açık kalır.

## 2026-09-13 — D1-N2 native başlangıç envanteri / kod 0.1.5 / mimari v0.12

Gerçek `bass.dll` girdisindeki raw padding farkı metadata notu olarak ayrıldı; dosya/image/overlap sınırları ve runtime kapıları korunur. Kaynağın loader uyumluluğu veya güvenilirliği bu gözlemle onaylanmaz.

Negatif testler, InvalidDataException'ın yerel aday ve CLI filtrelerine açıkça alınması gerektiğini yakaladı. Hata düzeltildi; mevcut engine inspect'in bu girdi hatalarında yakalanmamış exception yerine JSON/exit 1 döndürmesi de iki CLI regresyonuna bağlandı.

C# PeStartupInspector/NativeStartupInspector ve engine startup CLI eklendi. Bounded x86 PE EXE/DLL entry, import/delay VA-RVA, TLS callback metadata, upper-directory DLL/ASI aday grafiği, byte/modül/girdi limitleri, partial sorunlar ve inventory digest uygulanır. Native DLL/assembly yükleme ve process oluşturma yoktur; oyun dosyasına yazılmaz. Mevcut inspect başarılı JSON/exit sözleşmesi, native observer/bootstrap, C++20 core, C ABI ve SDK kaynak kilidi değişmedi; yeni bağımlılık veya kaldırılan özellik yoktur.

[Uygulama raporu](d1-native-startup.md), yeni managed corpus ve gerçek kurulum bulgularını ayrı kaydeder. ADR-40, engine/native/launcher sözleşmeleri, status/plan/workflow/foundation, AC-90/conformance/R-01, README/blueprint/roadmap/source-map/kaynaklar aynı kesitte güncellendi. Tam statik metadata bile canAdvanceToLoader=false; native runtime kapıları açık.

Sonuç: x64 Debug ve x86 Release standart akışları geçti; x64 4/x86 5 native suite, 57 managed test (31 yeni), x64 22/x86 32 Python testi. Debug/Release gerçek kurulum envanteri aynı digest ile executable + 12 DLL/ASI ve 75 import ilişkisini verdi; metadata issue sıfır, bass.dll padding notu açık. 13 native dosyanın hash'i değişmedi. N1 source verify geçti; SDK/native runtime fazı ilerletilmedi. 72 Markdown/899 yerel link/6 JSON örneği belge kontrolü temiz.

## 2026-09-13 — D1-N2 askıda süreç / kod 0.1.4 / mimari v0.11

SDK bağımsız ayrı SuspendedImage/CLI eklendi: exact preflight sonrası owned child, ilk create-debug olayının Windows tarafından duraklatılması, file ID/base/bounded header ve anchor okuma, kill-before-continue ve doğrulanmış çıkış. Oyun kodu resume edilmez; GTA DLL yükleme/hook yoktur. Canary kontrolü, 12 fixture lifecycle çevrimi, identity/bounds/thread/stop/unwind testleri ve 5 CLI ret testi build'e bağlandı. Sentetik PE iki CLI testi arasında ortaklaştırıldı. Mevcut bootstrap ABI, observed profile ve N1 SDK kilidi değişmedi; kaldırılan özellik yoktur.

ADR-39, engine/native sözleşmeleri, AC-90/R-01, uygulama planı/status/workflow/component map, foundation/preflight devam kayıtları, README/blueprint/roadmap birlikte güncellendi. [Ölçüm raporu](d1-suspended-process.md) tamamlanmış test ve gerçek süreç sonuçlarını ayrı kaydeder; initialized/unpacked GTA ve N3 kanıtı açık kalır.

Sonuç: x64 Debug ve x86 Debug/Release standart build/test, her yapılandırmada 12 fixture yaşam çevrimi ve üç gerçek GTA create-debug image gözlemi geçti. X64 4/x86 5 native suite, 26 managed, x64 22/x86 32 Python testi. Dosya hash'i korundu ve owned child exit doğrulandı; bootstrap DLL hash'leri değişmedi. İlk fixture denemesindeki CREATE_SUSPENDED kaynaklı event timeout gizlenmeden raporlandı; debug olayının OS duraklatmasıyla düzeltildi. Sonuç initialized GTA/ABI/hook desteği değildir.

## 2026-09-13 — D1-N2 başlangıç DLL / kod 0.1.3 / mimari v0.10

SDK bağımsız yüklenebilir x86 saex_bootstrap.dll, C ABI 1 Initialize/Query/Stop ve portable BootstrapSession eklendi. SAEX DllMain boş ve session constinit; başlangıç kendi host executable yolunu OS'den bulur. ABI/output doğrulaması, bir kez gözlem, terminal rejected/stopped ve concurrent BUSY uygulanır. can_attach ve bindings_loaded bütün yollarda sıfırdır; gerçek GTA loader/hook veya binding eklenmedi. [Modül raporu](d1-bootstrap-module.md).

Mevcut native image CLI'nin salt okunur dosya/hash/PE denetimi ortak WindowsFileObservation RAII yordamına taşındı; CLI JSON/exit ve gözlem profili değişmedi. Üretilmiş DLL'nin x86/import/export/TLS/CRT/map denetimi CTest load adımından önce standart x86 build'e bağlandı. Source map, execution/engine/native SDK sözleşmeleri, ADR-38, R-01/AC-90, N2 planı, status/workflow/README/roadmap/blueprint birlikte güncellendi. N1 SDK lock/source/recipe değişmedi; kaldırılan ürün özelliği yoktur.

Yeni testler: portable session, oyun dışı gerçek DLL lifecycle ve 10 artifact audit negatif testi. İlk cold handle baseline beklentisi başarısız oldu; load-only kontrolü ve ilk Initialize ayrı ölçüldü. İlk artış ayrıca raporlanır, her sonraki 100 warm çevrimde tam eşitlik istenir. Bu düzeltme genel sıfır leak iddiası vermez. Release CRT XTZ null alignment padding'i artifact'te gözlenip yalnız sıfır byte şartıyla denetime alındı; dinamik initializer/callback toleransı eklenmedi. Güncel platform ve artifact kanıtları modül raporundadır; gerçek GTA'ya yükleme/çalıştırma yapılmadı.

Sonuç: standart x64 Debug ve x86 Debug/Release build/test geçti. X64 üç, x86 dört native suite; 26 managed; x64 17/x86 27 Python testi. Her x86 yapılandırmasında 100 ölçülen oyun dışı DLL çevrimi geçti; cold/warm handle değerleri ve artifact kimlikleri saklandı. X86/x64 gerçek dosya/image regresyonunda dört anchor ve kaynak/dosya digest'leri eşleşti; oyun dosyası değişmedi. N1 lock verify başarılı. Belge sonucu 70 Markdown, 834 yerel bağlantı, 6 JSON örneği, sıfır hata. N2 gerçek GTA/symbol/load sırası, N3 hook ve Linux kanıtı açık.

## 2026-09-13 — D1-N2 preflight alt kümesi / kod 0.1.2 / mimari v0.9

SDK bağımsız C++20 PE parser/profile gate/Windows reader ve `saex_engine_image_probe` eklendi. Exact gözlem JSON'u native constexpr veriye üretilir, C# EngineInspector'a gömülür; duplicate key, bounded anchor ve observation-only kontrolleri vardır. Bilinmeyen hash Windows image eşlemesinden önce reddedilir. Aynı salt okunur handle'dan CNG hash ve `SEC_IMAGE_NO_EXECUTE` görünümü elde edilir; başlıklar ve kısa RVA anchor'ları karşılaştırılır. Profil kaydı GTA executable'ını içermez.

Yerel dosyanın 11 bölümü ve dört anchor'ı eşleşti; Hoodlum marker'ı aday sürüm bulgusudur. `canAttach=false` ve `recognizedProfile=false` korunur. Yeni CLI/gözlem alanları ve neden adları [N2 raporunda](d1-engine-preflight.md) kayıtlıdır; kaldırılan ürün özelliği yoktur. Native image observation exit 3, girdi reddi exit 1 kullanır. Gerçek GTA çalıştırma/SDK yükleme/hook veya başlatılmış process image kanıtı yoktur; N2 runtime/bootstrap kısmı ve N3 açık kalır.

Negatif doğrulama: native PE mutation/truncation ve fake image corpus; 26 managed test; 8 generator, 4 gerçek CLI ret ve 5 belge tooling testi. İlk x86 derlemesinde `SIZE_T/uintptr_t` template tür farkı ve testte signed/unsigned karşılaştırma uyarısı düzeltildi; /W4 /WX gevşetilmedi. Güncel platform/koşu kanıtı N2 raporuna işlenir. N1 kaynak/recipe/patch envanteri değiştirilmedi.

Belgeler: N2 raporu, ADR-37, engine/native SDK/execution sözleşmeleri, AC-90/R-01 ilerlemesi, uygulama planı/status/workflow, component map, kaynak incelemesi, README/blueprint/roadmap birlikte güncellendi. Component map N2 testlerini kendi raporuna, N1 testlerini N1 raporuna bağlar. Genel source→belge zorunluluğu sürer.

Sonuç: x64 Debug, x86 Debug ve x86 Release standart build akışları başarılı. Üç gerçek dosya/image gözlemi, C#/native profil digest eşleşmesi ve öncesi/sonrası oyun dosyası hash'i doğrulandı; artifact kimlikleri N2 raporunda. N1 dependency verify başarılı; kaynak kilidi değişmedi. Statik belge sonucu 69 Markdown, 801 yerel bağlantı, 6 JSON örneği, sıfır hata. Linux/x64 Release yeni kesitte çalıştırılmadı; gerçek GTA process testi yapılmadı.

## 2026-09-12 — D1-N1 native dependency / kod 0.1.1 / mimari v0.8

Önceki doküman değişikliği korunarak gerçek [source/patch/recipe lock](../../contracts/engine/plugin-sdk.lock.json), bounded edinme/doğrulama, standalone private x86 SDK library/probe ve opt-in build/test/evidence girişi eklendi. 23 upstream dosyası seçildi; installer/örnek/GTA kodu çalıştırılmadı. Cache orijinal ve iki patch'li build kaynağı olarak ayrılır; hash/ek header/patch/recipe uyuşmazlığı derlemeyi durdurur. Kaynak envanteri ve notice kapsamı [N1 raporundadır](d1-native-dependency.md).

C++20 denemesi SafetyHook std::expected gereksiniminde başarısız oldu. ADR-36 yalnız private SDK hedefinde kilitli C++23 derlemesini kabul eder; core C++20 kalır. C4458 için private üye adları düzenlendi; C4201 native union'a dar push/pop istisnasıdır. /W4 /WX ve /permissive- korunur. Doğru symbol link'i ve CPool layout'u GTA erişimi olmadan sınanır; dışarıdan hazır library kabul edilmez.

20 yeni test tanımı: bir SDK probe'u, 17 dependency testi, iki gerçek CMake x64/eksik-source reddi. x86 Debug/Release opt-in SDK build ve 20 yeni test tanımı geçti; x64 Debug/x86 Debug foundation regresyonu da 16 native + 25 managed/entegrasyon + 5 tooling ile geçti. Toplam 66 farklı test tanımı. İzole cold acquire ve EXE/lib/PDB hash eşleşmesi ayrıca doğrulandı. Bu bölümdeki önceki v0.7 kaydının eşzamanlı kod eklenmesine ilişkin co-change uyarısı bu kesitin sahip belge güncellemeleriyle giderildi; tarihsel kaydı değiştirilmedi.

Belgeler: engine/SDK/execution sözleşmeleri, N1 planı/kanıtı, status/workflow/README/roadmap/blueprint, ADR, research ve acceptance/conformance güncellendi. Overview'deki stale transient iş ile committed receipt ve persistence ret ile OutcomeUnknown ayrımları mevcut v0.5 kurallarıyla düzeltildi; bu düzeltmeler yeni persistence kodu değildir. Kaldırılan ürün özelliği yoktur. Sıradaki iş N2 executable/symbol/SDK bağımsız bootstrap; GTA attach, production IPC, GNS ve sandbox açık kalır.

## 2026-09-12 — Plugin-SDK doküman entegrasyonu / mimari v0.7 (tarihsel kayıt)

Kullanıcının Plugin-SDK-SA'yı üst düzeyde proje dokümanlarına entegre etme ve uygulamaya yol gösterme talebi işlendi. Dryxio/plugin-sdk-sa kaynak pini `b55e89b336a81448c1aa1a5b188431c9845ebaa9` seçildi; seçilmiş kaynak/build/lisans dosyaları salt okunur incelendi. SDK çalıştırılmadı veya build'e bağlanmadı. Kod sürümü 0.1.0 olarak kaldı.

Eklenenler: [kaynak bulguları](../references/plugin-sdk-sa.md), [normatif native SDK sözleşmesi](../architecture/native-sdk-integration.md) ve [D1-N1–N7 uygulama planı](d1-engine-integration.md). ADR-35, R-01a/b/c, R-02a/b/c, R-04a ve AC-89–96 ile kaynak/build lock, SDK bağımsız profil/bootstrap, hook ownership/drain, ABI/IPC, frame watchdog, pool generation, asset lease ve upgrade evidence şartları tanımlandı. 20 başlat/kapat ve 100 entity/asset döngüsü asgari deney hedefleridir; ölçülmüş sonuç değildir.

Bulgu etkisi: SDK sürüm marker'ı executable doğrulaması sayılmaz; callback remove hook restore değildir; upstream VS2026/C++latest/statik CRT ayarlarının yerel C++20 build'ine uyumu açık kapıdır. Genel attach/unload hedefi temiz session/process stop ve ayrıca kanıt isteyen dinamik DLL unload olarak ayrıldı. Kaynak pin/patch/recipe veya engine değişimi eski kanıtı otomatik taşımaz.

Güncellenen bağlantılar: README/blueprint, status/roadmap/workflow, engine/overview, research/ADR/sources, kabul/conformance, native güvenlik ve launcher. Component map yeni engine/tool/test kaynak ailelerini sahip belgelere bağlar; gelecekte EngineInspector değişiminde SDK sözleşmesi/planı da güncellenir. Tarihsel D0 ve foundation test kayıtları geçmiş kapsamlarıyla korunur.

Doğrulama: `tools/build.ps1 -Architecture x64 -Configuration Debug` gerçek Python runtime yolu verilerek geçti; generated contracts kontrolü, **16/16 native**, **25/25 managed/entegrasyon**, **5/5 tooling** testi başarılı. Belge kontrolü **67 Markdown, 724 yerel bağlantı, 6 JSON örneği, sıfır hata** raporladı. 96 AC ve 35 ADR tanımı tekil/eksiksiz; yedi temsilî yeni/genişletilmiş kaynak yolu için eksik sahip belge güncellemesinin gate tarafından reddedildiği ayrıca kontrol edildi. Başlangıç kopyasına göre 20 dosya değişti; yerel inceleme farkı `out/verification/plugin-sdk-doc-review.diff`, yapısal rapor `out/verification/plugin-sdk-doc-review.json` altındadır. Git deposunda henüz commit olmadığından boş Git diff'i doğrulama yerine kullanılmadı.

Bu koşu x64 foundation regresyonudur; SDK compile/link, yeni AC-89–96, bootstrap ve gerçek GTA testleri yapılmadı. X86 foundation'ın eski kanıtı kendi D1 kaydındadır; bu doküman değişikliğinde yeniden çalıştırılmadı. Kaldırılan ürün özelliği yoktur; runtime davranışı ve mevcut `canAttach=false` sonucu değişmedi. İlk kod işi D1-N1'dir; D1/D2 açık kalır.

Son kontrol sınırı: başarılı build'in 23:31 yerel kayıt zamanından sonra, eşzamanlı başka çalışma `src/engine/plugin_sdk/sdk_probe.cpp` ve `tools/native/CMakeLists.txt` dosyalarını ekledi. Son ortak çalışma dizini kontrolü bu eklemeler için status/engine-adapter/workflow güncellemelerini eksik buldu. Bu kaynaklar doküman teslimatının değişiklik/test kapsamına alınmadı; ortak dizinin son gate sonucu başarılı olarak sunulmaz. Önceki başarılı build ve bu sonraki co-change hatası ayrı sonuçlardır; uygulama ilerledikçe kendi kaynak sahibi dokümanlarıyla kapanır.

## 2026-09-12 — D1 foundation / kod 0.1.0 / mimari v0.6

Kullanıcının kodlama başlangıcı ve eşzamanlı belge güncelleme talebi uygulandı. D0 v0.5 tasarım seti korunarak C++20/.NET 10 build temeli, ortak kimlik generator'ı, native lease/clock ve bounded inbox, metadata WorldPlan aracı, salt okunur engine inspector ve gerçek iki süreçli conformance fixture eklendi. Yerel Git deposu başlatıldı; remote/commit/push yapılmadı.

Davranış: tick donması lease'i uzatmaz; expired/revoked grant yenilenmez; stale owner/revision/sequence/alan maskesi reddedilir. Provider/mutator ve dependency/critical closure çelişkisi plan metadata'sında bulunur. Frame'ler explicit little-endian ve bounded'dır; x86/x64 farkı raw pointer paylaşmaz. GTA dosyası incelenir ama doğrulanmamış profile attach verilmez.

Belgeler: [execution](../architecture/execution-contracts.md), [time](../architecture/time-fencing.md), [authority](../networking/authority.md), [WorldPlan](../architecture/world-plans.md), [engine](../architecture/engine-adapter.md), [SDK](../resources/runtime-sdk.md), README/blueprint/roadmap ve araştırma durum bağlantıları güncellendi. ADR-33/34 ile kod–belge kapısı ve fixture kapsamı kaydedildi. Eski D0 doğrulama raporu tarihsel kapsamıyla korundu.

Test: Windows x64 Debug/Release ve x86 Debug native/managed entegrasyon testleri; invalid plan/PE/frame, zaman/kimlik/kota negatif senaryoları. 16 native + 25 managed/entegrasyon + 5 tooling, 46 farklı test tanımı. İsimli testler ve kanıt sınırı [D1 raporunda](d1-foundation.md). `tools/build.ps1` belge ve generated-code drift kontrolünü build girişine bağlar; kod silinmesi de sahip belge güncellemesi ister. 64 Markdown ve 665 yerel bağlantı denetimi temiz; kullanıcıya özel Python runtime komutu workflow'a kaydedildi.

Kaldırılan ürün özelliği yoktur. “Yalnız Markdown çalışma dizini” durumu kullanıcı onayıyla D1 kodlamaya geçiş olarak güncellendi. GNS/HTTPS, C++20/.NET 10, x86 GTA/x64 Host ve sunucu otoritesi kararları korunur. Native GTA entegrasyonu, Linux runtime kanıtı, güvenlik yalıtımı, GNS kimlik entegrasyonu ve D2 multiplayer açık kalır.

0.1.8 gerçek context deneyi, loader_unexpected_event için event türünün raporda eksik olduğunu gösterdi. lastEventCode/lastEventThreadId additive alanları ve regresyonları eklendi; bilinmeyen olay hâlâ terminal ret alır. Başarısız Debug deneyini Release başarısıyla gizleme veya mevcut policy'yi genişletme yoktur.

0.1.11 aynı kesitin ek-pini: ilk gerçek initializer deneyi imm32.dll'de ret verdi. Yerel metadata/hash/import ve Microsoft imzası incelemesinden sonra ayrı entry-policy.json/compiler/header eklendi. Base source/hash değişmedi; ek tablo 1–8 system-only kayıt, base/engine digest ve override yasağıyla sınırlıdır. Sekiz generator testi ve build --check eklendi; ilk ek-pinli Debug özel kopyada entry byte mutation ve owned exit doğrulandı.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.

## Kod 0.1.17 — Kontrollü bootstrap yaşam döngüsü

[Ayrı yaşam döngüsü sözleşmesi ve kanıtı](d1-bootstrap-lifecycle.md): aynı build'in üç denetlenmiş export'u sekiz sabit çağrıyla, ASI dönüşünden sonra çalıştırılır. Yığın veri yazımı ve EIP/ESP yönlendirmesi ayrı izindir; eski komutların durakları korunur. C ABI 1, engine profili, GNS/HTTPS ve otorite değişmedi. Dört yerel Windows akışı, 24 test senaryosu + 12 tekrar ve gerçek GTA 12/12 koşuda 96/96 dönüş geçti. N2/N3 ve oynanabilir D2 açık; geçmiş sürüm başlıkları o kesitin kanıtını anlatır.

Export girişleri 20 byte'tır; PE32 HIGHLOW alanları metadata'dan tam dört byte olarak normalize edilir. Kısmi/çakışan fixup ve bilinmeyen tip reddedilir. Hiçbir prefix byte'ı karşılaştırmadan çıkarılmaz; C ABI 1 aynı kalır.

## Kod 0.1.32 — Sonuç kaydı

Dört Windows standart akışı geçti: x86 Debug/Release 43 native suite/79 managed/261 Python; x64 Debug/Release 20/79/190. Yeni 583 portable kontrol, 12 native senaryo, bir canary ve 12 warm başarılıdır. GTA query/acquire Debug/Release 4/4 beklenen OS profil reddi verdi; oyun süreci oluşturulmadı. 17 OS girdisi eski pinlerden farklı, 61/61 kontrol girdisi ve son 293 kaynak/config hash'i korundu. Gerçek GTA query pozitif kanıtı yoktur; yeni OS kabulü, copy/unlock/SEH ve D1/N3/D2 açık. [Ayrıntılı sonuç](d1-cwd-query.md).

## 0.1.33 nihai kanıt

Dört Windows akışı geçti: x86 43 native/79 managed/263 Python, x64 20/79/192. Yeni OS üzerinde gerçek GTA: 14 pozitif query, iki 127-byte ret, 50/50 matris, 41/41 child çıkışı. 293 kaynak/config hash'i korundu. Ayrıntılı ilk başarısızlık, düzeltme ve girdi/artifact kayıtları [profil raporunda](d1-system-profile-9445.md). Linux/hosted/N1 tekrar koşulmadı; sonraki doğal copy/unlock/SEH aşamaları açık.

0.1.36 araç doğrulama düzeltmesi: Run-SAEX hash okuması .NET SHA256/FileStream kullanır; Windows PowerShell Get-FileHash modül keşfine bağlı değildir. UTF-8 BOM/konsol ve kısmi hata raporu ile Windows PowerShell 5.1 üzerinde Debug/Release pozitif akış ve eksik klasör/bilinmeyen exe retleri geçti. Native izin ve başarı koşulları değişmedi.

## 0.1.37 — Bootstrap fixture stack regresyonu

İlk tam Debug kontrolünde 51/52 suite geçti; bootstrap lifecycle fixture çalıştırıcısı **0xC00000FD (stack overflow)** ile çıktı. Ortak trace'e tablo snapshot'ları eklenince eski testteki çok sayıda değer olarak tutulan büyük sonuç ve ternary temporary x86 stack sınırını aştı. Test çalıştırıcısı sonuçları heap üzerinde tutacak ve tek dispatch return slot'u kullanacak şekilde düzeltildi. Test senaryoları, üretim stack reserve, native izinler ve doğrulamalar gevşetilmedi. Hata `cd-stream-bootstrap-failure.json` ve ilk tam Debug log'unda korunur; hedefli tekrar ve tam sonuçlar final kanıtta kaydedilir.

## 0.1.31 GitHub kabul kaydı

[PR #10](https://github.com/saex-platform/saex/pull/10) yedi zorunlu kontrolü geçerek main dalına birleştirildi. x86 hosted Windows Server 2025 / VS 2026 / v143, tek Python executable seçimi ve yayın envanterinin ayrıntıları [yayın raporunda](github-publication.md) korunur. 0.1.40 geliştirmesi bu güncel tabanla birleştirildi.

## 14 Eylül 2026 — Linux fixture derleme düzeltmesi

0.1.40 GitHub yayınında GCC strict uyarısı, file-manager portable testindeki tek satırlık döngü/terminator yazımını reddetti. Döngü gövdesi süslü parantezle açıklaştırıldı ve terminator ayrı satıra alındı; test koşulları, üretim davranışı ve strict -Werror aynı kaldı. İlk hosted ret [yayın raporunda](github-publication.md) tutulur.
