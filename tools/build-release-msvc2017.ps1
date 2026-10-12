<#
   构建 olp Release|x64（MSVC v141 / Visual Studio 2017 + Qt 5.12.8 msvc2017_64 + vcpkg）

   用法：
     powershell -ExecutionPolicy Bypass -File tools\build-release-msvc2017.ps1

   产物：
     olp\x64\Release\olp.exe

   说明：
     * 工程 olp\olp.vcxproj 依赖 Qt VS Tools 的 QtMsBuild 目录（默认 %LOCALAPPDATA%\QtMsBuild）。
     * 工程内已带 PreBuildEvent，会把 olp\prebuild\rpc.tlh 预置到中间目录，
       避免干净构建时 pqkitcallback.h 因找不到 rpc.tlh 而报 fatal error C1083。
#>
[CmdletBinding()]
param(
    [string]$Configuration = 'Release',
    [string]$Platform      = 'x64',
    [string]$QtMsBuild     = (Join-Path $env:LOCALAPPDATA 'QtMsBuild'),
    [string]$MSBuild       = 'C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\MSBuild.exe'
)

$ErrorActionPreference = 'Stop'

$root     = Split-Path -Parent $PSScriptRoot
$solution = Join-Path $root 'olp\olp.sln'

foreach ($p in @($solution, $MSBuild)) {
    if (-not (Test-Path $p)) { throw "路径不存在：$p" }
}
if (-not (Test-Path (Join-Path $QtMsBuild 'qt.targets'))) {
    throw "QtMsBuild 不完整（缺少 qt.targets）：$QtMsBuild；请安装 Qt VS Tools 或用 -QtMsBuild 指定"
}

Write-Host "== 构建 $Configuration|$Platform ==" -ForegroundColor Cyan
Write-Host "solution  : $solution"
Write-Host "QtMsBuild : $QtMsBuild"

& $MSBuild $solution /p:Configuration=$Configuration /p:Platform=$Platform `
    /p:QtMsBuild=$QtMsBuild /m /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw "编译失败，MSBuild 退出码 $LASTEXITCODE" }

$exe = Join-Path $root "olp\x64\$Configuration\olp.exe"
if (-not (Test-Path $exe)) { throw "未生成 $exe" }

Write-Host "`n== 构建成功 ==" -ForegroundColor Green
Get-Item $exe | Select-Object FullName, Length, LastWriteTime | Format-List
