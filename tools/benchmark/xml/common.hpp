/**
 * @file common.hpp
 * @date 2026-08-02
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
 * @brief Общее окружение эталонных стендов сравнения контейнера XML —
 *        эталонные тексты разметки, параметры нагрузки, учёт выделений памяти,
 *        разбор параметров запуска и вывод результатов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_XML__
#define __AWH_BENCHMARK_RIVAL_XML__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <string>
#include <chrono>

/**
 * @brief Пространство имён эталонных стендов сравнения контейнера XML
 *
 * @details Эталонные тексты разметки и параметры нагрузки обязаны совпадать со
 *          сценариями `benchmark/codec/xml` библиотеки AWH: сравниваются
 *          реализации разбора, а не разные объёмы работы, поэтому любое
 *          расхождение здесь обесценивает отчёт целиком
 *
 */
namespace rival {
	/**
	 * @brief Размер эталонного крупного документа в октетах
	 *
	 */
	static constexpr size_t LARGE_SIZE = (16 * 1024 * 1024);

	/**
	 * @brief Размер эталонных документов с преобладанием одного вида разметки в октетах
	 *
	 */
	static constexpr size_t FOCUSED_SIZE = (4 * 1024 * 1024);

	/**
	 * @brief Глубина вложенности эталонного глубоко вложенного документа
	 *
	 */
	static constexpr uint32_t NESTED_DEPTH = 250;

	/**
	 * @brief Количество прогонов сценариев малых документов
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;

	/**
	 * @brief Количество прогонов сценария крупного документа
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;

	/**
	 * @brief Количество прогонов сценариев с преобладанием одного вида разметки
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 20;

	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	struct outcome_t {
		// Количество разобранных документов
		size_t documents;
		// Количество разобранных октетов
		size_t bytes;
		// Длительность замера в секундах
		double seconds;
	};

	/**
	 * @brief Контрольная сумма текстового содержимого, прочитанного стендом
	 *
	 * @details Складывается из октетов содержимого узлов. Совпадение суммы у всех
	 * стендов означает, что они прочитали одно и то же содержимое, а не разный
	 * его объём
	 *
	 * @note Имена узлов и атрибутов в сумму не входят намеренно: реализации выдают
	 * их по-разному - местным именем с разрешённым пространством имён, именем с
	 * приставкой либо развёрнутой парой из обозначения пространства имён и имени.
	 * Это разница моделей разбора, а не разный объём работы, и складывать её в
	 * одну сумму значило бы объявить расхождением то, что расхождением не является
	 *
	 */
	static volatile uint64_t checksum = 0;

	/**
	 * @brief Количество узлов разметки, обработанных стендом
	 *
	 */
	static volatile uint64_t nodes = 0;

	/**
	 * @brief Приёмник имён узлов и атрибутов
	 *
	 * @details Имена в контрольную сумму не входят, однако читать их стенд обязан:
	 * без чтения часть реализаций работу по их выдаче попросту не выполнит, и
	 * сравнение потеряет смысл
	 *
	 */
	static volatile uint64_t sink = 0;

