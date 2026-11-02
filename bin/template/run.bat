@echo off
rem Compila el QuakeC de la plantilla con fteqcc y/o la arranca en fteqw como juego propio.
rem
rem Uso:  run.bat [modo] [mapa]
rem   modo   all    compilar y, si no hay errores, correr (por defecto)
rem          build  solo compilar
rem          run    solo correr
rem   mapa   el mapa a cargar (por defecto start). Si existe maps\<mapa>.bsp se carga
rem          ese; si no, maps\<mapa>.map tal cual (el motor lee los .map sin compilar).
rem
rem El juego arranca con template.fmf: solo esta carpeta, nada de ninjawars ni de
rem default.fmf. Funciona desde cualquier carpeta y con doble clic.

setlocal
set "GAMEDIR=%~dp0"
set "GAMEDIR=%GAMEDIR:~0,-1%"
for %%I in ("%GAMEDIR%\..") do set "BIN=%%~fI"
set "QCC=%BIN%\fteqcc64.exe"
set "ENGINE=%BIN%\fteqw64.exe"

set "MODE=%~1"
set "MAP=%~2"
if "%MODE%"=="" set "MODE=all"
if "%MAP%"=="" set "MAP=start"

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
echo ===== fteqcc: compilando la plantilla =====
pushd "%GAMEDIR%\src" || goto :fail
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
set "MAPARG=%MAP%"
if not exist "%GAMEDIR%\maps\%MAP%.bsp" set "MAPARG=%MAP%.map"
echo ===== fteqw: arrancando la plantilla, mapa %MAPARG% =====
start "" "%ENGINE%" -basedir "%BIN%" -nohome -window -manifest "%GAMEDIR%\template.fmf" +set developer 1 +map %MAPARG%
goto :done

:fail
rem Con doble clic la ventana se cerraria sin dejar leer el error.
echo %CMDCMDLINE% | find /i "/c" >nul && pause
exit /b 1

:done
exit /b 0
