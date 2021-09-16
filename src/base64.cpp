/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include "base64.hpp"

/**
 * base64 Функция кодирования и декодирования base64
 * @param in   буфер входящих данных
 * @param out  буфер исходящих данных
 * @param lin  размер буфера входящих данных
 * @param lout размер буфера исходящих данных
 * @param mode режим работы (false - кодирование, true - декодирование)
 * @return     размер полученных данных
 */
const int awh::Base64::base64(const u_char * in, char * out, u_int lin, u_int lout, const bool mode) const noexcept {
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
		result = BIO_write(b64, in, lin);
		// Выполняем очистку объекта
		BIO_flush(b64);
		// Выполняем чтение полученного результата
		if(result) result = BIO_read(bio, out, lout);
	// Если это декодирование
	} else {
		// Выполняем декодирование из base64
		result = BIO_write(bio, in, lin);
		// Выполняем очистку объекта
		BIO_flush(bio);
		// Выполняем чтение полученного результата
		if(result) result = BIO_read(b64, out, lout);
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
		char out[256];
		// Выполняем кодирование
		const int len = this->base64(reinterpret_cast <const u_char *> (str.c_str()), out, str.length(), sizeof(out), false);
		// Выводим результат
		if(len > 0) result = string(out, len);
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
		char out[256];
		// Выполняем декодирование
		const int len = this->base64(reinterpret_cast <const u_char *> (str.c_str()), out, str.length(), sizeof(out), true);
		// Выводим результат
		if(len > 0) result = string(out, len);
	}
	// Выводим результат
	return result;
}
