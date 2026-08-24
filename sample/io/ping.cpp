/**
 * @file client.cpp
 * @date 2025-10-25
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
 * @brief Пример проверки доступности узла через сырой сокет — демонстрация ручной сборки ICMP-эхо-запроса,
 *        отправки его средствами движка ввода-вывода и разбора полученного ответа
 *
 * @copyright Copyright © 2025
 *
 */

#include <ctime>
#include <random>
#include <iostream>
#include <cinttypes>
#include <algorithm>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>
#include <net/addr.hpp>
#include <sys/chrono.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Функция подсчёта контрольной суммы
 *
 * @param buffer буфер данных для подсчёта
 * @param size   размер данных для подсчёта
 * @return       подсчитанная контрольная сумма
 *
 */
uint16_t checksum(const void * buffer, const size_t size) noexcept {
	// Переменная результата
	uint16_t result = 0;
	// Если данные переданы верные
	if((buffer != nullptr) && (size > 0)){
		// Контрольная сумма расчёта
		uint32_t sum = 0;
		// Устанавливаем длину контрольной суммы
		size_t length = size;
		// Выполняем приведение буфера в нужную нам форму
		auto data = reinterpret_cast <const uint16_t *> (buffer);
		// Если длина буфера всего один байт
		if(length & 1)
			// Выполняем расчёт контрольной суммы
			sum = reinterpret_cast <const uint8_t *> (data)[length - 1];
		// Делим длину байт пополам
		length /= 2;
		/**
		 *  Выполняем перебор буфера байт
		 */
		while(length--){
			// Выполняем расчёт контрольной суммы
			sum += * data++;
			// Если контрольная сумма достигла предела
			if(sum & 0xffff0000)
				// Выполняем смещение на оставшиеся 16 байт
				sum = ((sum >> 16) + (sum & 0xffff));
		}
		// Выполняем получение результата контрольной суммы
		result = static_cast <uint16_t> (~sum);
	}
	// Возвращаем результат
	return result;
}

