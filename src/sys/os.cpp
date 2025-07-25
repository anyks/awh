/**
 * @file: os.cpp
 * @date: 2024-04-02
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Стандартные модули
 */
#include <regex>

/**
 * Операционной системой является Linux
 */
#if __linux__
	/**
	 * Стандартные модули
	 */
	#include <queue>
	#include <linux/sysctl.h>
#endif

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Стандартные модули
	 */
	#include <pwd.h>
	#include <grp.h>
	#include <sys/resource.h>
/**
 * Для операционной системы OS Windows
 */
#else
	/**
	 * Стандартные модули
	 */
	#include <wchar.h>
#endif

/**
 * Если операционной системой является MacOS X, FreeBSD, NetBSD, OpenBSD
 */
#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * Стандартные модули
	 */
	#include <sys/sysctl.h>
#endif

/**
 * Подключаем заголовочный файл
 */
#include <sys/os.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * boost Метод применение сетевой оптимизации операционной системы
 * @return результат работы
 */
void awh::OS::boost() const noexcept {
	/**
	 * Для операционной системы OS Windows
	 */
	#if _WIN32 || _WIN64
		// Vista/7 также включает «Compound TCP (CTCP)», который похож на CUBIC в Linux
		this->exec("netsh interface tcp set global congestionprovider=ctcp");
		// Если вам вообще нужно включить автонастройку, вот команды
		this->exec("netsh interface tcp set global autotuninglevel=normal");
	/**
	 * Реализация под Sun Solaris
	 */
	#elif __sun__
		// Если эффективный идентификатор пользователя принадлежит ROOT
		if(::geteuid() == 0){
			// Эмпирическое правило: max_buf = 2 x cwnd_max (окно перегрузки)
			this->exec("ndd -set /dev/tcp tcp_max_buf 4194304");
			this->exec("ndd -set /dev/tcp tcp_cwnd_max 2097152");
			// Увеличиваем размер окна TCP по умолчанию
			this->exec("ndd -set /dev/tcp tcp_xmit_hiwat 65536");
			this->exec("ndd -set /dev/tcp tcp_recv_hiwat 65536");
		}
	/**
	 * Для операционной системы MacOS X
	 */
	#elif __APPLE__ || __MACH__
		// Если эффективный идентификатор пользователя принадлежит ROOT
		if(::geteuid() == 0){
			// Устанавливаем максимальное количество подключений
			this->sysctl("kern.ipc.somaxconn", 49152);
			/**
			 * Для хостов 10G было бы неплохо увеличить это значение,
			 * т.к. 4G, похоже, является пределом для некоторых установок MacOS X
			 */
			this->sysctl("kern.ipc.maxsockbuf", 6291456);
			// Увеличиваем максимальный размер буферов для отправки
			this->sysctl("net.inet.tcp.sendspace", 1042560);
			// Увеличиваем максимальный размер буферов для чтения
			this->sysctl("net.inet.tcp.recvspace", 1042560);
			// В MacOS X значение по умолчанию 3, что очень мало
			this->sysctl("net.inet.tcp.r", 8);
			// Увеличиваем максимумы автонастройки MacOS X TCP
			this->sysctl("net.inet.tcp.autorcvbufmax", 33554432);
			this->sysctl("net.inet.tcp.autosndbufmax", 33554432);
			// Устанавливаем прочие настройки
			this->sysctl("net.inet.tcp.slowstart_flightsize", 20);
			this->sysctl("net.inet.tcp.local_slowstart_flightsize", 20);
		}
	/**
	 * Для операционной системы Linux
	 */
	#elif __linux__
		// Если эффективный идентификатор пользователя принадлежит ROOT
		if(::geteuid() == 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Для отладки активируем создание дампов ядра
				this->sysctl("kernel.core_uses_pid", 1);
				this->sysctl("kernel.core_pattern", string{"/tmp/%e-%p.core"});
			#endif
			// Разрешаем выборочные подтверждения (Selective Acknowledgements, SACK)
			this->sysctl("net.ipv4.tcp_sack", 1);
			// Активируем параметр помогающий в борье за ресурсы
			this->sysctl("net.ipv4.tcp_tw_reuse", 1);
			// Разрешаем использование временных меток (timestamps) в протоколах TCP
			this->sysctl("net.ipv4.tcp_timestamps", 1);
			// Устанавливаем максимальное количество подключений
			this->sysctl("net.core.somaxconn", 49152);
			// Увеличиваем максимальный размер буферов для чтения
			this->sysctl("net.core.rmem_max", 16777216);
			// Увеличиваем максимальный размер буферов для отправки
			this->sysctl("net.core.wmem_max", 16777216);
			// Разрешаем масштабирование TCP-окна
			this->sysctl("net.ipv4.tcp_window_scaling", 1);
			// Запрещаем сохранять результаты измерений TCP-соединения в кэше при его закрытии
			this->sysctl("net.ipv4.tcp_no_metrics_save", 1);
			// Включаем автоматическую настройку размера приёмного буфера TCP
			this->sysctl("net.ipv4.tcp_moderate_rcvbuf", 1);
			// Определяем максимальное количество входящих пакетов
			this->sysctl("net.core.netdev_max_backlog", 2500);
			// Увеличиваем лимит автонастройки TCP-буфера Linux до 64 МБ
			this->sysctl("net.ipv4.tcp_rmem", string{"\"4096 87380 16777216\""});
			this->sysctl("net.ipv4.tcp_wmem", string{"\"4096 65536 16777216\""});
			// Рекомендуется для хостов с включенными большими фреймами
			this->sysctl("net.ipv4.tcp_mtu_probing", 1);
			// Рекомендуется для хостов CentOS 7/Debian 8
			this->sysctl("net.core.default_qdisc", string{"fq"});
			/**
			 * Рекомендуемый контроль перегрузки по умолчанию — htcp.
			 * Вы можете проверить, какие доступны алгоритмы получения доступных сообщений, используя net.ipv4.tcp_available_congestion_control
			 */
			const string & algorithm = this->congestionControl(this->sysctl <string> ("net.ipv4.tcp_available_congestion_control"));
			// Если выбран лучший доступны алгоритм
			if(!algorithm.empty())
				// Активируем выбранный нами алгоритм
				this->sysctl("net.ipv4.tcp_congestion_control", algorithm);
		}
	/**
	 * Для операционной системы FreeBSD, NetBSD или OpenBSD
	 */
	#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Если эффективный идентификатор пользователя принадлежит ROOT
		if(::geteuid() == 0){
			/**
			 * Данные оптимизаций операционной системы берет от сюда: http://fasterdata.es.net/host-tuning/freebsd
			 */
			// Активируем контроль работы временной марки и масштабируемого окна
			this->sysctl("net.inet.tcp.rfc1323", 1);
			// Устанавливаем максимальное количество подключений
			this->sysctl("kern.ipc.somaxconn", 49152);
			// Активируем автоматическую отправку и получение
			this->sysctl("net.inet.tcp.sendbuf_auto", 1);
			this->sysctl("net.inet.tcp.recvbuf_auto", 1);
			// Увеличиваем размер шага автонастройки
			this->sysctl("net.inet.tcp.sendbuf_inc", 8192);
			this->sysctl("net.inet.tcp.recvbuf_inc", 16384);
			// Активируем нормальное нормальное TCP Reno
			this->sysctl("net.inet.tcp.inflight.enable", 0);
			// Активируем на хостах тестирования/измерений
			this->sysctl("net.inet.tcp.hostcache.expire", 1);
			/**
			 * Для хостов 10G было бы неплохо увеличить это значение,
			 * т.к. 4G, похоже, является пределом для некоторых установок FreeBSD
			 */
			this->sysctl("kern.ipc.maxsockbuf", 16777216);
			// Увеличиваем максимальный размер буферов для отправки
			this->sysctl("net.inet.tcp.sendspace", 1042560);
			// Увеличиваем максимальный размер буферов для чтения
			this->sysctl("net.inet.tcp.recvspace", 1042560);
			// Увеличиваем максимальный размер буферов для отправки
			this->sysctl("net.inet.tcp.sendbuf_max", 16777216);
			// Увеличиваем максимальный размер буферов для чтения
			this->sysctl("net.inet.tcp.recvbuf_max", 16777216);
			/**
			 * Вы можете проверить, какие доступны алгоритмы получения доступных сообщений, используя net.inet.tcp.cc.available
			 */
			const string & algorithm = this->congestionControl(this->sysctl <string> ("net.inet.tcp.cc.available"));
			// Если выбран лучший доступны алгоритм
			if(!algorithm.empty())
				// Активируем выбранный нами алгоритм
				this->sysctl("net.inet.tcp.cc.algorithm", algorithm);
		}
	#endif
}
/**
 * type Метод определения операционной системы
 * @return название операционной системы
 */
