@rem build.bat -*- coding: us-ascii -*-
@echo off
rem
rem Project shochu
rem Copyright (C) 2025-2026 neige68
rem
rem Note: build (cmake + msbuild + ctest)
rem
rem Usage: build.bat [cm] [rebuild|re|clean] [d|deb] [r|rel]
rem   cm      run cmake again
rem   rebuild delete the build directory and run cmake again
rem   d       Debug only
rem   r       Release only
rem
rem The last line of the output is "BUILD SUCCEEDED." or "BUILD FAILED."
rem (do not trust the exit code: judge by that line)
rem

setlocal
pushd %~dp0

rem @VC: directory of vcvarsall.bat (VC142 = VS2019)
set @VC=%VC142%
set @CMAKEOPT=-G "Visual Studio 16 2019" -A Win32

set @exec_cmake=
set @bdir=build
set @build_deb=t
set @build_rel=t

:opt_loop
if "%1"=="" goto opt_end
if "%1"=="cm" set @exec_cmake=t
if "%1"=="cm" goto opt_next
if "%1"=="rebuild" goto opt_rebuild
if "%1"=="re" goto opt_rebuild
if "%1"=="clean" goto opt_rebuild
if "%1"=="d" set @build_rel=
if "%1"=="d" goto opt_next
if "%1"=="deb" set @build_rel=
if "%1"=="deb" goto opt_next
if "%1"=="r" set @build_deb=
if "%1"=="r" goto opt_next
if "%1"=="rel" set @build_deb=
if "%1"=="rel" goto opt_next
echo WARN: build.bat: ignore %1
:opt_next
shift
goto opt_loop
:opt_rebuild
if exist %@bdir% echo INFO: build.bat: deleting %@bdir% ...
if exist %@bdir% rmdir /s /q %@bdir%
if exist %@bdir% echo ERROR: build.bat: cannot delete %@bdir%
if exist %@bdir% goto err1
set @exec_cmake=t
goto opt_next
:opt_end

rem *** vcvars ***
if not "%VCToolsVersion%"=="" echo INFO: build.bat: VCToolsVersion is set. use the current environment.
if "%VCToolsVersion%"=="" call "%@VC%\vcvarsall.bat" x86

rem *** build directory ***
if not exist %@bdir% mkdir %@bdir%
pushd %@bdir%

rem *** cmake ***
if not exist ALL_BUILD.vcxproj set @exec_cmake=t
if not "%@exec_cmake%"=="" cmake %@CMAKEOPT% ..
if errorlevel 1 goto err2

rem *** debug ***
if "%@build_deb%"=="" goto skip_debug
msbuild ALL_BUILD.vcxproj /p:Configuration=Debug /m /nodeReuse:false
if errorlevel 1 goto err2
echo INFO: build.bat: msbuild Debug Done.
ctest -C Debug --output-on-failure --timeout 30
if errorlevel 1 goto err2
:skip_debug

rem *** release ***
if "%@build_rel%"=="" goto skip_release
msbuild ALL_BUILD.vcxproj /p:Configuration=Release /m /nodeReuse:false
if errorlevel 1 goto err2
echo INFO: build.bat: msbuild Release Done.
ctest -C Release --output-on-failure --timeout 30
if errorlevel 1 goto err2
:skip_release

popd
popd
echo BUILD SUCCEEDED.
goto end

rem *** error ***
:err2
popd
:err1
popd
echo BUILD FAILED.
exit /b 1

:end
rem end of build.bat
