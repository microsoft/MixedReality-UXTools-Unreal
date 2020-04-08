@ECHO OFF
rem Copyright (c) Microsoft Corporation.
rem Licensed under the MIT License.

rem This script sets up the repo

pushd %~dp0%
git config --local core.hooksPath Tools/hooks
popd
