@echo off
rem ============================================================================
rem  BROTLI для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=brotli"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Записана здесь, как и в build_third_party.sh этой ветки: отдельного перечня
rem  версий у v4 нет. Вторым доводом сценарий принимает запись --v/--b/--t/--c/--n,
rem  ею версию можно задать и вручную.
rem
rem  Вид метки у этой зависимости свой, потому приставка и хвост названы здесь, а
rem  не в общей части: из версии 1.1.0 выходит метка v1.1.0
rem ----------------------------------------------------------------------------
set "VPREFIX=v"
set "VSUFFIX="
set "VDEFAULT=1.1.0"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\brotlicommon.lib" del /f /q "%PREFIX%\lib\brotlicommon.lib" > nul 2>&1
	if exist "%PREFIX%\lib\brotlidec.lib" del /f /q "%PREFIX%\lib\brotlidec.lib" > nul 2>&1
	if exist "%PREFIX%\lib\brotlienc.lib" del /f /q "%PREFIX%\lib\brotlienc.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem ----------------------------------------------------------------------------
rem  Настройки сборки
rem
rem  Ключ BROTLI_BUILD_TOOLS, каким пользуется сборка v5, эта версия зависимости
rem  не знает: у неё консольная цель brotli заводится безусловно. Отказывать ей
rem  нечем, а собранный brotli.exe в общий архив не попадает - он не библиотека.
rem
rem  Три архива остаются раздельными, и слияние их в один делает 010_static.bat
rem  вместе с прочими зависимостями. Сборка под MinGW сводит их в libbrotli.a
rem  сама, но это имя нигде не ищется: AWH присоединяет один общий архив
rem ----------------------------------------------------------------------------
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DBROTLI_DISABLE_TESTS=ON"
if errorlevel 1 exit /b 1

rem Заголовки установка кладёт в include\brotli сама, раскладка им не нужна

exit /b 0