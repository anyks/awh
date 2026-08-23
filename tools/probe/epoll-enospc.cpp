/**
 * Щуп переходной ветви согласования подписки движка epoll.
 *
 * Ядро отвергает epoll_ctl(ADD) с ENOSPC, когда исчерпан предел наблюдений
 * пользователя. Отказ этот временный: предел отпускается вместе с чужими
 * подписками. Движок обязан оставить дескриптор в очереди согласования и
 * подать подписку заново, а не похоронить событие.
 *
 * Щуп опускает предел до единицы, поднимает пару TCP-событий по петле,
 * крутит опрос, затем возвращает предел и ждёт доставки сообщения.
 *
 * Успех: сообщение доставлено ПОСЛЕ возврата предела и до того были отказы
 *        согласования - значит подписка подана заново.
 * Отказ: сообщение не доставлено вовсе - событие похоронено.
 *
 * Запускать от суперпользователя: правит /proc/sys/fs/epoll/max_user_watches
 */
#include <net/io.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>

// Путь к пределу наблюдений epoll
static const char * WATCHES = "/proc/sys/fs/epoll/max_user_watches";

/**
 * Читает предел наблюдений
 */
static long getWatches() noexcept {
	std::ifstream file(WATCHES);
	long result = -1;
	if(file.is_open())
		file >> result;
	return result;
}

/**
 * Ставит предел наблюдений
 */
static bool setWatches(const long value) noexcept {
	std::ofstream file(WATCHES);
	if(!file.is_open())
		return false;
	file << value;
	return file.good();
}

int main(int argc, char * argv[]){
	// Порт петли
	const uint16_t port = static_cast <uint16_t> ((argc > 1) ? ::atoi(argv[1]) : 43917);
	// Исходный предел наблюдений
	const long saved = getWatches();
	if(saved <= 0){
		::printf("PROBE: предел наблюдений не прочитан\n");
		return 2;
	}
	::printf("PROBE: исходный предел наблюдений %ld\n", saved);

	awh::fmk_t fmk;
	awh::log_t log(&fmk);
	awh::engine::io_t io(&fmk, &log);

	// Признак доставки сообщения
	bool delivered = false;
	// Оборот опроса, на котором предел возвращён
	int restoredAt = -1;

	const auto events = io.events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	if((events[0] == 0) || (events[1] == 0)){
		::printf("PROBE: события не созданы\n");
		return 2;
	}
	io.setTargetPort(events[0], port);
	io.setSourcePort(events[1], port);

	if(!io.initialize()){
		::printf("PROBE: движок не инициализирован\n");
		return 2;
	}
	for(uint8_t i = 0; i < 2; i++)
		io.setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE |
		 awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC);

	// Серверное событие: принимает подключение и отвечает эхом
	io.setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1");
	io.on(events[1], static_cast <awh::engine::callback::accept_t> ([&io](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
		::printf("PROBE: подключение принято\n");
		io.on(cid, [&io](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			io.send(eid, reinterpret_cast <const char *> (data), size);
		});
		io.commit(cid);
		io.launch(cid);
	}));
	io.commit(events[1]);
	io.listen(events[1], 100);
	io.launch(events[1]);

	// Клиентское событие: подключается и шлёт сообщение
	io.setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0");
	io.setTarget(events[0], "127.0.0.1");
	io.on(events[0], static_cast <awh::engine::callback::connect_t> ([&io](const awh::event::id_t eid, const bool ok) noexcept -> void {
		if(ok){
			const std::string message("PING");
			io.send(eid, message.c_str(), message.size());
		}
	}));
	io.on(events[0], [&delivered](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		::printf("PROBE: получен ответ %zu байт\n", size);
		delivered = true;
	});

	// Опускаем предел наблюдений: подписки клиента ядро отвергнет с ENOSPC
	if(!setWatches(1)){
		::printf("PROBE: предел наблюдений не опущен (нужен root)\n");
		return 2;
	}
	::printf("PROBE: предел наблюдений опущен до 1\n");

	io.commit(events[0]);
	io.connect(events[0]);
	io.launch(events[0]);

	// Крутим опрос: первые обороты под опущенным пределом, затем предел возвращаем
	for(int turn = 0; (turn < 400) && !delivered; turn++){
		if(turn == ((argc > 2) ? ::atoi(argv[2]) : 40)){
			setWatches(saved);
			restoredAt = turn;
			::printf("PROBE: предел наблюдений возвращён на обороте %d\n", turn);
		}
		if(!io.poll(25)){
			::printf("PROBE: опрос отказал на обороте %d\n", turn);
			break;
		}
	}
	// Возвращаем предел в любом случае
	setWatches(saved);
	io.deinitialize();

	::printf("PROBE: доставлено=%s, предел возвращён на обороте=%d\n", delivered ? "ДА" : "НЕТ", restoredAt);
	return (delivered ? 0 : 1);
}
