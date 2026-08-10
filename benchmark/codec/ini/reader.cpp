/**
 * @file: reader.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Бенчмарки потокового чтения текста настроек INI — пропускная способность на
 *        файлах разного склада, расход выделений памяти, просадка от подачи кусками
 *        и задержка обработки одного файла настроек
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков контейнера INI
 */
#include "ini.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера INI
 */
using namespace awh::benchmark::config;

/**
 * @brief Внутренние параметры и сценарии бенчмарков потокового чтения
 *
 */
namespace {
	/**
	 * @brief Количество разбираемых мелких файлов настроек
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество разбираемых крупных файлов настроек
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество разбираемых файлов с преобладанием одного вида записи
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 20;
	/**
	 * @brief Размер куска подачи текста настроек в октетах
	 *
	 * @note Взят равным полезной части кадра сети: текст настроек приходит такими
	 *       кусками и с гнезда, и с файловой системы, а подача его целиком - случай
	 *       не самый частый
	 *
	 */
	static constexpr size_t CHUNK_SIZE = 1460;

	/**
	 * @brief Пороги пропускной способности потокового чтения в мегабайтах в секунду
	 *
	 * @details Пороги получены из показателей сборки для работы на рабочей машине -
	 *          Apple Silicon - делением на восемнадцать: девятикратный запас на
	 *          отставание самого медленного из отладочных стендов и двукратный на
	 *          разброс между прогонами. Так, чтение настроек службы даёт здесь
	 *          142 МБ/с, и порогу назначено 8
	 *
	 * @warning Пороги эти назначены расчётом, а не замером на стендах, и потому
	 *          подлежат уточнению после первого прогона там. Расчётный запас взят
	 *          из уже известного отношения между рабочей машиной и стендами, но
	 *          отношение это по сценариям расходится: у бенчмарков контейнера XML
	 *          самым медленным оказался OpenBSD, отстающий вдесятеро лишь на
	 *          задержке, а на пропускной способности - вчетверо
	 *
	 * @note Двукратного запаса довольно, чтобы заметить возвращение уже случавшихся
	 *       ошибок разбора: квадратичное изъятие разобранного из хранилища и
	 *       обратный проход по накопленному тексту при каждом поданном куске роняли
	 *       показатель в три раза и более
	 *
	 */
	static constexpr double READ_SERVICE_THRESHOLD = 8.0;
	/**
	 * @brief Порог пропускной способности чтения настроек по образцу Git
	 *
	 */
	static constexpr double READ_REPOSITORY_THRESHOLD = 7.0;
	/**
	 * @brief Порог пропускной способности чтения крупного файла настроек
	 *
	 */
	static constexpr double READ_LARGE_THRESHOLD = 5.5;
	/**
	 * @brief Порог пропускной способности чтения текста с преобладанием примечаний
	 *
	 */
	static constexpr double READ_ANNOTATED_THRESHOLD = 9.0;
	/**
	 * @brief Порог количества выделений памяти на чтение крупного файла настроек
	 *
	 * @details Показатель воспроизводим до единиц и потому годится в порог куда
	 *          больше времени: чтение ведётся на переиспользуемых хранилищах, и
	 *          количество выделений на файл от его размера не зависит. Рост
	 *          показателя означает, что какое-то хранилище перестало
	 *          переиспользоваться и заводится заново на каждую строку
	 *
	 */
	static constexpr double READ_ALLOCATIONS_THRESHOLD = 24.0;
	/**
	 * @brief Порог просадки чтения от подачи текста кусками
	 *
	 * @details Стоимость подачи кусками стережётся здесь, а не в наборе проверок:
	 *          надбавка эта постоянного размера, и счётчики покрытия отладочной
	 *          сборки затушёвывают её напрочь
	 *
	 * @note Измеряется отношение времени подачи кусками ко времени подачи того же
	 *       текста целиком, а не пропускная способность сама по себе. Отношение
	 *       двух прогонов на одной машине от её быстродействия не зависит, и порог
	 *       ему можно назначить впритык
	 *
	 */
	static constexpr double READ_CHUNKED_THRESHOLD = 2.2;
	/**
	 * @brief Порог задержки чтения файла настроек приложения в микросекундах
	 *
	 * @details Файл настроек приложения - это единицы килобайт, и на нём решает не
	 *          пропускная способность, а постоянные издержки на запуск разбора.
	 *          Превышение порога означает, что заведение хранилищ разбора подорожало
	 *
	 */
	static constexpr double READ_SERVICE_LATENCY_THRESHOLD = 60.0;

