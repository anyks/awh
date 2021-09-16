/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_BASE64__
#define __AWH_BASE64__

#include <string>
#include <sys/types.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

/**
 * Наши модули
 */
#include <win.hpp>

// Устанавливаем область видимости
using namespace std;

/*
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
			 * @param lin  размер буфера входящих данных
			 * @param lout размер буфера исходящих данных
			 * @param mode режим работы (false - кодирование, true - декодирование)
			 * @return     размер полученных данных
			 */
			const int base64(const u_char * in, char * out, u_int lin, u_int lout, const bool mode = false) const noexcept;
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
