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
 * @brief Заголовочный файл модуля работы с версиями — класс Version для разбора,
 *        сравнения и представления версий в текстовом и числовом виде с поддержкой произвольного количества разрядов
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс работы с версиями
	 *
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
			 * @brief Метод извлечения версии в виде числа
			 *
			 * @return версия в виде числа
			 *
			 */
			uint32_t num() const noexcept;
		public:
			/**
			 * @brief Метод извлечения версии в виде строки
			 *
			 * @param octets количество октетов
			 * @return      версия в виде строки
			 *
			 */
			string str(const uint8_t octets = 3) const noexcept;
		public:
			/**
			 * @brief Метод установки версии
			 *
			 * @param version устанавливаемая версия
			 *
			 */
			void set(const uint32_t version) noexcept;
			/**
			 * @brief Метод установки версии
			 *
			 * @param version устанавливаемая версия
			 *
			 */
			void set(const string & version) noexcept;
		public:
			/**
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 *
			 */
			void setLogger(const log_t * log) noexcept;
		public:
			/**
			 * @brief Оператор вывода версии в качестве числа
			 *
			 * @return версия в качестве числа
			 *
			 */
			operator uint32_t() const noexcept;
			/**
			 * @brief Оператор вывода версии в качестве строки
			 *
			 * @return версия в качестве строки
			 *
			 */
			operator string() const noexcept;
		public:
			/**
			 * @brief Оператор [<] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 */
			bool operator < (const Version & version) const noexcept;
			/**
			 * @brief Оператор [>] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 */
			bool operator > (const Version & version) const noexcept;
			/**
			 * @brief Оператор [<=] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 */
			bool operator <= (const Version & version) const noexcept;
			/**
			 * @brief Оператор [>=] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 */
			bool operator >= (const Version & version) const noexcept;
			/**
			 * @brief Оператор [!=] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 */
			bool operator != (const Version & version) const noexcept;
			/**
			 * @brief Оператор [==] сравнения версии
			 *
			 * @param version версия для сравнения
			 * @return        результат сравнения
			 *
			 */
			bool operator == (const Version & version) const noexcept;
		public:
			/**
			 * @brief Оператор присваивания присвоения версии
			 *
			 * @param version версия для присвоения
			 * @return        текущий объект
			 *
			 */
			Version & operator = (const char * version) noexcept;
			/**
			 * @brief Оператор присваивания присвоения версии
			 *
			 * @param version версия для присвоения
			 * @return        текущий объект
			 *
			 */
			Version & operator = (const string & version) noexcept;
			/**
			 * @brief Оператор присваивания присвоения версии
			 *
			 * @param version версия для присвоения
			 * @return        текущий объект
			 *
			 */
			Version & operator = (const uint32_t version) noexcept;
			/**
			 * @brief Оператор присваивания присвоения версии
			 *
			 * @param version версия для присвоения
			 * @return        текущий объект
			 *
			 */
			Version & operator = (const Version & version) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Version() noexcept;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Version(const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param version устанавливаемая версия
			 *
			 */
			explicit Version(const char * version) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param version устанавливаемая версия
			 *
			 */
			explicit Version(const string & version) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param version устанавливаемая версия
			 *
			 */
			explicit Version(const uint32_t version) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Version() noexcept {}
	} version_t;
	/**
	 * @brief Оператор [>>] чтения из потока версии
	 *
	 * @param is      поток для чтения
	 * @param version версия для присвоения
	 *
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, version_t & version) noexcept;
	/**
	 * @brief Оператор [<<] вывода в поток версии
	 *
	 * @param os      поток куда нужно вывести данные
	 * @param version версия извлечения
	 *
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const version_t & version) noexcept;
};

#endif // __AWH_VERSION__
