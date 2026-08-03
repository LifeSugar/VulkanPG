# Build APK from the pre-built libVulkanApp.so
param(
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path "$PSScriptRoot\.."
$SdkRoot = "$env:LOCALAPPDATA\Android\Sdk"
$BuildTools = "$SdkRoot\build-tools\android-15"
$PlatformJar = "$SdkRoot\platforms\android-35\android.jar"

$BuildDir = if ($Config -eq "Release") { "android-arm64-release" } else { "android-arm64-debug" }
$SoFile = "$RepoRoot\build\$BuildDir\libVulkanApp.so"

if (-not (Test-Path $SoFile)) {
    Write-Error "libVulkanApp.so not found at: $SoFile"
    Write-Error "Run: cmake --build build/$BuildDir first"
    exit 1
}

$OutDir = "$PSScriptRoot\build"
Remove-Item -Recurse -Force $OutDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$BaseZip = "$OutDir\base.zip"
$AlignedApk = "$OutDir\VulkanApp-aligned.apk"
$FinalApk = "$OutDir\VulkanApp.apk"

Write-Host "=== Building APK ($Config) ==="

# 1. Link manifest directly into base APK (no resources needed)
Write-Host "[1/4] Linking base APK from manifest..."
& "$BuildTools\aapt2.exe" link `
    -o $BaseZip `
    -I "$PlatformJar" `
    --manifest "$PSScriptRoot\AndroidManifest.xml" `
    -0 arsc
if ($LASTEXITCODE -ne 0) { throw "aapt2 link failed" }

# 2. Compile Java stub and create classes.dex
Write-Host "[2/4] Creating classes.dex..."
$ClassDir = "$OutDir\classes"
New-Item -ItemType Directory -Force -Path $ClassDir | Out-Null
& javac -d $ClassDir -source 1.8 -target 1.8 -bootclasspath "$PlatformJar" "$PSScriptRoot\Dummy.java"
if ($LASTEXITCODE -ne 0) { throw "javac failed" }

$d8Bat = "$BuildTools\d8.bat"
& cmd /c "$d8Bat --lib `"$PlatformJar`" --output `"$OutDir`" `"$ClassDir\com\vulkan\demo\Dummy.class`""
if ($LASTEXITCODE -ne 0) { throw "d8 failed" }

# 3. Add classes.dex and lib/ into the APK ZIP
Write-Host "[3/4] Packaging APK..."
Add-Type -AssemblyName System.IO.Compression.FileSystem

$ApkDir = "$OutDir\apk"
Remove-Item -Recurse -Force $ApkDir -ErrorAction SilentlyContinue
[System.IO.Compression.ZipFile]::ExtractToDirectory($BaseZip, $ApkDir)

# Copy classes.dex
Copy-Item "$OutDir\classes.dex" "$ApkDir\classes.dex" -Force

# Copy native library
New-Item -ItemType Directory -Force -Path "$ApkDir\lib\arm64-v8a" | Out-Null
Copy-Item $SoFile "$ApkDir\lib\arm64-v8a\libVulkanApp.so" -Force

# Re-zip with all entries uncompressed (required by Android for API 30+)
Remove-Item $BaseZip -Force
$archive = [System.IO.Compression.ZipFile]::Open($BaseZip, [System.IO.Compression.ZipArchiveMode]::Create)
$allFiles = Get-ChildItem -Recurse -File $ApkDir
foreach ($file in $allFiles) {
    $relativePath = $file.FullName.Substring($ApkDir.Length + 1) -replace '\\', '/'
    $entry = $archive.CreateEntry($relativePath, [System.IO.Compression.CompressionLevel]::NoCompression)
    $fileBytes = [System.IO.File]::ReadAllBytes($file.FullName)
    $entryStream = $entry.Open()
    $entryStream.Write($fileBytes, 0, $fileBytes.Length)
    $entryStream.Close()
}
$archive.Dispose()
Remove-Item -Recurse -Force $ApkDir

# 4. Align and sign
Write-Host "[4/4] Aligning and signing..."

& "$BuildTools\zipalign.exe" -f -p 4 $BaseZip $AlignedApk
if ($LASTEXITCODE -ne 0) { throw "zipalign failed" }

$DebugKeystore = "$env:USERPROFILE\.android\debug.keystore"
if (-not (Test-Path $DebugKeystore)) {
    New-Item -ItemType Directory -Force -Path (Split-Path $DebugKeystore) | Out-Null
    & keytool -genkey -v `
        -keystore $DebugKeystore `
        -alias androiddebugkey `
        -keyalg RSA -keysize 2048 -validity 10000 `
        -dname "CN=Android Debug,O=Android,C=US" `
        -storepass android -keypass android 2>&1 | Out-Null
}

& "$BuildTools\apksigner.bat" sign `
    --ks $DebugKeystore `
    --ks-pass pass:android `
    --key-pass pass:android `
    --ks-key-alias androiddebugkey `
    --out $FinalApk `
    $AlignedApk
if ($LASTEXITCODE -ne 0) { throw "apksigner failed" }

# Clean up intermediates
Remove-Item $BaseZip, $AlignedApk -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "=== APK built ==="
Write-Host "  $FinalApk ($((Get-Item $FinalApk).Length) bytes)"
Write-Host ""
Write-Host "Install: adb install -r `"$FinalApk`""
Write-Host "Launch:  adb shell am start -n com.vulkan.demo/android.app.NativeActivity"
Write-Host "Logs:    adb logcat -s VulkanApp:V"
