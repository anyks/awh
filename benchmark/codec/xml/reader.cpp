/**
 * @file reader.cpp
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
 * @brief Сценарии измерения потокового чтения текста разметки — мелкие ответы служб,
 *        крупные выгрузки, преобладание атрибутов и содержимого и глубокая вложенность
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
 * Подписываемся на пространство имён бенчмарков контейнера XML
 */
using namespace awh::benchmark::markup;

/**
 * @brief Внутренние параметры и сценарии бенчмарков потокового чтения
 *
 */
namespace {
	/**
	 * @brief Количество разбираемых мелких документов
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество разбираемых крупных документов
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество разбираемых документов с преобладанием одного вида разметки
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 20;

	/**
	 * @brief Пороги пропускной способности потокового чтения в мегабайтах в секунду
	 *
	 * @details Пороги откалиброваны по наименьшему из показателей, снятых на всех
	 *          отладочных стендах, с двукратным запасом: время на занятой машине
	 *          расходится между прогонами на десятки процентов, и порог, заданный
	 *          впритык, поднимал бы ложную тревогу чаще, чем ловил бы настоящую
	 *          регрессию. Двукратного запаса довольно, чтобы заметить возвращение
	 *          любой из уже случавшихся ошибок: квадратичное изъятие разобранного,
	 *          посимвольная дозапись содержимого, повторный проход по метке ради
	 *          места каждого атрибута - каждая из них роняла показатель в три раза
	 *          и более
	 *
	 * @warning Калибровать пороги по рабочей машине нельзя: между нею и самым
	 *          медленным из стендов разница девятикратная. Прежде пороги здесь
	 *          стояли назначенными вслепую, с сорокакратным запасом, и не стерегли
	 *          ничего: чтение ответа SOAP давало 322 МБ/с при пороге в 8
	 *
	 */
	static constexpr double READ_SOAP_THRESHOLD = 17.0;
	/**
	 * @brief Порог пропускной способности чтения описания устройства
	 *
	 */
	static constexpr double READ_DEVICE_THRESHOLD = 30.0;
	/**
	 * @brief Порог пропускной способности чтения крупного документа
	 *
	 */
	static constexpr double READ_LARGE_THRESHOLD = 30.0;
	/**
	 * @brief Порог пропускной способности чтения документа с преобладанием атрибутов
	 *
	 */
	static constexpr double READ_ATTRIBUTES_THRESHOLD = 21.0;
	/**
	 * @brief Порог пропускной способности чтения документа с преобладанием содержимого
	 *
	 */
	static constexpr double READ_CONTENT_THRESHOLD = 43.0;
	/**
	 * @brief Порог пропускной способности чтения глубоко вложенного документа
	 *
	 */
	static constexpr double READ_NESTED_THRESHOLD = 13.0;
	/**
	 * @brief Порог количества выделений памяти на чтение крупного документа
	 *
	 * @details Показатель воспроизводим до единиц и потому годится в порог куда
	 *          больше времени: чтение ведётся на переиспользуемых хранилищах, и
	 *          количество выделений на документ от его размера не зависит. Рост
	 *          показателя означает, что какое-то хранилище перестало переиспользоваться
	 *          и заводится заново на каждый узел
	 *
	 */
	static constexpr double READ_ALLOCATIONS_THRESHOLD = 24.0;
	/**
	 * @brief Порог просадки чтения от подачи текста кусками
	 *
	 * @details Стоимость подачи кусками стережётся здесь, а не в наборе проверок:
	 *          надбавка эта постоянного размера, и счётчики покрытия отладочной
	 *          сборки затушёвывают её напрочь. Замер же на собранном для работы
	 *          коде отличает её надёжно - обратный проход по всему накопленному
	 *          тексту при каждом поданном куске ронял показатель впятеро
	 *
	 * @note Измеряется отношение времени подачи кусками ко времени подачи того же
	 *       текста целиком, а не пропускная способность сама по себе. Отношение
	 *       двух прогонов на одной машине не зависит от её быстродействия, и порог
	 *       ему можно назначить впритык - тогда как порог по мегабайтам в секунду,
	 *       заданный с запасом на медленные машины, пропустил бы просадку впятеро
	 *
	 */
	static constexpr double READ_CHUNKED_THRESHOLD = 2.2;
	/**
	 * @brief Порог пропускной способности чтения объявлений пространств имён
	 *
	 * @details Поиск повторов среди объявлений обязан вестись раскладкой по свёртке
	 *          обозначения: попарное сличение давало на восьми тысячах объявлений
	 *          стократную просадку против нынешнего показателя. Порог назначен
	 *          заведомо выше просевшего показателя и заведомо ниже нынешнего, так
	 *          что возврат попарного сличения он отличает даже на машине,
	 *          уступающей в быстродействии впятеро
	 *
	 */
	static constexpr double READ_DECLARES_THRESHOLD = 34.0;
	/**
	 * @brief Порог задержки чтения ответа по договору SOAP в микросекундах
	 *
	 * @details Ответ службы перенаправления портов - документ на единицы килобайт,
	 *          и на нём решает не пропускная способность, а постоянные издержки на
	 *          запуск разбора. Порог назначен сверху: превышение означает, что
	 *          заведение хранилищ разбора подорожало
	 *
	 * @warning Порог откалиброван по самому медленному из отладочных стендов, а не
	 *          по рабочей машине: OpenBSD показывает здесь вдесятеро больше macOS,
	 *          и порог, снятый с рабочей машины, отказывал бы на стендах всякий раз.
	 *          Показатель этот от быстродействия машины зависит напрямую - в отличие
	 *          от просадки от подачи кусками, которая измеряется отношением
	 *
	 */
	static constexpr double READ_SOAP_LATENCY_THRESHOLD = 20.0;
	/**
	 * @brief Порог задержки чтения описания устройства в микросекундах
	 *
	 */
	static constexpr double READ_DEVICE_LATENCY_THRESHOLD = 110.0;
	/**
	 * @brief Размер куска подачи текста разметки в октетах
	 *
	 * @note Взят равным полезной части кадра сети: именно такими кусками текст
	 *       разметки и приходит с гнезда, и подача его целиком - случай надуманный
	 *
	 */
	static constexpr size_t CHUNK_SIZE = 1460;
	/**
	 * @brief Количество объявлений пространств имён у одного узла
	 *
	 * @warning Количество это обязано укладываться в предел `maxAttributes`
	 *          настроек разбора по умолчанию: за пределом разбор отказывает
	 *          сразу, и замер показал бы скорость отказа вместо скорости
	 *          разбора - при первой сборке сценария он так и показал 600 МБ/с
	 *
	 */
	static constexpr size_t DECLARES_COUNT = 4000;

