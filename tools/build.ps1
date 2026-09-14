[CmdletBinding()]
param(
    [ValidateSet('x64', 'x86')][string]$Architecture = 'x64',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [string]$Python = 'python',
    [string]$DocumentationBase = '',
    [ValidateSet('2022', '2026')][string]$VisualStudio = '2022'
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
    Invoke-Checked $Python @('tools/codec_policy.py', '--check')
    Invoke-Checked $Python @('tools/binding_policy.py', '--check')
    Invoke-Checked $Python @('tools/asi_policy.py', '--check')
    Invoke-Checked $Python @('tools/bootstrap_lifecycle_policy.py', '--check')
    Invoke-Checked $Python @('tools/frame_target_policy.py', '--check')
    Invoke-Checked $Python @('tools/startup_return_policy.py', '--check')
    Invoke-Checked $Python @('tools/crt_startup_policy.py', '--check')
    Invoke-Checked $Python @('tools/application_entry_policy.py', '--check')
    Invoke-Checked $Python @('tools/platform_startup_policy.py', '--check')
    Invoke-Checked $Python @('tools/platform_suppression_policy.py', '--check')
    Invoke-Checked $Python @('tools/instance_startup_policy.py', '--check')
    Invoke-Checked $Python @('tools/event_dispatch_policy.py', '--check')
    Invoke-Checked $Python @('tools/application_routing_policy.py', '--check')
    Invoke-Checked $Python @('tools/game_prelude_policy.py', '--check')
    Invoke-Checked $Python @('tools/file_manager_entry_policy.py', '--check')
    Invoke-Checked $Python @('tools/cwd_seh_policy.py', '--check')
    Invoke-Checked $Python @('tools/cwd_lock_policy.py', '--check')
    Invoke-Checked $Python @('tools/cwd_acquire_policy.py', '--check')
    $taskConfigureArgs = @('--preset', "windows-$Architecture")
    if ($VisualStudio -eq '2026') {
        $taskConfigureArgs += @('-G', 'Visual Studio 18 2026', '-T', 'v143')
    }
    if ($Architecture -eq 'x86') {
        $taskPythonPath = (Get-Command -Name $Python -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
        $taskConfigureArgs += "-DPython3_EXECUTABLE=$taskPythonPath"
    }
    Invoke-Checked 'cmake' $taskConfigureArgs
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
    Invoke-Checked $Python @('tests/engine/test_codec_policy.py')
    Invoke-Checked $Python @('tests/engine/test_binding_policy.py')
    Invoke-Checked $Python @('tests/engine/test_asi_policy.py')
    Invoke-Checked $Python @('tests/engine/test_bootstrap_lifecycle_policy.py')
    Invoke-Checked $Python @('tests/engine/test_frame_target_policy.py')
    Invoke-Checked $Python @('tests/engine/test_startup_return_policy.py')
    Invoke-Checked $Python @('tests/engine/test_crt_startup_policy.py')
    Invoke-Checked $Python @('tests/engine/test_application_entry_policy.py')
    Invoke-Checked $Python @('tests/engine/test_platform_startup_policy.py')
    Invoke-Checked $Python @('tests/engine/test_platform_suppression_policy.py')
    Invoke-Checked $Python @('tests/engine/test_instance_startup_policy.py')
    Invoke-Checked $Python @('tests/engine/test_event_dispatch_policy.py')
    Invoke-Checked $Python @('tests/engine/test_application_routing_policy.py')
    Invoke-Checked $Python @('tests/engine/test_game_prelude_policy.py')
    Invoke-Checked $Python @('tests/engine/test_file_manager_entry_policy.py')
    Invoke-Checked $Python @('tests/engine/test_cwd_seh_policy.py')
    Invoke-Checked $Python @('tests/engine/test_cwd_lock_policy.py')
    Invoke-Checked $Python @('tests/engine/test_cwd_acquire_policy.py')
    Invoke-Checked $Python @('tests/engine/test_image_protection_probe.py')
    Invoke-Checked $Python @('tests/engine/test_engine_probe.py', "out/windows-$Architecture/$Configuration/saex_engine_image_probe.exe")
    Invoke-Checked $Python @('tests/engine/test_process_probe.py', "out/windows-$Architecture/$Configuration/saex_engine_process_probe.exe")
    if ($Architecture -eq 'x86') {
        Invoke-Checked $Python @('tests/engine/test_bootstrap_audit.py', "$taskBootstrap.dll", "$taskBootstrap.map")
        Invoke-Checked $Python @('tests/engine/test_bootstrap_load_artifact.py', "$taskBootstrap.dll", "$taskBootstrap.map")
        Invoke-Checked $Python @('tests/engine/test_loader_probe.py', "out/windows-x86/$Configuration/saex_engine_loader_probe.exe")
    }
    Invoke-Checked $Python ($taskDocArgs + @('--record'))
    Write-Output "SAEX D1 foundation verified ($Architecture/$Configuration); GTA integration remains unverified."
}
finally { Pop-Location }
