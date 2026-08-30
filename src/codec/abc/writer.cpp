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
 canonical(false), validate(true), duplicates(true), maxDepth(0), reference(0), spanned(0) {}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 *
 */
awh::codec::abc::Writer::Writer(const log_t * log) noexcept :
 _error(error_t::NONE), _failed(false), _documents(0), _log(log) {}
/**
 * @brief Метод сброса состояния сборки
 *
 */
void awh::codec::abc::Writer::reset() noexcept {
	// Выполняем сброс кода отказа сборки
	this->_error = error_t::NONE;
	// Выполняем очистку буфера собираемой записи
	this->_record.clear();
	// Выполняем очистку врезок чужого содержимого
	this->_cuts.clear();
	// Выполняем очистку стека вместимых
	this->_stack.clear();
	// Выполняем очистку отрезков записей имён полей вместимых
	this->_keys.clear();
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
	/**
	 * Если объект логирования отдан, доносим об отказе сборки.
	 *
	 * Донесение идёт отсюда, из единственного места объявления отказа: сборка
	 * отвечает отказом множеством путей, и запись в каждом из них разошлась бы с
	 * прочими
	 */
	/**
	 * @warning Сброс кода отказа сюда НЕ идёт: воронка эта объявляет отказ, а сброс
	 *          лишь снимает прежний, и донесение о нём наполняло бы журнал записями
	 *          «no error» на всякий успешный вызов. Проверено на себе
	 */
	if((error != error_t::NONE) && (this->_log != nullptr)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("ABC: %s", __PRETTY_FUNCTION__,
			 make_tuple(static_cast <uint16_t> (error), this->_record.size()),
			 log_t::flag_t::WARNING, abc::message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("ABC: %s", log_t::flag_t::WARNING, abc::message(error));
		#endif
	}
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
 * @brief Метод укладки октетов содержимого значения
 *
 * @details Содержимое, чей размер порог укладки ссылкой превысил, ложится врезкой,
 * а прочее копируется в буфер собираемой записи
 *
 * @param buffer буфер укладываемых октетов
 * @param size   размер укладываемых октетов
 * @param copy   признак того, что копировать содержимое обязательно
 *
 */
void awh::codec::abc::Writer::content(const void * buffer, const size_t size, const bool copy) noexcept {
	// Если укладываемое содержимое пусто
	if((buffer == nullptr) || (size == 0))
		// Выходим из функции
		return;
	// Выполняем получение указателя на октеты содержимого
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	/**
	 * Если содержимое следует уложить ссылкой. Порог, равный нулю, укладку ссылкой
	 * снимает вовсе: настройка эта по умолчанию не впряжена
	 */
	if(!copy && (this->_settings.reference > 0) && (size >= this->_settings.reference)){
		// Выполняем укладку врезки чужого содержимого
		this->_cuts.push_back(cut_t(this->_record.size(), buffer, size));
		// Выходим из функции
		return;
	}
	// Выполняем укладку октетов содержимого в буфер собираемой записи
	this->_record.insert(this->_record.end(), octets, octets + size);
}
/**
 * @brief Метод вклейки содержимого, уложенного ссылкой
 *
 */
void awh::codec::abc::Writer::flatten() const noexcept {
	// Если врезок чужого содержимого нет
	if(this->_cuts.empty())
		// Выходим из функции
		return;
	// Буфер цельной записи
	vector <uint8_t> result;
	// Выполняем резервирование памяти под цельную запись
	result.reserve(this->length());
	// Смещение вклеенной части буфера собираемой записи
	size_t offset = 0;
	// Выполняем перебор врезок чужого содержимого
	for(auto & cut : this->_cuts){
		// Выполняем вклейку части буфера, стоящей перед врезкой
		result.insert(result.end(), this->_record.begin() + static_cast <ptrdiff_t> (offset),
		 this->_record.begin() + static_cast <ptrdiff_t> (cut.offset));
		// Выполняем получение указателя на октеты врезки
		const uint8_t * octets = reinterpret_cast <const uint8_t *> (cut.buffer);
		// Выполняем вклейку октетов врезки
		result.insert(result.end(), octets, octets + cut.size);
		// Выполняем сдвиг смещения вклеенной части буфера
		offset = cut.offset;
	}
	// Выполняем вклейку остатка буфера собираемой записи
	result.insert(result.end(), this->_record.begin() + static_cast <ptrdiff_t> (offset), this->_record.end());
	/**
	 * Выполняем сдвиг отрезков имён полей, уложенных прежде. Смещения их отсчитаны от
	 * начала буфера, и вклейка сдвинула бы их молча, обратив сличение имён в ложь
	 */
	for(auto & frame : this->_stack){
		// Если имени поля в этом вместимом ещё не было
		if(!frame.marked)
			// Переходим к следующему звену стека
			continue;
		// Размер содержимого, вклеенного перед отрезком имени поля
		size_t shift = 0;
		// Выполняем перебор врезок чужого содержимого
		for(auto & cut : this->_cuts){
			// Если врезка стоит после отрезка имени поля
			if(cut.offset > static_cast <size_t> (frame.key.offset))
				// Выполняем прекращение перебора врезок
				break;
			// Выполняем учёт размера вклеенного содержимого
			shift += cut.size;
		}
		// Выполняем сдвиг отрезка записи имени поля
		frame.key.offset += static_cast <uint32_t> (shift);
	}
	/**
	 * Выполняем сдвиг отрезков записей прежних имён полей всех вместимых стека
	 *
	 * @note Сдвиг у всякого имени свой: врезка, вставшая после имени, его не двигает
	 */
	for(auto & key : this->_keys){
		// Размер содержимого, вклеенного перед отрезком имени поля
		size_t own = 0;
		/**
		 * Выполняем перебор врезок чужого содержимого
		 */
		for(auto & cut : this->_cuts){
			// Если врезка стоит после отрезка имени поля
			if(cut.offset > static_cast <size_t> (key.offset))
				// Выполняем прекращение перебора врезок
				break;
			// Выполняем учёт размера вклеенного содержимого
			own += cut.size;
		}
		// Выполняем сдвиг отрезка записи имени поля
		key.offset += static_cast <uint32_t> (own);
	}
	// Выполняем замену буфера собираемой записи цельным
	this->_record.swap(result);
	// Выполняем очистку врезок чужого содержимого
	this->_cuts.clear();
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
		/**
		 * Если запись выросла шире отрезка, каким запоминается имя поля
		 *
		 * @details Отрезок имени несёт смещение и длину ТРИДЦАТЬЮ ДВУМЯ разрядами, а запись
		 *          растёт без предела. Сличение имени НЫНЕШНЕГО шло неусечённым смещением и
		 *          оттого было верно, а вот ЗАПОМИНАЛОСЬ имя усечённым - и всякое следующее
		 *          сличалось с завёрнутым местом
		 *
		 * @note Замерено щупом 30.08.2026, а не выведено рассуждением: отображение, где имя
		 *       «имя» уложено на месте 4.25 ГиБ, принимало ТО ЖЕ имя вторично - «повтор имени
		 *       за пределом 4 ГиБ ПРИНЯТ, код: no error». Сборщик тем самым нарушал
		 *       СОБСТВЕННОЕ умолчание, выдавая запись, какую свой же разбор отвергает
		 *
		 * @note Мерою взята `length()`, а не размер буфера: содержимое, уложенное ссылкой,
		 *       вклеивается позже и сдвигает отрезки имён, стоящих ЗА врезкой. Врезки,
		 *       стоящие перед именем, к мигу его укладки уже учтены длиною, а стоящие после
		 *       имени его не двигают вовсе - оттого мера эта точна, а не с запасом
		 *
		 * @note Сторож равняется на три таких же, стоящих в кодеке: обёртка записи кадром,
		 *       хранилище дерева документа и отрезок события у разбора. Здесь четвёртый
		 *       случай одного вида
		 */
		if(this->length() > static_cast <size_t> (numeric_limits <uint32_t>::max()))
			// Выполняем объявление отказа сборки
			return this->fail(error_t::INVALID_LENGTH);
		/**
		 * Если повторы отвергаются вне строгого вида, сличаем имя со ВСЕМИ прежними
		 *
		 * @note У строгого вида перечень не нужен: имена там идут по возрастанию, и сличения
		 *       с предыдущим довольно - повтор встал бы рядом. Вне его повтор бывает через
		 *       любое число полей, и предыдущего мало
		 */
		if(this->_settings.duplicates && !this->_settings.canonical){
			/**
			 * Выполняем перебор отрезков записей прежних имён полей вместимого
			 */
			for(size_t i = frame.base; i < this->_keys.size(); i++){
				// Выполняем получение отрезка записи прежнего имени поля
				const span_t & key = this->_keys.at(i);
				// Если длины записей имён не совпадают, имена различны заведомо
				if(static_cast <size_t> (key.length) != length)
					// Переходим к следующему отрезку записи имени поля
					continue;
				// Если записи имён совпали октет в октет
				if((length == 0) || (::memcmp(this->_record.data() + key.offset,
				 this->_record.data() + start, length) == 0))
					// Выполняем объявление отказа сборки
					return this->fail(error_t::DUPLICATE_KEY);
			}
			// Выполняем запоминание отрезка записи уложенного имени поля
			this->_keys.push_back(span_t(static_cast <uint32_t> (start), static_cast <uint32_t> (length)));
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
	// Смещение отведённого места записи размаха вместимого
	size_t spanned = 0;
	/**
	 * Если размах вместимого объявляется настройкою
	 *
	 * @note Место под размах отводится ЗДЕСЬ и заполняется при закрытии: размах в
	 *       октетах при открытии ещё не известен, а сдвигать уложенное следом нельзя
	 */
	if(!indefinite && (this->_settings.spanned > 0) && (count >= this->_settings.spanned)){
		// Выполняем укладку метки вместимого с объявленным размахом
		abc::mark(this->_record, group_t::EXTEND, static_cast <uint8_t> (extend_t::SPANNED));
		// Запоминаем смещение отведённого места записи размаха
		spanned = this->_record.size();
		// Выполняем отведение места под запись размаха вместимого
		this->_record.resize(this->_record.size() + SPAN_LENGTH, 0);
	}
	// Если длина вместимого неопределённая
	if(indefinite)
		// Выполняем укладку метки неопределённой длины
		abc::mark(this->_record, group, static_cast <uint8_t> (single_t::BREAK));
	// Выполняем укладку метки вместе с объявленной длиной
	else abc::put(this->_record, group, count);
	// Заводимое звено стека вместимых
	frame_t frame;
	// Выполняем установку смещения отведённого места записи размаха
	frame.spanned = spanned;
	// Выполняем установку начала части перечня отрезков имён полей вместимого
	frame.base = this->_keys.size();
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
	// Смещение отведённого места записи размаха закрываемого вместимого
	const size_t spanned = frame.spanned;
	/**
	 * Выполняем усечение перечня отрезков имён полей до начала части снимаемого звена
	 *
	 * @note Усечение вместимости не отнимает: перечень держит её до самой очистки сборки
	 */
	this->_keys.resize(this->_stack.back().base);
	// Выполняем снятие звена со стека вместимых
	this->_stack.pop_back();
	// Выполняем получение смещения начала записи закрытого вместимого
	const size_t start = this->_record.size();
	// Если длина вместимого неопределённая
	if(indefinite)
		// Выполняем укладку конца вместимого
		abc::mark(this->_record, group_t::SINGLE, static_cast <uint8_t> (single_t::BREAK));
	/**
	 * Если размах вместимого объявлен, заполняем отведённое под него место
	 *
	 * @note Размах считается от конца записи размаха до конца вместимого: чтение,
	 *       сняв размах, прибавляет его к своему месту и оказывается за вместимым
	 */
	if(spanned > 0){
		// Размер содержимого, уложенного ссылкой внутри закрытого вместимого
		size_t referenced = 0;
		/**
		 * Выполняем перебор врезок чужого содержимого
		 *
		 * @note Содержимое, уложенное ссылкой, во вместилище сборки не лежит и вклеивается
		 *       лишь выдачей записи - а размах объявлен в октетах ГОТОВОЙ записи. Не считая
		 *       врезок, размах выходил короче истинного, и своё же чтение отвечало отказом
		 */
		for(auto & cut : this->_cuts){
			// Если врезка стоит внутри закрытого вместимого
			if(cut.offset >= (spanned + SPAN_LENGTH))
				// Выполняем учёт размера вклеиваемого содержимого
				referenced += cut.size;
		}
		// Выполняем вычисление размаха закрытого вместимого
		const uint64_t width = static_cast <uint64_t> ((this->_record.size() + referenced) - (spanned + SPAN_LENGTH));
		// Выполняем перебор всех октетов записи размаха
		for(size_t i = 0; i < SPAN_LENGTH; i++)
			// Выполняем укладку очередного октета размаха
			this->_record.at(spanned + i) = static_cast <uint8_t> ((width >> (i * 8)) & 0xFF);
	}
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
	/**
	 * Выполняем получение признака имени поля отображения: имя копируется всегда,
	 * ибо сличается оно отрезком в буфере собираемой записи, а содержимое, уложенное
	 * ссылкой, отрезка в нём не имеет вовсе
	 */
	const bool key = (!this->_stack.empty() && this->_stack.back().mapping && this->_stack.back().expectKey);
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку метки строки вместе с её длиной
	abc::put(this->_record, group_t::STRING, static_cast <uint64_t> (value.size()));
	// Выполняем укладку октетов строки
	this->content(value.data(), value.size(), key);
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
	// Выполняем получение признака имени поля отображения
	const bool key = (!this->_stack.empty() && this->_stack.back().mapping && this->_stack.back().expectKey);
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку метки двоичных данных вместе с их длиной
	abc::put(this->_record, group_t::BLOB, static_cast <uint64_t> (size));
	// Выполняем укладку октетов данных
	this->content(buffer, size, key);
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
 * @brief Метод укладки открытого расширения
 *
 * @param subtype номер подвида расширения, заведённый потребителем
 * @param buffer  буфер октетов расширения
 * @param size    размер октетов расширения
 * @return        признак успешности сборки
 *
 */
bool awh::codec::abc::Writer::custom(const uint64_t subtype, const void * buffer, const size_t size) noexcept {
	// Если место укладки значения негодно
	if(!this->prepare(false))
		// Сообщаем, что сборка отвечена отказом
		return false;
	// Если буфер октетов расширения не существует, а октеты объявлены
	if((buffer == nullptr) && (size > 0))
		// Выполняем объявление отказа сборки
		return this->fail(error_t::INTERNAL);
	// Выполняем получение признака имени поля отображения
	const bool key = (!this->_stack.empty() && this->_stack.back().mapping && this->_stack.back().expectKey);
	// Выполняем получение смещения начала записи значения
	const size_t start = this->_record.size();
	// Выполняем укладку метки открытого расширения
	abc::mark(this->_record, group_t::EXTEND, static_cast <uint8_t> (extend_t::CUSTOM));
	// Выполняем укладку номера подвида расширения
	abc::put(this->_record, group_t::UNSIGNED, subtype);
	// Выполняем укладку длины октетов расширения
	abc::put(this->_record, group_t::UNSIGNED, static_cast <uint64_t> (size));
	// Выполняем укладку октетов расширения
	this->content(buffer, size, key);
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
	// Выполняем установку начала части перечня отрезков имён полей вместимого
	frame.base = this->_keys.size();
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
	/**
	 * Выполняем усечение перечня отрезков имён полей до начала части снимаемого звена
	 *
	 * @note Усечение вместимости не отнимает: перечень держит её до самой очистки сборки
	 */
	this->_keys.resize(this->_stack.back().base);
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
	// Выполняем вклейку содержимого, уложенного ссылкой
	this->flatten();
	// Выводим собранную запись
	return this->_record;
}
/**
 * @brief Метод извлечения собранной записи кусками
 *
 * @return куски собранной записи по порядку
 *
 */
vector <awh::codec::abc::piece_t> awh::codec::abc::Writer::pieces() const noexcept {
	// Результат работы функции
	vector <piece_t> result;
	// Выполняем резервирование памяти под куски выдачи
	result.reserve((this->_cuts.size() * 2) + 1);
	// Смещение выданной части буфера собираемой записи
	size_t offset = 0;
	// Выполняем перебор врезок чужого содержимого
	for(auto & cut : this->_cuts){
		// Если перед врезкой стоит часть буфера собираемой записи
		if(cut.offset > offset)
			// Выполняем выдачу части буфера, стоящей перед врезкой
			result.push_back(piece_t(this->_record.data() + offset, cut.offset - offset));
		// Выполняем выдачу октетов врезки
		result.push_back(piece_t(cut.buffer, cut.size));
		// Выполняем сдвиг смещения выданной части буфера
		offset = cut.offset;
	}
	// Если остаток буфера собираемой записи не пуст
	if(this->_record.size() > offset)
		// Выполняем выдачу остатка буфера собираемой записи
		result.push_back(piece_t(this->_record.data() + offset, this->_record.size() - offset));
	// Выводим куски собранной записи
	return result;
}
/**
 * @brief Метод извлечения длины собранной записи
 *
 * @return длина собранной записи в октетах
 *
 */
size_t awh::codec::abc::Writer::length() const noexcept {
	// Результат работы функции
	size_t result = this->_record.size();
	// Выполняем перебор врезок чужого содержимого
	for(auto & cut : this->_cuts)
		// Выполняем учёт размера октетов врезки
		result += cut.size;
	// Выводим длину собранной записи
	return result;
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
