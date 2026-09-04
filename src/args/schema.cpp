/**
 * @file schema.cpp
 * @date 2026-09-03
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Исходный файл описания ожидаемых параметров запуска
 *
 * \~english
 * @brief Source file of the description of the expected parameters of the launch
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <args/schema.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён параметров запуска приложения
 */
using namespace awh::args;

/**
 * @brief Метод очистки описания ожидаемых параметров
 *
 */
void awh::args::Schema::clear() noexcept {
	// Выполняем очистку описаний ожидаемых параметров
	this->_params.clear();
	// Выполняем очистку розыска по длинному имени
	this->_names.clear();
	// Выполняем очистку розыска по короткому имени
	this->_letters.clear();
}

/**
 * @brief Метод заведения описания ожидаемого параметра
 *
 * @param param описание ожидаемого параметра
 * @return      результат заведения
 *
 */
bool awh::args::Schema::add(const param_t & param) noexcept {
	// Если длинное имя параметра не подано вовсе
	if(param.name.empty()){
		// Выводим в лог сообщение об отсутствии имени параметра
		this->_log->print("Schema: parameter name is empty", log_t::flag_t::WARNING);
		// Выходим из метода, заводить нечего
		return false;
	}
	// Выполняем поиск описания с тем же длинным именем
	auto i = this->_names.find(param.name);
	// Если описание с таким именем уже заведено
	if(i != this->_names.end()){
		// Получаем описание, заведённое прежде
		param_t & current = this->_params.at(i->second);
		// Если прежнее описание несло короткое имя
		if(current.letter != 0)
			// Выполняем снятие розыска по прежнему короткому имени
			this->_letters.erase(current.letter);
		// Выполняем замену прежнего описания поданным
		current = param;
		// Если поданное описание несёт короткое имя
		if(param.letter != 0)
			// Выполняем заведение розыска по короткому имени
			this->_letters.emplace(param.letter, i->second);
		// Сообщаем, что заведение описания выполнено
		return true;
	}
	// Если короткое имя параметра уже занято иным описанием
	if((param.letter != 0) && (this->_letters.count(param.letter) > 0)){
		// Выводим в лог сообщение о занятости короткого имени
		this->_log->print("Schema: short name '%c' is already taken", log_t::flag_t::WARNING, param.letter);
		// Выходим из метода, заведение отвечено отказом
		return false;
	}
	// Выполняем заведение розыска по длинному имени
	this->_names.emplace(param.name, this->_params.size());
	// Если описание несёт короткое имя
	if(param.letter != 0)
		// Выполняем заведение розыска по короткому имени
		this->_letters.emplace(param.letter, this->_params.size());
	// Добавляем описание в контейнер описаний
	this->_params.push_back(param);
	// Сообщаем, что заведение описания выполнено
	return true;
}

/**
 * @brief Метод заведения описания ожидаемого параметра
 *
 * @param name        длинное имя параметра с разделителем звеньев
 * @param letter      короткое имя параметра одним знаком, нуль при отсутствии
 * @param value       потребность параметра в значении
 * @param description описание назначения параметра для справки
 * @return            результат заведения
 *
 */
bool awh::args::Schema::add(const string_view name, const char letter, const value_t value, const string_view description) noexcept {
	// Создаём описание ожидаемого параметра
	param_t param;
	// Устанавливаем длинное имя параметра
	param.name.assign(name);
	// Устанавливаем короткое имя параметра
	param.letter = letter;
	// Устанавливаем потребность параметра в значении
	param.value = value;
	// Устанавливаем описание назначения параметра
	param.description.assign(description);
	// Выполняем заведение собранного описания
	return this->add(param);
}

/**
 * @brief Метод извлечения описания параметра по длинному имени
 *
 * @param name длинное имя параметра
 * @return     описание параметра либо nullptr при его отсутствии
 *
 */
const Schema::param_t * awh::args::Schema::get(const string_view name) const noexcept {
	// Выполняем поиск описания по длинному имени параметра
	auto i = this->_names.find(string(name));
	// Если описание по длинному имени найдено
	if(i != this->_names.end())
		// Выводим найденное описание параметра
		return &(this->_params.at(i->second));
	// Выводим отсутствие описания параметра
	return nullptr;
}

/**
 * @brief Метод извлечения описания параметра по короткому имени
 *
 * @param letter короткое имя параметра одним знаком
 * @return       описание параметра либо nullptr при его отсутствии
 *
 */
const Schema::param_t * awh::args::Schema::get(const char letter) const noexcept {
	// Выполняем поиск описания по короткому имени параметра
	auto i = this->_letters.find(letter);
	// Если описание по короткому имени найдено
	if(i != this->_letters.end())
		// Выводим найденное описание параметра
		return &(this->_params.at(i->second));
	// Выводим отсутствие описания параметра
	return nullptr;
}

/**
 * @brief Метод разбора склейки коротких имён
 *
 * @param cluster склейка коротких имён без ведущего тире
 * @param result  контейнер разобранных длинных имён
 * @return        результат разбора
 *
 */
