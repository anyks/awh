/**
 * @file: document.cpp
 * @date: 2026-08-01
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения дерева разметки и записи текста разметки — сборка дерева
 *        из событий чтения, обход собранного дерева и запись разметки в двух видах
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
 * @brief Внутренние параметры и сценарии бенчмарков дерева и записи разметки
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
	 * @brief Порог пропускной способности сборки дерева ответа по договору SOAP
	 *
	 * @details Пороги откалиброваны по замеру с двукратным запасом: время на занятой
	 *          машине расходится между прогонами на десятки процентов
	 *
	 */
	static constexpr double TREE_SOAP_THRESHOLD = 130.0;
	/**
	 * @brief Порог пропускной способности сборки дерева крупного документа
	 *
	 */
	static constexpr double TREE_LARGE_THRESHOLD = 150.0;
	/**
	 * @brief Порог скорости обхода собранного дерева в узлах в секунду
	 *
	 * @details Обход ведётся по местам узлов в хранилище, а не по указателям, и
	 *          показатель ловит превращение обхода в поиск: узлы связаны местами
	 *          соседей, и переход к следующему узлу обязан быть однократным обращением
	 *
	 */
	static constexpr double TREE_WALK_THRESHOLD = 120000000.0;
	/**
	 * @brief Порог пропускной способности записи текста разметки
	 *
	 */
	static constexpr double WRITE_THRESHOLD = 90.0;

	/**
	 * @brief Функция сборки дерева разметки
	 *
	 * @param text разбираемый текст разметки
	 * @return     количество собранных узлов дерева
	 *
	 */
	static uint64_t build(const string & text) noexcept {
		// Объект дерева разметки
		awh::codec::xml::document_t document;
		/**
		 * Если разбор текста разметки выполнить не удалось
		 */
		if(!document.parse(text))
			// Выводим нулевое количество собранных узлов
			return 0;
		// Выводим количество собранных узлов дерева
		return static_cast <uint64_t> (document.size());
	}
	/**
	 * @brief Функция обхода собранного дерева разметки
	 *
	 * @param node узел, с которого начинается обход
	 * @return     количество пройденных узлов дерева
	 *
	 */
	static uint64_t walk(const awh::codec::xml::node_t & node) noexcept {
		// Количество пройденных узлов дерева
		uint64_t result = 0;
		/**
		 * Выполняем перебор всех вложенных узлов дерева
		 */
		for(awh::codec::xml::node_t child = node.first(); child.valid(); child = child.next())
			// Выполняем подсчёт пройденных узлов дерева
			result += (1 + walk(child));
		// Выводим количество пройденных узлов дерева
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки дерева ответа по договору SOAP
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeSoap() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = soap();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text]() noexcept {
			// Выполняем сборку дерева разметки
			return ::build(text);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки дерева крупного документа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст разметки
		const string & text = large();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&text]() noexcept {
			// Выполняем сборку дерева разметки
			return ::build(text);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария обхода собранного дерева
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeWalk() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект дерева разметки
		static awh::codec::xml::document_t document;
		/**
		 * Если разбор текста разметки выполнить не удалось
		 */
		if(!document.parse(large())){
			// Запоминаем, что измерение не выполнялось
			result.skipped = true;
			// Запоминаем причину, по которой измерение не выполнялось
			result.reason = "эталонный документ разобрать не удалось";
			// Выводим результат измерения
			return result;
		}
		// Получаем корневой узел собранного дерева
		const awh::codec::xml::node_t root = document.element();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(0, 4, [&root]() noexcept {
			// Выполняем обход собранного дерева разметки
			return ::walk(root);
		});
		/**
		 * Если замер не состоялся
		 */
		if(outcome.seconds <= 0.0){
			// Запоминаем, что измерение не выполнялось
			result.skipped = true;
			// Запоминаем причину, по которой измерение не выполнялось
			result.reason = "обход дерева завершился быстрее разрешения часов";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = ((static_cast <double> (document.size()) * outcome.operations) / outcome.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи текста разметки
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeDocument() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект дерева разметки
		static awh::codec::xml::document_t document;
		/**
		 * Если разбор текста разметки выполнить не удалось
		 */
		if(!document.parse(device())){
			// Запоминаем, что измерение не выполнялось
			result.skipped = true;
			// Запоминаем причину, по которой измерение не выполнялось
			result.reason = "эталонный документ разобрать не удалось";
			// Выводим результат измерения
			return result;
		}
		// Получаем корневой узел собранного дерева
		const awh::codec::xml::node_t root = document.element();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(device().size(), SMALL_ROUNDS, [&root]() noexcept -> uint64_t {
			// Объект записи текста разметки
			awh::codec::xml::writer_t writer;
			// Выполняем запись объявления разметки
			writer.declaration();
			/**
			 * Если запись собранного дерева выполнить не удалось
			 */
			if(!writer.element(root) || !writer.complete())
				// Выводим нулевой объём записанного текста
				return 0;
			// Выводим объём записанного текста разметки
			return static_cast <uint64_t> (writer.text().size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки дерева ответа по договору SOAP
	 */
	static const bool SOAP_REGISTERED = awh::benchmark::add(
		"codec/xml: дерево ответа SOAP", "МБ/с", TREE_SOAP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeSoap
	);
	/**
	 * Выполняем регистрацию сценария сборки дерева крупного документа
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/xml: дерево крупного документа", "МБ/с", TREE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeLarge
	);
	/**
	 * Выполняем регистрацию сценария обхода собранного дерева
	 */
	static const bool WALK_REGISTERED = awh::benchmark::add(
		"codec/xml: обход дерева", "узл./с", TREE_WALK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeWalk
	);
	/**
	 * Выполняем регистрацию сценария записи текста разметки
	 */
	static const bool WRITE_REGISTERED = awh::benchmark::add(
		"codec/xml: запись разметки", "МБ/с", WRITE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeDocument
	);
};
