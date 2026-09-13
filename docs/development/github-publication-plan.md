# SAEX GitHub organizasyonu ve ilk yayın planı

Tarih: 13 Eylül 2026. Durum: **onaylanan ilk kurgu; tarihsel plan**. Kullanıcı sonrasında uygulanmasını onayladı. Güncel gerçekleşen durum [yayın raporundadır](github-publication.md); aşağıdaki inceleme anı bulguları ve öneriler tarihsel olarak korunur. Kullanıcı önce kurguyu istedi, ilk yayının public olmasını seçti ve marka açılımının iyileştirilmesini istedi. Bu belge kaynak yayınını planlar; D1/D2 ürün kapılarını kapatmaz. [Uygulama durumu](status.md) · [Yol haritası](../roadmap.md) · [Derleme akışı](workflow.md) · [Değişiklik kaydı](change-log.md)

## 1. Marka ve sahiplik

- Marka: **SAEX**.
- Açılım: **San Andreas Extended**. Extended doğru İngilizce yazımdır; teknik kimliklerde `SAEX`, `Saex` ve `saex` mevcut kullanımları korunur.
- Türkçe kısa tanım: **GTA: San Andreas üzerinde kendi dünyanı ve oyun deneyimini geliştir.**
- Organizasyon görünen adı: **SAEX — San Andreas Extended**.
- Önerilen GitHub adres adı: **`saex-platform`**.
- İlk yayın görünürlüğü: **public**, kullanıcı tarafından seçildi.
- Organizasyonun Owner rolü: mevcut doğrulanmış kullanıcı **`Rohatcengizhanbucak`**. Repository yönetimi, üyelikler ve yayın kararları bu hesapta kalır; otomasyon hesabına organizasyon sahipliği verilmez.

13 Eylül sorgusunda `saex` ve `saex-dev` mevcut kişisel hesaplara aitti; `saex-platform` için GitHub API 404 döndürdü. Bu, adın rezerve edildiği veya oluşturulabileceğinin garanti olduğu anlamına gelmez. Oluşturma ekranındaki doğrulama son belirleyicidir. SAEX ile ilgisiz mevcut organizasyonlar yeniden adlandırılmaz veya taşınmaz.

Geliştiriciye geniş kontrol hedefi; dünya, resource, oyun kuralı, asset ve UI tanımlarını kapsayan ürün vizyonudur. Bugünkü kodun bütün GTA işlevlerine eriştiği, native adresleri serbestçe çağırdığı veya production sandbox sunduğu iddia edilmez. [Native sözleşme](../architecture/native-sdk-integration.md), C++20 core, private x86 SDK sınırı, GNS oyun taşıması ve HTTPS içerik kararı korunur. C# resource güvenliği ayrıca kendi D1/R kapılarını bekler.

## 2. Openmultiplayer'dan alınan düzen

