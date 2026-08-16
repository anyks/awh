/**
 * @file writer.cpp
 * @date 2026-08-16
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
 * @brief Сценарии измерения записи текста JSON — сборка ответа службы, запись чисел,
 *        отмена знаков, человекочитаемый вид и поток документов NDJSON
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
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
 * @brief Внутренние параметры и сценарии бенчмарков записи текста
 *
 */
namespace {
	/**
	 * @brief Количество собираемых документов
	 */
	static constexpr size_t WRITE_ROUNDS = 20000;
	/**
	 * @brief Количество записываемых чисел в одном документе
	 */
	static constexpr size_t BULK_NUMBERS = 2000;

	/**
	 * @brief Порог пропускной способности сборки ответа службы
	 *
	 * @details Пороги назначены по наименьшему из показателей отладочных стендов с
	 *          двукратным запасом на разброс между прогонами
	 *
	 */
	static constexpr double WRITE_RESPONSE_THRESHOLD = 25.0;
	/**
	 * @brief Порог пропускной способности записи чисел с плавающей запятой
	 *
	 * @details Сценарий этот стережёт устройство записи числа: кратчайшая запись
	 *          вычисляется разбором чисел сразу, без подбора точности пробами с обратным
	 *          чтением. Возврат к подбору уронил бы показатель более чем на порядок -
	 *          семнадцать записей и семнадцать разборов вместо одного прохода
	 *
	 */
	static constexpr double WRITE_NUMBERS_THRESHOLD = 15.0;
	/**
	 * @brief Порог пропускной способности записи строк, требующих отмены знаков
	 */
	static constexpr double WRITE_ESCAPED_THRESHOLD = 20.0;
	/**
	 * @brief Порог пропускной способности записи человекочитаемым видом
	 */
	static constexpr double WRITE_PRETTY_THRESHOLD = 20.0;
	/**
	 * @brief Порог расхода выделений памяти на сборку одного документа
	 *
	 * @details Собираемый текст удерживается одним хранилищем, память какого сброс
	 *          переживает: расход на документ обязан оставаться считанными единицами
	 *
	 */
	static constexpr double WRITE_ALLOCATIONS_THRESHOLD = 6.0;

