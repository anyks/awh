/**
 * @file http3.cpp
 * @date 2026-07-27
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
 * @brief Общее окружение бенчмарков протокола HTTP/3 — объекты окружения,
 *        эталонные наборы полей и сборка потоков соединения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

/**
 * Подключаем заголовочный файл бенчмарков протокола HTTP/3
 */
#include "http3.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция получения объекта фреймворка окружения бенчмарка
 *
 * @return объект фреймворка
 *
 */
const awh::fmk_t * awh::benchmark::http3::fmk() noexcept {
	// Объект фреймворка окружения бенчмарка
	static awh::fmk_t result;
	// Выводим объект фреймворка
	return &result;
}
/**
 * @brief Функция получения объекта логирования окружения бенчмарка
 *
 * @return объект логирования
 *
 */
const awh::log_t * awh::benchmark::http3::log() noexcept {
	// Объект логирования окружения бенчмарка
	static awh::log_t result(awh::benchmark::http3::fmk());
	// Отключаем вывод логов: часть сценариев намеренно упирается в лимиты
	result.level(awh::log_t::level_t::NONE);
	// Выводим объект логирования
	return &result;
}
/**
 * @brief Функция получения эталонного набора полей запроса браузера
 *
 * @param index номер запроса в последовательности
 * @return      набор полей запроса
 *
 */
vector <awh::http::h3::qpack::field_t> awh::benchmark::http3::request(const size_t index) noexcept {
	// Результат работы функции - набор полей запроса
	vector <awh::http::h3::qpack::field_t> result;
	// Резервируем память под набор полей
	result.reserve(12);
	// Дописываем псевдо-поле метода запроса
	result.emplace_back(":method", "GET");
	// Дописываем псевдо-поле схемы запроса
	result.emplace_back(":scheme", "https");
	// Дописываем псевдо-поле авторитета запроса
	result.emplace_back(":authority", "www.example.com");
	/**
	 * Путь делаем переменной частью запроса: постоянный путь целиком уходил бы
	 * в динамическую таблицу и сжатие вышло бы недостижимо оптимистичным
	 */
	result.emplace_back(":path", ("/assets/bundle-" + std::to_string(index % 64) + ".js"));
	// Дописываем поле принимаемых типов содержимого
	result.emplace_back("accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");
	// Дописываем поле принимаемых кодировок содержимого
	result.emplace_back("accept-encoding", "gzip, deflate, br, zstd");
	// Дописываем поле принимаемых языков
	result.emplace_back("accept-language", "ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7");
	// Дописываем поле клиентского приложения
	result.emplace_back("user-agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36");
	// Дописываем поле источника перехода
	result.emplace_back("referer", "https://www.example.com/catalog/index.html");
	// Дописываем поле печенья сессии
	result.emplace_back("cookie", "session=7f3c1a9b2e4d6f8a0c2e4b6d8f1a3c5e; theme=dark; lang=ru");
	// Дописываем поле политики кеширования
	result.emplace_back("cache-control", "no-cache");
	// Выводим набор полей запроса
	return result;
}
/**
 * @brief Функция получения эталонного набора полей ответа сервера
 *
 * @param index номер ответа в последовательности
 * @return      набор полей ответа
 *
 */
vector <awh::http::h3::qpack::field_t> awh::benchmark::http3::response(const size_t index) noexcept {
	// Результат работы функции - набор полей ответа
	vector <awh::http::h3::qpack::field_t> result;
	// Резервируем память под набор полей
	result.reserve(8);
	// Дописываем псевдо-поле статуса ответа
	result.emplace_back(":status", "200");
	// Дописываем поле типа содержимого
	result.emplace_back("content-type", "application/javascript; charset=utf-8");
	// Дописываем поле длины содержимого: она обязана совпасть с фактическим телом
	result.emplace_back("content-length", std::to_string(awh::benchmark::http3::payload().size()));
	// Дописываем поле политики кеширования
	result.emplace_back("cache-control", "public, max-age=31536000, immutable");
	// Дописываем поле метки версии содержимого
	result.emplace_back("etag", ("\"" + std::to_string(index) + "-a1b2c3d4\""));
	// Дописываем поле сервера
	result.emplace_back("server", "awh");
	// Выводим набор полей ответа
	return result;
}
/**
 * @brief Функция получения эталонного тела ответа
 *
 * @return тело ответа
 *
 */
const string & awh::benchmark::http3::payload() noexcept {
	// Эталонное тело ответа
	static const string result(64, 'r');
	// Выводим тело ответа
	return result;
}
/**
 * @brief Функция подсчёта размера набора полей до сжатия
 *
 * @param fields набор полей
 * @return       размер набора полей в октетах
 *
 */
size_t awh::benchmark::http3::length(const vector <awh::http::h3::qpack::field_t> & fields) noexcept {
	// Результат работы функции - размер набора полей
	size_t result = 0;
	/**
	 * Выполняем перебор всех полей набора
	 */
	for(const auto & field : fields)
		// Суммируем длины названия и значения поля
		result += (field.name.size() + field.value.size());
	// Выводим размер набора полей
	return result;
}
/**
 * @brief Функция кодирования целого переменной длины QUIC (RFC 9000 §16)
 *
 * @param value кодируемое значение
 * @return      закодированное значение
 *
 */
