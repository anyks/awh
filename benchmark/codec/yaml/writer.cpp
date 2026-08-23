/**
 * @file writer.cpp
 * @date 2026-08-17
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
 * @brief Бенчмарки записи текста YAML — пропускная способность сборки блочных и
 *        поточных построений, стоимость ограды значений и расход выделений памяти
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <vector>
#include "yaml.hpp"
#include <sys/log.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в
 *       одно, порождая порчу вдали от места её причины
 *
 */
namespace {
	/**
	 * @brief Объект журнала проверок с отключённым выводом
	 *
	 * @details Вывод отключается назначением пустого перечня приёмников: отказы
	 *          разбора проверки наводят намеренно, и журнал их засорял бы выдачу
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка проверок
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          проверки, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка проверок
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка проверок
			static awh::fmk_t fmk;
			// Выводим объект фреймворка проверок
			return fmk;
		}
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта журнала проверок
	 *
	 * @return объект журнала проверок
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект журнала проверок
		static Silent silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера YAML
 */
using namespace awh::benchmark::manifest;

/**
 * @brief Внутренние параметры и сценарии бенчмарков записи текста
 *
 */
namespace {
	/**
	 * @brief Количество собираемых мелких файлов настроек
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество пар, записываемых в один крупный файл настроек
	 *
	 */
	static constexpr size_t LARGE_KEYS = 200000;
	/**
	 * @brief Количество собираемых крупных файлов настроек
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;

	/**
	 * @brief Порог пропускной способности записи блочного построения
	 *
	 * @details Пороги назначены по замеру на рабочей машине 17.08.2026 с запасом
	 *          вчетверо: отладочные стенды отстают от неё вчетверо-впятеро
	 *
	 */
	static constexpr double WRITE_BLOCK_THRESHOLD = 12.78;
	/**
	 * @brief Порог пропускной способности записи поточного построения
	 *
	 * @note Поточное построение отступа не пишет вовсе, и запись его обязана идти
	 *       быстрее блочного: показатель этот стережёт именно разницу двух путей
	 *
	 */
	static constexpr double WRITE_FLOW_THRESHOLD = 13.11;
	/**
	 * @brief Порог пропускной способности записи значений с оградою
	 *
	 * @details Ограда решается содержимым: правила разрешения проходятся на всяком
	 *          значении, а двойная ограда вдобавок отменяет знаки последовательностями
	 *
	 */
	static constexpr double WRITE_QUOTED_THRESHOLD = 19.38;
	/**
	 * @brief Порог количества выделений памяти на запись крупного файла настроек
	 *
	 * @details Запись ведётся на одном накопителе, растущем удвоением: количество
	 *          выделений на файл обязано считаться десятками, а не тысячами. Рост
	 *          показателя означает, что накопитель заводится заново на всякую запись
	 *
	 */
	static constexpr double WRITE_ALLOCATIONS_THRESHOLD = 40.0;
	/**
	 * @brief Порог задержки записи файла настроек приложения в микросекундах
	 *
	 */
	static constexpr double WRITE_SERVICE_LATENCY_THRESHOLD = 45.08;

