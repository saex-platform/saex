# D1-N2 — Windows 26200.9445 profil yenilemesi

Kod 0.1.33 / mimari v0.40, 14 Eylül 2026. [Durum](status.md) · [Query](d1-cwd-query.md) · [Loader sözleşmesi](d1-loader-policy.md) · [ADR-68](../decisions/architecture-decisions.md).

## Sorumluluk ve doğrulama sınırı

Windows güncellemesinden sonra 0.1.32 GTA query denemesi child yaratılmadan reddedilmişti. Önceki 17 fark yalnız kök loader listesine aittir; entry/ASI/bootstrap ekleriyle birlikte **26 sistem dosyasının 20'si farklıdır**. Registry build 26200, UBR 9445, DisplayVersion 25H2'dir; DLL FileVersion değerleri birbirinden farklıdır. Profil adı bütün DLL'lerin 9445 sürümünde olduğunu söylemez.

Bu kesit aynı kontrollü native durakların yeni sistem dosyalarıyla yeniden incelenmesini sağlar. 23 policy kaynak/hash zinciri ve 23 generated header birlikte güncellenmiştir. `windows-26200` içeren profil kimlikleri `windows-26200-9445` olarak ayrılmıştır; OS adı taşımayan oyun alt profilleri yeni parent SHA-256 ile bağlanır. GTA executable profili, native kod byte/RVA'ları, stage/terminal, modül adları/origin/basis ve izin kümesinin kapsamı değişmez. C ABI 1, GNS/otorite ve resource davranışı değişmez. Tam initialization, renderer, N3 ve D1/D2 açık kalır.

Eski profil artık bu derlemenin aktif OS seçimi değildir. Eski dosyalara veya daha yeni bir güncellemeye otomatik fallback yoktur. Eski 0.1.31 GTA başarıları ve 0.1.32 ret kayıtları kendi policy/probe/hash bağlamlarında tarihsel olarak korunur; yeni profil için tekrar runtime kanıtı gerekir. Yeniden derleme gerekir; save/world/protokol migration'ı yoktur.

## Veri akışı ve arayüzler

1. Kapsam mevcut JSON izin listelerinden alınır; bulunan her DLL otomatik izin kazanmaz. `SysWOW64` dosyaları salt okunur boyut/hash/FileVersion/Authenticode kontrolünden geçirilmiştir. 26/26 imza `Valid` döndü; imza tek başına davranış veya sandbox kanıtı değildir.
2. İlgili PE export tabloları, RVA'lar, forwarder, ilk opcode'lar ve HIGHLOW kayıtları incelenir. Bağımsız `dumpbin /DISASM:BYTES /RANGE` çıktılarıyla komut girişleri karşılaştırılır. Tam OS fonksiyon eşdeğerliği iddia edilmez; bounded API sonuçları runtime'da yeniden sınanır.
3. İncelenen modül tanımları ve API reçeteleri açık kaynak JSON'a yazılır. Mevcut generator'lar parent hash, tekrar eden OS pinleri, range/shape/fixup ve stage sınırlarını aynen uygular. Yeni recipe runtime'da keşfedilmez; gözlem çıktısı hiçbir izin dosyasını otomatik değiştirmez.
4. Bütün recipe pinleri child öncesi hazırlanır; her LOAD aynı dosya kimliği ve hash ile denetlenir. CLI ve terminal adresleri 0.1.32 ile aynıdır. `--observe-cwd-query <exact-exe> <absolute-cwd>` explicit ASCII dizin/NUL/uzunluk/126-byte kapısını korur.
5. Yeni query deneyinde yedi checkpoint, API dönüşü, CRT kilit sahipliği, caller/root/SEH/NT_TIB/cookie/guard korunumları ve owned child çıkışı yeniden doğrulanır. Terminal `0x836E30`, kopyalama öncesidir. SystemParametersInfoA önceki sözleşmeyle bastırılır; doğal API çağrısı olarak sunulmaz. Tam child sonlandırması kilidin bırakılması veya SEH unwind kanıtı değildir.

## API incelemesi

RVA'lar aşağıdaki pinned dosyanın tercih edilen tabanından bağımsızdır. Runtime kontrolü yüklenmiş tabana göre HIGHLOW düzeltmelerini tam uygular; değişken adres byte'larını yok saymaz.

