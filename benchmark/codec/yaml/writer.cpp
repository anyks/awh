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
#include "yaml.hpp"

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
	static constexpr double WRITE_BLOCK_THRESHOLD = 8.0;
	/**
	 * @brief Порог пропускной способности записи поточного построения
	 *
	 * @note Поточное построение отступа не пишет вовсе, и запись его обязана идти
	 *       быстрее блочного: показатель этот стережёт именно разницу двух путей
	 *
	 */
	static constexpr double WRITE_FLOW_THRESHOLD = 8.0;
	/**
	 * @brief Порог пропускной способности записи значений с оградою
	 *
	 * @details Ограда решается содержимым: правила разрешения проходятся на всяком
	 *          значении, а двойная ограда вдобавок отменяет знаки последовательностями
	 *
	 */
	static constexpr double WRITE_QUOTED_THRESHOLD = 4.0;
	/**
	 * @brief Порог количества выделений памяти на запись крупного файла настроек
	 *
	 * @details Запись ведётся на одном накопителе, растущем удвоением: количество
	 *          выделений на файл обязано считаться десятками, а не тысячами. Рост
	 *          показателя означает, что накопитель заводится заново на всякую запись
	 *
	 */
	static constexpr double WRITE_ALLOCATIONS_THRESHOLD = 64.0;
	/**
	 * @brief Порог задержки записи файла настроек приложения в микросекундах
	 *
	 */
	static constexpr double WRITE_SERVICE_LATENCY_THRESHOLD = 100.0;

	/**
	 * @brief Функция записи текста настроек заданным построением
	 *
	 * @param layout построение, каким записывается текст настроек
	 * @param keys   количество записываемых пар отображения
	 * @param quoted признак записи значений, ограды требующих
	 * @return       размер собранного текста настроек
	 *
	 */
	static uint64_t write(const awh::codec::yaml::layout_t layout, const size_t keys, const bool quoted = false) noexcept {
		// Настройки записи текста настроек
		awh::codec::yaml::writer_t::settings_t settings;
		// Устанавливаем построение, каким записывается текст
		settings.layout = layout;
		// Объект записи текста настроек
		awh::codec::yaml::writer_t writer(settings);
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
			writer.key("key" + to_string(i));
			/**
			 * Если записываются значения, ограды требующие
			 */
			if(quoted)
				// Выполняем запись строкового значения, ограды требующего
				writer.value(string("значение: с двоеточием ") + to_string(i));
			/**
			 * Выполняем выбор вида значения очередной пары
			 */
			else switch(i % 4){
				// Если значением является последовательность знаков
				case 0: writer.value(string("значение") + to_string(i)); break;
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
		const size_t bytes = static_cast <size_t> (::write(layout, LARGE_KEYS, quoted));
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
			return ::write(layout, LARGE_KEYS, quoted);
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
		const size_t bytes = static_cast <size_t> (::write(awh::codec::yaml::layout_t::BLOCK, LARGE_KEYS));
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
			return ::write(awh::codec::yaml::layout_t::BLOCK, LARGE_KEYS);
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
		const size_t bytes = static_cast <size_t> (::write(awh::codec::yaml::layout_t::BLOCK, SERVICE_KEYS));
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
			return ::write(awh::codec::yaml::layout_t::BLOCK, SERVICE_KEYS);
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
