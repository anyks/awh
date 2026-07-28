/*
 * Удельный расход памяти на одно наблюдаемое подключение, движок AWH.
 *
 * Заводит заданное количество подключений, подписывает каждое на готовность к
 * чтению и снимает пик занятой памяти. Величина берётся не как полный объём
 * процесса, а как наклон: разность между двумя количествами подключений,
 * делённая на разность количеств. База процесса при этом сокращается, и
 * остаётся ровно то, что стоит само подключение.
 *
 * Обмена нет: подключения только заводятся и остаются наблюдаемыми.
 */
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <memory>

#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <net/io.hpp>

using namespace std;

int main(int argc, char ** argv){
	// Требуемое количество подключений
	const size_t count = ((argc > 1) ? static_cast <size_t> (::atol(argv[1])) : 1000);
	// Объект фреймворка
	awh::fmk_t fmk;
	// Объект логирования
	awh::log_t log(&fmk);
	// Отключаем вывод логирования
	log.level(awh::log_t::level_t::NONE);
	// Объект асинхронного движка ввода-вывода
	awh::engine::io_t io(&fmk, &log);
	// Выполняем инициализацию движка
	if(!io.initialize()){
		// Сообщаем о неудачной инициализации
		::printf("движок не инициализирован\n");
		// Выходим с признаком ошибки
		return 1;
	}
	// Набор опций события
	const uint16_t options = (
		awh::event::options::NO_SIGILL |
		awh::event::options::NO_SIGPIPE |
		awh::event::options::REUSE_ADDR |
		awh::event::options::NO_IO_BLOCK |
		awh::event::options::CLOSE_ON_EXEC |
		awh::event::options::TCP_NO_DELAY
	);
	// Добавляем событие сервера
	const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Устанавливаем опции события сервера
	io.setOptions(server, options);
	// Устанавливаем адрес события сервера
	io.setAddress(server, awh::event::address_t::IPV4, "127.0.0.1");
	// Подбираем свободный порт временным сокетом
	uint16_t number = 45000;
	{
		// Выполняем создание временного сокета
		const int32_t fd = ::socket(AF_INET, SOCK_STREAM, 0);
		// Если временный сокет создан
		if(fd >= 0){
			// Параметры привязки временного сокета
			struct sockaddr_in addr{};
			// Устанавливаем семейство адреса
			addr.sin_family = AF_INET;
			// Устанавливаем адрес петлевого интерфейса
			addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			// Запрашиваем у системы любой свободный порт
			addr.sin_port = 0;
			// Выполняем привязку временного сокета
			if(::bind(fd, reinterpret_cast <struct sockaddr *> (&addr), sizeof(addr)) == 0){
				// Размер структуры параметров сокета
				socklen_t length = sizeof(addr);
				// Извлекаем выделенный системой порт
				if(::getsockname(fd, reinterpret_cast <struct sockaddr *> (&addr), &length) == 0)
					// Запоминаем выделенный порт
					number = ntohs(addr.sin_port);
			}
			// Закрываем временный сокет
			::close(fd);
		}
	}
	// Устанавливаем порт события сервера
	io.setSourcePort(server, number);
	// Количество принятых сервером подключений
	size_t accepted = 0;
	// Устанавливаем функцию обратного вызова на принятие входящего подключения
	io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &accepted, options](const awh::event::id_t, const awh::event::id_t cid) noexcept -> void {
		// Устанавливаем опции принятого подключения
		io.setOptions(cid, options);
		// Считаем принятое подключение
		accepted++;
		// Подписываем принятое подключение на готовность к чтению
		io.on(cid, [](const awh::event::id_t, const uint8_t *, const size_t) noexcept -> void {});
	}));
	// Выполняем фиксацию настроек события сервера
	io.commit(server);
	/**
	 * Очередь ожидающих подключений берётся с запасом: клиенты подключаются все
	 * разом, до первого оборота опроса, и очередь по умолчанию переполнилась бы
	 */
	io.listen(server, 16384);
	// Запускаем событие сервера
	io.launch(server);
	/**
	 * Выполняем создание требуемого количества клиентских подключений
	 */
	for(size_t i = 0; i < count; i++){
		// Добавляем новое событие клиента
		const awh::event::id_t cid = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Устанавливаем порт назначения события клиента
		io.setTargetPort(cid, number);
		// Устанавливаем опции события клиента
		io.setOptions(cid, options);
		// Устанавливаем адрес привязки события клиента
		io.setAddress(cid, awh::event::address_t::IPV4, "0.0.0.0");
		// Устанавливаем адрес назначения события клиента
		io.setTarget(cid, "127.0.0.1");
		// Подписываем клиента на готовность к чтению
		io.on(cid, [](const awh::event::id_t, const uint8_t *, const size_t) noexcept -> void {});
		// Выполняем фиксацию настроек события клиента
		io.commit(cid);
		// Выполняем подключение клиента к серверу
		io.connect(cid);
		// Запускаем событие клиента
		io.launch(cid);
	}
	/**
	 * Крутим опрос, пока сервер не примет все подключения
	 */
	for(size_t turn = 0; (accepted < count) && (turn < 20000000); turn++)
		io.poll();
	// Если приняты не все подключения
	if(accepted < count){
		// Сообщаем, сколько подключений удалось принять
		::printf("принято только %zu из %zu\n", accepted, count);
		// Выходим с признаком ошибки
		return 1;
	}
	// Снимаем пик занятой памяти
	{
		// Объект сведений о расходе ресурсов
		struct rusage usage{};
		// Получаем сведения о расходе ресурсов
		::getrusage(RUSAGE_SELF, &usage);
		// Выводим количество подключений и пик занятой памяти в октетах
		::printf("%zu %zu\n", count, static_cast <size_t> (usage.ru_maxrss));
	}
	// Выполняем деинициализацию движка
	io.deinitialize();
	// Выводим успешный результат
	return 0;
}
