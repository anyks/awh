@echo off
rem ============================================================================
rem  Объединение библиотеки AWH с её зависимостями оснасткой Visual Studio
rem
rem  Двойник sh/build_static_lib.sh для машин без MSYS2. Задача та же: потребитель
rem  присоединяет ОДНУ библиотеку, а не дюжину, и порядок их перечисления его не
rem  касается.
rem
rem  Средство у оснасток разное по существу, а не по имени. Там, где архиватору
rem  POSIX приходится разбирать чужие архивы на объектные файлы и собирать их
rem  заново - потому что он принимает лишь объектные файлы, - здесь `lib.exe`
rem  принимает архивы напрямую и сливает их сам. Оттого двойник этот КОРОЧЕ
rem  подлинника, а не длиннее: разбор с его ловушками тут просто не нужен.
rem
rem  Употребление: build_static_lib.bat <путь к собранной awh.lib>
rem ============================================================================

setlocal enabledelayedexpansion

rem ----------------------------------------------------------------------------
rem  Собранная библиотека, к какой присоединяются зависимости
rem
rem  Разделители пути приводятся к виду системы: CMake передаёт путь прямыми
rem  разделителями, и приёмы работы с файлами такой путь не принимают. Отказ при
rem  этом приходит в виде "не удаётся найти указанный путь" - о пути, который
rem  существует и лежит прямо перед глазами
rem ----------------------------------------------------------------------------
set "TARGET=%~1"
set "TARGET=%TARGET:/=\%"

if "%TARGET%"=="" (
	echo [AWH] build_static_lib: the path to the library is not given
	exit /b 1
)

if not exist "%TARGET%" (
	echo [AWH] build_static_lib: library "%TARGET%" does not exist
	exit /b 1
)

rem Каталог, где лежит это средство
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

rem Каталог собранных зависимостей
for %%I in ("%ROOT%\..\third_party-msvc\lib") do set "THIRD_PARTY=%%~fI"

if not exist "%THIRD_PARTY%" (
	echo [AWH] build_static_lib: dependencies are missing in %THIRD_PARTY%
	exit /b 1
)

rem ----------------------------------------------------------------------------
rem  Сбор перечня присоединяемых библиотек
rem
rem  Импортные библиотеки разделяемых (.dll.lib и спутники .dll) не берутся: они
rem  несут не код, а отсылки к нему, и слитые в архив дали бы потребителю
rem  зависимость от файла, какого рядом нет
rem ----------------------------------------------------------------------------
set "PARTS="
set "COUNT=0"

for %%L in ("%THIRD_PARTY%\*.lib") do (
	set "SKIP=0"
	rem Сама библиотека AWH объединению не подлежит
	if /i "%%~nxL"=="awh.lib" set "SKIP=1"
	rem Прежний итог объединения тоже: иначе он вошёл бы в себя же
	if /i "%%~nxL"=="dependence.lib" set "SKIP=1"
	if /i "%%~nxL"=="libdependence.lib" set "SKIP=1"
	rem Спутник разделяемой библиотеки опознаётся по соседнему файлу
	if exist "%THIRD_PARTY%\%%~nL.dll" set "SKIP=1"

	if "!SKIP!"=="0" (
		set "PARTS=!PARTS! "%%~fL""
		set /a COUNT+=1
	)
)

if "%COUNT%"=="0" (
	echo [AWH] build_static_lib: no dependencies to merge were found
	exit /b 1
)

rem ----------------------------------------------------------------------------
rem  Объединение
rem
rem  Итог пишется во ВРЕМЕННЫЙ файл и лишь затем занимает место цели: прерванный
rem  на полпути архиватор оставил бы рваный архив на месте годного, а счёт членов
rem  в нём при этом лжёт - беда, уже стоившая часа разбора у слияния под POSIX
rem ----------------------------------------------------------------------------
set "MERGED=%TARGET%.merged"

if exist "%MERGED%" del /f /q "%MERGED%" > nul 2>&1

lib.exe /nologo /out:"%MERGED%" "%TARGET%" %PARTS%
if errorlevel 1 (
	echo [AWH] build_static_lib: merging failed
	if exist "%MERGED%" del /f /q "%MERGED%" > nul 2>&1
	exit /b 1
)

move /y "%MERGED%" "%TARGET%" > nul
if errorlevel 1 (
	echo [AWH] build_static_lib: could not replace "%TARGET%"
	exit /b 1
)

echo [AWH] build_static_lib: merged %COUNT% dependencies into "%TARGET%"
exit /b 0
