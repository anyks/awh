/**
 * @file document.cpp
 * @date 2026-08-12
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
 * @brief Реализация контейнера CSV — удержание таблицы целиком, доступ к полям по
 *        номеру записи и по имени столбца, чтение и запись файла
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <fstream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/csv/document.hpp>

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
	 * @brief Размер куска, каким читается файл таблицы
	 *
	 * @details Файл читается кусками, а не целиком: таблица в несколько гигабайт
	 * иначе поднималась бы в память дважды - сырым текстом и разобранной
	 *
	 */
	constexpr size_t CHUNK = 0x10000;
}

/**
 * Используем пространство имён библиотеки
 */
using namespace awh;

/**
 * @brief Метод получения содержимого по указанию в хранилище знаков
 *
 * @param span указание на содержимое в хранилище знаков
 * @return     содержимое, на которое указывает указание
 *
 */
string_view awh::codec::csv::Document::get(const span_t & span) const noexcept {
	// Выводим содержимое, на которое указывает указание
	return string_view(this->_storage.data() + span.offset, span.length);
}
/**
 * @brief Метод перестроения соответствия имён столбцов их номерам
 *
 */
void awh::codec::csv::Document::reindex() noexcept {
	// Очищаем соответствие имён столбцов их номерам
	this->_columns.clear();
	/**
	 * Выполняем перебор всех имён столбцов
	 */
	for(uint32_t i = 0; i < static_cast <uint32_t> (this->_header.size()); i++){
		// Получаем имя очередного столбца
		const span_t & name = this->_header.at(i);
		/**
		 * Заносим соответствие имени столбца его номеру
		 *
		 * @note Повторное имя не замещает прежнее: обращение по имени отдаёт первый
		 *       столбец с таким именем, а прочие остаются доступны по номеру. Разбор
		 *       повторные имена отвергает, но заголовок вправе быть задан и извне
		 */
		this->_columns.emplace(string_view(this->_names.data() + name.offset, name.length), i);
	}
}
/**
 * @brief Метод сбора событий разбора в таблицу
 *
 * @param reader чтение, выдающее события разбора
 *
 */
void awh::codec::csv::Document::consume(reader_t & reader) noexcept {
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		/**
		 * Определяем вид события разбора
		 */
		switch(static_cast <uint8_t> (reader.event())){
			/**
			 * Если событием является поле заголовка
			 */
			case static_cast <uint8_t> (event_t::HEADER): {
				// Получаем содержимое поля заголовка
				const string_view value = reader.field().value;
				// Заносим указание на имя столбца
				this->_header.emplace_back(static_cast <uint32_t> (this->_names.size()), static_cast <uint32_t> (value.length()));
				// Переносим имя столбца в хранилище имён
				this->_names.append(value);
			} break;
			/**
			 * Если событием является поле записи
			 */
			case static_cast <uint8_t> (event_t::FIELD): {
				// Получаем содержимое поля записи
				const string_view value = reader.field().value;
				/**
				 * Если запись ещё не начата
				 *
				 * @note Начало записи отмечается первым её полем, а не событием конца
				 *       предыдущей: событие конца приходит и у записи заголовка, поля
				 *       которой в таблицу не попадают вовсе
				 */
				if(!this->_opened){
					// Запоминаем признак начатой записи
					this->_opened = true;
					// Заносим указание на начало записи
					this->_records.push_back(static_cast <uint32_t> (this->_fields.size()));
				}
				// Заносим указание на поле записи
				this->_fields.emplace_back(static_cast <uint32_t> (this->_storage.size()), static_cast <uint32_t> (value.length()));
				// Переносим содержимое поля в хранилище знаков
				this->_storage.append(value);
			} break;
			/**
			 * Если событием является конец записи
			 */
			case static_cast <uint8_t> (event_t::RECORD):
				// Снимаем признак начатой записи
				this->_opened = false;
			break;
		}
	}
}
/**
 * @brief Метод разбора текста таблицы
 *
 * @param text разбираемый текст таблицы
 * @return     результат разбора
 *
 */
bool awh::codec::csv::Document::parse(const string_view text) noexcept {
	// Выполняем очистку таблицы
	this->clear();
	// Чтение текста таблицы
	reader_t reader(this->_log, this->_settings.reader);
	/**
	 * Выполняем подачу текста таблицы кусками
	 *
	 * @note Текст здесь уже в памяти целиком, и подать его разом было бы проще, однако
	 *       разбор собирает события в очередь выдачи, а выдаются они лишь по возврате
	 *       из подачи: очередь эта вместила бы события всей таблицы разом, заняв под
	 *       них памяти вшестеро больше самой таблицы. Подача кусками держит очередь
	 *       короткой, а на скорость разбора не влияет вовсе
	 */
	for(size_t offset = 0; offset < text.size(); offset += ::CHUNK){
		// Получаем размер очередного подаваемого куска
		const size_t size = (((offset + ::CHUNK) > text.size()) ? (text.size() - offset) : ::CHUNK);
		// Выполняем подачу очередного куска текста таблицы
		reader.feed(text.data() + offset, size, ((offset + size) >= text.size()));
		// Выполняем сбор событий разбора в таблицу
		this->consume(reader);
	}
	/**
	 * Если текст таблицы пуст
	 */
	if(text.empty()){
		// Выполняем подачу пустого текста таблицы
		reader.feed(text.data(), 0, true);
		// Выполняем сбор событий разбора в таблицу
		this->consume(reader);
	}
	// Запоминаем код ошибки разбора
	this->_error = reader.error();
	// Запоминаем положение ошибки в исходном тексте
	this->_location = reader.location();
	// Выполняем перестроение соответствия имён столбцов
	this->reindex();
	// Выводим результат разбора
	return (this->_error == error_t::NONE);
}
/**
 * @brief Метод разбора текста таблицы с заданными настройками
 *
 * @param text     разбираемый текст таблицы
 * @param settings настройки контейнера
 * @return         результат разбора
 *
 */
