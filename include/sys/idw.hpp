/**
 * @file: idw.hpp
 * @date: 2023-08-11
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_IDW__
#define __AWH_IDW__

/**
 * Стандартная библиотека
 */
#include <cmath>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include <bigint/BigIntegerLibrary.hh>

// Объявляем пространство имен
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * IDW Класс работы с генератором идентификаторов слов
	 */
	typedef class IDW {
		private:
			// Максимальный размер слова
			static constexpr uint8_t MAX_LEN_WORD = 35;
			// Алфавит символов для генерации ключа
			static constexpr const char * ALPHABET = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		private:
			// Алфавит ключей
			string _alphabet;
		private:
			// Модуль вектора
			uint64_t _modulus;
		private:
			// Список модулей для каждой буквы алфавита
			vector <uint64_t> _xs;
		public:
			/**
			 * id Метод генерирования идентификатора слова
			 * @param word слово для генерации
			 * @return     идентификатор слова
			 */
			uint64_t id(const string & word) const noexcept;
		public:
			/**
			 * alphabet Метод установки алфавита
			 * @param alphabet алфавит для установки
			 */
			void alphabet(const string & alphabet) noexcept;
		public:
			/**
			 * Конструктор
			 */
			IDW() noexcept;
	} idw_t;
};

#endif // __AWH_IDW__
