# camystat

Software for binary differential analysis as a high-throughput method for analyzing recordings of contractile cardiomyocytes derived from human-induced pluripotent stem cells.

## Running

Please download the latest release from https://github.com/camystat/camystat/releases.

> [!IMPORTANT]
> After downloading, please always run the executable from the directory (CWD) where it is placed.

> [!IMPORTANT]
> For running on Linux, wxWidgets (GUI library used by camystat) library is required to be installed on the machine. The package name depends on your distribution and version, but generally you can search in your package manager for the `libwx*` package. For instance, for Ubuntu with apt and GTK, you can try searching: `sudo apt-cache search libwxgt*`. An example of a valid installation command may be: `sudo apt install libwxgtk3.0-gtk3-0v5`.

## Building from source

Clone the repository (with submodules):

```bash
git clone --recurse-submodules https://github.com/cammystat/cammystat.git
cd cammystat
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

Then follow the instructions for your platform below.

---

### Linux

**Prerequisites**

- CMake (3.20+)
- C++17 toolchain (`build-essential` on Debian/Ubuntu)
- pkg-config, curl
- Python 3.10 or newer (python3-venv, python3-pip)
- [OpenCV](https://opencv.org/) (e.g. `libopencv-dev`)
- [wxWidgets](https://www.wxwidgets.org/) 3.2 (e.g. `libwxgtk3.2-dev`)

On Debian/Ubuntu the setup script can install these for you. Otherwise install manually, then run setup and build:

```bash
./setup.bash
./build.bash
```

The executable and runtime files (licenses, `plot.exe`, etc.) will be in **`build/cammystat/`**. The app must be run from that directory so it finds its assets.

---

### macOS

**Prerequisites**

- CMake (3.20+)
- pkg-config, curl
- Python 3.10 or newer (e.g. `python@3.12` via Homebrew)
- [OpenCV](https://opencv.org/) and [wxWidgets](https://www.wxwidgets.org/) (e.g. via Homebrew)

The setup script can install dependencies via Homebrew. Then run setup and build:

```bash
./setup.bash
./build.bash
```

The executable and runtime files will be in **`build/cammystat/`**. Run the app from that directory.

---

### Windows

**Prerequisites**

- [Visual Studio](https://visualstudio.microsoft.com/) with MSVC (e.g. Visual Studio 2022 with "Desktop development with C++")
- [Git](https://git-scm.com/)
- Python 3.10 or newer (on PATH)

Setup downloads OpenCV binaries, builds wxWidgets, and builds the bundled plot tool (Python/PyInstaller). Run setup from the repository root in PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File setup.ps1
```

Then build the solution:

- **Visual Studio:** Open `cammystat.sln`, choose configuration (e.g. **Release | x64**), then Build / Run.
- **Command line:** From the repo root (with MSBuild on PATH, e.g. from "Developer Command Prompt" or after `microsoft/setup-msbuild` in CI):

  ```powershell
  msbuild cammystat.sln /p:Configuration=Release /p:Platform=x64 /property:MultiProcessorCompilation=true
  ```

Outputs are in **`x64\Release\`** (or `x64\Debug\` for Debug). Run `cammystat.exe` from that folder so it finds `plot.exe`, DLLs, and other assets.
