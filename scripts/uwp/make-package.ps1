# TheSuperHackers @build githubawn 16/10/2026 Build a sideloadable UWP package (.msix)
# of the WindowsStore (Xbox) build: layout = exe + DLLs + app-local VC runtime + game
# data, then makeappx + signtool with a self-signed test certificate.
#
#   powershell -File scripts\uwp\make-package.ps1 -BuildDir build\uwp-x64 `
#     -ZeroHourDir "...\Command & Conquer Generals - Zero Hour" `
#     -GeneralsDir "...\Command and Conquer Generals" -OutDir build\uwp-package
#
# Run from a Visual Studio developer shell (makeappx/signtool on PATH).

param(
    [Parameter(Mandatory = $true)][string]$BuildDir,
    [Parameter(Mandatory = $true)][string]$ZeroHourDir,
    [Parameter(Mandatory = $true)][string]$GeneralsDir,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [string]$FfmpegBinDir = "",
    [string]$Version = "1.0.0.0",
    [string]$Publisher = "CN=GGCXboxDev",
    [switch]$NoData
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = (Resolve-Path $BuildDir).Path
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
$layout = Join-Path $OutDir "layout"
if (Test-Path $layout) { Remove-Item -Recurse -Force $layout }
New-Item -ItemType Directory -Force "$layout\Assets" | Out-Null

# --- binaries -----------------------------------------------------------------
Copy-Item "$BuildDir\GeneralsMD\generalszh.exe" $layout
foreach ($dll in @("_deps\sdl3-build\SDL3.dll", "_deps\openal_soft-build\OpenAL32.dll", "_deps\bink-build\binkw32.dll")) {
    if (Test-Path "$BuildDir\$dll") { Copy-Item "$BuildDir\$dll" $layout }
}
if ($FfmpegBinDir -ne "") {
    Get-ChildItem $FfmpegBinDir -Filter *.dll | Copy-Item -Destination $layout
}

# App-local VC++ runtime for UWP. A retail-console Dev Mode install has no
# Microsoft.VCLibs framework package, so ship the *_app.dll files next to the exe.
$vclibs = Get-AppxPackage -Name Microsoft.VCLibs.140.00 | Where-Object Architecture -eq 'X64' | Sort-Object Version -Descending | Select-Object -First 1
if ($null -eq $vclibs) { throw "Microsoft.VCLibs.140.00 (x64) is not installed on this PC; cannot bundle the UWP C++ runtime." }
Get-ChildItem $vclibs.InstallLocation -Filter *_app.dll | Copy-Item -Destination $layout

# --- placeholder tile / splash images -------------------------------------------
Add-Type -AssemblyName System.Drawing
function New-Tile([string]$path, [int]$w, [int]$h) {
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::FromArgb(255, 20, 24, 18))
    $size = [Math]::Max(6, [int]($h / 4))
    $font = New-Object System.Drawing.Font "Arial", $size, ([System.Drawing.FontStyle]::Bold), ([System.Drawing.GraphicsUnit]::Pixel)
    $fmt = New-Object System.Drawing.StringFormat
    $fmt.Alignment = [System.Drawing.StringAlignment]::Center
    $fmt.LineAlignment = [System.Drawing.StringAlignment]::Center
    $g.DrawString("ZH", $font, [System.Drawing.Brushes]::Gold, (New-Object System.Drawing.RectangleF 0, 0, $w, $h), $fmt)
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
}
New-Tile "$layout\Assets\Square150x150Logo.png" 150 150
New-Tile "$layout\Assets\Square44x44Logo.png" 44 44
New-Tile "$layout\Assets\StoreLogo.png" 50 50
New-Tile "$layout\Assets\SplashScreen.png" 620 300

# --- manifest -----------------------------------------------------------------
(Get-Content "$here\AppxManifest.xml.in" -Raw).Replace("@PUBLISHER@", $Publisher).Replace("@VERSION@", $Version) |
    Set-Content -Encoding UTF8 "$layout\AppxManifest.xml"

# --- game data ----------------------------------------------------------------
# Zero Hour archives go in the package root (the working directory); the base
# Generals archives go in "generals\" (see StdBIGFileSystem, GGC_PLATFORM_UWP).
if (-not $NoData) {
    Get-ChildItem $ZeroHourDir -Filter *.big | Copy-Item -Destination $layout
    New-Item -ItemType Directory -Force "$layout\generals" | Out-Null
    Get-ChildItem $GeneralsDir -Filter *.big | Copy-Item -Destination "$layout\generals"
    # Loose Data files (cursors, scripts, water planes). Movies are skipped: this
    # build has no video playback.
    foreach ($sub in @("Cursors", "Scripts", "WaterPlane", "INI")) {
        if (Test-Path "$ZeroHourDir\Data\$sub") {
            New-Item -ItemType Directory -Force "$layout\Data\$sub" | Out-Null
            Copy-Item -Recurse "$ZeroHourDir\Data\$sub\*" "$layout\Data\$sub"
        }
    }
}

# --- signing certificate --------------------------------------------------------
$pfx = Join-Path $OutDir "GGCXboxDev.pfx"
$pfxPassword = "ggc"
if (-not (Test-Path $pfx)) {
    $cert = New-SelfSignedCertificate -Type Custom -Subject $Publisher -KeyUsage DigitalSignature `
        -FriendlyName "GGC Xbox dev package signing" -CertStoreLocation "Cert:\CurrentUser\My" `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
    Export-PfxCertificate -Cert $cert -FilePath $pfx -Password (ConvertTo-SecureString $pfxPassword -AsPlainText -Force) | Out-Null
    Export-Certificate -Cert $cert -FilePath (Join-Path $OutDir "GGCXboxDev.cer") | Out-Null
}

# --- pack + sign ----------------------------------------------------------------
$msix = Join-Path $OutDir "GeneralsZH_${Version}_x64.msix"
if (Test-Path $msix) { Remove-Item -Force $msix }
& makeappx.exe pack /d $layout /p $msix /nv /o
if ($LASTEXITCODE -ne 0) { throw "makeappx failed ($LASTEXITCODE)" }
& signtool.exe sign /fd SHA256 /f $pfx /p $pfxPassword $msix
if ($LASTEXITCODE -ne 0) { throw "signtool failed ($LASTEXITCODE)" }
Write-Host "Package: $msix"

