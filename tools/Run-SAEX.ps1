[CmdletBinding()]
param(
    [string]$GameDirectory = 'C:\Program Files (x86)\Rockstar Games\GTA San Andreas',
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [switch]$OpenReport
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$taskRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,6)
$taskOutput = Join-Path $taskRoot "out/local-demo/$taskId"
$taskCapsule = Join-Path $taskOutput 'game'
$taskInputs = [ordered]@{}
$taskRows = [Collections.Generic.List[object]]::new()
$taskTrace = $null
$taskFailure = ''
$taskStarted = [DateTimeOffset]::Now
$taskClock = [Diagnostics.Stopwatch]::StartNew()
$taskPassed = $false
$taskUnchanged = $false
$taskProbeExit = $null
$taskUtf8 = [Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = $taskUtf8
function Get-Digest([string]$Path) {
    $taskStream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    $taskSha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($taskSha.ComputeHash($taskStream)).Replace('-', '').ToLowerInvariant() }
    finally { $taskSha.Dispose(); $taskStream.Dispose() }
}
function Add-Check([string]$Name, [bool]$Passed, [string]$Detail) {
    $taskRows.Add([ordered]@{ name=$Name; passed=$Passed; detail=$Detail })
    $taskLabel = if ($Passed) { 'GEÇTİ' } else { 'DURDU' }
    Write-Host "[$taskLabel] $Name — $Detail"
}
function Read-Input([string]$Path, [string]$Expected = '', [long]$Bytes = -1) {
    $taskFile = Get-Item -LiteralPath $Path
    if ($taskFile.PSIsContainer) { throw "Dosya bekleniyordu: $Path" }
    $taskHash = Get-Digest $taskFile.FullName
    $taskInputs[$taskFile.FullName] = $taskHash
    if (($Bytes -ge 0 -and $taskFile.Length -ne $Bytes) -or ($Expected -and $taskHash -ne $Expected)) {
        throw "Dosya kimliği desteklenen profille uyuşmuyor: $($taskFile.Name)"
    }
    return $taskFile.FullName
}
New-Item -ItemType Directory -Path $taskOutput | Out-Null
Write-Host "SAEX — D1 motor doğrulaması ($Configuration)"
try {
    $taskGame = (Get-Item -LiteralPath $GameDirectory).FullName
    if (-not (Test-Path -LiteralPath $taskGame -PathType Container)) { throw 'Oyun klasörü bulunamadı.' }
    # The reviewed CRT A API reserves one byte for backslash and one for NUL.
    if ($taskCapsule.Length -gt 126 -or $taskCapsule -match '[^\x20-\x7e]') {
        throw 'Bu motor denemesi için depo yolu ASCII olmalı ve üretilen oyun klasörü 126 karakteri aşmamalı.'
    }
    $taskPolicyPath = Join-Path $taskRoot 'contracts/engine/loader-policy.json'
    $taskCodecPath = Join-Path $taskRoot 'contracts/engine/codec-policy.json'
    $null = Read-Input $taskPolicyPath
    $null = Read-Input $taskCodecPath
    $taskPolicy = Get-Content -LiteralPath $taskPolicyPath -Raw | ConvertFrom-Json
    $taskCodec = Get-Content -LiteralPath $taskCodecPath -Raw | ConvertFrom-Json
    $taskExe = Read-Input (Join-Path $taskGame 'gta_sa.exe') $taskPolicy.engineSha256
    $taskBuild = Join-Path $taskRoot "out/windows-x86/$Configuration"
    $taskProbe = Read-Input (Join-Path $taskBuild 'saex_engine_loader_probe.exe')
    $taskBootstrap = Read-Input (Join-Path $taskBuild 'saex_bootstrap.dll')
    $taskSources = [ordered]@{ 'saex-engine-observation.exe'=$taskExe; 'saex_bootstrap.asi'=$taskBootstrap }
    $taskGameModules = @($taskPolicy.modules | Where-Object origin -eq 'game-root') + @($taskCodec.modules)
    foreach ($taskModule in $taskGameModules) {
        if ($taskModule.name -notmatch '^[a-z0-9_-]+\.dll$' -or $taskSources.Contains($taskModule.name)) { throw 'Geçersiz veya tekrarlı modül adı.' }
        $taskSources[$taskModule.name] = Read-Input (Join-Path $taskGame $taskModule.name) $taskModule.sha256 $taskModule.bytes
    }
    Add-Check 'Yerel oyun ve araç dosyaları' $true 'Desteklenen GTA kimliği ve gerekli dosyalar bulundu.'
    New-Item -ItemType Directory -Path $taskCapsule | Out-Null
    foreach ($taskName in $taskSources.Keys) {
        $taskCopy = Join-Path $taskCapsule $taskName
        Copy-Item -LiteralPath $taskSources[$taskName] -Destination $taskCopy
        $null = Read-Input $taskCopy $taskInputs[$taskSources[$taskName]]
    }
    Add-Check 'Ayrı çalışma klasörü' $true 'Yedi yerel dosya kopyalandı; özgün oyun klasörüne yazılmadı.'
    $taskDiskPolicyPath = Read-Input (Join-Path $taskRoot 'contracts/engine/cd-stream-disk-policy.json')
    $taskAllocationPolicyPath = Read-Input (Join-Path $taskRoot 'contracts/engine/cd-stream-allocation-policy.json')
    $taskChannelsPolicyPath = Read-Input (Join-Path $taskRoot 'contracts/engine/cd-stream-channels-policy.json')
    # The probe owns its child, pins OS/artifact identities and terminates at the reviewed boundary.
    $taskRaw = & $taskProbe --observe-cd-stream-channels (Join-Path $taskCapsule 'saex-engine-observation.exe') $taskCapsule
    $taskProbeExit = $LASTEXITCODE
    $taskText = $taskRaw -join "`n"
    [IO.File]::WriteAllText((Join-Path $taskOutput 'engine-trace.json'), $taskText + "`n", $taskUtf8)
    if ([string]::IsNullOrWhiteSpace($taskText)) { throw "Gözlem aracı JSON çıktı üretmedi (çıkış $taskProbeExit)." }
    $taskTrace = $taskText | ConvertFrom-Json
    if ($null -eq $taskTrace -or -not $taskTrace.PSObject.Properties['reason']) { throw "Gözlem aracı geçerli sonuç üretmedi (çıkış $taskProbeExit)." }
    if ($taskProbeExit -ne 3 -or $taskTrace.reason -ne 'cd_stream_channels_verified') {
        throw "Motor doğrulaması ilerlemeyi durdurdu: $($taskTrace.reason) (çıkış $taskProbeExit)."
    }
    if ($taskTrace.cdStreamDiskObservation.policySourceDigest -ne $taskInputs[$taskDiskPolicyPath]) { throw 'Disk policy kaynağı ile probe digest eşleşmiyor.' }
    if ($taskTrace.cdStreamAllocationObservation.policySourceDigest -ne $taskInputs[$taskAllocationPolicyPath]) { throw 'Bellek policy kaynağı ile probe digest eşleşmiyor.' }
    $taskAllocation = $taskTrace.cdStreamAllocationObservation
    if ($taskTrace.cdStreamChannelsObservation.policySourceDigest -ne $taskInputs[$taskChannelsPolicyPath]) { throw 'Kanal policy kaynağı ile probe digest eşleşmiyor.' }
    $taskChannels = $taskTrace.cdStreamChannelsObservation
    $taskReady = $taskTrace.fileManagerReadyObservation
    $taskDisk = $taskTrace.cdStreamDiskObservation
    $taskPhaseChecks = @(
        @('Başlangıç zinciri', [bool]$taskTrace.applicationRoutingObservation.verified, 'GTA uygulama yönlendirmesine ulaşıldı.'),
        @('Çalışma yolu', [bool]($taskTrace.cwdQueryObservation.verified -and $taskTrace.cwdCopyObservation.verified -and $taskTrace.cwdReturnObservation.verified), 'Yol okundu, kopyalandı ve cookie kontrolü geçti.'),
        @('Kilit ve hata kaydı', [bool]($taskReady.lockReleased -and $taskReady.sehRemoved -and $taskReady.priorRecordPreserved), 'Kilit bırakıldı; önceki SEH zinciri geri geldi.'),
        @('Dosya yöneticisi', [bool]($taskReady.verified -and $taskReady.suffixWritten -and $taskReady.managerReturned -and $taskReady.bufferValid), 'Yol tamamlandı; CFileMgr::Initialise doğal olarak döndü.'),
        @('Streaming tabloları', [bool]($taskTrace.cdStreamTablesObservation.verified -and $taskTrace.cdStreamTablesObservation.tablesValid), '32 dosya kaydı ve 32 isim başlangıcı GTA tarafından temizlendi; disk sorgusu boyunca korundu.'),
        @('Disk sorgusu', [bool]($taskDisk.apiReturned -and $taskDisk.querySucceeded -and $taskDisk.geometryValid -and $taskDisk.lastErrorRead), "$($taskDisk.bytesPerSector) byte/sektör, $($taskDisk.sectorsPerCluster) sektör/küme ölçüldü."),
        @('Streaming bellek hazırlığı', [bool]($taskDisk.verified -and $taskDisk.globalsValid -and $taskDisk.argumentsValid -and $taskDisk.frameValid -and -not $taskDisk.allocationCallAllowed), '2048 byte tamponun argümanları ve streaming bayrakları çağrı öncesinde doğrulandı.'),
        @('Hizalı streaming tamponu', [bool]($taskAllocation.verified -and $taskAllocation.allocationSucceeded -and $taskAllocation.blockValid -and $taskAllocation.metadataValid -and $taskAllocation.contentPreserved -and $taskAllocation.sehPreserved -and -not $taskAllocation.nativeFreeVerified -and -not $taskAllocation.nextInitializationCallAllowed), 'GTA 2048 byte için hizalı tampon ayırdı; başlangıç adresi, bellek bütünlüğü ve doğal dönüş doğrulandı. Bellek bırakma henüz sınanmadı.'),
        @('Streaming kanal belleği', [bool]($taskChannels.verified -and $taskChannels.errorReset -and $taskChannels.allocationSucceeded -and $taskChannels.zeroInitialized -and $taskChannels.pointerPublished -and $taskChannels.tablesValid -and $taskChannels.allocationPreserved -and -not $taskChannels.fileOpenAllowed -and -not $taskChannels.nativeFreeVerified), 'Beş kanal için 240 byte sıfırlanmış bellek ve global kaydı doğrulandı. GTA3.IMG açma çağrısı önünde duruldu.'),
        @('Deneme sürecinin kapanışı', [bool]$taskTrace.childExitConfirmed, 'Oluşturulan GTA deneme sürecinin çıkışı doğrulandı.')
    )
    foreach ($taskCheck in $taskPhaseChecks) {
        Add-Check $taskCheck[0] $taskCheck[1] $taskCheck[2]
        if (-not $taskCheck[1]) { throw "Eksik motor kanıtı: $($taskCheck[0])" }
    }
    if ($taskTrace.canAttach -or $taskTrace.initializationVerified) { throw 'Bu D1 aracının kapsamı dışındaki hazır bilgisi reddedildi.' }
    $taskPassed = $true
} catch {
    $taskFailure = $_.Exception.Message
    Add-Check 'Doğrulama sonucu' $false $taskFailure
} finally {
    try {
        $taskUnchanged = $taskInputs.Count -gt 0
        foreach ($taskPath in $taskInputs.Keys) {
            if ((Get-Digest $taskPath) -ne $taskInputs[$taskPath]) { $taskUnchanged = $false }
        }
        Add-Check 'Dosya bütünlüğü' $taskUnchanged "$($taskInputs.Count) izlenen girdinin işlem sonrası hash kontrolü."
    } catch { $taskUnchanged = $false; $taskFailure += ' İşlem sonrası hash kontrolü tamamlanamadı.' }
    $taskPassed = $taskPassed -and $taskUnchanged
    $taskClock.Stop()
    $taskStatus = if ($taskPassed) { 'passed' } else { 'failed' }
    $taskReport = [ordered]@{
        schemaVersion=1; scope='saex-d1-local-engine-demo'; status=$taskStatus; startedAt=$taskStarted.ToString('o')
        durationSeconds=[Math]::Round($taskClock.Elapsed.TotalSeconds,2); configuration=$Configuration
        gameDirectory=$GameDirectory; capsule=$taskCapsule; probeExitCode=$taskProbeExit
        playable=$false; multiplayerReady=$false; inputHashesPreserved=$taskUnchanged
        failure=$taskFailure; checks=@($taskRows.ToArray()); inputs=$taskInputs
    }
    [IO.File]::WriteAllText((Join-Path $taskOutput 'report.json'), ($taskReport | ConvertTo-Json -Depth 8) + "`n", $taskUtf8)
    $taskTitle = if ($taskPassed) { 'Streaming kanal belleği doğrulandı.' } else { 'Doğrulama durdu.' }
    $taskTraceLink = if (Test-Path -LiteralPath (Join-Path $taskOutput 'engine-trace.json')) {
        ' · <a href="engine-trace.json">Motor gözlemi</a>'
    } else { ' · Motor çalıştırılmadı.' }
    $taskHtmlRows = ($taskRows | ForEach-Object {
        $taskClass = if ($_.passed) { 'pass' } else { 'fail' }
        $taskSymbol = if ($_.passed) { '✓' } else { '!' }
        '<li><span class="badge ' + $taskClass + '">' + $taskSymbol + '</span><div><strong>' + [Net.WebUtility]::HtmlEncode($_.name) + '</strong><p>' + [Net.WebUtility]::HtmlEncode($_.detail) + '</p></div></li>'
    }) -join "`n"
    $taskHtml = @"
<!doctype html><html lang="tr"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SAEX · Yerel motor doğrulaması</title>
<style>
:root{color-scheme:dark;font-family:Segoe UI,Arial,sans-serif;background:#10141c;color:#eaf0f7}*{box-sizing:border-box}body{margin:0;padding:44px 24px}main{max-width:1000px;margin:auto}.eyebrow{color:#61e4b1;letter-spacing:.18em;font-size:13px;font-weight:700}h1{font-size:clamp(32px,5vw,54px);margin:14px 0 12px;letter-spacing:-.035em}.intro{font-size:18px;color:#aebccc;line-height:1.6;max-width:760px}.meta{display:flex;gap:12px;flex-wrap:wrap;margin:28px 0}.meta span{border:1px solid #344254;border-radius:999px;padding:8px 14px;color:#cbd8e8;font-size:13px}.grid{display:grid;grid-template-columns:1.45fr 1fr;gap:24px}.card{background:#19212e;border:1px solid #2c3b4c;border-radius:18px;padding:26px}h2{font-size:20px;margin:0 0 18px}ul{list-style:none;padding:0;margin:0}li{display:flex;gap:14px;padding:14px 0;border-bottom:1px solid #2d3948}li:last-child{border-bottom:0}li p{margin:5px 0 0;color:#aebccc;line-height:1.5;font-size:14px}.badge{width:28px;height:28px;flex:none;display:grid;place-items:center;border-radius:50%;font-weight:bold}.pass{background:#173e35;color:#6dffc5}.fail{background:#542733;color:#ffb4c5}.next{color:#aebccc;line-height:1.7}.next strong{color:#eaf0f7}a{color:#75d4ff}footer{margin-top:24px;color:#889ab0;font-size:13px;line-height:1.7}details{margin-top:18px}summary{cursor:pointer}code{word-break:break-all;color:#c5d7ec}@media(max-width:740px){.grid{grid-template-columns:1fr}body{padding:28px 16px}.card{padding:20px}}
</style><main><div class="eyebrow">SAEX / SAN ANDREAS EXTENDED</div><h1>$taskTitle</h1>
<p class="intro">Yerel GTA dosyaları üzerinde çalışan D1 motor doğrulaması. Aşağıdaki sonuçlar bu çalıştırmada ölçüldü.</p>
<div class="meta"><span>$Configuration · x86</span><span>$($taskReport.durationSeconds) saniye</span><span>$($taskStarted.ToString('dd.MM.yyyy HH:mm:ss zzz'))</span><span>Oynanabilir sürüm henüz hazır değil</span></div>
<div class="grid"><section class="card"><h2>Bu çalıştırmanın sonuçları</h2><ul>$taskHtmlRows</ul></section>
<aside class="card"><h2>Buradan sonra</h2><div class="next"><p><strong>1 · Motor başlangıcı</strong><br>Streaming arşivinin açılması, dosya okuma, thread, doğal bellek bırakma ve ardından renderer.</p><p><strong>2 · Doğal oyun döngüsü</strong><br>Gerçek frame çağrısı, kontrollü bağlanma ve kapanış.</p><p><strong>3 · İki istemcili örnek</strong><br>GNS bağlantısı ve ortak dünya durumunun ilk doğrulaması.</p></div><p class="next">Bu araç bir oyun launcher'ı değildir. Pencere, oynanış ve multiplayer doğrulanmış sayılmaz.</p><details><summary>Teknik kayıtlar</summary><p><a href="report.json">Çalıştırma raporu</a>$taskTraceLink</p><code>$([Net.WebUtility]::HtmlEncode($taskCapsule))</code></details></aside></div>
<footer>Dosyalar yalnız bu makinede hazırlanır. Rapor ağ isteği yapmaz. Eski çalıştırmalar korunur. Ham kayıtlar yerel yolları içerir.</footer></main></html>
"@
    $taskReportPath = Join-Path $taskOutput 'report.html'
    [IO.File]::WriteAllText($taskReportPath, $taskHtml, $taskUtf8)
    Write-Host "Rapor: $taskReportPath"
    if ($OpenReport) { Invoke-Item -LiteralPath $taskReportPath }
}
if (-not $taskPassed) { exit 1 }
exit 0
