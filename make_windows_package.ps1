$build_dir=$args[0]
$root_dir=$args[1]

mkdir sdrpp_windows_x64

# Copy root
cp -Recurse $root_dir/* sdrpp_windows_x64/

# Copy core
cp $build_dir/Release/* sdrpp_windows_x64/
cp 'C:/Program Files/PothosSDR/bin/volk.dll' sdrpp_windows_x64/

# Copy source modules
cp $build_dir/source_modules/airspy_source/Release/airspy_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/PothosSDR/bin/airspy.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/airspyhf_source/Release/airspyhf_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/PothosSDR/bin/airspyhf.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/audio_source/Release/audio_source.dll sdrpp_windows_x64/modules/

cp $build_dir/source_modules/bladerf_source/Release/bladerf_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/PothosSDR/bin/bladeRF.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/file_source/Release/file_source.dll sdrpp_windows_x64/modules/

cp $build_dir/source_modules/fobossdr_source/Release/fobossdr_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/RigExpert/Fobos/bin/fobos.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/hackrf_source/Release/hackrf_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/PothosSDR/bin/hackrf.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/hermes_source/Release/hermes_source.dll sdrpp_windows_x64/modules/

cp $build_dir/source_modules/hydrasdr_source/Release/hydrasdr_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/hydrasdr-host/bin/hydrasdr.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/limesdr_source/Release/limesdr_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/PothosSDR/bin/LimeSuite.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/network_source/Release/network_source.dll sdrpp_windows_x64/modules/

cp $build_dir/source_modules/perseus_source/Release/perseus_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/PothosSDR/bin/perseus-sdr.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/plutosdr_source/Release/plutosdr_source.dll sdrpp_windows_x64/modules/

# PlutoSDR / PlutoPlus runtime dependencies
$PothosBin = 'C:/Program Files/PothosSDR/bin'

$PlutoRuntimeDlls = @(
    'libiio.dll',
    'libad9361.dll',
    'libxml2.dll',
    'libusb-1.0.dll',
    'libserialport-0.dll'
)

foreach ($dll in $PlutoRuntimeDlls) {
    $src = Join-Path $PothosBin $dll
    if (Test-Path $src) {
        cp $src sdrpp_windows_x64/ -Force
    } else {
        Write-Warning "Missing Pluto runtime dependency: $src"
    }
}

cp $build_dir/source_modules/rfnm_source/Release/rfnm_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/RFNM/bin/rfnm.dll' sdrpp_windows_x64/
cp 'C:/Program Files/RFNM/bin/spdlog.dll' sdrpp_windows_x64/
cp 'C:/Program Files/RFNM/bin/fmt.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/rfspace_source/Release/rfspace_source.dll sdrpp_windows_x64/modules/

cp $build_dir/source_modules/rtl_sdr_source/Release/rtl_sdr_source.dll sdrpp_windows_x64/modules/
cp 'C:/Program Files/PothosSDR/bin/rtlsdr.dll' sdrpp_windows_x64/

cp $build_dir/source_modules/rtl_tcp_source/Release/rtl_tcp_source.dll sdrpp_windows_x64/modules/

cp $build_dir/source_modules/sdrplay_source/Release/sdrplay_source.dll sdrpp_windows_x64/modules/ -ErrorAction SilentlyContinue
cp 'C:/Program Files/SDRplay/API/x64/sdrplay_api.dll' sdrpp_windows_x64/ -ErrorAction SilentlyContinue

cp $build_dir/source_modules/sdrpp_server_source/Release/sdrpp_server_source.dll sdrpp_windows_x64/modules/

cp $build_dir/source_modules/spyserver_source/Release/spyserver_source.dll sdrpp_windows_x64/modules/

# cp $build_dir/source_modules/usrp_source/Release/usrp_source.dll sdrpp_windows_x64/modules/

# Copy sink modules
cp $build_dir/sink_modules/audio_sink/Release/audio_sink.dll sdrpp_windows_x64/modules/
cp "C:/Program Files (x86)/RtAudio/bin/rtaudio.dll" sdrpp_windows_x64/

cp $build_dir/sink_modules/new_portaudio_sink/Release/new_portaudio_sink.dll sdrpp_windows_x64/modules/

cp $build_dir/sink_modules/network_sink/Release/network_sink.dll sdrpp_windows_x64/modules/


# Copy decoder modules
cp $build_dir/decoder_modules/atv_decoder/Release/atv_decoder.dll sdrpp_windows_x64/modules/

cp $build_dir/decoder_modules/m17_decoder/Release/m17_decoder.dll sdrpp_windows_x64/modules/
cp "C:/Program Files/codec2/lib/libcodec2.dll" sdrpp_windows_x64/

cp $build_dir/decoder_modules/meteor_demodulator/Release/meteor_demodulator.dll sdrpp_windows_x64/modules/

cp $build_dir/decoder_modules/radio/Release/radio.dll sdrpp_windows_x64/modules/


# Copy misc modules
cp $build_dir/misc_modules/discord_integration/Release/discord_integration.dll sdrpp_windows_x64/modules/

cp $build_dir/misc_modules/frequency_manager/Release/frequency_manager.dll sdrpp_windows_x64/modules/

cp $build_dir/misc_modules/iq_exporter/Release/iq_exporter.dll sdrpp_windows_x64/modules/

cp $build_dir/misc_modules/recorder/Release/recorder.dll sdrpp_windows_x64/modules/

cp $build_dir/misc_modules/rigctl_client/Release/rigctl_client.dll sdrpp_windows_x64/modules/

cp $build_dir/misc_modules/rigctl_server/Release/rigctl_server.dll sdrpp_windows_x64/modules/

cp $build_dir/misc_modules/scanner/Release/scanner.dll sdrpp_windows_x64/modules/


# Copy supporting libs
cp 'C:/Program Files/PothosSDR/bin/libusb-1.0.dll' sdrpp_windows_x64/
cp 'C:/Program Files/PothosSDR/bin/pthreadVC2.dll' sdrpp_windows_x64/



# ---- SoapySDR source packaging ----

$VcpkgRoot  = "C:/vcpkg/installed/x64-windows"
$PothosRoot = "C:/Program Files/PothosSDR"

$PackageRoot = "sdrpp_windows_x64"
$ModuleDir   = "$PackageRoot/modules"
$SoapyDir    = "$PackageRoot/SoapySDR/modules0.8"

function Copy-RequiredFile {
    param(
        [Parameter(Mandatory=$true)] [string] $Source,
        [Parameter(Mandatory=$true)] [string] $Destination
    )

    if (!(Test-Path $Source)) {
        Write-Error "Required file not found: $Source"
        exit 1
    }

    Copy-Item $Source $Destination -Force
}

function Copy-OptionalFile {
    param(
        [Parameter(Mandatory=$true)] [string] $Source,
        [Parameter(Mandatory=$true)] [string] $Destination
    )

    if (Test-Path $Source) {
        Copy-Item $Source $Destination -Force
    } else {
        Write-Host "Optional file not found: $Source"
    }
}

New-Item -ItemType Directory -Force -Path $ModuleDir | Out-Null
New-Item -ItemType Directory -Force -Path $SoapyDir | Out-Null

# SDR++ Soapy source module
Copy-RequiredFile `
    "$build_dir/source_modules/soapy_source/Release/soapy_source.dll" `
    "$ModuleDir/"

# SoapySDR runtime.
# IMPORTANT: use vcpkg SoapySDR.dll because SDR++ was linked through vcpkg.
Copy-RequiredFile `
    "$VcpkgRoot/bin/SoapySDR.dll" `
    "$PackageRoot/"

# Optional Soapy debug utility
Copy-OptionalFile `
    "$VcpkgRoot/tools/soapysdr/SoapySDRUtil.exe" `
    "$PackageRoot/"

# Soapy plugins from PothosSDR
if (Test-Path "$PothosRoot/lib/SoapySDR/modules0.8") {
    Copy-Item "$PothosRoot/lib/SoapySDR/modules0.8/*.dll" `
        "$SoapyDir/" `
        -Force `
        -ErrorAction SilentlyContinue
} else {
    Write-Warning "PothosSDR Soapy plugin directory not found"
}

# Runtime dependencies needed by Pothos Soapy plugins.
# Do NOT copy $PothosRoot/bin/*.dll blindly, because it may overwrite vcpkg DLLs.
$pothosDeps = @(
    "libusb-1.0.dll",
    "pthreadVC2.dll",
    "rtlsdr.dll",
    "airspy.dll",
    "airspyhf.dll",
    "hackrf.dll",
    "bladeRF.dll",
    "LimeSuite.dll",
    "perseus-sdr.dll",
    "libiio.dll",
    "libad9361.dll"
)

foreach ($dll in $pothosDeps) {
    Copy-OptionalFile "$PothosRoot/bin/$dll" "$PackageRoot/"
}

# vcpkg runtime deps that SDR++
$vcpkgDeps = @(
    "zstd.dll",
    "fftw3.dll",
    "glfw3.dll",
    "portaudio.dll",
    "spdlog.dll",
    "fmt.dll"
)

foreach ($dll in $vcpkgDeps) {
    Copy-OptionalFile "$VcpkgRoot/bin/$dll" "$PackageRoot/"
}

# MSVC runtime DLLs.
# 0xc0000142 was caused by missing MSVCP140.dll
$systemDlls = @(
    "msvcp140.dll",
    "vcruntime140.dll",
    "vcruntime140_1.dll",
    "concrt140.dll"
)

foreach ($dll in $systemDlls) {
    $src = "$env:WINDIR/System32/$dll"
    Copy-OptionalFile $src "$PackageRoot/"
}

# Sanity check: make sure Pothos did not overwrite vcpkg SoapySDR.dll
Write-Host "Packaged SoapySDR.dll:"
Get-Item "$PackageRoot/SoapySDR.dll" | Select-Object FullName, Length, LastWriteTime

Write-Host "Soapy plugin DLLs:"
Get-ChildItem "$SoapyDir" -Filter "*.dll" | Select-Object Name, Length

# Launcher
@'
@echo off
setlocal
set PATH=%~dp0;%PATH%
set SOAPY_SDR_PLUGIN_PATH=%~dp0SoapySDR\modules0.8
start "" "%~dp0sdrpp.exe"
'@ | Out-File -Encoding ASCII "$PackageRoot/start_sdrpp.bat"

# Debug launcher
@'
@echo off
setlocal
set PATH=%~dp0;%PATH%
set SOAPY_SDR_PLUGIN_PATH=%~dp0SoapySDR\modules0.8
set SOAPY_SDR_LOG_LEVEL=TRACE

echo === PATH ===
echo %PATH%
echo.

echo === SOAPY_SDR_PLUGIN_PATH ===
echo %SOAPY_SDR_PLUGIN_PATH%
echo.

echo === SoapySDR info ===
"%~dp0SoapySDRUtil.exe" --info
echo.

echo === SoapySDR find ===
"%~dp0SoapySDRUtil.exe" --find
echo.

echo === SDR++ ===
"%~dp0sdrpp.exe" -c

pause
'@ | Out-File -Encoding ASCII "$PackageRoot/start_sdrpp_debug.bat"


Compress-Archive -Path sdrpp_windows_x64/* -DestinationPath sdrpp_windows_x64.zip -Force

rm -Force -Recurse sdrpp_windows_x64
