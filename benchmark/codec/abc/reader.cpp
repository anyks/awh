/**
 * @file reader.cpp
 * @date 2026-08-19
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
 * @brief Замеры потокового чтения бинарного контейнера ABC
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "abc.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::benchmark::binary;

/**
 * @brief Внутренние параметры сценариев потокового чтения
 *
 */
namespace {
	/**
	 * @brief Количество разбираемых записей ответа службы
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество разбираемых крупных записей
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество разбираемых записей с преобладанием одного вида значений
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 20;
	/**
	 * @brief Размер куска подачи записи в октетах
	 *
	 * @note Взят равным полезной части кадра сети: запись приходит такими кусками и с
	 *       гнезда, и с файловой системы, а подача её целиком - случай не самый частый
	 *
	 */
	static constexpr size_t CHUNK_SIZE = 1460;
	/**
	 * @brief Порог объявления размаха вложенного вместимого в значениях
	 *
	 */
	static constexpr uint64_t SPANNED_LIMIT = 8;
	/**
	 * @brief Количество полей груза записи с объявленным размахом
	 *
	 */
	static constexpr size_t SPANNED_FIELDS = 64;
	/**
	 * @brief Количество значений в каждом поле груза записи с объявленным размахом
	 *
	 */
	static constexpr size_t SPANNED_VALUES = 256;

