/**
 * @file: base64.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_BASE64__
#define __AWH_BASE64__

/**
 * Стандартные модули
 */
#include <string>
#include <vector>
#include <sys/types.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

/**
 * Наши модули
 */
#include <sys/os.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Base64 Класс
	 */
	typedef class AWHSHARED_EXPORT Base64 {
		private:
			/**
			 * base64 Метод кодирования и декодирования base64
			 * @param in   буфер входящих данных
			 * @param out  буфер исходящих данных
			 * @param sin  размер буфера входящих данных
			 * @param sout размер буфера исходящих данных
			 * @param mode режим работы (false - кодирование, true - декодирование)
			 * @return     размер полученных данных
			 */
			int32_t base64(const u_char * in, char * out, uint32_t sin, uint32_t sout, const bool mode = false) const noexcept;
		public:
			/**
			 * encode Метод кодирования в base64
			 * @param str входящая строка для кодирования
			 * @return    результирующая строка
			 */
			string encode(const string & str) const noexcept;
			/**
			 * decode Метод декодирования из base64
			 * @param str входящая строка для декодирования
			 * @return    результирующая строка
			 */
			string decode(const string & str) const noexcept;
		public:
			/**
			 * Base64 Конструктор
			 */
			Base64() noexcept {}
			/**
			 * ~Base64 Деструктор
			 */
			~Base64() noexcept {}
	} base64_t;
};

#endif // __AWH_BASE64__
