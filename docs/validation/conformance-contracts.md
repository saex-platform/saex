# Engine, SDK ve içerik uygunluk sözleşmesi

Durum: ADR-24 kapsamında kanıt taslağı; aşağıda test çıktısı yoktur. [Senaryolar](scenarios.md) · [Araştırmalar](../decisions/research-register.md)

## Sorumluluk

“Model yüklendi”, “DLL çalıştı” veya “test sunucusu açıldı” sonuçları yeterli ürün destek kaydı değildir. ConformanceProfile, belirli bir capability'nin hangi engine/build/artifact/schema koşullarında, hangi sonuçla sınandığını belirtir. WorldPlan üretim eligibility'si bu kanıtla değerlendirilir.

| Kayıt | Kapsam |
|---|---|
| EngineProfile | Gerçek executable fingerprint, adapter build, OS ve native sınıf |
| ContractProfile | ContractDigest, SDK/core sürümü ve decoder sınırları |
| ContentProfile | Artifact/cook/skeleton/collision/projection uyumluluğu |
| EnvironmentProfile | Hardware, driver, grafik ayarı, network ve bütçe |
| EvidenceRecord | AC/R kimliği, giriş, expected/observed, log ve pass/fail/unsupported |

`declared`, `experimental`, `verified` ve `unsupported` durumları ayrıdır. Aynı capability adı başka GTA sürümünde veya farklı native object sınıfında otomatik verified olmaz. İmza kanıtın hangi yayıncıdan geldiğini gösterebilir; kötü veya eksik test metodunu doğru yapmaz. Operator hangi kanıt yayıncısını kabul ettiğini policy'de seçer.

## Test katmanları

1. **Static:** Schema/WorldPlan/composition/link/closure doğrulaması. Native davranış göstermez.
2. **Contract:** C++/C# serialization corpus, size/revision/grant ve fake clock testleri.
3. **Headless simulation:** İş sırası, idempotency, journal restore ve projection invalidation; GTA renderer/physics kanıtı değildir.
4. **Native conformance:** Gerçek istemcide create/apply/retire, collider query, animation, input ve pool baskısı.
5. **End-to-end:** İki aktif client + late join, kayıtlı N0/N1/N2 koşulları, hata enjeksiyonu.
6. **Scale / soak:** P1/P2, resource sayısı, GC/IPC ve uzun oturum; gerçek client ve bot sonuçları ayrılır.

Her release değişiklik kapsamına göre ilgili testleri seçer; geçmiş sonucun tekrar kullanılabilmesi fingerprint/schema/parametre kapsamının değişmemiş olmasına bağlıdır. Değişiklik etkisi belirsizse sonuç yeniden ölçülür. Bütün koşuları gereksiz tekrarlamak yerine bağımlılık izine dayalı seçim yapılır.

## Referans fixture paketi

V0.7 native SDK EvidenceRecord'u; `EngineProfileId`, exact executable hash'i, `DependencyLockDigest`, adapter artifact hash'i/API major, bootstrap/loader ve Host build'i, operation/native sınıf ve ilgili asset/ortam kimlikleriyle sınırlar. Kaynak pininin incelenmesi `verified` engine sonucu değildir. Makine lock'u/kanıt şeması ilk native kesitte uygulanacaktır; bugünkü foundation bu alanları doğrulayan bir registry içermez. [Tam sözleşme](../architecture/native-sdk-integration.md).

AC-89–96 static/build, contract ve gerçek native katmanlarını ayrı ister. Callback remove, hook restore, temiz process stop ve dinamik DLL unload ayrı gözlemlerdir. SDK/patch/recipe/engine değişiminde eski evidence kapsamı yeniden denetlenir; yalnız capability adı veya sürüm metni eşleşmesiyle test taşınmaz. Dynamic unload desteklenmiyorsa bu sonuç gizlenmeden raporlanır; session/process stop yine ölçülür.

Harbor için cam, kasa, çit, lamba, parçalı duvar; kapı/portal, basit NPC ve araç temas fixture'ları tasarlanır. Her birinde sağlam/hasarlı/kırık/onarım state'i, beklenen collider sorgusu, yeni katılım sonucu ve kalıcı kayıt vardır. Kaynak ve dağıtım hakkı uygun özel içerikle başlanır.

Yeni prefab'ın model türü, skeleton, fizik materyali veya controller gereksinimi mevcut kanıtı aşarsa profile ek deney gerekir. QualityVariant testi yalnız screenshot farkı değildir: geçiş engeli, görüş avantajı ve hit proxy eşdeğerliği denetlenir. Özel shader/native parser maliyetine karşı tek “yüklenebilir” boolean yeterli değildir.

## Invariant'lar ve neden zinciri

