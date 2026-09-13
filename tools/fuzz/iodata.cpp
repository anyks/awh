/**
 * @file iodata.cpp
 * @date 2026-09-13
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
 * @brief Инструмент фаззинга сетевого движка по ДАННЫМ — подача узлу движка произвольно
 *        нарезанного потока октет со встречной стороны: рваные границы, нулевая длина,
 *        куски по одному октету, сообщение больше буфера, обрыв посреди сообщения
 *
 * @details Ворошитель этот дополняет `io.cpp`, а не повторяет его. Там ворошится ПОРЯДОК
 *          вызовов договора - вызов делается тогда, когда договор его не ждёт. Здесь
 *          ворошится второй вход движка: поток октет, приходящий из сети. Порядок вызовов
 *          при этом правильный, а негодной делается САМА ПОДАЧА.
 *
 * @note Находка здесь не только падение и зависание. У потока октет есть проверяемое
 *       обещание: движок обязан отдать потребителю РОВНО то, что пришло, и в том же
 *       порядке. Оттого каждый круг сличает число отданных октет и их свёртку с тем,
 *       что было послано. Расхождение - находка, и находка настоящая: договор нарушен
 *
 * @note У видов, хранящих границы сообщений (дейтаграммы, домен UNIX вида SEQPACKET),
 *       проверяется ещё и ЧИСЛО откликов: одна посылка обязана дойти одним откликом
 *
 * @warning Отказы вызовов находкой НЕ считаются: движок вправе отвечать отказом на
 *          негодную подачу, ради того он отказы и возвращает
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <set>

/**
 * Модули операционной системы
 */
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <sys/socket.h>
#include <sys/un.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/io.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace std;
using namespace awh;

/**
 * @brief Итоги работы ворошителя
 *
 */
static struct Totals {
	// Число сделанных кругов
	uint64_t rounds;
	// Число кругов, поставить какие не удалось
	uint64_t skipped;
	// Число посланных встречной стороной сообщений
	uint64_t messages;
	// Число посланных октет
	uint64_t written;
	// Число отданных движком октет
	uint64_t delivered;
	// Число откликов чтения
	uint64_t reads;
	// Число кругов с расхождением объёма
	uint64_t mismatched;
	// Число кругов с расхождением содержимого
	uint64_t corrupted;
	// Число кругов с потерянными границами сообщений
	uint64_t unbounded;
	// Число оборотов опроса
	uint64_t polls;
} totals = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/**
 * @brief Срок одного оборота опроса, за какой движок обязан вернуть управление
 *
 */
static uint32_t __awh_poll_limit__ = 30;

/**
 * @brief Обработчик сторожевого сигнала оборота опроса
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
	// Завершаем работу ворошителя: зависший опрос продолжать бессмысленно
	::_exit(EXIT_FAILURE);
}
/**
 * @brief Функция снятия перечня открытых описателей процесса
 *
 * @return перечень открытых описателей процесса
 *
 */
static set <int32_t> descriptors() noexcept {
	// Результат работы функции
	set <int32_t> result;
	/**
	 * Перебираем описатели процесса
	 */
	for(int32_t fd = 0; fd < 1024; fd++){
		// Если описатель открыт, запоминаем его
		if(::fcntl(fd, F_GETFD) != -1)
			// Запоминаем открытый описатель
			result.emplace(fd);
	}
	// Выводим перечень открытых описателей
	return result;
}
/**
 * @brief Функция подсчёта свёртки поданных октет
 *
 * @param sum    накопленная свёртка
 * @param buffer поданные октеты
 * @param size   размер поданных октет
 *
 * @return обновлённая свёртка
 *
 * @note Свёртка берётся зависящей от ПОРЯДКА октет: сумма их порядка не различает, а
 *       переставленные местами куски - ровно та беда, ради какой ворошитель и заводится
 *
 */
static uint64_t digest(const uint64_t sum, const uint8_t * buffer, const size_t size) noexcept {
	// Накопленная свёртка
	uint64_t result = sum;
	/**
	 * Перебираем поданные октеты
	 */
	for(size_t i = 0; i < size; i++)
		// Домешиваем октет к свёртке
		result = ((result * 1099511628211ULL) ^ static_cast <uint64_t> (buffer[i]));
	// Выводим обновлённую свёртку
	return result;
}
/**
 * @brief Функция получения пути узла домена UNIX
 *
 * @return путь узла домена UNIX
 *
 */
