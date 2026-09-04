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
bool awh::codec::Bridge::encodeJSON(const abc::value_t & value, string & result) noexcept {
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
bool awh::codec::Bridge::decodeJSON(const string_view text, abc::value_t & result) noexcept {
	// Выполняем сброс кода отказа последнего перевода
	this->_error = error_t::NONE;
	/**
	 * Выполняем очистку собираемого дерева ДО разбора
	 *
	 * @warning Очистка эта обязательна и стоит именно здесь: прежде при отказе
	 *          разбора дерево оставалось нетронутым, и потребитель, признака не
	 *          проверивший, работал с ПРЕЖНИМ своим деревом как с новым - `size()`
	 *          держал узлы, `at()` отдавал значения, а разобрано не было ничего.
	 *          Тот же порок найден 03.09.2026 у `abc::Document::parse` владельцем
	 *          контейнера: отказ на середине записи оставлял обломок
	 */
	result.clear();
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
 * @brief Метод снятия отменяющей записи со звена пути
 *
 * @param link звено пути с отменяющими записями
 * @return     исходное имя потомка
 *
 * @warning Порядок снятия предписан RFC 6901 и обратным быть не может: снятие
 *          `~0` прежде `~1` обратило бы записанное `~01` в косую черту вместо
 *          записи `~1`. Тот же довод стоит у кодека JSON
 *
 */
string awh::codec::Bridge::unescape(const string & link) const noexcept {
	// Собираемое исходное имя потомка
	string result = "";
	// Выполняем резервирование памяти под исходное имя
	result.reserve(link.size());
	// Выполняем перебор всех знаков звена пути
	for(size_t i = 0; i < link.size(); i++){
		// Если знак является началом отменяющей записи
		if((link[i] == '~') && ((i + 1) < link.size())){
			// Определяем вид отменяющей записи
			switch(link[i + 1]){
				// Если записана косая черта
				case '1': {
					// Выполняем добавление косой черты
					result.append(1, '/');
					// Выполняем пропуск второго знака записи
					i++;
				} continue;
				// Если записан знак самой отменяющей записи
				case '0': {
					// Выполняем добавление знака отменяющей записи
					result.append(1, '~');
					// Выполняем пропуск второго знака записи
					i++;
				} continue;
			}
		}
		// Выполняем добавление знака как он есть
		result.append(1, link[i]);
	}
	// Выводим собранное исходное имя потомка
	return result;
}

/**
 * @brief Метод укладки значения YAML в значение ABC
 *
 * @param document документ записи YAML
 * @param path     путь к укладываемому значению
 * @param result   укладываемое значение контейнера ABC
 * @param depth    глубина обхода дерева
 * @return         результат укладки
 *
 * @warning Обход идёт ходом `at()`, а не ходом `get()`: при повторе имени
 *          `get()` отдаёт ДЕЙСТВУЮЩЕЕ объявление по своей политике, тогда как
 *          `at()` отдаёт ВСЕ объявления перечнем. Это не изъян кодека, а два
 *          разных обещания, и мост, идущий за `get()`, терял бы данные молча
 *
 */
bool awh::codec::Bridge::absorbYAML(const yaml::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept {
	// Если глубина обхода превысила предел перевода
	if(depth > this->_settings.depth){
		// Запоминаем код отказа перевода
		this->_error = error_t::DEEP_TREE;
		// Выходим из метода, обход остановлен
		return false;
	}
	// Выполняем очистку укладываемого значения
	result.clear();
	// Выполняем извлечение значения документа YAML по пути
	const yaml::Document::Value & value = document.at(path);
	// Если значение документа YAML недействительно
	if(!value.valid())
		// Выходим из метода, укладывать нечего
		return true;
	// Определяем вид значения документа YAML
	switch(static_cast <uint32_t> (value.type())){
		// Если значение является пустым
		case static_cast <uint32_t> (yaml::type_t::NUL):
			// Выполняем укладку пустого значения
			result = abc::value_t(abc::kind_t::NUL);
		return true;
		// Если значение является логическим
		case static_cast <uint32_t> (yaml::type_t::BOOL): {
			// Извлекаемое логическое значение
			bool number = false;
			// Выполняем извлечение логического значения
			if(!value.value(number))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выполняем укладку логического значения
			result = abc::value_t(number);
		} return true;
		// Если значение является перечнем значений
		case static_cast <uint32_t> (yaml::type_t::SEQUENCE): {
			// Выполняем заведение вместимого контейнера ABC
			result = abc::value_t(abc::kind_t::ARRAY);
			// Выполняем перебор всех значений перечня
			for(size_t i = 0; i < value.size(); i++){
				// Выполняем укладку значения перечня в значение вместимого
				if(!this->absorbYAML(document, path + "/" + std::to_string(i), result[i], depth + 1))
					// Выходим из метода, укладка отвечена отказом
					return false;
			}
		} return true;
		// Если значение является отображением пар
		case static_cast <uint32_t> (yaml::type_t::MAPPING): {
			// Выполняем заведение отображения контейнера ABC
			result = abc::value_t(abc::kind_t::MAP);
			// Выполняем извлечение перечня звеньев пути отображения
			const vector <string> & links = document.keys(path);
			// Выполняем перебор всех звеньев пути отображения
			for(auto & link : links){
				/**
				 * Выполняем укладку значения потомка в поле отображения
				 *
				 * @warning Путь собирается звеном КАК ОНО ВЫДАНО, с отменяющими
				 *          записями, а имя поля берётся звеном со снятой записью:
				 *          кодек ждёт путь по RFC 6901, а контейнер ABC хранит имя
				 *          исходное. Смешение этих двух видов теряет имена с косой
				 *          чертой молча
				 */
				if(!this->absorbYAML(document, path + "/" + link, result[this->unescape(link)], depth + 1))
					// Выходим из метода, укладка отвечена отказом
					return false;
			}
		} return true;
	}
	// Если значение является числом дробным
	if(value.is(yaml::type_t::DOUBLE) || value.is(yaml::type_t::FLOAT)){
		// Извлекаемое число дробное
		double number = 0.;
		// Выполняем извлечение числа дробного
		if(!value.value(number))
			// Выходим из метода, извлечение отвечено отказом
			return false;
		// Выполняем укладку числа дробного
		result = abc::value_t(number);
		// Выводим результат укладки
		return true;
	}
	// Если значение является целым со знаком
	if(value.is(yaml::type_t::INT64) || value.is(yaml::type_t::INT32) || value.is(yaml::type_t::INT16) || value.is(yaml::type_t::INT8)){
		// Извлекаемое целое со знаком
		int64_t number = 0;
		// Выполняем извлечение целого со знаком
		if(!value.value(number))
			// Выходим из метода, извлечение отвечено отказом
			return false;
		// Выполняем укладку целого со знаком
		result = abc::value_t(number);
		// Выводим результат укладки
		return true;
	}
	// Если значение является целым без знака
	if(value.is(yaml::type_t::UINT64) || value.is(yaml::type_t::UINT32) || value.is(yaml::type_t::UINT16) || value.is(yaml::type_t::UINT8)){
		// Извлекаемое целое без знака
		uint64_t number = 0;
		// Выполняем извлечение целого без знака
		if(!value.value(number))
			// Выходим из метода, извлечение отвечено отказом
			return false;
		// Выполняем укладку целого без знака
		result = abc::value_t(number);
		// Выводим результат укладки
		return true;
	}
	/**
	 * Выполняем укладку значения последовательностью знаков
	 *
	 * @warning Ход этот берёт на себя и вид STRING, и всякий вид, мостом не
	 *          разобранный: запись YAML несёт метки (`!!binary`, `!!timestamp`),
	 *          под которые у контейнера ABC свои виды, а у моста разбора пока нет
	 */
	result = abc::value_t(string(value.text()));
	// Выводим результат укладки
	return true;
}

/**
 * @brief Метод перевода записи YAML в дерево ABC
 *
 * @param text   запись YAML для перевода
 * @param result собранное дерево значений контейнера ABC
 * @return       результат перевода
 *
 */
bool awh::codec::Bridge::decodeYAML(const string_view text, abc::value_t & result) noexcept {
	// Создаём документ записи YAML
	yaml::document_t document(this->_log);
	// Выполняем разбор поданной записи YAML
	if(!document.parse(string(text))){
		// Запоминаем код отказа перевода
		this->_error = error_t::PARSING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем укладку разобранного дерева в значение контейнера ABC
	return this->absorbYAML(document, "", result, 0);
}

/**
 * @brief Метод укладки значения XML в значение ABC
 *
 * @param document документ записи XML
 * @param path     путь к укладываемому значению
 * @param result   укладываемое значение контейнера ABC
 * @param depth    глубина обхода дерева
 * @return         результат укладки
 *
 * @warning Признак простого значения берётся ПЕРЕЧНЕМ ЗВЕНЬЕВ, а не числом
 *          потомков узла: примечание разрывает содержимое на две части, а
 *          раздел дословного текста узлом разметки не является - счёт потомков
 *          в обоих случаях лжёт, и узел с одним лишь текстом внутри выглядел бы
 *          вместилищем. Пустой перечень звеньев означает, что узлов разметки
 *          внутри нет, и значение простое
 *
 */
bool awh::codec::Bridge::absorbXML(const xml::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept {
	// Если глубина обхода превысила предел перевода
	if(depth > this->_settings.depth){
		// Запоминаем код отказа перевода
		this->_error = error_t::DEEP_TREE;
		// Выходим из метода, обход остановлен
		return false;
	}
	// Выполняем очистку укладываемого значения
	result.clear();
	// Выполняем извлечение значения документа XML по пути
	const xml::node_t & value = document.at(path);
	// Если значение документа XML недействительно
	if(!value.valid())
		// Выходим из метода, укладывать нечего
		return true;
	// Выполняем извлечение перечня звеньев пути узла
	const vector <string> & links = document.keys(path);
	// Признак наличия внутри узла хотя бы одного узла разметки
	bool markup = false;
	/**
	 * Выполняем поиск узлов разметки среди потомков
	 *
	 * @warning Пустота перечня звеньев признаком простого значения НЕ является,
	 *          и это замерено: у записи `<name>значение</name>` ход `keys()`
	 *          выдаёт звено «0» - текстовое содержимое узла тоже потомок и тоже
	 *          звено. Прежняя редакция считала простым значением узел с пустым
	 *          перечнем, и всякий узел с текстом внутри ложился ОТОБРАЖЕНИЕМ со
	 *          звеном «0» вместо своего текста. Признак берётся видом КАЖДОГО
	 *          потомка, а не их числом и не пустотой перечня
	 */
	for(auto & link : links){
		// Если потомок оказался узлом разметки
		if((document.at(path + "/" + link).kind() == xml::kind_t::ELEMENT)){
			// Запоминаем наличие узла разметки внутри
			markup = true;
			// Выходим из поиска, довольно одного
			break;
		}
	}
	// Если узлов разметки внутри не нашлось
	if(!markup){
		/**
		 * Выполняем укладку узла собранным текстом
		 *
		 * @warning Текст собирается по ВСЕЙ ГЛУБИНЕ узла: запись
		 *          `<a>свой<b>чужой</b></a>` даёт «свойчужой». Здесь это верно,
		 *          потому что узлов разметки внутри нет по условию ветви
		 */
		result = abc::value_t(value.text());
		// Выводим результат укладки
		return true;
	}
	// Выполняем заведение отображения контейнера ABC
	result = abc::value_t(abc::kind_t::MAP);
	// Выполняем перебор всех звеньев пути узла
	for(auto & link : links){
		// Если потомок узлом разметки не является
		if(!(document.at(path + "/" + link).kind() == xml::kind_t::ELEMENT))
			// Пропускаем потомка, вместилищу он не принадлежит
			continue;
		// Выполняем укладку значения потомка в поле отображения
		if(!this->absorbXML(document, path + "/" + link, result[this->unescape(link)], depth + 1))
			// Выходим из метода, укладка отвечена отказом
			return false;
	}
	// Выводим результат укладки
	return true;
}

/**
 * @brief Метод перевода записи XML в дерево ABC
 *
 * @param text   запись XML для перевода
 * @param result собранное дерево значений контейнера ABC
 * @return       результат перевода
 *
 */
bool awh::codec::Bridge::decodeXML(const string_view text, abc::value_t & result) noexcept {
	// Создаём документ записи XML
	xml::document_t document(this->_log);
	// Выполняем разбор поданной записи XML
	if(!document.parse(string(text))){
		// Запоминаем код отказа перевода
		this->_error = error_t::PARSING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем укладку разобранного дерева в значение контейнера ABC
	return this->absorbXML(document, "", result, 0);
}

/**
 * @brief Метод укладки значения TOML в значение ABC
 *
 * @param document документ записи TOML
 * @param path     путь к укладываемому значению
 * @param result   укладываемое значение контейнера ABC
 * @param depth    глубина обхода дерева
 * @return         результат укладки
 *
 * @warning Виды времени (OFFSET_DATETIME, LOCAL_DATETIME, LOCAL_DATE,
 *          LOCAL_TIME) укладываются последовательностью знаков, а не видом TIME
 *          контейнера ABC: у записи TOML время местное и время со смещением
 *          суть РАЗНЫЕ виды стандарта, и сведение их к одному мгновению
 *          придумало бы смещение, в записи не стоявшее. Обратный перевод из
 *          текста восстановит запись дословно
 *
 */
bool awh::codec::Bridge::absorbTOML(const toml::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept {
	// Если глубина обхода превысила предел перевода
	if(depth > this->_settings.depth){
		// Запоминаем код отказа перевода
		this->_error = error_t::DEEP_TREE;
		// Выходим из метода, обход остановлен
		return false;
	}
	// Выполняем очистку укладываемого значения
	result.clear();
	// Выполняем извлечение значения документа TOML по пути
	const toml::Value & value = document.at(path);
	// Если значение документа TOML недействительно
	if(!value.valid())
		// Выходим из метода, укладывать нечего
		return true;
	// Определяем вид значения документа TOML
	switch(static_cast <uint32_t> (value.type())){
		// Если значение является логическим
		case static_cast <uint32_t> (toml::type_t::BOOLEAN): {
			// Извлекаемое логическое значение
			bool number = false;
			// Выполняем извлечение логического значения
			if(!value.value(number))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выполняем укладку логического значения
			result = abc::value_t(number);
		} return true;
		// Если значение является целым
		case static_cast <uint32_t> (toml::type_t::INTEGER): {
			// Извлекаемое целое со знаком
			int64_t number = 0;
			// Выполняем извлечение целого со знаком
			if(!value.value(number))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выполняем укладку целого со знаком
			result = abc::value_t(number);
		} return true;
		// Если значение является дробным
		case static_cast <uint32_t> (toml::type_t::FLOAT): {
			// Извлекаемое число дробное
			double number = 0.;
			// Выполняем извлечение числа дробного
			if(!value.value(number))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выполняем укладку числа дробного
			result = abc::value_t(number);
		} return true;
		// Если значение является перечнем значений
		case static_cast <uint32_t> (toml::type_t::ARRAY): {
			// Выполняем заведение вместимого контейнера ABC
			result = abc::value_t(abc::kind_t::ARRAY);
			// Выполняем перебор всех значений перечня
			for(size_t i = 0; i < value.size(); i++){
				// Выполняем укладку значения перечня в значение вместимого
				if(!this->absorbTOML(document, path + "/" + std::to_string(i), result[i], depth + 1))
					// Выходим из метода, укладка отвечена отказом
					return false;
			}
		} return true;
		// Если значение является таблицей
		case static_cast <uint32_t> (toml::type_t::TABLE): {
			// Выполняем заведение отображения контейнера ABC
			result = abc::value_t(abc::kind_t::MAP);
			// Выполняем извлечение перечня звеньев пути таблицы
			const vector <string> & links = document.keys(path);
			// Выполняем перебор всех звеньев пути таблицы
			for(auto & link : links){
				// Выполняем укладку значения потомка в поле отображения
				if(!this->absorbTOML(document, path + "/" + link, result[this->unescape(link)], depth + 1))
					// Выходим из метода, укладка отвечена отказом
					return false;
			}
		} return true;
	}
	/**
	 * Выполняем укладку значения последовательностью знаков
	 *
	 * @warning Ход этот берёт на себя и вид STRING, и все четыре вида времени:
	 *          довод их сохранения текстом стоит в предупреждении к ходу
	 */
	result = abc::value_t(string(value.text()));
	// Выводим результат укладки
	return true;
}

/**
 * @brief Метод перевода записи TOML в дерево ABC
 *
 * @param text   запись TOML для перевода
 * @param result собранное дерево значений контейнера ABC
 * @return       результат перевода
 *
 * @warning Обход начинается с набора одноимённых таблиц наравне с прочим: ход
 *          `keys()` выдаёт на них ЧИСЛОВЫЕ звенья, тогда как прежний плоский
 *          перечень дочерних имён на них законно молчит, ибо набор таблиц
 *          таблицею не является. Мост обязан звать общий вид, а не прежний
 *
 */
bool awh::codec::Bridge::decodeTOML(const string_view text, abc::value_t & result) noexcept {
	// Создаём документ записи TOML
	toml::document_t document(this->_log);
	// Выполняем разбор поданной записи TOML
	if(!document.parse(string(text))){
		// Запоминаем код отказа перевода
		this->_error = error_t::PARSING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем укладку разобранного дерева в значение контейнера ABC
	return this->absorbTOML(document, "", result, 0);
}

/**
 * @brief Метод перевода записи INI в дерево ABC
 *
 * @param text   запись INI для перевода
 * @param result собранное дерево значений контейнера ABC
 * @return       результат перевода
 *
 * @warning Имена разделов и свойств снимаются КОПИЕЙ прежде обращения к дереву,
 *          и это не расточительство: кодек выдаёт их видами в своё хранилище
 *          знаков, а подстановка обращений считается по требованию - обращение
 *          к дереву, её устаревшей заставшее, хранилище ПЕРЕМЕЩАЕТ, и вид,
 *          взятый до пересчёта, повисает. Перебирая виды и зовя при этом `at()`,
 *          мост читал бы освобождённую память. Оговорка эта стоит в договоре
 *          обоих ходов кодека
 *
 * @warning Обход идёт родным ходом кодека, а не общим `keys(путь)`: у этого
 *          кодека общего вида покуда нет. Оттого глубина здесь ограничена
 *          устройством наречия - раздел, свойство, - и это НЕ недоделка моста,
 *          а свойство формата: дерева произвольной глубины в INI не бывает
 *
 */
bool awh::codec::Bridge::decodeINI(const string_view text, abc::value_t & result) noexcept {
	// Создаём документ записи INI
	ini::document_t document(this->_log);
	// Выполняем разбор поданной записи INI
	if(!document.parse(string(text))){
		// Запоминаем код отказа перевода
		this->_error = error_t::PARSING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем заведение корня дерева отображением
	result = abc::value_t(abc::kind_t::MAP);
	// Снимаемые копией имена разделов
	vector <pair <string, string>> sections;
	{
		// Выполняем извлечение перечня объявленных разделов
		const vector <ini::name_t> & names = document.sections();
		// Выполняем резервирование памяти под имена разделов
		sections.reserve(names.size() + 1);
		// Выполняем заведение безымянного раздела верхнего уровня
		sections.emplace_back(string(""), string(""));
		// Выполняем перебор всех объявленных разделов
		for(auto & name : names){
			// Если раздел оказался безымянным
			if(name.section.empty())
				// Пропускаем раздел, он уже заведён
				continue;
			// Выполняем снятие имени раздела копией
			sections.emplace_back(string(name.section), string(name.subsection));
		}
	}
	// Выполняем перебор всех объявленных разделов
	for(auto & section : sections){
		// Снимаемые копией имена свойств раздела
		vector <string> names;
		{
			// Выполняем извлечение перечня имён свойств раздела
			const vector <string_view> & keys = document.keys(section.first, section.second);
			// Выполняем резервирование памяти под имена свойств
			names.reserve(keys.size());
			// Выполняем перебор всех имён свойств раздела
			for(auto & key : keys)
				// Выполняем снятие имени свойства копией
				names.emplace_back(key);
		}
		/**
		 * Выполняем выбор вместилища свойств раздела
		 *
		 * @warning Свойства безымянного раздела ложатся прямо в корень дерева, а
		 *          не в поле с пустым именем: раздела этого в тексте настроек нет,
		 *          он лишь означает «до первого объявления раздела»
		 */
		abc::value_t & node = (section.first.empty() ? result : result[section.first + (section.second.empty() ? "" : "/" + section.second)]);
		// Если вместилищем оказалось поле раздела
		if(!section.first.empty())
			// Выполняем заведение содержимого раздела отображением
			node = abc::value_t(abc::kind_t::MAP);
		// Выполняем перебор всех имён свойств раздела
		for(auto & name : names){
			// Снимаемые копией значения свойства
			vector <string> records;
			{
				/**
				 * Выполняем извлечение перечня значений свойства
				 *
				 * @warning Ход этот берётся вместо `get()` намеренно: `get()` отдаёт
				 *          ДЕЙСТВУЮЩЕЕ объявление по политике повторов, а этот - ВСЕ
				 *          объявления в порядке следования. Наречия Git и systemd
				 *          задают перечень значений именно повтором свойства, и мост,
				 *          идущий за `get()`, терял бы их молча
				 */
				const vector <string_view> & values = document.values(name, section.first, section.second);
				// Выполняем резервирование памяти под значения свойства
				records.reserve(values.size());
				// Выполняем перебор всех значений свойства
				for(auto & value : values)
					// Выполняем снятие значения свойства копией
					records.emplace_back(value);
			}
			// Если свойство объявлено единожды
			if(records.size() == 1)
				// Выполняем укладку значения свойства последовательностью знаков
				node[name] = abc::value_t(records.front());
			// Если свойство объявлено несколько раз
			else if(records.size() > 1) {
				// Выполняем заведение вместимого контейнера ABC
				node[name] = abc::value_t(abc::kind_t::ARRAY);
				// Выполняем перебор всех значений свойства
				for(size_t i = 0; i < records.size(); i++)
					// Выполняем укладку значения свойства последовательностью знаков
					node[name][i] = abc::value_t(records.at(i));
			}
		}
	}
	// Сообщаем, что перевод выполнен
	return true;
}

/**
 * @brief Метод наложения отменяющей записи на имя потомка
 *
 * @param name исходное имя потомка
 * @return     звено пути с отменяющими записями
 *
 * @warning Порядок наложения предписан RFC 6901 и обратным быть не может:
 *          наложение на косую черту прежде наложения на тильду обратило бы
 *          имя `a~/b` в звено `a~01b`, при снятии дающее `a~/b` неверно.
 *          Тильда записывается ПЕРВОЙ
 *
 */
string awh::codec::Bridge::escape(const string & name) const noexcept {
	// Собираемое звено пути
	string result = "";
	// Выполняем резервирование памяти под звено пути
	result.reserve(name.size());
	// Выполняем перебор всех знаков имени потомка
	for(auto & letter : name){
		// Определяем знак имени потомка
		switch(letter){
			// Если знаком оказалась тильда
			case '~':
				// Выполняем наложение отменяющей записи тильды
				result.append("~0");
			break;
			// Если знаком оказалась косая черта
			case '/':
				// Выполняем наложение отменяющей записи косой черты
				result.append("~1");
			break;
			// Если знак отменяющей записи не требует
			default:
				// Выполняем добавление знака как он есть
				result.append(1, letter);
		}
	}
	// Выводим собранное звено пути
	return result;
}

/**
 * @brief Метод подачи значения ABC документу YAML
 *
 * @param value    значение контейнера ABC
 * @param document собираемый документ записи YAML
 * @param path     путь к подаваемому значению
 * @param depth    глубина обхода дерева
 * @return         результат подачи
 *
 * @warning Виды BLOB, TIME, UUID и CUSTOM записи YAML неведомы, и обращение с
 *          ними решает настройка сужения: STRICT отвечает отказом, TEXT кладёт
 *          их последовательностью знаков, SKIP пропускает вовсе. Умолчание -
 *          TEXT, ибо запись настроек читает человек, и потеря значения ему
 *          хуже неточного его вида
 *
 */
bool awh::codec::Bridge::feedYAML(const abc::value_t & value, yaml::Value & result, const uint32_t depth) noexcept {
	// Если глубина обхода превысила предел перевода
	if(depth > this->_settings.depth){
		// Запоминаем код отказа перевода
		this->_error = error_t::DEEP_TREE;
		// Выходим из метода, обход остановлен
		return false;
	}
	// Если значение контейнера ABC недействительно вовсе
	if(!value.valid())
		// Выходим из метода, подавать нечего
		return true;
	// Определяем вид значения контейнера ABC
	switch(static_cast <uint32_t> (value.type())){
		// Если значение является пустым
		case static_cast <uint32_t> (abc::type_t::NUL):
			/**
			 * Выходим из метода, пустое значение уже заведено
			 *
			 * @warning Значение, заведённое умолчанием, у записи YAML и есть
			 *          пустое: заводителя пустоты у владеющего значения нет, он
			 *          принадлежит СТРОИТЕЛЮ, а это иной класс
			 */
			return true;
		// Если значение является логическим
		case static_cast <uint32_t> (abc::type_t::BOOL): {
			// Извлекаемое логическое значение
			bool number = false;
			// Выполняем извлечение логического значения
			if(!value.value(number))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выполняем заведение логического значения
			result = yaml::Value(number);
		} return true;
		// Если значение является последовательностью знаков
		case static_cast <uint32_t> (abc::type_t::STRING):
			// Выводим результат записи последовательности знаков
			result = yaml::Value(value.text());
			// Выводим результат подачи
			return true;
		// Если значение является вместимым
		case static_cast <uint32_t> (abc::type_t::ARRAY): {
			/**
			 * Выполняем перебор всех значений вместимого
			 *
			 * @warning Перечень заводится самим добавлением, отдельного заводителя
			 *          у владеющего значения нет: `sequence()` принадлежит строителю
			 */
			for(size_t i = 0; i < value.size(); i++){
				// Собираемое значение перечня
				yaml::Value item;
				// Выполняем подачу значения вместимого значению перечня
				if(!this->feedYAML(value[i], item, depth + 1))
					// Выходим из метода, подача отвечена отказом
					return false;
				// Выполняем добавление значения концом перечня
				if(!result.push(item))
					// Выходим из метода, добавление отвечено отказом
					return false;
			}
		} return true;
		// Если значение является отображением
		case static_cast <uint32_t> (abc::type_t::MAP): {
			/**
			 * Выполняем перебор всех полей отображения
			 *
			 * @warning Отображение заводится самой вставкой, отдельного заводителя
			 *          у владеющего значения нет: `mapping()` принадлежит строителю
			 */
			for(size_t i = 0; i < value.size(); i++){
				// Извлекаемое имя поля отображения
				string name = "";
				// Выполняем извлечение имени поля отображения
				if(!value.key(i).value(name)){
					/**
					 * Имя поля, знаками не выражаемое, записи YAML неведомо: у неё
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
				// Собираемое значение поля отображения
				yaml::Value item;
				// Выполняем подачу значения поля значению отображения
				if(!this->feedYAML(value[name], item, depth + 1))
					// Выходим из метода, подача отвечена отказом
					return false;
				/**
				 * Выполняем добавление поля отображению
				 *
				 * @warning Имя кладётся ИСХОДНЫМ, а не записанным по RFC 6901:
				 *          отменяющая запись есть свойство ПУТИ, а не имени, и
				 *          ход этот берёт именно имя. Наложи мост запись здесь -
				 *          имя `a/b` легло бы в запись как `a~1b`
				 */
				if(!result.insert(name, item))
					// Выходим из метода, добавление отвечено отказом
					return false;
			}
		} return true;
	}
	// Если значение является числом
	if(value.is(abc::type_t::NUMBER)){
		// Извлекаемое целое со знаком
		int64_t number = 0;
		// Если значение извлекается целым со знаком
		if(value.value(number)){
			// Выполняем заведение целого со знаком
			result = yaml::Value(number);
			// Выводим результат подачи
			return true;
		}
		// Извлекаемое число дробное
		double real = 0.;
		// Если значение извлекается числом дробным
		if(value.value(real)){
			// Выполняем заведение числа дробного
			result = yaml::Value(real);
			// Выводим результат подачи
			return true;
		}
	}
	// Определяем правило обращения с видом, записи YAML неведомым
	switch(static_cast <uint8_t> (this->_settings.narrow)){
		// Если вид надлежит пропустить вовсе
		case static_cast <uint8_t> (narrow_t::SKIP):
			// Выходим из метода, значение пропущено
			return true;
		// Если вид надлежит обратить в последовательность знаков
		case static_cast <uint8_t> (narrow_t::TEXT): {
			// Выполняем заведение последовательности знаков
			result = yaml::Value(value.text());
			// Выводим результат подачи
			return true;
		}
	}
	// Запоминаем код отказа перевода
	this->_error = error_t::UNSUPPORTED;
	// Выходим из метода, перевод отвечен отказом
	return false;
}

/**
 * @brief Метод перевода дерева ABC в запись YAML
 *
 * @param value  дерево значений контейнера ABC
 * @param result собранная запись YAML
 * @return       результат перевода
 *
 */
bool awh::codec::Bridge::encodeYAML(const abc::value_t & value, string & result) noexcept {
	// Создаём документ записи YAML
	yaml::document_t document(this->_log);
	// Собираемое владеющее значение записи YAML
	yaml::Value root;
	// Выполняем подачу дерева значений владеющему значению
	if(!this->feedYAML(value, root, 0)){
		// Если код отказа перевода ещё не запомнен
		if(this->_error == error_t::NONE)
			// Запоминаем код отказа перевода
			this->_error = error_t::WRITING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	/**
	 * Выполняем посадку собранного значения в документ записи YAML
	 *
	 * @warning Значение собирается ЦЕЛИКОМ и садится разом, а не пишется по
	 *          путям ходом `set()`: договор его гласит, что части пути, кроме
	 *          последней, обязаны быть налицо - вместилищ по пути не заводится.
	 *          Замерено: `set()` у пустого документа отвечает отказом даже на
	 *          простейшем пути
	 */
	if(!root.graft(document)){
		// Запоминаем код отказа перевода
		this->_error = error_t::WRITING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем изъятие собранной записи YAML
	result = document.dump();
	// Сообщаем, что перевод выполнен
	return true;
}

/**
 * @brief Метод перевода дерева ABC в запись заданного вида
 *
 * @param value  дерево значений контейнера ABC
 * @param result собранная запись
 * @param format вид записи, в который переводится дерево
 * @return       результат перевода
 *
 * @warning Запись вида ABC ДВОИЧНА, а не текстова, и укладывается в строку
 *          восьмеричными знаками как есть. Строка здесь - вместилище байтов,
 *          а не текст: печатать её, мерить длину в знаках либо сличать с
 *          текстом прочих видов нельзя
 *
 */
bool awh::codec::Bridge::encode(const abc::value_t & value, string & result, const format_t format) noexcept {
	// Выполняем сброс кода отказа последнего перевода
	this->_error = error_t::NONE;
	// Выполняем очистку собираемой записи ДО перевода
	result.clear();
	// Определяем вид записи, в который переводится дерево
	switch(static_cast <uint8_t> (format)){
		// Если запись переводится в вид JSON
		case static_cast <uint8_t> (format_t::JSON):
			// Выводим результат перевода дерева в запись JSON
			return this->encodeJSON(value, result);
		// Если запись переводится в вид YAML
		case static_cast <uint8_t> (format_t::YAML):
			// Выводим результат перевода дерева в запись YAML
			return this->encodeYAML(value, result);
		// Если вид записи мостом ещё не переводится
		default: {
			// Запоминаем код отказа перевода
			this->_error = error_t::UNSUPPORTED;
			// Выводим сообщение о неведомом виде записи
			this->_log->print("Кодек вида %u мостом ещё не переводится", log_t::flag_t::WARNING, static_cast <uint16_t> (format));
			// Выходим из метода, перевод отвечен отказом
			return false;
		}
	}
}

/**
 * @brief Метод перевода записи заданного вида в дерево ABC
 *
 * @param text   запись для перевода
 * @param result собранное дерево значений контейнера ABC
 * @param format вид поданной записи
 * @return       результат перевода
 *
 */
bool awh::codec::Bridge::decode(const string_view text, abc::value_t & result, const format_t format) noexcept {
	// Выполняем сброс кода отказа последнего перевода
	this->_error = error_t::NONE;
	/**
	 * Выполняем очистку собираемого дерева ДО разбора
	 *
	 * @warning Очистка стоит здесь, а не у каждого перевода порознь, чтобы
	 *          дерево очищалось и при неведомом виде записи тоже: иначе
	 *          потребитель, признака не проверивший, работал бы с ПРЕЖНИМ
	 *          своим деревом как с новым
	 */
	result.clear();
	// Определяем вид поданной записи
	switch(static_cast <uint8_t> (format)){
		// Если подана запись вида JSON
		case static_cast <uint8_t> (format_t::JSON):
			// Выводим результат перевода записи JSON в дерево
			return this->decodeJSON(text, result);
		// Если подана запись вида YAML
		case static_cast <uint8_t> (format_t::YAML):
			// Выводим результат перевода записи YAML в дерево
			return this->decodeYAML(text, result);
		// Если подана запись вида XML
		case static_cast <uint8_t> (format_t::XML):
			// Выводим результат перевода записи XML в дерево
			return this->decodeXML(text, result);
		// Если подана запись вида TOML
		case static_cast <uint8_t> (format_t::TOML):
			// Выводим результат перевода записи TOML в дерево
			return this->decodeTOML(text, result);
		// Если подана запись вида INI
		case static_cast <uint8_t> (format_t::INI):
			// Выводим результат перевода записи INI в дерево
			return this->decodeINI(text, result);
		// Если вид записи мостом ещё не переводится
		default: {
			// Запоминаем код отказа перевода
			this->_error = error_t::UNSUPPORTED;
			// Выводим сообщение о неведомом виде записи
			this->_log->print("Кодек вида %u мостом ещё не переводится", log_t::flag_t::WARNING, static_cast <uint16_t> (format));
			// Выходим из метода, перевод отвечен отказом
			return false;
		}
	}
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
