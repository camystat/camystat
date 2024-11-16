Write-Host "Pulling submodules..."
git submodule update --init

Write-Host "Creating symlinks..."
cd cammystat/include
cmd /c 'mklink /J wxWidgets "../../wxWidgets/include"'
cmd /c 'mklink /J eigen "../../eigen"'
cd ../..

# prepare opencv
Write-Host "Downloading & extracting OpenCV binaries for development (this may take a while)..."
$wc = New-Object net.webclient
$wc.Downloadfile("https://github.com/opencv/opencv/releases/download/4.10.0/opencv-4.10.0-windows.exe", "opencv.exe")

.\opencv.exe -o"./opencv-tmp" -y | Out-Null
Copy-Item -Path opencv-tmp/opencv/build/include/opencv2 -Destination cammystat/include/opencv2/opencv2 -Recurse -Force
New-Item -ItemType Directory -Path cammystat/lib/opencv2 -Force | Out-Null
Copy-Item -Path opencv-tmp/opencv/build/x64/vc16/lib/* -Destination cammystat/lib/opencv2 -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/build/bin/opencv_videoio_ffmpeg4100_64.dll -Destination cammystat/lib/opencv2/opencv_videoio_ffmpeg4100_64.dll -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/build/x64/vc16/bin/opencv_world4100.dll -Destination cammystat/lib/opencv2/opencv_world4100.dll -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/build/x64/vc16/bin/opencv_world4100.pdb -Destination cammystat/lib/opencv2/opencv_world4100.pdb -Recurse -Force

Copy-Item -Path opencv-tmp/opencv/LICENSE* -Destination cammystat/lib/opencv2 -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/LICENSE.txt -Destination cammystat/lib/opencv2/OPENCV_LICENSE.txt -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/LICENSE_FFMPEG.txt -Destination cammystat/lib/opencv2/OPENCV_LICENSE_FFMPEG.txt -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/LICENSE* -Destination cammystat/include/opencv2/opencv2 -Recurse -Force

Remove-Item -Path opencv-tmp -Recurse -Force
Remove-Item -Path opencv.exe

# copy eigen license files
$directory = "eigen/"
$outputFile = "cammystat/misc/EIGEN_LICENSE.txt"

$files = Get-ChildItem -Path $directory -Filter "COPYING.*" -File | Where-Object { $_.Name -ne "COPYING.README" }

Set-Content -Path $outputFile -Value ""

foreach ($file in $files) {
    Add-Content -Path $outputFile -Value "`n====== $file ======`n"
    Get-Content -Path $file.FullName | Add-Content -Path $outputFile
    Add-Content -Path $outputFile -Value "`n=============`n"
}

# download wxWidgets license
$wc = New-Object net.webclient
$wc.Downloadfile("https://raw.githubusercontent.com/wxWidgets/wxWidgets/master/docs/licence.txt", "cammystat/misc/WXWIDGETS_LICENSE.txt")

# compile licenses of plot dependencies
cd plot

& pip install third-party-license-file-generator

$pythonPath = (Get-Command python).Source

& python -m third_party_license_file_generator -r requirements.txt -p $pythonPath

$outputFile = "../cammystat/misc/PLOT_EXE_LICENSES.txt"

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

    Add-Content -Path $outputFile -Value "`n====== $filename ======`n"
    $response.Content | Add-Content -Path $outputFile
    Add-Content -Path $outputFile -Value "`n=============`n"
}

cd ..

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
cd wxWidgets/build/msw
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

cd ../..
Write-Host "Copying wxWidgets lib files..."
Copy-Item -Path lib/vc_x64_lib/* -Destination ../cammystat/lib/wxwidgets-MT -Force
cd ..

Write-Host "Installing pyinstaller & build dependencies with pip..."
pip install -U pyinstaller plotly numpy matplotlib

Write-Host "Building plot.exe (this may take a while)..."
cd plot
pyinstaller --onefile plot.py
cd dist
Copy-Item -Path plot.exe -Destination ../../cammystat/plot.exe -Force
cd ../..

Write-Host "Done"
