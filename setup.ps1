Write-Host "Pulling submodules..."
git submodule update --init

Write-Host "Creating symlinks..."
cd DUMMY_GUI/include
cmd /c 'mklink /J python "../../python/Include"'
cmd /c 'mklink /J wxWidgets "../../wxWidgets/include"'
cd ../..

Write-Host "Copying files..."
Copy-Item -Path DUMMY_GUI/include/pyconfig.h -Destination DUMMY_GUI/include/python/pyconfig.h

Write-Host "Applying patches..."
Copy-Item -Path patches/matplotlibcpp.h.patch -Destination DUMMY_GUI/include/matplotlib-cpp/matplotlibcpp.h.patch
cd DUMMY_GUI/include/matplotlib-cpp
git apply matplotlibcpp.h.patch
Remove-Item matplotlibcpp.h.patch
cd ../../..

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
    $modifiedContent = $content -replace "MultiThreadedDebugDLL", "MultiThreaded"
    $modifiedContent = $content -replace "MultiThreadedDLL", "MultiThreaded"
    Set-Content -Path $file.FullName -Value $modifiedContent
}

msbuild wx_vc17.sln /p:Configuration=Release /property:MultiProcessorCompilation=true /p:Platform=x64

cd ../..
Write-Host "Copying wxWidgets lib files..."
Copy-Item -Path lib/vc_x64_lib/*.lib -Destination ../DUMMY_GUI/lib/wxwidgets-MT
cd ..

Write-Host "Installing pyinstaller with pip..."
pip install -U pyinstaller

Write-Host "Building plot.exe (this may take a while)..."
cd plot
pyinstaller --onefile plot.py
cd dist
Copy-Item -Path plot.exe -Destination ../../DUMMY_GUI/plot.exe
cd ../..

Write-Host "Done"
