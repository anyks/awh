/**
 * @brief Проверка адаптивного приёма на настоящей реализации Windows IOCP
 *
 * @details Собирается отдельно под Windows вместе с libawh.a. Backend включён напрямую:
 *          политика не копируется в тест. Проверки действуют и при NDEBUG.
 *          Все проверки выполняются в одном потоке; заключительная
 *          проверка запускает настоящий цикл событий для TCP localhost.
 *
 */
#include <chrono>
#include <string>
#include <cstdio>
#include <cstdlib>
/**
 * Если путь к проверяемому исходнику не задан
 */
#if !defined(AWH_IOCP_SOURCE)
	/**
	 * @brief Подключаем текущую реализацию движка
	 *
	 */
	#define AWH_IOCP_SOURCE "src/net/backend/win/iocp.cpp"
#endif
#include AWH_IOCP_SOURCE

namespace adaptive_probe {
	/**
	 * @brief Функция проверки условия с безусловным отказом процесса
	 *
	 * @param value   проверяемое условие
	 * @param message название проверки
	 *
	 */
	static void require(const bool value, const char * message) noexcept {
		// Если проверяемое условие нарушено
		if(!value){
			// Сообщаем причину отказа и завершаем процесс
			::fprintf(stderr, "ADAPTIVE_WHITEBOX_FAIL: %s\n", message);
			::fflush(stderr);
			::_Exit(EXIT_FAILURE);
		}
	}
	/**
	 * @brief Функция проверки неизменности состояния по неподходящему завершению
	 *
	 * @param state    запись подписки
	 * @param sock     дескриптор завершившейся операции
	 * @param token    метка завершившейся операции
	 * @param receiver владелец завершившейся операции
	 * @param capacity поданная длина приёма
	 * @param size     результат приёма
	 * @param message  название проверки
	 *
	 */
	static void unchanged(::kernel::registry_t & state, const awh::net::socket_t sock, const uint64_t token, void * receiver, const uint32_t capacity, const int64_t size, const char * message) noexcept {
		// Запоминаем оба поля перед обращением к настоящей политике
		const uint32_t previous = state.capacity;
		const uint8_t small = state.small;
		::kernel::adapt(state, sock, token, receiver, capacity, size);
		// Неподходящее завершение не должно менять даже счётчик коротких приёмов
		::adaptive_probe::require((state.capacity == previous) && (state.small == small), message);
	}
	/**
	 * @brief Функция проверки роста, уменьшения и защиты от чужих завершений
	 *
	 */
	static void policy() noexcept {
		// Задаём владельцев и условный дескриптор без обращения к системе
		int32_t owner = 0, other = 0;
		const awh::net::socket_t sock = 7;
		const uint64_t token = 42;
		::kernel::registry_t state;
		::adaptive_probe::require((state.capacity == 4096) && (state.small == 0), "initial 4 KiB");
		state.sock = sock;
		state.token = token;
		state.udata = &owner;
		// Полные завершения должны увеличить длину в два шага до 64 КиБ
		::kernel::adapt(state, sock, token, &owner, 4096, 4096);
		::adaptive_probe::require((state.capacity == 16384) && (state.small == 0), "full 4 to 16 KiB");
		::kernel::adapt(state, sock, token, &owner, 16384, 16384);
		::adaptive_probe::require((state.capacity == 65536) && (state.small == 0), "full 16 to 64 KiB");
		state.small = 7;
		::kernel::adapt(state, sock, token, &owner, 65536, 65536);
		::adaptive_probe::require((state.capacity == 65536) && (state.small == 0), "full at ceiling resets streak");
		// Каждое уменьшение допускается лишь после шестнадцатого короткого приёма
		for(uint32_t capacity = 65536; capacity > 4096; capacity /= 4){
			for(uint8_t count = 1; count < 16; count++){
				::kernel::adapt(state, sock, token, &owner, capacity, capacity / 4);
				::adaptive_probe::require((state.capacity == capacity) && (state.small == count), "short streak before threshold");
			}
			::kernel::adapt(state, sock, token, &owner, capacity, capacity / 4);
			::adaptive_probe::require((state.capacity == capacity / 4) && (state.small == 0), "sixteenth short shrinks");
		}
		// Нижняя граница сохраняется и не накапливает короткие завершения
		::kernel::adapt(state, sock, token, &owner, 4096, 64);
		::adaptive_probe::require((state.capacity == 4096) && (state.small == 0), "short at floor");
		// Средний приём прерывает последовательность коротких завершений
		state.capacity = 16384;
		state.small = 15;
		::kernel::adapt(state, sock, token, &owner, 16384, 8192);
		::adaptive_probe::require((state.capacity == 16384) && (state.small == 0), "medium resets streak");
		::kernel::adapt(state, sock, token, &owner, 16384, 64);
		::adaptive_probe::require((state.capacity == 16384) && (state.small == 1), "short after medium starts new streak");
		// Неподходящие завершения сохраняют длину и накопленную последовательность
		state.small = 7;
		::adaptive_probe::unchanged(state, sock, token, &owner, 16384, 0, "EOF");
		::adaptive_probe::unchanged(state, sock, token, &owner, 16384, -1, "error");
		::adaptive_probe::unchanged(state, sock, token, &owner, 0, 64, "nonadaptive operation");
		::adaptive_probe::unchanged(state, sock, token + 1, &owner, 16384, 16384, "stale token");
		::adaptive_probe::unchanged(state, sock, token, &other, 16384, 16384, "wrong owner");
		::adaptive_probe::unchanged(state, sock + 1, token, &owner, 16384, 16384, "wrong descriptor");
		::adaptive_probe::unchanged(state, sock, token, nullptr, 16384, 16384, "no receiver");
		::adaptive_probe::unchanged(state, sock, token, &owner, 4096, 4096, "obsolete posted capacity");
		::adaptive_probe::unchanged(state, sock, token, &owner, 16384, 16385, "oversized result");
		state.token = ::inflight::INVALID;
		::adaptive_probe::unchanged(state, sock, ::inflight::INVALID, &owner, 16384, 16384, "invalid token on both sides");
		state.token = token;
		state.capacity = 1024;
		::adaptive_probe::unchanged(state, sock, token, &owner, 1024, 1024, "capacity below minimum");
		state.capacity = 262144;
		::adaptive_probe::unchanged(state, sock, token, &owner, 262144, 262144, "capacity above maximum");
	}
	/**
	 * @brief Функция проверки очистки метаданных при повторном занятии ячейки
	 *
	 */
	static void slots() noexcept {
		// Занимаем первую ячейку и записываем метаданные адаптивного приёма
		int32_t owner = 0, other = 0;
		const uint64_t first = ::inflight::acquire(::inflight::kind_t::RECV, 7, &owner);
		::inflight::slot_t * slot = ::inflight::get(first);
		::adaptive_probe::require(slot != nullptr, "first slot acquired");
		slot->capacity = 65536;
		slot->receiver = &owner;
		const uint32_t index = slot->index;
		const uint32_t generation = slot->generation;
		// Освобождаем ячейку и занимаем её для другого владельца
		::inflight::release(first);
		::adaptive_probe::require(::inflight::get(first) == nullptr, "released token rejected");
		const uint64_t second = ::inflight::acquire(::inflight::kind_t::RECV, 7, &other);
		slot = ::inflight::get(second);
		::adaptive_probe::require(slot != nullptr, "reused slot acquired");
		::adaptive_probe::require((slot->index == index) && (slot->generation != generation) && (first != second), "slot generation changed");
		::adaptive_probe::require((slot->capacity == 0) && (slot->receiver == nullptr), "reused slot adaptive metadata cleared");
		::adaptive_probe::require((slot->udata == &other) && (::inflight::get(first) == nullptr), "old token cannot reach new owner");
		// Возвращаем ячейку без запущенных системных операций
		::inflight::release(second);
		::inflight::clear();
	}
	/**
	 * @brief Функция проверки частичного чтения фактически отложенного буфера
	 *
	 */
	static void partial() noexcept {
		// Проверяем смешанную последовательность на одном условном дескрипторе
		int32_t owner = 0;
		const awh::net::socket_t sock = 7;
		const uint32_t lengths[] = {64, 65536, 64};
		const size_t portions[] = {1, 17, 257, 4093};
		for(size_t phase = 0; phase < 3; phase++){
			// Занимаем настоящий буфер пула и заполняем различимыми октетами
			const uint16_t bid = ::pool::take();
			::adaptive_probe::require(bid != ::pool::INVALID, "pool buffer acquired");
			uint8_t * source = ::pool::data(bid);
			::adaptive_probe::require(source != nullptr, "pool buffer available");
			for(uint32_t index = 0; index < lengths[phase]; index++)
				source[index] = static_cast <uint8_t> ((index * 37 + phase * 53 + index / 251) & 0xFF);
			::pool::keep(sock, bid, static_cast <int32_t> (lengths[phase]), &owner, nullptr);
			// Размер потребительского буфера меняется и всегда меньше большого приёма
			size_t offset = 0, step = 0;
			while(offset < lengths[phase]){
				uint8_t target[4095];
				::memset(target, 0xA5, sizeof(target));
				const size_t capacity = portions[step++ % 4];
				const size_t expected = ::min(capacity, static_cast <size_t> (lengths[phase]) - offset);
				ssize_t received = -1;
				::adaptive_probe::require(::pool::receive(sock, target + 1, capacity, received), "partial receive handled");
				::adaptive_probe::require(received == static_cast <ssize_t> (expected), "partial receive length");
				::adaptive_probe::require((target[0] == 0xA5) && (target[expected + 1] == 0xA5), "consumer buffer boundaries");
				for(size_t index = 0; index < expected; index++){
					const uint8_t value = static_cast <uint8_t> (((offset + index) * 37 + phase * 53 + (offset + index) / 251) & 0xFF);
					::adaptive_probe::require(target[index + 1] == value, "partial receive byte integrity");
				}
				offset += expected;
				::adaptive_probe::require(::pool::pending(sock) == (offset < lengths[phase]), "unread tail remains pending");
			}
			// Полностью забранный буфер больше не обслуживает чтение
			uint8_t target = 0;
			ssize_t received = -1;
			::adaptive_probe::require(!::pool::receive(sock, &target, sizeof(target), received), "drained receive not replayed");
			const uint16_t reused = ::pool::take();
			::adaptive_probe::require(reused == bid, "drained buffer returned to pool");
			::pool::give(reused);
		}
		// Освобождаем таблицы, не содержащие системных операций
		::pool::destroy();
	}
	/**
	 * @brief Функция проверки фактической подачи короткого потокового приёма
	 *
	 * @details Сокет не связан с портом завершений. Отмена и ожидание результата
	 *          завершаются до освобождения буфера, ячейки и сокетов.
	 *
	 */
	static void submission() noexcept {
		// Поднимаем пару TCP на локальном интерфейсе без движка IO
		WSADATA startup{};
		::adaptive_probe::require(::WSAStartup(MAKEWORD(2, 2), &startup) == 0, "Winsock startup");
		const SOCKET listener = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		const SOCKET client = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		::adaptive_probe::require((listener != static_cast <SOCKET> (awh::net::invalid_socket_t)) && (client != static_cast <SOCKET> (awh::net::invalid_socket_t)), "TCP sockets");
		struct sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
		address.sin_port = 0;
		::adaptive_probe::require(::bind(listener, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) == 0, "TCP bind");
		int32_t length = sizeof(address);
		::adaptive_probe::require(::getsockname(listener, reinterpret_cast <struct sockaddr *> (&address), &length) == 0, "TCP address");
		::adaptive_probe::require(::listen(listener, 1) == 0, "TCP listen");
		::adaptive_probe::require(::connect(client, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) == 0, "TCP connect");
		const SOCKET server = ::accept(listener, nullptr, nullptr);
		::adaptive_probe::require(server != static_cast <SOCKET> (awh::net::invalid_socket_t), "TCP accept");
		// Подаём приём без данных, чтобы реальная операция ожидала у системы
		::io::node_t owner;
		::kernel::registry_t state;
		const uint64_t token = ::post::fetch(server, &state, false, false, &owner, 4096);
		::adaptive_probe::require(token != ::inflight::INVALID, "real fetch submitted");
		::inflight::slot_t * slot = ::inflight::get(token);
		::adaptive_probe::require(slot != nullptr, "real fetch token resolves");
		const bool metadata = ((slot->wsabuf.len == 4096) && (slot->capacity == 4096) && (slot->receiver == &owner) && (slot->udata == &state) && (slot->sock == server));
		// Сначала отменяем операцию, затем обязательно дожидаемся её завершения
		const BOOL cancelled = ::CancelIoEx(reinterpret_cast <HANDLE> (server), &slot->overlapped);
		const DWORD cancelError = (cancelled ? ERROR_SUCCESS : ::GetLastError());
		::adaptive_probe::require(cancelled || (cancelError == ERROR_NOT_FOUND), "pending receive cancellation");
		DWORD received = 0, flags = 0;
		const BOOL completed = ::WSAGetOverlappedResult(server, &slot->overlapped, &received, TRUE, &flags);
		const int32_t completionError = (completed ? 0 : ::WSAGetLastError());
		::adaptive_probe::require(completed || (completionError == WSA_OPERATION_ABORTED), "cancelled receive reached terminal completion");
		// Только завершённая операция разрешает вернуть память в свободные списки
		::pool::give(slot->bid);
		::inflight::release(token);
		::closesocket(server);
		::closesocket(client);
		::closesocket(listener);
		::WSACleanup();
		::pool::destroy();
		::inflight::clear();
		// Проверяем сохранённые результаты после безопасного освобождения ресурсов
		::adaptive_probe::require(metadata, "real fetch buffer length and adaptive metadata");
	}
};

