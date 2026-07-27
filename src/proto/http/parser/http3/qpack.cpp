/**
 * @file: qpack.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация кодека QPACK (RFC 9204) — статическая таблица, динамическая таблица
 *        с абсолютной индексацией, обработка потоков инструкций кодера и декодера,
 *        кодирование и декодирование секций полей
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http3/qpack.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <algorithm>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::http;

/**
 * @brief Пространство имён внутренних функций кодека
 *
 */
namespace {
	/**
	 * @brief Статическая таблица QPACK (RFC 9204 Appendix A)
	 *
	 * @details Нумерация записей начинается с нуля. Порядок записей задан спецификацией
	 *          и изменению не подлежит: он и есть кодировка
	 *
	 */
	static const h3::qpack::static_entry_t STATIC_TABLE[h3::qpack::STATIC_TABLE_SIZE] = {
		{":authority", ""},                                                                    //  1
		{":path", "/"},                                                                        //  2
		{"age", "0"},                                                                          //  3
		{"content-disposition", ""},                                                           //  4
		{"content-length", "0"},                                                               //  5
		{"cookie", ""},                                                                        //  6
		{"date", ""},                                                                          //  7
		{"etag", ""},                                                                          //  8
		{"if-modified-since", ""},                                                             //  9
		{"if-none-match", ""},                                                                 // 10
		{"last-modified", ""},                                                                // 11
		{"link", ""},                                                                         // 12
		{"location", ""},                                                                     // 13
		{"referer", ""},                                                                      // 14
		{"set-cookie", ""},                                                                   // 15
		{":method", "CONNECT"},                                                               // 16
		{":method", "DELETE"},                                                                // 17
		{":method", "GET"},                                                                   // 18
		{":method", "HEAD"},                                                                  // 19
		{":method", "OPTIONS"},                                                               // 20
		{":method", "POST"},                                                                  // 21
		{":method", "PUT"},                                                                   // 22
		{":scheme", "http"},                                                                  // 23
		{":scheme", "https"},                                                                 // 24
		{":status", "103"},                                                                   // 25
		{":status", "200"},                                                                   // 26
		{":status", "304"},                                                                   // 27
		{":status", "404"},                                                                   // 28
		{":status", "503"},                                                                   // 29
		{"accept", "*/*"},                                                                    // 30
		{"accept", "application/dns-message"},                                                // 31
		{"accept-encoding", "gzip, deflate, br"},                                             // 32
		{"accept-ranges", "bytes"},                                                           // 33
		{"access-control-allow-headers", "cache-control"},                                    // 34
		{"access-control-allow-headers", "content-type"},                                     // 35
		{"access-control-allow-origin", "*"},                                                 // 36
		{"cache-control", "max-age=0"},                                                       // 37
		{"cache-control", "max-age=2592000"},                                                 // 38
		{"cache-control", "max-age=604800"},                                                  // 39
		{"cache-control", "no-cache"},                                                        // 40
		{"cache-control", "no-store"},                                                        // 41
		{"cache-control", "public, max-age=31536000"},                                        // 42
		{"content-encoding", "br"},                                                           // 43
		{"content-encoding", "gzip"},                                                         // 44
		{"content-type", "application/dns-message"},                                          // 45
		{"content-type", "application/javascript"},                                           // 46
		{"content-type", "application/json"},                                                 // 47
		{"content-type", "application/x-www-form-urlencoded"},                                // 48
		{"content-type", "image/gif"},                                                        // 49
		{"content-type", "image/jpeg"},                                                       // 50
		{"content-type", "image/png"},                                                        // 51
		{"content-type", "text/css"},                                                         // 52
		{"content-type", "text/html; charset=utf-8"},                                         // 53
		{"content-type", "text/plain"},                                                       // 54
		{"content-type", "text/plain;charset=utf-8"},                                         // 55
		{"range", "bytes=0-"},                                                                // 56
		{"strict-transport-security", "max-age=31536000"},                                    // 57
		{"strict-transport-security", "max-age=31536000; includesubdomains"},                 // 58
		{"strict-transport-security", "max-age=31536000; includesubdomains; preload"},        // 59
		{"vary", "accept-encoding"},                                                          // 60
		{"vary", "origin"},                                                                   // 61
		{"x-content-type-options", "nosniff"},                                                // 62
		{"x-xss-protection", "1; mode=block"},                                                // 63
		{":status", "100"},                                                                   // 64
		{":status", "204"},                                                                   // 65
		{":status", "206"},                                                                   // 66
		{":status", "302"},                                                                   // 67
		{":status", "400"},                                                                   // 68
		{":status", "403"},                                                                   // 69
		{":status", "421"},                                                                   // 70
		{":status", "425"},                                                                   // 71
		{":status", "500"},                                                                   // 72
		{"accept-language", ""},                                                              // 73
		{"access-control-allow-credentials", "FALSE"},                                        // 74
		{"access-control-allow-credentials", "TRUE"},                                         // 75
		{"access-control-allow-headers", "*"},                                                // 76
		{"access-control-allow-methods", "get"},                                              // 77
		{"access-control-allow-methods", "get, post, options"},                               // 78
		{"access-control-allow-methods", "options"},                                          // 79
		{"access-control-expose-headers", "content-length"},                                  // 80
		{"access-control-request-headers", "content-type"},                                   // 81
		{"access-control-request-method", "get"},                                             // 82
		{"access-control-request-method", "post"},                                            // 83
		{"alt-svc", "clear"},                                                                 // 84
		{"authorization", ""},                                                                // 85
		{"content-security-policy", "script-src 'none'; object-src 'none'; base-uri 'none'"}, // 86
		{"early-data", "1"},                                                                  // 87
		{"expect-ct", ""},                                                                    // 88
		{"forwarded", ""},                                                                    // 89
		{"if-range", ""},                                                                     // 90
		{"origin", ""},                                                                       // 91
		{"purpose", "prefetch"},                                                              // 92
		{"server", ""},                                                                       // 93
		{"timing-allow-origin", "*"},                                                         // 94
		{"upgrade-insecure-requests", "1"},                                                   // 95
		{"user-agent", ""},                                                                   // 96
		{"x-forwarded-for", ""},                                                              // 97
		{"x-frame-options", "deny"},                                                          // 98
		{"x-frame-options", "sameorigin"}                                                     // 99
	};

	/**
	 * @brief Структура группы записей статической таблицы с одним названием
	 *
	 */
	typedef struct Static_Range {
		// Смещение первой записи группы в порядке группировки
		uint32_t first;
		// Количество записей в группе
		uint32_t count;
	} static_range_t;

	/**
	 * @brief Структура индекса статической таблицы по названию поля
	 *
	 * @details Записи с одним названием в статической таблице QPACK не занимают
	 *          непрерывный отрезок (`:status` встречается дважды - позиции 24-28
	 *          и 63-71), поэтому индексы сначала группируются по названию в отдельном
	 *          массиве, а хеш названия отображается уже в отрезок этого массива
	 *
	 */
	typedef struct Static_Index {
		// Индексы записей, сгруппированные по названию
		vector <uint16_t> order;
		// Отрезки групп по хешу названия
		unordered_map <size_t, static_range_t> ranges;
	} static_index_t;

	/**
	 * @brief Функция получения индекса статической таблицы по названию поля
	 *
	 * @details Индекс строится один раз при первом обращении: перебор девяноста девяти
	 *          записей на каждое кодируемое поле обошёлся бы дороже самого кодирования
	 *
	 * @return индекс статической таблицы по названию поля
	 *
	 */
	const static_index_t & staticNames() noexcept {
		/**
		 * Индекс объявлен статическим внутри функции намеренно: статический объект
		 * в области видимости файла строился бы до входа в main, а порядок такого
		 * построения между единицами трансляции не определён
		 */
		static const static_index_t index = [](){
			// Собираемый индекс статической таблицы
			static_index_t result;
			// Выделяем место под индексы всех записей таблицы
			result.order.reserve(h3::qpack::STATIC_TABLE_SIZE);
			/**
			 * Выполняем сбор индексов всех записей таблицы
			 */
			for(uint16_t i = 0; i < static_cast <uint16_t> (h3::qpack::STATIC_TABLE_SIZE); i++)
				// Дописываем индекс очередной записи
				result.order.push_back(i);
			/**
			 * Выполняем группировку индексов по названию поля: сортировка устойчивая,
			 * поэтому внутри группы индексы остаются возрастающими
			 */
			::std::stable_sort(result.order.begin(), result.order.end(), [](const uint16_t first, const uint16_t second) noexcept -> bool {
				// Выполняем сравнение названий записей
				return (STATIC_TABLE[first].name < STATIC_TABLE[second].name);
			});
			// Позиция начала текущей группы
			size_t begin = 0;
			/**
			 * Выполняем выделение отрезков групп
			 */
			for(size_t i = 1; i <= result.order.size(); i++){
				/**
				 * Если группа закончилась - запоминаем её отрезок
				 */
				if((i == result.order.size()) || (STATIC_TABLE[result.order[i]].name != STATIC_TABLE[result.order[begin]].name)){
					// Вычисляем хеш названия группы
					const size_t hash = ::std::hash <string_view> {}(STATIC_TABLE[result.order[begin]].name);
					// Запоминаем отрезок группы
					result.ranges.emplace(hash, static_range_t{static_cast <uint32_t> (begin), static_cast <uint32_t> (i - begin)});
					// Переходим к следующей группе
					begin = i;
				}
			}
			// Выводим собранный индекс
			return result;
		}();
		// Выводим индекс статической таблицы
		return index;
	}
	/**
	 * @brief Функция проверки названия поля на чувствительность
	 *
	 * @details Такие поля кодер всегда трактует как чувствительные: их значения
	 *          не попадают в динамическую таблицу и не индексируются
	 *
	 * @param name название поля
	 * @return     результат проверки
	 *
	 */
	bool isSensitiveName(string_view name) noexcept {
		// Чувствительными считаются поля авторизации и cookie
		return (
			(name == "cookie") || (name == "set-cookie") ||
			(name == "authorization") || (name == "proxy-authorization")
		);
	}
	/**
	 * @brief Функция вычисления хеша пары название-значение
	 *
	 * @param name  название поля
	 * @param value значение поля
	 * @return      хеш пары
	 *
	 */
	size_t pairHash(const size_t name, string_view value) noexcept {
		// Выводим хеш пары название-значение
		return ((name * 31) ^ ::std::hash <string_view> {}(value));
	}
}

/**
 * @brief Функция получения записи статической таблицы по индексу 0..98 (RFC 9204 Appendix A)
 *
 * @param index индекс записи (0-based); >= 99 - невалиден
 * @return      указатель на запись либо nullptr
 *
 */
const awh::http::h3::qpack::static_entry_t * awh::http::h3::qpack::staticTable(const size_t index) noexcept {
	// Если индекс записи вышел за границы таблицы
	if(index >= STATIC_TABLE_SIZE)
		// Выводим признак отсутствия записи
		return nullptr;
	// Выводим запись статической таблицы
	return &STATIC_TABLE[index];
}
/**
 * @brief Функция поиска записи статической таблицы
 *
 * @param name     название искомого поля
 * @param value    значение искомого поля
 * @param index    индекс полного совпадения
 * @param nameOnly индекс совпадения только по названию
 * @return         признак найденного полного совпадения
 *
 */
