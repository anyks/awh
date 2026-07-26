#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/third_party/rival/http1"

# Версия исходных текстов парсера nginx
readonly NGINX_VERSION="1.28.0"

# Ревизия исходных текстов парсера picohttpparser
readonly PICO_REVISION="master"

# Ревизия исходных текстов парсера http-parser
readonly LEGACY_REVISION="main"

##
# Функция получения одного файла
#
# @param $1 адрес файла
# @param $2 название файла в каталоге исходных текстов
#
obtain(){
	# Если файл уже получен
	if [ -f "$VENDOR/$2" ]; then
		# Выводим сообщение о пропуске файла
		echo "Skip \"$2\""
		# Выходим без получения файла
		return 0
	fi

	# Выводим сообщение о получении файла
	echo "Fetch \"$2\""

	# Выполняем получение файла
	curl -fsSL --max-time 60 -o "$VENDOR/$2" "$1" || exit 1
}

# Выполняем создание каталога исходных текстов
mkdir -p "$VENDOR" || exit 1

##
# Парсер picohttpparser проекта h2o
#
# Распространяется двумя файлами и системы сборки не имеет: авторы
# предполагают включение исходных текстов в проект как есть
#
obtain "https://raw.githubusercontent.com/h2o/picohttpparser/$PICO_REVISION/picohttpparser.c" "picohttpparser.c"
obtain "https://raw.githubusercontent.com/h2o/picohttpparser/$PICO_REVISION/picohttpparser.h" "picohttpparser.h"

##
# Парсер http-parser проекта Node.js
#
# Развитие прекращено в пользу llhttp, но парсер остаётся в эксплуатации
# у множества проектов и представляет в сравнении класс написанных вручную
# конечных автоматов, к которому относится и парсер nginx
#
obtain "https://raw.githubusercontent.com/nodejs/http-parser/$LEGACY_REVISION/http_parser.c" "http_parser.c"
obtain "https://raw.githubusercontent.com/nodejs/http-parser/$LEGACY_REVISION/http_parser.h" "http_parser.h"

##
# Парсер nginx
#
# Библиотекой не является: нужные функции извлекаются из единственного модуля
# исходных текстов сервера, а окружение подменяется обвязкой стенда
#
if [ ! -f "$VENDOR/ngx_http_parse.c" ]; then
	# Выводим сообщение о получении исходных текстов
	echo "Fetch \"ngx_http_parse.c\" from nginx-$NGINX_VERSION"

	# Выполняем получение архива исходных текстов сервера
	curl -fsSL --max-time 120 -o "$VENDOR/nginx.tar.gz" "https://nginx.org/download/nginx-$NGINX_VERSION.tar.gz" || exit 1

	# Извлекаем единственный нужный модуль исходных текстов
	tar -xzf "$VENDOR/nginx.tar.gz" -C "$VENDOR" --strip-components 3 "nginx-$NGINX_VERSION/src/http/ngx_http_parse.c" || exit 1

	# Удаляем архив исходных текстов сервера
	rm -f "$VENDOR/nginx.tar.gz"
else
	# Выводим сообщение о пропуске исходных текстов
	echo "Skip \"ngx_http_parse.c\""
fi

# Выводим сообщение о завершении получения исходных текстов
echo "Sources are placed in \"$VENDOR\""
