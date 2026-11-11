@echo off
rem ============================================================================
rem  Сборка зависимостей AWH v4 оснасткой Microsoft Visual Studio
rem
rem  Средство это - САМОСТОЯТЕЛЬНЫЙ двойник build_third_party.sh, а не оболочка
rem  над ним: MSYS2 у того, кто берёт AWH в проект на Visual Studio, может не быть
rem  вовсе, и требовать его установки ради зависимостей неверно.
rem
rem  Собранное кладётся в third_party-msvc\ рядом с деревом - место, отличное от
rem  third_party\, каким пользуется сборка под MinGW. Библиотеки выходят в виде
rem  .lib, и смешивать их с собранными MinGW нельзя: у оснасток разное устройство
rem  двоичного кода, а часть зависимостей собирает и ЗАГОЛОВКИ под оснастку -
rem  zlib решает судьбу `unistd.h` именно при сборке.
rem
rem  Употребление:
rem    build_third_party.bat                 собрать всё, что ещё не собрано
rem    build_third_party.bat --clean         снести собранное, исходники не трогая
rem    build_third_party.bat --rebuild       собрать заново, невзирая на отметки
rem    build_third_party.bat <имя>           собрать одну зависимость (zlib, lz4, ...)
rem
rem  Состав и порядок сборки записаны ниже, в перечне: зависимости связаны между
rem  собой, слияние в один архив идёт перед GPerfTools, а не после него.
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
rem  У каждой оснастки он свой: двоичный код MinGW и MSVC несовместим, а общий
rem  каталог означал бы, что последняя собранная зависимость молча ломает сборку
rem  соседней оснасткой
rem ----------------------------------------------------------------------------
set "PREFIX=%TREE%\third_party-msvc"

rem Каталог подмодулей
set "SUBMODULES=%TREE%\submodules"

rem Каталог правок к исходникам зависимостей
set "PATCHES=%TREE%\patches"

rem ----------------------------------------------------------------------------
rem  Приводим путь к известному виду
rem
rem  Средство вправе быть позвано из чужой оболочки, чей путь ведёт к своим
rem  двойникам системных приёмов. Оснастка Visual Studio дополняет путь, а не
rem  заменяет его, и чужие двойники оказываются впереди: набор Windows тогда не
rem  находится вовсе, а отказ приходит в виде "rc: no such file or directory" -
rem  далеко от причины.
rem
rem  Прежний путь не выбрасывается, а запоминается: CMake и Ninja, поставленные
rem  отдельно от Visual Studio, ищутся и в нём - иначе сброс пути прятал бы их, и
rem  на машине без части CMake в составе Visual Studio сборка отказывала бы в
rem  средстве, какое лежит на месте
rem ----------------------------------------------------------------------------
set "AWH_PATH=%PATH%"
set "PATH=%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem"