[Open Multiplayer organizasyonunda](https://github.com/openmultiplayer) ana proje `open.mp`, `launcher`, `open.mp-sdk` ve `web` öne çıkarılıyor. SAEX için alınan model; tek marka, açıklayıcı organizasyon profili, sorumluluğu belli depolar ve kolay bulunan başlangıç belgeleridir. Openmultiplayer'a ait kod, görsel kimlik veya ürünün hazır olma iddiaları aktarılmaz.

SAEX'in mevcut ortak şeması, generator'ları, C++/C# fixture testleri ve kaynak-belge eşlemesi aynı checkout'u kullanıyor. Bunları bugün ayrı core/client/server/sdk depolarına taşımak tek bir değişikliği birden fazla sürüm ve yayına bağlar. **İlk yayın: bir organizasyon, iki depo, tek kaynak monorepo.**

```text
saex-platform/                 Önerilen organizasyon
├── .github                    İlk yayın: profil ve ortak topluluk belgeleri
└── saex                       İlk yayın: mevcut kod, test, sözleşme ve belgeler

Bağımsız ürünler oluştuğunda:
├── sdk                        Sürümlü geliştirici SDK'sı
├── launcher                   Çalışan launcher ve güncelleme istemcisi
├── web                        Web uygulaması ve hizmetleri
└── examples                   Çalışan SDK/resource örnekleri
```

| Depo | İçerik ve kaynak | Ne zaman açılır? |
|---|---|---|
| `.github` | `profile/README.md`, ortak katkı, destek, davranış ve güvenlik yönlendirmeleri | İlk public yayın |
| `saex` | Mevcut `contracts`, `include`, `src`, `managed`, `tests`, `tools`, `samples`, `docs` ve build dosyaları | İlk public yayın |
| `sdk` | Kararlılaşmış C# geliştirici arayüzleri, paketleme, API referansı ve uyumluluk testleri | Monorepodan bağımsız build/test ve sürümlü tüketim gösterilince |
| `launcher` | Kurulum/başlatma/güncelleme arayüzü ve doğrulanmış dağıtım akışı | Gerçek launcher kesiti ve kendi testleri oluşunca |
| `web` | Web uygulaması ve gerekiyorsa servisleri | Gerçek web kodu ve kendi işletim ihtiyacı oluşunca |
| `examples` | Sürümlü SDK'yı tüketen çalıştırılabilir örnekler | Çalışan resource/API mevcut olunca; bugünkü sentetik fixture buna örnek gösterilmez |

İlk profilde ana `saex` deposu sabitlenir; katkı deposu gerçekten giriş noktasıysa `.github` da eklenir. Sonraki ürünler içerikleri oluşunca öne çıkarılır. GitHub organizasyon profili, public `.github` deposundaki `profile/README.md` üzerinden oluşturulur ve en fazla altı depo sabitlenebilir. [GitHub profil belgesi](https://docs.github.com/en/organizations/collaborating-with-groups-in-organizations/customizing-your-organizations-profile).

Dokümanın tek yetkili kaynağı ilk aşamada `saex/docs` olur. Ayrı `docs` deposuna kopya tutulmaz. Gezilebilir bir doküman sitesi istenirse aynı kaynaklardan üretilir; bu plan bir web sitesi yayını gerçekleştirmez.

## 3. GitHub'da görülecek giriş metni

Organizasyon profili için önerilen metin:

> **SAEX — San Andreas Extended**
>
> GTA: San Andreas üzerinde kendi dünyanı ve oyun deneyimini geliştir.
>
> SAEX; geliştiricilerin dünya, oyun kuralları, içerik ve arayüzlerini tanımlayabilmesi için geliştirilen modüler bir platformdur. C++ çekirdek ve C# araçları üzerine kuruludur.
>
> Proje D1 temel geliştirme aşamasındadır. Çekirdek bileşenler ve doğrulama araçları mevcuttur; oynanabilir multiplayer sürümü henüz yayımlanmadı.
>
> Başlangıç noktaları: kaynak kod, derleme rehberi, uygulama durumu, mimari ve yol haritası.

Bu bağlantılar yalnız gerçek depolar oluşturulduktan sonra doğru hedeflerle eklenir. Sahip olunmayan alan adı, çalışan indirme sayfası veya kullanılmayan Discord/e-posta yazılmaz. Avatar ve sosyal paylaşım görseli için özgün SAEX marka varlığı hazırlanması sonraki uygulama işidir.

Ana README'nin ilk ekran sırası: marka → kısa amaç → gerçek D1 durumu → doğrulanmış kurulum komutları → mevcut bileşenler → hedeflenen özellikler → katkı/doküman bağlantıları. Uzun mimari sürüm geçmişi mevcut değişiklik kaydından takip edilecek şekilde sadeleştirilir; kanıt ve eski açıklamalar silinmez. Başarılı CI rozeti ancak ilgili workflow gerçekten çalıştıktan sonra gösterilir. Kod ve mimari sürümü ayrı kalır: mevcut dosyalarda kod **0.1.11**, mimari **v0.18**.

## 4. Yerel başlangıç bulguları

İnceleme başlangıcında aşağıdaki durum doğrudan çalışma alanı ve GitHub CLI üzerinden gözlendi; yayın anında tekrar ölçülür:

| Alan | Bulgular | Yayına etkisi |
|---|---|---|
| Git geçmişi | `master` üzerinde henüz commit yok; tracked dosya yok; remote yok | İlk kaynak commit'i hazırlanmalı, hedef branch `main` olmalı |
| Dosya kapsamı | Ignore kuralları sonrası 188 aday dosya, yaklaşık 1,26 MiB; 78 Markdown | Küçük bir kaynak deposu; bu ilk envanter plan belgesi eklenmeden alındı |
| Çıktılar | `out/`, .NET `bin/obj`, `artifacts/local` ignore kapsamında | Build, araştırma ve yerel GTA deney kopyaları Git'e eklenmez |
| GitHub erişimi | Bağlı hesap ve yerel `gh` hesabı aynı kullanıcı | Mevcut hesapla sonraki kurulum yapılabilir; yeni organizasyon henüz yok |
| Mevcut SAEX hedefi | CLI'nin erişebildiği owned/member depolarda adı SAEX/gtasaonline ile eşleşen hedef görülmedi | Yeni hedef oluşturulurken çakışma yeniden kontrol edilir |
| Yayın dosyaları | `.github` workflow/şablonları ve kök `LICENSE` yok | CI, katkı ve lisans dosyaları hazırlanmalı |
| Kişisel yol | `workflow.md` içinde bu cihaza ait absolute Python yolu var | Public rehberde taşınabilir Python keşfi/parametresi anlatılmalı; kanıtın bağlamı korunmalı |
| Sınırlı sır taraması | Seçilmiş yaygın token/private-key imzalarında eşleşme görülmedi | Bu bir kapsamlı secret scan değildir; kesin staging içeriği ayrıca taranmalı |
| Sürüm kanıtı | Status belgesinin 0.1.11 bölümünde final doğrulamanın sürdüğü yazıyor | Sonuçlar exact yayın adayı üzerinde tamamlanmalı; eski 0.1.10 sonuçları devralınmamalı |

GitHub connector'ının organizasyon sorgusu boş döndü; CLI aynı hesap için mevcut organizasyon üyeliklerini gösterdi. Bu nedenle connector'ın boş yanıtı “hesabın organizasyonu yok” sonucu olarak kullanılmadı. Hiçbir mevcut organizasyon bu planın hedefi yapılmadı.

## 5. Yayın hazırlığında eklenecek dosyalar

`saex` için planlanan ekler:

- `.github/workflows/ci.yml`: mevcut Windows build girişini çalıştıran CI.
- `.github/workflows/docs.yml`: bağlantı, JSON, kaynak-belge eşlemesi ve yeni yayın belgelerinin denetimi.
- `.github/ISSUE_TEMPLATE/bug_report.yml` ve `feature_request.yml`: sürüm, platform, beklenen/gözlenen sonuç, yeniden üretim ve ilgili AC/R kapsamı.
- `.github/ISSUE_TEMPLATE/config.yml`: güvenlik açığı ve destek için gerçek yönlendirmeler.
- `.github/pull_request_template.md`: davranış, belge sahipleri, doğrulama ve kalan sınır.
- `.github/CODEOWNERS`: başlangıç sorumlusu mevcut sahip; ekipler ancak gerçekten oluşturulursa kullanılır.
- `LICENSE`, `THIRD_PARTY_NOTICES.md`, `CONTRIBUTING.md`, `SECURITY.md`, `SUPPORT.md`, `.gitattributes`.

`saex` katkı rehberi build/doc gate'e özel olur; organizasyon `.github` deposu diğer depolar için genel belgeleri sağlar. Ortak belgelerin GitHub'da görünmesi, onların clone veya release paketine eklendiği anlamına gelmez. `LICENSE` her ilgili depoda ayrıca bulunur. [GitHub ortak topluluk dosyaları](https://docs.github.com/en/communities/setting-up-your-project-for-healthy-contributions/creating-a-default-community-health-file).

**Lisans önerisi: SAEX'e ait özgün kaynaklar için MIT.** Geniş kullanım ve değişiklik özgürlüğü hedefiyle uyumludur; ticari kullanım dahil izinleri ve bildirim koşulu vardır. Bu öneri henüz uygulanmış veya kullanıcı tarafından seçilmiş lisans değildir. İlk public kaynak aktarımından önce kesinleştirilir. [MIT metni ve özeti](https://choosealicense.com/licenses/mit/).

Vendor dosyaları SAEX lisansıyla yeniden etiketlenmez. [Mevcut N1 notice envanteri](d1-native-dependency.md) ve [exact dependency lock](../../contracts/engine/plugin-sdk.lock.json) üzerinden kullanılan kaynak/patch ve dağıtılan içerik ayrımı korunur. GTA executable, oyun asset'leri, üçüncü taraf DLL/ASI'ler ve araştırma cache'i kaynak deposuna veya release'e dahil edilmez. Public görünürlük tek başına açık kaynak kullanım lisansı oluşturmaz. [GitHub lisans rehberi](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository).

Yeni workflow/config ve extension'sız dosyalar mevcut `check_docs.py` açısından eşleme gerektirir. Uygulama sırasında `component-map.json` ilgili belge sahipleriyle genişletilir; bilinmeyen yolları yok sayarak gate atlatılmaz. Eşleme değiştiğinde mevcut negatif tooling testleri ve standart build tekrar doğrulanır. `.gitattributes` eklenecekse satır sonu farkı incelenir; ilgisiz toplu kaynak yeniden yazımı yapılmaz.

## 6. CI ve inceleme düzeni

| Kontrol | Planlanan çalışma | Başarı sınırı |
|---|---|---|
| Windows core/araç | X64 ve x86, Debug ve Release; `tools/build.ps1` | Exact commit üzerindeki native, managed, generator, audit ve Python kontrolleri |
| Doküman | Yerel link/JSON ve kaynak→belge eşlemesi | Yapısal doğrulama; anlamsal ve GTA davranış kanıtı ayrı |
| İlk temiz checkout | Source dışı yerel cache olmadan build | Kullanıcının makinesine/kurulum yoluna örtük bağımlılık olmadığı |
| Native dependency | Ayrı opt-in N1 edinme/verify/build/probe işi | Pinli seçilmiş SDK alt kümesi; standart build bunu geçmiş sayılmaz |
| Linux | Önce portable CMake/Ninja ve managed fixture işinin uygulanıp çalıştırılması | İlk gerçek başarıya kadar Linux doğrulanmış gösterilmez; Windows script'i Linux'ta çalışır varsayılmaz |
| Kaynak yayını | Exact staging/commit dosyalarının secret, boyut, dosya türü ve notice kontrolü | Yalnız taranan aday; bütün yerel disk veya GTA kurulumu denetlenmiş sayılmaz |

CI, repodaki VS2022 preset'ine uygun C++ workload, Python 3 ve `global.json` ile uyumlu .NET SDK'yı açık kurar/seçer; runner'da hazır bulunduğu varsayılmaz. CMake/compiler/runtime sürümleri loglanır. Workflow action'ları doğrulanmış commit SHA'larına sabitlenir. Public PR build'leri read-only token ile çalışır; güvenilmeyen PR koduna yayın yetkisi veya kişisel runner erişimi verilmez. Yayın yetkisi ayrı ve yalnız gereken job kapsamındadır.

PR belge farkı gerçek base commit'e göre `-DocumentationBase`/`--base` ile ölçülür. Checkout gerekli Git geçmişini getirir. İlk push'ta geçmiş commit yoksa sıfır SHA geçerli referans gibi geçirilmez; mevcut ilk-envanter kontrolü çalışır. Sonraki push ve PR bazları ayrı belirlenir. Başarılı yerel build hash kaydı, GitHub üzerinde önceki commit'e karşı belge eşlemesinin yerine kullanılmaz.

`main` varsayılan dal olur; kısa ömürlü feature/fix/docs branch'leri PR ile birleşir. Merge yöntemi squash; konu konuşmalarının çözülmesi ve geçen required checks aranır. Force push ve branch silme kapalı tutulur. Tek bakımcı varken sahibin kendi PR'ını onaylamasını gerektiren review kuralı konmaz; ikinci gerçek bakımcı geldiğinde review şartı eklenir. Required check isimleri ancak ilk workflow koşusunda gerçek isimleri görüldükten sonra bağlanır. [GitHub protected branches](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches).

Owner kontrolü korunur; otomasyon yalnız tanımlı işini yapar. Sıradan katkıcılar tüm organizasyonun yöneticisi olmaz. Etiket aileleri `type:bug`, `type:feature`, `type:docs`, `area:core`, `area:engine`, `area:tools`, `area:docs`, `stage:D1`, `stage:D2` şeklinde kurulabilir. Issue şablonlarının kullandığı etiketler ilgili depoda gerçekten oluşturulur.

## 7. Yol haritası ve sürüm yayını

Proje panosu için sütunlar: **Planlandı → Çalışılıyor → İncelemede → Doğrulama bekliyor → Tamamlandı**. Milestone'lar mevcut D1/D2/D3 kapılarına ve gerekirse D1-N2/N3 alt kesitlerine bağlanır. D1 için kapatılmış bir alt görev D1 milestone'unu otomatik kapatmaz. AC/R maddeleri issue açıklamasından yetkili dokümana bağlanır; bütün araştırma kayıtları topluca tamamlanmış işlere çevrilmez.

İlk yayın kaynak deposunun açılmasıdır. Kod numarasının 0.1.11 olması otomatik olarak `v0.1.11` tag'i, oynanabilir indirme veya stable release oluşturmaz. Önce exact kaynak commit'i doğrulanır. Uygun görülürse daha sonra sınırlı foundation/developer **prerelease** hazırlanır; paket içeriği, commit, SHA-256, platform/toolchain ve doğrulama sınırı birlikte verilir. Native artifact dağıtımı için notice ve release koşulları ayrıca tamamlanır.

Bugün var olmayan player client, server, production SDK veya launcher indirmesi eklenmez. [D1/D2 yolu](../roadmap.md), [durum tablosu](status.md) ve [kabul senaryoları](../validation/scenarios.md) yayın dilinin kaynağıdır.

## 8. Uygulama sırası ve tamamlanma ölçütleri

1. **Yerel hazırlık:** marka/README ve taşınabilir rehberi düzenle; lisans seçimini kesinleştir; topluluk/CI dosyalarını ve belge eşlemelerini birlikte hazırla. Hedef organizasyon adını oluşturma anında tekrar doğrula.
2. **Koruma ve envanter:** mevcut uncommitted kaynakları ve Git metadata'sını yerel yedekle koru. Yedek yeni remote'a aktarılmaz. GTA/out dosyalarını kaynak staging'ine dahil etmeden gerçek manifest, boyutlar ve hash'leri üret. Commit yazar bilgilerini kullanıcının GitHub kimliğiyle doğrula; kişisel e-posta tahmin etme.
3. **Yerel yayın adayı:** staged diff'i kontrol et; kaynak/sır/notice kontrollerini tamamla; `main` üzerinde ilk yerel commit'i oluştur. Ayrı geçici temiz checkout'ta Windows x64/x86 Debug/Release standardını çalıştır; son 0.1.11 kayıtlarının başarılı/başarısız/çalıştırılmamış durumunu gerçek sonuçlarla tamamla. Son belge değişikliklerini de aday commit'e dahil et; kaynakta düzeltme varsa etkilenen kontrolleri tekrar çalıştır.
4. **Public GitHub kurulumu:** kullanıcıya ait yeni organizasyonu ve `.github`/`saex` public depolarını oluştur. Yerel geçmiş aktarılacak `saex` uzak deposuna otomatik README/license commit'i ekleme; ilk geçmiş çakışması yaratma. Hedef owner/repo ve public ayarını doğrulayarak normal push yap.
5. **GitHub doğrulaması:** yerel ve remote `main` SHA'larını karşılaştır; dosya kapsamı/clone, GitHub CI, README bağlantıları, public profil ve depoları doğrula. İlk sonuçlardan sonra branch kuralları, required checks, CODEOWNERS, etiket ve şablonları uygula. GitHub CI hatası varsa düzeltme commit'iyle gider ve tamamlandı ilanını beklet.
6. **Teslim:** gerçek organizasyon/depo bağlantıları, commit SHA, geçen platformlar ve kalan ürün kapılarıyla sonuç ver. Source publication ile playable release ayrı durumlar olarak kaydedilir.

Mevcut dosyaları silen Git cleanup, force push, unrelated remote değişimi, oyun dosyası dağıtımı veya server restart bu adımların parçası değildir. Public yayın hazırlığı başarısızsa eksik kontrol görünür kalır; sırf başarı rozeti için zorunlu adım kaldırılmaz.

**Bu planın teslimi:** yerel kurgu ve bağlantılı README/status/roadmap/change-log güncellemesi. Organizasyon, remote, commit, push, lisans uygulaması, workflow veya release bu planlama turunda oluşturulmadı. Uygulama aşamasında ilk yayın public tercihi yeniden sorulmaz; ad ve lisansın henüz kesinleşmeyen kısmı bu plan üzerinden netleştirilir.

Planın yerel doğrulaması: `tools/check_docs.py` 79 Markdown, 1100 yerel bağlantı, 6 JSON örneği ve sıfır hata verdi; son başarılı yerel hash kaydına göre beş belge değişti. `--record` kullanılmadı. Son envanter 189 aday dosya, sıfır tracked dosya ve sıfır remote; branch hâlâ commitsiz `master`. Bunlar belge teslimi kanıtıdır; kaynak build'i, GitHub CI veya public aktarım kanıtı değildir.