| Modül/export | Yeni RVA | HIGHLOW offset | İncelenen ilişki |
|---|---|---|---|
| kernel32.dll / VirtualProtect | `0x16B30` | 8 | 20-byte mevcut export sözleşmesi |
| kernel32.dll / GetStartupInfoA | `0x64520` | yok | 20-byte mevcut export sözleşmesi |
| kernel32.dll / GetLastError | `0x23640` | yok | 20-byte mevcut export sözleşmesi |
| kernel32.dll / CreateEventA | `0x1E7D0` | 2 | 6-byte FF25 thunk; slot 0x81B18 |
| kernel32.dll / GetCurrentDirectoryA | `0x31060` | 8 | 20-byte mevcut export sözleşmesi |
| kernel32.dll / EnterCriticalSection | `0x9C727` | yok | NTDLL.RtlEnterCriticalSection |
| kernelbase.dll / CreateEventA | `0x14DFB0` | yok | 20-byte mevcut export sözleşmesi |
| kernelbase.dll / GetCurrentDirectoryA | `0x2595B0` | 12 | 20-byte mevcut export sözleşmesi |
| ntdll.dll / RtlEnterCriticalSection | `0x543E0` | yok | 20-byte mevcut export sözleşmesi |
| user32.dll / SystemParametersInfoA | `0xC3580` | 3 | 20-byte mevcut export sözleşmesi |

Kernel32 CreateEventA yalnız kendi 6-byte thunk'ı üzerinden denetlenir. Araştırmada 20 byte okunduğunda 18. offsette görülen HIGHLOW komşu thunk'a aittir; API'nin 20-byte recipe'si gibi kullanılmadı. Kernelbase CreateEventA ve NTDLL RtlEnterCriticalSection ilk 20 byte'ı öncekiyle aynıdır, RVA değişmiştir. Query kernel32 slotu `0x81794`; kernelbase cookie operandı için offset 12 düzeltmesi korunur. User32 SystemParametersInfoA RVA/operand/CALL byte'ları değişmiştir; bu fonksiyonun çalışması yine bastırılmıştır. Mevcut CRT critical-section düzeni ayrıca runtime readback ile sınanmalıdır.

## Sistem dosyası kayıtları

SHA-256 tam dosyaya aittir. Önceki/simdiki hash eşitliği dosyanın bu kesitte değişmediğini gösterir; bütün dosyalar yeniden pinlenir. `FileVersion`, dosyanın kaynak bilgisinden alınmıştır. Bütün imzalar geçerli Microsoft imzası olarak raporlandı.