rem ----------------------------------------------------------------------------
rem  Опознание набора команд
rem
rem  Спрашивается САМА система, а не переменные окружения: под подражанием x86-64
rem  они отвечают про подражание, а не про машину, и на машине ARM64 сборка молча
rem  уходила бы в x64. Набор команд машины записан системой в её же настройках и
rem  подражанием не подделывается
rem ----------------------------------------------------------------------------
if not defined AWH_ARCH (
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
rem  молча берёт не ту редакцию, если их поставлено несколько
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
rem  Порядок поиска: заданное вручную (AWH_CMAKE, AWH_NINJA), часть CMake в
rem  составе Visual Studio, отдельная установка CMake, путь, с каким средство
rem  позвано. Часть CMake в установщике Visual Studio необязательна, и её может не
rem  быть вовсе - тогда годится CMake, поставленный отдельно.
rem
rem  Ninja нет - сборка идёт генератором "NMake Makefiles" из состава оснастки:
rem  она медленнее, в одно задание, но не отказывает. Молчать об этом нельзя:
rem  сборка, ставшая вчетверо дольше без объяснения, разыскивается потом как чужая
rem  беда
rem ----------------------------------------------------------------------------
set "CMAKE="
if defined AWH_CMAKE set "CMAKE=%AWH_CMAKE%"
if not defined CMAKE if exist "%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE=%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not defined CMAKE if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE=%ProgramFiles%\CMake\bin\cmake.exe"
if not defined CMAKE for %%I in (cmake.exe) do if not "%%~$AWH_PATH:I"=="" set "CMAKE=%%~$AWH_PATH:I"

if not defined CMAKE (
	echo [AWH] CMake not found: install the "C++ CMake tools for Windows" component of Visual Studio,
	echo [AWH] install CMake separately or set AWH_CMAKE to cmake.exe
	exit /b 1
)

set "NINJA="
if defined AWH_NINJA set "NINJA=%AWH_NINJA%"
if not defined NINJA if exist "%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" set "NINJA=%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if not defined NINJA for %%I in (ninja.exe) do if not "%%~$AWH_PATH:I"=="" set "NINJA=%%~$AWH_PATH:I"

if defined NINJA (
	set "GENERATOR=Ninja"
) else (
	set "GENERATOR=NMake Makefiles"
	echo [AWH] Ninja not found: building with NMake in a single job, which is slower
	echo [AWH] install Ninja or set AWH_NINJA to ninja.exe to build in parallel
)

rem ----------------------------------------------------------------------------
rem  Совместимость с CMake 4
rem
rem  CMake 4 отверг проекты, заявляющие совместимость ниже 3.5, а часть
rem  зависимостей заявляет именно такую: lzma 5.2.3 просит 3.0, и настройка
rem  отказывает ещё до сборки. Переменная та же, что у build_third_party.sh, -
rem  CMake читает её из окружения, и прежним версиям она ничем не мешает
rem ----------------------------------------------------------------------------
set "CMAKE_POLICY_VERSION_MINIMUM=3.5"

rem Число заданий сборки по числу вычислителей машины
set "JOBS=%NUMBER_OF_PROCESSORS%"
if not defined JOBS set "JOBS=2"

rem ----------------------------------------------------------------------------
rem  Средство работы с хранилищем исходников
rem
rem  Необязательно: оно нужно лишь чтобы перевести исходники на нужную версию, а
rem  собрать можно и то, что уже лежит на диске. Дерево, взятое архивом, хранилища
rem  не несёт вовсе, и требовать git ради сборки было бы требованием на пустом
rem  месте. Правка к lzma без него, впрочем, не накладывается - об этом сказано
rem  в её сценарии.
rem
rem  Ищется и в прежнем пути, как CMake и Ninja выше: сброшенный путь ведёт лишь
rem  в набор Windows, а git там не живёт никогда - ни у Git for Windows, ни у
rem  MSYS2. Без поиска в прежнем пути средство молча считало бы хранилища
rem  отсутствующим на машине, где оно стоит, и правка к lzma не легла бы.
rem
rem  Путь к нему запоминается ЦЕЛИКОМ, а каталог его в PATH не дописывается:
rem  у MSYS2 в том же каталоге лежат свои link.exe (утилита жёстких ссылок) и
rem  find.exe, и они перекрыли бы линковщик оснастки и системный find, которым
rem  выше определяется набор команд машины
rem ----------------------------------------------------------------------------
set "GIT="
for %%I in (git.exe) do if not "%%~$PATH:I"=="" set "GIT=%%~$PATH:I"
if not defined GIT for %%I in (git.exe) do if not "%%~$AWH_PATH:I"=="" set "GIT=%%~$AWH_PATH:I"

set "HAS_GIT=1"
if not defined GIT set "HAS_GIT=0"
if "%HAS_GIT%"=="0" echo [AWH] git not found: sources will be built as they lie on disk

echo.
echo [AWH] Visual Studio: %VSPATH%
echo [AWH] Architecture:  %AWH_ARCH%
echo [AWH] Prefix:        %PREFIX%
echo [AWH] Jobs:          %JOBS%
echo [AWH] CMake:         %CMAKE%
echo [AWH] Generator:     %GENERATOR%
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
rem  Перебор зависимостей
rem
rem  Перечень этот - тот же, что в build_third_party.sh, и порядок в нём не
rem  произволен: слияние (static) идёт ДО GPerfTools. Наоборот значило бы, что
rem  архивы подменного аллокатора попадают в общий архив зависимостей, а перехват
rem  выделений требует присоединения целым модулем - собранный образ тогда падал
rem  бы на первом же выделении.
rem
rem  Версии зависимостей записаны в их сценариях, а не в отдельном перечне:
rem  build_third_party.sh этой ветки задаёт их там же, и второй список рядом
rem  значил бы два источника правды, расходящихся с первой же правки
rem ----------------------------------------------------------------------------
for %%D in (a-nts zlib bz2 lz4 zstd lzma brotli pcre2 nghttp2 static gperftools) do (
	call :run "%%D"
	if errorlevel 1 exit /b 1
)

echo.
echo [AWH] Done
exit /b 0

rem ----------------------------------------------------------------------------
rem  Сборка одной зависимости
rem
rem  %1 - название зависимости
rem ----------------------------------------------------------------------------
:run
	set "SHORT=%~1"

	rem Заказ на одну зависимость отсеивает прочие
	if defined ONLY if /i not "%SHORT%"=="%ONLY%" exit /b 0

	rem ------------------------------------------------------------------------
	rem  Поиск сценария
	rem
	rem  Ищется по имени зависимости, а числовая приставка его при этом не
	rem  спрашивается: порядок сборки задаёт перечень выше, а приставка осталась
	rem  лишь затем, чтобы сценарии лежали в том же порядке и без перечня
	rem ------------------------------------------------------------------------
	set "SCRIPT="
	for /f "delims=" %%S in ('dir /b /on "%SCRIPTS%\*_%SHORT%.bat" 2^>nul') do set "SCRIPT=%%S"

	if not defined SCRIPT (
		echo [AWH] %SHORT%: no build script found in %SCRIPTS%
		exit /b 1
	)

	echo [AWH] ****** %SHORT% ******
	call "%SCRIPTS%\!SCRIPT!" %COMMAND%
	if errorlevel 1 (
		echo [AWH] FAILED: %SHORT%
		exit /b 1
	)
exit /b 0