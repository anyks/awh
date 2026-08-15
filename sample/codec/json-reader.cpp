/**
 * @file json-reader.cpp
 * @date 2026-08-15
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
 * @brief Пример потокового чтения текста JSON — подача текста кусками произвольного
 *        размера, выдача событий значение за значением без удержания документа целиком
 *        и потоковая выдача готовых документов из потока NDJSON
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/json/json.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён контейнеров данных
 */
using namespace awh::codec;

/**
 * @brief Метод получения названия события разбора
 *
 * @param event событие разбора
 * @return      название события разбора
 *
 */
static const char * naming(const json::event_t event) noexcept {
	/**
	 * Определяем вид события разбора
	 */
	switch(static_cast <uint8_t> (event)){
		// Если событие является открытием объекта
		case static_cast <uint8_t> (json::event_t::OBJECT_BEGIN): return "объект открыт";
		// Если событие является закрытием объекта
		case static_cast <uint8_t> (json::event_t::OBJECT_END): return "объект закрыт";
		// Если событие является открытием массива
		case static_cast <uint8_t> (json::event_t::ARRAY_BEGIN): return "массив открыт";
		// Если событие является закрытием массива
		case static_cast <uint8_t> (json::event_t::ARRAY_END): return "массив закрыт";
		// Если событие является именем поля объекта
		case static_cast <uint8_t> (json::event_t::KEY): return "имя поля";
		// Если событие является строковым значением
		case static_cast <uint8_t> (json::event_t::STRING): return "строка";
		// Если событие является числом
		case static_cast <uint8_t> (json::event_t::NUMBER): return "число";
		// Если событие является логическим значением
		case static_cast <uint8_t> (json::event_t::BOOL): return "логическое";
		// Если событие является пустым значением
		case static_cast <uint8_t> (json::event_t::NUL): return "пусто";
		// Если событие является примечанием
		case static_cast <uint8_t> (json::event_t::COMMENT): return "примечание";
		// Если событие является окончанием документа
		case static_cast <uint8_t> (json::event_t::DOCUMENT): return "документ окончен";
		// Если событие является исчерпанием поданного текста
		case static_cast <uint8_t> (json::event_t::FINISH): return "текст исчерпан";
	}
	// Выводим название неопознанного события
	return "неизвестно";
}

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Не используем параметры приложения
	(void) argc;
	(void) argv;
	// Разбираемый текст документа
	const string text = R"({"имя": "заказ", "число": 42, "метки": ["новый", "срочный"], "оплачен": false})";
	// Объект потокового чтения текста документа
	json::reader_t reader;
	/**
	 * Выполняем подачу текста документа кусками по семь байтов
	 *
	 * @details Размер куска взят нарочито неудобным: он рвёт и записи чисел, и знаки
	 * кодировки, и отменяющие последовательности. Выдача от нарезки текста не зависит
	 * вовсе - знак, чьё значение решается следующим за ним, переводит разбор в отдельное
	 * состояние, а не заглядывает вперёд
	 *
	 * @note Именно этот договор позволяет читать документ из сети, отдавая разбору то,
	 *       что пришло, не дожидаясь конца
	 */
	const size_t chunk = 7;
	/**
	 * Выполняем подачу текста документа кусками
	 */
	for(size_t offset = 0; offset <= text.size(); offset += chunk){
		// Получаем размер очередного подаваемого куска
		const size_t length = (((offset + chunk) < text.size()) ? chunk : (text.size() - offset));
		// Выполняем подачу очередного куска текста документа чтению
		const bool ok = reader.feed(text.data() + offset, length, ((offset + length) >= text.size()));
		/**
		 * Если подача куска текста документа завершилась отказом
		 *
		 * @note Место отказа снимается сразу по возвращении подачи: метод `location()`
		 *       отдаёт место ТЕКУЩЕГО события, и снятое после выдачи оставшихся событий
		 *       указывало бы уже на них
		 */
		if(!ok){
			// Выводим причину отказа разбора вместе с местом её
			cerr << "Разбор отвергнут: " << json::message(reader.error())
			     << " (строка " << reader.location().line
			     << ", знак " << reader.location().column << ")" << endl;
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		}
		/**
		 * Выполняем перебор всех собранных событий разбора
		 */
		while(reader.next()){
			// Получаем значение очередного события разбора
			const json::reader_t::value_t value = reader.value();
			// Выводим вид события разбора вместе с местом его
			cout << "строка " << reader.location().line
			     << ", знак " << reader.location().column
			     << ": " << ::naming(reader.event());
			/**
			 * Если событие несёт содержимое
			 */
			if(!value.text.empty())
				// Выводим содержимое события разбора
				cout << " «" << value.text << "»";
			// Выполняем перевод строки вывода
			cout << endl;
		}
		/**
		 * Если текст документа исчерпан
		 */
		if((offset + length) >= text.size())
			// Прекращаем подачу текста документа
			break;
	}
	// Выводим заголовок разбора потока документов
	cout << endl << "Поток документов NDJSON:" << endl;
	// Разбираемый поток документов
	const string stream = "{\"номер\": 1}\n{\"номер\": 2}\n{\"номер\": 3}";
	// Объект дерева документа
	json::document_t document;
	// Получаем настройки разбора документа
	json::document_t::settings_t settings = document.settings();
	// Разрешаем разбор потока документов
	settings.reader.stream = true;
	// Выполняем установку настроек разбора документа
	document.settings(settings);
	/**
	 * Выполняем разбор потока документов с потоковой выдачей
	 *
	 * @note Обработчик получает всякий документ готовым деревом, а память под него
	 *       переиспользуется: поток любой длины разбирается в постоянной памяти
	 */
	const bool ok = document.parse(stream, [](const json::document_t::value_t & value) noexcept -> bool {
		// Извлекаемый номер документа
		int64_t number = 0;
		// Выполняем извлечение номера документа
		value["номер"].value(number);
		// Выводим номер очередного документа потока
		cout << "принят документ с номером " << number << endl;
		/**
		 * Выводим признак продолжения разбора
		 *
		 * @note Ответ ложью прекращает разбор немедля: потребителю довольно прочесть
		 *       начало потока и остановиться
		 */
		return true;
	});
	/**
	 * Если разбор потока документов завершился отказом
	 */
	if(!ok){
		// Выводим причину отказа разбора
		cerr << "Разбор потока отвергнут: " << json::message(document.error()) << endl;
		// Выходим из приложения с кодом ошибки
		return EXIT_FAILURE;
	}
	// Выходим из приложения
	return EXIT_SUCCESS;
}