| Dosya | Önceki SHA-256 | Yeni SHA-256 / byte | FileVersion |
|---|---|---|---|
| advapi32.dll | `265a7530ffc6adf8a37e4fb9fa6d01770d2f4110b7de18455065503f3ca388ca` | `06e6ee5fcd0601e181e5dd8f9a6328dd79b6bcc8fcdfc2b1a7832f0556d0d049` / 537816 | 10.0.26100.9278 (WinBuild.160101.0800) |
| apphelp.dll | `85d8b3d7a05378b778094dbb226f7bf1d929fe4a0b370b6e61b3a366eeda0aef` | `a8109618c3bf3623be23e8e18f1e7184538600c953169297fd0009ed77564c6e` / 710832 | 10.0.26100.8457 (WinBuild.160101.0800) |
| bcrypt.dll | `d713ce8ccada35dcdc921184842a7316911c5151a44864cbfb3d02c7136d5ec0` | `137cec7506fdb011ed03c5a7b51c6ea168cc75ec4359b8dc3f83855622d86bc3` / 114240 | 10.0.26100.9444 (WinBuild.160101.0800) |
| bcryptprimitives.dll | `c0fba4406bf9ca909f6f463f08189d6eb47238d7bb590d58344af8cadd6bda8e` | `dfd29f2c1f28034949f0a3716a99f00816bda0bffd48a5a2750be4569e7d21ee` / 501136 | 10.0.26100.9444 (WinBuild.160101.0800) |
| combase.dll | `f45e74e3f784bdb765c562c138a81aa3daf297b11600e9035df874eae73be456` | `3f152587249ff6623eb0023cf52cad64aac02f6d8117d8754349f83768f9a6f8` / 2711704 | 10.0.26100.9444 (WinBuild.160101.0800) |
| gdi32.dll | `7c3cb17bc41a00e5167a82aef67d0da4a09e73e6deba9addcdc8b20a90b5186c` | `7c3cb17bc41a00e5167a82aef67d0da4a09e73e6deba9addcdc8b20a90b5186c` / 132352 | 10.0.26100.8521 (WinBuild.160101.0800) |
| gdi32full.dll | `51f85c77c33506f36a9c159994f1fdc7504bc7e7f4e5f4996997ae3690bbcf78` | `579857a52f8e9cccff64a4c1c1eb5d2a4a8c6e19c30eab16bef63ed6d61e8806` / 950488 | 10.0.26100.9444 (WinBuild.160101.0800) |
| imm32.dll | `c69ca53458deb7609f9a879b356f62330be743343aaa08afd055f15fa4fe2a95` | `027d404ea547f446740336b90dc69b0cfa7c2bea4f1f366053251364f0eefe02` / 147856 | 10.0.26100.9278 (WinBuild.160101.0800) |
| kernel32.dll | `4ef63ecfe99158e16a387028be295308b00618cec64d94f016f99b79ffe84e05` | `bafd9e061964642802d62f1eb694e7e4af60de739a8df1a492f90006dc5680f3` / 687472 | 10.0.26100.9444 (WinBuild.160101.0800) |
| kernelbase.dll | `10dcab84a598fa39859f5b967d8dec98b9eecca1419f2c6fde2803e40cb6004f` | `85ad413ff860898f9e511f2e63c0e30f7aed1275fd1fd44fa90801211d328253` / 2974168 | 10.0.26100.9444 (WinBuild.160101.0800) |
| msvcp_win.dll | `52fa16b01616d083a198aba230588bf947f84e33f5e79535f65c8d48b6219814` | `2c3dd2c830375591addea061588f8183bcafdba501c638526b85215465ac905a` / 505792 | 10.0.26100.9444 (WinBuild.160101.0800) |
| msvcp140.dll | `f0cda2a0cf1fe6fbbf579b9098462329d3aaa7513a207af2e0f33b01456e388a` | `f0cda2a0cf1fe6fbbf579b9098462329d3aaa7513a207af2e0f33b01456e388a` / 618944 | 14.51.36247.0 |
| msvcp140d.dll | `5588cb0546bffe357e864e2df9bf919dfe41f4d08dee14962467cbf803880b4f` | `5588cb0546bffe357e864e2df9bf919dfe41f4d08dee14962467cbf803880b4f` / 739408 | 14.44.35211.0 |
| msvcrt.dll | `d72870f695fc49e1cb9f4fc3f45e202a7effa26474067b0e328ce31affd4a437` | `23a918921c545140d189ce6dc68590bacdeb0ab2f3e3a9a2d722c5aff45ce64f` / 809472 | 7.0.26100.9444 (WinBuild.160101.0800) |
| ntdll.dll | `665df97f10439d9333e1390f5a9873b6af24d396e6bef6b0316ff1ed0f2418de` | `7e15bd30890e9bf93b47fc894a68b2445618ee7528eb39584a263b21e112f9df` / 1822400 | 10.0.26100.9278 (WinBuild.160101.0800) |
| ole32.dll | `05a52b6fae47a3719fdad41c4c80d39497ea91a21c09eede6cfc1d9f34e34d3b` | `d178e227ede5073ecfed4528199b03327e1c2edff9ddd09d79d4af821fdde968` / 1406032 | 10.0.26100.9444 (WinBuild.160101.0800) |
| rpcrt4.dll | `f4ac5be29a308bd5e65df9004e73ac74d3cc0a067387a591eb259ae499ec96ed` | `6e3f4e700f03dc415d0a52ffdaac0ce3dfe80a42cd5e27dd304ccc11afea11d8` / 772776 | 10.0.26100.9444 (WinBuild.160101.0800) |
| sechost.dll | `09f7906ef4a531c547927a7992adfb77fab05dae5b8052a5ef5a931f6b35b03e` | `bfb478f918ee80d1945237cb15db42e2c5e2e324fa00211de37511c1cc720ad4` / 538352 | 10.0.26100.9444 (WinBuild.160101.0800) |
| ucrtbase.dll | `c1024cd6c9be74752d682bbf391a92514869621f1b249d08811e102f034b43a0` | `60c5a497b52de80a3a0677564270dbea7e486086637debd567b4dffb28584c1b` / 1114240 | 10.0.26100.9444 (WinBuild.160101.0800) |
| ucrtbased.dll | `82a69bf9ec26a3fd3aea55b2d4c1b8a664c996520ae1319e98ea6050b868665d` | `82a69bf9ec26a3fd3aea55b2d4c1b8a664c996520ae1319e98ea6050b868665d` / 1691480 | 10.0.26100.7705 (WinBuild.160101.0800) |
| user32.dll | `b3cc6f999873061683021abd4aeea6551f0a56a642c6f90c77744e10488248d9` | `bff75906f4abcc050cf85e38e20090ea0edf7dc33e0ed60cb74cf9e1bc007527` / 1972624 | 10.0.26100.8117 (WinBuild.160101.0800) |
| vcruntime140.dll | `2fa6efc053203460a23d3a25158f227d895d2dadc63acc1a372da97c3a4281c3` | `2fa6efc053203460a23d3a25158f227d895d2dadc63acc1a372da97c3a4281c3` / 123328 | 14.51.36247.0 |
| vcruntime140d.dll | `061444aba60e5ec1b5bdb86633caaba8b05743b2fd5a4e4e8b9d8e1346b7e4b7` | `061444aba60e5ec1b5bdb86633caaba8b05743b2fd5a4e4e8b9d8e1346b7e4b7` / 131688 | 14.44.35211.0 |
| win32u.dll | `8cf8d9ba313e690e4f0ec59feea42feb9539d053caccdc6e0c62fa9d2b471672` | `7183521868938b0f6c131ef58288aed6cb55e4bde34d27d9612616ddc4dc2b33` / 108120 | 10.0.26100.9444 (WinBuild.160101.0800) |
| winmm.dll | `d4e679bcc39588ed6bd52251d7beb57169d4a62d255d2596c0872476d8db870a` | `eefb711e78a3e2031ff2015d04471da7258945f40bce2dd54433f664c2d667bf` / 195912 | 10.0.26100.9278 (WinBuild.160101.0800) |
| ws2_32.dll | `33dfca33070a9e42c6d1924d74135c04a86fa47f6c8c236bb8960e1f9b051742` | `4da35234f9306c1de886560eb34ec105e80a2da3917f59de66b349e969b3dbc5` / 426296 | 10.0.26100.9444 (WinBuild.160101.0800) |

