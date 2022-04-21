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

#include <string>
#include <vector>
#include <sys/types.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

/**
 * Наши модули
 */
#include <sys/win.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Base64 Класс
	 */
	typedef class Base64 {
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
			const int base64(const u_char * in, char * out, u_int sin, u_int sout, const bool mode = false) const noexcept;
		public:
			/**
			 * encode Метод кодирования в base64
			 * @param str входящая строка для кодирования
			 * @return    результирующая строка
			 */
			const string encode(const string & str) const noexcept;
			/**
			 * decode Метод декодирования из base64
			 * @param str входящая строка для декодирования
			 * @return    результирующая строка
			 */
			const string decode(const string & str) const noexcept;
	} base64_t;
};

#endif // __AWH_BASE64__