| Invariant | Gözlenecek kanıt |
|---|---|
| Tek durable sonuç | Actor/OperationId başına tek receipt ve domain sonucu |
| Tek geçerli yazar | Mutator/grant, aggregate reservation ve owner epoch izi |
| Doğru ilk etkileşim | JoinApplied anında gereken catalog/collider/state |
| Eski sonucu reddetme | Job/read revision, entity generation, projection generation |
| Kaynakları geri bırakma | Lease registry → retire → native ref/fence tamamlanması |
| Private veriyi taşımama | Alıcı filtresi ve gerçek wire/client snapshot kontrolü |
| Testten üretime etki olmaması | Test namespace, broker/OS ret ve dış servis fixture log'u |

`Explain` zinciri ReleaseId → WorldPlanDigest → ContractDigest → OperationId/TransactionId → StateRevision/ProjectionResult → client apply → native handle olarak ilerler. Uzun trace retention'ı sınırlıdır; private alanlar ve sırlar kaydedilmez. Sunucuda aynı revision olması client'ın collider'ının doğru olduğunu tek başına kanıtlamaz; native query kanıtı da gerekir.

## Kabul, arıza ve genişleme

ConformanceReporter girdisi fixture sonucu, çıktısı sürümlü EvidenceRecord'dur. CapabilityRegistry üretim profiline yalnız policy'nin kabul ettiği ve kapsamı eşleşen kanıtı açar. Fail veya unsupported sonuç “uyarıyla geçti” yapılmaz; ilgili world feature kapanır veya dünya deneysel profile alınır.

Yeni adapter/SDK aynı fixture sözleşmesini uygulayabilir. Headless runner için başarı, native katmanın eşdeğerliğini ispatlamaz. AC-44 eski build kanıtının yeni fingerprint'e taşınmasını reddeder; AC-34/35/41 ve R-13–16 v0.3 kapsamını doğrular. D0'da yalnız Markdown/örnek/diyagram kontrolü vardı; [D1 foundation](../development/d1-foundation.md) native core/managed fixture alt testlerini ekledi. Bu sonuçlar tam CapabilityRegistry veya gerçek GTA conformance implementasyonu değildir.

## D1-N1 build evidence alt kümesi

[Native dependency raporu](../development/d1-native-dependency.md), ilk gerçek source/patch/recipe kilidi ve x86 compile/link kanıtını tanımlar. EXE/.lib/PDB hash'leri, DependencyLockDigest, compiler/SDK/CRT/packing ve testler opt-in script sonunda kaydedilir; `gtaEligible=false` zorunludur. 17 dosya/edinim testi, iki gerçek CMake ret testi ve bir SDK probe'u, AC-89/R-02a'nın seçilmiş build alt kümesidir. C++20 başarısız deneyi ve ADR-36 private C++23 kararı saklanır. Dosya doğrulaması native hook, ABI export, OS sandbox veya CapabilityRegistry çalışması sayılmaz; yeni native kaynak bu kapsamın yeniden değerlendirilmesini ister.

## Population ve creature kanıt kapsamı

Yeni fixture ailesi pedestrians/driver/aircraft ve ground creature içerir. EngineProfile'e controller/native sınıf, ContentProfile'e species/breed/morphology/rig/animation/hit/nav digests, EnvironmentProfile'e kanallar/kota/yerel agent karışımı eklenir. Aynı canine görünümünün testi başka rig, beden ölçeği, uçuş veya mounting desteğine aktarılmaz.

AC-46–70 contract/headless/native/end-to-end katmanlarına [entegrasyon kaydında](population-animal-integration.md) bağlanır. Static Species manifest geçişi yalnız closure/metadata kanıtıdır; AC-50/51/52/55/56/57/58/64/67/70 için gerçek native client sonuçları gerekir. Protected drain, tek sahiplenme/doğum/ödül ve eski task reddi server/native log/query ile birlikte sınanır. R-17–20 alt capability'ler ayrı declared/experimental/verified/unsupported kalır.

## Zaman ve recovery kanıtı

EvidenceRecord v0.5 kapsamında RecoveryProfileDigest, clock provider/OS suspend semantiği, persistence provider ve flush/WriterTerm garantisi, fault profile/seed/timeline, invariant ihlali, unavailable scope süresi, queue peak ve recovery sonucunu taşır. RPO/RTO failure türüne bağlanır. Passed baseline logical hash testi native collider query testini geçirmiş sayılmaz; doğru clock okuması da controller resume emniyetini kanıtlamaz.

AC-71–88 ve R-21–23; headless registry/codec/backend fixture'ları ile gerçek GTA adapter/controller ölçümlerini ayırır. Failure injection yalnız izole dünyada ve test build'inde çalışır. Yeni provider bu arayüz/hata sınıflarını doldurmadan verified profile giremez. [Stabilite denetimi](stability-audit.md), [performans/fault ölçümü](performance.md).