bool awh::http::h3::qpack::stat::find(string_view name, string_view value, size_t & index, size_t & nameOnly) noexcept {
	// Сбрасываем индекс совпадения только по названию
	nameOnly = STATIC_TABLE_SIZE;
	// Получаем индекс статической таблицы по названию поля
	const static_index_t & names = ::staticNames();
	// Выполняем поиск группы записей с искомым названием
	auto i = names.ranges.find(::std::hash <string_view> {}(name));
	// Если группа записей с искомым названием не найдена
	if(i == names.ranges.end())
		// Выводим признак отсутствия совпадения
		return false;
	/**
	 * Выполняем перебор записей группы: совпадение хешей названий проверяется
	 * сравнением строк, иначе коллизия хеша давала бы чужую запись
	 */
	for(uint32_t offset = 0; offset < i->second.count; offset++){
		// Получаем индекс очередной записи группы
		const uint16_t candidate = names.order[i->second.first + offset];
		// Если название записи не совпало с искомым
		if(STATIC_TABLE[candidate].name != name)
			// Переходим к следующей записи группы
			continue;
		// Если совпадение только по названию ещё не найдено
		if(nameOnly == STATIC_TABLE_SIZE)
			// Запоминаем индекс совпадения только по названию
			nameOnly = candidate;
		// Если совпало и значение записи
		if(STATIC_TABLE[candidate].value == value){
			// Запоминаем индекс полного совпадения
			index = candidate;
			// Выводим признак найденного полного совпадения
			return true;
		}
	}
	// Выводим признак отсутствия полного совпадения
	return false;
}

/**
 * @brief Метод получения количества живых записей таблицы
 *
 * @return количество живых записей
 *
 */
size_t awh::http::h3::qpack::DynamicTable::count() const noexcept {
	// Выводим количество живых записей таблицы
	return this->_entries.size();
}
/**
 * @brief Метод получения текущего суммарного размера таблицы
 *
 * @return суммарный размер живых записей с накладными расходами
 *
 */
uint64_t awh::http::h3::qpack::DynamicTable::size() const noexcept {
	// Выводим текущий суммарный размер таблицы
	return this->_size;
}
/**
 * @brief Метод получения текущей ёмкости таблицы
 *
 * @return текущая ёмкость таблицы
 *
 */
uint64_t awh::http::h3::qpack::DynamicTable::capacity() const noexcept {
	// Выводим текущую ёмкость таблицы
	return this->_capacity;
}
/**
 * @brief Метод получения общего количества вставок
 *
 * @return общее количество вставленных записей
 *
 */
uint64_t awh::http::h3::qpack::DynamicTable::inserts() const noexcept {
	// Выводим общее количество вставок
	return this->_inserts;
}
/**
 * @brief Метод получения абсолютного номера самой старой живой записи
 *
 * @return количество вытесненных записей
 *
 */
uint64_t awh::http::h3::qpack::DynamicTable::dropped() const noexcept {
	// Выводим количество вытесненных записей
	return this->_dropped;
}
/**
 * @brief Метод вычисления размера записи (RFC 9204 §3.2.1)
 *
 * @param name  название поля
 * @param value значение поля
 * @return      размер записи с накладными расходами
 *
 */
uint64_t awh::http::h3::qpack::DynamicTable::entrySize(string_view name, string_view value) noexcept {
	// Выводим размер записи с накладными расходами
	return (static_cast <uint64_t> (name.size()) + static_cast <uint64_t> (value.size()) + ENTRY_OVERHEAD);
}
/**
 * @brief Метод управления сопровождением индекса записей
 *
 * @param mode режим сопровождения индекса
 *
 */
void awh::http::h3::qpack::DynamicTable::indexing(const bool mode) noexcept {
	// Устанавливаем режим сопровождения индекса записей
	this->_indexing = mode;
	// Если сопровождение индекса выключено
	if(!mode){
		// Выполняем очистку индекса записей по паре название-значение
		this->_index.clear();
		// Выполняем очистку индекса записей по названию
		this->_names.clear();
	}
}
/**
 * @brief Метод изменения ёмкости таблицы (RFC 9204 §3.2.3)
 *
 * @param capacity новая ёмкость таблицы
 *
 */
void awh::http::h3::qpack::DynamicTable::setCapacity(const uint64_t capacity) noexcept {
	// Устанавливаем новую ёмкость таблицы
	this->_capacity = capacity;
	// Приводим таблицу в соответствие с новой ёмкостью
	this->evict(0);
}
/**
 * @brief Метод вытеснения записей с самых старых
 *
 * @param room требуемое свободное место в октетах
 *
 */
void awh::http::h3::qpack::DynamicTable::evict(const uint64_t room) noexcept {
	/**
	 * Выполняем вытеснение записей с самых старых, пока в таблице не освободится
	 * требуемое место
	 */
	while(((this->_size + room) > this->_capacity) && !this->_entries.empty()){
		// Получаем самую старую живую запись таблицы
		const field_t & entry = this->_entries.front();
		// Если сопровождение индекса записей включено
		if(this->_indexing){
			// Вычисляем хеш названия вытесняемой записи
			const size_t hashName = ::std::hash <string_view> {}(entry.name);
			// Вычисляем хеш пары название-значение вытесняемой записи
			const size_t hashPair = ::pairHash(hashName, entry.value);
			/**
			 * Выполняем поиск вытесняемой записи в индексе по паре название-значение
			 */
			auto range = this->_index.equal_range(hashPair);
			/**
			 * Выполняем перебор записей ведра индекса
			 */
			for(auto i = range.first; i != range.second; ++i){
				// Если найдена вытесняемая запись
				if(i->second == this->_dropped){
					// Удаляем запись из индекса по паре название-значение
					this->_index.erase(i);
					// Прекращаем перебор ведра индекса
					break;
				}
			}
			// Выполняем поиск вытесняемой записи в индексе по названию
			auto j = this->_names.find(hashName);
			/**
			 * Индекс по названию хранит самую свежую запись с этим названием,
			 * поэтому удаляется он только вместе с последней такой записью
			 */
			if((j != this->_names.end()) && (j->second == this->_dropped))
				// Удаляем запись из индекса по названию
				this->_names.erase(j);
		}
		// Уменьшаем суммарный размер таблицы
		this->_size -= entrySize(entry.name, entry.value);
		// Удаляем самую старую живую запись таблицы
		this->_entries.pop_front();
		// Наращиваем количество вытесненных записей
		this->_dropped++;
	}
}
/**
 * @brief Метод проверки возможности вставки записи без вытеснения занятых
 *
 * @param name  название поля
 * @param value значение поля
 * @param hold  наименьший абсолютный номер записи, которую нельзя вытеснять
 * @return       признак возможности вставки
 *
 */
bool awh::http::h3::qpack::DynamicTable::insertable(string_view name, string_view value, const uint64_t hold) const noexcept {
	// Вычисляем размер вставляемой записи
	const uint64_t required = entrySize(name, value);
	// Если запись не помещается в ёмкость таблицы целиком
	if(required > this->_capacity)
		// Выводим признак невозможности вставки
		return false;
	// Вычисляем свободное место в таблице
	uint64_t available = (this->_capacity - this->_size);
	/**
	 * Выполняем перебор записей с самой старой, освобождая место
	 */
	for(size_t i = 0; (available < required) && (i < this->_entries.size()); i++){
		// Если очередная запись удерживается ссылками
		if((this->_dropped + i) >= hold)
			// Выводим признак невозможности вставки
			return false;
		// Наращиваем свободное место размером вытесняемой записи
		available += entrySize(this->_entries[i].name, this->_entries[i].value);
	}
	// Выводим признак возможности вставки
	return (available >= required);
}
/**
 * @brief Метод вставки записи в таблицу
 *
 * @param name  название поля
 * @param value значение поля
 * @return      признак успешной вставки
 *
 */
bool awh::http::h3::qpack::DynamicTable::add(string_view name, string_view value) noexcept {
	// Вычисляем размер вставляемой записи
	const uint64_t required = entrySize(name, value);
	/**
	 * Запись, не помещающаяся в ёмкость таблицы целиком, не вставляется.
	 * Здесь QPACK намеренно расходится с HPACK: в HPACK такая вставка опустошает
	 * таблицу и ошибкой не является (RFC 7541 §4.4), а в QPACK кодер обязан следить
	 * за размером сам, и попытка - ошибка соединения QPACK_ENCODER_STREAM_ERROR
	 * (RFC 9204 §3.2.2). Поднимает её вызывающий: на пути кодера отказ вовсе
	 * недостижим - вставку заранее отсекает insertable()
	 */
	if(required > this->_capacity)
		// Выводим признак неудачной вставки
		return false;
	// Выполняем вытеснение записей с самых старых, пока новая не поместится
	this->evict(required);
	// Если сопровождение индекса записей включено
	if(this->_indexing){
		// Вычисляем хеш названия вставляемой записи
		const size_t hashName = ::std::hash <string_view> {}(name);
		// Дописываем запись в индекс по паре название-значение
		this->_index.emplace(::pairHash(hashName, value), this->_inserts);
		// Дописываем запись в индекс по названию как самую свежую
		this->_names[hashName] = this->_inserts;
	}
	// Дописываем запись в таблицу
	this->_entries.emplace_back(::std::string(name), ::std::string(value));
	// Наращиваем суммарный размер таблицы
	this->_size += required;
	// Наращиваем общее количество вставок
	this->_inserts++;
	// Выводим признак успешной вставки
	return true;
}
/**
 * @brief Метод получения записи по абсолютному номеру
 *
 * @param absolute абсолютный номер записи
 * @return          указатель на запись либо nullptr, если запись вытеснена либо ещё не вставлена
 *
 */
const awh::http::h3::qpack::field_t * awh::http::h3::qpack::DynamicTable::at(const uint64_t absolute) const noexcept {
	// Если запись вытеснена либо ещё не вставлена
	if((absolute < this->_dropped) || (absolute >= this->_inserts))
		// Выводим признак отсутствия записи
		return nullptr;
	// Выводим запись таблицы по её позиции в очереди
	return &this->_entries[static_cast <size_t> (absolute - this->_dropped)];
}
/**
 * @brief Метод поиска записи по названию и значению
 *
 * @param name      название искомого поля
 * @param value     значение искомого поля
 * @param absolute  абсолютный номер полного совпадения
 * @param nameOnly  абсолютный номер совпадения только по названию
 * @return          признак найденного полного совпадения
 *
 */
bool awh::http::h3::qpack::DynamicTable::find(string_view name, string_view value, uint64_t & absolute, uint64_t & nameOnly) const noexcept {
	// Сбрасываем абсолютный номер совпадения только по названию
	nameOnly = UINT64_MAX;
	// Если таблица пуста либо индекс не сопровождается
	if(this->_entries.empty() || !this->_indexing)
		// Выводим признак отсутствия совпадения
		return false;
	// Вычисляем хеш названия искомого поля
	const size_t hashName = ::std::hash <string_view> {}(name);
	// Выполняем поиск совпадения только по названию
	auto j = this->_names.find(hashName);
	// Если совпадение только по названию найдено
	if(j != this->_names.end()){
		// Получаем запись совпадения только по названию
		const field_t * entry = this->at(j->second);
		// Если запись жива и её название совпало с искомым
		if((entry != nullptr) && (entry->name == name))
			// Запоминаем абсолютный номер совпадения только по названию
			nameOnly = j->second;
	}
	// Абсолютный номер самого свежего полного совпадения
	uint64_t found = UINT64_MAX;
	// Выполняем поиск полных совпадений по хешу пары название-значение
	auto range = this->_index.equal_range(::pairHash(hashName, value));
	/**
	 * Выполняем перебор записей ведра индекса
	 */
	for(auto i = range.first; i != range.second; ++i){
		// Получаем запись очередного кандидата
		const field_t * entry = this->at(i->second);
		// Если запись вытеснена
		if(entry == nullptr)
			// Переходим к следующему кандидату
			continue;
		// Если название либо значение записи не совпали с искомыми
		if((entry->name != name) || (entry->value != value))
			// Переходим к следующему кандидату
			continue;
		/**
		 * Запоминаем самое свежее полное совпадение: свежая запись переживёт
		 * вытеснение дольше, а разница в длине представления между записями
		 * таблицы невелика
		 */
		if((found == UINT64_MAX) || (i->second > found))
			// Запоминаем абсолютный номер полного совпадения
			found = i->second;
	}
	// Если полное совпадение найдено
	if(found != UINT64_MAX){
		// Устанавливаем абсолютный номер полного совпадения
		absolute = found;
		// Выводим признак найденного полного совпадения
		return true;
	}
	// Выводим признак отсутствия полного совпадения
	return false;
}
/**
 * @brief Метод поиска записи только по названию
 *
 * @param name     название искомого поля
 * @param absolute абсолютный номер совпадения
 * @return         признак найденного совпадения
 *
 */
