@ECHO OFF

rem This script creates the necesary symlinks to include external source files in the UXT plugin

set RepoPath=%~dp0

mklink /D "%RepoPath%\MRU_Game\Plugins\MixedRealityUtils\Source\MixedRealityUtils\Private\Native" "%RepoPath%\External\MixedRealityUtils-Native\libs\UX\src"
if %ErrorLevel% neq 0 (
    echo Failed to create symlink
) else (
    echo Symlink created succesfuly
)