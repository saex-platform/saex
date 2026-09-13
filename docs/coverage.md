# Kullanıcı vizyonunun kapsam eşlemesi

Durum: iki ek metin ve kabul edilen planın izlenebilir kapsamı. [İndeks](../README.md)

## İlk metin: yaşayan sandbox

| ID / fikir | Yerleşim ve gerekçe | İlgili belge / aşama |
|---|---|---|
| V-01 Yaşayan NPC: ihtiyaç, iş, takvim, envanter, ilişki | İsteğe bağlı AI/needs kit; core'a sabit yaşam modeli koymaz | [Kit'ler](gameplay/sandbox-kits.md), D4+ |
| V-02 Dinamik ekonomi ve tedarik | Domain transaction kullanan economy paketi | [Kalıcılık](networking/persistence.md), [kit'ler](gameplay/sandbox-kits.md), D4+ |
| V-03 İnşa, sahiplik, ev/işletme | Ownership/construction/business/housing kit'leri | [Harita](assets/maps-prefabs.md), [kit'ler](gameplay/sandbox-kits.md) |
| V-04 Araç motor/yakıt/akü/lastik/bakım | VehicleDefinition ve optional component'ler | [Kit'ler](gameplay/sandbox-kits.md), R-08 |
| V-05 Fizik objeleri, hareket/attach/destroy | Core entity/physics capability | [Entity](architecture/entities-worlds.md), [fizik](networking/physics.md), D2 |
| V-06 Oyuncu inşa modu | Yetkili placement, collision ve maliyet transaction'ı | [Harita](assets/maps-prefabs.md), AC-21 |
| V-07 AI faction, suç ve polis tepkisi | Policy-driven AI/faction/crime kit | [Kit'ler](gameplay/sandbox-kits.md), R-10 |
| V-08 Dinamik şehir, trafik ve nüfus | Bütçeli environment/AI/economy olayları | [Kit'ler](gameplay/sandbox-kits.md), D4+ |
| V-09 Görev üretimi | Mission template ve idempotent ödül | [Kit'ler](gameplay/sandbox-kits.md), AC-23 |
| V-10 Farklı oyunlara tam dönüşüm | World profile ve değiştirilebilir sağlayıcılar | [Tam dönüşüm](gameplay/total-conversion.md), AC-24 |

## İkinci metin: platform yetenekleri

| ID / fikir | Karar | İlgili belge |
|---|---|---|
| P-01 Hot reload | Transactional ve sınırlı rollback; native restart ayrımı | [Lifecycle](resources/lifecycle-hot-reload.md) |
| P-02 Dependency manager | Resource/asset DAG, service provider ve lock set | [Manifest](resources/manifest.md) |
| P-03 Resource versioning | SemVer authoring, exact artifact release | [Sürüm](assets/versioning-overrides.md) |
| P-04 Otomatik CDN | Self-host HTTPS origin/CDN desteği; merkezi hizmet zorunlu değil | [Dağıtım](assets/distribution-cache.md) |
| P-05 Delta update | Hash'li chunk yeniden kullanımı; kazanç garanti değil | [Dağıtım](assets/distribution-cache.md) |
| P-06 Bölgesel asset streaming | Hücre/prefetch/lease ve collision bariyeri | [Streaming](assets/streaming-budgets.md) |
| P-07 Dinamik dünya, hava/saat/blackout | Environment state; gameplay tepkileri kit | [Kit'ler](gameplay/sandbox-kits.md), [ses/efekt](assets/animation-audio.md) |
| P-08 Interest sistemi | Dünya/portal/dependency/visibility ve frekans sınıfları | [Replikasyon](networking/replication.md) |
| P-09 Server anti-cheat | Intent/lease doğrulama ve açıklanabilir ret; tam koruma iddiası yok | [Otorite](networking/authority.md) |
| P-10 Replay/spectator | Accepted state kaydı; deterministik native replay değil | [Araçlar](platform/developer-tools.md) |
| P-11 Profiler | Resource, engine, network, asset ve physics bütçeleri | [Araçlar](platform/developer-tools.md) |
| P-12 Debugger | Inspector, trace zinciri ve scoped debug | [Araçlar](platform/developer-tools.md) |
| P-13 C# async API | Snapshot + command/result, cancellation ve stale kontrolü | [SDK](resources/runtime-sdk.md) |
| P-14 DB abstraction | Provider garantileri; reference PostgreSQL, local SQLite, kit adapter'ları | [Kalıcılık](networking/persistence.md) |
| P-15 Web API | Kimlikli REST/WS operasyon sözleşmesi | [İşletim](platform/launcher-operations.md) |
| P-16 Web admin panel | Aynı API'nin permission'lı istemcisi | [İşletim](platform/launcher-operations.md) |
| P-17 Permission sistemi | Oyuncu rolü, resource capability ve sim owner ayrı | [Güvenlik](resources/security-native.md) |
| P-18 Event bus | Local/remote açık ayrım; schema ve idempotency | [SDK](resources/runtime-sdk.md) |
| P-19 Native.Call | Ham adres API'si yerine izinli typed engine komutları | [Güvenlik](resources/security-native.md) |
| P-20 C++ plugin | Güvenilen ayrı platform native dağıtımı; sunucudan otomatik DLL değil | [Güvenlik](resources/security-native.md) |

## Ek başlıklar ve plan düzeltmeleri

| ID / fikir | Sonuç |
|---|---|
| X-01 Native 3D voice, radio/phone/vehicle/interior | [UI/voice](platform/ui-voice.md); ayrı capture/route yetkisi |
| X-02 Server authority + prediction | [Fizik](networking/physics.md); native input replay deterministik varsayılmaz |
| X-03 Dimension yerine world instance | [Entity/world](architecture/entities-worlds.md); interior/portal ayrımı korunur |
| X-04 Dynamic entity ownership | [Otorite](networking/authority.md); süreli lease ve owner epoch |
| X-05 C++ core / C# SDK | [Mimari](architecture/overview.md); x86 adapter ve x64 worker ayrımı |
| X-06 Launcher/server browser | [İşletim](platform/launcher-operations.md); doğrudan bağlantı ve self-host |
| X-07 Taslaktaki ENet | V1 GNS olarak kesinleştirildi, ENet fallback yok; [ayrıntılı ADR-07](decisions/network-transport.md) |
| X-08 Her asset ve obje kırılganlığı | [Katalog](assets/catalog.md), [yıkım](assets/destruction.md); doğrulanmış engine capability sınırı |
| X-09 Oyuncu/client-server eşzamanlılık | [Replikasyon](networking/replication.md); state yakınsaması, late join ve durable dünya |

## V0.4 kullanıcı isteği: nüfus, trafik ve modüler hayvanlar

| ID / fikir | Yerleşim ve gerekçe | Kabul / aşama |
|---|---|---|
| L-01 Yaya/kara/hava ayrı aç-kapat | [PopulationPolicy](gameplay/population-traffic.md); dinamik ayar server state, statik sınır WorldPlan | AC-46/48; D3 nüfus kolu |
| L-02 Dünya/bölge/saat yoğunluğu | Tek director, açık zone önceliği, birleşik AOI ve kotalar | AC-47/68/70; R-17 |
| L-03 GTA gibi ortak etkileşim | Task/koltuk/hasar/sahiplik state; native AI sınırı açık | AC-49–54; R-17 |
| L-04 Uçak/helikopter hava trafiği | Ayrı flight provider, koridor, runway/helipad ve loss policy | AC-51/52/55; R-18 |
| L-05 Tam sync ve geç gelen oyuncu | [Agent](gameplay/agents-navigation.md) state/timeline/owner ile mevcut baseline sözleşmesi | AC-50/51/61/64; sıfır gecikme vaadi yok |
| L-06 Modüler tür ve cins ekleme | [Animals](gameplay/animals-species.md) data/rig/behavior ayrımı; yeni native sınıf ayrı kanıt | AC-56/57/62/67; R-19 |
| L-07 Gelişmiş hayvan AI | Algı/needs/task/nav/locomotion, sürü ve ilişkiler | AC-54/58/60/65; R-10/19 |
| L-08 Evcil/çiftlik/vahşi sınıflar | Ayrı opsiyonel kit ve ControlClass; protected drain | AC-59/63/66; ownership kapısı |
| L-09 Kalıcılık ve yaşam | Tek birey/ölüm/loot/slot; offline ilerleme varsayılan kapalı | AC-61/68/69; R-20 ileri kapsam |
| L-10 Bütün belgelere entegrasyon | [V0.4 kapsam kaydı](validation/population-animal-integration.md), ADR/AC/R ve asset/resource/ağ bağları | D0 belge doğrulaması |

Önceki 39 fikir kaydı korunur; bu 10 ek istek izidir. Hiçbir satır uygulamanın tamamlandığını göstermez. D2/D3 çekirdek doğrulaması geçmeden geniş gameplay kataloğunu bitirmek öncelik değildir.

## V0.5 kullanıcı isteği: stabilite ve eksiklerin giderilmesi

| ID / gereksinim | Karar ve gerekçe | Kabul / aşama |
|---|---|---|
| ST-01 Oyun takılınca geçerli otorite | [Ayrı saat ve resume fencing](architecture/time-fencing.md); tick/UTC lease'i uzatmaz | AC-71/72/84/86, D1–D2 |
| ST-02 İşlem ve resource çöküşü | [Core receipt recovery](architecture/failure-recovery.md); kesinleşmiş sonuç korunur, belirsiz commit tekrar etmez | AC-73/74, D2–D3 |
| ST-03 Tek dünya yazarı | WriterTerm guard, doğrulanmış takeover; v1 otomatik HA yok | AC-75, D2 |
| ST-04 Hayvan/araç transfer tekilliği | [Canonical transfer](networking/consistency-recovery.md); aynı core/backend, phase-aware quota | AC-76/77/87, ilgili D3 ürün kolu |
| ST-05 Ağ state'ini onarabilme | Baseline/delta tabanı, bağımsız motion, bounded resync ve native kontrol | AC-78–81, D2–D3 |
| ST-06 Bütün sistemin yük sınırı | RecoveryProfile, process toplamı, circuit breaker, admitted operation bütçesi | AC-82/83/88, D3–D4 |
| ST-07 Eski içeriği ve dünyayı kurtarma | Backup/operation/animal closure retention; failure türüne bağlı RPO/RTO | AC-85/86, D3 |
| ST-08 Denetlenebilir stabilite | [16 bulgu/düzeltme](validation/stability-audit.md), ADR/R/AC bağları ve fault profili | AC-88, D0 belge/D4 runtime kanıtı ayrı |

Güncel kapsam 57 gereksinim kaydıdır. V0.5'in 8 kaydı kullanıcı hedefini somut doğrulama alanlarına böler; hiçbirinde kusursuz runtime garantisi veya kodlanmış özellik iddiası yoktur.
