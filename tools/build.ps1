[CmdletBinding()]
param(
    [ValidateSet('x64', 'x86')][string]$Architecture = 'x64',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [string]$Python = 'python',
    [string]$DocumentationBase = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$taskRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
function Invoke-Checked([string]$Executable, [string[]]$Arguments) {
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed with exit code $LASTEXITCODE" }
}
Push-Location $taskRoot
try {
    $taskDocArgs = @('tools/check_docs.py')
    if ($DocumentationBase) { $taskDocArgs += @('--base', $DocumentationBase) }
    Invoke-Checked $Python $taskDocArgs
    Invoke-Checked 'dotnet' @('run', '--project', 'tools/ContractGen', '--', '.', '--check')
    Invoke-Checked $Python @('tools/engine_profiles.py', '--check')
    Invoke-Checked $Python @('tools/loader_policy.py', '--check')
    Invoke-Checked $Python @('tools/entry_policy.py', '--check')
    Invoke-Checked $Python @('tools/proxy_policy.py', '--check')
    Invoke-Checked $Python @('tools/startup_policy.py', '--check')
    Invoke-Checked 'cmake' @('--preset', "windows-$Architecture")
    Invoke-Checked 'cmake' @('--build', '--preset', "windows-$Architecture", '--config', $Configuration)
    if ($Architecture -eq 'x86') {
        $taskBootstrap = "out/windows-x86/$Configuration/saex_bootstrap"
        Invoke-Checked $Python @('tools/check_bootstrap.py', "$taskBootstrap.dll", "$taskBootstrap.map", '--record', "out/verification/engine/bootstrap-audit-$Configuration.json")
    }
    Invoke-Checked 'ctest' @('--preset', "windows-$Architecture", '-C', $Configuration)
    Invoke-Checked 'dotnet' @('run', '--project', 'tests/managed/Saex.Foundation.Tests', '-c', $Configuration, '--', '.', "out/windows-$Architecture/$Configuration/saex_contract_probe.exe")
    Invoke-Checked $Python @('tests/tooling/test_doc_gate.py')
    Invoke-Checked $Python @('tests/tooling/test_ci.py')
    Invoke-Checked $Python @('tests/engine/test_engine_profiles.py')
    Invoke-Checked $Python @('tests/engine/test_loader_policy.py')
    Invoke-Checked $Python @('tests/engine/test_entry_policy.py')
    Invoke-Checked $Python @('tests/engine/test_proxy_policy.py')
    Invoke-Checked $Python @('tests/engine/test_startup_policy.py')
    Invoke-Checked $Python @('tests/engine/test_engine_probe.py', "out/windows-$Architecture/$Configuration/saex_engine_image_probe.exe")
    Invoke-Checked $Python @('tests/engine/test_process_probe.py', "out/windows-$Architecture/$Configuration/saex_engine_process_probe.exe")
    if ($Architecture -eq 'x86') {
        Invoke-Checked $Python @('tests/engine/test_bootstrap_audit.py', "$taskBootstrap.dll", "$taskBootstrap.map")
        Invoke-Checked $Python @('tests/engine/test_loader_probe.py', "out/windows-x86/$Configuration/saex_engine_loader_probe.exe")
    }
    Invoke-Checked $Python ($taskDocArgs + @('--record'))
    Write-Output "SAEX D1 foundation verified ($Architecture/$Configuration); GTA integration remains unverified."
}
finally { Pop-Location }
