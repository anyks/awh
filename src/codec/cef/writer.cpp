/**
 * @file writer.cpp
 * @date 2026-09-04
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
 * @brief Реализация записи событий в запись CEF — обхода дерева контейнера ABC, постановки отмены
 *        знаков порознь по областям записи и сборки заголовка с расширением
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/writer.hpp>

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже
 */
#include <sys/macro/suppress.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;
	/**
	 * Пространство имён контейнера CEF
	 */
	using namespace awh::codec::cef;

	/**
	 * @brief Имена полей заголовка записи в дереве контейнера ABC
	 *
	 * @details Порядок отвечает порядку полей в записи и порядку членов перечня полей:
	 *          поле разыскивается по счёту, а не по имени, ибо имён у полей заголовка
	 *          сама запись CEF не несёт вовсе
	 */
	constexpr string_view FIELDS[] = {
		"version", "vendor", "product", "release", "signature", "name", "severity"
	};
}

/**
 * @brief Метод прекращения записи ошибкой
 *
 * @param error код ошибки записи
 * @param name  имя поля, на котором запись прекращена
 * @return      признак успешности записи
 */
bool awh::codec::cef::Writer::fail(const error_t error, const string_view name) noexcept {
	// Запоминаем код ошибки записи
	this->_error = error;
	// Выводим в лог сообщение об ошибке записи
	this->_log->print(
		"CEF writing failed: %s at the field \"%s\"",
		log_t::flag_t::CRITICAL,
		awh::codec::cef::message(error),
		string(name).c_str()
	);
	// Выводим отрицательный признак успешности записи
	return false;
}

/**
 * @brief Метод постановки отмены знаков в значении
 *
 * @param text   значение, отмены знаков требующее
 * @param area   область записи, в которую значение ставится
 * @param result значение с поставленной отменой знаков
 */
void awh::codec::cef::Writer::escape(const string_view text, const area_t area, string & result) const noexcept {
	// Выделяем память под результирующее значение
	result.reserve(result.size() + text.size());
	/**
	 * Если постановка отмены знаков выключена
	 *
	 * @details Настройка эта живёт в паре с настройкой снятия отмены у чтения:
	 * значения, отмену знаков несущие, ставятся в запись как есть. Порознь их
	 * задавать нельзя - постановка отмены поверх неснятой наращивает косые при
	 * всяком обороте, - оттого пару эту сводит документ, а не потребитель
	 */
	if(!this->_settings.escape){
		// Добавляем значение в результирующую запись как есть
		result.append(text);
		// Выходим из функции
		return;
	}
	/**
	 * Выполняем перебор всех знаков значения
	 */
	for(size_t i = 0; i < text.size(); i++){
		/**
		 * Определяем область записи, в которую значение ставится
		 */
		switch(static_cast <uint8_t> (area)){
			// Если значение ставится в заголовок записи
			case static_cast <uint8_t> (area_t::HEADER): {
				// Если знак отмены в заголовке требует
				if((text[i] == '|') || (text[i] == '\\'))
					// Ставим знак обратной косой перед отменяемым знаком
					result.append(1, '\\');
				// Добавляем знак в результирующее значение
				result.append(1, text[i]);
			} break;
			// Если значение ставится в расширение записи
			case static_cast <uint8_t> (area_t::EXTENSION): {
				/**
				 * Определяем знак значения
				 */
				switch(text[i]){
					// Если знак является переводом строки
					case '\n': {
						// Ставим перевод строки отменяющей последовательностью
						result.append("\\n");
					} break;
					// Если знак является возвратом каретки
					case '\r': {
						// Ставим возврат каретки отменяющей последовательностью
						result.append("\\r");
					} break;
					// Если знак является знаком равенства
					case '=':
					// Если знак является обратной косой
					case '\\': {
						// Ставим знак обратной косой перед отменяемым знаком
						result.append(1, '\\');
						// Добавляем отменяемый знак в результирующее значение
						result.append(1, text[i]);
					} break;
					// Если знак отмены не требует
					default: result.append(1, text[i]);
				}
			} break;
			// Если значение ставится в приставку syslog
			default: result.append(1, text[i]);
		}
	}
}

