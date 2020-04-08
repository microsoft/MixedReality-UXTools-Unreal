@ECHO OFF
rem Copyright (c) Microsoft Corporation.
rem Licensed under the MIT License.

rem Machine-dependent paths
set UePath=F:\2020-01-13_InstalledEngine_87e56d188
set SevenZPath=C:\Program Files\7-Zip

set RepoPath=%~dp0\..
set OutPath=%CD%
set TmpPath=%OutPath%\TMP
set PluginPathInRepo=UXToolsGame\Plugins\UXTools
set PluginPath=%RepoPath%\%PluginPathInRepo%
set PackagePath=%TmpPath%\UXTools
set ExitCode=0

if exist "%TmpPath%" (
    echo %TmpPath% already exists. Delete before running the script.
    exit /b 1
)

mkdir "%TmpPath%"

echo -------------------
echo Packaging plugin...
echo.

call "%UePath%\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="%PluginPath%\UXTools.uplugin" -Package="%PackagePath%"
if %ErrorLevel% neq 0  (
    echo ERROR executing command: "%UePath%\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="%PluginPath%\UXTools.uplugin" -Package="%PackagePath%"
    set ExitCode=1
    goto :Exit
)

echo.
echo Plugin packaged into "%PackagePath%"
echo ------------------------------------
echo.

pushd "%RepoPath%""

echo ------------------------------
echo Obtaining current commit ID...
echo.

git log -1 --pretty=format:"%%h" > "%TmpPath%\commit_id.tmp"
set /p CommitId= < "%TmpPath%\commit_id.tmp"

echo.
echo Commit ID is %CommitId%
echo -----------------------
echo.

popd

echo ---------------------
echo Compressing plugin...
echo.

"%SevenZPath%\7z.exe" a "%OutPath%\UXTools_%CommitId%.7z" "%PackagePath%"
if %ErrorLevel% neq 0 (
    echo ERROR executing command: "%SevenZPath%\7z.exe" a "%OutPath%\UXTools_%CommitId%.7z" "%PackagePath%"
    set ExitCode=1
    goto :Exit
)
echo.
echo Plugin compressed into "%OutPath%\UXTools_%CommitId%.7z"
echo ------------------------------------------------------------------
echo.

pushd "%TmpPath%"

echo ---------------------
echo Cloning repository...
echo.

git clone "%RepoPath%" UXT
if %ErrorLevel% neq 0 (
    echo ERROR executing command: git clone "%RepoPath%" UXT
    popd
    set ExitCode=1
    goto :Exit
)

echo.
echo Repository cloned into "%TmpPath%\UXT"
echo --------------------------------------
echo.

cd UXT
rmdir /s /q "%PluginPathInRepo%"

echo -------------------------------
echo Compressing examples project...
echo.

"%SevenZPath%\7z.exe" a "%OutPath%\Examples_%CommitId%.7z" UXToolsGame
if %ErrorLevel% neq 0 (
    echo ERROR executing command: "%SevenZPath%\7z.exe" a "%OutPath%\Examples_%CommitId%.7z" UXToolsGame
    popd
    set ExitCode=1
    goto :Exit
)

echo.
echo Examples project compressed into "%OutPath%\Examples_%CommitId%.7z"
echo -------------------------------------------------------------------
echo.

popd

:Exit

rmdir /s /q "%TmpPath%"
exit /b %ErrorCode%