## Policy bağı ve geçiş

Eski JSON kaynaklarının birebir snapshot'ı `out/verification/engine/before-os-9445-profile/` altında, hash karşılaştırması `os-9445-profile-migration.json` içindedir. Yalnız eski binary/policy ile elde edilmiş çıktı yeni kimlikle yeniden etiketlenmez.

| Kaynak | Eski SHA-256 | Yeni SHA-256 |
|---|---|---|
| [application-entry-policy.json](../../contracts/engine/application-entry-policy.json) | `d683a244ece6bcfeffcb78f461daeb1232b6fd7a33c6e7f7055fdbdb8bd5bca9` | `058b5def00081ec2292c92d32173c3c53956663b78b1116138d892963be08800` |
| [application-routing-policy.json](../../contracts/engine/application-routing-policy.json) | `d2794efb7784cb53b20d693533f5ab67c6f05a8fbd4c2d77264942910c408f66` | `b97ecbac6dc05cc18e90af42274e1b148e741fc62012cddce98b86a90809e88e` |
| [asi-policy.json](../../contracts/engine/asi-policy.json) | `42947538ada61d7a163ff3a085c17b5218a8d702a7fe44445cfd2a2be154b1e8` | `ac29aeefa1b2a2ccb54777a9c96ee40248f90b244c51e0587eca8912664407c7` |
| [binding-policy.json](../../contracts/engine/binding-policy.json) | `92856899cf2250668e1b644d9ec2c111b01e72eba46643bb4631285ee3de84eb` | `97959fa701ac22cfb309110ae9e67f21a113cc8808b7b6e90ea68300d5b40cf2` |
| [bootstrap-lifecycle-policy.json](../../contracts/engine/bootstrap-lifecycle-policy.json) | `cd6f7822e4916a1082ec502cc24e45915cefd0e78f5e17eb894655f17443948e` | `8bb3a30af35985c8b9d0f86612ec54a6d625d0b799739b395c124e38245f28d4` |
| [codec-policy.json](../../contracts/engine/codec-policy.json) | `425272040c068e1e73b0cc1a9c43f9328b4dd13a2a00441efdbebe0a2372348d` | `35bf28c6927e31dc3415f7f3890a277217556e27b551f8ee3e65342cbd1c62a2` |
| [crt-startup-policy.json](../../contracts/engine/crt-startup-policy.json) | `86d01866ee719968fb7f4349fd2bc51ea7a749f88a1db7dd6794502d9ec1f67b` | `7030484e097150fabc0c6858715bde074fa05762d87cdd4fb6560adddb6a7fce` |
| [cwd-acquire-policy.json](../../contracts/engine/cwd-acquire-policy.json) | `de8ff2899242f67396333ad305102eaf91a2afc9f21034962b2b1c0837dd3fd9` | `d082f21922b19c205d17a209666d8d862fe7991ffd5ce33305d9a5e2020055df` |
| [cwd-lock-policy.json](../../contracts/engine/cwd-lock-policy.json) | `098567bb8f34f0619e5a31f3c0f760e89d32af659ec4b9fec633245215ac6327` | `55f3fdceaebbcf44ff074506cc2265b697f8f8323010f841165abe5bca22e39e` |
| [cwd-query-policy.json](../../contracts/engine/cwd-query-policy.json) | `73dcc9c721a519001c25dd0d5905fd16875c7cdd8f857ac28e374d37db9e3167` | `8f676b4bfedba10a41fe041b9d80b1f4aa94e71969d7ac25a5c9e138b87dc289` |
| [cwd-seh-policy.json](../../contracts/engine/cwd-seh-policy.json) | `995b9d5bc9223df40245583de1ba6226d04b372fc4585d046df039d7859cd198` | `8d2309d8b8c5fde7a0414fe13243f1173242c1518a318d4856f45752716a867c` |
| [entry-policy.json](../../contracts/engine/entry-policy.json) | `56328c7da0b0de6f3584e526b5fae3484634af927ce3a2cc8ace311112166cb2` | `007e352551e021b4dd647866f9b58bb100a31a2be280134f6cf296b23f5453f1` |
| [event-dispatch-policy.json](../../contracts/engine/event-dispatch-policy.json) | `6265420804ada87df4beae0fc034fcb0478ad408dd05d04e9173d6d02f681058` | `77662469d61f491223d64f12e4f4c5176c8a75d871090dd3dd5b6176e9fe3a1d` |
| [file-manager-entry-policy.json](../../contracts/engine/file-manager-entry-policy.json) | `0e65b724069c440804e61d3aad88fa781ff490ffd757a7928c62c318e63cc2a4` | `a7dfec624fcba478c2d9fa69ba34f2f78c33ed5d66f00fa3427a63aa4cfb536b` |
| [frame-target-policy.json](../../contracts/engine/frame-target-policy.json) | `03b425ca10f2187fdc11a635798ccca269b8716c36bfe3f06a41e0f19315794d` | `8bfb3b0974dc2222b6459eb2a323516ce08b568c8f59412aacdb2c9993810d0b` |
| [game-prelude-policy.json](../../contracts/engine/game-prelude-policy.json) | `eee50036b97098d00b2c3be58df01fa3724c04123884a3fdc4a75e4305ccab4d` | `ab85c8782ca66f94bd52dc6137452c6b07aed12b38725f76b788240bf82c7bc6` |
| [instance-startup-policy.json](../../contracts/engine/instance-startup-policy.json) | `26c06199eaa5d4e14c1e693ac60bd6df5c69cb3557e80b1baad320d7fc46f7e9` | `911109662368bc58f07a22c7849f9b55c48beca958e8b002b6971437c194730b` |
| [loader-policy.json](../../contracts/engine/loader-policy.json) | `91bb9479a01a91fea9161939ed7ac063fce541c9bb9bd35e1a4c3c8e4dce49a8` | `6bd379294368d81d4de7b1a10756eb3ea9d71c8cae1b6b12cca1b7a3681c2287` |
| [platform-startup-policy.json](../../contracts/engine/platform-startup-policy.json) | `a5c666a8b72a2e4576770419fc801c8210416a08e54440f983f4993195bd35c7` | `33433396cd874a1a3744d8b713559529410706a1e271f26fe214bdb07aace082` |
| [platform-suppression-policy.json](../../contracts/engine/platform-suppression-policy.json) | `222da1c0f85ee4747b2f0010d3eadda50c85a560724201d5fab345368fa8acda` | `8833e2adb596afb6d0aeae54660054ea4d212ebf584c27a137c789e992a6c323` |
| [proxy-policy.json](../../contracts/engine/proxy-policy.json) | `20ff7fb664b56f3a43cded86faaa15743495e0cc1d9f17ccb675ccf6732fddcf` | `15adfca7cf644462ae99dc8dd9a7db3368d86cec0c8900a559d32bd43348109b` |
| [startup-policy.json](../../contracts/engine/startup-policy.json) | `ed2834167df81d2068197dd333e7f28602dc243adcd8d80290498c41508caf30` | `7d187537f9165399b14fce1b35ad6c45f9e029ebb78b969e467b25f09852b082` |
| [startup-return-policy.json](../../contracts/engine/startup-return-policy.json) | `57d4d3867122c623e8132d61a0866c56e0055949074c3ea098f54a5fd087aeb7` | `497aece5ffd9a9e82c61a7148cfddef5e35ed60ed771e6b82d11f7a99573fe4f` |