bool awh::codec::csv::Document::parse(const string_view text, const settings_t & settings) noexcept {
	// Выполняем установку настроек контейнера
	this->settings(settings);
	// Выполняем разбор текста таблицы
	return this->parse(text);
}
/**
 * @brief Метод чтения таблицы из файла
 *
 * @param filename адрес файла таблицы для чтения
 * @return         результат чтения
 *
 */
bool awh::codec::csv::Document::read(const string & filename) noexcept {
	// Выполняем очистку таблицы
	this->clear();
	// Открываем файл таблицы для чтения
	ifstream file(filename, ios::binary);
	/**
	 * Если файл таблицы открыть не удалось
	 */
	if(!file.is_open()){
		//
		// Запоминаем код ошибки чтения
		//
		// @note Отказ этот наш собственный внутренний изъян не означает: путь передан
		//       извне, и прежний код внутренней ошибки разбора вводил потребителя в
		//       заблуждение, отправляя искать дефект у нас
		//
		this->_error = error_t::FILE_NOT_OPENED;
		// Выполняем вывод сообщения об отказе в лог
		this->report();
		// Выводим признак неудачного чтения
		return false;
	}
	// Чтение текста таблицы
	reader_t reader(this->_log, this->_settings.reader);
	// Буфер очередного куска файла таблицы
	string buffer(::CHUNK, '\0');
	/**
	 * Выполняем чтение файла таблицы кусками
	 */
	while(file){
		// Выполняем чтение очередного куска файла таблицы
		file.read(buffer.data(), static_cast <streamsize> (buffer.size()));
		// Получаем размер прочитанного куска файла таблицы
		const size_t size = static_cast <size_t> (file.gcount());
		/**
		 * Если подача куска чтению не удалась
		 */
		if(!reader.feed(buffer.data(), size, !static_cast <bool> (file)))
			// Прекращаем чтение файла таблицы
			break;
		// Выполняем сбор событий разбора в таблицу
		this->consume(reader);
	}
	// Закрываем файл таблицы
	file.close();
	// Запоминаем код ошибки разбора
	this->_error = reader.error();
	// Запоминаем положение ошибки в исходном тексте
	this->_location = reader.location();
	// Выполняем перестроение соответствия имён столбцов
	this->reindex();
	// Выводим результат чтения
	return (this->_error == error_t::NONE);
}
/**
 * @brief Метод выдачи событий разбора записями обработчику
 *
 * @details Содержимое полей переносится в буфер записи, а не выдаётся ссылками в
 * хранилище чтения. Причина не в удобстве: запись вправе пересечь границу куска, и
 * поля её, выданные прежде границы, к приходу конца записи в хранилище чтения уже не
 * живут
 *
 * @param reader   чтение, выдающее события разбора
 * @param storage  буфер знаков текущей записи
 * @param spans    указания на поля текущей записи в буфере
 * @param fields   поля текущей записи, собранные для выдачи
 * @param callback обработчик очередной записи
 * @return         признак продолжения чтения
 *
 */
static bool dispatch(awh::codec::csv::reader_t & reader, string & storage, vector <awh::codec::csv::span_t> & spans, vector <string_view> & fields, const function <bool (const vector <string_view> &)> & callback) noexcept {
	/**
	 * Пространство имён контейнера CSV
	 */
	using namespace awh::codec::csv;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		/**
		 * Определяем вид события разбора
		 */
		switch(static_cast <uint8_t> (reader.event())){
			/**
			 * Если событием является поле записи
			 */
			case static_cast <uint8_t> (event_t::FIELD): {
				// Получаем содержимое поля записи
				const string_view value = reader.field().value;
				// Заносим указание на поле в буфере записи
				spans.emplace_back(static_cast <uint32_t> (storage.size()), static_cast <uint32_t> (value.length()));
				// Переносим содержимое поля в буфер записи
				storage.append(value);
			} break;
			/**
			 * Если событием является конец записи
			 */
			case static_cast <uint8_t> (event_t::RECORD): {
				/**
				 * Если полей у записи не собрано вовсе
				 *
				 * @note Событие конца записи приходит и у заголовка, поля которого
				 *       выдаются событиями иного вида и в запись не попадают. Выдать
				 *       такую запись обработчику значило бы предпослать таблице пустую
				 *       запись и сдвинуть нумерацию всех прочих на единицу. Записи же
				 *       без полей не существует: строка без полей пуста, а пустые
				 *       строки разбор пропускает
				 */
				if(spans.empty())
					// Переходим к следующему событию разбора
					break;
				// Очищаем поля, собранные для выдачи
				fields.clear();
				// Резервируем память под поля записи
				fields.reserve(spans.size());
				/**
				 * Выполняем перебор всех полей записи
				 *
				 * @note Ссылки собираются здесь, а не по мере разбора: буфер записи по
				 *       ходу её сбора прирастает и вправе быть перенесён в памяти,
				 *       обесценив ссылки, собранные прежде
				 */
				for(const span_t & span : spans)
					// Заносим ссылку на очередное поле записи
					fields.emplace_back(storage.data() + span.offset, span.length);
				// Выполняем выдачу собранной записи обработчику
				const bool result = callback(fields);
				// Очищаем буфер знаков текущей записи
				storage.clear();
				// Очищаем указания на поля текущей записи
				spans.clear();
				/**
				 * Если обработчик затребовал прекращение чтения
				 */
				if(!result)
					// Выводим признак прекращения чтения
					return false;
			} break;
		}
	}
	// Выводим признак продолжения чтения
	return true;
}
/**
 * @brief Метод чтения таблицы из файла записями
 *
 * @param filename адрес файла таблицы для чтения
 * @param callback обработчик очередной записи, ложь прекращает чтение
 * @return         результат чтения
 *
 */