awh::OS::type_t awh::OS::type() const noexcept {
	// Результат
	type_t result = type_t::NONE;
	/**
	 * Операционной системой является Windows 32bit
	 */
	#ifdef _WIN32
		// Заполняем структуру
		result = type_t::WIND32;
	/**
	 * Операционной системой является Windows 64bit
	 */
	#elif _WIN64
		// Заполняем структуру
		result = type_t::WIND64;
	/**
	 * Операционной системой является MacOS X
	 */
	#elif __APPLE__ || __MACH__
		// Заполняем структуру
		result = type_t::MACOSX;
	/**
	 * Операционной системой является Linux
	 */
	#elif __linux__
		// Заполняем структуру
		result = type_t::LINUX;
	/**
	 * Операционной системой является FreeBSD
	 */
	#elif __FreeBSD__
		// Заполняем структуру
		result = type_t::FREEBSD;
	/**
	 * Операционной системой является NetBSD
	 */
	#elif __NetBSD__
		// Заполняем структуру
		result = type_t::NETBSD;
	/**
	 * Операционной системой является OpenBSD
	 */
	#elif __OpenBSD__
		// Заполняем структуру
		result = type_t::OPENBSD;
	/**
	 * Реализация под Sun Solaris
	 */
	#elif __sun__
		// Заполняем структуру
		result = type_t::SOLARIS;
	/**
	 * Операционной системой является Unix
	 */
	#elif __unix || __unix__
		// Заполняем структуру
		result = type_t::UNIX;
	#endif
	// Выводим результат
	return result;
}
/**
 * enableCoreDumps Метод активации создания дампа ядра
 * @return результат установки лимитов дампов ядра
 */
