/**
 * @file: sctp.cpp
 * @date: 2026-08-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация бэкенда протокола SCTP — настройка параметров ассоциаций, входящих и исходящих потоков,
 *        heartbeat, авторизации и подписки на уведомления SCTP-сокета
 *
 * @copyright: Copyright © 2026
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
			/**
			 * Если состояние SCTP сокета - ассоциации нет
			 *
			 * @details Набор состояний у Linux свой: привязки и прослушивания среди них
			 *          нет вовсе. Состояния эти принадлежат **сокету**, а не ассоциации,
			 *          и Linux их отсюда не выдаёт - отчего сокет, привязанный либо
			 *          слушающий, числится здесь без ассоциации
			 *
			 * @note Взамен есть SCTP_EMPTY, которого нет у FreeBSD: им обозначается
			 *       именно отсутствие ассоциации, а не её закрытие. Наружу оба выдаются
			 *       как закрытое состояние - иного значения набор не держит, а заводить
			 *       его ради одной системы значило бы менять API ради частности
			 */
			case SCTP_EMPTY:
			// Если состояние SCTP сокета - закрытие
			case SCTP_CLOSED:
				// Устанавливаем состояние SCTP сокета - закрыт
				status.state = net::sctp::state_status_t::CLOSED;
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
	 * Выполняем перебор всех переданных событий SCTP
	 */
	for(auto & event : events){
		/**
		 * Современные события (RFC 6525) подписываются через отдельный API SCTP_EVENT,
		 * так как они отсутствуют в устаревшей структуре sctp_event_subscribe
		 */
		if((event == net::sctp::event_type_t::ASSOC_RESET_EVENT) || (event == net::sctp::event_type_t::STREAM_CHANGE_EVENT)){
			// Создаём объект подписки на событие (современный API SCTP_EVENT)
			struct sctp_event item{0};
			// Активируем получение события
			item.se_on = 1;
			// Применяем подписку ко всем будущим ассоциациям
			item.se_assoc_id = SCTP_FUTURE_ASSOC;
			// Устанавливаем тип события SCTP
			item.se_type = ((event == net::sctp::event_type_t::ASSOC_RESET_EVENT) ? SCTP_ASSOC_RESET_EVENT : SCTP_STREAM_CHANGE_EVENT);
			// Выполняем подписку на событие SCTP
			if(static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_EVENT, &item, sizeof(item)))){
				// Запоминаем ошибку подписки через современный API
				resultModern = false;
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
			// Если событие "отправитель сухой"
			case static_cast <uint8_t> (net::sctp::event_type_t::SENDER_DRY_EVENT):
				// Устанавливаем события SCTP_SENDER_DRY_EVENT
				subscribe.sctp_sender_dry_event = 1;
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
			// Если событие сброса потока
			case static_cast <uint8_t> (net::sctp::event_type_t::STREAM_RESET_EVENT):
				// Устанавливаем события сброса потока SCTP_STREAM_RESET_EVENT
				subscribe.sctp_stream_reset_event = 1;
			break;
			// Если событие аутентификации
			case static_cast <uint8_t> (net::sctp::event_type_t::AUTHENTICATION_EVENT):
				// Устанавливаем события SCTP_AUTHENTICATION_INDICATION
				subscribe.sctp_authentication_event = 1;
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
	// Переменная результата
	bool result = false;
	// Если количество поддерживаемых алгоритмов аутентификации передано
	if(!types.empty()){
		// Объект поддерживаемых алгоритмов аутентификации SCTP сокета
		struct sctp_hmacalgo * hmac = nullptr;
		// Вычисляем длину структуры поддерживаемых алгоритмов аутентификации SCTP сокета
		const size_t length = (offsetof(struct sctp_hmacalgo, shmac_idents) + types.size() * sizeof(uint16_t));
		// Выделяем память под объект поддерживаемых алгоритмов аутентификации SCTP сокета
		hmac = reinterpret_cast <struct sctp_hmacalgo *> (::calloc(1, length));
		// Если память под объект не выделена
		if(hmac == nullptr)
			// Выходим из функции
			return result;
		// Устанавливаем количество поддерживаемых алгоритмов аутентификации SCTP сокета
		hmac->shmac_number_of_idents = static_cast <uint32_t> (types.size());
		// Индекс для записи поддерживаемых алгоритмов аутентификации SCTP сокета
		uint32_t index = 0;
		/**
		 * Выполняем перебор всех переданных типов алгоритмов аутентификации
		 */
		for(auto & type : types){
			/**
			 * Определяем тип аутентификации
			 */
			switch(static_cast <uint8_t> (type)){
				// Если тип аутентификации - HMAC-SHA1
				case static_cast <uint8_t> (net::sctp::auth_type_t::HMAC_SHA1):
					// Устанавливаем номер ключа аутентификации для HMAC-SHA1
					hmac->shmac_idents[index++] = SCTP_AUTH_HMAC_ID_SHA1; // = 1
				break;
				// Если тип аутентификации - HMAC-SHA256
				case static_cast <uint8_t> (net::sctp::auth_type_t::HMAC_SHA256):
					// Устанавливаем номер ключа аутентификации для HMAC-SHA256
					hmac->shmac_idents[index++] = SCTP_AUTH_HMAC_ID_SHA256; // = 3
				break;
			}
		}
		// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP сокета
		if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_HMAC_IDENT, hmac, length)))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, types.size()), log_t::flag_t::WARNING, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
			#endif
		}
		// Очищаем память под объект поддерживаемых алгоритмов аутентификации SCTP сокета
		::free(hmac);
	}
	// Возвращаем результат
	return result;
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
	// Переменная результата
	bool result = false;
	// Если ключ аутентификации передан
	if(!key.empty()){
		// Получаем размер ключа аутентификации
		const socklen_t size = static_cast <socklen_t> (offsetof(sctp_authkey, sca_key) + key.size());
		// Выделяем память под ключ аутентификации
		struct sctp_authkey * authkey = reinterpret_cast <sctp_authkey *> (::calloc(1, size));
		// Если память под ключ не выделена
		if(authkey == nullptr)
			// Выходим из функции
			return result;
		// Устанавливаем идентификатор ассоциации
		authkey->sca_assoc_id = 0;
		// Устанавливаем номер ключа аутентификации
		authkey->sca_keynumber = num;
		// Устанавливаем размер ключа аутентификации
		authkey->sca_keylength = static_cast <uint16_t> (key.size());
		// Копируем ключ аутентификации в структуру
		::memcpy(authkey->sca_key, key.data(), key.size());
		// Устанавливаем ключ аутентификации SCTP сокета
		if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_AUTH_KEY, authkey, size)))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, num, key), log_t::flag_t::WARNING, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
			#endif
		}
		// Очищаем память под ключ аутентификации
		::free(authkey);
	}
	// Возвращаем результат
	return result;
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
	// Переменная результата
	bool result = false;
	// Создаём объект идентификатора ключа аутентификации
	struct sctp_authkeyid authkeyid{0};
	// Устанавливаем идентификатор ассоциации
	authkeyid.scact_assoc_id = id;
	// Устанавливаем номер ключа аутентификации
	authkeyid.scact_keynumber = num;
	/**
	 * Определяем режим активации/деактивации ключа аутентификации SCTP сокета
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать ключ аутентификации SCTP сокета
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): {
			// Активируем ключ аутентификации SCTP сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_AUTH_ACTIVE_KEY, &authkeyid, sizeof(authkeyid))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, static_cast <uint16_t> (mode), id, num), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		} break;
		// Если необходимо деактивировать ключ аутентификации SCTP сокета
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): {
			// Деактивируем ключ аутентификации SCTP сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_AUTH_DELETE_KEY, &authkeyid, sizeof(authkeyid))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, static_cast <uint16_t> (mode), id, num), log_t::flag_t::WARNING, ::strerror(errno));
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
 * @brief Метод установки чанков аутентификации SCTP сокета
 *
 * @param sock   сетевой сокет
 * @param chunks список чанков подлежащих аутентификации
 * @return       результат работы функции
 *
 */
