/**
 * @file: frame.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

// Подключаем заголовочный файл
#include <ws/frame.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён WS
 */
using namespace ws;

/**
 * find Метод поиска типа сообщения
 */
void awh::ws::Message::find() noexcept {
	// Если код сообщения передан
	if(this->code > 0){
		// Выполняем поиск типа сообщений
		auto it = this->_codes.find(this->code);
		// Если тип сообщения найден, устанавливаем
		if(it != this->_codes.end()){
			// Устанавливаем тип сообщения
			this->type = it->second.first;
			// Если текст сообщения получен
			if(this->text.empty())
				// Устанавливаем текст сообщения
				this->text = it->second.second;
		}
	}
}
/**
 * init Метод инициализации модуля
 */
void awh::ws::Message::init() noexcept {
	// Выполняем установку расшифровки кодов ошибок
	this->_codes = {
		{1000, {"CLOSE_NORMAL", "Successful operation/regular socket shutdown"}},
		{1001, {"CLOSE_GOING_AWAY", "Client is leaving (browser tab closing)"}},
		{1002, {"CLOSE_PROTOCOL_ERROR", "Endpoint received a malformed frame"}},
		{1003, {"CLOSE_UNSUPPORTED", "Endpoint received an unsupported frame (e.g. binary-only endpoint received text frame)"}},
		{1004, {"CLOSE_RESERVED", "Reserved"}},
		{1005, {"CLOSED_NO_STATUS", "Expected close status, received none"}},
		{1006, {"CLOSE_ABNORMAL", "No close code frame has been receieved"}},
		{1007, {"CLOSE_UNSUPPORTED_PAYLOAD", "Endpoint received inconsistent message (e.g. malformed UTF-8)"}},
		{1008, {"CLOSE_POLICY_VIOLATION", "Generic code used for situations other than 1003 and 1009"}},
		{1009, {"CLOSE_TOO_LARGE", "Endpoint won't process large frame"}},
		{1010, {"CLOSE_MANDATORY_EXTENSION", "Client wanted an extension which server did not negotiate"}},
		{1011, {"CLOSE_SERVER_ERROR", "Internal server error while operating"}},
		{1012, {"CLOSE_SERVICE_RESTART", "Server/service is restarting"}},
		{1013, {"CLOSE_TRY_AGAIN_LATER", "Temporary server condition forced blocking client's request"}},
		{1014, {"CLOSE_BAD_GATEWAY", "Server acting as gateway received an invalid response"}},
		{1015, {"CLOSE_TLS_HANDSHAKE_FAIL", "Transport Layer Security handshake failure"}}
	};
}
/**
 * operator= Оператор установки текстового сообщения
 * @param text текст сообщения
 * @return     ссылка на контекст объекта
 */
awh::ws::Message & awh::ws::Message::operator = (const string & text) noexcept {
	// Если текст сообщения передан
	if(!text.empty())
		// Устанавливаем текст сообщения
		this->text = text;
	// Выводим контекст текущего объекта
	return (* this);
}
/**
 * operator= Оператор установки кода сообщения
 * @param code код сообщения
 * @return     ссылка на контекст объекта
 */
awh::ws::Message & awh::ws::Message::operator = (const uint16_t code) noexcept {
	// Если код сообщения передан
	if(code > 0){
		// Устанавливаем код сообщения
		this->code = code;
		// Выполняем поиск сообщения
		this->find();
	}
	// Выводим контекст текущего объекта
	return (* this);
}
/**
 * Message Конструктор
 */
awh::ws::Message::Message() noexcept : code(0), text{""}, type{""} {
	// Выполняем инициализацию модуля
	this->init();
}
/**
 * Message Конструктор
 * @param code код сообщения
 */
awh::ws::Message::Message(const uint16_t code) noexcept : code(code), text{""}, type{""} {
	// Выполняем инициализацию модуля
	this->init();
	// Выполняем поиск сообщения
	this->find();
}
/**
 * Message Конструктор
 * @param text текст сообщения
 */
