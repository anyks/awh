/**
 * @file value.cpp
 * @date 2026-08-20
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
 * @brief Бенчмарки владеющего значения TOML — снятие поддерева с дерева настроек,
 *        потоковая сборка значения, запись его в текст и расход памяти на всё это
 *
 * @details Владеющее значение устроено **россыпью**: у всякого узла своя строка
 *          содержимого, свой перечень имён и свой перечень детей. Дерево же настроек
 *          хранит узлы сплошным перечнем, а знаки - одним хранилищем, и обходится
 *          немногими выделениями памяти на документ. Разница эта и есть цена владения,
 *          и сценарии ниже заведены ради того, чтобы цена эта была известна числом, а
 *          не выяснялась потребителем в работе
 *
 * @note Записью снятое значение исходному тексту не равно: оформление - расстановка
 *       строк, примечания и запись имён - остаётся у дерева, а владеющее значение его
 *       не удерживает вовсе. Оттого скорость записи здесь с дословной перезаписью
 *       дерева сличать нельзя: работы это разные
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
 * @brief Внутренние параметры и сценарии бенчмарков владеющего значения
 *
 */
namespace {
	/**
	 * @brief Количество обрабатываемых мелких файлов настроек
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 8000;
	/**
	 * @brief Количество обрабатываемых крупных файлов настроек
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 16;

	/**
	 * @brief Порог пропускной способности снятия значения с дерева настроек
	 *
	 * @details Пороги назначены по САМОМУ МЕДЛЕННОМУ из отладочных стендов с двукратным
	 *          запасом на разброс между прогонами. Порог, снятый на рабочей машине,
	 *          отказывал бы там без всякой регрессии
	 *
	 * @note Замер снимается отдельным стендом `benchmark/codec/toml/stand.sh`: он
	 *       собирает набор без библиотеки целиком и оттого гоняется на всякой машине
	 *
	 */
	static constexpr double TAKE_LARGE_THRESHOLD = 5.0;
	/**
	 * @brief Порог пропускной способности снятия значения с дерева настроек службы
	 *
	 * @note Порог этот выше порога крупного файла, а не равен ему: мелкий документ
	 *       снимается быстрее крупного, и общий порог на двоих сторожил бы лишь того,
	 *       кто медленнее
	 *
	 */
	static constexpr double TAKE_SERVICE_THRESHOLD = 10.0;
	/**
	 * @brief Порог пропускной способности потоковой сборки значения
	 *
	 */
	static constexpr double BUILD_SERVICE_THRESHOLD = 30.0;
	/**
	 * @brief Порог пропускной способности записи значения в текст
	 *
	 */
	static constexpr double DUMP_LARGE_THRESHOLD = 8.0;
	/**
	 * @brief Порог пропускной способности обхода владеющего значения
	 *
	 */
	static constexpr double WALK_LARGE_THRESHOLD = 100.0;
	/**
	 * @brief Порог расхода выделений памяти на снятие одного значения
	 *
	 * @details Расход этот **на порядки выше** расхода дерева, и так и должно быть:
	 *          дерево хранит узлы сплошным перечнем, а владеющее значение - россыпью,
	 *          где всякий узел заводит свою память. Порог сторожит не малость расхода,
	 *          а устройство: рост его сверх числа узлов означал бы, что память заводится
	 *          не по узлу, а по нескольку раз на узел
	 *
	 * @note Порог задан на файл настроек службы и берётся вдвое выше большего из
	 *       измеренных по обеим стандартным библиотекам: расхождение между ними идёт от
	 *       короткого запаса строки, а не от кодека
	 *
	 */
	static constexpr double TAKE_ALLOCATIONS_THRESHOLD = 256.0;

