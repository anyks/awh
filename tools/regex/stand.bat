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

rem Собираем обёртку вызова, приметы в оберегаемые регистры укладывающую
rem
rem Вставок на языке ассемблера связка вооружения Visual Studio не имеет вовсе,
rem отчего обёртка приходит отдельным файлом и переводится набором ml64 либо
rem armasm64 по разрядности цели. Отсутствие набора стенда не валит: проверка
rem сохранности отчитается пропуском, как и прежде
set "ASMSRC="
set "ASMTOOL="
if "%TOOLSET%" == "x64" (
	set "ASMSRC=tools\regex\preserving-x64.asm"
	set "ASMTOOL=ml64 /nologo /c /Fopreserving.obj"
)
if "%TOOLSET%" == "arm64" (
	set "ASMSRC=tools\regex\preserving-arm64.asm"
	set "ASMTOOL=armasm64 -nologo -o preserving.obj"
)
if "%TOOLSET%" == "x64_arm64" (
	set "ASMSRC=tools\regex\preserving-arm64.asm"
	set "ASMTOOL=armasm64 -nologo -o preserving.obj"
)

set "PRESERVING="
set "DEFINES="
if not "%ASMSRC%" == "" (
	%ASMTOOL% %ASMSRC% >> compile.log 2>&1
	if errorlevel 1 (
		echo Обёртка сохранности регистров не собрана, проверка её будет пропущена
	) else (
		set "PRESERVING=preserving.obj"
		set "DEFINES=/DAWH_PRESERVING_EXTERN"
	)
)

rem Сборка ведётся набором cl: он единственный доступен на всякой машине
rem Windows без установки постороннего, а переносимой проверке кроме
rem компилятора C++ ничего и не нужно
rem
rem Файлы переводятся по одному, а не единым вызовом: cl кладёт объектные
rem файлы в текущий каталог по имени исходного, и два файла «table.cpp» -
rem из src\regex и из src\encoding\unicode - дают один «table.obj».
rem Единый вызов оттого затирал таблицы Юникода и валил связывание
rem тридцатью тремя неразрешёнными именами, а предупреждение LNK4042
rem об этом терялось в выводе. Имя объектного файла потому нумеруется.
set "OBJECTS="
set /a NUMBER=0
for %%F in (tools\regex\conformance.cpp %SOURCES%) do (
	set /a NUMBER+=1
	call :COMPILE "%%F"
	if errorlevel 1 (
		echo Сборка не выполнена:
		type compile.log
		exit /b 4
	)
)

cl /nologo /Fe:conformance.exe %OBJECTS% %PRESERVING% >> compile.log 2>&1

if errorlevel 1 (
	echo Связывание не выполнено:
	type compile.log
	exit /b 4
)

call :RUN
exit /b %errorlevel%

rem Перевод одного файла исходного текста в объектный файл с нумерованным именем
rem
rem Имя объектного файла берётся номером, а не именем исходного: имена
rem исходных файлов по дереву повторяются, и совпадение их молча теряло
rem перевод одного из них
:COMPILE
cl /nologo /EHsc /std:c++17 /O2 /Iinclude /Itools/regex %DEFINES% /c ^
	/Fostand%NUMBER%.obj %~1 >> compile.log 2>&1
if errorlevel 1 exit /b 1
set "OBJECTS=%OBJECTS% stand%NUMBER%.obj"
exit /b 0

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
