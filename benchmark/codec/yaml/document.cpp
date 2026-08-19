/**
 * @file document.cpp
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
 * @brief Бенчмарки дерева документа YAML — сборка дерева, раскрытие ссылок, извлечение
 *        значений, перезапись собранного и удержание исходного текста ради дословной
 *        перезаписи с точечной правкой
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
 * @brief Внутренние параметры и сценарии бенчмарков дерева документа
 *
 */
namespace {
	/**
	 * @brief Количество собираемых деревьев мелкого файла настроек
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 8000;
	/**
	 * @brief Количество собираемых деревьев крупного файла настроек
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 4;
	/**
	 * @brief Количество собираемых деревьев текста одного вида записи
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 12;

	/**
	 * @brief Порог пропускной способности сборки дерева мелкого файла настроек
	 *
	 * @details Пороги назначены по замеру на рабочей машине 17.08.2026 с запасом
	 *          вчетверо: отладочные стенды отстают от неё вчетверо-впятеро
	 *
	 */
	static constexpr double TREE_SERVICE_THRESHOLD = 3.0;
	/**
	 * @brief Порог пропускной способности сборки дерева крупного файла настроек
	 *
	 */
	static constexpr double TREE_LARGE_THRESHOLD = 1.5;
	/**
	 * @brief Порог пропускной способности сборки дерева с раскрытием ссылок
	 *
	 * @details Ссылка раскрывается переносом поддерева метки: показатель этот стережёт
	 *          устройство переноса - размах поддерева считается числом узлов, и перенос
	 *          идёт копированием готовых узлов, а не повторным разбором записи
	 *
	 */
	static constexpr double TREE_ANCHORS_THRESHOLD = 1.5;
	/**
	 * @brief Порог пропускной способности перезаписи собранного дерева
	 *
	 */
	static constexpr double TREE_REWRITE_THRESHOLD = 2.0;
	/**
	 * @brief Порог надбавки удержания исходного текста
	 *
	 * @details Удержание держит исходный текст целиком и просит у чтения выдачи
	 *          примечаний с пустыми строками: событий выходит больше, а узлы обзаводятся
	 *          началом записи своей. Надбавка эта - плата за дословную перезапись, и
	 *          мерится она отношением ко времени сборки без удержания
	 *
	 * @note Отношение двух прогонов на одной машине от её быстродействия не зависит, и
	 *       порог ему можно назначить впритык
	 *
	 */
	static constexpr double TREE_RETAIN_THRESHOLD = 2.5;
	/**
	 * @brief Порог пропускной способности дословной перезаписи
	 *
	 * @details Перезапись дерева, правкой не тронутого, есть перенос исходных байтов
	 *          одним куском на документ: обходится она заметно дешевле сборки заново, и
	 *          показатель этот стережёт именно её устройство. Падение его означало бы,
	 *          что перенос разбился на куски либо сменился сборкой
	 *
	 */
	static constexpr double TREE_VERBATIM_THRESHOLD = 40.0;
	/**
	 * @brief Порог надбавки дословной перезаписи над простым копированием
	 *
	 * @details Отношение двух прогонов на одной машине от быстродействия её не зависит,
	 *          и порог ему ставится впритык - в отличие от пропускной способности, какой
	 *          приходится держать запас на разброс между машинами
	 *
	 * @note Величина назначена по съёму 19.08.2026: на семи машинах надбавка легла между
	 *       0.97 и 1.07 - перезапись стоит ровно как копия, что устройство и обещает, -
	 *       и порог взят почти втрое выше худшей из снятых
	 *
	 */
	static constexpr double TREE_OVERHEAD_THRESHOLD = 3.0;
	/**
	 * @brief Порог задержки точечной правки дерева настроек в микросекундах
	 *
	 * @details Правка одного значения с сохранением оформления - главный случай
	 *          применения удержания: приложение правит одну строку файла, человеком
	 *          писанного, и всё прочее обязано вернуться байт в байт
	 *
	 */
	static constexpr double TREE_EDIT_LATENCY_THRESHOLD = 200.0;
	/**
	 * @brief Порог задержки извлечения значения по пути к нему
	 *
	 * @note Путь к узлу проходится частями, а часть внутри отображения разыскивается
	 *       перебором детей: показатель этот стережёт стоимость самого обхода
	 *
	 */
	static constexpr double TREE_LOOKUP_LATENCY_THRESHOLD = 8.0;

