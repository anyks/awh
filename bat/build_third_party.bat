@echo off
rem ============================================================================
rem  Сборка зависимостей AWH оснасткой Microsoft Visual Studio
rem
rem  Средство это - САМОСТОЯТЕЛЬНЫЙ двойник sh/build_third_party.sh, а не оболочка
rem  над ним: MSYS2 у того, кто берёт AWH в проект на Visual Studio, может не быть
rem  вовсе, и требовать его установки ради зависимостей неверно.
rem
rem  Собранное кладётся в third_party\ рядом с деревом - тем же местом, каким
rem  пользуется и сборка под MinGW, но библиотеки выходят в виде .lib, и смешивать
rem  их с собранными MinGW нельзя: у оснасток разное устройство двоичного кода.
rem
rem  Употребление:
rem    build_third_party.bat                 собрать всё, что ещё не собрано
rem    build_third_party.bat --clean         снести собранное и очистить подмодули
rem    build_third_party.bat --rebuild       собрать заново, невзирая на отметки
rem    build_third_party.bat <имя>           собрать одну зависимость (zlib, lz4, ...)
rem
rem  Состав сборки, её порядок и версия каждой зависимости берутся из Requirements.txt
rem  в корне дерева - того же файла, каким правится сборка под MinGW.
rem
rem  Набор команд берётся у машины, но задаётся и вручную: set AWH_ARCH=x64
rem ============================================================================

setlocal enabledelayedexpansion

rem Каталог, где лежит это средство
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

rem Каталог сценариев зависимостей
set "SCRIPTS=%ROOT%\third_party"

rem Каталог дерева проекта
for %%I in ("%ROOT%\..") do set "TREE=%%~fI"

rem ----------------------------------------------------------------------------
rem  Каталог, куда кладётся собранное
rem
rem  У каждой оснастки он свой: двоичный код MinGW и MSVC несовместим, а часть
rem  зависимостей собирает и ЗАГОЛОВКИ под оснастку - zlib решает судьбу
rem  `unistd.h` именно при сборке. Общий каталог означал бы, что последняя
rem  собранная зависимость молча ломает сборку соседней оснасткой
rem ----------------------------------------------------------------------------
set "PREFIX=%TREE%\third_party-msvc"

rem Каталог подмодулей
set "SUBMODULES=%TREE%\submodules"

rem ----------------------------------------------------------------------------
rem  Перечень зависимостей и их версий
rem
rem  Он же управляет сборкой под MinGW: список один на обе оснастки, и держать
rem  второй такой же рядом значило бы обречь их разойтись. Здесь задаются и СОСТАВ
rem  сборки, и порядок, и версия каждой зависимости - править сценарии ради иной
rem  версии не нужно.
rem
rem  Файла может и не быть: дерево, взятое архивом, несёт не всё. Тогда собирается
rem  всё, что лежит в каталоге сценариев, каждое на версии, записанной в нём самом
rem ----------------------------------------------------------------------------
set "REQUIREMENTS=%TREE%\Requirements.txt"

rem ----------------------------------------------------------------------------
rem  Приводим путь к известному виду
rem
rem  Средство вправе быть позвано из чужой оболочки, чей путь ведёт к своим двойникам
rem  системных приёмов. Оснастка Visual Studio дополняет путь, а не заменяет его, и
rem  чужие двойники оказываются впереди: набор Windows тогда не находится вовсе, а
rem  отказ приходит в виде "rc: no such file or directory" - далеко от причины
rem ----------------------------------------------------------------------------
set "PATH=%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem"

rem ----------------------------------------------------------------------------
rem  Опознание набора команд
rem
rem  Спрашивается САМА система, а не переменные окружения: под подражанием x86-64
rem  они отвечают про подражание, а не про машину, и на машине ARM64 сборка молча
rem  уходила бы в x64. Признаком служит наличие каталога системных приёмов ARM64 -
rem  он есть только на машине ARM64 и подражанием не подделывается
rem ----------------------------------------------------------------------------
if not defined AWH_ARCH (
	rem Набор команд машины записан системой в её же настройках и подражанием не
	rem подделывается, в отличие от переменных окружения самой оболочки
	set "AWH_MACHINE="
	for /f "tokens=2*" %%A in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PROCESSOR_ARCHITECTURE 2^>nul ^| find "REG_SZ"') do set "AWH_MACHINE=%%B"

	if /i "!AWH_MACHINE!"=="ARM64" ( set "AWH_ARCH=arm64" ) else (
	if /i "!AWH_MACHINE!"=="AMD64" ( set "AWH_ARCH=x64" ) else (
	if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" ( set "AWH_ARCH=arm64" ) else (
	if /i "%PROCESSOR_ARCHITEW6432%"=="ARM64" ( set "AWH_ARCH=arm64" ) else (
		set "AWH_ARCH=x64"
	))))
)

rem ----------------------------------------------------------------------------
rem  Поиск Visual Studio
rem
rem  Ищется средством самой Visual Studio (vswhere), а не перебором путей: место
rem  установки задаётся при установке и у разных машин разное, а перебор путей
rem  молча берёт не ту редакцию, если их поставлено несколько.
rem ----------------------------------------------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
	echo [AWH] Visual Studio not found: vswhere.exe is missing
	echo [AWH] Install Visual Studio 2019 or newer with the "Desktop development with C++" workload
	exit /b 1
)

set "VSPATH="
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSPATH=%%I"

