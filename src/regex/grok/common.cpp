/**
 * @file common.cpp
 * @date 2026-08-04
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
 * @brief Файл реализации общих типов модуля Grok —
 *        коды ошибок разбора шаблона, виды значений полей, описание поля
 *        и собранный шаблон
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <regex/grok/common.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <charconv>
#include <cmath>
#include <cstdio>
#include <system_error>

/**
 * @brief Конструктор
 *
 * Подписываемся на стандартное пространство имён
 *
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::grok::Field::Field() noexcept :
 number(0), kind(kind_t::TEXT) {}

/**
 * @brief Конструктор
 *
 */
awh::grok::Value::Value() noexcept :
 kind(kind_t::TEXT) {}

/**
 * @brief Конструктор
 *
 */
awh::grok::Expression::Expression() noexcept :
 pattern{""}, expression{""} {}

/**
 * @brief Метод экранирования текста записи JSON
 *
 * @param text текст для экранирования
 * @return     экранированный текст
 */
static string escapeJSON(const string_view text) noexcept {
	// Результат экранирования
	string result;
	// Резервируем место под результат
	result.reserve(text.size() + 8);
	// Перебираем все байты текста
	for(const char c : text){
		/**
		 * Определяем байт текста
		 */
		switch(c){
			// Экранируем кавычку
			case '"': {
				// Заносим экранированную кавычку
				result.append("\\\"");
			} break;
			// Экранируем обратную косую черту
			case '\\': {
				// Заносим экранированную косую черту
				result.append("\\\\");
			} break;
			// Экранируем перевод строки
			case '\n': {
				// Заносим экранированный перевод строки
				result.append("\\n");
			} break;
			// Экранируем возврат каретки
			case '\r': {
				// Заносим экранированный возврат каретки
				result.append("\\r");
			} break;
			// Экранируем табуляцию
			case '\t': {
				// Заносим экранированную табуляцию
				result.append("\\t");
			} break;
			// Экранируем прочие управляющие байты
			default: {
				// Если байт управляющий
				if(static_cast <uint8_t> (c) < 0x20){
					// Заносим запись управляющего байта
					char code[8] = {0};
					// Формируем запись управляющего байта
					snprintf(code, sizeof(code), "\\u%04X", static_cast <uint8_t> (c));
					// Заносим запись управляющего байта
					result.append(code);
				// Иначе заносим байт как есть
				} else result.push_back(c);
			} break;
		}
	}
	// Выводим результат экранирования
	return result;
}

/**
 * @brief Метод очистки записи
 */
void awh::grok::Json::clear() noexcept {
	// Очищаем пункты записи
	this->_items.clear();
	// Очищаем название поля
	this->_key.clear();
}

/**
 * @brief Метод проверки пустоты записи
 *
 * @return результат проверки
 */
bool awh::grok::Json::empty() const noexcept {
	// Выводим результат проверки
	return this->_items.empty();
}

/**
 * @brief Метод открытия объекта записи
 */
void awh::grok::Json::object() noexcept {
	// Выполняем очистку записи
	this->clear();
}

/**
 * @brief Метод занесения названия поля
 *
 * @param name название поля
 */
void awh::grok::Json::key(const string_view name) noexcept {
	// Запоминаем название поля
	this->_key.assign(name.data(), name.size());
}

/**
 * @brief Метод занесения значения поля текстом
 *
 * @param value значение поля
 */
void awh::grok::Json::value(const string_view value) noexcept {
	// Ищем поле с тем же названием
	for(auto & item : this->_items){
		// Если название поля совпало
		if(item.name == this->_key){
			// Устанавливаем признак текста
			item.numeric = false;
			// Запоминаем значение поля
			item.value.assign(value.data(), value.size());
			// Выходим из метода
			return;
		}
	}
	// Заносим новый пункт записи
	this->_items.push_back({this->_key, false, string(value.data(), value.size())});
}

/**
 * @brief Метод занесения значения поля числом дробным
 *
 * @param value значение поля
 */
