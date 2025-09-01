Name:         @name@
Version:      @version@
Release:      @release_number@
Summary:      @summary@
License:      Subscription
BuildArch:    @architecture@
Group:        Trading/Crypto
URL:          @url@
Distribution: @distribution@

BuildRequires: cmake

%description
@description@

# Устанавливаем каталог сборки
%define _rpmdir @prefix@

# Отключаем проверку RPATH
%global __brp_check_rpaths %{nil}

# Если сборка производится в Alt-linux
%if "%_vendor" == "alt"
   # Отключаем проверку ELF зависимостей
   %set_verify_elf_method none

   # Отключаем проверку зависимостей
   %define __find_requires %{nil}
%endif

%install
# Выполняем создание каталогов
mkdir -p "@prefix@/usr/lib" || exit 1
mkdir -p "@prefix@/usr/share/@name@/cmake" || exit 1
mkdir -p "@prefix@/usr/include/lib@name@/@name@" || exit 1

# Копируем собранную статическую библиотеку
mv @tmp@/libawh.a "@prefix@/usr/lib"/libawh.a
# Копируем собранную динамическую библиотеку
mv @tmp@/libawh.so "@prefix@/usr/lib"/libawh.so
# Копируем зависимости сторонние
cp -r "@root@/../contrib/include"/* "@prefix@/usr/include/lib@name@"/
# Копируем собранные зависимости
cp -r "@root@/../third_party/include"/* "@prefix@/usr/include/lib@name@"/
# Копируем заголовки библиотеки
cp -r "@root@/../include"/* "@prefix@/usr/include/lib@name@/@name@"/
# Копируем файл cmake
cp "@root@/../contrib/cmake"/FindAWH.cmake "@prefix@/usr/share/@name@/cmake"/

# Удаляем более ненужный нам каталог
rm -rf @tmp@

# Активируем глобальную сорку
sed -i "s!SET(AHW_GLOBAL_INSTALLATION FALSE)!SET(AHW_GLOBAL_INSTALLATION TRUE)!g" "@prefix@/usr/share/@name@/cmake"/FindAWH.cmake

%clean
cp $(find @prefix@ -name "@name@*.rpm") %{buildroot}/../
%if "%{noclean}" == ""
   rm -rf %{buildroot}
%endif

%files
%defattr(-,root,root)
/usr/lib/libawh.a
/usr/lib/libawh.so
/usr/include/lib@name@/*
/usr/share/@name@/cmake/FindAWH.cmake

%post
modprobe sctp
sysctl -w net.sctp.auth_enable=1

# Получаем путь установки cmake
CMAKE_PATH=$(cmake --system-information | grep CMAKE_ROOT | cut -d= -f2 | awk '{print $2}' | sed "s/^\([\"']\)\(.*\)\1\$/\2/g")

# Если ссылка уже существует
if [ -f "$CMAKE_PATH/Modules/FindAWH.cmake" ]; then
   # Удаляем устаревшую ссылку
   rm "$CMAKE_PATH/Modules"/FindAWH.cmake
fi

# Выполняем перемещение файла CMake
ln -s /usr/share/@name@/cmake/FindAWH.cmake "$CMAKE_PATH/Modules"/FindAWH.cmake

%changelog
* @date@ @distribution@ <@email@> - @version@-@release_number@
- Trading platform version v@version@-@release_number@ release