## Hata davranışı, genişleme ve kabul

- Bir tek sistem dosyası dahi beklenenden farklıysa `loader_policy_file_mismatch`; eksikse `loader_policy_file_unavailable`. Child başlamaz. Runtime'da ek/uyumsuz mapping yine reddedilir. İmza veya Windows build numarası hash kontrolünü bypass etmez.
- Herhangi bir ancestor JSON'un byte'ları değiştirilip zincir yenilenmezse query generator reddeder. Yeni negatif test bütün ancestor kaynaklarını ayrı ayrı değiştirir; kernel32/kernelbase karma sürüm hash/boyutları da reddedilir.
- Yeni OS desteği, hash/export/source incelemesi ve yeniden üretimden sonra fixture ve gerçek GTA kanıtıyla açılır. Otomatik OS keşfi/izin, wildcard veya ENet fallback eklenmemiştir.
- Kabul: yeni profilde query'nin doğal API dönüşü ve eski acquire terminali; eski profil binary'sinde child öncesi drift reddi; bilinmeyen exe; eksik/bozuk yan DLL; 126/127-byte sınırı; yeni instance ve mevcut named-event çakışması; Debug/Release tekrarlarında bütün owned child'ların çıkışı ve orijinal girdilerin korunumu.

## Kanıt durumu

