# Üçüncü taraf kaynak ve lisans bildirimleri

SAEX'e ait özgün kaynaklar [MIT](LICENSE) lisanslıdır. Bu lisans GTA oyun dosyalarına veya üçüncü taraf kaynaklarına uygulanmaz. Bu depo orijinal GTA executable/asset'lerini ve oyun kurulumundan alınan DLL/ASI dosyalarını dağıtmaz.

## İsteğe bağlı Plugin-SDK-SA kaynağı

Standart foundation build'i vendor SDK'yı indirmez veya linklemez. D1-N1 opt-in build, [kilitte](contracts/engine/plugin-sdk.lock.json) belirtilen `Dryxio/plugin-sdk-sa` commit'inin seçilmiş kaynaklarını ayrı yerel cache'e edinir. Upstream kaynak/notice dosyaları cache'te korunur; [iki patch](tools/native/patches/0001-injector-member-names.json) ve [pool uyarlaması](tools/native/patches/0002-pool-native-union.json) değişiklikleri exact hash ve açık replacement olarak izler.

| Kaynak | Lisans/bildirim | Sabit kaynak |
|---|---|---|
| Plugin-SDK shared/SA seçili dosyalar | Zlib; dosya başlıkları korunur | [LICENSE](https://github.com/Dryxio/plugin-sdk-sa/blob/b55e89b336a81448c1aa1a5b188431c9845ebaa9/LICENSE) |
| LINK/2012 injector başlıkları | Zlib; inline bildirimler korunur | [injector.hpp](https://github.com/Dryxio/plugin-sdk-sa/blob/b55e89b336a81448c1aa1a5b188431c9845ebaa9/injector/injector.hpp) |
| Hooking.Patterns.h | MIT | [hooking/LICENSE.md](https://github.com/Dryxio/plugin-sdk-sa/blob/b55e89b336a81448c1aa1a5b188431c9845ebaa9/hooking/LICENSE.md) |
| SafetyHook header bağımlılığı | Boost Software License 1.0 | [safetyhook/LICENSE.txt](https://github.com/Dryxio/plugin-sdk-sa/blob/b55e89b336a81448c1aa1a5b188431c9845ebaa9/safetyhook/LICENSE.txt) |

Bu tablo bütün upstream ağacının lisans incelemesi değildir. Kullanılan dosya ve kaynak/patch/recipe digest'leri lock'tadır; [N1 raporu](docs/development/d1-native-dependency.md) kapsam dışı RenderWare/DXSDK ve diğer bileşenleri ayırır. Vendor kaynak veya binary dağıtılacaksa gerçek paketle birlikte ilgili tam lisans metinleri, telif başlıkları ve değişiklik bildirimleri de verilmelidir. Yalnız bu bağlantı tablosu binary dağıtım koşullarını karşılamaz.

## Araştırma ve CI araçları

MTA/open.mp mimari incelemeleri belgelerde kaynak gösterilen araştırmalardır; o projelerin kodu veya marka varlıkları SAEX'e aitmiş gibi lisanslanmaz. GNS planlanan oyun taşımasıdır; mevcut foundation kaynağında GNS runtime yoktur.

CI'da kullanılan resmi GitHub Actions ve Gitleaks kendi depolarından sabit sürüm/hash ile alınır; SAEX runtime paketinin parçası değildir. Yeni bağımlılık veya dağıtım kapsamı bu dosya, lock ve ilgili kanıt belgesiyle birlikte güncellenir.
