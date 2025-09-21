/**
 * @file: ver.hpp
 * @date: 2024-01-27
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

#ifndef __AWH_VERSION__
#define __AWH_VERSION__

/**
 * Стандартная библиотека
 */
#include <string>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <sys/types.h>

/**
 * Разрешаем сборку под Windows
 */
#include "global.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс работы с версиями
	 *
	 */
	typedef class AWHSHARED_EXPORT Version {
		private:
			// Данные версии
			uint32_t _version;
		public:
			/**
			 * @brief Метод извлечения версии в виде числа
			 *
			 * @return версия в виде числа
			 */
			uint32_t num() const noexcept;
		public:
			/**
			 * @brief Метод извлечения версии в виде строки
			 *
			 * @param octets количество октетов
			 * @return      версия в виде строки
			 */
			string str(const uint8_t octets = 3) const noexcept;
		public:
			/**
			 * @brief Метод установки версии
			 *
			 * @param ver устанавливаемая версия
			 */
			void set(const uint32_t ver) noexcept;
			/**
			 * @brief Метод установки версии
			 *
			 * @param ver устанавливаемая версия
			 */
			void set(const string & ver) noexcept;
		public:
			/**
			 * @brief Оператор вывода версии в качестве числа
			 *
			 * @return версия в качестве числа
			 */
			operator uint32_t() const noexcept;
			/**
			 * @brief Оператор вывода версии в качестве строки
			 *
			 * @return версия в качестве строки
			 */
			operator string() const noexcept;
		public:
			/**
			 * @brief Оператор [<] сравнения версии
			 *
			 * @param ver версия для сравнения
			 * @return    результат сравнения
			 */
			bool operator < (const Version & ver) const noexcept;
			/**
			 * @brief Оператор [>] сравнения версии
			 *
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator > (const Version & ver) const noexcept;
			/**
			 * @brief Оператор [<=] сравнения версии
			 *
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator <= (const Version & ver) const noexcept;
			/**
			 * @brief Оператор [>=] сравнения версии
			 *
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator >= (const Version & ver) const noexcept;
			/**
			 * @brief Оператор [!=] сравнения версии
			 *
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator != (const Version & ver) const noexcept;
			/**
			 * @brief Оператор [==] сравнения версии
			 *
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator == (const Version & ver) const noexcept;
		public:
			/**
			 * @brief Оператор [=] присвоения версии
			 *
			 * @param ver версия для присвоения
			 * @return    текущий объект
			 */
			Version & operator = (const char * ver) noexcept;
			/**
			 * @brief Оператор [=] присвоения версии
			 *
			 * @param ver версия для присвоения
			 * @return    текущий объект
			 */
			Version & operator = (const string & ver) noexcept;
			/**
			 * @brief Оператор [=] присвоения версии
			 *
			 * @param ver версия для присвоения
			 * @return    текущий объект
			 */
			Version & operator = (const uint32_t ver) noexcept;
			/**
			 * @brief Оператор [=] присвоения версии
			 *
			 * @param ver версия для присвоения
			 * @return    текущий объект
			 */
			Version & operator = (const Version & ver) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			Version() noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param ver устанавливаемая версия
			 */
			Version(const char * ver) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param ver устанавливаемая версия
			 */
			Version(const string & ver) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param ver устанавливаемая версия
			 */
			Version(const uint32_t ver) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Version() noexcept {}
	} ver_t;
	/**
	 * @brief Оператор [>>] чтения из потока версии
	 *
	 * @param is  поток для чтения
	 * @param ver верси для присвоения
	 */
	AWHSHARED_EXPORT istream & operator >> (istream & is, ver_t & ver) noexcept;
	/**
	 * @brief Оператор [<<] вывода в поток версии
	 *
	 * @param os  поток куда нужно вывести данные
	 * @param ver верси для присвоения
	 */
	AWHSHARED_EXPORT ostream & operator << (ostream & os, const ver_t & ver) noexcept;
};

#endif // __AWH_VERSION__
