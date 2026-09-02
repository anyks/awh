/**
 * @file bridge.cpp
 * @date 2026-09-02
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
 * @brief Исходный файл моста между контейнером ABC и текстовыми кодеками
 *
 * \~english
 * @brief Source file of the bridge between the container ABC and the text codecs
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <codec/bridge.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён контейнеров данных
 */
using namespace awh::codec;

/**
 * @brief Метод подачи значения ABC писателю JSON
 *
 * @param value  значение контейнера ABC
 * @param writer писатель записи JSON
 * @param depth  глубина обхода дерева
 * @return       результат подачи
 *
 */
bool awh::codec::Bridge::feed(const abc::value_t & value, json::writer_t & writer, const uint32_t depth) noexcept {
	// Если глубина обхода превысила предел перевода
	if(depth > this->_settings.depth){
		// Запоминаем код отказа перевода
		this->_error = error_t::DEEP_TREE;
		// Выходим из метода, обход остановлен
		return false;
	}
	// Если значение недействительно вовсе
	if(!value.valid())
		// Выводим пустое значение записью JSON
		return writer.null();
	// Определяем вид поданного значения
	switch(static_cast <uint32_t> (value.type())){
		// Если значение является пустым
		case static_cast <uint32_t> (abc::type_t::NUL):
			// Выводим пустое значение записью JSON
			return writer.null();
		// Если значение является логическим
		case static_cast <uint32_t> (abc::type_t::BOOL): {
			// Извлекаемое логическое значение
			bool result = false;
			// Выполняем извлечение логического значения
			if(!value.value(result))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выводим логическое значение записью JSON
			return writer.value(result);
		}
		// Если значение является последовательностью знаков
		case static_cast <uint32_t> (abc::type_t::STRING): {
			// Извлекаемая последовательность знаков
			string result = "";
			// Выполняем извлечение последовательности знаков
			if(!value.value(result))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выводим последовательность знаков записью JSON
			return writer.value(result);
		}
		// Если значение является вместимым
		case static_cast <uint32_t> (abc::type_t::ARRAY): {
			// Выполняем заведение вместимого записи JSON
			if(!writer.array())
				// Выходим из метода, заведение отвечено отказом
				return false;
			// Выполняем перебор всех значений вместимого
			for(size_t i = 0; i < value.size(); i++){
				// Выполняем подачу значения вместимого писателю JSON
				if(!this->feed(value[i], writer, depth + 1))
					// Выходим из метода, подача отвечена отказом
					return false;
			}
			// Выполняем закрытие вместимого записи JSON
			return writer.close();
		}
		// Если значение является отображением
		case static_cast <uint32_t> (abc::type_t::MAP): {
			// Выполняем заведение отображения записи JSON
			if(!writer.object())
				// Выходим из метода, заведение отвечено отказом
				return false;
			// Выполняем перебор всех полей отображения
			for(size_t i = 0; i < value.size(); i++){
				// Извлекаемое имя поля отображения
				string name = "";
				// Выполняем извлечение имени поля отображения
				if(!value.key(i).value(name)){
					/**
					 * Имя поля, знаками не выражаемое, записи JSON неведомо: у неё
					 * именем поля бывает лишь последовательность знаков
					 */
					if(this->_settings.narrow == narrow_t::SKIP)
						// Продолжаем перебор полей отображения дальше
						continue;
					// Запоминаем код отказа перевода
					this->_error = error_t::UNSUPPORTED;
					// Выходим из метода, перевод отвечен отказом
					return false;
				}
				// Выполняем запись имени поля отображения
				if(!writer.key(name))
					// Выходим из метода, запись отвечена отказом
					return false;
				// Выполняем подачу значения поля отображения писателю JSON
				if(!this->feed(value[name], writer, depth + 1))
					// Выходим из метода, подача отвечена отказом
					return false;
			}
			// Выполняем закрытие отображения записи JSON
			return writer.close();
		}
	}
	// Если значение является числом дробным
	if(value.is(abc::type_t::REAL)){
		// Извлекаемое число дробное
		double result = 0.;
		// Выполняем извлечение числа дробного
		if(!value.value(result))
			// Выходим из метода, извлечение отвечено отказом
			return false;
		// Выводим число дробное записью JSON
		return writer.value(result);
	}
	// Если значение является числом целым без знака
	if(value.is(abc::type_t::UNSIGNED)){
		// Извлекаемое число целое без знака
		uint64_t result = 0;
		// Выполняем извлечение числа целого без знака
		if(!value.value(result))
			// Выходим из метода, извлечение отвечено отказом
			return false;
		// Выводим число целое без знака записью JSON
		return writer.value(result);
	}
	// Если значение является числом целым со знаком
	if(value.is(abc::type_t::SIGNED)){
		// Извлекаемое число целое со знаком
		int64_t result = 0;
		// Выполняем извлечение числа целого со знаком
		if(!value.value(result))
			// Выходим из метода, извлечение отвечено отказом
			return false;
		// Выводим число целое со знаком записью JSON
		return writer.value(result);
	}
	/**
	 * Виды ABC, записи JSON неведомые - двоичные данные, отметка времени,
	 * опознаватель, десятичное с точным разрядом и целое сверх родных видов -
	 * обращаются по правилу, настройками заданному
	 */
	switch(static_cast <uint8_t> (this->_settings.narrow)){
		// Если вид, кодеку неведомый, следует пропустить вовсе
		case static_cast <uint8_t> (narrow_t::SKIP):
			// Выводим пустое значение записью JSON
			return writer.null();
		// Если вид, кодеку неведомый, следует обратить в знаки
		case static_cast <uint8_t> (narrow_t::TEXT): {
			// Извлекаемая последовательность знаков
			string result = "";
			// Если значение выражается последовательностью знаков
			if(value.value(result))
				// Выводим последовательность знаков записью JSON
				return writer.value(result);
			// Если значение является отметкою времени
			if(value.is(abc::type_t::TIME)){
				// Извлекаемая отметка времени
				int64_t stamp = 0;
				// Выполняем извлечение отметки времени числом
				if(value.value(stamp))
					// Выводим отметку времени числом записи JSON
					return writer.value(stamp);
			}
			// Если значение хранит запись числа знаками
			if(!value.text().empty())
				// Выводим запись числа последовательностью знаков
				return writer.value(value.text());
		} break;
	}
	// Запоминаем код отказа перевода
	this->_error = error_t::UNSUPPORTED;
	// Выходим из метода, перевод отвечен отказом
	return false;
}

