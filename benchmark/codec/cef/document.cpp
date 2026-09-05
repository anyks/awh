/**
 * @file document.cpp
 * @date 2026-09-04
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
 * @brief Бенчмарки события CEF, удерживаемого целиком — укладки разбора в дерево контейнера ABC,
 *        задержки обработки короткой записи, стоимости строгого сличения со словарём и полного
 *        оборота разбора со сборкой
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы бенчмарков
 */
#include "cef.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>

/**
 * @brief Пространство имён сценариев этого файла
 *
 * @note Держится оно безымянным намеренно: сценарии кодеков собираются одной программою
 *
 */
namespace {
	/**
	 * @brief Объект окружения сценариев с отключённым выводом журнала
	 *
	 */
	struct SilentCefDocument {
		// Объект фреймворка сценариев
		awh::fmk_t fmk;
		// Объект журнала сценариев
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		SilentCefDocument() noexcept : log(&this->fmk) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта окружения сценариев
	 *
	 * @return объект окружения сценариев
	 *
	 */
	SilentCefDocument & documentEnvironment() noexcept {
		// Объект окружения сценариев
		static SilentCefDocument env;
		// Выводим объект окружения сценариев
		return env;
	}

	/**
	 * @brief Порог пропускной способности укладки разбора в дерево
	 *
	 * @details Мера: длина эталонной записи, помноженная на число кругов, делённая на
	 *          время прогона. Замерено 04.09.2026 на macOS ARM64 в Release: 42-43 МБ/с,
	 *          05.09.2026 на FreeBSD amd64: 11.93 МБ/с, на Debian 12 amd64: 17.14 МБ/с.
	 *          Порог взят по ДНУ трёх проверенных машин с запасом вчетверо
	 *
	 */
	constexpr double PARSE_DETECTION_THRESHOLD = 3.0;

	/**
	 * @brief Порог пропускной способности укладки записи надзора за системой
	 *
	 * @details Мера та же, что и у записи обнаружения вторжений. Замерено 04.09.2026
	 *          на macOS ARM64 в Release: 38 МБ/с, 05.09.2026 на FreeBSD amd64: 9.64-9.96
	 *          МБ/с, на Debian 12 amd64: 13.69 МБ/с. Порог взят по ДНУ трёх проверенных
	 *          машин с запасом вчетверо: прежнее его значение, 5.0, оставляло на FreeBSD
	 *          запас лишь вдвое
	 *
	 */
	constexpr double PARSE_AUDIT_THRESHOLD = 2.5;

	/**
	 * @brief Порог задержки обработки записи наименьшей длины
	 *
	 * @details Мера: время прогона, делённое на число кругов. Замерено 04.09.2026 на
	 *          macOS ARM64 в Release: 1.6-1.7 мкс на запись, 05.09.2026 на FreeBSD
	 *          amd64: 6.16 мкс, на Debian 12 amd64: 4.22 мкс. Порог взят по ПОТОЛКУ трёх
	 *          проверенных машин с запасом
	 *
	 * @details Показатель этот назначен коротким записям, где решает не пропускная
	 *          способность, а постоянные издержки на заведение хранилищ разбора
	 *
	 */
	constexpr double PARSE_LATENCY_THRESHOLD = 40.0;

	/**
	 * @brief Порог удорожания разбора строгим сличением со словарём
	 *
	 * @details Мера: отношение времени разбора при строгом сличении ко времени разбора
	 *          без него. Замерено 04.09.2026 на macOS ARM64 в Release: 1.01 раза, то
	 *          есть сличение со словарём не стоит ничего
	 *
	 * @details Строгое сличение разыскивает всякий ключ в словаре и проверяет вид его
	 *          значения; удорожание измеряется отношением, ибо от машины оно зависит
	 *          слабее самого времени
	 *
	 */
	constexpr double PARSE_STRICT_THRESHOLD = 3.0;

	/**
	 * @brief Порог задержки полного оборота разбора со сборкой
	 *
	 * @details Мера: время прогона, делённое на число кругов. Замерено 04.09.2026 на
	 *          macOS ARM64 в Release: 15.5-16.5 мкс на запись, 05.09.2026 на FreeBSD
	 *          amd64: 56.6-56.8 мкс, на Debian 12 amd64: 37.44 мкс. Порог взят по ПОТОЛКУ
	 *          трёх проверенных машин с запасом втрое и половиной
	 *
	 */
	constexpr double ROUNDTRIP_LATENCY_THRESHOLD = 200.0;

