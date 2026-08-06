/**
 * @file: portmap.cpp
 * @date: 2026-08-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения кодеков перенаправления портов — сборка и разбор просьб
 *        договоров PCP и NAT-PMP, обнаружение устройств по договору SSDP, вызов служб
 *        по договору SOAP и чтение описания устройства
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <chrono>
#include <cstdio>
#include <cstring>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <proto/portmap/pcp.hpp>
#include <proto/portmap/natpmp.hpp>
#include <proto/portmap/ssdp.hpp>
#include <proto/portmap/soap.hpp>
#include <proto/portmap/device.hpp>

/**
 * Подключаем заголовочный файл набора бенчмарков
 */
#include "../../main.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён библиотеки
 */
using namespace awh;

/**
 * @brief Внутренние параметры и сценарии бенчмарков кодеков перенаправления портов
 *
 * @details Кодеки эти работают на каждом обращении к маршрутизатору, и обращений
 *          этих немного: одно на открытие перенаправления и по одному на его
 *          продление. Мерилом им потому служит не пропускная способность, а
 *          стоимость одной операции - и особенно расход выделений памяти, ведь
 *          обращения идут из обработчика событий сети
 *
 * @note Показатели эти прежде снимались вручную и в дереве не хранились. Пока их
 *       здесь не было, любая просадка кодеков прошла бы незамеченной - ровно так,
 *       как прошли две просадки чтения разметки, найденные лишь отдельным замером
 *
 */
namespace {
	/**
	 * @brief Количество повторений сценариев двоичных договоров
	 *
	 * @details Одна операция договоров PCP и NAT-PMP обходится в единицы наносекунд,
	 *          и на меньшем количестве повторений замер измерял бы разрешение часов
	 *
	 */
	static constexpr size_t BINARY_ROUNDS = 1000000;
	/**
	 * @brief Количество повторений сценариев текстовых договоров
	 *
	 * @details Договоры SSDP и SOAP собирают и разбирают текст, и одна их операция
	 *          дороже двоичной на два-три порядка
	 *
	 */
	static constexpr size_t TEXT_ROUNDS = 100000;
	/**
	 * @brief Количество повторений прогрева сценариев
	 *
	 * @details Первые обращения выходят на установившийся режим: распределитель
	 *          памяти набирает рабочий объём, а внутренние накопители кодеков -
	 *          ёмкость. Замер с холодного старта мерил бы разгон, а не скорость
	 *
	 */
	static constexpr size_t WARMUP = 5000;

