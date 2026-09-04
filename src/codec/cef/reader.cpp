/**
 * @file reader.cpp
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
 * @brief Реализация потокового чтения записей CEF — сборка записей из кусков, отделение приставки syslog,
 *        разбор полей заголовка по прямой черте, разбор пар расширения ходом fmk_t::kv и снятие
 *        отмены знаков порознь по областям записи
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/reader.hpp>

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
	 * @brief Наибольший объём разобранного текста, удерживаемый до уплотнения хранилища
	 *
	 * @details Разобранное начало хранилища освобождается не при всякой записи, а лишь по
	 * накоплении: перемещение остатка при каждой записи обошлось бы дороже самого разбора
	 */
	constexpr size_t COMPACT_THRESHOLD = 0x10000;

	/**
	 * @brief Метод проверки отменённости знака предшествующими обратными косыми
	 *
	 * @details Знак считается отменённым при НЕЧЁТНОМ числе предшествующих косых: запись
	 *          «\\|» несёт саму косую и черту-разделитель, а «\|» - черту внутри значения
	 *
	 * @param text   текст, знак содержащий
	 * @param offset смещение проверяемого знака в тексте
	 * @return       признак отменённости знака
	 */
	bool escaped(const string_view text, const size_t offset) noexcept {
		// Количество обратных косых, знаку предшествующих
		size_t count = 0;
		/**
		 * Выполняем перебор знаков, проверяемому предшествующих
		 */
		for(size_t i = offset; i > 0; i--){
			// Если предшествующий знак обратной косой не является
			if(text[i - 1] != '\\')
				// Выходим из цикла перебора
				break;
			// Увеличиваем количество обратных косых
			count++;
		}
		// Выводим признак отменённости знака
		return ((count % 2) == 1);
	}
}

/**
 * @brief Метод прекращения разбора ошибкой
 *
 * @param error  код ошибки разбора
 * @param offset смещение места ошибки в хранилище разбора
 * @return       признак наличия очередного события разбора
 */
bool awh::codec::cef::Reader::fail(const error_t error, const size_t offset) noexcept {
	// Запоминаем код ошибки разбора
	this->_error = error;
	// Устанавливаем состояние прекращения разбора ошибкой
	this->_state = state_t::FAILED;
	// Сбрасываем вид текущего события разбора
	this->_event = event_t::NONE;
	// Выполняем определение положения места ошибки в исходном тексте
	this->place(offset, this->_errorPosition);
	// Выводим в лог сообщение об ошибке разбора
	this->_log->print(
		"CEF parsing failed: %s at line %llu column %llu",
		log_t::flag_t::CRITICAL,
		awh::codec::cef::message(error),
		static_cast <unsigned long long> (this->_errorPosition.line),
		static_cast <unsigned long long> (this->_errorPosition.column)
	);
	// Выводим отсутствие очередного события разбора
	return false;
}

/**
 * @brief Метод определения положения смещения в исходном тексте
 *
 * @param offset смещение в хранилище разбора
 * @param result положение, вычисляемое по смещению
 */
void awh::codec::cef::Reader::place(const size_t offset, pos_t & result) const noexcept {
	// Устанавливаем смещение в байтах от начала текста
	result.offset = static_cast <uint64_t> (offset);
	// Устанавливаем номер строки, считая от единицы
	result.line = 1;
	// Устанавливаем номер столбца, считая от единицы
	result.column = 1;
	/**
	 * Выполняем перебор знаков хранилища до искомого смещения
	 */
	for(size_t i = 0; (i < offset) && (i < this->_buffer.size()); i++){
		// Если знак является переводом строки
		if(this->_buffer[i] == '\n'){
			// Увеличиваем номер строки
			result.line++;
			// Сбрасываем номер столбца
			result.column = 1;
		// Если знак переводом строки не является
		} else result.column++;
	}
}

