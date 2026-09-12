@echo off
rem ============================================================================
rem  Слияние зависимостей в один архив
rem
rem  Шаг последний: собранные порознь зависимости сливаются в libdependence.lib -
rem  ровно то имя, какое ищет cmake/FindDependence.cmake. Сборке AWH нужен ОДИН
rem  архив, а не дюжина, и порядок их перечисления её не касается.
rem
rem  Двойник sh/third_party/010_static.sh. Он короче подлинника: архиватору POSIX
rem  приходится разбирать чужие архивы на объектные файлы и собирать заново, а
rem  `lib.exe` принимает архивы напрямую.
rem ============================================================================

setlocal enabledelayedexpansion

set "TARGET=%PREFIX%\lib\libdependence.lib"

rem Снос собранного
if /i "%~1"=="--clean" (
	if exist "%TARGET%" del /f /q "%TARGET%" > nul 2>&1
	exit /b 0
)

rem ----------------------------------------------------------------------------
rem  Сбор перечня сливаемых библиотек
rem
rem  Прежний итог слияния берётся в перечень НЕ будет: иначе он вошёл бы в себя же
rem  и рос бы с каждым прогоном
rem ----------------------------------------------------------------------------
set "PARTS="
set "COUNT=0"

for %%L in ("%PREFIX%\lib\*.lib") do (
	set "SKIP=0"
	if /i "%%~nxL"=="libdependence.lib" set "SKIP=1"
	rem Спутник разделяемой библиотеки опознаётся по соседнему файлу: он несёт не
	rem код, а отсылки к нему, и слитый в архив дал бы зависимость от файла рядом
	if exist "%PREFIX%\lib\%%~nL.dll" set "SKIP=1"
	if exist "%PREFIX%\bin\%%~nL.dll" set "SKIP=1"

	if "!SKIP!"=="0" (
		set "PARTS=!PARTS! "%%~fL""
		set /a COUNT+=1
	)
)

if "%COUNT%"=="0" (
	echo [AWH] static: no dependencies to merge were found
	exit /b 1
)

rem ----------------------------------------------------------------------------
rem  Слияние
rem
rem  Итог пишется во ВРЕМЕННЫЙ файл и лишь затем занимает место цели: прерванный
rem  на полпути архиватор оставил бы рваный архив на месте годного, а счёт членов
rem  в нём при этом лжёт
rem ----------------------------------------------------------------------------
set "MERGED=%TARGET%.merged"
if exist "%MERGED%" del /f /q "%MERGED%" > nul 2>&1

lib.exe /nologo /out:"%MERGED%" %PARTS% || (
	echo [AWH] static: merging failed
	if exist "%MERGED%" del /f /q "%MERGED%" > nul 2>&1
	exit /b 1
)

move /y "%MERGED%" "%TARGET%" > nul || (
	echo [AWH] static: could not replace "%TARGET%"
	exit /b 1
)

echo [AWH] static: merged %COUNT% libraries into libdependence.lib
exit /b 0
