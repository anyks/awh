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
#include <sys/uio.h>
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

/**
 * @brief Метод опроса возможности протокола SCTP у текущей системы
 *
 * @details Таблица опирается на признаки, объявленные системными заголовочными файлами,
 *          а не на перечень имён систем: возможность появляется у системы с новой её
 *          выпуском, и признак это отражает, а имя - нет
 *
 * @warning У систем Sun проверки подлинности нет вовсе - ни у Solaris, ни у illumos, - как
 *          нет и перенастройки потоков и связи. Отказ этих приёмов есть свойство системы,
 *          а не изъян движка, и опрос заведён ради того, чтобы это можно было узнать
 *          заранее, а не по отказу вызова
 *
 * @param feature опрашиваемая возможность протокола
 * @return        результат опроса возможности
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::supported(const net::sctp::feature_t feature) const noexcept {
	/**
	 * Определяем опрашиваемую возможность протокола
	 */
	switch(static_cast <uint8_t> (feature)){
		// Если опрашивается проверка подлинности сообщений
		case static_cast <uint8_t> (net::sctp::feature_t::AUTHENTICATION):
			#if defined(SCTP_AUTH_KEY) && defined(SCTP_AUTH_ACTIVE_KEY)
				// Выводим положительный результат
				return true;
			#else
				// Выводим отрицательный результат
				return false;
			#endif
		// Если опрашивается перенастройка потоков связи
		case static_cast <uint8_t> (net::sctp::feature_t::STREAM_RESET):
			#if defined(SCTP_RESET_STREAMS)
				// Выводим положительный результат
				return true;
			#else
				// Выводим отрицательный результат
				return false;
			#endif
		// Если опрашивается перенастройка самой связи
		case static_cast <uint8_t> (net::sctp::feature_t::ASSOC_RESET):
			#if defined(SCTP_RESET_ASSOC)
				// Выводим положительный результат
				return true;
			#else
				// Выводим отрицательный результат
				return false;
			#endif
		// Если опрашивается смена состава потоков связи
		case static_cast <uint8_t> (net::sctp::feature_t::STREAM_CHANGE):
			#if defined(SCTP_ADD_STREAMS)
				// Выводим положительный результат
				return true;
			#else
				// Выводим отрицательный результат
				return false;
			#endif
		// Если опрашивается оповещение об опустевшей очереди отправки
		case static_cast <uint8_t> (net::sctp::feature_t::SENDER_DRY):
			#if defined(SCTP_SENDER_DRY_EVENT)
				// Выводим положительный результат
				return true;
			#else
				// Выводим отрицательный результат
				return false;
			#endif
		// Если опрашивается подключение по нескольким адресам одной заявкой
		case static_cast <uint8_t> (net::sctp::feature_t::MULTIHOMING):
			/**
			 * У illumos многодомного подключения одной заявкой нет
			 *
			 * @note Функции `sctp_connectx` там не существует, и связь заводится обычным
			 *       подключением по ПЕРВОМУ адресу списка - прочие остаются без дела
			 */
			#if defined(__illumos__)
				// Выводим отрицательный результат
				return false;
			#else
				// Выводим положительный результат
				return true;
			#endif
		// Если опрашивается явная граница записи при отправке по частям
		case static_cast <uint8_t> (net::sctp::feature_t::PARTIAL_MESSAGE):
			// Выводим результат опроса явной границы записи
			return this->partial();
	}
	// Выводим отрицательный результат: возможность не опознана
	return false;
}
/**
 * @brief Метод проверки поддержки системой современного набора вызовов SCTP
 *
 * @return результат проверки поддержки
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::modern() const noexcept {
	/**
	 * Если система несёт современный набор вызовов
	 */
	#if defined(SCTP_RECVRCVINFO) && defined(SCTP_SENDV_SNDINFO)
		// Выводим положительный результат
		return true;
	/**
	 * Если современного набора вызовов система не несёт
	 */
	#else
		// Выводим отрицательный результат
		return false;
	#endif
}
/**
 * @brief Метод проверки поддержки системой явной границы записи
 *
 * @return результат проверки поддержки
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::partial() const noexcept {
	/**
	 * Если система несёт режим явной границы записи
	 */
	#if defined(SCTP_EXPLICIT_EOR)
		// Выводим положительный результат
		return true;
	/**
	 * Если режима явной границы записи система не несёт
	 */
	#else
		// Выводим отрицательный результат
		return false;
	#endif
}
/**
 * @brief Метод управления подпиской на метаданные принимаемых сообщений
 *
 * @param sock сетевой сокет
 * @param mode режим подписки на метаданные
 * @return     результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::receiveInfo([[maybe_unused]] const net::socket_t sock, [[maybe_unused]] const bool mode) const noexcept {
	/**
	 * Подписки эти у Sun Solaris и illumos ВЗАИМНО ИСКЛЮЧАЮТ друг друга
	 *
	 * @details Замерено на Solaris 11.4: какой из двух приёмов подписки поставлен на
	 *          сокете первым, тот и остаётся, а второй отвергается отказом «Invalid
	 *          argument». Проверено обоими порядками на одном и том же сокете:
	 *
	 *          - сперва SCTP_RECVRCVINFO, следом SCTP_EVENTS - отказ у второго;
	 *          - сперва SCTP_EVENTS, следом SCTP_RECVRCVINFO - отказ у второго.
	 *
	 *          Выбор между ними не вкусовой. Оповещение о входящих сообщениях из
	 *          SCTP_EVENTS кладёт сведения служебным сообщением SCTP_SNDRCV, откуда
	 *          их и берёт чтение, и оно же наполняет опознаватель связи - без него
	 *          отделение связи отвергается, и сервер не принимает подключений вовсе.
	 *          SCTP_RECVRCVINFO же не даёт ни того, ни другого: он нужен лишь чтению
	 *          через sctp_recvv, каким эти системы всё равно не читают
	 *
	 * @warning Оттого метод НИЧЕГО не делает и отвечает согласием: тронув здесь
	 *          SCTP_RECVRCVINFO, мы отключили бы оповещения и положили бы приём
	 *          подключений. Согласие здесь не обман - метаданные подписаны, просто
	 *          другим приёмом, и приходят они при всяком чтении
	 *
	 * @param sock сетевой сокет
	 * @param mode режим подписки на метаданные
	 * @return     результат работы функции
	 *
	 */
	return true;
}
/**
 * @brief Метод управления режимом явной границы записи
 *
 * @param sock сетевой сокет
 * @param mode режим явной границы записи
 * @return     результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::explicitEndOfRecord(const net::socket_t sock, const bool mode) const noexcept {
	/**
	 * Если система несёт режим явной границы записи
	 */
	#if defined(SCTP_EXPLICIT_EOR)
		// Значение режима явной границы записи
		const int32_t value = static_cast <int32_t> (mode);
		// Выполняем установку режима явной границы записи
		if(::setsockopt(sock, IPPROTO_SCTP, SCTP_EXPLICIT_EOR, reinterpret_cast <const char *> (&value), sizeof(value)) != 0){
			// Выводим сообщение об ошибке
			this->_log->print("SCTP explicit end of record: %s", log_t::flag_t::WARNING, ::strerror(errno));
			// Выводим отрицательный результат
			return false;
		}
		// Выводим положительный результат
		return true;
	/**
	 * Если режима явной границы записи система не несёт
	 */
	#else
		// Выводим сообщение об ошибке
		this->_log->print("SCTP explicit end of record is not supported by the operating system", log_t::flag_t::WARNING);
		// Выводим отрицательный результат
		return false;
	#endif
}
/**
 * @brief Метод чтения сообщения SCTP вместе с метаданными
 *
 * @param sock   сетевой сокет
 * @param buffer буфер принимаемых данных
 * @param size   размер буфера принимаемых данных
 * @param addr   адрес удалённого узла
 * @param length размер адреса удалённого узла
 * @param info   метаданные полученного сообщения
 * @param legacy метаданные полученного сообщения прежнего вида
 * @param flags  флаги полученного сообщения в системном виде
 * @return       количество принятых октетов либо -1 при отказе
 *
 */
