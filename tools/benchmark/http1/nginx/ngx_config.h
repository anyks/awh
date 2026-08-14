/**
 * @file ngx_config.h
 * @date 2026-07-26
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Обвязка окружения парсера nginx — минимальный набор типов, констант и
 *        функций, требуемых модулем `ngx_http_parse.c` для сборки вне дерева сервера
 *
 * @details Парсер nginx библиотекой не является и в отрыве от сервера не собирается:
 *          модуль опирается на общее окружение сервера, а окружение тянет за собой
 *          весь остальной сервер. Обвязка подменяет собой только то, к чему модуль
 *          обращается, и делает это буквально: значения констант, состав полей
 *          структур и семантика функций повторяют исходное окружение, иначе
 *          собранный парсер вёл бы себя иначе, чем в сервере, и сравнивать было бы
 *          нечего. Исходные тексты самого парсера не изменяются ни на символ
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_NGINX__
#define __AWH_BENCHMARK_RIVAL_NGINX__

/**
 * Стандартные заголовочные файлы
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>
#include <sys/types.h>

/**
 * Свойства целевой платформы: обе включены на всех поддерживаемых стендом
 * архитектурах и выбирают в парсере путь сравнения названий методов машинным
 * словом вместо посимвольного
 */
#define NGX_HAVE_NONALIGNED     1
#define NGX_HAVE_LITTLE_ENDIAN  1

/**
 * Признак инициализации переменных ради подавления предупреждений компилятора
 */
#define NGX_SUPPRESS_WARN       1

/**
 * Коды возврата функций сервера
 */
#define NGX_OK        0
#define NGX_ERROR    -1
#define NGX_AGAIN    -2
#define NGX_BUSY     -3
#define NGX_DONE     -4
#define NGX_DECLINED -5
#define NGX_ABORT    -6

/**
 * Предельное значение смещения в файле
 */
#define NGX_MAX_OFF_T_VALUE  INT64_MAX

/**
 * Уровни журналирования
 */
#define NGX_LOG_ERR         4
#define NGX_LOG_DEBUG_HTTP  0x080

/**
 * Октеты завершения строки
 */
#define CR    (u_char) 13
#define LF    (u_char) 10
#define CRLF  "\x0d\x0a"

/**
 * Разделитель элементов пути в файловой системе
 */
#define ngx_path_separator(c)  ((c) == '/')

/**
 * Функция подмешивания октета в хеш названия заголовка
 */
#define ngx_hash(key, c)  ((ngx_uint_t) key * 31 + c)

/**
 * Сравнение строк
 */
#define ngx_strncmp(s1, s2, n)      strncmp((const char *) s1, (const char *) s2, n)
#define ngx_strncasecmp(s1, s2, n)  strncasecmp((const char *) s1, (const char *) s2, n)

/**
 * Журналирование: стенд измеряет разбор, поэтому вывод сообщений отключён.
 * Сервер собирается без отладочного журнала точно так же - вызовы ngx_log_debug
 * в сборке по умолчанию раскрываются в пустое выражение
 */
#define ngx_log_error(level, log, err, ...)  (void) 0
#define ngx_log_debug1(level, log, err, ...) (void) 0
#define ngx_log_debug2(level, log, err, ...) (void) 0
#define ngx_log_debug3(level, log, err, ...) (void) 0

/**
 * Целочисленные типы сервера
 */
typedef intptr_t   ngx_int_t;
typedef uintptr_t  ngx_uint_t;

/**
 * Строка сервера: длина и указатель на данные, завершающего нуля нет
 */
typedef struct {
	size_t   len;
	u_char  *data;
} ngx_str_t;

/**
 * Буфер сервера: разбор идёт от `pos` до `last`
 */
typedef struct {
	u_char  *pos;
	u_char  *last;
} ngx_buf_t;

/**
 * Элемент списка заголовков сервера
 */
typedef struct ngx_table_elt_s  ngx_table_elt_t;

struct ngx_table_elt_s {
	ngx_uint_t        hash;
	ngx_str_t         key;
	ngx_str_t         value;
	u_char           *lowcase_key;
	ngx_table_elt_t  *next;
};

/**
 * Журнал сервера
 */
typedef struct {
	int  level;
} ngx_log_t;

