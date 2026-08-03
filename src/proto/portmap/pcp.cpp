/**
 * @file: pcp.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация кодека договора PCP (RFC 6887) — сборка запросов, разбор ответов
 *        маршрутизатора, работа с дополнениями запроса и описания кодов итога
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <proto/portmap/pcp.hpp>

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
	 * @brief Отметка, отличающая ответ от запроса
	 *
	 * @details Ответ отличается от запроса старшим битом октета действия: договор
	 * отдельного поля вида сообщения не имеет
	 *
	 */
	constexpr uint8_t RESPONSE_FLAG = 0x80;

	/**
	 * @brief Размер содержимого действия перенаправления порта
	 *
	 */
	constexpr size_t MAP_PAYLOAD_SIZE = 0x24;

	/**
	 * @brief Размер содержимого действия сношения с узлом
	 *
	 */
	constexpr size_t PEER_PAYLOAD_SIZE = 0x38;

	/**
	 * @brief Размер заголовка дополнения запроса
	 *
	 */
	constexpr size_t OPTION_HEADER_SIZE = 0x04;

	/**
	 * @brief Метод извлечения двухоктетного числа в порядке октетов сети
	 *
	 * @param buffer место, откуда извлекается число
	 * @return       извлечённое число в порядке октетов машины
	 *
	 */
	uint16_t read16(const uint8_t * buffer) noexcept {
		// Выводим собранное из октетов число
		return static_cast <uint16_t> ((static_cast <uint16_t> (buffer[0]) << 8) | static_cast <uint16_t> (buffer[1]));
	}
	/**
	 * @brief Метод извлечения четырёхоктетного числа в порядке октетов сети
	 *
	 * @param buffer место, откуда извлекается число
	 * @return       извлечённое число в порядке октетов машины
	 *
	 */
	uint32_t read32(const uint8_t * buffer) noexcept {
		// Выводим собранное из октетов число
		return ((static_cast <uint32_t> (buffer[0]) << 24) | (static_cast <uint32_t> (buffer[1]) << 16) |
		        (static_cast <uint32_t> (buffer[2]) << 8) | static_cast <uint32_t> (buffer[3]));
	}
	/**
	 * @brief Метод записи двухоктетного числа в порядке октетов сети
	 *
	 * @param buffer место, куда записывается число
	 * @param value  записываемое число в порядке октетов машины
	 *
	 */
	void write16(uint8_t * buffer, const uint16_t value) noexcept {
		// Записываем старший октет числа
		buffer[0] = static_cast <uint8_t> ((value >> 8) & 0xFF);
		// Записываем младший октет числа
		buffer[1] = static_cast <uint8_t> (value & 0xFF);
	}
	/**
	 * @brief Метод записи четырёхоктетного числа в порядке октетов сети
	 *
	 * @param buffer место, куда записывается число
	 * @param value  записываемое число в порядке октетов машины
	 *
	 */
	void write32(uint8_t * buffer, const uint32_t value) noexcept {
		// Записываем старший октет числа
		buffer[0] = static_cast <uint8_t> ((value >> 24) & 0xFF);
		// Записываем второй октет числа
		buffer[1] = static_cast <uint8_t> ((value >> 16) & 0xFF);
		// Записываем третий октет числа
		buffer[2] = static_cast <uint8_t> ((value >> 8) & 0xFF);
		// Записываем младший октет числа
		buffer[3] = static_cast <uint8_t> (value & 0xFF);
	}
	/**
	 * @brief Метод получения размера содержимого действия договора
	 *
	 * @param opcode действие договора
	 * @return       размер содержимого действия в октетах
	 *
	 */
	size_t payload(const uint8_t opcode) noexcept {
		/**
		 * Определяем действие договора
		 */
		switch(opcode){
			// Если действием является уведомление о состоянии маршрутизатора
			case static_cast <uint8_t> (awh::proto::portmap::pcp_t::opcode_t::ANNOUNCE):
				// Выводим размер содержимого уведомления
				return 0;
			// Если действием является перенаправление порта
			case static_cast <uint8_t> (awh::proto::portmap::pcp_t::opcode_t::MAP):
				// Выводим размер содержимого перенаправления порта
				return MAP_PAYLOAD_SIZE;
			// Если действием является сношение с определённым узлом
			case static_cast <uint8_t> (awh::proto::portmap::pcp_t::opcode_t::PEER):
				// Выводим размер содержимого сношения с узлом
				return PEER_PAYLOAD_SIZE;
		}
		// Выводим признак неизвестного действия договора
		return static_cast <size_t> (-1);
	}
};

