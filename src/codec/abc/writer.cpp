/**
 * @file writer.cpp
 * @date 2026-08-18
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
 * @brief Файл реализации сборки записи бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the assembling of a record of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/writer.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор настроек сборки записи
 *
 */
awh::codec::abc::Writer::Settings::Settings() noexcept :
 canonical(false), validate(true), duplicates(true), maxDepth(0) {}
/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Writer::Writer() noexcept :
 _error(error_t::NONE), _failed(false), _documents(0) {}
/**
 * @brief Метод сброса состояния сборки
 *
 */
void awh::codec::abc::Writer::reset() noexcept {
	// Выполняем сброс кода отказа сборки
	this->_error = error_t::NONE;
	// Выполняем очистку буфера собираемой записи
	this->_record.clear();
	// Выполняем очистку стека вместимых
	this->_stack.clear();
	// Выполняем сброс признака отказа сборки
	this->_failed = false;
	// Выполняем сброс количества собранных документов
	this->_documents = 0;
}
/**
 * @brief Метод объявления отказа сборки
 *
 * @param error код отказа сборки
 * @return      признак успешности сборки, всегда ложь
 *
 */
bool awh::codec::abc::Writer::fail(const error_t error) noexcept {
	// Выполняем установку кода отказа сборки
	this->_error = error;
	// Выполняем установку признака отказа сборки
	this->_failed = true;
	// Сообщаем, что сборка отвечена отказом
	return false;
}
/**
 * @brief Метод проверки места, куда укладывается значение
 *
 * @param container признак того, что укладывается вместимое
 * @return          признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::prepare(const bool container) noexcept {
	// Если сборка уже отвечена отказом
	if(this->_failed)
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Если стек вместимых пуст
	if(this->_stack.empty())
		// Сообщаем, что место укладки годно
		return true;
	// Выполняем получение верхнего звена стека вместимых
	const frame_t & frame = this->_stack.back();
	/**
	 * Если значение собирается кусками, всякое иное значение внутри него негодно:
	 * куски ложатся своим путём и сюда не приходят
	 */
	if(frame.segment != type_t::UNDEFINED)
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INVALID_SEGMENT);
	// Если ожидается имя поля отображения
	if(frame.mapping && frame.expectKey){
		// Если именем поля стоит вместимое
		if(container)
			// Выполняем объявление отказа сборки
			return this->fail(error_t::INVALID_KEY);
	}
	// Если значений вместимого уложено больше объявленного
	if(!frame.indefinite && (frame.remain == 0))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::CONTAINER_OVERFLOW);
	// Сообщаем, что место укладки годно
	return true;
}
/**
 * @brief Метод учёта уложенного значения
 *
 * @param start смещение начала записи уложенного значения
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::account(const size_t start) noexcept {
	// Если стек вместимых пуст, уложен целый документ
	if(this->_stack.empty()){
		// Выполняем учёт собранного документа
		this->_documents++;
		// Сообщаем, что сборка успешна
		return true;
	}
	// Выполняем получение верхнего звена стека вместимых
	frame_t & frame = this->_stack.back();
	// Если уложенное значение является именем поля отображения
	if(frame.mapping && frame.expectKey){
		// Выполняем получение длины записи уложенного имени поля
		const size_t length = (this->_record.size() - start);
		// Если имя поля в этом вместимом уже было
		if(frame.marked){
			// Выполняем получение длины записи имени поля, уложенного прежде
			const size_t previous = static_cast <size_t> (frame.key.length);
			// Выполняем получение длины сличаемой части записей имён
			const size_t shared = ((previous < length) ? previous : length);
			// Выполняем сличение записей имён по общей их части
			int32_t compare = ((shared > 0) ? ::memcmp(this->_record.data() + frame.key.offset,
			 this->_record.data() + start, shared) : 0);
			/**
			 * Если общая часть записей имён совпала, сличаем их по длине.
			 *
			 * Доводка эта есть сторож, а не случай: всякая запись самоограничена, ибо длина
			 * её стоит в самой метке, - оттого запись одного имени началом другого оказаться
			 * не может, и общая часть у разных имён расходится всегда. Оставлена она затем,
			 * что сличение обязано задавать полный порядок само по себе, не опираясь на
			 * устройство записи
			 */
			if(compare == 0)
				// Выполняем сличение записей имён по их длине
				compare = ((previous < length) ? -1 : ((previous > length) ? 1 : 0));
			// Если имена полей отображения совпали
			if((compare == 0) && this->_settings.duplicates)
				// Выполняем объявление отказа сборки
				return this->fail(error_t::DUPLICATE_KEY);
			// Если имена полей идут не по возрастанию при строгом виде записи
			if((compare >= 0) && this->_settings.canonical)
				// Выполняем объявление отказа сборки
				return this->fail(error_t::UNORDERED_KEY);
		}
		// Выполняем запоминание отрезка записи уложенного имени поля
		frame.key = span_t(static_cast <uint32_t> (start), static_cast <uint32_t> (length));
		// Выполняем установку признака того, что имя поля уже было
		frame.marked = true;
		// Выполняем снятие признака ожидания имени поля
		frame.expectKey = false;
		// Сообщаем, что сборка успешна: пара ещё не завершена
		return true;
	}
	// Если вместимое является отображением, следующим ожидается имя поля
	if(frame.mapping)
		// Выполняем установку признака ожидания имени поля
		frame.expectKey = true;
	// Если длина вместимого объявлена
	if(!frame.indefinite)
		// Выполняем учёт значения вместимого
		frame.remain--;
	// Сообщаем, что сборка успешна
	return true;
}
/**
 * @brief Метод укладки начала вместимого
 *
 * @param mapping    признак того, что вместимое является отображением
 * @param count      количество значений вместимого
 * @param indefinite признак неопределённой длины вместимого
 * @return           признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::open(const bool mapping, const uint64_t count, const bool indefinite) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(true))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Если неопределённая длина уложена при строгом виде записи
	if(indefinite && this->_settings.canonical)
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INDEFINITE_REFUSED);
	// Выполняем получение предела глубины вложенности
	const uint32_t limit = ((this->_settings.maxDepth > 0) ?
	 ((this->_settings.maxDepth < MAX_DEPTH) ? this->_settings.maxDepth : MAX_DEPTH) : MAX_DEPTH);
	// Если глубина вложенности превышает допустимую
	if(static_cast <uint32_t> (this->_stack.size() + 1) > limit)
		// Выполняем объявление отказа сборки
		return this->fail(error_t::DEPTH_EXCEEDED);
	// Выполняем получение крупного вида укладываемого вместимого
	const group_t group = (mapping ? group_t::MAP : group_t::ARRAY);
	// Если длина вместимого неопределённая
	if(indefinite)
		// Выполняем укладку метки неопределённой длины
		abc::mark(this->_record, group, static_cast <uint8_t> (single_t::BREAK));
	// Выполняем укладку метки вместе с объявленной длиной
	else abc::put(this->_record, group, count);
	// Заводимое звено стека вместимых
	frame_t frame;
	// Выполняем установку признака отображения
	frame.mapping = mapping;
	// Выполняем установку признака неопределённой длины
	frame.indefinite = indefinite;
	// Выполняем установку признака ожидания имени поля
	frame.expectKey = mapping;
	// Выполняем установку количества значений вместимого
	frame.remain = (indefinite ? 0 : count);
	// Выполняем добавление звена в стек вместимых
	this->_stack.push_back(frame);
	// Сообщаем, что сборка успешна
	return true;
}
/**
 * @brief Метод укладки конца вместимого
 *
 * @param mapping признак того, что вместимое является отображением
 * @return        признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::close(const bool mapping) noexcept {
	// Если сборка уже отвечена отказом
	if(this->_failed)
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Если закрывать нечего
	if(this->_stack.empty())
		// Выполняем объявление отказа сборки
		return this->fail(error_t::UNBALANCED_CONTAINER);
	// Выполняем получение верхнего звена стека вместимых
	const frame_t & frame = this->_stack.back();
	// Если закрывается вместимое иного вида, нежели открытое
	if(frame.mapping != mapping)
		// Выполняем объявление отказа сборки
		return this->fail(error_t::UNBALANCED_CONTAINER);
	// Если отображение закрывается на имени поля
	if(frame.mapping && !frame.expectKey)
		// Выполняем объявление отказа сборки
		return this->fail(error_t::MISSING_VALUE);
	// Если значений вместимого уложено меньше объявленного
	if(!frame.indefinite && (frame.remain > 0))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::UNBALANCED_CONTAINER);
	// Признак неопределённой длины закрываемого вместимого
	const bool indefinite = frame.indefinite;
	// Выполняем снятие звена со стека вместимых
	this->_stack.pop_back();
	// Выполняем получение смещения начала записи закрытого вместимого
	const size_t start = this->_record.size();
	// Если длина вместимого неопределённая
	if(indefinite)
		// Выполняем укладку конца вместимого
		abc::mark(this->_record, group_t::SINGLE, static_cast <uint8_t> (single_t::BREAK));
	// Выполняем учёт закрытого вместимого значением вместившего
	return this->account(start);
}
/**
 * @brief Метод укладки пустого значения
 *
 * @return признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::nul() noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку пустого значения
	abc::mark(this->_record, group_t::SINGLE, static_cast <uint8_t> (single_t::NUL));
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки логического значения
 *
 * @param value укладываемое значение
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::boolean(const bool value) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку логического значения
	abc::mark(this->_record, group_t::SINGLE,
	 static_cast <uint8_t> (value ? single_t::TRUE : single_t::FALSE));
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки целого числа со знаком
 *
 * @param value укладываемое значение
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::number(const int64_t value) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку целого числа со знаком
	abc::integer(this->_record, value);
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки целого числа без знака
 *
 * @param value укладываемое значение
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::number(const uint64_t value) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку целого числа без знака
	abc::put(this->_record, group_t::UNSIGNED, value);
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки дробного числа двойной точности
 *
 * @param value укладываемое значение
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::number(const double value) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку дробного числа двойной точности
	abc::real(this->_record, value);
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки дробного числа одинарной точности
 *
 * @param value укладываемое значение
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::number(const float value) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку дробного числа одинарной точности
	abc::real(this->_record, value);
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки строки
 *
 * @param value укладываемое значение
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::text(const string_view value) noexcept {
	/**
	 * Выполняем получение признака куска собираемого значения: внутри строки,
	 * собираемой кусками, всякая строка есть кусок её, а не отдельное значение
	 */
	const bool chunk = (!this->_stack.empty() && (this->_stack.back().segment != type_t::UNDEFINED));
	/**
	 * Если кусок ложится внутрь двоичных данных, собираемых кусками
	 */
	if(chunk && (this->_stack.back().segment != type_t::STRING))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INVALID_SEGMENT);
	// Если место укладки значения негодно
	if(!chunk && !this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Если строку следует проверить на соответствие кодировке
	if(this->_settings.validate){
		// Смещение первой негодной последовательности
		size_t position = 0;
		// Если строка кодировке UTF-8 не отвечает
		if(!abc::utf8(reinterpret_cast <const uint8_t *> (value.data()), value.size(), position))
			// Выполняем объявление отказа сборки
			return this->fail(error_t::INVALID_ENCODING);
	}
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку метки строки вместе с её длиной
	abc::put(this->_record, group_t::STRING, static_cast <uint64_t> (value.size()));
	// Если строка не пуста
	if(!value.empty()){
		// Выполняем получение указателя на октеты строки
		const uint8_t * octets = reinterpret_cast <const uint8_t *> (value.data());
		// Выполняем укладку октетов строки
		this->_record.insert(this->_record.end(), octets, octets + value.size());
	}
	/**
	 * Если уложен кусок собираемого значения, учитывать его вместившим нельзя:
	 * кусок есть часть значения, а не значение
	 */
	if(chunk)
		// Сообщаем, что укладка куска успешна
		return true;
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки двоичных данных
 *
 * @param buffer буфер укладываемых данных
 * @param size   размер укладываемых данных в октетах
 * @return       признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::blob(const void * buffer, const size_t size) noexcept {
	// Выполняем получение признака куска собираемого значения
	const bool chunk = (!this->_stack.empty() && (this->_stack.back().segment != type_t::UNDEFINED));
	/**
	 * Если кусок ложится внутрь строки, собираемой кусками
	 */
	if(chunk && (this->_stack.back().segment != type_t::BLOB))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INVALID_SEGMENT);
	// Если место укладки значения негодно
	if(!chunk && !this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Если буфер укладываемых данных не существует, а октеты объявлены
	if((buffer == nullptr) && (size > 0))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INTERNAL);
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку метки двоичных данных вместе с их длиной
	abc::put(this->_record, group_t::BLOB, static_cast <uint64_t> (size));
	// Если укладываемые данные не пусты
	if(size > 0){
		// Выполняем получение указателя на октеты данных
		const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
		// Выполняем укладку октетов данных
		this->_record.insert(this->_record.end(), octets, octets + size);
	}
	/**
	 * Если уложен кусок собираемого значения, учитывать его вместившим нельзя
	 */
	if(chunk)
		// Сообщаем, что укладка куска успешна
		return true;
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки отметки времени
 *
 * @param value укладываемое значение
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::timestamp(const int64_t value) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку метки отметки времени
	abc::mark(this->_record, group_t::SINGLE, static_cast <uint8_t> (single_t::TIME));
	// Выполняем укладку значения отметки времени
	abc::fixed(this->_record, static_cast <uint64_t> (value), static_cast <uint8_t> (TIME_WIDTH));
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки опознавателя
 *
 * @param buffer буфер укладываемого опознавателя
 * @param size   размер укладываемого опознавателя в октетах
 * @return       признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::uuid(const void * buffer, const size_t size) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Если буфер укладываемого опознавателя не существует
	if(buffer == nullptr)
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INTERNAL);
	// Если размер укладываемого опознавателя недопустим
	if(size != UUID_WIDTH)
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INVALID_LENGTH);
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку метки опознавателя
	abc::mark(this->_record, group_t::SINGLE, static_cast <uint8_t> (single_t::UUID));
	// Выполняем получение указателя на октеты опознавателя
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	// Выполняем укладку октетов опознавателя
	this->_record.insert(this->_record.end(), octets, octets + size);
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод укладки целого числа неограниченной ширины
 *
 * @param buffer   буфер октетов величины
 * @param size     размер октетов величины
 * @param negative признак того, что число меньше нуля
 * @return         признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::bignum(const void * buffer, const size_t size, const bool negative) noexcept {
	// Выполняем укладку числа неограниченной ширины без десятичного порядка
	return this->decimal(buffer, size, negative, 0);
}
/**
 * @brief Метод укладки десятичного числа
 *
 * @param buffer   буфер октетов величины
 * @param size     размер октетов величины
 * @param negative признак того, что число меньше нуля
 * @param exponent десятичный порядок величины
 * @return         признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::decimal(const void * buffer, const size_t size, const bool negative, const int64_t exponent) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Если буфер октетов величины не существует, а октеты объявлены
	if((buffer == nullptr) && (size > 0))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INTERNAL);
	// Выполняем получение указателя на октеты величины
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	// Если старший октет величины нулевой
	if((size > 0) && (octets[size - 1] == 0))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INVALID_BIGNUM);
	// Если величина объявлена отрицательным нулём
	if(negative && (size == 0))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INVALID_BIGNUM);
	// Признак того, что число является десятичным
	const bool fraction = (exponent != 0);
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку метки расширения
	abc::mark(this->_record, group_t::EXTEND,
	 static_cast <uint8_t> (fraction ? extend_t::DECIMAL : extend_t::BIGNUM));
	// Если число является десятичным
	if(fraction)
		// Выполняем укладку десятичного порядка величины
		abc::integer(this->_record, exponent);
	// Выполняем укладку длины октетов величины
	abc::put(this->_record, group_t::UNSIGNED, static_cast <uint64_t> (size));
	// Выполняем укладку знака величины
	this->_record.push_back(static_cast <uint8_t> (negative ? 1 : 0));
	// Если октеты величины не пусты
	if(size > 0)
		// Выполняем укладку октетов величины
		this->_record.insert(this->_record.end(), octets, octets + size);
	// Выполняем учёт уложенного значения
	return this->account(start);
}
/**
 * @brief Метод начала значения, собираемого кусками
 *
 * @param string признак того, что собирается строка, а не двоичные данные
 * @return       признак успешности укладки
 *
 */