static string path() noexcept {
	// Собираем путь из опознавателя процесса: два ворошителя разом друг другу не мешают
	return ("/tmp/awh_iodata_" + to_string(static_cast <uint64_t> (::getpid())) + ".sock");
}
/**
 * @brief Функция подачи октет встречной стороной
 *
 * @details Сокет встречной стороны неблокирующий, и отказ занятости (`EAGAIN`) означает,
 *          что буфер полон: разбирает его движок, и разбор идёт из ЭТОГО ЖЕ потока. Оттого
 *          отказ занятости снимается оборотом опроса, а не ожиданием.
 *
 * @param io     объект сетевого движка
 * @param peer   сокет встречной стороны
 * @param buffer подаваемые октеты
 * @param size   размер подаваемых октет
 *
 * @return число поданных октет либо -1
 *
 */
static ssize_t deliver(awh::engine::io_t & io, const int32_t peer, const uint8_t * buffer, const size_t size) noexcept {
	/**
	 * Пробуем подать октеты, разбирая накопленное при отказе занятости
	 */
	for(uint16_t attempt = 0; attempt < 512; attempt++){
		// Выполняем подачу октет встречной стороной
		const ssize_t result = ::send(peer, buffer, size, 0);
		// Если октеты поданы, выводим число поданных
		if(result >= 0) return result;
		// Если отказ не связан с занятостью буфера, выводим отказ
		if((errno != EAGAIN) && (errno != EWOULDBLOCK)) return -1;
		// Взводим сторожевой срок оборота опроса
		::alarm(__awh_poll_limit__);
		// Выполняем оборот цикла событий, разбирая накопленное
		static_cast <void> (io.poll(1));
		// Снимаем сторожевой срок
		::alarm(0);
		// Считаем оборот опроса
		::totals.polls++;
	}
	// Подать октеты не удалось
	return -1;
}
/**
 * @brief Главная функция приложения
 *
 * @param argc количество полученных аргументов
 * @param argv полученные аргументы
 *
 * @return код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	awh::fmk::initialize();
	// Количество кругов ворошителя
	const uint64_t count = ((argc > 1) ? static_cast <uint64_t> (::atoll(argv[1])) : 500);
	// Зерно источника случайных чисел
	const uint64_t seed = ((argc > 2) ? static_cast <uint64_t> (::atoll(argv[2])) : 20260913);
	// Источник случайных чисел
	mt19937_64 random(seed);
	// Значение срока оборота, заданное окружением
	const char * limit = ::getenv("AWH_FUZZ_POLL_LIMIT");
	// Если срок оборота задан окружением
	if(limit != nullptr){
		// Полученное значение срока
		const int32_t value = ::atoi(limit);
		// Если значение срока положительное, запоминаем его
		if(value > 0)
			// Запоминаем заданный окружением срок
			__awh_poll_limit__ = static_cast <uint32_t> (value);
	}
	// Устанавливаем обработчик сторожевого сигнала оборота опроса
	::signal(SIGALRM, __awh_poll_watchdog__);
	// Снимаем обработчик обрыва канала: обрыв посреди сообщения ворошитель делает нарочно
	::signal(SIGPIPE, SIG_IGN);
	// Снимаем перечень описателей до работы ворошителя
	const set <int32_t> before = ::descriptors();
	// Объект сетевого движка
	awh::engine::io_t io;
	// Выполняем заведение сетевого движка
	if(!io.initialize()){
		// Сообщаем, что движок завести не удалось
		::fprintf(stderr, "Сетевой движок завести не удалось\n");
		// Выходим из приложения с кодом ошибки
		return EXIT_FAILURE;
	}
	// Путь, каким встречаются узел движка и встречная сторона
	const string uds = ::path();
	/**
	 * Крутим круги ворошителя
	 */
	for(uint64_t round = 0; round < count; round++){
		// Считаем круг
		::totals.rounds++;
		// Убираем путь, оставшийся от прошлого круга
		::unlink(uds.c_str());
		/**
		 * Вид узла круга: поток либо сообщения
		 *
		 * @note Домен UNIX несёт оба вида, и оба ворошатся одним ходом: разнится лишь то,
		 *       обязан ли движок держать границы сообщений
		 */
		const bool stream = ((random() % 2) == 0);
		// Заводим событие сервера домена UNIX
		const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::UDS,
		 (stream ? awh::event::type_t::STREAM : awh::event::type_t::DATAGRAM));
		// Если событие завести не удалось, круг пропускается
		if(server == 0){
			// Считаем круг пропущенным
			::totals.skipped++;
			// Переходим к следующему кругу
			continue;
		}
		// Число откликов чтения круга
		uint64_t reads = 0;
		// Число отданных движком октет круга
		uint64_t delivered = 0;
		// Свёртка отданных движком октет круга
		uint64_t taken = 1469598103934665603ULL;
		// Размеры откликов чтения круга
		vector <size_t> parts;
		/**
		 * @brief Функция приёма данных узлом движка
		 *
		 * @param buffer принятые данные
		 * @param size   размер принятых данных
		 *
		 */
		auto receive = [&](const uint8_t * buffer, const size_t size) noexcept -> void {
			// Считаем отклик чтения
			reads++;
			// Считаем отданные октеты
			delivered += size;
			// Домешиваем принятое к свёртке
			taken = ::digest(taken, buffer, size);
			// Запоминаем размер отклика
			parts.push_back(size);
		};
		// Устанавливаем функцию обратного вызова на принятие подключения
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &receive]([[maybe_unused]] const awh::event::id_t sid, const awh::event::id_t pid) noexcept -> void {
			// Выставляем принятому узлу неблокирующий обмен
			static_cast <void> (io.setOptions(pid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK));
			// Устанавливаем функцию обратного вызова на чтение данных принятым узлом
			io.on(pid, static_cast <awh::engine::callback::read_t> ([&receive]([[maybe_unused]] const awh::event::id_t eid, const uint8_t * buffer, const size_t size) noexcept -> void {
				// Принимаем данные принятым узлом
				receive(buffer, size);
			}));
		}));
		// Устанавливаем функцию обратного вызова на чтение данных сервером
		io.on(server, static_cast <awh::engine::callback::read_t> ([&receive]([[maybe_unused]] const awh::event::id_t eid, const uint8_t * buffer, const size_t size) noexcept -> void {
			// Принимаем данные сервером
			receive(buffer, size);
		}));
		// Выставляем узлу сервера путь и настройки
		static_cast <void> (io.setAddress(server, awh::event::address_t::UDS, uds));
		static_cast <void> (io.setOptions(server, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK));
		// Если настройки узла приняты не были, круг пропускается
		if(!io.commit(server)){
			// Считаем круг пропущенным
			::totals.skipped++;
			// Сносим заведённый узел
			io.destroy(server);
			// Переходим к следующему кругу
			continue;
		}
		// Объявляем слушание у связного вида: дейтаграммный сервер слушания не объявляет
		if(stream)
			// Выполняем объявление слушания
			static_cast <void> (io.listen(server, 16));
		// Если запустить узел сервера не удалось, круг пропускается
		if(!io.launch(server)){
			// Считаем круг пропущенным
			::totals.skipped++;
			// Сносим заведённый узел
			io.destroy(server);
			// Переходим к следующему кругу
			continue;
		}
		/**
		 * Встречная сторона заводится СЫРЫМ сокетом, а не узлом движка
		 *
		 * @details Ворошится подача, и подавать негодное обязан тот, кого движок не
		 *          сдерживает: узел движка нарезал бы поток по своим правилам и негодного
		 *          попросту не выдал бы
		 */
		const int32_t peer = ::socket(AF_UNIX, (stream ? SOCK_STREAM : SOCK_DGRAM), 0);
		// Если сокет встречной стороны завести не удалось, круг пропускается
		if(peer < 0){
			// Считаем круг пропущенным
			::totals.skipped++;
			// Сносим заведённый узел
			io.destroy(server);
			// Переходим к следующему кругу
			continue;
		}
		// Адрес узла движка
		struct sockaddr_un address{};
		// Заполняем адрес узла движка
		address.sun_family = AF_UNIX;
		// Копируем путь узла движка
		::strncpy(address.sun_path, uds.c_str(), sizeof(address.sun_path) - 1);
		/**
		 * Сокет встречной стороны заводится НЕБЛОКИРУЮЩИМ
		 *
		 * @warning Блокирующий сокет вешает сам ворошитель: буфер отправки заполняется
		 *          быстрее, чем движок его разбирает, и `send` засыпает навсегда - разбор
		 *          идёт из того же потока, что и подача, и разбудить отправку некому.
		 *          Найдено первым же прогоном: щуп встал в `send`, а не движок
		 */
		static_cast <void> (::fcntl(peer, F_SETFL, (::fcntl(peer, F_GETFL, 0) | O_NONBLOCK)));
		// Если подключиться к узлу движка не удалось, круг пропускается
		if(::connect(peer, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) != 0){
			// Считаем круг пропущенным
			::totals.skipped++;
			// Закрываем сокет встречной стороны
			::close(peer);
			// Сносим заведённый узел
			io.destroy(server);
			// Переходим к следующему кругу
			continue;
		}
		// Даём движку принять подключение
		for(uint8_t i = 0; i < 4; i++){
			// Взводим сторожевой срок оборота опроса
			::alarm(__awh_poll_limit__);
			// Выполняем оборот цикла событий
			static_cast <void> (io.poll(5));
			// Снимаем сторожевой срок
			::alarm(0);
			// Считаем оборот опроса
			::totals.polls++;
		}
		// Число посланных встречной стороной октет круга
		uint64_t written = 0;
		// Свёртка посланных встречной стороной октет круга
		uint64_t given = 1469598103934665603ULL;
		// Размеры посланных сообщений круга
		vector <size_t> sizes;
		// Число сообщений круга
		const uint32_t messages = static_cast <uint32_t> (1 + (random() % 8));
		/**
		 * Шлём сообщения выбранного круга
		 */
		for(uint32_t message = 0; message < messages; message++){
			/**
			 * Размер сообщения берётся из НЕРАВНОМЕРНОГО набора
			 *
			 * @note Равномерный размер не задел бы ни границ буфера, ни нулевой длины:
			 *       беда ждёт именно по краям, и они берутся нарочно чаще середины
			 */
			size_t size = 0;
			/**
			 * Определяем вид размера сообщения
			 */
			switch(random() % 6){
				// Пустое сообщение
				case 0: size = 0; break;
				// Сообщение в один октет
				case 1: size = 1; break;
				// Сообщение мелкое
				case 2: size = (1 + (random() % 64)); break;
				// Сообщение около границы буфера
				case 3: size = (4000 + (random() % 200)); break;
				// Сообщение крупное
				case 4: size = (1 + (random() % 16384)); break;
				// Сообщение больше всякого буфера
				case 5: size = (60000 + (random() % 5000)); break;
			}
			// Тело сообщения
			vector <uint8_t> body(size, 0);
			/**
			 * Наполняем тело сообщения случайными октетами
			 */
			for(size_t i = 0; i < size; i++)
				// Записываем случайный октет тела
				body[i] = static_cast <uint8_t> (random() % 256);
			/**
			 * Дейтаграмма уходит ЦЕЛИКОМ, а поток нарезается
			 *
			 * @note Нарезка потока - главное, ради чего ворошитель заведён: движок обязан
			 *       собрать поток обратно каким угодно способом нарезки
			 */
			if(!stream){
				// Отправляем дейтаграмму целиком, давая движку разобрать накопленное
				const ssize_t result = ::deliver(io, peer, body.data(), body.size());
				// Если дейтаграмма ушла, считаем её
				if(result >= 0){
					// Считаем посланные октеты
					written += static_cast <uint64_t> (result);
					// Домешиваем посланное к свёртке
					given = ::digest(given, body.data(), static_cast <size_t> (result));
					// Запоминаем размер посланного сообщения
					sizes.push_back(static_cast <size_t> (result));
					// Считаем посланное сообщение
					::totals.messages++;
				}
			// Если сообщение уходит потоком
			} else {
				// Положение нарезки тела сообщения
				size_t position = 0;
				/**
				 * Режем тело сообщения на куски произвольного размера
				 */
				while(position < body.size()){
					/**
					 * Размер куска: по одному октету, мелкими долями либо остатком целиком
					 */
					size_t piece = 0;
					/**
					 * Определяем вид нарезки
					 */
					switch(random() % 3){
						// Нарезка по одному октету
						case 0: piece = 1; break;
						// Нарезка мелкими долями
						case 1: piece = (1 + (random() % 17)); break;
						// Остаток целиком
						case 2: piece = (body.size() - position); break;
					}
					// Ограничиваем кусок остатком тела
					if(piece > (body.size() - position))
						// Урезаем кусок до остатка тела
						piece = (body.size() - position);
					// Отправляем кусок тела, давая движку разобрать накопленное
					const ssize_t result = ::deliver(io, peer, body.data() + position, piece);
					// Если кусок отправить не удалось, прекращаем нарезку
					if(result <= 0) break;
					// Считаем посланные октеты
					written += static_cast <uint64_t> (result);
					// Домешиваем посланное к свёртке
					given = ::digest(given, body.data() + position, static_cast <size_t> (result));
					// Сдвигаем положение нарезки
					position += static_cast <size_t> (result);
					/**
					 * Изредка крутим оборот опроса ПОСРЕДИ сообщения
					 *
					 * @note Разбор половины сообщения - обычное дело у потока, и движок
					 *       обязан переживать его, не теряя остатка
					 */
					if((random() % 4) == 0){
						// Взводим сторожевой срок оборота опроса
						::alarm(__awh_poll_limit__);
						// Выполняем оборот цикла событий
						static_cast <void> (io.poll(1));
						// Снимаем сторожевой срок
						::alarm(0);
						// Считаем оборот опроса
						::totals.polls++;
					}
				}
				// Если тело ушло целиком, запоминаем его размер
				if(position == body.size())
					// Запоминаем размер посланного сообщения
					sizes.push_back(body.size());
				// Считаем посланное сообщение
				::totals.messages++;
			}
		}
		/**
		 * Изредка рвём связь, не дождавшись разбора
		 *
		 * @note Обрыв посреди разбора - обычная беда сети, и движок обязан отдать всё,
		 *       что успел принять, а не терять принятое вместе со связью
		 */
		const bool abrupt = ((random() % 8) == 0);
		// Если связь рвётся сразу
		if(abrupt)
			// Закрываем сокет встречной стороны
			::close(peer);
		// Запоминаем миг начала ожидания разбора
		const auto start = chrono::steady_clock::now();
		/**
		 * Крутим обороты опроса, покуда движок не отдаст всё посланное
		 */
		while((delivered < written) && (chrono::duration_cast <chrono::seconds> (chrono::steady_clock::now() - start).count() < 3)){
			// Взводим сторожевой срок оборота опроса
			::alarm(__awh_poll_limit__);
			// Выполняем оборот цикла событий
			static_cast <void> (io.poll(5));
			// Снимаем сторожевой срок
			::alarm(0);
			// Считаем оборот опроса
			::totals.polls++;
		}
		// Если связь ещё не разорвана, закрываем сокет встречной стороны
		if(!abrupt)
			// Закрываем сокет встречной стороны
			::close(peer);
		// Считаем итоги круга
		::totals.written += written;
		::totals.delivered += delivered;
		::totals.reads += reads;
		/**
		 * Сличаем обещанное с отданным
		 *
		 * @note Обрыв посреди разбора из сличения ИСКЛЮЧАЕТСЯ: там потеря хвоста законна,
		 *       и требовать полноты от оборванной связи значило бы ловить несуществующее
		 */
		if(!abrupt){
			// Если объём отданного разошёлся с объёмом посланного
			if(delivered != written){
				// Считаем круг расхождения объёма
				::totals.mismatched++;
				// Сообщаем о находке
				::fprintf(stderr, "НАХОДКА: круг %llu (%s): послано %llu октет, отдано %llu\n",
				 static_cast <unsigned long long> (round), (stream ? "поток" : "сообщения"),
				 static_cast <unsigned long long> (written), static_cast <unsigned long long> (delivered));
			// Если объём сошёлся, сличаем содержимое
			} else if(taken != given) {
				// Считаем круг расхождения содержимого
				::totals.corrupted++;
				// Сообщаем о находке
				::fprintf(stderr, "НАХОДКА: круг %llu (%s): объём сошёлся, а содержимое разошлось\n",
				 static_cast <unsigned long long> (round), (stream ? "поток" : "сообщения"));
			}
			/**
			 * У вида, хранящего границы сообщений, сличаем ещё и их
			 *
			 * @note Пустое сообщение отклика не порождает, и в сличение оно не идёт
			 */
			if(!stream && (delivered == written)){
				// Размеры посланных непустых сообщений
				vector <size_t> expected;
				/**
				 * Перебираем размеры посланных сообщений
				 */
				for(auto & size : sizes){
					// Если сообщение непустое, запоминаем его размер
					if(size > 0)
						// Запоминаем размер непустого сообщения
						expected.push_back(size);
				}
				// Если число откликов либо их размеры разошлись с посланными
				if(parts != expected){
					// Считаем круг потери границ
					::totals.unbounded++;
					// Сообщаем о находке
					::fprintf(stderr, "НАХОДКА: круг %llu: границы сообщений потеряны - послано %zu, отдано %zu откликами\n",
					 static_cast <unsigned long long> (round), expected.size(), parts.size());
				}
			}
		}
		// Сносим заведённый узел
		io.destroy(server);
		// Даём движку довести снос до конца
		for(uint8_t i = 0; i < 3; i++){
			// Взводим сторожевой срок оборота опроса
			::alarm(__awh_poll_limit__);
			// Выполняем оборот цикла событий
			static_cast <void> (io.poll(5));
			// Снимаем сторожевой срок
			::alarm(0);
			// Считаем оборот опроса
			::totals.polls++;
		}
	}
	// Сворачиваем движок
	static_cast <void> (io.deinitialize());
	// Убираем путь узла движка
	::unlink(uds.c_str());
	// Снимаем перечень описателей после работы ворошителя
	const set <int32_t> after = ::descriptors();
	// Число описателей, переживших работу ворошителя
	uint64_t leaked = 0;
	/**
	 * Перебираем описатели, открытые после работы
	 */
	for(auto & fd : after){
		// Если описатель прежде открыт не был, считаем его утёкшим
		if(before.count(fd) == 0)
			// Считаем утёкший описатель
			leaked++;
	}
	// Выводим итоги работы ворошителя
	::printf("\nИТОГИ ворошителя данных (зерно %llu)\n", static_cast <unsigned long long> (seed));
	::printf("  кругов: %llu, пропущено: %llu, оборотов опроса: %llu\n",
	 static_cast <unsigned long long> (::totals.rounds), static_cast <unsigned long long> (::totals.skipped),
	 static_cast <unsigned long long> (::totals.polls));
	::printf("  сообщений: %llu, послано октет: %llu, отдано: %llu, откликов чтения: %llu\n",
	 static_cast <unsigned long long> (::totals.messages), static_cast <unsigned long long> (::totals.written),
	 static_cast <unsigned long long> (::totals.delivered), static_cast <unsigned long long> (::totals.reads));
	::printf("  расхождений объёма: %llu, расхождений содержимого: %llu, потерь границ: %llu\n",
	 static_cast <unsigned long long> (::totals.mismatched), static_cast <unsigned long long> (::totals.corrupted),
	 static_cast <unsigned long long> (::totals.unbounded));
	::printf("  описателей утекло: %llu\n", static_cast <unsigned long long> (leaked));
	// Число находок ворошителя
	const uint64_t found = (::totals.mismatched + ::totals.corrupted + ::totals.unbounded + leaked);
	// Выводим итог работы ворошителя
	::printf("%s\n", ((found == 0) ? "НАХОДОК НЕТ" : "ЕСТЬ НАХОДКИ"));
	// Выводим результат работы приложения
	return ((found == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
