/**
 * @file: common.hpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общее окружение эталонных стендов сравнения сетевого движка — параметры нагрузки,
 *        единые для всех сравниваемых реализаций, разбор параметров запуска и вывод результатов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL__
#define __AWH_BENCHMARK_RIVAL__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <unordered_map>

/**
 * Системные заголовочные файлы
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

/**
 * @brief Пространство имён эталонных стендов сравнения
 *
 * @details Параметры нагрузки обязаны совпадать со сценариями `benchmark/net/io`
 *          библиотеки AWH: сравниваются реализации цикла событий, а не разные
 *          объёмы работы, поэтому любое расхождение здесь обесценивает отчёт
 *
 */
namespace rival {
	/**
	 * @brief Размер полезной нагрузки одного обмена в октетах
	 *
	 */
	static constexpr size_t ECHO_PAYLOAD = 64;
	/**
	 * @brief Количество обменов сценария эха на одном подключении
	 *
	 */
	static constexpr size_t ECHO_SINGLE_ROUNDS = 100000;
	/**
	 * @brief Количество одновременных подключений сценария эха на множестве подключений
	 *
	 */
	static constexpr size_t ECHO_MULTI_CONNECTIONS = 500;
	/**
	 * @brief Количество обменов сценария эха на множестве подключений
	 *
	 */
	static constexpr size_t ECHO_MULTI_ROUNDS = 100000;
	/**
	 * @brief Размер блока постановки в очередь сценария пропускной способности в октетах
	 *
	 */
	static constexpr size_t STREAM_CHUNK = 65536;
	/**
	 * @brief Объём передачи сценария пропускной способности в октетах
	 *
	 */
	static constexpr size_t STREAM_VOLUME = (512 * 1024 * 1024);
	/**
	 * @brief Количество циклов подключения сценария установления соединений
	 *
	 * @details Количество ограничено сверху диапазоном динамических портов
	 *          операционной системы: закрытая сторона подключения удерживает
	 *          порт в состоянии TIME_WAIT, и прогон на 16 тысячах циклов
	 *          упирается в исчерпание диапазона, а не в цикл событий
	 *
	 */
	static constexpr size_t ACCEPT_ROUNDS = 4000;
	/**
	 * @brief Количество циклов прогрева сценария установления соединений
	 *
	 */
	static constexpr size_t ACCEPT_WARMUP = 200;
	/**
	 * @brief Прототип реестра описателей таймеров по идентификатору
	 *
	 * @details Сценарии с суффиксом `by-id` ставят стенд в те же условия, в
	 *          которых работает движок AWH: он принимает от пользователя
	 *          целочисленный идентификатор события и разрешает его поиском в
	 *          реестре узлов, тогда как интерфейс чужих библиотек принимает
	 *          готовый указатель на наблюдатель. Разница в интерфейсе - не разница
	 *          в объёме работы: настоящему серверу описатель тоже неоткуда взять
	 *          иначе, чем из собственного реестра, и стоимость поиска он платит
	 *          сам, просто вне библиотеки. Реестр здесь взят той же структуры и с
	 *          тем же типом ключа, что и у движка, чтобы сравнение было честным
	 *
	 * @tparam T тип описателя таймера
	 *
	 */
	template <typename T>
	using registry_t = std::unordered_map <uint32_t, T>;