/**
 * @brief Метод отыскания конца текущей записи
 *
 * @param length длина найденной записи без знака конца строки
 * @param next   смещение начала следующей записи
 * @return       признак того, что запись найдена целиком
 */
bool awh::codec::cef::Reader::measure(size_t & length, size_t & next) const noexcept {
	// Выполняем поиск перевода строки, запись завершающего
	const size_t pos = this->_buffer.find('\n', this->_offset);
	// Если перевод строки найден
	if(pos != string::npos){
		// Устанавливаем длину найденной записи
		length = (pos - this->_offset);
		// Устанавливаем смещение начала следующей записи
		next = (pos + 1);
		// Если запись оканчивается возвратом каретки
		if((length > 0) && (this->_buffer[this->_offset + length - 1] == '\r'))
			// Уменьшаем длину записи на знак возврата каретки
			length--;
		// Выводим признак того, что запись найдена целиком
		return true;
	}
	// Если подан последний кусок текста
	if(this->_end){
		// Устанавливаем длину остатка текста записью
		length = (this->_buffer.size() - this->_offset);
		// Устанавливаем смещение начала следующей записи концом текста
		next = this->_buffer.size();
		// Выводим признак того, что запись найдена целиком
		return (length > 0);
	}
	// Выводим признак того, что запись целиком не найдена
	return false;
}

/**
 * @brief Метод отыскания слова «CEF:» в записи
 *
 * @param text   текст записи целиком
 * @param result смещение найденного слова от начала записи
 * @return       признак того, что слово найдено
 */
bool awh::codec::cef::Reader::signature(const string_view text, size_t & result) const noexcept {
	// Смещение поиска слова в записи
	size_t offset = 0;
	/**
	 * Выполняем поиск слова, пока оно в записи отыскивается
	 */
	while((offset = text.find(SIGNATURE, offset)) != string_view::npos){
		// Если найденное слово отмене не подлежит
		if(!escaped(text, offset)){
			// Устанавливаем смещение найденного слова
			result = offset;
			// Выводим признак того, что слово найдено
			return true;
		}
		// Продолжаем поиск слова со следующего знака
		offset++;
	}
	// Выводим признак того, что слово не найдено
	return false;
}

/**
 * @brief Метод отыскания конца поля заголовка
 *
 * @param text   текст, начинающийся с поля заголовка
 * @param result длина поля до разделяющей черты
 * @return       признак того, что черта найдена
 */
bool awh::codec::cef::Reader::bounds(const string_view text, size_t & result) const noexcept {
	// Смещение поиска разделяющей черты в тексте
	size_t offset = 0;
	/**
	 * Выполняем поиск черты, пока она в тексте отыскивается
	 */
	while((offset = text.find('|', offset)) != string_view::npos){
		// Если найденная черта отмене не подлежит
		if(!escaped(text, offset)){
			// Устанавливаем длину поля до разделяющей черты
			result = offset;
			// Выводим признак того, что черта найдена
			return true;
		}
		// Продолжаем поиск черты со следующего знака
		offset++;
	}
	// Выводим признак того, что черта не найдена
	return false;
}

/**
 * @brief Метод снятия отмены знаков со значения
 *
 * @param text   значение, отмену знаков несущее
 * @param area   область записи, из которой значение взято
 * @param result значение со снятой отменой знаков
 */