/**
 * @brief Метод обращения значения дерева в последовательность знаков
 *
 * @param value  значение дерева контейнера ABC
 * @param result значение последовательностью знаков
 * @return       признак успешности обращения значения
 */
bool awh::codec::cef::Writer::stringify(const abc::value_t & value, string & result) noexcept {
	// Выполняем очистку результирующего значения
	result.clear();
	/**
	 * Определяем вид значения дерева контейнера ABC
	 */
	switch(static_cast <uint32_t> (value.type())){
		// Если значение является отображением
		case static_cast <uint32_t> (abc::type_t::MAP):
		// Если значение является массивом
		case static_cast <uint32_t> (abc::type_t::ARRAY): {
			/**
			 * Определяем обращение с вложенным значением
			 */
			switch(static_cast <uint8_t> (this->_settings.nested)){
				// Если вложенное значение отвечается отказом
				case static_cast <uint8_t> (nested_t::STRICT):
					// Выводим отрицательный признак обращения значения
					return false;
				// Если вложенное значение обращается в знаки
				case static_cast <uint8_t> (nested_t::TEXT): {
					// Выполняем сбор двоичной записи вложенного значения
					const auto & buffer = value.dump();
					// Обращаем двоичную запись вложенного значения в знаки
					result.assign(buffer.begin(), buffer.end());
					// Выводим положительный признак обращения значения
					return true;
				}
				// Если вложенное значение пропускается вовсе: отказ обращения
				// разбирается зовущим по настройке, ибо пропуск пары есть дело
				// сборки записи, а не обращения значения
				default: return false;
			}
		}
		// Если значение является строкой
		case static_cast <uint32_t> (abc::type_t::STRING): {
			// Запоминаем содержимое строки результирующим значением
			result.assign(value.text());
			// Выводим положительный признак обращения значения
			return true;
		}
		// Если значение является пустым
		case static_cast <uint32_t> (abc::type_t::NUL): {
			// Выводим положительный признак обращения значения
			return true;
		}
		// Если значение является логическим
		case static_cast <uint32_t> (abc::type_t::BOOL): {
			// Логическое значение, из дерева извлекаемое
			bool current = false;
			// Если извлечение логического значения отказом завершилось
			if(!value.value(current))
				// Выводим отрицательный признак обращения значения
				return false;
			// Запоминаем логическое значение результирующим значением
			result.assign(current ? "true" : "false");
			// Выводим положительный признак обращения значения
			return true;
		}
	}
	// Если значение является целым со знаком
	if((static_cast <uint32_t> (value.type()) & static_cast <uint32_t> (abc::type_t::SIGNED)) > 0){
		// Целое значение со знаком, из дерева извлекаемое
		int64_t current = 0;
		// Если извлечение целого значения отказом завершилось
		if(!value.value(current))
			// Выводим отрицательный признак обращения значения
			return false;
		// Запоминаем целое значение результирующим значением
		result = ::std::to_string(current);
		// Выводим положительный признак обращения значения
		return true;
	}
	// Если значение является целым без знака
	if((static_cast <uint32_t> (value.type()) & static_cast <uint32_t> (abc::type_t::UNSIGNED)) > 0){
		// Целое значение без знака, из дерева извлекаемое
		uint64_t current = 0;
		// Если извлечение целого значения отказом завершилось
		if(!value.value(current))
			// Выводим отрицательный признак обращения значения
			return false;
		// Запоминаем целое значение результирующим значением
		result = ::std::to_string(current);
		// Выводим положительный признак обращения значения
		return true;
	}
	// Если значение является дробным
	if((static_cast <uint32_t> (value.type()) & (static_cast <uint32_t> (abc::type_t::FLOAT) | static_cast <uint32_t> (abc::type_t::DOUBLE))) > 0){
		// Дробное значение, из дерева извлекаемое
		double current = 0.;
		// Если извлечение дробного значения отказом завершилось
		if(!value.value(current))
			// Выводим отрицательный признак обращения значения
			return false;
		// Запоминаем дробное значение результирующим значением
		result = this->_fmk->noexp(current, true);
		// Выводим положительный признак обращения значения
		return true;
	}
	// Строковое представление значения прочих видов
	string current = "";
	// Если извлечение значения знаками отказом завершилось
	if(!value.value(current))
		// Выводим отрицательный признак обращения значения
		return false;
	// Запоминаем значение знаками результирующим значением
	result = ::std::move(current);
	// Выводим положительный признак обращения значения
	return true;
}