bool awh::codec::csv::Document::read(const string & filename, const function <bool (const vector <string_view> &)> & callback) noexcept {
	// Выполняем очистку таблицы
	this->clear();
	/**
	 * Если обработчик записей не передан
	 */
	if(!static_cast <bool> (callback)){
		// Запоминаем код ошибки разбора
		this->_error = error_t::INTERNAL;
		// Выполняем вывод сообщения об отказе в лог
		this->report();
		// Выводим признак неудачного чтения
		return false;
	}
	// Открываем файл таблицы для чтения
	ifstream file(filename, ios::binary);
	/**
	 * Если файл таблицы открыть не удалось
	 */
	if(!file.is_open()){
		//
		// Запоминаем код ошибки чтения
		//
		// @note Отказ этот наш собственный внутренний изъян не означает: путь передан
		//       извне, и прежний код внутренней ошибки разбора вводил потребителя в
		//       заблуждение, отправляя искать дефект у нас
		//
		this->_error = error_t::FILE_NOT_OPENED;
		// Выполняем вывод сообщения об отказе в лог
		this->report();
		// Выводим признак неудачного чтения
		return false;
	}
	// Чтение текста таблицы
	reader_t reader(this->_log, this->_settings.reader);
	// Буфер очередного куска файла таблицы
	string buffer(::CHUNK, '\0');
	// Буфер знаков текущей записи
	string storage;
	// Указания на поля текущей записи в буфере
	vector <span_t> spans;
	// Поля текущей записи, собранные для выдачи
	vector <string_view> fields;
	/**
	 * Выполняем чтение файла таблицы кусками
	 */
	while(file){
		// Выполняем чтение очередного куска файла таблицы
		file.read(buffer.data(), static_cast <streamsize> (buffer.size()));
		// Получаем размер прочитанного куска файла таблицы
		const size_t size = static_cast <size_t> (file.gcount());
		/**
		 * Если подача куска чтению не удалась
		 */
		if(!reader.feed(buffer.data(), size, !static_cast <bool> (file)))
			// Прекращаем чтение файла таблицы
			break;
		/**
		 * Если обработчик затребовал прекращение чтения
		 */
		if(!::dispatch(reader, storage, spans, fields, callback))
			// Прекращаем чтение файла таблицы
			break;
	}
	// Закрываем файл таблицы
	file.close();
	// Запоминаем код ошибки разбора
	this->_error = reader.error();
	// Запоминаем положение ошибки в исходном тексте
	this->_location = reader.location();
	/**
	 * Выполняем перебор всех имён столбцов заголовка
	 *
	 * @note Заголовок переносится в контейнер, а записи - нет: имена столбцов стоят
	 *       памяти в размер одной записи, а нужны бывают уже после чтения
	 */
	for(const string_view name : reader.header()){
		// Заносим указание на имя столбца
		this->_header.emplace_back(static_cast <uint32_t> (this->_names.size()), static_cast <uint32_t> (name.length()));
		// Переносим имя столбца в хранилище имён
		this->_names.append(name);
	}
	// Выполняем перестроение соответствия имён столбцов
	this->reindex();
	// Выводим результат чтения
	return (this->_error == error_t::NONE);
}
/**
 * @brief Метод разбора текста таблицы записями
 *
 * @param text     разбираемый текст таблицы
 * @param callback обработчик очередной записи, ложь прекращает разбор
 * @return         результат разбора
 *
 */
