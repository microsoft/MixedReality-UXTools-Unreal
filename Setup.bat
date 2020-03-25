@ECHO OFF

rem This script sets up the repo

pushd %~dp0%
git config --local core.hooksPath Tools/hooks
Tools\scripts\SetupSymlinks.bat
popd
