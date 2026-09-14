# D1 — CRT cwd helper'ın doğal dönüşü

Kod 0.1.35 / mimari v0.42, 14 Eylül 2026. [Durum](status.md) · [Önceki copy](d1-cwd-copy.md) · [ADR-70](../decisions/architecture-decisions.md). Açık native izin: `--observe-cwd-return <exact-executable> <absolute-cwd>`.

## Sorumluluk ve veri akışı

Önceki copy terminalinden iki POP ve JMP yürür; helper encoded cookie'yi geri çözer, EBX'i geri alır, cookie checker'ın eşitlik yolunu çalıştırır ve LEAVE/RET ile wrapper'a döner. Terminal `0x836EB6`, wrapper'ın üç argümanı temizleyen ADD komutundan öncedir. Wrapper sonucu kaydetme/try-state geçişi, unlock(7), SEH sökümü, CFileMgr suffix/dönüşü bu kesite dahil değildir. Kilit hâlâ owned main thread üzerindedir.

Önceki komutların durakları korunur: copy `0x836E8A`, query `0x836E30`. Yeni modu açmak gözlem başarısı değildir; `cwdReturnObservation.verified` yalnız altı durak ve bütün kontroller geçerse true olur. Parent `cwdCopyObservation.helperReturnAllowed` yeni izni, `helperReturnVerified` yeni sonucun türetilmiş değerini taşır. Copy'nin kendi `verified` değeri önceki kopya kanıtını belirtir.

## Altı durak ve ABI

T, acquire terminalindeki ESP; helper EBP=T−16, wrapper EBP=T+44, local kaynak=T−284, hedef R=`0x00B71AE0`. Önceki copy dönüşünde ESP=T−296'dır.

| Durak | GTA adresi | ESP | Doğrulama |
|---|---|---|---|
| Helper epilogue entry | `0x836DE2` | T−288 | İki argüman POP edilmiş; ECX=kaynak; copy EAX/EDX/flags korunmuş |
| Cookie CALL | `0x836DE9` | T−284 | ECX=global cookie; encoded cookie XOR doğal return; EBX geri alınmış |
| Checker entry | `0x82AAD4` | T−288 | CALL doğal return'ü `0x836DEE` olarak eski saved-EBX slotuna yazmış |
| Checker JNE | `0x82AADA` | T−288 | CMP eşit; ZF/PF=1, CF/SF/OF/AF=0; hata dalına izin yok |
| Checker dönüşü | `0x836DEE` | T−284 | Doğal RET tamam; LEAVE henüz çalışmamış |
| Helper dönüşü | `0x836EB6` | T−8 | Doğal LEAVE/RET tamam; EBP=T+44; stack'te [drive=0,R,max=128] |

Altı durakta EAX=R, EBX/ESI=0, EDI=24; EDX copy dönüş değerini korur. İlk durakta copy flags aynı; XOR sonrası AF tanımsız kabul edilir, diğer aritmetik flags çözülen cookie'ye göre hesaplanır. CMP sonrası aritmetik flags `0x44` olmalıdır. TF/DF ve kontrol flags drift reddedilir; debugger RF hariçtir. Checker'ın eşit olmayan kolu production crash handler deneyi değildir; beklenmeyen cookie/flags durumunda owned child kapatılır.

## Kaynak, policy ve arayüzler

[CwdReturnSpec](../../include/saex/engine/cwd_return.hpp), [portable kod/ABI kontrolü](../../src/engine/loader/cwd_return.cpp), [strict policy](../../contracts/engine/cwd-return-policy.json), [generator](../../tools/cwd_return_policy.py) ve [observer](../../src/engine/loader/loader_observation.cpp) birlikte sürümlenir. Policy copy JSON hash'ine, onun üzerinden exact `f01a00ce...` GTA ve Windows 26200.9445 zincirine bağlıdır; OS pinleri değişmedi.

