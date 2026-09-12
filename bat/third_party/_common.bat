@echo off
rem ============================================================================
rem  Общая часть сборки зависимостей оснасткой Visual Studio
rem
rem  Зависимости различаются немногим: названием, версией и набором настроек CMake.
rem  Всё остальное - проверка исходников, перевод их на нужную версию, настройка,
rem  сборка, установка, отметка о сделанном - у них одно и то же, и держать его в
rem  десяти списаниях значило бы обречь их расходиться с первой же правки.
rem
rem  Зовётся из сценария зависимости так:
rem
rem    call "%~dp0_common.bat" :version "<запись>" "<приставка>" "<хвост>" "<умолчание>"
rem    call "%~dp0_common.bat" :checkout "<имя>"
rem    call "%~dp0_common.bat" :build "<имя>" "<версия>" "<настройки CMake>"
rem    call "%~dp0_common.bat" :clean "<имя>"
rem
rem  Ожидает заведённого окружения: PREFIX, SUBMODULES, CMAKE, NINJA, JOBS.
rem ============================================================================

if "%~1"=="" exit /b 1
call %1 %2 %3 %4 %5 %6
exit /b %ERRORLEVEL%

rem ----------------------------------------------------------------------------
rem  Разбор записи о версии зависимости
rem
rem  Запись приходит из Requirements.txt в том же виде, в каком её понимает сборка
rem  под MinGW: три знака признака и следом значение - --v1.3.1, --bmaster, --tX,
rem  --c<снимок>, --n. Признак этот решает, ЧЕМ названное является, и обращаться с
rem  веткой как с меткой нельзя: ветка движется, а метка стоит.
rem
rem  Вид метки у каждой зависимости свой: одна метит v1.3.1, другая 1.2.1, третья
rem  bzip2-1.0.8. Потому приставка и хвост задаются самой зависимостью, а не здесь -
rem  общим здесь остаётся только разбор записи.
rem
rem  %1 - запись, %2 - приставка метки, %3 - хвост метки, %4 - версия по умолчанию
rem
rem  Выдаёт AWH_REF (на что переходить; пусто - оставить как лежит) и AWH_MODE
rem ----------------------------------------------------------------------------
:version
	set "AWH_SPEC=%~1"
	set "AWH_MODE=version"
	set "AWH_VALUE=%~4"

	if not "%AWH_SPEC%"=="" (
		set "AWH_FLAG=!AWH_SPEC:~0,3!"
		set "AWH_VALUE=!AWH_SPEC:~3!"

		if "!AWH_FLAG!"=="--n" ( set "AWH_MODE=none" ) else (
		if "!AWH_FLAG!"=="--b" ( set "AWH_MODE=branch" ) else (
		if "!AWH_FLAG!"=="--t" ( set "AWH_MODE=tag" ) else (
		if "!AWH_FLAG!"=="--c" ( set "AWH_MODE=commit" ) else (
		if "!AWH_FLAG!"=="--v" ( set "AWH_MODE=version" ) else (
			echo [AWH] Flag "!AWH_FLAG!" is not supported
			exit /b 1
		)))))
	)

	rem Метка собирается из приставки и хвоста самой зависимости, прочее берётся как есть
	if "%AWH_MODE%"=="version" ( set "AWH_REF=%~2!AWH_VALUE!%~3" ) else (
	if "%AWH_MODE%"=="none"    ( set "AWH_REF=" ) else (
		set "AWH_REF=!AWH_VALUE!"
	))
exit /b 0

rem ----------------------------------------------------------------------------
rem  Перевод исходников на названную версию
rem
rem  Работает лишь тогда, когда есть чем и когда версия названа: дерево, взятое
rem  архивом, хранилища не несёт вовсе, а собрать лежащее на диске можно и без него.
rem
rem  Ветка тянется с хранилища, метка берётся из выкачанных заново меток, а снимок
rem  ставится жёстко: у ветки и метки может совпасть имя, и без разделения этого
rem  переход уходил бы не туда молча
rem ----------------------------------------------------------------------------
:checkout
	if not "%HAS_GIT%"=="1" exit /b 0
	if "%AWH_REF%"=="" exit /b 0

	pushd "%SUBMODULES%\%~1" > nul

	set "AWH_DONE=0"

	if "%AWH_MODE%"=="branch" (
		git fetch origin "%AWH_REF%" > nul 2>&1
		git checkout "%AWH_REF%" > nul 2>&1 && (
			git pull origin "%AWH_REF%" > nul 2>&1
			set "AWH_DONE=1"
		)
	) else (
	if "%AWH_MODE%"=="commit" (
		git fetch --all > nul 2>&1
		git reset --hard "%AWH_REF%" > nul 2>&1 && set "AWH_DONE=1"
	) else (
		git fetch --all --tags > nul 2>&1
		git checkout "%AWH_REF%" > nul 2>&1 && set "AWH_DONE=1"
	))

	rem Молчать об отказе перехода нельзя: собранное иначе выдало бы чужую версию за названную
	if "!AWH_DONE!"=="0" echo [AWH] %~1: staying on the current revision, "%AWH_REF%" is unavailable

	popd > nul