void awh::codec::cef::Reader::unescape(const string_view text, const area_t area, string & result) const noexcept {
	// Выполняем очистку результирующего значения
	result.clear();
	// Выделяем память под результирующее значение
	result.reserve(text.size());
	/**
	 * Выполняем перебор всех знаков значения
	 */
	for(size_t i = 0; i < text.size(); i++){
		// Если знак обратной косой является последним в значении
		if((text[i] != '\\') || ((i + 1) >= text.size())){
			// Добавляем знак в результирующее значение как есть
			result.append(1, text[i]);
			// Переходим к следующему знаку
			continue;
		}
		// Получаем знак, обратной косой отменяемый
		const char letter = text[i + 1];
		/**
		 * Определяем область записи, из которой значение взято
		 */
		switch(static_cast <uint8_t> (area)){
			// Если значение взято из заголовка записи
			case static_cast <uint8_t> (area_t::HEADER): {
				// Если знак отмене в заголовке подлежит
				if((letter == '|') || (letter == '\\')){
					// Добавляем отменённый знак в результирующее значение
					result.append(1, letter);
					// Пропускаем знак обратной косой
					i++;
				// Если знак отмене в заголовке не подлежит
				} else result.append(1, text[i]);
			} break;
			// Если значение взято из расширения записи
			case static_cast <uint8_t> (area_t::EXTENSION): {
				/**
				 * Определяем знак, обратной косой отменяемый
				 */
				switch(letter){
					// Если отменён знак равенства
					case '=':
					// Если отменена сама обратная косая
					case '\\': {
						// Добавляем отменённый знак в результирующее значение
						result.append(1, letter);
						// Пропускаем знак обратной косой
						i++;
					} break;
					// Если отменён перевод строки
					case 'n': {
						// Добавляем перевод строки в результирующее значение
						result.append(1, '\n');
						// Пропускаем знак обратной косой
						i++;
					} break;
					// Если отменён возврат каретки
					case 'r': {
						// Добавляем возврат каретки в результирующее значение
						result.append(1, '\r');
						// Пропускаем знак обратной косой
						i++;
					} break;
					// Если отменён знак, отмене не подлежащий
					default: result.append(1, text[i]);
				}
			} break;
			// Если значение взято из приставки syslog
			default: result.append(1, text[i]);
		}
	}
}

/**
 * @brief Метод получения настроек разбора записей
 *
 * @return настройки разбора записей
 */
const awh::codec::cef::Reader::settings_t & awh::codec::cef::Reader::settings() const noexcept {
	// Выводим настройки разбора записей
	return this->_settings;
}

/**
 * @brief Метод установки настроек разбора записей
 *
 * @param settings настройки разбора записей
 * @return         результат выполнения операции
 */
