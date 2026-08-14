/**
 * @file sctp.cpp
 * @date 2026-01-28
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
 * @brief Реализация бэкенда протокола SCTP — настройка параметров ассоциаций, входящих и исходящих потоков,
 *        heartbeat, авторизации и подписки на уведомления SCTP-сокета
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cerrno>
#include <cstring>
#include <cstdlib>

/**
 * Системные заголовочные файлы
 */
#include <sys/socket.h>
#include <netinet/sctp.h>

/**
 * @brief Опознаватель модуля протокола передачи с управлением потоком
 *
 * @note Заведён по образцу прочих модулей: сообщения о самом модуле метятся им,
 *       и по журналу видно, какая система отказала
 *
 */
static constexpr const char * __AWH_SCTP_BACKEND__ = "Sun Solaris SCTP backend";

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth/sctp.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод получения статуса SCTP сокета
 *
 * @param sock   сетевой сокет
 * @param status объект для извлечения статуса инициализации SCTP сокета
 * @return       результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::status(const net::socket_t sock, net::sctp::status_t & status) const noexcept {
	// Переменная результата
	bool result = false;
	// Создаём объект статуса SCTP сокета
	struct sctp_status data = {};
	// Устанавливаем идентификатор ассоциации
	data.sstat_assoc_id = status.id;
	// Размер структуры статуса SCTP сокета
	socklen_t length = sizeof(data);
	// Извлекаем статус SCTP сокета
	if(!(result = !static_cast <bool> (::getsockopt(sock, IPPROTO_SCTP, SCTP_STATUS, &data, &length)))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
		#endif
	// Заполняем объект ответа
	} else {
		// Извлекаем идентификатор ассоциации SCTP сокета
		status.id = data.sstat_assoc_id;
		// Извлекаем количество входящих окон SCTP сокета
		status.ratewind = data.sstat_rwnd;
		// Извлекаем количество отправленных SCTP пакетов
		status.unackdata = data.sstat_unackdata;
		// Извлекаем количество ожидающих подтверждений SCTP сокета
		status.penddata = data.sstat_penddata;
		// Извлекаем количество входящих стримов SCTP сокета
		status.istreams = data.sstat_instrms;
		// Извлекаем количество выходящих стримов SCTP сокета
		status.ostreams = data.sstat_outstrms;
		// Извлекаем точку фрагментации SCTP сокета
		status.fragpoint = data.sstat_fragmentation_point;
		/**
		 * Обрабатываем состояние SCTP сокета
		 */
		switch(data.sstat_state){
			// Если состояние SCTP сокета - привязан
			case SCTP_BOUND:
				// Устанавливаем состояние SCTP сокета - привязан
				status.state = net::sctp::state_status_t::BOUND;
			break;
			// Если состояние SCTP сокета - закрытие
			case SCTP_CLOSED:
				// Устанавливаем состояние SCTP сокета - закрыт
				status.state = net::sctp::state_status_t::CLOSED;
			break;
			// Если состояние SCTP сокета - прослушивание
			case SCTP_LISTEN:
				// Устанавливаем состояние SCTP сокета - прослушивание
				status.state = net::sctp::state_status_t::LISTEN;
			break;
			// Если состояние SCTP сокета - установлено
			case SCTP_ESTABLISHED:
				// Устанавливаем состояние SCTP сокета - установлено
				status.state = net::sctp::state_status_t::ESTABLISHED;
			break;
			// Если состояние SCTP сокета - в процессе установления
			case SCTP_COOKIE_WAIT:
				// Устанавливаем состояние SCTP сокета - в процессе установления
				status.state = net::sctp::state_status_t::COOKIE_WAIT;
			break;
			// Если состояние SCTP сокета - ожидание подтверждения cookie
			case SCTP_COOKIE_ECHOED:
				// Устанавливаем состояние SCTP сокета - ожидание подтверждения cookie
				status.state = net::sctp::state_status_t::COOKIE_ECHOED;
			break;
			// Если состояние SCTP сокета - в процессе завершения
			case SCTP_SHUTDOWN_SENT:
				// Устанавливаем состояние SCTP сокета - в процессе завершения
				status.state = net::sctp::state_status_t::SHUTDOWN_SENT;
			break;
			// Если состояние SCTP сокета - завершение
			case SCTP_SHUTDOWN_PENDING:
				// Устанавливаем состояние SCTP сокета - завершение
				status.state = net::sctp::state_status_t::SHUTDOWN_PENDING;
			break;
			// Если состояние SCTP сокета - ожидание завершения
			case SCTP_SHUTDOWN_RECEIVED:
				// Устанавливаем состояние SCTP сокета - ожидание завершения
				status.state = net::sctp::state_status_t::SHUTDOWN_RECEIVED;
			break;
			// Если состояние SCTP сокета - ожидание завершения
			case SCTP_SHUTDOWN_ACK_SENT:
				// Устанавливаем состояние SCTP сокета - ожидание завершения
				status.state = net::sctp::state_status_t::SHUTDOWN_ACK_SENT;
			break;
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод инициализации SCTP сокета
 *
 * @param sock    сетевой сокет
 * @param initmsg параметры инициализации SCTP сокета
 * @return        результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::initMessages(const net::socket_t sock, const net::sctp::initmsg_t & initmsg) const noexcept {
	// Переменная результата
	bool result = false;
	// Создаём объект инициализации SCTP сокета
	struct sctp_initmsg init{0};
	// Устанавливаем количество попыток инициализации
	init.sinit_max_attempts = initmsg.attempts;
	// Устанавливаем таймаут инициализации
	init.sinit_max_init_timeo = initmsg.timeout;
	// Устанавливаем количество выходящих стримов
	init.sinit_num_ostreams = initmsg.ostreams;
	// Устанавливаем количество входящих стримов
	init.sinit_max_instreams = initmsg.istreams;
	// Выполняем инициализацию SCTP сокета
	if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &init, sizeof(init))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод подписки на SCTP события
 *
 * @param sock   сетевой сокет
 * @param events список событий SCTP для активации
 * @return       результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::eventsSubscribe(const net::socket_t sock, const net::sctp::event_types_t & events) const noexcept {
	// Если список событий для подписки пустой
	if(events.empty()){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, events.size()), log_t::flag_t::WARNING, "SCTP events list is empty");
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "SCTP events list is empty");
		#endif
		// Выходим из функции
		return false;
	}
	// Результат подписки через устаревший API SCTP_EVENTS
	bool resultLegacy = true;
	// Результат подписки через современный API SCTP_EVENT
	bool resultModern = true;
	// Создаём объект подписки на события (устаревший API SCTP_EVENTS)
	struct sctp_event_subscribe subscribe{0};
	/**
	 * Оповещение о каждом входящем сообщении взводится ВСЕГДА
	 *
	 * @details Движок читает сокет обычным recvmsg - приём sctp_recvmsg на сокете
	 *          упорядоченных сообщений этими системами не поддерживается вовсе, - а
	 *          сведения о сообщении разбирает из служебного сообщения SCTP_SNDRCV.
	 *          Служебное же это сообщение ядро прикладывает лишь при взведённом
	 *          оповещении о входящих сообщениях
	 *
	 * @warning Без него опознаватель связи приходит нулевым, и отделение связи в
	 *          отдельный сокет (sctp_peeloff) отвергается отказом «Invalid argument».
	 *          Наружу это выходит тем, что сервер не принимает подключений вовсе.
	 *          Проверено прогоном 12.08.2026
	 *
	 * @note Взводится оно сверх запрошенного вызывающей стороной, а не вместо: перебор
	 *       ниже лишь добавляет к набору. Само по себе оповещение это ничего наружу не
	 *       выдаёт - оно кладёт сведения рядом с данными, а не порождает событие
	 *
	 */
	subscribe.sctp_data_io_event = 1;
	/**
	 * Выполняем перебор всех переданных событий SCTP
	 */
	for(auto & event : events){
		/**
		 * Событий, каких эти системы не знают, молча не пропускаем
		 *
		 * @note Сброс ассоциации и смена набора потоков (RFC 6525) подписываются у BSD
		 *       отдельным приёмом SCTP_EVENT. У Sun Solaris приём этот есть, а обозначений
		 *       самих событий НЕТ; у illumos нет и приёма. То же с событиями «отправитель
		 *       сух», сброса потока и проверки подлинности: полей под них нет в структуре
		 *       подписки ни у одной из систем - замерено на обеих
		 *
		 * @warning Молчаливый пропуск отчитался бы УСПЕХОМ о подписке, которой не случилось,
		 *          и потребитель ждал бы событий, каких никогда не придёт. Оттого подписка
		 *          отвечает отказом с названием непокрытого события
		 */
		if((event == net::sctp::event_type_t::ASSOC_RESET_EVENT) || (event == net::sctp::event_type_t::STREAM_CHANGE_EVENT) ||
		   (event == net::sctp::event_type_t::SENDER_DRY_EVENT) || (event == net::sctp::event_type_t::STREAM_RESET_EVENT) ||
		   (event == net::sctp::event_type_t::AUTHENTICATION_EVENT)){
			// Запоминаем отказ в подписке на событие
			resultModern = false;
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s: SCTP event of type %u is not supported by this system", __PRETTY_FUNCTION__, make_tuple(sock, events.size()), log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__, static_cast <uint32_t> (event));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s: SCTP event of type %u is not supported by this system", log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__, static_cast <uint32_t> (event));
			#endif
			// Переходим к следующему событию
			continue;
		}
		/**
		 * Определяем тип события SCTP
		 */
		switch(static_cast <uint8_t> (event)){
			// Если требуется уведомление о каждом входящем DATA-пакете
			case static_cast <uint8_t> (net::sctp::event_type_t::DATA_IO):
				// Устанавливаем уведомления о каждом входящем DATA-пакете
				subscribe.sctp_data_io_event = 1;
			break;
			// Если ошибка удалённого узла
			case static_cast <uint8_t> (net::sctp::event_type_t::REMOTE_ERROR):
				// Устанавливаем события SCTP_PEER_ERROR_EVENT
				subscribe.sctp_peer_error_event = 1;
			break;
			// Если изменение ассоциации
			case static_cast <uint8_t> (net::sctp::event_type_t::ASSOC_CHANGE):
				// Устанавливаем асоциационные события SCTP_ASSOC_CHANGE
				subscribe.sctp_association_event = 1;
			break;
			// Если событие завершения работы
			case static_cast <uint8_t> (net::sctp::event_type_t::SHUTDOWN_EVENT):
				// Устанавливаем события SCTP_SHUTDOWN_EVENT
				subscribe.sctp_shutdown_event = 1;
			break;
			// Если изменение адреса однорангового узла
			case static_cast <uint8_t> (net::sctp::event_type_t::PEER_ADDR_CHANGE):
				// Устанавливаем события SCTP_ADDR_CHANGE
				subscribe.sctp_address_event = 1;
			break;
			// Если событие ошибки отправки (устаревший тип)
			case static_cast <uint8_t> (net::sctp::event_type_t::SEND_FAILED):
			// Если событие ошибки отправки
			case static_cast <uint8_t> (net::sctp::event_type_t::SEND_FAILED_EVENT):
				// Устанавливаем события SCTP_SEND_FAILED_EVENT
				subscribe.sctp_send_failure_event = 1;
			break;
			// Если событие адаптационное указание
			case static_cast <uint8_t> (net::sctp::event_type_t::ADAPTATION_INDICATION):
				// Устанавливаем события SCTP_ADAPTATION_INDICATION
				subscribe.sctp_adaptation_layer_event = 1;
			break;
			// Если событие частичной доставки
			case static_cast <uint8_t> (net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
				// Устанавливаем события SCTP_PARTIAL_DELIVERY_EVENT
				subscribe.sctp_partial_delivery_event = 1;
			break;
		}
	}
	// Выполняем активацию получения событий SCTP для сокета
	if(static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_EVENTS, &subscribe, sizeof(subscribe)))){
		// Запоминаем ошибку подписки через устаревший API
		resultLegacy = false;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, events.size()), log_t::flag_t::CRITICAL, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
		#endif
	}
	// Возвращаем результат
	return (resultModern && resultLegacy);
}
/**
 * @brief Метод установки поддерживаемых алгоритмов аутентификации SCTP сокета
 *
 * @param sock  сетевой сокет
 * @param types список поддерживаемых алгоритмов аутентификации
 * @return      результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::authenticateSupportAlgorithms(const net::socket_t sock, const vector <net::sctp::auth_type_t> & types) const noexcept {
	/**
	 * Проверка подлинности у этих систем отсутствует целиком
	 *
	 * @note Замерено на обеих: нет ни обозначений SCTP_AUTH_KEY, SCTP_AUTH_CHUNK,
	 *       SCTP_AUTH_ACTIVE_KEY, SCTP_AUTH_DELETE_KEY, SCTP_HMAC_IDENT,
	 *       SCTP_LOCAL_AUTH_CHUNKS и SCTP_PEER_AUTH_CHUNKS, ни структур
	 *       sctp_authkey, sctp_authkeyid, sctp_authchunk, sctp_authchunks и
	 *       sctp_hmacalgo. Возместить это в своём коде нельзя: проверка
	 *       подлинности (RFC 4895) делается ядром при сборке пакета
	 *
	 * @warning Отказ здесь ОСОЗНАННЫЙ. Успех без действия означал бы, что
	 *          потребитель считает обмен подтверждённым, тогда как подтверждения
	 *          нет вовсе - это хуже отказа
	 */
	/**
	 * Если включён режим отладки
	 */
	#if DEBUG_MODE
		// Записываем ошибку в лог
		this->_log->debug("%s: SCTP authentication is not implemented by this system", __PRETTY_FUNCTION__, make_tuple(sock), log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	/**
	 * Если режим отладки не включён
	 */
	#else
		// Записываем ошибку в лог
		this->_log->print("%s: SCTP authentication is not implemented by this system", log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	#endif
	// Выводим результат
	return false;
}
/**
 * @brief Метод установки ключа аутентификации SCTP сокета
 *
 * @param sock сетевой сокет
 * @param num  номер ключа аутентификации
 * @param key  ключ аутентификации
 * @return     результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::authenticateKey(const net::socket_t sock, const uint16_t num, string_view key) const noexcept {
	/**
	 * Проверка подлинности у этих систем отсутствует целиком
	 *
	 * @note Довод и перечень недостающего - у метода authenticateSupportAlgorithms
	 */
	/**
	 * Если включён режим отладки
	 */
	#if DEBUG_MODE
		// Записываем ошибку в лог
		this->_log->debug("%s: SCTP authentication is not implemented by this system", __PRETTY_FUNCTION__, make_tuple(sock), log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	/**
	 * Если режим отладки не включён
	 */
	#else
		// Записываем ошибку в лог
		this->_log->print("%s: SCTP authentication is not implemented by this system", log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	#endif
	// Выводим результат
	return false;
}
/**
 * @brief Метод активации/деактивации ключа аутентификации SCTP сокета
 *
 * @param sock сетевой сокет
 * @param mode режим установки типа сокета
 * @param id   идентификатор ассоциации
 * @param num  номер ключа аутентификации
 * @return     результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::authenticateKey(const net::socket_t sock, const net::socket_mode_t mode, const uint32_t id, const uint16_t num) const noexcept {
	/**
	 * Проверка подлинности у этих систем отсутствует целиком
	 *
	 * @note Довод и перечень недостающего - у метода authenticateSupportAlgorithms
	 */
	/**
	 * Если включён режим отладки
	 */
	#if DEBUG_MODE
		// Записываем ошибку в лог
		this->_log->debug("%s: SCTP authentication is not implemented by this system", __PRETTY_FUNCTION__, make_tuple(sock), log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	/**
	 * Если режим отладки не включён
	 */
	#else
		// Записываем ошибку в лог
		this->_log->print("%s: SCTP authentication is not implemented by this system", log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	#endif
	// Выводим результат
	return false;
}
/**
 * @brief Метод установки чанков аутентификации SCTP сокета
 *
 * @param sock   сетевой сокет
 * @param chunks список чанков подлежащих аутентификации
 * @return       результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::authenticateChunks(const net::socket_t sock, const vector <net::sctp::auth_chunk_t> & chunks) const noexcept {
	/**
	 * Проверка подлинности у этих систем отсутствует целиком
	 *
	 * @note Довод и перечень недостающего - у метода authenticateSupportAlgorithms
	 */
	/**
	 * Если включён режим отладки
	 */
	#if DEBUG_MODE
		// Записываем ошибку в лог
		this->_log->debug("%s: SCTP authentication is not implemented by this system", __PRETTY_FUNCTION__, make_tuple(sock), log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	/**
	 * Если режим отладки не включён
	 */
	#else
		// Записываем ошибку в лог
		this->_log->print("%s: SCTP authentication is not implemented by this system", log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	#endif
	// Выводим результат
	return false;
}
/**
 * @brief Метод извлечения чанков аутентификации SCTP сокета
 *
 * @param sock   сетевой сокет
 * @param origin источник события
 * @param id     идентификатор ассоциации
 * @param chunks список чанков подлежащих аутентификации
 * @return       результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::authenticateChunks(const net::socket_t sock, const event::origin_t origin, const uint32_t id, vector <net::sctp::auth_chunk_t> & chunks) const noexcept {
	/**
	 * Проверка подлинности у этих систем отсутствует целиком
	 *
	 * @note Довод и перечень недостающего - у метода authenticateSupportAlgorithms
	 */
	/**
	 * Если включён режим отладки
	 */
	#if DEBUG_MODE
		// Записываем ошибку в лог
		this->_log->debug("%s: SCTP authentication is not implemented by this system", __PRETTY_FUNCTION__, make_tuple(sock), log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	/**
	 * Если режим отладки не включён
	 */
	#else
		// Записываем ошибку в лог
		this->_log->print("%s: SCTP authentication is not implemented by this system", log_t::flag_t::WARNING, ::__AWH_SCTP_BACKEND__);
	#endif
	// Выводим результат
	return false;
}
/**
 * @brief Метод получения таймаута SCTP сокета
 *
 * @param sock сетевой сокет
 * @param id   идентификатор ассоциации
 * @param type тип таймаута
 * @param ctx  контекст установки таймаута
 * @return     значение таймаута в миллисекундах
 *
 */
uint32_t awh::eth::Stream_Control_Transmission_Protocol::timeout(const net::socket_t sock, const uint32_t id, const net::sctp::timeout_t type, void * ctx) const noexcept {
	// Переменная результата
	uint32_t result = 0;
	/**
	 * Определяем тип таймаута
	 */
	switch(static_cast <uint8_t> (type)){
		// Если тип таймаута - INIT
		case static_cast <uint8_t> (net::sctp::timeout_t::INIT): {
			// Создаём объект параметров инициализации SCTP сокета
			struct sctp_initmsg params{0};
			// Устанавливаем длину объекта параметров инициализации
			socklen_t length = sizeof(params);
			// Извлекаем параметры инициализации SCTP сокета
			if(!(result = !static_cast <bool> (::getsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &params, &length)))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), ctx), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			// Устанавливаем значение таймаута инициализации (максимальное время INIT)
			} else result = params.sinit_max_init_timeo;
		} break;
		// Если тип таймаута - DATA
		case static_cast <uint8_t> (net::sctp::timeout_t::DATA): {
			// Создаём объект параметров таймаута SCTP сокета
			struct sctp_rtoinfo params{0};
			// Устанавливаем длину объекта параметров таймаута
			socklen_t length = sizeof(params);
			// Устанавливаем идентификатор ассоциации
			params.srto_assoc_id = id;
			// Извлекаем параметры таймаута SCTP сокета
			if(!(result = !static_cast <bool> (::getsockopt(sock, IPPROTO_SCTP, SCTP_RTOINFO, &params, &length)))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), ctx), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			// Устанавливаем значение таймаута
			} else result = params.srto_initial;
		} break;
		// Если тип таймаута - SACK
		case static_cast <uint8_t> (net::sctp::timeout_t::SACK):
		// Если тип таймаута - SHUTDOWN
		case static_cast <uint8_t> (net::sctp::timeout_t::SHUTDOWN):
		// Если тип таймаута - SHUTDOWNACK
		case static_cast <uint8_t> (net::sctp::timeout_t::SHUTDOWNACK): {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), ctx), log_t::flag_t::WARNING, ::strerror(EOPNOTSUPP));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(EOPNOTSUPP));
			#endif
		} break;
		// Если тип таймаута - HEARTBEAT
		case static_cast <uint8_t> (net::sctp::timeout_t::HEARTBEAT): {
			// Создаём объект параметров таймаута SCTP сокета
			struct sctp_paddrparams params{0};
			// Устанавливаем длину объекта параметров таймаута
			socklen_t length = sizeof(params);
			// Устанавливаем идентификатор ассоциации
			params.spp_assoc_id = id;
			// Если передан контекст установки таймаута
			if(ctx != nullptr)
				// Устанавливаем адрес удалённой стороны из контекста
				::memcpy(&params.spp_address, ctx, sizeof(struct sockaddr_storage));
			// Извлекаем параметры таймаута SCTP сокета
			if(!(result = !static_cast <bool> (::getsockopt(sock, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &params, &length)))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), ctx), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			// Устанавливаем значение таймаута
			} else result = params.spp_hbinterval;
		} break;
		// Если тип таймаута - COOKIE
		case static_cast <uint8_t> (net::sctp::timeout_t::COOKIE): {
			// Создаём объект параметров таймаута SCTP сокета
			struct sctp_assocparams params{0};
			// Устанавливаем длину объекта параметров таймаута
			socklen_t length = sizeof(params);
			// Устанавливаем идентификатор ассоциации
			params.sasoc_assoc_id = id;
			// Извлекаем параметры таймаута SCTP сокета
			if(!(result = !static_cast <bool> (::getsockopt(sock, IPPROTO_SCTP, SCTP_ASSOCINFO, &params, &length)))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), ctx), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			// Устанавливаем значение таймаута
			} else result = params.sasoc_cookie_life;
		} break;
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки таймаута SCTP сокета
 *
 * @param sock    сетевой сокет
 * @param id      идентификатор ассоциации
 * @param type    тип таймаута
 * @param timeout значение таймаута в миллисекундах
 * @param ctx     контекст установки таймаута
 * @return        результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::timeout(const net::socket_t sock, const uint32_t id, const net::sctp::timeout_t type, const uint32_t timeout, void * ctx) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Определяем тип таймаута
	 */
	switch(static_cast <uint8_t> (type)){
		// Если тип таймаута - INIT
		case static_cast <uint8_t> (net::sctp::timeout_t::INIT): {
			// Создаём объект параметров инициализации SCTP сокета
			struct sctp_initmsg params{0};
			// Устанавливаем длину объекта параметров инициализации
			socklen_t length = sizeof(params);
			// Читаем текущие параметры инициализации, чтобы не сбросить остальные поля (потоки/попытки)
			::getsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &params, &length);
			// Устанавливаем новое значение максимального времени INIT
			params.sinit_max_init_timeo = static_cast <uint16_t> (timeout);
			// Активируем новые параметры инициализации SCTP сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &params, sizeof(params))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), timeout, ctx), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		} break;
		// Если тип таймаута - DATA
		case static_cast <uint8_t> (net::sctp::timeout_t::DATA): {
			// Создаём объект параметров таймаута SCTP сокета
			struct sctp_rtoinfo params{0};
			// Устанавливаем идентификатор ассоциации
			params.srto_assoc_id = id;
			// Устанавливаем новое значение таймаута
			params.srto_initial = timeout;
			// Активируем новые таймауты SCTP сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_RTOINFO, &params, sizeof(params))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), timeout, ctx), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		} break;
		// Если тип таймаута - SACK
		case static_cast <uint8_t> (net::sctp::timeout_t::SACK):
		// Если тип таймаута - SHUTDOWN
		case static_cast <uint8_t> (net::sctp::timeout_t::SHUTDOWN):
		// Если тип таймаута - SHUTDOWNACK
		case static_cast <uint8_t> (net::sctp::timeout_t::SHUTDOWNACK): {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), timeout, ctx), log_t::flag_t::WARNING, ::strerror(EOPNOTSUPP));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(EOPNOTSUPP));
			#endif
		} break;
		// Если тип таймаута - HEARTBEAT
		case static_cast <uint8_t> (net::sctp::timeout_t::HEARTBEAT): {
			// Создаём объект параметров таймаута SCTP сокета
			struct sctp_paddrparams params{0};
			// Устанавливаем идентификатор ассоциации
			params.spp_assoc_id = id;
			// Устанавливаем новое значение таймаута
			params.spp_hbinterval = timeout;
			// Если передан контекст установки таймаута
			if(ctx != nullptr)
				// Устанавливаем адрес удалённой стороны из контекста
				::memcpy(&params.spp_address, ctx, sizeof(struct sockaddr_storage));
			/**
			 * Устанавливаем флаг принудительного включения HB с новым интервалом
			 *
			 * @note У illumos структура настроек удалённой стороны СТАРОГО образца,
			 *       без поля признаков: поддержание связи там включается самим
			 *       ненулевым значением промежутка, как это было до RFC 6458.
			 *       Замерено сборкой на обеих системах - у Solaris поле есть
			 */
			#if !defined(__illumos__)
				// Устанавливаем флаг принудительного включения HB с новым интервалом
				params.spp_flags = SPP_HB_ENABLE;
			#endif
			// Активируем новые таймауты SCTP сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &params, sizeof(params))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), timeout, ctx), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		} break;
		// Если тип таймаута - COOKIE
		case static_cast <uint8_t> (net::sctp::timeout_t::COOKIE): {
			// Создаём объект параметров таймаута SCTP сокета
			struct sctp_assocparams params{0};
			// Устанавливаем идентификатор ассоциации
			params.sasoc_assoc_id = id;
			// Устанавливаем новое значение таймаута
			params.sasoc_cookie_life = timeout;
			// Активируем новые таймауты SCTP сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_ASSOCINFO, &params, sizeof(params))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id, static_cast <uint16_t> (type), timeout, ctx), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		} break;
	}
	// Возвращаем результат
	return result;
}
