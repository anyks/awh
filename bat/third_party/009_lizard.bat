@echo off
rem ============================================================================
rem  Lizard для оснастки Microsoft Visual Studio
rem
rem  Собирается напрямую, а не через CMake: описания сборки для него у зависимости
rem  нет вовсе - есть лишь Makefile для GNU make, которого на машине с Visual
rem  Studio может не быть. Библиотека невелика и укладывается в один перевод всех
rem  своих исходников с последующим сбором в архив.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=lizard"
set "SRC=%SUBMODULES%\%NAME%"
set "STAMP=%SRC%\.stamp_done_msvc"
set "WORK=%SRC%\build-msvc"

rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Задаётся она Requirements.txt в корне дерева, а записанное здесь берётся лишь
rem  тогда, когда записи в нём нет: сборка обязана работать и с деревом, взятым
rem  архивом без этого файла.
rem
rem  Вид метки у этой зависимости свой, потому приставка и хвост названы здесь, а не
rem  в общей части: из версии 1.0 выходит метка v1.0
rem ----------------------------------------------------------------------------
set "VPREFIX=v"
set "VSUFFIX="
set "VDEFAULT=1.0"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	if exist "%STAMP%" del /f /q "%STAMP%" > nul 2>&1
	if exist "%WORK%" rmdir /s /q "%WORK%" > nul 2>&1
	if exist "%PREFIX%\lib\lizard.lib" del /f /q "%PREFIX%\lib\lizard.lib" > nul 2>&1
	if exist "%PREFIX%\include\%NAME%" rmdir /s /q "%PREFIX%\include\%NAME%" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном
if /i "%~1"=="--rebuild" (
	if exist "%STAMP%" del /f /q "%STAMP%" > nul 2>&1
)

rem ----------------------------------------------------------------------------
rem  Отметка о сделанном
rem
rem  В ней записана та версия, какой зависимость собрана. Сличение с нею, а не одно
rem  лишь наличие отметки, нужно ради Requirements.txt: смена версии там иначе не
rem  привела бы ни к чему - собранное прежде считалось бы годным
rem ----------------------------------------------------------------------------
set "STAMPED="
if exist "%STAMP%" for /f "usebackq delims=" %%V in ("%STAMP%") do set "STAMPED=%%V"

if exist "%STAMP%" (
	if "!STAMPED!"=="%VERSION%" (
		echo [AWH] %NAME%: already built
		exit /b 0
	)
	echo [AWH] %NAME%: rebuilding, version changed "!STAMPED!" -^> "%VERSION%"
	if exist "%WORK%" rmdir /s /q "%WORK%" > nul 2>&1
)

if not exist "%SRC%\lib" (
	echo [AWH] %NAME%: sources are missing in %SRC%
	echo [AWH] Run: git submodule update --init --recursive
	exit /b 1
)

rem Переводим исходники на нужную версию
call "%~dp0_common.bat" :checkout "%NAME%"

if not exist "%WORK%" mkdir "%WORK%" > nul 2>&1
pushd "%WORK%" > nul

rem ----------------------------------------------------------------------------
rem  Перевод исходников
rem
rem  Правила выдачи предупреждений у чужой библиотеки свои, и ужесточать их мы не
rem  вправе: сборка зависимости не должна валиться от того, что НАШИ мерки строже
rem  её собственных
rem ----------------------------------------------------------------------------
set "OBJECTS="
set "COUNT=0"

for /r "%SRC%\lib" %%C in (*.c) do (
	cl.exe /nologo /c /O2 /DNDEBUG /W0 /I "%SRC%\lib" /I "%SRC%\lib\entropy" /I "%SRC%\lib\xxhash" /Fo:"%WORK%\!COUNT!_%%~nC.obj" "%%~fC" > nul || (
		echo [AWH] %NAME%: failed to compile %%~nxC
		popd > nul
		exit /b 1
	)
	set "OBJECTS=!OBJECTS! "%WORK%\!COUNT!_%%~nC.obj""
	set /a COUNT+=1
)

if "%COUNT%"=="0" (
	echo [AWH] %NAME%: no sources were found
	popd > nul
	exit /b 1
)

rem Собираем переведённое в архив
lib.exe /nologo /out:"%PREFIX%\lib\lizard.lib" %OBJECTS% || (
	echo [AWH] %NAME%: archiving failed
	popd > nul
	exit /b 1
)

popd > nul

rem ----------------------------------------------------------------------------
rem  Раскладка заголовков
rem
rem  Кладутся с сохранением вложенности: заголовки зависимости ссылаются друг на
rem  друга относительными путями, и уложенные россыпью они друг друга не найдут
rem ----------------------------------------------------------------------------
if not exist "%PREFIX%\include\%NAME%" mkdir "%PREFIX%\include\%NAME%" > nul 2>&1
xcopy /s /y /q "%SRC%\lib\*.h" "%PREFIX%\include\%NAME%\" > nul

rem Помечаем сборку выполненной
rem Отметка пишется без перевода строки: с нею сличают, а лишний знак сличение ломает
<nul set /p ="%VERSION%" > "%STAMP%"
echo [AWH] %NAME%: done
exit /b 0