	/**
	 * @brief Функция сборки дерева документа по тексту настроек
	 *
	 * @param text   разбираемый текст настроек
	 * @param retain признак удержания исходного текста
	 * @return       количество собранных узлов дерева
	 *
	 */
	static uint64_t build(const string & text, const bool retain = false) noexcept {
		// Настройки разбора дерева документа
		awh::codec::yaml::document_t::settings_t settings;
		// Устанавливаем признак удержания исходного текста
		settings.retain = retain;
		// Объект дерева документа
		awh::codec::yaml::document_t document(settings);
		/**
		 * Если разобрать текст настроек не удалось
		 */
		if(!document.parse(text))
			// Выводим нулевое количество собранных узлов
			return 0;
		// Выводим количество собранных узлов дерева
		return static_cast <uint64_t> (document.size());
	}
	/**
	 * @brief Функция прогона сценария сборки дерева заданного текста настроек
	 *
	 * @param text   разбираемый текст настроек
	 * @param rounds количество собираемых деревьев
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t building(const string & text, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), rounds, [&text]() noexcept {
			// Выполняем сборку дерева документа
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
	 * @brief Функция прогона сценария сборки дерева файла настроек приложения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeService() noexcept {
		// Выполняем прогон сценария сборки дерева файла настроек приложения
		return ::building(service(), SMALL_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария сборки дерева крупного файла настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeLarge() noexcept {
		// Выполняем прогон сценария сборки дерева крупного файла настроек
		return ::building(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария сборки дерева с раскрытием ссылок
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeAnchors() noexcept {
		// Выполняем прогон сценария сборки дерева с раскрытием ссылок
		return ::building(anchors(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария перезаписи собранного дерева
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeRewrite() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = large();
		// Объект дерева документа
		awh::codec::yaml::document_t document;
		/**
		 * Если разобрать текст настроек не удалось
		 */
		if(!document.parse(text)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор эталонного текста настроек не удался";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&document]() noexcept {
			// Выполняем перезапись собранного дерева документа
			return static_cast <uint64_t> (document.dump().size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария надбавки удержания исходного текста
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeRetain() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = decorated();
		// Выполняем прогон сборки дерева без удержания исходного текста
		const outcome_t plain = measure(text.size(), FOCUSED_ROUNDS, [&text]() noexcept {
			// Выполняем сборку дерева документа без удержания
			return ::build(text, false);
		});
		// Выполняем прогон сборки дерева с удержанием исходного текста
		const outcome_t held = measure(text.size(), FOCUSED_ROUNDS, [&text]() noexcept {
			// Выполняем сборку дерева документа с удержанием
			return ::build(text, true);
		});
		/**
		 * Если замер не состоялся
		 */
		if((plain.seconds <= 0.0) || (held.seconds <= 0.0)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "замер времени не состоялся";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное отношение времени сборки с удержанием к сборке без него
		result.value = (held.seconds / plain.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(held);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария дословной перезаписи удержанного текста
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeVerbatim() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = decorated();
		// Настройки разбора дерева с удержанием исходного текста
		awh::codec::yaml::document_t::settings_t settings;
		// Устанавливаем удержание исходного текста
		settings.retain = true;
		// Объект дерева документа
		awh::codec::yaml::document_t document(settings);
		/**
		 * Если разобрать текст настроек не удалось
		 */
		if(!document.parse(text)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор эталонного текста настроек не удался";
			// Выводим результат измерения
			return result;
		}
		/**
		 * Если перезапись с исходным текстом побайтово разошлась
		 *
		 * @note Сличение это стоит в замере нарочно: показатель дословной перезаписи
		 *       мерил бы стоимость сборки заново, случись удержание сломанным, и
		 *       отчитался бы просто числом пониже вместо отказа
		 */
		if(document.dump() != text){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "дословная перезапись с исходным текстом побайтово разошлась";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), FOCUSED_ROUNDS, [&document]() noexcept {
			// Выполняем дословную перезапись удержанного текста
			return static_cast <uint64_t> (document.dump().size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария надбавки дословной перезаписи над копией
	 *
	 * @details Перезапись дерева, правкой не тронутого, есть перенос исходных байтов
	 *          одним куском, и стоить она обязана немногим дороже простого копирования
	 *          строки. Мерится здесь отношение одного к другому
	 *
	 * @note Сценарий этот держит то, чего не держит ни побайтовое сличение, ни
	 *       пропускная способность. Сличение молчит, случись удержание сломанным, а
	 *       пересборка выдай те же самые байты - а выдать их она вправе, ибо текст
	 *       эталонный оформления необычного не несёт. Пропускная же способность
	 *       расходится между машинами восемнадцатикратно (перенос памяти зависит от
	 *       машины, а не от кодека), и порог ей приходится ставить с таким запасом, что
	 *       падение вдесятеро он пропускает молча
	 *
	 * @warning Отношение от быстродействия машины не зависит вовсе, и порог ему назначен
	 *          впритык: перенос одним куском обязан укладываться в несколько копирований
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t verbatimOverhead() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = decorated();
		// Настройки разбора дерева с удержанием исходного текста
		awh::codec::yaml::document_t::settings_t settings;
		// Устанавливаем удержание исходного текста
		settings.retain = true;
		// Объект дерева документа
		awh::codec::yaml::document_t document(settings);
		/**
		 * Если разобрать текст настроек не удалось
		 */
		if(!document.parse(text)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор эталонного текста настроек не удался";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон простого копирования строки
		const outcome_t copied = measure(text.size(), FOCUSED_ROUNDS, [&text]() noexcept {
			// Выполняем копирование исходного текста
			const string copy(text);
			// Выводим размер полученной копии
			return static_cast <uint64_t> (copy.size());
		});
		// Выполняем прогон дословной перезаписи удержанного текста
		const outcome_t written = measure(text.size(), FOCUSED_ROUNDS, [&document]() noexcept {
			// Выполняем дословную перезапись удержанного текста
			return static_cast <uint64_t> (document.dump().size());
		});
		/**
		 * Если замер не состоялся
		 */
		if((copied.seconds <= 0.0) || (written.seconds <= 0.0)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "замер времени не состоялся";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное отношение перезаписи к копированию
		result.value = (written.seconds / copied.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(written);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки точечной правки дерева настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t editLatency() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = service();
		// Настройки разбора дерева с удержанием исходного текста
		awh::codec::yaml::document_t::settings_t settings;
		// Устанавливаем удержание исходного текста
		settings.retain = true;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text, &settings]() noexcept {
			// Объект дерева документа
			awh::codec::yaml::document_t document(settings);
			/**
			 * Если разобрать текст настроек не удалось
			 */
			if(!document.parse(text))
				// Выводим нулевой размер перезаписи
				return static_cast <uint64_t> (0);
			/**
			 * Если внести правку в дерево настроек не удалось
			 */
			if(!document.set("/server/port", static_cast <int64_t> (9090)))
				// Выводим нулевой размер перезаписи
				return static_cast <uint64_t> (0);
			// Выводим размер перезаписи правленого дерева
			return static_cast <uint64_t> (document.dump().size());
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "правка не выполнила ни одной операции";
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
	 * @brief Функция прогона сценария задержки извлечения значения по пути
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t lookupLatency() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = service();
		// Объект дерева документа
		awh::codec::yaml::document_t document;
		/**
		 * Если разобрать текст настроек не удалось
		 */
		if(!document.parse(text)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор эталонного текста настроек не удался";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&document]() noexcept {
			// Извлекаемое значение узла дерева
			uint64_t value = 0;
			/**
			 * Если извлечь значение по пути к нему не удалось
			 */
			if(!document.root().at("/server/limits/connections").value(value))
				// Выводим нулевое извлечённое значение
				return static_cast <uint64_t> (0);
			// Выводим извлечённое значение узла дерева
			return value;
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "извлечение не выполнило ни одной операции";
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
	 * Выполняем регистрацию сценария сборки дерева файла настроек приложения
	 */
	static const bool TREE_SERVICE_REGISTERED = awh::benchmark::add(
		"codec/yaml: сборка дерева настроек службы", "МБ/с", TREE_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeService
	);
	/**
	 * Выполняем регистрацию сценария сборки дерева крупного файла настроек
	 */
	static const bool TREE_LARGE_REGISTERED = awh::benchmark::add(
		"codec/yaml: сборка дерева крупного файла", "МБ/с", TREE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeLarge
	);
	/**
	 * Выполняем регистрацию сценария сборки дерева с раскрытием ссылок
	 */
	static const bool TREE_ANCHORS_REGISTERED = awh::benchmark::add(
		"codec/yaml: сборка дерева с раскрытием ссылок", "МБ/с", TREE_ANCHORS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeAnchors
	);
	/**
	 * Выполняем регистрацию сценария перезаписи собранного дерева
	 */
	static const bool TREE_REWRITE_REGISTERED = awh::benchmark::add(
		"codec/yaml: перезапись собранного дерева", "МБ/с", TREE_REWRITE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeRewrite
	);
	/**
	 * Выполняем регистрацию сценария надбавки удержания исходного текста
	 */
	static const bool TREE_RETAIN_REGISTERED = awh::benchmark::add(
		"codec/yaml: надбавка удержания текста", "раз", TREE_RETAIN_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, treeRetain
	);
	/**
	 * Выполняем регистрацию сценария дословной перезаписи удержанного текста
	 */
	static const bool TREE_VERBATIM_REGISTERED = awh::benchmark::add(
		"codec/yaml: дословная перезапись", "МБ/с", TREE_VERBATIM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeVerbatim
	);
	/**
	 * Выполняем регистрацию сценария надбавки дословной перезаписи над копией
	 */
	static const bool TREE_OVERHEAD_REGISTERED = awh::benchmark::add(
		"codec/yaml: надбавка перезаписи над копией", "раз", TREE_OVERHEAD_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, verbatimOverhead
	);
	/**
	 * Выполняем регистрацию сценария задержки точечной правки дерева настроек
	 */
	static const bool TREE_EDIT_REGISTERED = awh::benchmark::add(
		"codec/yaml: задержка точечной правки", "мкс/файл", TREE_EDIT_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, editLatency
	);
	/**
	 * Выполняем регистрацию сценария задержки извлечения значения по пути
	 */
	static const bool TREE_LOOKUP_REGISTERED = awh::benchmark::add(
		"codec/yaml: задержка извлечения по пути", "мкс/файл", TREE_LOOKUP_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, lookupLatency
	);
};