bool awh::codec::abc::Writer::segment(const bool string) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	/**
	 * Если вид записи строгий, неопределённая длина отвергается: длина обязана быть
	 * объявлена, иначе запись одного и того же значения выйдет разной
	 */
	if(this->_settings.canonical)
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INDEFINITE_REFUSED);
	/**
	 * Если значение стоит именем поля отображения: имя обязано быть цельным, иначе
	 * сличение имён по записи станет невозможным
	 */
	if(!this->_stack.empty()){
		// Выполняем получение верхнего звена стека вместимых
		const frame_t & frame = this->_stack.back();
		// Если ожидается имя поля отображения
		if(frame.mapping && frame.expectKey)
			// Выполняем объявление отказа сборки
			return this->fail(error_t::INVALID_KEY);
	}
	// Выполняем получение предела глубины вложенности
	const uint32_t limit = ((this->_settings.maxDepth > 0) ?
	 ((this->_settings.maxDepth < MAX_DEPTH) ? this->_settings.maxDepth : MAX_DEPTH) : MAX_DEPTH);
	// Если глубина вложенности превышает допустимую
	if(static_cast <uint32_t> (this->_stack.size() + 1) > limit)
		// Выполняем объявление отказа сборки
		return this->fail(error_t::DEPTH_EXCEEDED);
	// Выполняем укладку метки значения неопределённой длины
	abc::mark(this->_record, (string ? group_t::STRING : group_t::BLOB),
	 static_cast <uint8_t> (single_t::BREAK));
	// Заводимое звено стека вместимых
	frame_t frame;
	// Выполняем установку вида значения, собираемого кусками
	frame.segment = (string ? type_t::STRING : type_t::BLOB);
	// Выполняем установку признака неопределённой длины
	frame.indefinite = true;
	// Выполняем добавление звена в стек вместимых
	this->_stack.push_back(frame);
	// Сообщаем, что укладка успешна
	return true;
}
/**
 * @brief Метод конца значения, собираемого кусками
 *
 * @param string признак того, что собирается строка, а не двоичные данные
 * @return       признак успешности укладки
 *
 */
