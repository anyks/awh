/**
 * @file xml.cpp
 * @date 2026-08-01
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
 * @brief Реализация общего окружения бенчмарков контейнера XML — эталонные тексты разметки
 *        всех путей разбора и средства представления итогов замера
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков контейнера XML
 */
#include "xml.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние параметры сборки эталонных текстов разметки
 *
 */
namespace {
	/**
	 * @brief Размер эталонного крупного документа в октетах
	 *
	 * @details Размер выбран заведомо превосходящим кэш последнего уровня: разбор
	 *          документа, целиком укладывающегося в кэш, показывает скорость работы
	 *          с кэшем, а не установившуюся пропускную способность
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
	 * @brief Функция получения десятичной записи числа
	 *
	 * @note Запись выполняется средствами стандартной библиотеки намеренно: эталонные
	 *       тексты собираются однократно до замера, и стоимость их сборки в замер не
	 *       входит. Втягивать сюда модуль длинной арифметики незачем - он измеряется
	 *       своим набором бенчмарков
	 *
	 * @param value записываемое число
	 * @return      десятичная запись числа
	 *
	 */
	static string number(const uint32_t value) noexcept {
		// Выводим десятичную запись числа
		return to_string(value);
	}
};

/**
 * @brief Функция формирования сведений о прогоне сценария
 *
 * @param output итоги прогона сценария
 * @return       сведения о прогоне для вывода
 *
 */
string awh::benchmark::markup::details(const outcome_t & output) noexcept {
	// Собираемые сведения о прогоне
	string result;
	// Выполняем добавление количества разобранных документов
	result.append(::number(static_cast <uint32_t> (output.operations))).append(" док., ");
	// Выполняем добавление количества выделений памяти на один документ
	result.append(::number(static_cast <uint32_t> (perDocument(output) + 0.5))).append(" выд./док., ");
	// Выполняем добавление объёма выделенной памяти на один документ
	result.append(::number(static_cast <uint32_t> (output.operations > 0 ? (output.allocated / output.operations) : 0))).append(" окт./док.");
	// Выводим собранные сведения о прогоне
	return result;
}
/**
 * @brief Функция извлечения пропускной способности разбора
 *
 * @param output итоги прогона сценария
 * @return       пропускная способность в мегабайтах в секунду
 *
 */
double awh::benchmark::markup::perSecond(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.seconds <= 0.0)
		// Выводим нулевую пропускную способность
		return 0.0;
	// Выводим пропускную способность разбора
	return ((static_cast <double> (output.bytes) / (1024.0 * 1024.0)) / output.seconds);
}
/**
 * @brief Функция проверки работоспособности учёта выделений памяти
 *
 * @param output итоги прогона сценария
 * @param result заполняемый результат измерения
 * @return       признак работоспособности учёта
 *
 */
bool awh::benchmark::markup::counted(const outcome_t & output, awh::benchmark::result_t & result) noexcept {
	/**
	 * Если счётчик выделений памяти хоть что-нибудь насчитал
	 */
	if(output.allocations > 0)
		// Выводим признак работоспособности учёта
		return true;
	// Устанавливаем признак негодности измерения
	result.invalid = true;
	// Устанавливаем причину негодности измерения
	result.reason = "счётчик выделений памяти молчит - сборка ведётся без замены оператора"
	                " выделения памяти либо стандартная библиотека подключена отдельной"
	                " библиотекой (MinGW: связывать с ключом -static-libstdc++)";
	// Выводим признак неработоспособности учёта
	return false;
}
/**
 * @brief Функция извлечения количества выделений памяти на один документ
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на один документ
 *
 */
