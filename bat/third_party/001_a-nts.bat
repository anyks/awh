@echo off
rem ============================================================================
rem  A-NTS для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=a-nts"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  A-NTS доводится под требования AWH v4 прямо сейчас, поэтому сборка берёт
rem  свежий main, а не снимок, записанный в суперпроекте: иначе правки наружной
rem  библиотеки не попадали бы в сборку, пока кто-нибудь не сдвинет указатель.
rem  Запись подмодуля остаётся отметкой о том, на чём последний раз принимали.
rem  Довод тот же, что у build_third_party.sh этой ветки.
rem
rem  В отметку о сделанном пишется снимок ветки, а не её имя: имя не меняется, и
rem  собранное прежде считалось бы годным после того, как ветка ушла вперёд.
rem  Без хранилища снимка не прочесть, и тогда отметкой служит имя ветки.
rem
rem  Вторым доводом сценарий принимает запись --v/--b/--t/--c/--n: ею версию
rem  можно задать и вручную, не правя этот файл
rem ----------------------------------------------------------------------------
set "SPEC=%~2"
if "%SPEC%"=="" set "SPEC=--bmain"

call "%~dp0_common.bat" :version "%SPEC%" "v" "" "main"
if errorlevel 1 exit /b 1

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\nts.lib" del /f /q "%PREFIX%\lib\nts.lib" > nul 2>&1
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
	call "%~dp0_common.bat" :version "--c!AWH_HASH!" "v" "" "main"
)

rem ----------------------------------------------------------------------------
rem  Выполняем сборку
rem
rem  Проверки, замеры, образцы и размытие сборщиком здесь ни к чему: AWH берёт от
rem  A-NTS одну статическую библиотеку. Разделяемая сборка запрещена общей частью,
rem  и признак ANTS_STATICLIB она ставит себе сама - без него заголовки объявляют
rem  символы импортом из DLL, и связывание отвечает сворой __imp_
rem ----------------------------------------------------------------------------
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DCMAKE_BUILD_TESTS=OFF -DCMAKE_BUILD_SANITIZE=OFF -DCMAKE_BUILD_BENCHMARKS=OFF -DCMAKE_BUILD_SAMPLES=OFF -DCMAKE_BUILD_LIBFUZZER=OFF"
if errorlevel 1 exit /b 1

rem Заголовки установка кладёт в include\nts сама, раскладка им не нужна

exit /b 0