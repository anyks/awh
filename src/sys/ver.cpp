/**
 * @file: ver.cpp
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

// Подключаем заголовочный файл
#include <sys/ver.hpp>

/**
 * dump Метод извлечения версии в виде числа
 * @return версия в виде числа
 */
uint32_t awh::Version::num() const noexcept {
	// Выводим версию в виде числа
	return this->_data;
}
/**
 * get Метод извлечения версии
 * @return версия в виде строки
 */
string awh::Version::get() const noexcept {
	// Результат работы функции
	string result = "";
	// Создаём временный контейнер числа
	uint8_t buffer[3];
	// Выполняем копирование данных в буфер
	memcpy(buffer, &this->_data, sizeof(buffer));
	// Переходим по всему массиву
	for(uint8_t i = 0; i < sizeof(buffer); i++){
		// Если строка уже существует, добавляем разделитель
		if(!result.empty())
			// Добавляем разделитель
			result.append(1, '.');
		// Добавляем октет в версию
		result.append(to_string(buffer[i]));
	}
	// Выводим результат
	return result;
}
/**
 * set Метод установки версии
 * @param ver устанавливаемая версия
 */
void awh::Version::set(const string & ver) noexcept {
	// Если версия передана
	if(!ver.empty()){
		// Создаём временный контейнер числа
		uint8_t buffer[3];
		// Позиция разделителя
		size_t start = 0, stop = 0, index = 0;
		// Выполняем поиск разделителя
		while((stop = ver.find('.', start)) != string::npos){
			// Извлекаем полученное число
			buffer[index] = std::stoi(ver.substr(start, stop));
			// Выполняем смещение
			start = (stop + 1);
			// Увеличиваем смещение индекса
			index++;
		}
		// Выполняем установку последнего октета
		buffer[index] = std::stoi(ver.substr(start));
		// Устанавливаем версию приложения
		memcpy(&this->_data, buffer, sizeof(buffer));
	}
}
/**
 * set Метод установки версии
 * @param ver устанавливаемая версия
 */
void awh::Version::set(const uint32_t ver) noexcept {
	// Устанавливаем версию в виде числа
	this->_data = ver;
}
/**
 * Оператор вывода версии в качестве числа
 * @return версия в качестве числа
 */
awh::Version::operator uint32_t() const noexcept {
	// Выводим данные версии в виде числа
	return this->num();
}
/**
 * Оператор вывода версии в качестве строки
 * @return версия в качестве строки
 */
awh::Version::operator std::string() const noexcept {
	// Выводим данные версии в виде строки
	return this->get();
}
/**
 * Оператор [<] сравнения версии
 * @param ver версия для сравнения
 * @return    результат сравнения
 */
bool awh::Version::operator < (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_data < ver._data);
}
/**
 * Оператор [>] сравнения версии
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator > (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_data > ver._data);
}
/**
 * Оператор [<=] сравнения версии
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator <= (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_data <= ver._data);
}
/**
 * Оператор [>=] сравнения версии
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator >= (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_data >= ver._data);
}
/**
 * Оператор [!=] сравнения версии
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator != (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_data != ver._data);
}
/**
 * Оператор [==] сравнения версии
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator == (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_data == ver._data);
}
/**
 * Оператор [=] присвоения версии
 * @param ver версия для присвоения
 * @return    текущий объект
 */
awh::Version & awh::Version::operator = (const string & ver) noexcept {
	// Устанавливаем версию
	this->set(ver);
	// Выводим результат
	return (* this);
}
/**
 * Оператор [=] присвоения версии
 * @param ver версия для присвоения
 * @return    текущий объект
 */
awh::Version & awh::Version::operator = (const uint32_t ver) noexcept {
	// Устанавливаем версию
	this->_data = ver;
	// Выводим результат
	return (* this);
}
/**
 * Оператор [>>] чтения из потока версии
 * @param is  поток для чтения
 * @param ver верси для присвоения
 */
istream & awh::operator >> (istream & is, ver_t & ver) noexcept {
	// Версия в текстовом виде
	string data = "";
	// Считываем версию
	is >> data;
	// Если версия передана
	if(!data.empty())
		// Устанавливаем версию
		ver.set(data);
	// Выводим результат
	return is;
}
/**
 * Оператор [<<] вывода в поток версии
 * @param os  поток куда нужно вывести данные
 * @param ver верси для присвоения
 */
ostream & awh::operator << (ostream & os, const ver_t & ver) noexcept {
	// Записываем в поток версию
	os << ver.get();
	// Выводим результат
	return os;
}