bool awh::OS::enableCoreDumps() const noexcept {
	/**
	 * Если включён режим отладки
	 */
	#if DEBUG_MODE
		/**
		 * Для операционной системы не являющейся OS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Структура лимитов дампов
			struct rlimit limit;
			// Устанавливаем текущий лимит равный бесконечности
			limit.rlim_cur = RLIM_INFINITY;
			// Устанавливаем максимальный лимит равный бесконечности
			limit.rlim_max = RLIM_INFINITY;
			// Выводим результат установки лимита дампов ядра
			return (::setrlimit(RLIMIT_CORE, &limit) == 0);
		#endif
	#endif
	// Сообщаем, что ничего не установленно
	return false;
}
/**
 * uid Метод вывода идентификатора пользователя
 * @param name имя пользователя
 * @return     полученный идентификатор пользователя
 */
uid_t awh::OS::uid(const string & name) const noexcept {
	// Результат работы функции
	uid_t result = 0;
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если имя пользователя передано
		if(!name.empty()){
			// Получаем идентификатор имени пользователя
			const auto * pwd = ::getpwnam(name.c_str());
			// Если идентификатор пользователя не найден
			if(pwd == nullptr)
				// Сообщаем что ничего не найдено
				return result;
			// Выводим идентификатор пользователя
			result = pwd->pw_uid;
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * gid Метод вывода идентификатора группы пользователя
 * @param name название группы пользователя
 * @return     полученный идентификатор группы пользователя
 */
gid_t awh::OS::gid(const string & name) const noexcept {
	// Результат работы функции
	gid_t result = 0;
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если имя пользователя передано
		if(!name.empty()){
			// Получаем идентификатор группы пользователя
			struct group * grp = ::getgrnam(name.c_str());
			// Если идентификатор группы не найден
			if(grp == nullptr)
				// Сообщаем что ничего не найдено
				return result;
			// Выводим идентификатор группы пользователя
			result = grp->gr_gid;
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * congestionControl Метод определения алгоритма работы сети
 * @param str строка с выводмом доступных алгоритмов из sysctl
 * @return    выбранная строка с названием алгоритма
 */
string awh::OS::congestionControl(const string & str) const noexcept {
	// Если строка с выводом доступных алгоритмов передана
	if(!str.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("(cubic|htcp)", regex::ECMAScript | regex::icase);
		// Выполняем поиск протокола
		regex_search(str, match, e);
		// Если протокол найден
		if(!match.empty() && (match.size() == 2))
			// Выводим полученный результат
			return match[1].str();
	}
	// Если ничего не найдено то выводим пустую строку
	return "";
}
/**
 * limitFDs Метод установки количество разрешенных файловых дескрипторов
 * @param cur текущий лимит на количество открытых файловых дискриптеров
 * @param max максимальный лимит на количество открытых файловых дискриптеров
 * @return    результат выполнения операции
 */
bool awh::OS::limitFDs(const uint32_t cur, const uint32_t max) const noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Структура для установки лимитов
		struct rlimit limit;
		// зададим текущий лимит на кол-во открытых дискриптеров
		limit.rlim_cur = static_cast <rlim_t> (cur);
		// зададим максимальный лимит на кол-во открытых дискриптеров
		limit.rlim_max = static_cast <rlim_t> (max);
		// установим указанное кол-во
		return (::setrlimit(RLIMIT_NOFILE, &limit) == 0);
	/**
	 * Для операционной системы OS Windows
	 */
	#else
		// Сообщаем, что ничего не установленно
		return false;
	#endif
}
/**
 * chown Метод запуска приложения от имени указанного пользователя
 * @param uid идентификатор пользователя
 * @param gid идентификатор группы пользователя
 * @return    результат выполнения операции
 */
bool awh::OS::chown(const uid_t uid, const gid_t gid) const noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Результат работы функции
		bool result = true;
		// Если идентификатор пользвоателя передан
		if(uid > 0)
			// Устанавливаем идентификатор пользователя
			result = (::setuid(uid) == 0);
		// Если группа паользователя передана
		if(result && (gid > 0))
			// Устанавливаем идентификатор группы
			result = (::setgid(gid) == 0);
		// Выводим результат
		return result;
	/**
	 * Для операционной системы OS Windows
	 */
	#else
		// Сообщаем, что ничего не установленно
		return false;
	#endif
}
/**
 * chown Метод запуска приложения от имени указанного пользователя
 * @param user  название пользователя
 * @param group название группы пользователя
 * @return      результат выполнения операции
 */
bool awh::OS::chown(const string & user, const string & group) const noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Результат работы функции
		bool result = true;
		// Если название пользователя передано
		if(!user.empty()){
			// Получаем идентификатор имени пользователя
			const auto * pwd = ::getpwnam(user.c_str());
			// Если идентификатор пользователя не найден
			if(pwd == nullptr)
				// Сообщаем что ничего не найдено
				return false;
			// Устанавливаем идентификатор пользователя
			result = (::setuid(pwd->pw_uid) == 0);
		}
		// Если название группы пользователя передано
		if(result && !group.empty()){
			// Получаем идентификатор группы пользователя
			const auto * grp = ::getgrnam(group.c_str());
			// Если идентификатор группы не найден
			if(grp == nullptr)
				// Сообщаем что ничего не найдено
				return false;
			// Устанавливаем идентификатор группы
			result = (::setgid(grp->gr_gid) == 0);
		}
		// Выводим результат
		return result;
	/**
	 * Для операционной системы OS Windows
	 */
	#else
		// Сообщаем, что ничего не установленно
		return false;
	#endif
}
/**
 * sysctl Метод извлечения настроек ядра операционной системы
 * @param name   название записи для получения настроек
 * @param buffer бинарный буфер с извлечёнными значениями
 */
void awh::OS::sysctl(const string & name, vector <char> & buffer) const noexcept {
	// Если название параметра и тип извлекаемого значения переданы
	if(!name.empty()){
		// Выполняем очистку буфера данных
		buffer.clear();
		/**
		 * Если мы работаем в MacOS X, FreeBSD, NetBSD или OpenBSD
		 */
		#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Получаем размер буфера
			size_t length = 0;
			// Если размеры удачно получены
			if(::sysctlbyname(name.c_str(), nullptr, &length, nullptr, 0) == 0){
				// Выделяем в буфере нужное количество памяти
				buffer.resize(length, 0);
				// Запрашиваем искомые данные
				if(::sysctlbyname(name.c_str(), buffer.data(), &length, nullptr, 0) < 0)
					// Выполняем очистку буфера данных
					buffer.clear();
			}
		/**
		 * Если это Linux
		 */
		#elif __linux__
			// Создаём комманду запуска
			string command = "sysctl";
			// Добавляем разделитель
			command.append(1, ' ');
			// Добавляем название параметра
			command.append(name);
			// Добавляем разделитель
			command.append(1, ' ');
			// Добавляем усечение строки
			command.append("| cut -d \"=\" -f2 | xargs");
			// Выполняем получение значения команды
			const string & result = this->exec(command, false);
			// Если результат получен
			if(!result.empty()){
				// Очередь собранных данных
				std::queue <pair <string, bool>> data;
				// Выполняем перебор всего полученного результата
				for(auto & item : result){
					// Если символ является пробелом
					if(::isspace(item) || (item == '\t') || (item == '\n') || (item == '\r') || (item == '\f') || (item == '\v')){
						// Если очередь уже не пустая
						if(!data.empty()){
							// Если запись является числом
							if(data.back().second){
								// Выполняем создание блока данных
								pair <string, bool> record = std::make_pair("", true);
								// Выполняем добавление записи в очередь
								data.push(::move(record));
							// Если запись является строкой, добавляем полученный символ в запись
							} else data.back().first.append(1, ' ');
						}
					// Если символ является числом
					} else if(std::isdigit(item) || ((item == '-') && (data.empty() || data.back().first.empty()))) {
						// Если данных в очереди ещё нет
						if(data.empty()){
							// Выполняем создание блока данных
							pair <string, bool> record = std::make_pair(string(1, item), true);
							// Выполняем добавление записи в очередь
							data.push(::move(record));
						// Если данные в очереди уже есть, добавляем полученный символ в запись
						} else data.back().first.append(1, item);
					// Если символ является простым символом		  
					} else if(item != 0) {
						// Если данных в очереди ещё нет
						if(data.empty()){
							// Выполняем создание блока данных
							pair <string, bool> record = std::make_pair(string(1, item), false);
							// Выполняем добавление записи в очередь
							data.push(::move(record));
						// Если данные в очереди уже есть
						} else {
							// Помечаем что запись не является числом
							data.back().second = false;
							// Добавляем полученный символ в запись
							data.back().first.append(1, item);
						}
					}
				}
				// Выполняем перебор всей очереди собранных данных
				while(!data.empty()){
					// Если запись существует
					if(!data.front().first.empty()){
						// Если запись является числом
						if(data.front().second){
							/**
							 * Выполняем отлов ошибок
							 */
							try {
								// Выполняем получение числа
								const uint64_t value1 = ::stoull(data.front().first);
								// Пытаемся уменьшить число
								if(static_cast <uint64_t> (static_cast <uint32_t> (value1)) == value1){
									// Выполняем преобразование в unsigned int 32
									const uint32_t value2 = static_cast <uint32_t> (value1);
									// Выполняем добавление новой записи в буфер
									buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value2), reinterpret_cast <const char *> (&value2) + sizeof(value2));
								// Выполняем добавление новой записи в буфер
								} else buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value1), reinterpret_cast <const char *> (&value1) + sizeof(value1));
							/**
							 * Если возникает ошибка
							 */
							} catch(const exception &) {
								// Выполняем получение числа
								const uint64_t value1 = 0;
								// Пытаемся уменьшить число
								if(static_cast <uint64_t> (static_cast <uint32_t> (value1)) == value1){
									// Выполняем преобразование в unsigned int 32
									const uint32_t value2 = static_cast <uint32_t> (value1);
									// Выполняем добавление новой записи в буфер
									buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value2), reinterpret_cast <const char *> (&value2) + sizeof(value2));
								// Выполняем добавление новой записи в буфер
								} else buffer.insert(buffer.end(), reinterpret_cast <const char *> (&value1), reinterpret_cast <const char *> (&value1) + sizeof(value1));
							}
						// Если запись является текстом
						} else {
							// Если последний символ является пробелом, удаляем его
							if(::isspace(data.front().first.back()))
								// Выполняем удаление последний символ
								data.front().first.pop_back();
							// Выполняем добавление новой записи в буфер
							buffer.insert(buffer.end(), data.front().first.begin(), data.front().first.end());
						}
					}
					// Удаляем используемую запись
					data.pop();
				}
			}
		#endif
	}
}
/**
 * sysctl Метод установки настроек ядра операционной системы
 * @param name   название записи для установки настроек
 * @param buffer буфер бинарных данных записи для установки настроек
 * @param size   размер буфера данных
 * @return       результат выполнения установки
 */