Orijinal executable'dan salt okunur PE extraction + `dumpbin /DISASM:BYTES` ile cleanup 7, epilogue 14, checker'ın eşitlik gövdesi 9 ve wrapper terminal prefix'i 16 byte incelendi. Checker `CMP ECX,[0x8E31BC]; JNE +1; RET` biçimindedir; eşleşmeyen yolun JMP'si çalıştırılmaz. Epilogue CALL rel32, cookie mutlak operandı, parent cleanup prefix ve executable/range/overlap kuralları birlikte denetlenir. İlk yanlış hizadan başlatılmış disassembly kullanılmadı; `0x836DE2` tam komut sınırında yeniden incelendi. Adres tahmini veya kanıt dosyasından otomatik izin yoktur.

`LoaderObservation::run_cwd_return` yeni C++ girişidir. CLI JSON'a `cwdReturnObservation` eklenir; eski modlarda null. Stage/adresler, argüman temizliği, cookie eşleşmesi, checker/helper dönüşü, kaynak ömrü ve korunma alanları ayrı tutulur. Ham stack verisi dışarı verilmez. Başarılı gözlem ve doğrulanmış exit kodu 3, ret 1, kullanım hatası 2'dir. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, GNS/HTTPS ve otorite/SDK sözleşmeleri aynı kalır. Kaldırma, kalıcı migration veya yeni bağımlılık yoktur.

## Bellek ömrü ve hata davranışı

- Query/copy doğrulanmadan helper dönüşü açılmaz. 126-byte ASCII kapasite ve expected cwd kuralları parent'ta kalır; 127 byte copy ve return öncesi reddedilir.
- 136-byte hedef penceresi, doğrulanmış copy dönüş görüntüsüyle aynı kalmalıdır. Eski hedefte yalnız dizin+NUL değişmiş olma kanıtı parent'a aittir.
- 260-byte kaynak ve 8-byte guard her checkpoint'te karşılaştırılır. Son RET'ten sonra local kaynak **retired** durumundadır: o anda debugger'ın okuyabildiği eski stack görüntüsü kontrol edilir; artık canlı nesne/ödünç pointer veya gelecek karelerde kullanım hakkı değildir. `sourceSnapshotPreserved` ile `sourceRetired` birlikte raporlanır. `sourceRetired=true` tamamlanmış doğrulamanın sonucudur; ret sırasında bu alanın false kalması kaynağın canlı veya kullanılabilir olduğu anlamına gelmez. `helperReturned` checkpoint gözlemi ile genel `verified` sonucu ayrıdır.
- Cookie CALL, T−288 saved-EBX slotunu `0x836DEE` return adresiyle doğal değiştirir. İlk iki durakta sıfır, üçüncü ve sonrakilerde bu exact return beklenir. Kaynak T−284'te başlar ve bu yazıdan etkilenmez. Encoded/global cookie, saved EBP ve helper return + üç argüman ayrıca korunur.
- 84-byte parent stack, 32/156-byte dış caller, NT_TIB/FS head/önceki SEH kaydı, CRT lock slot/nesnesi, localization, LastError ve named-event kimliği korunur. Parent stack karşılaştırması beklenen max/try-state içeriğini kullanır; wrapper henüz try-state değiştirmez.
- Yalnız DR0 yeni duraklara taşınır; diğer üç hardware guard korunur. Kaynak/hedef/stack/TEB'ye sentetik yazı yoktur. Owned process sonlandırması normal unlock veya SEH unwind kanıtı değildir.

Yanlış recipe `cwd_return_invalid_spec`, copy sonrası code uyuşmazlığı `cwd_return_precondition_shape`, checkpoint drift `cwd_return_shape_drift`, ABI `cwd_return_frame`, saved slot/cookie `cwd_return_cookie_drift`, caller argümanları `cwd_return_helper_arguments` ile reddedilir. Kaynak/hedef/SEH/kilit/guard/localization/LastError için ayrı ret nedenleri vardır. Yanlış thread owner devam/cleanup yapamaz; gerçek owner yeniden çağırabilir. Önceki dosya/OS hash kapıları güvenli ret verir. Bu checkpoint denetimi genel native sandbox veya tüm race'leri engelleyen mekanizma değildir.