awh::ws::Message::Message(const string & text) noexcept : code(0), text{text}, type{""} {
	// Выполняем инициализацию модуля
	this->init();
	// Выполняем поиск сообщения
	this->find();
}
/**
 * Message Конструктор
 * @param code код сообщения
 * @param text текст сообщения
 */
awh::ws::Message::Message(const uint16_t code, const string & text) noexcept : code(code), text{text}, type{""} {
	// Выполняем инициализацию модуля
	this->init();
	// Выполняем поиск сообщения
	this->find();
}
/**
 * head Функция извлечения заголовка фрейма
 * @param head   объект для извлечения заголовка
 * @param buffer буфер с данными заголовка
 * @param size   размер передаваемого буфера
 * @param log    объект для работы с логами
 */
static void head(frame_t::head_t & head, const void * buffer, const size_t size, const log_t * log) noexcept {
	// Если данные переданы
	if((buffer != nullptr) && (log != nullptr) && (size >= 2)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем является ли сообщение последним
			head.fin = (reinterpret_cast <const uint8_t *> (buffer)[0] & 0x080);
			// Получаем наличие маски
			head.mask = (reinterpret_cast <const uint8_t *> (buffer)[1] & 0x080);
			// Определяем байты расширенного протокола
			head.rsv[0] = (reinterpret_cast <const uint8_t *> (buffer)[0] & 0x040);
			head.rsv[1] = (reinterpret_cast <const uint8_t *> (buffer)[0] & 0x020);
			head.rsv[2] = (reinterpret_cast <const uint8_t *> (buffer)[0] & 0x010);
			// Получаем малый размер полезной нагрузки
			head.payload = static_cast <uint64_t> (reinterpret_cast <const uint8_t *> (buffer)[1] & 0x07F);
			// Получаем опкод
			head.optcode = static_cast <frame_t::opcode_t> (reinterpret_cast <const uint8_t *> (buffer)[0] & 0x00F);
			// Если размер пересылаемых данных, имеет малый размер
			if(head.payload < 0x07E)
				// Получаем размер блока заголовков
				head.size = 2;
			// Если размер пересылаемых данных, имеет более высокий размер
			else if((head.payload == 0x07E) && (size >= 4)) {
				// Получаем размер блока заголовков
				head.size = 4;
				// Размер полезной нагрузки
				uint16_t size = 0;
				// Получаем размер данных
				::memcpy(&size, reinterpret_cast <const uint8_t *> (buffer) + 2, sizeof(size));
				// Преобразуем сетевой порядок расположения байтов
				head.payload = static_cast <uint64_t> (ntohs(size));
			// Если размер пересылаемых данных, имеет очень большой размер
			} else if((head.payload == 0x07F) && (size >= 10)) {
				// Получаем размер блока заголовков
				head.size = 10;
				// Получаем размер данных
				::memcpy(&head.payload, reinterpret_cast <const uint8_t *> (buffer) + 2, sizeof(head.payload));
				// Преобразуем сетевой порядок расположения байтов
				head.payload = static_cast <uint64_t> (ntohl(head.payload));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * frame Шаблон функции создания бинарного фрейма
 * @tparam T тип контейнера
 */
template <typename T>
/**
 * frame Функция создания бинарного фрейма
 * @param payload бинарный буфер фрейма
 * @param buffer  бинарные данные полезной нагрузки
 * @param size    размер передаваемого буфера
 * @param mask    флаг выполнения маскировки сообщения
 * @param log     объект для работы с логами
 */
static void frame(T & payload, const void * buffer, const size_t size, const bool mask, const log_t * log) noexcept {
	// Если данные переданы
	if(!payload.empty() && (buffer != nullptr) && (log != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Размер смещения в буфере и размер передаваемых данных
			uint8_t offset = 0;
			// Если размер строки меньше 126 байт, значит строка умещается во второй байт
			if(size < 0x07E){
				// Устанавливаем смещение в буфере
				offset = 2;
				// Устанавливаем размер строки
				payload.back() = (static_cast <uint8_t> (mask ? 0x080 : 0x00) | (0x07F & size));
			// Если строка не помещается во второй байт
			} else if(size < 0x10000) {
				// Устанавливаем смещение в буфере
				offset = 4;
				// Заполняем второй байт максимальным значением
				payload.back() = (static_cast <uint8_t> (mask ? 0x080 : 0x00) | (0x07F & 0x07E));
				// Увеличиваем память ещё на два байта
				payload.resize(offset, 0x00);
				// Выполняем перерасчёт размера передаваемых данных
				const uint16_t bytes = static_cast <uint64_t> (htons(static_cast <uint16_t> (size)));
				// Устанавливаем размер строки в следующие 2 байта
				::memcpy(payload.data() + 2, &bytes, sizeof(bytes));
			// Если сообщение очень большого размера
			} else {
				// Устанавливаем смещение в буфере
				offset = 10;
				// Заполняем второй байт максимальным значением
				payload.back() = (static_cast <uint8_t> (mask ? 0x080 : 0x00) | 0x07F);
				// Увеличиваем память ещё на восемь байт
				payload.resize(offset, 0x00);
				// Выполняем перерасчёт размера передаваемых данных
				const uint64_t bytes = static_cast <uint64_t> (htonl(size));
				// Устанавливаем размер строки в следующие 8 байт
				::memcpy(payload.data() + 2, &bytes, sizeof(bytes));
			}
			// Если нужно выполнить маскировку сообщения
			if(mask){
				// Получаем генератор случайных чисел
				random_device randev;
				// Бинарные данные маски
				vector <uint8_t> mask(4);
				// Подключаем генератор к двигателю
				mt19937 engine {randev()};
				// Устанавливаем диапазон генератора случайных чисел
				uniform_int_distribution <uint8_t> dist {0, 255};
				// Выполняем заполнение маски случайными числами
				generate(mask.begin(), mask.end(), [&dist, &engine]() noexcept -> uint8_t {
					// Выполняем генерирование случайного числа
					return dist(engine);
				});
				// Выполняем перебор всех байт передаваемых данных
				for(size_t i = 0; i < size; i++)
					// Выполняем шифрование данных
					const_cast <uint8_t *> (reinterpret_cast <const uint8_t *> (buffer))[i] ^= mask[i % 4];
				// Увеличиваем память ещё на четыре байта
				payload.resize(offset + 4, 0x00);
				// Устанавливаем сгенерированную маску
				::memcpy(payload.data() + offset, mask.data(), mask.size());
			}
			// Выполняем копирования оставшихся данных в буфер
			payload.insert(payload.end(), reinterpret_cast <const uint8_t *> (buffer), reinterpret_cast <const uint8_t *> (buffer) + size);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, mask), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * message Метод создание фрейма сообщения
 * @param mess данные сообщения
 * @return     бинарные данные фрейма
 */
vector <char> awh::ws::Frame::message(const mess_t & mess) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если сообщение передано
	if(mess.code > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Увеличиваем память на 4 байта
			result.resize(4, 0x00);
			// Устанавливаем первый байт
			result.front() = (static_cast <char> (0x080) | (0x00F & static_cast <char> (opcode_t::CLOSE)));
			// Размер смещения в буфере
			uint16_t offset = 0;
			// Размер передаваемых данных
			uint64_t size = static_cast <uint64_t> (mess.text.size());
			// Если размер строки меньше 126 байт, значит строка умещается во второй байт
			if(size < 0x07E){
				// Устанавливаем смещение в буфере
				offset = 2;
				// Устанавливаем размер строки
				result.at(1) = (static_cast <char> (0x07F & (size + 2)));
			// Если строка не помещается во второй байт
			} else if(size < 0x10000) {
				// Устанавливаем смещение в буфере
				offset = 4;
				// Увеличиваем память ещё на два байта
				result.resize(offset + 2, 0x00);
				// Заполняем второй байт максимальным значением
				result.at(1) = (static_cast <char> (0x07F & 0x07E));
				// Выполняем перерасчёт размера передаваемых данных
				const uint16_t bytes = static_cast <uint64_t> (htons(static_cast <uint16_t> (size + 2)));
				// Устанавливаем размер строки в следующие 2 байта
				::memcpy(result.data() + 2, &bytes, sizeof(bytes));
			}
			// Получаем код сообщения
			const uint16_t code = htons(mess.code);
			// Устанавливаем код сообщения
			::memcpy(result.data() + offset, &code, sizeof(code));
			// Если данные текстового сообщения получены
			if(!mess.text.empty())
				// Выполняем копирования оставшихся данных в буфер
				result.insert(result.end(), mess.text.begin(), mess.text.end());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * message Метод извлечения сообщения из фрейма
 * @param buffer бинарные данные сообщения
 * @param size   размер буфера данных сообщения
 * @return       сообщение в текстовом виде
 */
awh::ws::mess_t awh::ws::Frame::message(const void * buffer, const size_t size) const noexcept {
	// Результат работы функции
	mess_t result;
	// Если данные переданы
	if((buffer != nullptr) && (size >= sizeof(result.code))){
		/**
		 * Подробнее: https://github.com/Luka967/websocket-close-codes
		 */
		// Считываем код ошибки
		::memcpy(&result.code, buffer, sizeof(result.code));
		// Преобразуем сетевой порядок расположения байтов
		result = ntohs(result.code);
		// Если коды ошибок соответствуют
		if((result.code > 0) && (result.code <= 4999)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Если текст сообщения существует
				if(size > sizeof(result.code))
					// Извлекаем текст сообщения
					result.text.assign(reinterpret_cast <const char *> (buffer) + sizeof(result.code), size);
				// Иначе запоминаем, что текст не установлен
				else result.text.clear();
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Устанавливаем текст ошибки
				result = this->_fmk->format("%s", error.what());
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		// Запоминаем размер смещения
		} else result = 1007;
	}
	// Выводим результат
	return result;
}
/**
 * message Метод извлечения сообщения из заголовка фрейма
 * @param head       заголовки фрейма
 * @param code       код сообщения
 * @param compressed флаг сжатых ожидаемых данных
 * @return           сообщение в текстовом виде
 */
awh::ws::mess_t awh::ws::Frame::message(const head_t & head, const uint16_t code, const bool compressed) const noexcept {
	// Проверяем состояние флагов RSV2 и RSV3
	if(head.rsv[1] || head.rsv[2])
		// Создаём сообщение
		return mess_t(1002, "RSV2 and RSV3 must be clear");
	// Если флаг компресси включён а данные пришли не сжатые
	else if(head.rsv[0] && (!compressed || (head.optcode == opcode_t::CONTINUATION) ||
	       ((static_cast <uint8_t> (head.optcode) > 0x007) && (static_cast <uint8_t> (head.optcode) < 0x00B))))
		// Создаём сообщение
		return mess_t(1002, "RSV1 must be clear");
	// Если опкоды требуют финального фрейма
	else if(!head.fin && (static_cast <uint8_t> (head.optcode) > 0x007) && (static_cast <uint8_t> (head.optcode) < 0x00B))
		// Создаём сообщение
		return mess_t(1002, "FIN must be set");
	// Выводим результат
	return (head.state == state_t::BAD ? mess_t(code == 0 ? 1005 : code) : mess_t(code));
}
/**
 * ping Метод создания фрейма пинга
 * @param mess данные сообщения
 * @param mask флаг выполнения маскировки сообщения
 * @return     бинарные данные фрейма
 */
vector <char> awh::ws::Frame::ping(const string & mess, const bool mask) const noexcept {
	// Создаём тело запроса и устанавливаем первый байт PING с пустой полезной нагрузкой
	vector <char> result = {static_cast <char> (0x080) | (0x00F & static_cast <char> (opcode_t::PING)), 0x00};
	// Если сообщение передано
	if(!mess.empty())
		// Выполняем формирование фрейма
		::frame(result, mess.data(), mess.size(), mask, this->_log);
	// Выводим результат
	return result;
}
/**
 * ping Метод создания фрейма пинга
 * @param buffer бинарный буфер данных для создания фрейма
 * @param size   размер буфера данных для создания фрейма
 * @param mask   флаг выполнения маскировки сообщения
 * @return       бинарные данные фрейма
 */
vector <char> awh::ws::Frame::ping(const void * buffer, const size_t size, const bool mask) const noexcept {
	// Создаём тело запроса и устанавливаем первый байт PING с пустой полезной нагрузкой
	vector <char> result = {static_cast <char> (0x080) | (0x00F & static_cast <char> (opcode_t::PING)), 0x00};
	// Если сообщение передано
	if((buffer != nullptr) && (size > 0))
		// Выполняем формирование фрейма
		::frame(result, buffer, size, mask, this->_log);
	// Выводим результат
	return result;
}
/**
 * pong Метод создания фрейма понга
 * @param mess данные сообщения
 * @param mask флаг выполнения маскировки сообщения
 * @return     бинарные данные фрейма
 */
vector <char> awh::ws::Frame::pong(const string & mess, const bool mask) const noexcept {
	// Создаём тело запроса и устанавливаем первый байт PONG с пустой полезной нагрузкой
	vector <char> result = {static_cast <char> (0x080) | (0x00F & static_cast <char> (opcode_t::PONG)), 0x00};
	// Если сообщение передано
	if(!mess.empty())
		// Выполняем формирование фрейма
		::frame(result, mess.data(), mess.size(), mask, this->_log);
	// Выводим результат
	return result;
}
/**
 * pong Метод создания фрейма понга
 * @param buffer бинарный буфер данных для создания фрейма
 * @param size   размер буфера данных для создания фрейма
 * @param mask   флаг выполнения маскировки сообщения
 * @return       бинарные данные фрейма
 */
vector <char> awh::ws::Frame::pong(const void * buffer, const size_t size, const bool mask) const noexcept {
	// Создаём тело запроса и устанавливаем первый байт PONG с пустой полезной нагрузкой
	vector <char> result = {static_cast <char> (0x080) | (0x00F & static_cast <char> (opcode_t::PONG)), 0x00};
	// Если сообщение передано
	if((buffer != nullptr) && (size > 0))
		// Выполняем формирование фрейма
		::frame(result, buffer, size, mask, this->_log);
	// Выводим результат
	return result;
}
/**
 * get Метод извлечения данных фрейма
 * @param head   заголовки фрейма
 * @param buffer бинарные данные фрейма для извлечения
 * @param size   размер передаваемого буфера
 * @return       бинарные данные полезной нагрузки
 */
vector <char> awh::ws::Frame::get(head_t & head, const void * buffer, const size_t size) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные переданы в достаточном объёме
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем чтение заголовков
			::head(head, buffer, size, this->_log);
			// Устанавливаем стейт фрейма
			head.state = state_t::GOOD;
			// Получаем размер смещения
			head.frame = static_cast <uint64_t> (head.size);
			// Получаем общее количество байтов
			uint64_t bytes = (head.payload + head.frame);
			// Если данные переданы в достаточном объёме для проверки входящих данных
			if(static_cast <size_t> (bytes) <= size){
				// Если входящие данные не являются мусоромы
				if((head.optcode == opcode_t::TEXT) || (head.optcode == opcode_t::BINARY) ||
				   (head.optcode == opcode_t::PING) || (head.optcode == opcode_t::PONG) ||
				   (head.optcode == opcode_t::CLOSE) || (head.optcode == opcode_t::CONTINUATION)){
					// Если маска требуется, маскируем данные
					if(head.mask)
						// Увеличиваем количество ожидаемых байт
						bytes += 4;
					// Если ожидаемых байт фрейма достаточно для обработки
					if(static_cast <size_t> (bytes) <= size){
						// Бинарные данные маски
						uint8_t mask[4];
						// Если маска требуется, маскируем данные
						if(head.mask){
							// Считываем ключ маски
							::memcpy(mask, reinterpret_cast <const uint8_t *> (buffer) + head.frame, 4);
							// Увеличиваем размер смещения
							head.frame += 4;
						}
						// Если полезная нагрузка получена
						if(head.payload > 0){
							// Получаем оставшиеся данные полезной нагрузки
							result.assign(
								reinterpret_cast <const char *> (buffer) + head.frame,
								reinterpret_cast <const char *> (buffer) + (head.payload + head.frame)
							);
							// Если маска требуется, размаскируем данные
							if(head.mask){
								// Выполняем перебор всех байт передаваемых данных
								for(size_t i = 0; i < result.size(); i++)
									// Выполняем шифрование данных
									result.at(i) ^= mask[i % 4];
							}
						}
						// Увеличиваем размер смещения
						head.frame += head.payload;
						// Если размер не установлен
						if((head.payload == 0) && ((head.optcode != opcode_t::PING) &&
						  (head.optcode != opcode_t::PONG) && (head.optcode != opcode_t::CLOSE)))
							// Устанавливаем статус битого фрейма
							head.state = state_t::BAD;
						// Проверяем состояние флагов RSV2 и RSV3
						else if(head.rsv[1] || head.rsv[2])
							// Устанавливаем статус битого фрейма
							head.state = state_t::BAD;
						// Если флаг компресси включён а данные пришли не сжатые
						else if(head.rsv[0] && ((head.optcode == opcode_t::CONTINUATION) ||
						       ((static_cast <uint8_t> (head.optcode) > 0x007) && (static_cast <uint8_t> (head.optcode) < 0x00B))))
							// Устанавливаем статус битого фрейма
							head.state = state_t::BAD;
						// Если опкоды требуют финального фрейма
						else if(!head.fin && (static_cast <uint8_t> (head.optcode) > 0x007) && (static_cast <uint8_t> (head.optcode) < 0x00B))
							// Устанавливаем статус битого фрейма
							head.state = state_t::BAD;
						// Если фрейм испорчен
						if(head.state == state_t::BAD){
							// Очищаем результирующий буфер
							result.clear();
							// Выполняем очистку выделенной памяти
							vector <char> ().swap(result);
						}
					}
				// Устанавливаем статус битого фрейма
				} else head.state = state_t::BAD;
			// Если размер данных уже слишком большой, выводим сообщение об ошибке
			} else if((size > sizeof(head_t)) && (head.payload > MAX_FRAME_SIZE))
				// Устанавливаем статус битого фрейма
				head.state = state_t::BAD;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * set Метод создания данных фрейма
 * @param head   заголовки фрейма
 * @param buffer бинарные данные полезной нагрузки
 * @param size   размер передаваемого буфера
 * @return       бинарные данные фрейма
 */
vector <char> awh::ws::Frame::set(const head_t & head, const void * buffer, const size_t size) const noexcept {
	/**
	 * rsv[0] должен быть установлен в TRUE для первого сообщения в GZip,
	 * и установлен в FALSE для всех остальных сообщений, в рамках одной сессии
	 */
	vector <char> result = {
		(
			static_cast <char> (
				(head.fin ? 0x080 : 0x00) |
				(head.rsv[0] ? 0x040 : 0x00) |
				(head.rsv[1] ? 0x020 : 0x00) |
				(head.rsv[2] ? 0x010 : 0x00) |
				(0x00F & static_cast <int32_t> (head.optcode))
			)
		), 0x00
	};
	// Если данные переданы
	if((buffer != nullptr) && (size > 0))
		// Выполняем формирование фрейма
		::frame(result, buffer, size, head.mask, this->_log);
	// Выводим результат
	return result;
}