## D1-N2 gözlem kanıtının kapsamı

[N2 raporu](../development/d1-engine-preflight.md), `scope=non-executing-image-observation` ile exact file hash, profil JSON digest'i, pointer genişliği, header ve dört anchor eşleşmesini kaydeder. Bu yerel teşhis JSON'u tam EvidenceRecord/CapabilityRegistry implementasyonu veya gerçek GTA mapped-image/ABI kanıtı değildir. `canAttach=false` zorunlu kalır. PE corpus, aynı marker/yanlış hash CLI testi, bounded reader retleri ve managed gömülü kaynak eşleşmesi AC-90'ın alt testleridir. Gerçek process'in unpack/başlatma fazı ve SDK bağımsız modül load denetimi tamamlanmadan R-01a/b kapanmaz; test sonuçları ayrı scope'larda tutulur.

## D1-N2 yüklenebilir modül alt kanıtı

[Bootstrap raporu](../development/d1-bootstrap-module.md) `non-game-bootstrap-module` scope'unda gerçek OS load/export/stop/unload testini, ayrı `bootstrap-artifact-audit` scope'unda DLL/map hash ve imports/exports/TLS/CRT kontrolünü kaydeder. X86 test host'u GTA değildir ve file_rejected üretir. 100 warm lifecycle çevriminde process handle sayısı her çevrimde eşit olmalıdır; cold/load-only/ilk-inceleme değerleri saklanır, ilk artış gizlenmez. Bu kontrol memory/native hook quiescence veya bütün process için sıfır leak iddiası değildir. Portable fake observation başarısı da gerçek GTA process yolunu geçmiş sayılmaz. AC-90/R-01b alt kapsamı; N3/AC-91 kapısı açık.

## İlk create-debug olayı scope'u

Kod 0.1.4 [observer raporu](../development/d1-suspended-process.md), created-suspended-process-observation scope'unda file SHA-256/profil digest, observer pointer genişliği, owned child PID/base, event file identity, image eşleşmesi, primaryThreadResumed=false ve childExitConfirmed alanlarını tanımlar. Exit 3 yalnız eşleşme ve doğrulanmış kapanış içindir; cleanup belirsizliği exit 1 verir. Standard native canary/cleanup testleri kendi executable'ımızdadır; GTA gözlemi ayrı opt-in koşudur. Windows API failure injection, zorla observer kill ve initialized oyun davranışı bu alt testlerden doğrulanmış sayılmaz. canAttach=false ve AC-90/R-01a/b'nin açık runtime sınırı korunur.

## Statik native başlangıç scope'u

LayoutNotes alanı raw padding gibi gözlemlenen sapmaları içerir; boş not listesi Windows loader uyumluluğu garantisi değildir. Snapshot digest bu alanı da kapsar; doğrudan CLI'nin pretty JSON hash'iyle karşılaştırılmaz.

[StartupInventory](../development/d1-native-startup.md), schema 1/static-native-startup-inventory scope'unda parse edilen modül hash/boyut/entry/import/delay/TLS, LocalCandidate edge ve açık metadata sorunları taşır. SHA-256 inventoryDigest raporda tanımlı JSON serialization'a bağlıdır; partial envanter veya farklı dosyalar arasında atomik gözlem varsayılmaz. CanAttach/CanAdvanceToLoader/RuntimeResolutionVerified/DynamicLoadsEnumerated false; native runtime EvidenceRecord yerine kullanılamaz. Aynı hash'li exe yanında değişmiş DLL bulunduğunda kullanılan çevre kapsamı yeniden incelenir.

## Kod 0.1.6 loader evidence eşlemesi

[Bounded loader raporu](../development/d1-loader-observation.md) kendi scope/schemaVersion=1 ve observed engine/tool hash'iyle kaydedilir. En az policy, child PID/exit, loaderAdvanced, modül event sırası/file ID/hash/identityRead/admitted, sayaçlar, exception ve reason bulunur. identityRead=false kaydındaki sıfır hash veri değildir. Fixture breakpoint adayı exit 3 gözlem; gerçek apphelp unpinned exit 1 ret; ikisi ayrı outcome'dur. DLL/TLS/main canary ve handle stabilitesi yalnız test artifact/Windows build'i için geçerlidir. canAttach ve initializationVerified false kalır; N2/N3/production conformance kapısı açılmaz.

## Kod 0.1.7 mapping policy evidence

[Policy kanıtı](../development/d1-loader-policy.md), önceki loader schema 1'e additive policySourceDigest/failedPolicyModule ekler. Policy ID/source hash'i, engine hash, kullanılan executable basename/root, CWD/inherited environment, seçili yan dosyalar ve compiler artifact hash'i birlikte kaydedilir. Bütün recipe/dosya kontrolleri child öncesi tamamlanır; missing/mismatch reason ve childCreated=false somut negatif kanıttır. Original/private launch context sonuçları tek runtime evidence'e birleştirilmez; initializationVerified/canAttach=false kalır. Source hash değişince eski mapping sonucu yeni profile otomatik taşınmaz.

