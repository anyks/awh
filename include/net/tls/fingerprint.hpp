/**
 * @file: fingerprint.hpp
 * @date: 2026-04-28
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

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SSL_FINGERPRINT__
#define __AWH_SSL_FINGERPRINT__

/**
 * Наши модули
 */
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Структура цифрового отпечатка устройства
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Fingerprint {
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод парсинга данных цифрового отпечатка
			 *
			 * @param buffer бинарный буфер данных цифрового отпечатка
			 * @param size   размер бинарного буфера данных цифрового отпечатка
			 * @return       результат парсинга данных цифрового отпечатка
			 */
			bool parse(const uint8_t * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Fingerprint(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Fingerprint() noexcept;
	} fgp_t;
};

#endif // __AWH_SSL_FINGERPRINT__