bool awh::eth::Stream_Control_Transmission_Protocol::authenticateChunks(const net::socket_t sock, const vector <net::sctp::auth_chunk_t> & chunks) const noexcept {
	// Переменная результата
	bool result = false;
	// Если количество чанков аутентификации передано
	if(!chunks.empty()){
		// Создаём объект чанка аутентификации SCTP сокета
		struct sctp_authchunk authchunk{0};
		/**
		 * Выполняем перебор всех переданных чанков аутентификации
		 */
		for(auto & chunk : chunks){
			/**
			 * Определяем тип чанка аутентификации
			 */
			switch(static_cast <uint8_t> (chunk)){
				// Если чанк аутентификации - DATA
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::DATA):
					// Устанавливаем чанк аутентификации DATA
					authchunk.sauth_chunk = 0x00;
				break;
				// Если чанк аутентификации - INIT
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::INIT):
					// Устанавливаем чанк аутентификации INIT
					authchunk.sauth_chunk = 0x01;
				break;
				// Если чанк аутентификации - INIT_ACK
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::INIT_ACK):
					// Устанавливаем чанк аутентификации INIT_ACK
					authchunk.sauth_chunk = 0x02;
				break;
				// Если чанк аутентификации - SACK
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::SACK):
					// Устанавливаем чанк аутентификации SACK
					authchunk.sauth_chunk = 0x03;
				break;
				// Если чанк аутентификации - HEARTBEAT
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::HEARTBEAT):
					// Устанавливаем чанк аутентификации HEARTBEAT
					authchunk.sauth_chunk = 0x04;
				break;
				// Если чанк аутентификации - HEARTBEAT_ACK
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::HEARTBEAT_ACK):
					// Устанавливаем чанк аутентификации HEARTBEAT_ACK
					authchunk.sauth_chunk = 0x05;
				break;
				// Если чанк аутентификации - ABORT
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::ABORT):
					// Устанавливаем чанк аутентификации ABORT
					authchunk.sauth_chunk = 0x06;
				break;
				// Если чанк аутентификации - SHUTDOWN
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::SHUTDOWN):
					// Устанавливаем чанк аутентификации SHUTDOWN
					authchunk.sauth_chunk = 0x07;
				break;
				// Если чанк аутентификации - SHUTDOWN_ACK
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::SHUTDOWN_ACK):
					// Устанавливаем чанк аутентификации SHUTDOWN_ACK
					authchunk.sauth_chunk = 0x08;
				break;
				// Если чанк аутентификации - ERROR
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::ERROR):
					// Устанавливаем чанк аутентификации ERROR
					authchunk.sauth_chunk = 0x09;
				break;
				// Если чанк аутентификации - COOKIE_ECHO
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::COOKIE_ECHO):
					// Устанавливаем чанк аутентификации COOKIE_ECHO
					authchunk.sauth_chunk = 0x0A;
				break;
				// Если чанк аутентификации - COOKIE_ACK
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::COOKIE_ACK):
					// Устанавливаем чанк аутентификации COOKIE_ACK
					authchunk.sauth_chunk = 0x0B;
				break;
				// Если чанк аутентификации - ECNE
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::ECNE):
					// Устанавливаем чанк аутентификации ECNE
					authchunk.sauth_chunk = 0x0C;
				break;
				// Если чанк аутентификации - CWR
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::CWR):
					// Устанавливаем чанк аутентификации CWR
					authchunk.sauth_chunk = 0x0D;
				break;
				// Если чанк аутентификации - SHUTDOWN_COMPLETE
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::SHUTDOWN_COMPLETE):
					// Устанавливаем чанк аутентификации SHUTDOWN_COMPLETE
					authchunk.sauth_chunk = 0x0E;
				break;
				// Если чанк аутентификации - AUTH
				case static_cast <uint8_t> (net::sctp::auth_chunk_t::AUTH):
					// Устанавливаем чанк аутентификации AUTH
					authchunk.sauth_chunk = 0x0F;
				break;
			}
			// Активируем чанк аутентификации SCTP сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_AUTH_CHUNK, &authchunk, sizeof(authchunk))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, chunks.size()), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
				// Прекращаем дальнейшую обработку чанков аутентификации
				break;
			}
		}
	}
	// Возвращаем результат
	return result;
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
	// Переменная результата
	bool result = false;
	// Максимум 256 типов чанков — более чем достаточно
	const socklen_t capacity = static_cast <socklen_t> (sizeof(struct sctp_authchunks) + 256);
	// Выделяем память под объект чанков аутентификации SCTP сокета (с запасом под гибкий массив gauth_chunks)
	struct sctp_authchunks * authchunks = reinterpret_cast <struct sctp_authchunks *> (::calloc(1, capacity));
	// Если память под объект не выделена
	if(authchunks == nullptr)
		// Выходим из функции
		return result;
	// Устанавливаем идентификатор ассоциации
	authchunks->gauth_assoc_id = id;
	// Размер выделенного буфера под чанки аутентификации
	socklen_t length = capacity;
	// Переменная для хранения типа опции
	int32_t optname = 0;
	/**
	 * Определяем источник события
	 */
	switch(static_cast <uint8_t> (origin)){
		// Если источник события - локальный
		case static_cast <uint8_t> (event::origin_t::LOCAL):
			// Устанавливаем тип опции - локальные чанки аутентификации
			optname = SCTP_LOCAL_AUTH_CHUNKS;
		break;
		// Если источник события - удалённый
		case static_cast <uint8_t> (event::origin_t::REMOTE):
			// Устанавливаем тип опции - удалённые чанки аутентификации
			optname = SCTP_PEER_AUTH_CHUNKS;
		break;
	}
	// Получаем чанки аутентификации SCTP сокета
	if((result = (::getsockopt(sock, IPPROTO_SCTP, optname, authchunks, &length) == 0))){
		// Вычисляем количество полученных чанков из фактически возвращённой длины буфера
		const uint32_t count = (length > offsetof(struct sctp_authchunks, gauth_chunks)) ? static_cast <uint32_t> (length - offsetof(struct sctp_authchunks, gauth_chunks)) : 0;
		/**
		 * Перебираем все полученные чанки аутентификации
		 */
		for(uint32_t i = 0; i < count; i++){
			/**
			 * Определяем тип чанка аутентификации
			 */
			switch(authchunks->gauth_chunks[i]){
				// Если чанк аутентификации - DATA
				case 0x00:
					// Добавляем чанк аутентификации DATA
					chunks.push_back(net::sctp::auth_chunk_t::DATA);
				break;
				// Если чанк аутентификации - INIT
				case 0x01:
					// Добавляем чанк аутентификации INIT
					chunks.push_back(net::sctp::auth_chunk_t::INIT);
				break;
				// Если чанк аутентификации - INIT_ACK
				case 0x02:
					// Добавляем чанк аутентификации INIT_ACK
					chunks.push_back(net::sctp::auth_chunk_t::INIT_ACK);
				break;
				// Если чанк аутентификации - SACK
				case 0x03:
					// Добавляем чанк аутентификации SACK
					chunks.push_back(net::sctp::auth_chunk_t::SACK);
				break;
				// Если чанк аутентификации - HEARTBEAT
				case 0x04:
					// Добавляем чанк аутентификации HEARTBEAT
					chunks.push_back(net::sctp::auth_chunk_t::HEARTBEAT);
				break;
				// Если чанк аутентификации - HEARTBEAT_ACK
				case 0x05:
					// Добавляем чанк аутентификации HEARTBEAT_ACK
					chunks.push_back(net::sctp::auth_chunk_t::HEARTBEAT_ACK);
				break;
				// Если чанк аутентификации - ABORT
				case 0x06:
					// Добавляем чанк аутентификации ABORT
					chunks.push_back(net::sctp::auth_chunk_t::ABORT);
				break;
				// Если чанк аутентификации - SHUTDOWN
				case 0x07:
					// Добавляем чанк аутентификации SHUTDOWN
					chunks.push_back(net::sctp::auth_chunk_t::SHUTDOWN);
				break;
				// Если чанк аутентификации - SHUTDOWN_ACK
				case 0x08:
					// Добавляем чанк аутентификации SHUTDOWN_ACK
					chunks.push_back(net::sctp::auth_chunk_t::SHUTDOWN_ACK);
				break;
				// Если чанк аутентификации - ERROR
				case 0x09:
					// Добавляем чанк аутентификации ERROR
					chunks.push_back(net::sctp::auth_chunk_t::ERROR);
				break;
				// Если чанк аутентификации - COOKIE_ECHO
				case 0x0A:
					// Добавляем чанк аутентификации COOKIE_ECHO
					chunks.push_back(net::sctp::auth_chunk_t::COOKIE_ECHO);
				break;
				// Если чанк аутентификации - COOKIE_ACK
				case 0x0B:
					// Добавляем чанк аутентификации COOKIE_ACK
					chunks.push_back(net::sctp::auth_chunk_t::COOKIE_ACK);
				break;
				// Если чанк аутентификации - ECNE
				case 0x0C:
					// Добавляем чанк аутентификации ECNE
					chunks.push_back(net::sctp::auth_chunk_t::ECNE);
				break;
				// Если чанк аутентификации - CWR
				case 0x0D:
					// Добавляем чанк аутентификации CWR
					chunks.push_back(net::sctp::auth_chunk_t::CWR);
				break;
				// Если чанк аутентификации - SHUTDOWN_COMPLETE
				case 0x0E:
					// Добавляем чанк аутентификации SHUTDOWN_COMPLETE
					chunks.push_back(net::sctp::auth_chunk_t::SHUTDOWN_COMPLETE);
				break;
				// Если чанк аутентификации - AUTH
				case 0x0F:
					// Добавляем чанк аутентификации AUTH
					chunks.push_back(net::sctp::auth_chunk_t::AUTH);
				break;
				// Если чанк аутентификации - FORWARD_TSN
				case 0x80:
					// Добавляем чанк аутентификации FORWARD_TSN
					chunks.push_back(net::sctp::auth_chunk_t::FORWARD_TSN);
				break;
				// Если чанк аутентификации - RE_CONFIG
				case 0xC1:
					// Добавляем чанк аутентификации RE_CONFIG
					chunks.push_back(net::sctp::auth_chunk_t::RE_CONFIG);
				break;
			}
		}
	// Если возникает ошибка получения протокола сокета
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, id), log_t::flag_t::WARNING, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Очищаем память под объект чанков аутентификации SCTP сокета
	::free(authchunks);
	// Возвращаем результат
	return result;
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
			// Устанавливаем флаг принудительного включения HB с новым интервалом
			params.spp_flags = SPP_HB_ENABLE;
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
