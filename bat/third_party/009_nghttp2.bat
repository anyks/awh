@echo off
rem ============================================================================
rem  NGHTTP2 для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=nghttp2"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Записана здесь, как и в build_third_party.sh этой ветки: отдельного перечня
rem  версий у v4 нет. Вторым доводом сценарий принимает запись --v/--b/--t/--c/--n,
rem  ею версию можно задать и вручную.
rem
rem  Вид метки у этой зависимости свой, потому приставка и хвост названы здесь, а
rem  не в общей части: из версии 1.67.1 выходит метка v1.67.1
rem ----------------------------------------------------------------------------
set "VPREFIX=v"
set "VSUFFIX="
set "VDEFAULT=1.67.1"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\nghttp2.lib" del /f /q "%PREFIX%\lib\nghttp2.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem ----------------------------------------------------------------------------
rem  Настройки сборки
rem
rem  Состав тот же, что у сборки под MinGW: одна библиотека, без приложений,
rem  примеров, HTTP/3 и привязок к чужим библиотекам.
rem
rem  ENABLE_STATIC_CRT здесь ВЫКЛЮЧЕН, хотя сборка под MinGW включает его: ключ
rem  переписывает /MD на /MT, а образ AWH собирается с одной CRT на всех
rem  зависимостей. Включённый ключ дал бы архив с иной CRT, и связывание отказало
rem  бы на смешении библиотек времени исполнения.
rem
rem  Хвост _static к имени архива зависимость добавляет лишь тогда, когда строит
rem  обе цели сразу; разделяемая запрещена общей частью, потому архив зовётся
rem  nghttp2.lib
rem ----------------------------------------------------------------------------
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DENABLE_STATIC_CRT=OFF -DENABLE_APP=OFF -DENABLE_DOC=OFF -DWITH_MRUBY=OFF -DWITH_LIBXML2=OFF -DWITH_LIBBPF=OFF -DWITH_NEVERBLEED=OFF -DENABLE_HTTP3=OFF -DENABLE_LIB_ONLY=ON -DENABLE_EXAMPLES=OFF -DBUILD_STATIC_LIBS=ON -DENABLE_APP_DEFAULT=ON -DENABLE_HPACK_TOOLS=OFF -DENABLE_DEBUG=OFF -DENABLE_WERROR=OFF -DENABLE_FAILMALLOC=OFF"
if errorlevel 1 exit /b 1

rem Заголовки установка кладёт в include\nghttp2 сама, раскладка им не нужна

exit /b 0