/**
 * @file reader.cpp
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
 * @brief Бенчмарки потокового чтения записей CEF — пропускной способности на записях живых
 *        журналов, числа разбираемых записей в секунду, просадки от подачи кусками и расхода
 *        выделений памяти
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
 * @note Держится оно безымянным намеренно: сценарии кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в одно
 *
 */
namespace {
	/**
	 * @brief Объект окружения сценариев с отключённым выводом журнала
	 *
	 */
	struct SilentCefReader {
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
		SilentCefReader() noexcept : log(&SilentCefReader::framework()) {
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
		static SilentCefReader silent;
		// Выводим объект журнала сценариев
		return &silent.log;
	}

	/**
	 * @brief Порог пропускной способности чтения записи обнаружения вторжений
	 *
	 * @details Порог взят по дну сценария на самой медленной из проверенных машин с
	 *          запасом: показатель этот от машины ЗАВИСИТ, и держать его тесным
	 *          значило бы получать ложные тревоги на всяком чужом стенде
	 *
	 */
	constexpr double READ_DETECTION_THRESHOLD = 10.0;

	/**
	 * @brief Порог пропускной способности чтения записи заслона сети
	 *
	 */
	constexpr double READ_FIREWALL_THRESHOLD = 10.0;

	/**
	 * @brief Порог пропускной способности чтения записи надзора за системой
	 *
	 */
	constexpr double READ_AUDIT_THRESHOLD = 10.0;

	/**
	 * @brief Порог числа разбираемых записей в секунду
	 *
	 * @details Показатель этот и есть тот самый EPS, каким мерилась работа прежнего
	 *          модуля CEF на образце владельца
	 *
	 */
	constexpr double READ_EVENTS_THRESHOLD = 20000.0;

	/**
	 * @brief Порог просадки чтения от подачи текста кусками
	 *
	 * @details Подача кусками по одному октету - случай худший из мыслимых, и просадка
	 *          на нём измеряется отношением, а не временем: отношение от машины зависит
	 *          слабее
	 *
	 */
	constexpr double READ_CHUNKED_THRESHOLD = 12.0;

	/**
	 * @brief Порог расхода выделений памяти на чтение одной записи
	 *
	 */
	constexpr double READ_ALLOCATIONS_THRESHOLD = 400.0;

	/**
	 * @brief Функция чтения одной записи потоковым чтением
	 *
	 * @param text разбираемая запись CEF
	 * @param step размер куска подаваемого текста, нулевой для подачи целиком
	 * @return     количество выданных событий разбора
	 *
	 */
	size_t consume(const std::string & text, const size_t step = 0) noexcept {
		// Объект потокового чтения записей
		awh::codec::cef::reader_t reader(&SilentCefReader::framework(), ::readerLogger());
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
	 * @brief Функция замера чтения записи обнаружения вторжений
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readDetection() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись обнаружения вторжений
		const std::string & text = awh::benchmark::event::detection();
		// Выполняем замер чтения эталонной записи
		const auto output = awh::benchmark::event::measure(text.size(), 20000, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
		// Устанавливаем измеренную пропускную способность чтения
		result.value = awh::benchmark::event::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера чтения записи заслона сети
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readFirewall() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись заслона сети
		const std::string & text = awh::benchmark::event::firewall();
		// Выполняем замер чтения эталонной записи
		const auto output = awh::benchmark::event::measure(text.size(), 20000, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
		// Устанавливаем измеренную пропускную способность чтения
		result.value = awh::benchmark::event::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера чтения записи надзора за системой
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readAudit() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонную запись надзора за системой
		const std::string & text = awh::benchmark::event::audit();
		// Выполняем замер чтения эталонной записи
		const auto output = awh::benchmark::event::measure(text.size(), 5000, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
		// Устанавливаем измеренную пропускную способность чтения
		result.value = awh::benchmark::event::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера числа разбираемых записей в секунду
	 *
	 * @details Замер ведётся потоком из ста двадцати восьми записей подряд: сборщик
	 *          журналов принимает именно поток, и разбор его по одной записи вносил бы
	 *          в замер стоимость заведения хранилищ чтения на всякую запись
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t readEvents() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонный поток записей
		const std::string & text = awh::benchmark::event::stream();
		// Выполняем замер чтения эталонного потока записей
		const auto output = awh::benchmark::event::measure(text.size(), 200, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
		// Итоги прогона, пересчитанные на отдельные записи потока
		awh::benchmark::event::outcome_t records = output;
		// Устанавливаем количество разобранных записей потока
		records.operations = (output.operations * 128);
		// Устанавливаем измеренное число разбираемых записей в секунду
		result.value = awh::benchmark::event::perEvents(records);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(records);
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
		// Получаем эталонную запись обнаружения вторжений
		const std::string & text = awh::benchmark::event::detection();
		// Выполняем замер чтения записи, поданной целиком
		const auto whole = awh::benchmark::event::measure(text.size(), 2000, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
		// Выполняем замер чтения записи, поданной кусками по одному октету
		const auto chunked = awh::benchmark::event::measure(text.size(), 2000, [&text]() noexcept -> size_t {
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
		result.details = awh::benchmark::event::details(chunked);
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
		// Получаем эталонную запись обнаружения вторжений
		const std::string & text = awh::benchmark::event::detection();
		// Выполняем замер чтения эталонной записи
		const auto output = awh::benchmark::event::measure(text.size(), 2000, [&text]() noexcept -> size_t {
			// Выводим количество выданных событий разбора
			return ::consume(text);
		});
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
	 * Выполняем регистрацию сценария чтения записи обнаружения вторжений
	 */
	static const bool DETECTION_REGISTERED = awh::benchmark::add(
		"codec/cef: чтение записи IDS", "МБ/с", READ_DETECTION_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readDetection
	);
	/**
	 * Выполняем регистрацию сценария чтения записи заслона сети
	 */
	static const bool FIREWALL_REGISTERED = awh::benchmark::add(
		"codec/cef: чтение записи заслона", "МБ/с", READ_FIREWALL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readFirewall
	);
	/**
	 * Выполняем регистрацию сценария чтения записи надзора за системой
	 */
	static const bool AUDIT_REGISTERED = awh::benchmark::add(
		"codec/cef: чтение записи auditd", "МБ/с", READ_AUDIT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readAudit
	);
	/**
	 * Выполняем регистрацию сценария числа разбираемых записей в секунду
	 */
	static const bool EVENTS_REGISTERED = awh::benchmark::add(
		"codec/cef: разбор потока записей", "записей/с", READ_EVENTS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readEvents
	);
	/**
	 * Выполняем регистрацию сценария просадки чтения от подачи кусками
	 */
	static const bool CHUNKED_REGISTERED = awh::benchmark::add(
		"codec/cef: просадка от подачи кусками", "раз", READ_CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readChunked
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на чтение
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/cef: выделения на чтение", "выд./запись", READ_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readAllocations
	);
};
