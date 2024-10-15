Write-Host "Pulling submodules..."
git submodule update --init
Write-Host "Copying files..."
Copy-Item -Path DUMMY_GUI/include/pyconfig.h -Destination DUMMY_GUI/include/python/pyconfig.h
Write-Host "Done"