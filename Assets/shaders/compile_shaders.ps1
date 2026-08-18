$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$dxc = Get-Command dxc -ErrorAction SilentlyContinue
if (-not $dxc) {
    throw "dxc not found. Open a terminal with Vulkan SDK or add dxc to PATH."
}

$shaders = @(
    @{ Source = "triangle.vert.hlsl"; Target = "vs_6_0" },
    @{ Source = "triangle.frag.hlsl"; Target = "ps_6_0" },
    @{ Source = "present.vert.hlsl";  Target = "vs_6_0" },
    @{ Source = "present.frag.hlsl";  Target = "ps_6_0" }
)

foreach ($shader in $shaders) {
    $source = Join-Path $scriptDir $shader.Source
    $output = [System.IO.Path]::ChangeExtension($source, ".spv")
    & dxc -spirv -T $shader.Target -E main -Fo $output $source
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to compile $($shader.Source)."
    }
    Write-Host "[OK] $($shader.Source) -> $(Split-Path -Leaf $output)"
}