/**
 * @brief Конструктор
 *
 */
awh::proto::portmap::PCP::Option::Option() noexcept : code(option_t::NONE) {}

/**
 * @brief Конструктор
 *
 */
awh::proto::portmap::PCP::Request::Request() noexcept :
 opcode(opcode_t::MAP), proto(proto_t::ALL), lifeTime(0),
 internalPort(0), externalPort(0), remotePort(0),
 nonce{0}, client{0}, external{0}, remote{0} {}

/**
 * @brief Конструктор
 *
 */
awh::proto::portmap::PCP::Answer::Answer() noexcept :
 opcode(opcode_t::ANNOUNCE), result(result_t::SUCCESS), proto(proto_t::ALL),
 lifeTime(0), epoch(0), internalPort(0), externalPort(0), remotePort(0),
 nonce{0}, external{0}, remote{0} {}

/**
 * @brief Метод сборки запроса к маршрутизатору
 *
 * @param buffer  место под собираемое сообщение
 * @param size    размер отведённого места
 * @param request параметры запроса к маршрутизатору
 * @param error   ссылка на код причины отказа
 * @return        размер собранного сообщения
 *
 */
size_t awh::proto::portmap::PCP::request(void * buffer, const size_t size, const request_t & request, error_t & error) const noexcept {
	// Выполняем сброс кода причины отказа
	error = error_t::NONE;
	// Получаем размер содержимого запрошенного действия
	const size_t payload = ::payload(static_cast <uint8_t> (request.opcode));
	/**
	 * Если действие договора кодеку неизвестно
	 */
	if(payload == static_cast <size_t> (-1)){
		// Запоминаем код причины отказа
		error = error_t::INVALID_OPCODE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (request.opcode)), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим нулевой размер собранного сообщения
		return 0;
	}
	// Подсчитываем размер собираемого сообщения
	size_t length = (HEADER_SIZE + payload);
	/**
	 * Выполняем перебор всех дополнений запроса
	 */
	for(const opt_t & option : request.options)
		// Увеличиваем размер сообщения на размер очередного дополнения с выравниванием
		length += (OPTION_HEADER_SIZE + ((option.data.size() + 3) & ~static_cast <size_t> (3)));
	/**
	 * Если собранное сообщение вышло длиннее допустимого договором
	 *
	 * @note Предел договором задан твёрдо: сообщение длиннее маршрутизатор
	 *       обязан отвергнуть, и отправлять такое незачем
	 */
	if(length > MAX_MESSAGE_SIZE){
		// Запоминаем код причины отказа
		error = error_t::TOO_LARGE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(length), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим нулевой размер собранного сообщения
		return 0;
	}
	/**
	 * Если места под сообщение не хватает
	 */
	if((buffer == nullptr) || (size < length)){
		// Запоминаем код причины отказа
		error = error_t::BUFFER_TOO_SMALL;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size, length), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим нулевой размер собранного сообщения
		return 0;
	}
	// Получаем место под собираемое сообщение
	uint8_t * data = reinterpret_cast <uint8_t *> (buffer);
	// Выполняем обнуление отведённого под сообщение места
	::memset(data, 0, length);
	// Записываем издание договора
	data[0] = VERSION;
	// Записываем действие договора без отметки ответа
	data[1] = static_cast <uint8_t> (request.opcode);
	// Записываем запрашиваемый срок жизни перенаправления
	::write32(data + 4, request.lifeTime);
	// Записываем адрес машины, обращающейся к маршрутизатору
	::memcpy(data + 8, request.client, ADDRESS_SIZE);
	// Получаем место под содержимое действия договора
	uint8_t * body = (data + HEADER_SIZE);
	/**
	 * Если действие договора содержимое имеет
	 *
	 * @note Уведомление о состоянии маршрутизатора содержимого не имеет вовсе:
	 *       им лишь спрашивают, работает ли маршрутизатор и не перезапускался ли он
	 */
	if(payload > 0){
		// Записываем отличительную метку перенаправления
		::memcpy(body, request.nonce, NONCE_SIZE);
		// Записываем договор перенаправления порта
		body[NONCE_SIZE] = static_cast <uint8_t> (request.proto);
		// Записываем внутренний порт перенаправления
		::write16(body + 16, request.internalPort);
		// Записываем желаемый внешний порт перенаправления
		::write16(body + 18, request.externalPort);
		// Записываем желаемый внешний адрес перенаправления
		::memcpy(body + 20, request.external, ADDRESS_SIZE);
		/**
		 * Если действием является сношение с определённым узлом
		 */
		if(request.opcode == opcode_t::PEER){
			// Записываем порт узла, с которым ведётся сношение
			::write16(body + 36, request.remotePort);
			// Записываем адрес узла, с которым ведётся сношение
			::memcpy(body + 40, request.remote, ADDRESS_SIZE);
		}
	}
	// Получаем место под дополнения запроса
	uint8_t * tail = (body + payload);
	/**
	 * Выполняем перебор всех дополнений запроса
	 */
	for(const opt_t & option : request.options){
		// Получаем размер содержимого очередного дополнения
		const size_t content = option.data.size();
		// Записываем код дополнения запроса
		tail[0] = static_cast <uint8_t> (option.code);
		// Записываем размер содержимого дополнения
		::write16(tail + 2, static_cast <uint16_t> (content));
		/**
		 * Если дополнение содержимое имеет
		 */
		if(content > 0)
			// Записываем содержимое дополнения запроса
			::memcpy(tail + OPTION_HEADER_SIZE, option.data.data(), content);
		/**
		 * Выполняем переход к месту под следующее дополнение с выравниванием
		 *
		 * @note Договор велит дополнять содержимое нулями до кратности четырём:
		 *       обнулено отведённое место было заранее, дополнять нечем
		 */
		tail += (OPTION_HEADER_SIZE + ((content + 3) & ~static_cast <size_t> (3)));
	}
	// Выводим размер собранного сообщения
	return length;
}
/**
 * @brief Метод разбора ответа маршрутизатора
 *
 * @param buffer разбираемое сообщение
 * @param size   размер разбираемого сообщения
 * @param answer ссылка на разобранный ответ
 * @param error  ссылка на код причины отказа
 * @return       признак успешного разбора
 *
 */
