/**
 * @file: version.cpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля работы с версиями — разбор строкового представления версии,
 *        сравнение версий и обратное преобразование в текстовый и числовой вид
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/types.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/os.hpp>
#include <sys/version.hpp>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системный заголовочный файл
	 */
	#include <arpa/inet.h>
#endif

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод извлечения версии в виде числа
 *
 * @return версия в виде числа
 *
 */
uint32_t awh::Version::num() const noexcept {
	// Возвращаем версию в виде числа
	return ntohl(this->_version);
}
/**
 * @brief Метод извлечения версии в виде строки
 *
 * @param octets количество октетов
 * @return       версия в виде строки
 *
 */
string awh::Version::str(const uint8_t octets) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Получаем текущее значение версии в host-order
		const uint32_t version = ntohl(this->_version);
		// Нормализуем количество октетов (1..4) в локальной переменной
		uint8_t count = octets;
		// Если количество октетов не указанно
		if(count == 0)
			// Выполняем корректировку
			count = 1;
		// Если октетов больше 4-х
		else if(count > 4)
			// Выполняем корректировку
			count = 4;
		/**
		 * Переходим по всему массиву
		 */
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
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(octets), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки версии
 *
 * @param version устанавливаемая версия
 *
 */
void awh::Version::set(const uint32_t version) noexcept {
	// Устанавливаем версию в виде числа
	this->_version = htonl(version);
}
/**
 * @brief Метод установки версии
 *
 * @param version устанавливаемая версия
 *
 */
