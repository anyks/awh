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
 * Подключаем заголовочный файл разбора записи числа
 */
#include <num/lexical/lexical.hpp>

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
	/**
	 * @warning Обход ведётся ходом `keys()`, и это верно ПОКУДА разбор держит
	 *          умолчание: YAML 1.2 требует имена пар отображения уникальными, и
	 *          умолчание повтор ОТВЕРГАЕТ, так что до моста он не доходит вовсе.
	 *          Под послаблением `duplicate_t::KEEP` обход РАЗОМКНУТ - имя
	 *          выдаётся дважды, а оба звена ведут к ПЕРВОЙ паре, и вторая
	 *          недостижима ничем; `has()` и `valid()` при этом отвечают истиной
	 *          обе, так что беспечная проверка была бы зелёной. Послабление это
	 *          стоит вне стандарта, и разомкнутость есть цена его, а не порок
	 *          хода. Понадобится KEEP - брать пары перебором по номеру, а не
	 *          обходом по звеньям
	 */
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
 * @warning Содержимое СМЕШАННОЕ - текст узла вперемешку с узлами разметки, как
 *          у записи `<a>текст<b/></a>`, - теряет текст: у отображения он лечь
 *          может лишь полем, а поле это неотличимо стало бы от потомка с тем же
 *          именем. Правило то же, что и у эталонного моста, и потеря здесь
 *          свойством СОЧЛЕНЕНИЯ вызвана, а не изъяном обхода: у настроек, ради
 *          коих мост писан, смешанного содержимого не бывает вовсе
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
	/**
	 * Определяем уровень самого документа
	 *
	 * @warning Уровень этот ДВОИТСЯ: `at("")` отдаёт корневой узел записи, и
	 *          `keys("")` отдаёт звено, ведущее к нему же. Собирая свойства с
	 *          `at("")` и обходя при том звенья, мы уложили бы свойства корня
	 *          ДВАЖДЫ - разом полями верхнего уровня и полями самого корня.
	 *          Замерено 04.09.2026 ворошителем: запись `<config uzvysi="-1"/>`
	 *          давала дерево с полем `uzvysi` рядом с полем `config`, и круг
	 *          перевода полз - всякий проход добавлял уровень вложенности,
	 *          оставаясь при том разбираемым, то есть порок был тихим
	 *
	 * @note Свойства и текст берутся здесь лишь у узлов ВНУТРИ записи; корневой
	 *       узел свои отдаёт на следующем витке обхода, придя туда потомком
	 */
	const bool outer = path.empty();
	// Выполняем извлечение перечня свойств узла разметки
	const vector <xml::attribute_t> & attributes = value.attributes();
	// Выполняем извлечение перечня звеньев пути узла
	const vector <string> & links = document.keys(path);
	// Собираемые звенья пути потомков, по именам их разложенные
	vector <pair <string, vector <string>>> children;
	/**
	 * Выполняем разбор звеньев пути по именам потомков
	 *
	 * @warning Звено пути ИМЕНЕМ потомка не является: у одноимённых узлов
	 *          разметки кодек выдаёт звенья ЧИСЛОВЫЕ - по месту потомка среди
	 *          прочих. Замерено: `<config><item>a</item><item>b</item>` даёт
	 *          звенья «3» и «4», а не «item» дважды. Имя берётся у САМОГО УЗЛА
	 *
	 * @warning Пустота перечня звеньев признаком простого значения НЕ является:
	 *          текстовое содержимое узла тоже ПОТОМОК и тоже звено. Признак
	 *          берётся видом каждого потомка, а не их числом
	 */
	for(auto & link : links){
		// Выполняем извлечение потомка по звену пути
		const xml::node_t & child = document.at(path + "/" + link);
		// Если потомок узлом разметки не является
		if(child.kind() != xml::kind_t::ELEMENT)
			// Пропускаем потомка, вместилищу он не принадлежит
			continue;
		// Выполняем извлечение местного имени потомка
		const string name(child.name().local);
		// Признак нахождения имени среди уже разобранных
		bool found = false;
		// Выполняем перебор всех разобранных имён
		for(auto & child : children){
			// Если имя потомка уже встречалось
			if(child.first == name){
				// Выполняем добавление звена пути к прежнему имени
				child.second.emplace_back(link);
				// Запоминаем нахождение имени
				found = true;
				// Выходим из перебора имён
				break;
			}
		}
		// Если имя потомка встретилось впервые
		if(!found)
			// Выполняем заведение имени с его первым звеном пути
			children.emplace_back(name, vector <string> {link});
	}
	/**
	 * Если узел не несёт ни узлов разметки, ни свойств
	 *
	 * @details Узел этот прост, и ложится он ЗНАЧЕНИЕМ своим, а не отображением:
	 *          обносить его отображением значило бы городить уровень, в записи
	 *          не стоявший
	 */
	if(children.empty() && (outer || attributes.empty())){
		// Извлекаемый текст узла разметки
		const string text = value.text();
		/**
		 * Если узел пуст вовсе
		 *
		 * @warning Узел этот выражает то, чего у прочих форматов нет вовсе -
		 *          НАЛИЧИЕ без значения, - и правило укладки его берётся
		 *          настройкою: согласия в мире нет, а `<NewEnabled/>` у настроек
		 *          означает «включено», тогда как у разметки текста он значит
		 *          пустую запись
		 */
		if(text.empty() && !outer){
			// Определяем правило укладки пустого узла разметки
			switch(static_cast <uint8_t> (this->_settings.empty)){
				// Если пустой узел ложится логическою истиной
				case static_cast <uint8_t> (empty_t::BOOLEAN):
					// Выполняем укладку узла логическою истиной
					result = abc::value_t(true);
				break;
				// Если пустой узел ложится пустым значением
				case static_cast <uint8_t> (empty_t::NONE):
					// Выполняем укладку узла пустым значением
					result = abc::value_t(abc::kind_t::NUL);
				break;
				// Если пустой узел ложится пустою последовательностью знаков
				case static_cast <uint8_t> (empty_t::STRING):
					// Выполняем укладку узла пустою последовательностью знаков
					result = abc::value_t(string(""));
				break;
				// Если пустой узел ложится пустым отображением
				case static_cast <uint8_t> (empty_t::OBJECT):
					// Выполняем укладку узла пустым отображением
					result = abc::value_t(abc::kind_t::MAP);
				break;
			}
			// Выводим результат укладки
			return true;
		}
		// Выполняем укладку узла собранным текстом
		result = this->infer(text);
		// Выводим результат укладки
		return true;
	}
	/**
	 * Определяем пометку перечня у узла разметки
	 *
	 * @warning Пометка эта - единственный способ отличить перечень безымянный от
	 *          отображения: повтор одноимённых узлов выражает перечень лишь при
	 *          двух звеньях и более, а перечень верхнего уровня и перечень,
	 *          вложенный в другой перечень, повтора не дают вовсе. Без пометки
	 *          запись `[[1,2],[3]]` возвращалась отображением с полями обноса, и
	 *          круг перевода не замыкался ни на одном проходе
	 */
	bool marked = false;
	// Если пометка перечня настройками разрешена
	if(!this->_settings.array.empty() && !outer){
		// Выполняем перебор всех свойств узла разметки
		for(auto & attribute : attributes){
			// Если свойство несёт пометку перечня
			if(this->_fmk->compare(this->_settings.array, string(attribute.name.local))){
				// Запоминаем нахождение пометки перечня
				marked = this->_fmk->compare("true", string(attribute.value));
				// Выходим из перебора свойств узла
				break;
			}
		}
	}
	/**
	 * Если узел помечен перечнем
	 *
	 * @note Имена потомков здесь ЗНАЧЕНИЯ не имеют: перечень выражается их
	 *       порядком, а имя всякому из них дано обносом при записи
	 */
	if(marked){
		// Выполняем заведение перечня контейнера ABC
		result = abc::value_t(abc::kind_t::ARRAY);
		// Выполняем перебор всех имён потомков узла
		for(auto & child : children){
			// Выполняем перебор всех звеньев пути потомка
			for(auto & link : child.second){
				// Собираемое значение звена перечня
				abc::value_t item;
				// Выполняем укладку значения звена перечня
				if(!this->absorbXML(document, path + "/" + link, item, depth + 1))
					// Выходим из метода, укладка отвечена отказом
					return false;
				// Выполняем добавление собранного значения звеном перечня
				result.push(item);
			}
		}
		// Выводим результат укладки
		return true;
	}
	// Выполняем заведение отображения контейнера ABC
	result = abc::value_t(abc::kind_t::MAP);
	/**
	 * Выполняем укладку свойств узла разметки
	 *
	 * @warning Свойства ложатся полями отображения под СВОИМИ именами, наравне
	 *          с потомками, а не под приставкою вида `@` либо `-`. Приставка
	 *          выражала бы их отличие ценою искажения имени, тогда как запись
	 *          `<net host="localhost"/>` и запись `<net><host>localhost</host></net>`
	 *          суть два написания одного и того же содержимого настроек
	 */
	if(!outer){
		// Выполняем перебор всех свойств узла разметки
		for(auto & attribute : attributes){
			// Извлекаемое имя свойства узла разметки
			const string name(attribute.name.local);
			// Если свойство несёт пометку перечня, полем оно не ложится
			if(!this->_settings.array.empty() && this->_fmk->compare(this->_settings.array, name))
				// Продолжаем перебор свойств узла дальше
				continue;
			// Выполняем укладку значения свойства узла
			result[name] = this->infer(string(attribute.value));
		}
	}
	/**
	 * Если узел несёт свойства, но узлов разметки не несёт
	 *
	 * @warning Текст такого узла ложится ОТДЕЛЬНЫМ полем, ибо самим значением
	 *          лечь уже не может - место занято отображением свойств. Имя поля
	 *          берётся настройкою и при столкновении с одноимённым свойством
	 *          получает приставку `_` столько раз, сколько нужно
	 */
	if(children.empty()){
		// Извлекаемый текст узла разметки
		const string text = value.text();
		// Если текст узла разметки не пуст
		if(!text.empty()){
			// Собираемое имя поля текста узла
			string name = this->_settings.text;
			// Выполняем поиск свободного имени поля
			while(result.contains(name))
				// Выполняем наращивание имени приставкой
				name.insert(name.begin(), '_');
			// Выполняем укладку текста узла разметки
			result[name] = this->infer(text);
		}
		// Выводим результат укладки
		return true;
	}
	// Выполняем перебор всех разобранных имён потомков
	for(auto & child : children){
		// Если имя потомка встретилось единожды
		if(child.second.size() == 1){
			// Выполняем укладку значения потомка в поле отображения
			if(!this->absorbXML(document, path + "/" + child.second.front(), result[child.first], depth + 1))
				// Выходим из метода, укладка отвечена отказом
				return false;
		// Если имя потомка встретилось несколько раз
		} else {
			// Выполняем заведение вместимого контейнера ABC
			result[child.first] = abc::value_t(abc::kind_t::ARRAY);
			// Выполняем перебор всех звеньев пути одноимённых потомков
			for(size_t i = 0; i < child.second.size(); i++){
				// Выполняем укладку значения потомка в значение вместимого
				if(!this->absorbXML(document, path + "/" + child.second.at(i), result[child.first][i], depth + 1))
					// Выходим из метода, укладка отвечена отказом
					return false;
			}
		}
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
 * @brief Метод подачи значения ABC значению YAML
 *
 * @param value  значение контейнера ABC
 * @param result собираемое значение записи YAML
 * @param depth  глубина обхода дерева
 * @return       результат подачи
 *
 * @warning Виды BLOB, TIME, UUID и CUSTOM записи YAML неведомы, и обращение с
 *          ними решает настройка сужения: STRICT отвечает отказом, TEXT кладёт
 *          их последовательностью знаков, SKIP пропускает вовсе
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
		/**
		 * Если значение является пустым
		 *
		 * @warning Значение, заведённое умолчанием, у записи YAML и есть пустое:
		 *          заводителя пустоты у владеющего значения нет, он принадлежит
		 *          СТРОИТЕЛЮ, а это иной класс
		 */
		case static_cast <uint32_t> (abc::type_t::NUL):
			// Выходим из метода, пустое значение уже заведено
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
		case static_cast <uint32_t> (abc::type_t::STRING): {
			// Выполняем заведение последовательности знаков
			result = yaml::Value(value.text());
		} return true;
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
					// Если вид имени надлежит пропустить вовсе
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
				 *          ход этот берёт именно имя
				 */
				if(!result.insert(name, item))
					// Выходим из метода, добавление отвечено отказом
					return false;
			}
		} return true;
	}
	/**
	 * Если значение является числом
	 *
	 * @warning Целое БЕЗ ЗНАКА спрашивается ПЕРВЫМ, а не после целого со знаком:
	 *          извлечение целого со знаком из значения без знака отвечает УСПЕХОМ
	 *          и заворачивает величину по договору приведения. Замерено 04.09.2026
	 *          ворошителем - предел разрядности `18446744073709551615` ложился в
	 *          запись числом `-1`, и запись оставалась годной
	 */
	if(value.is(abc::type_t::NUMBER)){
		// Если значение является целым без знака
		if(value.is(abc::type_t::UNSIGNED)){
			// Извлекаемое целое без знака
			uint64_t digit = 0;
			// Если значение извлекается целым без знака
			if(value.value(digit)){
				// Выполняем заведение целого без знака
				result = yaml::Value(digit);
				// Выводим результат подачи
				return true;
			}
		}
		// Извлекаемое целое со знаком
		int64_t number = 0;
		// Если значение является целым со знаком
		if(value.is(abc::type_t::SIGNED) && value.value(number)){
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
	/**
	 * Если значение является отметкою времени
	 *
	 * @warning Отметка времени укладывается ЧИСЛОМ, а не правилом сужения: своего
	 *          вида у неё ни у одного текстового кодека нет, зато число есть у
	 *          всех, и величина её - число октетов времени - выражается им без
	 *          потери. Ход этот повторяет ход записи JSON намеренно: разойдись
	 *          дороги, одно и то же дерево дало бы число у одного вида записи и
	 *          строку у другого
	 */
	if(value.is(abc::type_t::TIME)){
		// Извлекаемая отметка времени
		int64_t stamp = 0;
		// Если отметка времени извлекается числом
		if(value.value(stamp)){
			// Выполняем заведение отметки времени числом
			result = yaml::Value(stamp);
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
		/**
		 * Если вид надлежит обратить в последовательность знаков
		 *
		 * @warning Запись берётся ходом `record()`, а НЕ `text()`: последний отдаёт
		 *          содержимое лишь у строковых значений и молчит пустотою у прочих.
		 *          Замерено 04.09.2026 - двоичные данные уходили в запись пустою
		 *          строкою. Ход этот общий у всех дорог, и обходить его нельзя ни
		 *          одной
		 */
		case static_cast <uint8_t> (narrow_t::TEXT): {
			// Выполняем заведение последовательности знаков
			result = yaml::Value(this->record(value));
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
	/**
	 * Выполняем заведение первого документа записи YAML
	 *
	 * @warning Затравка эта обязательна: посадка ведётся от корня ПЕРВОГО
	 *          документа, а у документа, ничего не разобравшего, его нет вовсе -
	 *          прививка отвечает отказом `EMPTY_TEXT` даже на простейшем дереве.
	 *          Замерено: `{a: 17}` прививается лишь по затравке, а без неё
	 *          отвечает кодом 41. Знаком начала документа затравка взята потому,
	 *          что содержимого он не задаёт и толкованию не мешает
	 */
	if(!document.parse(string("---"))){
		// Запоминаем код отказа перевода
		this->_error = error_t::WRITING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
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
	 *          последней, обязаны быть налицо - вместилищ по пути не заводится
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
 * @brief Метод подачи значения ABC значению TOML
 *
 * @param value  значение контейнера ABC
 * @param result собираемое значение записи TOML
 * @param depth  глубина обхода дерева
 * @return       результат подачи
 *
 * @warning Пустого значения у записи TOML нет ВОВСЕ: запись `a = ` отвергается
 *          самим стандартом. Оттого вид NUL укладывается по правилу сужения, а
 *          не своим видом, и при умолчании TEXT ложится пустой строкой. Потеря
 *          вида здесь свойством формата вызвана
 *
 */
bool awh::codec::Bridge::feedTOML(const abc::value_t & value, toml::Value & result, const uint32_t depth) noexcept {
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
		// Если значение является логическим
		case static_cast <uint32_t> (abc::type_t::BOOL): {
			// Извлекаемое логическое значение
			bool number = false;
			// Выполняем извлечение логического значения
			if(!value.value(number))
				// Выходим из метода, извлечение отвечено отказом
				return false;
			// Выполняем заведение логического значения
			result = toml::Value(number);
		} return true;
		// Если значение является последовательностью знаков
		case static_cast <uint32_t> (abc::type_t::STRING): {
			// Выполняем заведение последовательности знаков
			result = toml::Value(value.text());
		} return true;
		// Если значение является вместимым
		case static_cast <uint32_t> (abc::type_t::ARRAY): {
			// Выполняем перебор всех значений вместимого
			for(size_t i = 0; i < value.size(); i++){
				// Собираемое значение перечня
				toml::Value item;
				// Выполняем подачу значения вместимого значению перечня
				if(!this->feedTOML(value[i], item, depth + 1))
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
			// Выполняем перебор всех полей отображения
			for(size_t i = 0; i < value.size(); i++){
				// Извлекаемое имя поля отображения
				string name = "";
				// Выполняем извлечение имени поля отображения
				if(!value.key(i).value(name)){
					// Если вид имени надлежит пропустить вовсе
					if(this->_settings.narrow == narrow_t::SKIP)
						// Продолжаем перебор полей отображения дальше
						continue;
					// Запоминаем код отказа перевода
					this->_error = error_t::UNSUPPORTED;
					// Выходим из метода, перевод отвечен отказом
					return false;
				}
				// Собираемое значение поля отображения
				toml::Value item;
				// Выполняем подачу значения поля значению отображения
				if(!this->feedTOML(value[name], item, depth + 1))
					// Выходим из метода, подача отвечена отказом
					return false;
				/**
				 * Выполняем добавление поля отображению
				 *
				 * @warning Имя кладётся ИСХОДНЫМ: отменяющая запись есть свойство
				 *          ПУТИ, а не имени. Ограду же кавычками ставит сам кодек -
				 *          голое имя ключа по стандарту TOML лишь ASCII, и
				 *          кириллица кавычек требует
				 */
				if(!result.insert(name, item))
					// Выходим из метода, добавление отвечено отказом
					return false;
			}
		} return true;
	}
	/**
	 * Если значение является числом
	 *
	 * @warning Целое БЕЗ ЗНАКА спрашивается ПЕРВЫМ, а не после целого со знаком:
	 *          извлечение целого со знаком из значения без знака отвечает УСПЕХОМ
	 *          и заворачивает величину по договору приведения. Замерено 04.09.2026
	 *          ворошителем - предел разрядности `18446744073709551615` ложился в
	 *          запись числом `-1`, и запись оставалась годной
	 */
	if(value.is(abc::type_t::NUMBER)){
		// Если значение является целым без знака
		if(value.is(abc::type_t::UNSIGNED)){
			// Извлекаемое целое без знака
			uint64_t digit = 0;
			/**
			 * Если значение извлекается целым без знака и в запись вмещается
			 *
			 * @warning Описание TOML знает целое ЛИШЬ СО ЗНАКОМ шириною в восемь
			 *          октетов: величина свыше `9223372036854775807` записи этой
			 *          неведома вовсе, и уходит она общим правилом сужения, а не
			 *          собирателем записи. Замерено 04.09.2026 - предел разрядности
			 *          выходил записью `top = -1`, годной по виду и неверной по
			 *          величине
			 */
			if(value.value(digit) && (digit <= static_cast <uint64_t> (9223372036854775807ULL))){
				// Выполняем заведение целого без знака
				result = toml::Value(digit);
				// Выводим результат подачи
				return true;
			}
			// Если сужение велит отвечать отказом
			if(this->_settings.narrow == narrow_t::STRICT){
				// Запоминаем код отказа перевода
				this->_error = error_t::UNSUPPORTED;
				// Выходим из метода, перевод отвечен отказом
				return false;
			}
			// Если сужение велит пропускать значение вовсе
			if(this->_settings.narrow == narrow_t::SKIP)
				// Выходим из метода, значение пропущено
				return true;
			// Выполняем заведение записи числа последовательностью знаков
			result = toml::Value(this->record(value));
			// Выводим результат подачи
			return true;
		}
		// Извлекаемое целое со знаком
		int64_t number = 0;
		// Если значение является целым со знаком
		if(value.is(abc::type_t::SIGNED) && value.value(number)){
			// Выполняем заведение целого со знаком
			result = toml::Value(number);
			// Выводим результат подачи
			return true;
		}
		// Извлекаемое число дробное
		double real = 0.;
		// Если значение извлекается числом дробным
		if(value.value(real)){
			// Выполняем заведение числа дробного
			result = toml::Value(real);
			// Выводим результат подачи
			return true;
		}
	}
	/**
	 * Если значение является отметкою времени
	 *
	 * @warning Отметка времени укладывается ЧИСЛОМ, а не правилом сужения: своего
	 *          вида у неё ни у одного текстового кодека нет, зато число есть у
	 *          всех, и величина её - число октетов времени - выражается им без
	 *          потери. Ход этот повторяет ход записи JSON намеренно: разойдись
	 *          дороги, одно и то же дерево дало бы число у одного вида записи и
	 *          строку у другого
	 */
	if(value.is(abc::type_t::TIME)){
		// Извлекаемая отметка времени
		int64_t stamp = 0;
		// Если отметка времени извлекается числом
		if(value.value(stamp)){
			// Выполняем заведение отметки времени числом
			result = toml::Value(stamp);
			// Выводим результат подачи
			return true;
		}
	}
	// Определяем правило обращения с видом, записи TOML неведомым
	switch(static_cast <uint8_t> (this->_settings.narrow)){
		// Если вид надлежит пропустить вовсе
		case static_cast <uint8_t> (narrow_t::SKIP):
			// Выходим из метода, значение пропущено
			return true;
		/**
		 * Если вид надлежит обратить в последовательность знаков
		 *
		 * @warning Запись берётся ходом `record()`, а НЕ `text()`: последний отдаёт
		 *          содержимое лишь у строковых значений и молчит пустотою у прочих.
		 *          Замерено 04.09.2026 - двоичные данные уходили в запись пустою
		 *          строкою. Ход этот общий у всех дорог, и обходить его нельзя ни
		 *          одной
		 */
		case static_cast <uint8_t> (narrow_t::TEXT): {
			// Выполняем заведение последовательности знаков
			result = toml::Value(this->record(value));
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
 * @brief Метод перевода дерева ABC в запись TOML
 *
 * @param value  дерево значений контейнера ABC
 * @param result собранная запись TOML
 * @return       результат перевода
 *
 */
bool awh::codec::Bridge::encodeTOML(const abc::value_t & value, string & result) noexcept {
	// Создаём документ записи TOML
	toml::document_t document(this->_log);
	// Собираемое владеющее значение записи TOML
	toml::Value root;
	// Выполняем подачу дерева значений владеющему значению
	if(!this->feedTOML(value, root, 0)){
		// Если код отказа перевода ещё не запомнен
		if(this->_error == error_t::NONE)
			// Запоминаем код отказа перевода
			this->_error = error_t::WRITING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	/**
	 * Выполняем посадку собранного значения в документ записи TOML
	 *
	 * @warning Путь посадки задаётся ПЕРЕЧНЕМ КУСКОВ, а не строкою, как у
	 *          записи YAML: ходы прививки у двух кодеков покуда разной формы, и
	 *          пустой перечень означает здесь корень документа
	 */
	if(!root.graft(document, vector <string_view> ())){
		// Запоминаем код отказа перевода
		this->_error = error_t::WRITING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем изъятие собранной записи TOML
	result = document.dump();
	// Сообщаем, что перевод выполнен
	return true;
}

/**
 * @brief Метод выдачи записи простого значения ABC
 *
 * @param value значение контейнера ABC
 * @return      запись простого значения
 *
 * @warning Ход `text()` у контейнера ABC отдаёт запись ЛИШЬ у последовательности
 *          знаков, а у числа, логического значения и пустоты ПУСТ - это не
 *          изъян, а неверно заданный ему вопрос. Виды записи, своей системы
 *          видов не имеющие (INI, XML), пишут всё текстом, и спрос `text()` у
 *          числа обращал бы `port = 8080` в `port = ` МОЛЧА. Замерено на выдаче
 *          настроек, собранных из доводов запуска, где число приходит числом,
 *          а не строкой, - круговая проверка через сам кодек этого не ловила,
 *          ибо там всякое значение приходило из разбора уже строкою
 *
 */
string awh::codec::Bridge::record(const abc::value_t & value) const noexcept {
	// Если значение является последовательностью знаков
	if(value.is(abc::type_t::STRING))
		// Выводим запись последовательности знаков
		return value.text();
	// Если значение является логическим
	if(value.is(abc::type_t::BOOL)){
		// Извлекаемое логическое значение
		bool number = false;
		// Если логическое значение извлечено
		if(value.value(number))
			// Выводим запись логического значения
			return (number ? "true" : "false");
	}
	/**
	 * Если значение является числом дробным
	 *
	 * @warning Дробное спрашивается ПРЕЖДЕ целого, а не после: извлечение целого
	 *          из дробного значения отвечает УСПЕХОМ и округляет по договору
	 *          приведения. Замерено 04.09.2026 на образцах владельца - `0.25`
	 *          ложилось в запись нулём, а `1.5` двойкой, и запись оставалась
	 *          годной, теряя лишь дробную часть
	 */
	if(value.is(abc::type_t::REAL) || value.is(abc::type_t::DECIMAL)){
		// Извлекаемое число дробное
		double real = 0.;
		// Если значение извлекается числом дробным
		if(value.value(real)){
			// Собираемая запись числа дробного
			string result = std::to_string(real);
			/**
			 * Выполняем снятие незначащих нулей хвоста записи
			 *
			 * @note Запись `std::to_string` всегда несёт шесть знаков после точки, и
			 *       число `0.25` выходило бы записью `0.250000`
			 */
			if(result.find('.') != string::npos){
				// Выполняем снятие незначащих нулей
				while(!result.empty() && (result.back() == '0'))
					// Выполняем снятие одного незначащего нуля
					result.pop_back();
				/**
				 * Если запись оканчивается точкой
				 *
				 * @warning Точка со следующим за нею нулём остаётся НАМЕРЕННО: своей
				 *          системы видов у разметки нет, и вид числа несёт лишь сама
				 *          запись. Сними мы точку - запись `3000.0` вышла бы записью
				 *          `3000`, обратное чтение вернуло бы целое, и круг перевода
				 *          разомкнулся бы на всяком дробном числе с нулевой дробной
				 *          частью
				 */
				if(!result.empty() && (result.back() == '.'))
					// Выполняем добавление значащего нуля дробной части
					result.append(1, '0');
			}
			// Выводим собранную запись числа дробного
			return result;
		}
	}
	/**
	 * Если значение является целым без знака
	 *
	 * @warning Целое без знака спрашивается ПРЕЖДЕ целого со знаком: извлечение
	 *          со знаком отвечает УСПЕХОМ и заворачивает величину по договору
	 *          приведения, отчего предел разрядности выходил записью `-1`
	 */
	if(value.is(abc::type_t::UNSIGNED)){
		// Извлекаемое целое без знака
		uint64_t digit = 0;
		// Если значение извлекается целым без знака
		if(value.value(digit))
			// Выводим запись целого без знака
			return std::to_string(digit);
	}
	/**
	 * Извлекаемое целое со знаком
	 *
	 * @note Ходом этим берётся и отметка времени: своего вида у неё ни у одного
	 *       текстового кодека нет, и записью её служит число октетов времени
	 */
	int64_t number = 0;
	// Если значение извлекается целым со знаком
	if(value.value(number))
		// Выводим запись целого со знаком
		return std::to_string(number);
	// Извлекаемое целое без знака
	uint64_t digit = 0;
	// Если значение извлекается целым без знака
	if(value.value(digit))
		// Выводим запись целого без знака
		return std::to_string(digit);
	// Извлекаемое число дробное
	double real = 0.;
	// Если значение извлекается числом дробным
	if(value.value(real))
		// Выводим запись числа дробного
		return std::to_string(real);
	// Выводим запись значения, как её отдаёт контейнер
	return value.text();
}

/**
 * @brief Метод вывода значения ABC из его записи
 *
 * @param text запись значения
 * @return     выведенное значение контейнера ABC
 *
 * @warning Порядок звеньев цепи значим: целое спрашивается ПРЕЖДЕ дробного,
 *          ибо запись `8080` отвечает обоим, а видом ей надлежит быть целым.
 *          Логическое спрашивается ПОСЛЕДНИМ из видов, ибо записи `true` и
 *          `false` числами не бывают вовсе и порядка не меняют
 *
 * @warning Запись с ведущим нулём числом НЕ признаётся: `007` есть опознаватель
 *          либо код, а не число семь, и обращение его в число теряет ведущие
 *          нули безвозвратно. Решение это принято в модуле разбора доводов
 *          запуска и здесь повторено намеренно, дабы два хода одного набора не
 *          расходились в толковании одной и той же записи
 *
 * @warning Извлечение идёт `num/lexical` НАПРЯМУЮ, а не через `codec::numeric`,
 *          и расхождение это намеренно. Договор `codec::numeric` общий у
 *          кодеков рамки: запись, разрядность превысившая, числом быть не
 *          перестаёт и уходит дробным разбором с приведением к затребованному
 *          виду. Кодеку то годится - вид записи ему задан грамматикой, - а
 *          выводу вида НЕТ: замер 04.09.2026 показал, что запись
 *          `18446744073709551616` ложилась целым без знака со значением
 *          `18446744073709551615`, теряя единицу молча. Здесь же вопрос иной -
 *          «число ли это вовсе», - и запись, в родной вид не вместимая,
 *          остаётся последовательностью знаков целиком
 *
 */
awh::codec::abc::value_t awh::codec::Bridge::infer(const string & text) const noexcept {
	// Если вывод вида отключён настройками либо запись пуста
	if(!this->_settings.typed || text.empty())
		// Выводим значение последовательностью знаков
		return abc::value_t(text);
	/**
	 * Определяем запись с ведущим нулём
	 *
	 * @note Ведущий нуль допускается лишь у записи `0` самой и у дробной записи
	 *       вида `0.5`, где он значим
	 */
	const bool zeroed = (
		((text.front() == '0') && (text.size() > 1) && (text.at(1) != '.')) ||
		((text.front() == '-') && (text.size() > 2) && (text.at(1) == '0') && (text.at(2) != '.'))
	);
	// Если запись ведущего нуля не несёт
	if(!zeroed){
		// Запись числа, от обвязки пробелами очищенная
		string record = text;
		// Выполняем очистку записи от обвязки пробелами
		this->_fmk->transform(record, fmk_t::transform_t::TRIM);
		// Если запись несёт ведущий плюс, разбору не поддающийся
		if(!record.empty() && (record.front() == '+'))
			// Выполняем отбрасывание ведущего плюса
			record.erase(0, 1);
		// Если запись пуста
		if(record.empty())
			// Выводим значение последовательностью знаков
			return abc::value_t(text);
		// Получаем конец записи числа
		const char * end = record.data() + record.size();
		// Если запись является целым числом
		if(this->_fmk->is(record, fmk_t::check_t::NUMBER)){
			// Если число записано со знаком
			if(record.front() == '-'){
				// Извлекаемое целое со знаком
				int64_t number = 0;
				// Выполняем извлечение целого со знаком из записи
				const lexical_t::result_t <char> res = lexical_t::fromChars(record.data(), end, number);
				// Если запись разобрана целиком и без отказа
				if(static_cast <bool> (res) && (res.ptr == end))
					// Выводим значение целым со знаком
					return abc::value_t(number);
			// Если число записано без знака
			} else {
				// Извлекаемое целое без знака
				uint64_t number = 0;
				// Выполняем извлечение целого без знака из записи
				const lexical_t::result_t <char> res = lexical_t::fromChars(record.data(), end, number);
				// Если запись разобрана целиком и без отказа
				if(static_cast <bool> (res) && (res.ptr == end))
					// Выводим значение целым без знака
					return abc::value_t(number);
			}
		}
		// Если запись является числом дробным
		if(this->_fmk->is(record, fmk_t::check_t::DECIMAL)){
			// Извлекаемое число дробное
			double number = 0.;
			// Выполняем извлечение числа дробного из записи
			const lexical_t::result_t <char> res = lexical_t::fromChars(record.data(), end, number);
			// Если запись разобрана целиком и без отказа
			if(static_cast <bool> (res) && (res.ptr == end))
				// Выводим значение числом дробным
				return abc::value_t(number);
		}
	}
	// Если запись означает истину
	if(this->_fmk->compare("true", text))
		// Выводим значение логическою истиной
		return abc::value_t(true);
	// Если запись означает ложь
	if(this->_fmk->compare("false", text))
		// Выводим значение логическою ложью
		return abc::value_t(false);
	// Выводим значение последовательностью знаков
	return abc::value_t(text);
}

/**
 * @brief Метод подачи значения ABC значению XML
 *
 * @param value  значение контейнера ABC
 * @param result собираемое значение записи XML
 * @param depth  глубина обхода дерева
 * @return       результат подачи
 *
 * @warning Своей системы видов у разметки нет: логическое значение, число и
 *          пустота ложатся ЗАПИСЬЮ, а не видом, и обратное чтение вернёт их
 *          последовательностью знаков. Потеря вида здесь свойством формата
 *          вызвана, а не изъяном моста
 *
 * @warning Вместимое ложится ОДНОИМЁННЫМИ узлами по имени поля, его несущего:
 *          перечня как такового у разметки нет, и повтор узла есть
 *          единственная его запись. Оттого перечень из одного значения по
 *          обратному чтению перечнем уже не окажется
 *
 */
bool awh::codec::Bridge::feedXML(const abc::value_t & value, xml::Value & result, const uint32_t depth) noexcept {
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
	// Если значение является отображением
	if(value.is(abc::type_t::MAP)){
		/**
		 * Определяем, несёт ли отображение вложенные вместилища
		 *
		 * @warning Признак этот решает, чем станут поля отображения: полагая
		 *          свойствами ВСЕ поля, мы потеряли бы вложенность, ибо свойство
		 *          узла разметки значением своим несёт лишь текст; полагая же
		 *          потомками все, мы обратили бы в узлы и те поля, что пришли из
		 *          свойств, и круг перестал бы быть неподвижным. Оттого правило
		 *          таково: отображение из одних лишь простых значений ложится
		 *          СВОЙСТВАМИ, а отображение, вместилище несущее, - ПОТОМКАМИ
		 */
		bool nested = false;
		// Выполняем перебор всех полей отображения
		for(size_t i = 0; i < value.size(); i++){
			// Извлекаемое имя поля отображения
			string name = "";
			// Если имя поля отображения извлечь не удалось
			if(!value.key(i).value(name))
				// Продолжаем перебор полей отображения дальше
				continue;
			// Если значением поля оказалось вместилище
			if(value[name].is(abc::type_t::MAP) || value[name].is(abc::type_t::ARRAY)){
				// Запоминаем наличие вложенного вместилища
				nested = true;
				// Выходим из перебора полей отображения
				break;
			}
		}
		/**
		 * Отбираем поле, текст узла несущее
		 *
		 * @details Имя его есть имя настройки с приставками `_`, наложенными при
		 * разборе ради обхода столкновения с одноимённым свойством. Отбирается поле
		 * с НАИБОЛЬШИМ числом приставок: столкновение наращивало приставку всякий раз
		 * заново, и последним, то есть самым длинным, вышло имя текста
		 *
		 * @warning Отбор этот - не украшение. Беря первое же поле, чьё имя после
		 *          снятия приставок совпадает с настройкою, мы у записи с ОБОИМИ
		 *          полями - `value` свойством и `_value` текстом - уложили бы текстом
		 *          свойство, а текст потеряли вовсе. Замерено 04.09.2026 на образцах
		 *          владельца
		 */
		string textual = "";
		// Число приставок у отобранного имени поля
		size_t marks = 0;
		// Выполняем перебор всех полей отображения
		for(size_t i = 0; i < value.size(); i++){
			// Извлекаемое имя поля отображения
			string name = "";
			// Если имя поля отображения извлечь не удалось
			if(!value.key(i).value(name))
				// Продолжаем перебор полей отображения дальше
				continue;
			// Число приставок у имени поля отображения
			size_t count = 0;
			// Выполняем счёт приставок имени поля
			while((count < name.length()) && (name.at(count) == '_'))
				// Увеличиваем счёт приставок имени поля
				count++;
			// Если имя поля без приставок именем текста не является
			if(name.substr(count) != this->_settings.text)
				// Продолжаем перебор полей отображения дальше
				continue;
			// Если приставок у имени поля больше, чем у отобранного прежде
			if(textual.empty() || (count > marks)){
				// Запоминаем имя поля, текст узла несущего
				textual = name;
				// Запоминаем число приставок отобранного имени
				marks = count;
			}
		}
		// Выполняем перебор всех полей отображения
		for(size_t i = 0; i < value.size(); i++){
			// Извлекаемое имя поля отображения
			string name = "";
			// Выполняем извлечение имени поля отображения
			if(!value.key(i).value(name)){
				// Если вид имени надлежит пропустить вовсе
				if(this->_settings.narrow == narrow_t::SKIP)
					// Продолжаем перебор полей отображения дальше
					continue;
				// Запоминаем код отказа перевода
				this->_error = error_t::UNSUPPORTED;
				// Выходим из метода, перевод отвечен отказом
				return false;
			}
			// Извлекаемое значение поля отображения
			const abc::value_t & item = value[name];
			/**
			 * Выполняем правку имени, разметке негодного
			 *
			 * @warning Имя узла и свойства разметки с цифры начинаться НЕ МОЖЕТ по
			 *          стандарту XML 1.0, а у отображения имя такое законно: поле
			 *          `3` приходит и из записи JSON, и из перечня, в отображение
			 *          обращённого. Без правки этой запись отвечала бы отказом
			 *          «invalid name», и дерево не выражалось бы вовсе - замерено
			 *          04.09.2026 на дереве с полем `3`. Приставка взята по образцу
			 *          эталонного моста
			 *
			 * @note Круга приставка эта не замыкает: обратное чтение отдаст `Item3`,
			 *       а не `3`. Замкнуть его нечем - разметка имени такого не несёт
			 */
			if(!name.empty() && (name.front() >= '0') && (name.front() <= '9'))
				// Выполняем наложение приставки на имя поля
				name.insert(0, "Item");
			// Если отображение вложенных вместилищ не несёт
			if(!nested){
				/**
				 * Определяем поле, текст узла несущее
				 *
				 * @note Имя его есть имя настройки с приставками `_`, наложенными
				 *       при разборе ради обхода столкновения с одноимённым свойством.
				 *       Приставки снимаются здесь же, ибо в записи их не было
				 */
				// Если поле несёт текст узла разметки
				if(name == textual){
					// Выполняем запись текста узла разметки
					if(!result.text(this->record(item)))
						// Выходим из метода, запись отвечена отказом
						return false;
				// Если поле несёт свойство узла разметки
				} else if(!result.attribute(name, this->record(item)))
					// Выходим из метода, запись отвечена отказом
					return false;
				// Продолжаем перебор полей отображения дальше
				continue;
			}
			// Если значением поля оказалось вместимое
			if(item.is(abc::type_t::ARRAY)){
				/**
				 * Определяем, выразим ли перечень ПОВТОРОМ одноимённых узлов
				 *
				 * @warning Повтор выражает перечень лишь при двух звеньях и более:
				 *          узел, стоящий однажды, обратным чтением перечнем не
				 *          признаётся вовсе, и перечень из одного звена возвращался
				 *          бы значением. Перечень же, звеном своим несущий другой
				 *          перечень, повтором не выразим и подавно - звенья двух
				 *          перечней легли бы вперемешку. Оба случая уходят пометкою
				 */
				bool plain = (item.size() > 1);
				// Выполняем перебор всех звеньев перечня
				for(size_t j = 0; plain && (j < item.size()); j++){
					// Если звеном перечня оказался перечень вложенный
					if(item[j].is(abc::type_t::ARRAY))
						// Запоминаем непригодность повтора
						plain = false;
				}
				// Если перечень повтором не выразим, а пометка разрешена
				if(!plain && !this->_settings.array.empty()){
					// Собираемый узел перечня
					xml::Value node(name);
					// Выполняем подачу перечня узлу разметки
					if(!this->feedXML(item, node, depth + 1))
						// Выходим из метода, подача отвечена отказом
						return false;
					// Выполняем добавление узла перечня потомком
					if(!result.push(node))
						// Выходим из метода, добавление отвечено отказом
						return false;
					// Продолжаем перебор полей отображения дальше
					continue;
				}
				// Выполняем перебор всех значений вместимого
				for(size_t j = 0; j < item.size(); j++){
					// Собираемый узел разметки
					xml::Value node(name);
					// Выполняем подачу значения вместимого узлу разметки
					if(!this->feedXML(item[j], node, depth + 1))
						// Выходим из метода, подача отвечена отказом
						return false;
					/**
					 * Выполняем добавление узла разметки потомком
					 *
					 * @warning Добавление ведётся ходом `push()`, а НЕ `insert()`:
					 *          вставка по имени при повторе его ЗАМЕЩАЕТ прежний
					 *          узел, и от перечня уцелевал бы один последний.
					 *          Замерено: перечень из двух узлов `item` давал в
					 *          записи один лишь «второй»
					 */
					if(!result.push(node))
						// Выходим из метода, добавление отвечено отказом
						return false;
				}
			// Если значением поля оказалось значение обычное
			} else {
				// Собираемый узел разметки
				xml::Value node(name);
				// Выполняем подачу значения поля узлу разметки
				if(!this->feedXML(item, node, depth + 1))
					// Выходим из метода, подача отвечена отказом
					return false;
				// Выполняем добавление узла разметки потомком
				if(!result.push(node))
					// Выходим из метода, добавление отвечено отказом
					return false;
			}
		}
		// Выводим результат подачи
		return true;
	}
	// Если значение является пустым
	if(value.is(abc::type_t::NUL))
		// Выходим из метода, узел остаётся пустым
		return true;
	// Если значение вместимым верхнего уровня не оказалось
	if(!value.is(abc::type_t::ARRAY))
		// Выводим результат записи значения содержимым узла
		return result.text(this->record(value));
	/**
	 * Если пометка перечня настройками разрешена
	 *
	 * @details Перечень, узлу значением ставший, повтором выражен быть не может -
	 * повторять нечего, узел здесь один, - и выражается он пометкою при обносе
	 * каждого звена. Ход этот общий: и перечень верхнего уровня, и перечень внутри
	 * перечня, и перечень из одного звена приходят сюда
	 *
	 * @warning Обнос звеньев обязателен и при звене простом: пометка велит
	 *          обратному чтению собирать перечень из ПОТОМКОВ узла, а текст узла
	 *          потомком разметки не является - перечень вышел бы пустым
	 */
	if(!this->_settings.array.empty()){
		// Выполняем пометку узла перечнем
		if(!result.attribute(this->_settings.array, "true"))
			// Выходим из метода, запись отвечена отказом
			return false;
		// Выполняем перебор всех звеньев перечня
		for(size_t i = 0; i < value.size(); i++){
			// Собираемый узел звена перечня
			xml::Value node(this->_settings.item);
			// Выполняем подачу значения звена узлу разметки
			if(!this->feedXML(value[i], node, depth + 1))
				// Выходим из метода, подача отвечена отказом
				return false;
			// Выполняем добавление узла звена потомком
			if(!result.push(node))
				// Выходим из метода, добавление отвечено отказом
				return false;
		}
		// Выводим результат подачи
		return true;
	}
	/**
	 * Определяем правило обращения с вместимым верхнего уровня
	 *
	 * @warning Имени у такого вместилища нет, и одноимённых узлов из него не
	 *          собрать: разметка перечня без имени выразить не может
	 */
	switch(static_cast <uint8_t> (this->_settings.narrow)){
		// Если вид надлежит пропустить вовсе
		case static_cast <uint8_t> (narrow_t::SKIP):
			// Выходим из метода, значение пропущено
			return true;
		// Если вид надлежит обратить в последовательность знаков
		case static_cast <uint8_t> (narrow_t::TEXT):
			// Выводим результат записи значения содержимым узла
			return result.text(this->record(value));
	}
	// Запоминаем код отказа перевода
	this->_error = error_t::UNSUPPORTED;
	// Выходим из метода, перевод отвечен отказом
	return false;
}

/**
 * @brief Метод перевода дерева ABC в запись XML
 *
 * @param value  дерево значений контейнера ABC
 * @param result собранная запись XML
 * @return       результат перевода
 *
 * @warning Дерево обносится корневым узлом, имя коему берётся настройкою:
 *          стандарт XML 1.0 требует у документа РОВНО ОДИН корневой элемент, и
 *          корнем становится лишь узел РАЗМЕТКИ - текст, примечание либо
 *          указание обработчику корнем не бывают и отвечают отказом
 *          `MISSING_ROOT`. У дерева контейнера ABC корень безымянен, оттого имя
 *          и берётся снаружи
 *
 */
bool awh::codec::Bridge::encodeXML(const abc::value_t & value, string & result) noexcept {
	// Создаём документ записи XML
	xml::document_t document(this->_log);
	// Имя корневого узла собираемой записи
	string name = this->_settings.root;
	// Значение, содержимым корневого узла становящееся
	const abc::value_t * content = &value;
	/**
	 * Выполняем поиск готового корня среди полей дерева
	 *
	 * @warning Проверка эта не украшение, а условие ЗАМКНУТОСТИ круга. Разбор
	 *          записи XML отдаёт дерево с ОДНИМ полем - корневым узлом записи, -
	 *          и запись, обносящая его ещё одним корнем, вкладывала бы содержимое
	 *          на уровень глубже с КАЖДЫМ проходом: `<config>` обращался бы в
	 *          `<config><config>`, затем в три и так без конца. Круговой перевод
	 *          перестал бы быть неподвижным, оставаясь при этом разбираемым, -
	 *          то есть порок был бы тихим
	 */
	if(value.is(abc::type_t::MAP) && (value.size() == 1)){
		// Извлекаемое имя единственного поля отображения
		string local = "";
		// Если имя поля выражается последовательностью знаков
		if(value.key(0).value(local)){
			// Запоминаем имя поля именем корневого узла
			name = local;
			// Запоминаем значение поля содержимым корневого узла
			content = &value[local];
		}
	}
	// Собираемый корневой узел записи XML
	xml::Value root(name);
	// Выполняем подачу дерева значений корневому узлу
	if(!this->feedXML(* content, root, 0)){
		// Если код отказа перевода ещё не запомнен
		if(this->_error == error_t::NONE)
			// Запоминаем код отказа перевода
			this->_error = error_t::WRITING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем посадку собранного узла корнем документа
	if(!document.set("", root)){
		// Запоминаем код отказа перевода
		this->_error = error_t::WRITING;
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	// Выполняем изъятие собранной записи XML
	result = document.dump();
	// Сообщаем, что перевод выполнен
	return true;
}

/**
 * @brief Метод укладки значения INI в значение ABC
 *
 * @param document документ записи INI
 * @param path     путь к укладываемому значению
 * @param result   укладываемое значение контейнера ABC
 * @param depth    глубина обхода дерева
 * @return         результат укладки
 *
 * @note Обход путей ведётся ходом `children()`, а НЕ `keys()`: у этого кодека
 *       имя `keys` несёт ДВА хода разом - прежний, разделом именованный, и общий,
 *       путём именованный, - и выбор между ними идёт по ТОЧНОМУ виду довода.
 *       Литерал уходил бы там в ход прежний и отвечал бы пустым перечнем МОЛЧА.
 *       Имя `children()` однозначно всегда и вида довода не разбирает - оттого
 *       здесь стоит именно оно, а не правило «подавать только `string`»
 *
 * @warning Обход идёт ходом `at()`, а не `get()`: при повторе имени `get()`
 *          отдаёт ДЕЙСТВУЮЩЕЕ объявление по политике повторов, тогда как `at()`
 *          отдаёт ВСЕ объявления перечнем. Наречия Git и systemd задают
 *          перечень значений именно повтором свойства
 *
 */
bool awh::codec::Bridge::absorbINI(const ini::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept {
	// Если глубина обхода превысила предел перевода
	if(depth > this->_settings.depth){
		// Запоминаем код отказа перевода
		this->_error = error_t::DEEP_TREE;
		// Выходим из метода, обход остановлен
		return false;
	}
	// Выполняем очистку укладываемого значения
	result.clear();
	// Выполняем извлечение перечня звеньев пути значения
	const vector <string> & links = document.children(path);
	// Если звеньев пути не нашлось
	if(links.empty()){
		/**
		 * Выполняем укладку значения выводом вида из записи
		 *
		 * @warning Вид выводится ЗДЕСЬ так же, как у разметки: своей системы видов
		 *          у INI нет вовсе, и число, и логическое значение приходят записью
		 *          знаков. Клади мост их строкою - одно и то же дерево давало бы
		 *          число, пройдя через разметку, и строку, пройдя через INI.
		 *          Замерено 04.09.2026: отметка времени возвращалась из INI строкою
		 */
		result = this->infer(string(document.at(path).text()));
		// Выводим результат укладки
		return true;
	}
	/**
	 * Выполняем определение вида вместилища по первому звену пути
	 *
	 * @note Звено числовое означает перечень значений - повтор одноимённого
	 *       свойства, - а звено именное означает раздел либо корень
	 */
	const bool array = (!links.front().empty() && (links.front().find_first_not_of("0123456789") == string::npos));
	// Выполняем заведение вместилища контейнера ABC
	result = abc::value_t(array ? abc::kind_t::ARRAY : abc::kind_t::MAP);
	// Выполняем перебор всех звеньев пути значения
	for(size_t i = 0; i < links.size(); i++){
		// Собираемый путь к потомку
		const string route = path + "/" + links.at(i);
		// Выполняем укладку значения потомка во вместилище
		if(!this->absorbINI(document, route, (array ? result[i] : result[this->unescape(links.at(i))]), depth + 1))
			// Выходим из метода, укладка отвечена отказом
			return false;
	}
	// Выводим результат укладки
	return true;
}

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
	// Выполняем укладку разобранного дерева в значение контейнера ABC
	return this->absorbINI(document, string(""), result, 0);
}

/**
 * @brief Метод записи значения свойством раздела INI
 *
 * @param value    значение контейнера ABC
 * @param document собираемый документ записи INI
 * @param name     имя свойства
 * @param section  имя раздела
 * @return         результат записи
 *
 * @warning Перечень значений пишется ходом `push()`, а НЕ повторным `set()`:
 *          установка правит объявление, УЖЕ В РАЗДЕЛЕ СТОЯЩЕЕ, и перечня тем
 *          не строит - от него уцелело бы одно последнее значение. Долив же
 *          всякий раз заводит НОВОЕ объявление того же имени, а перечень
 *          значений наречия INI и есть череда одноимённых свойств
 *
 */
bool awh::codec::Bridge::feedINI(const abc::value_t & value, ini::document_t & document, const string & name, const string & section) noexcept {
	// Если значение контейнера ABC недействительно вовсе
	if(!value.valid())
		// Выходим из метода, записывать нечего
		return true;
	// Если значение оказалось перечнем значений
	if(value.is(abc::type_t::ARRAY)){
		// Выполняем перебор всех значений перечня
		for(size_t i = 0; i < value.size(); i++){
			// Извлекаемое значение перечня
			const abc::value_t & item = value[i];
			// Если значением перечня оказалось вместилище
			if(item.is(abc::type_t::ARRAY) || item.is(abc::type_t::MAP)){
				// Если вид надлежит пропустить вовсе
				if(this->_settings.narrow == narrow_t::SKIP)
					// Продолжаем перебор значений перечня дальше
					continue;
				// Запоминаем код отказа перевода
				this->_error = error_t::UNSUPPORTED;
				// Выходим из метода, перевод отвечен отказом
				return false;
			}
			// Выполняем долив значения к перечню свойства
			if(!document.push(name, this->record(item), section))
				// Выходим из метода, долив отвечен отказом
				return false;
		}
		// Выводим результат записи
		return true;
	}
	// Выводим результат установки значения свойства
	return document.set(name, this->record(value), section);
}

/**
 * @brief Метод перевода дерева ABC в запись INI
 *
 * @param value  дерево значений контейнера ABC
 * @param result собранная запись INI
 * @return       результат перевода
 *
 * @warning Глубина записи INI ограничена устройством наречия: раздел, свойство,
 *          и не более. Отображение, глубже второго уровня лежащее, наречием не
 *          выражается вовсе и укладывается по правилу сужения - это НЕ предел
 *          моста, а свойство формата
 *
 */
bool awh::codec::Bridge::encodeINI(const abc::value_t & value, string & result) noexcept {
	// Создаём документ записи INI
	ini::document_t document(this->_log);
	// Если дерево значений отображением не является
	if(!value.is(abc::type_t::MAP)){
		// Запоминаем код отказа перевода
		this->_error = error_t::UNSUPPORTED;
		// Выводим сообщение о негодном строении дерева
		this->_log->print("Запись INI требует отображения корнем дерева", log_t::flag_t::WARNING);
		// Выходим из метода, перевод отвечен отказом
		return false;
	}
	/**
	 * Выполняем запись свойств верхнего уровня
	 *
	 * @warning Свойства верхнего уровня пишутся ПЕРВЫМИ, и порядок этот значим:
	 *          у записи INI всё, что стоит после объявления раздела, разделу
	 *          тому и принадлежит. Запиши мост их последними - они оказались бы
	 *          свойствами последнего раздела, а не верхнего уровня
	 */
	for(size_t i = 0; i < value.size(); i++){
		// Извлекаемое имя поля отображения
		string name = "";
		// Выполняем извлечение имени поля отображения
		if(!value.key(i).value(name)){
			// Если вид имени надлежит пропустить вовсе
			if(this->_settings.narrow == narrow_t::SKIP)
				// Продолжаем перебор полей отображения дальше
				continue;
			// Запоминаем код отказа перевода
			this->_error = error_t::UNSUPPORTED;
			// Выходим из метода, перевод отвечен отказом
			return false;
		}
		// Если поле отображением не является
		if(!value[name].is(abc::type_t::MAP)){
			// Выполняем запись значения свойством верхнего уровня
			if(!this->feedINI(value[name], document, name, ""))
				// Выходим из метода, запись отвечена отказом
				return false;
		}
	}
	// Выполняем перебор всех полей отображения повторно
	for(size_t i = 0; i < value.size(); i++){
		// Извлекаемое имя поля отображения
		string name = "";
		// Выполняем извлечение имени поля отображения
		if(!value.key(i).value(name))
			// Продолжаем перебор полей отображения дальше
			continue;
		// Извлекаемое значение поля отображения
		const abc::value_t & node = value[name];
		// Если поле отображением не является
		if(!node.is(abc::type_t::MAP))
			// Продолжаем перебор полей отображения дальше
			continue;
		// Выполняем заведение раздела записи настроек
		if(!document.create(name))
			// Выходим из метода, заведение отвечено отказом
			return false;
		// Выполняем перебор всех свойств раздела
		for(size_t j = 0; j < node.size(); j++){
			// Извлекаемое имя свойства раздела
			string key = "";
			// Выполняем извлечение имени свойства раздела
			if(!node.key(j).value(key)){
				// Если вид имени надлежит пропустить вовсе
				if(this->_settings.narrow == narrow_t::SKIP)
					// Продолжаем перебор свойств раздела дальше
					continue;
				// Запоминаем код отказа перевода
				this->_error = error_t::UNSUPPORTED;
				// Выходим из метода, перевод отвечен отказом
				return false;
			}
			// Если свойство раздела оказалось отображением
			if(node[key].is(abc::type_t::MAP)){
				// Если вид надлежит пропустить вовсе
				if(this->_settings.narrow == narrow_t::SKIP)
					// Продолжаем перебор свойств раздела дальше
					continue;
				// Запоминаем код отказа перевода
				this->_error = error_t::UNSUPPORTED;
				// Выходим из метода, перевод отвечен отказом
				return false;
			}
			// Выполняем запись значения свойством раздела
			if(!this->feedINI(node[key], document, key, name))
				// Выходим из метода, запись отвечена отказом
				return false;
		}
	}
	// Выполняем изъятие собранной записи INI
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
	/**
	 * @brief Заслон пустой записи при успешном переводе
	 *
	 * @details Кодек, запись собрать не сумевший, отдаёт её ПУСТОЙ, а признака
	 * отказа при том не подаёт: отказ его остаётся внутри собирателя записи.
	 * Замерено 04.09.2026 - дерево с полем, разметке негодным по имени,
	 * переводилось признаком успеха и записью пустою, и потребитель сохранял
	 * пустой файл вместо своих настроек
	 *
	 * @warning Заслон стоит ЗДЕСЬ, а не у каждого перевода порознь, оттого что
	 *          изъятие записи у всех кодеков одно и то же и всякий из них
	 *          молчит одинаково
	 *
	 * @note Пустая запись при пустом дереве отказом НЕ считается, равно как и
	 *       при правиле сужения `SKIP`: там пустота есть законный исход
	 */
	auto guardFn = [this, &value, &result](const bool ok) noexcept -> bool {
		/**
		 * Если перевод отвечен отказом
		 *
		 * @warning Отказ ОБЯЗАН нести причину: перевод, отвеченный отказом с кодом
		 *          `NONE`, оставляет потребителя без ответа на вопрос «почему», и
		 *          выглядит это как отказ без причины вовсе. Замерено 04.09.2026
		 *          ворошителем - 5291 отказ из 37 463 приходил без кода, ибо часть
		 *          путей отдаёт отказ собирателя записи, своего кода не запомнив
		 */
		if(!ok){
			// Если код отказа перевода не запомнен
			if(this->_error == error_t::NONE)
				// Запоминаем код отказа перевода
				this->_error = error_t::WRITING;
			// Выводим признак отказа перевода
			return false;
		}
		// Если запись собрана непустою
		if(!result.empty())
			// Выводим признак перевода, как его отдал кодек
			return ok;
		// Если дерево пусто само либо сужение велит пропускать
		if(!value.valid() || (value.is(abc::type_t::CONTAINER) && (value.size() == 0)) || (this->_settings.narrow == narrow_t::SKIP))
			// Выводим признак успешного перевода
			return true;
		// Запоминаем код отказа перевода
		this->_error = error_t::WRITING;
		// Выводим сообщение о пустой записи при непустом дереве
		this->_log->print("Кодек отдал пустую запись при непустом дереве значений", log_t::flag_t::WARNING);
		// Выходим из метода, перевод отвечен отказом
		return false;
	};
	// Определяем вид записи, в который переводится дерево
	switch(static_cast <uint8_t> (format)){
		// Если запись переводится в вид JSON
		case static_cast <uint8_t> (format_t::JSON):
			// Выводим результат перевода дерева в запись JSON
			return guardFn(this->encodeJSON(value, result));
		// Если запись переводится в вид YAML
		case static_cast <uint8_t> (format_t::YAML):
			// Выводим результат перевода дерева в запись YAML
			return guardFn(this->encodeYAML(value, result));
		// Если запись переводится в вид TOML
		case static_cast <uint8_t> (format_t::TOML):
			// Выводим результат перевода дерева в запись TOML
			return guardFn(this->encodeTOML(value, result));
		// Если запись переводится в вид XML
		case static_cast <uint8_t> (format_t::XML):
			// Выводим результат перевода дерева в запись XML
			return guardFn(this->encodeXML(value, result));
		// Если запись переводится в вид INI
		case static_cast <uint8_t> (format_t::INI):
			// Выводим результат перевода дерева в запись INI
			return guardFn(this->encodeINI(value, result));
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
