@echo off

setlocal

:: "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

:: Получаем корневую директорию
set ROOT=%~dp0
set PREFIX=%ROOT%\third_party

:: Создаем директорию установки
mkdir %PREFIX%

git submodule update --init --recursive --remote

:: *******************************
:: 		Сборка OpenSSL
:: *******************************
echo ****** OpenSSL ******

set src=%ROOT%submodules\openssl
cd /D %src%

:: Выполняем конфигурацию проекта
perl Configure VC-WIN64A no-shared no-async --prefix=%PREFIX% --openssldir=%PREFIX%

:: Выполняем сборку и установку
namke
nmake install

cd /D %ROOT%

:: *******************************
:: 		Сборка ZLib
:: *******************************
echo ****** ZLib ******

set src=%ROOT%submodules\zlib
cd /D %src%

:: Создаём каталог сборки
mkdir build
cd build

:: Выполняем конфигурацию проекта
cmake -G "Visual Studio 16 2019" -A x64 ^
	-DCMAKE_BUILD_TYPE=Release ^
	-DCMAKE_INSTALL_PREFIX=%PREFIX% ^
	-DINSTALL_INC_DIR=%PREFIX%\include\zlib ^
	-DBUILD_SHARED_LIBS=NO ^
	..
:: Выполняем сборку и установку
cmake --build . --config Release --target install

cd /D %ROOT%

:: *******************************
:: 		Сборка Brotli
:: *******************************
echo ****** Brotli ******

set src=%ROOT%submodules\brotli
cd /D %src%

:: Создаём каталог сборки
mkdir out
cd out

:: Выполняем конфигурацию проекта
cmake -G "Visual Studio 16 2019" -A x64 ^
	-DCMAKE_BUILD_TYPE=Release ^
	-DCMAKE_INSTALL_PREFIX=%PREFIX% ^
	-DBROTLI_INCLUDE_DIRS=%PREFIX%\include ^
	-DBROTLI_LIBRARIES=%PREFIX%\lib ^
	-DBUILD_SHARED_LIBS=NO ^
	-DBROTLI_BUNDLED_MODE=NO ^
	-DBROTLI_EMSCRIPTEN=YES ^
	-DBROTLI_DISABLE_TESTS=YES ^
	..
:: Выполняем сборку и установку
cmake --build . --config Release --target install

cd /D %ROOT%

:: *******************************
::  	Сборка LibEvent2
:: *******************************
echo ****** LibEvent2 ******

set src=%ROOT%submodules\libevent
cd /D %src%

:: Создаём каталог сборки
mkdir build
cd build

:: Выполняем конфигурацию проекта
cmake -G "Visual Studio 16 2019" -A x64 ^
	 -DEVENT__LIBRARY_TYPE="STATIC" ^
	 -DEVENT__DISABLE_DEBUG_MODE="ON" ^
	 -DEVENT__DISABLE_BENCHMARK="ON" ^
	 -DEVENT__DISABLE_SAMPLES="ON" ^
	 -DEVENT__DISABLE_TESTS="ON" ^
	 -DEVENT__DISABLE_MBEDTLS="ON" ^
	 -DEVENT__DISABLE_THREAD_SUPPORT="ON" ^
	 -DCMAKE_INSTALL_PREFIX=%PREFIX% ^
	 -DOPENSSL_ROOT_DIR=%PREFIX% ^
	 -DOPENSSL_LIBRARIES=%PREFIX%\lib ^
	 -DOPENSSL_INCLUDE_DIR=%PREFIX%\include ^
	 ..
:: Выполняем сборку и установку
cmake --build . --target install

cd /D %ROOT%

echo.
echo ****************************************
echo ************   Success!!!   ************
echo ****************************************
echo.