bool awh::codec::csv::Document::parse(const string_view text, const function <bool (const vector <string_view> &)> & callback) noexcept {
	// Выполняем очистку таблицы
	this->clear();
	/**
	 * Если обработчик записей не передан
	 */
	if(!static_cast <bool> (callback)){
		// Запоминаем код ошибки разбора
		this->_error = error_t::INTERNAL;
		// Выполняем вывод сообщения об отказе в лог
		this->report();
		// Выводим признак неудачного разбора
		return false;
	}
	// Чтение текста таблицы
	reader_t reader(this->_log, this->_settings.reader);
	// Буфер знаков текущей записи
	string storage;
	// Указания на поля текущей записи в буфере
	vector <span_t> spans;
	// Поля текущей записи, собранные для выдачи
	vector <string_view> fields;
	/**
	 * Выполняем подачу текста таблицы кусками
	 *
	 * @note Текст здесь уже в памяти целиком, и подать его разом было бы проще, однако
	 *       разбор собирает события в очередь выдачи, а выдаются они лишь по возврате
	 *       из подачи: очередь эта вместила бы события всей таблицы разом. Путь этот
	 *       заведён ради таблиц, в память не помещающихся, и удерживать в памяти
	 *       разобранное значило бы отнять у него то единственное, ради чего он есть
	 */
	for(size_t offset = 0; offset < text.size(); offset += ::CHUNK){
		// Получаем размер очередного подаваемого куска
		const size_t size = (((offset + ::CHUNK) > text.size()) ? (text.size() - offset) : ::CHUNK);
		/**
		 * Если подача куска чтению не удалась
		 */
		if(!reader.feed(text.data() + offset, size, ((offset + size) >= text.size())))
			// Прекращаем подачу текста таблицы
			break;
		/**
		 * Если обработчик затребовал прекращение разбора
		 */
		if(!::dispatch(reader, storage, spans, fields, callback))
			// Прекращаем подачу текста таблицы
			break;
	}
	/**
	 * Если текст таблицы пуст
	 *
	 * @note Подача пустого текста нужна затем, чтобы разбор объявил его окончание:
	 *       без неё чтение осталось бы ждать продолжения
	 */
	if(text.empty())
		// Выполняем подачу пустого текста таблицы
		reader.feed(text.data(), 0, true);
	// Запоминаем код ошибки разбора
	this->_error = reader.error();
	// Запоминаем положение ошибки в исходном тексте
	this->_location = reader.location();
	/**
	 * Выполняем перебор всех имён столбцов заголовка
	 */
	for(const string_view name : reader.header()){
		// Заносим указание на имя столбца
		this->_header.emplace_back(static_cast <uint32_t> (this->_names.size()), static_cast <uint32_t> (name.length()));
		// Переносим имя столбца в хранилище имён
		this->_names.append(name);
	}
	// Выполняем перестроение соответствия имён столбцов
	this->reindex();
	// Выводим результат разбора
	return (this->_error == error_t::NONE);
}
/**
 * @brief Метод записи таблицы в файл
 *
 * @details Текст собирается записями, а не целиком: таблица в несколько гигабайт иначе
 * держалась бы в памяти дважды - разобранной и собранной
 *
 * @param filename адрес файла таблицы для записи
 * @return         результат записи
 *
 */
bool awh::codec::csv::Document::write(const string & filename) const noexcept {
	// Открываем файл таблицы для записи
	ofstream file(filename, ios::binary | ios::trunc);
	/**
	 * Если файл таблицы открыть не удалось
	 */
	/**
	 * Если файл таблицы открыть не удалось
	 *
	 * @note Отказ этот идёт мимо разбора, а вывод в лог ведёт именно он: без настоящего
	 *       вывода запись файла отказывала бы молча, тогда как чтение того же файла
	 *       оглашает отказ кодом `FILE_NOT_OPENED`
	 */
	if(!file.is_open()){
		/**
		 * Если объект для работы с логами установлен
		 */
		if(this->_log != nullptr)
			// Выполняем вывод сообщения об отказе
			this->_log->print("CSV document failed: %s", log_t::flag_t::CRITICAL, awh::codec::csv::message(error_t::FILE_NOT_OPENED));
		// Выводим признак неудачной записи
		return false;
	}
	// Запись текста таблицы
	writer_t writer(this->_log, this->_settings.writer);
	/**
	 * Если таблица имеет заголовок
	 */
	if(!this->_header.empty()){
		/**
		 * Выполняем перебор всех имён столбцов
		 */
		for(const span_t & name : this->_header)
			// Записываем имя очередного столбца
			writer.field(string_view(this->_names.data() + name.offset, name.length));
		// Завершаем запись заголовка
		writer.record();
	}
	/**
	 * Выполняем перебор всех записей таблицы
	 */
	for(size_t i = 0; i < this->_records.size(); i++){
		// Получаем номер первого поля записи
		const uint32_t begin = this->_records.at(i);
		// Получаем номер за последним полем записи
		const uint32_t end = (((i + 1) < this->_records.size()) ? this->_records.at(i + 1) : static_cast <uint32_t> (this->_fields.size()));
		/**
		 * Выполняем перебор всех полей записи
		 */
		for(uint32_t j = begin; j < end; j++)
			// Записываем очередное поле записи
			writer.field(this->get(this->_fields.at(j)));
		// Завершаем запись
		writer.record();
		/**
		 * Если собранного текста накопилось довольно
		 */
		if(writer.size() >= ::CHUNK){
			// Изымаем собранный текст
			const string text = writer.take();
			// Записываем изъятый текст в файл таблицы
			file.write(text.data(), static_cast <streamsize> (text.size()));
		}
	}
	// Изымаем остаток собранного текста
	const string text = writer.take();
	// Записываем остаток собранного текста в файл таблицы
	file.write(text.data(), static_cast <streamsize> (text.size()));
	/**
	 * Выполняем закрытие файла таблицы ДО сличения исхода
	 *
	 * @note Порядок здесь значим: поток сбрасывает свой буфер закрытием своим, и
	 *       признак, снятый прежде закрытия, отказа сброса не видит. Текст, целиком
	 *       уместившийся в буфер, уходил бы отказом молча, а вызов отчитывался бы
	 *       успехом - у кодеков JSON и XML то же место чинено тем же порядком
	 */
	file.close();
	/**
	 * Если запись текста таблицы в файл не удалась
	 */
	if(!file){
		/**
		 * Если объект для работы с логами установлен
		 */
		if(this->_log != nullptr)
			// Выполняем вывод сообщения об отказе
			this->_log->print("CSV document failed: %s", log_t::flag_t::CRITICAL, awh::codec::csv::message(error_t::FILE_NOT_WRITTEN));
		// Выводим признак неудачной записи
		return false;
	}
	// Выводим признак успешной записи
	return true;
}
/**
 * @brief Метод получения кода ошибки разбора
 *
 * @return код ошибки разбора
 *
 */
