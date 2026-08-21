/**
 * @file value.cpp
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
 * @brief Бенчмарки владеющего значения YAML — снятие поддерева с дерева документа,
 *        потоковая сборка значения, запись его в текст и расход памяти на всё это
 *
 * @details Владеющее значение устроено **россыпью**: у всякого узла своя строка
 *          содержимого, свой перечень имён и свой перечень детей. Дерево же документа
 *          хранит узлы сплошным перечнем, а знаки - одним хранилищем, и обходится
 *          немногими выделениями памяти на документ. Разница эта и есть цена владения,
 *          и сценарии ниже заведены ради того, чтобы цена эта была известна числом, а
 *          не выяснялась потребителем в работе
 *
 * @note У кодека YAML цена эта выше, чем у соседних: дерево его держит ещё и исходный
 *       текст с оформлением - отступы, вид записи, примечания, - а снятие всё это
 *       отбрасывает. Оттого снятое значение записью своей исходному тексту не равно, и
 *       сличать скорость записи с дословной перезаписью дерева нельзя вовсе: работы это
 *       разные
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
	static constexpr size_t LARGE_ROUNDS = 4;

	/**
	 * @brief Количество устанавливаемых значений при сборке вызовами
	 *
	 */
	static constexpr size_t ASSEMBLY_COUNT = 20000;

	/**
	 * @brief Порог скорости сборки значения вызовами
	 *
	 * @details Показатель этот стережёт устройство отображения: поиск поля попарным
	 *          сличением имён обращает сборку в квадратичную
	 *
	 * @warning ПОРОГ ЭТОТ ВРЕМЕННЫЙ, снят по одной рабочей машине: разметку брать
	 *          раскладкой по стендам, как у прочих сценариев набора
	 *
	 * @warning Сборка эта КВАДРАТИЧНА ныне, и порог поставлен по измеренному, а не по
	 *          должному. Свойство общее у владеющих значений ВСЕХ пяти кодеков, решение
	 *          по устройству за владельцем. Порог привязан к ASSEMBLY_COUNT: изменив
	 *          его, пороговое число надлежит переснять
	 *
	 */
	static constexpr double ASSEMBLY_THRESHOLD = 25000.0;
	/**
	 * @brief Порог скорости поиска значения по имени
	 *
	 * @details Сценарий этот ПАРНЫЙ сборке вызовами и заведён потому, что та стережёт
	 *          лишь половину беды: перебор имён стоит одинаково и при заведении поля, и
	 *          при обращении к нему. Правка, ускорившая построение и замедлившая поиск,
	 *          прошла бы мимо порога, будь сценарий один
	 *
	 * @warning ПОРОГ ЭТОТ ВРЕМЕННЫЙ, снят по одной рабочей машине
	 *
	 */
	static constexpr double LOOKUP_THRESHOLD = 25000.0;

	/**
	 * @brief Порог пропускной способности снятия значения с дерева документа
	 *
	 * @details Пороги назначены по САМОМУ МЕДЛЕННОМУ из отладочных стендов - OpenBSD на
	 *          ARM64 - с двукратным запасом на разброс между прогонами. Порог, снятый на
	 *          рабочей машине, отказывал бы там без всякой регрессии: она обгоняет
	 *          OpenBSD в двадцать раз на этих сценариях
	 *
	 * @note Замер снимается отдельным стендом `benchmark/codec/yaml/stand.sh`: он
	 *       собирает набор без библиотеки целиком и оттого гоняется на всякой машине.
	 *       Показатели прогона 19.08.2026 сведены в описании набора
	 *
	 */
	static constexpr double TAKE_LARGE_THRESHOLD = 7.25;
	/**
	 * @brief Порог пропускной способности снятия значения с дерева настроек службы
	 *
	 * @note Порог этот выше порога крупного файла, а не равен ему: мелкий документ
	 *       снимается вдвое быстрее крупного, и общий порог на двоих сторожил бы лишь
	 *       того, кто медленнее
	 *
	 */
	static constexpr double TAKE_SERVICE_THRESHOLD = 16.93;
	/**
	 * @brief Порог пропускной способности потоковой сборки значения
	 *
	 */
	static constexpr double BUILD_SERVICE_THRESHOLD = 54.73;
	/**
	 * @brief Порог пропускной способности записи значения в текст
	 *
	 */
	static constexpr double DUMP_LARGE_THRESHOLD = 10.59;
	/**
	 * @brief Порог пропускной способности обхода владеющего значения
	 *
	 */
	static constexpr double WALK_LARGE_THRESHOLD = 212.91;
	/**
	 * @brief Порог расхода выделений памяти на снятие одного значения
	 *
	 * @details Расход этот **на порядки выше** расхода дерева, и так и должно быть:
	 *          дерево хранит узлы сплошным перечнем, а владеющее значение - россыпью,
	 *          где всякий узел заводит свою память. Порог сторожит не малость расхода,
	 *          а устройство: рост его сверх числа узлов означал бы, что память заводится
	 *          не по узлу, а по нескольку раз на узел
	 *
	 * @note Порог задан на файл настроек службы: узлов в нём около полусотни, и расход
	 *       обязан остаться того же порядка. Замерено 59 выделений у libc++ и 62 у
	 *       libstdc++ - расхождение это от короткого запаса строки, и порог берётся
	 *       вдвое выше большего из них
	 *
	 */
	static constexpr double TAKE_ALLOCATIONS_THRESHOLD = 124.0;

	/**
	 * @brief Функция обхода владеющего значения
	 *
	 * @param value обходимое значение
	 * @return      количество обойдённых значений
	 *
	 */
	static uint64_t walk(const awh::codec::yaml::value_t & value) noexcept {
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
	 * @brief Функция прогона сценария снятия значения с дерева документа
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
		// Объект дерева документа
		awh::codec::yaml::document_t doc;
		/**
		 * Если сборка дерева документа завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонного документа завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), rounds, [&doc]() noexcept {
			// Выполняем снятие дерева документа собственной памятью
			const awh::codec::yaml::value_t value(doc.root());
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
	 *          заведена: потребитель строит исходящий документ полем за полем, дерева
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
		awh::codec::yaml::builder_t builder;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&builder]() noexcept {
			// Выполняем открытие отображения
			builder.mapping();
			// Выполняем запись имени пары
			builder.key("title");
			// Выполняем запись строкового значения
			builder.value("служба обмена сообщениями");
			// Выполняем запись имени пары
			builder.key("port");
			// Выполняем запись целого числа без знака
			builder.value(static_cast <uint64_t> (8080));
			// Выполняем запись имени пары
			builder.key("hosts");
			// Выполняем открытие перечня
			builder.sequence();
			/**
			 * Выполняем запись записей перечня
			 */
			for(uint64_t i = 0; i < 4; i++)
				// Выполняем запись очередной записи перечня
				builder.value(i);
			// Выполняем закрытие перечня
			builder.close();
			// Выполняем запись имени пары
			builder.key("logging");
			// Выполняем открытие отображения
			builder.mapping();
			// Выполняем запись имени пары
			builder.key("level");
			// Выполняем запись строкового значения
			builder.value("debug");
			// Выполняем закрытие отображения
			builder.close();
			// Выполняем закрытие отображения
			builder.close();
			// Изымаем собранное значение
			const awh::codec::yaml::value_t value = builder.finish();
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
	 * @note Работа эта дословной перезаписи дерева не равна вовсе: снятое значение
	 *       оформления исходного текста не держит, и записывается оно заново
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t dumpLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонный текст настроек
		const string & text = large();
		// Объект дерева документа
		awh::codec::yaml::document_t doc;
		/**
		 * Если сборка дерева документа завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонного документа завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем снятие дерева документа собственной памятью
		const awh::codec::yaml::value_t value(doc.root());
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&value]() noexcept {
			// Выполняем запись значения в текст
			return static_cast <uint64_t> (value.dump().size());
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
	 * @details Сценарий этот сличается с обходом дерева напрямую: дерево обходится
	 *          сплошным перечнем узлов, а значение - россыпью вместилищ, и разница
	 *          между ними есть цена владения при чтении
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t walkLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонный текст настроек
		const string & text = large();
		// Объект дерева документа
		awh::codec::yaml::document_t doc;
		/**
		 * Если сборка дерева документа завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонного документа завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем снятие дерева документа собственной памятью
		const awh::codec::yaml::value_t value(doc.root());
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&value]() noexcept {
			// Выполняем обход снятого значения
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
		// Объект дерева документа
		awh::codec::yaml::document_t doc;
		/**
		 * Если сборка дерева документа завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонного документа завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&doc]() noexcept {
			// Выполняем снятие дерева документа собственной памятью
			const awh::codec::yaml::value_t value(doc.root());
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
	 * @brief Функция прогона сценария сборки значения вызовами
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t assembly() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(0, 1, []() noexcept {
			// Собираемое владеющее значение
			awh::codec::yaml::value_t value;
			// Количество выполненных установок
			uint64_t count = 0;
			/**
			 * Выполняем установку всех значений отображения
			 */
			for(size_t i = 0; i < ASSEMBLY_COUNT; i++){
				// Выполняем установку очередного значения отображения
				value["поле" + to_string(i)] = awh::codec::yaml::value_t(static_cast <int64_t> (i));
				// Выполняем учёт выполненной установки
				count++;
			}
			// Выводим количество выполненных установок
			return count;
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "сборка не выполнила ни одной установки";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренную скорость выполнения установок
		result.value = ((outcome.seconds > 0.0) ? (static_cast <double> (outcome.operations * ASSEMBLY_COUNT) / outcome.seconds) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария поиска значения по имени
	 *
	 * @details Отображение собирается до замера, чтобы сборка и поиск не скрывали друг
	 * друга: обе они упираются в один и тот же перебор имён
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t lookup() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Обыскиваемое владеющее значение
		awh::codec::yaml::value_t value;
		/**
		 * Выполняем сборку обыскиваемого отображения
		 */
		for(size_t i = 0; i < ASSEMBLY_COUNT; i++)
			// Выполняем установку очередного значения отображения
			value["поле" + to_string(i)] = awh::codec::yaml::value_t(static_cast <int64_t> (i));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(0, 1, [&value]() noexcept {
			// Количество разысканных значений
			uint64_t count = 0;
			/**
			 * Выполняем перебор всех полей отображения
			 */
			for(size_t i = 0; i < ASSEMBLY_COUNT; i++){
				// Если очередное поле отображения разыскано
				if(value["поле" + to_string(i)].valid())
					// Выполняем учёт разысканного значения
					count++;
			}
			// Выводим количество разысканных значений
			return count;
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "поиск не выполнил ни одного обращения";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренную скорость выполнения обращений
		result.value = ((outcome.seconds > 0.0) ? (static_cast <double> (outcome.operations * ASSEMBLY_COUNT) / outcome.seconds) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * Выполняем регистрацию сценария снятия значения с дерева крупного файла настроек
	 */
	static const bool TAKE_LARGE_REGISTERED = awh::benchmark::add(
		"codec/yaml: снятие значения с дерева", "МБ/с", TAKE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, takeLarge
	);
	/**
	 * Выполняем регистрацию сценария снятия значения с дерева настроек службы
	 */
	static const bool TAKE_SERVICE_REGISTERED = awh::benchmark::add(
		"codec/yaml: снятие значения настроек службы", "МБ/с", TAKE_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, takeService
	);
	/**
	 * Выполняем регистрацию сценария потоковой сборки значения
	 */
	static const bool BUILD_REGISTERED = awh::benchmark::add(
		"codec/yaml: потоковая сборка значения", "МБ/с", BUILD_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildService
	);
	/**
	 * Выполняем регистрацию сценария записи значения в текст
	 */
	static const bool DUMP_REGISTERED = awh::benchmark::add(
		"codec/yaml: запись значения в текст", "МБ/с", DUMP_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, dumpLarge
	);
	/**
	 * Выполняем регистрацию сценария обхода владеющего значения
	 */
	static const bool WALK_REGISTERED = awh::benchmark::add(
		"codec/yaml: обход владеющего значения", "МБ/с", WALK_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, walkLarge
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на снятие значения
	 */
	static const bool TAKE_ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/yaml: выделения на снятие значения", "выд./док.", TAKE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, takeAllocations
	);
	/**
	 * Выполняем регистрацию сценария сборки значения вызовами
	 */
	static const bool ASSEMBLY_REGISTERED = awh::benchmark::add(
		"codec/yaml: сборка значения вызовами", "уст./с", ASSEMBLY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, assembly
	);
	/**
	 * Выполняем регистрацию сценария поиска значения по имени
	 */
	static const bool LOOKUP_REGISTERED = awh::benchmark::add(
		"codec/yaml: поиск значения по имени", "обр./с", LOOKUP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, lookup
	);
};
