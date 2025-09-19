/**
 * @file: ver.cpp
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

/**
 * Подключаем заголовочный файл
 */
#include <sys/os.hpp>
#include <sys/ver.hpp>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Стандартная библиотека
	 */
	#include <winsock2.h>
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Стандартная библиотека
	 */
	#include <arpa/inet.h>
#endif

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод извлечения версии в виде числа
 *
 * @return версия в виде числа
 */
uint32_t awh::Version::num() const noexcept {
	// Выводим версию в виде числа
	return this->_version;
}
/**
 * @brief Метод извлечения версии в виде строки
 *
 * @param octets количество октетов
 * @return       версия в виде строки
 */
string awh::Version::str(const uint8_t octets) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Получаем текущее значение версии в Lite-Endian
		const uint32_t version = htons(this->_version);
		// Если количество октетов не указанно
		if(octets == 0)
			// Выполняем корректировку
			const_cast <uint8_t &> (octets) = 1;
		// Если октетов больше 4-х
		else if(octets > 4)
			// Выполняем корректировку
			const_cast <uint8_t &> (octets) = 4;
		// Переходим по всему массиву
		for(uint8_t i = 0; i < octets; i++){
			// Если строка уже существует, добавляем разделитель
			if(!result.empty())
				// Добавляем разделитель
				result.append(1, '.');
			// Добавляем октет в версию
			result.append(std::to_string(reinterpret_cast <const uint8_t *> (&version)[i]));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки версии
 *
 * @param ver устанавливаемая версия
 */
void awh::Version::set(const uint32_t ver) noexcept {
	// Устанавливаем версию в виде числа
	this->_version = ver;
}
/**
 * @brief Метод установки версии
 *
 * @param ver устанавливаемая версия
 */
void awh::Version::set(const string & ver) noexcept {
	// Если версия передана
	if(!ver.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем сброс версии
			this->_version = 0;
			// Позиция разделителя
			size_t start = 0, stop = 0, index = 0;
			/**
			 * Выполняем поиск разделителя
			 */
			while((stop = ver.find('.', start)) != string::npos){
				// Извлекаем полученное число
				reinterpret_cast <uint8_t *> (&this->_version)[index] = static_cast <uint8_t> (::stoi(ver.substr(start, stop)));
				// Выполняем смещение
				start = (stop + 1);
				// Увеличиваем смещение индекса
				index++;
				// Если индекс перешёл диапазон, выходим
				if(index > 3)
					// Выходим из цикла
					break;
			}
			// Выполняем установку последнего октета
			reinterpret_cast <uint8_t *> (&this->_version)[index] = static_cast <uint8_t> (::stoi(ver.substr(start)));
			// Переводим число в Big-Endian
			this->_version = htonl(this->_version);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n\n", error.what());
			#endif
		}
	}
}
/**
 * @brief Оператор вывода версии в качестве числа
 *
 * @return версия в качестве числа
 */
awh::Version::operator uint32_t() const noexcept {
	// Выводим данные версии в виде числа
	return this->num();
}
/**
 * @brief Оператор вывода версии в качестве строки
 *
 * @return версия в качестве строки
 */
awh::Version::operator string() const noexcept {
	// Выводим данные версии в виде строки
	return this->str();
}
/**
 * @brief Оператор [<] сравнения версии
 *
 * @param ver версия для сравнения
 * @return    результат сравнения
 */
bool awh::Version::operator < (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_version < ver._version);
}
/**
 * @brief Оператор [>] сравнения версии
 *
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator > (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_version > ver._version);
}
/**
 * @brief Оператор [<=] сравнения версии
 *
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator <= (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_version <= ver._version);
}
/**
 * @brief Оператор [>=] сравнения версии
 *
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator >= (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_version >= ver._version);
}
/**
 * @brief Оператор [!=] сравнения версии
 *
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator != (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_version != ver._version);
}
/**
 * @brief Оператор [==] сравнения версии
 *
 * @param ver версия для сравнения
 * @return     результат сравнения
 */
bool awh::Version::operator == (const ver_t & ver) const noexcept {
	// Выводим результат
	return (this->_version == ver._version);
}
/**
 * @brief Оператор [=] присвоения версии
 *
 * @param ver версия для присвоения
 * @return    текущий объект
 */
awh::Version & awh::Version::operator = (const char * ver) noexcept {
	// Устанавливаем версию
	this->set(ver);
	// Выводим результат
	return (* this);
}
/**
 * @brief Оператор [=] присвоения версии
 *
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
 * @brief Оператор [=] присвоения версии
 *
 * @param ver версия для присвоения
 * @return    текущий объект
 */
awh::Version & awh::Version::operator = (const uint32_t ver) noexcept {
	// Устанавливаем версию
	this->_version = ver;
	// Выводим результат
	return (* this);
}
/**
 * @brief Конструктор
 *
 */
awh::Version::Version() noexcept : _version(0) {}
/**
 * @brief Конструктор
 *
 * @param ver устанавливаемая версия
 */
awh::Version::Version(const char * ver) noexcept : _version(0) {
	// Устанавливаем версию
	this->set(ver);
}
/**
 * @brief Конструктор
 *
 * @param ver устанавливаемая версия
 */
awh::Version::Version(const string & ver) noexcept : _version(0) {
	// Устанавливаем версию
	this->set(ver);
}
/**
 * @brief Конструктор
 *
 * @param ver устанавливаемая версия
 */
awh::Version::Version(const uint32_t ver) noexcept : _version(0) {
	// Устанавливаем версию
	this->set(ver);
}
/**
 * @brief Оператор [>>] чтения из потока версии
 *
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
 * @brief Оператор [<<] вывода в поток версии
 *
 * @param os  поток куда нужно вывести данные
 * @param ver верси для присвоения
 */
ostream & awh::operator << (ostream & os, const ver_t & ver) noexcept {
	// Записываем в поток версию
	os << ver.str();
	// Выводим результат
	return os;
}