bool awh::http::h3::qpack::DynamicTable::findName(string_view name, uint64_t & absolute) const noexcept {
	// Если таблица пуста либо индекс не сопровождается
	if(this->_entries.empty() || !this->_indexing)
		// Выводим признак отсутствия совпадения
		return false;
	// Выполняем поиск совпадения по хешу названия
	auto i = this->_names.find(::std::hash <string_view> {}(name));
	// Если совпадение не найдено
	if(i == this->_names.end())
		// Выводим признак отсутствия совпадения
		return false;
	// Получаем найденную запись
	const field_t * entry = this->at(i->second);
	// Если запись вытеснена либо её название не совпало с искомым
	if((entry == nullptr) || (entry->name != name))
		// Выводим признак отсутствия совпадения
		return false;
	// Устанавливаем абсолютный номер совпадения
	absolute = i->second;
	// Выводим признак найденного совпадения
	return true;
}
/**
 * @brief Метод очистки таблицы
 *
 */
void awh::http::h3::qpack::DynamicTable::clear() noexcept {
	// Выполняем очистку списка записей таблицы
	this->_entries.clear();
	// Выполняем очистку индекса записей по паре название-значение
	this->_index.clear();
	// Выполняем очистку индекса записей по названию
	this->_names.clear();
	// Сбрасываем суммарный размер таблицы
	this->_size = 0;
	// Сбрасываем общее количество вставок
	this->_inserts = 0;
	// Сбрасываем количество вытесненных записей
	this->_dropped = 0;
}
/**
 * @brief Конструктор
 *
 * @param capacity начальная ёмкость таблицы
 *
 */
awh::http::h3::qpack::DynamicTable::DynamicTable(const uint64_t capacity) noexcept :
 _size(0), _capacity(capacity), _inserts(0), _dropped(0), _indexing(false) {}

/**
 * @brief Метод декодирования строки, закодированной литералом либо Huffman
 *
 * @param data       входной буфер
 * @param size       доступно байт
 * @param prefixBits размер префикса длины в битах
 * @param output     выходной буфер строки
 * @param consumed   количество прочитанных байт
 * @return           результат декодирования
 *
 */
awh::http::h3::status_t awh::http::h3::qpack::Decoder::decodeString(const uint8_t * data, const size_t size, const uint8_t prefixBits, string & output, size_t & consumed) noexcept {
	// Если данных для разбора нет
	if((data == nullptr) || (size < 1))
		// Выводим признак нехватки данных
		return status_t::INCOMPLETE;
	/**
	 * Признак Huffman-кодирования занимает бит, следующий сразу за префиксом длины:
	 * его положение зависит от представления, поэтому вычисляется по размеру префикса
	 */
	const bool huffman = ((data[0] & static_cast <uint8_t> (1u << prefixBits)) != 0);
	// Длина закодированной строки
	uint64_t length = 0;
	// Количество прочитанных байт длины
	size_t used = 0;
	// Выполняем чтение длины закодированной строки
	const h2::status_t status = h2::hpack::prefixed::decode(data, size, prefixBits, length, used);
	// Если длину строки прочитать не удалось
	if(status == h2::status_t::INCOMPLETE)
		// Выводим признак нехватки данных
		return status_t::INCOMPLETE;
	// Если при чтении длины строки произошло переполнение
	else if(status != h2::status_t::OK)
		// Выводим признак ошибки
		return status_t::ERROR;
	// Если тело строки во входном буфере ещё не целиком
	if(static_cast <uint64_t> (size - used) < length)
		// Выводим признак нехватки данных
		return status_t::INCOMPLETE;
	// Если строка закодирована Huffman
	if(huffman){
		// Выполняем декодирование строки
		if(!h2::hpack::huffman::decode((data + used), static_cast <size_t> (length), output))
			// Выводим признак ошибки
			return status_t::ERROR;
	// Если строка передана литералом
	} else output.assign(reinterpret_cast <const char *> (data + used), static_cast <size_t> (length));
	// Устанавливаем количество прочитанных байт
	consumed = (used + static_cast <size_t> (length));
	// Выводим признак успешного декодирования
	return status_t::OK;
}
/**
 * @brief Метод декодирования строки представления прямо в арену
 *
 * @details Отличается от разбора в отдельный буфер только назначением, но именно оно
 *          и стоит дорого: декодированное поле всё равно ложится в арену, и разбор
 *          в промежуточный буфер означал бы лишнее копирование каждой строки секции
 *
 * @param data       входной буфер
 * @param size       доступно байт
 * @param prefixBits размер префикса длины в битах
 * @param consumed   количество прочитанных байт
 * @return           результат декодирования
 *
 */
awh::http::h3::status_t awh::http::h3::qpack::Decoder::decodeString(const uint8_t * data, const size_t size, const uint8_t prefixBits, size_t & consumed) noexcept {
	// Если данных для разбора нет
	if((data == nullptr) || (size < 1))
		// Выводим признак нехватки данных
		return status_t::INCOMPLETE;
	/**
	 * Признак Huffman-кодирования занимает бит, следующий сразу за префиксом длины:
	 * его положение зависит от представления, поэтому вычисляется по размеру префикса
	 */
	const bool huffman = ((data[0] & static_cast <uint8_t> (1u << prefixBits)) != 0);
	// Длина закодированной строки
	uint64_t length = 0;
	// Количество прочитанных байт длины
	size_t used = 0;
	// Выполняем чтение длины закодированной строки
	const h2::status_t status = h2::hpack::prefixed::decode(data, size, prefixBits, length, used);
	// Если длину строки прочитать не удалось
	if(status == h2::status_t::INCOMPLETE)
		// Выводим признак нехватки данных
		return status_t::INCOMPLETE;
	// Если при чтении длины строки произошло переполнение
	else if(status != h2::status_t::OK)
		// Выводим признак ошибки
		return status_t::ERROR;
	// Если тело строки во входном буфере ещё не целиком
	if(static_cast <uint64_t> (size - used) < length)
		// Выводим признак нехватки данных
		return status_t::INCOMPLETE;
	// Если строка закодирована Huffman
	if(huffman){
		// Запоминаем смещение декодируемой строки в арене
		const size_t offset = this->_arenaLength;
		/**
		 * Место в арене берётся по оценке сверху: фактическая длина известна только
		 * после декодирования, а проверять границу на каждый символ дороже самого
		 * декодирования. Излишек возвращается арене сразу по его окончании
		 */
		char * target = this->reserve(h2::hpack::huffman::space(static_cast <size_t> (length)));
		// Выполняем декодирование строки прямо в арену
		const size_t decoded = h2::hpack::huffman::decode((data + used), static_cast <size_t> (length), target);
		// Если строка декодирована с ошибкой
		if(decoded == SIZE_MAX){
			// Возвращаем арене занятое под строку место
			this->_arenaLength = offset;
			// Выводим признак ошибки
			return status_t::ERROR;
		}
		// Возвращаем арене излишек, взятый по оценке сверху
		this->_arenaLength = (offset + decoded);
	// Если строка передана литералом - переносим её в арену как есть
	} else ::memcpy(this->reserve(static_cast <size_t> (length)), (data + used), static_cast <size_t> (length));
	// Устанавливаем количество прочитанных байт
	consumed = (used + static_cast <size_t> (length));
	// Выводим признак успешного декодирования
	return status_t::OK;
}
/**
 * @brief Метод разрешения абсолютного номера записи в пару полей
 *
 * @param absolute абсолютный номер записи динамической таблицы
 * @param entry    найденная запись
 * @return         признак успешного разрешения
 *
 */
bool awh::http::h3::qpack::Decoder::resolve(const uint64_t absolute, const field_t *& entry) const noexcept {
	// Выполняем поиск записи по абсолютному номеру
	entry = this->_table.at(absolute);
	// Выводим признак успешного разрешения
	return (entry != nullptr);
}
/**
 * @brief Метод дописывания декодированного поля в арену
 *
 * @param name        название поля
 * @param value       значение поля
 * @param sensitive   признак чувствительного значения
 * @param listSize    накопленный размер списка полей
 * @param maxListSize лимит размера списка полей
 *
 */
void awh::http::h3::qpack::Decoder::emit(const size_t nameOffset, const size_t nameLength, const size_t valueOffset, const size_t valueLength, const bool sensitive, uint64_t & listSize, const uint64_t maxListSize) noexcept {
	// Наращиваем размер списка полей по правилу RFC 9114 §4.2.2
	listSize += (static_cast <uint64_t> (nameLength) + static_cast <uint64_t> (valueLength) + ENTRY_OVERHEAD);
	// Если лимит списка полей превышен
	if((maxListSize > 0) && (listSize > maxListSize))
		// Запоминаем превышение лимита списка полей
		this->_overflow = true;
	/**
	 * Поле, не прошедшее лимит, наружу не отдаётся, но разбор секции продолжается:
	 * динамическая таблица обязана остаться синхронной с кодером пира, иначе пришлось
	 * бы рвать всё соединение вместо одного потока
	 */
	if(this->_overflow){
		// Возвращаем арене место, занятое полем: наружу оно уже не пойдёт
		this->_arenaLength = nameOffset;
		// Выходим из метода
		return;
	}
	// Собираемый срез декодированного поля
	slice_t slice;
	// Устанавливаем смещение названия поля в арене
	slice.nameOffset = nameOffset;
	// Устанавливаем длину названия поля
	slice.nameLength = nameLength;
	// Устанавливаем смещение значения поля в арене
	slice.valueOffset = valueOffset;
	// Устанавливаем длину значения поля
	slice.valueLength = valueLength;
	// Устанавливаем признак чувствительного значения
	slice.sensitive = sensitive;
	// Дописываем срез декодированного поля
	this->_slices.push_back(slice);
}
/**
 * @brief Метод дописывания строки в арену декодированных строк
 *
 * @param value дописываемая строка
 * @return      смещение дописанной строки в арене
 *
 */
size_t awh::http::h3::qpack::Decoder::append(string_view value) noexcept {
	// Запоминаем смещение дописываемой строки в арене
	const size_t result = this->_arenaLength;
	// Дописываем строку в арену
	::memcpy(this->reserve(value.size()), value.data(), value.size());
	// Выводим смещение дописанной строки в арене
	return result;
}
/**
 * @brief Метод выделения места в арене декодированных строк
 *
 * @param size требуемое количество октетов
 * @return     указатель на выделенное место
 *
 */
char * awh::http::h3::qpack::Decoder::reserve(const size_t size) noexcept {
	// Требуемый размер арены
	const size_t required = (this->_arenaLength + size);
	// Если места в арене не хватает
	if(required > this->_arena.size())
		/**
		 * Наращиваем арену удвоением: секция состоит из десятков коротких полей,
		 * и наращивание под каждое из них означало бы перевыделение на каждое поле
		 */
		this->_arena.resize(std::max(required, (this->_arena.size() * 2)));
	// Получаем указатель на выделенное место
	char * result = (&this->_arena[0] + this->_arenaLength);
	// Наращиваем занятую часть арены
	this->_arenaLength = required;
	// Выводим указатель на выделенное место
	return result;
}
/**
 * @brief Метод получения динамической таблицы кодера пира
 *
 * @return динамическая таблица кодера пира
 *
 */
