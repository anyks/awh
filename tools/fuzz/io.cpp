/**
 * @file io.cpp
 * @date 2026-08-26
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
 * @brief Инструмент фаззинга сетевого движка — подача движку случайных ПОСЛЕДОВАТЕЛЬНОСТЕЙ
 *        вызовов его договора: заведение узлов всех родов в любых сочетаниях семейства,
 *        типа и протокола, настройка их в произвольном порядке до и после фиксации,
 *        обхождение с ними вопреки состоянию (правка после фиксации, двойная фиксация,
 *        пауза непаузируемого, обращение к снесённому опознавателю) и обороты опроса
 *        между всем этим — для поиска аварийных завершений, выходов за границы буфера,
 *        утечек описателей и зависаний
 *
 * @details Ворошитель этот устроен ИНАЧЕ, чем ворошители кодеков и разбора адресов. Там
 *          поверяется разбор входного потока октетов, и порча наносится самому потоку.
 *          Здесь поверять нечего разбором: движок принимает не поток, а ПОРЯДОК ВЫЗОВОВ,
 *          и порча наносится именно порядку - вызов делается тогда, когда договор его не
 *          ждёт. Оттого и находки тут иного рода: не расхождение разбора, а падение либо
 *          зависание на сочетании, какого набор проверок не перебирает
 *
 * @note Живой движок ворошителю нужен, а надзорные права - нет. Всё, чего он касается,
 *       это петля 127.0.0.1, ::1, каналы и таймеры. Туннели, сырые сокеты и правка
 *       таблицы маршрутизации сюда не входят намеренно: без надзорных прав они отвечают
 *       отказом, и ворошить в них нечего
 *
 * @warning Отказы вызовов ворошителем НЕ считаются находкой. Договор движка возвращать
 *          отказ на вызов не ко времени - это и есть его правильное обхождение, ради
 *          него всё и заводится. Находкой считается лишь то, что отказом сообщить нельзя:
 *          аварийное завершение, порча памяти, зависание опроса, утечка описателей
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <random>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <chrono>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <sys/time.h>
/**
 * Если снятие стека вызовов доступно
 *
 * @note У musl заголовка `execinfo.h` нет вовсе, оттого включение условное
 */
#if defined(__has_include)
	#if __has_include(<execinfo.h>)
		#include <execinfo.h>
	#endif
#endif

/**
 * Срок одного оборота опроса В МИЛЛИСЕКУНДАХ. Оборот, не вернувший управление за него,
 * означает вставший движок: обычный оборот укладывается в миллисекунды, и запас кратный.
 * Мера взята мельче секунды намеренно - `alarm()` дробных сроков не принимает, а без них
 * обратный опыт над сторожем поставить нечем
 */
#define __AWH_FUZZ_POLL_LIMIT__ 30000
/**
 * Действующий срок одного оборота: умолчание выше, переопределяется переменной
 * окружения `AWH_FUZZ_POLL_LIMIT`
 *
 * @note Срок сделан настраиваемым не для удобства, а ради обратного опыта: сторож,
 *       ни разу не сработавший, от отсутствующего неотличим. Малым значением его
 *       заставляют сработать обычным прогоном, не правя код ради проверки
 */
static uint32_t __awh_poll_limit__ = __AWH_FUZZ_POLL_LIMIT__;

/**
 * Если операционной системой является не MS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	/**
	 * Если операционной системой является macOS либо BSD
	 */
	#if defined(__APPLE__) || defined(__MACH__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
		#include <sys/event.h>
		#include <sys/time.h>
	#endif
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/io.hpp>

/**
 * Подключаем пространство имён
 */
using namespace std;

/**
 * @brief Функция получения объекта фреймворка
 *
 * @note Объект заводится функционально-статическим намеренно, а не на уровне файла:
 *       порядок построения статических объектов между единицами трансляции не задан
 *
 * @return объект фреймворка
 *
 */
static const awh::fmk_t * framework() noexcept {
	// Объект фреймворка
	static awh::fmk_t result;
	// Выводим объект фреймворка
	return &result;
}
/**
 * @brief Функция получения объекта работы с логами
 *
 * @return объект работы с логами
 *
 */
static const awh::log_t * logger() noexcept {
	// Объект работы с логами
	static awh::log_t result(::framework());
	// Снимаем вывод журнала: ворошитель делает десятки тысяч заведомо отказных вызовов,
	// и журнал их обращает в гигабайты шума, за каким находки не видно
	const_cast <awh::log_t *> (&result)->level(awh::log_t::level_t::NONE);
	// Выводим объект работы с логами
	return &result;
}
/**
 * @brief Функция снятия случайного числа из промежутка
 *
 * @param engine источник случайных чисел
 * @param bound  верхняя граница промежутка, не включая её саму
 * @return       случайное число из промежутка
 *
 */
static uint64_t pick(mt19937_64 & engine, const uint64_t bound) noexcept {
	// Если граница вырождена, выбирать не из чего
	if(bound == 0)
		// Выводим ноль
		return 0;
	// Выводим случайное число из промежутка
	return (engine() % bound);
}
/**
 * @brief Функция снятия числа открытых описателей процесса
 *
 * @details Утечка описателей отказом не сообщается и падением не проявляется: она копится
 *          молча, пока процесс не упрётся в предел. Оттого число их снимается сличением
 *          до и после прохода
 *
 * @return число открытых описателей процесса
 *
 */
static size_t descriptors() noexcept;
/**
 * @brief Функция снятия перечня открытых описателей процесса
 *
 * @details Одного числа мало: узнав об утечке, надо ещё понять, ЧТО утекло. Перечень
 *          позволяет сличить снимки и назвать сами описатели, а по ним - их род
 *
 * @return перечень открытых описателей процесса
 *
 */
static vector <int32_t> descriptorList() noexcept;

/**
 * @brief Итоги работы ворошителя
 *
 */
static struct Totals {
	// Число сделанных проходов
	uint64_t rounds;
	// Число заведённых узлов
	uint64_t nodes;
	// Число узлов, каких завести не удалось
	uint64_t refused;
	// Число вызовов настройки
	uint64_t settings;
	// Число вызовов вопреки состоянию узла
	uint64_t misuses;
	// Число выполненных фиксаций
	uint64_t commits;
	// Число оборотов опроса
	uint64_t polls;
	// Число снесённых узлов
	uint64_t destroyed;
	// Число обращений к снесённым опознавателям
	uint64_t zombies;
	// Число узлов, снести какие движок отказался
	uint64_t undestroyed;
} totals = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/**
 * Роды узлов, какие ворошитель заводит
 *
 * @note Туннель и посредник исключены намеренно: первый требует надзорных прав, второй
 *       заводится движком сам и потребителем не создаётся
 */
