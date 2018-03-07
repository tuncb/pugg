@echo off

setlocal
pushd %1

set build_folder="%~dp0./build/" || goto :FINALLY

if exist %build_folder% rd /q /s %build_folder% || goto :FINALLY
md %build_folder% || goto :FINALLY
cd %build_folder% || goto :FINALLY

cmake -G "Visual Studio 15 2017 Win64" ../ || goto :FINALLY

echo Success :)

:FINALLY
  popd
  endlocal

  IF /I "%ERRORLEVEL%" NEQ "0" (
      echo Solution generation failed with error #%ERRORLEVEL%.
      exit /b %ERRORLEVEL%
  )