if not defined VSPATH (
	for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -property installationPath 2^>nul`) do set "VSPATH=%%I"
)

if not defined VSPATH (
	echo [AWH] Visual Studio with the C++ toolset not found
	exit /b 1
)

rem Заводим окружение оснастки под выбранный набор команд
call "%VSPATH%\VC\Auxiliary\Build\vcvarsall.bat" %AWH_ARCH% > nul
if errorlevel 1 (
	echo [AWH] Failed to initialize the MSVC environment for %AWH_ARCH%
	exit /b 1
)

rem ----------------------------------------------------------------------------
rem  Средства сборки
rem
rem  CMake и Ninja берутся из состава Visual Studio, если своих в пути нет: ставить
rem  их отдельно ради сборки зависимостей потребителю незачем.
rem ----------------------------------------------------------------------------
set "CMAKE=cmake"
where cmake > nul 2>&1 || set "CMAKE=%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

set "NINJA=ninja"
where ninja > nul 2>&1 || set "NINJA=%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

if not exist "%CMAKE%" if /i not "%CMAKE%"=="cmake" (
	echo [AWH] CMake not found neither in PATH nor in the Visual Studio installation
	exit /b 1
)

rem Число заданий сборки по числу вычислителей машины
set "JOBS=%NUMBER_OF_PROCESSORS%"
if not defined JOBS set "JOBS=2"

rem ----------------------------------------------------------------------------
rem  Средство работы с хранилищем исходников
rem
rem  Необязательно: оно нужно лишь чтобы перевести исходники на нужную версию, а
rem  собрать можно и то, что уже лежит на диске. Дерево, взятое архивом, хранилища
rem  не несёт вовсе, и требовать git ради сборки было бы требованием на пустом месте
rem ----------------------------------------------------------------------------
set "HAS_GIT=1"
where git > nul 2>&1 || set "HAS_GIT=0"
if "%HAS_GIT%"=="0" echo [AWH] git not found: sources will be built as they lie on disk

echo.
echo [AWH] Visual Studio: %VSPATH%
echo [AWH] Architecture:  %AWH_ARCH%
echo [AWH] Prefix:        %PREFIX%
echo [AWH] Jobs:          %JOBS%
echo.

rem Создаём каталоги, куда ляжет собранное
if not exist "%PREFIX%\lib" mkdir "%PREFIX%\lib" > nul 2>&1
if not exist "%PREFIX%\include" mkdir "%PREFIX%\include" > nul 2>&1

rem ----------------------------------------------------------------------------
rem  Разбор задания
rem ----------------------------------------------------------------------------
set "COMMAND=--build"
set "ONLY="

if not "%~1"=="" (
	if /i "%~1"=="--clean"   ( set "COMMAND=--clean" ) else (
	if /i "%~1"=="--rebuild" ( set "COMMAND=--rebuild" ) else (
		set "ONLY=%~1"
	))
)

rem ----------------------------------------------------------------------------
rem  Перебор зависимостей по перечню
rem
rem  Порядок берётся из перечня: зависимости связаны между собой, и слияние в один
rem  архив идёт последним.
rem ----------------------------------------------------------------------------
if exist "%REQUIREMENTS%" (
	rem ------------------------------------------------------------------------
	rem  Разбор перечня
	rem
	rem  Строка его - имя зависимости, признак и значение: "zlib --version 1.3.1".
	rem  Строки с решётки в счёт не идут (eol=#), как и строки с чёрточки: обе
	rem  служат тому, чтобы отключить зависимость, не стирая записи о ней
	rem ------------------------------------------------------------------------
	for /f "usebackq eol=# tokens=1,2,3" %%A in ("%REQUIREMENTS%") do (
		set "DEP=%%A"
		set "FLAG=%%B"
		set "VALUE=%%C"

		if not "!DEP:~0,1!"=="-" (
			rem Признак сокращается до трёх знаков: --version становится --v, и
			rem запись эта - та же, какой пользуется сборка под MinGW
			set "SPEC=!FLAG:~0,3!!VALUE!"

			call :run "!DEP!" "!SPEC!"
			if errorlevel 1 exit /b 1
		)
	)
) else (
	echo [AWH] Requirements.txt is missing: dependencies are built at the versions written in their own scripts
	echo.

	for /f "delims=" %%S in ('dir /b /on "%SCRIPTS%\*.bat" 2^>nul') do (
		set "NAME=%%~nS"

		rem Общая часть зависимостью не является: имя её начинается с подчёркивания
		if not "!NAME:~0,1!"=="_" (
			call :run "!NAME:~4!" ""
			if errorlevel 1 exit /b 1
		)
	)
)

echo.
echo [AWH] Done
exit /b 0

rem ----------------------------------------------------------------------------
rem  Сборка одной зависимости
rem
rem  %1 - название зависимости, %2 - запись о версии для её сценария
rem ----------------------------------------------------------------------------
:run
	set "SHORT=%~1"
	set "SPEC=%~2"

	rem Заказ на одну зависимость отсеивает прочие
	if defined ONLY if /i not "%SHORT%"=="%ONLY%" exit /b 0

	rem ------------------------------------------------------------------------
	rem  Поиск сценария
	rem
	rem  Ищется по имени зависимости, а числовая приставка его при этом не
	rem  спрашивается: порядок сборки задаёт перечень, а приставка осталась лишь
	rem  затем, чтобы сценарии лежали в том же порядке и без перечня
	rem ------------------------------------------------------------------------
	set "SCRIPT="
	for /f "delims=" %%S in ('dir /b /on "%SCRIPTS%\*_%SHORT%.bat" 2^>nul') do set "SCRIPT=%%S"

	if not defined SCRIPT (
		echo [AWH] %SHORT%: no build script found in %SCRIPTS%
		exit /b 1
	)

	echo [AWH] ****** %SHORT% ******
	call "%SCRIPTS%\!SCRIPT!" %COMMAND% %SPEC%
	if errorlevel 1 (
		echo [AWH] FAILED: %SHORT%
		exit /b 1
	)
exit /b 0
