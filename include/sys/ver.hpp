/**
 * @file: ver.hpp
 * @date: 2023-12-21
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_VERSION__
#define __AWH_VERSION__

/**
 * Стандартные модули
 */
#include <string>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <sys/types.h>

/**
 * Разрешаем сборку под Windows
 */
#include <sys/global.hpp>

// Объявляем пространство имен
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Version Класс работы с версиями
	 */
	typedef class AWHSHARED_EXPORT Version {
		private:
			// Данные версии
			uint32_t _data;
		public:
			/**
			 * dump Метод извлечения версии в виде числа
			 * @return версия в виде числа
			 */
			uint32_t num() const noexcept;
		public:
			/**
			 * get Метод извлечения версии
			 * @return версия в виде строки
			 */
			string get() const noexcept;
			/**
			 * set Метод установки версии
			 * @param ver устанавливаемая версия
			 */
			void set(const string & ver) noexcept;
			/**
			 * set Метод установки версии
			 * @param ver устанавливаемая версия
			 */
			void set(const uint32_t ver) noexcept;
		public:
			/**
			 * Оператор вывода версии в качестве числа
			 * @return версия в качестве числа
			 */
			operator uint32_t() const noexcept;
			/**
			 * Оператор вывода версии в качестве строки
			 * @return версия в качестве строки
			 */
			operator std::string() const noexcept;
		public:
			/**
			 * Оператор [<] сравнения версии
			 * @param ver версия для сравнения
			 * @return    результат сравнения
			 */
			bool operator < (const Version & ver) const noexcept;
			/**
			 * Оператор [>] сравнения версии
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator > (const Version & ver) const noexcept;
			/**
			 * Оператор [<=] сравнения версии
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator <= (const Version & ver) const noexcept;
			/**
			 * Оператор [>=] сравнения версии
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator >= (const Version & ver) const noexcept;
			/**
			 * Оператор [!=] сравнения версии
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator != (const Version & ver) const noexcept;
			/**
			 * Оператор [==] сравнения версии
			 * @param ver версия для сравнения
			 * @return     результат сравнения
			 */
			bool operator == (const Version & ver) const noexcept;
		public:
			/**
			 * Оператор [=] присвоения версии
			 * @param ver версия для присвоения
			 * @return    текущий объект
			 */
			Version & operator = (const string & ver) noexcept;
			/**
			 * Оператор [=] присвоения версии
			 * @param ver версия для присвоения
			 * @return    текущий объект
			 */
			Version & operator = (const uint32_t ver) noexcept;
		public:
			/**
			 * Version Конструктор
			 */
			Version() noexcept : _data(0) {}
			/**
			 * Version Конструктор
			 * @param ver устанавливаемая версия
			 */
			Version(const string & ver) noexcept : _data(0) {
				// Устанавливаем версию
				this->set(ver);
			}
			/**
			 * Version Конструктор
			 * @param ver устанавливаемая версия
			 */
			Version(const uint32_t ver) noexcept : _data(0) {
				// Устанавливаем версию
				this->set(ver);
			}
	} ver_t;
	/**
	 * Оператор [>>] чтения из потока версии
	 * @param is  поток для чтения
	 * @param ver верси для присвоения
	 */
	istream & operator >> (istream & is, ver_t & ver) noexcept;
	/**
	 * Оператор [<<] вывода в поток версии
	 * @param os  поток куда нужно вывести данные
	 * @param ver верси для присвоения
	 */
	ostream & operator << (ostream & os, const ver_t & ver) noexcept;
};

#endif // __AWH_VERSION__