Statik inceleme ve kaynak geçişi tamamlandı. Dört Windows build'i ve yeni profil GTA deneyleri bu kesitin sonunda aşağıda kaydedilir. Henüz sonuç yazılmamış bir kontrol başarılı sayılmaz. Linux, hosted CI ve N1 SDK deneyi bu kaynak geçişiyle kendiliğinden doğrulanmış olmaz.

## İlk GTA denemesi ve CLI yığın düzeltmesi

İlk yeni profil Debug koşusu `0xC00000FD` (stack overflow) ile, JSON üretmeden sonlandı. `os-9445-initial-Debug.json` ham exit/stdout/stderr ve girdi hash'lerini korur. Bundan önce geçen fixture/CLI ret testleri gerçek GTA kabulünden sonraki derin CLI yolunu kapsamamıştı. Başarısız koşu başarılı query kanıtı sayılmaz.

CLI içindeki çok sayıda büyük `LoaderTrace` hata kaydı tek ortak heap hata yoluna taşındı; ana gözlem kaydı da heap'te tutuluyor. `run_impl` ve terminal izinleri değişmedi; Windows varsayılan stack rezervi büyütülmedi. `std::unique_ptr` kayıt ömrünü yönetir, borrowed dosya/child ömrü önceki sırayla sonlanır. Hata çıktısında child PID ve doğrulanmış exit bilgisi korunur. C++ yeniden derleme gerekir; JSON alanları ve C ABI aynı kaldı.

İlk düzeltme tekrarında gerçek GTA `cwd_query_verified` verdi: yedi durak, 83-byte dizin, bütün query korunumları ve child çıkışı doğrulandı. Bu keşif sonucu `os-9445-stack-fix-Debug.json` içindedir; nihai dört build ve tam matris aşağıda ayrıca kaydedilir. Standart testler GTA gerektirmez; bu CLI stack regresyonunun pozitif kanıtı explicit özel GTA matrisidir.

## Nihai doğrulama — 14 Eylül 2026

| Windows akışı | Native suite | Managed | Python | Sonuç |
|---|---|---|---|---|
| x86 Debug | 43 | 79 | 263 | geçti |
| x86 Release | 43 | 79 | 263 | geçti |
| x64 Debug | 20 | 79 | 192 | geçti |
| x64 Release | 20 | 79 | 192 | geçti |

Yeni iki generator testiyle birlikte x86 263, x64 192 Python testi geçti. Query fixture: 583 portable kontrol, 12 native senaryo, post-query canary ve 12 warm çevrim korunur. Dört build aynı **293 kaynak/config hash'i** ile tamamlandı. İlk stack-fix öncesi Debug build'i ve başarısız GTA koşusu nihai sonuç tablosuna dahil değildir.