	/**
	 * @brief Количество таймеров сценариев измерения структуры дедлайнов
	 *
	 */
	static constexpr size_t DEADLINE_COUNT = 50000;
	/**
	 * @brief Количество таймеров уменьшенного прогона сценариев структуры дедлайнов
	 *
	 * @details Служит для оценки сложности: отношение стоимости одной
	 *          операции на полном прогоне к стоимости на уменьшенном
	 *          показывает, как структура ведёт себя при десятикратном
	 *          росте количества таймеров. У логарифмической структуры
	 *          отношение близко к единице, у линейной - к десяти
	 *
	 */
	static constexpr size_t DEADLINE_SMALL_COUNT = 5000;
	/**
	 * @brief Разброс дедлайнов сценариев структуры дедлайнов в миллисекундах
	 *
	 */
	static constexpr uint32_t DEADLINE_SPREAD = 1000;
	/**
	 * @brief Отступ дедлайнов сценариев структуры дедлайнов в миллисекундах
	 *
	 * @details Дедлайны отнесены далеко в будущее нарочно: сценарии измеряют
	 *          постановку, отмену и перевзведение, и срабатывание таймера в
	 *          середине замера исказило бы показатель
	 *
	 */
	static constexpr uint32_t DEADLINE_OFFSET = 60000;
	/**
	 * @brief Количество таймеров сценария измерения таймеров
	 *
	 */
	static constexpr size_t TIMER_COUNT = 50000;
	/**
	 * @brief Разброс срабатывания таймеров в миллисекундах
	 *
	 */
	static constexpr uint32_t TIMER_SPREAD = 8;
	/**
	 * @brief Глубина очереди принятия входящих соединений
	 *
	 */
	static constexpr int32_t BACKLOG = 1024;
	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	typedef struct Outcome {
		// Количество выполненных операций
		size_t operations;
		// Объём переданных данных в октетах
		size_t bytes;
		// Затраченное время в секундах
		double seconds;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Outcome() noexcept : operations(0), bytes(0), seconds(0.0) {}
	} outcome_t;
	/**
	 * @brief Структура состояния прогона сценария обмена короткими сообщениями
	 *
	 * @note Учёт вынесен в общее окружение намеренно: границы прогрева и замера
	 *       обязаны совпадать у всех сравниваемых реализаций, а повторение
	 *       логики в каждом стенде рано или поздно даёт расхождение
	 *
	 */
	typedef struct Echo {
		// Флаг активности замера
		bool measuring;
		// Количество выполненных обменов прогрева
		size_t warmed;
		// Количество выполненных обменов замера
		size_t done;
		// Требуемое количество обменов прогрева
		size_t warmup;
		// Требуемое количество обменов замера
		size_t rounds;
		// Момент начала замера
		std::chrono::steady_clock::time_point start;
		// Момент окончания замера
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 * @param connections количество одновременных подключений
		 * @param count       требуемое количество обменов замера
		 *
		 */
		explicit Echo(const size_t connections, const size_t count) noexcept :
		 measuring(false), warmed(0), done(0), warmup(connections * 8), rounds(count) {}
		/**
		 * @brief Метод учёта выполненного обмена
		 *
		 * @return результат учёта (true - обмен следует продолжать)
		 *
		 */
		bool account() noexcept {
			// Если замер ещё не начат
			if(!this->measuring){
				// Считаем выполненный обмен прогрева
				this->warmed++;
				// Если прогрев ещё не завершён
				if(this->warmed < this->warmup)
					// Продолжаем обмен
					return true;
				// Включаем режим замера
				this->measuring = true;
				// Запоминаем момент начала замера
				this->start = std::chrono::steady_clock::now();
				// Продолжаем обмен
				return true;
			}
			// Считаем выполненный обмен замера
			this->done++;
			// Если требуемое количество обменов ещё не выполнено
			if(this->done < this->rounds)
				// Продолжаем обмен
				return true;
			// Запоминаем момент окончания замера
			this->finish = std::chrono::steady_clock::now();
			// Прекращаем обмен
			return false;
		}
	} echo_t;
	/**
	 * @brief Функция получения пикового объёма занятой процессом памяти
	 *
	 * @return пиковый объём занятой процессом памяти в октетах
	 *
	 */
	static inline size_t footprint() noexcept {
		// Объект сведений о потреблении ресурсов процессом
		struct rusage usage{};
		// Если сведения о потреблении ресурсов не получены
		if(::getrusage(RUSAGE_SELF, &usage) != 0)
			// Выводим нулевой объём занятой памяти
			return 0;
		/**
		 * Если стенд собран под операционную систему macOS
		 */
		#if __APPLE__
			// Выводим пиковый объём занятой памяти как есть: macOS сообщает его в октетах
			return static_cast <size_t> (usage.ru_maxrss);
		/**
		 * Если стенд собран под все остальные операционные системы
		 */
		#else
			// Выводим пиковый объём занятой памяти: остальные системы сообщают его в кибибайтах
			return (static_cast <size_t> (usage.ru_maxrss) * 1024);
		#endif
	}
	/**
	 * @brief Функция вывода результата прогона сценария
	 *
	 * @note Формат вывода повторяет набор бенчмарков AWH, поэтому результаты
	 *       стендов и библиотеки сводятся в одну таблицу без пересчёта
	 *
	 * @param name    название сценария
	 * @param units   единица измерения характеристики
	 * @param value   измеренное значение характеристики
	 * @param outcome итоги прогона сценария
	 *
	 */
	static inline void report(const char * name, const char * units, const double value, const outcome_t & outcome) noexcept {
		// Вычисляем среднее время выполнения одной операции в микросекундах
		const double microseconds = ((outcome.operations > 0)
		 ? ((outcome.seconds * 1e6) / static_cast <double> (outcome.operations)) : 0.0);
		// Выводим измеренное значение характеристики
		::printf("%-32s %14.2f   (%s)\n", name, value, units);
		// Выводим сведения о прогоне сценария
		::printf(
			"%34sопераций: %zu, время: %.3f с, на операцию: %.2f мкс, память процесса: %.1f МБ\n",
			"", outcome.operations, outcome.seconds, microseconds,
			(static_cast <double> (footprint()) / 1048576.0)
		);
	}
	/**
	 * @brief Функция извлечения количества операций в секунду
	 *
	 * @param outcome итоги прогона сценария
	 * @return        количество операций в секунду
	 *
	 */
	static inline double perSecond(const outcome_t & outcome) noexcept {
		// Если время прогона не измерено
		if(outcome.seconds <= 0.0)
			// Выводим нулевое количество операций в секунду
			return 0.0;
		// Выводим количество операций в секунду
		return (static_cast <double> (outcome.operations) / outcome.seconds);
	}
	/**
	 * @brief Функция извлечения пропускной способности в мебибайтах в секунду
	 *
	 * @param outcome итоги прогона сценария
	 * @return        пропускная способность
	 *
	 */
	static inline double megabytes(const outcome_t & outcome) noexcept {
		// Если время прогона не измерено
		if(outcome.seconds <= 0.0)
			// Выводим нулевую пропускную способность
			return 0.0;
		// Выводим пропускную способность в мебибайтах в секунду
		return ((static_cast <double> (outcome.bytes) / 1048576.0) / outcome.seconds);
	}
	/**
	 * @brief Функция проверки соответствия сценария фильтру запуска
	 *
	 * @param name   название сценария
	 * @param filter фильтр названий сценариев
	 * @return       результат проверки соответствия
	 *
	 */
	static inline bool selected(const char * name, const char * filter) noexcept {
		// Если фильтр не задан, выполняются все сценарии
		return ((filter == nullptr) || (::strstr(name, filter) != nullptr));
	}
	/**
	 * @brief Функция получения фильтра названий сценариев из параметров запуска
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 * @return     фильтр названий сценариев
	 *
	 */
	static inline const char * filter(const int32_t argc, char ** argv) noexcept {
		/**
		 * Перебираем параметры запуска стенда
		 */
		for(int32_t i = 1; i < argc; i++){
			// Если задан фильтр названий выполняемых сценариев
			if(::strncmp(argv[i], "--filter=", 9) == 0)
				// Выводим фильтр названий сценариев
				return (argv[i] + 9);
		}
		// Выводим отсутствие фильтра названий сценариев
		return nullptr;
	}
	/**
	 * @brief Функция создания слушающего сокета петлевого интерфейса
	 *
	 * @note Порт выделяется системой: фиксированный порт ломал бы повторный
	 *       прогон, пока предыдущий удерживает его в состоянии TIME_WAIT
	 *
	 * @param address выводимые параметры привязки слушающего сокета
	 * @return        дескриптор слушающего сокета
	 *
	 */
	static inline int32_t listener(struct sockaddr_in & address) noexcept {
		// Выполняем создание слушающего сокета
		const int32_t result = ::socket(AF_INET, SOCK_STREAM, 0);
		// Если слушающий сокет не создан
		if(result < 0)
			// Выводим признак ошибки создания сокета
			return -1;
		// Значение активации опции сокета
		const int32_t enable = 1;
		// Активируем переиспользование адреса сокета
		::setsockopt(result, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
		// Обнуляем параметры привязки сокета
		::memset(&address, 0, sizeof(address));
		// Устанавливаем семейство адреса
		address.sin_family = AF_INET;
		// Устанавливаем адрес петлевого интерфейса
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		// Запрашиваем у системы любой свободный порт
		address.sin_port = 0;
		// Если привязка слушающего сокета не выполнена
		if(::bind(result, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) != 0){
			// Выполняем закрытие слушающего сокета
			::close(result);
			// Выводим признак ошибки привязки сокета
			return -1;
		}
		// Размер структуры параметров сокета
		socklen_t length = sizeof(address);
		// Извлекаем параметры привязки слушающего сокета
		::getsockname(result, reinterpret_cast <struct sockaddr *> (&address), &length);
		// Переводим сокет в режим прослушивания входящих подключений
		::listen(result, BACKLOG);
		// Переводим слушающий сокет в неблокирующий режим
		::fcntl(result, F_SETFL, (::fcntl(result, F_GETFL, 0) | O_NONBLOCK));
		// Выводим дескриптор слушающего сокета
		return result;
	}
	/**
	 * @brief Функция создания подключённого клиентского сокета
	 *
	 * @note Набор опций повторяет опции событий сетевого движка AWH:
	 *       неблокирующий ввод-вывод и отключённый алгоритм Нейгла
	 *
	 * @param address параметры подключения к слушающему сокету
	 * @return        дескриптор клиентского сокета
	 *
	 */
	static inline int32_t connector(const struct sockaddr_in & address) noexcept {
		// Выполняем создание клиентского сокета
		const int32_t result = ::socket(AF_INET, SOCK_STREAM, 0);
		// Если клиентский сокет не создан
		if(result < 0)
			// Выводим признак ошибки создания сокета
			return -1;
		// Значение активации опции сокета
		const int32_t enable = 1;
		// Отключаем алгоритм Нейгла
		::setsockopt(result, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
		// Переводим сокет в неблокирующий режим
		::fcntl(result, F_SETFL, (::fcntl(result, F_GETFL, 0) | O_NONBLOCK));
		// Выполняем подключение к слушающему сокету
		::connect(result, reinterpret_cast <const struct sockaddr *> (&address), sizeof(address));
		// Выводим дескриптор клиентского сокета
		return result;
	}
	/**
	 * @brief Функция настройки принятого сокета
	 *
	 * @param fd дескриптор принятого сокета
	 *
	 */
	static inline void adjust(const int32_t fd) noexcept {
		// Значение активации опции сокета
		const int32_t enable = 1;
		// Отключаем алгоритм Нейгла
		::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
		// Переводим сокет в неблокирующий режим
		::fcntl(fd, F_SETFL, (::fcntl(fd, F_GETFL, 0) | O_NONBLOCK));
	}
	/**
	 * @brief Функция получения текущего момента времени
	 *
	 * @return текущий момент времени
	 *
	 */
	static inline std::chrono::steady_clock::time_point now() noexcept {
		// Выводим текущий момент времени
		return std::chrono::steady_clock::now();
	}
	/**
	 * @brief Функция вычисления затраченного времени в секундах
	 *
	 * @param start  момент начала замера
	 * @param finish момент окончания замера
	 * @return       затраченное время в секундах
	 *
	 */
	static inline double elapsed(const std::chrono::steady_clock::time_point & start, const std::chrono::steady_clock::time_point & finish) noexcept {
		// Выводим затраченное время в секундах
		return std::chrono::duration <double> (finish - start).count();
	}
};

#endif // __AWH_BENCHMARK_RIVAL__