awh::codec::csv::error_t awh::codec::csv::Document::error() const noexcept {
	// Выводим код ошибки разбора
	return this->_error;
}
/**
 * @brief Метод получения положения ошибки разбора
 *
 * @return положение ошибки в исходном тексте
 *
 */
const awh::codec::csv::location_t & awh::codec::csv::Document::location() const noexcept {
	// Выводим положение ошибки в исходном тексте
	return this->_location;
}
/**
 * @brief Метод получения количества записей таблицы
 *
 * @return количество записей таблицы
 *
 */
size_t awh::codec::csv::Document::rows() const noexcept {
	// Выводим количество записей таблицы
	return this->_records.size();
}
/**
 * @brief Метод получения количества столбцов таблицы
 *
 * @return количество столбцов таблицы
 *
 */
size_t awh::codec::csv::Document::cols() const noexcept {
	// Количество столбцов таблицы
	size_t result = this->_header.size();
	/**
	 * Выполняем перебор всех записей таблицы
	 */
	for(size_t i = 0; i < this->_records.size(); i++){
		// Получаем количество полей очередной записи
		const size_t count = this->size(i);
		/**
		 * Если запись содержит больше полей, чем найдено прежде
		 */
		if(count > result)
			// Запоминаем количество столбцов таблицы
			result = count;
	}
	// Выводим количество столбцов таблицы
	return result;
}
/**
 * @brief Метод получения количества полей записи
 *
 * @param row номер записи, считая с нуля
 * @return    количество полей записи
 *
 */
size_t awh::codec::csv::Document::size(const size_t row) const noexcept {
	/**
	 * Если записи с таким номером таблица не содержит
	 */
	if(row >= this->_records.size())
		// Выводим отсутствие полей
		return 0;
	// Получаем номер первого поля записи
	const uint32_t begin = this->_records.at(row);
	// Получаем номер за последним полем записи
	const uint32_t end = (((row + 1) < this->_records.size()) ? this->_records.at(row + 1) : static_cast <uint32_t> (this->_fields.size()));
	// Выводим количество полей записи
	return static_cast <size_t> (end - begin);
}
/**
 * @brief Метод получения имён столбцов
 *
 * @return имена столбцов в порядке объявления
 *
 */
vector <string_view> awh::codec::csv::Document::header() const noexcept {
	// Имена столбцов таблицы
	vector <string_view> result;
	// Резервируем память под имена столбцов
	result.reserve(this->_header.size());
	/**
	 * Выполняем перебор всех имён столбцов
	 */
	for(const span_t & name : this->_header)
		// Заносим имя очередного столбца
		result.emplace_back(this->_names.data() + name.offset, name.length);
	// Выводим имена столбцов таблицы
	return result;
}
/**
 * @brief Метод проверки наличия столбца с заданным именем
 *
 * @param name имя проверяемого столбца
 * @return     результат проверки
 *
 */
bool awh::codec::csv::Document::has(const string_view name) const noexcept {
	// Выводим результат проверки наличия столбца
	return (this->_columns.find(name) != this->_columns.end());
}
/**
 * @brief Метод получения номера столбца по его имени
 *
 * @param name имя искомого столбца
 * @return     номер столбца либо признак отсутствия
 *
 */
uint32_t awh::codec::csv::Document::column(const string_view name) const noexcept {
	// Выполняем поиск столбца по его имени
	const auto i = this->_columns.find(name);
	/**
	 * Если столбец с таким именем найден
	 */
	if(i != this->_columns.end())
		// Выводим номер найденного столбца
		return i->second;
	// Выводим признак отсутствия столбца
	return NO_INDEX;
}
/**
 * @brief Метод получения содержимого поля по номеру записи и столбца
 *
 * @details Отсутствующее поле выдаётся пустым, а не отказом: записи с разным числом
 * полей договор дозволяет, и обращение к полю, которого в записи нет, - обыкновенное
 * дело при обходе таблицы по столбцам
 *
 * @param row номер записи, считая с нуля
 * @param col номер столбца, считая с нуля
 * @return    содержимое поля, пустое при его отсутствии
 *
 */
