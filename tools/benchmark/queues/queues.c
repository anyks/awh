/**
 * @file queues.c
 * @date 2026-08-11
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
 * @brief Стенд сличения очередей готовности Sun Solaris и illumos - /dev/poll,
 *        Event Ports и связка из них, под нагрузкой и на растущем наборе сокетов
 *
 * @details Отвечает на вопрос, ради чего вводили Event Ports, если /dev/poll держит
 *          подписку сам и заводится пачкой: растёт ли расход с числом ЗАВЕДЁННЫХ
 *          сокетов или только с числом сработавших. Оттого доля работающих задаётся
 *          ОТДЕЛЬНО от их общего числа - без этого разделения два расхода неразличимы,
 *          а весь вопрос именно в их различии
 *
 * @note Итог замера и решение по нему - в README.md рядом
 *
 * @copyright Copyright © 2026
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <port.h>
#include <time.h>
#include <sys/devpoll.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <procfs.h>

static int  * rfd;                 /* приёмные концы пар */
static int  * wfd;                 /* передающие концы пар */
static long   syscalls;            /* обращения к ядру за замер */

/* Текущий расход памяти в килооктетах */
static long memory(void){
	char path[64];
	psinfo_t info;
	int fd;
	long size = -1;
	snprintf(path, sizeof(path), "/proc/%d/psinfo", (int) getpid());
	if((fd = open(path, O_RDONLY)) < 0)
		return size;
	if(read(fd, &info, sizeof(info)) == sizeof(info))
		size = (long) info.pr_rssize;
	close(fd);
	return size;
}

/* Отметка времени в наносекундах */
static long long now(void){
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((long long) ts.tv_sec * 1000000000LL + ts.tv_nsec);
}

/* Поднятие предела на число дескрипторов */
static void descriptors(const int need){
	struct rlimit rl;
	if(getrlimit(RLIMIT_NOFILE, &rl) == 0){
		if((long) rl.rlim_cur < (long) need){
			rl.rlim_cur = rl.rlim_max;
			setrlimit(RLIMIT_NOFILE, &rl);
		}
	}
}

/* Заведение пар сокетов */
static int pairs(const int count){
	int i, sv[2];
	rfd = calloc((size_t) count, sizeof(int));
	wfd = calloc((size_t) count, sizeof(int));
	for(i = 0; i < count; i++){
		if(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0){
			printf("  пар заведено лишь %d: %s\n", i, strerror(errno));
			return i;
		}
		rfd[i] = sv[0];
		wfd[i] = sv[1];
	}
	return count;
}

/* Побудка доли сокетов: берём с шагом, чтобы работающие были разбросаны по набору */
static void wake(const int total, const int active){
	int i;
	const int step = ((active > 0) ? (total / active) : 1);
	for(i = 0; i < active; i++)
		write(wfd[(i * step) % total], "x", 1);
}

/* Устройство: /dev/poll */
static double bench_devpoll(const int total, const int active, const int rounds){
	int dp, i, n, got;
	struct pollfd * reg = calloc((size_t) total, sizeof(struct pollfd));
	struct pollfd * out = calloc((size_t) total, sizeof(struct pollfd));
	struct dvpoll dvp;
	char buf[8];
	long long begin;
	double result;
	if((dp = open("/dev/poll", O_RDWR)) < 0)
		return -1.0;
	for(i = 0; i < total; i++){
		reg[i].fd = rfd[i];
		reg[i].events = POLLIN;
		reg[i].revents = 0;
	}
	/* Подписка пачкой: одно обращение на весь набор */
	if(write(dp, reg, (size_t) total * sizeof(struct pollfd)) < 0){
		close(dp);
		return -1.0;
	}
	syscalls = 1;
	begin = now();
	for(n = 0; n < rounds; n++){
		int k, ready;
		wake(total, active);
		got = 0;
		while(got < active){
			dvp.dp_fds = out;
			dvp.dp_nfds = ((total < 1024) ? total : 1024);
			dvp.dp_timeout = 1000;
			ready = ioctl(dp, DP_POLL, &dvp);
			syscalls++;
			if(ready <= 0)
				break;
			for(k = 0; k < ready; k++)
				read(out[k].fd, buf, 1);
			got += ready;
		}
	}
	result = (double)(now() - begin) / 1000000000.0;
	close(dp);
	free(reg);
	free(out);
	return result;
}

