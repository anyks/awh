@echo off
rem ============================================================================
rem  ZLIB для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=zlib"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Записана здесь, как и в build_third_party.sh этой ветки: отдельного перечня
rem  версий у v4 нет. Вторым доводом сценарий принимает запись --v/--b/--t/--c/--n,
rem  ею версию можно задать и вручную.
rem
rem  Вид метки у этой зависимости свой, потому приставка и хвост названы здесь, а
rem  не в общей части: из версии 1.3.1 выходит метка v1.3.1
rem ----------------------------------------------------------------------------
set "VPREFIX=v"
set "VSUFFIX="
set "VDEFAULT=1.3.1"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\zlibstatic.lib" del /f /q "%PREFIX%\lib\zlibstatic.lib" > nul 2>&1
	if exist "%PREFIX%\lib\zlib.lib" del /f /q "%PREFIX%\lib\zlib.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem Выполняем сборку
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DZLIB_BUILD_EXAMPLES=NO"
if errorlevel 1 exit /b 1

rem ----------------------------------------------------------------------------
rem  Снос разделяемой библиотеки и её спутника
rem
rem  Запрет разделяемой сборки эта зависимость НЕ соблюдает: она заводит обе цели
rem  всегда, и рядом с настоящим архивом (zlibstatic.lib) остаётся импортный
rem  (zlib.lib) - тот, что несёт не код, а отсылки к нему.
rem
rem  Слитый в общий архив, импортный отбирает имена у настоящего, а отсылки его
rem  при слиянии теряются. Связывание чужой программы отвечает тогда
rem  «неразрешённое внешнее имя gzwrite» - об имени, какое в архиве ЕСТЬ, и даже
rem  дважды: отказ указывает на нехватку там, где на деле избыток.
rem
rem  Тот же снос делает и сборка под MinGW, только имена у неё свои: libzlib.dll
rem  и libzlib.dll.a
rem ----------------------------------------------------------------------------
if exist "%PREFIX%\lib\zlib.lib" del /f /q "%PREFIX%\lib\zlib.lib" > nul 2>&1
if exist "%PREFIX%\lib\zlib.dll" del /f /q "%PREFIX%\lib\zlib.dll" > nul 2>&1
if exist "%PREFIX%\bin\zlib.dll" del /f /q "%PREFIX%\bin\zlib.dll" > nul 2>&1

rem Раскладка заголовков: AWH ищет их в include\zlib
call "%~dp0_common.bat" :headers "%NAME%"

exit /b 0