/**
 * @brief Метод получения настроек записи событий
 *
 * @return настройки записи событий
 */
const awh::codec::cef::Writer::settings_t & awh::codec::cef::Writer::settings() const noexcept {
	// Выводим настройки записи событий
	return this->_settings;
}

/**
 * @brief Метод установки настроек записи событий
 *
 * @param settings настройки записи событий
 */
void awh::codec::cef::Writer::settings(const settings_t & settings) noexcept {
	// Устанавливаем настройки записи событий
	this->_settings = settings;
}

/**
 * @brief Метод получения кода ошибки записи
 *
 * @return код ошибки последней операции записи
 */
awh::codec::cef::error_t awh::codec::cef::Writer::error() const noexcept {
	// Выводим код ошибки последней операции записи
	return this->_error;
}

/**
 * @brief Метод сборки записи CEF из дерева контейнера ABC
 *
 * @param value  дерево контейнера ABC
 * @param result собранная запись CEF
 * @return       признак успешности сборки записи
 */
bool awh::codec::cef::Writer::write(const abc::value_t & value, string & result) noexcept {
	// Выполняем очистку результирующей записи
	result.clear();
	// Сбрасываем код ошибки последней операции записи
	this->_error = error_t::NONE;
	// Если дерево отображением не является
	if(value.type() != abc::type_t::MAP)
		// Выводим отказ записи неверным устройством дерева
		return this->fail(error_t::UNREPRESENTABLE_VALUE, "/");
	// Значение поля дерева последовательностью знаков
	string text = "";
	// Если запись приставки syslog включена и приставка деревом объявлена
	if(this->_settings.syslog && value.contains("syslog")){
		// Если обращение приставки в знаки отказом завершилось
		if(!this->stringify(value.at("syslog"), text))
			// Выводим отказ записи неверным значением приставки
			return this->fail(error_t::UNREPRESENTABLE_VALUE, "syslog");
		// Если приставка syslog не пуста
		if(!text.empty()){
			// Добавляем приставку syslog в результирующую запись
			result.append(text);
			// Отделяем приставку syslog от заголовка записи пробелом
			result.append(1, ' ');
		}
	}
	// Добавляем слово, заголовок записи открывающее
	result.append(SIGNATURE);
	// Получаем поля заголовка записи из дерева
	const abc::value_t & header = value.at("header");
	// Если поля заголовка деревом не объявлены
	if(header.type() != abc::type_t::MAP)
		// Выводим отказ записи отсутствием заголовка
		return this->fail(error_t::INCOMPLETE_HEADER, "header");
	/**
	 * Выполняем перебор всех полей заголовка записи
	 */
	for(uint32_t i = 0; i < HEADER_FIELDS; i++){
		// Получаем имя очередного поля заголовка
		const string name(FIELDS[i]);
		// Если поле заголовка деревом объявлено
		if(header.contains(name)){
			// Если обращение поля заголовка в знаки отказом завершилось
			if(!this->stringify(header.at(name), text)){
				// Если вложенное поле заголовка пропускается вовсе
				if((this->_settings.nested == nested_t::SKIP) &&
				   ((header.at(name).type() == abc::type_t::MAP) || (header.at(name).type() == abc::type_t::ARRAY)))
					// Оставляем поле заголовка пустым
					text.clear();
				// Если вложенное поле заголовка пропуску не подлежит
				else return this->fail(
					(header.at(name).type() == abc::type_t::MAP) || (header.at(name).type() == abc::type_t::ARRAY) ?
					error_t::NESTED_VALUE : error_t::UNREPRESENTABLE_VALUE,
					name
				);
			}
		// Если поле заголовка деревом не объявлено
		} else text.clear();
		// Если полем заголовка является номер редакции записи
		if(i == static_cast <uint32_t> (field_t::VERSION)){
			// Если номер редакции записи деревом не объявлен
			if(text.empty())
				// Устанавливаем номер редакции записи из настроек записи
				text = ::std::to_string(this->_settings.version);
			// Добавляем номер редакции записи как есть: отмены знаков он не требует
			result.append(text);
		// Если полем заголовка является поле, отмены знаков требующее
		} else this->escape(text, area_t::HEADER, result);
		// Отделяем поле заголовка от следующего прямой чертой
		result.append(1, '|');
	}
	// Получаем пары расширения записи из дерева
	const abc::value_t & extension = value.at("extension");
	// Если пары расширения деревом объявлены
	if(extension.type() == abc::type_t::MAP){
		// Признак того, что пара расширения уже записана
		bool written = false;
		/**
		 * Выполняем перебор всех пар расширения записи
		 */
		for(size_t i = 0; i < extension.size(); i++){
			// Получаем имя ключа очередной пары расширения
			const string & key = extension.key(i).text();
			/**
			 * Если имя ключа записи CEF непредставимо
			 *
			 * @details Пары расширения разделяются пробелом, и отмены пробела запись
			 * CEF не знает вовсе: ключ, пробельный знак несущий, записанным быть не
			 * может - повторный разбор такой записи разделил бы его на две пары.
			 * Молчаливая же запись его оставляла бы потребителя с записью, которая
			 * разбирается, но означает иное
			 *
			 * @note Найдено ворошителем 04.09.2026 расхождением деревьев после оборота
			 */
			if(key.find_first_of(" \t\n\r") != string::npos){
				// Если непредставимое значение пропускается вовсе
				if(this->_settings.nested == nested_t::SKIP)
					// Переходим к следующей паре расширения
					continue;
				// Выводим отказ записи непредставимым именем ключа
				return this->fail(error_t::UNREPRESENTABLE_VALUE, key);
			}
			// Если имя ключа расширения пусто
			if(key.empty()){
				// Если непредставимое значение пропускается вовсе
				if(this->_settings.nested == nested_t::SKIP)
					// Переходим к следующей паре расширения
					continue;
				// Выводим отказ записи пустым именем ключа
				return this->fail(error_t::EMPTY_KEY, key);
			}
			// Получаем значение очередной пары расширения
			const abc::value_t & current = extension.at(key);
			// Количество значений, ключом объявленных
			const size_t count = ((current.type() == abc::type_t::ARRAY) ? current.size() : 1);
			/**
			 * Выполняем перебор всех значений, ключом объявленных
			 */
			for(size_t j = 0; j < count; j++){
				// Получаем очередное значение, ключом объявленное
				const abc::value_t & item = ((current.type() == abc::type_t::ARRAY) ? current[j] : current);
				// Если обращение значения в знаки отказом завершилось
				if(!this->stringify(item, text)){
					// Если значение вложенным является
					if((item.type() == abc::type_t::MAP) || (item.type() == abc::type_t::ARRAY)){
						// Если вложенное значение пропускается вовсе
						if(this->_settings.nested == nested_t::SKIP)
							// Переходим к следующему значению
							continue;
						// Выводим отказ записи вложенным значением
						return this->fail(error_t::NESTED_VALUE, key);
					}
					// Выводим отказ записи неверным значением расширения
					return this->fail(error_t::UNREPRESENTABLE_VALUE, key);
				}
				// Если пара расширения уже записана
				if(written)
					// Отделяем пару расширения от предыдущей пробелом
					result.append(1, ' ');
				// Ставим отмену знаков в имени ключа пары расширения
				this->escape(key, area_t::EXTENSION, result);
				// Отделяем имя ключа от значения знаком равенства
				result.append(1, '=');
				// Ставим отмену знаков в значении пары расширения
				this->escape(text, area_t::EXTENSION, result);
				// Запоминаем, что пара расширения записана
				written = true;
			}
		}
	}
	// Если запись знака конца строки включена
	if(this->_settings.terminate)
		// Добавляем знак конца строки за записью
		result.append(1, '\n');
	// Выводим положительный признак успешности сборки записи
	return true;
}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::codec::cef::Writer::Writer(const fmk_t * fmk, const log_t * log) noexcept :
 _error(error_t::NONE), _fmk(fmk), _log(log) {}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
