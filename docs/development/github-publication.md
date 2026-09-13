# GitHub kaynak yayını: iş akışı ve doğrulama

Tarih: 13 Eylül 2026. **Yerel yayın adayı doğrulandı; GitHub kontrolleri sürüyor.** Kullanıcı [kurgunun](github-publication-plan.md) uygulanmasını, ilk yayının public olmasını ve mevcut hesabın sahipliğini onayladı. Kod 0.1.11 / mimari v0.18 kapsamı korunur; bu çalışma oyun içi yetenek eklemez.

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

İlk kaynak commit'i `8d775e7848c792b8070cf4e8155e14fc55fbec90`, sıfır build cache'iyle ayrı yerel checkout'ta doğrulandı:

| Yerel temiz checkout | Native suite | Managed/entegrasyon | Python test |
|---|---|---|---|
| Windows x64 Debug | 6/6 | 79/79 | 46/46 |
| Windows x64 Release | 6/6 | 79/79 | 46/46 |
| Windows x86 Debug | 9/9 | 79/79 | 67/67 |
| Windows x86 Release | 9/9 | 79/79 | 67/67 |

Actionlint 1.7.12 ve YAML/SVG parse kontrolleri geçti. Gitleaks 8.30.1 ilk Git commit'inin tamamında ve 10 dosyalı topluluk deposunda sıfır bulgu verdi. Staging 213 dosya, yaklaşık 1,40 MB; yasak oyun/build binary'si, symlink veya submodule yok. Exact engine JSON/recipe/patch hash'leri Git index'inde doğrulandı.

[Organizasyon](https://github.com/saex-platform), [ana depo](https://github.com/saex-platform/saex) ve [profil/topluluk deposu](https://github.com/saex-platform/.github) public olarak oluşturuldu. Hesabın active/admin organizasyon üyeliği ve özel güvenlik bildirim kanalı doğrulandı. İlk kaynak push ve GitHub CI sonuçları bu kayıtta tamamlanacak. 0.1.11'in daha önce yapılmış gerçek GTA gözlem sonuçları [status](status.md) ve [entry raporunda](d1-entry-boundary.md) tarihsel kapsamıyla korunur; bu yayın çalışmasında GTA tekrar başlatılmaz.

## İlk hosted CI bulguları ve düzeltme

[İlk Build koşusu](https://github.com/saex-platform/saex/actions/runs/34746684095) başarısızdır: Linux GCC enum/uint32_t koşullu dönüşümü reddetti; dört Windows yapılandırmasında own launch-context fixture ham cwd metin eşitliğinde durdu. Bootstrap fallback açık uint32_t dönüşümüne çevrildi. Fixture gerçek/beklenen klasörü volume/file ID ile eşler; eşdeğer yol pozitifi ve farklı dizin/yanlış token negatifleri eklenir. C ABI, production LaunchContext ve hash/policy sınırları değişmedi. [Doküman ve sır kontrolü](https://github.com/saex-platform/saex/actions/runs/34746684074) ilk turda geçti. Düzeltme sonrası yerel dört standart akış yeniden geçti: x64 Debug/Release 6 native suite, 79 managed, 46 Python; x86 Debug/Release 9 native suite, 79 managed, 67 Python. Yeni fixture alias/farklı dizin/yanlış token kontrolleri bu koşulara dahildir. Hosted yeniden koşu ayrıca kaydedilecektir.

İsteğe bağlı N1 SDK temiz clone denemesi uzun yerel checkout yolunda MSBuild compiler-ID tlog dizini oluşturulamadığı için durdu. Daha kısa, ayrı bir geçici checkout ile Debug/Release yeniden çalıştırıldı: her biri 1 SDK probe, 17 dependency ve 2 configure ret testini geçti. SDK recipe/patch/lock byte'ları değiştirilmedi; lock SHA-256 `eec9cbc16685563a08c4a121ddb885fcfe554dff99b055a4d516d810fe5d7670`. İlk hata kaydı korunur.

[İkinci Build koşusu](https://github.com/saex-platform/saex/actions/runs/34747151868) Linux x64 Debug ve Windows x64 Debug/Release için geçti. X86 iki yapılandırmada bütün 9 native suite geçtikten sonra Python loader CLI cwd string eşitliği `runneradmin`/`RUNNER~1` yazım farkıyla durdu. Aynı test `Path.samefile` ile kimlik karşılaştırmasına geçirildi; engine/child/redaction kapıları korunur. Bootstrap reason corpus'una exact fallback ve UINT32_MAX assertion'ı eklenmiştir. Yeniden koşu bu iki son test değişimini de doğrular.

## Kod 0.1.12 — Proxy dönüş sınırı

0.1.12 geliştirmesi Linux CI girişine proxy_policy --check ve portable policy corpus’unu ekler; Windows standart build aynı source gate’i ve x86 proxy fixture’ını kapsar. Bu yerel değişiklik yeni hosted workflow sonucu veya yayın/push kanıtı değildir. Önceki yayın ve CI kayıtlarının kapsamı korunur.

## Kod 0.1.13 — Startup çağrı sınırı

0.1.13 kaynak değişimi Linux CI girişine startup-policy generation check ve portable negatif corpus’u ekler. Windows standart build yeni x86 startup suite’ini kapsar. Yerel sonuçlar [raporda](d1-startup-call.md) tutulur; yeni hosted çalışma/push veya yayın kanıtı değildir. Önceki yayın/CI kayıtları korunur.