	/**
	 * @brief Пороги пропускной способности потокового чтения в мегабайтах в секунду
	 *
	 * @details Пороги назначены по замеру на рабочей машине 19.08.2026 с запасом
	 *          вчетверо-впятеро: отладочные стенды отстают от неё вчетверо-впятеро, и
	 *          порог, назначенный по рабочей машине впритык, валил бы прогон на них
	 *
	 */
	static constexpr double READ_SERVICE_THRESHOLD = 8.0;
	/**
	 * @brief Порог пропускной способности чтения крупной записи
	 *
	 */
	static constexpr double READ_LARGE_THRESHOLD = 10.0;
	/**
	 * @brief Порог пропускной способности чтения записи с преобладанием чисел
	 *
	 */
	static constexpr double READ_NUMBERS_THRESHOLD = 12.0;
	/**
	 * @brief Порог пропускной способности чтения записи с преобладанием строк
	 *
	 */
	static constexpr double READ_STRINGS_THRESHOLD = 53.0;
	/**
	 * @brief Порог пропускной способности чтения записи с двоичными значениями
	 *
	 */
	static constexpr double READ_BLOBS_THRESHOLD = 93.0;
	/**
	 * @brief Порог пропускной способности чтения записи с глубокой вложенностью
	 *
	 */
	static constexpr double READ_NESTED_THRESHOLD = 2.5;
	/**
	 * @brief Порог количества выделений памяти на чтение крупной записи
	 *
	 * @details Показатель воспроизводим до единиц и потому годится в порог куда больше
	 *          времени: чтение ведётся на переиспользуемых хранилищах, и количество
	 *          выделений на запись от её размера не зависит. Рост показателя означает,
	 *          что какое-то хранилище перестало переиспользоваться
	 *
	 */
	static constexpr double READ_ALLOCATIONS_THRESHOLD = 48.0;
	/**
	 * @brief Порог просадки чтения от подачи записи кусками
	 *
	 * @note Измеряется отношение времени подачи кусками ко времени подачи той же записи
	 *       целиком, а не пропускная способность сама по себе. Отношение двух прогонов
	 *       на одной машине от её быстродействия не зависит, и порог ему можно назначить
	 *       впритык
	 *
	 */
	static constexpr double READ_CHUNKED_THRESHOLD = 1.26;
	/**
	 * @brief Порог задержки чтения записи ответа службы в микросекундах
	 *
	 * @note Порог держится по самому медленному стенду, а не по рабочей машине: стенд
	 *       FreeBSD равномерно впятеро медленнее её
	 *
	 */
	static constexpr double READ_SERVICE_LATENCY_THRESHOLD = 8.0;
	/**
	 * @brief Порог выигрыша от пропуска вложенного груза по объявленному размаху
	 *
	 * @details Измеряется отношение времени разбора записи целиком ко времени разбора
	 *          её же с пропуском вложенного груза, а не быстродействие само по себе.
	 *          Отношение двух прогонов на одной машине от её быстродействия не зависит,
	 *          и порог ему можно назначить впритык
	 *
	 * @note Порог стережёт не скорость, а самоё работу пропуска: разбор, перешагнувший
	 *       груз по объявленной длине, обязан быть кратно быстрее прошедшего его насквозь.
	 *       Утрата пропуска обрушила бы показатель к единице
	 *
	 * @details Замер 22.08.2026 по десяти стендам: OpenBSD 63.2, macOS 62.6, OpenIndiana
	 *          56.5, NetBSD 56.2, FreeBSD 55.0, DragonFly 54.6, Fedora 54.4, OpenSuse
	 *          54.0, Solaris 53.4, Debian 52.5 и Alpine 51.7. Разброс полутора крат,
	 *          выбросов нет; порог назначен по дну с запасом в полтора раза
	 *
	 * @note Первая калибровка дала у Alpine 18.0 против 51.6-62.1 у прочих, и просадка
	 *       эта списалась на musl. Довод был негоден дважды: Alpine отличался от Fedora
	 *       СРАЗУ и libc, и поколением собирателя, а стенд на musl у нас один. Сличение
	 *       с одной разницей (OpenSuse, gcc 15.3 на glibc - 52.7) сняло собирателя, а
	 *       щуп нашёл настоящую причину: сценарий заводил НОВЫЙ разбиратель на всякую
	 *       запись, а с ним и новый буфер. Выделение 32 КБ с первым касанием стоит на
	 *       Alpine 23.3 мкс против 1.1 мкс у OpenSuse - musl берёт память у ядра заново,
	 *       glibc держит свою. Плата эта от пропуска не зависит и в дешёвом прогоне
	 *       становится главной: сценарий мерил распределитель. Переиспользуемый
	 *       разбиратель вернул Alpine к 51.7, и просадки не стало вовсе
	 *
	 */
	static constexpr double READ_SKIPPED_THRESHOLD = 35.0;
	/**
	 * @brief Порог платы за заведение разбирателя в микросекундах на запись
	 *
	 * @details Замер 22.08.2026 по одиннадцати стендам: macOS 0.22, Fedora 0.28,
	 *          OpenSuse 0.31, Debian 0.33, NetBSD 0.34, FreeBSD 0.48, DragonFly 0.58,
	 *          OpenIndiana 0.73, Solaris 0.84, Alpine 1.10 и OpenBSD 2.54. Дно берёт
	 *          OpenBSD - там и доступ к памяти иной, и таймеры дороги; порог назначен
	 *          по нему с запасом вдвое
	 *
	 * @note Плата эта - не изъян кодека, а цена заведения буфера. Стережётся она ради
	 *       иного: заведение разбирателя обязано остаться дешёвым. Ранняя укладка чего
	 *       бы то ни было в конструктор поднимет показатель немедленно
	 *
	 */
	static constexpr double READ_SETUP_THRESHOLD = 5.0;
	/**
	 * @brief Порог расхода выделений памяти на чтение переиспользуемым разбирателем
	 *
	 * @details Порог НУЛЕВОЙ, и это договор, а не запас: разбор записи разбирателем,
	 *          удержанным от прежней записи, не выделяет памяти ВОВСЕ. Проверено
	 *          22.08.2026 на девяти стендах - macOS, FreeBSD, NetBSD, OpenBSD, Solaris,
	 *          OpenIndiana, DragonFly, Debian и Alpine, - всюду ровно ноль, и на libc++,
	 *          и на libstdc++
	 *
	 * @note Показатель этот от машины не зависит и оттого годен договором, тогда как
	 *       плата за заведение в микросекундах разнится по стендам в двенадцать раз.
	 *       Он же стережёт и сохранение вместимости буфера при `reset()`: освобождение
	 *       буфера сбросом даёт выделение на всякую запись и валит порог немедленно
	 *
	 */
	static constexpr double READ_KEPT_ALLOCATIONS_THRESHOLD = 0.0;