	/**
	 * @brief Порог расхода выделений памяти на укладку одной записи в дерево
	 *
	 * @details Мера: число выделений памяти за прогон, делённое на число уложенных
	 *          записей. Замерено 04.09.2026 на macOS ARM64 в Release: 70 выделений на
	 *          запись, 05.09.2026 на FreeBSD amd64: 70 - число в число, на Debian 12
	 *          amd64 (libstdc++): 80. Запас держится широким ПО ЗАМЕРУ, а не из
	 *          осторожности: стандартная библиотека расход меняет, и сужение порога по
	 *          libc++ давало бы ложную тревогу на всяком стенде с glibc
	 *
	 */
	constexpr double PARSE_ALLOCATIONS_THRESHOLD = 800.0;

	/**
	 * @brief Функция разбора одной записи событием, удерживаемым целиком
	 *
	 * @param doc  объект события CEF
	 * @param text разбираемая запись CEF
	 * @return     количество пар расширения разобранной записи
	 *
	 */
	size_t digest(awh::codec::cef::document_t & doc, const std::string & text) noexcept {
		// Если разбор записи отказом завершился
		if(!doc.parse(text))
			// Выводим отсутствие пар расширения
			return 0;
		// Выводим количество пар расширения разобранной записи
		return doc.size();
	}

	/**
	 * @brief Функция поверки пригодности эталонной записи сценарию
	 *
	 * @details Числитель меры берётся длиной ЭТАЛОННОЙ записи, помноженной на число
	 *          кругов, а не длиной сделанного. Откажи разбор или сборка - и сценарий
	 *          отчитается о работе, какой не было, тем БЫСТРЕЕ, чем раньше отказ
	 *          наступил. Оттого пригодность поверяется ОДНИМ кругом до замера, а
	 *          негодность объявляется исходом наравне с числом
	 *
	 * @param doc       объект события CEF
	 * @param text      эталонная запись
	 * @param result    заполняемый результат измерения
	 * @param assembled требование обратной сборки записи из дерева
	 * @return          признак пригодности эталонной записи сценарию
	 *
	 */
	bool viable(awh::codec::cef::document_t & doc, const std::string & text, awh::benchmark::result_t & result, const bool assembled = false) noexcept {
		// Если разбор эталонной записи отказом завершился
		if(::digest(doc, text) == 0){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "разбор эталонной записи отказом завершился";
			// Выводим отсутствие пригодности эталонной записи
			return false;
		}
		// Если обратная сборка записи требуется и отказом завершается
		if(assembled && doc.dump().empty()){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "сборка эталонной записи из дерева отказом завершилась";
			// Выводим отсутствие пригодности эталонной записи
			return false;
		}
		// Выводим пригодность эталонной записи сценарию
		return true;
	}

