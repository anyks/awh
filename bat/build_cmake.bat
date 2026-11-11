@echo off
rem ============================================================================
rem  Создание файла поиска AWH для CMake оснасткой Visual Studio
rem
rem  Двойник build_cmake.sh для машин без MSYS2. Задача та же: положить образец
rem  FindAWH.cmake из дерева в набор зависимостей MSVC, откуда его забирает
rem  установка. Подставлять место установки в образец больше не нужно: модуль
rem  вычисляет его от собственного каталога, и копия выходит дословной.
rem
rem  Образец у этой ветки лежит в contrib\cmake, а не в cmake: каталог cmake
rem  хранит средства поиска самого проекта (FindDependence, FindTcMalloc), а
rem  потребителю отдаётся другой файл - тот, что ищет уже собранную AWH.
rem
rem  Употребление: build_cmake.bat <каталог установки>
rem ============================================================================

setlocal enabledelayedexpansion

rem Каталог, куда ставится библиотека
set "DESTINATION=%~1"

if "%DESTINATION%"=="" (
	echo [AWH] build_cmake: destination address is not specified
	exit /b 1
)

rem Каталог, где лежит это средство
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

rem Название создаваемого файла
set "NAME=FindAWH.cmake"

rem Образец, из какого он создаётся
for %%I in ("%ROOT%\..\contrib\cmake\%NAME%") do set "SOURCE=%%~fI"

rem ----------------------------------------------------------------------------
rem  Место, куда он кладётся
rem
rem  Каталог свой, отдельный от MinGW: наборы зависимостей у двух оснасток
rem  разные (third_party и third_party-msvc), и установка берёт модуль из того,
rem  который собран. Путей к собранному сам модуль больше не называет
rem ----------------------------------------------------------------------------
for %%I in ("%ROOT%\..\third_party-msvc\cmake") do set "TARGET_DIR=%%~fI"
set "TARGET=%TARGET_DIR%\%NAME%"

if not exist "%SOURCE%" (
	echo [AWH] build_cmake: the sample "%SOURCE%" is missing
	exit /b 1
)

if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%" > nul 2>&1

rem ----------------------------------------------------------------------------
rem  Перенос образца в набор зависимостей
rem
rem  Средства построчной правки у MS Windows нет, и перенос ведёт оболочка
rem  PowerShell - она есть у всякой поддерживаемой редакции системы. Разделители
rem  пути заменяются на прямые: CMake принимает их на всех системах, а обратные
rem  в его строках читаются как начало управляющей последовательности.
rem
rem  Подстановка места установки сохранена прежней, но образца в тексте больше
rem  нет: модуль вычисляет место от себя. Аргумент принимается по-прежнему - его
rem  передаёт сборка, и снимать его значило бы сломать зовущие. Выходит копия
rem ----------------------------------------------------------------------------
set "DESTINATION=%DESTINATION:\=/%"
set "SOURCE=%SOURCE:/=\%"
set "TARGET=%TARGET:/=\%"
set "TARGET_DIR=%TARGET_DIR:/=\%"

rem Зовётся он полным именем: в пути его может не быть вовсе, а отказ об этом
rem приходит как о ненайденной команде - неотличимо от отсутствия самой оболочки
set "POWERSHELL=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%POWERSHELL%" set "POWERSHELL=powershell"

"%POWERSHELL%" -NoProfile -ExecutionPolicy Bypass -Command ^
  "$text = Get-Content -Raw -LiteralPath '%SOURCE%';" ^
  "$text = $text -replace [regex]::Escape('${CMAKE_SOURCE_DIR}/third_party'), '%DESTINATION%';" ^
  "Set-Content -NoNewline -LiteralPath '%TARGET%' -Value $text"

if errorlevel 1 (
	echo [AWH] build_cmake: failed to create "%TARGET%"
	exit /b 1
)

echo [AWH] build_cmake: created "%TARGET%"
exit /b 0