## Kabul, test ve genişleme

[Portable corpus](../../tests/engine/cwd_return_spec_tests.cpp) sıfır/yüksek-bit/dolu cookie, altı aşamada register/flags/stack negatifleri, adres taşması ve aralık çakışmalarını sınar. [Native corpus](../../tests/engine/cwd_return_tests.cpp) doğal başarı, yanlış checker/prefix, parent API drift, owner recovery, event budget, yanlış cwd, held event ve eski copy/query/acquire sınırlarını içerir. Post-helper canary, yeni terminalden sonraki wrapper komutunun yürütülmediğini doğrular; warm tekrar ve handle dengesi ayrıca ölçülür. [Generator testleri](../../tests/engine/test_cwd_return_policy.py) bütün parent hash'lerini ve admitted byte'ları; CLI testleri bilinmeyen exe/izin/argüman retlerini kapsar.

Hedefli kontroller, dört Windows akışı ve private GTA kabul/regresyon matrisi tamamlandı; sonuçlar aşağıda ayrı kaydedilir. Standart build GTA başlatmaz. Linux/hosted/N1 bu çalışmanın kapsamına alınmadı. Alt test AC-90/R/N2/N3/D1/D2 kapılarının tamamını kapatmaz.

Sıradaki aşama wrapper'ın üç argümanı temizlemesi, dönüş değerini kaydetmesi, try-state geçişi ve unlock(7) yoludur. Ardından SEH epilogue, CFileMgr suffix/dönüşü, diğer initializer yardımcıları, renderer/doğal frame/N3 ve D2 sırası korunur.

## İlk doğrulama

İlk hedefli derleme ve iki native suite geçti: **789 portable kontrol**, **14 native senaryo**, bir post-helper canary ve **12 warm çevrim**, handle **143→143**. Altı generator testi geçti. İlk CLI testinde relative cwd için kullanım kodu 2 beklenmesi hatalıydı; mevcut sözleşme `launch_directory_input`/1 olduğundan yalnız test beklentisi düzeltildi. Ardından **54/54 CLI testi** geçti. İlk Debug private GTA denemesi `cwd_return_verified`, altı durak ve doğrulanmış child çıkışı verdi. Bu tek keşif koşusu nihai matrisin yerine geçmez. Kanıtlar `out/verification/engine/cwd-return-first-build.log`, `cwd-return-first-tests.log`, `cwd-return-targeted-native.log`, `cwd-return-initial-Debug.json` içindedir.

## Nihai doğrulama — 14 Eylül 2026

| Windows akışı | Native suite | Managed | Python | Sonuç |
|---|---|---|---|---|
| x86 Debug | 48 | 79 | 277 | geçti |
| x86 Release | 48 | 79 | 277 | geçti |
| x64 Debug | 22 | 79 | 204 | geçti |
| x64 Release | 22 | 79 | 204 | geçti |

Dört standart akış **309/309 aynı kaynak/config hash'i** ile tamamlandı. Her iki x86 yapılandırmasında 789 portable kontrol, 14 native senaryo, post-helper canary ve 12 warm çevrim geçti. Handle sayıları Debug **136→136**, Release **143→143**; büyüme görülmedi. Altı generator ve toplam 54 CLI testi geçti. İlk CLI beklenti düzeltmesi yukarıda kaydedildi; dört tam akışta başarısız kontrol yoktu.

Private GTA matrisi **56/56 beklenen sonuç**, **51/51 yaratılan child için doğrulanmış exit**, **176/176 girdi hash'i korunumu** verdi. Debug ve Release normal dizinlerinde altışar, 123/124/125/126-byte sınırlarında birer olmak üzere **20 pozitif helper dönüşü** doğrulandı. 127-byte dizinler her iki yapılandırmada query kapasite kapısında, copy/return stage=0 ve verified=false olarak reddedildi. İlk Debug/Release keşif koşuları ayrıca başarılıydı; 56 koşuluk matrise dahil edilmedi.

