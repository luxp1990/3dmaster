$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projectRoot = (Resolve-Path (Join-Path $scriptDir "..\..")).Path
$csharpReleaseDir = Join-Path $scriptDir "bin\Release"
$previewExe = Join-Path $projectRoot "3dmaster-quicklook\3dmaster-preview\build\Release\3dmaster-preview.exe"
$qtBin = Join-Path $projectRoot "qt6\6.8.2\msvc2022_64\bin"
$qtPlatforms = Join-Path $projectRoot "qt6\6.8.2\msvc2022_64\plugins\platforms"
$occtBin = Join-Path $projectRoot "occt\win64\vc14\bin"

$distDir = Join-Path (Split-Path -Parent $scriptDir) "dist\QuickLook.Plugin.ThreeDMaster"
$outputQlPlugin = Join-Path (Split-Path -Parent $scriptDir) "QuickLook.Plugin.ThreeDMaster.qlplugin"
$outputZip = Join-Path (Split-Path -Parent $scriptDir) "QuickLook.Plugin.ThreeDMaster_Portable.zip"

Write-Host "=========================================================="
Write-Host "  Building 3dmaster QuickLook Portable Package"
Write-Host "=========================================================="

if (-not (Test-Path $previewExe)) {
    throw "Missing 3dmaster-preview.exe at $previewExe"
}
if (-not (Test-Path (Join-Path $csharpReleaseDir "QuickLook.Plugin.ThreeDMaster.dll"))) {
    Write-Host "Building QuickLook C# plugin project..."
    Push-Location $scriptDir
    dotnet build QuickLook.Plugin.3DMaster.csproj -c Release
    Pop-Location
}

if (Test-Path $distDir) {
    Remove-Item $distDir -Recurse -Force
}
New-Item -ItemType Directory -Path "$distDir\platforms" -Force | Out-Null

Write-Host "[1/5] Copying QuickLook C# plugin files..."
Get-ChildItem -Path $csharpReleaseDir -Exclude *.pdb,*.xml,QuickLook.Common.dll | ForEach-Object {
    Copy-Item $_.FullName "$distDir\" -Force
}

Write-Host "[2/5] Copying 3dmaster-preview.exe..."
Copy-Item $previewExe "$distDir\" -Force

Write-Host "[3/5] Copying Qt 6 runtime..."
$qtDlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6OpenGL.dll", "Qt6OpenGLWidgets.dll", "Qt6Network.dll")
foreach ($q in $qtDlls) {
    $src = Join-Path $qtBin $q
    if (-not (Test-Path $src)) { throw "Missing Qt DLL: $src" }
    Copy-Item $src "$distDir\" -Force
}
Copy-Item (Join-Path $qtPlatforms "qwindows.dll") "$distDir\platforms\" -Force

Write-Host "[4/5] Copying OpenCASCADE runtime closure..."
$occtDlls = @(
    "avcodec-57.dll", "avformat-57.dll", "avutil-55.dll", "FreeImage.dll", "freetype.dll",
    "jemalloc.dll", "openvr_api.dll", "swscale-4.dll", "tbb12.dll", "TKBO.dll",
    "TKBool.dll", "TKBRep.dll", "TKCAF.dll", "TKCDF.dll", "TKDE.dll", "TKDEGLTF.dll",
    "TKDEIGES.dll", "TKDESTEP.dll", "TKernel.dll", "TKG2d.dll", "TKG3d.dll",
    "TKGeomAlgo.dll", "TKGeomBase.dll", "TKHLR.dll", "TKLCAF.dll", "TKMath.dll",
    "TKMesh.dll", "TKPrim.dll", "TKRWMesh.dll", "TKService.dll", "TKShHealing.dll",
    "TKTopAlgo.dll", "TKV3d.dll", "TKVCAF.dll", "TKXCAF.dll", "TKXSBase.dll"
)
foreach ($o in $occtDlls) {
    $src = Join-Path $occtBin $o
    if (-not (Test-Path $src)) { throw "Missing OCCT DLL: $src" }
    Copy-Item $src "$distDir\" -Force
}

Write-Host "[5/5] Generating archives..."
if (Test-Path $outputQlPlugin) { Remove-Item $outputQlPlugin -Force }
$tempZip1 = [System.IO.Path]::ChangeExtension($outputQlPlugin, ".zip")
if (Test-Path $tempZip1) { Remove-Item $tempZip1 -Force }
Compress-Archive -Path "$distDir\*" -DestinationPath $tempZip1 -CompressionLevel Optimal
Move-Item $tempZip1 $outputQlPlugin -Force

if (Test-Path $outputZip) { Remove-Item $outputZip -Force }
Compress-Archive -Path $distDir -DestinationPath $outputZip -CompressionLevel Optimal

$qlSizeMB = [math]::Round((Get-Item $outputQlPlugin).Length / 1MB, 2)
$zipSizeMB = [math]::Round((Get-Item $outputZip).Length / 1MB, 2)

Write-Host "=========================================================="
Write-Host "Packaging Complete!"
Write-Host "1. QuickLook One-Click Plugin: $outputQlPlugin ($qlSizeMB MB)"
Write-Host "2. Portable Dist Directory:    $distDir"
Write-Host "3. Portable Zip Archive:       $outputZip ($zipSizeMB MB)"
Write-Host "=========================================================="