/**
 * @brief Проверка связи завершения TCP-приёма с размером следующей подачи
 *
 * @note Проверка связывает публичную доставку данных с состоянием подписки
 *       и фактической длиной следующего приёма, поданного системе.
 *
 */
namespace adaptive_probe {
	/**
	 * @brief Функция проверки адаптации через настоящий цикл событий
	 *
	 */
	static void integration() noexcept {
		// Движок проверяемого обмена
		awh::engine::io_t engine;
		// Идентификатор принятого подключения
		awh::event::id_t peer = 0;
		// Признак успешной настройки принятого подключения
		bool configured = false;
		// Принятый и ожидаемый потоки данных
		std::string received, expected;
		// Выполняем инициализацию движка
		::adaptive_probe::require(engine.initialize(), "integration initialize");
		// Сервер получает свободный порт от системы без предварительного резервирования
		const awh::event::id_t server = engine.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		::adaptive_probe::require(server > 0, "integration server event");
		// Набор опций неблокирующего TCP-подключения
		const uint16_t options = (awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::TCP_NO_DELAY);
		// Настраиваем слушающее событие на локальном интерфейсе
		::adaptive_probe::require(engine.setOptions(server, options), "integration server options");
		::adaptive_probe::require(engine.setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"), "integration server address");
		::adaptive_probe::require(engine.setSourcePort(server, 0), "integration ephemeral port");
		// Устанавливаем обработчик принятого подключения
		engine.on(server, static_cast <awh::engine::callback::accept_t> ([&]([[maybe_unused]] const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Сохраняем принятое подключение
			peer = cid;
			// Применяем опции к принятому подключению
			configured = engine.setOptions(cid, options);
			// Принимаем данные публичным обработчиком
			engine.on(cid, [&received]([[maybe_unused]] const awh::event::id_t eid, const uint8_t * buffer, const size_t size) noexcept -> void {
				// Проверка ниже сравнивает поток целиком, включая границы предыдущих отправок
				received.append(reinterpret_cast <const char *> (buffer), size);
			});
		}));
		// Запускаем слушающее событие
		::adaptive_probe::require(engine.commit(server), "integration server commit");
		::adaptive_probe::require(engine.listen(server, 8), "integration server listen");
		::adaptive_probe::require(engine.launch(server), "integration server launch");
		// Находим узел слушающего события для запроса назначенного порта
		auto listener = ::__awh_nodes__.find(server);
		::adaptive_probe::require(listener != ::__awh_nodes__.end(), "integration listener node");
		// Адрес слушающего сокета
		struct sockaddr_in address{};
		// Размер структуры адреса
		int32_t length = sizeof(address);
		// Получаем порт, выбранный системой
		::adaptive_probe::require(::getsockname(static_cast <SOCKET> (::io::descriptor(listener->second.get())), reinterpret_cast <struct sockaddr *> (&address), &length) == 0, "integration listener port");
		// Клиент подключается через публичный интерфейс движка
		const awh::event::id_t client = engine.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		::adaptive_probe::require(client > 0, "integration client event");
		// Настраиваем неблокирующее подключение к назначенному порту
		::adaptive_probe::require(engine.setOptions(client, options), "integration client options");
		::adaptive_probe::require(engine.setAddress(client, awh::event::address_t::IPV4, "0.0.0.0"), "integration client address");
		::adaptive_probe::require(engine.setTarget(client, "127.0.0.1"), "integration client target");
		::adaptive_probe::require(engine.setTargetPort(client, ::ntohs(address.sin_port)), "integration client port");
		// Запускаем клиентское подключение
		::adaptive_probe::require(engine.commit(client), "integration client commit");
		::adaptive_probe::require(engine.connect(client), "integration client connect");
		::adaptive_probe::require(engine.launch(client), "integration client launch");
		// Начало ограниченного ожидания принятия подключения
		const auto connected = std::chrono::steady_clock::now();
		// Ожидаем принятия подключения не дольше установленного срока
		while((peer == 0) && ((std::chrono::steady_clock::now() - connected) < std::chrono::seconds(10)))
			// Ожидание ограничено независимо от появления сетевых событий
			::adaptive_probe::require(engine.poll(10), "integration connect poll");
		::adaptive_probe::require((peer > 0) && configured, "integration accepted peer");
		// Находим узел принятого подключения
		auto node = ::__awh_nodes__.find(peer);
		::adaptive_probe::require(node != ::__awh_nodes__.end(), "integration peer node");
		// Дескриптор принятого подключения
		const awh::net::socket_t sock = ::io::descriptor(node->second.get());
		// Владелец операций приёма проверяемого подключения
		void * const receiver = node->second.get();
		/**
		 * @brief Функция ожидания новой подачи приёма после разбора отправки
		 *
		 * @details Проверяет выбранную длину и WSABUF действующей операции.
		 *
		 * @return запись подписки с поданной операцией приёма
		 *
		 */
		auto armed = [&]() noexcept -> ::kernel::registry_t * {
			// Начало ограниченного ожидания новой подачи
			const auto started = std::chrono::steady_clock::now();
			// Ожидаем появления действующей операции приёма
			while((std::chrono::steady_clock::now() - started) < std::chrono::seconds(10)){
				// Находим подписку проверяемого дескриптора
				auto entry = ::kernel::registry.find(sock);
				// Если подписка существует
				if(entry != ::kernel::registry.end()){
					// Получаем состояние подписки
					::kernel::registry_t & state = entry->second;
					// Получаем запись поданной операции
					::inflight::slot_t * slot = ::inflight::get(state.token);
					// Если операция является действующим приёмом
					if((slot != nullptr) && (slot->kind == ::inflight::kind_t::RECV) && !slot->cancelled){
						// Проверяем принадлежность операции текущей подписке и владельцу
						::adaptive_probe::require((state.udata == receiver) && (slot->receiver == receiver) && (slot->udata == &state) && (slot->sock == sock), "integration posted owner");
						// Нулевая адаптивная длина выдаёт отключение адаптации при подаче
						::adaptive_probe::require((slot->capacity == state.capacity) && (slot->wsabuf.len == state.capacity), "integration next posted capacity");
						// Выводим подписку с проверенной операцией
						return &state;
					}
				}
				// Разбираем события, необходимые для новой подачи
				::adaptive_probe::require(engine.poll(1), "integration rearm poll");
			}
			// Завершаем проверку отказом при истечении срока
			::adaptive_probe::require(false, "integration receive rearm deadline");
			// Выводим отсутствие подписки для полноты возвращаемого типа
			return nullptr;
		};
		/**
		 * @brief Функция передачи данных с проверкой следующей подачи приёма
		 *
		 * @details Отправка начинается при поданном приёме и заканчивается полной
		 *          выдачей данных публичному обработчику и новой подачей приёма.
		 *
		 * @param size размер передаваемых данных
		 * @return     запись подписки после передачи и новой подачи приёма
		 *
		 */
		auto transfer = [&](const size_t size) noexcept -> ::kernel::registry_t * {
			// Дожидаемся действующего приёма до начала отправки
			(void) armed();
			// Буфер передаваемых данных
			std::string payload(size, '\0');
			// Заполняем буфер различимым содержимым с учётом предыдущих отправок
			for(size_t index = 0; index < size; index++)
				// Устанавливаем очередной октет
				payload[index] = static_cast <char> ((index * 37 + expected.size()) & 0xFF);
			// Дополняем ожидаемый поток до начала отправки
			expected.append(payload);
			// Число байт, принятых отправкой
			size_t sent = 0;
			// Начало ограниченного ожидания обмена
			const auto started = std::chrono::steady_clock::now();
			// Передаём данные и ожидаем их публичной доставки
			while(((sent < size) || (received.size() < expected.size())) && ((std::chrono::steady_clock::now() - started) < std::chrono::seconds(10))){
				// Если остались неотправленные данные
				if(sent < size){
					// Передаём оставшийся хвост
					const size_t accepted = engine.send(client, payload.data() + sent, size - sent);
					// Проверяем и учитываем принятый объём
					::adaptive_probe::require(accepted <= (size - sent), "integration send size");
					sent += accepted;
				}
				// Разбираем завершения отправки и приёма
				::adaptive_probe::require(engine.poll(1), "integration transfer poll");
			}
			// Проверяем полную доставку и побайтное совпадение потока
			::adaptive_probe::require((sent == size) && (received == expected), "integration byte integrity and deadline");
			// Выводим подписку после следующей подачи приёма
			return armed();
		};
		// Начальный приём обязан быть действительно подан на четыре КиБ
		::adaptive_probe::require(armed()->capacity == 4096, "integration initial posted 4 KiB");
		// Проверяем оба перехода роста до верхней границы
		for(uint32_t capacity = 4096; capacity < 65536; capacity *= 4){
			// TCP может разделить отправку: повторяем ограниченно, не требуя границ сообщений
			::kernel::registry_t * state = armed();
			// Повторяем отправку до роста либо исчерпания числа попыток
			for(uint8_t attempt = 0; (attempt < 32) && (state->capacity == capacity); attempt++)
				// Получаем подписку после передачи и следующей подачи
				state = transfer(capacity);
			// Проверяем увеличение фактически поданной длины вчетверо
			::adaptive_probe::require(state->capacity == (capacity * 4), "integration completion grows next submission");
		}
		// Перед уменьшением начинаем новую последовательность коротких завершений
		::adaptive_probe::require(armed()->small == 0, "integration growth cleared short streak");
		// Проверяем оба перехода уменьшения до нижней границы
		for(uint32_t capacity = 65536; capacity > 4096; capacity /= 4){
			// Передаём ровно шестнадцать коротких сообщений
			for(uint8_t count = 1; count <= 16; count++){
				// Один байт не может разделиться на несколько положительных завершений
				::kernel::registry_t * state = transfer(1);
				// Ожидаемая длина следующего приёма
				const uint32_t next = (count < 16 ? capacity : capacity / 4);
				// Ожидаемая длина последовательности коротких завершений
				const uint8_t small = (count < 16 ? count : 0);
				// Проверяем порог уменьшения и сброс последовательности
				::adaptive_probe::require((state->capacity == next) && (state->small == small), "integration short completion shrinks next submission");
			}
		}
		// Уничтожаем события и обработчики до разрушения захваченных данных
		::adaptive_probe::require(engine.destroy(client), "integration destroy client");
		::adaptive_probe::require(engine.destroy(peer), "integration destroy peer");
		::adaptive_probe::require(engine.destroy(server), "integration destroy server");
		::adaptive_probe::require(engine.deinitialize(), "integration deinitialize");
		// Сообщаем об успешном завершении интеграционной проверки
		::fprintf(stdout, "ADAPTIVE_INTEGRATION_OK: completion, next submit, growth, shrink, integrity\n");
	}
};

/**
 * @brief Точка входа автономной проверки без GoogleTest
 *
 */
int main() {
	// Инициализируем выдачу памяти до создания объектов проверки
	awh::fmk::initialize();
	// Проверяем настоящий helper, повторное занятие ячеек и сохранность хвоста
	::adaptive_probe::policy();
	::adaptive_probe::slots();
	::adaptive_probe::partial();
	::adaptive_probe::submission();
	// Проверяем связь завершения с размером следующего реального приёма
	::adaptive_probe::integration();
	::fprintf(stdout, "ADAPTIVE_WHITEBOX_OK: policy, slot reuse, partial integrity, real fetch\n");
	return EXIT_SUCCESS;
}