GTA matrisi **50/50 beklenen sonuç**, **41/41 yaratılan child çıkışı** ve **134/134 girdi hash'i korunumu** verdi. Normal dizinde Debug/Release altışar query, 126-byte dizinde birer query: toplam **14 pozitif query**. 127-byte dizin iki build'de `cwd_query_suffix_capacity` ile kopyalama öncesi reddedildi; API dönüşü/stack/root/kilit/SEH/cookie/guard korunumları doğru kaldı.

22 önceki mod Release'te kendi terminalinde başarı verdi; acquire Debug'ta da tekrarlandı. Eski Debug/Release probe'larında query/acquire dört girişin tamamı `advapi32.dll` drift ile child öncesi reddedildi. Yeni binary'de yanlış context, yanlış ASI artifact, eksik ASI, bozuk codec ve bilinmeyen executable child öncesi reddedildi. Mevcut named event `183`, aynı adlı mutex `6` ile query'ye geçilmeden reddedildi; pencere aktivasyonu açılmadı. Deneyin kendi manual-reset event'i sinyallenmedi, mevcut oyun event'i tüketilmedi. Host setting deney öncesi/sonrası aynı kaldı.

Kritik bölüm yeni NTDLL üzerinde GTA image `0x00C9ADF0` adresinde, `MEM_IMAGE/PAGE_EXECUTE_READWRITE` olarak doğrulandı. LockCount `FFFFFFFF→FFFFFFFE`, recursion `0→1`, owner `0→owned main thread` oldu ve query boyunca korundu. Yeni probe ve eski probe PE stack rezervi aynı **1 MiB**; taşma heap kayıt düzenlemesiyle çözüldü. Terminal `0x836E30`; copy/helper return/unlock/SEH removal çalıştırılmadı. Standart build GTA çalıştırmaz; gerçek GTA kanıtı bu explicit yerel matristir. Linux/hosted/N1 bu kesitte tekrar çalıştırılmadı.

Kanıt: `out/verification/engine/os-9445-gta-evidence.json`, `os-9445-final-verification.json`, `os-9445-contract-diff.json`, `os-9445-build-inputs.json`, `build-os-9445-*-final.log` ve `os-9445-*-native-final.log`. Kaynak/kanal incelemesi eski ve yeni 23 policy'de stage, oyun RVA/body, modül sayısı/origin/basis ve şema anahtarlarının korunduğunu denetledi. Orijinal GTA native dosyaları ve önceki deney girdileri değiştirilmedi; yeni yerel kopyalar Git dışındaki `out/experiments` içindedir.

| Artifact | Debug SHA-256 | Release SHA-256 |
|---|---|---|
| saex_engine_loader_probe.exe | `c3e3eb702c3e3506d6262163730330b10f08e9bcf62404f7f72447052a598f5e` | `8e5bdb755af0d4eb6e330de398e6b7345aab5bdf9b6bb42f1635c9f44029b855` |
| saex_bootstrap.dll | `76bddbac6984b6f77c8176c5be160ea241724e01bf9ce0dd9d8635248e26acbe` | `eaab67ac564fb52e3db767ec4efbf4c0d3b6ed0aeac64d6c34c35d384d8a3273` |
| saex_bootstrap.map | `79be4aa18b9a0f67f6c29ae2687bfd8cb4a4352f542a144018150f5533c874b0` | `d62c1e2fd00946af04c8af5c29cf1c0631b6ec395fc68f9d068b9d25fdfec241` |

Son belge kapısı: **113 Markdown / 1951 yerel bağlantı / 6 JSON örneği**, sıfır hata. `git diff --check` geçti.

## 0.1.34 — Doğal cwd copy bağı

[Ayrı copy kesiti](d1-cwd-copy.md), query'den sonra native kontrol/CALL/dönüş ve gerçek hedef içerik kanıtını ekler. Önceki komutların terminali korunur; helper/unlock/SEH sökümü yeni izin kapsamına girmez. Source/guard, caller/SEH/kilit/cookie denetimleri ve yeni test/GTA kanıtının kapsamı ilgili rapordadır. C ABI 1, OS modül pinleri, GNS/otorite ve production kapıları aynı kalır; C++ observer yeniden derlenir.

## 0.1.35 — Cwd helper dönüş bağı