ssize_t awh::eth::Stream_Control_Transmission_Protocol::receive(const net::socket_t sock, void * buffer, const size_t size, struct sockaddr * addr, socklen_t * length, net::sctp::rinfo_t & info, struct sctp_sndrcvinfo & legacy, int32_t & flags) const noexcept {
	// Количество принятых октетов
	ssize_t result = -1;
	// Выполняем сброс флагов полученного сообщения
	info.flags.clear();
	/**
	 * Выполняем сброс флагов сообщения в системном виде
	 *
	 * @note Довод этот у обоих способов чтения входной И выходной: не обнулив его,
	 *       мы подали бы ядру флаги прошлого приёма как просьбу, а прежний признак
	 *       границы записи пережил бы чтение, у которого её нет
	 */
	flags = 0;
	/**
	 * Чтение идёт обычным recvmsg - и на Solaris тоже, а не только на illumos
	 *
	 * @details Современное чтение sctp_recvv у Sun Solaris есть, но метаданные оно
	 *          берёт из подписки SCTP_RECVRCVINFO, а та ВЗАИМНО ИСКЛЮЧАЕТ подписку
	 *          SCTP_EVENTS - замерено на Solaris 11.4, см. receiveInfo(). Выбрать
	 *          sctp_recvv значило бы остаться без оповещения о входящих сообщениях,
	 *          а без него опознаватель связи приходит нулевым и сервер не принимает
	 *          подключений вовсе. Оттого обе системы читают одним способом
	 *
	 * @warning Приём sctp_recvmsg из libsctp на сокете упорядоченных сообщений
	 *          отвечает отказом «Operation not supported on transport endpoint» -
	 *          проверено пробой на обеих системах 12.08.2026. Обычный же recvmsg
	 *          на том же сокете читает и данные, и оповещения, потому чтение и
	 *          идёт им, а сведения о сообщении разбираются из служебных сообщений
	 *
	 * @note Сведения приходят служебным сообщением SCTP_SNDRCV, а признак того,
	 *       что прочитано оповещение, а не данные - разрядом MSG_NOTIFICATION в
	 *       признаках сообщения
	 */
	// Описание принимаемого сообщения
	struct msghdr message;
	// Описание блока принимаемых данных
	struct iovec iov;
	// Буфер служебных сообщений приёма
	uint8_t control[512];
	/**
	 * Собственный буфер адреса отправителя на случай, если его не спросили
	 *
	 * @warning Буфер адреса здесь ОБЯЗАТЕЛЕН, даже когда сам адрес вызывающему
	 *          не нужен. Чтение с пустым msg_name на сокете упорядоченных
	 *          сообщений отвергается отказом «Operation not supported on
	 *          transport endpoint» - проверено пробой на обеих системах
	 *          12.08.2026, причём проверено дважды: с буфером то же самое
	 *          чтение проходит. В руководстве sctp(4P) это следует из того, что
	 *          связи такого сокета опознаются именно адресом
	 */
	struct sockaddr_storage storage;
	// Зануляем описание принимаемого сообщения
	::memset(&message, 0, sizeof(message));
	// Зануляем собственный буфер адреса отправителя
	::memset(&storage, 0, sizeof(storage));
	// Устанавливаем буфер принимаемых данных
	iov.iov_base = buffer;
	// Устанавливаем размер буфера принимаемых данных
	iov.iov_len = size;
	// Устанавливаем блок принимаемых данных
	message.msg_iov = &iov;
	// Устанавливаем количество блоков принимаемых данных
	message.msg_iovlen = 1;
	// Устанавливаем буфер адреса отправителя, а не спрошенный - собственный
	message.msg_name = ((addr != nullptr) ? reinterpret_cast <void *> (addr) : reinterpret_cast <void *> (&storage));
	// Устанавливаем размер буфера адреса отправителя
	message.msg_namelen = (((addr != nullptr) && (length != nullptr)) ? (* length) : sizeof(storage));
	// Устанавливаем буфер служебных сообщений
	message.msg_control = control;
	// Устанавливаем размер буфера служебных сообщений
	message.msg_controllen = sizeof(control);
	// Выполняем чтение сообщения из сокета
	result = ::recvmsg(sock, &message, 0);
	/**
	 * Если сообщение получено
	 *
	 * @note Прежний способ несёт не все метаданные: номера передачи система
	 *       здесь не выдаёт, и поля эти остаются пустыми. Скрывать разницу
	 *       подстановкой было бы хуже - приложение вправе о ней знать
	 */
	if(result >= 0){
		// Выдаём признаки принятого сообщения
		flags = message.msg_flags;
		// Если размер адреса отправителя запрошен, выдаём его
		if((addr != nullptr) && (length != nullptr))
			// Выдаём размер адреса отправителя
			(* length) = message.msg_namelen;
		/**
		 * Перебираем все служебные сообщения принятого сообщения
		 */
		for(struct cmsghdr * cmsg = CMSG_FIRSTHDR(&message); cmsg != nullptr; cmsg = CMSG_NXTHDR(&message, cmsg)){
			// Если служебное сообщение несёт сведения о сообщении SCTP
			if((cmsg->cmsg_level == IPPROTO_SCTP) && (cmsg->cmsg_type == SCTP_SNDRCV)){
				// Переносим сведения о принятом сообщении
				::memcpy(&legacy, CMSG_DATA(cmsg), sizeof(legacy));
				// Выходим из цикла
				break;
			}
		}
		// Устанавливаем идентификатор полезной нагрузки
		info.ppid = legacy.sinfo_ppid;
		// Устанавливаем номер потока
		info.num = legacy.sinfo_stream;
		// Устанавливаем порядковый номер сообщения в потоке
		info.ssn = legacy.sinfo_ssn;
		// Устанавливаем контекст для уведомлений об ошибках
		info.ctx = legacy.sinfo_context;
		// Устанавливаем идентификатор ассоциации
		info.id = static_cast <uint32_t> (legacy.sinfo_assoc_id);
		/**
		 * Если сообщение доставлено без учёта порядка в потоке
		 */
		#if defined(SCTP_UNORDERED)
			// Если сообщение доставлено без учёта порядка в потоке
			if(legacy.sinfo_flags & SCTP_UNORDERED)
				// Устанавливаем флаг доставки без учёта порядка
				info.flags.emplace(net::sctp::receipt_t::DELIVERY_UNORDERED);
		#endif
	}
	/**
	 * Если сообщение получено
	 */
	if(result >= 0){
		// Если сообщение получено целиком
		if(flags & MSG_EOR)
			// Устанавливаем флаг границы записи
			info.flags.emplace(net::sctp::receipt_t::END_OF_RECORD);
		/**
		 * Если система несёт признак известия протокола
		 */
		#if defined(MSG_NOTIFICATION)
			// Если вместо данных получено известие протокола
			if(flags & MSG_NOTIFICATION)
				// Устанавливаем флаг известия протокола
				info.flags.emplace(net::sctp::receipt_t::NOTIFICATION);
		#endif
		// Если данные сообщения усечены нехваткой буфера
		if(flags & MSG_TRUNC)
			// Устанавливаем флаг усечения данных
			info.flags.emplace(net::sctp::receipt_t::DATA_TRUNCATED);
		// Если метаданные сообщения усечены нехваткой буфера
		if(flags & MSG_CTRUNC)
			// Устанавливаем флаг усечения метаданных
			info.flags.emplace(net::sctp::receipt_t::INFO_TRUNCATED);
	}
	// Выводим количество принятых октетов
	return result;
}
/**
 * @brief Метод отправки сообщения SCTP вместе с метаданными
 *
 * @param sock     сетевой сокет
 * @param buffer   буфер отправляемых данных
 * @param size     размер буфера отправляемых данных
 * @param addr     адрес удалённого узла
 * @param length   размер адреса удалённого узла
 * @param info     информационные метаданные сообщения в системном виде
 * @param complete признак завершения сообщения на этом куске
 * @return         количество отправленных октетов либо -1 при отказе
 *
 */
