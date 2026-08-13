/**
 * @file: version.hpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля работы с версиями — класс Version для разбора,
 *        сравнения и представления версий в текстовом и числовом виде с поддержкой произвольного количества разрядов
 *
 * \~english
 * @brief Header file of the version handling module — the Version class for parsing,
 *        comparing and representing versions in text and numeric form with the support of an arbitrary number of parts
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_VERSION__
#define __AWH_VERSION__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>

/**
 * Подключаем заголовочный файл проекта
 */
#include "log.hpp"

/**
 * Разрешаем сборку под Windows
 */
#include "global.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс работы с версиями
	 *
	 * \~english
	 * @brief Class for working with versions
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Version {
		private:
			// Данные версии
			uint32_t _version;
		private:
			// Объект логера
			const log_t * _log;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения версии в виде числа
			 *
			 * @return версия в виде числа
			 *
			 * \~english
			 * @brief Method of getting the version as a number
			 * @return version as a number
			 *
			 * \~
			 */
			uint32_t num() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения версии в виде строки
			 *
			 * @param octets количество октетов
			 * @return      версия в виде строки
			 *
			 * \~english
			 * @brief Method of getting the version as a string
			 * @param octets number of octets
			 * @return      version as a string
			 *
			 * \~
			 */
			string str(const uint8_t octets = 3) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки версии
			 *
			 * @param version устанавливаемая версия
			 *
			 *
			 * \~english
			 * @brief Method of setting the version
			 * @param version version to set
			 *
			 * \~
			 */
			void set(const uint32_t version) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки версии
			 *
			 * @param version устанавливаемая версия
			 *
			 *
			 * \~english
			 * @brief Method of setting the version
			 * @param version version to set
			 *
			 * \~
			 */
			void set(const string & version) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 *
			 * \~english
			 * @brief Method of setting the logging object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			void setLogger(const log_t * log) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор вывода версии в качестве числа
			 *
			 * @return версия в качестве числа
			 *
			 * \~english
			 * @brief Operator of outputting the version as a number
			 * @return version as a number
			 *
			 * \~
			 */
			operator uint32_t() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор вывода версии в качестве строки
			 *
			 * @return версия в качестве строки
			 *
			 * \~english
			 * @brief Operator of outputting the version as a string
			 * @return version as a string
			 *
			 * \~
			 */
			operator string() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор [<] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 * \~english
			 * @brief Operator [<] of the comparison of a version
			 * @param version version to compare with
			 * @return        result of the comparison
			 *
			 * \~
			 */
			bool operator < (const Version & version) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [>] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 * \~english
			 * @brief Operator [>] of the comparison of a version
			 * @param version version to compare with
			 * @return        result of the comparison
			 *
			 * \~
			 */
			bool operator > (const Version & version) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [<=] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 * \~english
			 * @brief Operator [<=] of the comparison of a version
			 * @param version version to compare with
			 * @return        result of the comparison
			 *
			 * \~
			 */
			bool operator <= (const Version & version) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [>=] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 * \~english
			 * @brief Operator [>=] of the comparison of a version
			 * @param version version to compare with
			 * @return        result of the comparison
			 *
			 * \~
			 */
			bool operator >= (const Version & version) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [!=] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 * \~english
			 * @brief Operator [!=] of the comparison of a version
			 * @param version version to compare with
			 * @return        result of the comparison
			 *
			 * \~
			 */
			bool operator != (const Version & version) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор [==] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 * \~english
			 * @brief Operator [==] of the comparison of a version
			 * @param version version to compare with
			 * @return        result of the comparison
			 *
			 * \~
			 */
			bool operator == (const Version & version) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор присваивания присвоения версии
			 *
			 * @param version версия для присвоения
			 * @return        текущий объект
			 *
			 *
			 * \~english
			 * @brief Assignment operator of a version
			 * @param version version to assign
			 * @return        the current object
			 *
			 * \~
			 */
			Version & operator = (const char * version) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания присвоения версии
			 *
			 * @param version версия для присвоения
			 * @return        текущий объект
			 *
			 *
			 * \~english
			 * @brief Assignment operator of a version
			 * @param version version to assign
			 * @return        the current object
			 *
			 * \~
			 */
			Version & operator = (const string & version) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания присвоения версии
			 *
			 * @param version версия для присвоения
			 * @return        текущий объект
			 *
			 *
			 * \~english
			 * @brief Assignment operator of a version
			 * @param version version to assign
			 * @return        the current object
			 *
			 * \~
			 */
			Version & operator = (const uint32_t version) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания присвоения версии
			 *
			 * @param version версия для присвоения
			 * @return        текущий объект
			 *
			 *
			 * \~english
			 * @brief Assignment operator of a version
			 * @param version version to assign
			 * @return        the current object
			 *
			 * \~
			 */
			Version & operator = (const Version & version) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Version() noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Version(const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param version устанавливаемая версия
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param version version to set
			 *
			 * \~
			 */
			explicit Version(const char * version) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param version устанавливаемая версия
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param version version to set
			 *
			 * \~
			 */
			explicit Version(const string & version) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param version устанавливаемая версия
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param version version to set
			 *
			 * \~
			 */
			explicit Version(const uint32_t version) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Version() noexcept {}
	} version_t;
	/**
	 * \~russian
	 * @brief Оператор [>>] чтения из потока версии
	 *
	 * @param is      поток для чтения
	 * @param version версия для присвоения
	 *
	 * \~english
	 * @brief Operator [>>] of reading a version from a stream
	 * @param is      stream to read from
	 * @param version version to assign
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, version_t & version) noexcept;
	/**
	 * \~russian
	 * @brief Оператор [<<] вывода в поток версии
	 *
	 * @param os      поток куда нужно вывести данные
	 * @param version версия извлечения
	 *
	 * \~english
	 * @brief Operator [<<] of outputting a version to a stream
	 * @param os      stream to output the data to
	 * @param version version being output
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const version_t & version) noexcept;
};

#endif // __AWH_VERSION__