/**
 * Подключение сервера
 */
typedef struct {
	ngx_log_t  *log;
} ngx_connection_t;

/**
 * Пул памяти сервера
 */
typedef struct {
	u_char  *last;
	u_char  *end;
} ngx_pool_t;

/**
 * @brief Функция выделения невыровненной памяти из пула
 *
 * @note Пул подменён линейным распределителем: разбор обращается к нему только
 *       на нормализации сложного адреса запроса, которая в сценариях сравнения
 *       не участвует, а поведение линейного распределителя совпадает с пулом
 *       сервера в той части, которая парсеру видна
 *
 * @param pool пул памяти
 * @param size размер выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
static inline void * ngx_pnalloc(ngx_pool_t *pool, size_t size) {
	// Если запрошенная память в пуле отсутствует
	if((pool == NULL) || ((size_t) (pool->end - pool->last) < size))
		// Выводим признак ошибки выделения памяти
		return NULL;
	// Получаем указатель на выделенную память
	u_char *result = pool->last;
	// Выполняем сдвиг границы занятой памяти пула
	pool->last += size;
	// Выводим указатель на выделенную память
	return result;
}
/**
 * @brief Функция поиска октета в строке заданной длины
 *
 * @param p    начало строки
 * @param last конец строки
 * @param c    искомый октет
 * @return     указатель на найденный октет
 *
 */
static inline u_char * ngx_strlchr(u_char *p, u_char *last, u_char c) {
	/**
	 * Перебираем октеты строки
	 */
	while(p < last){
		// Если искомый октет найден
		if(*p == c)
			// Выводим указатель на найденный октет
			return p;
		// Переходим к следующему октету строки
		p++;
	}
	// Выводим признак отсутствия искомого октета
	return NULL;
}
/**
 * @brief Функция регистронезависимого поиска подстроки в строке заданной длины
 *
 * @note Повторяет соглашение сервера: искомая подстрока задаётся в нижнем
 *       регистре, а её длина - на единицу меньше фактической
 *
 * @param s1   начало строки
 * @param last конец строки
 * @param s2   искомая подстрока в нижнем регистре
 * @param n    длина искомой подстроки без первого октета
 * @return     указатель на найденную подстроку
 *
 */
static inline u_char * ngx_strlcasestrn(u_char *s1, u_char *last, u_char *s2, size_t n) {
	// Первый октет искомой подстроки
	u_char c2 = *s2++;
	// Приводим первый октет искомой подстроки к нижнему регистру
	c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
	// Выполняем сдвиг конца строки на длину искомой подстроки
	last -= n;
	/**
	 * Перебираем октеты строки
	 */
	while(s1 < last){
		// Получаем очередной октет строки
		u_char c1 = *s1++;
		// Приводим очередной октет строки к нижнему регистру
		c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
		// Если первый октет искомой подстроки не совпал
		if(c1 != c2)
			// Переходим к следующему октету строки
			continue;
		// Если искомая подстрока найдена
		if(ngx_strncasecmp(s1, s2, n) == 0)
			// Выводим указатель на найденную подстроку
			return (s1 - 1);
	}
	// Выводим признак отсутствия искомой подстроки
	return NULL;
}
/**
 * @brief Функция раскодирования экранированных последовательностей адреса
 *
 * @note Обращение к функции возможно только из `ngx_http_parse_unsafe_uri`,
 *       которая в сценариях сравнения не участвует: нормализацией адреса
 *       занимается сервер, а не парсер. Обвязка выполняет побайтовое
 *       копирование, чтобы сборка была полной
 *
 * @param dst  выводимый указатель на приёмник
 * @param src  выводимый указатель на источник
 * @param size размер раскодируемых данных
 * @param type тип раскодирования
 *
 */
static inline void ngx_unescape_uri(u_char **dst, u_char **src, size_t size, ngx_uint_t type) {
	// Подавляем предупреждение о неиспользуемом параметре
	(void) type;
	// Выполняем копирование раскодируемых данных
	memcpy(*dst, *src, size);
	// Выполняем сдвиг указателя приёмника
	*dst += size;
	// Выполняем сдвиг указателя источника
	*src += size;
}

#endif // __AWH_BENCHMARK_RIVAL_NGINX__
