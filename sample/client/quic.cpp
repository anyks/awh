/**
 * @file: quic.cpp
 * @date: 2026-07-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример клиента QUIC — демонстрация установки соединения по протоколу QUIC через фасад клиента,
 *        выполнения рукопожатия, открытия потоков и обмена данными с сервером
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем стандартные модули
 */
#include <cinttypes>

/**
 * Подключаем заголовочный файл проекта
 */
#include <client/client.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Класс объекта исполнителя эхо-клиента QUIC
 *
 */
class Executor {
	private:
		// Объект фреймворка
		const fmk_t * _fmk;
		// Объект работы с логами
		const log_t * _log;
	private:
		// Идентификатор потока обмена полезной нагрузкой
		uint64_t _sid;
		// Отправленная серверу полезная нагрузка потока (эталон для верификации эхо-ответа)
		string _sent;
		// Накопленный от сервера эхо-ответ потока
		string _received;
		// Отправленная серверу датаграмма приложения (эталон для верификации эхо-ответа)
		string _datagram;
	private:
		// Флаг завершения обмена потоком (данные потока возвращены и сверены)
		bool _streamDone;
		// Флаг завершения обмена датаграммой (датаграмма возвращена и сверена либо не поддерживается)
		bool _datagramDone;
	private:
		/**
		 * @brief Метод завершения работы клиента по окончании обмена
		 *
		 * @note Клиент останавливается только когда завершены оба обмена - потоком и
		 *       датаграммой (либо датаграммы не поддерживаются удалённым сервером)
		 *
		 * @param client объект клиента
		 *
		 */
		void maybeFinish(client_t * client) noexcept {
			// Если обмен потоком и датаграммой завершены
			if(this->_streamDone && this->_datagramDone)
				// Останавливаем работу клиента
				client->stop();
		}
	private:
		/**
		 * @brief Метод формирования осмысленной полезной нагрузки для проверки эхо
		 *
		 * @note Отправляется распознаваемый текст HTTP-запроса: эхо-сервер возвращает
		 *       его без изменений, а клиент сверяет принятое с отправленным
		 *
		 * @return сформированная полезная нагрузка
		 *
		 */
		string makePayload() const noexcept {
			// Формируем распознаваемый текст HTTP-запроса как полезную нагрузку
			return string(
				"GET / HTTP/1.1\r\n"
				"Host: anyks.com\r\n"
				"Connection: close\r\n"
				"User-Agent: awh-quic-sample/1.0\r\n"
				"\r\n"
			);
		}
	public:
		/**
		 * @brief Метод обработки событий изменения статуса клиента
		 *
		 * @param status новый статус клиента
		 * @param client объект клиента
		 *
		 */
		void status(const event::status_t status, client_t * client) noexcept {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					// Выполняем подключение клиента к удалённому серверу
					client->connect();
				break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события клиента
					this->_log->print("QUIC client destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки готовности клиента к работе
		 *
		 * @param family семейство адресов клиента
		 * @param domain доменное имя удалённого сервера
		 * @param ip     IP-адрес удалённого сервера
		 *
		 */
		void ready([[maybe_unused]] const event::family_t family, const string & domain, const string & ip) noexcept {
			// Записываем в лог сообщение о готовности клиента к работе
			this->_log->print("QUIC client is ready: %s (%s)", log_t::flag_t::INFO, domain.c_str(), ip.c_str());
		}
		/**
		 * @brief Метод обработки события подключения к серверу
		 *
		 * @param ok     результат подключения к серверу
		 * @param client объект клиента
		 *
		 */
		void connect(const bool ok, client_t * client) noexcept {
			// Если подключение к серверу не выполнено
			if(!ok){
				// Записываем ошибку в лог
				this->_log->print("QUIC connection failed", log_t::flag_t::CRITICAL);
				// Выходим из метода
				return;
			}
			// Записываем в лог сообщение об установленном соединении
			this->_log->print("QUIC connection established", log_t::flag_t::INFO);
			// Формируем осмысленную полезную нагрузку и сохраняем её как эталон для верификации
			this->_sent = this->makePayload();
			// Очищаем накопитель эхо-ответа перед началом обмена
			this->_received.clear();
			// Открываем двунаправленный поток приложения для обмена нагрузкой (RFC 9000 §2.1)
			this->_sid = client->open(false);
			// Если поток приложения открыть не удалось
			if(this->_sid == quic::connection_t::INVALID_STREAM){
				// Записываем ошибку в лог
				this->_log->print("Failed to open QUIC stream", log_t::flag_t::CRITICAL);
				// Выходим из метода
				return;
			}
			/**
			 * Отправляем всю нагрузку одним потоком с флагом завершения: сервер вернёт
			 * её обратно тем же потоком, а клиент сверит принятое с отправленным
			 */
			if(client->send(this->_sid, this->_sent.data(), this->_sent.size(), true) > 0){
				// Записываем в лог сообщение об отправке нагрузки
				this->_log->print("Sent payload: Stream=%" PRIu64 ", %zu bytes", log_t::flag_t::INFO, this->_sid, this->_sent.size());
				// Печатаем отправленную полезную нагрузку, чтобы видеть её содержимое
				cout << endl << "==== SENT REQUEST (" << this->_sent.size() << " bytes) ====" << endl
				     << this->_sent
				     << "==== END OF REQUEST ====" << endl;
			// Если отправка нагрузки не выполнена
			} else this->_log->print("Failed to send payload to server", log_t::flag_t::WARNING);
			/**
			 * Дополнительно отправляем датаграмму приложения (RFC 9221): ненадёжная
			 * доставка вне потока (без гарантий, без ретрансмита) - применяется для
			 * данных реального времени. Сервер вернёт её обратно датаграммой
			 */
			this->_datagram = "AWH QUIC DATAGRAM (RFC 9221): fire-and-forget hello";
			// Получаем согласованный хендшейком предельный размер отправляемой датаграммы
			const size_t limit = client->datagrams();
			// Если удалённый сервер поддерживает датаграммы и нагрузка помещается в предел
			if((limit > 0) && (this->_datagram.size() <= limit)){
				// Отправляем датаграмму приложения серверу
				if(client->datagram(this->_datagram.data(), this->_datagram.size())){
					// Записываем в лог сообщение об отправке датаграммы
					this->_log->print("Sent datagram: %zu bytes", log_t::flag_t::INFO, this->_datagram.size());
					// Печатаем отправленную датаграмму, чтобы видеть её содержимое
					cout << endl << "==== SENT DATAGRAM (" << this->_datagram.size() << " bytes) ====" << endl
					     << this->_datagram << endl
					     << "==== END OF DATAGRAM ====" << endl;
				// Если отправка датаграммы не выполнена - не ожидаем её эхо
				} else {
					// Записываем ошибку в лог
					this->_log->print("Failed to send datagram to server", log_t::flag_t::WARNING);
					// Помечаем обмен датаграммой завершённым
					this->_datagramDone = true;
				}
			// Если датаграммы удалённым сервером не поддерживаются - не ожидаем её эхо
			} else {
				// Записываем в лог сообщение об отсутствии поддержки датаграмм
				this->_log->print("QUIC datagrams are not supported by the peer, skipping", log_t::flag_t::WARNING);
				// Помечаем обмен датаграммой завершённым
				this->_datagramDone = true;
			}
		}
		/**
		 * @brief Метод обработки собранных данных потока приложения QUIC (эхо-ответ сервера)
		 *
		 * @param sid    идентификатор потока приложения
		 * @param data   собранные данные потока
		 * @param fin    флаг завершения потока
		 * @param client объект клиента
		 *
		 */
		void stream(const uint64_t sid, const string & data, const bool fin, client_t * client) noexcept {
			// Накапливаем принятый от сервера эхо-ответ (поток может прийти частями)
			this->_received.append(data);
			// Записываем в лог сведения о принятой части эхо-ответа
			this->_log->print("Echo chunk: Stream=%" PRIu64 ", %zu bytes (total %zu / %zu)", log_t::flag_t::INFO, sid, data.size(), this->_received.size(), this->_sent.size());
			// Пока поток не завершён удалённым сервером - ждём остальные части
			if(!fin)
				// Выходим из метода
				return;
			/**
			 * Поток завершён: выполняем верификацию эхо-ответа - принятые данные должны
			 * побайтово совпадать с отправленной полезной нагрузкой
			 */
			// Печатаем принятый от сервера эхо-ответ, чтобы видеть его содержимое
			cout << endl << "==== ECHO RECEIVED (" << this->_received.size() << " bytes) ====" << endl
			     << this->_received
			     << "==== END OF ECHO ====" << endl;
			// Если принятые данные побайтово совпадают с отправленной нагрузкой
			if(this->_received == this->_sent){
				// Записываем в лог сообщение об успешной верификации эхо-ответа
				this->_log->print("ECHO VERIFIED: %zu bytes returned intact, payload matches byte-for-byte", log_t::flag_t::INFO, this->_received.size());
				// Печатаем итог проверки
				cout << endl << ">>> ECHO VERIFIED: payload matches byte-for-byte <<<" << endl << endl;
			// Если принятые данные не совпали с отправленными
			} else {
				// Записываем ошибку в лог
				this->_log->print("ECHO MISMATCH: sent %zu bytes, received %zu bytes - payload corrupted", log_t::flag_t::CRITICAL, this->_sent.size(), this->_received.size());
				// Печатаем итог проверки
				cout << endl << ">>> ECHO MISMATCH: payload corrupted <<<" << endl << endl;
			}
			// Помечаем обмен потоком завершённым
			this->_streamDone = true;
			// Завершаем работу клиента, если завершён и обмен датаграммой
			this->maybeFinish(client);
		}
		/**
		 * @brief Метод обработки принятой датаграммы приложения QUIC (эхо-ответ сервера, RFC 9221)
		 *
		 * @param data   данные принятой датаграммы
		 * @param client объект клиента
		 *
		 */
		void datagram(const string & data, client_t * client) noexcept {
			// Печатаем принятую от сервера эхо-датаграмму, чтобы видеть её содержимое
			cout << endl << "==== DATAGRAM ECHO RECEIVED (" << data.size() << " bytes) ====" << endl
			     << data << endl
			     << "==== END OF DATAGRAM ECHO ====" << endl;
			// Если принятая датаграмма побайтово совпадает с отправленной
			if(data == this->_datagram){
				// Записываем в лог сообщение об успешной верификации эхо-датаграммы
				this->_log->print("DATAGRAM VERIFIED: %zu bytes returned intact", log_t::flag_t::INFO, data.size());
				// Печатаем итог проверки
				cout << endl << ">>> DATAGRAM VERIFIED: payload matches byte-for-byte <<<" << endl << endl;
			// Если принятая датаграмма не совпала с отправленной
			} else {
				// Записываем ошибку в лог
				this->_log->print("DATAGRAM MISMATCH: sent %zu bytes, received %zu bytes", log_t::flag_t::CRITICAL, this->_datagram.size(), data.size());
				// Печатаем итог проверки
				cout << endl << ">>> DATAGRAM MISMATCH <<<" << endl << endl;
			}
			// Помечаем обмен датаграммой завершённым
			this->_datagramDone = true;
			// Завершаем работу клиента, если завершён и обмен потоком
			this->maybeFinish(client);
		}
		/**
		 * @brief Метод обработки ошибок клиента
		 *
		 * @param error   код ошибки
		 * @param message сообщение об ошибке
		 *
		 */
		void error([[maybe_unused]] const event::error_t error, const string & message) noexcept {
			// Записываем ошибку в лог
			this->_log->print("QUIC client error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 *
		 */
		Executor(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log), _sid(quic::connection_t::INVALID_STREAM), _streamDone(false), _datagramDone(false) {}
};

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main([[maybe_unused]] int32_t argc, [[maybe_unused]] char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект исполнителя для обработки событий клиента
	Executor executor(&fmk, &log);
	// Создаём объект транспортного уровня безопасности
	tls::coder_t tls(&fmk, &log);
	// Регистрируем шаблон контекста безопасности для транспорта QUIC
	const tls::coder_t::id_t cts = tls.context(event::node_t::CLIENT, event::protocol_t::QUIC);
	// Если шаблон контекста безопасности не создан
	if(cts == 0){
		// Записываем в лог сообщение об ошибке
		log.print("QUIC security context is not created", log_t::flag_t::CRITICAL);
		// Выходим из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Устанавливаем список поддерживаемых ALPN-протоколов (RFC 9001 §8.1)
	tls.alpn(cts, {tls::coder_t::alpn_t{0, "h3"}});
	// Снимаем проверку сертификата удалённого сервера для локального эхо-прогона
	tls.validateServerNameIndication(cts, false);
	// Устанавливаем сертификат клиента
	tls.certificate(cts, "../sh/certificates/client/cert.pem");
	// Устанавливаем приватный ключ клиента
	tls.privateKey(cts, "../sh/certificates/client/key.pem");
	// Создаём объект клиента QUIC на шаблоне контекста безопасности
	client_t client(tls.transport(cts), &tls, &fmk, &log);
	// Создаём событие клиента транспорта QUIC поверх UDP
	client.init(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::QUIC);
	// Локальные транспортные параметры соединения QUIC (RFC 9000 §7.4)
	quic::params::params_t params;
	// Устанавливаем таймаут простоя соединения в миллисекундах
	params.maxIdleTimeout = 30000;
	// Устанавливаем лимит данных соединения
	params.initialMaxData = 1048576;
	// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiLocal = 262144;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiRemote = 262144;
	// Устанавливаем лимит данных однонаправленных потоков
	params.initialMaxStreamDataUni = 262144;
	// Устанавливаем лимит числа двунаправленных потоков
	params.initialMaxStreamsBidi = 100;
	// Устанавливаем лимит числа однонаправленных потоков
	params.initialMaxStreamsUni = 100;
	// Устанавливаем предельный размер принимаемой датаграммы приложения (RFC 9221 §3)
	params.maxDatagramFrameSize = 1200;
	// Устанавливаем локальные транспортные параметры соединения QUIC
	client.params(params);
	// Включаем уведомление о перегрузке пути (RFC 9000 §13.4)
	client.ecn(true);
	// Устанавливаем адрес и порт удалённого сервера
	if(client.setTarget("127.0.0.1") && client.setTargetPort(2222)){
		// Регистрируем функцию обратного вызова на событие изменения статуса клиента
		client.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие готовности клиента к работе
		client.on <void (const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3);
		// Регистрируем функцию обратного вызова на событие подключения к серверу
		client.on <void (const bool)> ("connect", &Executor::connect, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на собранные данные потока приложения QUIC
		client.on <void (const uint64_t, const string &, const bool)> ("stream", &Executor::stream, &executor, _1, _2, _3, &client);
		// Регистрируем функцию обратного вызова на принятую датаграмму приложения QUIC (RFC 9221)
		client.on <void (const string &)> ("datagram", &Executor::datagram, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие ошибок клиента
		client.on <void (const event::error_t, const string &)> ("error", &Executor::error, &executor, _1, _2);
		// Запускаем событие клиента
		client.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
