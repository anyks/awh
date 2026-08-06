/**
 * @file: server.cpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример сервера DTLS на низкоуровневом движке ввода-вывода —
 *        демонстрация приёма защищённых датаграммных подключений, выполнения рукопожатия и обслуживания клиентов
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Стандартные модули
 */
#include <iostream>
#include <cinttypes>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>
#include <cryptography/tls/coder.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Создаём объект транспортного уровня безопасности
	tls::coder_t tls(&fmk, &log);
	// Добавляем новое событие сервера
	event::id_t eid = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::DATAGRAM);
	// Устанавливаем порт события
	io.setSourcePort(eid, 2222);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Регистрируем объект транспортного уровня безопасности
		tls::coder_t::id_t cts = tls.context(event::node_t::SERVER, event::protocol_t::UDP);
		// Устанавливаем ALPN протоколы TLS
		tls.alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации DTLS
		tls.ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста DTLS
		tls.validateServerNameIndication(cts, false);
		// Устанавливаем клиентский сертификат DTLS
		tls.certificate(cts, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ DTLS
		tls.privateKey(cts, "../sh/certificates/server/key.pem");
		// Регистрируем функцию обратного вызова на получение ошибок DTLS
		tls.on(cts, [&log](const tls::coder_t::id_t id, [[maybe_unused]] const tls::coder_t::error_t error, const string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке DTLS
			log.print("Ошибка DTLS: ID=%" PRIu64 ", Сообщение=%s", log_t::flag_t::CRITICAL, id, message.c_str());
		});
		// Устананавливаем опции события
		if(io.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Успешно установлены опции события!" << endl;
		// Записываем ошибку в лог установки опций события
		else cout << " Ошибка установки опций события!" << endl;
		// Устанавливаем IP-адрес события
		if(io.setAddress(eid, event::address_t::IPV4, "127.0.0.1")){
			// Устанавливаем функцию обратного вызова на изменение статуса события
			io.on(eid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						log.print("Событие принято: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						log.print("Событие подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						log.print("Событие инициализировано: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						log.print("Событие запущено: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						log.print("Событие на паузе: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						log.print("Событие возобновлено: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						log.print("Событие успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						log.print("Событие выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						log.print("Событие в ожидании: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						log.print("Событие подключено: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						log.print("Событие отменено: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						log.print("Событие переподключено: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						log.print("Событие прослушивается: ID=%u", log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на подключение нового клиента
			io.on(eid, static_cast <engine::callback::accept_t> ([cts, &tls, &io, &log](const event::id_t eid, const event::id_t cid) noexcept -> void {
				// Записываем в лог сообщение о принятии события
				log.print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", log_t::flag_t::INFO, eid, cid, io.getAddress(cid, event::address_t::IPV4).c_str(), io.getSourcePort(cid));
				// Создаём идентификатор транспортного уровня DTLS
				tls::coder_t::id_t ctl = tls.transport(cts);
				// Регистрируем функцию обратного вызова на получение ошибок DTLS
				tls.on(ctl, [&log](const tls::coder_t::id_t id, [[maybe_unused]] const tls::coder_t::error_t error, const string & message) noexcept -> void {
					// Записываем в лог сообщение о предупреждающей ошибке DTLS
					log.print("Ошибка DTLS: ID=%" PRIu64 ", Сообщение=%s", log_t::flag_t::CRITICAL, id, message.c_str());
				});
				// Регистрируем функцию обратного вызова на запись данных DTLS
				tls.on(ctl, [&log](const tls::coder_t::id_t id, const tls::coder_t::event_t event, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (tls::coder_t::event_t::ENCRYPTION):
							// Записываем в лог сообщение о записи зашифрованных данных DTLS
							log.print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", log_t::flag_t::INFO, id, size);
						break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (tls::coder_t::event_t::DECRYPTION):
							// Записываем в лог сообщение о записи дешифрованных данных DTLS
							log.print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", log_t::flag_t::INFO, id, size);
						break;
					}
				});
				// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
				tls.on(ctl, [&tls, &io, &log](const tls::coder_t::id_t id, const tls::coder_t::state_t state) noexcept -> void {
					/**
					 * Обрабатываем входящие состояния DTLS
					 */
					switch(static_cast <uint8_t> (state)){
						// Если состояние ошибки транспортного уровня
						case static_cast <uint8_t> (tls::coder_t::state_t::FAILED):
							// Записываем ошибку в лог транспортного уровня TLS
							log.print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", log_t::flag_t::CRITICAL, id);
						break;
						// Если состояние уничтожения объекта транспортного уровня
						case static_cast <uint8_t> (tls::coder_t::state_t::DESTROYED):
							// Записываем в лог сообщение об успешном удалении контекста TLS
							log.print("Контекст TLS успешно удалён: ID=%" PRIu64 "", log_t::flag_t::INFO, id);
						break;
						// Если состояние рукопожатия успешно завершено
						case static_cast <uint8_t> (tls::coder_t::state_t::HANDSHAKED): {
							// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << tls.info(id) << endl;
							cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << (u_short) tls.alpn(id) << endl;
							cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << tls.serverNameIndication(id) << endl << endl;
							cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
							cout << "Версия OpenSSL: " << tls.version() << endl << endl;
							cout << "Cipher: " << tls.cipherInfo(id) << endl << endl;
							cout << "Certificate: " << tls.certificateInfo(id) << endl << endl;
							cout << "CRL Info: " << tls.certificateRevocationListInfo(id) << endl << endl;
							cout << "Certificate Validation: " << (tls.validateCertificate(id) ? "Valid" : "Invalid") << endl << endl;
							// Возвращаем данные сертификата DTLS
							cout << "Certificate data:\n" << tls.certificateExtract(id) << endl << endl;
							// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							log.print("Рукопожатие DTLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", log_t::flag_t::INFO, id, tls.alpn(id));
							// Записываем в лог информацию о DTLS соединении
							cout << tls.peerInfo(id) << endl;
							/*
							// Выполняем повторную передачу данных TLS
							if(tls.retransmit(id))
								// Записываем в лог сообщение об успешной повторной передаче данных TLS
								log.print("Успешно выполнена повторная передача данных TLS: ID=%" PRIu64 "", log_t::flag_t::INFO, id);
							// Записываем ошибку в лог повторной передачи данных TLS
							else log.print("Ошибка повторной передачи данных TLS: ID=%" PRIu64 "", log_t::flag_t::CRITICAL, id);
							*/
						} break;
					}
				});
				// Устанавливаем клиента DTLS для события
				tls.peer(ctl, io.getAddress(cid, event::address_t::IPV4), io.getSourcePort(cid));
				// Регистрируем функцию обратного вызова на чтение данных DTLS
				tls.on(ctl, [cid, &tls, &io, &log](const tls::coder_t::id_t id, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (tls::coder_t::event_t::ENCRYPTION): {
							// Отправляем данные обратно клиенту
							if(io.send(cid, reinterpret_cast <const char *> (buffer), size))
								// Если данные успешно отправлены
								log.print("Отправлено зашифрованных данных: ID=%u, %zu байт", log_t::flag_t::INFO, cid, size);
							// Если данные не отправлены
							else log.print("Ошибка отправки зашифрованных данных: ID=%u", log_t::flag_t::CRITICAL, cid);
						} break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (tls::coder_t::event_t::DECRYPTION): {
							// Получаем ответ сервера в расшифрованном виде
							const string response(reinterpret_cast <const char *> (buffer), size);
							// Записываем в лог сообщение полученных данных с сервера
							log.print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", log_t::flag_t::INFO, id, size, response.c_str());
							// Если данные успешно зашифрованы DTLS
							if(tls.encrypt(id, response.c_str(), response.size()))
								// Записываем в лог сообщение об успешном шифровании данных DTLS
								log.print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", log_t::flag_t::INFO, id, response.size());
							// Если данные не отправлены
							else log.print("Ошибка шифрования: ID=%" PRIu64 "", log_t::flag_t::CRITICAL, id);
						} break;
					}
				});
				// Устанавливаем функцию обратного вызова на изменение статуса события
				io.on(cid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Записываем в лог сообщение о принятии события
							log.print("Событие принято: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Записываем в лог сообщение об уничтожении события
							log.print("Событие подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Записываем в лог сообщение об инициализации события
							log.print("Событие инициализировано: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Записываем в лог сообщение о запуске события
							log.print("Событие запущено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Записываем в лог сообщение о паузе события
							log.print("Событие на паузе: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Записываем в лог сообщение о возобновлении события
							log.print("Событие возобновлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Записываем в лог сообщение о успешном выполнении события
							log.print("Событие успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Записываем в лог сообщение о неудачном выполнении события
							log.print("Событие выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Записываем в лог сообщение о выполнении события в ожидании
							log.print("Событие в ожидании: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Записываем в лог сообщение о подключении события
							log.print("Событие подключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Записываем в лог сообщение об отмене события
							log.print("Событие отменено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие переподключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Записываем в лог сообщение о прослушивании события
							log.print("Событие прослушивается: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на запись в событие
				io.on(cid, static_cast <engine::callback::write_t> ([&log](const event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о записи данных
					log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(cid, [ctl, &tls, &io, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Если данные успешно дешифрованы DTLS
					if(tls.decrypt(ctl, data, size)){
						// Записываем в лог сообщение об успешном дешифровании данных DTLS
						log.print("Успешно дешифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", log_t::flag_t::INFO, ctl, size);
						// Если рукопожатие DTLS успешно
						if(tls.handshake(ctl))
							// Записываем в лог сообщение о начале рукопожатия DTLS
							log.print("Начинаем процесс рукопожатия: ID=%" PRIu64 "", log_t::flag_t::INFO, ctl);
						// Если рукопожатие DTLS не выполнено
						else log.print("Ошибка рукопожатия DTLS: ID=%" PRIu64 "", log_t::flag_t::CRITICAL, ctl);
					// Если данные не отправлены
					} else log.print("Ошибка дешифрования: ID=%u", log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на ошибку события
				io.on(cid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Записываем ошибку в лог неизвестного события
							log.print("Неизвестная ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Записываем ошибку в лог недопустимой операции
							log.print("Недопустимая операция события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Записываем ошибку в лог доступа запрещёния
							log.print("Доступ к событию запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Записываем ошибку в лог уже существующего объекта
							log.print("Объект события уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Записываем ошибку в лог доступа к сокету
							log.print("Ошибка доступа к сокету события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Записываем ошибку в лог некорректного адреса
							log.print("Некорректный адрес события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Записываем ошибку в лог подключения
							log.print("Ошибка подключения события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Записываем ошибку в лог недостаточно ресурсов
							log.print("Недостаточно ресурсов для события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Записываем ошибку в лог события
							log.print("Ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Записываем ошибку в лог события
							log.print("Объект события не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на общее событие
				io.on(cid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							log.print("Событие на чтение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							log.print("Событие на запись: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							log.print("Событие на подключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							log.print("Событие на отключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие на переподключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							log.print("Событие на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							log.print("Событие на изменение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							log.print("Событие на удаление: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							log.print("Событие на переименование: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							log.print("Событие на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							log.print("Событие на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							log.print("Событие на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			io.on(eid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						log.print("Событие на чтение: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						log.print("Событие на запись: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						log.print("Событие на подключение: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						log.print("Событие на отключение: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						log.print("Событие на переподключение: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						log.print("Событие на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						log.print("Событие на изменение: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						log.print("Событие на удаление: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						log.print("Событие на переименование: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						log.print("Событие на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						log.print("Событие на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						log.print("Событие на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			// io.setTimeout(eid, event::action_t::READ, 5000);
			// Устанавливаем таймаут события на запись
			io.setTimeout(eid, event::action_t::WRITE, 5000);
			// Выполняем фиксацию настроек события сервера
			if(io.commit(eid)){
				// Выполняем запуск события
				if(io.launch(eid)){
					// Записываем в лог сообщение об успешном запуске события
					cout << " Событие успешно запущено!" << endl;
					/**
					 * Запускаем опрос событий
					 */
					while(io.poll());
				// Записываем ошибку в лог запуска события
				} else cout << " Ошибка запуска события!" << endl;
			}
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса события!" << endl;
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