	/**
	 * @brief Метод чтения имени узла либо атрибута
	 *
	 * @param buffer читаемая последовательность знаков
	 * @param size   размер читаемой последовательности
	 *
	 */
	static inline void touch(const void * buffer, const size_t size) noexcept {
		// Накапливаемая сумма октетов
		size_t sum = 0;
		/**
		 * Если читаемая последовательность передана
		 */
		if((buffer != nullptr) && (size > 0)){
			// Получаем читаемую последовательность знаков
			const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer);
			/**
			 * Выполняем перебор всех октетов последовательности
			 */
			for(size_t i = 0; i < size; i++)
				// Выполняем накопление суммы октетов
				sum += data[i];
		}
		// Выполняем накопление прочитанного в приёмнике
		sink += (sum + size);
	}
	/**
	 * @brief Метод учёта обработанного узла разметки
	 *
	 */
	static inline void node() noexcept {
		// Выполняем учёт обработанного узла разметки
		nodes += 1;
	}
	/**
	 * @brief Метод учёта прочитанного содержимого в контрольной сумме
	 *
	 * @param buffer учитываемая последовательность знаков
	 * @param size   размер учитываемой последовательности
	 *
	 */
	static inline void consume(const void * buffer, const size_t size) noexcept {
		// Накапливаемая сумма октетов
		size_t sum = 0;
		/**
		 * Если учитываемая последовательность передана
		 */
		if((buffer != nullptr) && (size > 0)){
			// Получаем учитываемую последовательность знаков
			const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer);
			/**
			 * Выполняем перебор всех октетов последовательности
			 */
			for(size_t i = 0; i < size; i++)
				// Выполняем накопление суммы октетов
				sum += data[i];
		}
		// Выполняем учёт выполненной работы в контрольной сумме
		checksum += (sum + size);
	}
	/**
	 * @brief Метод получения десятичной записи числа
	 *
	 * @param value записываемое число
	 * @return      десятичная запись числа
	 *
	 */
	static inline std::string number(const uint32_t value) noexcept {
		// Выводим десятичную запись числа
		return std::to_string(value);
	}
	/**
	 * @brief Метод получения эталонного ответа по договору SOAP
	 *
	 * @return эталонный текст разметки
	 *
	 */
	static inline const std::string & soap() noexcept {
		// Эталонный текст разметки
		static const std::string result =
			"<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
			" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>"
			"<u:GetExternalIPAddressResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
			"<NewExternalIPAddress>203.0.113.7</NewExternalIPAddress>"
			"</u:GetExternalIPAddressResponse></s:Body></s:Envelope>";
		// Выводим эталонный текст разметки
		return result;
	}
	/**
	 * @brief Метод получения эталонного описания устройства по договору UPnP
	 *
	 * @return эталонный текст разметки
	 *
	 */
	static inline const std::string & device() noexcept {
		// Эталонный текст разметки
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст разметки
			std::string result =
				"<?xml version=\"1.0\"?>\n<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
				"<specVersion><major>1</major><minor>0</minor></specVersion>"
				"<device><deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>"
				"<friendlyName>Маршрутизатор</friendlyName><manufacturer>ANYKS</manufacturer>"
				"<modelName>AWH-1000</modelName><UDN>uuid:12345678-1234-1234-1234-123456789012</UDN><serviceList>";
			/**
			 * Выполняем сборку перечня служб устройства
			 */
			for(uint32_t i = 0; i < 12; i++){
				// Получаем порядковый номер очередной службы
				const std::string index = number(i);
				// Выполняем добавление очередной службы устройства
				result.append("<service><serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>"
					"<serviceId>urn:upnp-org:serviceId:WANIPConn").append(index).append("</serviceId>"
					"<controlURL>/ctl/IPConn").append(index).append("</controlURL>"
					"<eventSubURL>/evt/IPConn").append(index).append("</eventSubURL>"
					"<SCPDURL>/gatedesc.xml</SCPDURL></service>");
			}
			// Выполняем завершение текста разметки
			result.append("</serviceList></device></root>");
			// Выводим собранный текст разметки
			return result;
		}();
		// Выводим эталонный текст разметки
		return result;
	}
	/**
	 * @brief Метод получения эталонного крупного документа
	 *
	 * @return эталонный текст разметки
	 *
	 */
	static inline const std::string & large() noexcept {
		// Эталонный текст разметки
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст разметки
			std::string result = "<?xml version=\"1.0\"?><site><regions>";
			// Порядковый номер очередного узла
			uint32_t i = 0;
			/**
			 * Выполняем сборку текста разметки заданного размера
			 */
			while(result.size() < LARGE_SIZE){
				// Выполняем добавление очередного узла разметки
				result.append("<item id=\"item").append(number(i)).append("\" featured=\"no\" category=\"c")
					.append(number(i % 97)).append("\"><location>Москва</location><quantity>")
					.append(number(i % 13)).append("</quantity><name>Товар ").append(number(i))
					.append("</name><payment>Creditcard, Cash</payment><description><text>"
					"Описание товара с некоторым количеством слов, чтобы содержимое узла было похоже на настоящее"
					"</text></description><shipping>Will ship internationally</shipping></item>");
				// Выполняем переход к следующему узлу разметки
				i++;
			}
			// Выполняем завершение текста разметки
			result.append("</regions></site>");
			// Выводим собранный текст разметки
			return result;
		}();
		// Выводим эталонный текст разметки
		return result;
	}
	/**
	 * @brief Метод получения эталонного документа с преобладанием атрибутов
	 *
	 * @return эталонный текст разметки
	 *
	 */
	static inline const std::string & attributes() noexcept {
		// Эталонный текст разметки
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст разметки
			std::string result = "<r>";
			// Порядковый номер очередного узла
			uint32_t i = 0;
			/**
			 * Выполняем сборку текста разметки заданного размера
			 */
			while(result.size() < FOCUSED_SIZE){
				// Выполняем добавление очередного узла разметки
				result.append("<n a=\"1\" b=\"2\" c=\"3\" d=\"4\" e=\"5\" f=\"6\" g=\"7\" h=\"8\" i=\"")
					.append(number(i)).append("\"/>");
				// Выполняем переход к следующему узлу разметки
				i++;
			}
			// Выполняем завершение текста разметки
			result.append("</r>");
			// Выводим собранный текст разметки
			return result;
		}();
		// Выводим эталонный текст разметки
		return result;
	}
	/**
	 * @brief Метод получения эталонного документа с преобладанием содержимого
	 *
	 * @return эталонный текст разметки
	 *
	 */
	static inline const std::string & content() noexcept {
		// Эталонный текст разметки
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст разметки
			std::string result = "<r>";
			// Порядковый номер очередного узла
			uint32_t i = 0;
			/**
			 * Выполняем сборку текста разметки заданного размера
			 */
			while(result.size() < FOCUSED_SIZE){
				// Выполняем добавление очередного узла разметки
				result.append("<p>Текст узла с ссылками &amp; и &lt;экранированием&gt;, номер ")
					.append(number(i)).append(", а также обычные слова без всякой разметки внутри узла.</p>");
				// Выполняем переход к следующему узлу разметки
				i++;
			}
			// Выполняем завершение текста разметки
			result.append("</r>");
			// Выводим собранный текст разметки
			return result;
		}();
		// Выводим эталонный текст разметки
		return result;
	}
	/**
	 * @brief Метод получения эталонного глубоко вложенного документа
	 *
	 * @return эталонный текст разметки
	 *
	 */
	static inline const std::string & nested() noexcept {
		// Эталонный текст разметки
		static const std::string result = []() noexcept -> std::string {
			// Собираемый текст разметки
			std::string result = "<r>";
			/**
			 * Выполняем сборку открывающих меток узлов разметки
			 */
			for(uint32_t i = 0; i < NESTED_DEPTH; i++)
				// Выполняем добавление очередной открывающей метки
				result.append("<n").append(number(i % 10)).append(">");
			// Выполняем добавление содержимого самого вложенного узла
			result.append("глубина");
			/**
			 * Выполняем сборку закрывающих меток узлов разметки
			 */
			for(uint32_t i = NESTED_DEPTH; i > 0; i--)
				// Выполняем добавление очередной закрывающей метки
				result.append("</n").append(number((i - 1) % 10)).append(">");
			// Выполняем завершение текста разметки
			result.append("</r>");
			// Выводим собранный текст разметки
			return result;
		}();
		// Выводим эталонный текст разметки
		return result;
	}
	/**
	 * @brief Шаблон типа разбираемого документа
	 *
	 * @tparam Subject тип разбираемого документа
	 *
	 */
	template <typename Subject>
	/**
	 * @brief Метод прогона одного сценария разбора
	 *
	 * @details Первый прогон выполняется до начала замера: он выводит накопители
	 * разбора на рабочий объём, и его стоимость к установившемуся режиму отношения
	 * не имеет
	 *
	 * @param subject функция разбора одного документа
	 * @param text    разбираемый текст разметки
	 * @param rounds  количество прогонов разбора
	 * @param output  ссылка на итоги прогона сценария
	 * @return        признак успешного прогона
	 *
	 */
	static bool parsing(Subject && subject, const std::string & text, const size_t rounds, outcome_t & output) noexcept {
		// Выполняем прогрев разбора вне замера
		if(!subject(text))
			// Выводим признак неудачного прогона
			return false;
		// Запоминаем время начала замера
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем прогон разбора заданное количество раз
		 */
		for(size_t i = 0; i < rounds; i++){
			/**
			 * Если разбор документа выполнить не удалось
			 */
			if(!subject(text))
				// Выводим признак неудачного прогона
				return false;
		}
		// Запоминаем время окончания замера
		const auto finish = std::chrono::steady_clock::now();
		// Запоминаем количество разобранных документов
		output.documents = rounds;
		// Запоминаем количество разобранных октетов
		output.bytes = (text.size() * rounds);
		// Запоминаем длительность замера
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим признак успешного прогона
		return true;
	}
	/**
	 * @brief Метод получения пропускной способности разбора
	 *
	 * @param outcome итоги прогона сценария
	 * @return        пропускная способность в мегабайтах в секунду
	 *
	 */
	static inline double megabytes(const outcome_t & outcome) noexcept {
		/**
		 * Если замер не состоялся
		 */
		if(outcome.seconds <= 0.0)
			// Выводим нулевую пропускную способность
			return 0.0;
		// Выводим пропускную способность разбора
		return ((static_cast <double> (outcome.bytes) / (1024.0 * 1024.0)) / outcome.seconds);
	}
	/**
	 * @brief Метод вывода результата прогона сценария
	 *
	 * @param name    название сценария
	 * @param outcome итоги прогона сценария
	 *
	 */
	static inline void report(const char * name, const outcome_t & outcome) noexcept {
		// Выводим результат прогона сценария
		::printf("%-24s %12.2f MB/s %10zu docs\n", name, megabytes(outcome), outcome.documents);
	}
	/**
	 * @brief Метод вывода сообщения о пропуске сценария
	 *
	 * @param name   название сценария
	 * @param reason причина пропуска сценария
	 *
	 */
	static inline void skip(const char * name, const char * reason) noexcept {
		// Выводим сообщение о пропуске сценария
		::printf("%-24s %12s      %s\n", name, "skipped", reason);
	}
	/**
	 * @brief Метод проверки отбора сценария
	 *
	 * @param name   название сценария
	 * @param filter отбор сценариев по вхождению в название
	 * @return       признак отбора сценария
	 *
	 */
	static inline bool selected(const char * name, const char * filter) noexcept {
		// Выводим признак отбора сценария
		return ((filter == nullptr) || (::strstr(name, filter) != nullptr));
	}
	/**
	 * @brief Метод получения отбора сценариев из параметров запуска
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 * @return     отбор сценариев по вхождению в название
	 *
	 */
	static inline const char * filter(const int32_t argc, char ** argv) noexcept {
		/**
		 * Выполняем перебор всех параметров запуска
		 */
		for(int32_t i = 1; i < argc; i++){
			/**
			 * Если параметром является отбор сценариев
			 */
			if(::strncmp(argv[i], "--filter=", 9) == 0)
				// Выводим отбор сценариев по вхождению в название
				return (argv[i] + 9);
		}
		// Выводим пустой отбор сценариев
		return nullptr;
	}
	/**
	 * @brief Метод вывода контрольной суммы работы, выполненной стендом
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 *
	 */
	static inline void digest(const int32_t argc, char ** argv) noexcept {
		/**
		 * Выполняем перебор всех параметров запуска
		 */
		for(int32_t i = 1; i < argc; i++){
			/**
			 * Если параметром является вывод контрольной суммы
			 */
			if(::strcmp(argv[i], "--checksum") == 0){
				// Выводим контрольную сумму содержимого и количество узлов разметки
				::printf("checksum %llu nodes %llu\n", static_cast <unsigned long long> (checksum), static_cast <unsigned long long> (nodes));
				// Выходим из перебора параметров запуска
				return;
			}
		}
	}
};

#endif // __AWH_BENCHMARK_RIVAL_XML__
