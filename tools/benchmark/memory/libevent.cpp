/*
 * Удельный расход памяти на одно наблюдаемое подключение, libevent.
 *
 * Заводит заданное количество соединённых сокетов, подписывает каждый на
 * готовность к чтению и снимает пик занятой памяти. Величина берётся не как
 * полный объём процесса, а как наклон: разность между двумя количествами
 * подключений, делённая на разность количеств. База процесса при этом
 * сокращается, и остаётся ровно то, что стоит само подключение.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <event2/event.h>

// Пик занятой памяти в октетах
static size_t rss(){
	struct rusage r{};
	::getrusage(RUSAGE_SELF, &r);
	return static_cast <size_t> (r.ru_maxrss);
}

// Пустой обработчик готовности к чтению
static void onRead(evutil_socket_t, short, void *){}

int main(int argc, char ** argv){
	const size_t count = ((argc > 1) ? static_cast <size_t> (::atol(argv[1])) : 1000);
	struct event_base * loop = event_base_new();
	// Слушающий сокет на свободном порту
	const int listener = ::socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr{};
	socklen_t length = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	if(::bind(listener, reinterpret_cast <struct sockaddr *> (&addr), sizeof(addr)) != 0) return 1;
	if(::listen(listener, 512) != 0) return 1;
	if(::getsockname(listener, reinterpret_cast <struct sockaddr *> (&addr), &length) != 0) return 1;
	// Заводим подключения и подписываем каждое на чтение
	std::vector <struct event *> watchers;
	watchers.reserve(count * 2);
	for(size_t i = 0; i < count; i++){
		const int client = ::socket(AF_INET, SOCK_STREAM, 0);
		if(::connect(client, reinterpret_cast <struct sockaddr *> (&addr), sizeof(addr)) != 0) return 1;
		const int peer = ::accept(listener, nullptr, nullptr);
		if(peer < 0) return 1;
		for(const int fd : {client, peer}){
			::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
			struct event * w = event_new(loop, fd, EV_READ | EV_PERSIST, onRead, nullptr);
			event_add(w, nullptr);
			watchers.push_back(w);
		}
	}
	::printf("%zu %zu\n", count, rss());
	return 0;
}
