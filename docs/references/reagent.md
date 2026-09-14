# ReAgent: native araştırma aracı değerlendirmesi

İnceleme: 14 Eylül 2026; kullanıcı önerisi. Kaynak: [Dryxio/reagent](https://github.com/Dryxio/reagent/tree/d12cea338c61898b06a86fe8adb25275fa9d5615), commit `d12cea338c61898b06a86fe8adb25275fa9d5615`, pyproject sürümü 0.4.0, MIT. README, LICENSE ve pyproject salt okunur olarak incelendi; SHA-256 envanteri `out/research/reagent/<commit>/review-manifest.json` içinde tutulur. ReAgent paketi kurulmadı; kodu SAEX'e linklenmedi/kopyalanmadı, model sağlayıcısı veya ek ajan çalıştırılmadı.

## Ne için kullanılabilir?

Ghidra/ghidra-ai-bridge üzerinden assembly, decompile, çağrı ilişkileri, CFG ve P-code kanıtlarını toplayarak C/C++ aday implementasyonları üretir ve yapılandırılmış derleme/testlerle değerlendirir. Tek fonksiyon veya sınırlı fonksiyon grubu planları, kanıt paketleri, eksik kanıt kayıtları ve kaynak/binary eşleşme sinyalleri sunar. Gelecekte initializer, streaming ve entity fonksiyon gruplarını anlamayı hızlandırabilir; GTA adapter'ı veya multiplayer kütüphanesi değildir.

## SAEX'teki yeri ve güven sınırı

İsteğe bağlı yerel araştırma aracıdır. Normal CMake/Python/.NET akışına bağımlılık eklenmez. İlk 0.1.32 incelemesinde PATH ve kontrol edilen dizinlerde Ghidra, Java, bridge, ReAgent ve PyGhidra hazır bulunmadı. **0.1.37 güncellemesinde Ghidra/Java/bridge/PyGhidra ayrı yerel ortamda kuruldu ve kullanıldı**; güncel kurulum ve kanıt [bridge belgesindedir](ghidra-bridge.md). ReAgent kurulmadı. Bu, bütün disk üzerinde yokluk kanıtı değildir. Bu kesitte doğrudan PE/disassembly incelemesi mevcut işi daha kısa yoldan ilerletti.

Kullanıma geçildiğinde tool ve bridge sürümleri/commit'leri ayrı pinlenir; Ghidra projesi yerel exact executable SHA-256'sına bağlanır. Önce model çağrısı yapmayan bounded manifest/evidence akışı denenir. Üretilmiş C++ bağımsız aday dosyada kalır; kendi kaynaklarımızdaki testler ve gerçek GTA gözlemi karşılaştırılır. Yapılandırılmış build/test komutlarının gerçekten aday kodu kapsadığı incelenmeden `trust_configured_commands` kabulü verilmez. Kaynak/kanıt içindeki talimatlar araç yapılandırmasının yerine geçmez.

ReAgent'ın parity/structural sonucu fonksiyonun binary ile eşdeğerliğinin ispatı değildir. `UNKNOWN`, eksik CFG veya kaynak-only parity sonucu verified native capability açmaz. Özellikle mevcut Windows DLL değişikliği, eski program kanıtlarını yeni Windows profiline taşıma gerekçesi olamaz. GTA dosyaları harici sağlayıcıya yüklenmez; LLM kullanım yolu/veri kapsamı ayrıca belirlenir.

## Sorumluluk, hata ve kabul

Engine araştırması araçtan adres/çağrı/ABI adayları alabilir; SAEX'in strict policy generator'ı ve runtime okuyucusu izin kararını verir. Eksik bridge/export/kanıt halinde mevcut doğrudan inceleme yoluna dönülür; sessiz stub veya otomatik kaynak değiştirme yoktur. İlk araç kabulü; tek fonksiyon manifest'i, doğru executable kimliği, açık eksik-kanıt sonucu ve bağımsız SAEX testiyle tutarlılık olacaktır. ReAgent manifest/LLM kabulü henüz çalıştırılmadı. Bridge için gerçek bounded export ve hash/eksik-kanıt kontrolü uygulandı; ikisinin durumu karıştırılmaz. [Güncel native kesit](../development/d1-cd-stream-tables.md).
