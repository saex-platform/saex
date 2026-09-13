[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [string]$Python = 'python',
    [switch]$Acquire
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$taskRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
function Invoke-SdkChecked([string]$Executable, [string[]]$Arguments) {
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed with exit code $LASTEXITCODE" }
}
Push-Location $taskRoot
try {
    Invoke-SdkChecked $Python @('tools/check_docs.py')
    if ($Acquire) { Invoke-SdkChecked $Python @('tools/native/dependency.py', 'acquire') }
    Invoke-SdkChecked $Python @('tools/native/dependency.py', 'verify')
    $taskDigest = (Get-FileHash -LiteralPath 'contracts/engine/plugin-sdk.lock.json' -Algorithm SHA256).Hash.ToLowerInvariant()
    $taskBuild = "out/native-sdk/$taskDigest"
    $taskPython = (& $Python -c 'import sys; print(sys.executable)').Trim()
    if ($LASTEXITCODE -ne 0) { throw 'Python discovery failed' }
    Invoke-SdkChecked 'cmake' @('-S', 'tools/native', '-B', $taskBuild, '-G', 'Visual Studio 17 2022', '-A', 'Win32', "-DPython3_EXECUTABLE=$taskPython")
    # Rebuild the tiny library from its locked source; never trust an old external .lib.
    Invoke-SdkChecked 'cmake' @('--build', $taskBuild, '--config', $Configuration, '--clean-first')
    Invoke-SdkChecked 'ctest' @('--test-dir', $taskBuild, '-C', $Configuration, '--output-on-failure')
    Invoke-SdkChecked $Python @('tests/engine/test_dependency.py')
    Invoke-SdkChecked $Python @('tests/engine/test_sdk_configure.py', $taskPython)
    Invoke-SdkChecked $Python @('tools/native/dependency.py', 'verify')
    if ((Get-FileHash -LiteralPath 'contracts/engine/plugin-sdk.lock.json' -Algorithm SHA256).Hash.ToLowerInvariant() -ne $taskDigest) {
        throw 'Dependency lock changed during build; result cannot be published'
    }
    $taskArtifacts = @()
    foreach ($taskName in @('saex_sdk_probe.exe', 'saex_sdk_probe.pdb', 'saex_sdk_private.lib')) {
        $taskPath = "$taskBuild/$Configuration/$taskName"
        $taskArtifacts += [ordered]@{ path = $taskPath; sha256 = (Get-FileHash -LiteralPath $taskPath -Algorithm SHA256).Hash.ToLowerInvariant(); bytes = (Get-Item -LiteralPath $taskPath).Length }
    }
    $taskRecord = [ordered]@{
        schemaVersion = 1; scope = 'D1-N1-build-only'; verified = $true; gtaEligible = $false
        utc = [DateTime]::UtcNow.ToString('o'); dependencyLockDigest = $taskDigest
        configuration = $Configuration
        crt = $(if ($Configuration -eq 'Debug') { 'MDd' } else { 'MD' })
        settings = Get-Content -Raw -LiteralPath "$taskBuild/sdk-build-settings.json" | ConvertFrom-Json
        artifacts = $taskArtifacts
        tests = @('engine.sdk_build_probe', 'dependency negative suite', 'x64/missing-source configure rejection')
    }
    New-Item -ItemType Directory -Force -Path 'out/verification/engine' | Out-Null
    $taskRecord | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 -LiteralPath "out/verification/engine/sdk-build-$Configuration.json"
    Write-Output "SAEX D1-N1 SDK build verified ($Configuration); no GTA access."
}
finally { Pop-Location }