static const awh::event::node_t NODES[] = {
	awh::event::node_t::NONE,
	awh::event::node_t::IPC,
	awh::event::node_t::DIR,
	awh::event::node_t::FILE,
	awh::event::node_t::PEER,
	awh::event::node_t::ORIGIN,
	awh::event::node_t::NOTIFY,
	awh::event::node_t::CLIENT,
	awh::event::node_t::SERVER,
	awh::event::node_t::TIMEOUT,
	awh::event::node_t::INTERVAL
};

/**
 * Семейства событий, какие ворошитель подаёт
 */
static const awh::event::family_t FAMILIES[] = {
	awh::event::family_t::NONE,
	awh::event::family_t::UDS,
	awh::event::family_t::PIPE,
	awh::event::family_t::FSYS,
	awh::event::family_t::USER,
	awh::event::family_t::IPV4,
	awh::event::family_t::IPV6,
	awh::event::family_t::TIMER
};

/**
 * Типы сокетов, какие ворошитель подаёт
 */
static const awh::event::type_t TYPES[] = {
	awh::event::type_t::NONE,
	awh::event::type_t::RAW,
	awh::event::type_t::STREAM,
	awh::event::type_t::DATAGRAM,
	awh::event::type_t::SEQPACKET
};

/**
 * Протоколы, какие ворошитель подаёт
 */
static const awh::event::protocol_t PROTOCOLS[] = {
	awh::event::protocol_t::NONE,
	awh::event::protocol_t::RAW,
	awh::event::protocol_t::UDP,
	awh::event::protocol_t::TCP,
	awh::event::protocol_t::ICMP,
	awh::event::protocol_t::IGMP,
	awh::event::protocol_t::SCTP,
	awh::event::protocol_t::QUIC,
	awh::event::protocol_t::FILE,
	awh::event::protocol_t::DIR
};

/**
 * @brief Годное сочетание рода узла, семейства, типа и протокола
 *
 */
static const struct Valid {
	// Род узла
	awh::event::node_t node;
	// Семейство событий
	awh::event::family_t family;
	// Тип сокета
	awh::event::type_t type;
	// Протокол
	awh::event::protocol_t protocol;
} VALIDS[] = {
	// Сервер TCP по IPv4 и по IPv6
	{awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP},
	{awh::event::node_t::SERVER, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP},
	// Сервер UDP по IPv4 и по IPv6
	{awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP},
	{awh::event::node_t::SERVER, awh::event::family_t::IPV6, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP},
	// Клиент TCP по IPv4 и по IPv6
	{awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP},
	{awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP},
	// Клиент UDP по IPv4
	{awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP},
	// Сервер и клиент через сокет местной области
	{awh::event::node_t::SERVER, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::NONE},
	{awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::NONE},
	// Таймаут и интервал времени
	{awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER, awh::event::type_t::NONE, awh::event::protocol_t::NONE},
	{awh::event::node_t::INTERVAL, awh::event::family_t::TIMER, awh::event::type_t::NONE, awh::event::protocol_t::NONE},
	// Узел уведомления
	{awh::event::node_t::NOTIFY, awh::event::family_t::USER, awh::event::type_t::NONE, awh::event::protocol_t::NONE}
};

/**
 * Записи адресов, какими ворошитель настраивает узлы
 *
 * @note Помимо правильных петлевых адресов поданы и заведомо негодные: договор обязан
 *       отвечать на них отказом, а не падением
 */
static const char * ADDRESSES[] = {
	"127.0.0.1",
	"::1",
	"0.0.0.0",
	"::",
	"localhost",
	"",
	" ",
	"256.256.256.256",
	":::::",
	"127.0.0.1:99999",
	"\x01\x02\x03",
	"127.0.0.1\x00\x7f"
};

/**
 * @brief Функция настройки заведённого узла случайными вызовами
 *
 * @details Вызовы делаются в СЛУЧАЙНОМ порядке и в случайном числе, в том числе такие,
 *          каких этому роду узла не полагается вовсе. Договор обязан отвечать отказом,
 *          а не падением
 *
 * @param io     объект сетевого движка
 * @param id     опознаватель узла
 * @param engine источник случайных чисел
 *
 */
static void settings(awh::engine::io_t & io, const awh::event::id_t id, mt19937_64 & engine) noexcept {
	// Число вызовов настройки на этом проходе
	const uint64_t count = (1 + ::pick(engine, 12));
	/**
	 * Выполняем случайные вызовы настройки
	 */
	for(uint64_t i = 0; i < count; i++){
		// Увеличиваем счёт вызовов настройки
		::totals.settings++;
		/**
		 * Определяем, какой вызов настройки сделать
		 */
		switch(static_cast <uint8_t> (::pick(engine, 12))){
			// Устанавливаем опции события
			case 0: static_cast <void> (io.setOptions(id, static_cast <uint16_t> (engine()))); break;
			// Устанавливаем отдельную опцию события
			case 1: static_cast <void> (io.setOption(id, static_cast <uint16_t> (engine()), (::pick(engine, 2) != 0))); break;
			// Устанавливаем порт привязки события
			case 2: static_cast <void> (io.setSourcePort(id, static_cast <uint16_t> (engine()))); break;
			// Устанавливаем порт назначения события
			case 3: static_cast <void> (io.setTargetPort(id, static_cast <uint16_t> (engine()))); break;
			// Устанавливаем адрес привязки события
			case 4: static_cast <void> (io.setAddress(id, awh::event::address_t::IPV4, ADDRESSES[::pick(engine, sizeof(ADDRESSES) / sizeof(ADDRESSES[0]))])); break;
			// Устанавливаем адрес назначения события
			case 5: static_cast <void> (io.setTarget(id, ADDRESSES[::pick(engine, sizeof(ADDRESSES) / sizeof(ADDRESSES[0]))])); break;
			// Устанавливаем предел числа подключений
			case 6: static_cast <void> (io.setMaxConnections(id, static_cast <uint32_t> (engine()))); break;
			// Устанавливаем наибольший размер передаваемого блока
			case 7: static_cast <void> (io.setMaximumTransmissionUnit(id, static_cast <uint32_t> (engine()))); break;
			// Устанавливаем сетевой интерфейс события
			case 8: static_cast <void> (io.setIface(id, ADDRESSES[::pick(engine, sizeof(ADDRESSES) / sizeof(ADDRESSES[0]))])); break;
			// Запрашиваем опции события
			case 9: static_cast <void> (io.getOptions(id)); break;
			// Запрашиваем порт привязки события
			case 10: static_cast <void> (io.getSourcePort(id)); break;
			// Запрашиваем адрес привязки события
			case 11: static_cast <void> (io.getAddress(id, awh::event::address_t::IPV4)); break;
		}
	}
}
/**
 * @brief Функция обхождения с узлом вопреки его состоянию
 *
 * @details Здесь и лежит суть ворошителя. Вызовы делаются ровно тогда, когда договор их
 *          не ждёт: пауза узла, какой не запускался; возобновление того, что не
 *          приостановлено; передача в узел, какой не подключён; повторная фиксация уже
 *          зафиксированного. Всё это договор обязан сообщать отказом
 *
 * @param io     объект сетевого движка
 * @param id     опознаватель узла
 * @param engine источник случайных чисел
 *
 */
