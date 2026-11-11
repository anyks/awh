@echo off
rem ============================================================================
rem  BZIP2 для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=bz2"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Сборка под MinGW этой ветки следует за master, а не за меткой: запись о
rem  версии у неё снята, и довод её тот же - правки, которых ждёт AWH, лежат в
rem  ветке. Здесь она следует за тем же master.
rem
rem  В отметку о сделанном пишется снимок ветки, а не её имя: имя не меняется, и
rem  собранное прежде считалось бы годным после того, как ветка ушла вперёд.
rem  Без хранилища снимка не прочесть, и тогда отметкой служит имя ветки.
rem
rem  Вторым доводом сценарий принимает запись --v/--b/--t/--c/--n: ею версию
rem  можно задать и вручную, не правя этот файл
rem ----------------------------------------------------------------------------
set "SPEC=%~2"
if "%SPEC%"=="" set "SPEC=--bmaster"

call "%~dp0_common.bat" :version "%SPEC%" "v" "" "master"
if errorlevel 1 exit /b 1

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\bz2_static.lib" del /f /q "%PREFIX%\lib\bz2_static.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem Переводим исходники на ветку и читаем снимок, на котором они встали
call "%~dp0_common.bat" :checkout "%NAME%"
call "%~dp0_common.bat" :commit "%NAME%"

set "VERSION=%AWH_REF%"
if not "%AWH_HASH%"=="" (
	set "VERSION=!AWH_HASH!"
	call "%~dp0_common.bat" :version "--c!AWH_HASH!" "v" "" "master"
)

rem ----------------------------------------------------------------------------
rem  Выполняем сборку
rem
rem  Набор настроек тот же, что у сборки под MinGW: одна библиотека, без
rem  приложений, образцов, справок и проверок. Разделяемая цель запрещена, а
rem  статическая названа явно - по умолчанию эта зависимость собирает обе
rem ----------------------------------------------------------------------------
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DENABLE_LIB_ONLY=ON -DENABLE_DOCS=OFF -DENABLE_TESTS=OFF -DENABLE_DEBUG=OFF -DENABLE_SHARED_LIB=OFF -DENABLE_STATIC_LIB=ON"
if errorlevel 1 exit /b 1

rem Раскладка заголовков: AWH ищет их в include\bz2
call "%~dp0_common.bat" :headers "%NAME%"

exit /b 0