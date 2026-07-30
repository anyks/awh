/**
 * @file: nwt.hpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля определения типов сетевых адресов — класс Network_Types,
 *        распознающий во входной строке URL, домен, IP-адрес, MAC-адрес,
 *        e-mail или путь файловой системы и выполняющий разбор URL-адреса на составные части
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NWT__
#define __AWH_NWT__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>
#include <unordered_set>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/global.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Прототип класса работы с логами
	 *
	 */
	class Logging;

	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Структура списка параметров URL
	 *
	 * @details Отвечает на вопрос «что это такое», а не «разбери мне вот это».
	 *          На вход подаётся произвольная строка, на выходе - её тип и разобранные
	 *          части. Нужно это там, где вид записи заранее неизвестен: пользователь
	 *          ввёл строку в поле, и она может оказаться адресом сайта, голым доменом,
	 *          адресом IPv4 или IPv6, MAC-адресом или электронной почтой.
	 *
	 *          Этим модуль и отличается от `uri_t`: тот разбирает то, что уже признано
	 *          идентификатором ресурса, и делает это по правилам RFC 3986, а здесь
	 *          сперва определяется сам вид записи. Если разбирается заведомо известный
	 *          URI, обращаться следует к `uri_t` - он строже и полнее
	 *
	 * @note Распознавание домена опирается на список доменных зон, потому что отличить
	 *       домен от чего угодно другого можно только по зоне. Список встроенный, а
	 *       собственные зоны добавляются методами `zone()` и `zones()`: без них строка
	 *       вида `host.local` доменом признана не будет
	 *
	 * @par Пример: определение вида записи
	 * @code{.cpp}
	 * awh::nwt_t nwt(&log);
	 * // Добавляем собственную доменную зону, иначе она останется неизвестной
	 * nwt.zone("local");
	 * const awh::nwt_t::url_t url = nwt.parse("https://user:pass@anyks.com:443/path?query=1#anchor");
	 * // Здесь url.type равен URL, url.host - "anyks.com", url.port - 443,
	 * // url.domain - "com", а url.schema - "https"
	 * if(url.type == awh::nwt_t::types_t::URL)
	 *     connect(url.host, url.port);
	 * @endcode
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Network_Types {
		public:
			/**
			 * @brief Типы URL-адреса
			 *
			 * @note Отдельного значения для голого домена здесь нет: доменное имя -
			 *       частный случай адреса, и распознаётся оно как `URL`, а зона
			 *       верхнего уровня кладётся в поле `domain`. Так же поступает и адрес
			 *       со схемой: `URL` возвращается и для `anyks.com`, и для
			 *       `https://anyks.com/path`, различаются они заполненностью полей
			 *
			 * @note Значение `NONE` означает, что строку не удалось отнести ни к
			 *       одному из видов, и проверять его следует всегда: разбор в этом
			 *       случае полей не заполняет
			 *
			 */
			enum class types_t : uint8_t {
				NONE  = 0x00, // Тип не определён
				MAC   = 0x01, // MAC-адрес
				URL   = 0x02, // URL-адрес
				IPV4  = 0x03, // IPv4-адрес
				IPV6  = 0x04, // IPv6-адрес
				EMAIL = 0x05  // Электронная почта
			};
		public:
			/**
			 * @brief Класс URL-адреса
			 *
			 * @details Итог разбора. Какие поля заполнены, зависит от распознанного
			 *          вида записи, и полагаться на заполненность без проверки `type`
			 *          нельзя:
			 *
			 *          | Вид | Что заполняется |
			 *          |---|---|
			 *          | `URL` | `host`, `domain`, а также `schema`, `port`, `path`, `params`, `anchor`, `user`, `pass` - по мере наличия в записи |
			 *          | `IPV4`, `IPV6` | `host` |
			 *          | `MAC` | `host` |
			 *          | `EMAIL` | `user`, `host`, `domain` |
			 *          | `NONE` | ничего |
			 *
			 * @note Нулевой `port` означает, что порт в записи отсутствовал, а не что
			 *       он равен нулю: подстановкой порта по умолчанию для схемы модуль не
			 *       занимается - этим ведает `uri_t`
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ URL {
				public:
					types_t type;  // Тип URL-адреса
					uint32_t port; // Порт URL-адреса
					string uri;    // Полный URI-параметры
					string host;   // Хост URL-адреса
					string path;   // Путь URL-адреса
					string user;   // Ник пользователя (для электронной почты)
					string pass;   // Пароль пользователя
					string anchor; // Якорь URL-адреса
					string domain; // Домен верхнего уровня
					string params; // Параметры URL-адреса
					string schema; // Протокол URL-адреса
				public:
					/**
					 * @brief Оператор перемещения
					 *
					 * @param url параметры адреса
					 * @return    параметры URL-запроса
					 *
					 */
					URL & operator = (URL && url) noexcept;
					/**
					 * @brief Оператор присванивания
					 *
					 * @param url параметры адреса
					 * @return    параметры URL-запроса
					 *
					 */
					URL & operator = (const URL & url) noexcept;
				public:
					/**
					 * @brief Оператор сравнения
					 *
					 * @param url параметры адреса
					 * @return    результат сравнения
					 *
					 */
					bool operator == (const URL & url) const noexcept;
				public:
					/**
					 * @brief Конструктор перемещения
					 *
					 * @param url параметры адреса
					 *
					 */
					URL(URL && url) noexcept;
					/**
					 * @brief Конструктор копирования
					 *
					 * @param url параметры адреса
					 *
					 */
					URL(const URL & url) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					explicit URL() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~URL() noexcept = default;
			} url_t;
		private:
			// Список пользовательских доменных зон интернета
			unordered_set <string> _user;
		private:
			// Объект логера
			const Logging * _log;
		private:
			/**
			 * @brief Метод проверки, является ли домен верхнего уровня известной доменной зоной
			 *
			 * @param domain домен верхнего уровня для проверки
			 * @return       результат проверки (true, если зона известна)
			 *
			 */
			bool isZone(const string & domain) const noexcept;
		public:
			/**
			 * @brief Метод очистки результатов парсинга
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Метод установки пользовательской зоны
			 *
			 * @details Добавляет доменную зону к встроенному списку. Отличить домен от
			 *          произвольной строки можно только по зоне верхнего уровня,
			 *          поэтому запись с неизвестной зоной доменом признана не будет:
			 *          внутрисетевые имена вроде `host.local` или `service.internal`
			 *          требуют явного добавления
			 *
			 * @note Зоны накапливаются, а не заменяют друг друга: каждый вызов
			 *       добавляет одну. Заменить весь список целиком позволяет перегрузка
			 *       `zones()`
			 *
			 * @param zone пользовательская зона
			 *
			 */
			void zone(string_view zone) noexcept;
		public:
			/**
			 * @brief Метод извлечения списка пользовательских зон интернета
			 *
			 */
			const unordered_set <string> & zones() const noexcept;
			/**
			 * @brief Метод установки списка пользовательских зон
			 *
			 * @param zones список доменных зон интернета
			 *
			 */
			void zones(const unordered_set <string> & zones) noexcept;
		public:
			/**
			 * @brief Метод парсинга URI-строки
			 *
			 * @details Определяет вид записи и разбирает её на части. Метод постоянный
			 *          и состояния объекта не меняет, поэтому один разборщик можно
			 *          держать на всё приложение и обращаться к нему откуда угодно -
			 *          при условии, что список пользовательских зон уже задан
			 *
			 * @note Итог возвращается значением, а не ссылкой на внутреннее состояние:
			 *          результаты разных разборов друг друга не затирают
			 *
			 * @note Вид записи следует проверять всегда. Строка, не отнесённая ни к
			 *       одному виду, вернётся с типом `NONE` и пустыми полями, и обращение
			 *       к ним без проверки даст не ошибку, а тихо неверное поведение
			 *
			 * @param text текст для парсинга
			 * @return     параметры полученные в результате парсинга
			 *
			 */
			url_t parse(string_view text) const noexcept;
		public:
			/**
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 *
			 */
			void setLogger(const Logging * log) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Network_Types() noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 *
			 */
			explicit Network_Types(const Logging * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Network_Types() noexcept = default;
	} nwt_t;
};

#endif // __AWH_NWT__
