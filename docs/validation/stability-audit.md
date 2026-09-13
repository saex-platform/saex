# V0.5 stabilite ve kurtarma denetimi

Durum: 12 Eylül 2026, dokümantasyon düzeltme kaydı. [Ana indeks](../../README.md) · [Güncel inceleme sonucu](documentation-review.md) · [Kabul şartnamesi](scenarios.md)

## Sorumluluk ve inceleme kapsamı

V0.4'teki yaşayan dünya tasarımı; authority, zaman, durable commit, reload, replikasyon, transfer, bütçe ve restore açısından yeniden incelendi. Amaç daha çok özellik adı eklemekten önce arızada hangi state'in geçerli olduğunu belirlemektir. Bu kayıt statik sözleşme incelemesidir; kusursuz çalışan yazılım veya tamamlanmış güvenlik denetimi beyanı değildir. V0.2/v0.3/v0.4 kayıtları tarihsel kapsamlarıyla korunur.

## Bulgular ve düzeltmeler

| ID | Önceki açık / risk | Güncel sözleşme ve veri akışı | Kabul |
|---|---|---|---|
| F-01 | Lease expiry server tick'e bağlıydı; stall hak süresini uzatabilirdi | [Zaman](../architecture/time-fencing.md): monoton ControlTime; simülasyon zamanı ayrı | AC-71/72 |
| F-02 | Resume ve yerel watchdog saat farkı açık değildi | Domain-bound deadline, resume öncesi revoke, muhafazakâr watchdog; controller emniyeti ayrı kanıt | AC-72 |
| F-03 | Eski epoch sonucu reddi durable commit sonucunu da kapsayabilirdi | [Kurtarma](../architecture/failure-recovery.md): transient JobResult ayrı, core-owned CommitReceipt ayrı | AC-73 |
| F-04 | Commit cevabı kaybı “backend başarısız” olarak yorumlanabilirdi | OutcomeUnknown; aggregate fence, receipt sorgusu; timeout rollback kanıtı değil | AC-74 |
| F-05 | Tek writer mantıksal ilkeydi; ikinci process guard'ı eksikti | Backend WriterTerm guard; v1 otomatik failover yok, kesin eski-process duruşu gerekli | AC-75 |
| F-06 | Hayvan transferinde atomik/outbox seçenekleri destek sınırını belirsiz bırakıyordu | [Transfer](../networking/consistency-recovery.md): tek core/backend; canonical location ve TransferGeneration | AC-76/77 |
| F-07 | Precommit reservation TTL, uncertain commit sonrası yanlış serbest bırakılabilirdi | Phase-aware expiry ve restart'ta persistent occupancy'den kota kurma | AC-87 |
| F-08 | Baseline sayfası bütünlüğü ve kesit karışması yeterince somut değildi | BaselineId, cut, descriptor, digest, byte sınırı ve atomic staging | AC-78 |
| F-09 | Delta tabanı ve transport ACK / applied ACK ayrımı eksikti | Exact reliable base; bağımsız absolute motion örneği; retained history yoksa refresh | AC-79 |
| F-10 | Sürekli değişen dünyada baseline reset'i sınırsız döngüye girebilirdi | Peer/global budget, deneme deadline'ı, bounded restart ve minimum critical scope | AC-80 |
| F-11 | Sessiz logical sapma ve yanlış native binding farklı teşhis edilmemişti | Yetkili view digest repair; native binding kontrolü ayrı, fence ve rebind | AC-81 |
| F-12 | Worker restart ve completion kuyruklarının toplam baskısı tam tanımlı değildi | Circuit breaker, process bütçesi ve core receipt recovery; admission öncesi kapasite | AC-82/83 |
| F-13 | UTC sıçraması offline yaş/üreme penceresini bozabilirdi | Negatif/büyük fark politikası, bounded pencere ve durable OperationId | AC-84 |
| F-14 | Aktif pin/son sürümler retention'ı eski yedeği veya offline hayvanı kurtarmayabilirdi | Backup/snapshot/operation closure GC kökü; exact schema/controller pin | AC-85 |
| F-15 | Sayaç wrap ve shutdown'da belirsiz işlem sonucu açık değildi | Checked sayaç/epoch değişimi; drain sonucu unresolved olarak restore'a taşınır | AC-86 |
| F-16 | Stabiliteyi kanıtlayacak ortak arıza profili eksikti | [Performans](performance.md) ve conformance içinde fault soak, recovery ölçümü, kapsamlı pass/fail | AC-88 |

## Arayüzler ve belge sahipliği

Zaman ve deadline'ın normatif sahibi `time-fencing`; health/receipt/writer/queue/backup sahibi `failure-recovery`; baseline repair ve entity transfer sahibi `consistency-recovery` belgesidir. Authority, activation, execution, persistence, resource ve gameplay belgeleri bu tanımlarla uyumlu kılınmıştır. Yeni şartlar ADR-29–32, R-21–23, AC-71–88 ve fikir kapsamındaki ST-01–08'e bağlanır. F-01–16 bu belgenin bulgu kimlikleri, ST-01–08 kapsam tablosunun gereksinim kimlikleridir.

## Hata davranışı ve genişleme ölçütü

Varsayılan çizgi şudur: bilinmeyen commit sonucu tekrarlanmaz; güvenilmeyen writer devralınmaz; eksik critical state oynanışa açılmaz; aşırı yükte yeni admission kısılır; sahipli birey kota için silinmez. Bunun bedeli bazı arızalarda scope/dünya duruşudur. Bu kullanılabilirlik sınırı belgelenmiş bir seçimdir; mevcut native motor üzerinde hiç durmadan kusursuz kurtarma garantisi değildir.

Yeni feature/provider ekleyen kişi zaman alanı, admission bütçesi, authoritative aggregate, OperationId/receipt gereksinimi, native fallback, recovery deadline ve conformance kapsamını doldurur. Bütün kanalların açıldığı nüfus/hayvan profili, sade yarış profilinden ayrı ölçülür. Sadece bir senaryonun geçtiği capability bütün feature ailesini açmaz.

## Kanıt ve kabul sınırı

S-28–31 zaman ve transaction/lock davranışları için bu revizyonda okunan birincil kaynaklardır. SAEX state machine, fencing, recovery ve transfer tasarımları bu kaynaklardan yapılan tasarım çıkarımlarıdır. Önceki MTA/GNS kaynak kodu incelemesi korunmuştur; bu revizyonda yeni binary çalıştırılmadı veya rakip kapasite testi yapılmadı.

Doküman bağlantısı, JSON, Mermaid ve kayıt kimliği kontrollerinin kesin sonuçları [inceleme raporunda](documentation-review.md) tutulur. AC-01–88 gelecekte çalıştırılacak şartnamedir; burada testlerin geçtiği iddia edilmez. R-21–23 başarısızsa ilgili recovery/transfer capability kapalı kalır. “Kusursuz” sözcüğü yerine hangi arıza koşulunda neyin korunduğu ve hangi sınırın henüz kanıtlanmadığı izlenir.