static void misuse(awh::engine::io_t & io, const awh::event::id_t id, mt19937_64 & engine) noexcept {
	// Число вызовов вопреки состоянию на этом проходе
	const uint64_t count = (1 + ::pick(engine, 6));
	// Запись, какая подаётся на передачу
	static const char message[] = "ворошитель";
	/**
	 * Выполняем случайные вызовы вопреки состоянию узла
	 */
	for(uint64_t i = 0; i < count; i++){
		// Увеличиваем счёт вызовов вопреки состоянию
		::totals.misuses++;
		/**
		 * Определяем, какой вызов сделать
		 */
		switch(static_cast <uint8_t> (::pick(engine, 8))){
			// Приостанавливаем узел
			case 0: static_cast <void> (io.pause(id)); break;
			// Возобновляем узел
			case 1: static_cast <void> (io.resume(id)); break;
			// Запускаем узел
			case 2: static_cast <void> (io.launch(id)); break;
			// Разрываем связь узла
			case 3: static_cast <void> (io.disconnect(id)); break;
			// Переводим узел в прослушивание
			case 4: static_cast <void> (io.listen(id, static_cast <uint32_t> (::pick(engine, 64)))); break;
			// Выполняем передачу через узел
			case 5: static_cast <void> (io.send(id, message, sizeof(message) - 1)); break;
			// Выполняем повторную фиксацию узла
			case 6: {
				// Увеличиваем счёт фиксаций
				::totals.commits++;
				// Выполняем фиксацию настроек узла
				static_cast <void> (io.commit(id));
			} break;
			// Выполняем пересборку узла
			case 7: static_cast <void> (io.rebuild(id)); break;
		}
	}
}

/**
 * @brief Функция снятия числа открытых описателей процесса
 *
 * @return число открытых описателей процесса
 *
 */
static size_t descriptors() noexcept {
	// Результат работы функции
	size_t result = 0;
	/**
	 * Если операционной системой является не MS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		/**
		 * Перебираем описатели процесса
		 *
		 * @note Перебор ведётся до тысячи: предел мягкий у всех наших стендов не ниже,
		 *       а сличается тут не само число, а его прирост
		 */
		for(int32_t fd = 0; fd < 1024; fd++){
			// Сведения об описателе
			struct stat info{};
			// Описатель считается открытым лишь при двух согласных ответах
			if((::fcntl(fd, F_GETFD) != -1) && (::fstat(fd, &info) == 0))
				// Увеличиваем счёт открытых описателей
				result++;
		}
	#endif
	// Выводим число открытых описателей процесса
	return result;
}
/**
 * @brief Функция снятия перечня открытых описателей процесса
 *
 * @return перечень открытых описателей процесса
 *
 */
static vector <int32_t> descriptorList() noexcept {
	// Результат работы функции
	vector <int32_t> result;
	/**
	 * Если операционной системой является не MS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		/**
		 * Предел перебора описателей процесса
		 *
		 * @warning Прежде предел был 1024, и этого МАЛО. Движок поднимает мягкий предел
		 *          описателей процесса до 131072 при заведении (в журнале так и
		 *          сказано: «raised soft and hard FD limit to 131072»), и утёкший
		 *          описатель с номером выше прежнего предела прибор не увидел бы
		 *          ВОВСЕ - отчитавшись при этом чистым итогом. Ловушка та же, что
		 *          вскрылась у Андрея на переносе ворошителя под MS Windows: узкий
		 *          перебор прячет держателя ЗА собой, а прибор о том молчит
		 *
		 * @note Плата невелика: два обхода за прогон, по два обращения к ядру на номер.
		 *       Замерено - около четверти секунды на прогон, тогда как сам прогон идёт
		 *       минуты
		 */
		constexpr int32_t LIMIT = 262144;
		/**
		 * Перебираем описатели процесса
		 */
		for(int32_t fd = 0; fd < LIMIT; fd++){
			// Сведения об описателе
			struct stat info{};
			/**
			 * Описатель считается открытым лишь при ДВУХ согласных ответах
			 *
			 * @warning Одного fcntl мало, и это доказано пробным закрытием. Движок держит
			 *          свои потоки, описатели в нём заводятся и закрываются постоянно, и
			 *          между двумя опросами тот же номер успевает закрыться. Прибор,
			 *          доверявший одному fcntl, отчитывался утечкой семи описателей, у
			 *          каждого из которых и fstat, и getsockname, и само закрытие
			 *          отвечали «негодный описатель» - то есть открытыми они не были
			 *          вовсе. Находка та принадлежала прибору, а не движку
			 */
			if((::fcntl(fd, F_GETFD) != -1) && (::fstat(fd, &info) == 0))
				// Запоминаем открытый описатель
				result.push_back(fd);
		}
	#endif
	// Выводим перечень открытых описателей процесса
	return result;
}
/**
 * @brief Внешнее объявление щупа учёта заведений описателей
 *
 * @details Щуп собирается отдельным файлом и в сборку входит не всегда. Объявление
 *          слабое: без щупа имя разрешается нулём, и обращение к нему не делается
 *
 * @param fds   перечень описателей, признанных утёкшими
 * @param count число описателей в перечне
 *
 */
