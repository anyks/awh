/**
 * @file: reader.cpp
 * @date: 2026-08-01
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения потокового чтения текста разметки — мелкие ответы служб,
 *        крупные выгрузки, преобладание атрибутов и содержимого и глубокая вложенность
 *
 * @copyright: Copyright © 2026
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
	 * @details Пороги откалиброваны по замеру с двукратным запасом: время на занятой
	 *          машине расходится между прогонами на десятки процентов, и порог,
	 *          заданный впритык, поднимал бы ложную тревогу чаще, чем ловил бы
	 *          настоящую регрессию. Двукратного запаса довольно, чтобы заметить
	 *          возвращение любой из уже случавшихся ошибок: квадратичное изъятие
	 *          разобранного, посимвольная дозапись содержимого, повторный проход по
	 *          метке ради места каждого атрибута - каждая из них роняла показатель в
	 *          три раза и более
	 *
	 */
	static constexpr double READ_SOAP_THRESHOLD = 160.0;
	/**
	 * @brief Порог пропускной способности чтения описания устройства
	 *
	 */
	static constexpr double READ_DEVICE_THRESHOLD = 200.0;
	/**
	 * @brief Порог пропускной способности чтения крупного документа
	 *
	 */
	static constexpr double READ_LARGE_THRESHOLD = 200.0;
	/**
	 * @brief Порог пропускной способности чтения документа с преобладанием атрибутов
	 *
	 */
	static constexpr double READ_ATTRIBUTES_THRESHOLD = 85.0;
	/**
	 * @brief Порог пропускной способности чтения документа с преобладанием содержимого
	 *
	 */
	static constexpr double READ_CONTENT_THRESHOLD = 350.0;
	/**
	 * @brief Порог пропускной способности чтения глубоко вложенного документа
	 *
	 */
	static constexpr double READ_NESTED_THRESHOLD = 75.0;
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
	static constexpr double READ_ALLOCATIONS_THRESHOLD = 32.0;

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
		// Устанавливаем измеренное значение
		result.value = perDocument(outcome);
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
};
