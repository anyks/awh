/**
 * @file: quic-proxy.cpp
 * @date: 2026-07-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем стандартные модули
 */
#include <cinttypes>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <client/client.hpp>
#include <units/timer.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Класс объекта исполнителя QUIC-клиента для проверки splice-прокси
 *
 * @details В отличие от эхо-клиента (sample/client/quic.cpp), этот клиент рассчитан
 *          на прокси со splice: обратный путь прокси доставляет ответ бэкенда не тем
 *          же потоком и не датаграммой, а туннельным потоком соединения (см. inject).
 *          Поэтому проверка накапливает ВСЕ принятые от сервера байты потоков (с любого
 *          потока, без опоры на флаг завершения) и сверяет их с отправленной нагрузкой
 */
class Executor {
	private:
		// Объект фреймворка
		const fmk_t * _fmk;
		// Объект работы с логами
		const log_t * _log;
	private:
		// Объект таймера для отложенной отправки нагрузки
		unit::timer_t * _timer;
	private:
		// Отправленная прокси полезная нагрузка (эталон для верификации эхо-ответа)
		string _sent;
		// Накопленный от прокси эхо-ответ (может приходить частями и разными потоками)
		string _received;
	private:
		// Флаг уже выполненной верификации (защита от повторного завершения)
		bool _done;
	private:
		/**
		 * @brief Метод формирования осмысленной полезной нагрузки для проверки эхо
		 *
		 * @return сформированная полезная нагрузка
		 */
		string makePayload() const noexcept {
			// Формируем распознаваемый текст как полезную нагрузку
			return string(
				"GET / HTTP/1.1\r\n"
				"Host: anyks.com\r\n"
				"Connection: close\r\n"
				"User-Agent: awh-quic-proxy-sample/1.0\r\n"
				"\r\n"
			);
		}
	public:
		/**
		 * @brief Метод обработки событий изменения статуса клиента
		 *
		 * @param status новый статус клиента
		 * @param client объект клиента
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
					this->_log->print("QUIC proxy client destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки готовности клиента к работе
		 *
		 * @param family семейство адресов клиента
		 * @param domain доменное имя удалённого сервера
		 * @param ip     IP-адрес удалённого сервера
		 */
		void ready([[maybe_unused]] const event::family_t family, const string & domain, const string & ip) noexcept {
			// Записываем в лог сообщение о готовности клиента к работе
			this->_log->print("QUIC proxy client is ready: %s (%s)", log_t::flag_t::INFO, domain.c_str(), ip.c_str());
		}
		/**
		 * @brief Метод обработки события подключения к серверу
		 *
		 * @param ok     результат подключения к серверу
		 * @param client объект клиента
		 */
		void connect(const bool ok, client_t * client) noexcept {
			// Если подключение к серверу не выполнено
			if(!ok){
				// Записываем ошибку в лог
				this->_log->print("QUIC proxy connection failed", log_t::flag_t::CRITICAL);
				// Выходим из метода
				return;
			}
			// Записываем в лог сообщение об установленном соединении
			this->_log->print("QUIC proxy connection established", log_t::flag_t::INFO);
			/**
			 * Откладываем отправку нагрузки коротким таймаутом: у прокси исходящее
			 * соединение к бэкенду устанавливается асинхронно после приёма нашей сессии,
			 * поэтому даём ему завершиться до отправки данных (в реальном прокси бэкенд
			 * готов к моменту прихода полезной нагрузки, напр. после рукопожатия SOCKS5)
			 */
			const event::id_t tid = this->_timer->timeout(300);
			// Регистрируем отправку нагрузки на срабатывание таймера
			this->_timer->on <void (const event::id_t)> (tid, [this, client](const event::id_t) noexcept -> void {
				// Выполняем отправку полезной нагрузки прокси
				this->sendPayload(client);
			}, _1);
		}
		/**
		 * @brief Метод отправки полезной нагрузки прокси
		 *
		 * @param client объект клиента
		 */
		void sendPayload(client_t * client) noexcept {
			// Формируем осмысленную полезную нагрузку и сохраняем её как эталон для верификации
			this->_sent = this->makePayload();
			// Очищаем накопитель эхо-ответа перед началом обмена
			this->_received.clear();
			// Открываем двунаправленный поток приложения для обмена нагрузкой (RFC 9000 §2.1)
			const uint64_t sid = client->open(false);
			// Если поток приложения открыть не удалось
			if(sid == quic::connection_t::INVALID_STREAM){
				// Записываем ошибку в лог
				this->_log->print("Failed to open QUIC stream", log_t::flag_t::CRITICAL);
				// Выходим из метода
				return;
			}
			/**
			 * Отправляем всю нагрузку одним потоком с флагом завершения: прокси перенаправит
			 * её бэкенду, а вернувшееся эхо доставит туннельным потоком соединения
			 */
			if(client->send(sid, this->_sent.data(), this->_sent.size(), true) > 0){
				// Записываем в лог сообщение об отправке нагрузки
				this->_log->print("Sent payload: Stream=%" PRIu64 ", %zu bytes", log_t::flag_t::INFO, sid, this->_sent.size());
				// Печатаем отправленную полезную нагрузку, чтобы видеть её содержимое
				cout << endl << "==== SENT REQUEST (" << this->_sent.size() << " bytes) ====" << endl
				     << this->_sent
				     << "==== END OF REQUEST ====" << endl;
			// Если отправка нагрузки не выполнена
			} else this->_log->print("Failed to send payload to proxy", log_t::flag_t::WARNING);
		}
		/**
		 * @brief Метод обработки собранных данных потока приложения QUIC (эхо-ответ через прокси)
		 *
		 * @param sid    идентификатор потока приложения
		 * @param data   собранные данные потока
		 * @param fin    флаг завершения потока
		 * @param client объект клиента
		 */
		void stream(const uint64_t sid, const string & data, [[maybe_unused]] const bool fin, client_t * client) noexcept {
			// Если верификация уже выполнена - игнорируем остаточные данные
			if(this->_done)
				// Выходим из метода
				return;
			// Накапливаем принятый от прокси эхо-ответ (обратный путь идёт туннельным потоком)
			this->_received.append(data);
			// Записываем в лог сведения о принятой части эхо-ответа
			this->_log->print("Echo chunk: Stream=%" PRIu64 ", %zu bytes (total %zu / %zu)", log_t::flag_t::INFO, sid, data.size(), this->_received.size(), this->_sent.size());
			// Пока накоплено меньше отправленного - ждём остальные части
			if(this->_received.size() < this->_sent.size())
				// Выходим из метода
				return;
			// Помечаем верификацию выполненной, чтобы не завершиться повторно
			this->_done = true;
			// Печатаем принятый через прокси эхо-ответ, чтобы видеть его содержимое
			cout << endl << "==== PROXY ECHO RECEIVED (" << this->_received.size() << " bytes) ====" << endl
			     << this->_received
			     << "==== END OF ECHO ====" << endl;
			// Если принятые данные побайтово совпадают с отправленной нагрузкой
			if(this->_received == this->_sent){
				// Записываем в лог сообщение об успешной верификации эхо-ответа через прокси
				this->_log->print("PROXY ECHO VERIFIED: %zu bytes relayed through QUIC splice intact", log_t::flag_t::INFO, this->_received.size());
				// Печатаем итог проверки
				cout << endl << ">>> PROXY ECHO VERIFIED: payload matches byte-for-byte <<<" << endl << endl;
			// Если принятые данные не совпали с отправленными
			} else {
				// Записываем ошибку в лог
				this->_log->print("PROXY ECHO MISMATCH: sent %zu bytes, received %zu bytes", log_t::flag_t::CRITICAL, this->_sent.size(), this->_received.size());
				// Печатаем итог проверки
				cout << endl << ">>> PROXY ECHO MISMATCH: payload corrupted <<<" << endl << endl;
			}
			// Завершаем работу клиента
			client->stop();
		}
		/**
		 * @brief Метод обработки ошибок клиента
		 *
		 * @param error   код ошибки
		 * @param message сообщение об ошибке
		 */
		void error([[maybe_unused]] const event::error_t error, const string & message) noexcept {
			// Записываем ошибку в лог
			this->_log->print("QUIC proxy client error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 * @param timer объект таймера для отложенной отправки нагрузки
		 * @param fmk   объект фреймворка
		 * @param log   объект логирования
		 */
		Executor(unit::timer_t * timer, const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log), _timer(timer), _done(false) {}
};

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main([[maybe_unused]] int32_t argc, [[maybe_unused]] char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект таймера для отложенной отправки нагрузки
	unit::timer_t timer(&fmk, &log);
	// Создаём объект исполнителя для обработки событий клиента
	Executor executor(&timer, &fmk, &log);
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
	// Снимаем проверку сертификата удалённого сервера для локального прогона
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
	// Устанавливаем адрес и порт QUIC-прокси
	if(client.setTarget("127.0.0.1") && client.setTargetPort(2222)){
		// Регистрируем функцию обратного вызова на событие изменения статуса клиента
		client.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие готовности клиента к работе
		client.on <void (const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3);
		// Регистрируем функцию обратного вызова на событие подключения к серверу
		client.on <void (const bool)> ("connect", &Executor::connect, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на собранные данные потока приложения QUIC
		client.on <void (const uint64_t, const string &, const bool)> ("stream", &Executor::stream, &executor, _1, _2, _3, &client);
		// Регистрируем функцию обратного вызова на событие ошибок клиента
		client.on <void (const event::error_t, const string &)> ("error", &Executor::error, &executor, _1, _2);
		// Запускаем событие клиента
		client.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