/**
 * @brief Главная функция приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Устанавливаем логгер
	fmk.setLogger(&log);
	// Устанавливаем уровень логирования
	// log.level(log_t::level_t::NONE);

	/**
	 * @brief Структура заголовков ICMP
	 *
	 */
	struct IcmpHeader {
		uint8_t type;      // Тип запроса
		uint8_t code;      // Код запроса
		uint16_t checksum; // Контрольная сумма
		/**
		 * Объединение структур запроса
		 */
		union {
			/**
			 * @brief Структура отправляемого запроса
			 *
			 */
			struct {
				uint16_t identifier = 0; // Идентификатор запроса
				uint16_t sequence   = 0; // Номер последовательности
				uint64_t payload    = 0; // Тело полезной нагрузки
			} echo;
			/**
			 * @brief Структура указателя запроса
			 *
			 */
			struct ICMP_PACKET_POINTER_HEADER {
				// Указатель пакета
				uint8_t pointer = 0;
			} pointer;
			/**
			 * @brief Структура адреса ответа
			 *
			 */
			struct ICMP_PACKET_REDIRECT_HEADER {
				// Адрес ответа IPv4
				uint32_t gatewayAddress = 0;
			} redirect;
			/**
			 * @brief Структура адреса ответа
			 *
			 */
			struct ICMP6_PACKET_REDIRECT_HEADER {
				// Адрес ответа IPv6
				uint32_t gatewayAddress[4] = {0,0,0,0};
			} redirect6;
		} meta;
	} __attribute__((packed));

	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Создаём объект работы с датами
	chrono_t chrono(&fmk, &log);
	// Создаём объект работы с IP-адресами
	net_addr_t addr(&fmk, &log);
	// Идентификатор события
	event::id_t eid = 0;
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Добавляем новое событие клиента ICMP
		eid = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::RAW, event::protocol_t::ICMP);
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Если пользователь является привилигированным
		if(::getuid())
			// Добавляем новое событие клиента ICMP
			eid = io.event(awh::event::node_t::CLIENT, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::ICMP);
		// Добавляем новое событие клиента ICMP
		else eid = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::RAW, event::protocol_t::ICMP);
	#endif
	// Устанавливаем порт события
	io.setTargetPort(eid, 2222);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события
		if(io.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::CLOSE_ON_EXEC))
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Успешно установлены опции события!" << endl;
		// Записываем ошибку в лог установки опций события
		else cout << " Ошибка установки опций события!" << endl;
		// Устанавливаем IP-адрес события
		if(io.setAddress(eid, event::address_t::IPV4, "0.0.0.0")){
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid, "8.8.8.8")){
				// Устанавливаем функцию обратного вызова на запись в событие
				io.on(eid, static_cast <engine::callback::write_t> ([&log](const event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о записи данных
					log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(eid, [&addr, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Если данные пришли с IP-заголовком
					if((size >= 20) && ((data[0] & 0xF0) == 0x40)){
						// Если размер данных меньше размера заголовка IP
						if(size < sizeof(struct ip))
							// Выходим из функции
							return;
						// Приводим данные к структуре IP-заголовка
						const struct ip * iph = reinterpret_cast <const struct ip *> (data);
						// Длина IP-заголовка
						size_t iphl = (iph->ip_hl * 4);
						// Если заголовок пришёл битый
						if((iphl < 20) || (size < (iphl + 8)))
							// минимум ICMP-заголовок
							return;
						// Длина IP-заголовка: iph->ip_hl * 4
						size_t ip_hdr_len = (iph->ip_hl * 4);
						// Минимум 8 байт ICMP
						if(size >= (ip_hdr_len + 8)){
							// Приводим данные к структуре ICMP-заголовка
							const struct IcmpHeader * icmp = reinterpret_cast <const struct IcmpHeader *> (data + ip_hdr_len);
							// Возвращаем полученные данные
							printf("ICMP type: %u\n", icmp->type); // будет 0 (Echo Reply)
							// Возвращаем идентификатор и индекс последовательности ответа
							printf("ID: %u, Seq: %u\n", ntohs(icmp->meta.echo.identifier), ntohs(icmp->meta.echo.sequence));
							// Добавляем полученный IP-адрес
							addr.v4(icmp->meta.redirect.gatewayAddress);
							// Записываем в лог сообщение о чтении данных
							log.print("Прочитано: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, size, static_cast <string> (addr).c_str());
						}
					// Если данные пришли с читсым ICMP
					} else {
						// Результат полученных данных
						auto icmpResponseHeader = reinterpret_cast <const struct IcmpHeader *> (data);
						// Добавляем полученный IP-адрес
						addr.v4(icmpResponseHeader->meta.redirect.gatewayAddress);
						// Записываем в лог сообщение о чтении данных
						log.print("Прочитано: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, size, static_cast <string> (addr).c_str());
					}
				});
				// Устанавливаем функцию обратного вызова на ошибку события
				io.on(eid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
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
				// Устанавливаем функцию обратного вызова на удачное подключение к серверу
				io.on(eid, static_cast <engine::callback::connect_t> ([&io, &log](const event::id_t eid, const bool ok) noexcept -> void {
					// Записываем в лог сообщение о принятии события
					log.print("Событие подключения: ID=%u, результат: %s", log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
					// Если подключение успешно
					if(ok){
						// Текст исходящего сообщения
						const string message("Hello from async client!");
						// Отправляем данные обратно клиенту
						if(io.send(eid, message.c_str(), message.size()))
							// Если данные успешно отправлены
							log.print("Отправлено: ID=%u, %zu байт", log_t::flag_t::INFO, eid, message.size());
						// Если данные не отправлены
						else log.print("Ошибка отправки: ID=%u", log_t::flag_t::CRITICAL, eid);
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
				// Устанавливаем таймаут события на запись
				io.setTimeout(eid, event::action_t::WRITE, 3000);
				// Устанавливаем таймаут события на чтение
				io.setTimeout(eid, event::action_t::READ, 10000);
				// Выполняем фиксацию настроек события сервера
				if(io.commit(eid) && io.launch(eid)){
					// Выполняем инициализацию генератора
					std::random_device randev;
					// Подключаем устройство генератора
					mt19937 generator(randev());
					// Выполняем генерирование случайного числа
					uniform_int_distribution <mt19937::result_type> dist6(0, numeric_limits <uint32_t>::max() - 1);
					/**
					 * @brief Структура layout совпадает с RFC
					 *
					 */
					struct IcmpEchoPacket {
						uint8_t type;
						uint8_t code;
						uint16_t checksum;
						uint16_t identifier;
						uint16_t sequence;
						uint64_t payload;
					} __attribute__((packed)); // ← отключает padding
					// Создаём объект заголовков
					IcmpEchoPacket icmp{};
					// Создаём объект заголовков
					// struct IcmpHeader icmp{};
					// Выполняем установку типа запроса
					icmp.type = 8; // IPv4
					// icmp.type = 128; // IPv6
					// Устанавливаем код запроса
					icmp.code = 0;
					// Последовательность
					uint16_t sequence = 0;
					/**
					 * Выполняем пинг 10 раз
					 */
					for(uint8_t i = 0; i < 10; i++){
						// Устанавливаем номер последовательности
						icmp.sequence = htons(sequence);
						// Устанавливаем идентификатор запроса
						icmp.identifier = htons(::getpid() & 0xFFFF);
						// Устанавливаем данные полезной нагрузки
						icmp.payload = static_cast <uint64_t> (dist6(generator));
						// Обнуляем структуру (ОЧЕНЬ ВАЖНО ТАК-КАК РАСЧЁТ КОНТРОЛЬНОЙ СУММЫ НАЧИНАЕТСЯ С НУЛЯ!!!)
						icmp.checksum = 0;
						// Выполняем подсчёт контрольной суммы
						icmp.checksum = ::checksum(&icmp, sizeof(icmp));
						// Запоминаем текущее значение времени в миллисекундах
						const uint64_t mseconds = chrono.timestamp(chrono_t::type_t::MILLISECONDS);
						// Отправляем сообщение серверу
						if(io.send(eid, reinterpret_cast <char *> (&icmp), sizeof(icmp))){
							// Выполняем чтение ответа
							if(io.recv(eid)){
								// Выполняем подсчёт количество прошедшего времени
								cout << "Response: " << (chrono.timestamp(chrono_t::type_t::MILLISECONDS) - mseconds) << " msec." << endl;
								// Увеличиваем последовательность запроса
								sequence++;
							}
						}
					}
				}
			// Если адрес назначения не установлен
			} else cout << " Ошибка установки адреса сервера!" << endl;
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса клиента!" << endl;
	}
	// Возвращаем результат
	return 0;
}