/**
 * @brief Метод укладки значения JSON в значение ABC
 *
 * @param value  значение документа JSON
 * @param result укладываемое значение контейнера ABC
 * @param depth  глубина обхода дерева
 * @return       результат укладки
 *
 */
bool awh::codec::Bridge::absorb(const json::Document::value_t & value, abc::value_t & result, const uint32_t depth) noexcept {
	// Если глубина обхода превысила предел перевода
	if(depth > this->_settings.depth){
		// Запоминаем код отказа перевода
		this->_error = error_t::DEEP_TREE;
		// Выходим из метода, обход остановлен
		return false;
	}
	// Выполняем очистку укладываемого значения
	result.clear();
	// Если значение документа JSON недействительно
	if(!value.valid())
		// Выходим из метода, укладывать нечего
		return true;
	// Определяем вид значения документа JSON
	switch(static_cast <uint32_t> (value.type())){
		// Если значение является пустым
		case static_cast <uint32_t> (json::type_t::NUL):
			// Выполняем укладку пустого значения
			result = abc::value_t(abc::kind_t::NUL);
		return true;
		// Если значение является логическим
		case static_cast <uint32_t> (json::type_t::BOOL): {
			// Извлекаемое логическое значение
			bool number = false;
			// Выполняем извлечение логического значения
			if(!value.value(number))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выполняем укладку логического значения
			result = abc::value_t(number);
		} return true;
		// Если значение является последовательностью знаков
		case static_cast <uint32_t> (json::type_t::STRING): {
			// Извлекаемая последовательность знаков
			string text = "";
			// Выполняем извлечение последовательности знаков
			if(!value.value(text))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выполняем укладку последовательности знаков
			result = abc::value_t(text);
		} return true;
		// Если значение является массивом
		case static_cast <uint32_t> (json::type_t::ARRAY): {
			// Выполняем заведение вместимого контейнера ABC
			result = abc::value_t(abc::kind_t::ARRAY);
			// Выполняем перебор всех значений массива
			for(size_t i = 0; i < value.size(); i++){
				// Выполняем укладку значения массива в значение вместимого
				if(!this->absorb(value[i], result[i], depth + 1))
					// Выходим из метода, укладка отвечена отказом
					return false;
			}
		} return true;
		// Если значение является объектом
		case static_cast <uint32_t> (json::type_t::OBJECT): {
			// Выполняем заведение отображения контейнера ABC
			result = abc::value_t(abc::kind_t::MAP);
			// Выполняем перебор всех полей объекта
			for(auto item = value.begin(); item.valid(); item = item.next()){
				// Выполняем укладку значения поля в поле отображения
				if(!this->absorb(item, result[string(item.name())], depth + 1))
					// Выходим из метода, укладка отвечена отказом
					return false;
			}
		} return true;
	}
	// Если значение является числом дробным
	if(value.is(json::type_t::REAL)){
		// Извлекаемое число дробное
		double number = 0.;
		// Выполняем извлечение числа дробного
		if(!value.value(number))
			// Выходим из метода, извлечение отвечено отказом
			return false;
		// Выполняем укладку числа дробного
		result = abc::value_t(number);
		// Сообщаем, что укладка значения выполнена
		return true;
	}
	// Если значение является числом целым без знака
	if(value.is(json::type_t::UNSIGNED)){
		// Извлекаемое число целое без знака
		uint64_t number = 0;
		// Выполняем извлечение числа целого без знака
		if(!value.value(number))
			// Выходим из метода, извлечение отвечено отказом
			return false;
		// Выполняем укладку числа целого без знака
		result = abc::value_t(number);
		// Сообщаем, что укладка значения выполнена
		return true;
	}
	// Если значение является числом целым со знаком
	if(value.is(json::type_t::SIGNED)){
		// Извлекаемое число целое со знаком
		int64_t number = 0;
		// Выполняем извлечение числа целого со знаком
		if(!value.value(number))
			// Выходим из метода, извлечение отвечено отказом
			return false;
		// Выполняем укладку числа целого со знаком
		result = abc::value_t(number);
		// Сообщаем, что укладка значения выполнена
		return true;
	}
	/**
	 * Число, ни в один родной вид не вместимое, укладывается записью знаками.
	 *
	 * @warning Запись числа берётся ходом `raw()`, а НЕ `text()`: последний выдаёт
	 *          содержимое лишь у строковых узлов, а у числа отвечает пустотой.
	 *          Замерено срывом: числа «1e400» и «123456789012345678901234567890»
	 *          проходили мостом в пустую строку, теряя значение целиком
	 *
	 * @note Обратный ход выдаст такое число уже последовательностью знаков, в
	 *       кавычках, а терять разряды переводом в родной вид - хуже.
	 *
	 *       Причина знаков лежит НЕ у контейнера ABC: ход `Value::decimal(buffer,
	 *       size, negative, exponent)` заведён 03.09.2026 и вид этот собрать
	 *       позволяет. Величины нет у САМОГО JSON: число вида `EXTENDED` он
	 *       держит в хранилище знаков записью, а не октетами, и `raw()` выдаёт
	 *       именно запись. Накормить `decimal` октетами мосту нечем, не разбирая
	 *       десятичную запись произвольной длины самому, - а это своё длинное
	 *       число внутри моста, чего он делать не должен
	 */
	result = abc::value_t(value.raw());
	// Сообщаем, что укладка значения выполнена
	return true;
}