	/**
	 * @brief Функция получения перечня записываемых строковых значений
	 *
	 * @note Значения подобраны требующими отмены знаков: кавычка, знак отмены, перевод
	 *       строки и управляющий знак - каждое из них ведёт запись по своему пути
	 *
	 * @return перечень записываемых строковых значений
	 *
	 */
	static const vector <string> & values() noexcept {
		// Собираемый перечень записываемых строковых значений
		static const vector <string> result = {
			"строка с \"кавычками\"", "путь\\к\\файлу", "первая строка\nвторая строка",
			"знак табуляции\tвнутри", "обычное значение без отмены", "управляющий\x01знак"
		};
		// Выводим перечень записываемых строковых значений
		return result;
	}
	/**
	 * @brief Функция сборки текста ответа службы
	 *
	 * @param writer объект записи текста документа
	 * @return       размер собранного текста документа
	 *
	 */
	static uint64_t compose(awh::codec::json::writer_t & writer) noexcept {
		// Выполняем сброс состояния записи текста документа
		writer.reset();
		// Выполняем открытие объекта документа
		writer.object();
		// Записываем имя поля состояния ответа
		writer.key("status");
		// Записываем состояние ответа
		writer.value(string("ok"));
		// Записываем имя поля кода ответа
		writer.key("code");
		// Записываем код ответа
		writer.value(static_cast <int64_t> (200));
		// Записываем имя поля времени обработки
		writer.key("elapsed");
		// Записываем время обработки
		writer.value(12.5);
		// Записываем имя поля описания потребителя
		writer.key("user");
		// Выполняем открытие объекта описания потребителя
		writer.object();
		// Записываем имя поля опознавателя потребителя
		writer.key("id");
		// Записываем опознаватель потребителя
		writer.value(static_cast <int64_t> (9007199254740993LL));
		// Записываем имя поля имени потребителя
		writer.key("name");
		// Записываем имя потребителя
		writer.value(string("Иван Петров"));
		// Записываем имя поля признака включения
		writer.key("active");
		// Записываем признак включения
		writer.value(true);
		// Выполняем закрытие объекта описания потребителя
		writer.close();
		// Записываем имя поля меток
		writer.key("tags");
		// Выполняем открытие массива меток
		writer.array();
		// Записываем первую метку
		writer.value(string("новый"));
		// Записываем вторую метку
		writer.value(string("срочный"));
		// Выполняем закрытие массива меток
		writer.close();
		// Записываем имя поля отказа
		writer.key("error");
		// Записываем пустое значение отказа
		writer.null();
		// Выполняем закрытие объекта документа
		writer.close();
		// Выводим размер собранного текста документа
		return static_cast <uint64_t> (writer.text().size());
	}
	/**
	 * @brief Функция сборки массива чисел с плавающей запятой
	 *
	 * @param writer объект записи текста документа
	 * @return       размер собранного текста документа
	 *
	 */
	static uint64_t digits(awh::codec::json::writer_t & writer) noexcept {
		// Выполняем сброс состояния записи текста документа
		writer.reset();
		// Выполняем открытие массива чисел
		writer.array();
		/**
		 * Выполняем запись всех чисел массива
		 */
		for(size_t i = 0; i < BULK_NUMBERS; i++)
			// Записываем очередное число массива
			writer.value(static_cast <double> (i) + 0.125);
		// Выполняем закрытие массива чисел
		writer.close();
		// Выводим размер собранного текста документа
		return static_cast <uint64_t> (writer.text().size());
	}
	/**
	 * @brief Функция сборки массива строк, требующих отмены знаков
	 *
	 * @param writer объект записи текста документа
	 * @return       размер собранного текста документа
	 *
	 */
	static uint64_t escaped(awh::codec::json::writer_t & writer) noexcept {
		// Получаем перечень записываемых строковых значений
		const vector <string> & values = ::values();
		// Выполняем сброс состояния записи текста документа
		writer.reset();
		// Выполняем открытие массива строк
		writer.array();
		/**
		 * Выполняем запись всех строк массива
		 */
		for(size_t i = 0; i < BULK_NUMBERS; i++)
			// Записываем очередную строку массива
			writer.value(values.at(i % values.size()));
		// Выполняем закрытие массива строк
		writer.close();
		// Выводим размер собранного текста документа
		return static_cast <uint64_t> (writer.text().size());
	}
	/**
	 * @brief Функция прогона сценария сборки ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeResponse() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект записи текста документа
		awh::codec::json::writer_t writer;
		// Получаем размер собираемого текста документа
		const size_t bytes = static_cast <size_t> (::compose(writer));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, WRITE_ROUNDS, [&writer]() noexcept {
			// Выполняем сборку текста ответа службы
			return ::compose(writer);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи чисел с плавающей запятой
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeNumbers() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект записи текста документа
		awh::codec::json::writer_t writer;
		// Получаем размер собираемого текста документа
		const size_t bytes = static_cast <size_t> (::digits(writer));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, (WRITE_ROUNDS / 100), [&writer]() noexcept {
			// Выполняем сборку массива чисел
			return ::digits(writer);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи строк, требующих отмены знаков
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeEscaped() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект записи текста документа
		awh::codec::json::writer_t writer;
		// Получаем размер собираемого текста документа
		const size_t bytes = static_cast <size_t> (::escaped(writer));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, (WRITE_ROUNDS / 100), [&writer]() noexcept {
			// Выполняем сборку массива строк, требующих отмены знаков
			return ::escaped(writer);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи человекочитаемым видом
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writePretty() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект записи текста документа
		awh::codec::json::writer_t writer;
		// Получаем настройки записи текста документа
		awh::codec::json::writer_t::settings_t settings = writer.settings();
		// Устанавливаем человекочитаемый вид записи
		settings.format = awh::codec::json::format_t::PRETTY;
		// Выполняем установку настроек записи текста документа
		writer.settings(settings);
		// Получаем размер собираемого текста документа
		const size_t bytes = static_cast <size_t> (::compose(writer));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, WRITE_ROUNDS, [&writer]() noexcept {
			// Выполняем сборку текста ответа службы
			return ::compose(writer);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на запись
	 *
	 * @details Замер ведётся вместе с изъятием собранного текста: так и поступает служба -
	 *          собирает ответ и отдаёт его наружу. Без изъятия расход выходит нулевым, ибо
	 *          сброс записи память свою удерживает, и мерить было бы нечего
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект записи текста документа
		awh::codec::json::writer_t writer;
		// Получаем размер собираемого текста документа
		const size_t bytes = static_cast <size_t> (::compose(writer));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, WRITE_ROUNDS, [&writer]() noexcept {
			// Выполняем сборку текста ответа службы
			::compose(writer);
			// Выполняем изъятие собранного текста документа
			return static_cast <uint64_t> (writer.take().size());
		});
		/**
		 * Если учёт выделений памяти неработоспособен
		 */
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренный расход выделений памяти
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки ответа службы
	 */
	static const bool RESPONSE_REGISTERED = awh::benchmark::add(
		"codec/json: запись ответа службы", "МБ/с", WRITE_RESPONSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeResponse
	);
	/**
	 * Выполняем регистрацию сценария записи чисел с плавающей запятой
	 */
	static const bool NUMBERS_REGISTERED = awh::benchmark::add(
		"codec/json: запись чисел", "МБ/с", WRITE_NUMBERS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeNumbers
	);
	/**
	 * Выполняем регистрацию сценария записи строк, требующих отмены знаков
	 */
	static const bool ESCAPED_REGISTERED = awh::benchmark::add(
		"codec/json: запись отменяемых знаков", "МБ/с", WRITE_ESCAPED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeEscaped
	);
	/**
	 * Выполняем регистрацию сценария записи человекочитаемым видом
	 */
	static const bool PRETTY_REGISTERED = awh::benchmark::add(
		"codec/json: запись человекочитаемым видом", "МБ/с", WRITE_PRETTY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writePretty
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на запись
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/json: выделения на запись", "выд./док.", WRITE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, writeAllocations
	);
};
