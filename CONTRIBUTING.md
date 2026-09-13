# SAEX'e katkı

SAEX — San Andreas Extended, D1 geliştirme aşamasındadır. Başlangıç için [README](README.md), [uygulama durumu](docs/development/status.md), [yol haritası](docs/roadmap.md) ve [geliştirme akışını](docs/development/workflow.md) okuyun.

## Bir değişiklik hazırlamak

1. Hata veya öneriyi bir issue'da somutlaştırın. Beklenen/gözlenen davranışı ve varsa ilgili AC/R maddesini belirtin.
2. Kendi fork'unuzda kısa ömürlü bir `fix/`, `feat/` veya `docs/` dalı açın.
3. Küçük, derlenen bir kesit hazırlayın; davranış değişikliği için anlamlı negatif testleri ekleyin.
4. [Bileşen haritasındaki](docs/development/component-map.json) bütün sahip belgeleri, [status](docs/development/status.md) ve [değişiklik kaydını](docs/development/change-log.md) aynı PR'da güncelleyin. Kaldırma ve geçiş etkilerini yazın.
5. Etkilenen mimariler için `tools/build.ps1` çalıştırın; karşılaştırma referansını `-DocumentationBase` ile verebilirsiniz. PR'da gerçek komut ve sonuçları paylaşın.

Türkçe iletişim ve dokümantasyon, İngilizce teknik kimlikler/kaynak adları kullanılır. Core C++20, araçlar C#/.NET 10'dur. ABI, kimlik veya otorite kararı değişiyorsa ilgili ADR ve kabul senaryosu da güncellenir. Yeni kaynak/CI dosyası eşlemesiz bırakılamaz.

## İnceleme ve doğrulama

`main` için PR, geçen CI kontrolleri ve çözülmüş inceleme konuşmaları kullanılır. Tek bakımcı döneminde ikinci kişinin zorunlu onayı aranmaz; teknik kapılar korunur. Planlanan, kodlanmış, test edilmiş ve gerçek GTA üzerinde doğrulanmış kapsamı ayrı yazın. Bir fixture'ın geçmesi bütün D1/D2 veya AC/R kapısını kapatmaz.

Orijinal GTA dosyaları, üçüncü taraf oyun DLL/ASI'leri, yerel loglar, credential veya derleme çıktıları eklemeyin. GNS oyun taşıması ve HTTPS asset kararı korunur. SDK kaynakları yalnız exact lock ve notice kurallarıyla edinilir. Güvenlik açığı için public issue yerine [güvenlik politikasını](SECURITY.md) izleyin.

SAEX'e ait katkılar projenin [MIT lisansı](LICENSE) altında sunulur. Üçüncü taraf içerik eklerken kökenini ve kendi lisansını [bildirim dosyasında](THIRD_PARTY_NOTICES.md) belirtin. Sahibi olmadığınız içeriğe lisans veremezsiniz.