20 pozitif koşuda cookie checker `0x82AAD4` eşitlik yolunu geçti; altı checkpoint ve `0x836EB6` terminali doğrulandı. EAX hedefi/EDX/nonvolatile register'lar, ESP/EBP geçişleri, eski saved-EBX slotundaki checker return adresi, source snapshot ve retired işareti, kopyalanmış hedef, wrapper argümanları, caller/SEH/kilit/cookie/guard/localization/LastError denetimleri başarılıydı. Kilit tutulmaya devam etti; normal unlock veya SEH sökümü kanıtı üretilmedi.

Copy ve query komutları iki yapılandırmada kendi terminalinde kaldı; return kaydı null, helper dönüş izni kapalıydı. Diğer 22 önceki mod Release'te, acquire ayrıca Debug'ta geçti. Yanlış context, yanlış ASI yapılandırması, eksik ASI, bozuk codec ve bilinmeyen executable child öncesi reddedildi. Mevcut named event `183`, aynı adlı mutex `6` ile query/copy/return başlamadan reddedildi. Deneyin manual-reset event'i sinyallenmedi; Windows host setting önce/sonra aynı kaldı.

Orijinal GTA native dosyaları ve OS/policy girdileri değişmedi. Bootstrap DLL/map hash'leri 0.1.34 ile aynı, probe stack rezervi **1 MiB** olarak korundu. Kanıtlar `out/verification/engine/cwd-return-gta-evidence.json`, `cwd-return-final-verification.json`, `cwd-return-initial-*.json`, `cwd-return-build-inputs.json`, `build-cwd-return-*-final.log` ve `cwd-return-*-native-final.log` içindedir. Private kopyalar `out/experiments/gta-cwd-return-f01a00ce-*-v1` altında tutulur; orijinal oyun verisi yayımlanmaz.

| Artifact | Debug SHA-256 | Release SHA-256 |
|---|---|---|
| saex_engine_loader_probe.exe | `178d431c7ef650fed071844b9875e1fe625c63ecc44d8b0659867133266af81e` | `3d2ad8531c7bc46a866f10ed8d6118becf2f3629d5faa50c8664a6ac34150806` |
| saex_bootstrap.dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| saex_bootstrap.map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Kanıt kapsamı exact `f01a00ce...` GTA / Windows 26200.9445'tir. Wrapper üç argüman temizliği, sonuç kaydı/try-state, unlock(7), SEH sökümü ve CFileMgr suffix/dönüşü açık kalır. Linux/hosted/N1 yeniden çalıştırılmadı; N2/N3/D1/D2 ve oynanabilir multiplayer tamamlanmadı.

Rapor birleştirmesinin ilk kontrolü x64 Python toplamını yanlışlıkla 205 bekledi ve durdu (`cwd-return-finalize-first.log`). Yeni CLI testi yalnız x86 akışında bulunduğundan doğru x64 toplamı 204'tür; rapor beklentisi düzeltildi ve kanıt birleştirmesi geçti. Bu, derleme veya ürün testi başarısızlığı değildir.

Son belge kontrolü **115 Markdown / 2.038 yerel bağlantı / 6 JSON örneği**, sıfır hata verdi. `git diff --check` geçti; çalışan gözlem/return fixture süreci kalmadı.

## Sonraki birleşik kesit — 0.1.36

Bu belgedeki terminal ve eski test kayıtları kendi sürümüne aittir. [File-manager-ready](d1-file-manager-ready.md) önceki zinciri tek komutta tamamlar: normal kilit bırakma, SEH sökümü, suffix ve CFileMgr dönüşü. Eski komut otomatik ilerletilmez; buradaki snapshot güncel final durum gibi yorumlanmaz. Genel initializer/renderer/frame ve D1/D2 açıktır.
