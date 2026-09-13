# Asset manifesti, kimlik ve bağımlılıklar

Durum: sözleşme taslağı. [Katalog](catalog.md) · [Override](versioning-overrides.md)

## Kimlikler

| Kimlik | Ömür ve kullanım |
|---|---|
| PackageId | `sandbox.props` gibi yayıncı ad alanı |
| PackageVersion | İnsanların uyumluluk seçtiği SemVer |
| AssetRef | `sandbox.props:crate/wood` gibi mantıksal yol |
| ArtifactDigest | Gerçek artifact byte'larının SHA-256 özeti |
| CatalogRevision | Seçilmiş tanımlar, artifact'ler ve override düzeninin özeti |
| NativeHandle | Tek client yüklemesine ait geçici GTA ID'si |
| PlacementId | Harita yerleşiminin export'lardan bağımsız kalıcı kimliği |

Aynı package version altında farklı byte yayımlanmaz. Authoring alias'ları kolaylık içindir; release lock set tam digest içerir. İki sunucu aynı artifact'i kullanırsa cache byte'ları paylaşılabilir; resource storage ve server izinleri paylaşılmaz.

## Authoring manifest örneği

Örnek authoring manifesti henüz hash üretilmemiş kaynakları anlatır. Release manifestinde kaynak yollarının yerini artifact kayıtları ve çözülmüş bağımlılıklar alır.

```json
{
  "schemaVersion": 1,
  "package": { "id": "sandbox.props", "version": "1.0.0" },
  "target": "gta-sa-classic",
  "dependencies": [],
  "assets": [
    {
      "id": "crate/mesh",
      "type": "static-mesh",
      "source": "models/crate.dff",
      "requires": ["sandbox.props:crate/textures"]
    },
    {
      "id": "crate/textures",
      "type": "texture-set",
      "source": "textures/crate.txd",
      "requires": []
    },
    {
      "id": "crate/collision",
      "type": "collision-shape",
      "source": "collision/crate.col",
      "requires": []
    },
    {
      "id": "crate/wood",
      "type": "prefab",
      "requires": [
        "sandbox.props:crate/mesh",
        "sandbox.props:crate/collision"
      ],
      "components": {
        "model": { "asset": "sandbox.props:crate/mesh" },
        "collider": { "asset": "sandbox.props:crate/collision" }
      }
    }
  ]
}
```

Bu, temel görsel/collision örneğidir; hasar/yıkım eklemesi [yıkım belgesinde](destruction.md) gösterilir. Örnekteki dosyalar teslimata dahil değildir.

## Release artifact kayıt alanları

`artifactDigest`, byte uzunluğu, media type, cook tool version, target engine profile, compression format, uncompressed limit, chunk listesi ve gerekli capability seti kayıtlıdır. Grafik, fizik ve animation hedefleri aynı prefab altında farklı artifact closure'ları üretir.

Digest exact cooked bytes üzerinden hesaplanır. Catalog digest, canonical manifest biçimi ve sıralanmış çözülmüş bağımlılıklar üzerinden üretilir; JSON boşluk değişimi insan düzenlemesiyle katalog kimliğini rastgele değiştirmemelidir. Canonical biçimin byte sözleşmesi R-04 çıktısıdır.

Server-only collision/query artifact'leri ile client render artifact'leri dağıtım etiketleriyle ayrılır. Public client manifesti gizli origin credential'ı veya server script yolunu içermez.

## Referans ve graph kuralları

- Asset tanımındaki dependency grafiği döngüsüzdür. Gameplay'de oluşan runtime referans döngüsü farklı bir konudur.
- `requires` zorunlu yükleme bağıdır. Alternatif/ses gibi isteğe bağlı içerik ayrı `optional` bağıdır ve fallback davranışı ilan edilir.
- Aynı normalize AssetRef iki kez tanımlanamaz. Sürüm override açık mapping olmadan sessizce kazanamaz.
- Aşırı graph derinliği veya closure büyüklüğü build limitinde reddedilir.
- Dynamic dependency yalnız manifestte ilan edilmiş namespace/allowlist içinden seçilebilir; runtime limitsiz indirme isteyemez.

İlk doğrulayıcı sınırları: package başına 50.000 asset, dependency derinliği 64, path uzunluğu 240 UTF-8 byte. Bunlar dosya sistemi evrensel sınırı değil platform politikasıdır; artırma ölçüm ve sürüm değişikliği gerektirir.

## Native ID tahsisi

Bir AssetRef iki client'ta farklı native model numarası alabilir; ağda mantıksal referans taşınır. Handle release sırasında GPU/native kullanımının bitmesi beklenir. Yeniden tahsiste generation artar. Modelin serbest bırakılması bütün prefab bağımlılıklarını rastgele unload etmez; lease/ref count zinciri izlenir.

## Hata ve kabul

Unknown type, missing dependency, cyclic graph, duplicate normalized ID, incompatible profile ve digest mismatch ayrı diagnostic üretir. Diagnostic paket, asset ve dependency zincirini gösterir.

Kabul: AC-06 sürüm/kimlik, AC-07 migration, AC-12 native ID tekrar kullanımı, AC-14 bozuk manifest. Public manifest DTO'ları versioned'dır; mevcut C# sınıfını JSON serialize etmek şema tanımı yerine geçmez.

## Tür, birey ve spawn slot kimlikleri

SpeciesId/BreedId teknik olarak ad alanlı AssetRef'tir; örnekler [Animals tanımında](../gameplay/animals-species.md) bulunur. AnimalDefinition asset'inin source/definition referans alanları zorunlu closure'ı üretir; açık requires listesiyle çelişirse build hatasıdır. Appearance değişimi yeni birey kimliği yaratmaz. PersistentEntityId birey, SpawnSlotId/generation üretim kaydıdır; native ped/model numarası kalıcı hayvan kimliği olamaz.

Predator/prey veya sosyal üyelik ilişkisi runtime/domain verisidir; karşılıklı hard asset bağıyla döngü oluşturulmaz. Server behavior/seed ve private sahiplik ayrıntıları client closure'ına konmaz. Yeni tanım allowlist ve release lock'a girmeden URL veya serbest model ID ile spawn edilemez. AC-47/56/59/68.
