#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "boringssl"
		# Для операционной системы Windows
		if [[ $OS = "Windows" ]]; then
			# Удаляем все зависимости библиотеки
			rm -f "$PREFIX/lib/libdependence.lib"
		# Для всех остальных операционных систем
		else
			# Удаляем все зависимости библиотеки
			rm -f "$PREFIX/lib/libdependence.a"
		fi
		# Удаляем все зависимые заголовки библиотеки
		rm -rf "$PREFIX/include/openssl"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/boringssl"
		printf "\n****** BoringSSL ******\n"
		cd "$src" || exit 1

		# Выполняем получение данных с репозитория
		git pull origin $2 || exit 1
		git fetch origin $2 || exit 1

		# Переключаемся на указанную ветку
		git checkout $2 || exit 1

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо собрать проект
	elif [ $1 = "--build" ] || [ $1 = "--update" ]; then
		# Сборка BoringSSL
		src="$ROOT/../submodules/boringssl"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** BoringSSL ******\n"
			cd "$src" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="20260413.0"

			# Если ветка или тег передан
			if [ -n "$2" ]; then
				# Получаем название ветки или тега
				NAME=$(echo "${2:3}")
				# Получаем флаг ветки или тега
				FLAG=$(echo "$2" | cut -c-3)
			fi

			# Если флаг передан в виде ветки
			if [ $FLAG = "--b" ]; then
				# Скачиваем зависимости
				git fetch origin $NAME
				# Переключаемся на ветку
				git checkout $NAME
				# Выполняем обновление репозитория
				git pull origin $NAME
			# Если флаг передан в виде коммита
			elif [ $FLAG = "--c" ]; then
				# Если установлена команда обновления сборки
				if [[ $1 = "--update" ]]; then
					# Выполняем обновление репозитория
					git pull origin
				fi
				# Переходим на указанный коммит
				git reset --hard $NAME
			# Если флаг передан в виде тега
			elif [ $FLAG = "--t" ]; then
				# Выполняем удаление предыдущей закаченной версии
				git tag -d $NAME
				# Закачиваем все изменения
				git fetch --all
				# Закачиваем все теги
				git fetch --all --tags
				# Выполняем жесткое переключение на main
				git reset --hard origin/main
				# Переключаемся на main
				git checkout main
				# Выполняем обновление данных
				git pull origin main
				# Удаляем старую ветку
				git branch -D $NAME-branch
				# Выполняем переключение на указанную версию
				git checkout -b $NAME-branch $NAME

				# Если установлена команда обновления сборки
				if [[ $1 = "--update" ]]; then
					# Выполняем обновление репозитория
					git pull origin
				fi
			# Если флаг передан в виде версии
			elif [ $FLAG = "--v" ]; then
				# Выполняем удаление предыдущей закаченной версии
				git tag -d "0.$NAME"
				# Закачиваем все изменения
				git fetch --all
				# Закачиваем все теги
				git fetch --all --tags
				# Выполняем жесткое переключение на main
				git reset --hard origin/main
				# Переключаемся на main
				git checkout main
				# Выполняем обновление данных
				git pull origin main
				# Удаляем старую ветку
				git branch -D v${NAME}-branch
				# Выполняем переключение на указанную версию
				git checkout -b v${NAME}-branch "0.$NAME"

				# Если установлена команда обновления сборки
				if [[ $1 = "--update" ]]; then
					# Выполняем обновление репозитория
					git pull origin main
				fi
			# Если передан непонятный флаг
			else
				# Сообщаем, что флаги не поддерживаются
				echo "Flag \"$FLAG\" is not supported"
				# Выходим из скрипта
				exit 1
			fi

			# Для операционной системы Windows
			if [ $OS = "Windows" ]; then
				##
				 # Накладываем исправление расхождения условий сборки ассемблера FIAT
				 #
				 # Дефект в самом BoringSSL, проявляется лишь под MinGW. Условия в двух
				 # местах расходятся:
				 #
				 #   third_party/fiat/p256_64.h        - вызов:
				 #     !OPENSSL_NO_ASM && __GNUC__ && __x86_64__
				 #   third_party/fiat/asm/*_adx_*.S    - определение:
				 #     !OPENSSL_NO_ASM && OPENSSL_X86_64 && (__APPLE__ || __ELF__)
				 #
				 # Под MinGW первое истинно, второе ложно - формат PE/COFF не ELF и не
				 # Mach-O. Вызов есть, тела нет, и компоновка отвечает отказом на
				 # fiat_p256_adx_mul и fiat_p256_adx_sqr. Под MSVC расхождения нет:
				 # там __GNUC__ не объявлен, и оба условия ложны
				 #
				 # Исправление приводит условие вызова к условию определения. Собирать
				 # эти файлы .S под Windows нельзя: соглашение о вызовах там SysV
				 # (rdi, rsi, rdx), а у Windows x64 своё (rcx, rdx, r8) - скомпоновалось
				 # бы, но работало бы неверно, что хуже отказа сборки
				 #
				 # Накладывается здесь, а не хранится в подмодуле, потому что выше
				 # выполняется "git reset --hard origin/main", стирающий любые правки.
				 # Мера временная: как только исправление ляжет в зеркало подмодуля,
				 # этот блок подлежит удалению
				 ##
				sed -i \
				 "s@defined(__GNUC__) \&\& defined(__x86_64__)\$@defined(__GNUC__) \&\& defined(__x86_64__) \&\& (defined(__APPLE__) || defined(__ELF__))@" \
				 "$src/third_party/fiat/p256_64.h" || exit 1
			fi

			# Создаём каталог сборки
			mkdir -p "build" || exit 1
			# Переходим в каталог
			cd "build" || exit 1

			# Удаляем старый файл кэша
			rm -rf "$src/build/CMakeCache.txt"

			# Выполняем конфигурацию проекта под Windows
			if [ $OS = "Windows" ]; then
				##
				 # Выполняем конфигурацию проекта
				 #
				 # Компиляторы указываются явно намеренно. Сборочный файл самого BoringSSL
				 # содержит "if(WIN32) set(CMAKE_GENERATOR_CC cl) endif()", то есть под
				 # MS Windows предпочитает cl от MSVC, даже когда доступен GCC. В сборке
				 # MSYS2 MinGW64 компилятора cl нет, и настройка отвечает отказом
				 # "The CMAKE_C_COMPILER: cl is not a full path and was not found in the PATH"
				 #
				 # Сборке требуется ассемблер NASM: без него настройка отвечает отказом
				 # "No CMAKE_ASM_NASM_COMPILER could be found". Ставится он командой
				 # pacman -S mingw-w64-x86_64-nasm. Отключать ассемблерные вставки через
				 # OPENSSL_NO_ASM не следует - потеря в скорости шифрования заметная
				 #
				 # Гашение -Werror=format нужно из-за дефекта в самой утилите bssl:
				 # в tool/transport_common.cc значение типа int выводится по образцу "%x",
				 # ждущему unsigned int, а собирается BoringSSL с -Werror. Под MinGW это
				 # останавливает сборку. Гасится точечно, лишь этот разряд предупреждений,
				 # и лишь потому, что дефект в чужом коде и правке с нашей стороны
				 # не подлежит. Сами библиотеки собираются без единого предупреждения
				 ##
				cmake \
				 -DCMAKE_C_COMPILER=gcc \
				 -DCMAKE_CXX_COMPILER=g++ \
				 -DCMAKE_C_FLAGS="-Wno-error=format" \
				 -DCMAKE_CXX_FLAGS="-Wno-error=format" \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DBUILD_SHARED_LIBS=OFF \
				 -DBUILD_TESTING=OFF \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -G "MSYS Makefiles" \
				 .. || exit 1
			# Выполняем конфигурацию проекта под macOS
			elif [ $OS = "Darwin" ]; then
				# Если архитектура ARM
				if [[ $ARCHITECTURE = "arm" ]]; then
					# Устанавливаем архитектуру для сборки под ARM
					ARCH="arm64"
				# Если архитектура x86
				else
					# Устанавливаем архитектуру для сборки под x86
					ARCH="x86_64"
				fi

				# Выполняем конфигурацию проекта
				cmake \
				 -DCMAKE_C_COMPILER=clang \
				 -DCMAKE_CXX_COMPILER=clang++ \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DBUILD_SHARED_LIBS=OFF \
				 -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
				 -DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET \
				 -DBUILD_TESTING=OFF \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 .. || exit 1
			# Выполняем конфигурацию проекта под все остальные операционные системы
			else
				# Дополнительные библиотеки, требуемые компоновщику
				EXTRA_LIBS=""

				# Если операционной системой является SunOS
				if [ $OS = "SunOS" ]; then
					##
					 # Сетевые вызовы у illumos лежат в отдельных библиотеках
					 #
					 # Утилита bssl обращается к recv, send, socket, bind, listen, accept,
					 # setsockopt и getaddrinfo. У illumos (OpenIndiana) они лежат в libsocket
					 # и libnsl, а не в libc, и сама BoringSSL их не подключает - компоновка
					 # отвечает отказом "Undefined symbol: __xnet_socket, __xnet_bind, recv, send".
					 # Solaris 11.4 перенёс эти вызовы в libc, и там сборка проходит и без них,
					 # но подключение лишних библиотек ей не вредит
					 #
					 # Отказ этот приходится на утилиту bssl, а не на сами libcrypto и libssl:
					 # те собираются целыми в обоих случаях
					 ##
					EXTRA_LIBS="-lsocket -lnsl"
				fi

				# Дополнительные признаки сборки
				EXTRA_FLAGS=""

				##
				 # Опознание возможностей процессора на ходу у NetBSD с ARM отсутствует
				 #
				 # BoringSSL опознаёт их своим файлом на каждую пару «система и
				 # архитектура»: для aarch64 у неё есть Apple, Fuchsia, Linux, OpenBSD,
				 # Windows и FreeBSD, а NetBSD нет вовсе. Опознаватель
				 # OPENSSL_cpuid_setup() при этом объявлен и вызывается, но не собирается
				 # ни из одного файла, и компоновка отвечает отказом
				 # "undefined reference to bssl::OPENSSL_cpuid_setup()"
				 #
				 # Признак OPENSSL_STATIC_ARMCAP предусмотрен самой BoringSSL ровно для
				 # таких систем: опознание на ходу выключается, а возможности берутся у
				 # компилятора - из макросов __ARM_NEON, __ARM_FEATURE_AES и им подобных.
				 # Набор NEON у ARMv8 обязателен и остаётся всегда, а расширения
				 # шифрования включаются в той мере, в какой их объявляет компилятор
				 #
				 # Оттого одного признака этого НЕДОСТАТОЧНО. По умолчанию компилятор
				 # нацелен на armv8-a без расширений и объявляет единственно __ARM_NEON,
				 # а __ARM_FEATURE_AES не объявляет вовсе - проверено выводом cc -dM.
				 # Возможности при этом сводятся к NEON, шифрование идёт исполнением на
				 # общих командах, и замер даёт 260 МБ/с на AES-256-GCM там, где сам
				 # процессор способен на 7183 МБ/с. Различие двадцатисемикратное,
				 # проверено сличением двух сборок BoringSSL на одной машине
				 #
				 # Потому вместе с признаком задаётся и цель armv8-a+crypto: опознание
				 # на ходу у NetBSD невозможно (чтение регистра признаков из EL0 система
				 # не эмулирует и отвечает отказом Illegal instruction, а sysctl его не
				 # отдаёт), и объявить возможности иначе как при сборке нечем
				 #
				 # @warning Задание это - утверждение, а не опознание: собранное таким
				 # образом требует у процессора расширений шифрования ARMv8 и на ARM64
				 # без них даст отказ исполнения. Для NetBSD ограничение принято
				 # сознательно, ибо выбора между утверждением и опознанием там нет
				 #
				 # Проверено опытом на стенде NetBSD 11.0 aarch64
				 ##
				if [ $OS = "NetBSD" ] && [ "$ARCHITECTURE" = "arm" ]; then
					# Выключаем опознание возможностей процессора на ходу и объявляем расширения
					EXTRA_FLAGS="-DOPENSSL_STATIC_ARMCAP -march=armv8-a+crypto"
				fi

				# Выполняем конфигурацию проекта
				cmake \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DBUILD_SHARED_LIBS=OFF \
				 -DBUILD_TESTING=OFF \
				 -DCMAKE_C_FLAGS="$EXTRA_FLAGS" \
				 -DCMAKE_CXX_FLAGS="$EXTRA_FLAGS" \
				 -DCMAKE_EXE_LINKER_FLAGS="$EXTRA_LIBS" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 .. || exit 1
			fi

			# Выполняем сборку на всех логических ядрах
			$MAKE -j"$numproc" || exit 1
			# Выполняем установку проекта
			$MAKE install || exit 1

			# Копируем статическую библиотеку в каталог установки
			cp ./libdecrepit.a "$PREFIX/lib/"

			# Выполняем компенсацию каталогов
			restorelibs $PREFIX

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"

			# Переходим обратно в рабочий каталог
			cd "$ROOT" || exit 1
		fi
	fi
fi
