/**
 * @file: readpath.c
 * @date: 2026-08-11
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Стенд сличения путей приёма данных - чтение до EAGAIN против запроса
 *        доступного объёма через ioctl(FIONREAD)
 *
 * @details Очередь Event Ports, в отличие от kqueue, НЕ сообщает вместе с событием
 *          готовности, сколько октетов доступно для чтения. Взять это число можно
 *          отдельным обращением к ядру, а можно не брать вовсе и читать, пока чтение
 *          не упрётся в EAGAIN. Стенд отвечает, чего стоит каждый путь - по времени,
 *          по числу обращений к ядру и по расходу памяти
 *
 * @note Мера здесь не одна. Путь с запросом объёма делает лишнее обращение к ядру, но
 *       читает ровно один раз и ровно в буфер нужного размера. Путь до EAGAIN лишнего
 *       обращения не делает, но всегда доплачивает одним холостым чтением, а на
 *       сообщениях крупнее буфера - ещё и несколькими полными
 *
 * @copyright: Copyright © 2026
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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <sys/resource.h>
#include <procfs.h>

static int  * rfd;          /* приёмные концы пар */
static int  * wfd;          /* передающие концы пар */
static long   syscalls;     /* обращения к ядру за замер */
static long   reads;        /* вызовы чтения за замер */
static long   allocated;    /* наибольший разовый расход буферов */

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
static int pairs(const int count, const int payload){
	int i, sv[2], size;
	rfd = calloc((size_t) count, sizeof(int));
	wfd = calloc((size_t) count, sizeof(int));
	for(i = 0; i < count; i++){
		if(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0){
			printf("  пар заведено лишь %d: %s\n", i, strerror(errno));
			return i;
		}
		/* Приёмная сторона неблокирующая: иначе чтение до EAGAIN повиснет */
		fcntl(sv[0], F_SETFL, fcntl(sv[0], F_GETFL, 0) | O_NONBLOCK);
		/* Буферы сокета обязаны вмещать полезную нагрузку целиком */
		size = (payload * 2);
		setsockopt(sv[0], SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
		setsockopt(sv[1], SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
		rfd[i] = sv[0];
		wfd[i] = sv[1];
	}
	return count;
}

/* Приём данных запросом доступного объёма */
static void take_ioctl(const int fd, char * scratch, const int limit){
	long total = 0;
	/**
	 * Вычерпываем сокет ДО КОНЦА, как это делает и путь до EAGAIN. Иначе сличение
	 * нечестно: один путь освобождает сокет целиком, другой оставляет остаток, и на
	 * нагрузке крупнее буфера отправитель упирается в переполнение
	 */
	for(;;){
		int volume = 0;
		/* Спрашиваем у ядра доступный объём */
		if(ioctl(fd, FIONREAD, &volume) < 0)
			break;
		syscalls++;
		if(volume <= 0)
			break;
		/* Читаем ровно столько, сколько сказало ядро, но не свыше буфера */
		if(read(fd, scratch, (size_t)((volume > limit) ? limit : volume)) <= 0){
			syscalls++;
			reads++;
			break;
		}
		syscalls++;
		reads++;
		total += ((volume > limit) ? limit : volume);
		/* Объём уместился в буфер целиком - сокет пуст */
		if(volume <= limit)
			break;
	}
	allocated = ((total > allocated) ? total : allocated);
}

/* Приём данных чтением до отказа */
static void take_eagain(const int fd, char * scratch, const int limit){
	ssize_t size;
	long total = 0;
	for(;;){
		size = read(fd, scratch, (size_t) limit);
		syscalls++;
		reads++;
		if(size <= 0)
			break;
		total += size;
		/* Чтение короче буфера означает, что данных больше нет */
		if(size < (ssize_t) limit)
			break;
	}
	allocated = ((total > allocated) ? total : allocated);
}

/* Общий прогон */
static double run(const int mode, const int total, const int active, const int rounds, const int payload, const int limit){
	int port, i, n, got;
	uint_t count;
	port_event_t * list = calloc((size_t) total, sizeof(port_event_t));
	char * scratch = malloc((size_t) limit);
	char * message = malloc((size_t) payload);
	timespec_t ts;
	long long begin;
	double result;
	const int step = ((active > 0) ? (total / active) : 1);
	memset(message, 'x', (size_t) payload);
	if((port = port_create()) < 0)
		return -1.0;
	for(i = 0; i < total; i++){
		if(port_associate(port, PORT_SOURCE_FD, (uintptr_t) rfd[i], POLLIN, NULL) < 0){
			close(port);
			return -1.0;
		}
	}
	syscalls = total;
	reads = 0;
	allocated = 0;
	begin = now();
	for(n = 0; n < rounds; n++){
		for(i = 0; i < active; i++)
			write(wfd[(i * step) % total], message, (size_t) payload);
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
				const int fd = (int) list[i].portev_object;
				if(mode)
					take_ioctl(fd, scratch, limit);
				else take_eagain(fd, scratch, limit);
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
	free(scratch);
	free(message);
	return result;
}

int main(int argc, char ** argv){
	const int total   = ((argc > 1) ? atoi(argv[1]) : 1000);
	const int active  = ((argc > 2) ? atoi(argv[2]) : 100);
	const int rounds  = ((argc > 3) ? atoi(argv[3]) : 200);
	const int payload = ((argc > 4) ? atoi(argv[4]) : 4096);
	const int limit   = ((argc > 5) ? atoi(argv[5]) : 65536);
	/**
	 * Путь замера задаётся отдельно ради ЧЕСТНОГО расхода памяти: гоняя оба пути в
	 * одном процессе, второй наследует расход первого, и столбец памяти лжёт
	 */
	const int only    = ((argc > 6) ? atoi(argv[6]) : -1);
	long base;
	setvbuf(stdout, NULL, _IONBF, 0);
	descriptors((total * 2) + 64);
	if(pairs(total, payload) < total)
		return 1;
	base = memory();
	printf("нагрузка %d октетов, буфер %d, сокетов %d, работает %d, оборотов %d\n", payload, limit, total, active, rounds);
	printf("%-10s %9s %12s %12s %12s %12s\n", "путь", "секунд", "событий/с", "вызовов", "чтений", "память,КБ");
	{
		struct { const char * name; int mode; } list[] = {
			{ "eagain", 0 },
			{ "fionread", 1 },
		};
		size_t i;
		for(i = 0; i < (sizeof(list) / sizeof(list[0])); i++){
			// Если замеряется единственный путь, прочие пропускаем
			if((only >= 0) && (list[i].mode != only))
				continue;
			const double seconds = run(list[i].mode, total, active, rounds, payload, limit);
			const long used = (memory() - base);
			if(seconds < 0.0){
				printf("%-10s %9s\n", list[i].name, "отказ");
				continue;
			}
			printf("%-10s %9.3f %12.0f %12ld %12ld %12ld\n", list[i].name, seconds,
			 ((double)(rounds * active) / seconds), syscalls, reads, used);
		}
	}
	return 0;
}