void awh::Version::set(const string & version) noexcept {
	// Если версия передана
	if(!version.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Временное значение версии (фиксируется в объекте только при полном успехе)
			uint32_t result = 0;
			// Позиция разделителя
			size_t start = 0, stop = 0, index = 0;
			/**
			 * @brief Функция извлечения и валидации одного октета версии
			 *
			 * @param octet строковое представление октета
			 * @return      числовое значение октета [0..255]
			 *
			 */
			auto parse = [](const string & octet) -> uint8_t {
				// Если октет пустой или содержит нецифровые символы
				if(octet.empty() || (octet.find_first_not_of("0123456789") != string::npos))
					// Сообщаем об ошибке формата октета
					throw invalid_argument("invalid version octet: \"" + octet + "\"");
				// Извлекаем числовое значение октета
				const int value = ::stoi(octet);
				// Если значение октета выходит за пределы диапазона [0..255]
				if(value > 255)
					// Сообщаем об ошибке диапазона октета
					throw out_of_range("version octet out of range [0..255]: \"" + octet + "\"");
				// Возвращаем числовое значение октета
				return static_cast <uint8_t> (value);
			};
			/**
			 * Выполняем поиск разделителя
			 */
			while((stop = version.find('.', start)) != string::npos){
				// Извлекаем полученное число
				reinterpret_cast <uint8_t *> (&result)[index] = parse(version.substr(start, stop - start));
				// Выполняем смещение
				start = (stop + 1);
				// Увеличиваем смещение индекса
				index++;
				// Если индекс достиг предела (3, т.к. последний запишем после цикла), выходим
				if(index >= 3)
					// Выходим из цикла
					break;
			}
			// Если индекс в допустимых пределах
			if(index < 4)
				// Выполняем установку последнего октета (если индекс в допустимых пределах)
				reinterpret_cast <uint8_t *> (&result)[index] = parse(version.substr(start));
			// Переводим число в сетевой порядок байт и фиксируем результат
			this->_version = htonl(result);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(version), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
}
/**
 * @brief Метод установки объекта логирования
 *
 * @param log объект работы с логами
 *
 */
void awh::Version::setLogger(const log_t * log) noexcept {
	// Устанавливаем объект логирования
	this->_log = log;
}
/**
 * @brief Оператор вывода версии в качестве числа
 *
 * @return версия в качестве числа
 *
 */
awh::Version::operator uint32_t() const noexcept {
	// Возвращаем данные версии в виде числа
	return this->num();
}
/**
 * @brief Оператор вывода версии в качестве строки
 *
 * @return версия в качестве строки
 *
 */
awh::Version::operator string() const noexcept {
	// Возвращаем данные версии в виде строки
	return this->str();
}
/**
 * @brief Оператор [<] сравнения версии
 *
 * @param version версия для сравнения
 * @return        результат сравнения
 *
 */
bool awh::Version::operator < (const version_t & version) const noexcept {
	// Возвращаем результат
	return (this->_version < version._version);
}
/**
 * @brief Оператор [>] сравнения версии
 *
 * @param version версия для сравнения
 * @return        результат сравнения
 *
 */
bool awh::Version::operator > (const version_t & version) const noexcept {
	// Возвращаем результат
	return (this->_version > version._version);
}
/**
 * @brief Оператор [<=] сравнения версии
 *
 * @param version версия для сравнения
 * @return        результат сравнения
 *
 */
bool awh::Version::operator <= (const version_t & version) const noexcept {
	// Возвращаем результат
	return (this->_version <= version._version);
}
/**
 * @brief Оператор [>=] сравнения версии
 *
 * @param version версия для сравнения
 * @return        результат сравнения
 *
 */
bool awh::Version::operator >= (const version_t & version) const noexcept {
	// Возвращаем результат
	return (this->_version >= version._version);
}
/**
 * @brief Оператор [!=] сравнения версии
 *
 * @param version версия для сравнения
 * @return        результат сравнения
 *
 */
bool awh::Version::operator != (const version_t & version) const noexcept {
	// Возвращаем результат
	return (this->_version != version._version);
}
/**
 * @brief Оператор [==] сравнения версии
 *
 * @param version версия для сравнения
 * @return        результат сравнения
 *
 */
bool awh::Version::operator == (const version_t & version) const noexcept {
	// Возвращаем результат
	return (this->_version == version._version);
}
/**
 * @brief Оператор присваивания присвоения версии
 *
 * @param version версия для присвоения
 * @return        текущий объект
 *
 */
awh::Version & awh::Version::operator = (const char * version) noexcept {
	// Устанавливаем версию
	this->set(version);
	// Возвращаем результат
	return (* this);
}
/**
 * @brief Оператор присваивания присвоения версии
 *
 * @param version версия для присвоения
 * @return        текущий объект
 *
 */
awh::Version & awh::Version::operator = (const string & version) noexcept {
	// Устанавливаем версию
	this->set(version);
	// Возвращаем результат
	return (* this);
}
/**
 * @brief Оператор присваивания присвоения версии
 *
 * @param version версия для присвоения
 * @return        текущий объект
 *
 */
awh::Version & awh::Version::operator = (const uint32_t version) noexcept {
	// Устанавливаем версию
	this->set(version);
	// Возвращаем результат
	return (* this);
}
/**
 * @brief Оператор присваивания присвоения версии
 *
 * @param version версия для присвоения
 * @return        текущий объект
 *
 */
awh::Version & awh::Version::operator = (const Version & version) noexcept {
	// Устанавливаем версию
	this->_version = version._version;
	// Копируем объект логирования (для согласованности с конструктором копирования)
	this->_log = version._log;
	// Возвращаем результат
	return (* this);
}
/**
 * @brief Конструктор
 *
 */
awh::Version::Version() noexcept : _version(0), _log(nullptr) {}
/**
 * @brief Конструктор
 *
 */
awh::Version::Version(const log_t * log) noexcept : _version(0), _log(log) {}
/**
 * @brief Конструктор
 *
 * @param version устанавливаемая версия
 *
 */
awh::Version::Version(const char * version) noexcept : _version(0), _log(nullptr) {
	// Устанавливаем версию
	this->set(version);
}
/**
 * @brief Конструктор
 *
 * @param version устанавливаемая версия
 *
 */
awh::Version::Version(const string & version) noexcept : _version(0), _log(nullptr) {
	// Устанавливаем версию
	this->set(version);
}
/**
 * @brief Конструктор
 *
 * @param version устанавливаемая версия
 *
 */
awh::Version::Version(const uint32_t version) noexcept : _version(0), _log(nullptr) {
	// Устанавливаем версию
	this->set(version);
}
/**
 * @brief Оператор [>>] чтения из потока версии
 *
 * @param is      поток для чтения
 * @param version версия для присвоения
 *
 */
istream & awh::operator >> (istream & is, version_t & version) noexcept {
	// Версия в текстовом виде
	string result = "";
	// Считываем версию
	is >> result;
	// Если версия передана
	if(!result.empty())
		// Устанавливаем версию
		version = result;
	// Возвращаем результат
	return is;
}
/**
 * @brief Оператор [<<] вывода в поток версии
 *
 * @param os      поток куда нужно вывести данные
 * @param version версия извлечения
 *
 */
ostream & awh::operator << (ostream & os, const version_t & version) noexcept {
	// Записываем в поток версию
	os << version.str();
	// Возвращаем результат
	return os;
}
