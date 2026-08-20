/**
 * @file reader.cpp
 * @date 2026-08-13
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
 * @brief Бенчмарки потокового чтения текста CSV — пропускная способность на таблицах
 *        разного склада, расход выделений памяти, просадка от подачи кусками, стоимость
 *        определения разделителя и задержка обработки одной таблицы
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков контейнера CSV
 */
#include "csv.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера CSV
 */
using namespace awh::benchmark::table;

/**
 * @brief Внутренние параметры и сценарии бенчмарков потокового чтения
 *
 */
namespace {
	/**
	 * @brief Количество разбираемых мелких таблиц
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество разбираемых крупных таблиц
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество разбираемых таблиц с преобладанием одного вида содержимого
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 16;
	/**
	 * @brief Размер куска подачи текста таблицы в октетах
	 *
	 * @note Взят равным полезной части кадра сети: таблица приходит такими кусками и с
	 *       гнезда, и с файловой системы, а подача её целиком - случай не самый частый
	 *
	 */
	static constexpr size_t CHUNK_SIZE = 1460;

	/**
	 * @brief Пороги пропускной способности потокового чтения в мегабайтах в секунду
	 *
	 * @details Пороги сняты прогоном по двенадцати отладочным стендам x86_64
	 *          (14.08.2026) и назначены по дну КАЖДОГО СЦЕНАРИЯ, делённому надвое - тем
	 *          же правилом, что и у контейнера INI. Дно чтения держит OpenBSD: он
	 *          отстаёт от рабочей машины вдесятеро, тогда как прочие стенды -
	 *          втрое-впятеро
	 *
	 * @warning Одной «самой медленной машины» не существует, и брать её на весь набор
	 *          нельзя: прогон порогов времени по семи стендам 20.08.2026 показал, что
	 *          дно берут разные машины на разных сценариях - у части их хуже всех
	 *          Alpine, у части FreeBSD, у одного NetBSD. Дно берётся по каждому
	 *          сценарию в отдельности
	 *
	 * @note Двукратная просадка здесь не отвлечённая опасность: разбор знака за знаком
	 *       вместо быстрого прохода по знакам, содержимым являющимся, ронял показатель
	 *       ровно вдвое, и стерегут пороги именно его возвращение
	 *
	 * @note Порог, назначенный по рабочей машине с запасом вдесятеро, для чтения
	 *       широкой таблицы оказывался ЗАВЫШЕН - OpenBSD давал 19.01 при пороге 20.0.
	 *       Отсюда правило: запас к рабочей машине доводом не служит, порог берётся
	 *       замером на стендах
	 *
	 */
	static constexpr double READ_SERVICE_THRESHOLD = 10.0;
	/**
	 * @brief Порог пропускной способности чтения крупной таблицы
	 *
	 */
	static constexpr double READ_LARGE_THRESHOLD = 10.0;
	/**
	 * @brief Порог пропускной способности чтения широкой таблицы
	 *
	 */
	static constexpr double READ_WIDE_THRESHOLD = 9.0;
	/**
	 * @brief Порог пропускной способности чтения таблицы с преобладанием кавычек
	 *
	 */
	static constexpr double READ_QUOTED_THRESHOLD = 14.0;
	/**
	 * @brief Порог пропускной способности чтения многострочных полей
	 *
	 */
	static constexpr double READ_MULTILINE_THRESHOLD = 14.0;
	/**
	 * @brief Порог количества выделений памяти на чтение крупной таблицы
	 *
	 * @details Показатель воспроизводим до единиц и потому годится в порог куда больше
	 *          времени: чтение ведётся на переиспользуемых хранилищах, и количество
	 *          выделений на таблицу от её размера зависеть не должно вовсе. Рост
	 *          показателя означает, что какое-то хранилище перестало переиспользоваться
	 *          и заводится заново на каждую запись
	 *
	 * @note Показатель этот зависит от стандартной библиотеки: короткий запас строки у
	 *       libc++ вмещает 22 знака, а у libstdc++ - 15, и поле длиннее запаса
	 *       выделение памяти вызывает, а короче - нет. Замер по стендам расхождение
	 *       это подтвердил: 63 у libc++ (macOS, FreeBSD, OpenBSD) против 64 у libstdc++
	 *
	 * @warning Порог держится ВПРИТЫК намеренно - тугим он и нашёл очередь событий,
	 *          выделявшую двадцать шесть тысяч раз на таблицу. Ослаблять его при
	 *          отказе нельзя: отказ означает, что хранилище перестало
	 *          переиспользоваться
	 *
	 */
	static constexpr double READ_ALLOCATIONS_THRESHOLD = 72.0;
	/**
	 * @brief Порог просадки чтения от подачи текста кусками
	 *
	 * @details Стоимость подачи кусками стережётся здесь, а не в наборе проверок:
	 *          надбавка эта постоянного размера, и счётчики покрытия отладочной сборки
	 *          затушёвывают её напрочь
	 *
	 * @note Измеряется отношение времени подачи кусками ко времени подачи того же
	 *       текста целиком, а не пропускная способность сама по себе. Отношение двух
	 *       прогонов на одной машине от её быстродействия не зависит, и порог ему можно
	 *       назначить впритык
	 *
	 */
	static constexpr double READ_CHUNKED_THRESHOLD = 2.0;
	/**
	 * @brief Порог просадки чтения от определения разделителя
	 *
	 * @details Определение разделителя откладывает выдачу до тех пор, пока разделитель
	 *          не определится, и просматривает отложенное по одному разу на всякий
	 *          проверяемый знак. Надбавка эта приходится на начало текста и с его
	 *          размером не растёт: на крупной таблице она обязана теряться вовсе
	 *
	 */
	static constexpr double READ_DETECT_THRESHOLD = 1.5;
	/**
	 * @brief Порог задержки чтения таблицы ответа службы в микросекундах
	 *
	 * @details Ответ службы - это сотни байтов, и на нём решает не пропускная
	 *          способность, а постоянные издержки на заведение разбора. Превышение
	 *          порога означает, что заведение хранилищ разбора подорожало
	 *
	 * @note Порог снят по дну этого сценария с двукратным запасом: дно держит OpenBSD -
	 *       12.09 мкс против 1.45 у рабочей машины
	 *
	 */
	static constexpr double READ_SERVICE_LATENCY_THRESHOLD = 24.0;

