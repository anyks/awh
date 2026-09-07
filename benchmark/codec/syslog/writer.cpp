/**
 * @file writer.cpp
 * @date 2026-09-07
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
 * @brief Сценарии замеров записи событий в сообщение системного журнала — пропускной способности
 *        сборки обоих описаний и расхода выделений памяти
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы бенчмарков
 */
#include "syslog.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>

/**
 * @brief Пространство имён сценариев этого файла
 *
 * @note Держится оно безымянным намеренно: сценарии кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в одно
 *
 */
namespace {
	/**
	 * @brief Объект окружения сценариев с отключённым выводом журнала
	 *
	 */
	struct SilentSysLogWriter {
		/**
		 * @brief Функция получения объекта фреймворка сценариев
		 *
		 * @return объект фреймворка сценариев
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка сценариев
			static awh::fmk_t fmk;
			// Выводим объект фреймворка сценариев
			return fmk;
		}
		// Объект журнала сценариев
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		SilentSysLogWriter() noexcept : log(&SilentSysLogWriter::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};

	/**
	 * @brief Функция получения объекта журнала сценариев
	 *
	 * @return объект журнала сценариев
	 *
	 */
	const awh::log_t * writerLogger() noexcept {
		// Объект журнала сценариев
		static SilentSysLogWriter silent;
		// Выводим объект журнала сценариев
		return &silent.log;
	}

	/**
	 * @brief Порог пропускной способности сборки записи устаревшего описания
	 *
	 * @details Мера: длина ЭТАЛОННОЙ записи, помноженная на число кругов, делённая на
	 *          время прогона. Порог поставлен ожиданием и подлежит уточнению по стендам
	 *
	 * @warning Мера берёт длину эталона, а не собранного: откажи сборка на полпути - и
	 *          сценарий отчитался бы тем БЫСТРЕЕ, чем раньше отказ наступил. Оттого
	 *          всякий круг поверяется полнотой собранного, а не одним лишь признаком
	 *          успеха
	 *
	 */
	constexpr double WRITE_LEGACY_THRESHOLD = 10.0;

	/**
	 * @brief Порог пропускной способности сборки записи нынешнего описания
	 *
	 * @details Мера та же, что и у сборки записи устаревшего описания
	 *
	 */
	constexpr double WRITE_MODERN_THRESHOLD = 10.0;

	/**
	 * @brief Порог пропускной способности сборки записи со многими блоками данных
	 *
	 * @details Постановка отмены знаков и сборка блоков и есть та работа, какой запись
	 *          RFC 5424 отличается от устаревшей
	 *
	 */
	constexpr double WRITE_STRUCTURED_THRESHOLD = 10.0;

	/**
	 * @brief Порог расхода выделений памяти на сборку одной записи
	 *
	 * @details Показатель этот от машины НЕ зависит, но зависит от библиотеки языка:
	 *          запас короткой строки у libc++ и libstdc++ разный
	 *
	 */
	constexpr double WRITE_ALLOCATIONS_THRESHOLD = 200.0;

	/**
	 * @brief Функция получения дерева события по эталонной записи
	 *
	 * @details Дерево собирается ОДИН раз и держится статикой: сборка его внутри
	 *          измеряемого цикла вносила бы в замер стоимость разбора вместо стоимости
	 *          записи
	 *
	 * @param text эталонная запись
	 * @return     дерево события контейнером ABC
	 *
	 */
	const awh::codec::abc::value_t & tree(const std::string & text) noexcept {
		// Объект события, удерживаемого целиком
		static awh::codec::syslog::document_t document(&SilentSysLogWriter::framework(), ::writerLogger());
		// Разобранная эталонная запись, деревом удерживаемая
		static awh::codec::abc::value_t result;
		// Выполняем разбор эталонной записи
		if(document.parse(text))
			// Запоминаем дерево разобранной записи
			result = document.root();
		// Выводим дерево события контейнером ABC
		return result;
	}

	/**
	 * @brief Функция сборки одной записи из дерева события
	 *
	 * @param value дерево события контейнером ABC
	 * @return      длина собранной записи в октетах
	 *
	 */
	size_t produce(const awh::codec::abc::value_t & value) noexcept {
		// Объект записи событий
		awh::codec::syslog::writer_t writer(&SilentSysLogWriter::framework(), ::writerLogger());
		// Собранная запись системного журнала
		std::string result;
		// Если сборка записи отказом завершилась
		if(!writer.write(value, result))
			// Выводим отсутствие собранной записи
			return 0;
		// Выводим длину собранной записи в октетах
		return result.size();
	}

