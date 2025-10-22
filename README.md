# camystat

Research project.

## Building from source

### Prerequisites

- MSVC along with Visual Studio
- GIT
- Python >= 3.10

To build the project, first clone it along with submodules:

`git clone git@github.com:cammystat/cammystat.git --recurse-submodules`

And run the setup script:

`powershell.exe -noprofile -executionpolicy bypass -file setup.ps1`

Finally, open `cammystat.sln` with Visual Studio, select a configuration (Debug with console window or Release without it) and build / run the program.
You will then be able to find the outputs in `x64/{Debug,Release}`.
