@rem dist.bat -*- coding: cp932-dos -*-
@echo off
rem
rem Project shochu2
rem Copyright (C) 2026 neige68
rem
rem Note: dist
rem
rem Windows:   8 and later
rem

setlocal
title %~f0
pushd %~dp0
set @this=%~f0

mkdir build\dist
7z a build\dist\shochu2.7z .\build\release\shochu.exe shochu.ini.sample LICENSE

@rem end of dist.bat
