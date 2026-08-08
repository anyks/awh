@echo off
rem Сборка и прогон переносимой проверки модуля регулярных выражений на Windows.
rem
rem Разрядность набора команд задаётся первым доводом в виде связки
rem вооружения Visual Studio: x64 (сборка своя), x64_arm64 (сборка встречная
rem с AMD64 на ARM64), arm64 (сборка своя на машине ARM64). При доводе
rem опущенном берётся сборка своя.
rem
rem Скрипт вызывается стендом tools/regex/stand-windows.sh, но годен и к запуску
rem напрямую из каталога, куда разложен набор исходных текстов.

setlocal

set TOOLSET=%1
if "%TOOLSET%" == "" set TOOLSET=x64

rem Отыскиваем вооружение Visual Studio
set VCVARS=
for %%E in (Community Professional Enterprise BuildTools) do (
	if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
		if "%VCVARS%" == "" set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat
	)
)

rem Ветвь сборки сборщиком MinGW-w64 уводится меткой, а не пишется скобками: cmd раскрывает переменные
rem во всей скобочной ветви разом до исполнения её, отчего перечень исходных
rem текстов, собранный внутри ветви, в строку сборки не попал бы вовсе
if "%VCVARS%" == "" goto MINGW

call "%VCVARS%" %TOOLSET% >nul
if errorlevel 1 (
	echo Связка вооружения "%TOOLSET%" недоступна
	exit /b 3
)

rem Переходим в корень разложенного набора: пакетный файл лежит в
rem tools\regex, а пути сборки отсчитываются от корня
cd /d "%~dp0..\.."

call :SOURCES

rem Сборка ведётся набором cl: он единственный доступен на всякой машине
rem Windows без установки постороннего, а переносимой проверке кроме
rem компилятора C++ ничего и не нужно
cl /nologo /EHsc /std:c++17 /O2 /Iinclude /Itools/regex ^
	/Fe:conformance.exe ^
	tools\regex\conformance.cpp %SOURCES% > compile.log 2>&1

if errorlevel 1 (
	echo Сборка не выполнена:
	type compile.log
	exit /b 4
)

call :RUN
exit /b %errorlevel%

rem Сборка сборщиком MinGW-w64
rem
rem Вооружения Visual Studio на машине может не быть вовсе, тогда как сборщик
rem MinGW-w64 приходит вместе со Strawberry Perl и с Git for Windows, отчего
rem встречается чаще. Переносимой проверке кроме компилятора C++ ничего
rem не нужно, поэтому при отсутствии Visual Studio берётся он
:MINGW
where g++ >nul 2>&1
if errorlevel 1 (
	if exist "C:\Strawberry\c\bin\g++.exe" (
		set "PATH=C:\Strawberry\c\bin;%PATH%"
	) else (
		echo Ни вооружения Visual Studio 2022, ни сборщика MinGW-w64 не обнаружено
		exit /b 3
	)
)
cd /d "%~dp0..\.."
echo Сборка ведётся сборщиком MinGW-w64
call :SOURCES
g++ -std=c++17 -O2 -Iinclude -Itools/regex -o conformance.exe tools\regex\conformance.cpp %SOURCES% > compile.log 2>&1
if errorlevel 1 (
	echo Сборка не выполнена:
	type compile.log
	exit /b 4
)
call :RUN
exit /b %errorlevel%

rem Сбор перечня собираемых исходных текстов
rem
rem Перечень выводится стендом tools/regex/stand-windows.sh и кладётся рядом
rem с исходными текстами. Разбирать состав поиском здесь нельзя: дерево
rem src\regex несёт подкаталог grok, модулю выражений подчинённый, однако
rem переносимой проверке не нужный и связанный с прочими частями библиотеки
:SOURCES
set SOURCES=
for /f "usebackq delims=" %%F in ("tools\regex\sources.list") do call set "SOURCES=%%SOURCES%% %%F"
exit /b 0

rem Прогон переносимой проверки
rem
rem Запись хранилища рабочей машины кладётся стендом рядом с исходными
rem текстами, и при наличии её проверка ведётся с нею: стенду надлежит
rem либо восстановить запись в точности, либо отвергнуть её опознанием
rem устройства машины, но не прочесть неверно. Запись эта передаётся не
rem всегда - на рабочей машине может не оказаться компилятора, - потому
rem наличие её проверяется, а не полагается
:RUN
if exist tools\regex\record.bin (
	conformance.exe --read=tools/regex/record.bin
) else (
	conformance.exe
)
exit /b %errorlevel%
