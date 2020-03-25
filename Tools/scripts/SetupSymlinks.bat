@ECHO OFF

rem This script creates the necesary symlinks to include external source files in the UXT plugin

set RepoPath=%~dp0\..\..
set SymlinkPath="%RepoPath%\UXToolsGame\Plugins\UXTools\Source\UXTools\Private\Native"

if exist "%SymlinkPath%" (
    rmdir /s /q %SymlinkPath%
)

mklink /D %SymlinkPath% "%RepoPath%\External\UXT-Native\libs\UX\src" > NUL
if %ErrorLevel% neq 0 (
    echo Failed to create symlink
) else (
    if "%1" neq "-quiet" (
        echo Symlink created succesfuly
    )
)