/**
 * @brief Тип тела щупа учёта заведений описателей
 *
 * @param fds   перечень описателей, признанных утёкшими
 * @param count число описателей в перечне
 *
 */
typedef void (* fdtrace_fn_t) (const int32_t * fds, const size_t count);
/**
 * @brief Тело щупа учёта заведений описателей, если он в сборку включён
 *
 */
static fdtrace_fn_t __awh_fdtrace__ = nullptr;
/**
 * @brief Метод заведения щупа учёта заведений описателей
 *
 * @details Щуп собирается отдельным файлом и в сборку входит не всегда. Зовёт этот
 *          метод он сам, из переменной процесса, ещё до входа в ворошитель
 *
 * @warning Устроено УКАЗАТЕЛЕМ, а не слабым объявлением, и это не прихоть. Признак
 *          слабости у систем разный: ELF понимает `weak`, а Mach-O ни `weak`, ни
 *          `weak_import` для имени, какого нет нигде, не разрешает - связывание
 *          валится «Undefined symbols», и щуп, задуманный необязательным,
 *          становится обязательным. Указатель же не требует от связывателя ничего:
 *          нет щупа - остаётся пустым
 *
 * @param fn тело щупа учёта заведений описателей
 *
 */
extern "C" void __awh_fdtrace_install__(fdtrace_fn_t fn) noexcept {
	// Запоминаем тело щупа учёта заведений описателей
	__awh_fdtrace__ = fn;
}

/**
 * @brief Функция вывода рода описателя
 *
 * @param fd описатель, род какого требуется назвать
 * @return   название рода описателя
 *
 */
static const char * descriptorKind([[maybe_unused]] const int32_t fd) noexcept {
	/**
	 * Если операционной системой является не MS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Сведения об описателе
		struct stat info{};
		/**
		 * Если сведения снять не удалось
		 *
		 * @note Это ещё не значит, что описатель негоден: у macOS и BSD `fstat` отвечает
		 *       отказом на очередь ядра `kqueue`, и такой описатель распознаётся не им, а
		 *       пробным обращением к самой очереди
		 */
		if(::fstat(fd, &info) != 0){
			/**
			 * Если операционной системой является macOS либо BSD
			 */
			#if defined(__APPLE__) || defined(__MACH__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
				// Срок ожидания очереди: обращение обязано вернуться немедленно
				struct timespec timeout{0, 0};
				// Если пробное обращение к очереди принято, описатель ею и является
				if(::kevent(fd, nullptr, 0, nullptr, 0, &timeout) == 0)
					// Сообщаем, что описатель является очередью ядра
					return "очередь ядра kqueue";
			#endif
			// Запоминаем причину отказа снятия сведений
			static char reason[1200];
			// Запоминаем причину отказа
			const int32_t code = errno;
			/**
			 * Если операционной системой является macOS, спрашиваем ещё и путь описателя
			 */
			#if defined(__APPLE__) || defined(__MACH__)
				// Путь, каким описатель заведён
				char path[1024];
				// Если путь описателя получить удалось
				if(::fcntl(fd, F_GETPATH, path) != -1){
					// Собираем род описателя с его путём
					::snprintf(reason, sizeof(reason), "неизвестен (fstat: %s, путь: %s)", ::strerror(code), path);
					// Выводим род описателя
					return reason;
				}
			#endif
			// Имя, каким описатель привязан
			struct sockaddr_storage name{};
			// Длина имени описателя
			socklen_t length = static_cast <socklen_t> (sizeof(name));
			// Если имя описателя снять удалось, описатель является сокетом
			if(::getsockname(fd, reinterpret_cast <struct sockaddr *> (&name), &length) == 0){
				// Собираем род описателя по семейству его имени
				::snprintf(reason, sizeof(reason), "сокет, семейство %d, порт %u (fstat: %s)",
				 static_cast <int32_t> (name.ss_family),
				 /**
				  * @note Обращение без «::» намеренно: у macOS ntohs заведён макросом, и
				  *       обращение к нему как к имени пространства имён сборку валит
				  */
				 static_cast <uint32_t> (ntohs(reinterpret_cast <struct sockaddr_in *> (&name)->sin_port)),
				 ::strerror(code));
				// Выводим род описателя
				return reason;
			}
			// Запоминаем причину отказа снятия имени
			const int32_t named = errno;
			/**
			 * Проверяем сам обнаружитель: годен ли описатель на деле
			 *
			 * @warning Сочетание «fcntl отвечает удачей, а fstat и getsockname - отказом
			 *          негодного описателя» для настоящего открытого описателя
			 *          невозможно. Значит либо лжёт обнаружитель, либо описатель имеет
			 *          природу, какой ни fstat, ни getsockname не понимают. Пробным
			 *          закрытием это и различается: закрылся - был открыт
			 */
			const int32_t closed = ::close(fd);
			// Собираем итог пробного закрытия
			static char verdict[64];
			// Записываем итог пробного закрытия
			::snprintf(verdict, sizeof(verdict), "%s", ((closed == 0) ? "был открыт, закрылся" : ::strerror(errno)));
			// Собираем причину отказа снятия сведений
			::snprintf(reason, sizeof(reason), "неизвестен (fstat: %s, getsockname: %s, пробное закрытие: %s)", ::strerror(code), ::strerror(named), verdict);
			// Сообщаем, что род описателя неизвестен
			return reason;
		}
		// Если описатель является сокетом
		if(S_ISSOCK(info.st_mode)){
			// Собираемое описание сокета
			static char socketKind[512];
			// Устройство, род и наречие сокета
			int32_t domain = -1, kind = -1, protocol = -1;
			// Длина снимаемой настройки сокета
			socklen_t length = static_cast <socklen_t> (sizeof(int32_t));
			/**
			 * Настройки SO_DOMAIN, SO_TYPE и SO_PROTOCOL заведены не всюду: у Linux они
			 * есть все три, у macOS и BSD - лишь часть, поэтому неснятые остаются -1
			 */
			#ifdef SO_DOMAIN
				// Выполняем снятие устройства сокета
				static_cast <void> (::getsockopt(fd, SOL_SOCKET, SO_DOMAIN, &domain, &length));
				// Восстанавливаем длину снимаемой настройки
				length = static_cast <socklen_t> (sizeof(int32_t));
			#endif
			#ifdef SO_TYPE
				// Выполняем снятие рода сокета
				static_cast <void> (::getsockopt(fd, SOL_SOCKET, SO_TYPE, &kind, &length));
				// Восстанавливаем длину снимаемой настройки
				length = static_cast <socklen_t> (sizeof(int32_t));
			#endif
			#ifdef SO_PROTOCOL
				// Выполняем снятие наречия сокета
				static_cast <void> (::getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &protocol, &length));
			#endif
			// Имя, каким сокет привязан
			struct sockaddr_storage name{};
			// Длина имени сокета
			socklen_t named = static_cast <socklen_t> (sizeof(name));
			// Порт, каким сокет привязан
			uint32_t port = 0;
			// Семейство имени сокета
			int32_t family = -1;
			// Если имя сокета снять удалось
			if(::getsockname(fd, reinterpret_cast <struct sockaddr *> (&name), &named) == 0){
				// Запоминаем семейство имени сокета
				family = static_cast <int32_t> (name.ss_family);
				/**
				 * @note Обращение без «::» намеренно: у macOS ntohs заведён макросом, и
				 *       обращение к нему как к имени пространства имён сборку валит
				 */
				port = static_cast <uint32_t> (ntohs(reinterpret_cast <struct sockaddr_in *> (&name)->sin_port));
			}
			// Собираем описание сокета
			::snprintf(socketKind, sizeof(socketKind), "сокет, устройство %d, род %d, наречие %d, семейство имени %d, порт %u",
			 domain, kind, protocol, family, port);
			// Выводим описание сокета
			return socketKind;
		}
		// Если описатель является каналом
		if(S_ISFIFO(info.st_mode))
			// Сообщаем, что описатель является каналом
			return "канал";
		// Если описатель является обычным файлом
		if(S_ISREG(info.st_mode))
			// Сообщаем, что описатель является файлом
			return "файл";
		// Если описатель является каталогом
		if(S_ISDIR(info.st_mode))
			// Сообщаем, что описатель является каталогом
			return "каталог";
		// Если описатель является символьным устройством
		if(S_ISCHR(info.st_mode))
			// Сообщаем, что описатель является устройством
			return "устройство";
	#endif
	// Сообщаем, что род описателя иной
	return "иной";
}
/**
 * @brief Функция успокоения движка
 *
 * @details Сносит всё, что ворошитель оставил живым, и крутит обороты опроса, пока снос
 *          не дойдёт до ядра. Только на успокоенном движке снимок описателей что-то
 *          значит: живой узел законно держит свой описатель, а снос у движка не
 *          мгновенный - узел переводится в состояние сноса, а описатель закрывается
 *          оборотом опроса
 *
 * @param io    объект сетевого движка
 * @param alive перечень узлов, ворошителем не снесённых
 *
 */
