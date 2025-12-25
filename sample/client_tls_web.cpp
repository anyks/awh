/**
 * @file: client.cpp
 * @date: 2025-10-25
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

#include <iostream>
#include <cinttypes>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>
#include <net/tls.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Устанавливаем логгер
	fmk.setLogger(&log);
	// Устанавливаем уровень логирования
	// log.level(log_t::level_t::NONE);
	// Создаём объект асинхронного движка ввода-вывода
	io_t io(&fmk, &log);
	// Создаём объект транспортного уровня безопасности
	tls_t tls(&fmk, &log);
	// Добавляем новое событие клиента TCP
	event::id_t eid = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	// Устанавливаем порт события
	io.port(eid, 443);
	// io.port(eid, 8006);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Флаг завершения работы
		bool stop = false;
		// Регистрируем объект транспортного уровня безопасности
		tls_t::id_t tid = tls.create(event::node_t::CLIENT, event::protocol_t::TCP);
		// Устанавливаем ALPN протоколы TLS
		tls.alpn(tid, vector <tls_t::alpn_t> {{5,"http/1.1"}});
		// tls.alpn(tid, vector <tls_t::alpn_t> {{0,"http/1.1"},{1,"h2"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		tls.ca(tid, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		// tls.validateHostname(tid, false);
		// Устанавливаем имя хоста TLS
		// tls.hostname(tid, "contms.ru");
		tls.hostname(tid, "www.google.com");
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		tls.on(tid, [&tls, &log](const tls_t::id_t id) noexcept -> void {
			// Выводим сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
			cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << tls.info(id) << endl;
			cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << (u_short) tls.alpn(id) << endl;
			cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
			cout << "Версия OpenSSL: " << tls.version() << endl << endl;
			cout << "Cipher: " << tls.cipherInfo(id) << endl << endl;
			cout << "Certificate: " << tls.certificateInfo(id) << endl << endl;
			cout << "CRL Info: " << tls.certificateRevocationListInfo(id) << endl << endl;
			cout << "Certificate Validation: " << (tls.validateCertificate(id) ? "Valid" : "Invalid") << endl << endl;

			// Текст запроса к серверу
			const string request =
				"GET / HTTP/1.1\r\n"
				"Host: www.google.com\r\n"
				"Connection: close\r\n"
				"User-Agent: iouring-openssl-sample/1.0\r\n"
				"\r\n";
			// Если данные успешно зашифрованы TLS
			if(tls.encrypt(id, request.c_str(), request.size()))
				// Выводим сообщение об успешном шифровании данных TLS
				log.print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", log_t::flag_t::INFO, id, request.size());
			// Если данные не отправлены
			else log.print("Ошибка шифрования: ID=%" PRIu64 "", log_t::flag_t::CRITICAL, id);
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		tls.on(tid, [&log](const tls_t::id_t id, const tls_t::error_t error, const string & message) noexcept -> void {
			/**
			 * Обрабатываем входящие ошибки TLS
			 */
			switch(static_cast <uint8_t> (error)){
				// Если получено предупреждение TLS
				case static_cast <uint8_t> (tls_t::error_t::WARNING):
					// Выводим сообщение о предупреждающей ошибке TLS
					log.print("Предупреждение TLS: ID=%" PRIu64 ", Сообщение=%s", log_t::flag_t::WARNING, id, message.c_str());
				break;
				// Если получена критическая ошибка TLS
				case static_cast <uint8_t> (tls_t::error_t::CRITICAL):
					// Выводим сообщение о предупреждающей ошибке TLS
					log.print("Ошибка TLS: ID=%" PRIu64 ", Сообщение=%s", log_t::flag_t::CRITICAL, id, message.c_str());
				break;
			}
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		tls.on(tid, [&log](const tls_t::id_t id, const tls_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (tls_t::event_t::ENCRYPTION):
					// Выводим сообщение о записи зашифрованных данных TLS
					log.print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (tls_t::event_t::DECRYPTION):
					// Выводим сообщение о записи дешифрованных данных TLS
					log.print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		tls.on(tid, [eid, &stop, &io, &log](const tls_t::id_t id, const tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (tls_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(io.send(eid, reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						log.print("Отправлено зашифрованных данных: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else log.print("Ошибка отправки зашифрованных данных: ID=%u", log_t::flag_t::CRITICAL, eid);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (tls_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const string response(reinterpret_cast <const char *> (buffer), size);
					// Выводим сообщение полученных данных с сервера
					log.print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", log_t::flag_t::INFO, id, size, response.c_str());
					// Если флаг остановки ещё не указан
					if(!stop)
						// Устанавливаем флаг завершения работы
						stop = (response.rfind("</html>") != string::npos);
				} break;
			}
		});
		// Устананавливаем опции события
		if(io.options(eid, event::options::NOSIGILL | event::options::NOSIGPIPE | event::options::REUSEADDR | event::options::NOIOBLOCK | event::options::CLOSEONEXEC | event::options::TCPNODELAY | event::options::KEEPALIVE))
			// Выводим сообщение об успешной установке опций события
			cout << " Успешно установлены опции события!" << endl;
		// Выводим сообщение об ошибке установки опций события
		else cout << " Ошибка установки опций события!" << endl;
		// Устанавливаем IP-адрес события
		if(io.address(eid, event::address_t::IPV4, "0.0.0.0")){
			// Устанавливаем адрес сервера назначения
			// if(io.target(eid, "198.18.0.23")){ // contms.ru VPN
			if(io.target(eid, "198.18.0.35")){ // VPN
			// if(io.target(eid, "74.125.205.102")){
				// Устанавливаем функцию обратного вызова на событие таймера
				io.on(eid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Выводим сообщение о принятии события
							log.print("Событие принято: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Выводим сообщение об уничтожении события
							log.print("Событие подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Выводим сообщение об инициализации события
							log.print("Событие инициализировано: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::RUNNING):
							// Выводим сообщение о запуске события
							log.print("Событие запущено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус остановки события
						case static_cast <uint8_t> (event::status_t::STOPPED):
							// Выводим сообщение о остановке события
							log.print("Событие остановлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Выводим сообщение о паузе события
							log.print("Событие на паузе: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Выводим сообщение о возобновлении события
							log.print("Событие возобновлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Выводим сообщение о успешном выполнении события
							log.print("Событие успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Выводим сообщение о неудачном выполнении события
							log.print("Событие выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Выводим сообщение о выполнении события в ожидании
							log.print("Событие в ожидании: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Выводим сообщение о подключении события
							log.print("Событие подключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Выводим сообщение об отмене события
							log.print("Событие отменено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Выводим сообщение о переподключении события
							log.print("Событие переподключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на запись в событие
				io.on(eid, static_cast <event::callback::write_t> ([&log](const event::id_t eid, const size_t size) noexcept -> void {
					// Выводим сообщение о переподключении события
					log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(eid, [tid, &tls, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Если данные успешно дешифрованы TLS
					if(tls.decrypt(tid, data, size))
						// Выводим сообщение об успешном дешифровании данных TLS
						log.print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", log_t::flag_t::INFO, tid, size);
					// Если данные не отправлены
					else log.print("Ошибка дешифрования: ID=%u", log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на ошибку события
				io.on(eid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Выводим сообщение об ошибке неизвестного события
							log.print("Неизвестная ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Выводим сообщение об ошибке недопустимой операции
							log.print("Недопустимая операция события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Выводим сообщение об ошибке доступа запрещёния
							log.print("Доступ к событию запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Выводим сообщение об ошибке уже существующего объекта
							log.print("Объект события уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Выводим сообщение об ошибке доступа к сокету
							log.print("Ошибка доступа к сокету события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Выводим сообщение об ошибке некорректного адреса
							log.print("Некорректный адрес события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Выводим сообщение об ошибке подключения
							log.print("Ошибка подключения события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Выводим сообщение об ошибке недостаточно ресурсов
							log.print("Недостаточно ресурсов для события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Выводим сообщение об ошибке события
							log.print("Ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Выводим сообщение об ошибке события
							log.print("Объект события не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на удачное подключение к серверу
				io.on(eid, static_cast <event::callback::connect_t> ([tid, &tls, &io, &log](const event::id_t eid, const bool ok) noexcept -> void {
					// Выводим сообщение о принятии события
					log.print("Событие подключения: ID=%u, результат: %s", log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
					// Если подключение успешно
					if(ok){
						// Если рукопожатие TLS успешно
						if(tls.handshake(tid))
							// Выводим сообщение о начале рукопожатия TLS
							log.print("Начинаем процесс рукопожатия: ID=%u", log_t::flag_t::INFO, tid);
						// Если рукопожатие TLS не выполнено
						else log.print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", log_t::flag_t::CRITICAL, tid);
					}
				}));
				// Устанавливаем функцию обратного вызова на общее событие
				io.on(eid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Выводим сообщение о чтении события
							log.print("Событие на чтение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Выводим сообщение о записи события
							log.print("Событие на запись: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Выводим сообщение о подключении события
							log.print("Событие на подключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Выводим сообщение об отключении события
							log.print("Событие на отключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Выводим сообщение о переподключении события
							log.print("Событие на переподключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Выводим сообщение о закрытии события
							log.print("Событие на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение об изменении события
							log.print("Событие на изменение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение об удалении события
							log.print("Событие на удаление: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Выводим сообщение о переименовании события
							log.print("Событие на переименование: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Выводим сообщение об изменении атрибутов события
							log.print("Событие на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Выводим сообщение об отзыве доступа события
							log.print("Событие на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Выводим сообщение о изменении счётчика жёстких ссылок события
							log.print("Событие на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем таймаут события на чтение
				io.timeout(eid, event::action_t::READ, 3000);
				// Устанавливаем таймаут события на запись
				io.timeout(eid, event::action_t::WRITE, 3000);
				// Устанавливаем таймаут события на подключение
				io.timeout(eid, event::action_t::CONNECT, 5000);
				// Выполняем фиксацию настроек события сервера
				if(io.commit(eid)){
					// Если подключение к серверу прошло успешно
					if(io.connect(eid, true)){
						/**
						 * Запускаем опрос событий
						 */
						while(!stop && io.poll());
					}
				}
			// Если адрес назначения не установлен
			} else cout << " Ошибка установки адреса сервера!" << endl;
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса клиента!" << endl;
	}
	// Выводим результат
	return 0;
}