	/**
	 * @brief Пороги количества операций в секунду
	 *
	 * @details Пороги откалиброваны по самому медленному из отладочных стендов
	 *          (OpenBSD) с двойным запасом: между ним и рабочей машиной разница
	 *          девятикратная, и порог, снятый с рабочей машины, отказывал бы на
	 *          стендах всякий раз. Стеречь им положено не потерю процентов, а
	 *          обвал показателя - появление выделения памяти на каждое обращение
	 *          либо перебор там, где стояла раскладка
	 *
	 * @note Самый медленный стенд у разных сценариев разный: двоичные договоры
	 *       медленнее всего идут у FreeBSD, а текстовые - у OpenBSD, и порядок
	 *       машин по быстродействию тут не сплошной. Порог потому взят по
	 *       наименьшему из снятых для каждого сценария в отдельности
	 *
	 */
	static constexpr double PCP_BUILD_THRESHOLD = 10000000.0;
	/**
	 * @brief Порог количества разборов ответа договора PCP в секунду
	 *
	 */
	static constexpr double PCP_PARSE_THRESHOLD = 15000000.0;
	/**
	 * @brief Порог количества сборок просьбы договора NAT-PMP в секунду
	 *
	 */
	static constexpr double NATPMP_BUILD_THRESHOLD = 80000000.0;
	/**
	 * @brief Порог количества разборов ответа договора NAT-PMP в секунду
	 *
	 */
	static constexpr double NATPMP_PARSE_THRESHOLD = 28000000.0;
	/**
	 * @brief Порог количества сборок просьбы обнаружения по договору SSDP в секунду
	 *
	 */
	static constexpr double SSDP_BUILD_THRESHOLD = 400000.0;
	/**
	 * @brief Порог количества разборов ответа обнаружения по договору SSDP в секунду
	 *
	 */
	static constexpr double SSDP_PARSE_THRESHOLD = 200000.0;
	/**
	 * @brief Порог количества сборок вызова службы по договору SOAP в секунду
	 *
	 */
	static constexpr double SOAP_BUILD_THRESHOLD = 35000.0;
	/**
	 * @brief Порог количества разборов ответа службы по договору SOAP в секунду
	 *
	 */
	static constexpr double SOAP_PARSE_THRESHOLD = 30000.0;
	/**
	 * @brief Порог количества разборов описания устройства в секунду
	 *
	 */
	static constexpr double DEVICE_PARSE_THRESHOLD = 35000.0;
	/**
	 * @brief Порог количества выделений памяти на сборку просьбы двоичного договора
	 *
	 * @details Просьба договоров PCP и NAT-PMP собирается в буфер вызывающего, и
	 *          выделять память кодеку незачем вовсе. Показатель этот воспроизводим
	 *          до единиц и годится в порог куда больше времени: единственное
	 *          появившееся выделение означает, что просьба стала собираться через
	 *          промежуточное хранилище
	 *
	 */
	static constexpr double BINARY_ALLOCATIONS_THRESHOLD = 0.5;

	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	typedef struct Outcome {
		// Количество выполненных операций
		size_t operations;
		// Затраченное время в секундах
		double seconds;
		// Количество выполненных выделений памяти
		size_t allocations;
		// Суммарный объём выделенной памяти в октетах
		size_t bytes;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Outcome() noexcept : operations(0), seconds(0.0), allocations(0), bytes(0) {}
	} outcome_t;