static void settle(awh::engine::io_t & io, vector <awh::event::id_t> & alive) noexcept {
	/**
	 * Перебираем узлы, ворошителем не снесённые
	 */
	for(auto & id : alive){
		// Если снос узла не выполнен, считаем его отказ
		if(!io.destroy(id))
			// Увеличиваем счёт отказов сноса
			::totals.undestroyed++;
	}
	// Очищаем перечень живых узлов
	alive.clear();
	/**
	 * Выполняем обороты опроса, чтобы снос дошёл до ядра
	 */
	for(uint8_t turn = 0; turn < 64; turn++)
		// Выполняем оборот опроса
		static_cast <void> (io.poll(1));
	/**
	 * Даём движку время закрыть описатели своими потоками
	 *
	 * @warning Выдержка тут не от лени, а по устройству, и это доказано пробным
	 *          закрытием: описатели, какие снимок заставал открытыми, спустя доли
	 *          мгновения отвечали «негодный описатель» и на fstat, и на getsockname, и
	 *          на само закрытие. То есть закрывал их движок сам, уже после оборотов
	 *          опроса, и снимок ловил их на полпути. Без выдержки прибор отчитывался
	 *          утечкой на исправном движке
	 */
	std::this_thread::sleep_for(std::chrono::milliseconds(300));
	/**
	 * Сносим всё, что осталось в списке узлов движка
	 *
	 * @note Путей свёртывания у движка ДВА, и они устроены по-разному: у `clear()` свой
	 *       разбор узлов, у `deinitialize()` свой. Щуп, ходящий одним из них, мерит не
	 *       тот путь. Здесь зовётся `clear()` намеренно: снимок описателей снимается до
	 *       `deinitialize()`, и без этого вызова второй путь остался бы непроверенным
	 */
	io.clear();
	/**
	 * Крутим обороты опроса ещё раз: часть закрытий движок делает ими
	 */
	for(uint8_t turn = 0; turn < 64; turn++)
		// Выполняем оборот опроса
		static_cast <void> (io.poll(1));
}
/**
 * @brief Обработчик сторожевого сигнала одного оборота опроса
 *
 * @details Печатает находку и выходит с кодом ошибки: движок не вернул управление
 *          из одного оборота за отведённый срок
 *
 * @param signal номер полученного сигнала
 *
 */
