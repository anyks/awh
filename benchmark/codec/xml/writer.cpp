/**
 * @file: writer.cpp
 * @date: 2026-08-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения записи текста разметки потоком — сборка узлов и атрибутов,
 *        экранирование содержимого, назначение префиксов пространств имён и задержка
 *        сборки вызова службы по договору SOAP
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков контейнера XML
 */
#include "xml.hpp"

/**
 * Подключаем заголовочный файл записи текста разметки
 */
#include <codec/xml/writer.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера XML
 */
using namespace awh::benchmark::markup;

/**
 * @brief Внутренние параметры и сценарии бенчмарков записи текста разметки
 *
 * @details Записью собираются исходящие обращения к службам, и мерилом ей служит
 *          не столько пропускная способность, сколько задержка: вызов службы
 *          перенаправления портов - это документ на единицы килобайт, собираемый
 *          по одному на каждое обращение к устройству. Измеряется потоковый вид
 *          записи, а не запись готового дерева: обращения к службам собираются
 *          именно потоком, дерево для них не заводится вовсе
 *
 */
namespace {
	/**
	 * @brief Количество собираемых мелких документов
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество собираемых документов с преобладанием одного вида разметки
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 200;
	/**
	 * @brief Количество узлов собираемого документа с преобладанием одного вида разметки
	 *
	 */
	static constexpr size_t FOCUSED_COUNT = 20000;

	/**
	 * @brief Порог пропускной способности записи узлов с атрибутами
	 *
	 * @details Пороги откалиброваны по замеру с запасом, покрывающим разброс времени
	 *          между прогонами и разницу в быстродействии машин отладочных стендов.
	 *          Стеречь им положено не потерю процентов, а возврат к записи с иным
	 *          порядком стоимости - вроде посимвольной дозаписи содержимого или
	 *          повторного прохода по накопленному тексту при каждом закрытии узла
	 *
	 * @warning Пороги откалиброваны по самому медленному из отладочных стендов
	 *          (OpenBSD), а не по рабочей машине: между ними разница восьмикратная,
	 *          и порог, снятый с рабочей машины, отказывал бы на стендах всякий раз
	 *
	 */
	static constexpr double WRITE_ELEMENTS_THRESHOLD = 10.0;
	/**
	 * @brief Порог пропускной способности записи содержимого с экранированием
	 *
	 * @details Содержимое здесь взято таким, что экранирования требует каждый
	 *          третий знак: именно на нём и видна стоимость самого экранирования,
	 *          а не переноса содержимого
	 *
	 */
	static constexpr double WRITE_ESCAPE_THRESHOLD = 20.0;
	/**
	 * @brief Порог пропускной способности записи с назначением префиксов
	 *
	 * @details Всякому узлу здесь назначается собственное пространство имён, и
	 *          записи приходится подбирать ему префикс среди объявленных. Подбор
	 *          этот обязан вестись раскладкой, а не перебором объявлений
	 *
	 */
	static constexpr double WRITE_NAMESPACES_THRESHOLD = 10.0;
	/**
	 * @brief Порог задержки сборки вызова службы по договору SOAP в микросекундах
	 *
	 */
	static constexpr double WRITE_SOAP_LATENCY_THRESHOLD = 35.0;
	/**
	 * @brief Порог количества выделений памяти на сборку вызова службы
	 *
	 * @details Показатель воспроизводим до единиц и потому годится в порог куда
	 *          больше времени. Рост его означает, что какое-то хранилище записи
	 *          перестало переиспользоваться и заводится заново на каждый узел
	 *
	 * @warning Воспроизводим он в пределах одной системы, но не между системами:
	 *          у NetBSD показатель этот равен двадцати двум против шестнадцати у
	 *          macOS, OpenBSD и FreeBSD, потому что короткая строка укладывается
	 *          внутрь объекта строки по-разному в разных стандартных библиотеках.
	 *          Порог назначен по наибольшему из снятых на стендах
	 *
	 */
	static constexpr double WRITE_ALLOCATIONS_THRESHOLD = 32.0;

