/**
 * @file: h2spec-server.cpp
 * @date: 2026-07-27
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
 * Минимальный сервер HTTP/2 поверх парсера AWH для прогона набора h2spec.
 * Транспорт нарочно примитивен: один блокирующий сокет и последовательная
 * обработка соединений - h2spec открывает по соединению на проверку
 */

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <csignal>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <proto/http/parser/http2/http.hpp>

using namespace awh;
using namespace awh::http;

/**
 * @brief Функция печати кадров потока байт
 *
 * @param prefix направление передачи
 * @param buffer буфер потока байт
 * @param size   размер потока байт
 */
static void trace(const char * prefix, const void * buffer, const size_t size) noexcept {
	// Таблица названий типов кадров
	static const char * names[] = {
		"DATA", "HEADERS", "PRIORITY", "RST_STREAM", "SETTINGS",
		"PUSH_PROMISE", "PING", "GOAWAY", "WINDOW_UPDATE", "CONTINUATION"
	};
	// Указатель на данные потока
	const uint8_t * data = static_cast <const uint8_t *> (buffer);
	// Текущая позиция разбора потока
	size_t pos = 0;
	// Пропускаем magic-строку preface
	if((size >= 24) && (::memcmp(data, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24) == 0))
		// Сдвигаем позицию за magic-строку
		pos = 24;
	/**
	 * Выполняем перебор всех кадров потока
	 */
	while((pos + 9) <= size){
		// Извлекаем длину полезной нагрузки кадра
		const size_t length = ((static_cast <size_t> (data[pos]) << 16) | (static_cast <size_t> (data[pos + 1]) << 8) | data[pos + 2]);
		// Извлекаем тип кадра
		const uint8_t type = data[pos + 3];
		// Извлекаем идентификатор потока
		const uint32_t sid = ((static_cast <uint32_t> (data[pos + 5] & 0x7F) << 24) | (static_cast <uint32_t> (data[pos + 6]) << 16) | (static_cast <uint32_t> (data[pos + 7]) << 8) | data[pos + 8]);
		// Печатаем параметры кадра
		::printf(
			"%s %s len=%zu flags=%02X sid=%u\n", prefix,
			((type < 10) ? names[type] : "UNKNOWN"), length, data[pos + 4], sid
		);
		// Сбрасываем буфер вывода: трасса нужна по ходу прогона, а не в конце
		::fflush(stdout);
		// Если кадр целиком не поместился - прекращаем разбор
		if((pos + 9 + length) > size)
			// Прекращаем разбор потока
			break;
		// Сдвигаем позицию за разобранный кадр
		pos += (9 + length);
	}
}