bool awh::codec::abc::Writer::segmentEnd(const bool string) noexcept {
	// Если сборка уже отвечена отказом
	if(this->_failed)
		// Сообщаем, что сборка отвечена отказом
		return false;
	/**
	 * Если стек вместимых пуст либо закрывается не значение, собираемое кусками
	 */
	if(this->_stack.empty() || (this->_stack.back().segment !=
	 (string ? type_t::STRING : type_t::BLOB)))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::UNBALANCED_CONTAINER);
	// Выполняем снятие звена со стека вместимых
	this->_stack.pop_back();
	// Выполняем получение смещения начала записи конца значения
	const size_t start = this->_record.size();
	// Выполняем укладку конца значения, собираемого кусками
	abc::mark(this->_record, group_t::SINGLE, static_cast <uint8_t> (single_t::BREAK));
	/**
	 * Выполняем учёт уложенного значения: значение, собранное кусками, есть одно
	 * значение вместившего, а не череда их
	 */
	return this->account(start);
}
/**
 * @brief Метод начала строки, собираемой кусками
 *
 * @return признак успешности укладки
 *
 */
bool awh::codec::abc::Writer::textBegin() noexcept {
	// Выводим результат начала строки, собираемой кусками
	return this->segment(true);
}
/**
 * @brief Метод конца строки, собираемой кусками
 *
 * @return признак успешности укладки
 *
 */