	/**
	 * @brief Функция потокового чтения текста таблицы
	 *
	 * @param text     разбираемый текст таблицы
	 * @param settings настройки разбора текста таблицы
	 * @return         количество полученных событий разбора
	 *
	 */
	static uint64_t read(const string & text, const awh::codec::csv::reader_t::settings_t & settings) noexcept {
		// Объект потокового чтения текста таблицы
		awh::codec::csv::reader_t reader(settings);
		/**
		 * Если передать текст таблицы не удалось
		 */
		if(!reader.feed(text.data(), text.size(), true))
			// Выводим нулевое количество событий разбора
			return 0;
		// Количество полученных событий разбора
		uint64_t result = 0;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next())
			// Выполняем подсчёт полученных событий разбора
			result++;
		// Выводим количество полученных событий разбора
		return result;
	}
	/**
	 * @brief Функция потокового чтения текста таблицы, поданного кусками
	 *
	 * @param text разбираемый текст таблицы
	 * @return     количество полученных событий разбора
	 *
	 */
	static uint64_t feed(const string & text) noexcept {
		// Объект потокового чтения текста таблицы
		awh::codec::csv::reader_t reader;
		// Количество полученных событий разбора
		uint64_t result = 0;
		// Смещение очередного подаваемого куска текста таблицы
		size_t offset = 0;
		/**
		 * Выполняем подачу текста таблицы до его окончания
		 */
		do {
			// Размер очередного подаваемого куска текста таблицы
			const size_t size = (((offset + ::CHUNK_SIZE) > text.size()) ? (text.size() - offset) : ::CHUNK_SIZE);
			/**
			 * Если передать очередной кусок текста таблицы не удалось
			 */
			if(!reader.feed(text.data() + offset, size, ((offset + size) >= text.size())))
				// Выводим нулевое количество событий разбора
				return 0;
			/**
			 * Выполняем перебор всех событий, полученных из очередного куска
			 */
			while(reader.next())
				// Выполняем подсчёт полученных событий разбора
				result++;
			// Выполняем смещение на размер поданного куска текста таблицы
			offset += size;
		// Выполняем подачу до исчерпания текста таблицы
		} while(offset < text.size());
		// Выводим количество полученных событий разбора
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения таблицы ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = service();
		// Настройки разбора текста таблицы
		const awh::codec::csv::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::SMALL_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения крупной таблицы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = large();
		// Настройки разбора текста таблицы
		const awh::codec::csv::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::LARGE_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения широкой таблицы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readWide() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = wide();
		// Настройки разбора текста таблицы
		const awh::codec::csv::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::FOCUSED_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения таблицы с преобладанием кавычек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readQuoted() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = quoted();
		// Настройки разбора текста таблицы
		const awh::codec::csv::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::FOCUSED_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения многострочных полей
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readMultiline() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = multiline();
		// Настройки разбора текста таблицы
		const awh::codec::csv::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::FOCUSED_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на чтение
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = large();
		// Настройки разбора текста таблицы
		const awh::codec::csv::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::LARGE_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, settings);
		});
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		/**
		 * Если учёт выделений памяти не работает
		 */
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное значение
		result.value = perDocument(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария просадки чтения от подачи кусками
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readChunked() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = large();
		// Настройки разбора текста таблицы
		const awh::codec::csv::reader_t::settings_t settings;
		// Выполняем прогон чтения текста таблицы, поданного целиком
		const outcome_t whole = measure(text.size(), ::LARGE_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, settings);
		});
		// Выполняем прогон чтения текста таблицы, поданного кусками
		const outcome_t chunked = measure(text.size(), ::LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста таблицы кусками
			return ::feed(text);
		});
		/**
		 * Если хотя бы один из прогонов не состоялся
		 */
		if((whole.seconds <= 0.0) || (chunked.seconds <= 0.0)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = (chunked.seconds / whole.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(chunked);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария просадки чтения от определения разделителя
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readDetect() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = large();
		// Настройки разбора с заданным разделителем
		const awh::codec::csv::reader_t::settings_t given;
		// Собираемые настройки разбора с определением разделителя
		awh::codec::csv::reader_t::settings_t detected;
		// Включаем определение разделителя по содержимому
		detected.separator = '\0';
		// Выполняем прогон чтения таблицы с заданным разделителем
		const outcome_t assigned = measure(text.size(), ::LARGE_ROUNDS, [&text, &given]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, given);
		});
		// Выполняем прогон чтения таблицы с определением разделителя
		const outcome_t guessed = measure(text.size(), ::LARGE_ROUNDS, [&text, &detected]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, detected);
		});
		/**
		 * Если хотя бы один из прогонов не состоялся
		 */
		if((assigned.seconds <= 0.0) || (guessed.seconds <= 0.0)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = (guessed.seconds / assigned.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(guessed);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки чтения таблицы ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст таблицы
		const string & text = service();
		// Настройки разбора текста таблицы
		const awh::codec::csv::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), ::SMALL_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста таблицы
			return ::read(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария чтения таблицы ответа службы
	 */
	static const bool SERVICE_REGISTERED = awh::benchmark::add(
		"codec/csv: чтение ответа службы", "МБ/с", ::READ_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::readService
	);
	/**
	 * Выполняем регистрацию сценария чтения крупной таблицы
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/csv: чтение крупной таблицы", "МБ/с", ::READ_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::readLarge
	);
	/**
	 * Выполняем регистрацию сценария чтения широкой таблицы
	 */
	static const bool WIDE_REGISTERED = awh::benchmark::add(
		"codec/csv: чтение широкой таблицы", "МБ/с", ::READ_WIDE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::readWide
	);
	/**
	 * Выполняем регистрацию сценария чтения таблицы с преобладанием кавычек
	 */
	static const bool QUOTED_REGISTERED = awh::benchmark::add(
		"codec/csv: чтение полей в кавычках", "МБ/с", ::READ_QUOTED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::readQuoted
	);
	/**
	 * Выполняем регистрацию сценария чтения многострочных полей
	 */
	static const bool MULTILINE_REGISTERED = awh::benchmark::add(
		"codec/csv: чтение многострочных полей", "МБ/с", ::READ_MULTILINE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, ::readMultiline
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на чтение
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/csv: выделения на чтение", "выд./табл.", ::READ_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::readAllocations
	);
	/**
	 * Выполняем регистрацию сценария просадки чтения от подачи кусками
	 */
	static const bool CHUNKED_REGISTERED = awh::benchmark::add(
		"codec/csv: просадка от подачи кусками", "раз", ::READ_CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::readChunked
	);
	/**
	 * Выполняем регистрацию сценария просадки чтения от определения разделителя
	 */
	static const bool DETECT_REGISTERED = awh::benchmark::add(
		"codec/csv: просадка от определения разделителя", "раз", ::READ_DETECT_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::readDetect
	);
	/**
	 * Выполняем регистрацию сценария задержки чтения таблицы ответа службы
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/csv: задержка чтения ответа службы", "мкс/табл.", ::READ_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, ::latencyService
	);
};