string_view awh::codec::csv::Document::get(const size_t row, const size_t col) const noexcept {
	/**
	 * Если записи с таким номером таблица не содержит
	 */
	if(row >= this->_records.size())
		// Выводим пустое содержимое
		return string_view();
	/**
	 * Если поля с таким номером запись не содержит
	 */
	if(col >= this->size(row))
		// Выводим пустое содержимое
		return string_view();
	// Выводим содержимое искомого поля
	return this->get(this->_fields.at(this->_records.at(row) + col));
}
/**
 * @brief Метод получения содержимого поля по номеру записи и имени столбца
 *
 * @param row  номер записи, считая с нуля
 * @param name имя столбца
 * @return     содержимое поля, пустое при его отсутствии
 *
 */
string_view awh::codec::csv::Document::get(const size_t row, const string_view name) const noexcept {
	// Получаем номер столбца по его имени
	const uint32_t col = this->column(name);
	/**
	 * Если столбца с таким именем таблица не содержит
	 */
	if(col == NO_INDEX)
		// Выводим пустое содержимое
		return string_view();
	// Выводим содержимое искомого поля
	return this->get(row, static_cast <size_t> (col));
}
/**
 * @brief Метод получения записи целиком
 *
 * @param row номер записи, считая с нуля
 * @return    поля записи в порядке следования
 *
 */
vector <string_view> awh::codec::csv::Document::row(const size_t row) const noexcept {
	// Поля искомой записи
	vector <string_view> result;
	// Получаем количество полей записи
	const size_t count = this->size(row);
	// Резервируем память под поля записи
	result.reserve(count);
	/**
	 * Выполняем перебор всех полей записи
	 */
	for(size_t i = 0; i < count; i++)
		// Заносим содержимое очередного поля
		result.push_back(this->get(this->_fields.at(this->_records.at(row) + i)));
	// Выводим поля искомой записи
	return result;
}
/**
 * @brief Метод получения столбца целиком
 *
 * @param col номер столбца, считая с нуля
 * @return    поля столбца в порядке следования записей
 *
 */
vector <string_view> awh::codec::csv::Document::col(const size_t col) const noexcept {
	// Поля искомого столбца
	vector <string_view> result;
	// Резервируем память под поля столбца
	result.reserve(this->_records.size());
	/**
	 * Выполняем перебор всех записей таблицы
	 */
	for(size_t i = 0; i < this->_records.size(); i++)
		// Заносим содержимое очередного поля
		result.push_back(this->get(i, col));
	// Выводим поля искомого столбца
	return result;
}
/**
 * @brief Метод получения столбца целиком по его имени
 *
 * @param name имя столбца
 * @return     поля столбца в порядке следования записей
 *
 */
vector <string_view> awh::codec::csv::Document::col(const string_view name) const noexcept {
	// Получаем номер столбца по его имени
	const uint32_t col = this->column(name);
	/**
	 * Если столбца с таким именем таблица не содержит
	 */
	if(col == NO_INDEX)
		// Выводим пустой перечень полей
		return vector <string_view> ();
	// Выводим поля искомого столбца
	return this->col(static_cast <size_t> (col));
}
/**
 * @brief Метод приведения содержимого поля к числу либо логическому значению
 *
 * @tparam T тип получаемого значения
 * @param row номер записи, считая с нуля
 * @param col номер столбца, считая с нуля
 * @param result полученное значение
 * @return       результат приведения
 *
 */