## V0.15 — Context kanıtının kimliği

[0.1.8 raporu](../development/d1-launch-context.md) engine/policy/probe kimliklerine explicit cwd ve environmentSha256/count ekler. Ham environment değerleri saklanmaz; digest'ten replay veya güvenlik yetkisi türetilmez. Parent mutation testi, own DLL resolution testi ve gerçek GTA gözlemi ayrı sonuçlardır. Null launchContext eski inherited modu belirtir; belge veya parser bunu explicit/reproducible environment diye yorumlamaz.

0.1.8 explicit context gözleminde beklenmeyen OS debug olayının tanısı için loader çıktısına lastEventCode/lastEventThreadId eklendi. Son alınan olay metadata'sıdır; initialization kanıtı veya olayın devamına izin değildir. Child öncesi retlerde ikisi de sıfırdır. Unknown event terminal ret davranışı ve incelenmiş DLL policy değişmedi.

## V0.16 — Mapping kanıtının ömrü

[0.1.9 mappingId](../development/d1-loader-lifecycle.md) yalnız tek process observation içinde anlamlıdır; EntityRef/Grant veya native symbol kimliği değildir. Yeni alanlar eski admitted tarihçesinin aktif state diye yorumlanmasını engeller. Modül/byte/event bütçeleri yaşam boyu kümülatiftir. Raporlar portable ledger, Windows canary ve gerçek GTA unload/remap kanıtını ayrı sayar.

## V0.17 — Sembol metadata kanıtı

[Linkage report](../development/d1-native-linkage.md) scope=static-native-linkage/schemaVersion=1; consumer/candidate byte/SHA, requestedModule, normal/delay requirements, compact matched ordinal/RVA/kind/forwarder, issues ve beş false runtime flag taşır. staticSymbolsComplete yalnız seçilen ikili için non-empty direct metadata eşleşmesidir; candidate'in transitif bağımlılıkları, delay çağrısının gerçekleşmesi, Windows resolution veya ABI kapsama girmez. Artifact hash, expected/observed CLI exit ve input-before/after hash kanıtı deney raporuna ayrıca eklenir. Raporların merge edilmesi atomik dependency closure/loader izni yaratmaz; mevcut mapping/policy kanıtı kendi ayrı kimliğinde kalır.

## V0.18 — Entry-boundary evidence

[Scope bounded-entry-boundary-observation](../development/d1-entry-boundary.md) schema 1; mapping policy/digest ile entryObservation.executionPolicy ve probe artifact SHA ayrı kimliklerdir. breakpointArmed→initialBreakpointContinued→boundaryReached→bytesRead zinciri, beforeHex/afterHex/bytesMatch ve childExitConfirmed kaydedilir. Null entryObservation legacy scope'tur. Modified entry tanısal complete olabilir; canAttach ve initializationVerified yine false. Hardware DR6/DR7/EIP kimliği, fixture marker'ı ve gerçek GTA entry gözlemi farklı kanıt seviyeleridir; diğer thread yürütmesi/oynanış/SDK capability sonucu türetilmez.

## Kod 0.1.11 — Entry supplement bağı

executionPolicySourceDigest entry JSON SHA’sıdır; basePolicySha256 immutable mapping source’una bağlanır. Source’lar, gerçek modül hash’leri, probe artifact SHA ve context birlikte kaydedilir; policy adı tek başına kanıt değildir. [Ayrıntı](../development/d1-entry-boundary.md).

## GitHub kaynak yayını — taşınabilirlik düzeltmesi

Own fixture cwd kanıtı artık string yazımı değil açılmış volume/file ID eşitliğidir. Eşdeğer alias pozitifi, farklı dizin ve yanlış token negatifleri ayrı koşulur. Bu test başarısı bütün ortam eşitliği, sandbox veya GTA çalışma kanıtı değildir; portable bootstrap dönüşümü ABI layout kanıtını değiştirmez.

### Hosted corpus tamamlaması

İkinci hosted tur Linux ve Windows x64'te geçti; x86 native 9/9 sonrasında Python cwd karşılaştırması runneradmin/RUNNER~1 yazım farkını reddetti. CLI testi artık pathlib.samefile ile dizin kimliğini sınar; bilinmeyen engine/child öncesi ret ve ortam redaction korunur. Bootstrap corpus da geçersiz reason için exact INTERNAL_ERROR ve bilinen ret değerlerini açık doğrular. Production API, profile/recipe ve oyun davranışı değişmez.