bool awh::OS::sysctl(const string & name, const void * buffer, const size_t size) const noexcept {
	// Если название параметра передано
	if(!name.empty() && (buffer != nullptr) && (size > 0)){
		/**
		 * Если мы работаем в MacOS X, FreeBSD, NetBSD или OpenBSD
		 */
		#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Устанавливаем новые параметры настройки ядра
			return (::sysctlbyname(name.c_str(), nullptr, 0, const_cast <uint8_t *> (reinterpret_cast <const uint8_t *> (buffer)), size) == 0);
		/**
		 * Операционной системой является Linux
		 */
		#elif __linux__
			// Создаём комманду запуска
			string command = "sysctl -w";
			// Добавляем разделитель
			command.append(1, ' ');
			// Добавляем название параметра
			command.append(name);
			// Добавляем разделитель
			command.append(1, '=');
			// Добавляем параметр для установки
			command.append(reinterpret_cast <const char *> (buffer), size);
			// Выполняем установку параметров ядра
			return !this->exec(command, false).empty();
		#endif
	}
	// Сообщаем, что ничего не установленно
	return false;
}
/**
 * exec Метод запуска внешнего приложения
 * @param cmd       команда запуска
 * @param multiline данные должны вернутся многострочные
 */
string awh::OS::exec(const string & cmd, const bool multiline) const noexcept {
	// Полученный результат
	string result = "";
	// Если комманда запуска приложения передана правильно
	if(!cmd.empty()){
		/**
		 * Для операционной системы не являющейся OS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Создаем буфер для чтения результата
			char buffer[128];
			// Создаем пайп для чтения результата работы OS
			FILE * stream = ::popen(cmd.c_str(), "r");
			// Если пайп открыт
			if(stream != nullptr){
				// Считываем до тех пор пока все не прочитаем
				while(::fgets(buffer, sizeof(buffer), stream) != nullptr){
					// Добавляем полученный результат
					result.append(buffer);
					// Если это не мультилайн
					if(!multiline)
						// Выходим из цикла
						break;
				}
				// Закрываем пайп
				::pclose(stream);
			}
		/**
		 * Для операционной системы OS Windows
		 */
		#else
			// Создаем буфер для чтения результата
			wchar_t buffer[128];
			// Строка системной команды
			wstring command = L"";
			// Если используется BOOST
			#ifdef USE_BOOST_CONVERT
				// Объявляем конвертер
				using boost::locale::conv::utf_to_utf;
				// Выполняем конвертирование в utf-8 строку
				command = utf_to_utf <wchar_t> (cmd.c_str(), cmd.c_str() + cmd.size());
			// Если нужно использовать стандартную библиотеку
			#else
				// Объявляем конвертер
				// wstring_convert <codecvt_utf8 <wchar_t>> conv;
				wstring_convert <codecvt_utf8_utf16 <wchar_t, 0x10ffff, little_endian>> conv;
				// Выполняем конвертирование в utf-8 строку
				command = conv.from_bytes(cmd);
			#endif
			// Создаем пайп для чтения результата работы OS
			FILE * stream = _wpopen(command.c_str(), L"rt");
			// Если пайп открыт
			if(stream){
				// Считываем до тех пор пока все не прочитаем
				while(::fgetws(buffer, sizeof(buffer), stream) != nullptr){
					// Если используется BOOST
					#ifdef USE_BOOST_CONVERT
						// Выполняем конвертирование в utf-8 строку
						result.append(utf_to_utf <char> (buffer, buffer + strlen(buffer)));
					// Если нужно использовать стандартную библиотеку
					#else
						// Выполняем конвертирование в utf-8 строку
						result.append(conv.to_bytes(buffer));
					#endif
					// Если это не мультилайн
					if(!multiline)
						// Выходим из цикла
						break;
				}
				// Закрываем пайп
				_pclose(stream);
			}
		#endif
	}
	// Выводим результат
	return result;
}
