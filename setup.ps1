Write-Host "Pulling submodules..."
git submodule update --init

Write-Host "Creating symlinks..."
Set-Location cammystat/include
cmd /c 'mklink /J wxWidgets "../../wxWidgets/include"'
cmd /c 'mklink /J eigen "../../eigen"'
Set-Location ../..

# prepare licenses output directory
$licensesDirectory = "cammystat/misc/licenses"
if(!(test-path -PathType container $licensesDirectory)) {
  New-Item -ItemType Directory -Path $licensesDirectory -Force | Out-Null
}

# prepare opencv
If(!(test-path -PathType container ./opencv)) {
  Write-Host "Downloading & extracting OpenCV binaries for development (this may take a while)..."
  $wc = New-Object net.webclient
  $wc.Downloadfile("https://github.com/opencv/opencv/releases/download/4.10.0/opencv-4.10.0-windows.exe", "opencv.exe")

  .\opencv.exe -o"./opencv" -y | Out-Null
  Remove-Item -Path opencv.exe
}

Write-Host "Copying OpenCV files..."
Copy-Item -Path opencv/opencv/build/include/opencv2 -Destination cammystat/include/opencv2/opencv2 -Recurse -Force
New-Item -ItemType Directory -Path cammystat/lib/opencv2 -Force | Out-Null
Copy-Item -Path opencv/opencv/build/x64/vc16/lib/* -Destination cammystat/lib/opencv2 -Recurse -Force
Copy-Item -Path opencv/opencv/build/bin/opencv_videoio_ffmpeg4100_64.dll -Destination cammystat/lib/opencv2/opencv_videoio_ffmpeg4100_64.dll -Recurse -Force
Copy-Item -Path opencv/opencv/build/x64/vc16/bin/opencv_world4100.dll -Destination cammystat/lib/opencv2/opencv_world4100.dll -Recurse -Force
Copy-Item -Path opencv/opencv/build/x64/vc16/bin/opencv_world4100.pdb -Destination cammystat/lib/opencv2/opencv_world4100.pdb -Recurse -Force

Copy-Item -Path opencv/opencv/LICENSE.txt -Destination cammystat/misc/licenses/OPENCV_LICENSE.txt -Force
Copy-Item -Path opencv/opencv/LICENSE_FFMPEG.txt -Destination cammystat/misc/licenses/OPENCV_LICENSE_FFMPEG.txt -Force
$opencvEtcLicenses = "opencv/opencv/build/etc/licenses"
if (Test-Path -PathType Container $opencvEtcLicenses) {
    Copy-Item -Path "$opencvEtcLicenses/*" -Destination cammystat/misc/licenses -Recurse -Force
}

# copy eigen license files
$directory = "eigen/"
$outputFile = "cammystat/misc/licenses/EIGEN_LICENSE.txt"

$files = Get-ChildItem -Path $directory -Filter "COPYING.*" -File | Where-Object { $_.Name -ne "COPYING.README" }

Set-Content -Path $outputFile -Value ""

foreach ($file in $files) {
    Add-Content -Path $outputFile -Value "`n====== $file ======`n"
    Get-Content -Path $file.FullName | Add-Content -Path $outputFile
    Add-Content -Path $outputFile -Value "`n=============`n"
}

# download wxWidgets license
$wc = New-Object net.webclient
$wc.Downloadfile("https://raw.githubusercontent.com/wxWidgets/wxWidgets/master/docs/licence.txt", "cammystat/misc/licenses/WXWIDGETS_LICENSE.txt")

# build wxWidgets
$VSWPath = "${Env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"

$installationPath = & $VSWPath -prerelease -latest -property installationPath
if ($installationPath -and (test-path "$installationPath\Common7\Tools\vsdevcmd.bat")) {
  & "${env:COMSPEC}" /s /c "`"$installationPath\Common7\Tools\vsdevcmd.bat`" -no_logo && set" | foreach-object {
    $name, $value = $_ -split '=', 2
    set-content env:\"$name" $value
  }
}

Write-Host "Building wxWidgets (this may take a while)..."
Set-Location wxWidgets/build/msw
git submodule update --init
# set CL=/MP
# nmake.exe -f makefile.vc SHARED=0 BUILD=release RUNTIME_LIBS=static TARGET_CPU=X64 # CFG=-mt TARGET_CPU=X64
$vcxprojFiles = Get-ChildItem -Path . -Recurse -Filter *.vcxproj

foreach ($file in $vcxprojFiles) {
    $content = Get-Content -Path $file.FullName
    $modifiedContent = $content -replace "MultiThreadedDebugDLL", "MultiThreadedDebug"
    $modifiedContent = $modifiedContent -replace "MultiThreadedDLL", "MultiThreaded"
    Set-Content -Path $file.FullName -Value $modifiedContent
}

msbuild wx_vc17.sln /p:Configuration=Release /property:MultiProcessorCompilation=true /p:Platform=x64

Set-Location ../..
Write-Host "Copying wxWidgets lib files..."
Copy-Item -Path lib/vc_x64_lib/* -Destination ../cammystat/lib/wxwidgets-MT -Force
Set-Location ..

Write-Host "Activating python venv..."
Set-Location plot
python -m venv venv
.\venv\Scripts\Activate.ps1

Write-Host "Installing build dependencies with pip..."
pip install -r requirements.txt

# compile licenses of plot dependencies
& pip install third-party-license-file-generator setuptools

$pythonPath = (Get-Command python).Source

& python -m third_party_license_file_generator -r requirements.txt -p $pythonPath

$outputFile = "../cammystat/misc/licenses/PLOT_EXE_LICENSES.txt"

Set-Content -Path $outputFile -Value ""

Copy-Item -Path THIRDPARTYLICENSES -Destination $outputFile -Force

foreach ($filename in @(
  "LICENSE",
  "LICENSE_AMSFONTS",
  "LICENSE_BAKOMA",
  "LICENSE_CARLOGO",
  "LICENSE_COLORBREWER",
  "LICENSE_COURIERTEN",
  "LICENSE_JSXTOOLS_RESIZE_OBSERVER",
  "LICENSE_QT4_EDITOR",
  "LICENSE_SOLARIZED",
  "LICENSE_STIX",
  "LICENSE_YORICK"
)) {
    $response = Invoke-WebRequest -Uri "https://raw.githubusercontent.com/matplotlib/matplotlib/refs/heads/main/LICENSE/$filename"

    Add-Content -Path $outputFile -Value "`n====== matplotlib/LICENSE/$filename ======`n"
    $response.Content | Add-Content -Path $outputFile
    Add-Content -Path $outputFile -Value "`n=============`n"
}

Write-Host "Building plot.exe (this may take a while)..."
python -m pip install pyinstaller
pyinstaller --onefile plot.py
Set-Location dist
Copy-Item -Path plot.exe -Destination ../../cammystat/plot.exe -Force
Set-Location ../..

Write-Host "Done"
