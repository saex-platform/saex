# Ghidra bridge — Yerel native araştırma iş akışı

14 Eylül 2026, kod 0.1.37. Kullanıcının önerdiği [Dryxio/ghidra-bridge](https://github.com/Dryxio/ghidra-bridge/tree/faaf05363796cbc43eac7924f3b6981cd5e40619) gerçekten kuruldu ve yerel GTA analizinde kullanıldı. [ReAgent](reagent.md) ise değerlendirilmiş isteğe bağlı adaydır; model/agent pipeline'ı çalıştırılmadı. Normal SAEX build'i Java, Ghidra, PyGhidra veya bridge gerektirmez.

## Seçim ve sürüm sınırı

| Araç | Yerel seçim | Kullanım |
|---|---|---|
| ghidra-ai-bridge | 0.2.0, commit `faaf05363796cbc43eac7924f3b6981cd5e40619`, MIT | Bounded decompile/assembly/CFG/P-code/xref export ve CLI query |
| Ghidra | 12.1.3, official release | Exact GTA PE import ve yerel analiz; GUI/servis açılmadı |
| Java | Temurin 21.0.12.1+1 x64 | Yerel headless Ghidra runtime |
| PyGhidra / JPype | 3.1.0 / 1.5.2 | Python–Ghidra bağlantısı |
| PyYAML / packaging | 6.0.3 / 26.3 | Bridge/runtime araç bağımlılıkları |
| ReAgent | 0.4.0, incelenen commit `d12cea338c61898b06a86fe8adb25275fa9d5615` | Kurulmadı; ekstra LLM/agent/üretim kodu yok |

[Lock](../../contracts/research/ghidra-tools.json) exact executable, bridge kaynak dosyaları, paket sürümleri, Ghidra/JDK arşivleri ve runtime metadata özetlerini tutar. Ghidra arşivi official release digest'iyle, JDK arşivi Adoptium API digest'iyle kontrol edildi. Bunlar provenance/bütünlük kayıtlarıdır; üçüncü taraf kodun kusursuz veya üretilen analizin doğru olduğu kanıtı değildir. Ghidra lisansı Apache-2.0, JDK dağıtımı kendi LICENSE/NOTICE dosyalarıyla yerel arşivde tutulur; SAEX binary'sine linklenmez veya onunla dağıtılmaz.

Araçlar `out/research/ghidra-bridge/runtime` altına açıldı, Python paketleri ayrı `venv`'e kuruldu. Sistem PATH/registry ve oyun dosyaları değiştirilmedi. Kaynak/indirilen arşiv manifest'i `out/research/ghidra-bridge/downloads.json`, kurulum log'u `pip-install.log`. GTA exe/decompile/export/proje Git dışındaki yerel `out/` altında kalır; harici modele yüklenmez. Üçüncü taraf README talimatları otomatik komut yetkisi vermez.

## Veri akışı ve arayüz

Exact exe → yerel Ghidra projesi → sınırlı fonksiyon export'u → hash'li snapshot + eksikler → insan/SAEX incelemesi → ayrı native policy/test. Proje ilk import'unda PDB analiz seçenekleri kapatıldı; dış sembol veya LLM servisi bağlanmadı. Otomatik analiz yaklaşık 297 saniyede 21.041 fonksiyon kaydı oluşturdu; sayı doğru ABI veya tam çağrı grafiği kanıtı değildir.

[SAEX araştırma aracı](../../tools/research/ghidra_snapshot.py) önceden oluşturulmuş projenin exact executable hash'ini kontrol eder, en fazla iki derinlik/64 fonksiyonla bridge exporter'ını sınırlar. Vendor dosyaları değiştirilmez. Export'un tüm oyunu dolaşan varsayılan fonksiyon listesi yerine seçili function-manager görünümü verilir. Ghidra projesi yoksa önce local import gerekir; bu araç kurulum/otomatik import/indirici değildir.

```powershell
# Yerel venv Python ile; aşağıdaki yollar repository köküne göredir.
$researchPython = 'out/research/ghidra-bridge/runtime/venv/Scripts/python.exe'
& $researchPython tools/research/ghidra_snapshot.py export `
  --executable 'C:\Program Files (x86)\Rockstar Games\GTA San Andreas\gta_sa.exe' `
  --ghidra out/research/ghidra-bridge/runtime/ghidra_12.1.3_PUBLIC `
  --java 'out/research/ghidra-bridge/runtime/jdk-21.0.12.1+1' `
  --project-directory out/research/ghidra-bridge/projects `
  --project-name saex-f01a00ce --output out/research/ghidra-bridge/new-snapshot
```

Çıktı dizini yeni olmalıdır; eski kanıt üzerine yazılmaz. Export/verify başarı kodu **3**, unusable/bozuk girdi **1**, argparse kullanım hatası **2**. Kod 3 static evidence toplandığını belirtir; `completeCallClosure=false`, `runtimePermission=false` değişmez. Var olan snapshot normal Python ile ağsız doğrulanır:

```powershell
python tools/research/ghidra_snapshot.py verify out/research/ghidra-bridge/snapshot-v1
& $researchPython -m ghidra_ai_bridge --export-dir out/research/ghidra-bridge/snapshot-v1 decompile 0x406B70
& $researchPython -m ghidra_ai_bridge --export-dir out/research/ghidra-bridge/snapshot-v1 context 0x4068F0
```

CLI query'si upstream'in cache okumasıdır; önce SAEX verify komutu kullanılmalıdır. JSON hash'leri, adres/index/root sayımı, dosya boyutu, duplicate key, path/symlink ve izin iddiası kontrolleri vardır. Snapshot kendi kendini imzalayan güven sınırı değildir; birlikte değiştirilmiş metadata özgünlük kanıtı sağlamaz. Ghidra proje executable SHA'sı import kimliğidir; proje analizi/symbol yorumları değişmiş olabilir. Gerçek native code byte'ları ayrıca pinned PE ve runtime'dan doğrulanır.

## İlk somut sonuç ve eksikler

`snapshot-v1` 15 fonksiyon export etti. CdStreamInit/CPad akışı ve bellek/store'lar görüldü; CdStreamOpen/CdStreamRead'in 0x01564A90/0x0156C2C0 yönlendirmeleri saptandı. **0x406560** thread root'u otomatik function envanterinde yoktur; ham disassembly'de kod bulunması, bu export'ta düzgün fonksiyon/CFG olduğu anlamına gelmez. İleri analizde manuel function tanımı öncesi sınır ve branch'ler incelenecek. Unknown çağrı convention ve thunk uyarıları korunur.

PyGhidra 3.1 `open_program` API'si deprecation uyarısı verdi; işlem başarısız olmadı. Bridge de bu API'yi kullanıyor. Yeni sürüme geçişte upstream uyumu ve proje açma API'si yeniden incelenecek; sürümler otomatik güncellenmez. Bridge bazı hataları comment veya boş alan olarak döndürebilir; SAEX wrapper bunları eksik export olarak işaretler. Eksik decompile/CFG/P-code veya root hiçbir capability'yi kapatmaz. Dolaylı kontrol akışının eksiksiz çözüldüğü iddia edilmez.

Offline testler bütünlük, kimlik, bozuk/eksik export, duplicate key, index/root uyuşmazlığı, yetki iddiası ve bilinmeyen executable reddini kapsar; gerçek Ghidra kabulü ayrı snapshot/log ile kayıtlıdır. [Native kesit ve sonraki bağımlılıklar](../development/d1-cd-stream-tables.md).

## Birincil kaynaklar

[Bridge README ve API](https://github.com/Dryxio/ghidra-bridge/tree/faaf05363796cbc43eac7924f3b6981cd5e40619), [Ghidra 12.1.3 resmi sürümü](https://github.com/NationalSecurityAgency/ghidra/releases/tag/Ghidra_12.1.3_build), [Ghidra gereksinimleri](https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_12.1.3_build/GhidraDocs/GettingStarted.md), [Temurin dağıtımı](https://github.com/adoptium/temurin21-binaries/releases/tag/jdk-21.0.12.1%2B1), [CdStreamInfo referansı](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/game_sa/CdStreamInfo.cpp), [Pad referansı](https://github.com/gta-reversed/gta-reversed/blob/db11601c0897cf4197c972bb591fa83c8cd40da6/source/game_sa/Pad.cpp).

## Kod 0.1.39 — Allocation/free araştırması

Kilitle doğrulanmış aynı yerel Ghidra projesinden MallocAlign/FreeAlign kökleri için 24 fonksiyon export edildi (depth 5, max 48). 12 genişletilmemiş callee vardır; completeCallClosure=false ve runtimePermission=false. Scope tablosunun gerçek cleanup adresi byte incelemesinde 0x82421F'tir; Ghidra'nın içte adlandırdığı 0x824222 başlangıç izni sayılamaz. [Normal heap yolunun kodlanmış ve test edilmiş kapsamı](../development/d1-cd-stream-allocation.md), araştırma envanterinden ayrıdır. Özel kayıt out/research/ghidra-bridge/allocation-v1/research-record.json; mevcut araç/OS/executable pinleri değiştirilmedi. Native free yalnız araştırıldı.