[Ayrı return kesiti](d1-cwd-return.md) copy sonrasındaki iki POP, cookie checker eşitliği ve doğal LEAVE/RET'i açar. Önceki terminal izinleri korunur; wrapper/unlock/SEH işlemleri henüz açılmaz. Kaynak stack ömrü, CALL ile değişen saved slot, hedef/caller/kilit/SEH ve hata retleri sözleşmede açıklanır. C++ trace yeniden derlenir; C ABI 1, OS pinleri, GNS/otorite ve production kapıları aynı kalır. Yeni fixture/gerçek GTA kanıtı ilgili raporda ayrı izlenir.

## 0.1.36 — Dosya yöneticisinin tamamlanması

Aynı 26200.9445 dosya pinleriyle RtlLeaveCriticalSection RVA 0x53460/prefix20 ve HIGHLOW yokluğu salt okunur doğrulandı. Yeni izin mevcut OS kimliğine bağlıdır; otomatik profil yenilemesi, farklı DLL fallback veya eski OS dosyası yüklemesi eklenmedi. [Sözleşme, kullanıcı komutu ve doğrulama](d1-file-manager-ready.md).

## Kod 0.1.37 — Streaming tablo kesitiyle bağlantı

[CdStream tablo sözleşmesi](d1-cd-stream-tables.md) ortak observer/CLI ve fixture zincirine ayrı bir üst mod ekler. Bu belgenin eski komut ve checkpoint sınırı korunur; yalnız `--observe-cd-stream-tables` tam manager dönüşünden sonra iki tablo döngüsünü ve disk argüman hazırlığını açar. Sonuç yeni `cdStreamTablesObservation` alanında izlenir; eski kayıtlar final durum değil önceki checkpoint snapshot'ıdır. C++ trace tüketicileri yeniden derlenir; C ABI 1, GNS, OS pinleri ve production IPC sınırı değişmez. Yeni portable/native testler ile eski mod regresyonları standart build'e dahildir; gerçek GTA ve platform bazındaki final kanıt ana raporda tutulur.

## Kod 0.1.38 — Disk sonucu ve allocation önkoşulu

[Disk hazırlığı sözleşmesi](d1-cd-stream-disk.md) önceki native zincire ayrı `--observe-cd-stream-disk` modu ekler. BOOL başarısızsa dört output kullanılmadan ret; başarılı ve kabul edilen mantıksal geometride doğal bayrak/argüman hazırlığı, 0x406BF4 allocation CALL önünde doğrulanır. Eski modların terminal ve snapshot anlamı korunur. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS, network otoritesi ve sandbox kapsamı değişmez. Gerçek allocation, fiziksel hizalama, dosya okuma ve thread/renderer hazır kanıtı bu değişiklikten çıkarılamaz. Portable hata kararı ile native/gerçek GTA kanıtının ayrımı yeni raporun kabul tablosunda izlenir.

## Kod 0.1.39 — Hizalı tamponun doğal dönüşü

[Allocation sözleşmesi](d1-cd-stream-allocation.md) ayrı `--observe-cd-stream-allocation` API/CLI ile MallocAlign → CRT → HeapAlloc → back-pointer → 0x406BF9 doğal dönüşünü ekler. Heap modu/new-handler/SBH dalı yürütmeden önce denetlenir; NULL, taşma, metadata ve payload bütünlüğü guard'ları vardır. Eski alt modların terminalleri ve snapshot anlamı korunur; yeni mod 160, eskiler 128 olay üst sınırındadır. C++ trace tüketicileri yeniden derlenir; bootstrap C ABI 1, OS pinleri, GNS/HTTPS ve sandbox kapsamı değişmez. İlk gerçek GTA allocation geçti; güncel toplu kanıt yeni sözleşmede izlenir. Native free, I/O/thread, renderer ve D1/D2 hazır kabul edilmez.

## Kod 0.1.40 — Kanal belleği kesiti

[Yeni sözleşme](d1-cd-stream-channels.md) `run_cd_stream_channels` / `--observe-cd-stream-channels` ile SetLastError ve LocalAlloc doğal yolunu, 5 × 48 sıfır byte ve global pointer kaydını ekler. Terminal 0x406C34, arşiv CALL önüdür. Önceki allocation/parent kayıtları kendi duraklarının snapshot anlamını korur; canlı tabloda yalnız kanal sayısı/etkin sayı DWORD çifti değişebilir. C++ trace tüketicileri yeniden derlenir; C ABI 1 ve mevcut OS pinleri aynıdır. Allocation ve yeni mod 160, daha eski modlar 128 olay sınırındadır. Native free, dosya açma/okuma, thread, renderer ve D1/D2 kapıları açıktır. Güncel test ve GTA kanıtı yeni sözleşmede tutulur.
