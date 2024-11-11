Write-Host "Pulling submodules..."
# git submodule update --init

Write-Host "Creating symlinks..."
cd cammystat/include
cmd /c 'mklink /J wxWidgets "../../wxWidgets/include"'
cmd /c 'mklink /J matplotlib-cpp "../../matplotlib-cpp"'
cmd /c 'mklink /J eigen "../../eigen"'
cd ../..

Write-Host "Downloading & extracting OpenCV binaries for development (this may take a while)..."
$wc = New-Object net.webclient
$wc.Downloadfile("https://github.com/opencv/opencv/releases/download/4.9.0/opencv-4.9.0-windows.exe", "opencv.exe")

.\opencv.exe -o"./opencv-tmp" -y | Out-Null
Copy-Item -Path opencv-tmp/opencv/build/include/opencv2 -Destination cammystat/include/opencv2/opencv2 -Recurse -Force
New-Item -ItemType Directory -Path cammystat/lib/opencv2 -Force | Out-Null
Copy-Item -Path opencv-tmp/opencv/build/x64/vc16/lib/* -Destination cammystat/lib/opencv2 -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/build/x64/vc16/bin/opencv_world490.dll -Destination cammystat/lib/opencv2/opencv_world490.dll -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/build/x64/vc16/bin/opencv_world490.pdb -Destination cammystat/lib/opencv2/opencv_world490.pdb -Recurse -Force

Copy-Item -Path opencv-tmp/opencv/LICENSE* -Destination cammystat/lib/opencv2 -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/LICENSE.txt -Destination cammystat/lib/opencv2/OPENCV_LICENSE.txt -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/LICENSE_FFMPEG.txt -Destination cammystat/lib/opencv2/OPENCV_LICENSE_FFMPEG.txt -Recurse -Force
Copy-Item -Path opencv-tmp/opencv/LICENSE* -Destination cammystat/include/opencv2/opencv2 -Recurse -Force

Remove-Item -Path opencv-tmp -Recurse -Force
Remove-Item -Path opencv.exe

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
