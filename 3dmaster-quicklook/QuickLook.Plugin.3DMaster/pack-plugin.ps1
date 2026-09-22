$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$releaseDir = Join-Path $scriptDir "bin\Release"
$outputPlugin = Join-Path (Split-Path -Parent $scriptDir) "QuickLook.Plugin.ThreeDMaster.qlplugin"
$tempZip = Join-Path (Split-Path -Parent $scriptDir) "QuickLook.Plugin.ThreeDMaster.zip"

if (Test-Path $outputPlugin) {
    Remove-Item $outputPlugin -Force
}
if (Test-Path $tempZip) {
    Remove-Item $tempZip -Force
}

# 搜集打包内容：排除 pdb/xml 以及宿主自带的 QuickLook.Common.dll
$files = Get-ChildItem -Path $releaseDir -Exclude *.pdb,*.xml,QuickLook.Common.dll

Write-Host "正在打包 QuickLook.Plugin.ThreeDMaster.qlplugin ..."
Compress-Archive -Path $files.FullName -DestinationPath $tempZip
Move-Item $tempZip $outputPlugin -Force

Write-Host "打包成功: $outputPlugin"