	/**
	 * @brief Функция получения эталонного содержимого, требующего экранирования
	 *
	 * @note Знаки, требующие экранирования, расставлены здесь часто намеренно:
	 *       на содержимом, где их нет вовсе, измерялся бы перенос содержимого, а
	 *       не работа самого экранирования
	 *
	 * @return эталонное содержимое узла разметки
	 *
	 */
	static const string & escapable() noexcept {
		// Собираемое эталонное содержимое
		static const string result = []() noexcept -> string {
			// Собираемое содержимое узла разметки
			string result;
			/**
			 * Выполняем сборку содержимого, требующего экранирования
			 */
			for(size_t i = 0; i < 64; i++)
				// Выполняем добавление очередного куска содержимого
				result.append("a<b>c&d\"e'f ");
			// Выводим собранное содержимое узла разметки
			return result;
		}();
		// Выводим эталонное содержимое узла разметки
		return result;
	}
	/**
	 * @brief Функция сборки вызова службы по договору SOAP
	 *
	 * @note Собирается здесь тот самый вызов, каким модуль перенаправления портов
	 *       обращается к службе устройства: он и есть рабочий случай записи
	 *
	 * @return объём записанного текста разметки
	 *
	 */
	static uint64_t soapCall() noexcept {
		// Объект записи текста разметки
		awh::codec::xml::writer_t writer;
		// Выполняем запись объявления разметки
		writer.declaration();
		// Выполняем открытие узла оболочки обращения
		writer.open("Envelope", "http://schemas.xmlsoap.org/soap/envelope/");
		// Выполняем запись атрибута правил кодирования обращения
		writer.attribute("encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/", "http://schemas.xmlsoap.org/soap/envelope/");
		// Выполняем открытие узла тела обращения
		writer.open("Body", "http://schemas.xmlsoap.org/soap/envelope/");
		// Выполняем открытие узла вызываемого действия
		writer.open("AddPortMapping", "urn:schemas-upnp-org:service:WANIPConnection:1");
		// Выполняем запись обозначения удалённого узла отображения
		writer.element("NewRemoteHost", "");
		// Выполняем запись внешнего порта отображения
		writer.element("NewExternalPort", "8080");
		// Выполняем запись договора передачи отображения
		writer.element("NewProtocol", "TCP");
		// Выполняем запись внутреннего порта отображения
		writer.element("NewInternalPort", "8080");
		// Выполняем запись адреса внутреннего узла отображения
		writer.element("NewInternalClient", "192.168.1.10");
		// Выполняем запись признака включения отображения
		writer.element("NewEnabled", "1");
		// Выполняем запись описания отображения
		writer.element("NewPortMappingDescription", "AWH <port> mapping & test");
		// Выполняем запись времени жизни отображения
		writer.element("NewLeaseDuration", "3600");
		// Выполняем закрытие узла вызываемого действия
		writer.close();
		// Выполняем закрытие узла тела обращения
		writer.close();
		// Выполняем закрытие узла оболочки обращения
		writer.close();
		/**
		 * Если запись текста разметки завершена неудачно
		 */
		if(!writer.complete())
			// Выводим нулевой объём записанного текста
			return 0;
		// Выводим объём записанного текста разметки
		return static_cast <uint64_t> (writer.text().size());
	}
	/**
	 * @brief Функция прогона сценария записи узлов с атрибутами
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeElements() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объём записанного текста разметки одного документа
		static size_t volume = 0;
		/**
		 * @brief Функция записи документа с узлами и атрибутами
		 *
		 * @return объём записанного текста разметки
		 *
		 */
		const auto write = []() noexcept -> uint64_t {
			// Объект записи текста разметки
			awh::codec::xml::writer_t writer;
			// Выполняем открытие корневого узла разметки
			writer.open("root");
			/**
			 * Выполняем запись узлов с атрибутами
			 */
			for(size_t i = 0; i < FOCUSED_COUNT; i++){
				// Выполняем открытие очередного узла разметки
				writer.open("item");
				// Выполняем запись обозначения узла разметки
				writer.attribute("id", "1024");
				// Выполняем запись имени узла разметки
				writer.attribute("name", "value");
				// Выполняем закрытие очередного узла разметки
				writer.close();
			}
			// Выполняем закрытие корневого узла разметки
			writer.close();
			/**
			 * Если запись текста разметки завершена неудачно
			 */
			if(!writer.complete())
				// Выводим нулевой объём записанного текста
				return 0;
			// Выводим объём записанного текста разметки
			return static_cast <uint64_t> (writer.text().size());
		};
		// Запоминаем объём записанного текста разметки одного документа
		volume = static_cast <size_t> (write());
		/**
		 * Если запись эталонного документа отказывает
		 */
		if(volume == 0){
			// Помечаем измерение как не выполненное
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = "запись эталонного документа с узлами отказала";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(volume, FOCUSED_ROUNDS, write);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи содержимого с экранированием
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeEscape() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Записываемое содержимое узлов разметки
		const string & content = escapable();
		// Объём записанного текста разметки одного документа
		size_t volume = 0;
		/**
		 * @brief Функция записи документа с экранируемым содержимым
		 *
		 * @return объём записанного текста разметки
		 *
		 */
		const auto write = [&content]() noexcept -> uint64_t {
			// Объект записи текста разметки
			awh::codec::xml::writer_t writer;
			// Выполняем открытие корневого узла разметки
			writer.open("root");
			/**
			 * Выполняем запись узлов с экранируемым содержимым
			 */
			for(size_t i = 0; i < (FOCUSED_COUNT / 16); i++)
				// Выполняем запись очередного узла с содержимым
				writer.element("item", content);
			// Выполняем закрытие корневого узла разметки
			writer.close();
			/**
			 * Если запись текста разметки завершена неудачно
			 */
			if(!writer.complete())
				// Выводим нулевой объём записанного текста
				return 0;
			// Выводим объём записанного текста разметки
			return static_cast <uint64_t> (writer.text().size());
		};
		// Запоминаем объём записанного текста разметки одного документа
		volume = static_cast <size_t> (write());
		/**
		 * Если запись эталонного документа отказывает
		 */
		if(volume == 0){
			// Помечаем измерение как не выполненное
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = "запись эталонного документа с экранированием отказала";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(volume, FOCUSED_ROUNDS, write);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи с назначением префиксов
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeNamespaces() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объём записанного текста разметки одного документа
		size_t volume = 0;
		/**
		 * @brief Функция записи документа с пространствами имён
		 *
		 * @return объём записанного текста разметки
		 *
		 */
		const auto write = []() noexcept -> uint64_t {
			// Объект записи текста разметки
			awh::codec::xml::writer_t writer;
			// Выполняем открытие корневого узла разметки
			writer.open("root", "urn:example:root");
			/**
			 * Выполняем запись узлов в разных пространствах имён
			 *
			 * @note Пространства имён чередуются намеренно: записи приходится
			 *       подбирать префикс среди объявленных при каждом узле, а не
			 *       однажды объявить его у корня и забыть
			 */
			for(size_t i = 0; i < (FOCUSED_COUNT / 4); i++){
				// Выполняем открытие очередного узла разметки
				writer.open("item", (((i % 2) == 0) ? "urn:example:first" : "urn:example:second"));
				// Выполняем запись атрибута узла в отдельном пространстве имён
				writer.attribute("key", "value", "urn:example:attribute");
				// Выполняем закрытие очередного узла разметки
				writer.close();
			}
			// Выполняем закрытие корневого узла разметки
			writer.close();
			/**
			 * Если запись текста разметки завершена неудачно
			 */
			if(!writer.complete())
				// Выводим нулевой объём записанного текста
				return 0;
			// Выводим объём записанного текста разметки
			return static_cast <uint64_t> (writer.text().size());
		};
		// Запоминаем объём записанного текста разметки одного документа
		volume = static_cast <size_t> (write());
		/**
		 * Если запись эталонного документа отказывает
		 */
		if(volume == 0){
			// Помечаем измерение как не выполненное
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = "запись эталонного документа с пространствами имён отказала";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(volume, FOCUSED_ROUNDS, write);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки сборки вызова службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencySoap() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объём собранного вызова службы
		const size_t volume = static_cast <size_t> (::soapCall());
		/**
		 * Если сборка вызова службы отказывает
		 */
		if(volume == 0){
			// Помечаем измерение как не выполненное
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = "сборка вызова службы по договору SOAP отказала";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(volume, SMALL_ROUNDS, ::soapCall);
		// Устанавливаем измеренное значение
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на сборку вызова службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объём собранного вызова службы
		const size_t volume = static_cast <size_t> (::soapCall());
		/**
		 * Если сборка вызова службы отказывает
		 */
		if(volume == 0){
			// Помечаем измерение как не выполненное
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = "сборка вызова службы по договору SOAP отказала";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(volume, SMALL_ROUNDS, ::soapCall);
		// Устанавливаем измеренное значение
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария записи узлов с атрибутами
	 */
	static const bool ELEMENTS_REGISTERED = awh::benchmark::add(
		"codec/xml: запись узлов с атрибутами", "МБ/с", WRITE_ELEMENTS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeElements
	);
	/**
	 * Выполняем регистрацию сценария записи содержимого с экранированием
	 */
	static const bool ESCAPE_REGISTERED = awh::benchmark::add(
		"codec/xml: запись с экранированием", "МБ/с", WRITE_ESCAPE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeEscape
	);
	/**
	 * Выполняем регистрацию сценария записи с назначением префиксов
	 */
	static const bool NAMESPACES_REGISTERED = awh::benchmark::add(
		"codec/xml: запись с пространствами имён", "МБ/с", WRITE_NAMESPACES_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeNamespaces
	);
	/**
	 * Выполняем регистрацию сценария задержки сборки вызова службы
	 */
	static const bool SOAP_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/xml: задержка сборки вызова SOAP", "мкс/док.", WRITE_SOAP_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencySoap
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на сборку вызова службы
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/xml: выделения на сборку вызова SOAP", "выд./док.", WRITE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, writeAllocations
	);
};