/**
 * @brief Метод перевода дерева ABC в запись JSON
 *
 * @param value  дерево значений контейнера ABC
 * @param result собранная запись JSON
 * @return       результат перевода
 *
 */
bool awh::codec::Bridge::encode(const abc::value_t & value, string & result) noexcept {
	// Выполняем очистку собранной записи JSON
	result.clear();
	// Выполняем сброс кода отказа последнего перевода
	this->_error = error_t::NONE;
	// Создаём писателя записи JSON
	json::writer_t writer(this->_log);
	// Получаем настройки записи текста JSON
	json::writer_t::settings_t settings = writer.settings();
	// Устанавливаем вид оформления собираемой записи
	settings.format = this->_settings.format;
	// Устанавливаем настройки записи текста JSON
	writer.settings(settings);
	// Выполняем подачу дерева значений писателю JSON
	if(!this->feed(value, writer, 0)){
		// Если код отказа перевода ещё не запомнен
		if(this->_error == error_t::NONE)
			// Запоминаем код отказа перевода
			this->_error = error_t::WRITING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем завершение записи JSON
	if(!writer.finish()){
		// Запоминаем код отказа перевода
		this->_error = error_t::WRITING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем изъятие собранной записи JSON
	result = writer.take();
	// Сообщаем, что перевод выполнен
	return true;
}

/**
 * @brief Метод перевода записи JSON в дерево ABC
 *
 * @param text   запись JSON для перевода
 * @param result собранное дерево значений контейнера ABC
 * @return       результат перевода
 *
 */
bool awh::codec::Bridge::decode(const string_view text, abc::value_t & result) noexcept {
	// Выполняем сброс кода отказа последнего перевода
	this->_error = error_t::NONE;
	// Создаём документ записи JSON
	json::document_t document(this->_log);
	// Выполняем разбор поданной записи JSON
	if(!document.parse(text)){
		// Запоминаем код отказа перевода
		this->_error = error_t::PARSING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем укладку разобранного дерева в значение контейнера ABC
	return this->absorb(document.root(), result, 0);
}

/**
 * @brief Метод извлечения кода отказа последнего перевода
 *
 * @return код отказа последнего перевода
 *
 */
Bridge::error_t awh::codec::Bridge::error() const noexcept {
	// Выводим код отказа последнего перевода
	return this->_error;
}

/**
 * @brief Метод извлечения настроек перевода
 *
 * @return настройки перевода
 *
 */
const Bridge::settings_t & awh::codec::Bridge::settings() const noexcept {
	// Выводим настройки перевода
	return this->_settings;
}

/**
 * @brief Метод установки настроек перевода
 *
 * @param settings настройки перевода
 *
 */
void awh::codec::Bridge::settings(const settings_t & settings) noexcept {
	// Устанавливаем настройки перевода
	this->_settings = settings;
}