/* Устройство: Event Ports */
static double bench_ports(const int total, const int active, const int rounds){
	int port, i, n, got;
	uint_t count;
	port_event_t * list = calloc((size_t) total, sizeof(port_event_t));
	char buf[8];
	timespec_t ts;
	long long begin;
	double result;
	if((port = port_create()) < 0)
		return -1.0;
	for(i = 0; i < total; i++){
		if(port_associate(port, PORT_SOURCE_FD, (uintptr_t) rfd[i], POLLIN, NULL) < 0){
			close(port);
			return -1.0;
		}
	}
	syscalls = total;
	begin = now();
	for(n = 0; n < rounds; n++){
		wake(total, active);
		got = 0;
		while(got < active){
			count = 1;
			ts.tv_sec = 1;
			ts.tv_nsec = 0;
			if(port_getn(port, list, (uint_t) total, &count, &ts) < 0){
				if(errno != ETIME)
					break;
			}
			syscalls++;
			if(count == 0)
				break;
			for(i = 0; i < (int) count; i++){
				read((int) list[i].portev_object, buf, 1);
				/* Перевзвод: подписка снята выдачей события */
				port_associate(port, PORT_SOURCE_FD, list[i].portev_object, POLLIN, NULL);
				syscalls++;
			}
			got += (int) count;
		}
	}
	result = (double)(now() - begin) / 1000000000.0;
	close(port);
	free(list);
	return result;
}

/* Устройство: /dev/poll внутри порта оповещений */
static double bench_hybrid(const int total, const int active, const int rounds){
	int dp, port, i, n, got;
	uint_t count;
	struct pollfd * reg = calloc((size_t) total, sizeof(struct pollfd));
	struct pollfd * out = calloc((size_t) total, sizeof(struct pollfd));
	struct dvpoll dvp;
	port_event_t event;
	timespec_t ts;
	char buf[8];
	long long begin;
	double result;
	if(((dp = open("/dev/poll", O_RDWR)) < 0) || ((port = port_create()) < 0))
		return -1.0;
	for(i = 0; i < total; i++){
		reg[i].fd = rfd[i];
		reg[i].events = POLLIN;
		reg[i].revents = 0;
	}
	if(write(dp, reg, (size_t) total * sizeof(struct pollfd)) < 0)
		return -1.0;
	port_associate(port, PORT_SOURCE_FD, (uintptr_t) dp, POLLIN, NULL);
	syscalls = 2;
	begin = now();
	for(n = 0; n < rounds; n++){
		wake(total, active);
		got = 0;
		while(got < active){
			ts.tv_sec = 1;
			ts.tv_nsec = 0;
			if(port_get(port, &event, &ts) < 0)
				break;
			syscalls++;
			dvp.dp_fds = out;
			dvp.dp_nfds = ((total < 1024) ? total : 1024);
			dvp.dp_timeout = 0;
			i = ioctl(dp, DP_POLL, &dvp);
			syscalls++;
			if(i <= 0)
				break;
			for(count = 0; count < (uint_t) i; count++)
				read(out[count].fd, buf, 1);
			(void) 0;
			got += i;
			/* Перевзвод одного дескриптора на всю пачку */
			port_associate(port, PORT_SOURCE_FD, (uintptr_t) dp, POLLIN, NULL);
			syscalls++;
		}
	}
	result = (double)(now() - begin) / 1000000000.0;
	close(dp);
	close(port);
	free(reg);
	free(out);
	return result;
}

int main(int argc, char ** argv){
	const int total  = ((argc > 1) ? atoi(argv[1]) : 1000);
	const int active = ((argc > 2) ? atoi(argv[2]) : 10);
	const int rounds = ((argc > 3) ? atoi(argv[3]) : 100);
	int made;
	long base;
	setvbuf(stdout, NULL, _IONBF, 0);
	descriptors((total * 2) + 64);
	if((made = pairs(total)) < total)
		return 1;
	base = memory();
	printf("%-8s %8s %8s %10s %12s %12s %10s\n", "приём", "всего", "работ", "секунд", "событий/с", "вызовов", "память,КБ");
	{
		struct { const char * name; double (* run)(int, int, int); } list[] = {
			{ "devpoll", bench_devpoll },
			{ "ports",   bench_ports   },
			{ "hybrid",  bench_hybrid  },
		};
		size_t i;
		for(i = 0; i < (sizeof(list) / sizeof(list[0])); i++){
			const double seconds = list[i].run(total, active, rounds);
			const long used = (memory() - base);
			if(seconds < 0.0){
				printf("%-8s %8d %8d %10s\n", list[i].name, total, active, "отказ");
				continue;
			}
			printf("%-8s %8d %8d %10.3f %12.0f %12ld %10ld\n", list[i].name, total, active,
			 seconds, ((double)(rounds * active) / seconds), syscalls, used);
		}
	}
	return 0;
}