int32_t main(int32_t argc, char * argv[]) noexcept {
	// Порт прослушивания сервера
	const uint16_t port = static_cast <uint16_t> ((argc > 1) ? ::atoi(argv[1]) : 8080);
	// Признак трассировки кадров
	const bool tracing = ((argc > 2) && (::strcmp(argv[2], "trace") == 0));
	/**
	 * Игнорируем сигнал разрыва канала: набор проверок закрывает соединение
	 * в произвольный момент, а запись в закрытый сокет иначе снимет процесс
	 */
	::signal(SIGPIPE, SIG_IGN);
	// Создаём объект фреймворка
	std::unique_ptr <awh::fmk_t> fmk(new awh::fmk_t());
	// Создаём объект для работы с логами
	std::unique_ptr <awh::log_t> log(new awh::log_t(fmk.get()));
	// Отключаем вывод логов: набор проверок намеренно шлёт некорректный трафик
	log->level(awh::log_t::level_t::NONE);
	// Создаём сокет прослушивания
	const int listener = ::socket(AF_INET, SOCK_STREAM, 0);
	// Если сокет создать не удалось
	if(listener < 0){
		// Печатаем ошибку создания сокета
		std::cout << "не удалось создать сокет" << std::endl;
		// Выводим результат
		return EXIT_FAILURE;
	}
	// Признак переиспользования адреса
	const int reuse = 1;
	// Разрешаем переиспользование адреса
	::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	// Параметры адреса прослушивания
	struct sockaddr_in addr;
	// Обнуляем параметры адреса
	::memset(&addr, 0, sizeof(addr));
	// Устанавливаем семейство адреса
	addr.sin_family = AF_INET;
	// Устанавливаем адрес прослушивания
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	// Устанавливаем порт прослушивания
	addr.sin_port = htons(port);
	// Выполняем привязку сокета к адресу
	if(::bind(listener, reinterpret_cast <struct sockaddr *> (&addr), sizeof(addr)) < 0){
		// Печатаем ошибку привязки сокета
		std::cout << "не удалось занять порт " << port << std::endl;
		// Закрываем сокет прослушивания
		::close(listener);
		// Выводим результат
		return EXIT_FAILURE;
	}
	// Переводим сокет в режим прослушивания
	::listen(listener, 64);
	// Печатаем готовность сервера
	std::cout << "сервер HTTP/2 слушает 127.0.0.1:" << port << std::endl;
	/**
	 * Выполняем обработку всех входящих соединений
	 */
	for(;;){
		// Принимаем очередное соединение
		const int fd = ::accept(listener, nullptr, nullptr);
		// Если соединение принять не удалось
		if(fd < 0)
			// Переходим к следующему соединению
			continue;
		/**
		 * Обрабатываем соединение в отдельном потоке: набор проверок открывает
		 * соединения одно за другим и не всегда закрывает предыдущее, а обработка
		 * по очереди задержала бы следующую проверку до истечения таймаута
		 */
		std::thread([fd, tracing, &fmk, &log]() noexcept {
			// Отключаем алгоритм Нейгла: ответы обязаны уходить немедленно
			const int nodelay = 1;
			// Применяем параметр сокета
			::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
			// Таймаут чтения соединения
			struct timeval timeout;
			// Устанавливаем секунды таймаута
			timeout.tv_sec = 3;
			// Устанавливаем микросекунды таймаута
			timeout.tv_usec = 0;
			// Применяем таймаут чтения
			::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			// Создаём объект парсера сервера
			parser_http2_t server(direct_t::REQUEST, fmk.get(), log.get());
			// Признак разрыва соединения
			bool closed = false;
			// Устанавливаем функцию обратного вызова записи исходящих байт
			server.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
				// Если включена трассировка кадров
				if(tracing)
					// Печатаем отправляемые кадры
					::trace("<-", buffer, size);
				// Текущая позиция отправки
				size_t offset = 0;
				/**
				 * Отправляем исходящие байты в сокет целиком
				 */
				while(offset < size){
					// Отправляем очередную порцию исходящих байт
					const ssize_t sent = ::send(fd, static_cast <const char *> (buffer) + offset, size - offset, 0);
					// Если отправка не удалась
					if(sent <= 0){
						// Помечаем разрыв соединения
						closed = true;
						// Прекращаем отправку
						return;
					}
					// Сдвигаем позицию отправки
					offset += static_cast <size_t> (sent);
				}
			}));
			// Устанавливаем функцию обратного вызова фазы приёма сообщения
			server.on(parser_http2_t::phase_callback_t([&](const uint32_t sid, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
				// Если приём запроса завершён полностью
				if((phase == parser_t::phase_t::END) && (part == parser_t::part_t::NONE)){
					// Формируем заголовки ответа сервера
					std::vector <h2::hpack::field_t> response;
					// Дописываем псевдо-заголовок статуса ответа
					response.emplace_back(":status", "200");
					// Дописываем заголовок типа содержимого
					response.emplace_back("content-type", "text/plain");
					// Дописываем заголовок длины тела
					response.emplace_back("content-length", "2");
					// Отправляем заголовки ответа
					server.sendHeaders(sid, response, false);
					// Отправляем тело ответа с завершением потока
					server.sendData(sid, "ok", 2, true);
				}
				// Продолжаем разбор
				return true;
			}));
			// Обновляем время rate-лимитов парсера
			server.updateTime(0);
			// Отправляем preface сервера
			server.sendPreface();
			// Буфер принимаемых байт
			std::vector <char> buffer(65536);
			/**
			 * Выполняем обмен байтами с клиентом
			 */
			while(!closed){
				// Принимаем очередную порцию байт
				const ssize_t bytes = ::recv(fd, buffer.data(), buffer.size(), 0);
				// Если соединение закрыто либо истёк таймаут
				if(bytes <= 0)
					// Прекращаем обмен
					break;
				// Если включена трассировка кадров
				if(tracing)
					// Печатаем принятые кадры
					::trace("->", buffer.data(), static_cast <size_t> (bytes));
				// Подаём принятые байты на разбор
				server.parse(buffer.data(), static_cast <size_t> (bytes));
				// Если разбор завершился ошибкой уровня соединения
				if(server.status() == parser_t::status_t::ERROR)
					// Прекращаем обмен: GOAWAY уже отправлен функцией записи
					break;
			}
			// Закрываем соединение
			::close(fd);
		}).detach();
	}
	// Закрываем сокет прослушивания
	::close(listener);
	// Выводим результат
	return EXIT_SUCCESS;
}
