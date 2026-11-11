@echo off
rem ============================================================================
rem  LZ4 для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=lz4"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Записана здесь, как и в build_third_party.sh этой ветки: отдельного перечня
rem  версий у v4 нет. Вторым доводом сценарий принимает запись --v/--b/--t/--c/--n,
rem  ею версию можно задать и вручную.
rem
rem  Вид метки у этой зависимости свой, потому приставка и хвост названы здесь, а
rem  не в общей части: из версии 1.10.0 выходит метка v1.10.0
rem ----------------------------------------------------------------------------
set "VPREFIX=v"
set "VSUFFIX="
set "VDEFAULT=1.10.0"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\lz4.lib" del /f /q "%PREFIX%\lib\lz4.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem ----------------------------------------------------------------------------
rem  Выполняем сборку
rem
rem  Описание сборки лежит не в корне исходников, а в build\cmake. Имя архива
rem  выходит lz4.lib: приставку _static эта зависимость даёт лишь тогда, когда
rem  рядом собирается и разделяемая цель, а она запрещена общей частью
rem ----------------------------------------------------------------------------
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DBUILD_STATIC_LIBS=ON" "build\cmake"
if errorlevel 1 exit /b 1

rem Раскладка заголовков: AWH ищет их в include\lz4
call "%~dp0_common.bat" :headers "%NAME%"

exit /b 0