	/**
	 * @brief Функция потокового чтения записи
	 *
	 * @note Название `read` здесь занято работой POSIX: свободная работа с таким именем
	 *       видна из заголовочных файлов системы, и вызов ушёл бы не туда
	 *
	 * @param record разбираемая запись
	 * @return       количество полученных событий разбора
	 *
	 */
	static uint64_t scan(const vector <uint8_t> & record) noexcept {
		// Разбиратель бинарной записи
		awh::codec::abc::reader_t reader;
		// Если подать запись разбирателю не удалось
		if(!reader.feed(record.data(), record.size(), true))
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
	 * @brief Функция потокового чтения записи, поданной кусками
	 *
	 * @param record разбираемая запись
	 * @return       количество полученных событий разбора
	 *
	 */
	static uint64_t feed(const vector <uint8_t> & record) noexcept {
		// Разбиратель бинарной записи
		awh::codec::abc::reader_t reader;
		// Количество полученных событий разбора
		uint64_t result = 0;
		// Смещение очередного подаваемого куска записи
		size_t offset = 0;
		/**
		 * Выполняем подачу записи до её окончания
		 */
		do {
			// Размер очередного подаваемого куска записи
			const size_t size = (((offset + CHUNK_SIZE) > record.size()) ? (record.size() - offset) : CHUNK_SIZE);
			// Если передать очередной кусок записи не удалось
			if(!reader.feed(record.data() + offset, size, ((offset + size) >= record.size())))
				// Выводим нулевое количество событий разбора
				return 0;
			/**
			 * Выполняем перебор всех событий, полученных из очередного куска
			 */
			while(reader.next())
				// Выполняем подсчёт полученных событий разбора
				result++;
			// Выполняем смещение на размер поданного куска записи
			offset += size;
		// Выполняем подачу до исчерпания записи
		} while(offset < record.size());
		// Выводим количество полученных событий разбора
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения заданной записи
	 *
	 * @param record разбираемая запись
	 * @param rounds количество разбираемых записей
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t reading(const vector <uint8_t> & record, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), rounds, [&record]() noexcept {
			// Выполняем чтение записи
			return ::scan(record);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария чтения записи ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readService() noexcept {
		// Выполняем прогон сценария чтения записи ответа службы
		return ::reading(service(), SMALL_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения крупной записи
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readLarge() noexcept {
		// Выполняем прогон сценария чтения крупной записи
		return ::reading(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения записи с преобладанием чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readNumbers() noexcept {
		// Выполняем прогон сценария чтения записи с преобладанием чисел
		return ::reading(numbers(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения записи с преобладанием строк
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readStrings() noexcept {
		// Выполняем прогон сценария чтения записи с преобладанием строк
		return ::reading(strings(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения записи с двоичными значениями
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readBlobs() noexcept {
		// Выполняем прогон сценария чтения записи с двоичными значениями
		return ::reading(blobs(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария чтения записи с глубокой вложенностью
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readNested() noexcept {
		// Выполняем прогон сценария чтения записи с глубокой вложенностью
		return ::reading(nested(), FOCUSED_ROUNDS);
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
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), LARGE_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи
			return ::scan(record);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Если учёт выделений памяти не работает
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное количество выделений памяти на одну запись
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария просадки чтения от подачи записи кусками
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readChunked() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = large();
		// Выполняем прогон чтения записи, поданной целиком
		const outcome_t whole = measure(record.size(), LARGE_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи, поданной целиком
			return ::scan(record);
		});
		// Выполняем прогон чтения записи, поданной кусками
		const outcome_t sliced = measure(record.size(), LARGE_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи, поданной кусками
			return ::feed(record);
		});
		/**
		 * Если время подачи записи целиком не измерено
		 */
		if(whole.seconds <= 0.0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "время подачи записи целиком не измерено";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренную просадку чтения
		result.value = (sliced.seconds / whole.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(sliced);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки чтения записи ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = service();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), SMALL_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи
			return ::scan(record);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную задержку чтения записи
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция получения записи с объявленным размахом вложенных перечней
	 *
	 * @details Запись изображает случай, ради какого метка размаха и заведена: крупный
	 *          груз лежит вложенными перечнями, а потребителю нужно одно поле рядом с
	 *          ними. Без метки размаха разбор обязан пройти груз целиком, с меткою -
	 *          перешагнуть его по объявленной длине
	 *
	 * @return запись с объявленным размахом вложенных перечней
	 *
	 */
	static const vector <uint8_t> & spanned() noexcept {
		// Запись с объявленным размахом вложенных перечней
		static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
			// Сборка бинарной записи
			awh::codec::abc::writer_t writer;
			// Настройки сборки записи
			awh::codec::abc::writer_t::settings_t settings = writer.settings();
			// Выполняем установку порога объявления размаха
			settings.spanned = SPANNED_LIMIT;
			// Выполняем установку настроек сборки записи
			writer.settings(settings);
			// Если открыть отображение записи не удалось
			if(!writer.mapBegin(static_cast <uint64_t> (SPANNED_FIELDS + 1)))
				// Выводим пустую запись
				return vector <uint8_t> ();
			/**
			 * Выполняем укладку полей груза записи
			 */
			for(size_t i = 0; i < SPANNED_FIELDS; i++){
				// Если уложить имя очередного поля груза не удалось
				if(!(writer.text("груз" + to_string(i)) &&
				     writer.arrayBegin(static_cast <uint64_t> (SPANNED_VALUES))))
					// Выводим пустую запись
					return vector <uint8_t> ();
				/**
				 * Выполняем укладку значений очередного поля груза
				 */
				for(size_t j = 0; j < SPANNED_VALUES; j++){
					// Если уложить очередное значение поля груза не удалось
					if(!writer.number(static_cast <uint64_t> (j)))
						// Выводим пустую запись
						return vector <uint8_t> ();
				}
				// Если закрыть перечень значений очередного поля груза не удалось
				if(!writer.arrayEnd())
					// Выводим пустую запись
					return vector <uint8_t> ();
			}
			// Если уложить искомое поле записи не удалось
			if(!(writer.text("хвост") && writer.text("искомое") && writer.mapEnd()))
				// Выводим пустую запись
				return vector <uint8_t> ();
			// Выводим собранную запись
			return writer.record();
		}();
		// Выводим запись с объявленным размахом вложенных перечней
		return result;
	}
	/**
	 * @brief Функция потокового чтения записи прямой выдачей событий
	 *
	 * @note Пропуск зовётся ТОЛЬКО из обработчика прямой выдачи: события копятся
	 *       очередью, и к выдаче события разбор уходит вперёд - пропускать к тому
	 *       времени уже нечего
	 *
	 * @param record   разбираемая запись
	 * @param skipping признак пропуска вложенных перечней целиком
	 * @return         количество полученных событий разбора
	 *
	 */
	static uint64_t sweep(const vector <uint8_t> & record, const bool skipping) noexcept {
		/**
		 * @brief Переиспользуемый разбиратель записи
		 *
		 * @details Разбиратель заводится ОДИН на все прогоны намеренно. Новый разбиратель
		 *          на всякую запись заводит и новый буфер, а плата за него от пропуска не
		 *          зависит вовсе и в дешёвом прогоне становится главной. Замер 22.08.2026
		 *          вскрыл это на Alpine: там выделение 32 КБ с первым касанием стоит
		 *          23.3 мкс против 1.1 мкс у OpenSuse (musl берёт память у ядра заново
		 *          всякий раз, glibc держит свою), и отношение просело с 55.4 до 18.0 -
		 *          сценарий мерил распределитель, а не пропуск. Потребитель поточного
		 *          разбирателя держит его между записями точно так же
		 *
		 */
		static awh::codec::abc::reader_t reader;
		// Выполняем сброс разбирателя под очередной прогон
		reader.reset();
		/**
		 * @brief Опора прямой выдачи событий разбора
		 *
		 */
		struct Sink {
			// Признак пропуска вложенных перечней целиком
			bool skipping = false;
			// Количество снятых событий разбора
			uint64_t counted = 0;
			// Признак снятия искомого значения записи
			bool found = false;
		} sink;
		// Выполняем установку признака пропуска вложенных перечней
		sink.skipping = skipping;
		// Выполняем установку обработчика прямой выдачи событий разбора
		reader.handler([](void * context, awh::codec::abc::reader_t & reader, const awh::codec::abc::event_t event) noexcept -> void {
			// Выполняем получение опоры прямой выдачи событий
			Sink * sink = reinterpret_cast <Sink *> (context);
			// Выполняем учёт снятого события разбора
			sink->counted++;
			// Если снято событие начала перечня и пропуск затребован
			if(sink->skipping && (event == awh::codec::abc::event_t::ARRAY_BEGIN))
				// Выполняем пропуск вложенного перечня целиком
				(void) reader.skip();
			// Если снято искомое значение записи
			else if((event == awh::codec::abc::event_t::STRING) && (reader.value().data == "искомое"))
				// Выполняем запоминание снятия искомого значения записи
				sink->found = true;
		}, &sink);
		// Если подать запись разбирателю не удалось
		if(!reader.feed(record.data(), record.size(), true))
			// Выводим нулевое количество событий разбора
			return 0;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next())
			// Выполняем продвижение разбора записи
			;
		/**
		 * Если искомое значение записи снято не было
		 *
		 * @note Поверка обязательна: разбор, отвалившийся на первом же событии, без
		 *       неё отчитался бы самым быстрым прогоном, и пропуск изобразил бы
		 *       выигрыш там, где записи не разобрано вовсе
		 */
		if(!sink.found)
			// Выводим нулевое количество событий разбора
			return 0;
		// Выводим количество полученных событий разбора
		return sink.counted;
	}
	/**
	 * @brief Функция потокового чтения записи переиспользуемым разбирателем
	 *
	 * @note Разбиратель заводится ОДИН на все прогоны: потребитель поточного разбора
	 *       держит его между записями точно так же, а плата за заведение вынесена в
	 *       свой сценарий
	 *
	 * @param record разбираемая запись
	 * @return       количество полученных событий разбора
	 *
	 */
	static uint64_t keep(const vector <uint8_t> & record) noexcept {
		// Переиспользуемый разбиратель бинарной записи
		static awh::codec::abc::reader_t reader;
		// Выполняем сброс разбирателя под очередную запись
		reader.reset();
		// Если подать запись разбирателю не удалось
		if(!reader.feed(record.data(), record.size(), true))
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
	 * @brief Функция прогона сценария платы за заведение разбирателя
	 *
	 * @details Меряется разница между разбором новым разбирателем на всякую запись и
	 *          разбором разбирателем переиспользуемым. Разница эта - плата за буфер:
	 *          новый разбиратель заводит его заново, переиспользуемый держит вместимость
	 *          от прежней записи
	 *
	 * @note Сценарий заведён находкой 22.08.2026: плата эта на стендах Linux забирает
	 *       41-47 процентов замера чтения крупной записи, а на macOS лишь 6.5, и оттого
	 *       сличение пропускной способности МЕЖДУ системами ею искажено. Здесь она
	 *       вынесена наружу и стережётся отдельно: пусть лучше стоит в отчёте своей
	 *       строкой, чем прячется в чужих
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readSetup() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = service();
		// Выполняем прогон разбора новым разбирателем на всякую запись
		const outcome_t fresh = measure(record.size(), SMALL_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи новым разбирателем
			return ::scan(record);
		});
		// Выполняем прогон разбора переиспользуемым разбирателем
		const outcome_t kept = measure(record.size(), SMALL_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи переиспользуемым разбирателем
			return ::keep(record);
		});
		// Если сценарий работы не выполнил
		if(!worked(fresh, result) || !worked(kept, result))
			// Выводим результат измерения
			return result;
		/**
		 * Если разбор новым разбирателем оказался не дороже
		 *
		 * @note Отрицательная плата означает, что мерилось не то: разбор новым
		 *       разбирателем дешевле переиспользуемого быть не может
		 */
		if(fresh.seconds <= kept.seconds){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор новым разбирателем не дороже переиспользуемого";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренную плату за заведение разбирателя
		result.value = (perLatency(fresh) - perLatency(kept));
		// Устанавливаем сведения о прогоне
		result.details = details(fresh);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария выигрыша от пропуска вложенного груза
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readSkipped() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = ::spanned();
		// Выполняем прогон разбора записи целиком
		const outcome_t whole = measure(record.size(), FOCUSED_ROUNDS, [&record]() noexcept {
			// Выполняем разбор записи целиком
			return ::sweep(record, false);
		});
		// Выполняем прогон разбора записи с пропуском вложенного груза
		const outcome_t skipped = measure(record.size(), FOCUSED_ROUNDS, [&record]() noexcept {
			// Выполняем разбор записи с пропуском вложенного груза
			return ::sweep(record, true);
		});
		// Если сценарий работы не выполнил
		if(!worked(whole, result) || !worked(skipped, result))
			// Выводим результат измерения
			return result;
		/**
		 * Если время разбора записи с пропуском не измерено
		 */
		if(skipped.seconds <= 0.0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "время разбора записи с пропуском не измерено";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренный выигрыш от пропуска вложенного груза
		result.value = (whole.seconds / skipped.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(skipped);
		// Выводим результат измерения
		return result;
	}
	/**
	 * Выполняем регистрацию сценария чтения записи ответа службы
	 */
	static const bool SERVICE_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение ответа службы", "МБ/с", READ_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readService
	);
	/**
	 * Выполняем регистрацию сценария чтения крупной записи
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение крупной записи", "МБ/с", READ_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readLarge
	);
	/**
	 * Выполняем регистрацию сценария чтения записи с преобладанием чисел
	 */
	static const bool NUMBERS_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение чисел", "МБ/с", READ_NUMBERS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readNumbers
	);
	/**
	 * Выполняем регистрацию сценария чтения записи с преобладанием строк
	 */
	static const bool STRINGS_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение строк", "МБ/с", READ_STRINGS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readStrings
	);
	/**
	 * Выполняем регистрацию сценария чтения записи с двоичными значениями
	 */
	static const bool BLOBS_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение двоичных значений", "МБ/с", READ_BLOBS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readBlobs
	);
	/**
	 * Выполняем регистрацию сценария чтения записи с глубокой вложенностью
	 */
	static const bool NESTED_REGISTERED = awh::benchmark::add(
		"codec/abc: чтение вложенности", "МБ/с", READ_NESTED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readNested
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на чтение
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/abc: выделения на чтение", "выд./зап.", READ_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readAllocations
	);
	/**
	 * Выполняем регистрацию сценария просадки чтения от подачи кусками
	 */
	static const bool CHUNKED_REGISTERED = awh::benchmark::add(
		"codec/abc: просадка от подачи кусками", "раз", READ_CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readChunked
	);
	/**
	 * Выполняем регистрацию сценария задержки чтения записи ответа службы
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/abc: задержка чтения ответа службы", "мкс/зап.", READ_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencyService
	);
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на чтение
	 *        переиспользуемым разбирателем
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t readKeptAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись
		const vector <uint8_t> & record = service();
		/**
		 * Если запись разобрать не удалось
		 *
		 * @note Сторож обязателен: показатель здесь НУЛЕВОЙ, и молчащий разбор дал бы
		 *       ровно тот же ноль. Без поверки событий сценарий отчитывался бы успехом
		 *       при кодеке, не разбирающем ничего
		 */
		if(::keep(record) == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "запись не разобрана переиспользуемым разбирателем";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), SMALL_ROUNDS, [&record]() noexcept {
			// Выполняем чтение записи переиспользуемым разбирателем
			return ::keep(record);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренный расход выделений памяти
		result.value = (static_cast <double> (outcome.allocations) / static_cast <double> (outcome.operations));
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария расхода выделений на чтение переиспользуемым
	 */
	static const bool KEPT_ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/abc: выделения переиспользуемым", "выд./зап.", READ_KEPT_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readKeptAllocations
	);
	/**
	 * Выполняем регистрацию сценария платы за заведение разбирателя
	 */
	static const bool SETUP_REGISTERED = awh::benchmark::add(
		"codec/abc: плата за заведение разбирателя", "мкс/зап.", READ_SETUP_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, readSetup
	);
	/**
	 * Выполняем регистрацию сценария выигрыша от пропуска вложенного груза
	 */
	static const bool SKIPPED_REGISTERED = awh::benchmark::add(
		"codec/abc: выигрыш от пропуска груза", "раз", READ_SKIPPED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, readSkipped
	);
};