	/**
	 * @brief Функция обхода владеющего значения
	 *
	 * @param value обходимое значение
	 * @return      количество обойдённых значений
	 *
	 */
	static uint64_t walk(const awh::codec::toml::value_t & value) noexcept {
		// Количество обойдённых значений
		uint64_t result = 1;
		/**
		 * Выполняем перебор всех значений вместилища
		 */
		for(size_t i = 0; i < value.size(); i++)
			// Выполняем обход очередного значения вместилища
			result += ::walk(value[i]);
		// Выводим количество обойдённых значений
		return result;
	}
	/**
	 * @brief Функция прогона сценария снятия значения с дерева настроек
	 *
	 * @details Дерево собирается однажды до замера: снятие мерится отдельно от разбора,
	 *          ибо сложенные вместе они скрывают, какая из двух работ подорожала
	 *
	 * @param text   разбираемый текст настроек
	 * @param rounds количество снимаемых значений
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t taking(const string & text, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект дерева настроек
		awh::codec::toml::document_t doc;
		/**
		 * Если сборка дерева настроек завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонных настроек завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), rounds, [&doc]() noexcept {
			// Выполняем снятие дерева настроек собственной памятью
			const awh::codec::toml::value_t value(doc);
			// Выводим количество снятых значений верхнего уровня
			return static_cast <uint64_t> (value.size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария снятия значения с дерева крупного файла настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t takeLarge() noexcept {
		// Выводим результат измерения
		return ::taking(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария снятия значения с дерева настроек службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t takeService() noexcept {
		// Выводим результат измерения
		return ::taking(service(), SMALL_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария потоковой сборки значения
	 *
	 * @details Собирается кусок настроек - тот самый случай, ради которого сборка и
	 *          заведена: потребитель строит исходящие настройки парой за парой, дерева
	 *          не имея вовсе
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонный текст настроек
		const string & text = service();
		// Объект потоковой сборки значения
		awh::codec::toml::builder_t builder;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&builder]() noexcept {
			// Выполняем открытие таблицы
			builder.table();
			// Выполняем запись имени пары
			builder.key("title");
			// Выполняем запись строкового значения
			builder.value("служба обмена сообщениями");
			// Выполняем запись имени пары
			builder.key("port");
			// Выполняем запись целого числа
			builder.value(static_cast <int64_t> (8080));
			// Выполняем запись имени пары
			builder.key("hosts");
			// Выполняем открытие перечня
			builder.array();
			/**
			 * Выполняем запись значений перечня
			 */
			for(int64_t i = 0; i < 4; i++)
				// Выполняем запись очередного значения перечня
				builder.value(i);
			// Выполняем закрытие перечня
			builder.close();
			// Выполняем запись имени пары
			builder.key("logging");
			// Выполняем открытие вложенной таблицы
			builder.table();
			// Выполняем запись имени пары
			builder.key("level");
			// Выполняем запись строкового значения
			builder.value("debug");
			// Выполняем закрытие вложенной таблицы
			builder.close();
			// Выполняем закрытие таблицы
			builder.close();
			// Изымаем собранное значение
			const awh::codec::toml::value_t value = builder.finish();
			// Выводим количество пар собранного значения
			return static_cast <uint64_t> (value.size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи значения в текст
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t dumpLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонный текст настроек
		const string & text = large();
		// Объект дерева настроек
		awh::codec::toml::document_t doc;
		/**
		 * Если сборка дерева настроек завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонных настроек завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		/**
		 * Снимаемое владеющее значение
		 *
		 * @note Снятие ведётся до замера: мерится одна лишь запись, ибо сложенные вместе
		 *       снятие и запись скрывают, какая из двух работ подорожала
		 */
		const awh::codec::toml::value_t value(doc);
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&value]() noexcept {
			// Выполняем запись владеющего значения в текст настроек
			const string result = value.dump();
			// Выводим длину записанного текста настроек
			return static_cast <uint64_t> (result.size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария обхода владеющего значения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t walkLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонный текст настроек
		const string & text = large();
		// Объект дерева настроек
		awh::codec::toml::document_t doc;
		/**
		 * Если сборка дерева настроек завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонных настроек завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Снимаемое владеющее значение
		const awh::codec::toml::value_t value(doc);
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&value]() noexcept {
			// Выводим количество обойдённых значений
			return ::walk(value);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на снятие значения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t takeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонный текст настроек
		const string & text = service();
		// Объект дерева настроек
		awh::codec::toml::document_t doc;
		/**
		 * Если сборка дерева настроек завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонных настроек завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&doc]() noexcept {
			// Выполняем снятие дерева настроек собственной памятью
			const awh::codec::toml::value_t value(doc);
			// Выводим количество снятых значений верхнего уровня
			return static_cast <uint64_t> (value.size());
		});
		/**
		 * Если учёт выделений памяти не работает
		 */
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное значение
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария снятия значения с дерева крупного файла настроек
	 */
	static const bool TAKE_LARGE_REGISTERED = awh::benchmark::add(
		"codec/toml: снятие значения с дерева", "МБ/с", TAKE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, takeLarge
	);
	/**
	 * Выполняем регистрацию сценария снятия значения с дерева настроек службы
	 */
	static const bool TAKE_SERVICE_REGISTERED = awh::benchmark::add(
		"codec/toml: снятие значения настроек службы", "МБ/с", TAKE_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, takeService
	);
	/**
	 * Выполняем регистрацию сценария потоковой сборки значения
	 */
	static const bool BUILD_REGISTERED = awh::benchmark::add(
		"codec/toml: потоковая сборка значения", "МБ/с", BUILD_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildService
	);
	/**
	 * Выполняем регистрацию сценария записи значения в текст
	 */
	static const bool DUMP_REGISTERED = awh::benchmark::add(
		"codec/toml: запись значения в текст", "МБ/с", DUMP_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, dumpLarge
	);
	/**
	 * Выполняем регистрацию сценария обхода владеющего значения
	 */
	static const bool WALK_REGISTERED = awh::benchmark::add(
		"codec/toml: обход владеющего значения", "МБ/с", WALK_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, walkLarge
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на снятие значения
	 */
	static const bool TAKE_ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/toml: выделения на снятие значения", "выд./док.", TAKE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, takeAllocations
	);
};
