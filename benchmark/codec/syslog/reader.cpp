/**
 * @file reader.cpp
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
 * @brief Сценарии замеров потокового чтения сообщений системного журнала — пропускной способности
 *        обоих описаний, числа разбираемых записей в секунду и расхода выделений памяти
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
	struct SilentSysLogReader {
		/**
		 * @brief Функция получения объекта фреймворка сценариев
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          сценарии, ибо фреймворк сам опирается на статику из библиотеки
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
		SilentSysLogReader() noexcept : log(&SilentSysLogReader::framework()) {
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
	const awh::log_t * readerLogger() noexcept {
		// Объект журнала сценариев
		static SilentSysLogReader silent;
		// Выводим объект журнала сценариев
		return &silent.log;
	}

	/**
	 * @brief Порог пропускной способности чтения записи устаревшего описания
	 *
	 * @details Мера: длина эталонной записи, помноженная на число кругов, делённая на
	 *          время прогона. Порог поставлен ОЖИДАНИЕМ, а не замером по стендам: замерена
	 *          одна машина, прочие стенды кодеком SysLog ещё не проходились. Число взято
	 *          заведомо ниже замеренного, потому что показатель этот от машины ЗАВИСИТ, и
	 *          держать его тесным значило бы получать ложные тревоги на всяком чужом стенде
	 *
	 * @warning Порог, ни разу не проходившийся, и порог, проходящийся всегда, дурны
	 *          одинаково - оба приучают читать отчёт по диагонали. Выставить по дну
	 *          сценария на самой медленной машине - работа ПОСЛЕ прогона по стендам
	 *
	 */
	constexpr double READ_LEGACY_THRESHOLD = 10.0;

	/**
	 * @brief Порог пропускной способности чтения записи нынешнего описания
	 *
	 * @details Мера та же, что и у чтения записи устаревшего описания. Порог поставлен
	 *          ожиданием и подлежит уточнению по стендам
	 *
	 */
	constexpr double READ_MODERN_THRESHOLD = 10.0;

	/**
	 * @brief Порог пропускной способности чтения записи со многими блоками данных
	 *
	 * @details Разбор блоков структурированных данных и есть та работа, какой запись
	 *          RFC 5424 отличается от устаревшей: порог этот сторожит именно её
	 *
	 */
	constexpr double READ_STRUCTURED_THRESHOLD = 10.0;

	/**
	 * @brief Порог числа разбираемых записей в секунду
	 *
	 * @details Показатель этот и есть тот самый EPS, каким мерится работа сборщика
	 *          журналов: он принимает поток событий от многих устройств, и решает у него
	 *          не пропускная способность в мегабайтах, а число событий, какое он успевает
	 *          разобрать. Порог поставлен ожиданием и подлежит уточнению по стендам
	 *
	 */
	constexpr double READ_EVENTS_THRESHOLD = 20000.0;

	/**
	 * @brief Порог просадки чтения от подачи текста кусками
	 *
	 * @details Мера: время прогона подачею по одному октету, делённое на время прогона
	 *          подачею целиком. Показатель этот от машины зависит слабо - обе части меры
	 *          мерятся на ней же, - и потому держится теснее прочих
	 *
	 */
	constexpr double READ_CHUNKED_THRESHOLD = 40.0;

	/**
	 * @brief Порог расхода выделений памяти на чтение одной записи
	 *
	 * @details Показатель этот от машины НЕ зависит, но зависит от библиотеки языка:
	 *          запас короткой строки у libc++ и libstdc++ разный, и на libstdc++ число
	 *          может вырасти кратно, поломкой не будучи. Оттого порог и держится широким,
	 *          покуда стенд на libstdc++ не пройден
	 *
	 */
	constexpr double READ_ALLOCATIONS_THRESHOLD = 400.0;

	/**
	 * @brief Функция чтения записей потоковым чтением
	 *
	 * @param text разбираемый текст записей
	 * @param step размер куска подаваемого текста, нулевой для подачи целиком
	 * @return     количество выданных событий разбора
	 *
	 */
	size_t consume(const std::string & text, const size_t step = 0) noexcept {
		// Объект потокового чтения записей
		awh::codec::syslog::reader_t reader(&SilentSysLogReader::framework(), ::readerLogger());
		// Количество выданных событий разбора
		size_t result = 0;
		// Смещение подачи текста записей
		size_t offset = 0;
		/**
		 * Выполняем подачу текста записей, пока он не исчерпан
		 */
		do {
			// Получаем размер очередного куска подаваемого текста
			const size_t size = ((step == 0) ? (text.size() - offset) : ::std::min(step, text.size() - offset));
			// Выполняем подачу очередного куска текста записей
			reader.feed(text.data() + offset, size, (offset + size) >= text.size());
			// Сдвигаем смещение подачи текста записей
			offset += size;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next())
				// Наращиваем количество выданных событий разбора
				result++;
		} while(offset < text.size());
		// Выводим количество выданных событий разбора
		return result;
	}

	/**
	 * @brief Функция поверки пригодности эталонной записи сценарию
	 *
	 * @details Числитель меры берётся длиной ЭТАЛОННОЙ записи, помноженной на число
	 *          кругов, а не объёмом прочитанного. Откажи чтение - и сценарий отчитается
	 *          о работе, какой не было, тем БЫСТРЕЕ, чем раньше отказ наступил: знак
	 *          меры переворачивается, и поломка вознаграждается. Оттого пригодность
	 *          поверяется ОДНИМ кругом до замера, а негодность объявляется исходом
	 *          наравне с числом
	 *
	 * @param text   эталонная запись
	 * @param result заполняемый результат измерения
	 * @return       признак пригодности эталонной записи сценарию
	 *
	 */
	bool viable(const std::string & text, awh::benchmark::result_t & result) noexcept {
		// Если чтение эталонной записи ни одного события не выдало
		if(::consume(text) == 0){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "чтение эталонной записи ни одного события не выдало";
			// Выводим отсутствие пригодности эталонной записи
			return false;
		}
		// Выводим пригодность эталонной записи сценарию
		return true;
	}

	/**
	 * @brief Функция замера чтения записи заданного эталона
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
		// Выполняем замер чтения эталонной записи
		const auto output = awh::benchmark::journal::measure(text.size(), rounds, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::journal::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную пропускную способность чтения
		result.value = awh::benchmark::journal::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::journal::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера чтения записи устаревшего описания
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readLegacy() noexcept {
		// Выводим результат замера чтения записи устаревшего описания
		return ::throughput(awh::benchmark::journal::legacy(), 20000);
	}

	/**
	 * @brief Функция замера чтения записи нынешнего описания
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readModern() noexcept {
		// Выводим результат замера чтения записи нынешнего описания
		return ::throughput(awh::benchmark::journal::modern(), 20000);
	}

	/**
	 * @brief Функция замера чтения записи со многими блоками данных
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readStructured() noexcept {
		// Выводим результат замера чтения записи со многими блоками данных
		return ::throughput(awh::benchmark::journal::structured(), 10000);
	}

	/**
	 * @brief Функция замера числа разбираемых записей в секунду
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readEvents() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонный поток записей
		const std::string & text = awh::benchmark::journal::stream();
		// Если эталонный поток сценарию непригоден
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Выполняем замер чтения эталонного потока записей
		const auto output = awh::benchmark::journal::measure(text.size(), 500, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::journal::worked(output, result))
			// Выводим результат измерения
			return result;
		// Итоги прогона, пересчитанные на отдельные записи потока
		awh::benchmark::journal::outcome_t records = output;
		/**
		 * Устанавливаем количество разобранных записей потока
		 *
		 * @note Число записей берётся у самого потока, а не постоянной: постоянная,
		 *       от эталона оторванная, разошлась бы с ним при первой же правке
		 */
		records.operations = (output.operations * awh::benchmark::journal::records());
		// Устанавливаем измеренное число разбираемых записей в секунду
		result.value = awh::benchmark::journal::perEvents(records);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::journal::details(records);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера просадки чтения от подачи текста кусками
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readChunked() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись нынешнего описания
		const std::string & text = awh::benchmark::journal::modern();
		// Если эталонная запись сценарию непригодна
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Выполняем замер чтения записи, поданной целиком
		const auto whole = awh::benchmark::journal::measure(text.size(), 2000, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
		// Выполняем замер чтения записи, поданной кусками по одному октету
		const auto chunked = awh::benchmark::journal::measure(text.size(), 2000, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text, 1);
		});
		// Если время какого-либо из прогонов не измерено
		if((whole.seconds <= 0.0) || (chunked.seconds <= 0.0)){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "время прогона не измерено";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренную просадку чтения от подачи кусками
		result.value = (chunked.seconds / whole.seconds);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::journal::details(chunked);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера задержки чтения одной короткой записи
	 *
	 * @details Показатель назначен записи наименьшей длины: у коротких записей решают
	 *          постоянные издержки на заведение хранилищ разбора, а не сама работа
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readLatency() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись наименьшей длины
		const std::string & text = awh::benchmark::journal::minimal();
		// Если эталонная запись сценарию непригодна
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Выполняем замер чтения эталонной записи
		const auto output = awh::benchmark::journal::measure(text.size(), 50000, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
		// Если измеряемая работа кругами не состоялась
		if(!awh::benchmark::journal::worked(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную задержку чтения одной записи
		result.value = awh::benchmark::journal::perLatency(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::journal::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера расхода выделений памяти на чтение одной записи
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись нынешнего описания
		const std::string & text = awh::benchmark::journal::modern();
		// Если эталонная запись сценарию непригодна
		if(!::viable(text, result))
			// Выводим результат измерения
			return result;
		// Выполняем замер чтения эталонной записи
		const auto output = awh::benchmark::journal::measure(text.size(), 2000, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
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
	 * Выполняем регистрацию сценария чтения записи устаревшего описания
	 */
	static const bool LEGACY_REGISTERED = awh::benchmark::add(
		"codec/syslog: чтение записи RFC 3164", "МБ/с", READ_LEGACY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readLegacy
	);
	/**
	 * Выполняем регистрацию сценария чтения записи нынешнего описания
	 */
	static const bool MODERN_REGISTERED = awh::benchmark::add(
		"codec/syslog: чтение записи RFC 5424", "МБ/с", READ_MODERN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readModern
	);
	/**
	 * Выполняем регистрацию сценария чтения записи со многими блоками данных
	 */
	static const bool STRUCTURED_REGISTERED = awh::benchmark::add(
		"codec/syslog: чтение блоков данных", "МБ/с", READ_STRUCTURED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readStructured
	);
	/**
	 * Выполняем регистрацию сценария числа разбираемых записей в секунду
	 */
	static const bool EVENTS_REGISTERED = awh::benchmark::add(
		"codec/syslog: разбор потока записей", "записей/с", READ_EVENTS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readEvents
	);
	/**
	 * Выполняем регистрацию сценария просадки чтения от подачи кусками
	 */
	static const bool CHUNKED_REGISTERED = awh::benchmark::add(
		"codec/syslog: просадка от подачи кусками", "раз", READ_CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readChunked
	);
	/**
	 * Выполняем регистрацию сценария задержки чтения короткой записи
	 */
	static const bool LATENCY_REGISTERED = awh::benchmark::add(
		"codec/syslog: задержка чтения записи", "мкс", 50.0,
		awh::benchmark::bound_t::MAXIMUM, readLatency
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на чтение
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/syslog: выделения на чтение", "выд./запись", READ_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readAllocations
	);
};
