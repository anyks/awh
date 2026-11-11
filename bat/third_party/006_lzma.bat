@echo off
rem ============================================================================
rem  LZMA для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=lzma"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Записана здесь, как и в build_third_party.sh этой ветки: отдельного перечня
rem  версий у v4 нет. Вторым доводом сценарий принимает запись --v/--b/--t/--c/--n,
rem  ею версию можно задать и вручную.
rem
rem  Вид метки у этой зависимости свой, потому приставка и хвост названы здесь, а
rem  не в общей части: из версии 5.2.3 выходит метка v5.2.3-p4. Сборка под MinGW
rem  заводит ветку от той же метки, так что собранное у обеих оснасток одно
rem ----------------------------------------------------------------------------
set "VPREFIX=v"
set "VSUFFIX=-p4"
set "VDEFAULT=5.2.3"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\lzma.lib" del /f /q "%PREFIX%\lib\lzma.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem ----------------------------------------------------------------------------
rem  Перевод исходников ДО правки
rem
rem  Порядок здесь не вольный: перевод на метку после правки столкнул бы её с
rem  содержимым метки, а общая часть сборки переводит исходники ещё раз. Перевод
rem  на ту же метку правки не трогает, потому он и стоит раньше неё
rem ----------------------------------------------------------------------------
call "%~dp0_common.bat" :checkout "%NAME%"

rem ----------------------------------------------------------------------------
rem  Правка исходников под MSVC
rem
rem  Метка v5.2.3-p4 уже знает про MSVC x64: в memcmplen.h у неё есть и развилка
rem  _MSC_VER && _M_X64, и вызов _BitScanForward64. Не хватает ей одного -
rem  объявления самой внутренней функции: <intrin.h> с pragma intrinsic. Без него
rem  сборка x64 отказывает на необъявленном _BitScanForward64.
rem
rem  Правку кладёт patches\lzma\lzma.patch, тот же, что и сборка под MinGW.
rem  Повторное наложение отказало бы, потому сперва спрашиваем, не наложена ли она
rem  уже: обратная проверка проходит ровно тогда, когда правка на месте
rem ----------------------------------------------------------------------------
set "PATCH=%PATCHES%\lzma\lzma.patch"

if not "%HAS_GIT%"=="1" (
	echo [AWH] %NAME%: git is absent, the MSVC patch is not applied; the x64 build may fail
) else (
	"%GIT%" -C "%SUBMODULES%\%NAME%" apply --reverse --check "%PATCH%" > nul 2>&1
	if errorlevel 1 (
		"%GIT%" -C "%SUBMODULES%\%NAME%" apply "%PATCH%" || (
			echo [AWH] %NAME%: could not apply %PATCH%
			exit /b 1
		)
		echo [AWH] %NAME%: patch applied
	) else (
		echo [AWH] %NAME%: patch already applied
	)
)

rem Выполняем сборку
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DLZMA_BUILD_TESTS=OFF"
if errorlevel 1 exit /b 1

rem Раскладка заголовков: AWH ищет их в include\lzma, вложенный include\lzma\lzma
rem установка создаёт сама и он уже лежит где надо
call "%~dp0_common.bat" :headers "%NAME%"

exit /b 0