@echo off
setlocal enabledelayedexpansion

set version_file=version.h
set build_number=0

rem Extract the current build number from the version file
for /f "tokens=3" %%A in ('findstr /R /C:"#define PROJECT_VERSION_BUILD" %version_file%') do (
    set build_number=%%A
)

echo Current build number: %build_number%

rem Increment the build number
set /a new_build_number=%build_number%+1

echo New build number: %new_build_number%

rem Replace the old build number with the new one
(for /f "delims=" %%A in (%version_file%) do (
    set line=%%A
    if "%%A"=="#define PROJECT_VERSION_BUILD %build_number%" (
        echo #define PROJECT_VERSION_BUILD %new_build_number%
    ) else (
        echo %%A
    )
)) > temp_version.h

move /Y temp_version.h %version_file%
endlocal