string awh::benchmark::http3::varint(const uint64_t value) noexcept {
	// Результат работы функции - закодированное значение
	string result;
	// Если значение помещается в один октет
	if(value <= 0x3F)
		// Дописываем однооктетное представление
		result.push_back(static_cast <char> (value & 0xFF));
	// Если значение помещается в два октета
	else if(value <= 0x3FFF) {
		// Дописываем старший октет с признаком длины
		result.push_back(static_cast <char> (0x40 | ((value >> 8) & 0x3F)));
		// Дописываем младший октет значения
		result.push_back(static_cast <char> (value & 0xFF));
	// Если значение помещается в четыре октета
	} else if(value <= 0x3FFFFFFF) {
		// Дописываем старший октет с признаком длины
		result.push_back(static_cast <char> (0x80 | ((value >> 24) & 0x3F)));
		/**
		 * Выполняем дозапись оставшихся октетов значения
		 */
		for(int32_t shift = 16; shift >= 0; shift -= 8)
			// Дописываем очередной октет значения
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	// Если значение требует восьми октетов
	} else {
		// Дописываем старший октет с признаком длины
		result.push_back(static_cast <char> (0xC0 | ((value >> 56) & 0x3F)));
		/**
		 * Выполняем дозапись оставшихся октетов значения
		 */
		for(int32_t shift = 48; shift >= 0; shift -= 8)
			// Дописываем очередной октет значения
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	}
	// Выводим закодированное значение
	return result;
}
/**
 * @brief Функция сборки кадра HTTP/3
 *
 * @param type    тип кадра
 * @param payload полезная нагрузка кадра
 * @return        собранный кадр
 *
 */
string awh::benchmark::http3::frame(const uint64_t type, const string & payload) noexcept {
	// Заголовок собираемого кадра
	string result = (awh::benchmark::http3::varint(type) + awh::benchmark::http3::varint(payload.size()));
	// Резервируем память под кадр целиком
	result.reserve(result.size() + payload.size());
	// Дописываем полезную нагрузку кадра
	result += payload;
	// Выводим собранный кадр
	return result;
}
/**
 * @brief Функция сборки инструкции подтверждения секции полей (RFC 9204 §4.4.1)
 *
 * @param sid идентификатор потока, чья секция подтверждается
 * @return    собранная инструкция потока декодера
 *
 */
string awh::benchmark::http3::acknowledge(const uint64_t sid) noexcept {
	// Результат работы функции - собранная инструкция
	string result;
	// Если идентификатор потока помещается в семибитный префикс целиком
	if(sid < 0x7F){
		// Дописываем идентификатор потока прямо в префикс инструкции
		result.push_back(static_cast <char> (0x80 | static_cast <uint8_t> (sid)));
		// Выводим собранную инструкцию
		return result;
	}
	// Заполняем префикс единицами - признак продолжения значения
	result.push_back(static_cast <char> (0xFF));
	// Остаток идентификатора сверх префикса
	uint64_t rest = (sid - 0x7F);
	/**
	 * Выполняем дозапись остатка семибитными группами
	 */
	while(rest >= 128){
		// Дописываем очередную группу с признаком продолжения
		result.push_back(static_cast <char> ((rest & 0x7F) | 0x80));
		// Сдвигаем остаток на разобранную группу
		rest >>= 7;
	}
	// Дописываем последнюю группу остатка
	result.push_back(static_cast <char> (rest & 0x7F));
	// Выводим собранную инструкцию
	return result;
}
/**
 * @brief Функция сборки байтов управляющего потока пира
 *
 * @return байты управляющего потока: тип потока и кадр SETTINGS
 *
 */
string awh::benchmark::http3::control() noexcept {
	// Нагрузка кадра параметров соединения
	string options;
	// Дописываем ёмкость динамической таблицы QPACK
	options += (awh::benchmark::http3::varint(static_cast <uint64_t> (awh::http::h3::setting_t::QPACK_MAX_TABLE_CAPACITY)) +
	            awh::benchmark::http3::varint(awh::http::h3::proto::QPACK_TABLE_CAPACITY));
	// Дописываем число потоков, которым разрешено ожидать пополнения таблицы
	options += (awh::benchmark::http3::varint(static_cast <uint64_t> (awh::http::h3::setting_t::QPACK_BLOCKED_STREAMS)) +
	            awh::benchmark::http3::varint(awh::http::h3::proto::QPACK_BLOCKED_STREAMS));
	// Выводим байты управляющего потока
	return (awh::benchmark::http3::varint(static_cast <uint64_t> (awh::http::h3::unistream_t::CONTROL)) +
	        awh::benchmark::http3::frame(static_cast <uint64_t> (awh::http::h3::frame_t::SETTINGS), options));
}