awh::http::h3::qpack::dynamic_table_t & awh::http::h3::qpack::Decoder::table() noexcept {
	// Выводим динамическую таблицу кодера пира
	return this->_table;
}
/**
 * @brief Метод установки верхней границы ёмкости таблицы
 *
 * @param capacity верхняя граница ёмкости таблицы
 *
 */
void awh::http::h3::qpack::Decoder::maxCapacity(const uint64_t capacity) noexcept {
	// Устанавливаем верхнюю границу ёмкости таблицы
	this->_maxCapacity = capacity;
}
/**
 * @brief Метод установки числа потоков, которым разрешено ожидать пополнения таблицы
 *
 * @param count число потоков
 *
 */
void awh::http::h3::qpack::Decoder::maxBlocked(const uint64_t count) noexcept {
	// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
	this->_maxBlocked = count;
}
/**
 * @brief Метод получения количества потоков, ожидающих пополнения таблицы
 *
 * @return количество заблокированных потоков
 *
 */
size_t awh::http::h3::qpack::Decoder::blocked() const noexcept {
	// Выводим количество заблокированных потоков
	return this->_blocked.size();
}
/**
 * @brief Метод обработки инструкций потока кодера (RFC 9204 §4.3)
 *
 * @param data     входной буфер потока кодера
 * @param consumed количество разобранных октетов
 * @param error    код ошибки протокола
 * @return         результат обработки (OK/ERROR)
 *
 */