	/**
	 * @brief Функция замера укладки записи обнаружения вторжений в дерево
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t parseDetection() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект события CEF
		awh::codec::cef::document_t doc(&::documentEnvironment().fmk, &::documentEnvironment().log);
		// Получаем эталонную запись обнаружения вторжений
		const std::string & text = awh::benchmark::event::detection();
		// Если эталонная запись сценарию непригодна
		if(!::viable(doc, text, result))
			// Выводим результат измерения
			return result;
		// Выполняем замер укладки эталонной записи в дерево
		const auto output = awh::benchmark::event::measure(text.size(), 10000, [&doc, &text]() noexcept -> size_t {
			// Выводим количество пар расширения разобранной записи
			return ::digest(doc, text);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::event::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную пропускную способность укладки
		result.value = awh::benchmark::event::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера укладки записи надзора за системой в дерево
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t parseAudit() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект события CEF
		awh::codec::cef::document_t doc(&::documentEnvironment().fmk, &::documentEnvironment().log);
		// Получаем эталонную запись надзора за системой
		const std::string & text = awh::benchmark::event::audit();
		// Если эталонная запись сценарию непригодна
		if(!::viable(doc, text, result))
			// Выводим результат измерения
			return result;
		// Выполняем замер укладки эталонной записи в дерево
		const auto output = awh::benchmark::event::measure(text.size(), 2000, [&doc, &text]() noexcept -> size_t {
			// Выводим количество пар расширения разобранной записи
			return ::digest(doc, text);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::event::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную пропускную способность укладки
		result.value = awh::benchmark::event::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера задержки обработки записи наименьшей длины
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t parseLatency() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект события CEF
		awh::codec::cef::document_t doc(&::documentEnvironment().fmk, &::documentEnvironment().log);
		// Получаем эталонную запись наименьшей длины
		const std::string & text = awh::benchmark::event::minimal();
		// Выполняем замер укладки эталонной записи в дерево
		const auto output = awh::benchmark::event::measure(text.size(), 20000, [&doc, &text]() noexcept -> size_t {
			// Если разбор записи отказом завершился
			if(!doc.parse(text))
				// Выводим отсутствие полей заголовка
				return 0;
			// Выводим количество полей заголовка разобранной записи
			return doc.keys("/header").size();
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::event::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную задержку обработки одной записи
		result.value = awh::benchmark::event::perLatency(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера удорожания разбора строгим сличением со словарём
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t parseStrict() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект события CEF
		awh::codec::cef::document_t doc(&::documentEnvironment().fmk, &::documentEnvironment().log);
		// Настройки разбора записей
		awh::codec::cef::reader_t::settings_t settings;
		// Получаем эталонную запись обнаружения вторжений
		const std::string & text = awh::benchmark::event::detection();
		// Выполняем замер разбора без сличения со словарём
		const auto plain = awh::benchmark::event::measure(text.size(), 5000, [&doc, &text]() noexcept -> size_t {
			// Выводим количество пар расширения разобранной записи
			return ::digest(doc, text);
		});
		// Устанавливаем сличение имён ключей и простых видов значений
		settings.mode = awh::codec::cef::mode_t::MEDIUM;
		// Устанавливаем настройки разбора записей
		doc.settings(settings);
		// Выполняем замер разбора со сличением со словарём
		const auto strict = awh::benchmark::event::measure(text.size(), 5000, [&doc, &text]() noexcept -> size_t {
			// Выводим количество пар расширения разобранной записи
			return ::digest(doc, text);
		});
		// Если время какого-либо из прогонов не измерено
		if((plain.seconds <= 0.0) || (strict.seconds <= 0.0)){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "время прогона не измерено";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное удорожание разбора строгим сличением
		result.value = (strict.seconds / plain.seconds);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(strict);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера полного оборота разбора со сборкой
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t roundtrip() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект события CEF
		awh::codec::cef::document_t doc(&::documentEnvironment().fmk, &::documentEnvironment().log);
		// Получаем эталонную запись обнаружения вторжений
		const std::string & text = awh::benchmark::event::detection();
		// Если эталонная запись сценарию непригодна, обратной сборки не выдерживая
		if(!::viable(doc, text, result, true))
			// Выводим результат измерения
			return result;
		// Выполняем замер полного оборота разбора со сборкой
		const auto output = awh::benchmark::event::measure(text.size(), 5000, [&doc, &text]() noexcept -> size_t {
			// Если разбор записи отказом завершился
			if(!doc.parse(text))
				// Выводим отсутствие собранной записи
				return 0;
			// Выводим длину собранной заново записи
			return doc.dump().size();
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::event::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную задержку полного оборота
		result.value = awh::benchmark::event::perLatency(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера расхода выделений памяти на укладку записи в дерево
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t parseAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект события CEF
		awh::codec::cef::document_t doc(&::documentEnvironment().fmk, &::documentEnvironment().log);
		// Получаем эталонную запись обнаружения вторжений
		const std::string & text = awh::benchmark::event::detection();
		// Если эталонная запись сценарию непригодна
		if(!::viable(doc, text, result))
			// Выводим результат измерения
			return result;
		// Выполняем замер укладки эталонной записи в дерево
		const auto output = awh::benchmark::event::measure(text.size(), 2000, [&doc, &text]() noexcept -> size_t {
			// Выводим количество пар расширения разобранной записи
			return ::digest(doc, text);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::event::worked(output, result))
			// Выводим результат измерения
			return result;
		// Если учёт выделений памяти неработоспособен
		if(!awh::benchmark::event::counted(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренный расход выделений памяти на запись
		result.value = awh::benchmark::event::perRecord(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария укладки записи IDS в дерево
	 */
	static const bool PARSE_DETECTION_REGISTERED = awh::benchmark::add(
		"codec/cef: разбор записи IDS в дерево", "МБ/с", PARSE_DETECTION_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, parseDetection
	);
	/**
	 * Выполняем регистрацию сценария укладки записи auditd в дерево
	 */
	static const bool PARSE_AUDIT_REGISTERED = awh::benchmark::add(
		"codec/cef: разбор записи auditd в дерево", "МБ/с", PARSE_AUDIT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, parseAudit
	);
	/**
	 * Выполняем регистрацию сценария задержки обработки короткой записи
	 */
	static const bool PARSE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/cef: задержка разбора короткой записи", "мкс/запись", PARSE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, parseLatency
	);
	/**
	 * Выполняем регистрацию сценария удорожания разбора сличением со словарём
	 */
	static const bool PARSE_STRICT_REGISTERED = awh::benchmark::add(
		"codec/cef: удорожание сличением со словарём", "раз", PARSE_STRICT_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, parseStrict
	);
	/**
	 * Выполняем регистрацию сценария полного оборота разбора со сборкой
	 */
	static const bool ROUNDTRIP_REGISTERED = awh::benchmark::add(
		"codec/cef: оборот разбора со сборкой", "мкс/запись", ROUNDTRIP_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, roundtrip
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на разбор
	 */
	static const bool PARSE_ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/cef: выделения на разбор в дерево", "выд./запись", PARSE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, parseAllocations
	);
};