	/**
	 * @brief Функция получения накопителя итогов работы сценариев
	 *
	 * @details Итог каждой операции накапливается: без этого оптимизатор вправе
	 *          убрать измеряемую работу целиком, и стенд мерил бы пустой цикл
	 *
	 * @return ссылка на накопитель итогов
	 *
	 */
	static uint64_t & checksum() noexcept {
		// Накопитель итогов работы сценариев
		static uint64_t result = 0;
		// Выводим ссылку на накопитель итогов
		return result;
	}
	/**
	 * @brief Функция получения объекта фреймворка сценариев
	 *
	 * @return объект фреймворка сценариев
	 *
	 */
	static const fmk_t * framework() noexcept {
		// Объект фреймворка сценариев
		static fmk_t result;
		// Выводим объект фреймворка сценариев
		return &result;
	}
	/**
	 * @brief Функция получения объекта логирования сценариев
	 *
	 * @return объект логирования сценариев
	 *
	 */
	static const log_t * logger() noexcept {
		// Объект логирования сценариев
		static log_t result(framework());
		// Выводим объект логирования сценариев
		return &result;
	}
	/**
	 * @brief Шаблон типа измеряемого сценария
	 *
	 * @tparam SCENARIO тип функции прогоняемого сценария
	 *
	 */
	template <typename SCENARIO>
	/**
	 * @brief Функция прогона одного сценария
	 *
	 * @param rounds   количество повторений замера
	 * @param counting признак учёта выделений памяти
	 * @param scenario прогоняемый сценарий
	 * @return         итоги прогона сценария
	 *
	 */
	static outcome_t measure(const size_t rounds, const bool counting, SCENARIO scenario) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		/**
		 * Выполняем прогрев измеряемой операции
		 */
		for(size_t i = 0; i < WARMUP; i++)
			// Выполняем очередное повторение прогрева
			checksum() += scenario(i);
		/**
		 * Если измеряются выделения памяти
		 */
		if(counting)
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = chrono::steady_clock::now();
		/**
		 * Выполняем требуемое количество повторений замера
		 */
		for(size_t i = 0; i < rounds; i++)
			// Выполняем очередное повторение замера
			checksum() += scenario(i);
		// Запоминаем момент окончания измерения
		const auto finish = chrono::steady_clock::now();
		/**
		 * Если измеряются выделения памяти
		 */
		if(counting){
			// Отключаем учёт выделений памяти
			awh::benchmark::counting(false);
			// Получаем статистику выделений памяти
			awh::benchmark::allocations(result.allocations, result.bytes);
		}
		// Запоминаем количество выполненных операций
		result.operations = rounds;
		// Запоминаем затраченное время
		result.seconds = chrono::duration <double> (finish - start).count();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция формирования результата замера скорости
	 *
	 * @param outcome итоги прогона сценария
	 * @return        результат замера
	 *
	 */
	static awh::benchmark::result_t speed(const outcome_t & outcome) noexcept {
		// Результат замера
		awh::benchmark::result_t result;
		// Устанавливаем количество операций в секунду
		result.value = ((outcome.seconds > 0.0) ? (static_cast <double> (outcome.operations) / outcome.seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Выполняем сборку сведений о прогоне
		::snprintf(
			details, sizeof(details), "операций: %zu, время: %.3f с, на операцию: %.3f мкс",
			outcome.operations, outcome.seconds,
			((outcome.operations > 0) ? ((outcome.seconds * 1e6) / static_cast <double> (outcome.operations)) : 0.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат замера
		return result;
	}
	/**
	 * @brief Функция формирования результата замера выделений памяти
	 *
	 * @param outcome итоги прогона сценария
	 * @return        результат замера
	 *
	 */
	static awh::benchmark::result_t memory(const outcome_t & outcome) noexcept {
		// Результат замера
		awh::benchmark::result_t result;
		// Устанавливаем количество выделений памяти на одну операцию
		result.value = ((outcome.operations > 0)
			? (static_cast <double> (outcome.allocations) / static_cast <double> (outcome.operations)) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Выполняем сборку сведений о прогоне
		::snprintf(
			details, sizeof(details), "операций: %zu, выделений: %zu, выделено: %.1f МБ",
			outcome.operations, outcome.allocations, (static_cast <double> (outcome.bytes) / 1048576.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат замера
		return result;
	}
	/**
	 * @brief Функция получения образца просьбы договора PCP
	 *
	 * @return образец просьбы о перенаправлении порта
	 *
	 */
	static const proto::portmap::pcp_t::request_t & pcpRequest() noexcept {
		// Собираемый образец просьбы о перенаправлении порта
		static const proto::portmap::pcp_t::request_t result = []() noexcept -> proto::portmap::pcp_t::request_t {
			// Собираемая просьба о перенаправлении порта
			proto::portmap::pcp_t::request_t result;
			// Устанавливаем вид запрашиваемого действия
			result.opcode = proto::portmap::pcp_t::opcode_t::MAP;
			// Устанавливаем договор передачи перенаправляемого порта
			result.proto = proto::portmap::pcp_t::proto_t::TCP;
			// Устанавливаем запрашиваемый срок жизни перенаправления
			result.lifeTime = 3600;
			// Устанавливаем внутренний порт перенаправления
			result.internalPort = 8080;
			// Устанавливаем внешний порт перенаправления
			result.externalPort = 8080;
			// Выводим собранную просьбу о перенаправлении порта
			return result;
		}();
		// Выводим образец просьбы о перенаправлении порта
		return result;
	}
	/**
	 * @brief Функция получения образца просьбы договора NAT-PMP
	 *
	 * @return образец просьбы о перенаправлении порта
	 *
	 */
	static const proto::portmap::natpmp_t::request_t & natpmpRequest() noexcept {
		// Собираемый образец просьбы о перенаправлении порта
		static const proto::portmap::natpmp_t::request_t result = []() noexcept -> proto::portmap::natpmp_t::request_t {
			// Собираемая просьба о перенаправлении порта
			proto::portmap::natpmp_t::request_t result;
			// Устанавливаем договор передачи перенаправляемого порта
			result.proto = proto::portmap::natpmp_t::proto_t::TCP;
			// Устанавливаем внутренний порт перенаправления
			result.internalPort = 8080;
			// Устанавливаем внешний порт перенаправления
			result.externalPort = 8080;
			// Устанавливаем запрашиваемый срок жизни перенаправления
			result.lifeTime = 3600;
			// Выводим собранную просьбу о перенаправлении порта
			return result;
		}();
		// Выводим образец просьбы о перенаправлении порта
		return result;
	}
	/**
	 * @brief Функция получения образца ответа обнаружения по договору SSDP
	 *
	 * @return образец ответа маршрутизатора на просьбу обнаружения
	 *
	 */
	static const string & ssdpAnswer() noexcept {
		// Образец ответа маршрутизатора на просьбу обнаружения
		static const string result =
			"HTTP/1.1 200 OK\r\nCACHE-CONTROL: max-age=1800\r\nEXT:\r\n"
			"LOCATION: http://192.168.1.1:5000/rootDesc.xml\r\n"
			"SERVER: Linux/1.0 UPnP/1.0 miniupnpd/2.3\r\n"
			"ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
			"USN: uuid:0001::urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n\r\n";
		// Выводим образец ответа маршрутизатора
		return result;
	}
	/**
	 * @brief Функция получения образца доводов вызова службы по договору SOAP
	 *
	 * @return образец доводов вызова службы
	 *
	 */
	static const vector <proto::portmap::soap_t::argument_t> & soapArguments() noexcept {
		// Собираемый образец доводов вызова службы
		static const vector <proto::portmap::soap_t::argument_t> result = []() noexcept -> vector <proto::portmap::soap_t::argument_t> {
			// Собираемые доводы вызова службы
			vector <proto::portmap::soap_t::argument_t> result;
			/**
			 * Выполняем сборку доводов вызова открытия перенаправления
			 */
			for(const char * name : {"NewRemoteHost", "NewExternalPort", "NewProtocol", "NewInternalPort", "NewInternalClient"}){
				// Собираемый очередной довод вызова службы
				proto::portmap::soap_t::argument_t argument;
				// Устанавливаем название довода вызова службы
				argument.name = name;
				// Устанавливаем значение довода вызова службы
				argument.value = "8080";
				// Выполняем добавление довода к вызову службы
				result.push_back(argument);
			}
			// Выводим собранные доводы вызова службы
			return result;
		}();
		// Выводим образец доводов вызова службы
		return result;
	}
	/**
	 * @brief Функция получения образца описания устройства
	 *
	 * @return образец описания устройства, выдаваемого маршрутизатором
	 *
	 */
	static const string & description() noexcept {
		// Образец описания устройства
		static const string result =
			"<?xml version=\"1.0\"?><root xmlns=\"urn:schemas-upnp-org:device-1-0\"><device>"
			"<deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>"
			"<UDN>uuid:0001</UDN><friendlyName>Router</friendlyName><serviceList><service>"
			"<serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>"
			"<controlURL>/ctl/IPConn</controlURL></service></serviceList></device></root>";
		// Выводим образец описания устройства
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки просьбы договора PCP
	 *
	 * @param counting признак учёта выделений памяти
	 * @return         результат измерения
	 *
	 */
	static awh::benchmark::result_t pcpBuild(const bool counting) noexcept {
		// Объект кодека договора PCP
		static proto::portmap::pcp_t pcp(framework(), logger());
		// Буфер собираемой просьбы о перенаправлении порта
		static uint8_t buffer[1500];
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(BINARY_ROUNDS, counting, [](const size_t) noexcept -> uint64_t {
			// Код ошибки сборки просьбы
			proto::portmap::pcp_t::error_t error = proto::portmap::pcp_t::error_t::NONE;
			// Выводим размер собранной просьбы о перенаправлении порта
			return static_cast <uint64_t> (pcp.request(buffer, sizeof(buffer), pcpRequest(), error));
		});
		// Выводим результат измерения
		return (counting ? memory(outcome) : speed(outcome));
	}
	/**
	 * @brief Функция прогона сценария скорости сборки просьбы договора PCP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t pcpBuildSpeed() noexcept {
		// Выводим результат измерения скорости сборки просьбы
		return pcpBuild(false);
	}
	/**
	 * @brief Функция прогона сценария выделений памяти на сборку просьбы договора PCP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t pcpBuildMemory() noexcept {
		// Выводим результат измерения выделений памяти на сборку просьбы
		return pcpBuild(true);
	}
	/**
	 * @brief Функция прогона сценария разбора ответа договора PCP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t pcpParse() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект кодека договора PCP
		static proto::portmap::pcp_t pcp(framework(), logger());
		// Буфер разбираемого ответа маршрутизатора
		static uint8_t answer[1500];
		// Код ошибки сборки просьбы
		proto::portmap::pcp_t::error_t error = proto::portmap::pcp_t::error_t::NONE;
		/**
		 * Собираем разбираемый ответ из просьбы, выставив признак ответа
		 *
		 * @note Ответ собирается кодеком же, а не записывается здесь байтами:
		 *       образец, записанный вручную, разошёлся бы с кодеком при первом же
		 *       изменении договора, и замер пошёл бы по пути отказа
		 */
		static const size_t size = pcp.request(answer, sizeof(answer), pcpRequest(), error);
		// Выставляем признак ответа маршрутизатора
		answer[1] |= 0x80;
		// Выставляем успешный итог обращения
		answer[3] = 0;
		/**
		 * Если разбор собранного ответа отказывает
		 *
		 * @note Проверка эта обязательна: отказавший разбор возвращается мгновенно,
		 *       и без неё сценарий показал бы скорость отказа вместо скорости разбора
		 */
		{
			// Разбираемый ответ маршрутизатора
			proto::portmap::pcp_t::answer_t got;
			// Код ошибки разбора ответа
			proto::portmap::pcp_t::error_t reason = proto::portmap::pcp_t::error_t::NONE;
			/**
			 * Если разбор собранного ответа выполнить не удалось
			 */
			if(!pcp.parse(answer, size, got, reason)){
				// Помечаем измерение как не выполненное
				result.skipped = true;
				// Устанавливаем причину, по которой измерение не выполнялось
				result.reason = "разбор образца ответа договора PCP отказал";
				// Выводим результат измерения
				return result;
			}
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(BINARY_ROUNDS, false, [](const size_t) noexcept -> uint64_t {
			// Разбираемый ответ маршрутизатора
			proto::portmap::pcp_t::answer_t got;
			// Код ошибки разбора ответа
			proto::portmap::pcp_t::error_t reason = proto::portmap::pcp_t::error_t::NONE;
			// Выводим срок жизни разобранного перенаправления
			return (pcp.parse(answer, size, got, reason) ? static_cast <uint64_t> (got.lifeTime) : 0);
		});
		// Выводим результат измерения
		return speed(outcome);
	}
	/**
	 * @brief Функция прогона сценария сборки просьбы договора NAT-PMP
	 *
	 * @param counting признак учёта выделений памяти
	 * @return         результат измерения
	 *
	 */
	static awh::benchmark::result_t natpmpBuild(const bool counting) noexcept {
		// Объект кодека договора NAT-PMP
		static proto::portmap::natpmp_t natpmp(framework(), logger());
		// Буфер собираемой просьбы о перенаправлении порта
		static uint8_t buffer[1500];
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(BINARY_ROUNDS, counting, [](const size_t) noexcept -> uint64_t {
			// Код ошибки сборки просьбы
			proto::portmap::natpmp_t::error_t error = proto::portmap::natpmp_t::error_t::NONE;
			// Выводим размер собранной просьбы о перенаправлении порта
			return static_cast <uint64_t> (natpmp.mapping(buffer, sizeof(buffer), natpmpRequest(), error));
		});
		// Выводим результат измерения
		return (counting ? memory(outcome) : speed(outcome));
	}
	/**
	 * @brief Функция прогона сценария скорости сборки просьбы договора NAT-PMP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t natpmpBuildSpeed() noexcept {
		// Выводим результат измерения скорости сборки просьбы
		return natpmpBuild(false);
	}
	/**
	 * @brief Функция прогона сценария выделений памяти на сборку просьбы договора NAT-PMP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t natpmpBuildMemory() noexcept {
		// Выводим результат измерения выделений памяти на сборку просьбы
		return natpmpBuild(true);
	}
	/**
	 * @brief Функция прогона сценария разбора ответа договора NAT-PMP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t natpmpParse() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект кодека договора NAT-PMP
		static proto::portmap::natpmp_t natpmp(framework(), logger());
		/**
		 * Разбираемый ответ маршрутизатора о перенаправлении порта
		 *
		 * @note Ответ записан здесь байтами намеренно: кодек ответы не собирает, а
		 *       вид ответа задан договором RFC 6886 и меняться не может
		 */
		static const uint8_t answer[16] = {
			0x00, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x1F, 0x90, 0x1F, 0x90, 0x00, 0x00, 0x00, 0x3C
		};
		/**
		 * Если разбор образца ответа отказывает
		 */
		{
			// Разбираемый ответ маршрутизатора
			proto::portmap::natpmp_t::answer_t got;
			// Код ошибки разбора ответа
			proto::portmap::natpmp_t::error_t reason = proto::portmap::natpmp_t::error_t::NONE;
			/**
			 * Если разбор образца ответа выполнить не удалось
			 */
			if(!natpmp.parse(answer, sizeof(answer), got, reason)){
				// Помечаем измерение как не выполненное
				result.skipped = true;
				// Устанавливаем причину, по которой измерение не выполнялось
				result.reason = "разбор образца ответа договора NAT-PMP отказал";
				// Выводим результат измерения
				return result;
			}
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(BINARY_ROUNDS, false, [](const size_t) noexcept -> uint64_t {
			// Разбираемый ответ маршрутизатора
			proto::portmap::natpmp_t::answer_t got;
			// Код ошибки разбора ответа
			proto::portmap::natpmp_t::error_t reason = proto::portmap::natpmp_t::error_t::NONE;
			// Выводим срок жизни разобранного перенаправления
			return (natpmp.parse(answer, sizeof(answer), got, reason) ? static_cast <uint64_t> (got.lifeTime) : 0);
		});
		// Выводим результат измерения
		return speed(outcome);
	}
	/**
	 * @brief Функция прогона сценария сборки просьбы обнаружения по договору SSDP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t ssdpBuild() noexcept {
		// Объект кодека договора SSDP
		static proto::portmap::ssdp_t ssdp(framework(), logger());
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(TEXT_ROUNDS, false, [](const size_t) noexcept -> uint64_t {
			// Выводим размер собранной просьбы обнаружения устройства
			return static_cast <uint64_t> (ssdp.search(proto::portmap::ssdp_t::TARGET_GATEWAY).size());
		});
		// Выводим результат измерения
		return speed(outcome);
	}
	/**
	 * @brief Функция прогона сценария разбора ответа обнаружения по договору SSDP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t ssdpParse() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект кодека договора SSDP
		static proto::portmap::ssdp_t ssdp(framework(), logger());
		/**
		 * Если разбор образца ответа отказывает
		 */
		{
			// Разбираемый ответ маршрутизатора
			proto::portmap::ssdp_t::answer_t got;
			// Код ошибки разбора ответа
			proto::portmap::ssdp_t::error_t reason = proto::portmap::ssdp_t::error_t::NONE;
			/**
			 * Если разбор образца ответа выполнить не удалось
			 */
			if(!ssdp.parse(ssdpAnswer(), got, reason)){
				// Помечаем измерение как не выполненное
				result.skipped = true;
				// Устанавливаем причину, по которой измерение не выполнялось
				result.reason = "разбор образца ответа обнаружения отказал";
				// Выводим результат измерения
				return result;
			}
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(TEXT_ROUNDS, false, [](const size_t) noexcept -> uint64_t {
			// Разбираемый ответ маршрутизатора
			proto::portmap::ssdp_t::answer_t got;
			// Код ошибки разбора ответа
			proto::portmap::ssdp_t::error_t reason = proto::portmap::ssdp_t::error_t::NONE;
			// Выводим размер разобранного адреса описания устройства
			return (ssdp.parse(ssdpAnswer(), got, reason) ? static_cast <uint64_t> (got.location.size()) : 0);
		});
		// Выводим результат измерения
		return speed(outcome);
	}
	/**
	 * @brief Функция прогона сценария сборки вызова службы по договору SOAP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t soapBuild() noexcept {
		// Объект кодека договора SOAP
		static proto::portmap::soap_t soap(framework(), logger());
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(TEXT_ROUNDS, false, [](const size_t) noexcept -> uint64_t {
			// Выводим размер собранного вызова службы устройства
			return static_cast <uint64_t> (soap.request(
				"urn:schemas-upnp-org:service:WANIPConnection:1", "AddPortMapping", soapArguments()
			).size());
		});
		// Выводим результат измерения
		return speed(outcome);
	}
	/**
	 * @brief Функция прогона сценария разбора ответа службы по договору SOAP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t soapParse() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект кодека договора SOAP
		static proto::portmap::soap_t soap(framework(), logger());
		// Разбираемый ответ службы устройства
		static const string answer = soap.request(
			"urn:schemas-upnp-org:service:WANIPConnection:1", "AddPortMappingResponse", soapArguments()
		);
		/**
		 * Если разбор образца ответа отказывает
		 */
		{
			// Разбираемый ответ службы устройства
			proto::portmap::soap_t::answer_t got;
			// Код ошибки разбора ответа
			proto::portmap::soap_t::error_t reason = proto::portmap::soap_t::error_t::NONE;
			/**
			 * Если разбор образца ответа выполнить не удалось
			 */
			if(!soap.parse(answer, got, reason)){
				// Помечаем измерение как не выполненное
				result.skipped = true;
				// Устанавливаем причину, по которой измерение не выполнялось
				result.reason = "разбор образца ответа службы отказал";
				// Выводим результат измерения
				return result;
			}
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(TEXT_ROUNDS, false, [](const size_t) noexcept -> uint64_t {
			// Разбираемый ответ службы устройства
			proto::portmap::soap_t::answer_t got;
			// Код ошибки разбора ответа
			proto::portmap::soap_t::error_t reason = proto::portmap::soap_t::error_t::NONE;
			// Выводим количество разобранных доводов ответа службы
			return (soap.parse(answer, got, reason) ? static_cast <uint64_t> (got.arguments.size()) : 0);
		});
		// Выводим результат измерения
		return speed(outcome);
	}
	/**
	 * @brief Функция прогона сценария разбора описания устройства
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t deviceParse() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект кодека описания устройства
		static proto::portmap::device_t device(framework(), logger());
		/**
		 * Если разбор образца описания отказывает
		 */
		{
			// Разбираемое описание устройства
			proto::portmap::device_t::description_t got;
			// Код ошибки разбора описания
			proto::portmap::device_t::error_t reason = proto::portmap::device_t::error_t::NONE;
			/**
			 * Если разбор образца описания выполнить не удалось
			 */
			if(!device.parse(description(), got, reason)){
				// Помечаем измерение как не выполненное
				result.skipped = true;
				// Устанавливаем причину, по которой измерение не выполнялось
				result.reason = "разбор образца описания устройства отказал";
				// Выводим результат измерения
				return result;
			}
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(TEXT_ROUNDS, false, [](const size_t) noexcept -> uint64_t {
			// Разбираемое описание устройства
			proto::portmap::device_t::description_t got;
			// Код ошибки разбора описания
			proto::portmap::device_t::error_t reason = proto::portmap::device_t::error_t::NONE;
			// Выводим количество разобранных служб устройства
			return (device.parse(description(), got, reason) ? static_cast <uint64_t> (got.services.size()) : 0);
		});
		// Выводим результат измерения
		return speed(outcome);
	}

	/**
	 * Выполняем регистрацию сценария сборки просьбы договора PCP
	 */
	static const bool PCP_BUILD_REGISTERED = awh::benchmark::add(
		"proto/portmap: сборка просьбы PCP", "оп./с", PCP_BUILD_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, pcpBuildSpeed
	);
	/**
	 * Выполняем регистрацию сценария выделений памяти на сборку просьбы договора PCP
	 */
	static const bool PCP_MEMORY_REGISTERED = awh::benchmark::add(
		"proto/portmap: выделения на просьбу PCP", "выд./оп.", BINARY_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, pcpBuildMemory
	);
	/**
	 * Выполняем регистрацию сценария разбора ответа договора PCP
	 */
	static const bool PCP_PARSE_REGISTERED = awh::benchmark::add(
		"proto/portmap: разбор ответа PCP", "оп./с", PCP_PARSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, pcpParse
	);
	/**
	 * Выполняем регистрацию сценария сборки просьбы договора NAT-PMP
	 */
	static const bool NATPMP_BUILD_REGISTERED = awh::benchmark::add(
		"proto/portmap: сборка просьбы NAT-PMP", "оп./с", NATPMP_BUILD_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, natpmpBuildSpeed
	);
	/**
	 * Выполняем регистрацию сценария выделений памяти на сборку просьбы договора NAT-PMP
	 */
	static const bool NATPMP_MEMORY_REGISTERED = awh::benchmark::add(
		"proto/portmap: выделения на просьбу NAT-PMP", "выд./оп.", BINARY_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, natpmpBuildMemory
	);
	/**
	 * Выполняем регистрацию сценария разбора ответа договора NAT-PMP
	 */
	static const bool NATPMP_PARSE_REGISTERED = awh::benchmark::add(
		"proto/portmap: разбор ответа NAT-PMP", "оп./с", NATPMP_PARSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, natpmpParse
	);
	/**
	 * Выполняем регистрацию сценария сборки просьбы обнаружения по договору SSDP
	 */
	static const bool SSDP_BUILD_REGISTERED = awh::benchmark::add(
		"proto/portmap: сборка просьбы SSDP", "оп./с", SSDP_BUILD_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ssdpBuild
	);
	/**
	 * Выполняем регистрацию сценария разбора ответа обнаружения по договору SSDP
	 */
	static const bool SSDP_PARSE_REGISTERED = awh::benchmark::add(
		"proto/portmap: разбор ответа SSDP", "оп./с", SSDP_PARSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ssdpParse
	);
	/**
	 * Выполняем регистрацию сценария сборки вызова службы по договору SOAP
	 */
	static const bool SOAP_BUILD_REGISTERED = awh::benchmark::add(
		"proto/portmap: сборка вызова SOAP", "оп./с", SOAP_BUILD_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, soapBuild
	);
	/**
	 * Выполняем регистрацию сценария разбора ответа службы по договору SOAP
	 */
	static const bool SOAP_PARSE_REGISTERED = awh::benchmark::add(
		"proto/portmap: разбор ответа SOAP", "оп./с", SOAP_PARSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, soapParse
	);
	/**
	 * Выполняем регистрацию сценария разбора описания устройства
	 */
	static const bool DEVICE_PARSE_REGISTERED = awh::benchmark::add(
		"proto/portmap: разбор описания устройства", "оп./с", DEVICE_PARSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, deviceParse
	);
};
