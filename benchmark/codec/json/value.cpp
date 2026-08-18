/**
 * @file value.cpp
 * @date 2026-08-18
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
 * @brief Бенчмарки владеющего значения JSON — снятие поддерева с дерева документа,
 *        потоковая сборка значения, запись его в текст и расход памяти на всё это
 *
 * @details Владеющее значение устроено **россыпью**: у всякого узла своя строка
 *          содержимого, свой перечень имён и свой перечень детей. Дерево же документа
 *          хранит узлы сплошным перечнем, а знаки - одним хранилищем, и обходится
 *          одним выделением памяти на документ. Разница эта и есть цена владения, и
 *          сценарии ниже заведены ради того, чтобы цена эта была известна числом, а
 *          не выяснялась потребителем в работе
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include "json.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера JSON
 */
using namespace awh::benchmark::notation;

/**
 * @brief Внутренние параметры и сценарии бенчмарков владеющего значения
 *
 */
namespace {
	/**
	 * @brief Количество обрабатываемых мелких документов
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество обрабатываемых крупных документов
	 */
	static constexpr size_t LARGE_ROUNDS = 8;

	/**
	 * @brief Порог пропускной способности снятия значения с дерева документа
	 *
	 * @details Пороги назначены по наименьшему из показателей отладочных стендов с
	 *          двукратным запасом на разброс между прогонами
	 *
	 */
	static constexpr double TAKE_LARGE_THRESHOLD = 8.0;
	/**
	 * @brief Порог пропускной способности потоковой сборки значения
	 */
	static constexpr double BUILD_RESPONSE_THRESHOLD = 4.0;
	/**
	 * @brief Порог пропускной способности записи значения в текст
	 */
	static constexpr double DUMP_LARGE_THRESHOLD = 20.0;
	/**
	 * @brief Порог пропускной способности обхода владеющего значения
	 */
	static constexpr double WALK_LARGE_THRESHOLD = 40.0;
	/**
	 * @brief Порог расхода выделений памяти на снятие одного значения
	 *
	 * @details Расход этот **на порядки выше** расхода дерева, и так и должно быть:
	 *          дерево хранит узлы сплошным перечнем, а владеющее значение - россыпью,
	 *          где всякий узел заводит свою память. Порог сторожит не малость расхода,
	 *          а устройство: рост его сверх числа узлов означал бы, что память заводится
	 *          не по узлу, а по нескольку раз на узел
	 *
	 * @note Порог задан на документ ответа службы: в нём двенадцать узлов, и расход
	 *       обязан остаться того же порядка
	 *
	 */
	static constexpr double TAKE_ALLOCATIONS_THRESHOLD = 60.0;

	/**
	 * @brief Функция обхода владеющего значения
	 *
	 * @param value обходимое значение
	 * @return      количество обойдённых значений
	 *
	 */
	static uint64_t walk(const awh::codec::json::value_t & value) noexcept {
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
	 * @param text   разбираемый текст документа
	 * @param rounds количество снимаемых значений
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t taking(const string & text, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект дерева документа
		awh::codec::json::document_t doc;
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
			const awh::codec::json::value_t value(doc.root());
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
	 * @brief Функция прогона сценария снятия значения с дерева крупного документа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t takeLarge() noexcept {
		// Выводим результат измерения
		return ::taking(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария снятия значения с дерева ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t takeResponse() noexcept {
		// Выводим результат измерения
		return ::taking(response(), SMALL_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария потоковой сборки значения
	 *
	 * @details Собирается ответ службы - тот самый случай, ради которого сборка и
	 *          заведена: потребитель строит исходящий документ полем за полем, дерева
	 *          не имея вовсе
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildResponse() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем эталонный текст документа
		const string & text = response();
		// Объект потоковой сборки значения
		awh::codec::json::builder_t builder;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&builder]() noexcept {
			// Выполняем открытие объекта
			builder.object();
			// Выполняем запись имени поля объекта
			builder.key("status");
			// Выполняем запись строкового значения
			builder.value("ok");
			// Выполняем запись имени поля объекта
			builder.key("code");
			// Выполняем запись целого числа без знака
			builder.value(static_cast <uint64_t> (200));
			// Выполняем запись имени поля объекта
			builder.key("items");
			// Выполняем открытие массива
			builder.array();
			/**
			 * Выполняем запись значений массива
			 */
			for(uint64_t i = 0; i < 4; i++)
				// Выполняем запись очередного значения массива
				builder.value(i);
			// Выполняем закрытие массива
			builder.close();
			// Выполняем запись имени поля объекта
			builder.key("nested");
			// Выполняем открытие объекта
			builder.object();
			// Выполняем запись имени поля объекта
			builder.key("name");
			// Выполняем запись строкового значения
			builder.value("сервер");
			// Выполняем закрытие объекта
			builder.close();
			// Выполняем закрытие объекта
			builder.close();
			// Изымаем собранное значение
			const awh::codec::json::value_t value = builder.finish();
			// Выводим количество полей собранного значения
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
		// Получаем эталонный текст документа
		const string & text = large();
		// Объект дерева документа
		awh::codec::json::document_t doc;
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
		const awh::codec::json::value_t value(doc.root());
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
		// Получаем эталонный текст документа
		const string & text = large();
		// Объект дерева документа
		awh::codec::json::document_t doc;
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
		const awh::codec::json::value_t value(doc.root());
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
		// Получаем эталонный текст документа
		const string & text = response();
		// Объект дерева документа
		awh::codec::json::document_t doc;
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
			const awh::codec::json::value_t value(doc.root());
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
	 * Выполняем регистрацию сценария снятия значения с дерева крупного документа
	 */
	static const bool TAKE_LARGE_REGISTERED = awh::benchmark::add(
		"codec/json: снятие значения с дерева", "МБ/с", TAKE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, takeLarge
	);
	/**
	 * Выполняем регистрацию сценария снятия значения с дерева ответа службы
	 */
	static const bool TAKE_RESPONSE_REGISTERED = awh::benchmark::add(
		"codec/json: снятие значения ответа службы", "МБ/с", BUILD_RESPONSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, takeResponse
	);
	/**
	 * Выполняем регистрацию сценария потоковой сборки значения
	 */
	static const bool BUILD_REGISTERED = awh::benchmark::add(
		"codec/json: потоковая сборка значения", "МБ/с", BUILD_RESPONSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildResponse
	);
	/**
	 * Выполняем регистрацию сценария записи значения в текст
	 */
	static const bool DUMP_REGISTERED = awh::benchmark::add(
		"codec/json: запись значения в текст", "МБ/с", DUMP_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, dumpLarge
	);
	/**
	 * Выполняем регистрацию сценария обхода владеющего значения
	 */
	static const bool WALK_REGISTERED = awh::benchmark::add(
		"codec/json: обход владеющего значения", "МБ/с", WALK_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, walkLarge
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на снятие значения
	 */
	static const bool TAKE_ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/json: выделения на снятие значения", "выд./док.", TAKE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, takeAllocations
	);
};
