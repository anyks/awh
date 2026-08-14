/**
 * @file http2.cpp
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
 * @brief Общее окружение бенчмарков протокола HTTP/2 — объекты окружения,
 *        эталонные наборы заголовков и сборка потока кадров соединения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

/**
 * Подключаем заголовочный файл бенчмарков протокола HTTP/2
 */
#include "http2.hpp"

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
const awh::fmk_t * awh::benchmark::http2::fmk() noexcept {
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
const awh::log_t * awh::benchmark::http2::log() noexcept {
	// Объект логирования окружения бенчмарка
	static awh::log_t result(awh::benchmark::http2::fmk());
	// Отключаем вывод логов: часть сценариев намеренно упирается в лимиты
	result.level(awh::log_t::level_t::NONE);
	// Выводим объект логирования
	return &result;
}
/**
 * @brief Функция получения эталонного набора заголовков запроса браузера
 *
 * @param index номер запроса в последовательности
 * @return      набор заголовков запроса
 *
 */
vector <awh::http::h2::hpack::field_t> awh::benchmark::http2::request(const size_t index) noexcept {
	// Результат работы функции - набор заголовков запроса
	vector <awh::http::h2::hpack::field_t> result;
	// Резервируем память под набор заголовков
	result.reserve(12);
	// Дописываем псевдо-заголовок метода запроса
	result.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	result.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок авторитета запроса
	result.emplace_back(":authority", "www.example.com");
	/**
	 * Путь делаем переменной частью запроса: постоянный путь целиком уходил бы
	 * в динамическую таблицу и сжатие вышло бы недостижимо оптимистичным
	 */
	result.emplace_back(":path", ("/assets/bundle-" + std::to_string(index % 64) + ".js"));
	// Дописываем заголовок принимаемых типов содержимого
	result.emplace_back("accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");
	// Дописываем заголовок принимаемых кодировок содержимого
	result.emplace_back("accept-encoding", "gzip, deflate, br, zstd");
	// Дописываем заголовок принимаемых языков
	result.emplace_back("accept-language", "ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7");
	// Дописываем заголовок клиентского приложения
	result.emplace_back("user-agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36");
	// Дописываем заголовок источника перехода
	result.emplace_back("referer", "https://www.example.com/catalog/index.html");
	// Дописываем заголовок печенья сессии
	result.emplace_back("cookie", "session=7f3c1a9b2e4d6f8a0c2e4b6d8f1a3c5e; theme=dark; lang=ru");
	// Дописываем заголовок политики кеширования
	result.emplace_back("cache-control", "no-cache");
	// Выводим набор заголовков запроса
	return result;
}
/**
 * @brief Функция получения эталонного набора заголовков ответа сервера
 *
 * @param index номер ответа в последовательности
 * @return      набор заголовков ответа
 *
 */
vector <awh::http::h2::hpack::field_t> awh::benchmark::http2::response(const size_t index) noexcept {
	// Результат работы функции - набор заголовков ответа
	vector <awh::http::h2::hpack::field_t> result;
	// Резервируем память под набор заголовков
	result.reserve(8);
	// Дописываем псевдо-заголовок статуса ответа
	result.emplace_back(":status", "200");
	// Дописываем заголовок типа содержимого
	result.emplace_back("content-type", "application/javascript; charset=utf-8");
	// Дописываем заголовок длины содержимого: она обязана совпасть с фактическим телом
	result.emplace_back("content-length", std::to_string(awh::benchmark::http2::payload().size()));
	// Дописываем заголовок политики кеширования
	result.emplace_back("cache-control", "public, max-age=31536000, immutable");
	// Дописываем заголовок метки версии содержимого
	result.emplace_back("etag", ("\"" + std::to_string(index) + "-a1b2c3d4\""));
	// Дописываем заголовок сервера
	result.emplace_back("server", "awh");
	// Выводим набор заголовков ответа
	return result;
}
/**
 * @brief Функция получения эталонного тела ответа
 *
 * @return тело ответа
 *
 */
const string & awh::benchmark::http2::payload() noexcept {
	// Эталонное тело ответа
	static const string result(64, 'r');
	// Выводим тело ответа
	return result;
}
/**
 * @brief Функция подсчёта размера набора заголовков до сжатия
 *
 * @param fields набор заголовков
 * @return       размер набора заголовков в октетах
 *
 */
size_t awh::benchmark::http2::length(const vector <awh::http::h2::hpack::field_t> & fields) noexcept {
	// Результат работы функции - размер набора заголовков
	size_t result = 0;
	/**
	 * Выполняем перебор всех заголовков набора
	 */
	for(const auto & field : fields)
		// Суммируем длины названия и значения заголовка
		result += (field.name.size() + field.value.size());
	// Выводим размер набора заголовков
	return result;
}
/**
 * @brief Функция сборки кадра HTTP/2
 *
 * @param type    тип кадра
 * @param flags   флаги кадра
 * @param sid     идентификатор потока
 * @param payload полезная нагрузка кадра
 * @return        собранный кадр
 *
 */
string awh::benchmark::http2::frame(const uint8_t type, const uint8_t flags, const uint32_t sid, const string & payload) noexcept {
	// Результат работы функции - собранный кадр
	string result;
	// Резервируем память под кадр целиком
	result.reserve(9 + payload.size());
	// Дописываем 24-битную длину полезной нагрузки
	result.push_back(static_cast <char> ((payload.size() >> 16) & 0xFF));
	result.push_back(static_cast <char> ((payload.size() >> 8) & 0xFF));
	result.push_back(static_cast <char> (payload.size() & 0xFF));
	// Дописываем тип кадра
	result.push_back(static_cast <char> (type));
	// Дописываем флаги кадра
	result.push_back(static_cast <char> (flags));
	// Дописываем идентификатор потока
	result.push_back(static_cast <char> ((sid >> 24) & 0x7F));
	result.push_back(static_cast <char> ((sid >> 16) & 0xFF));
	result.push_back(static_cast <char> ((sid >> 8) & 0xFF));
	result.push_back(static_cast <char> (sid & 0xFF));
	// Дописываем полезную нагрузку
	result += payload;
	// Выводим собранный кадр
	return result;
}
/**
 * @brief Функция сборки преамбулы соединения клиента
 *
 * @return преамбула соединения: magic-строка и кадр SETTINGS
 *
 */
string awh::benchmark::http2::preface() noexcept {
	// Результат работы функции - преамбула соединения
	string result(awh::http::h2::proto::PREFACE);
	// Дописываем кадр SETTINGS без параметров
	result += awh::benchmark::http2::frame(0x04, 0x00, 0, "");
	// Выводим преамбулу соединения
	return result;
}