bool awh::proto::portmap::PCP::parse(const void * buffer, const size_t size, answer_t & answer, error_t & error) const noexcept {
	// Выполняем сброс кода причины отказа
	error = error_t::NONE;
	// Выполняем сброс разобранного ответа
	answer = answer_t();
	/**
	 * Если сообщение короче заголовка
	 */
	if((buffer == nullptr) || (size < HEADER_SIZE)){
		// Запоминаем код причины отказа
		error = error_t::TRUNCATED;
		/**
		 * Если включён режим отладки
		 *
		 * @note Отказ разбора записывается лишь при отладке намеренно: сообщения,
		 *       разбору не поддающиеся, приходят на открытый порт постоянно, и
		 *       запись о каждом из них засоряла бы журнал, а при обстреле снаружи
		 *       ещё и служила бы средством нападения
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Получаем разбираемое сообщение
	const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer);
	/**
	 * Если издание договора в сообщении неизвестно
	 */
	if(data[0] != VERSION){
		// Запоминаем код причины отказа
		error = error_t::INVALID_VERSION;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(data[0]), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * Если сообщение ответом не является
	 */
	if((data[1] & RESPONSE_FLAG) == 0){
		// Запоминаем код причины отказа
		error = error_t::NOT_A_RESPONSE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(data[1]), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Получаем действие договора без отметки ответа
	const uint8_t opcode = static_cast <uint8_t> (data[1] & ~RESPONSE_FLAG);
	// Получаем размер содержимого полученного действия
	const size_t payload = ::payload(opcode);
	/**
	 * Если действие договора кодеку неизвестно
	 */
	if(payload == static_cast <size_t> (-1)){
		// Запоминаем код причины отказа
		error = error_t::INVALID_OPCODE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(opcode), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * Если сообщение короче содержимого полученного действия
	 */
	if(size < (HEADER_SIZE + payload)){
		// Запоминаем код причины отказа
		error = error_t::TRUNCATED;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size, payload), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Запоминаем действие договора
	answer.opcode = static_cast <opcode_t> (opcode);
	/**
	 * Запоминаем код итога, выданный маршрутизатором, как он есть
	 *
	 * @note Неизвестный код подменяться известным не должен: договор оставляет
	 *       место под новые коды, и подмена скрыла бы настоящую причину отказа
	 */
	answer.result = static_cast <result_t> (data[3]);
	// Запоминаем назначенный срок жизни перенаправления
	answer.lifeTime = ::read32(data + 4);
	// Запоминаем время работы маршрутизатора
	answer.epoch = ::read32(data + 8);
	/**
	 * Если действие договора содержимое имеет
	 */
	if(payload > 0){
		// Получаем содержимое действия договора
		const uint8_t * body = (data + HEADER_SIZE);
		// Запоминаем отличительную метку перенаправления
		::memcpy(answer.nonce, body, NONCE_SIZE);
		// Запоминаем договор перенаправления порта
		answer.proto = static_cast <proto_t> (body[NONCE_SIZE]);
		// Запоминаем внутренний порт перенаправления
		answer.internalPort = ::read16(body + 16);
		// Запоминаем внешний порт, назначенный маршрутизатором
		answer.externalPort = ::read16(body + 18);
		// Запоминаем внешний адрес, назначенный маршрутизатором
		::memcpy(answer.external, body + 20, ADDRESS_SIZE);
		/**
		 * Если действием является сношение с определённым узлом
		 */
		if(answer.opcode == opcode_t::PEER){
			// Запоминаем порт узла, с которым ведётся сношение
			answer.remotePort = ::read16(body + 36);
			// Запоминаем адрес узла, с которым ведётся сношение
			::memcpy(answer.remote, body + 40, ADDRESS_SIZE);
		}
	}
	// Получаем смещение первого дополнения ответа
	size_t offset = (HEADER_SIZE + payload);
	/**
	 * Выполняем перебор всех дополнений ответа
	 */
	while(offset < size){
		/**
		 * Если остатка сообщения не хватает под заголовок дополнения
		 */
		if((size - offset) < OPTION_HEADER_SIZE){
			// Запоминаем код причины отказа
			error = error_t::MALFORMED_OPTION;
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(offset, size), log_t::flag_t::WARNING, message(error));
			#endif
			// Выводим признак неудачного разбора
			return false;
		}
		// Получаем размер содержимого очередного дополнения
		const size_t length = static_cast <size_t> (::read16(data + offset + 2));
		// Получаем размер очередного дополнения с выравниванием
		const size_t padded = (OPTION_HEADER_SIZE + ((length + 3) & ~static_cast <size_t> (3)));
		/**
		 * Если дополнение выходит за границы сообщения
		 *
		 * @note Проверять это обязательно: без неё длина, взятая из сообщения,
		 *       увела бы чтение за отведённое место
		 */
		if((size - offset) < padded){
			// Запоминаем код причины отказа
			error = error_t::MALFORMED_OPTION;
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(offset, length, size), log_t::flag_t::WARNING, message(error));
			#endif
			// Выводим признак неудачного разбора
			return false;
		}
		// Создаём разбираемое дополнение ответа
		opt_t option;
		// Запоминаем код дополнения ответа
		option.code = static_cast <option_t> (data[offset]);
		/**
		 * Если дополнение содержимое имеет
		 */
		if(length > 0)
			// Запоминаем содержимое дополнения ответа
			option.data.assign(data + offset + OPTION_HEADER_SIZE, data + offset + OPTION_HEADER_SIZE + length);
		// Выполняем добавление разобранного дополнения к ответу
		answer.options.push_back(::std::move(option));
		// Выполняем переход к следующему дополнению ответа
		offset += padded;
	}
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод проверки принадлежности ответа запросу
 *
 * @param answer  разобранный ответ маршрутизатора
 * @param request отправленный запрос к маршрутизатору
 * @return        признак принадлежности ответа запросу
 *
 */
bool awh::proto::portmap::PCP::belongs(const answer_t & answer, const request_t & request) const noexcept {
	/**
	 * Если действие ответа с действием запроса не совпадает
	 */
	if(answer.opcode != request.opcode)
		// Выводим признак того, что ответ запросу не принадлежит
		return false;
	/**
	 * Если действие содержимого не имеет
	 *
	 * @note Уведомление о состоянии маршрутизатора отличительной метки не несёт,
	 *       и сличать в нём попросту нечего
	 */
	if(answer.opcode == opcode_t::ANNOUNCE)
		// Выводим признак того, что ответ запросу принадлежит
		return true;
	/**
	 * Если отличительная метка ответа с меткой запроса не совпадает
	 */
	if(::memcmp(answer.nonce, request.nonce, NONCE_SIZE) != 0)
		// Выводим признак того, что ответ запросу не принадлежит
		return false;
	/**
	 * Если внутренний порт ответа с портом запроса не совпадает
	 */
	if(answer.internalPort != request.internalPort)
		// Выводим признак того, что ответ запросу не принадлежит
		return false;
	// Выводим признак того, что ответ запросу принадлежит
	return true;
}
/**
 * @brief Метод получения срока ожидания ответа на очередную попытку
 *
 * @param attempt порядковый номер попытки, считая с нуля
 * @return        срок ожидания ответа в миллисекундах
 *
 */
uint32_t awh::proto::portmap::PCP::timeout(const uint8_t attempt) noexcept {
	// Собираемый срок ожидания ответа
	uint32_t result = INITIAL_TIMEOUT;
	/**
	 * Выполняем удвоение срока ожидания по числу попыток
	 */
	for(uint8_t i = 0; i < attempt; i++){
		/**
		 * Если удвоенный срок превысил наибольший допустимый
		 *
		 * @note Договор верхний предел задаёт твёрдо: расти сроку ожидания дальше
		 *       незачем, а без ограничения он вышел бы за разрядность числа
		 */
		if(result >= (MAX_TIMEOUT / 2))
			// Выводим наибольший допустимый срок ожидания
			return MAX_TIMEOUT;
		// Выполняем удвоение срока ожидания
		result *= 2;
	}
	// Выводим собранный срок ожидания ответа
	return result;
}
/**
 * @brief Метод получения описания кода итога договора PCP
 *
 * @param result код итога, выданный маршрутизатором
 * @return       описание кода итога на английском языке
 *
 */
const char * awh::proto::portmap::message(const pcp_t::result_t result) noexcept {
	/**
	 * Определяем код итога, выданный маршрутизатором
	 */
	switch(static_cast <uint8_t> (result)){
		// Если просьба выполнена
		case static_cast <uint8_t> (pcp_t::result_t::SUCCESS):
			// Выводим описание кода итога
			return "success";
		// Если издание договора не поддерживается
		case static_cast <uint8_t> (pcp_t::result_t::UNSUPP_VERSION):
			// Выводим описание кода итога
			return "unsupported protocol version";
		// Если просьба отвергнута настройкой маршрутизатора
		case static_cast <uint8_t> (pcp_t::result_t::NOT_AUTHORIZED):
			// Выводим описание кода итога
			return "not authorized or refused by the gateway";
		// Если запрос построен ошибочно
		case static_cast <uint8_t> (pcp_t::result_t::MALFORMED_REQUEST):
			// Выводим описание кода итога
			return "malformed request";
		// Если действие не поддерживается
		case static_cast <uint8_t> (pcp_t::result_t::UNSUPP_OPCODE):
			// Выводим описание кода итога
			return "unsupported opcode";
		// Если дополнение запроса не поддерживается
		case static_cast <uint8_t> (pcp_t::result_t::UNSUPP_OPTION):
			// Выводим описание кода итога
			return "unsupported option";
		// Если дополнение запроса построено ошибочно
		case static_cast <uint8_t> (pcp_t::result_t::MALFORMED_OPTION):
			// Выводим описание кода итога
			return "malformed option";
		// Если маршрутизатор не имеет связи с внешней сетью
		case static_cast <uint8_t> (pcp_t::result_t::NETWORK_FAILURE):
			// Выводим описание кода итога
			return "gateway network failure";
		// Если у маршрутизатора не осталось места под перенаправления
		case static_cast <uint8_t> (pcp_t::result_t::NO_RESOURCES):
			// Выводим описание кода итога
			return "gateway out of resources";
		// Если договор перенаправления не поддерживается
		case static_cast <uint8_t> (pcp_t::result_t::UNSUPP_PROTOCOL):
			// Выводим описание кода итога
			return "unsupported transport protocol";
		// Если машина исчерпала отведённую ей долю перенаправлений
		case static_cast <uint8_t> (pcp_t::result_t::USER_EX_QUOTA):
			// Выводим описание кода итога
			return "user exceeded its mapping quota";
		// Если запрошенный внешний адрес выдать невозможно
		case static_cast <uint8_t> (pcp_t::result_t::CANNOT_PROVIDE_EXTERNAL):
			// Выводим описание кода итога
			return "cannot provide the requested external address";
		// Если адрес в запросе не совпадает с адресом отправителя
		case static_cast <uint8_t> (pcp_t::result_t::ADDRESS_MISMATCH):
			// Выводим описание кода итога
			return "client address mismatch";
		// Если узлов в просьбе указано больше допустимого
		case static_cast <uint8_t> (pcp_t::result_t::EXCESSIVE_REMOTE_PEERS):
			// Выводим описание кода итога
			return "excessive number of remote peers";
	}
	// Выводим описание неизвестного кода итога
	return "unknown result code";
}
/**
 * @brief Метод получения описания кода причины отказа кодека PCP
 *
 * @param error код причины отказа кодека
 * @return      описание кода причины отказа на английском языке
 *
 */
const char * awh::proto::portmap::message(const pcp_t::error_t error) noexcept {
	/**
	 * Определяем код причины отказа кодека
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (pcp_t::error_t::NONE):
			// Выводим описание кода причины отказа
			return "no error";
		// Если сообщение короче положенного
		case static_cast <uint8_t> (pcp_t::error_t::TRUNCATED):
			// Выводим описание кода причины отказа
			return "message is truncated";
		// Если отведённого места не хватает под сообщение
		case static_cast <uint8_t> (pcp_t::error_t::BUFFER_TOO_SMALL):
			// Выводим описание кода причины отказа
			return "buffer is too small for the message";
		// Если издание договора в сообщении неизвестно
		case static_cast <uint8_t> (pcp_t::error_t::INVALID_VERSION):
			// Выводим описание кода причины отказа
			return "unsupported protocol version";
		// Если код действия в сообщении неизвестен
		case static_cast <uint8_t> (pcp_t::error_t::INVALID_OPCODE):
			// Выводим описание кода причины отказа
			return "unknown opcode";
		// Если сообщение ответом не является
		case static_cast <uint8_t> (pcp_t::error_t::NOT_A_RESPONSE):
			// Выводим описание кода причины отказа
			return "message is not a response";
		// Если дополнение запроса построено ошибочно
		case static_cast <uint8_t> (pcp_t::error_t::MALFORMED_OPTION):
			// Выводим описание кода причины отказа
			return "malformed option";
		// Если сообщение длиннее допустимого договором
		case static_cast <uint8_t> (pcp_t::error_t::TOO_LARGE):
			// Выводим описание кода причины отказа
			return "message exceeds the protocol size limit";
	}
	// Выводим описание неизвестного кода причины отказа
	return "unknown error";
}
