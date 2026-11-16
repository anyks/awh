/**
 * @file zerocopy.cpp
 * @date 2026-11-16
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
 * @brief Пример узла AF_XDP нативного режима с нулевым копированием — демонстрация
 *        приёма кадров штатным колбэком чтения узла движка: проверка поддержки
 *        драйвера, заведение узла с перенаправлением, цикл событий и итог приёма
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Спутник AF_XDP объявлен только под Linux и лишь когда он включён сборкой
 *
 * @details Признак AWH_XDP ставит выбор сборки -DCMAKE_BUILD_XDP=ON. Вне этого
 *          условия пример печатает причину и выходит: звать engine::xdp неоткуда,
 *          а молча собранный пустой образ ничего бы не показал
 *
 */
#if defined(__linux__) && defined(AWH_XDP)
	/**
	 * Стандартные модули
	 */
	#include <iostream>
	#include <atomic>
	#include <chrono>
	#include <csignal>
	#include <cstring>
	#include <cstdlib>
	#include <cstdint>
	#include <string>
	#include <algorithm>
	/**
	 * Подключаем заголовочные файлы проекта
	 */
	#include <sys/fmk.hpp>
	#include <sys/log.hpp>
	#include <net/engine.hpp>
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;
	/**
	 * @brief Параметры запуска примера
	 *
	 * @details Устройство и очередь называются доводами обязательно: перенаправление
	 *          забирает трафик названной очереди в кольцо AF_XDP, и очередь,
	 *          несущая управление машиной, теряет связь. Выбирать их самому нельзя
	 *
	 */
	typedef struct Params {
		// Название сетевого устройства
		string iface;
		// Номер очереди устройства
		uint32_t queue;
		// Срок кручения цикла событий в секундах
		uint32_t seconds;
		// Метка полезной нагрузки искомого кадра
		string marker;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Params() noexcept : iface{""}, queue(0), seconds(10), marker{""} {}
	} params_t;
	// Признак заказанной остановки работы
	static std::atomic_bool __awh_stop__{false};
	/**
	 * @brief Функция обработки сигнала остановки
	 *
	 * @param signal номер полученного сигнала
	 *
	 */
	static void onSignal([[maybe_unused]] int32_t signal) noexcept {
		// Отмечаем работу подлежащей остановке по любому сигналу
		__awh_stop__.store(true);
	}
	/**
	 * @brief Функция печати способа запуска
	 *
	 * @param name название исполняемого файла
	 *
	 */
	static void usage(const char * name) noexcept {
		// Печатаем заголовок примера
		cout << "Узел AF_XDP нативного режима с нулевым копированием AWH" << endl << endl;
		// Печатаем строку запуска
		cout << "Запуск: " << name << " --iface <имя> [доводы]" << endl << endl;
		// Печатаем довод названия устройства
		cout << "  --iface <имя>     сетевое устройство (обязательно), например enp8s0" << endl;
		// Печатаем довод номера очереди
		cout << "  --queue <номер>   очередь устройства (по умолчанию 0)" << endl;
		// Печатаем довод срока кручения цикла
		cout << "  --seconds <число> срок кручения цикла событий (по умолчанию 10)" << endl;
		// Печатаем довод метки искомого кадра
		cout << "  --marker <текст>  метка в полезной нагрузке: цикл встаёт, приняв её" << endl;
		// Печатаем довод справки
		cout << "  --help            напечатать этот текст" << endl << endl;
		// Печатаем пояснение о правах
		cout << "Права суперпользователя обязательны: заведение узла AF_XDP требует CAP_NET_ADMIN." << endl;
		// Печатаем предупреждение о перенаправлении
		cout << "ВНИМАНИЕ: программа BPF крепится, и трафик названной очереди уходит" << endl;
		// Печатаем предупреждение о потере управления
		cout << "в кольцо AF_XDP, минуя сетевой стек. Очередь, несущая ssh, потеряет" << endl;
		// Печатаем совет о выборе очереди
		cout << "связь: называйте очередь без управления и ставьте сторожа, снимающий" << endl;
		// Печатаем команду снятия программы
		cout << "программу при обрыве: ip link set <имя> xdp off" << endl << endl;
		// Печатаем пояснение о внешнем впрыскивателе
		cout << "Кадры в кольцо приходят извне: впрыск с самой машины через AF_PACKET" << endl;
		// Печатаем пояснение о невозврате своего кадра
		cout << "в кольцо приёма не возвращается, коммутатор не шлёт кадр обратно в тот же порт." << endl;
	}
	/**
	 * @brief Функция разбора доводов запуска
	 *
	 * @param argc   длина массива параметров
	 * @param argv   массив параметров
	 * @param params параметры запуска
	 * @return       результат разбора
	 *
	 */
	static bool parse(int32_t argc, char * argv[], params_t & params) noexcept {
		/**
		 * Перебираем все переданные доводы
		 */
		for(int32_t i = 1; i < argc; i++){
			// Получаем название довода
			const string arg(argv[i]);
			// Если запрошен способ запуска
			if(arg.compare("--help") == 0){
				// Печатаем способ запуска
				usage(argv[0]);
				// Возвращаем отказ: работать дальше незачем
				return false;
			// Иначе довод требует значения
			} else {
				// Если значение довода не передано
				if((i + 1) >= argc){
					// Сообщаем о доводе без значения
					cout << "Довод \"" << arg << "\" требует значения" << endl;
					// Возвращаем отказ разбора
					return false;
				}
				// Получаем значение довода
				const string value(argv[++i]);
				// Если названо сетевое устройство
				if(arg.compare("--iface") == 0)
					// Запоминаем название устройства
					params.iface = value;
				// Если назван номер очереди
				else if(arg.compare("--queue") == 0)
					// Запоминаем номер очереди
					params.queue = static_cast <uint32_t> (::strtoul(value.c_str(), nullptr, 10));
				// Если назван срок кручения цикла событий
				else if(arg.compare("--seconds") == 0)
					// Запоминаем срок кручения цикла
					params.seconds = static_cast <uint32_t> (::strtoul(value.c_str(), nullptr, 10));
				// Если названа метка искомого кадра
				else if(arg.compare("--marker") == 0)
					// Запоминаем метку искомого кадра
					params.marker = value;
				// Иначе довод неизвестен
				else {
					// Сообщаем о неизвестном доводе
					cout << "Неизвестный довод \"" << arg << "\"" << endl;
					// Возвращаем отказ разбора
					return false;
				}
			}
		}
		// Если устройство не названо, работать не на чем
		if(params.iface.empty()){
			// Сообщаем об обязательном доводе
			cout << "Довод --iface обязателен" << endl;
			// Возвращаем отказ разбора
			return false;
		}
		// Если срок кручения цикла нулевой, крутить нечего
		if(params.seconds == 0){
			// Сообщаем о недопустимом сроке
			cout << "Довод --seconds требует числа больше нуля" << endl;
			// Возвращаем отказ разбора
			return false;
		}
		// Возвращаем успех разбора
		return true;
	}
	/**
	 * @brief Функция главного цикла примера
	 *
	 * @param params параметры запуска
	 * @return       код выхода из приложения
	 *
	 */
	static int32_t run(const params_t & params) noexcept {
		// Если ядро не несёт AF_XDP, показывать нечего
		if(!engine::xdp::supported()){
			// Сообщаем об отсутствии поддержки ядром
			log::print("Ядро не поддерживает AF_XDP", log::flag_t::CRITICAL);
			// Возвращаем отказ
			return EXIT_FAILURE;
		}
		// Если драйвер устройства не несёт нулевого копирования, нативный режим не заведётся
		if(!engine::xdp::zerocopySupported(params.iface, params.queue)){
			// Сообщаем об отказе пробы: причина и поддержка драйвера, и нехватка прав
			log::print("Проба zero-copy отказала: устройство=%s, очередь=%u (драйвер не несёт нулевого копирования либо нет прав CAP_NET_ADMIN)", log::flag_t::CRITICAL, params.iface.c_str(), params.queue);
			// Возвращаем отказ: отката к обобщённому режиму нет намеренно
			return EXIT_FAILURE;
		}
		// Сообщаем о поддержке нулевого копирования устройством
		log::print("Устройство %s поддерживает AF_XDP zero-copy в очереди %u", log::flag_t::INFO, params.iface.c_str(), params.queue);
		// Инициализируем асинхронный движок ввода-вывода
		if(!engine::io::initialize()){
			// Сообщаем об отказе инициализации движка
			log::print("Ошибка инициализации движка ввода-вывода", log::flag_t::CRITICAL);
			// Возвращаем отказ
			return EXIT_FAILURE;
		}
		// Число кадров, принятых колбэком чтения узла
		uint64_t frames = 0;
		// Число байт, принятых колбэком чтения узла
		uint64_t bytes = 0;
		// Признак приёма кадра с названной меткой
		bool mark = false;
		// Сбрасываем признак заказанной остановки
		__awh_stop__.store(false);
		// Заводим узел нативного режима с нулевым копированием и перенаправлением трафика
		const event::id_t id = engine::xdp::create(params.iface, params.queue, true, net::xdp::mode_t::NATIVE);
		// Если узел не заведён, датапуть недоступен
		if(id == 0){
			// Сообщаем об отказе заведения узла
			log::print("Узел AF_XDP не заведён: устройство=%s, очередь=%u (нужны права CAP_NET_ADMIN)", log::flag_t::CRITICAL, params.iface.c_str(), params.queue);
			// Возвращаем отказ
			return EXIT_FAILURE;
		}
		// Если узел заведён не в нативном режиме, нулевое копирование не активно
		if(engine::xdp::mode(id) != net::xdp::mode_t::NATIVE){
			// Сообщаем об отказе нативного режима
			log::print("Узел заведён не в нативном режиме: ID=%u", log::flag_t::CRITICAL, id);
			// Сносим заведённый узел
			engine::xdp::destroy(id);
			// Возвращаем отказ
			return EXIT_FAILURE;
		}
		// Сообщаем о заведении узла нулевого копирования
		log::print("Узел заведён: ID=%u, устройство=%s, очередь=%u, режим=native", log::flag_t::INFO, id, params.iface.c_str(), params.queue);
		// Подписываемся на чтение узла: кадры приходят штатным колбэком, без ручного приёма
		engine::io::on(id, [&frames, &bytes, &mark, &params](const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept -> void {
			// Учитываем принятый кадр
			frames++;
			// Учитываем принятые байты
			bytes += size;
			// Если метка названа, ещё не найдена и кадр не короче метки
			if(!params.marker.empty() && !mark && (size >= params.marker.size())){
				// Ищем метку в полезной нагрузке кадра
				const uint8_t * found = std::search(buffer, (buffer + size), params.marker.begin(), params.marker.end());
				// Если метка найдена, отмечаем приём искомого кадра
				if(found != (buffer + size)){
					// Отмечаем приём кадра с меткой
					mark = true;
					// Сообщаем о приёме искомого кадра
					log::print("Кадр с меткой принят: ID=%u, %zu байт", log::flag_t::INFO, eid, size);
				}
			}
		});
		// Подписываемся на ошибку узла
		engine::io::on(id, [](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
			// Сообщаем об ошибке узла
			log::print("Ошибка узла: ID=%u, код=%u, описание=%s", log::flag_t::CRITICAL, eid, static_cast <uint32_t> (error), description.c_str());
		});
		// Ставим обработчик сигнала прерывания
		::signal(SIGINT, onSignal);
		// Ставим обработчик сигнала завершения
		::signal(SIGTERM, onSignal);
		// Сообщаем о начале кручения цикла событий
		log::print("Цикл событий крутится %u с, остановка по Ctrl-C", log::flag_t::INFO, params.seconds);
		// Определяем срок кручения цикла событий
		const auto deadline = (std::chrono::steady_clock::now() + std::chrono::seconds(params.seconds));
		/**
		 * Крутим цикл событий до срока, до остановки либо до приёма кадра с меткой
		 */
		while(!__awh_stop__.load() && (std::chrono::steady_clock::now() < deadline)){
			// Если опрос событий отказал, крутить дальше нечего
			if(!engine::io::poll(100))
				// Прекращаем кручение цикла
				break;
			// Если метка названа и кадр с ней уже принят, ждать больше нечего
			if(!params.marker.empty() && mark)
				// Прекращаем кручение цикла
				break;
		}
		// Сносим узел датапути AF_XDP: программа BPF снимается вместе с ним
		engine::xdp::destroy(id);
		// Сообщаем итог приёма
		log::print("Приём завершён: ID=%u, кадров=%llu, байт=%llu", log::flag_t::INFO, id, static_cast <unsigned long long> (frames), static_cast <unsigned long long> (bytes));
		// Если метка названа, сообщаем результат её поиска
		if(!params.marker.empty())
			// Сообщаем, принят ли кадр с названной меткой
			log::print("Кадр с меткой \"%s\": %s", (mark ? log::flag_t::INFO : log::flag_t::WARNING), params.marker.c_str(), (mark ? "принят" : "не принят"));
		// Если кадров не принято вовсе, кольцо приёма осталось тихим
		if(frames == 0)
			// Сообщаем о тихом кольце приёма
			log::print("Кольцо приёма тихо: внешний впрыскиватель в очередь %u не посылал кадров", log::flag_t::WARNING, params.queue);
		// Возвращаем успех
		return EXIT_SUCCESS;
	}
	/**
	 * @brief Функция входа приложения
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 * @return     код выхода из приложения
	 *
	 */
	int32_t main(int32_t argc, char * argv[]){
		/**
		 * Заводим модуль ядра первым делом
		 *
		 * @note Заведение захватывает выдачу памяти процесса и обязано идти
		 *       ДО всякой выдачи и ДО порождения потоков
		 *
		 */
		fmk::initialize();
		// Параметры запуска примера
		params_t params;
		// Если доводы разобраны, показываем датапуть
		if(parse(argc, argv, params))
			// Возвращаем результат показа датапути
			return run(params);
		// Возвращаем отказ разбора доводов
		return EXIT_FAILURE;
	}
	/**
	 * Вне собранного спутника пример лишь называет причину
	 *
	 * @details Спутник engine::xdp объявлен только под Linux с признаком AWH_XDP,
	 *          и на прочих системах либо при выключенном спутнике звать его
	 *          неоткуда. Образ при этом собирается и запускается везде, как
	 *          всякий пример проекта, а не выпадает из сборки по системе
	 *
	 */
#else
	/**
	 * Стандартные модули
	 */
	#include <iostream>
	/**
	 * Подключаем заголовочные файлы проекта
	 */
	#include <sys/fmk.hpp>
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;
	/**
	 * @brief Функция входа приложения вне собранного спутника AF_XDP
	 *
	 * @return код выхода из приложения
	 *
	 */
	int32_t main(){
		/**
		 * Заводим модуль ядра первым делом
		 *
		 * @note Заведение захватывает выдачу памяти процесса и обязано идти
		 *       ДО всякой выдачи и ДО порождения потоков
		 *
		 */
		fmk::initialize();
		// Сообщаем о причине отсутствия примера
		cout << "Спутник AF_XDP не собран: он есть только под Linux и включается" << endl;
		// Сообщаем о выборе сборки, каком спутник включается
		cout << "выбором -DCMAKE_BUILD_XDP=ON при наличии libbpf и libxdp." << endl;
		// Возвращаем успех: отказ здесь означает лишь отсутствие спутника
		return EXIT_SUCCESS;
	}
#endif