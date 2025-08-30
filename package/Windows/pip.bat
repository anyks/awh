@echo off

setlocal

:: Module name
set MODULE_NAME=GLB

:: Getting the application directory
set PATH_NAME=%ProgramFiles%\%MODULE_NAME%

:: Setting Python Paths
set PYTHONHOME=%PATH_NAME%\py
set PYTHONPATH=%PATH_NAME%\py;%PATH_NAME%\py\Lib;%PATH_NAME%\py\site-packages

:: Go to the directory with Python
cd /D "%PATH_NAME%"

:: We perform the installation of the specified dependency
python.exe -m pip install %1

echo.
echo ****************************************
echo ************   Success!!!   ************
echo ****************************************
echo.

endlocal