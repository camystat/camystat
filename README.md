# cammystat

Research project.

## Setup

### Prerequisites

- MSVC along with Visual Studio
- GIT
- OpenCV 4.9.0 installed and on PATH
- Python 3.12 installed and on PATH (`python312.dll`)

To build the project, first clone it along with submodules:

`git clone git@github.com:cammystat/cammystat.git --recurse-submodules`

And run the setup script:

`powershell.exe -noprofile -executionpolicy bypass -file setup.ps1`

Finally, open `DUMMY_GUI.sln` with Visual Studio, select a configuration (Debug with console window or Release without it) and build / run the program.
You will then be able to find the outputs in `x64/{Debug,Release}`.
