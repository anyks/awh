@echo off
rem ============================================================================
rem  PCRE2 для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=pcre2"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Записана здесь, как и в build_third_party.sh этой ветки: отдельного перечня
rem  версий у v4 нет. Вторым доводом сценарий принимает запись --v/--b/--t/--c/--n,
rem  ею версию можно задать и вручную.
rem
rem  Вид метки у этой зависимости свой, потому приставка и хвост названы здесь, а
rem  не в общей части: из версии 10.46 выходит метка pcre2-10.46
rem ----------------------------------------------------------------------------
set "VPREFIX=pcre2-"
set "VSUFFIX="
set "VDEFAULT=10.46"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\pcre2-8-static.lib" del /f /q "%PREFIX%\lib\pcre2-8-static.lib" > nul 2>&1
	if exist "%PREFIX%\lib\pcre2-16-static.lib" del /f /q "%PREFIX%\lib\pcre2-16-static.lib" > nul 2>&1
	if exist "%PREFIX%\lib\pcre2-32-static.lib" del /f /q "%PREFIX%\lib\pcre2-32-static.lib" > nul 2>&1
	if exist "%PREFIX%\lib\pcre2-posix-static.lib" del /f /q "%PREFIX%\lib\pcre2-posix-static.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem ----------------------------------------------------------------------------
rem  Настройки сборки
rem
rem  Состав тот же, что у сборки под MinGW: три разрядности кодовых единиц и
rem  POSIX-обёртка, без JIT и без привязки к чужим библиотекам.
rem
rem  PCRE2_STATIC_RUNTIME назван явно и оставлен выключенным: он переписывает /MD
rem  на /MT, а образ AWH собирается с одной CRT на всех зависимостей. Включённый
rem  здесь ключ дал бы архив с иной CRT, и связывание отказало бы на смешении
rem  библиотек времени исполнения
rem ----------------------------------------------------------------------------
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DPCRE2_STATIC_PIC=ON -DBUILD_STATIC_LIBS=ON -DPCRE2_BUILD_TESTS=OFF -DPCRE2_SUPPORT_UNICODE=ON -DPCRE2_BUILD_PCRE2_8=ON -DPCRE2_BUILD_PCRE2_16=ON -DPCRE2_BUILD_PCRE2_32=ON -DPCRE2_SUPPORT_JIT=OFF -DPCRE2_SUPPORT_LIBZ=OFF -DPCRE2_SUPPORT_LIBBZ2=OFF -DPCRE2_SUPPORT_LIBEDIT=OFF -DPCRE2_SUPPORT_LIBREADLINE=OFF -DPCRE2_STATIC_RUNTIME=OFF"
if errorlevel 1 exit /b 1

rem ----------------------------------------------------------------------------
rem  Четыре архива остаются раздельными
rem
rem  Сборка под MinGW сводит их в один libpcre2.a, и делает это извлечением
rem  модулей с переименованием двойников: имена модулей у разрядностей совпадают.
rem  Под MSVC в этом нет нужды - lib.exe сливает архивы целиком, а состав имён
rem  модулей ему не помеха: связыватель ищет по имени символа, не по имени модуля.
rem  Слияние делает 010_static.bat вместе с прочими зависимостями
rem ----------------------------------------------------------------------------

rem Раскладка заголовков: AWH ищет их в include\pcre2
call "%~dp0_common.bat" :headers "%NAME%"

exit /b 0