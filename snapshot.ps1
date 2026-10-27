# snapshot.ps1 - CHOPPER 工程快照存档脚本
# 用法: powershell -ExecutionPolicy Bypass -File snapshot.ps1 <版本号> [备注]
# 例:   powershell -ExecutionPolicy Bypass -File snapshot.ps1 016 "砍树枝手感初版"
# 产物: 与工程平级的 ChopperVersions\ChopperVer<版本号>_<备注>.zip
# 说明: 只打包 源码+工程配置+资源(fonts/sound/graphics)，
#       排除 Debug / x64 / .vs / SFML2 / DownloadData 等临时或依赖目录

param(
    [Parameter(Mandatory = $true)][string]$Version,
    [string]$Note = ""
)

$ErrorActionPreference = "Stop"

# 工程目录 = 本脚本所在目录；存档目录 = 与工程平级的 ChopperVersions
$projDir    = Split-Path -Parent $MyInvocation.MyCommand.Path
$parent     = Split-Path -Parent $projDir
$archiveDir = Join-Path $parent "ChopperVersions"

if (-not (Test-Path -LiteralPath $archiveDir)) {
    New-Item -ItemType Directory -Path $archiveDir | Out-Null
}

# 1) 源码文件（工程根下的 .cpp / .h）
$sourceFiles = Get-ChildItem -LiteralPath $projDir -File |
    Where-Object { $_.Extension -in '.cpp', '.h' } |
    Select-Object -ExpandProperty FullName

# 2) 工程配置文件（存在才加；config.h 已含在源码 .h 收集里，不重复加）
$configNames = @('settings.txt', 'ChopperVer015New.vcxproj', 'ChopperVer015New.vcxproj.filters')
$configFiles = foreach ($n in $configNames) {
    $fp = Join-Path $projDir $n
    if (Test-Path -LiteralPath $fp) { $fp }
}

# 3) 资源目录（字体 / 音效 / 图片，整体打包）
$assetDirs = foreach ($d in @('fonts', 'sound', 'graphics')) {
    $dp = Join-Path $projDir $d
    if (Test-Path -LiteralPath $dp) { $dp }
}

$items = @($sourceFiles) + @($configFiles) + @($assetDirs)

# 版本名（备注里的非法字符替换成下划线）
$safeNote = $Note -replace '[\\/:*?"<>|]', '_'
$verName  = "ChopperVer${Version}" + $(if ($safeNote) { "_" + $safeNote } else { "" })
$zipPath  = Join-Path $archiveDir ($verName + ".zip")

# 目标已存在则先删（避免 Compress-Archive 报"文件已存在"）
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }

Compress-Archive -Path $items -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host ""
Write-Host "已存档: $zipPath"
Write-Host ("打包条目数(文件/目录): {0}" -f $items.Count)
Write-Host ""
