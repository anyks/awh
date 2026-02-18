// socket_compat.h
#ifndef SOCKET_COMPAT_H
#define SOCKET_COMPAT_H

// Обычные сокеты (по умолчанию):
#ifndef USE_FSTACK
#ifndef USE_XDP
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/epoll.h>
	
	// Сокеты:
	#define socket_compat(domain, type, protocol)  socket(domain, type, protocol)
	#define bind_compat(sock, addr, addrlen)       bind(sock, addr, addrlen)
	#define listen_compat(sock, backlog)           listen(sock, backlog)
	#define accept_compat(sock, addr, addrlen)     accept(sock, addr, addrlen)
	#define connect_compat(sock, addr, addrlen)    connect(sock, addr, addrlen)
	#define close_compat(sock)                     close(sock)
	#define shutdown_compat(sock, how)             shutdown(sock, how)
	
	// Чтение/запись:
	#define recv_compat(sock, buf, len, flags)     recv(sock, buf, len, flags)
	#define send_compat(sock, buf, len, flags)     send(sock, buf, len, flags)
	#define read_compat(sock, buf, len)            read(sock, buf, len)
	#define write_compat(sock, buf, len)           write(sock, buf, len)
	#define recvfrom_compat(sock, buf, len, flags, src, srclen) \
		recvfrom(sock, buf, len, flags, src, srclen)
	#define sendto_compat(sock, buf, len, flags, dst, dstlen) \
		sendto(sock, buf, len, flags, dst, dstlen)
	
	// Epoll:
	#define epoll_create_compat(size)              epoll_create(size)
	#define epoll_ctl_compat(epfd, op, fd, event)  epoll_ctl(epfd, op, fd, event)
	#define epoll_wait_compat(epfd, events, max, timeout) \
		epoll_wait(epfd, events, max, timeout)
	
	// Poll/Select:
	#define poll_compat(fds, nfds, timeout)        poll(fds, nfds, timeout)
	#define select_compat(nfds, readfds, writefds, exceptfds, timeout) \
		select(nfds, readfds, writefds, exceptfds, timeout)
#endif
#endif

// F-Stack (DPDK):
#ifdef USE_FSTACK
	#include <ff.h>
	#include <ff_epoll.h>
	
	// Сокеты:
	#define socket_compat(domain, type, protocol)  ff_socket(domain, type, protocol)
	#define bind_compat(sock, addr, addrlen)       ff_bind(sock, addr, addrlen)
	#define listen_compat(sock, backlog)           ff_listen(sock, backlog)
	#define accept_compat(sock, addr, addrlen)     ff_accept(sock, addr, addrlen)
	#define connect_compat(sock, addr, addrlen)    ff_connect(sock, addr, addrlen)
	#define close_compat(sock)                     ff_close(sock)
	#define shutdown_compat(sock, how)             ff_shutdown(sock, how)
	
	// Чтение/запись:
	#define recv_compat(sock, buf, len, flags)     ff_recv(sock, buf, len, flags)
	#define send_compat(sock, buf, len, flags)     ff_send(sock, buf, len, flags)
	#define read_compat(sock, buf, len)            ff_read(sock, buf, len)
	#define write_compat(sock, buf, len)           ff_write(sock, buf, len)
	#define recvfrom_compat(sock, buf, len, flags, src, srclen) \
		ff_recvfrom(sock, buf, len, flags, src, srclen)
	#define sendto_compat(sock, buf, len, flags, dst, dstlen) \
		ff_sendto(sock, buf, len, flags, dst, dstlen)
	
	// Epoll:
	#define epoll_create_compat(size)              ff_epoll_create(size)
	#define epoll_ctl_compat(epfd, op, fd, event)  ff_epoll_ctl(epfd, op, fd, event)
	#define epoll_wait_compat(epfd, events, max, timeout) \
		ff_epoll_wait(epfd, events, max, timeout)
	
	// Poll/Select:
	#define poll_compat(fds, nfds, timeout)        ff_poll(fds, nfds, timeout)
	#define select_compat(nfds, readfds, writefds, exceptfds, timeout) \
		ff_select(nfds, readfds, writefds, exceptfds, timeout)
#endif

#endif // SOCKET_COMPAT_H