static void __awh_poll_watchdog__(int signal) noexcept {
	// Отмечаем полученный сигнал использованным
	static_cast <void> (signal);
	// Сообщаем о находке в поток ошибок
	static const char message[] =
		"\nНАХОДКА: движок не вернул управление из одного оборота опроса "
		"за отведённый срок - опрос встал\n";
	// Пишем сообщение напрямую: печать через stdio в обработчике сигнала недопустима
	static_cast <void> (::write(STDERR_FILENO, message, sizeof(message) - 1));
	/**
	 * Если снятие стека вызовов доступно
	 *
	 * @note Стек тут не роскошь: без него находка сообщает, ЧТО движок встал, но не
	 *       ГДЕ. Замер 30.08.2026 стоил трёх зависших процессов и снятия образцов
	 *       вручную через `sample`, причём поймать живой случай удалось лишь потому,
	 *       что процессы простояли трое суток. `backtrace_symbols_fd` выбран
	 *       намеренно: он не выделяет памяти и в обработчике сигнала допустим,
	 *       в отличие от `backtrace_symbols`
	 */
	/**
	 * @warning Снятие стека РАБОТАЕТ НЕ ВЕЗДЕ, и пустота вывода тут не означает,
	 *          что показывать нечего. Проверено срывом сторожа 31.08.2026:
	 *
	 *          macOS, FreeBSD - стек полный и полезный, разворачивается сквозь
	 *              переходник сигнала, видны и «IO::poll», и сам вызов ядра;
	 *          NetBSD - ДВА кадра: сам обработчик и «__sigtramp_siginfo_2».
	 *              Дальше разворот не идёт, и места зависания по нему не узнать;
	 *          OpenBSD - НОЛЬ кадров, печатается одно лишь сообщение находки.
	 *              «-fno-omit-frame-pointer» положения не меняет: «backtrace» из
	 *              обработчика сигнала там отдаёт нуль. Свойство libexecinfo,
	 *              а не наша недоработка.
	 *
	 *          У NetBSD и OpenBSD «backtrace» лежит в отдельной libexecinfo, а не
	 *          в libc, и сборке нужен «-lexecinfo» (вписан в build.sh по системам).
	 *
	 *          У MinGW «backtrace» нет вовсе, и включение по «__has_include» тут
	 *          не сработает - но это НЕ значит, что стека под Windows не снять:
	 *          он снимается родным средством системы, «CaptureStackBackTrace» из
	 *          dbghelp, а имена берутся через «SymFromAddr». Замерено на стенде
	 *          31.08.2026: связывание с «-ldbghelp» проходит, снимается 4 кадра.
	 *          Отсутствие ИМЕНИ в чужой системе не означает отсутствия того, что
	 *          за ним стоит - судить надо по возможности, а не по совпадению имён
	 *          Заголовок «execinfo.h» там на месте, поэтому включение по
	 *          «__has_include» срабатывает, сборка проходит целиком и валится лишь
	 *          на связывании - «слинковался» и «снимает стек» тут разные утверждения
	 */
	#if defined(__has_include)
		#if __has_include(<execinfo.h>)
			{
				// Массив адресов возврата
				void * frames[64];
				// Число снятых уровней стека
				const int32_t count = ::backtrace(frames, 64);
				// Выполняем печать стека напрямую в поток ошибок
				::backtrace_symbols_fd(frames, count, STDERR_FILENO);
			}
		#endif
	#endif
	// Выходим с кодом ошибки
	::_exit(EXIT_FAILURE);
}
/**
 * @brief Функция взведения сторожевого срока одного оборота опроса
 *
 * @param ms срок в миллисекундах, нуль снимает сторож
 *
 */
static void __awh_poll_guard__(const uint32_t ms) noexcept {
	// Объект настройки таймера
	struct itimerval timer;
	// Обнуляем повтор: сторож одноразовый на каждый оборот
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	// Устанавливаем целые секунды срока
	timer.it_value.tv_sec = static_cast <time_t> (ms / 1000);
	// Устанавливаем остаток срока в микросекундах
	timer.it_value.tv_usec = static_cast <suseconds_t> ((ms % 1000) * 1000);
	// Выполняем взведение таймера
	static_cast <void> (::setitimer(ITIMER_REAL, &timer, nullptr));
}
/**
 * @brief Функция выполнения оборотов опроса
 *
 * @details Обороты обязаны быть СРОЧНЫМИ: движок, вставший навсегда, от медленного
 *          неотличим, и ворошитель без срока просто замолкает, ничего не сообщив
 *
 * @warning Срок в условии цикла эту цель НЕ достигает: он сличается МЕЖДУ оборотами,
 *          а движок встаёт ВНУТРИ одного `poll()`. Замер: ворошитель простоял трое
 *          суток в блокирующем `::connect` на пути переподключения по таймеру, не
 *          напечатав ни строки. Оттого срок здесь сторожевой и снаружи оборота -
 *          сигналом, единственным средством прервать вызов, уже ушедший в ядро
 *
 * @param io     объект сетевого движка
 * @param engine источник случайных чисел
 *
 */