bool awh::codec::cef::Reader::settings(const settings_t & settings) noexcept {
	// Если разбор текста уже начат
	if(!this->_buffer.empty()){
		// Выводим в лог сообщение о невозможности смены настроек
		this->_log->print("CEF settings cannot be changed in the middle of a parsing", log_t::flag_t::WARNING);
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Устанавливаем настройки разбора записей
	this->_settings = settings;
	// Выводим положительный результат выполнения операции
	return true;
}

/**
 * @brief Метод сброса состояния чтения
 *
 */
void awh::codec::cef::Reader::reset() noexcept {
	// Сбрасываем текущее состояние чтения
	this->_state = state_t::HUNGRY;
	// Сбрасываем вид текущего события разбора
	this->_event = event_t::NONE;
	// Сбрасываем код ошибки последней операции разбора
	this->_error = error_t::NONE;
	// Сбрасываем положение обнаруженной ошибки
	this->_errorPosition = pos_t();
	// Сбрасываем положение начала текущего события
	this->_position = pos_t();
	// Выполняем очистку хранилища подаваемого текста
	this->_buffer.clear();
	// Сбрасываем смещение разбора в хранилище
	this->_offset = 0;
	// Сбрасываем смещение начала неразобранного остатка записи
	this->_record = 0;
	// Сбрасываем признак подачи последнего куска текста
	this->_end = false;
	// Сбрасываем этап разбора текущей записи
	this->_stage = stage_t::RECORD;
	// Сбрасываем номер поля заголовка
	this->_field = field_t::VERSION;
	// Сбрасываем указатель выдачи полей и пар
	this->_index = 0;
	// Выполняем очистку полей заголовка текущей записи
	this->_fields.clear();
	// Выполняем очистку пар расширения текущей записи
	this->_pairs.clear();
	// Сбрасываем номер редакции записи
	this->_version = 0;
	// Сбрасываем важность события записи
	this->_severity = 0;
	// Выполняем очистку имени ключа текущего события
	this->_key.clear();
	// Выполняем очистку значения текущего события
	this->_value.clear();
}

/**
 * @brief Метод передачи очередного куска исходного текста
 *
 * @param buffer буфер очередного куска исходного текста
 * @param size   размер буфера очередного куска исходного текста
 * @param end    признак того, что кусок является последним
 * @return       результат выполнения операции
 */
bool awh::codec::cef::Reader::feed(const void * buffer, const size_t size, const bool end) noexcept {
	// Если разбор уже прекращён ошибкой
	if(this->_state == state_t::FAILED)
		// Выводим отрицательный результат выполнения операции
		return false;
	// Если текст уже подан последним куском
	if(this->_end){
		// Выводим в лог сообщение о подаче текста после последнего куска
		this->_log->print("CEF text is fed after the last chunk", log_t::flag_t::WARNING);
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Если буфер куска передан
	if((buffer != nullptr) && (size > 0)){
		// Если хранилище разбора уплотнить возможно
		if(this->_offset >= COMPACT_THRESHOLD){
			// Выполняем удаление разобранного начала хранилища
			this->_buffer.erase(0, this->_offset);
			// Сбрасываем смещение разбора в хранилище
			this->_offset = 0;
		}
		// Добавляем поданный кусок текста в хранилище разбора
		this->_buffer.append(reinterpret_cast <const char *> (buffer), size);
	}
	// Запоминаем признак подачи последнего куска текста
	this->_end = end;
	// Устанавливаем состояние готовности разбора
	this->_state = state_t::HUNGRY;
	// Выводим положительный результат выполнения операции
	return true;
}

/**
 * @brief Метод передачи исходного текста целиком
 *
 * @param text исходный текст записей целиком
 * @return     результат выполнения операции
 */
bool awh::codec::cef::Reader::feed(const string_view text) noexcept {
	// Выполняем передачу текста единственным и последним куском
	return this->feed(text.data(), text.size(), true);
}

/**
 * @brief Метод разбора очередной записи целиком
 *
 * @param record текст записи целиком
 * @return       признак успешности разбора записи
 */
bool awh::codec::cef::Reader::prepare(const string_view record) noexcept {
	// Выполняем очистку полей заголовка текущей записи
	this->_fields.clear();
	// Выполняем очистку пар расширения текущей записи
	this->_pairs.clear();
	// Выполняем очистку приставки syslog текущей записи
	this->_value.clear();
	// Сбрасываем указатель выдачи полей и пар
	this->_index = 0;
	// Сбрасываем важность события записи
	this->_severity = 0;
	// Смещение слова «CEF:» от начала записи
	size_t begin = 0;
	// Если слово «CEF:» в записи не найдено
	if(!this->signature(record, begin))
		// Выводим отказ разбора отсутствием слова «CEF:»
		return this->fail(error_t::MISSING_SIGNATURE, this->_offset);
	// Если приставка syslog записи предшествует
	if(begin > 0){
		// Если признание приставки syslog выключено
		if(!this->_settings.syslog)
			// Выводим отказ разбора содержимым перед словом «CEF:»
			return this->fail(error_t::MISSING_SIGNATURE, this->_offset);
		// Запоминаем приставку syslog значением события
		this->_value.assign(record.begin(), record.begin() + begin);
		// Снимаем пробелы в конце приставки syslog
		while(!this->_value.empty() && ((this->_value.back() == ' ') || (this->_value.back() == '\t')))
			// Удаляем пробельный знак в конце приставки
			this->_value.pop_back();
		/**
		 * Если приставка syslog одними пробельными знаками была
		 *
		 * @details Приставка, от пробелов очищенная и пустой оказавшаяся, событием НЕ
		 * выдаётся вовсе: пустая приставка и отсутствие приставки в записи CEF
		 * неразличимы, и выдача события заводила бы узел, какому в собранной записи
		 * места нет - оборот терял бы его молча
		 *
		 * @note Найдено ворошителем 04.09.2026 расхождением деревьев после оборота
		 */
		if(this->_value.empty())
			// Сбрасываем смещение начала приставки: приставки в записи нет
			begin = 0;
	}
	// Получаем текст записи, приставкой не занятый
	const string_view body(record.data() + begin, record.size() - begin);
	// Смещение разбора полей заголовка от начала тела записи
	size_t offset = SIGNATURE.size();
	// Длина очередного поля заголовка до разделяющей черты
	size_t size = 0;
	/**
	 * Выполняем перебор всех полей заголовка записи
	 */
	for(uint32_t i = 0; i < HEADER_FIELDS; i++){
		// Получаем неразобранный остаток тела записи
		const string_view rest(body.data() + offset, body.size() - offset);
		// Если разделяющая черта за полем заголовка не найдена
		if(!this->bounds(rest, size))
			// Выводим отказ разбора неполнотой заголовка
			return this->fail(error_t::INCOMPLETE_HEADER, this->_offset + begin + offset);
		// Если длина поля заголовка превышает допустимую
		if(size > MAX_HEADER_FIELD)
			// Выводим отказ разбора превышением длины поля
			return this->fail(error_t::FIELD_TOO_LONG, this->_offset + begin + offset);
		// Создаём поле заголовка записи
		this->_fields.emplace_back();
		// Если снятие отмены знаков со значений включено
		if(this->_settings.unescape)
			// Выполняем снятие отмены знаков с поля заголовка
			this->unescape(string_view(rest.data(), size), area_t::HEADER, this->_fields.back());
		// Если снятие отмены знаков со значений выключено
		else this->_fields.back().assign(rest.data(), size);
		// Сдвигаем смещение разбора полей заголовка
		offset += (size + 1);
	}
	// Получаем текст номера редакции записи
	const string & version = this->_fields.front();
	// Если номер редакции записи пуст
	if(version.empty())
		// Выводим отказ разбора ошибочным номером редакции
		return this->fail(error_t::INVALID_VERSION, this->_offset + begin);
	// Сбрасываем номер редакции записи
	this->_version = 0;
	/**
	 * Выполняем перебор знаков номера редакции записи
	 */
	for(size_t i = 0; i < version.size(); i++){
		// Если знак номера редакции цифрой не является
		if((version[i] < '0') || (version[i] > '9'))
			// Выводим отказ разбора ошибочным номером редакции
			return this->fail(error_t::INVALID_VERSION, this->_offset + begin + SIGNATURE.size() + i);
		// Наращиваем номер редакции записи очередной цифрой
		this->_version = ((this->_version * 10) + static_cast <uint32_t> (version[i] - '0'));
	}
	// Получаем текст важности события записи
	const string & severity = this->_fields.back();
	/**
	 * Выполняем перебор знаков важности события записи
	 */
	for(size_t i = 0; i < severity.size(); i++){
		// Если знак важности события цифрой не является
		if((severity[i] < '0') || (severity[i] > '9')){
			// Сбрасываем важность события записи
			this->_severity = 0;
			// Выходим из цикла перебора: важность бывает записана и словом
			break;
		}
		// Наращиваем важность события очередной цифрой
		this->_severity = ((this->_severity * 10) + static_cast <uint32_t> (severity[i] - '0'));
	}
	// Если важность события за допустимый предел выходит
	if(this->_severity > MAX_SEVERITY){
		// Сбрасываем важность события записи
		this->_severity = 0;
		// Если сличение со словарём ведётся строго
		if(this->_settings.mode == mode_t::STRONG)
			// Выводим отказ разбора ошибочной важностью события
			return this->fail(error_t::INVALID_SEVERITY, this->_offset + begin);
	}
	// Получаем текст расширения записи
	const string_view extension(body.data() + offset, body.size() - offset);
	// Если расширение записи не пусто
	if(!extension.empty()){
		// Выполняем разбор пар расширения ходом фреймворка
		this->_fmk->kv(0, extension, " ", [this](const uint64_t sid, const string_view key, const string_view value) noexcept -> void {
			// Помечаем опознаватель разбора неиспользуемым
			(void) sid;
			// Если количество пар расширения превышает допустимое
			if(this->_pairs.size() >= static_cast <size_t> (this->_settings.maxExtensions))
				// Выходим из функции обратного вызова
				return;
			// Создаём пару расширения записи
			this->_pairs.emplace_back();
			/**
			 * Если снятие отмены знаков со значений включено
			 *
			 * @details Отмена снимается и с ИМЕНИ ключа, а не с одного лишь значения:
			 * писатель имя ключа отменою знаков ограждает, и снятие её лишь со значения
			 * давало бы разбор, обратный записи не равный - имя росло бы косыми при
			 * всяком обороте. Найдено ворошителем 04.09.2026 расхождением деревьев:
			 * ключ «\,|rt\» после оборота обращался в «\\,|rt\\»
			 */
			if(this->_settings.unescape)
				// Выполняем снятие отмены знаков с имени ключа расширения
				this->unescape(key, area_t::EXTENSION, this->_pairs.back().first);
			// Если снятие отмены знаков со значений выключено
			else this->_pairs.back().first.assign(key.begin(), key.end());
			// Если снятие отмены знаков со значений включено
			if(this->_settings.unescape)
				// Выполняем снятие отмены знаков со значения расширения
				this->unescape(value, area_t::EXTENSION, this->_pairs.back().second);
			// Если снятие отмены знаков со значений выключено
			else this->_pairs.back().second.assign(value.begin(), value.end());
		});
		// Если количество пар расширения превышает допустимое
		if(this->_pairs.size() >= static_cast <size_t> (this->_settings.maxExtensions))
			// Выводим отказ разбора превышением количества пар
			return this->fail(error_t::OVERFLOW_LIMIT, this->_offset + begin + offset);
		/**
		 * Выполняем перебор всех разобранных пар расширения
		 */
		for(auto & pair : this->_pairs){
			// Если имя ключа расширения пусто
			if(pair.first.empty())
				// Выводим отказ разбора пустым именем ключа
				return this->fail(error_t::EMPTY_KEY, this->_offset + begin + offset);
			// Если длина имени ключа превышает допустимую
			if(pair.first.size() > MAX_NAME)
				// Выводим отказ разбора превышением длины имени
				return this->fail(error_t::NAME_TOO_LONG, this->_offset + begin + offset);
		}
	}
	// Устанавливаем этап выдачи приставки syslog, если она записи предшествует
	this->_stage = (begin > 0 ? stage_t::SYSLOG : stage_t::HEADER);
	// Выполняем определение положения начала записи
	this->place(this->_offset, this->_position);
	// Выводим признак успешности разбора записи
	return true;
}

/**
 * @brief Метод перехода к следующему событию разбора
 *
 * @return признак наличия очередного события разбора
 */
bool awh::codec::cef::Reader::next() noexcept {
	// Если разбор прекращён ошибкой либо доведён до конца
	if((this->_state == state_t::FAILED) || (this->_state == state_t::FINISHED))
		// Выводим отсутствие очередного события разбора
		return false;
	// Длина текущей записи без знака конца строки
	size_t length = 0;
	// Смещение начала следующей записи
	size_t next = 0;
	/**
	 * Выполняем разбор записей, пока они в хранилище отыскиваются
	 */
	while(true){
		/**
		 * Определяем этап разбора текущей записи
		 */
		switch(static_cast <uint8_t> (this->_stage)){
			// Если ведётся выдача приставки syslog
			case static_cast <uint8_t> (stage_t::SYSLOG): {
				// Устанавливаем этап выдачи полей заголовка
				this->_stage = stage_t::HEADER;
				// Выполняем очистку имени ключа текущего события
				this->_key.clear();
				// Устанавливаем вид текущего события приставкой syslog
				this->_event = event_t::SYSLOG;
				// Устанавливаем состояние готовности события
				this->_state = state_t::READY;
				// Выводим наличие очередного события разбора
				return true;
			}
			// Если ведётся выдача полей заголовка
			case static_cast <uint8_t> (stage_t::HEADER): {
				// Если все поля заголовка уже выданы
				if(this->_index >= this->_fields.size()){
					// Устанавливаем этап выдачи пар расширения
					this->_stage = stage_t::EXTENSION;
					// Сбрасываем указатель выдачи пар расширения
					this->_index = 0;
					// Продолжаем разбор следующим этапом
					continue;
				}
				// Устанавливаем поле заголовка текущего события
				this->_field = static_cast <field_t> (this->_index);
				// Запоминаем значение поля заголовка
				this->_value = this->_fields.at(this->_index);
				// Выполняем очистку имени ключа текущего события
				this->_key.clear();
				// Сдвигаем указатель выдачи полей заголовка
				this->_index++;
				// Устанавливаем вид текущего события полем заголовка
				this->_event = event_t::HEADER;
				// Устанавливаем состояние готовности события
				this->_state = state_t::READY;
				// Выводим наличие очередного события разбора
				return true;
			}
			// Если ведётся выдача пар расширения
			case static_cast <uint8_t> (stage_t::EXTENSION): {
				// Если все пары расширения уже выданы
				if(this->_index >= this->_pairs.size()){
					// Устанавливаем этап выдачи знака окончания записи
					this->_stage = stage_t::FINISH;
					// Продолжаем разбор следующим этапом
					continue;
				}
				// Запоминаем имя ключа пары расширения
				this->_key = this->_pairs.at(this->_index).first;
				// Запоминаем значение пары расширения
				this->_value = this->_pairs.at(this->_index).second;
				// Сдвигаем указатель выдачи пар расширения
				this->_index++;
				// Устанавливаем вид текущего события парой расширения
				this->_event = event_t::EXTENSION;
				// Устанавливаем состояние готовности события
				this->_state = state_t::READY;
				// Выводим наличие очередного события разбора
				return true;
			}
			// Если ведётся выдача знака окончания записи
			case static_cast <uint8_t> (stage_t::FINISH): {
				// Устанавливаем этап отыскания очередной записи
				this->_stage = stage_t::RECORD;
				// Выполняем очистку имени ключа текущего события
				this->_key.clear();
				// Выполняем очистку значения текущего события
				this->_value.clear();
				// Устанавливаем вид текущего события окончанием записи
				this->_event = event_t::RECORD;
				// Устанавливаем состояние готовности события
				this->_state = state_t::READY;
				// Выводим наличие очередного события разбора
				return true;
			}
		}
		// Если очередная запись целиком не найдена
		if(!this->measure(length, next)){
			// Если подан последний кусок текста
			if(this->_end){
				// Устанавливаем состояние окончания разбора
				this->_state = state_t::FINISHED;
				// Устанавливаем вид текущего события окончанием текста
				this->_event = event_t::FINISH;
				// Выполняем очистку имени ключа текущего события
				this->_key.clear();
				// Выполняем очистку значения текущего события
				this->_value.clear();
				/**
				 * Выводим наличие очередного события разбора
				 *
				 * @note Окончание текста выдаётся событием наравне с прочими, а не
				 *       остаётся признаком состояния: перебор ведётся циклом «покуда
				 *       next()», и событие, при отказе выставляемое, потребителю не
				 *       достаётся вовсе. Следующий же спрос отвечает отказом заставою
				 *       окончания разбора, оттого событие это выдаётся ровно однажды
				 *
				 */
				return true;
			// Если текст ещё не исчерпан
			} else this->_state = state_t::HUNGRY;
			// Выводим отсутствие очередного события разбора
			return false;
		}
		// Если длина записи превышает допустимую
		if(length > static_cast <size_t> (this->_settings.maxRecord))
			// Выводим отказ разбора превышением длины записи
			return this->fail(error_t::RECORD_TOO_LONG, this->_offset);
		// Получаем текст очередной записи целиком
		const string_view record(this->_buffer.data() + this->_offset, length);
		// Если запись пуста
		if(record.find_first_not_of(" \t") == string_view::npos){
			// Сдвигаем смещение разбора к следующей записи
			this->_offset = next;
			// Продолжаем разбор со следующей записи
			continue;
		}
		// Если разбор очередной записи отказом завершился
		if(!this->prepare(record))
			// Выводим отсутствие очередного события разбора
			return false;
		// Сдвигаем смещение разбора к следующей записи
		this->_offset = next;
	}
}

/**
 * @brief Метод получения текущего состояния чтения
 *
 * @return текущее состояние чтения записей
 */
awh::codec::cef::state_t awh::codec::cef::Reader::state() const noexcept {
	// Выводим текущее состояние чтения записей
	return this->_state;
}

/**
 * @brief Метод получения вида текущего события разбора
 *
 * @return вид текущего события разбора
 */
awh::codec::cef::event_t awh::codec::cef::Reader::event() const noexcept {
	// Выводим вид текущего события разбора
	return this->_event;
}

/**
 * @brief Метод получения кода ошибки разбора
 *
 * @return код ошибки последней операции разбора
 */
awh::codec::cef::error_t awh::codec::cef::Reader::error() const noexcept {
	// Выводим код ошибки последней операции разбора
	return this->_error;
}

/**
 * @brief Метод получения места обнаружения ошибки
 *
 * @return положение обнаруженной ошибки в исходном тексте
 */
const awh::codec::cef::pos_t & awh::codec::cef::Reader::errorPosition() const noexcept {
	// Выводим положение обнаруженной ошибки в исходном тексте
	return this->_errorPosition;
}

/**
 * @brief Метод получения места начала текущего события
 *
 * @return положение начала текущего события в исходном тексте
 */
const awh::codec::cef::pos_t & awh::codec::cef::Reader::position() const noexcept {
	// Выводим положение начала текущего события в исходном тексте
	return this->_position;
}

/**
 * @brief Метод получения поля заголовка текущего события
 *
 * @return поле заголовка, выдаваемое текущим событием
 */
awh::codec::cef::field_t awh::codec::cef::Reader::field() const noexcept {
	// Выводим поле заголовка, выдаваемое текущим событием
	return this->_field;
}

/**
 * @brief Метод получения имени ключа текущего события
 *
 * @return имя ключа, выдаваемое текущим событием
 */
const string & awh::codec::cef::Reader::key() const noexcept {
	// Выводим имя ключа, выдаваемое текущим событием
	return this->_key;
}

/**
 * @brief Метод получения значения текущего события
 *
 * @return значение, выдаваемое текущим событием
 */
const string & awh::codec::cef::Reader::value() const noexcept {
	// Выводим значение, выдаваемое текущим событием
	return this->_value;
}

/**
 * @brief Метод получения номера редакции текущей записи
 *
 * @return номер редакции записи
 */
uint32_t awh::codec::cef::Reader::version() const noexcept {
	// Выводим номер редакции записи
	return this->_version;
}

/**
 * @brief Метод получения важности события текущей записи
 *
 * @return важность события записи
 */
uint32_t awh::codec::cef::Reader::severity() const noexcept {
	// Выводим важность события записи
	return this->_severity;
}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::codec::cef::Reader::Reader(const fmk_t * fmk, const log_t * log) noexcept :
 _state(state_t::HUNGRY), _event(event_t::NONE), _error(error_t::NONE), _offset(0), _record(0),
 _end(false), _stage(stage_t::RECORD), _field(field_t::VERSION), _index(0), _version(0), _severity(0),
 _fmk(fmk), _log(log) {}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
