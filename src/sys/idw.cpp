/**
 * @file: idw.cpp
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

// Подключаем заголовочный файл
#include <sys/idw.hpp>

/**
 * id Метод генерирования идентификатора слова
 * @param word слово для генерации
 * @return     идентификатор слова
 */
uint64_t awh::IDW::id(const string & word) const noexcept {
	// Результат работы функции
	uint64_t result = 0;
	// Если слово передано
	if(!word.empty()){
		// Контрольная сумма
		uint64_t sum = 0;
		// Переходим по всему слову
		for(size_t i = 0; i < word.size(); i++){
			// Получаем позицию буквы в алфавите
			const uint64_t pos = this->_alphabet.find(word.at(i));
			// Генерируем вектор
			if(pos != string::npos)
				// Выполняем подсчёт суммы позиций
				sum += (this->_xs[i] * pos);
		}
		// Убираем колизии
		result = (sum % this->_modulus);
	}
	// Выводим результат
	return result;
}
/**
 * alphabet Метод установки алфавита
 * @param alphabet алфавит для установки
 */
void awh::IDW::alphabet(const string & alphabet) noexcept {
	// Если алфавит передан
	if(!alphabet.empty()){
		// Выполняем очистку список модулей
		this->_xs.clear();
		// Выполняем очистку ранее установленного алфавита
		this->_alphabet.clear();
		// Устанавливаем префикс
		this->_alphabet.append("¶");
		// Устанавливаем полученный алфавит
		this->_alphabet.append(alphabet);
		// Устанавливаем постфикс
		this->_alphabet.append("0123456789+*-_./\\");
		// Формируем диапазон значений
		for(uint8_t i = 0; i < MAX_LEN_WORD; i++)
			// Добавляем новое значение
			this->_xs.push_back(
				::modexp(
					static_cast <u_int> (this->_alphabet.size()),
					static_cast <uint64_t> (i),
					this->_modulus
				).toUnsignedLong()
			);
	}
}
/**
 * Конструктор
 */
awh::IDW::IDW() noexcept : _alphabet(""), _modulus(0) {
	// Выполняем расчёт модуля слова
	this->_modulus = (::pow(2, MAX_LEN_WORD + 1) - 1);
	// Выполняем установку алфавита
	this->alphabet(ALPHABET);
}
