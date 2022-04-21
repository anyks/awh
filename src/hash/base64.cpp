/**
 * @file: base64.cpp
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

// Подключаем заголовочный файл
#include "hash/base64.hpp"

/**
 * base64 Функция кодирования и декодирования base64
 * @param in   буфер входящих данных
 * @param out  буфер исходящих данных
 * @param sin  размер буфера входящих данных
 * @param sout размер буфера исходящих данных
 * @param mode режим работы (false - кодирование, true - декодирование)
 * @return     размер полученных данных
 */
const int awh::Base64::base64(const u_char * in, char * out, u_int sin, u_int sout, const bool mode) const noexcept {
	// Результат
	int result = 0;
	// Инициализируем объекты
	BIO * b64 = BIO_new(BIO_f_base64());
	BIO * bio = BIO_new(BIO_s_mem());
	// Устанавливаем флаги
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	// Записываем параметры
	BIO_push(b64, bio);
	// Если это кодирование
	if(!mode){
		// Выполняем кодирование в base64
		result = BIO_write(b64, in, sin);
		// Выполняем очистку объекта
		BIO_flush(b64);
		// Выполняем чтение полученного результата
		if(result) result = BIO_read(bio, out, sout);
	// Если это декодирование
	} else {
		// Выполняем декодирование из base64
		result = BIO_write(bio, in, sin);
		// Выполняем очистку объекта
		BIO_flush(bio);
		// Выполняем чтение полученного результата
		if(result) result = BIO_read(b64, out, sout);
    }
	// Очищаем объект base64
	BIO_free(b64);
	// Выводим результат
	return result;
}
/**
 * encode Метод кодирования в base64
 * @param str входящая строка для кодирования
 * @return    результирующая строка
 */
const string awh::Base64::encode(const string & str) const noexcept {
	// Результирующая строка
	string result = "";
	// Если строка передана
	if(!str.empty()){
		// Буфер для кодирования
		vector <char> buffer((4 * ((str.size() + 2) / 3)) + 1, 0);
		// Выполняем кодирование
		const int size = this->base64(reinterpret_cast <const u_char *> (str.c_str()), buffer.data(), str.size(), buffer.size(), false);
		// Выводим результат
		if(size > 0) result = string(buffer.data(), size);
	}
	// Выводим результат
	return result;
}
/**
 * decode Метод декодирования из base64
 * @param str входящая строка для декодирования
 * @return    результирующая строка
 */
const string awh::Base64::decode(const string & str) const noexcept {
	// Результирующая строка
	string result = "";
	// Если строка передана
	if(!str.empty()){
		// Буфер для декодирования
		vector <char> buffer((3 * str.size() / 4) + 1, 0);
		// Выполняем декодирование
		const int size = this->base64(reinterpret_cast <const u_char *> (str.c_str()), buffer.data(), str.size(), buffer.size(), true);
		// Выводим результат
		if(size > 0) result = string(buffer.data(), size);
	}
	// Выводим результат
	return result;
}
