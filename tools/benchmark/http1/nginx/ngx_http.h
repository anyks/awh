/**
 * @file ngx_http.h
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
 * @brief Обвязка окружения парсера nginx — состояние разбираемого запроса
 *
 * @details Состав полей структуры запроса и значения констант повторяют
 *          `src/http/ngx_http_request.h` сервера в той части, к которой
 *          обращается модуль `ngx_http_parse.c`
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_NGINX_HTTP__
#define __AWH_BENCHMARK_RIVAL_NGINX_HTTP__

/**
 * Подключаем обвязку окружения парсера
 */
#include <ngx_config.h>

/**
 * Размер буфера названия заголовка в нижнем регистре
 */
#define NGX_HTTP_LC_HEADER_LEN  32

/**
 * Версии протокола
 */
#define NGX_HTTP_VERSION_9   9
#define NGX_HTTP_VERSION_10  1000
#define NGX_HTTP_VERSION_11  1001
#define NGX_HTTP_VERSION_20  2000
#define NGX_HTTP_VERSION_30  3000

/**
 * Методы запроса
 */
#define NGX_HTTP_UNKNOWN    0x00000001
#define NGX_HTTP_GET        0x00000002
#define NGX_HTTP_HEAD       0x00000004
#define NGX_HTTP_POST       0x00000008
#define NGX_HTTP_PUT        0x00000010
#define NGX_HTTP_DELETE     0x00000020
#define NGX_HTTP_MKCOL      0x00000040
#define NGX_HTTP_COPY       0x00000080
#define NGX_HTTP_MOVE       0x00000100
#define NGX_HTTP_OPTIONS    0x00000200
#define NGX_HTTP_PROPFIND   0x00000400
#define NGX_HTTP_PROPPATCH  0x00000800
#define NGX_HTTP_LOCK       0x00001000
#define NGX_HTTP_UNLOCK     0x00002000
#define NGX_HTTP_PATCH      0x00004000
#define NGX_HTTP_TRACE      0x00008000
#define NGX_HTTP_CONNECT    0x00010000

/**
 * Коды результата разбора
 */
#define NGX_HTTP_PARSE_HEADER_DONE        1
#define NGX_HTTP_PARSE_INVALID_METHOD     10
#define NGX_HTTP_PARSE_INVALID_REQUEST    11
#define NGX_HTTP_PARSE_INVALID_VERSION    12
#define NGX_HTTP_PARSE_INVALID_09_METHOD  13
#define NGX_HTTP_PARSE_INVALID_HEADER     14

/**
 * Признак журналирования небезопасного адреса
 */
#define NGX_HTTP_LOG_UNSAFE  1

/**
 * Состояние разбора тела в кодировке chunked
 */
typedef struct {
	ngx_uint_t  state;
	off_t       size;
	off_t       length;
} ngx_http_chunked_t;

/**
 * Состояние разбора строки состояния ответа
 */
typedef struct {
	ngx_uint_t   http_version;
	ngx_uint_t   code;
	ngx_uint_t   count;
	u_char      *start;
	u_char      *end;
} ngx_http_status_t;

/**
 * Состояние разбираемого запроса
 */
typedef struct {
	ngx_connection_t  *connection;
	void              *upstream;
	ngx_pool_t        *pool;

	ngx_uint_t         method;
	ngx_uint_t         http_version;

	ngx_str_t          uri;
	ngx_str_t          args;
	ngx_str_t          exten;
	ngx_str_t          unparsed_uri;
	ngx_str_t          method_name;
	ngx_str_t          http_protocol;

	unsigned           complex_uri:1;
	unsigned           quoted_uri:1;
	unsigned           plus_in_uri:1;
	unsigned           empty_path_in_uri:1;
	unsigned           space_in_uri:1;
	unsigned           invalid_header:1;

	ngx_uint_t         state;
	ngx_uint_t         header_hash;
	ngx_uint_t         lowcase_index;
	u_char             lowcase_header[NGX_HTTP_LC_HEADER_LEN];

	u_char            *header_name_start;
	u_char            *header_name_end;
	u_char            *header_start;
	u_char            *header_end;

	u_char            *uri_start;
	u_char            *uri_end;
	u_char            *uri_ext;
	u_char            *args_start;
	u_char            *request_start;
	u_char            *request_end;
	u_char            *method_end;
	u_char            *schema_start;
	u_char            *schema_end;
	u_char            *host_start;
	u_char            *host_end;
	u_char            *port_start;
	u_char            *port_end;

	unsigned           http_minor:16;
	unsigned           http_major:16;
} ngx_http_request_t;

/**
 * Объявления функций разбора, определяемых модулем `ngx_http_parse.c`
 */
ngx_int_t ngx_http_parse_request_line(ngx_http_request_t *r, ngx_buf_t *b);
ngx_int_t ngx_http_parse_header_line(ngx_http_request_t *r, ngx_buf_t *b, ngx_uint_t allow_underscores);
ngx_int_t ngx_http_parse_uri(ngx_http_request_t *r);
ngx_int_t ngx_http_parse_complex_uri(ngx_http_request_t *r, ngx_uint_t merge_slashes);
ngx_int_t ngx_http_parse_status_line(ngx_http_request_t *r, ngx_buf_t *b, ngx_http_status_t *status);
ngx_int_t ngx_http_parse_unsafe_uri(ngx_http_request_t *r, ngx_str_t *uri, ngx_str_t *args, ngx_uint_t *flags);
ngx_table_elt_t *ngx_http_parse_multi_header_lines(ngx_http_request_t *r, ngx_table_elt_t *headers, ngx_str_t *name, ngx_str_t *value);
ngx_table_elt_t *ngx_http_parse_set_cookie_lines(ngx_http_request_t *r, ngx_table_elt_t *headers, ngx_str_t *name, ngx_str_t *value);
ngx_int_t ngx_http_arg(ngx_http_request_t *r, u_char *name, size_t len, ngx_str_t *value);
void ngx_http_split_args(ngx_http_request_t *r, ngx_str_t *uri, ngx_str_t *args);
ngx_int_t ngx_http_parse_chunked(ngx_http_request_t *r, ngx_buf_t *b, ngx_http_chunked_t *ctx, ngx_uint_t keep_trailers);

#endif // __AWH_BENCHMARK_RIVAL_NGINX_HTTP__
