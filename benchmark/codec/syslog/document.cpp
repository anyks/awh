/**
 * @file document.cpp
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
 * @brief Сценарии замеров события, удерживаемого целиком — разбора записи в дерево, оборота
 *        записи, обхода дерева по пути и расхода выделений памяти
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
	struct SilentSysLogDocument {
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
		SilentSysLogDocument() noexcept : log(&SilentSysLogDocument::framework()) {
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
	const awh::log_t * documentLogger() noexcept {
		// Объект журнала сценариев
		static SilentSysLogDocument silent;
		// Выводим объект журнала сценариев
		return &silent.log;
	}

	/**
	 * @brief Порог пропускной способности разбора записи в дерево
	 *
	 * @details Мера: длина эталонной записи, помноженная на число кругов, делённая на
	 *          время прогона. Разбор в дерево заведомо дороже потокового чтения - к
	 *          работе разбора добавляется заведение узлов дерева, - и порог у него свой
	 *
	 */
	constexpr double PARSE_THRESHOLD = 5.0;

	/**
	 * @brief Порог пропускной способности оборота записи
	 *
	 * @details Оборот есть разбор с последующей сборкой: показатель этот и есть
	 *          стоимость превращения записи одного вида в запись другого - работы, ради
	 *          какой сборщик журналов кодек и держит
	 *
	 */
	constexpr double ROUNDTRIP_THRESHOLD = 3.0;

	/**
	 * @brief Порог числа обходов дерева в секунду
	 *
	 * @details Обход по пути и есть то, чем потребитель дерево читает: показатель
	 *          сторожит стоимость розыска значения, а не разбора
	 *
	 */
	constexpr double TRAVERSAL_THRESHOLD = 20000.0;

	/**
	 * @brief Порог расхода выделений памяти на разбор одной записи в дерево
	 *
	 * @details Показатель этот от машины НЕ зависит, но зависит от библиотеки языка:
	 *          запас короткой строки у libc++ и libstdc++ разный
	 *
	 */
	constexpr double PARSE_ALLOCATIONS_THRESHOLD = 600.0;

	/**
	 * @brief Функция разбора одной записи в дерево события
	 *
	 * @param text разбираемая запись системного журнала
	 * @return     количество полей дерева события
	 *
	 */
	size_t digest(const std::string & text) noexcept {
		// Объект события, удерживаемого целиком
		awh::codec::syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
		// Если разбор записи отказом завершился
		if(!document.parse(text))
			// Выводим отсутствие полей дерева события
			return 0;
		// Выводим количество полей дерева события
		return document.root().size();
	}

	/**
	 * @brief Функция оборота одной записи через дерево события
	 *
	 * @param text разбираемая запись системного журнала
	 * @return     длина собранной записи в октетах
	 *
	 */
	size_t roundtrip(const std::string & text) noexcept {
		// Объект события, удерживаемого целиком
		awh::codec::syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
		// Если разбор записи отказом завершился
		if(!document.parse(text))
			// Выводим отсутствие собранной записи
			return 0;
		// Выводим длину записи, собранной из дерева события
		return document.dump().size();
	}

	/**
	 * @brief Функция поверки пригодности эталонной записи сценарию
	 *
	 * @details Числитель меры берётся длиной ЭТАЛОННОЙ записи: откажи разбор - и
	 *          сценарий отчитается о работе, какой не было, тем БЫСТРЕЕ, чем раньше
	 *          отказ наступил
	 *
	 * @param text   эталонная запись
	 * @param result заполняемый результат измерения
	 * @return       признак пригодности эталонной записи сценарию
	 *
	 */
	bool viable(const std::string & text, awh::benchmark::result_t & result) noexcept {
		// Если разбор эталонной записи ни одного поля не выдал
		if(::digest(text) == 0){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "разбор эталонной записи ни одного поля не выдал";
			// Выводим отсутствие пригодности эталонной записи
			return false;
		}
		// Выводим пригодность эталонной записи сценарию
		return true;
	}

	/**
	 * @brief Функция замера разбора записи в дерево события
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t parseRecord() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись нынешнего описания
		const std::string & text = awh::benchmark::journal::modern();
		// Если эталонная запись сценарию непригодна
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Выполняем замер разбора эталонной записи в дерево события
		const auto output = awh::benchmark::journal::measure(text.size(), 20000, [&text]() noexcept -> size_t {
			// Выводим количество полей дерева события
			return ::digest(text);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::journal::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную пропускную способность разбора
		result.value = awh::benchmark::journal::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::journal::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера оборота записи через дерево события
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t roundtripRecord() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись со многими блоками данных
		const std::string & text = awh::benchmark::journal::structured();
		// Если эталонная запись сценарию непригодна
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Если оборот эталонной записи отказом завершился
		if(::roundtrip(text) == 0){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "оборот эталонной записи отказом завершился";
			// Выводим результат измерения
			return result;
		}
		// Выполняем замер оборота эталонной записи
		const auto output = awh::benchmark::journal::measure(text.size(), 10000, [&text]() noexcept -> size_t {
			// Выводим длину собранной записи в октетах
			return ::roundtrip(text);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::journal::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную пропускную способность оборота
		result.value = awh::benchmark::journal::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::journal::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера обхода дерева события по пути
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t traverseTree() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись со многими блоками данных
		const std::string & text = awh::benchmark::journal::structured();
		// Если эталонная запись сценарию непригодна
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Объект события, удерживаемого целиком
		awh::codec::syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
		// Если разбор эталонной записи отказом завершился
		if(!document.parse(text)){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "разбор эталонной записи отказом завершился";
			// Выводим результат измерения
			return result;
		}
		// Выполняем замер обхода дерева события по пути
		const auto output = awh::benchmark::journal::measure(text.size(), 20000, [&document]() noexcept -> size_t {
			// Количество найденных значений дерева события
			size_t found = 0;
			/**
			 * Выполняем перебор всех блоков структурированных данных
			 */
			for(auto & block : document.keys("/structures")){
				/**
				 * Выполняем перебор всех полей блока структурированных данных
				 */
				for(auto & param : document.keys("/structures/" + block)){
					// Если значение поля блока деревом объявлено
					if(document.has("/structures/" + block + "/" + param))
						// Наращиваем количество найденных значений
						found++;
				}
			}
			// Выводим количество найденных значений дерева события
			return found;
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::journal::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное число обходов дерева в секунду
		result.value = awh::benchmark::journal::perEvents(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::journal::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера расхода выделений памяти на разбор записи в дерево
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t parseAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись нынешнего описания
		const std::string & text = awh::benchmark::journal::modern();
		// Если эталонная запись сценарию непригодна
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Выполняем замер разбора эталонной записи в дерево события
		const auto output = awh::benchmark::journal::measure(text.size(), 2000, [&text]() noexcept -> size_t {
			// Выводим количество полей дерева события
			return ::digest(text);
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
	 * Выполняем регистрацию сценария разбора записи в дерево события
	 */
	static const bool PARSE_REGISTERED = awh::benchmark::add(
		"codec/syslog: разбор записи в дерево", "МБ/с", PARSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, parseRecord
	);
	/**
	 * Выполняем регистрацию сценария оборота записи через дерево события
	 */
	static const bool ROUNDTRIP_REGISTERED = awh::benchmark::add(
		"codec/syslog: оборот записи", "МБ/с", ROUNDTRIP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, roundtripRecord
	);
	/**
	 * Выполняем регистрацию сценария обхода дерева события по пути
	 */
	static const bool TRAVERSAL_REGISTERED = awh::benchmark::add(
		"codec/syslog: обход дерева по пути", "обходов/с", TRAVERSAL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, traverseTree
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на разбор
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/syslog: выделения на разбор", "выд./запись", PARSE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, parseAllocations
	);
};
