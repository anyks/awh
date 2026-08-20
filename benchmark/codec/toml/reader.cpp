/**
 * @file reader.cpp
 * @date 2026-08-12
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
 * @brief Бенчмарки потокового чтения текста настроек TOML — пропускная способность
 *        разбора на текстах всех путей, расход выделений памяти, просадка от подачи
 *        текста кусками и задержка разбора мелкого файла настроек
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include "toml.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера TOML
 */
using namespace awh::benchmark::settings;

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
	 * @details Пороги назначены по замеру на рабочей машине 12.08.2026 с запасом
	 *          вчетверо: отладочные стенды отстают от неё вчетверо-впятеро, и порог,
	 *          назначенный по рабочей машине впритык, валил бы прогон на них
	 *
	 * @note Запаса этого довольно, чтобы заметить возвращение уже случавшихся ошибок
	 *       разбора: квадратичное изъятие разобранного из хранилища и обратный проход
	 *       по накопленному тексту при каждом поданном куске роняли показатель у
	 *       кодека настроек INI втрое и более
	 *
	 */
	static constexpr double READ_SERVICE_THRESHOLD = 6.8;
	/**
	 * @brief Порог пропускной способности чтения крупного файла настроек
	 *
	 * @note Запас взят двукратным к самому медленному из известных стендов: на
	 *       OpenBSD 7.9/amd64 сценарий даёт 8.48 МБ/с против 46.90 у рабочей машины
	 *       (13.08.2026). Просадка эта принадлежит машине, а не модулю - у соседних
	 *       кодеков на той же машине отношение то же самое либо хуже: xml 6.1 раза,
	 *       ini 6.1 раза, toml 5.5 раза
	 *
	 */
	static constexpr double READ_LARGE_THRESHOLD = 4.2;
	/**
	 * @brief Порог пропускной способности чтения текста с преобладанием строк
	 *
	 */
	static constexpr double READ_STRINGS_THRESHOLD = 10.0;
	/**
	 * @brief Порог пропускной способности чтения текста с преобладанием чисел
	 *
	 */
	static constexpr double READ_NUMBERS_THRESHOLD = 7.6;
	/**
	 * @brief Порог пропускной способности чтения текста с преобладанием перечней
	 *
	 */
	static constexpr double READ_ARRAYS_THRESHOLD = 5.0;
	/**
	 * @brief Порог расхода выделений памяти на чтение
	 *
	 * @details Величина назначена по съёму 20.08.2026 на ПЯТИ машинах: показатель этот
	 *          делится строго по стандартной библиотеке, а не по системе - libc++ даёт
	 *          26, libstdc++ даёт 33, и внутри каждого набора совпадение до единицы.
	 *          Причина в длине короткого запаса строки: libc++ вмещает в себя 22 октета,
	 *          libstdc++ - 15. Порог взят вдвое выше худшего из снятых
	 *
	 * @warning Прежний порог 48 держался на одном лишь замере с libc++ и оставлял
	 *          libstdc++ запас в полтора раза - его снесла бы всякая мелкая правка. Счёт
	 *          выделений, снятый на одной библиотеке, не значит НИЧЕГО, покуда не сверен
	 *          со второй: у соседнего кодека такая несверенность прятала 460 821
	 *          выделение против 66
	 *
	 */
	static constexpr double READ_ALLOCATIONS_THRESHOLD = 66.0;
	/**
	 * @brief Порог количества выделений памяти на одно объявленное имя
	 *
	 * @details Проверка повторного объявления удерживает имена уже объявленного, и
	 *          хранилище их выделяет память на всякое имя: показатель этот стережёт
	 *          не сам расход, а его устройство - выделение на имя одно, и рост его
	 *          означал бы, что имя стало храниться дороже
	 *
	 * @note Расход этот заложен самим описанием: повтор запрещён, а обнаружить его
	 *       потоком иначе как памятью об объявленном нечем. Снимается он отключением
	 *       проверки - тогда разбор всего файла обходится единицами выделений, что и
	 *       мерит соседний сценарий
	 *
	 */
	static constexpr double READ_DECLARED_THRESHOLD = 5.0;
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
	static constexpr double READ_CHUNKED_THRESHOLD = 2.0;
	/**
	 * @brief Порог задержки чтения файла настроек приложения в микросекундах
	 *
	 * @details Файл настроек приложения - это единицы килобайт, и на нём решает не
	 *          пропускная способность, а постоянные издержки на запуск разбора
	 *
	 * @note Порог держится по самому медленному стенду, а не по рабочей машине: стенд
	 *       FreeBSD равномерно впятеро медленнее её - у XML отношение 5.1 и 5.2, у INI
	 *       4.0, у TOML 5.0, - и порог в тридцать микросекунд, запас к которому был
	 *       вчетверо, там не держался при исправном модуле. Запас взят теперь
	 *       четырнадцатикратным к рабочей машине, наравне с соседними кодеками
	 */
	static constexpr double READ_SERVICE_LATENCY_THRESHOLD = 100.0;

	/**
	 * @brief Функция потокового чтения текста настроек
	 *
	 * @param text разбираемый текст настроек
	 * @return     количество полученных событий разбора
	 *
	 */
	static uint64_t read(const string & text, const bool duplicates = true) noexcept {
		// Настройки разбора текста настроек
		awh::codec::toml::reader_t::settings_t settings;
		// Устанавливаем признак проверки повторного объявления имён
		settings.duplicates = duplicates;
		// Объект потокового чтения текста настроек
		awh::codec::toml::reader_t reader(settings);
		/**
		 * Если передать текст настроек не удалось
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
	 * @brief Функция потокового чтения текста настроек, поданного кусками
	 *
	 * @param text разбираемый текст настроек
	 * @return     количество полученных событий разбора
	 *
	 */
	static uint64_t feed(const string & text) noexcept {
		// Объект потокового чтения текста настроек
		awh::codec::toml::reader_t reader;
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
	 * @brief Функция прогона сценария чтения заданного текста настроек
	 *
	 * @param text   разбираемый текст настроек
	 * @param rounds количество разбираемых файлов настроек
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t reading(const string & text, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), rounds, [&text]() noexcept {
			// Выполняем чтение текста настроек
			return ::read(text);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения файла настроек приложения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readService() noexcept {
		// Выполняем прогон сценария чтения файла настроек приложения
		return ::reading(service(), SMALL_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения крупного файла настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readLarge() noexcept {
		// Выполняем прогон сценария чтения крупного файла настроек
		return ::reading(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения текста с преобладанием строк
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readStrings() noexcept {
		// Выполняем прогон сценария чтения текста с преобладанием строк
		return ::reading(strings(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения текста с преобладанием чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readNumbers() noexcept {
		// Выполняем прогон сценария чтения текста с преобладанием чисел
		return ::reading(numbers(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения текста с преобладанием перечней
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readArrays() noexcept {
		// Выполняем прогон сценария чтения текста с преобладанием перечней
		return ::reading(arrays(), FOCUSED_ROUNDS);
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
		/**
		 * Выполняем прогон измеряемой операции с отключённой проверкой повторов
		 *
		 * @note Проверка повторов удерживает имена объявленного и выделяет память на
		 *       всякое имя: её расход мерит соседний сценарий, а этот стережёт
		 *       переиспользование хранилищ самого разбора, и учёт имён его затушевал
		 *       бы напрочь
		 */
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек
			return ::read(text, false);
		});
		/**
		 * Если ни одной операции не выполнено
		 *
		 * @note Показатель «на одну операцию» при нуле операций выдал бы ноль, а ноль
		 *       укладывается в любой порог с верхней границей: молчание сценария
		 *       отчиталось бы успехом
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор не выполнил ни одной операции";
			// Выводим результат измерения
			return result;
		}
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
		// Разбираемый текст настроек
		const string & text = large();
		// Выполняем прогон подачи текста настроек целиком
		const outcome_t whole = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек, поданного целиком
			return ::read(text);
		});
		// Выполняем прогон подачи текста настроек кусками
		const outcome_t chunked = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек, поданного кусками
			return ::feed(text);
		});
		/**
		 * Если замер не состоялся
		 */
		if((whole.seconds <= 0.0) || (chunked.seconds <= 0.0)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "замер времени не состоялся";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное отношение времени подачи кусками ко времени подачи целиком
		result.value = (chunked.seconds / whole.seconds);
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
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек
			return ::read(text);
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор не выполнил ни одной операции";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция прогона сценария расхода выделений на учёт объявленных имён
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readDeclared() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = large();
		// Количество объявленных имён разбираемого текста настроек
		uint64_t names = 0;
		/**
		 * Выполняем подсчёт объявленных имён разбираемого текста настроек
		 *
		 * @note Считаются объявления таблиц и пар: имя удерживается учётом на всякое
		 *       из них, и показатель считается именно по ним, а не по файлам
		 */
		for(size_t i = 0; i < text.length(); i++){
			/**
			 * Если знаком является начало строки объявления либо пары
			 */
			if(((i == 0) || (text[i - 1] == '\n')) && (text[i] != '\n'))
				// Выполняем учёт очередного объявленного имени
				names++;
		}
		/**
		 * Если объявленных имён в тексте настроек не нашлось
		 */
		if(names == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "объявленных имён в эталонном тексте не нашлось";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста настроек с проверкой повторов
			return ::read(text, true);
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор не выполнил ни одной операции";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		/**
		 * Если учёт выделений памяти не работает
		 */
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное количество выделений памяти на одно объявленное имя
		result.value = (perDocument(outcome) / static_cast <double> (names));
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария чтения файла настроек приложения
	 */
	static const bool SERVICE_REGISTERED = awh::benchmark::add(
		"codec/toml: чтение настроек службы", "МБ/с", READ_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readService
	);
	/**
	 * Выполняем регистрацию сценария чтения крупного файла настроек
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/toml: чтение крупного файла", "МБ/с", READ_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readLarge
	);
	/**
	 * Выполняем регистрацию сценария чтения текста с преобладанием строк
	 */
	static const bool STRINGS_REGISTERED = awh::benchmark::add(
		"codec/toml: чтение строковых значений", "МБ/с", READ_STRINGS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readStrings
	);
	/**
	 * Выполняем регистрацию сценария чтения текста с преобладанием чисел
	 */
	static const bool NUMBERS_REGISTERED = awh::benchmark::add(
		"codec/toml: чтение чисел и отметок времени", "МБ/с", READ_NUMBERS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readNumbers
	);
	/**
	 * Выполняем регистрацию сценария чтения текста с преобладанием перечней
	 */
	static const bool ARRAYS_REGISTERED = awh::benchmark::add(
		"codec/toml: чтение перечней и встроенных таблиц", "МБ/с", READ_ARRAYS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readArrays
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на чтение
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/toml: выделения на чтение", "выд./файл", READ_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readAllocations
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений на учёт объявленных имён
	 */
	static const bool DECLARED_REGISTERED = awh::benchmark::add(
		"codec/toml: выделения на учёт имён", "выд./имя", READ_DECLARED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readDeclared
	);
	/**
	 * Выполняем регистрацию сценария просадки чтения от подачи кусками
	 */
	static const bool CHUNKED_REGISTERED = awh::benchmark::add(
		"codec/toml: просадка от подачи кусками", "раз", READ_CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readChunked
	);
	/**
	 * Выполняем регистрацию сценария задержки чтения файла настроек приложения
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/toml: задержка чтения настроек службы", "мкс/файл", READ_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencyService
	);
};