awh::http::h3::status_t awh::http::h3::qpack::Decoder::decodeEncoderStream(string_view data, size_t & consumed, error_t & error) noexcept {
	// Сбрасываем количество разобранных октетов
	consumed = 0;
	// Получаем указатель на входной буфер
	const uint8_t * buffer = reinterpret_cast <const uint8_t *> (data.data());
	// Получаем размер входного буфера
	const size_t size = data.size();
	/**
	 * Выполняем разбор всех целых инструкций входного буфера: неполный остаток
	 * не является ошибкой и остаётся вызывающему для подачи вместе с продолжением
	 */
	while(consumed < size){
		// Получаем первый октет очередной инструкции
		const uint8_t byte = buffer[consumed];
		// Позиция разбора текущей инструкции
		size_t offset = consumed;
		// Количество прочитанных байт очередного поля инструкции
		size_t used = 0;
		/**
		 * Инструкция вставки со ссылкой на название (RFC 9204 §4.3.2)
		 */
		if((byte & 0x80) != 0){
			// Признак ссылки на статическую таблицу
			const bool isStatic = ((byte & 0x40) != 0);
			// Номер записи с искомым названием
			uint64_t index = 0;
			// Выполняем чтение номера записи с названием
			const h2::status_t status = h2::hpack::prefixed::decode((buffer + offset), (size - offset), 6, index, used);
			// Если данных для разбора инструкции ещё недостаточно
			if(status == h2::status_t::INCOMPLETE)
				// Прекращаем разбор инструкций
				break;
			// Если при чтении номера записи произошло переполнение
			else if(status != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			/**
			 * Название копируется из таблицы, а не берётся представлением: вставка
			 * способна вытеснить как раз ту запись, на которую ссылается
			 */
			if(isStatic){
				// Получаем запись статической таблицы
				const static_entry_t * entry = staticTable(static_cast <size_t> (index));
				// Если запись статической таблицы не найдена
				if(entry == nullptr){
					// Устанавливаем код ошибки протокола
					error = error_t::QPACK_ENCODER_STREAM_ERROR;
					// Выводим результат обработки
					return status_t::ERROR;
				}
				// Запоминаем название поля
				this->_name.assign(entry->name);
			} else {
				/**
				 * Номер записи динамической таблицы задан относительно точки вставки:
				 * нулевой номер означает самую свежую запись (RFC 9204 §3.2.5)
				 */
				if(index >= this->_table.inserts()){
					// Устанавливаем код ошибки протокола
					error = error_t::QPACK_ENCODER_STREAM_ERROR;
					// Выводим результат обработки
					return status_t::ERROR;
				}
				// Получаем запись динамической таблицы
				const field_t * entry = this->_table.at(this->_table.inserts() - index - 1);
				// Если запись динамической таблицы вытеснена
				if(entry == nullptr){
					// Устанавливаем код ошибки протокола
					error = error_t::QPACK_ENCODER_STREAM_ERROR;
					// Выводим результат обработки
					return status_t::ERROR;
				}
				// Запоминаем название поля
				this->_name.assign(entry->name);
			}
			// Выполняем чтение значения поля
			const status_t status2 = this->decodeString((buffer + offset), (size - offset), 7, this->_scratch, used);
			// Если данных для разбора инструкции ещё недостаточно
			if(status2 == status_t::INCOMPLETE)
				// Прекращаем разбор инструкций
				break;
			// Если значение поля прочитать не удалось
			else if(status2 != status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			/**
			 * Вставка записи, не помещающейся в ёмкость таблицы, недопустима:
			 * кодер обязан следить за размером сам (RFC 9204 §3.2.2)
			 */
			if(!this->_table.add(this->_name, this->_scratch)){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
		/**
		 * Инструкция вставки с литеральным названием (RFC 9204 §4.3.3)
		 */
		} else if((byte & 0x40) != 0) {
			// Выполняем чтение названия поля
			const status_t status = this->decodeString((buffer + offset), (size - offset), 5, this->_name, used);
			// Если данных для разбора инструкции ещё недостаточно
			if(status == status_t::INCOMPLETE)
				// Прекращаем разбор инструкций
				break;
			// Если название поля прочитать не удалось
			else if(status != status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			// Выполняем чтение значения поля
			const status_t status2 = this->decodeString((buffer + offset), (size - offset), 7, this->_scratch, used);
			// Если данных для разбора инструкции ещё недостаточно
			if(status2 == status_t::INCOMPLETE)
				// Прекращаем разбор инструкций
				break;
			// Если значение поля прочитать не удалось
			else if(status2 != status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			// Выполняем вставку записи в динамическую таблицу
			if(!this->_table.add(this->_name, this->_scratch)){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
		/**
		 * Инструкция изменения ёмкости динамической таблицы (RFC 9204 §4.3.1)
		 */
		} else if((byte & 0x20) != 0) {
			// Новая ёмкость динамической таблицы
			uint64_t capacity = 0;
			// Выполняем чтение новой ёмкости динамической таблицы
			const h2::status_t status = h2::hpack::prefixed::decode((buffer + offset), (size - offset), 5, capacity, used);
			// Если данных для разбора инструкции ещё недостаточно
			if(status == h2::status_t::INCOMPLETE)
				// Прекращаем разбор инструкций
				break;
			// Если при чтении ёмкости произошло переполнение
			else if(status != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			/**
			 * Ёмкость сверх анонсированной нами - ошибка: пир требует памяти больше,
			 * чем мы согласились держать (RFC 9204 §3.2.3)
			 */
			if(capacity > this->_maxCapacity){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Устанавливаем новую ёмкость динамической таблицы
			this->_table.setCapacity(capacity);
		/**
		 * Инструкция дублирования записи (RFC 9204 §4.3.4)
		 */
		} else {
			// Номер дублируемой записи
			uint64_t index = 0;
			// Выполняем чтение номера дублируемой записи
			const h2::status_t status = h2::hpack::prefixed::decode((buffer + offset), (size - offset), 5, index, used);
			// Если данных для разбора инструкции ещё недостаточно
			if(status == h2::status_t::INCOMPLETE)
				// Прекращаем разбор инструкций
				break;
			// Если при чтении номера записи произошло переполнение
			else if(status != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			// Если номер записи вышел за границы таблицы
			if(index >= this->_table.inserts()){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Получаем дублируемую запись динамической таблицы
			const field_t * entry = this->_table.at(this->_table.inserts() - index - 1);
			// Если дублируемая запись вытеснена
			if(entry == nullptr){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Копируем название дублируемой записи - вставка способна её вытеснить
			this->_name.assign(entry->name);
			// Копируем значение дублируемой записи
			this->_scratch.assign(entry->value);
			// Выполняем вставку копии записи в динамическую таблицу
			if(!this->_table.add(this->_name, this->_scratch)){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_ENCODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
		}
		// Отмечаем инструкцию разобранной целиком
		consumed = offset;
	}
	/**
	 * Извещаем кодер пира о полученных вставках приращением счётчика: без извещения
	 * кодер не смог бы вытеснять записи и ссылался бы только на статическую таблицу
	 */
	if(this->_table.inserts() > this->_acked){
		// Записываем инструкцию приращения счётчика вставок (RFC 9204 §4.4.3)
		h2::hpack::prefixed::encode(this->_output, (this->_table.inserts() - this->_acked), 6, 0x00);
		// Запоминаем количество вставок, о котором извещён кодер пира
		this->_acked = this->_table.inserts();
	}
	// Выводим результат обработки
	return status_t::OK;
}
/**
 * @brief Метод декодирования секции полей целиком
 *
 * @param sid         идентификатор потока, которому принадлежит секция
 * @param section     секция полей целиком (нагрузка кадра HEADERS)
 * @param output      декодированные поля (ссылки в арену декодера)
 * @param maxListSize лимит суммарного размера списка полей; 0 - без лимита
 * @param error       код ошибки протокола
 * @return            результат декодирования (OK/BLOCKED/ERROR)
 *
 */
awh::http::h3::status_t awh::http::h3::qpack::Decoder::decode(const uint64_t sid, string_view section, vector <field_view_t> & output, const uint64_t maxListSize, error_t & error) noexcept {
	// Выполняем очистку списка декодированных полей
	output.clear();
	// Сбрасываем признак превышения лимита списка полей
	this->_overflow = false;
	// Получаем указатель на разбираемую секцию
	const uint8_t * buffer = reinterpret_cast <const uint8_t *> (section.data());
	// Получаем размер разбираемой секции
	const size_t size = section.size();
	// Позиция разбора секции
	size_t offset = 0;
	// Количество прочитанных байт очередного поля
	size_t used = 0;
	// Закодированное требуемое число вставок
	uint64_t encoded = 0;
	/**
	 * Секция обязана начинаться с префикса из двух целых с префиксом (RFC 9204 §4.5.1)
	 */
	if(h2::hpack::prefixed::decode(buffer, size, 8, encoded, used) != h2::status_t::OK){
		// Устанавливаем код ошибки протокола
		error = error_t::QPACK_DECOMPRESSION_FAILED;
		// Выводим результат декодирования
		return status_t::ERROR;
	}
	// Выполняем смещение разбора
	offset += used;
	// Если разница базы в секции отсутствует
	if(offset >= size){
		// Устанавливаем код ошибки протокола
		error = error_t::QPACK_DECOMPRESSION_FAILED;
		// Выводим результат декодирования
		return status_t::ERROR;
	}
	// Получаем знак разницы базы
	const bool sign = ((buffer[offset] & 0x80) != 0);
	// Разница базы относительно требуемого числа вставок
	uint64_t delta = 0;
	// Выполняем чтение разницы базы
	if(h2::hpack::prefixed::decode((buffer + offset), (size - offset), 7, delta, used) != h2::status_t::OK){
		// Устанавливаем код ошибки протокола
		error = error_t::QPACK_DECOMPRESSION_FAILED;
		// Выводим результат декодирования
		return status_t::ERROR;
	}
	// Выполняем смещение разбора
	offset += used;
	/**
	 * Требуемое число вставок передаётся по модулю удвоенной ёмкости таблицы в записях:
	 * так оно занимает мало места и остаётся однозначным, пока разница между кодером
	 * и декодером не превышает половины периода (RFC 9204 §4.5.1.1)
	 */
	const uint64_t entries = (this->_maxCapacity / ENTRY_OVERHEAD);
	// Вычисляем период неоднозначности требуемого числа вставок
	const uint64_t range = (entries * 2);
	// Требуемое число вставок
	uint64_t required = 0;
	// Если секция ссылается на динамическую таблицу
	if(encoded != 0){
		// Если период неоднозначности пуст либо значение вышло за его границы
		if((range == 0) || (encoded > range)){
			// Устанавливаем код ошибки протокола
			error = error_t::QPACK_DECOMPRESSION_FAILED;
			// Выводим результат декодирования
			return status_t::ERROR;
		}
		// Вычисляем наибольшее значение, которое кодер мог иметь в виду
		const uint64_t maximum = (this->_table.inserts() + entries);
		// Вычисляем начало периода, в который попадает наибольшее значение
		const uint64_t wrapped = ((maximum / range) * range);
		// Восстанавливаем требуемое число вставок
		required = (wrapped + encoded - 1);
		// Если восстановленное значение оказалось за наибольшим
		if(required > maximum){
			// Если сдвинуть значение на период назад нельзя
			if(required <= range){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Сдвигаем значение на период назад
			required -= range;
		}
		// Если восстановленное значение обнулилось
		if(required == 0){
			// Устанавливаем код ошибки протокола
			error = error_t::QPACK_DECOMPRESSION_FAILED;
			// Выводим результат декодирования
			return status_t::ERROR;
		}
	}
	// База относительной нумерации записей в секции
	uint64_t base = 0;
	// Если база больше либо равна требуемому числу вставок
	if(!sign){
		// Если сложение выйдет за разрядность
		if(delta > (UINT64_MAX - required)){
			// Устанавливаем код ошибки протокола
			error = error_t::QPACK_DECOMPRESSION_FAILED;
			// Выводим результат декодирования
			return status_t::ERROR;
		}
		// Вычисляем базу относительной нумерации
		base = (required + delta);
	// Если база меньше требуемого числа вставок
	} else {
		// Если вычитание уйдёт ниже нуля
		if(delta >= required){
			// Устанавливаем код ошибки протокола
			error = error_t::QPACK_DECOMPRESSION_FAILED;
			// Выводим результат декодирования
			return status_t::ERROR;
		}
		// Вычисляем базу относительной нумерации
		base = (required - delta - 1);
	}
	/**
	 * Если нужных вставок ещё не пришло - поток блокируется до их прихода.
	 * Это штатное состояние, а не ошибка: секция и инструкции кодера идут
	 * разными потоками и обгоняют друг друга (RFC 9204 §2.1.2)
	 */
	if(required > this->_table.inserts()){
		// Запоминаем требуемое потоку число вставок
		this->_blocked[sid] = required;
		/**
		 * Число одновременно заблокированных потоков ограничено нашим анонсом:
		 * его превышение кодером - ошибка уровня соединения
		 */
		if(static_cast <uint64_t> (this->_blocked.size()) > this->_maxBlocked){
			// Устанавливаем код ошибки протокола
			error = error_t::QPACK_DECOMPRESSION_FAILED;
			// Выводим результат декодирования
			return status_t::ERROR;
		}
		// Выводим результат декодирования
		return status_t::BLOCKED;
	}
	// Снимаем блокировку потока
	this->_blocked.erase(sid);
	// Выполняем очистку арены декодированных строк с сохранением её ёмкости
	this->_arenaLength = 0;
	// Выполняем очистку срезов декодированных полей
	this->_slices.clear();
	// Накопленный размер списка полей
	uint64_t listSize = 0;
	/**
	 * Выполняем разбор всех представлений полей секции
	 */
	while(offset < size){
		// Получаем первый октет очередного представления
		const uint8_t byte = buffer[offset];
		// Номер записи в представлении
		uint64_t index = 0;
		// Признак чувствительного значения
		bool sensitive = false;
		// Абсолютный номер записи динамической таблицы
		uint64_t absolute = 0;
		// Признак ссылки на динамическую таблицу
		bool dynamic = false;
		/**
		 * Название и значение поля собираются прямо в арене: наружу они отдаются
		 * представлениями в неё, и складывать их сначала в отдельный буфер значило бы
		 * копировать каждую строку секции дважды
		 */
		size_t nameOffset = this->_arenaLength, nameLength = 0;
		// Смещение и длина значения поля в арене
		size_t valueOffset = this->_arenaLength, valueLength = 0;
		// Признак того, что поле собрано целиком из таблицы
		bool complete = false;
		/**
		 * Представление со ссылкой на запись целиком (RFC 9204 §4.5.2)
		 */
		if((byte & 0x80) != 0){
			// Признак ссылки на статическую таблицу
			const bool isStatic = ((byte & 0x40) != 0);
			// Выполняем чтение номера записи
			if(h2::hpack::prefixed::decode((buffer + offset), (size - offset), 6, index, used) != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			// Если ссылка ведёт в статическую таблицу
			if(isStatic){
				// Получаем запись статической таблицы
				const static_entry_t * entry = staticTable(static_cast <size_t> (index));
				// Если запись статической таблицы не найдена
				if(entry == nullptr){
					// Устанавливаем код ошибки протокола
					error = error_t::QPACK_DECOMPRESSION_FAILED;
					// Выводим результат декодирования
					return status_t::ERROR;
				}
				// Переносим название поля в арену
				nameOffset = this->append(entry->name);
				// Запоминаем длину названия поля
				nameLength = entry->name.size();
				// Переносим значение поля в арену
				valueOffset = this->append(entry->value);
				// Запоминаем длину значения поля
				valueLength = entry->value.size();
			// Если ссылка ведёт в динамическую таблицу
			} else {
				// Если номер записи вышел за границы базы
				if(index >= base){
					// Устанавливаем код ошибки протокола
					error = error_t::QPACK_DECOMPRESSION_FAILED;
					// Выводим результат декодирования
					return status_t::ERROR;
				}
				// Вычисляем абсолютный номер записи
				absolute = (base - index - 1);
				// Запоминаем ссылку на динамическую таблицу
				dynamic = true;
			}
			// Запоминаем, что поле собрано целиком
			complete = true;
		/**
		 * Представление со ссылкой на название (RFC 9204 §4.5.4)
		 */
		} else if((byte & 0x40) != 0) {
			// Извлекаем признак чувствительного значения
			sensitive = ((byte & 0x20) != 0);
			// Признак ссылки на статическую таблицу
			const bool isStatic = ((byte & 0x10) != 0);
			// Выполняем чтение номера записи с названием
			if(h2::hpack::prefixed::decode((buffer + offset), (size - offset), 4, index, used) != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			// Если ссылка ведёт в статическую таблицу
			if(isStatic){
				// Получаем запись статической таблицы
				const static_entry_t * entry = staticTable(static_cast <size_t> (index));
				// Если запись статической таблицы не найдена
				if(entry == nullptr){
					// Устанавливаем код ошибки протокола
					error = error_t::QPACK_DECOMPRESSION_FAILED;
					// Выводим результат декодирования
					return status_t::ERROR;
				}
				// Переносим название поля в арену
				nameOffset = this->append(entry->name);
				// Запоминаем длину названия поля
				nameLength = entry->name.size();
			// Если ссылка ведёт в динамическую таблицу
			} else {
				// Если номер записи вышел за границы базы
				if(index >= base){
					// Устанавливаем код ошибки протокола
					error = error_t::QPACK_DECOMPRESSION_FAILED;
					// Выводим результат декодирования
					return status_t::ERROR;
				}
				// Вычисляем абсолютный номер записи
				absolute = (base - index - 1);
				// Запоминаем ссылку на динамическую таблицу
				dynamic = true;
			}
		/**
		 * Представление с литеральным названием (RFC 9204 §4.5.6)
		 */
		} else if((byte & 0x20) != 0) {
			// Извлекаем признак чувствительного значения
			sensitive = ((byte & 0x10) != 0);
			// Запоминаем смещение названия поля в арене
			nameOffset = this->_arenaLength;
			// Выполняем чтение названия поля прямо в арену
			if(this->decodeString((buffer + offset), (size - offset), 3, used) != status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			// Запоминаем длину названия поля
			nameLength = (this->_arenaLength - nameOffset);
		/**
		 * Представление со ссылкой на запись за базой (RFC 9204 §4.5.3)
		 */
		} else if((byte & 0x10) != 0) {
			// Выполняем чтение номера записи
			if(h2::hpack::prefixed::decode((buffer + offset), (size - offset), 4, index, used) != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			// Если сложение выйдет за разрядность
			if(index > (UINT64_MAX - base)){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Вычисляем абсолютный номер записи
			absolute = (base + index);
			// Запоминаем ссылку на динамическую таблицу
			dynamic = true;
			// Запоминаем, что поле собрано целиком
			complete = true;
		/**
		 * Представление со ссылкой на название записи за базой (RFC 9204 §4.5.5)
		 */
		} else {
			// Извлекаем признак чувствительного значения
			sensitive = ((byte & 0x08) != 0);
			// Выполняем чтение номера записи с названием
			if(h2::hpack::prefixed::decode((buffer + offset), (size - offset), 3, index, used) != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			// Если сложение выйдет за разрядность
			if(index > (UINT64_MAX - base)){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Вычисляем абсолютный номер записи
			absolute = (base + index);
			// Запоминаем ссылку на динамическую таблицу
			dynamic = true;
		}
		// Если представление ссылается на динамическую таблицу
		if(dynamic){
			/**
			 * Ссылка обязана лежать ниже требуемого числа вставок: иначе кодер
			 * объявил в префиксе меньше, чем использовал (RFC 9204 §4.5.1)
			 */
			if(absolute >= required){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Запись динамической таблицы
			const field_t * entry = nullptr;
			// Если запись динамической таблицы вытеснена
			if(!this->resolve(absolute, entry)){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Переносим название поля в арену
			nameOffset = this->append(entry->name);
			// Запоминаем длину названия поля
			nameLength = entry->name.size();
			// Если поле собрано целиком
			if(complete){
				// Переносим значение поля в арену
				valueOffset = this->append(entry->value);
				// Запоминаем длину значения поля
				valueLength = entry->value.size();
			}
		}
		// Если значение поля передано отдельно
		if(!complete){
			// Запоминаем смещение значения поля в арене
			valueOffset = this->_arenaLength;
			// Выполняем чтение значения поля прямо в арену
			if(this->decodeString((buffer + offset), (size - offset), 7, used) != status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECOMPRESSION_FAILED;
				// Выводим результат декодирования
				return status_t::ERROR;
			}
			// Выполняем смещение разбора
			offset += used;
			// Запоминаем длину значения поля
			valueLength = (this->_arenaLength - valueOffset);
		}
		// Выполняем учёт декодированного поля
		this->emit(nameOffset, nameLength, valueOffset, valueLength, sensitive, listSize, maxListSize);
	}
	/**
	 * Выполняем сборку представлений декодированных полей: арена во время разбора
	 * дописывалась и могла быть перевыделена, поэтому представления собираются
	 * один раз по её окончательному адресу
	 */
	if(!this->_overflow){
		// Выделяем место под декодированные поля
		output.reserve(this->_slices.size());
		/**
		 * Выполняем перебор всех срезов декодированных полей
		 */
		for(const auto & slice : this->_slices){
			// Собираемое декодированное поле
			field_view_t field;
			// Устанавливаем название поля
			field.name = string_view(this->_arena.data() + slice.nameOffset, slice.nameLength);
			// Устанавливаем значение поля
			field.value = string_view(this->_arena.data() + slice.valueOffset, slice.valueLength);
			// Устанавливаем признак чувствительного значения
			field.sensitive = slice.sensitive;
			// Дописываем декодированное поле
			output.push_back(field);
		}
	}
	/**
	 * Секция, сославшаяся на динамическую таблицу, подтверждается кодеру: только так
	 * он узнаёт, что удерживаемые ею записи освободились (RFC 9204 §4.4.1)
	 */
	if(required != 0){
		// Записываем инструкцию подтверждения секции
		h2::hpack::prefixed::encode(this->_output, sid, 7, 0x80);
		// Если подтверждение извещает кодера о большем числе вставок
		if(required > this->_acked)
			// Запоминаем количество вставок, о котором извещён кодер пира
			this->_acked = required;
	}
	// Выводим результат декодирования
	return status_t::OK;
}
/**
 * @brief Метод проверки превышения лимита списка полей последней секцией
 *
 * @return признак превышения лимита последней декодированной секцией
 *
 */
bool awh::http::h3::qpack::Decoder::overflowed() const noexcept {
	// Выводим признак превышения лимита списка полей
	return this->_overflow;
}
/**
 * @brief Метод отмены потока (RFC 9204 §4.4.2)
 *
 * @param sid идентификатор отменяемого потока
 *
 */
void awh::http::h3::qpack::Decoder::cancel(const uint64_t sid) noexcept {
	// Снимаем блокировку потока
	this->_blocked.erase(sid);
	// Записываем инструкцию отмены потока
	h2::hpack::prefixed::encode(this->_output, sid, 6, 0x40);
}
/**
 * @brief Метод получения накопленных инструкций потока декодера
 *
 * @return представление накопленных инструкций
 *
 */
string_view awh::http::h3::qpack::Decoder::pending() const noexcept {
	// Выводим представление накопленных инструкций
	return string_view(this->_output).substr(this->_consumed);
}
/**
 * @brief Метод отметки инструкций потока декодера как отправленных
 *
 * @param size количество отправленных октетов
 *
 */
void awh::http::h3::qpack::Decoder::consumePending(const size_t size) noexcept {
	// Наращиваем количество выданных наружу октетов
	this->_consumed += ::std::min(size, (this->_output.size() - this->_consumed));
	// Если буфер инструкций выдан наружу целиком
	if(this->_consumed >= this->_output.size()){
		// Выполняем очистку буфера инструкций
		this->_output.clear();
		// Сбрасываем количество выданных наружу октетов
		this->_consumed = 0;
	}
}
/**
 * @brief Метод сброса состояния декодера
 *
 */
void awh::http::h3::qpack::Decoder::clear() noexcept {
	// Выполняем очистку динамической таблицы
	this->_table.clear();
	// Выполняем очистку арены декодированных строк
	this->_arena.clear();
	// Выполняем сброс занятой части арены декодированных строк
	this->_arenaLength = 0;
	// Выполняем очистку буфера декодирования значения
	this->_scratch.clear();
	// Выполняем очистку буфера декодирования названия
	this->_name.clear();
	// Выполняем очистку срезов декодированных полей
	this->_slices.clear();
	// Выполняем очистку буфера инструкций
	this->_output.clear();
	// Сбрасываем количество выданных наружу октетов
	this->_consumed = 0;
	// Сбрасываем количество вставок, о котором извещён кодер пира
	this->_acked = 0;
	// Выполняем очистку списка заблокированных потоков
	this->_blocked.clear();
	// Сбрасываем признак превышения лимита списка полей
	this->_overflow = false;
}
/**
 * @brief Конструктор
 *
 * @param maxCapacity верхняя граница ёмкости динамической таблицы
 * @param maxBlocked  число потоков, которым разрешено ожидать пополнения таблицы
 *
 */
awh::http::h3::qpack::Decoder::Decoder(const uint64_t maxCapacity, const uint64_t maxBlocked) noexcept :
 _arenaLength(0), _consumed(0), _maxCapacity(maxCapacity), _maxBlocked(maxBlocked), _acked(0), _overflow(false) {}

/**
 * @brief Метод получения наименьшего абсолютного номера удерживаемой записи
 *
 * @return наименьший абсолютный номер удерживаемой ссылками записи
 *
 */
uint64_t awh::http::h3::qpack::Encoder::hold() const noexcept {
	// Наименьший абсолютный номер удерживаемой записи
	uint64_t result = UINT64_MAX;
	/**
	 * Выполняем перебор всех записей, удерживаемых неподтверждёнными секциями
	 */
	for(const auto & item : this->_refs){
		// Если запись всё ещё удерживается и оказалась старше найденной
		if((item.second > 0) && (item.first < result))
			// Запоминаем наименьший абсолютный номер удерживаемой записи
			result = item.first;
	}
	// Выводим наименьший абсолютный номер удерживаемой записи
	return result;
}
/**
 * @brief Метод подсчёта потоков, заблокированных ссылками на неполученные записи
 *
 * @return количество заблокированных потоков
 *
 */
size_t awh::http::h3::qpack::Encoder::blocking() const noexcept {
	// Количество заблокированных потоков
	size_t result = 0;
	/**
	 * Выполняем перебор всех потоков с неподтверждёнными секциями
	 */
	for(const auto & stream : this->_sections){
		/**
		 * Выполняем перебор неподтверждённых секций потока
		 */
		for(const auto & section : stream.second){
			/**
			 * Секция блокирует поток, если ссылается на записи, которых декодер
			 * пира ещё заведомо не получил
			 */
			if(section.required > this->_known){
				// Считаем заблокированный поток
				result++;
				// Прекращаем перебор секций потока
				break;
			}
		}
	}
	// Выводим количество заблокированных потоков
	return result;
}
/**
 * @brief Метод принятия решения об индексации поля
 *
 * @param name  название поля
 * @param value значение поля
 * @return      признак необходимости занести поле в таблицу
 *
 */
bool awh::http::h3::qpack::Encoder::indexable(string_view name, string_view value) noexcept {
	// Если адаптивная индексация выключена - индексируем всё подряд
	if(this->_history.empty())
		// Поле подлежит индексации
		return true;
	// Вычисляем хеш пары название-значение
	const uint32_t hash = static_cast <uint32_t> (::pairHash(::std::hash <string_view> {}(name), value));
	/**
	 * Ёмкость кольца на единицу меньше выделенного места: последняя ячейка
	 * отведена под ограничитель перебора и в историю не входит
	 */
	const size_t capacity = (this->_history.size() - 1);
	// Позиция, до которой кольцо заполнено
	const size_t last = (this->_historyWrapped ? capacity : this->_historyIndex);
	/**
	 * Ставим искомый хеш ограничителем за концом заполненной части: перебор
	 * тогда обходится без проверки границы на каждом шаге и останавливается
	 * либо на совпадении, либо на ограничителе
	 */
	this->_history[last] = hash;
	// Позиция найденного совпадения
	size_t position = 0;
	/**
	 * Выполняем поиск хеша среди ранее встреченных
	 */
	while(this->_history[position] != hash)
		// Переходим к следующей позиции кольца
		position++;
	// Запоминаем хеш поля в кольце
	this->_history[this->_historyIndex] = hash;
	// Продвигаем позицию записи кольца
	this->_historyIndex = ((this->_historyIndex + 1) % capacity);
	// Если кольцо заполнено целиком - запоминаем это
	this->_historyWrapped = (this->_historyWrapped || (this->_historyIndex == 0));
	/**
	 * Индексируем поле, если оно уже встречалось в пределах кольца. Пока кольцо
	 * не заполнено, индексируем всё: на старте соединения истории ещё нет, и отказ
	 * от индексации лишил бы таблицу как раз тех полей, которые повторяются
	 * в каждом запросе
	 */
	return ((position < last) || !this->_historyWrapped);
}
/**
 * @brief Метод записи строки литералом либо Huffman
 *
 * @param output      выходной буфер
 * @param input       записываемая строка
 * @param prefixBits  размер префикса длины в битах
 * @param prefixValue значение старших бит первого байта
 * @param useHuffman  применять Huffman-кодирование
 *
 */
void awh::http::h3::qpack::Encoder::encodeString(string & output, string_view input, const uint8_t prefixBits, const uint8_t prefixValue, const bool useHuffman) noexcept {
	// Вычисляем длину строки после Huffman-кодирования
	const size_t length = (useHuffman ? h2::hpack::huffman::length(input) : 0);
	/**
	 * Huffman применяется, только если он действительно короче литерала: на коротких
	 * строках из редких символов кодирование удлиняет строку
	 */
	if(useHuffman && (length < input.size())){
		// Записываем длину закодированной строки с признаком Huffman-кодирования
		h2::hpack::prefixed::encode(output, length, prefixBits, static_cast <uint8_t> (prefixValue | static_cast <uint8_t> (1u << prefixBits)));
		// Записываем строку, закодированную Huffman
		h2::hpack::huffman::encode(input, output);
	// Если строка записывается литералом
	} else {
		// Записываем длину строки
		h2::hpack::prefixed::encode(output, input.size(), prefixBits, prefixValue);
		// Записываем саму строку
		output.append(input);
	}
}
/**
 * @brief Метод записи инструкции пополнения динамической таблицы
 *
 * @param name       название поля
 * @param value      значение поля
 * @param nameStatic индекс названия в статической таблице либо STATIC_TABLE_SIZE
 * @param nameEntry  абсолютный номер названия в динамической таблице либо UINT64_MAX
 * @param useHuffman применять Huffman-кодирование к строкам
 *
 */
void awh::http::h3::qpack::Encoder::insertInstruction(string_view name, string_view value, const size_t nameStatic, const uint64_t nameEntry, const bool useHuffman) noexcept {
	// Если название есть в статической таблице (RFC 9204 §4.3.2)
	if(nameStatic < STATIC_TABLE_SIZE){
		// Записываем номер названия со ссылкой на статическую таблицу
		h2::hpack::prefixed::encode(this->_output, nameStatic, 6, 0xC0);
		// Записываем значение поля
		this->encodeString(this->_output, value, 7, 0x00, useHuffman);
	/**
	 * Если название есть в динамической таблице: номер задаётся относительно точки
	 * вставки, а не базы секции - у потока кодера своей базы нет
	 */
	} else if((nameEntry != UINT64_MAX) && (this->_table.at(nameEntry) != nullptr)) {
		// Записываем номер названия со ссылкой на динамическую таблицу
		h2::hpack::prefixed::encode(this->_output, (this->_table.inserts() - nameEntry - 1), 6, 0x80);
		// Записываем значение поля
		this->encodeString(this->_output, value, 7, 0x00, useHuffman);
	// Если название передаётся литералом (RFC 9204 §4.3.3)
	} else {
		// Записываем название поля
		this->encodeString(this->_output, name, 5, 0x40, useHuffman);
		// Записываем значение поля
		this->encodeString(this->_output, value, 7, 0x00, useHuffman);
	}
}
/**
 * @brief Метод кодирования секции полей из представлений
 *
 * @param sid        идентификатор потока, которому принадлежит секция
 * @param fields     кодируемые поля
 * @param output     выходной буфер секции полей
 * @param useHuffman применять Huffman-кодирование к строкам
 *
 */
void awh::http::h3::qpack::Encoder::encodeSection(const uint64_t sid, const vector <field_view_t> & fields, string & output, const bool useHuffman) noexcept {
	// Сбрасываем размер закодированного списка полей до сжатия
	this->_listSize = 0;
	// Выполняем очистку буфера сборки представлений полей
	this->_scratch.clear();
	/**
	 * База относительной нумерации фиксируется на начало секции: записи, вставленные
	 * во время кодирования, получают номера не ниже базы и адресуются представлениями
	 * с нумерацией за базой
	 */
	const uint64_t base = this->_table.inserts();
	// Требуемое число вставок для разбора секции
	uint64_t required = 0;
	/**
	 * Записи, на которые ссылается секция
	 *
	 * Накопитель переиспользуется между секциями: его ёмкость выживает очистку,
	 * поэтому в установившемся режиме роста списка ссылок аллокатор не стоит
	 * ни одного обращения
	 */
	vector <uint64_t> & refs = this->_references;
	// Выполняем очистку накопителя ссылок секции
	refs.clear();
	// Наименьший абсолютный номер записи, которую нельзя вытеснять
	uint64_t holding = this->hold();
	// Признак того, что поток уже заблокирован собственными секциями
	bool self = false;
	// Выполняем поиск неподтверждённых секций потока
	auto stream = this->_sections.find(sid);
	// Если неподтверждённые секции потока найдены
	if(stream != this->_sections.end()){
		/**
		 * Выполняем перебор неподтверждённых секций потока
		 */
		for(const auto & section : stream->second){
			// Если секция ссылается на записи, которых декодер пира ещё не получил
			if(section.required > this->_known){
				// Запоминаем, что поток уже заблокирован
				self = true;
				// Прекращаем перебор секций потока
				break;
			}
		}
	}
	/**
	 * Заблокировать поток можно, только если он уже заблокирован либо если общее
	 * число заблокированных потоков не достигло анонсированной пиром границы
	 */
	const bool blockable = (self || (static_cast <uint64_t> (this->blocking()) < this->_maxBlocked));
	/**
	 * @brief Функция записи ссылки на запись динамической таблицы
	 *
	 * @param absolute абсолютный номер записи
	 *
	 */
	auto reference = [&](const uint64_t absolute) noexcept -> void {
		// Если запись лежит до базы секции (RFC 9204 §4.5.2)
		if(absolute < base)
			// Записываем ссылку с нумерацией относительно базы
			h2::hpack::prefixed::encode(this->_scratch, (base - absolute - 1), 6, 0x80);
		// Если запись вставлена уже после фиксации базы (RFC 9204 §4.5.3)
		else h2::hpack::prefixed::encode(this->_scratch, (absolute - base), 4, 0x10);
		// Если ссылка подняла требуемое число вставок
		if((absolute + 1) > required)
			// Запоминаем требуемое число вставок
			required = (absolute + 1);
		// Запоминаем удерживаемую секцией запись
		refs.push_back(absolute);
		// Если запись оказалась старше уже удерживаемых
		if(absolute < holding)
			// Понижаем границу удержания записей
			holding = absolute;
	};
	/**
	 * @brief Функция записи ссылки на название записи динамической таблицы
	 *
	 * @param absolute  абсолютный номер записи
	 * @param sensitive признак чувствительного значения
	 *
	 */
	auto referenceName = [&](const uint64_t absolute, const bool sensitive) noexcept -> void {
		// Если запись лежит до базы секции (RFC 9204 §4.5.4)
		if(absolute < base)
			// Записываем ссылку с нумерацией относительно базы
			h2::hpack::prefixed::encode(this->_scratch, (base - absolute - 1), 4, static_cast <uint8_t> (0x40 | (sensitive ? 0x20 : 0x00)));
		// Если запись вставлена уже после фиксации базы (RFC 9204 §4.5.5)
		else h2::hpack::prefixed::encode(this->_scratch, (absolute - base), 3, static_cast <uint8_t> (sensitive ? 0x08 : 0x00));
		// Если ссылка подняла требуемое число вставок
		if((absolute + 1) > required)
			// Запоминаем требуемое число вставок
			required = (absolute + 1);
		// Запоминаем удерживаемую секцией запись
		refs.push_back(absolute);
		// Если запись оказалась старше уже удерживаемых
		if(absolute < holding)
			// Понижаем границу удержания записей
			holding = absolute;
	};
	/**
	 * Выполняем кодирование всех полей секции
	 */
	for(const auto & field : fields){
		// Определяем чувствительность значения поля
		const bool sensitive = (field.sensitive || (this->_sensitiveHeuristic && ::isSensitiveName(field.name)));
		// Наращиваем размер списка полей по правилу RFC 9114 §4.2.2
		this->_listSize += (static_cast <uint64_t> (field.name.size()) + static_cast <uint64_t> (field.value.size()) + ENTRY_OVERHEAD);
		// Индекс полного совпадения в статической таблице
		size_t staticIndex = 0;
		// Индекс совпадения по названию в статической таблице
		size_t staticName = STATIC_TABLE_SIZE;
		// Выполняем поиск поля в статической таблице
		if(stat::find(field.name, field.value, staticIndex, staticName)){
			// Записываем ссылку на запись статической таблицы (RFC 9204 §4.5.2)
			h2::hpack::prefixed::encode(this->_scratch, staticIndex, 6, 0xC0);
			// Переходим к следующему полю секции
			continue;
		}
		// Абсолютный номер полного совпадения в динамической таблице
		uint64_t entry = 0;
		// Абсолютный номер совпадения по названию в динамической таблице
		uint64_t entryName = UINT64_MAX;
		// Признак найденного полного совпадения в динамической таблице
		bool found = false;
		// Если значение поля не является чувствительным
		if(!sensitive)
			// Выполняем поиск поля в динамической таблице
			found = this->_table.find(field.name, field.value, entry, entryName);
		/**
		 * Ссылка на запись, которой декодер пира ещё не получил, блокирует поток:
		 * пользоваться такой записью можно, только если блокировка разрешена
		 */
		if(found && (blockable || (entry < this->_known))){
			// Записываем ссылку на запись динамической таблицы
			reference(entry);
			// Переходим к следующему полю секции
			continue;
		}
		/**
		 * Решаем, стоит ли заносить поле в динамическую таблицу. Вытеснить запись,
		 * удерживаемую неподтверждённой секцией, нельзя - иначе декодер пира
		 * разобрал бы ссылку на неё неверно
		 */
		const bool insert = (
			!sensitive && (this->_table.capacity() > 0) &&
			this->indexable(field.name, field.value) &&
			this->_table.insertable(field.name, field.value, holding)
		);
		// Если поле заносится в динамическую таблицу
		if(insert){
			// Записываем инструкцию пополнения таблицы в поток кодера
			this->insertInstruction(field.name, field.value, staticName, entryName, useHuffman);
			// Выполняем вставку поля в динамическую таблицу
			if(this->_table.add(field.name, field.value)){
				// Вычисляем абсолютный номер вставленной записи
				const uint64_t absolute = (this->_table.inserts() - 1);
				// Если поток разрешено заблокировать ссылкой на свежую запись
				if(blockable){
					// Записываем ссылку на вставленную запись
					reference(absolute);
					// Переходим к следующему полю секции
					continue;
				}
				/**
				 * Ссылаться на свежую запись нельзя, но вставка не напрасна: следующие
				 * секции воспользуются ею, когда декодер пира подтвердит получение
				 */
				this->_table.findName(field.name, entryName);
			}
		}
		// Если название поля есть в статической таблице (RFC 9204 §4.5.4)
		if(staticName < STATIC_TABLE_SIZE)
			// Записываем ссылку на название записи статической таблицы
			h2::hpack::prefixed::encode(this->_scratch, staticName, 4, static_cast <uint8_t> (0x50 | (sensitive ? 0x20 : 0x00)));
		/**
		 * Если название поля есть в динамической таблице и ссылка на него допустима
		 */
		else if((entryName != UINT64_MAX) && (this->_table.at(entryName) != nullptr) && (blockable || (entryName < this->_known)))
			// Записываем ссылку на название записи динамической таблицы
			referenceName(entryName, sensitive);
		// Если название поля передаётся литералом (RFC 9204 §4.5.6)
		else this->encodeString(this->_scratch, field.name, 3, static_cast <uint8_t> (0x20 | (sensitive ? 0x10 : 0x00)), useHuffman);
		// Записываем значение поля
		this->encodeString(this->_scratch, field.value, 7, 0x00, useHuffman);
	}
	// Закодированное требуемое число вставок
	uint64_t encoded = 0;
	// Если секция ссылается на динамическую таблицу
	if(required != 0){
		// Вычисляем ёмкость таблицы в записях
		const uint64_t entries = (this->_maxCapacity / ENTRY_OVERHEAD);
		/**
		 * Требуемое число вставок передаётся по модулю удвоенной ёмкости таблицы
		 * в записях (RFC 9204 §4.5.1.1)
		 */
		if(entries > 0)
			// Вычисляем закодированное требуемое число вставок
			encoded = ((required % (entries * 2)) + 1);
	}
	// Записываем требуемое число вставок
	h2::hpack::prefixed::encode(output, encoded, 8, 0x00);
	// Если база лежит не ниже требуемого числа вставок
	if(base >= required)
		// Записываем разницу базы с положительным знаком
		h2::hpack::prefixed::encode(output, (base - required), 7, 0x00);
	// Если база лежит ниже требуемого числа вставок
	else h2::hpack::prefixed::encode(output, (required - base - 1), 7, 0x80);
	// Дописываем представления полей секции
	output.append(this->_scratch);
	/**
	 * Секция, не сославшаяся на динамическую таблицу, декодером не подтверждается,
	 * поэтому и в учёт неподтверждённых секций не попадает (RFC 9204 §4.4.1)
	 */
	if(required != 0){
		// Собираемый учёт отправленной секции
		section_t section;
		// Устанавливаем требуемое число вставок
		section.required = required;
		/**
		 * Записи, удерживаемые секцией, копируются в её учёт: накопитель
		 * переиспользуется следующей секцией и отдать его во владение нельзя
		 */
		section.refs = refs;
		/**
		 * Выполняем учёт ссылок секции на записи таблицы
		 */
		for(const auto absolute : section.refs)
			// Наращиваем счётчик ссылок на запись
			this->_refs[absolute]++;
		// Дописываем секцию в список неподтверждённых секций потока
		this->_sections[sid].push_back(::std::move(section));
	}
}
/**
 * @brief Метод получения собственной динамической таблицы
 *
 * @return собственная динамическая таблица
 *
 */
awh::http::h3::qpack::dynamic_table_t & awh::http::h3::qpack::Encoder::table() noexcept {
	// Выводим собственную динамическую таблицу
	return this->_table;
}
/**
 * @brief Метод установки верхней границы ёмкости таблицы, анонсированной пиром
 *
 * @param capacity верхняя граница ёмкости таблицы
 *
 */
void awh::http::h3::qpack::Encoder::maxCapacity(const uint64_t capacity) noexcept {
	// Устанавливаем верхнюю границу ёмкости таблицы
	this->_maxCapacity = capacity;
	// Устанавливаем ёмкость собственной динамической таблицы
	this->_table.setCapacity(capacity);
	// Включаем сопровождение индекса записей, если таблица используется
	this->_table.indexing(capacity > 0);
	/**
	 * Ёмкость кольца истории считается от размера таблицы, поэтому пересобирается
	 * вместе с ней
	 */
	if(!this->_history.empty())
		// Пересобираем кольцо истории под новый размер таблицы
		this->adaptiveIndexing(true);
	/**
	 * Извещаем декодер пира о выбранной ёмкости: без инструкции он держал бы
	 * таблицу нулевого размера и не принял бы ни одной вставки (RFC 9204 §4.3.1).
	 * Нулевая ёмкость и так действует по умолчанию, поэтому анонсировать её незачем
	 */
	if(capacity > 0)
		// Записываем инструкцию изменения ёмкости таблицы
		h2::hpack::prefixed::encode(this->_output, capacity, 5, 0x20);
}
/**
 * @brief Метод установки числа потоков, которым пир разрешил ожидать пополнения таблицы
 *
 * @param count число потоков
 *
 */
void awh::http::h3::qpack::Encoder::maxBlocked(const uint64_t count) noexcept {
	// Устанавливаем число потоков, которым пир разрешил ожидать пополнения таблицы
	this->_maxBlocked = count;
}
/**
 * @brief Метод получения размера закодированного списка полей до сжатия
 *
 * @return размер списка полей последней секции до сжатия
 *
 */
uint64_t awh::http::h3::qpack::Encoder::listSize() const noexcept {
	// Выводим размер закодированного списка полей до сжатия
	return this->_listSize;
}
/**
 * @brief Метод управления автоматическим определением чувствительных полей
 *
 * @param mode режим автоматического определения
 *
 */
void awh::http::h3::qpack::Encoder::sensitiveHeuristic(const bool mode) noexcept {
	// Устанавливаем режим автоматического определения чувствительных полей
	this->_sensitiveHeuristic = mode;
}
/**
 * @brief Метод управления адаптивной индексацией полей
 *
 * @param mode режим адаптивной индексации
 *
 */
void awh::http::h3::qpack::Encoder::adaptiveIndexing(const bool mode) noexcept {
	// Сбрасываем позицию записи кольца хешей
	this->_historyIndex = 0;
	// Сбрасываем признак заполненности кольца хешей
	this->_historyWrapped = false;
	// Если адаптивная индексация выключается
	if(!mode){
		// Выполняем очистку кольца хешей
		this->_history.clear();
		// Выходим из метода
		return;
	}
	/**
	 * Ёмкость кольца выбрана по числу записей, помещающихся в таблицу: история
	 * длиннее таблицы удерживала бы решение об индексации для полей, которые
	 * из таблицы давно вытеснены. Последняя ячейка отведена под ограничитель
	 * перебора и в ёмкость не входит
	 */
	const size_t capacity = static_cast <size_t> (::std::max <uint64_t> (1, (this->_maxCapacity / (ENTRY_OVERHEAD * 3))));
	// Выделяем место под кольцо хешей с ограничителем перебора
	this->_history.assign(capacity + 1, 0);
}
/**
 * @brief Метод обработки инструкций потока декодера (RFC 9204 §4.4)
 *
 * @param data     входной буфер потока декодера
 * @param consumed количество разобранных октетов
 * @param error    код ошибки протокола
 * @return         результат обработки (OK/ERROR)
 *
 */
awh::http::h3::status_t awh::http::h3::qpack::Encoder::decodeDecoderStream(string_view data, size_t & consumed, error_t & error) noexcept {
	// Сбрасываем количество разобранных октетов
	consumed = 0;
	// Получаем указатель на входной буфер
	const uint8_t * buffer = reinterpret_cast <const uint8_t *> (data.data());
	// Получаем размер входного буфера
	const size_t size = data.size();
	/**
	 * Выполняем разбор всех целых инструкций входного буфера
	 */
	while(consumed < size){
		// Получаем первый октет очередной инструкции
		const uint8_t byte = buffer[consumed];
		// Разбираемое значение инструкции
		uint64_t value = 0;
		// Количество прочитанных байт
		size_t used = 0;
		// Результат чтения значения инструкции
		h2::status_t status = h2::status_t::OK;
		/**
		 * Подтверждение секции (RFC 9204 §4.4.1)
		 */
		if((byte & 0x80) != 0){
			// Выполняем чтение идентификатора потока
			status = h2::hpack::prefixed::decode((buffer + consumed), (size - consumed), 7, value, used);
			// Если данных для разбора инструкции ещё недостаточно
			if(status == h2::status_t::INCOMPLETE)
				// Прекращаем разбор инструкций
				break;
			// Если при чтении идентификатора потока произошло переполнение
			else if(status != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Выполняем поиск неподтверждённых секций потока
			auto stream = this->_sections.find(value);
			/**
			 * Подтверждение секции, которой мы не отправляли, - ошибка: счётчики
			 * кодера и декодера разошлись (RFC 9204 §4.4.1)
			 */
			if((stream == this->_sections.end()) || stream->second.empty()){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Получаем самую старую неподтверждённую секцию потока
			const section_t & section = stream->second.front();
			// Если подтверждение извещает о большем числе полученных вставок
			if(section.required > this->_known)
				// Запоминаем число вставок, заведомо полученных декодером пира
				this->_known = section.required;
			/**
			 * Выполняем снятие ссылок секции на записи таблицы
			 */
			for(const auto absolute : section.refs){
				// Выполняем поиск счётчика ссылок на запись
				auto i = this->_refs.find(absolute);
				// Если счётчик ссылок на запись найден
				if(i != this->_refs.end()){
					// Уменьшаем счётчик ссылок на запись
					if((--i->second) == 0)
						// Удаляем исчерпанный счётчик ссылок
						this->_refs.erase(i);
				}
			}
			// Удаляем подтверждённую секцию
			stream->second.pop_front();
			// Если неподтверждённых секций у потока не осталось
			if(stream->second.empty())
				// Удаляем поток из списка
				this->_sections.erase(stream);
		/**
		 * Отмена потока (RFC 9204 §4.4.2)
		 */
		} else if((byte & 0x40) != 0) {
			// Выполняем чтение идентификатора потока
			status = h2::hpack::prefixed::decode((buffer + consumed), (size - consumed), 6, value, used);
			// Если данных для разбора инструкции ещё недостаточно
			if(status == h2::status_t::INCOMPLETE)
				// Прекращаем разбор инструкций
				break;
			// Если при чтении идентификатора потока произошло переполнение
			else if(status != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Выполняем снятие ссылок отменённого потока
			this->cancel(value);
		/**
		 * Приращение счётчика вставок (RFC 9204 §4.4.3)
		 */
		} else {
			// Выполняем чтение приращения счётчика вставок
			status = h2::hpack::prefixed::decode((buffer + consumed), (size - consumed), 6, value, used);
			// Если данных для разбора инструкции ещё недостаточно
			if(status == h2::status_t::INCOMPLETE)
				// Прекращаем разбор инструкций
				break;
			// Если при чтении приращения произошло переполнение
			else if(status != h2::status_t::OK){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			/**
			 * Нулевое приращение бессмысленно, а приращение сверх числа отправленных
			 * вставок означает, что декодер насчитал больше, чем мы отправили
			 */
			if((value == 0) || (value > (this->_table.inserts() - this->_known))){
				// Устанавливаем код ошибки протокола
				error = error_t::QPACK_DECODER_STREAM_ERROR;
				// Выводим результат обработки
				return status_t::ERROR;
			}
			// Наращиваем число вставок, заведомо полученных декодером пира
			this->_known += value;
		}
		// Отмечаем инструкцию разобранной целиком
		consumed += used;
	}
	// Выводим результат обработки
	return status_t::OK;
}
/**
 * @brief Метод кодирования секции полей
 *
 * @param sid        идентификатор потока, которому принадлежит секция
 * @param fields     поля (псевдо-заголовки должны идти первыми)
 * @param output     выходной буфер секции полей
 * @param useHuffman применять Huffman-кодирование к строкам
 *
 */
void awh::http::h3::qpack::Encoder::encode(const uint64_t sid, const vector <field_t> & fields, string & output, const bool useHuffman) noexcept {
	// Выполняем очистку списка представлений кодируемых полей
	this->_views.clear();
	// Выделяем место под представления кодируемых полей
	this->_views.reserve(fields.size());
	/**
	 * Выполняем сбор представлений всех кодируемых полей
	 */
	for(const auto & field : fields){
		// Собираемое представление поля
		field_view_t view;
		// Устанавливаем название поля
		view.name = field.name;
		// Устанавливаем значение поля
		view.value = field.value;
		// Устанавливаем признак чувствительного значения
		view.sensitive = field.sensitive;
		// Дописываем представление поля
		this->_views.push_back(view);
	}
	// Выполняем кодирование секции полей
	this->encodeSection(sid, this->_views, output, useHuffman);
}
/**
 * @brief Метод кодирования секции полей из декодированного списка (перекодирование)
 *
 * @param sid        идентификатор потока, которому принадлежит секция
 * @param fields     декодированные поля
 * @param output     выходной буфер секции полей
 * @param useHuffman применять Huffman-кодирование к строкам
 *
 */
void awh::http::h3::qpack::Encoder::encode(const uint64_t sid, const vector <field_view_t> & fields, string & output, const bool useHuffman) noexcept {
	// Выполняем кодирование секции полей
	this->encodeSection(sid, fields, output, useHuffman);
}
/**
 * @brief Метод снятия ссылок отменённого потока
 *
 * @param sid идентификатор отменяемого потока
 *
 */
void awh::http::h3::qpack::Encoder::cancel(const uint64_t sid) noexcept {
	// Выполняем поиск неподтверждённых секций потока
	auto stream = this->_sections.find(sid);
	// Если неподтверждённых секций у потока нет
	if(stream == this->_sections.end())
		// Выходим из метода
		return;
	/**
	 * Выполняем перебор всех неподтверждённых секций потока
	 */
	for(const auto & section : stream->second){
		/**
		 * Выполняем снятие ссылок секции на записи таблицы
		 */
		for(const auto absolute : section.refs){
			// Выполняем поиск счётчика ссылок на запись
			auto i = this->_refs.find(absolute);
			// Если счётчик ссылок на запись найден
			if(i != this->_refs.end()){
				// Уменьшаем счётчик ссылок на запись
				if((--i->second) == 0)
					// Удаляем исчерпанный счётчик ссылок
					this->_refs.erase(i);
			}
		}
	}
	// Удаляем поток из списка неподтверждённых секций
	this->_sections.erase(stream);
}
/**
 * @brief Метод получения накопленных инструкций потока кодера
 *
 * @return представление накопленных инструкций
 *
 */
string_view awh::http::h3::qpack::Encoder::pending() const noexcept {
	// Выводим представление накопленных инструкций
	return string_view(this->_output).substr(this->_consumed);
}
/**
 * @brief Метод отметки инструкций потока кодера как отправленных
 *
 * @param size количество отправленных октетов
 *
 */
void awh::http::h3::qpack::Encoder::consumePending(const size_t size) noexcept {
	// Наращиваем количество выданных наружу октетов
	this->_consumed += ::std::min(size, (this->_output.size() - this->_consumed));
	// Если буфер инструкций выдан наружу целиком
	if(this->_consumed >= this->_output.size()){
		// Выполняем очистку буфера инструкций
		this->_output.clear();
		// Сбрасываем количество выданных наружу октетов
		this->_consumed = 0;
	}
}
/**
 * @brief Метод сброса состояния кодера
 *
 */
void awh::http::h3::qpack::Encoder::clear() noexcept {
	// Выполняем очистку динамической таблицы
	this->_table.clear();
	// Выполняем очистку буфера инструкций
	this->_output.clear();
	// Сбрасываем количество выданных наружу октетов
	this->_consumed = 0;
	// Выполняем очистку буфера сборки представлений полей
	this->_scratch.clear();
	// Выполняем очистку списка представлений кодируемых полей
	this->_views.clear();
	// Сбрасываем число вставок, заведомо полученных декодером пира
	this->_known = 0;
	// Выполняем очистку списка неподтверждённых секций
	this->_sections.clear();
	// Выполняем очистку счётчиков ссылок на записи таблицы
	this->_refs.clear();
	// Сбрасываем размер закодированного списка полей
	this->_listSize = 0;
	// Сбрасываем позицию записи кольца хешей
	this->_historyIndex = 0;
	// Сбрасываем признак заполненности кольца хешей
	this->_historyWrapped = false;
	/**
	 * Выполняем очистку кольца хешей: история прежнего соединения не имеет
	 * отношения к новому, а решения об индексации она бы исказила
	 */
	for(auto & item : this->_history)
		// Сбрасываем очередную ячейку кольца хешей
		item = 0;
}
/**
 * @brief Конструктор
 *
 * @param maxCapacity верхняя граница ёмкости динамической таблицы
 * @param maxBlocked  число потоков, которым разрешено ожидать пополнения таблицы
 *
 */
awh::http::h3::qpack::Encoder::Encoder(const uint64_t maxCapacity, const uint64_t maxBlocked) noexcept :
 _table(maxCapacity), _consumed(0), _maxCapacity(maxCapacity), _maxBlocked(maxBlocked),
 _known(0), _listSize(0), _historyIndex(0), _historyWrapped(false), _sensitiveHeuristic(true) {
	// Включаем сопровождение индекса записей, если таблица используется
	this->_table.indexing(maxCapacity > 0);
	// Включаем адаптивную индексацию полей
	this->adaptiveIndexing(true);
}
