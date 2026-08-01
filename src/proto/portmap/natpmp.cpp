/**
 * @file: natpmp.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация кодека договора NAT-PMP (RFC 6886) — сборка запросов внешнего адреса
 *        и перенаправления порта, разбор ответов маршрутизатора и описания кодов итога
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <proto/portmap/natpmp.hpp>

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
	 * @brief Код действия, отмечающий сообщение ответом
	 *
	 * @details Ответ отличается от запроса старшим битом кода действия: договор
	 * отдельного поля вида сообщения не имеет
	 *
	 */
	constexpr uint8_t RESPONSE_FLAG = 0x80;

	/**
	 * @brief Код действия запроса внешнего адреса маршрутизатора
	 *
	 */
	constexpr uint8_t OPCODE_ADDRESS = 0x00;

	/**
	 * @brief Размер запроса внешнего адреса маршрутизатора
	 *
	 */
	constexpr size_t REQUEST_ADDRESS_SIZE = 0x02;

	/**
	 * @brief Размер просьбы о перенаправлении порта
	 *
	 */
	constexpr size_t REQUEST_MAPPING_SIZE = 0x0C;

	/**
	 * @brief Размер ответа с внешним адресом маршрутизатора
	 *
	 */
	constexpr size_t RESPONSE_ADDRESS_SIZE = 0x0C;

	/**
	 * @brief Размер ответа о перенаправлении порта
	 *
	 */
	constexpr size_t RESPONSE_MAPPING_SIZE = 0x10;

	/**
	 * @brief Метод извлечения двухоктетного числа в порядке октетов сети
	 *
	 * @details Сборка ведётся сдвигами намеренно: так кодек не зависит ни от порядка
	 * октетов машины, ни от заголовочных файлов работы с сетью, и разбирать сообщение
	 * можно где угодно, включая проверочные наборы
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
};

/**
 * @brief Конструктор
 *
 */
awh::proto::portmap::NAT_PMP::Answer::Answer() noexcept :
 kind(kind_t::NONE), result(result_t::SUCCESS), proto(proto_t::NONE),
 epoch(0), address(0), lifeTime(0), internalPort(0), externalPort(0) {}

/**
 * @brief Конструктор
 *
 */
awh::proto::portmap::NAT_PMP::Request::Request() noexcept :
 proto(proto_t::NONE), internalPort(0), externalPort(0), lifeTime(0) {}

/**
 * @brief Метод сборки запроса внешнего адреса маршрутизатора
 *
 * @param buffer место под собираемое сообщение
 * @param size   размер отведённого места
 * @param error  ссылка на код причины отказа
 * @return       размер собранного сообщения
 *
 */
size_t awh::proto::portmap::NAT_PMP::address(void * buffer, const size_t size, error_t & error) const noexcept {
	// Выполняем сброс кода причины отказа
	error = error_t::NONE;
	/**
	 * Если места под сообщение не хватает
	 */
	if((buffer == nullptr) || (size < REQUEST_ADDRESS_SIZE)){
		// Запоминаем код причины отказа
		error = error_t::BUFFER_TOO_SMALL;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::WARNING, message(error));
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
	// Записываем издание договора
	data[0] = VERSION;
	// Записываем код действия запроса внешнего адреса
	data[1] = OPCODE_ADDRESS;
	// Выводим размер собранного сообщения
	return REQUEST_ADDRESS_SIZE;
}
/**
 * @brief Метод сборки просьбы о перенаправлении порта
 *
 * @param buffer  место под собираемое сообщение
 * @param size    размер отведённого места
 * @param request параметры просьбы о перенаправлении порта
 * @param error   ссылка на код причины отказа
 * @return        размер собранного сообщения
 *
 */