void awh::grok::Json::value(const double value) noexcept {
	// Токен числа
	string token;
	// Если число конечно
	if(std::isfinite(value)){
		// Буфер записи числа
		char buffer[64] = {0};
		// Формируем кратчайшую запись числа
		const auto result = std::to_chars(buffer, (buffer + sizeof(buffer)), value);
		// Если запись числа выполнена
		if(result.ec == std::errc())
			// Запоминаем токен числа
			token.assign(buffer, result.ptr);
		// Иначе формируем запись числа запасным способом
		else {
			// Формируем запись числа
			snprintf(buffer, sizeof(buffer), "%.17g", value);
			// Запоминаем токен числа
			token.assign(buffer);
		}
	// Иначе кладём значение пустым
	} else token.assign("null");
	// Ищем поле с тем же названием
	for(auto & item : this->_items){
		// Если название поля совпало
		if(item.name == this->_key){
			// Устанавливаем признак числа
			item.numeric = true;
			// Запоминаем токен числа
			item.value = ::move(token);
			// Выходим из метода
			return;
		}
	}
	// Заносим новый пункт записи
	this->_items.push_back({this->_key, true, ::move(token)});
}

/**
 * @brief Метод занесения значения поля числом целым со знаком
 *
 * @param value значение поля
 */
void awh::grok::Json::value(const int64_t value) noexcept {
	// Ищем поле с тем же названием
	for(auto & item : this->_items){
		// Если название поля совпало
		if(item.name == this->_key){
			// Устанавливаем признак числа
			item.numeric = true;
			// Запоминаем запись числа
			item.value = to_string(value);
			// Выходим из метода
			return;
		}
	}
	// Заносим новый пункт записи
	this->_items.push_back({this->_key, true, to_string(value)});
}

/**
 * @brief Метод занесения значения поля числом целым без знака
 *
 * @param value значение поля
 */
void awh::grok::Json::value(const uint64_t value) noexcept {
	// Ищем поле с тем же названием
	for(auto & item : this->_items){
		// Если название поля совпало
		if(item.name == this->_key){
			// Устанавливаем признак числа
			item.numeric = true;
			// Запоминаем запись числа
			item.value = to_string(value);
			// Выходим из метода
			return;
		}
	}
	// Заносим новый пункт записи
	this->_items.push_back({this->_key, true, to_string(value)});
}

/**
 * @brief Метод получения собранной записи
 *
 * @return собранная запись
 */
awh::grok::Json awh::grok::Json::finish() const noexcept {
	// Выводим собранную запись
	return *this;
}

/**
 * @brief Метод вывода записи JSON текстом
 *
 * @param format вид записи
 * @return       запись JSON текстом
 */
string awh::grok::Json::dump(const format_t format) const noexcept {
	// Результат вывода записи
	string result;
	// Открываем объект записи
	result.push_back('{');
	// Перебираем все пункты записи
	for(size_t i = 0; i < this->_items.size(); ++i){
		// Если запись ведётся с отступами
		if(format == format_t::PRETTY){
			// Переводим строку и выполняем отступ
			result.append("\n    ");
		// Иначе разделяем пункты запятой
		} else if(i > 0) result.push_back(',');
		// Заносим название поля
		result.push_back('"');
		// Заносим экранированное название поля
		result.append(escapeJSON(this->_items[i].name));
		// Закрываем название поля
		result.push_back('"');
		// Заносим разделитель поля и значения
		result.append(format == format_t::PRETTY ? ": " : ":");
		/**
		 * Если значение поля является числом
		 */
		if(this->_items[i].numeric)
			// Заносим запись числа
			result.append(this->_items[i].value);
		/**
		 * Если значение поля является текстом
		 */
		else {
			// Открываем текст
			result.push_back('"');
			// Заносим экранированный текст
			result.append(escapeJSON(this->_items[i].value));
			// Закрываем текст
			result.push_back('"');
		}
		// Если запись ведётся с отступами и пункт не последний
		if((format == format_t::PRETTY) && ((i + 1) < this->_items.size()))
			// Заносим запятую
			result.push_back(',');
	}
	// Если запись ведётся с отступами и объект не пуст
	if((format == format_t::PRETTY) && !this->_items.empty())
		// Закрываем объект с новой строки
		result.append("\n}");
	// Иначе закрываем объект
	else result.push_back('}');
	// Выводим результат вывода записи
	return result;
}