bool awh::args::Schema::cluster(const string_view cluster, vector <string> & result) const noexcept {
	// Выполняем очистку контейнера разобранных длинных имён
	result.clear();
	// Если склейка коротких имён пуста вовсе
	if(cluster.empty())
		// Выходим из метода, разбирать нечего
		return false;
	// Выполняем резервирование памяти под разбираемые имена
	result.reserve(cluster.length());
	// Выполняем перебор всех знаков склейки коротких имён
	for(size_t i = 0; i < cluster.length(); i++){
		// Выполняем поиск описания по короткому имени
		const param_t * param = this->get(cluster.at(i));
		/**
		 * Если знак склейки описанию неизвестен, разбор ОТКАЗЫВАЕТ ЦЕЛИКОМ: запись
		 * эта неотличима от длинного имени под одним тире, каковое запись
		 * «-name VALUE» дозволяет, и разбирать её наполовину значило бы гадать
		 */
		if(param == nullptr){
			// Выполняем очистку контейнера разобранных длинных имён
			result.clear();
			// Выходим из метода, разбор отвечен отказом
			return false;
		}
		/**
		 * Если знак склейки значения требует, разбор ОТКАЗЫВАЕТ ЦЕЛИКОМ: значение
		 * досталось бы лишь последнему знаку склейки, а прочие остались бы без него,
		 * и запись эта означала бы разное для разных знаков одной записи
		 */
		if(param->value == value_t::REQUIRED){
			// Выполняем очистку контейнера разобранных длинных имён
			result.clear();
			// Выходим из метода, разбор отвечен отказом
			return false;
		}
		// Добавляем длинное имя разобранного параметра
		result.push_back(param->name);
	}
	// Сообщаем, что разбор склейки выполнен
	return true;
}

/**
 * @brief Метод извлечения описаний всех ожидаемых параметров
 *
 * @return описания в порядке их заведения
 *
 */
const vector <Schema::param_t> & awh::args::Schema::params() const noexcept {
	// Выводим описания ожидаемых параметров
	return this->_params;
}

/**
 * @brief Метод проверки пустоты описания
 *
 * @return результат проверки
 *
 */
bool awh::args::Schema::empty() const noexcept {
	// Выводим признак пустоты описания ожидаемых параметров
	return this->_params.empty();
}

/**
 * @brief Метод сборки справки о применении
 *
 * @return собранный текст справки
 *
 */
string awh::args::Schema::usage() const noexcept {
	// Собираемый текст справки о применении
	string result = "";
	// Если название приложения установлено
	if(!this->_application.empty()){
		// Добавляем в справку строку применения приложения
		result.append(this->_fmk->format("Usage: %s [OPTIONS]", this->_application.c_str()));
		// Добавляем в справку перевод строки
		result.append(1, '\n');
		// Если описание назначения приложения установлено
		if(!this->_description.empty()){
			// Добавляем в справку перевод строки
			result.append(1, '\n');
			// Добавляем в справку описание назначения приложения
			result.append(this->_description);
			// Добавляем в справку перевод строки
			result.append(1, '\n');
		}
	}
	// Если описаний ожидаемых параметров нет вовсе
	if(this->_params.empty())
		// Выводим собранный текст справки
		return result;
	// Собранные строки имён параметров
	vector <string> names;
	// Выполняем резервирование памяти под строки имён
	names.reserve(this->_params.size());
	// Наибольшая длина строки имени параметра
	size_t width = 0;
	// Выполняем перебор всех описаний ожидаемых параметров
	for(auto & param : this->_params){
		// Собираемая строка имени параметра
		string name = "";
		// Если описание несёт короткое имя параметра
		if(param.letter != 0)
			// Добавляем в строку короткое имя параметра
			name.append(this->_fmk->format("-%c, ", param.letter));
		// Добавляем в строку длинное имя параметра
		name.append("--").append(param.name);
		// Определяем потребность параметра в значении
		switch(static_cast <uint8_t> (param.value)){
			// Если значение параметру потребно непременно
			case static_cast <uint8_t> (value_t::REQUIRED):
				// Добавляем в строку обозначение потребного значения
				name.append("=VALUE");
			break;
			// Если значение параметру необязательно
			case static_cast <uint8_t> (value_t::OPTIONAL):
				// Добавляем в строку обозначение необязательного значения
				name.append("[=VALUE]");
			break;
		}
		// Если длина строки имени превысила наибольшую
		if(name.length() > width)
			// Запоминаем длину строки имени наибольшей
			width = name.length();
		// Добавляем собранную строку имени в контейнер
		names.push_back(::move(name));
	}
	// Добавляем в справку перевод строки
	result.append(1, '\n');
	// Добавляем в справку заголовок перечня параметров
	result.append("Options:").append(1, '\n');
	// Выполняем перебор всех описаний ожидаемых параметров
	for(size_t i = 0; i < this->_params.size(); i++){
		// Получаем описание ожидаемого параметра
		const param_t & param = this->_params.at(i);
		// Добавляем в справку отступ строки параметра
		result.append("  ");
		// Добавляем в справку строку имени параметра
		result.append(names.at(i));
		// Добавляем в справку выравнивание описаний по наибольшему имени
		result.append((width + 2) - names.at(i).length(), ' ');
		// Добавляем в справку описание назначения параметра
		result.append(param.description);
		// Если параметр обязателен к подаче
		if(param.required)
			// Добавляем в справку признак обязательности параметра
			result.append(" (required)");
		// Если параметр несёт значение по умолчанию
		if(param.preset)
			// Добавляем в справку значение параметра по умолчанию
			result.append(this->_fmk->format(" (default: %s)", param.fallback.c_str()));
		// Если параметр дозволено подавать повторно
		if(param.multiple)
			// Добавляем в справку признак дозволенности повтора
			result.append(" (repeatable)");
		// Добавляем в справку перевод строки
		result.append(1, '\n');
	}
	// Выводим собранный текст справки
	return result;
}

/**
 * @brief Метод установки названия приложения для справки
 *
 * @param application название приложения
 * @param description описание назначения приложения
 *
 */
void awh::args::Schema::application(const string_view application, const string_view description) noexcept {
	// Устанавливаем название приложения для справки
	this->_application.assign(application);
	// Устанавливаем описание назначения приложения
	this->_description.assign(description);
}