	/**
	 * @brief Функция потокового чтения текста настроек
	 *
	 * @param text     разбираемый текст настроек
	 * @param settings настройки разбора текста настроек
	 * @return         количество полученных событий разбора
	 *
	 */
	static uint64_t read(const string & text, const awh::codec::ini::reader_t::settings_t & settings) noexcept {
		// Объект потокового чтения текста настроек
		awh::codec::ini::reader_t reader(settings);
		/**
		 * Если передать текст настроек не удалось
		 */
		if(!reader.feed(text))
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
	 * @brief Функция потокового чтения текста настроек, поданного кусками
	 *
	 * @param text разбираемый текст настроек
	 * @return     количество полученных событий разбора
	 *
	 */
	static uint64_t feed(const string & text) noexcept {
		// Объект потокового чтения текста настроек
		awh::codec::ini::reader_t reader;
		// Количество полученных событий разбора
		uint64_t result = 0;
		// Смещение очередного подаваемого куска текста настроек
		size_t offset = 0;
		/**
		 * Выполняем подачу текста настроек до его окончания
		 */
		do {
			// Размер очередного подаваемого куска текста настроек
			const size_t size = (((offset + CHUNK_SIZE) > text.size()) ? (text.size() - offset) : CHUNK_SIZE);
			/**
			 * Если передать очередной кусок текста настроек не удалось
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
			// Выполняем смещение на размер поданного куска текста настроек
			offset += size;
		// Выполняем подачу до исчерпания текста настроек
		} while(offset < text.size());
		// Выводим количество полученных событий разбора
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения файла настроек приложения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = service();
		// Настройки разбора текста настроек
		const awh::codec::ini::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста настроек
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
	 * @brief Функция прогона сценария чтения настроек по образцу Git
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readRepository() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = repository();
		// Получаем настройки разбора наречия настроек Git
		const awh::codec::ini::reader_t::settings_t settings = awh::codec::ini::reader_t::settings_t::git();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста настроек
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
	 * @brief Функция прогона сценария чтения крупного файла настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = large();
		// Настройки разбора текста настроек
		const awh::codec::ini::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста настроек
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
	 * @brief Функция прогона сценария чтения текста с преобладанием примечаний
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readAnnotated() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = annotated();
		// Настройки разбора текста настроек
		const awh::codec::ini::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), FOCUSED_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста настроек
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
		// Разбираемый текст настроек
		const string & text = large();
		// Настройки разбора текста настроек
		const awh::codec::ini::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста настроек
			return ::read(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария просадки чтения от подачи текста кусками
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readChunked() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = large();
		// Настройки разбора текста настроек
		const awh::codec::ini::reader_t::settings_t settings;
		// Выполняем прогон подачи текста настроек целиком
		const outcome_t whole = measure(text.size(), LARGE_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста настроек
			return ::read(text, settings);
		});
		// Выполняем прогон подачи текста настроек кусками
		const outcome_t chunked = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек, поданного кусками
			return ::feed(text);
		});
		/**
		 * Устанавливаем измеренное значение
		 *
		 * @note Измеряется отношение времён, а не пропускная способность: оно от
		 *       быстродействия машины не зависит и потому сличается с порогом,
		 *       заданным впритык
		 */
		result.value = ((whole.seconds > 0.0) ? (chunked.seconds / whole.seconds) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = details(chunked);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки чтения файла настроек приложения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = service();
		// Настройки разбора текста настроек
		const awh::codec::ini::reader_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем чтение текста настроек
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
	 * Выполняем регистрацию сценария чтения файла настроек приложения
	 */
	static const bool SERVICE_REGISTERED = awh::benchmark::add(
		"codec/ini: чтение настроек службы", "МБ/с", READ_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readService
	);
	/**
	 * Выполняем регистрацию сценария чтения настроек по образцу Git
	 */
	static const bool REPOSITORY_REGISTERED = awh::benchmark::add(
		"codec/ini: чтение настроек Git", "МБ/с", READ_REPOSITORY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readRepository
	);
	/**
	 * Выполняем регистрацию сценария чтения крупного файла настроек
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/ini: чтение крупного файла", "МБ/с", READ_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readLarge
	);
	/**
	 * Выполняем регистрацию сценария чтения текста с преобладанием примечаний
	 */
	static const bool ANNOTATED_REGISTERED = awh::benchmark::add(
		"codec/ini: чтение примечаний", "МБ/с", READ_ANNOTATED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readAnnotated
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на чтение
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/ini: выделения на чтение", "выд./файл", READ_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readAllocations
	);
	/**
	 * Выполняем регистрацию сценария просадки чтения от подачи кусками
	 */
	static const bool CHUNKED_REGISTERED = awh::benchmark::add(
		"codec/ini: просадка от подачи кусками", "раз", READ_CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readChunked
	);
	/**
	 * Выполняем регистрацию сценария задержки чтения файла настроек приложения
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/ini: задержка чтения настроек службы", "мкс/файл", READ_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencyService
	);
};
