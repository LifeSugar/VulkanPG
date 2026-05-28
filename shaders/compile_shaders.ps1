$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$vert = Join-Path $scriptDir "triangle.vert.hlsl"
$frag = Join-Path $scriptDir "triangle.frag.hlsl"
$vertSpv = Join-Path $scriptDir "triangle.vert.spv"
$fragSpv = Join-Path $scriptDir "triangle.frag.spv"

$dxc = Get-Command dxc -ErrorAction SilentlyContinue
if (-not $dxc) {
    throw "dxc not found. Open a terminal with Vulkan SDK or add dxc to PATH."
}

& dxc -spirv -T vs_6_0 -E main -Fo $vertSpv $vert
& dxc -spirv -T ps_6_0 -E main -Fo $fragSpv $frag

Write-Host "[OK] Compiled shaders:"
Write-Host "  $vertSpv"
Write-Host "  $fragSpv"