	/**
	 * @brief Функция записи текста настроек заданным построением
	 *
	 * @param layout построение, каким записывается текст настроек
	 * @param keys   количество записываемых пар отображения
	 * @param quoted признак записи значений, ограды требующих
	 * @return       размер собранного текста настроек
	 *
	 */
	/**
	 * @brief Функция получения записей имён пар, заранее собранных
	 *
	 * @details Собираются они ОДИН РАЗ и вне замеряемого места: сборка строки заводит
	 *          память сама по себе, и счёт выделений мерил бы работу оснастки
	 *          вперемешку с работой кодека
	 *
	 * @warning Так и было: из 233 353 выделений прогона 83 333 принадлежали самой
	 *          оснастке. Наружу это торчало лишь у libstdc++, где короткий запас строки
	 *          вмещает 15 октетов, а не 22, и потому на рабочей машине пряталось
	 *          целиком - порог сторожил смесь, а разглядеть её было нечем
	 *
	 * @note Заводится перечень при первом обращении, и обращаться к нему надлежит ДО
	 *       начала замера: перенос сборки в начало замеряемого места её оттуда не
	 *       выносит вовсе - мерится вызов целиком
	 *
	 * @param keys количество собираемых записей
	 * @return     перечень собранных записей имён
	 *
	 */
	static const vector <string> & names(const size_t keys) noexcept {
		// Перечень собранных записей имён пар
		static vector <string> result;
		/**
		 * Если перечень ещё не собран
		 */
		if(result.size() < keys){
			// Выполняем заведение места под записи имён
			result.reserve(keys);
			/**
			 * Выполняем сборку недостающих записей имён
			 */
			while(result.size() < keys)
				// Выполняем сборку очередной записи имени
				result.push_back("key" + to_string(result.size()));
		}
		// Выводим перечень собранных записей имён
		return result;
	}
	/**
	 * @brief Функция получения записей значений, заранее собранных
	 *
	 * @param keys   количество собираемых записей
	 * @param quoted признак того, что записи ограды требуют
	 * @return       перечень собранных записей значений
	 *
	 */
	static const vector <string> & texts(const size_t keys, const bool quoted) noexcept {
		// Перечень собранных записей значений, ограды не требующих
		static vector <string> plain;
		// Перечень собранных записей значений, ограды требующих
		static vector <string> fenced;
		// Получаем перечень, виду записей отвечающий
		vector <string> & result = (quoted ? fenced : plain);
		/**
		 * Если перечень ещё не собран
		 */
		if(result.size() < keys){
			// Выполняем заведение места под записи значений
			result.reserve(keys);
			/**
			 * Выполняем сборку недостающих записей значений
			 */
			while(result.size() < keys)
				// Выполняем сборку очередной записи значения
				result.push_back(quoted ?
					(string("значение: с двоеточием ") + to_string(result.size())) :
					(string("значение") + to_string(result.size())));
		}
		// Выводим перечень собранных записей значений
		return result;
	}
	static uint64_t writing(const awh::codec::yaml::layout_t layout, const size_t keys, const bool quoted = false) noexcept {
		// Настройки записи текста настроек
		awh::codec::yaml::writer_t::settings_t settings;
		// Устанавливаем построение, каким записывается текст
		settings.layout = layout;
		// Объект записи текста настроек
		awh::codec::yaml::writer_t writer(::logger(), settings);
		// Получаем записи имён и значений, заранее собранные
		const vector <string> & names = ::names(keys);
		// Получаем записи значений, заранее собранные
		const vector <string> & texts = ::texts(keys, quoted);
		/**
		 * Если открыть отображение пар не удалось
		 */
		if(!writer.mapping())
			// Выводим нулевой размер собранного текста
			return 0;
		/**
		 * Выполняем запись всех пар отображения
		 */
		for(size_t i = 0; i < keys; i++){
			// Выполняем запись имени очередной пары отображения
			writer.key(names.at(i));
			/**
			 * Если записываются значения, ограды требующие
			 */
			if(quoted)
				// Выполняем запись строкового значения, ограды требующего
				writer.value(texts.at(i));
			/**
			 * Выполняем выбор вида значения очередной пары
			 */
			else switch(i % 4){
				// Если значением является последовательность знаков
				case 0: writer.value(texts.at(i)); break;
				// Если значением является целое число
				case 1: writer.value(static_cast <int64_t> (i * 1000)); break;
				// Если значением является логическое значение
				case 2: writer.value((i % 8) == 2); break;
				// Если значением является число с плавающей точкой
				case 3: writer.value(static_cast <double> (i) + 0.25); break;
			}
		}
		// Выполняем закрытие отображения пар
		writer.close();
		// Выполняем завершение записи текста настроек
		writer.finish();
		// Выводим размер собранного текста настроек
		return static_cast <uint64_t> (writer.size());
	}
	/**
	 * @brief Функция прогона сценария записи текста настроек
	 *
	 * @param layout построение, каким записывается текст настроек
	 * @param quoted признак записи значений, ограды требующих
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t writing(const awh::codec::yaml::layout_t layout, const bool quoted = false) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем размер собираемого текста настроек
		const size_t bytes = static_cast <size_t> (::writing(layout, LARGE_KEYS, quoted));
		/**
		 * Если собрать текст настроек не удалось
		 */
		if(bytes == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "сборка эталонного текста настроек не удалась";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, LARGE_ROUNDS, [layout, quoted]() noexcept {
			// Выполняем запись текста настроек
			return ::writing(layout, LARGE_KEYS, quoted);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи блочного построения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeBlock() noexcept {
		// Выполняем прогон сценария записи блочного построения
		return ::writing(awh::codec::yaml::layout_t::BLOCK);
	}
	/**
	 * @brief Функция прогона сценария записи поточного построения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeFlow() noexcept {
		// Выполняем прогон сценария записи поточного построения
		return ::writing(awh::codec::yaml::layout_t::FLOW);
	}
	/**
	 * @brief Функция прогона сценария записи значений с оградою
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeQuoted() noexcept {
		// Выполняем прогон сценария записи значений с оградою
		return ::writing(awh::codec::yaml::layout_t::BLOCK, true);
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на запись
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем размер собираемого текста настроек
		const size_t bytes = static_cast <size_t> (::writing(awh::codec::yaml::layout_t::BLOCK, LARGE_KEYS));
		/**
		 * Если собрать текст настроек не удалось
		 */
		if(bytes == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "сборка эталонного текста настроек не удалась";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, LARGE_ROUNDS, []() noexcept {
			// Выполняем запись текста настроек
			return ::writing(awh::codec::yaml::layout_t::BLOCK, LARGE_KEYS);
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
			result.reason = "запись не выполнила ни одной операции";
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
	 * @brief Функция прогона сценария задержки записи файла настроек приложения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Количество пар мелкого файла настроек приложения
		static constexpr size_t SERVICE_KEYS = 32;
		// Получаем размер собираемого текста настроек
		const size_t bytes = static_cast <size_t> (::writing(awh::codec::yaml::layout_t::BLOCK, SERVICE_KEYS));
		/**
		 * Если собрать текст настроек не удалось
		 */
		if(bytes == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "сборка эталонного текста настроек не удалась";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, SMALL_ROUNDS, []() noexcept {
			// Выполняем запись текста настроек
			return ::writing(awh::codec::yaml::layout_t::BLOCK, SERVICE_KEYS);
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "запись не выполнила ни одной операции";
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
	 * Выполняем регистрацию сценария записи блочного построения
	 */
	static const bool BLOCK_REGISTERED = awh::benchmark::add(
		"codec/yaml: запись блочного построения", "МБ/с", WRITE_BLOCK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeBlock
	);
	/**
	 * Выполняем регистрацию сценария записи поточного построения
	 */
	static const bool FLOW_REGISTERED = awh::benchmark::add(
		"codec/yaml: запись поточного построения", "МБ/с", WRITE_FLOW_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeFlow
	);
	/**
	 * Выполняем регистрацию сценария записи значений с оградою
	 */
	static const bool QUOTED_REGISTERED = awh::benchmark::add(
		"codec/yaml: запись значений с оградою", "МБ/с", WRITE_QUOTED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeQuoted
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на запись
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/yaml: выделения на запись", "выд./файл", WRITE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, writeAllocations
	);
	/**
	 * Выполняем регистрацию сценария задержки записи файла настроек приложения
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/yaml: задержка записи настроек службы", "мкс/файл", WRITE_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencyService
	);
};