exit /b 0

rem ----------------------------------------------------------------------------
rem  Снос собранного
rem
rem  Исходники не трогаются: у потребителя может не быть выхода в сеть, и выкачать
rem  их заново было бы нечем
rem ----------------------------------------------------------------------------
:clean
	set "NAME=%~1"
	set "SRC=%SUBMODULES%\%NAME%"
	if exist "%SRC%\.stamp_done_msvc" del /f /q "%SRC%\.stamp_done_msvc" > nul 2>&1
	if exist "%SRC%\build-msvc" rmdir /s /q "%SRC%\build-msvc" > nul 2>&1
	if exist "%PREFIX%\include\%NAME%" rmdir /s /q "%PREFIX%\include\%NAME%" > nul 2>&1
exit /b 0

rem ----------------------------------------------------------------------------
rem  Сборка зависимости
rem
rem  %1 - название подмодуля, %2 - версия (тег без приставки), %3 - настройки CMake
rem ----------------------------------------------------------------------------
:build
	set "NAME=%~1"
	set "VERSION=%~2"
	set "OPTIONS=%~3"
	rem Каталог описания сборки внутри исходников: у части зависимостей он не корневой
	set "SUBDIR=%~4"

	set "SRC=%SUBMODULES%\%NAME%"
	set "STAMP=%SRC%\.stamp_done_msvc"

	rem Каталог, где лежит описание сборки
	if "%SUBDIR%"=="" ( set "CMAKEDIR=%SRC%" ) else ( set "CMAKEDIR=%SRC%\%SUBDIR%" )

	rem ------------------------------------------------------------------------
	rem  Отметка о сделанном
	rem
	rem  В ней записана та версия, какой зависимость собрана. Сличение с нею, а не
	rem  одно лишь наличие отметки, нужно ради Requirements.txt: смена версии там
	rem  иначе не привела бы ни к чему - собранное прежде считалось бы годным, и
	rem  сборка молча осталась бы на старой версии
	rem ------------------------------------------------------------------------
	set "STAMPED="
	if exist "%STAMP%" for /f "usebackq delims=" %%V in ("%STAMP%") do set "STAMPED=%%V"

	if exist "%STAMP%" (
		if "!STAMPED!"=="%VERSION%" (
			echo [AWH] %NAME%: already built
			exit /b 0
		)
		echo [AWH] %NAME%: rebuilding, version changed "!STAMPED!" -^> "%VERSION%"
		if exist "%SRC%\build-msvc" rmdir /s /q "%SRC%\build-msvc" > nul 2>&1
	)

	rem ------------------------------------------------------------------------
	rem  Проверка исходников
	rem
	rem  Пустой каталог подмодуля - обычное дело у свежего клона: без --recursive
	rem  подмодули не выкачиваются вовсе. Сказать об этом надо прямо, а не отказом
	rem  CMake о ненайденном CMakeLists.txt
	rem ------------------------------------------------------------------------
	if not exist "%CMAKEDIR%\CMakeLists.txt" (
		echo [AWH] %NAME%: sources are missing in %SRC%
		echo [AWH] Run: git submodule update --init --recursive
		exit /b 1
	)

	rem Переводим исходники на нужную версию
	set "AWH_REF=%VERSION%"
	call :checkout "%NAME%"

	rem ------------------------------------------------------------------------
	rem  Настройка сборки
	rem
	rem  Каталог сборки свой, отдельный от MinGW: кэш CMake хранит путь к оснастке,
	rem  и общий каталог означал бы, что каждая смена оснастки требует его сноса
	rem ------------------------------------------------------------------------
	if not exist "%SRC%\build-msvc" mkdir "%SRC%\build-msvc" > nul 2>&1
	pushd "%SRC%\build-msvc" > nul

	"%CMAKE%" -G Ninja ^
	 -DCMAKE_MAKE_PROGRAM="%NINJA%" ^
	 -DCMAKE_C_COMPILER=cl.exe ^
	 -DCMAKE_CXX_COMPILER=cl.exe ^
	 -DCMAKE_BUILD_TYPE=Release ^
	 -DCMAKE_INSTALL_PREFIX="%PREFIX%" ^
	 -DBUILD_SHARED_LIBS=NO ^
	 %OPTIONS% ^
	 "%CMAKEDIR%" || (
		echo [AWH] %NAME%: configure failed
		popd > nul
		exit /b 1
	)

	rem Выполняем сборку на всех вычислителях машины
	"%CMAKE%" --build . --parallel %JOBS% || (
		echo [AWH] %NAME%: build failed
		popd > nul
		exit /b 1
	)

	rem Выполняем установку собранного
	"%CMAKE%" --install . || (
		echo [AWH] %NAME%: install failed
		popd > nul
		exit /b 1
	)

	popd > nul

	rem Отметка пишется без перевода строки: с нею сличают, а лишний знак сличение ломает
	<nul set /p ="%VERSION%" > "%STAMP%"
	echo [AWH] %NAME%: done
exit /b 0