bool awh::codec::abc::Writer::textEnd() noexcept {
	// Выводим результат конца строки, собираемой кусками
	return this->segmentEnd(true);
}
/**
 * @brief Метод начала двоичных данных, собираемых кусками
 *
 * @return признак успешности укладки
 *
 */
bool awh::codec::abc::Writer::blobBegin() noexcept {
	// Выводим результат начала двоичных данных, собираемых кусками
	return this->segment(false);
}
/**
 * @brief Метод конца двоичных данных, собираемых кусками
 *
 * @return признак успешности укладки
 *
 */
bool awh::codec::abc::Writer::blobEnd() noexcept {
	// Выводим результат конца двоичных данных, собираемых кусками
	return this->segmentEnd(false);
}
/**
 * @brief Метод укладки начала массива объявленной длины
 *
 * @param count количество значений массива
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::arrayBegin(const uint64_t count) noexcept {
	// Выполняем укладку начала массива объявленной длины
	return this->open(false, count, false);
}
/**
 * @brief Метод укладки начала массива неопределённой длины
 *
 * @return признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::arrayBegin() noexcept {
	// Выполняем укладку начала массива неопределённой длины
	return this->open(false, 0, true);
}
/**
 * @brief Метод укладки конца массива
 *
 * @return признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::arrayEnd() noexcept {
	// Выполняем укладку конца массива
	return this->close(false);
}
/**
 * @brief Метод укладки начала отображения объявленной длины
 *
 * @param count количество пар отображения
 * @return      признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::mapBegin(const uint64_t count) noexcept {
	// Выполняем укладку начала отображения объявленной длины
	return this->open(true, count, false);
}
/**
 * @brief Метод укладки начала отображения неопределённой длины
 *
 * @return признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::mapBegin() noexcept {
	// Выполняем укладку начала отображения неопределённой длины
	return this->open(true, 0, true);
}
/**
 * @brief Метод укладки конца отображения
 *
 * @return признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::mapEnd() noexcept {
	// Выполняем укладку конца отображения
	return this->close(true);
}
/**
 * @brief Метод проверки завершённости собранной записи
 *
 * @return признак завершённости собранной записи
 *
 */
