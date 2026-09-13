# SAEX geliştirme kuralları

Kullanıcı 12 Eylül 2026'da kodlamaya geçilmesini ve her ekleme/değiştirme/çıkarmada dokümanların birlikte güncellenmesini istedi. Önceki D0 belgelerindeki yalnız dokümantasyon sınırı tarihsel teslimata aittir.

- Türkçe iletişim/dokümantasyon; teknik kimlik ve kaynak dosyaları İngilizce.
- Önce README, docs/development/status.md, ilgili normatif sözleşme ve docs/roadmap.md okunur. D1 kapıları kapanmadan D2 multiplayer hazır ilan edilmez.
- Her davranış değişiminde ilgili mimari sözleşme, uygulama durum tablosu ve değişiklik kaydı aynı değişiklikte güncellenir. Kaldırılan özellik ve geçiş etkisi de kaydedilir. docs/development/component-map.json ve tools/check_docs.py bu eşlemeyi denetler.
- Planlanan, kodlanmış, test edilmiş ve gerçek GTA üzerinde doğrulanmış farklı durumlardır. Geçen alt test bütün AC/R kaydını kapatmaz.
- C++20 core; .NET 10 C# araç/SDK. Oyun taşıması GNS, asset HTTPS; ENet fallback eklenmez. Kimlik/otorite/ABI davranışı değişiyorsa ADR ve kabul senaryosu güncellenir.
- GTA native adresleri tahmin edilmez. Bilinmeyen executable veya kanıtlanmamış capability güvenli ret verir. Orijinal oyun dosyaları değiştirilmez/dağıtılmaz.
- İndirilen C# için process/sandbox kanıtı gerekir. Yerel conformance probe, production IPC veya güvenlik sandbox'ı diye sunulmaz.
- Küçük, derlenen ve anlamlı negatif testleri olan kesitler teslim edilir. tools/build.ps1 ve docs/development/workflow.md izlenir; başarısız kontrol gizlenmez.
- Kullanıcı istemedikçe alt ajan veya ayrı task açılmaz. Git remote/push/deployment/server restart açık yetki gerektirir. Mevcut dosyaları silen Git cleanup yapılmaz.