double awh::benchmark::markup::perDocument(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на один документ
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения задержки обработки одного документа
 *
 * @param output итоги прогона сценария
 * @return       задержка обработки одного документа в микросекундах
 *
 */
double awh::benchmark::markup::perLatency(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевую задержку обработки документа
		return 0.0;
	// Выводим задержку обработки одного документа
	return ((output.seconds * 1000000.0) / static_cast <double> (output.operations));
}
/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::markup::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонного ответа по договору SOAP
 *
 * @return эталонный текст разметки
 *
 */
const string & awh::benchmark::markup::soap() noexcept {
	// Эталонный текст разметки
	static const string result =
		"<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
		" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>"
		"<u:GetExternalIPAddressResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewExternalIPAddress>203.0.113.7</NewExternalIPAddress>"
		"</u:GetExternalIPAddressResponse></s:Body></s:Envelope>";
	// Выводим эталонный текст разметки
	return result;
}
/**
 * @brief Функция получения эталонного описания устройства по договору UPnP
 *
 * @return эталонный текст разметки
 *
 */
const string & awh::benchmark::markup::device() noexcept {
	/**
	 * @brief Функция сборки эталонного текста разметки
	 *
	 * @return собранный текст разметки
	 *
	 */
	static const string result = []() noexcept -> string {
		// Собираемый текст разметки
		string result =
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
			const string index = ::number(i);
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
 * @brief Функция получения эталонного крупного документа
 *
 * @return эталонный текст разметки
 *
 */
const string & awh::benchmark::markup::large() noexcept {
	/**
	 * @brief Функция сборки эталонного текста разметки
	 *
	 * @return собранный текст разметки
	 *
	 */
	static const string result = []() noexcept -> string {
		// Собираемый текст разметки
		string result = "<?xml version=\"1.0\"?><site><regions>";
		// Порядковый номер очередного узла
		uint32_t i = 0;
		/**
		 * Выполняем сборку текста разметки заданного размера
		 */
		while(result.size() < LARGE_SIZE){
			// Выполняем добавление очередного узла разметки
			result.append("<item id=\"item").append(::number(i)).append("\" featured=\"no\" category=\"c")
				.append(::number(i % 97)).append("\"><location>Москва</location><quantity>")
				.append(::number(i % 13)).append("</quantity><name>Товар ").append(::number(i))
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
 * @brief Функция получения эталонного документа с преобладанием атрибутов
 *
 * @return эталонный текст разметки
 *
 */
const string & awh::benchmark::markup::attributes() noexcept {
	/**
	 * @brief Функция сборки эталонного текста разметки
	 *
	 * @return собранный текст разметки
	 *
	 */
	static const string result = []() noexcept -> string {
		// Собираемый текст разметки
		string result = "<r>";
		// Порядковый номер очередного узла
		uint32_t i = 0;
		/**
		 * Выполняем сборку текста разметки заданного размера
		 */
		while(result.size() < FOCUSED_SIZE){
			// Выполняем добавление очередного узла разметки
			result.append("<n a=\"1\" b=\"2\" c=\"3\" d=\"4\" e=\"5\" f=\"6\" g=\"7\" h=\"8\" i=\"")
				.append(::number(i)).append("\"/>");
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
 * @brief Функция получения эталонного документа с преобладанием содержимого
 *
 * @return эталонный текст разметки
 *
 */
const string & awh::benchmark::markup::content() noexcept {
	/**
	 * @brief Функция сборки эталонного текста разметки
	 *
	 * @return собранный текст разметки
	 *
	 */
	static const string result = []() noexcept -> string {
		// Собираемый текст разметки
		string result = "<r>";
		// Порядковый номер очередного узла
		uint32_t i = 0;
		/**
		 * Выполняем сборку текста разметки заданного размера
		 */
		while(result.size() < FOCUSED_SIZE){
			// Выполняем добавление очередного узла разметки
			result.append("<p>Текст узла с ссылками &amp; и &lt;экранированием&gt;, номер ")
				.append(::number(i)).append(", а также обычные слова без всякой разметки внутри узла.</p>");
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
 * @brief Функция получения эталонного глубоко вложенного документа
 *
 * @return эталонный текст разметки
 *
 */
const string & awh::benchmark::markup::nested() noexcept {
	/**
	 * @brief Функция сборки эталонного текста разметки
	 *
	 * @return собранный текст разметки
	 *
	 */
	static const string result = []() noexcept -> string {
		// Собираемый текст разметки
		string result = "<r>";
		/**
		 * Выполняем сборку открывающих меток узлов разметки
		 */
		for(uint32_t i = 0; i < NESTED_DEPTH; i++)
			// Выполняем добавление очередной открывающей метки
			result.append("<n").append(::number(i % 10)).append(">");
		// Выполняем добавление содержимого самого вложенного узла
		result.append("глубина");
		/**
		 * Выполняем сборку закрывающих меток узлов разметки
		 */
		for(uint32_t i = NESTED_DEPTH; i > 0; i--)
			// Выполняем добавление очередной закрывающей метки
			result.append("</n").append(::number((i - 1) % 10)).append(">");
		// Выполняем завершение текста разметки
		result.append("</r>");
		// Выводим собранный текст разметки
		return result;
	}();
	// Выводим эталонный текст разметки
	return result;
}