	/**
	 * @brief Функция потокового чтения текста разметки
	 *
	 * @param text разбираемый текст разметки
	 * @return     количество полученных событий разбора
	 *
	 */
	static uint64_t read(const string & text) noexcept {
		// Объект потокового чтения текста разметки
		awh::codec::xml::reader_t reader;
		/**
		 * Если передать текст разметки не удалось
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
	 * @brief Функция потокового чтения текста разметки, поданного кусками
	 *
	 * @param text разбираемый текст разметки
	 * @return     количество полученных событий разбора
	 *
	 */
	static uint64_t feed(const string & text) noexcept {
		// Объект потокового чтения текста разметки
		awh::codec::xml::reader_t reader;
		// Количество полученных событий разбора
		uint64_t result = 0;
		// Смещение очередного подаваемого куска текста разметки
		size_t offset = 0;
		/**
		 * Выполняем подачу текста разметки до его окончания
		 */
		do {
			// Размер очередного подаваемого куска текста разметки
			const size_t size = (((offset + CHUNK_SIZE) > text.size()) ? (text.size() - offset) : CHUNK_SIZE);
			/**
			 * Если передать очередной кусок текста разметки не удалось
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
			// Выполняем смещение на размер поданного куска текста разметки
			offset += size;
		// Выполняем подачу до исчерпания текста разметки
		} while(offset < text.size());
		// Выводим количество полученных событий разбора
		return result;
	}
	/**
	 * @brief Функция получения эталонного текста с объявлениями пространств имён
	 *
	 * @note Обозначения подобраны различными намеренно: повтор объявления разбор
	 *       отвергает, а искать его приходится среди всех прочих - именно этот
	 *       поиск сценарий и измеряет
	 *
	 * @return эталонный текст разметки
	 *
	 */
	static const string & declares() noexcept {
		// Собираемый эталонный текст разметки
		static const string result = []() noexcept -> string {
			// Собираемый текст разметки
			string result("<r");
			/**
			 * Выполняем запись объявлений пространств имён узла
			 */
			for(size_t i = 0; i < DECLARES_COUNT; i++)
				// Выполняем добавление очередного объявления пространства имён
				result.append(" xmlns:p").append(to_string(i)).append("=\"urn:example:").append(to_string(i)).append("\"");
			// Выводим собранный текст разметки
			return result.append("/>");
		}();
		// Выводим эталонный текст разметки
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения ответа по договору SOAP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readSoap() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = soap();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки
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
	 * @brief Функция прогона сценария чтения описания устройства по договору UPnP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readDevice() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = device();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), (SMALL_ROUNDS / 4), [&text]() noexcept {
			// Выполняем чтение текста разметки
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
	 * @brief Функция прогона сценария чтения крупного документа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки
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
	 * @brief Функция прогона сценария чтения документа с преобладанием атрибутов
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readAttributes() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = attributes();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), FOCUSED_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки
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
	 * @brief Функция прогона сценария чтения документа с преобладанием содержимого
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readContent() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = content();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), FOCUSED_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки
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
	 * @brief Функция прогона сценария чтения глубоко вложенного документа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readNested() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = nested();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки
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
	 * @brief Функция прогона сценария расхода выделений памяти на чтение
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки
			return ::read(text);
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
	 * @brief Функция прогона сценария чтения текста разметки, поданного кусками
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readChunked() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = large();
		// Итоги прогона подачи текста разметки целиком
		const outcome_t whole = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки целиком
			return ::read(text);
		});
		// Итоги прогона подачи текста разметки кусками
		const outcome_t parts = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки кусками
			return ::feed(text);
		});
		/**
		 * Если хотя бы один из прогонов не состоялся
		 */
		if((whole.seconds <= 0.0) || (parts.seconds <= 0.0)){
			// Помечаем измерение как не выполненное
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = "замер времени разбора не состоялся";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = (parts.seconds / whole.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(parts);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения объявлений пространств имён
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readDeclares() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = declares();
		/**
		 * Если разбор эталонного текста отказывает
		 *
		 * @note Проверка эта обязательна: отказавший разбор возвращается мгновенно,
		 *       и без неё сценарий показал бы скорость отказа вместо скорости
		 *       разбора - показатель тем выше, чем раньше разбор сдался
		 */
		if(::read(text) == 0){
			// Помечаем измерение как не выполненное
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = "разбор эталонного текста с объявлениями пространств имён отказал";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), FOCUSED_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки
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
	 * @brief Функция прогона сценария задержки чтения ответа по договору SOAP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencySoap() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = soap();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text]() noexcept {
			// Выполняем чтение текста разметки
			return ::read(text);
		});
		// Устанавливаем измеренное значение
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки чтения описания устройства
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyDevice() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = device();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), (SMALL_ROUNDS / 4), [&text]() noexcept {
			// Выполняем чтение текста разметки
			return ::read(text);
		});
		// Устанавливаем измеренное значение
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария чтения ответа по договору SOAP
	 */
	static const bool SOAP_REGISTERED = awh::benchmark::add(
		"codec/xml: чтение ответа SOAP", "МБ/с", READ_SOAP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readSoap
	);
	/**
	 * Выполняем регистрацию сценария чтения описания устройства
	 */
	static const bool DEVICE_REGISTERED = awh::benchmark::add(
		"codec/xml: чтение описания UPnP", "МБ/с", READ_DEVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readDevice
	);
	/**
	 * Выполняем регистрацию сценария чтения крупного документа
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/xml: чтение крупного документа", "МБ/с", READ_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readLarge
	);
	/**
	 * Выполняем регистрацию сценария чтения документа с преобладанием атрибутов
	 */
	static const bool ATTRIBUTES_REGISTERED = awh::benchmark::add(
		"codec/xml: чтение атрибутов", "МБ/с", READ_ATTRIBUTES_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readAttributes
	);
	/**
	 * Выполняем регистрацию сценария чтения документа с преобладанием содержимого
	 */
	static const bool CONTENT_REGISTERED = awh::benchmark::add(
		"codec/xml: чтение содержимого", "МБ/с", READ_CONTENT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readContent
	);
	/**
	 * Выполняем регистрацию сценария чтения глубоко вложенного документа
	 */
	static const bool NESTED_REGISTERED = awh::benchmark::add(
		"codec/xml: чтение вложенности", "МБ/с", READ_NESTED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readNested
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на чтение
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/xml: выделения на чтение", "выд./док.", READ_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readAllocations
	);
	/**
	 * Выполняем регистрацию сценария чтения текста разметки, поданного кусками
	 */
	static const bool CHUNKED_REGISTERED = awh::benchmark::add(
		"codec/xml: просадка от подачи кусками", "раз", READ_CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readChunked
	);
	/**
	 * Выполняем регистрацию сценария чтения объявлений пространств имён
	 */
	static const bool DECLARES_REGISTERED = awh::benchmark::add(
		"codec/xml: чтение объявлений пространств имён", "МБ/с", READ_DECLARES_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readDeclares
	);
	/**
	 * Выполняем регистрацию сценария задержки чтения ответа по договору SOAP
	 */
	static const bool SOAP_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/xml: задержка чтения ответа SOAP", "мкс/док.", READ_SOAP_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencySoap
	);
	/**
	 * Выполняем регистрацию сценария задержки чтения описания устройства
	 */
	static const bool DEVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/xml: задержка чтения описания UPnP", "мкс/док.", READ_DEVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencyDevice
	);
};
