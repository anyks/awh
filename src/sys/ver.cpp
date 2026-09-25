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
#include <stdexcept>
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
	/**
	 * Версия хранится в сетевом порядке байт (htonl), поэтому обратное
	 * преобразование выполняется через ntohl. Прежний вызов htons усекал
	 * значение до 16 бит и возвращал неверное число
	 */
	return ntohl(this->_version);
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
		// Получаем текущее значение версии в порядке байт узла (htons усекал значение до 16 бит)
		const uint32_t version = ntohl(this->_version);
		// Нормализуем количество октетов (1..4) в локальной переменной (изменение константного параметра недопустимо)
		uint8_t count = octets;
		// Если количество октетов не указанно
		if(count == 0)
			// Выполняем корректировку
			count = 1;
		// Если октетов больше 4-х
		else if(count > 4)
			// Выполняем корректировку
			count = 4;
		// Переходим по всему массиву
		for(uint8_t i = 0; i < count; i++){
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
			::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "ERROR! %s\n\n", error.what());
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
	this->_version = htonl(ver);
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
			// Временное значение версии (фиксируется в объекте только при полном успехе разбора)
			uint32_t result = 0;
			// Позиция разделителя
			size_t start = 0, stop = 0, index = 0;
			/**
			 * @brief Функция извлечения и проверки одного октета версии
			 *
			 * Числовой префикс октета разбирается как и прежде через stoi, поэтому
			 * хвосты вида "3-beta" по-прежнему дают 3 (совместимость с AWH 4).
			 * Отрицательные значения и значения больше 255 отвергаются: раньше
			 * они молча заворачивались (-1 -> 255, 300 -> 44)
			 *
			 * @param octet строковое представление октета
			 * @return      числовое значение октета [0..255]
			 */
			auto parseFn = [](const string & octet) -> uint8_t {
				// Извлекаем числовое значение октета (пустой или нечисловой октет вызывает исключение)
				const int value = ::stoi(octet);
				// Если значение октета выходит за пределы диапазона [0..255]
				if((value < 0) || (value > 255))
					// Сообщаем об ошибке диапазона октета
					throw out_of_range("version octet out of range [0..255]: \"" + octet + "\"");
				// Выводим числовое значение октета
				return static_cast <uint8_t> (value);
			};
			/**
			 * Выполняем поиск разделителя
			 */
			while((stop = ver.find('.', start)) != string::npos){
				// Извлекаем полученное число (длина подстроки считается от начала октета)
				reinterpret_cast <uint8_t *> (&result)[index] = parseFn(ver.substr(start, stop - start));
				// Выполняем смещение
				start = (stop + 1);
				// Увеличиваем смещение индекса
				index++;
				// Если индекс достиг последнего октета, выходим (последний октет записывается после цикла)
				if(index >= 3)
					// Выходим из цикла
					break;
			}
			/**
			 * Если после третьего разделителя остались ещё разделители, октетов больше четырёх.
			 * Раньше пятый октет записывался за пределы 32-битного значения
			 */
			if(ver.find('.', start) != string::npos)
				// Сообщаем об ошибке количества октетов
				throw out_of_range("version has more than four octets: \"" + ver + "\"");
			// Выполняем установку последнего октета
			reinterpret_cast <uint8_t *> (&result)[index] = parseFn(ver.substr(start));
			// Переводим число в Big-Endian и фиксируем результат (при ошибке прежнее значение сохраняется)
			this->_version = htonl(result);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
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
	// Если версия передана (построение строки из нулевого указателя приводит к падению)
	if(ver != nullptr)
		// Устанавливаем версию
		this->set(string(ver));
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
awh::Version & awh::Version::operator = (const Version & ver) noexcept {
	// Устанавливаем версию
	this->_version = ver._version;
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
	// Если версия передана (построение строки из нулевого указателя приводит к падению)
	if(ver != nullptr)
		// Устанавливаем версию
		this->set(string(ver));
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
	string version = "";
	// Считываем версию
	is >> version;
	// Если версия передана
	if(!version.empty())
		// Устанавливаем версию
		ver = version;
	// Выводим результат
	return is;
}
/**
 * @brief Оператор [<<] вывода в поток версии
 *
 * @param os  поток куда нужно вывести данные
 * @param ver верси извлечения
 */
ostream & awh::operator << (ostream & os, const ver_t & ver) noexcept {
	// Записываем в поток версию
	os << ver.str();
	// Выводим результат
	return os;
}
