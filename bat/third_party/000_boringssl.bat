@echo off
rem ============================================================================
rem  BORINGSSL для оснастки Microsoft Visual Studio
rem
rem  Зовётся из build_third_party.bat. Общая часть сборки живёт в _common.bat,
rem  здесь остаётся лишь то, чем эта зависимость отличается от прочих.
rem
rem  Заголовки эта зависимость кладёт своим каталогом (include\openssl), и раскладки
rem  ей не требуется - в отличие от тех, что ставят их россыпью.
rem ============================================================================

setlocal enabledelayedexpansion

set "NAME=boringssl"
rem ----------------------------------------------------------------------------
rem  Версия зависимости
rem
rem  Задаётся она Requirements.txt в корне дерева, а записанное здесь берётся лишь
rem  тогда, когда записи в нём нет: сборка обязана работать и с деревом, взятым
rem  архивом без этого файла.
rem
rem  Вид метки у этой зависимости свой, потому приставка и хвост названы здесь, а не
rem  в общей части: из версии 20260413.0 выходит метка 0.20260413.0
rem ----------------------------------------------------------------------------
set "VPREFIX=0."
set "VSUFFIX="
set "VDEFAULT=20260413.0"

call "%~dp0_common.bat" :version "%~2" "%VPREFIX%" "%VSUFFIX%" "%VDEFAULT%"
if errorlevel 1 exit /b 1

set "VERSION=%AWH_REF%"

rem Снос собранного
if /i "%~1"=="--clean" (
	call "%~dp0_common.bat" :clean "%NAME%"
	if exist "%PREFIX%\lib\boringssl.lib" del /f /q "%PREFIX%\lib\boringssl.lib" > nul 2>&1
	exit /b 0
)

rem Пересборка снимает отметку о сделанном, исходники не трогая
if /i "%~1"=="--rebuild" (
	if exist "%SUBMODULES%\%NAME%\.stamp_done_msvc" del /f /q "%SUBMODULES%\%NAME%\.stamp_done_msvc" > nul 2>&1
)

rem Выполняем сборку
rem ----------------------------------------------------------------------------
rem  Настройки сборки
rem
rem  Библиотека времени выполнения задаётся ФЛАГОМ, а не настройкой CMake. Настройка
rem  накладывается на ВСЕ языки перевода, а часть кода этой зависимости написана на
rem  языке машины - переводчик его такой настройки не знает вовсе и отвечает отказом
rem  "MSVC_RUNTIME_LIBRARY value not known for this ASM compiler".
rem
rem  Молчать же об этом нельзя: оставь настройку пустой - и зависимость возьмёт
rem  НЕразделяемую библиотеку времени выполнения, тогда как сам AWH берёт
rem  разделяемую. Смешение их в одном образе сборку проходит молча, а рушится
rem  потом, на выдаче памяти - и причину ищут не там
rem ----------------------------------------------------------------------------
set "BORINGSSL_OPTIONS=-DBUILD_TESTING=OFF -DCMAKE_MSVC_RUNTIME_LIBRARY= -DCMAKE_C_FLAGS=/MD -DCMAKE_CXX_FLAGS=/MD"

rem ----------------------------------------------------------------------------
rem  Код на языке машины
rem
rem  Ускоренные части этой зависимости написаны в записи GNU. Переводчик MSVC такой
rem  записи не понимает вовсе, а `armasm64` из его же состава принимает лишь свою.
rem  Годится сюда только clang-cl - им эту зависимость под MS Windows и собирают.
rem
rem  Есть он не всегда: в установщике Visual Studio это отдельная часть ("Средства
rem  C++ Clang для Windows"). Нет его - ускоренные части отключаются, и шифрование
rem  идёт переносимым кодом на C. Библиотека при этом РАБОТАЕТ, но медленнее, и
rem  молчать об этом нельзя: потеря скорости, о какой не сказано, потом
rem  разыскивается замерами как чужая беда
rem ----------------------------------------------------------------------------
set "CLANGCL=%VSPATH%\VC\Tools\Llvm\bin\clang-cl.exe"
if not exist "%CLANGCL%" set "CLANGCL=%VSPATH%\VC\Tools\Llvm\x64\bin\clang-cl.exe"

if exist "%CLANGCL%" (
	set "BORINGSSL_OPTIONS=%BORINGSSL_OPTIONS% -DCMAKE_ASM_COMPILER=\"%CLANGCL%\""
) else (
	echo [AWH] boringssl: clang-cl not found, building without assembly optimizations
	echo [AWH] boringssl: install the "C++ Clang tools for Windows" component to enable them
	set "BORINGSSL_OPTIONS=%BORINGSSL_OPTIONS% -DOPENSSL_NO_ASM=1"
)

call "%~dp0_common.bat" :build "%NAME%" "%VERSION%" "%BORINGSSL_OPTIONS%"
if errorlevel 1 exit /b 1

rem ----------------------------------------------------------------------------
rem  Архив устаревших приёмов
rem
rem  Установка его не кладёт: зависимость считает эти приёмы отжившими и держит
rem  отдельной целью. AWH же ими пользуется - это старые способы шифрования, какие
rem  договор обязан понимать, встретив их у собеседника.
rem
rem  Пропажа эта неразделяемой сборке ничем себя не выдаёт: связывания там нет вовсе,
rem  и архив выходит годным на вид. Отказ приходит потом, при связывании РАЗДЕЛЯЕМОЙ
rem  библиотеки либо чужой программы, именами вида EVP_aes_256_cfb128
rem ----------------------------------------------------------------------------
if exist "%SUBMODULES%\%NAME%\build-msvc\decrepit.lib" (
	copy /y "%SUBMODULES%\%NAME%\build-msvc\decrepit.lib" "%PREFIX%\lib\" > nul || (
		echo [AWH] %NAME%: could not install decrepit.lib
		exit /b 1
	)
)

exit /b 0
