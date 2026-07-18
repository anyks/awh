/**
 * @file: parser.hpp
 * @date: 2026-07-18
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

#ifndef __AWH_HTTP_PARSER_1__
#define __AWH_HTTP_PARSER_1__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Подключаем наши заголовочные файлы
 */
#include "../../http.hpp"
#include "../../../../sys/global.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
        /**
		 * @brief Класс HTTP-парсера
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser {

        } parser_t;
    };
};

#endif // __AWH_HTTP_PARSER_1__
