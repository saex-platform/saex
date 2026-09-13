# GitHub kaynak yayını: iş akışı ve doğrulama

Tarih: 13 Eylül 2026. **Uygulama sürüyor.** Kullanıcı [kurgunun](github-publication-plan.md) uygulanmasını, ilk yayının public olmasını ve mevcut hesabın sahipliğini onayladı. Kod 0.1.11 / mimari v0.18 kapsamı korunur; bu çalışma oyun içi yetenek eklemez.

## Depolar ve sahiplik

Organizasyon `saex-platform`, marka **SAEX — San Andreas Extended**, Owner `Rohatcengizhanbucak`. İlk hedefler public `.github` profil/topluluk deposu ve public `saex` monorepodur. SDK/launcher/web/examples bağımsız ürün ve testleri oluşmadan ayrı depolara bölünmez. Kayıt sırasında verilen iletişim e-postası yalnız hesap iletişim alanındadır; public kaynaklara veya profile eklenmez.

SAEX'e ait kaynaklar [MIT](../../LICENSE) lisanslıdır. [Üçüncü taraf bildirimleri](../../THIRD_PARTY_NOTICES.md) exact dependency lock ile birlikte korunur. GTA dosyaları, yerel dependency/research cache'i ve build çıktıları aktarılmaz.

## Otomatik kontrollerin sözleşmesi

- [Build workflow](../../.github/workflows/ci.yml): Windows x64/x86 Debug/Release ve Linux x64 Debug. Windows mevcut `tools/build.ps1` girişini kullanır. Linux yalnız portable C++/C# fixture kapsamını `tools/ci.py linux` ile sınar.
- [Doküman ve sır kontrolü](../../.github/workflows/docs.yml): gerçek PR base/push-before referansına göre belge eşlemesi, tooling negatif testleri ve sabit hash'li Gitleaks 8.30.1 ile bütün checkout Git geçmişi. Sır taraması çıktısı redacted'dır.
- [N1 opt-in workflow](../../.github/workflows/native-sdk.yml): yalnız manuel dispatch; açık edinme ve exact lock doğrulaması. Runner toolchain'i lock'a uymuyorsa güvenli hata verir; standart foundation CI bunun yerine geçmez.

GTA kurulumuna erişim veya oyun başlatma bu workflow'larda yoktur. Windows native testleri yalnız projenin kendi test executable/DLL'lerini çalıştırır. Public PR'lar GitHub hosted runner'da read-only token ile yürür; checkout credential'ı diskte kalıcı tutulmaz. Actions SHA'ları resmi kaynaklardan doğrulanıp sabitlenir. Native artifact release'i bu iş akışlarının otomatik yan etkisi değildir; yalnız sınırlı test logları yedi gün tutulur.

`tools/ci.py` yalnız `pull_request`, `push`, `workflow_dispatch` event'lerini kabul eder. PR için base SHA; push için `before` SHA kullanılır. İlk push'un sıfır SHA'sı referans yerine geçirilmez. Bozuk/eksik SHA ve `pull_request_target` reddedilir. Referans ayrı subprocess argümanıdır; başlık/dal adı shell koduna yerleştirilmez. Sekiz negatif/pozitif context testi standart Windows build'e ve doküman/Linux kontrolüne dahildir.

`.github/*`, lisans, marka varlıkları ve yeni tooling yolları [component-map](component-map.json) üzerinden bu belgeye ve workflow'a bağlanır. Tooling test ailesinin mevcut foundation belge sahipliği korunur. Kodlanmış CI tanımı ile GitHub'da geçen CI ayrı kaydedilir.

## İnceleme ve topluluk

`main` için PR, geçen required checks, çözülmüş konuşmalar ve linear/squash geçmişi kullanılır. Force push ve silme kapalıdır. Başlangıçta tek bakımcı olduğundan başka kişinin zorunlu onayı konmaz. Sahip organizasyon yönetimini korur; yeni kişilere yönetim veya yayın yetkisi otomatik verilmez.

İssue şablonları hata/özellik ayrımı yapar. Güvenlik açıklamaları [özel bildirim](../../SECURITY.md), sorular [destek](../../SUPPORT.md) yolunu izler. CODEOWNERS ve etiketler gerçek kullanıcı/depolar üzerinden doğrulanır. Proje panosu ve milestone'lar D1/D2/D3 bağımlılığını korur; alt test sonucu bütün aşamayı kapatmaz.

## Yerel koruma ve kaynak aktarımı

Değişiklikten önce 189 kaynak dosyası ve `.git` metadata'sı Git dışındaki yerel ZIP'e alındı; her kaynak hash'i ve ZIP bütünlüğü doğrulandı. Çıktılar orijinal yerlerinde korundu. İlk aktarım yalnız gözden geçirilmiş staging dosyalarından normal commit/push ile yapılır. Yedek ve kişisel laboratuvar çıktıları public yayına dahil edilmez.

İlk kaynak commit'i `main` dalında oluşturulur; ayrı geçici checkout, mevcut makinenin eski `out/bin/obj` içeriğini devralmadan derlenir. İlk uzak depo otomatik README commit'i olmadan oluşturulur. Aktarımdan sonra local/remote SHA, temiz clone, GitHub check sonuçları ve public görünüm doğrulanır.

İlk staging incelemesi, genel LF kuralının raw SHA-256 ile bağlı engine JSON/SDK patch girdilerini dönüştüreceğini gösterdi. Yayından önce `.gitattributes` bu iki aile için `-text` olarak düzeltildi; byte'lar ve mevcut profile/lock/patch digest'leri korunur. Normal metinler LF kullanır. Hash kapıları gevşetilmedi, gözlem profilleri yeniden onaylanmadı.

Bu exact-byte girdilerindeki CRLF, Git whitespace kontrolünde `cr-at-eol` ile satır sonu olarak tanınır; trailing-space/blank-at-eof/space-before-tab kontrolleri korunur. İlk varsayılan whitespace denemesi bu nedenle hata verdi ve kaynak byte'ları değiştirilmeden düzeltildi.

## Sonuç kaydı

Yerel kaynak/CI hazırlığı ve GitHub kurulum doğrulaması sürüyor. Son commit, platform sonuçları, depo bağlantıları ve kalan sınırlar işlem tamamlanınca bu bölümde kaydedilir. 0.1.11'in daha önce yapılmış gerçek GTA gözlem sonuçları [status](status.md) ve [entry raporunda](d1-entry-boundary.md) tarihsel kapsamıyla korunur; bu yayın çalışmasında GTA tekrar başlatılmaz.