template <typename T>
bool awh::codec::csv::Document::numeric(const size_t row, const size_t col, T & result) const noexcept {
	// Получаем содержимое искомого поля
	const string_view value = this->get(row, col);
	/**
	 * Если получено логическое значение
	 */
	if constexpr(is_same <T, bool>::value)
		// Выполняем приведение содержимого поля к логическому значению
		return boolean(value, result);
	/**
	 * Если получено число с плавающей точкой
	 */
	else if constexpr(is_floating_point <T>::value){
		// Полученное число с плавающей точкой
		double number = 0.;
		/**
		 * Если приведение содержимого поля не удалось
		 */
		if(!real(value, number))
			// Выводим признак неудачного приведения
			return false;
		// Запоминаем полученное значение
		result = static_cast <T> (number);
		// Выводим признак успешного приведения
		return true;
	/**
	 * Если получено целое число со знаком
	 */
	} else if constexpr(is_signed <T>::value) {
		// Полученное целое число со знаком
		int64_t number = 0;
		/**
		 * Если приведение содержимого поля не удалось
		 */
		if(!integer(value, number))
			// Выводим признак неудачного приведения
			return false;
		/**
		 * Если число в затребованный вид не помещается
		 *
		 * @details Отказ здесь отвечает правилу самого кодека: разбор записи, в шестьдесят
		 * четыре разряда не вместившейся, отвечает отказом, - и сужение к виду поменьше
		 * обязано отвечать им же. Приведение языка вместо этого переносило разряды и
		 * отдавало значение, ничего общего с содержимым поля не имеющее: поле «300» видом
		 * в один байт без знака выходило числом 44
		 *
		 * @note Кодеки JSON и XML в том же положении приводят число к пределам вида, а не
		 *       отвечают отказом: договор извлечения у них так и записан - отказ следует
		 *       лишь тогда, когда значение числом не является вовсе. Здесь же числом не
		 *       является само содержимое поля, коль скоро оно в затребованный вид не легло
		 */
		if((number < static_cast <int64_t> (numeric_limits <T>::lowest())) ||
		   (number > static_cast <int64_t> (numeric_limits <T>::max())))
			// Выводим признак неудачного приведения
			return false;
		// Запоминаем полученное значение
		result = static_cast <T> (number);
		// Выводим признак успешного приведения
		return true;
	/**
	 * Если получено целое число без знака
	 */
	} else {
		// Полученное целое число без знака
		uint64_t number = 0;
		/**
		 * Если приведение содержимого поля не удалось
		 */
		if(!integer(value, number))
			// Выводим признак неудачного приведения
			return false;
		/**
		 * Если число в затребованный вид не помещается
		 *
		 * @note Отказ отвечает правилу самого кодека: разбор записи, в шестьдесят четыре
		 *       разряда не вместившейся, отвечает отказом - и сужение к виду поменьше
		 *       обязано отвечать им же
		 */
		if(number > static_cast <uint64_t> (numeric_limits <T>::max()))
			// Выводим признак неудачного приведения
			return false;
		// Запоминаем полученное значение
		result = static_cast <T> (number);
		// Выводим признак успешного приведения
		return true;
	}
}
/**
 * Выполняем явное порождение метода приведения содержимого поля для всех числовых типов
 */
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <bool> (const size_t, const size_t, bool &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <int8_t> (const size_t, const size_t, int8_t &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <uint8_t> (const size_t, const size_t, uint8_t &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <int16_t> (const size_t, const size_t, int16_t &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <uint16_t> (const size_t, const size_t, uint16_t &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <int32_t> (const size_t, const size_t, int32_t &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <uint32_t> (const size_t, const size_t, uint32_t &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <int64_t> (const size_t, const size_t, int64_t &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <uint64_t> (const size_t, const size_t, uint64_t &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <float> (const size_t, const size_t, float &) const noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::csv::Document::numeric <double> (const size_t, const size_t, double &) const noexcept;
/**
 * @brief Метод установки заголовка таблицы
 *
 * @param names имена столбцов в порядке следования
 * @return      результат установки
 *
 */
bool awh::codec::csv::Document::header(const vector <string> & names) noexcept {
	// Очищаем хранилище имён столбцов
	this->_names.clear();
	// Очищаем указания на имена столбцов
	this->_header.clear();
	/**
	 * Выполняем перебор всех имён столбцов
	 */
	for(const string & name : names){
		/**
		 * Если имя столбца пусто
		 *
		 * @note Пустое имя обращением по имени недостижимо, и принять его значило бы
		 *       оставить столбец без доступа
		 */
		if(name.empty()){
			// Очищаем хранилище имён столбцов
			this->_names.clear();
			// Очищаем указания на имена столбцов
			this->_header.clear();
			// Выполняем перестроение соответствия имён столбцов
			this->reindex();
			// Выводим признак неудачной установки
			return false;
		}
		// Заносим указание на имя столбца
		this->_header.emplace_back(static_cast <uint32_t> (this->_names.size()), static_cast <uint32_t> (name.length()));
		// Переносим имя столбца в хранилище имён
		this->_names.append(name);
	}
	// Выполняем перестроение соответствия имён столбцов
	this->reindex();
	/**
	 * Если имена столбцов повторяются
	 *
	 * @note Повтор обнаруживается сложением соответствия: повторное имя замещает в нём
	 *       прежнее, отчего соответствие выходит короче заголовка. Принять повтор
	 *       значило бы оставить один из столбцов недостижимым по имени, а разбор
	 *       заголовка повтор отвергает - отвергать его надлежит и здесь
	 */
	if(this->_columns.size() != this->_header.size()){
		// Очищаем хранилище имён столбцов
		this->_names.clear();
		// Очищаем указания на имена столбцов
		this->_header.clear();
		// Выполняем перестроение соответствия имён столбцов
		this->reindex();
		// Выводим признак неудачной установки
		return false;
	}
	// Выводим признак успешной установки
	return true;
}
/**
 * @brief Метод добавления записи в конец таблицы
 *
 * @param fields поля добавляемой записи
 *
 */
void awh::codec::csv::Document::append(const vector <string> & fields) noexcept {
	// Заносим указание на начало записи
	this->_records.push_back(static_cast <uint32_t> (this->_fields.size()));
	/**
	 * Выполняем перебор всех полей записи
	 */
	for(const string & value : fields){
		// Заносим указание на поле записи
		this->_fields.emplace_back(static_cast <uint32_t> (this->_storage.size()), static_cast <uint32_t> (value.length()));
		// Переносим содержимое поля в хранилище знаков
		this->_storage.append(value);
	}
}
/**
 * @brief Метод добавления записи в конец таблицы
 *
 * @param fields поля добавляемой записи
 *
 */
void awh::codec::csv::Document::append(const vector <string_view> & fields) noexcept {
	// Заносим указание на начало записи
	this->_records.push_back(static_cast <uint32_t> (this->_fields.size()));
	// Длина всех полей добавляемой записи
	size_t length = 0;
	/**
	 * Выполняем перебор всех полей записи
	 */
	for(const string_view value : fields)
		// Накапливаем длину полей записи
		length += value.length();
	/**
	 * Выполняем сбор полей записи во временное хранилище
	 *
	 * @note Поля собираются на стороне, а не доливаются в хранилище знаков по одному:
	 *       вид, поданный сюда, вправе указывать в это же хранилище - именно такой вид
	 *       выдают `row()` и `col()`, - и первый же долив, хранилище перераспределивший,
	 *       обратил бы виды остальных полей записи в висячие. Резервированием места
	 *       наперёд беды не избыть: перераспределяет хранилище и оно само, обесценивая
	 *       поданные виды ещё до первого долива
	 */
	string buffer;
	// Резервируем память под поля записи
	buffer.reserve(length);
	/**
	 * Выполняем перебор всех полей записи
	 */
	for(const string_view value : fields)
		// Переносим содержимое поля во временное хранилище
		buffer.append(value);
	// Смещение первого поля записи в хранилище знаков
	size_t offset = this->_storage.size();
	// Переносим собранные поля в хранилище знаков
	this->_storage.append(buffer);
	/**
	 * Выполняем перебор всех полей записи
	 */
	for(const string_view value : fields){
		// Заносим указание на поле записи
		this->_fields.emplace_back(static_cast <uint32_t> (offset), static_cast <uint32_t> (value.length()));
		// Смещаемся на длину занесённого поля
		offset += value.length();
	}
}
/**
 * @brief Метод получения текста таблицы
 *
 * @return собранный текст таблицы
 *
 */
string awh::codec::csv::Document::text() const noexcept {
	// Запись текста таблицы
	writer_t writer(this->_log, this->_settings.writer);
	/**
	 * Если таблица имеет заголовок
	 */
	if(!this->_header.empty()){
		/**
		 * Выполняем перебор всех имён столбцов
		 */
		for(const span_t & name : this->_header)
			// Записываем имя очередного столбца
			writer.field(string_view(this->_names.data() + name.offset, name.length));
		// Завершаем запись заголовка
		writer.record();
	}
	/**
	 * Выполняем перебор всех записей таблицы
	 */
	for(size_t i = 0; i < this->_records.size(); i++){
		// Получаем количество полей записи
		const size_t count = this->size(i);
		/**
		 * Выполняем перебор всех полей записи
		 */
		for(size_t j = 0; j < count; j++)
			// Записываем очередное поле записи
			writer.field(this->get(this->_fields.at(this->_records.at(i) + j)));
		// Завершаем запись
		writer.record();
	}
	// Выводим собранный текст таблицы
	return writer.text();
}
/**
 * @brief Метод очистки таблицы
 *
 * @details Настройки контейнера сохраняются: очищается лишь то, что накоплено разбором
 *
 */
void awh::codec::csv::Document::clear() noexcept {
	// Сбрасываем код ошибки разбора
	this->_error = error_t::NONE;
	// Сбрасываем положение ошибки в исходном тексте
	this->_location = location_t();
	// Очищаем хранилище знаков полей таблицы
	this->_storage.clear();
	// Очищаем хранилище имён столбцов
	this->_names.clear();
	// Очищаем указания на поля таблицы
	this->_fields.clear();
	// Очищаем указания на начало записей
	this->_records.clear();
	// Очищаем указания на имена столбцов
	this->_header.clear();
	// Снимаем признак начатой записи
	this->_opened = false;
	// Очищаем соответствие имён столбцов их номерам
	this->_columns.clear();
}
/**
 * @brief Метод получения настроек контейнера
 *
 * @return настройки контейнера
 *
 */
const awh::codec::csv::Document::settings_t & awh::codec::csv::Document::settings() const noexcept {
	// Выводим настройки контейнера
	return this->_settings;
}
/**
 * @brief Метод установки настроек контейнера
 *
 * @param settings настройки контейнера
 *
 */
void awh::codec::csv::Document::settings(const settings_t & settings) noexcept {
	// Запоминаем настройки контейнера
	this->_settings = settings;
}
/**
 * @brief Оператор вывода таблицы последовательностью знаков
 *
 * @return собранный текст таблицы
 *
 */
awh::codec::csv::Document::operator string() const noexcept {
	// Выводим собранный текст таблицы
	return this->text();
}
/**
 * @brief Метод вывода сообщения об отказе в лог
 *
 */
void awh::codec::csv::Document::report() const noexcept {
	/**
	 * Если объект для работы с логами установлен
	 */
	if(this->_log != nullptr)
		// Выполняем вывод сообщения об отказе
		this->_log->print("CSV document failed: %s", log_t::flag_t::CRITICAL, awh::codec::csv::message(this->_error));
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 *
 */
awh::codec::csv::Document::Document(const log_t * log) noexcept :
 _log(log), _error(error_t::NONE), _opened(false) {}
/**
 * @brief Конструктор
 *
 * @param log      объект для работы с логами
 * @param settings настройки контейнера
 *
 */
awh::codec::csv::Document::Document(const log_t * log, const settings_t & settings) noexcept :
 _log(log),
 _settings(settings), _error(error_t::NONE), _opened(false) {}
