# GitHub kaynak yayını: iş akışı ve doğrulama

Tarih: 13 Eylül 2026. **İlk public kaynak yayını ve yedi hosted kontrol tamamlandı.** Kullanıcı [kurgunun](github-publication-plan.md) uygulanmasını, ilk yayının public olmasını ve mevcut hesabın sahipliğini onayladı. İlk yayın kanıtı kod 0.1.11 / mimari v0.18 kapsamındadır; sonraki sürümlerin aktarım ve CI kanıtı aşağıda ayrı tutulur.

## Depolar ve sahiplik

Organizasyon `saex-platform`, marka **SAEX — San Andreas Extended**, Owner `Rohatcengizhanbucak`. Public `.github` profil/topluluk deposu ve public `saex` monorepo yayımlandı. SDK/launcher/web/examples bağımsız ürün ve testleri oluşmadan ayrı depolara bölünmez. Kayıt sırasında verilen iletişim e-postası yalnız hesap iletişim alanındadır; public kaynaklara veya profile eklenmez.

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

[Organizasyon](https://github.com/saex-platform), [ana depo](https://github.com/saex-platform/saex) ve [profil/topluluk deposu](https://github.com/saex-platform/.github) public olarak oluşturuldu. Hesabın active/admin organizasyon üyeliği ve özel güvenlik bildirim kanalı doğrulandı. İlk kaynak push ve aşağıdaki hosted doğrulama tamamlandı. 0.1.11'in daha önce yapılmış gerçek GTA gözlem sonuçları [status](status.md) ve [entry raporunda](d1-entry-boundary.md) tarihsel kapsamıyla korunur; bu yayın çalışmasında GTA tekrar başlatılmaz.

## İlk hosted CI bulguları ve düzeltme

[İlk Build koşusu](https://github.com/saex-platform/saex/actions/runs/34746684095) başarısızdır: Linux GCC enum/uint32_t koşullu dönüşümü reddetti; dört Windows yapılandırmasında own launch-context fixture ham cwd metin eşitliğinde durdu. Bootstrap fallback açık uint32_t dönüşümüne çevrildi. Fixture gerçek/beklenen klasörü volume/file ID ile eşler; eşdeğer yol pozitifi ve farklı dizin/yanlış token negatifleri eklenir. C ABI, production LaunchContext ve hash/policy sınırları değişmedi. [Doküman ve sır kontrolü](https://github.com/saex-platform/saex/actions/runs/34746684074) ilk turda geçti. Düzeltme sonrası yerel dört standart akış yeniden geçti: x64 Debug/Release 6 native suite, 79 managed, 46 Python; x86 Debug/Release 9 native suite, 79 managed, 67 Python. Yeni fixture alias/farklı dizin/yanlış token kontrolleri bu koşulara dahildir. Hosted son sonuç aşağıda ayrı kaydedilmiştir.

İsteğe bağlı N1 SDK temiz clone denemesi uzun yerel checkout yolunda MSBuild compiler-ID tlog dizini oluşturulamadığı için durdu. Daha kısa, ayrı bir geçici checkout ile Debug/Release yeniden çalıştırıldı: her biri 1 SDK probe, 17 dependency ve 2 configure ret testini geçti. SDK recipe/patch/lock byte'ları değiştirilmedi; lock SHA-256 `eec9cbc16685563a08c4a121ddb885fcfe554dff99b055a4d516d810fe5d7670`. İlk hata kaydı korunur.

[İkinci Build koşusu](https://github.com/saex-platform/saex/actions/runs/34747151868) Linux x64 Debug ve Windows x64 Debug/Release için geçti. X86 iki yapılandırmada bütün 9 native suite geçtikten sonra Python loader CLI cwd string eşitliği `runneradmin`/`RUNNER~1` yazım farkıyla durdu. Aynı test `Path.samefile` ile kimlik karşılaştırmasına geçirildi; engine/child/redaction kapıları korunur. Bootstrap reason corpus'una exact fallback ve UINT32_MAX assertion'ı eklenmiştir. Yeniden koşu bu iki son test değişimini de doğrular.


Son iki corpus düzeltmesi, ilk yayındaki kaynaklardan oluşturulmuş ayrı temiz checkout'ta dört standart Windows akışının tamamını geçti (x64: 6 native/79 managed/46 Python; x86: 9 native/79 managed/67 Python). Yayın sırasında ana çalışma klasöründe başlayan sonraki proxy geliştirmesi belge kapısını etkilediği için yayın doğrulaması bu ayrı checkout'ta yapıldı; eşzamanlı çalışma dosyaları korunur ve bu yayın commit'ine eklenmez. İlk ana-klasör belge kapısı hatası saklanmıştır.


## Doğrulanmış public yayın

Kaynak commit'i **`e5e2dd0d77e37447eca4b7e32c0b89306978c096`**: [Build başarı kaydı](https://github.com/saex-platform/saex/actions/runs/34747513672) ve [Documentation and source safety başarı kaydı](https://github.com/saex-platform/saex/actions/runs/34747513666).

| GitHub hosted kontrolü | Native suite | Managed/entegrasyon | Python |
|---|---|---|---|
| Windows x64 Debug | 6/6 | 79/79 | 46/46 |
| Windows x64 Release | 6/6 | 79/79 | 46/46 |
| Windows x86 Debug | 9/9 | 79/79 | 67/67 |
| Windows x86 Release | 9/9 | 79/79 | 67/67 |
| Linux x64 Debug / Ubuntu 24.04 | 4/4 | 79/79 | 37/37 |

Ayrı Documentation kontrolü 89 Markdown, 1191 yerel bağlantı, 6 JSON örneği ve 13 tooling testinde geçti. Secret scan başarılıdır. Yerel Gitleaks dört commit'li ana kaynak geçmişi ve iki commit'li topluluk geçmişinde sıfır bulgu verdi. Son kaynak manifest'i 213 dosya / 1.418.606 byte; GTA/build binary'si, submodule/symlink, büyük dosya veya özel kayıt e-postası içermez. Bütün engine sözleşmelerinin raw byte'ları ilk source commit'iyle aynıdır. Bu sayılar belirtilen kaynak commit'ine aittir; sonraki belge-only özet commit'inin boyutu farklı olabilir.

Her iki repo public, default branch `main`, merge yöntemi squash'tır. İki `main` için PR, çözülmüş konuşmalar ve linear history zorunludur; force push/silme kapalı ve kural yöneticilere de uygulanır. Tek bakımcı için zorunlu review sayısı sıfırdır. Ana depo strict base ile şu yedi check'i **GitHub Actions app 15368** kimliğine bağlı ister: Documentation, Secret scan, Windows x64 Debug, Windows x64 Release, Windows x86 Debug, Windows x86 Release, Linux x64 Debug. Owner organizasyon yönetimini korur; bu kurallar yeni kişilere erişim vermez.

İki depoda secret scanning/push protection açıktır. Ana depoda private vulnerability reporting ve read-only workflow token ayarı doğrulandı. GitHub community health API ana depo için 100 puan, MIT lisansı ve topluluk belgelerini tanıdı; bu puan uygulama güvenliği derecesi değildir. Hata/özellik formları ve gerçek `type:bug` etiketi tarayıcıda doğrulandı.

Organizasyon profilindeki banner ve belge bağlantıları görünür, `saex` ana repo sabitlenmiştir. Organizasyon Discussions, `saex` tartışmalarına bağlandı. [Public geliştirme panosu](https://github.com/orgs/saex-platform/projects/1) beş Türkçe durum sütunu, yedi açık iş, D1/D2/D3 milestone'ları içerir; yeni eklenen iş Planlandı durumuna geçer. D1/D2/D3 otomatik tamamlandı sayılmaz. Avatar varlığı kaynaklarda hazırdır; organizasyona profil resmi yükleme adımı tarayıcının yerel dosya erişim iznini bekler.

İsteğe bağlı N1 SDK hosted workflow'u tanımlıdır ve manuel çalışır; bu yayında hosted olarak çalıştırılmadı. Yukarıdaki kısa temiz checkout Debug/Release kanıtı kendi kapsamındadır. Oynanabilir release, installer, server deployment veya gerçek GTA başlangıcı bu kaynak yayınına dahil değildir.

## Kod 0.1.12 — Proxy dönüş sınırı

0.1.12 geliştirmesi Linux CI girişine proxy_policy --check ve portable policy corpus’unu ekler; Windows standart build aynı source gate’i ve x86 proxy fixture’ını kapsar. Bu yerel değişiklik yeni hosted workflow sonucu veya yayın/push kanıtı değildir. Önceki yayın ve CI kayıtlarının kapsamı korunur.

## Kod 0.1.13 — Startup çağrı sınırı

0.1.13 kaynak değişimi Linux CI girişine startup-policy generation check ve portable negatif corpus’u ekler. Windows standart build yeni x86 startup suite’ini kapsar. Yerel sonuçlar [raporda](d1-startup-call.md) tutulur; yeni hosted çalışma/push veya yayın kanıtı değildir. Önceki yayın/CI kayıtları korunur.

## Kod 0.1.13 kaynak birleştirmesi

0.1.12 ve 0.1.13 geliştirmesini içeren yerel `aede99f` commit’i, yayımlanmış `a0562ef` tabanıyla ayrı `feature/startup-call-observation` dalında birleştirildi. Önceki yerel commit tam Git bundle ve güvenlik dalıyla korundu. Yalnız ek belge bölümleri çakıştı; iki sürümün kayıtları korundu, kaynak dosyaları ve hash’e bağlı sözleşme byte’ları değiştirilmedi. Yayın mevcut PR ve yedi zorunlu kontrol akışını kullanır; bu birleştirme tek başına yeni hosted başarı kanıtı değildir.

### Kod 0.1.13 GitHub doğrulaması

[PR #9](https://github.com/saex-platform/saex/pull/9) kaynak commit’i `bd98c4a7746888c89c919312d8ea6de95bcd461c` için yedi zorunlu kontrol geçti: [Build](https://github.com/saex-platform/saex/actions/runs/34749019782), [Documentation and source safety](https://github.com/saex-platform/saex/actions/runs/34749019756).

| GitHub hosted kontrolü | Native suite | Managed/entegrasyon | Python |
|---|---|---|---|
| Windows x64 Debug | 6/6 | 79/79 | 60/60 |
| Windows x64 Release | 6/6 | 79/79 | 60/60 |
| Windows x86 Debug | 11/11 | 79/79 | 85/85 |
| Windows x86 Release | 11/11 | 79/79 | 85/85 |
| Linux x64 Debug / Ubuntu 24.04 | 4/4 | 79/79 | 51/51 |

Ayrı Documentation kontrolü gerçek `a0562ef` PR tabanına göre 46 değişmiş yol, 91 Markdown, 1254 yerel bağlantı, 6 JSON örneği ve 13 tooling testinde geçti; Secret scan başarılıdır. Ana çalışma klasöründeki x86 Debug birleştirme doğrulaması da 11 native suite/79 managed/85 Python ve aynı belge kapısıyla geçti. Altı commit’li yerel Git geçmişinde Gitleaks sıfır bulgu verdi. 229 tracked dosyada oyun/build binary’si ve çözümlenmemiş çakışma işareti bulunmadı; kaynak dosyaları `aede99f` ile aynıdır. Bu kanıtlar adı geçen kaynak commit’ine aittir; sonraki sonuç kaydı yalnız belgedir ve PR’ın güncel başı yine zorunlu kontrollerden geçer.

N1 SDK hosted işi ve gerçek GTA bu aktarımda çalıştırılmadı. Önceki gerçek GTA raporları kendi artifact kimlikleriyle korunur; proxy/startup wrapper gövdesi, dinamik SAEX DLL yüklemesi ve D1/D2 ürün kapıları bu CI sonucuyla kapanmaz.
