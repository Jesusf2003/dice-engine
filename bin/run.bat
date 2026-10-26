@echo off
rem Compila el QuakeC de un mod con fteqcc y/o arranca fteqw.
rem
rem Uso:  run.bat [mod] [modo] [mapa]
rem   mod    carpeta del mod dentro de bin (por defecto ninjawars)
rem   modo   all    compilar y, si no hay errores, correr (por defecto)
rem          build  solo compilar
rem          run    solo correr
rem   mapa   opcional, carga ese mapa directamente (ej. run.bat ninjawars all start_demo)
rem
rem El mod se puede omitir: run.bat all start_demo sigue usando ninjawars.
rem Funciona desde cualquier carpeta y con doble clic.

setlocal
set "BIN=%~dp0"
set "BASEGAME=ninjawars"
set "QCC=%BIN%fteqcc64.exe"
set "ENGINE=%BIN%fteqw64.exe"

rem Si el primer argumento es un modo, no se indico mod.
set "GAME=%~1"
if /i "%GAME%"=="all"   set "GAME="
if /i "%GAME%"=="build" set "GAME="
if /i "%GAME%"=="run"   set "GAME="
if "%GAME%"=="" (
	set "GAME=%BASEGAME%"
	set "MODE=%~1"
	set "MAP=%~2"
) else (
	set "MODE=%~2"
	set "MAP=%~3"
)
if "%MODE%"=="" set "MODE=all"

if not exist "%BIN%%GAME%\" (
	echo ERROR: no existe el mod "%GAME%" en %BIN%
	goto :fail
)

if /i "%MODE%"=="all"   goto :build
if /i "%MODE%"=="build" goto :build
if /i "%MODE%"=="run"   goto :run
echo Modo desconocido: "%MODE%"  (usa all, build o run)
goto :fail

:build
if not exist "%BIN%%GAME%\src\progs.src" (
	if /i "%MODE%"=="build" (
		echo ERROR: %GAME% no tiene src\progs.src
		goto :fail
	)
	echo ===== %GAME% no tiene src\progs.src, se arranca sin compilar =====
	goto :run
)
if not exist "%QCC%" (
	echo ERROR: no se encuentra %QCC%
	goto :fail
)
echo ===== fteqcc: compilando %GAME% =====
pushd "%BIN%%GAME%\src" || goto :fail
"%QCC%" -srcfile progs.src
set "ERR=%ERRORLEVEL%"
popd
if not "%ERR%"=="0" (
	echo ===== ERROR de compilacion, no se arranca el motor =====
	goto :fail
)
if /i "%MODE%"=="build" goto :done

:run
if not exist "%ENGINE%" (
	echo ERROR: no se encuentra %ENGINE%
	goto :fail
)
rem default.fmf en esta carpeta hace de %BASEGAME% el juego base; -nohome deja todas las escrituras dentro de bin.
rem Cualquier otro mod se monta encima de la base con -game.
set "ARGS=-basedir "%BIN:~0,-1%" -nohome -window +set developer 1 +set sv_public 0"
if /i not "%GAME%"=="%BASEGAME%" set "ARGS=%ARGS% -game %GAME%"
if not "%MAP%"=="" set "ARGS=%ARGS% +map %MAP%"
echo ===== fteqw: arrancando %GAME% =====
start "" "%ENGINE%" %ARGS%
goto :done

:fail
rem Con doble clic la ventana se cerraria sin dejar leer el error.
echo %CMDCMDLINE% | find /i "/c" >nul && pause
exit /b 1

:done
exit /b 0