ssize_t awh::eth::Stream_Control_Transmission_Protocol::send(const net::socket_t sock, const void * buffer, const size_t size, const struct sockaddr * addr, const socklen_t length, const struct sctp_sndrcvinfo & info, const bool complete) const noexcept {
	/**
	 * Если система несёт современный набор вызовов
	 */
	#if defined(SCTP_SENDV_SNDINFO) && defined(SCTP_SENDV_SPA)
		// Буфер отправляемых данных
		struct iovec iov{};
		// Устанавливаем буфер отправляемых данных
		iov.iov_base = const_cast <void *> (buffer);
		// Устанавливаем размер буфера отправляемых данных
		iov.iov_len = size;
		// Набор параметров отправки сообщения
		struct sctp_sendv_spa spa{};
		// Устанавливаем признак заполненности параметров отправки
		spa.sendv_flags = SCTP_SEND_SNDINFO_VALID;
		// Устанавливаем номер потока
		/**
		 * Устанавливаем опознаватель связи
		 *
		 * @warning Без него посылка, идущая без адреса получателя, опознаётся системой
		 *          как заявка на НОВУЮ связь. Sun Solaris и illumos отвечают на это
		 *          отказом EADDRINUSE - связь по этому адресу уже заведена, - и до
		 *          получателя доходит ровно первый кусок. Опознаватель приходит
		 *          известием о смене состояния связи и хранится в метаданных узла
		 */
		spa.sendv_sndinfo.snd_assoc_id = info.sinfo_assoc_id;
		spa.sendv_sndinfo.snd_sid = info.sinfo_stream;
		// Устанавливаем идентификатор полезной нагрузки
		spa.sendv_sndinfo.snd_ppid = info.sinfo_ppid;
		// Устанавливаем контекст для уведомлений об ошибках
		spa.sendv_sndinfo.snd_context = info.sinfo_context;
		// Устанавливаем флаги отправки сообщения
		spa.sendv_sndinfo.snd_flags = static_cast <uint16_t> (info.sinfo_flags);
		/**
		 * Если система несёт политики частичной надёжности
		 */
		#if defined(SCTP_SEND_PRINFO_VALID) && defined(SCTP_PR_SCTP_MASK)
			/**
			 * Политика частичной надёжности сообщения
			 *
			 * @note У прежнего набора вызовов политика едет не отдельным полем, а
			 *       младшими разрядами тех же флагов отправки. Современный набор
			 *       разнёс их порознь, и разряды эти из флагов надлежит убрать -
			 *       иначе политика уедет дважды, вторым разом как чужой флаг
			 */
			const uint16_t policy = static_cast <uint16_t> (info.sinfo_flags & SCTP_PR_SCTP_MASK);
			// Если политика частичной надёжности установлена
			if(policy != 0){
				// Убираем разряды политики из флагов отправки сообщения
				spa.sendv_sndinfo.snd_flags = static_cast <uint16_t> (info.sinfo_flags & ~SCTP_PR_SCTP_MASK);
				// Устанавливаем признак заполненности политики частичной надёжности
				spa.sendv_flags |= SCTP_SEND_PRINFO_VALID;
				// Устанавливаем политику частичной надёжности
				spa.sendv_prinfo.pr_policy = policy;
				// Устанавливаем значение политики частичной надёжности
				spa.sendv_prinfo.pr_value = info.sinfo_timetolive;
			}
		#endif
		// Флаги отправки сообщения
		int32_t flags = 0;
		/**
		 * Если система умеет подавлять сигнал разорванного канала при отправке
		 *
		 * @warning Флаг этот ОБЯЗАТЕЛЕН: отправка в оборванное соединение без него
		 *          валит процесс потребителя сигналом `SIGPIPE`, а библиотека ронять
		 *          чужой процесс не смеет. Вскрыто ворошителем на Debian 12 у наречия
		 *          GNU (код выхода 141, воспроизводится дважды из трёх, место снято
		 *          отладчиком); здесь правится тем же порядком, потому что устройство
		 *          отправки то же самое. Прочие пути движка защищены `MSG_NOSIGNAL`
		 *          давно - этот оставался открытым, поскольку флаги уходят библиотеке,
		 *          а не ядру напрямую, и по имени вызова этого не видно
		 */
		#if defined(MSG_NOSIGNAL)
			// Подавляем сигнал разорванного канала
			flags |= MSG_NOSIGNAL;
		#endif
		/**
		 * Если сообщение завершается на этом куске
		 *
		 * @note Граница записи ставится в явном виде лишь тогда, когда включён режим
		 *       явной границы: без него система закрывает запись на каждой отправке
		 *       сама, и флаг этот ей ничего не меняет
		 */
		if(complete)
			// Устанавливаем флаг границы записи
			flags |= MSG_EOR;
		// Выполняем отправку сообщения вместе с метаданными
		const ssize_t bytes = ::sctp_sendv(sock, &iov, 1, const_cast <struct sockaddr *> (addr), ((addr != nullptr) ? 1 : 0), &spa, sizeof(spa), SCTP_SENDV_SPA, flags);
		/**
		 * Приводим успешный исход к числу отправленных октетов
		 *
		 * @details У систем Sun `sctp_sendv` объявлен отдающим `ssize_t`, но при успехе
		 *          возвращает НУЛЬ, а не число отправленных октетов: отправка идёт через
		 *          `ioctl(SIOCSCTPSNDV)`, и его успешный исход отдаётся как есть. Сообщение
		 *          при этом уходит целиком - частичной отправки у этого приёма нет
		 *
		 * @warning Без приведения нуль достаётся вызывающей стороне как «отправлено нуль
		 *          октетов», а это признак давно закрытого сокета - и движок сносит живую
		 *          связь. Доказано отдельным щупом на Solaris 11.4: отправка 512 октетов
		 *          отдала нуль при `errno` равном нулю, а трассировка ядра показала
		 *          успешный `ioctl(3, SIOCSCTPSNDV) = 0`
		 */
		return ((bytes == 0) ? static_cast <ssize_t> (size) : bytes);
	/**
	 * Если современного набора вызовов система не несёт
	 */
	#else
		/**
		 * Отправка по частям здесь невозможна, но отказом это не считается
		 *
		 * @details Прежний способ отправки границу записи в явном виде задать не даёт:
		 *          флагов у него нет вовсе, и сообщение придёт несколькими записями
		 *          вместо одной. Обмен это не рвёт, и отменять отправку незачем
		 *
		 * @warning Предупреждать отсюда НЕЛЬЗЯ: метод зовётся на всякий кусок, и на
		 *          четырёх мегабайтах вышло бы больше тысячи одинаковых строк в
		 *          журнале. О понижении сообщает движок - один раз на узел, при
		 *          заведении режима явной границы записи
		 */
		/**
		 * Отправка идёт вызовом sendmsg со сведениями в служебных данных
		 *
		 * @details Прежний вызов sctp_sendmsg опознавателя связи среди доводов не несёт
		 *          вовсе: связь он выбирает по адресу получателя. Отправке без адреса -
		 *          а движок шлёт именно так, узел уже подключён - выбирать связь тогда
		 *          нечем, и система вправе счесть посылку заявкой на НОВУЮ связь. Sun
		 *          Solaris и illumos так и делают, отвечая отказом EADDRINUSE, потому что
		 *          связь по этому адресу уже заведена: до получателя доходит ровно первый
		 *          кусок. Служебные данные же опознаватель несут, и по нему связь
		 *          выбирается однозначно
		 *
		 * @warning Границу записи в явном виде прежний способ задать не даёт: сообщение
		 *          придёт несколькими записями вместо одной. Обмен это не рвёт, и
		 *          отменять отправку незачем. Предупреждать отсюда НЕЛЬЗЯ - метод зовётся
		 *          на всякий кусок, и на четырёх мегабайтах вышло бы больше тысячи
		 *          одинаковых строк в журнале. О понижении сообщает движок, один раз на
		 *          узел, при заведении режима явной границы записи
		 */
		// Буфер отправляемых данных
		struct iovec iov{};
		// Устанавливаем буфер отправляемых данных
		iov.iov_base = const_cast <void *> (buffer);
		// Устанавливаем размер буфера отправляемых данных
		iov.iov_len = size;
		// Заголовок отправляемого сообщения
		struct msghdr message{};
		// Буфер служебных данных сообщения
		uint8_t control[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
		// Очищаем буфер служебных данных сообщения
		::memset(control, 0, sizeof(control));
		// Устанавливаем буфер отправляемых данных
		message.msg_iov = &iov;
		// Устанавливаем количество буферов отправляемых данных
		message.msg_iovlen = 1;
		// Устанавливаем адрес получателя, если он передан
		message.msg_name = const_cast <struct sockaddr *> (addr);
		// Устанавливаем размер адреса получателя
		message.msg_namelen = ((addr != nullptr) ? length : 0);
		// Устанавливаем буфер служебных данных сообщения
		message.msg_control = control;
		// Устанавливаем размер служебных данных сообщения
		message.msg_controllen = sizeof(control);
		// Получаем заголовок служебных данных сообщения
		struct cmsghdr * cmsg = CMSG_FIRSTHDR(&message);
		// Устанавливаем уровень служебных данных сообщения
		cmsg->cmsg_level = IPPROTO_SCTP;
		// Устанавливаем тип служебных данных сообщения
		cmsg->cmsg_type = SCTP_SNDRCV;
		// Устанавливаем размер служебных данных сообщения
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
		// Получаем сведения отправляемого сообщения
		struct sctp_sndrcvinfo * sndrcv = reinterpret_cast <struct sctp_sndrcvinfo *> (CMSG_DATA(cmsg));
		// Копируем сведения отправляемого сообщения целиком, вместе с опознавателем связи
		(* sndrcv) = info;
		// Выполняем отправку сообщения вместе со сведениями
		return ::sendmsg(sock, &message, 0);
	#endif
}
