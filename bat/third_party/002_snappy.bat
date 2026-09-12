@echo off
rem ============================================================================
rem  SNAPPY для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=snappy"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Задаётся она Requirements.txt в корне дерева, а записанное здесь берётся лишь
rem  тогда, когда записи в нём нет: сборка обязана работать и с деревом, взятым
rem  архивом без этого файла.
rem
rem  Вид метки у этой зависимости свой, потому приставка и хвост названы здесь, а не
rem  в общей части: из версии 1.2.1 выходит метка 1.2.1
rem ----------------------------------------------------------------------------
set "VPREFIX="
set "VSUFFIX="
set "VDEFAULT=1.2.1"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\snappy.lib" del /f /q "%PREFIX%\lib\snappy.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem Выполняем сборку
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DSNAPPY_INSTALL=YES -DSNAPPY_BUILD_TESTS=NO -DSNAPPY_FUZZING_BUILD=NO -DSNAPPY_BUILD_BENCHMARKS=NO"
if errorlevel 1 exit /b 1

rem ----------------------------------------------------------------------------
rem  Раскладка заголовков
rem
rem  AWH ищет заголовки зависимости в своём каталоге (include\snappy), а установка
rem  кладёт их россыпью в include. Раскладка эта - не опрятность: одноимённые
rem  заголовки разных зависимостей иначе затирали бы друг друга
rem ----------------------------------------------------------------------------
if not exist "%PREFIX%\include\snappy" mkdir "%PREFIX%\include\snappy" > nul 2>&1
for %%H in ("%PREFIX%\include\*.h" "%PREFIX%\include\*.hpp") do (
	if exist "%%~fH" move /y "%%~fH" "%PREFIX%\include\snappy\" > nul 2>&1
)

exit /b 0
