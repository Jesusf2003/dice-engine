@echo off
rem Compila el QuakeC de ninjawars con fteqcc y/o arranca fteqw.
rem
rem Uso:  run.bat [modo] [mapa]
rem   modo   all    compilar y, si no hay errores, correr (por defecto)
rem          build  solo compilar
rem          run    solo correr
rem   mapa   opcional, carga ese mapa directamente (ej. run.bat all start_demo)
rem
rem Funciona desde cualquier carpeta y con doble clic.

setlocal
set "BIN=%~dp0"
set "GAME=ninjawars"
set "QCC=%BIN%fteqcc64.exe"
set "ENGINE=%BIN%fteqw64.exe"

set "MODE=%~1"
if "%MODE%"=="" set "MODE=all"
set "MAP=%~2"

if /i "%MODE%"=="all"   goto :build
if /i "%MODE%"=="build" goto :build
if /i "%MODE%"=="run"   goto :run
echo Modo desconocido: "%MODE%"  (usa all, build o run)
goto :fail

:build
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
rem default.fmf en esta carpeta hace de %GAME% el juego base; -nohome deja todas las escrituras dentro de bin\%GAME%.
set "ARGS=-basedir "%BIN:~0,-1%" -nohome -window +set developer 1 +set sv_public 0"
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
