@echo off
rem ============================================================================
rem  GPERFTOOLS для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat ПОСЛЕДНИМ, уже после слияния зависимостей:
rem  архивы перехвата выдачи памяти объединению не подлежат, и разбор их в общий
rem  архив лишил бы перехват силы (см. 010_static.bat).
rem
rem  Общая часть сборки живёт в _common.bat, здесь остаётся лишь то, чем эта
rem  зависимость отличается от прочих.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=gperftools"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Записана здесь, как и в build_third_party.sh этой ветки: отдельного перечня
rem  версий у v4 нет. Вторым доводом сценарий принимает запись --v/--b/--t/--c/--n,
rem  ею версию можно задать и вручную.
rem
rem  Вид метки у этой зависимости свой, потому приставка названа здесь, а не в
rem  общей части: из версии 2.17.2 выходит метка gperftools-2.17.2. Сборка под
rem  MinGW заводит от этой метки рабочую ветку, но собирает ту же ревизию.
rem
rem  Правка patches/gperftools/gperftools.patch здесь не нужна: она чинит
rem  объявление malloc_usable_size на DragonFly и регистры ucontext на illumos,
rem  а оснастка Visual Studio работает лишь на MS Windows. Под MSVC ARM64 нужна
rem  своя правка, gperftools-msvc-arm64.patch: её наложение ведётся ниже.
rem ----------------------------------------------------------------------------
set "VPREFIX=gperftools-"
set "VSUFFIX="
set "VDEFAULT=2.17.2"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\tcmalloc_minimal.lib" del /f /q "%PREFIX%\lib\tcmalloc_minimal.lib" > nul 2>&1
	if exist "%PREFIX%\lib\common.lib" del /f /q "%PREFIX%\lib\common.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem ----------------------------------------------------------------------------
rem  Перевод исходников ДО правки
rem
rem  Порядок не вольный: перевод на метку после правки столкнул бы её с содержимым
rem  метки, а общая часть сборки переводит исходники ещё раз. Перевод на ту же
rem  метку правки не трогает, потому он и стоит раньше неё
rem ----------------------------------------------------------------------------
call "%~dp0_common.bat" :checkout "%NAME%"

rem ----------------------------------------------------------------------------
rem  Правка исходников под MSVC ARM64
rem
rem  В src/windows/system-alloc.cc символ tc_get_sysalloc_override объявлен, но
rem  тела не имеет: под MSVC его подменяет tc_get_sysalloc_default через pragma
rem  /alternatename. Развилка pragma знает лишь _M_IX86 и _M_X64, и на ARM64
rem  (_M_ARM64) подмена не выходит: InitSystemAllocators зовёт неразрешённый
rem  символ, и сэмплы падают на LNK2019. Правка добавляет _M_ARM64 к ветке x64 -
rem  имена символов у обеих без ведущего подчёркивания. На x64 правка инертна.
rem
rem  Правку кладёт patches\gperftools\gperftools-msvc-arm64.patch. Повторное
rem  наложение отказало бы, потому сперва спрашиваем, не наложена ли она уже:
rem  обратная проверка проходит ровно тогда, когда правка на месте
rem ----------------------------------------------------------------------------
set "PATCH=%PATCHES%\gperftools\gperftools-msvc-arm64.patch"

if not "%HAS_GIT%"=="1" (
	echo [AWH] %NAME%: git is absent, the ARM64 patch is not applied; the ARM64 build may fail
) else (
	"%GIT%" -C "%SUBMODULES%\%NAME%" apply --reverse --check "%PATCH%" > nul 2>&1
	if errorlevel 1 (
		"%GIT%" -C "%SUBMODULES%\%NAME%" apply "%PATCH%" || (
			echo [AWH] %NAME%: could not apply %PATCH%
			exit /b 1
		)
		echo [AWH] %NAME%: ARM64 patch applied
	) else (
		echo [AWH] %NAME%: ARM64 patch already applied
	)
)

rem ----------------------------------------------------------------------------
rem  Настройки сборки
rem
rem  Состав тот же, что у сборки под MinGW: только minimal-аллокатор, без
rem  отладочного и без проверок. Под MSVC зависимость и сама выбирает этот состав
rem  (if(MINGW OR MSVC) в её CMakeLists.txt), но ключи названы явно: молчаливая
rem  ветвь чужого сценария при смене версии зависимости исчезла бы незаметно.
rem
rem  Установка заказана НЕ БЫТЬ: цель install у tcmalloc_minimal закрыта ключом
rem  gperftools_enable_broken_install_targets, и cmake --install положил бы лишь
rem  заголовки, оставив архивы в каталоге сборки. Перенос потому ведётся ниже
rem  своими руками - тем же порядком, что у сборки под MinGW.
rem ----------------------------------------------------------------------------
call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "-DBUILD_TESTING=OFF -DDEFAULT_BUILD_MINIMAL=ON -DDEFAULT_BUILD_DEBUGALLOC=OFF" "" "NOINSTALL"
if errorlevel 1 exit /b 1

rem ----------------------------------------------------------------------------
rem  Перенос собранного
rem
rem  Архивы лежат в корне каталога сборки: зависимость не задаёт
rem  CMAKE_ARCHIVE_OUTPUT_DIRECTORY, а генераторы этой оснастки (Ninja и NMake)
rem  одноконфигурационные, потому подкаталога Release у них нет.
rem
rem  Заголовки берутся из src, а не из каталога установки: своих подкаталогов
rem  gperftools в include не заводит, и раскладка _common.bat здесь не нужна.
rem ----------------------------------------------------------------------------
if not exist "%PREFIX%\include\%NAME%" mkdir "%PREFIX%\include\%NAME%" > nul 2>&1

set "COPIED=0"
for %%L in ("%SUBMODULES%\%NAME%\build-msvc\*.lib") do (
	if exist "%%~fL" (
		copy /y "%%~fL" "%PREFIX%\lib\" > nul
		if errorlevel 1 (
			echo [AWH] %NAME%: could not copy "%%~fL"
			exit /b 1
		)
		set /a COPIED+=1
	)
)

if "!COPIED!"=="0" (
	echo [AWH] %NAME%: no libraries were found in build-msvc
	exit /b 1
)

set "HEADERS=0"
for %%H in ("%SUBMODULES%\%NAME%\src\*.h") do (
	if exist "%%~fH" (
		copy /y "%%~fH" "%PREFIX%\include\%NAME%\" > nul
		set /a HEADERS+=1
	)
)

rem ----------------------------------------------------------------------------
rem  Что остаётся оснастке проекта
rem
rem  Перехват выдачи памяти под MSVC требует ключа /INCLUDE:__tcmalloc: зависимость
rem  объявляет его INTERFACE-свойством цели, а AWH присоединяет архивы ПУТЕМ К
rem  ФАЙЛУ (cmake/FindTcMalloc.cmake), и свойства цели до него не доходят. Ключ
rem  потому обязана положить оснастка проекта - без него аллокатор линкуется, но
rem  не перехватывает.
rem ----------------------------------------------------------------------------
echo [AWH] %NAME%: copied !COPIED! libraries and !HEADERS! headers
exit /b 0