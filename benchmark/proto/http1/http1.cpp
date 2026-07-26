/**
 * @file: http1.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общее окружение бенчмарков протокола HTTP/1.x — объекты фреймворка
 *        и логирования, а также построение эталонных сообщений сценариев
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков протокола HTTP/1.x
 */
#include "http1.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция получения объекта фреймворка окружения бенчмарка
 *
 * @note Объекты окружения создаются при первом обращении: порядок
 *       статической инициализации между единицами трансляции не
 *       определён, а фреймворк зависит от таблиц чужих модулей
 *
 * @return объект фреймворка
 *
 */
const awh::fmk_t * awh::benchmark::http1::fmk() noexcept {
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
const awh::log_t * awh::benchmark::http1::log() noexcept {
	// Объект логирования окружения бенчмарка
	static awh::log_t result(awh::benchmark::http1::fmk());
	// Выводим объект логирования
	return &result;
}
/**
 * @brief Функция получения эталонного запроса без заголовков
 *
 * @details Нагрузка, на которой доминируют постоянные накладные расходы
 *          на сообщение: сброс состояния, стартовая строка и фазовые события
 *
 * @return эталонный запрос
 *
 */
const string & awh::benchmark::http1::tiny() noexcept {
	// Эталонный запрос без заголовков
	static const string result = "GET / HTTP/1.1\r\n\r\n";
	// Выводим эталонный запрос
	return result;
}
/**
 * @brief Функция получения эталонного запроса браузера
 *
 * @details Нагрузка, на которой доминируют заголовки: длинные значения
 *          сканируются крупноблочно, а имена и значения копятся в накопители
 *
 * @return эталонный запрос
 *
 */
const string & awh::benchmark::http1::typical() noexcept {
	// Эталонный запрос браузера с типовым набором заголовков
	static const string result =
		"GET /path/to/some/resource?query=value&other=thing HTTP/1.1\r\n"
		"Host: www.anyks.com\r\n"
		"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
		"Accept-Language: ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7\r\n"
		"Accept-Encoding: gzip, deflate, br\r\n"
		"Cookie: session=8f3a9c2b1e7d4f05; theme=dark; lang=ru; tracking=abcdef0123456789abcdef0123456789\r\n"
		"Connection: keep-alive\r\n"
		"Upgrade-Insecure-Requests: 1\r\n"
		"\r\n";
	// Выводим эталонный запрос
	return result;
}
/**
 * @brief Функция получения эталонного запроса с телом фиксированного размера
 *
 * @param size размер тела сообщения
 * @return     эталонный запрос
 *
 */
const string & awh::benchmark::http1::identity(const size_t size) noexcept {
	// Эталонный запрос с телом фиксированного размера
	static string result = "";
	// Если эталонный запрос ещё не построен
	if(result.empty()){
		// Формируем блок заголовков запроса
		result.append("POST /upload HTTP/1.1\r\nHost: www.anyks.com\r\nContent-Length: ").append(std::to_string(size)).append("\r\n\r\n");
		// Дописываем тело запроса
		result.append(size, 'x');
	}
	// Выводим эталонный запрос
	return result;
}
/**
 * @brief Функция получения эталонного запроса с телом в кодировке chunked
 *
 * @param size    размер тела сообщения
 * @param portion размер одного чанка
 * @return        эталонный запрос
 *
 */
const string & awh::benchmark::http1::chunked(const size_t size, const size_t portion) noexcept {
	// Эталонный запрос с телом в кодировке chunked
	static string result = "";
	// Если эталонный запрос ещё не построен
	if(result.empty()){
		// Формируем блок заголовков запроса
		result.append("POST /upload HTTP/1.1\r\nHost: www.anyks.com\r\nTransfer-Encoding: chunked\r\n\r\n");
		// Формируем данные одного чанка
		const string block(portion, 'y');
		// Текстовый буфер строки размера чанка
		char line[32];
		// Формируем строку размера чанка
		const int length = ::snprintf(line, sizeof(line), "%zX\r\n", portion);
		/**
		 * Дописываем чанки тела запроса до достижения требуемого размера
		 */
		for(size_t i = 0; i < (size / portion); i++)
			// Дописываем очередной чанк тела запроса
			result.append(line, static_cast <size_t> (length)).append(block).append("\r\n", 2);
		// Дописываем последний чанк и пустой блок трейлеров
		result.append("0\r\n\r\n");
	}
	// Выводим эталонный запрос
	return result;
}