bool awh::codec::abc::Writer::complete() const noexcept {
	// Выводим признак завершённости собранной записи
	return (!this->_failed && this->_stack.empty() && (this->_documents > 0));
}
/**
 * @brief Метод извлечения собранной записи
 *
 * @return собранная запись
 *
 */
const vector <uint8_t> & awh::codec::abc::Writer::record() const noexcept {
	// Выводим собранную запись
	return this->_record;
}
/**
 * @brief Метод извлечения кода отказа сборки
 *
 * @return код отказа сборки
 *
 */
awh::codec::abc::error_t awh::codec::abc::Writer::error() const noexcept {
	// Выводим код отказа сборки
	return this->_error;
}
/**
 * @brief Метод извлечения глубины вложенности сборки
 *
 * @return глубина вложенности сборки
 *
 */
uint32_t awh::codec::abc::Writer::depth() const noexcept {
	// Выводим глубину вложенности сборки
	return static_cast <uint32_t> (this->_stack.size());
}
/**
 * @brief Метод извлечения настроек сборки записи
 *
 * @return настройки сборки записи
 *
 */
const awh::codec::abc::Writer::settings_t & awh::codec::abc::Writer::settings() const noexcept {
	// Выводим настройки сборки записи
	return this->_settings;
}
/**
 * @brief Метод установки настроек сборки записи
 *
 * @param settings устанавливаемые настройки сборки записи
 *
 */
void awh::codec::abc::Writer::settings(const settings_t & settings) noexcept {
	// Выполняем установку настроек сборки записи
	this->_settings = settings;
}
