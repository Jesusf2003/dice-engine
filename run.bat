@echo off
rem Build/launch helper for dice-engine on Windows; usable from cmd, PowerShell or a VS Code task.
rem
rem   run.bat [command] [-game <dir>] [-map <map>] [-dev] [-n] [extra args...]
rem
rem Commands:
rem   buildrun  compile the game's QuakeC, then start the engine (default)
rem   build     compile the game's QuakeC only (build_win64.sh game, through MSYS2 UCRT64)
rem   run       start the engine only: bin\<PROJECT>_x64.exe when it was built for the game,
rem             otherwise the generic bin\fteqw.exe
rem   fteqw     start the generic engine bin\fteqw.exe
rem   fteqcc    run bin\fteqcc.exe inside bin\<game>\src (no extra args: compiles progs.src there)
rem
rem Options:
rem   -game <dir>  game/mod directory under bin\ (default: %GAME% or base)
rem   -map <map>   map to load on start (+map <map>)
rem   -dev         developer 1, and +devmap instead of +map (cheats allowed)
rem   -n           print the command instead of running it
rem   Anything else goes to the engine (e.g. +set sv_cheats 1) or, for fteqcc, to the compiler.
rem
rem Environment: MSYS2_ROOT (default C:\msys64), GAME.

setlocal EnableExtensions DisableDelayedExpansion

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "BIN=%ROOT%\bin"
if not defined GAME set "GAME=base"
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
set "CMD=buildrun"
set "MAP="
set "DEV=0"
set "DRY=0"
set "EXTRA="

rem The first argument is the command unless it is an option or an engine +command.
set "FIRST=%~1"
if not defined FIRST goto parse
if "%FIRST:~0,1%"=="-" goto parse
if "%FIRST:~0,1%"=="+" goto parse
if "%FIRST%"=="/?" goto usage
set "CMD=%FIRST%"
shift

:parse
if "%~1"=="" goto parsed
if /i "%~1"=="-game" goto opt_game
if /i "%~1"=="-map" goto opt_map
if /i "%~1"=="-dev" (set "DEV=1" & shift & goto parse)
if /i "%~1"=="-n" (set "DRY=1" & shift & goto parse)
if /i "%~1"=="-h" goto usage
if /i "%~1"=="-help" goto usage
set EXTRA=%EXTRA% %1
shift
goto parse

:opt_game
if "%~2"=="" (echo error: -game needs a directory name & exit /b 1)
set "GAME=%~2"
shift & shift
goto parse

:opt_map
if "%~2"=="" (echo error: -map needs a map name & exit /b 1)
set "MAP=%~2"
shift & shift
goto parse

:parsed
if not exist "%BIN%\%GAME%\" (
	echo error: game directory bin\%GAME% not found
	exit /b 1
)
if /i "%CMD%"=="buildrun" goto cmd_buildrun
if /i "%CMD%"=="build" goto cmd_build
if /i "%CMD%"=="run" goto cmd_run
if /i "%CMD%"=="fteqw" goto cmd_fteqw
if /i "%CMD%"=="fteqcc" goto cmd_fteqcc
if /i "%CMD%"=="help" goto usage
echo error: unknown command "%CMD%"
echo.
call :usage
exit /b 1

:cmd_buildrun
call :build || exit /b 1
call :find_engine
call :launch
exit /b %errorlevel%

:cmd_build
call :build
exit /b %errorlevel%

:cmd_run
call :find_engine
call :launch
exit /b %errorlevel%

:cmd_fteqw
set "ENGINE=%BIN%\fteqw.exe"
call :launch
exit /b %errorlevel%

:cmd_fteqcc
if not exist "%BIN%\fteqcc.exe" (
	echo error: bin\fteqcc.exe not found, build it with: build_win64.sh fteqcc
	exit /b 1
)
if not exist "%BIN%\%GAME%\src\" (
	echo error: bin\%GAME%\src not found
	exit /b 1
)
if "%DRY%"=="1" (
	echo [bin\%GAME%\src] "%BIN%\fteqcc.exe"%EXTRA%
	exit /b 0
)
pushd "%BIN%\%GAME%\src"
"%BIN%\fteqcc.exe"%EXTRA%
set "RC=%errorlevel%"
popd
exit /b %RC%

rem ---------------------------------------------------------------------------

:build
if not exist "%MSYS2_ROOT%\usr\bin\bash.exe" (
	echo error: MSYS2 not found in %MSYS2_ROOT%, set MSYS2_ROOT
	exit /b 1
)
rem GAME is inherited by bash; CHERE_INVOKING keeps the login shell in the repo root.
if "%DRY%"=="1" (
	echo [%ROOT%] MSYSTEM=UCRT64 GAME=%GAME% bash -lc "./build_win64.sh game"
	exit /b 0
)
pushd "%ROOT%"
set "MSYSTEM=UCRT64"
set "CHERE_INVOKING=1"
"%MSYS2_ROOT%\usr\bin\bash.exe" -lc "./build_win64.sh game"
set "RC=%errorlevel%"
popd
if not "%RC%"=="0" echo error: build failed (exit code %RC%)
exit /b %RC%

rem Same name rule as bin\Tools\get-name.sh: first line of <game>\PROJECT without spaces or dashes.
:find_engine
set "NAME="
if exist "%BIN%\%GAME%\PROJECT" set /p NAME=<"%BIN%\%GAME%\PROJECT"
if not defined NAME set "NAME=%GAME%"
set "NAME=%NAME: =%"
set "NAME=%NAME:-=%"
set "ENGINE=%BIN%\%NAME%_x64.exe"
if exist "%ENGINE%" exit /b 0
echo note: bin\%NAME%_x64.exe not built (build_win64.sh engine), using bin\fteqw.exe
set "ENGINE=%BIN%\fteqw.exe"
exit /b 0

:launch
if not exist "%ENGINE%" (
	echo error: %ENGINE% not found
	exit /b 1
)
set "ARGS=+game %GAME%"
if "%DEV%"=="1" set "ARGS=%ARGS% +set developer 1"
if not defined MAP goto launch_go
if "%DEV%"=="1" (set "ARGS=%ARGS% +devmap %MAP%") else (set "ARGS=%ARGS% +map %MAP%")

:launch_go
if "%DRY%"=="1" (
	echo [bin] "%ENGINE%" %ARGS%%EXTRA%
	exit /b 0
)
rem The engine uses its working directory as the base dir, so start it from bin\.
start "" /D "%BIN%" "%ENGINE%" %ARGS%%EXTRA%
exit /b 0

:usage
echo usage: run.bat [buildrun^|build^|run^|fteqw^|fteqcc] [-game ^<dir^>] [-map ^<map^>] [-dev] [-n] [args...]
echo.
echo   buildrun  compile the game's QuakeC, then start the engine (default)
echo   build     compile the game's QuakeC only
echo   run       start the game's engine (bin\^<PROJECT^>_x64.exe, else bin\fteqw.exe)
echo   fteqw     start the generic engine bin\fteqw.exe
echo   fteqcc    run bin\fteqcc.exe in bin\^<game^>\src with the extra args
echo.
echo   -game ^<dir^>  game/mod directory under bin\ (default: base)
echo   -map ^<map^>   map to load on start
echo   -dev         developer mode, +devmap instead of +map
echo   -n           print the command instead of running it
echo.
echo examples:
echo   run.bat                              build base and play
echo   run.bat run -map demo -dev           play demo.bsp with cheats
echo   run.bat build -game mymod            compile bin\mymod
echo   run.bat fteqw -game base +set r_fullbright 1
echo   run.bat fteqcc -game base -srcfile client/progs.src
exit /b 0