size_t awh::proto::portmap::NAT_PMP::mapping(void * buffer, const size_t size, const request_t & request, error_t & error) const noexcept {
	// Выполняем сброс кода причины отказа
	error = error_t::NONE;
	/**
	 * Если места под сообщение не хватает
	 */
	if((buffer == nullptr) || (size < REQUEST_MAPPING_SIZE)){
		// Запоминаем код причины отказа
		error = error_t::BUFFER_TOO_SMALL;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::WARNING, message(error));
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
	 * Если договор перенаправления не задан
	 *
	 * @note Код действия и есть обозначение договора, поэтому неопределённый
	 *       договор собрать в сообщение попросту нечем
	 */
	if(request.proto == proto_t::NONE){
		// Запоминаем код причины отказа
		error = error_t::INVALID_OPCODE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(request.internalPort, request.externalPort, request.lifeTime), log_t::flag_t::WARNING, message(error));
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
	// Записываем издание договора
	data[0] = VERSION;
	// Записываем код действия, отвечающий договору перенаправления
	data[1] = static_cast <uint8_t> (request.proto);
	// Записываем отведённое договором пустое поле
	::write16(data + 2, 0);
	// Записываем внутренний порт перенаправления
	::write16(data + 4, request.internalPort);
	// Записываем желаемый внешний порт перенаправления
	::write16(data + 6, request.externalPort);
	// Записываем запрашиваемый срок жизни перенаправления
	::write32(data + 8, request.lifeTime);
	// Выводим размер собранного сообщения
	return REQUEST_MAPPING_SIZE;
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
bool awh::proto::portmap::NAT_PMP::parse(const void * buffer, const size_t size, answer_t & answer, error_t & error) const noexcept {
	// Выполняем сброс кода причины отказа
	error = error_t::NONE;
	// Выполняем сброс разобранного ответа
	answer = answer_t();
	/**
	 * Если сообщение короче заголовка ответа
	 */
	if((buffer == nullptr) || (size < 4)){
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
		 *
		 * @note Отказ разбора записывается лишь при отладке намеренно: сообщения,
		 *       разбору не поддающиеся, приходят на открытый порт постоянно, и
		 *       запись о каждом из них засоряла бы журнал, а при обстреле снаружи
		 *       ещё и служила бы средством нападения
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
	 *
	 * @note Отличать это необходимо: на порт ответов приходят и уведомления
	 *       маршрутизатора, и чужие запросы, разосланные по сети
	 */
	if((data[1] & RESPONSE_FLAG) == 0){
		// Запоминаем код причины отказа
		error = error_t::NOT_A_RESPONSE;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(data[1]), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Получаем код действия без отметки ответа
	const uint8_t opcode = static_cast <uint8_t> (data[1] & ~RESPONSE_FLAG);
	// Получаем код итога, выданный маршрутизатором
	const uint16_t result = ::read16(data + 2);
	/**
	 * Запоминаем код итога, выданный маршрутизатором, как он есть
	 *
	 * @note Неизвестный код к отказу разбора не ведёт и подменяться известным не
	 *       должен: договор оставляет место под новые коды, а сообщение при этом
	 *       построено верно. Вызывающий обязан сличать итог с успехом, а не с
	 *       перечнем известных отказов - иначе новый код молча прошёл бы за успех
	 */
	answer.result = static_cast <result_t> (result);
	/**
	 * Определяем код действия полученного ответа
	 */
	switch(opcode){
		/**
		 * Если получен ответ с внешним адресом маршрутизатора
		 */
		case OPCODE_ADDRESS: {
			/**
			 * Если сообщение короче ответа с внешним адресом
			 */
			if(size < RESPONSE_ADDRESS_SIZE){
				// Запоминаем код причины отказа
				error = error_t::TRUNCATED;
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::WARNING, message(error));
				#endif
				// Выводим признак неудачного разбора
				return false;
			}
			// Запоминаем вид полученного сообщения
			answer.kind = kind_t::ADDRESS;
			// Запоминаем время работы маршрутизатора
			answer.epoch = ::read32(data + 4);
			// Запоминаем внешний адрес маршрутизатора
			answer.address = ::read32(data + 8);
		} break;
		/**
		 * Если получен ответ о перенаправлении порта UDP
		 */
		case static_cast <uint8_t> (proto_t::UDP):
		/**
		 * Если получен ответ о перенаправлении порта TCP
		 */
		case static_cast <uint8_t> (proto_t::TCP): {
			/**
			 * Если сообщение короче ответа о перенаправлении порта
			 */
			if(size < RESPONSE_MAPPING_SIZE){
				// Запоминаем код причины отказа
				error = error_t::TRUNCATED;
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::WARNING, message(error));
				#endif
				// Выводим признак неудачного разбора
				return false;
			}
			// Запоминаем вид полученного сообщения
			answer.kind = kind_t::MAPPING;
			// Запоминаем договор перенаправления порта
			answer.proto = static_cast <proto_t> (opcode);
			// Запоминаем время работы маршрутизатора
			answer.epoch = ::read32(data + 4);
			// Запоминаем внутренний порт перенаправления
			answer.internalPort = ::read16(data + 8);
			// Запоминаем внешний порт, назначенный маршрутизатором
			answer.externalPort = ::read16(data + 10);
			// Запоминаем срок жизни перенаправления
			answer.lifeTime = ::read32(data + 12);
		} break;
		/**
		 * Если код действия кодеку неизвестен
		 */
		default: {
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
	}
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод получения срока ожидания ответа на очередную попытку
 *
 * @param attempt порядковый номер попытки, считая с нуля
 * @return        срок ожидания ответа в миллисекундах
 *
 */
uint32_t awh::proto::portmap::NAT_PMP::timeout(const uint8_t attempt) noexcept {
	/**
	 * Если попыток отведено больше положенного
	 *
	 * @note Ограничение обязательно: без него сдвиг вышел бы за разрядность
	 *       числа, а срок ожидания стал бы бессмысленным
	 */
	if(attempt >= MAX_ATTEMPTS)
		// Выводим срок ожидания последней попытки
		return (INITIAL_TIMEOUT << (MAX_ATTEMPTS - 1));
	// Выводим удвоенный по числу попыток срок ожидания
	return (INITIAL_TIMEOUT << attempt);
}
/**
 * @brief Метод получения описания кода итога договора NAT-PMP
 *
 * @param result код итога, выданный маршрутизатором
 * @return       описание кода итога на английском языке
 *
 */
const char * awh::proto::portmap::message(const natpmp_t::result_t result) noexcept {
	/**
	 * Определяем код итога, выданный маршрутизатором
	 */
	switch(static_cast <uint16_t> (result)){
		// Если просьба выполнена
		case static_cast <uint16_t> (natpmp_t::result_t::SUCCESS):
			// Выводим описание кода итога
			return "success";
		// Если издание договора не поддерживается
		case static_cast <uint16_t> (natpmp_t::result_t::UNSUPPORTED_VERSION):
			// Выводим описание кода итога
			return "unsupported protocol version";
		// Если просьба отвергнута настройкой маршрутизатора
		case static_cast <uint16_t> (natpmp_t::result_t::NOT_AUTHORIZED):
			// Выводим описание кода итога
			return "not authorized or refused by the gateway";
		// Если маршрутизатор не имеет связи с внешней сетью
		case static_cast <uint16_t> (natpmp_t::result_t::NETWORK_FAILURE):
			// Выводим описание кода итога
			return "gateway network failure";
		// Если у маршрутизатора не осталось места под перенаправления
		case static_cast <uint16_t> (natpmp_t::result_t::OUT_OF_RESOURCES):
			// Выводим описание кода итога
			return "gateway out of resources";
		// Если действие не поддерживается
		case static_cast <uint16_t> (natpmp_t::result_t::UNSUPPORTED_OPCODE):
			// Выводим описание кода итога
			return "unsupported opcode";
	}
	// Выводим описание неизвестного кода итога
	return "unknown result code";
}
/**
 * @brief Метод получения описания кода причины отказа кодека NAT-PMP
 *
 * @param error код причины отказа кодека
 * @return      описание кода причины отказа на английском языке
 *
 */
const char * awh::proto::portmap::message(const natpmp_t::error_t error) noexcept {
	/**
	 * Определяем код причины отказа кодека
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (natpmp_t::error_t::NONE):
			// Выводим описание кода причины отказа
			return "no error";
		// Если сообщение короче положенного
		case static_cast <uint8_t> (natpmp_t::error_t::TRUNCATED):
			// Выводим описание кода причины отказа
			return "message is truncated";
		// Если отведённого места не хватает под сообщение
		case static_cast <uint8_t> (natpmp_t::error_t::BUFFER_TOO_SMALL):
			// Выводим описание кода причины отказа
			return "buffer is too small for the message";
		// Если издание договора в сообщении неизвестно
		case static_cast <uint8_t> (natpmp_t::error_t::INVALID_VERSION):
			// Выводим описание кода причины отказа
			return "unsupported protocol version";
		// Если код действия в сообщении неизвестен
		case static_cast <uint8_t> (natpmp_t::error_t::INVALID_OPCODE):
			// Выводим описание кода причины отказа
			return "unknown opcode";
		// Если сообщение ответом не является
		case static_cast <uint8_t> (natpmp_t::error_t::NOT_A_RESPONSE):
			// Выводим описание кода причины отказа
			return "message is not a response";
	}
	// Выводим описание неизвестного кода причины отказа
	return "unknown error";
}