	/**
	 * @brief Функция поверки пригодности эталонной записи сценарию
	 *
	 * @details Поверяется не признак успеха сборки, а ПОЛНОТА собранного: сборка,
	 *          отказавшая на полпути, оставляет в записи собранное до места отказа, и
	 *          признак успеха её не ловит. Эталон и собранное разнятся видом даты, но
	 *          не порядком - оттого поверка требует хотя бы четырёх пятых длины эталона
	 *
	 * @note Правило это заведено у кодека CEF 04.09.2026 после того, как сценарий
	 *       сборки записи auditd отчитался 3248 МБ/с, собирая 45 октетов вместо 387
	 *
	 * @param text   эталонная запись
	 * @param result заполняемый результат измерения
	 * @return       признак пригодности эталонной записи сценарию
	 *
	 */
	bool viable(const std::string & text, awh::benchmark::result_t & result) noexcept {
		// Получаем дерево события по эталонной записи
		const awh::codec::abc::value_t & value = ::tree(text);
		// Получаем длину записи, собранной из дерева события
		const size_t size = ::produce(value);
		// Если сборка записи отказом завершилась
		if(size == 0){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "сборка эталонной записи отказом завершилась";
			// Выводим отсутствие пригодности эталонной записи
			return false;
		}
		// Если собранная запись эталона существенно короче
		if(size < ((text.size() * 4) / 5)){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "собранная запись существенно короче эталонной";
			// Выводим отсутствие пригодности эталонной записи
			return false;
		}
		// Выводим пригодность эталонной записи сценарию
		return true;
	}

	/**
	 * @brief Функция замера сборки записи заданного эталона
	 *
	 * @param text   эталонная запись
	 * @param rounds количество кругов замера
	 * @return       результат измерения
	 *
	 */
	awh::benchmark::result_t throughput(const std::string & text, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Если эталонная запись сценарию непригодна
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Получаем дерево события по эталонной записи
		const awh::codec::abc::value_t & value = ::tree(text);
		// Выполняем замер сборки записи из дерева события
		const auto output = awh::benchmark::journal::measure(text.size(), rounds, [&value]() noexcept -> size_t {
			// Выводим длину собранной записи в октетах
			return ::produce(value);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::journal::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную пропускную способность сборки
		result.value = awh::benchmark::journal::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::journal::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера сборки записи устаревшего описания
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t writeLegacy() noexcept {
		// Выводим результат замера сборки записи устаревшего описания
		return ::throughput(awh::benchmark::journal::legacy(), 20000);
	}

	/**
	 * @brief Функция замера сборки записи нынешнего описания
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t writeModern() noexcept {
		// Выводим результат замера сборки записи нынешнего описания
		return ::throughput(awh::benchmark::journal::modern(), 20000);
	}

	/**
	 * @brief Функция замера сборки записи со многими блоками данных
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t writeStructured() noexcept {
		// Выводим результат замера сборки записи со многими блоками данных
		return ::throughput(awh::benchmark::journal::structured(), 10000);
	}

	/**
	 * @brief Функция замера расхода выделений памяти на сборку одной записи
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t writeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись нынешнего описания
		const std::string & text = awh::benchmark::journal::modern();
		// Если эталонная запись сценарию непригодна
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Получаем дерево события по эталонной записи
		const awh::codec::abc::value_t & value = ::tree(text);
		// Выполняем замер сборки записи из дерева события
		const auto output = awh::benchmark::journal::measure(text.size(), 2000, [&value]() noexcept -> size_t {
			// Выводим длину собранной записи в октетах
			return ::produce(value);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::journal::worked(output, result))
			// Выводим результат измерения
			return result;
		// Если учёт выделений памяти неработоспособен
		if(!awh::benchmark::journal::counted(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренный расход выделений памяти на запись
		result.value = awh::benchmark::journal::perRecord(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::journal::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки записи устаревшего описания
	 */
	static const bool LEGACY_REGISTERED = awh::benchmark::add(
		"codec/syslog: сборка записи RFC 3164", "МБ/с", WRITE_LEGACY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeLegacy
	);
	/**
	 * Выполняем регистрацию сценария сборки записи нынешнего описания
	 */
	static const bool MODERN_REGISTERED = awh::benchmark::add(
		"codec/syslog: сборка записи RFC 5424", "МБ/с", WRITE_MODERN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeModern
	);
	/**
	 * Выполняем регистрацию сценария сборки записи со многими блоками данных
	 */
	static const bool STRUCTURED_REGISTERED = awh::benchmark::add(
		"codec/syslog: сборка блоков данных", "МБ/с", WRITE_STRUCTURED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeStructured
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на сборку
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/syslog: выделения на сборку", "выд./запись", WRITE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, writeAllocations
	);
};
