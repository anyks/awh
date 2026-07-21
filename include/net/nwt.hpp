/**
 * @file: nwt.hpp
 * @date: 2025-10-25
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
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
	 */
	typedef class __AWH_SHARED_EXPORT__ Network_Types {
		public:
			/**
			 * @brief Типы URL-адреса
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
					 */
					URL & operator = (URL && url) noexcept;
					/**
					 * @brief Оператор присванивания
					 *
					 * @param url параметры адреса
					 * @return    параметры URL-запроса
					 */
					URL & operator = (const URL & url) noexcept;
				public:
					/**
					 * @brief Оператор сравнения
					 *
					 * @param url параметры адреса
					 * @return    результат сравнения
					 */
					bool operator == (const URL & url) const noexcept;
				public:
					/**
					 * @brief Конструктор перемещения
					 *
					 * @param url параметры адреса
					 */
					URL(URL && url) noexcept;
					/**
					 * @brief Конструктор копирования
					 *
					 * @param url параметры адреса
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
			 * @param zone пользовательская зона
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
			 */
			void zones(const unordered_set <string> & zones) noexcept;
		public:
			/**
			 * @brief Метод парсинга URI-строки
			 *
			 * @param text текст для парсинга
			 * @return     параметры полученные в результате парсинга
			 */
			url_t parse(string_view text) const noexcept;
		public:
			/**
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
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