static void turns(awh::engine::io_t & io, mt19937_64 & engine) noexcept {
	// Число оборотов опроса на этом проходе
	const uint64_t count = (1 + ::pick(engine, 4));
	// Срок, за который обороты обязаны уложиться
	const auto deadline = (std::chrono::steady_clock::now() + std::chrono::seconds(5));
	/**
	 * Выполняем обороты опроса
	 */
	for(uint64_t i = 0; (i < count) && (std::chrono::steady_clock::now() < deadline); i++){
		// Увеличиваем счёт оборотов опроса
		::totals.polls++;
		/**
		 * Если операционной системой является не MS Windows
		 *
		 * @note У MS Windows нет ни `SIGALRM`, ни таймеров этого вида, и прервать вызов, ушедший
		 *       в ядро, тем же способом нечем: там вставший оборот придётся ловить
		 *       снаружи - присмотром за процессорным временем процесса
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Взводим сторожевой сигнал на один оборот опроса
			__awh_poll_guard__(__awh_poll_limit__);
		#endif
		// Выполняем оборот опроса
		static_cast <void> (io.poll(static_cast <int32_t> (::pick(engine, 3))));
		/**
		 * Если операционной системой является не MS Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Снимаем сторожевой сигнал: оборот вернул управление
			__awh_poll_guard__(0);
		#endif
	}
}
/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Количество проходов ворошителя
	const uint64_t count = ((argc > 1) ? static_cast <uint64_t> (::atoll(argv[1])) : 3000);
	// Зерно источника случайных чисел
	const uint64_t seed = ((argc > 2) ? static_cast <uint64_t> (::atoll(argv[2])) : 20260826);
	// Источник случайных чисел
	mt19937_64 engine(seed);
	/**
	 * Если операционной системой является не MS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Значение срока оборота, заданное окружением
		const char * limit = ::getenv("AWH_FUZZ_POLL_LIMIT");
		// Если срок оборота задан окружением
		if(limit != nullptr){
			// Полученное значение срока
			const int32_t value = ::atoi(limit);
			// Если значение срока положительное
			if(value > 0)
				// Запоминаем заданный окружением срок
				__awh_poll_limit__ = static_cast <uint32_t> (value);
		}
		// Устанавливаем обработчик сторожевого сигнала оборота опроса
		::signal(SIGALRM, __awh_poll_watchdog__);
	#endif
	// Объект сетевого движка
	awh::engine::io_t io(::framework(), ::logger());
	// Выполняем заведение сетевого движка
	if(!io.initialize()){
		// Сообщаем, что движок завести не удалось
		::fprintf(stderr, "Сетевой движок завести не удалось\n");
		// Выходим из приложения с кодом ошибки
		return EXIT_FAILURE;
	}
	/**
	 * Перечень открытых описателей, снятый ПОСЛЕ разогрева
	 *
	 * @warning Снимать его сразу после заведения движка НЕЛЬЗЯ, и это замерено: движок
	 *          заводит часть своих описателей ЛЕНИВО, первым настоящим употреблением, а
	 *          не заведением. Снимок до разогрева давал прирост +2 описателя, и прибор
	 *          отчитывался утечкой на исправном движке. Опорой служил прогон с нулём
	 *          проходов и ряд 5-10-20-50-100-200: прирост вставал на +2 и с числом
	 *          проходов НЕ РОС, а настоящая утечка растёт вместе с ними. Оттого снимок
	 *          снимается после разогрева, а сличается прирост, зависящий от числа проходов
	 */
	vector <int32_t> opened;
	// Число открытых описателей после разогрева
	size_t before = 0;
	// Число проходов разогрева
	const uint64_t warmup = ((count / 10) + 1);
	// Перечень опознавателей, уже снесённых ворошителем
	vector <awh::event::id_t> zombies;
	/**
	 * Перечень узлов, ворошителем не снесённых
	 *
	 * @warning Вести его обязательно. Ворошитель сносит узел НЕ ВСЕГДА - оставленный
	 *          в живых узел тоже надо поворошить, - и живой узел законно держит свой
	 *          описатель. Без этого перечня сличение описателей отчиталось находкой на
	 *          первом же прогоне: 123 узла заведено, 107 снесено, и десять описателей
	 *          шестнадцати живых узлов вышли «утечкой». Находка та принадлежала самому
	 *          ворошителю, а не движку
	 */
	vector <awh::event::id_t> alive;
	/**
	 * Выполняем проходы ворошителя
	 */
	for(uint64_t round = 0; round < count; round++){
		// Увеличиваем счёт проходов
		::totals.rounds++;
		/**
		 * Если разогрев окончен, снимаем опорный перечень описателей
		 */
		if(round == warmup){
			// Успокаиваем движок, чтобы снимок что-то значил
			::settle(io, alive);
			// Снимаем перечень открытых описателей после разогрева
			opened = ::descriptorList();
			// Снимаем число открытых описателей после разогрева
			before = opened.size();
		}
		// Опознаватель заведённого узла
		awh::event::id_t id = 0;
		/**
		 * Половина проходов идёт ГОДНЫМИ сочетаниями, половина - случайными
		 *
		 * @warning Одними случайными сочетаниями ворошить нельзя, и это замерено: из
		 *          двухсот проходов случайными сочетаниями заводилось лишь 25 узлов,
		 *          а 175 отвергались ещё на заведении. Ворошитель при этом отчитывался
		 *          успехом, ни разу не дойдя до того, ради чего заводился, - до
		 *          обхождения с ЖИВЫМ узлом. Годные сочетания подмешиваются, чтобы
		 *          вызовы вопреки состоянию доставались настоящим узлам, а не пустоте
		 */
		if(::pick(engine, 2) != 0){
			// Годное сочетание рода узла, семейства, типа и протокола
			const size_t index = static_cast <size_t> (::pick(engine, sizeof(VALIDS) / sizeof(VALIDS[0])));
			// Заводим узел годным сочетанием
			id = io.event(VALIDS[index].node, VALIDS[index].family, VALIDS[index].type, VALIDS[index].protocol);
		// Заводим узел случайного рода в случайном сочетании
		} else id = io.event(
			NODES[::pick(engine, sizeof(NODES) / sizeof(NODES[0]))],
			FAMILIES[::pick(engine, sizeof(FAMILIES) / sizeof(FAMILIES[0]))],
			TYPES[::pick(engine, sizeof(TYPES) / sizeof(TYPES[0]))],
			PROTOCOLS[::pick(engine, sizeof(PROTOCOLS) / sizeof(PROTOCOLS[0]))]);
		/**
		 * Если узел завести не удалось
		 *
		 * @note Это НЕ находка: сочетания подаются вперемешку, и негодных среди них
		 *       больше, чем годных. Отказ тут и есть правильное обхождение
		 */
		if(id == 0){
			// Увеличиваем счёт узлов, каких завести не удалось
			::totals.refused++;
			/**
			 * Обращаемся к снесённому опознавателю: движок обязан ответить отказом,
			 * а не тронуть освобождённую память
			 */
			if(!zombies.empty()){
				// Увеличиваем счёт обращений к снесённым опознавателям
				::totals.zombies++;
				// Выполняем обхождение со снесённым узлом
				::misuse(io, zombies[::pick(engine, zombies.size())], engine);
			}
			// Переходим к следующему проходу
			continue;
		}
		// Увеличиваем счёт заведённых узлов
		::totals.nodes++;
		// Запоминаем узел живым
		alive.push_back(id);
		/**
		 * Настраиваем узел ДО фиксации
		 */
		::settings(io, id, engine);
		/**
		 * Обходимся с узлом вопреки состоянию ещё до фиксации
		 *
		 * @note Порядок этот намеренно неправильный: договор обязан отвергнуть запуск
		 *       и передачу у незафиксированного узла
		 */
		if(::pick(engine, 2) != 0)
			// Выполняем обхождение вопреки состоянию
			::misuse(io, id, engine);
		/**
		 * Фиксируем настройки узла
		 */
		if(::pick(engine, 4) != 0){
			// Увеличиваем счёт фиксаций
			::totals.commits++;
			// Выполняем фиксацию настроек узла
			static_cast <void> (io.commit(id));
		}
		/**
		 * Настраиваем узел ПОСЛЕ фиксации
		 *
		 * @note Правка после фиксации - отдельный род обхождения: часть настроек уходит
		 *       ядру при создании сокета, и договор обязан либо принять правку, либо
		 *       отвергнуть её, но не остаться в противоречии с ядром
		 */
		if(::pick(engine, 2) != 0)
			// Выполняем настройку узла
			::settings(io, id, engine);
		// Выполняем обхождение вопреки состоянию
		::misuse(io, id, engine);
		// Выполняем обороты опроса
		::turns(io, engine);
		/**
		 * Сносим узел
		 */
		if(::pick(engine, 8) != 0){
			// Увеличиваем счёт снесённых узлов
			::totals.destroyed++;
			// Выполняем снос узла
			static_cast <void> (io.destroy(id));
			// Убираем узел из перечня живых
			alive.pop_back();
			// Запоминаем опознаватель снесённого узла
			zombies.push_back(id);
			/**
			 * Держим перечень снесённых опознавателей в узде
			 *
			 * @note Расти ему без предела нельзя: ворошитель гоняется десятками тысяч
			 *       проходов, и перечень съел бы память быстрее, чем нашёл дефект
			 */
			if(zombies.size() > 64)
				// Убираем самый старый опознаватель
				zombies.erase(zombies.begin());
		}
		/**
		 * Обращаемся к снесённому опознавателю
		 */
		if(!zombies.empty() && (::pick(engine, 3) == 0)){
			// Увеличиваем счёт обращений к снесённым опознавателям
			::totals.zombies++;
			// Выполняем обхождение со снесённым узлом
			::misuse(io, zombies[::pick(engine, zombies.size())], engine);
		}
	}
	/**
	 * Сносим узлы, ворошителем не снесённые
	 *
	 * @note Делается это ДО снимка описателей: живой узел законно держит свой описатель,
	 *       и сличать надо то, что осталось ПОСЛЕ честного сноса всего заведённого
	 */
	// Число узлов, оставленных ворошителем живыми
	const size_t left = alive.size();
	// Успокаиваем движок тем же порядком, что и перед опорным снимком
	::settle(io, alive);
	/**
	 * Снимаем число открытых описателей ДО снятия движка
	 *
	 * @warning Порядок обязателен: снятие движка закрывает описатели узлов, какие
	 *          ворошитель оставил незакрытыми, и утечка, случись она, оказалась бы
	 *          прибрана прежде, чем её сличили. Снимок после снятия показал бы
	 *          благополучие всегда
	 */
	const vector <int32_t> remained = ::descriptorList();
	// Снимаем число открытых описателей после работы
	const size_t after = remained.size();
	// Признак того, что находка сделана
	bool leaked = false;
	/**
	 * Разбираем утечку описателей ДО снятия движка
	 *
	 * @warning Порядок обязателен, и он стоил мне часа розыска. Отчёт стоял ПОСЛЕ
	 *          deinitialize(), а тот закрывает всё, чем движок владел. Опрос утёкших
	 *          описателей отвечал тогда «негодный описатель» и на fstat, и на
	 *          getsockname, и на пробное закрытие - и я едва не признал находку
	 *          ложной, а виноват был порядок вызовов в самом приборе
	 */
	/**
	 * Если описателей после работы стало больше, чем до неё
	 *
	 * @warning Утечка описателей отказом не сообщается и падением не проявляется: она
	 *          копится молча, пока процесс не упрётся в предел. Оттого она и проверяется
	 *          здесь отдельно, а не полагается на аварийное завершение
	 */
	/**
	 * Если разогрев не состоялся, сличать не с чем
	 *
	 * @note Опорный снимок снимается на проходе разогрева, а при малом числе проходов
	 *       того прохода не случается вовсе, и опора остаётся нулевой. Сличение с нулём
	 *       объявляло бы утечкой каждый описатель процесса, включая его же потоки вывода
	 */
	if((::totals.rounds > warmup) && (after > before)){
		// Сообщаем об утечке описателей
		::fprintf(stderr, "НАХОДКА: описатели утекли, после разогрева было %zu, в конце %zu\n", before, after);
		// Перечень утёкших описателей для щупа учёта заведений
		vector <int32_t> traced;
		/**
		 * Перебираем описатели, оставшиеся после работы
		 */
		for(auto & fd : remained){
			// Признак того, что описатель был открыт и до работы
			bool known = false;
			/**
			 * Перебираем описатели, открытые до работы
			 */
			for(auto & item : opened){
				// Если описатель был открыт и до работы
				if(item == fd){
					// Запоминаем, что описатель не нов
					known = true;
					// Прекращаем перебор
					break;
				}
			}
			// Если описатель появился за время работы, называем его
			if(!known){
				// Выводим описатель и его род
				::fprintf(stderr, "  утёк описатель %d, род: %s\n", fd, ::descriptorKind(fd));
				// Запоминаем описатель для щупа учёта заведений
				traced.push_back(fd);
			}
		}
		/**
		 * Если щуп учёта заведений описателей в сборку включён, спрашиваем у него
		 * места заведения утёкших описателей
		 *
		 * @note Объявление слабое: без щупа тела у него нет, и обращение не делается
		 */
		if(::__awh_fdtrace__ != nullptr)
			// Выводим места заведения утёкших описателей
			::__awh_fdtrace__(traced.data(), traced.size());
		// Запоминаем, что находка сделана: выйти отказом надо ПОСЛЕ вывода итогов
		leaked = true;
	}

	// Выполняем снятие сетевого движка
	static_cast <void> (io.deinitialize());
	// Выводим итоги проделанной работы
	::fprintf(stdout,
		"ЗЕРНО=%llu ПРОХОДОВ=%llu\n"
		"  узлов заведено: %llu, отвергнуто сочетаний: %llu\n"
		"  вызовов настройки: %llu\n"
		"  вызовов вопреки состоянию: %llu\n"
		"  фиксаций: %llu\n"
		"  оборотов опроса: %llu\n"
		"  узлов снесено: %llu\n"
		"  обращений к снесённым опознавателям: %llu\n"
		"  узлов оставлено живыми и снесено в конце: %zu, из них снести отказано: %llu\n"
		"  описателей после разогрева (%llu проходов): %zu, в конце: %zu\n",
		static_cast <unsigned long long> (seed), static_cast <unsigned long long> (::totals.rounds),
		static_cast <unsigned long long> (::totals.nodes), static_cast <unsigned long long> (::totals.refused),
		static_cast <unsigned long long> (::totals.settings), static_cast <unsigned long long> (::totals.misuses),
		static_cast <unsigned long long> (::totals.commits), static_cast <unsigned long long> (::totals.polls),
		static_cast <unsigned long long> (::totals.destroyed), static_cast <unsigned long long> (::totals.zombies),
		left, static_cast <unsigned long long> (::totals.undestroyed),
		static_cast <unsigned long long> (warmup), before, after);
	// Если описатели утекли, выходим отказом
	if(leaked)
		// Выводим код ошибки выхода из приложения
		return EXIT_FAILURE;
	// Выводим успешный код выхода из приложения
	return EXIT_SUCCESS;
}
