/**
 * @file awh-tree.cpp
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
 * @brief Стенд поэлементного снятия возможностей сборки дерева текста настроек
 *
 * @details Стенд отвечает на вопрос, куда уходит время сборки дерева: тот же
 *          текст разбирается несколько раз, и всякий раз снимается одна из
 *          возможностей, которых у сличаемых реализаций нет. Ступень выбирается
 *          параметром `--ablation=<номер>` и снимается накопительно: ступень N
 *          снимает первые N из перечня. Равенство работы до всякого числа
 *          проверяется параметром `--checksum`: суммы всякой ступени обязаны
 *          сойтись с исходным чтением
 *
 * @note Пределы (`maxDepth`, `maxScalar`, `maxExpansion`) в перечень не входят:
 *       при нуле кодек берёт предел модуля и сравнение остаётся, а снять его
 *       настройкой нельзя. Схема `schema` меняет самые виды значений, а `retain`
 *       возможность добавляет, а не снимает
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/yaml/document.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"
#include <sys/log.hpp>
#include <sys/fmk.hpp>

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
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept {
			// Выполняем отключение вывода логов
			awh::log::mode({});
		}
	};
}


/**
 * @brief Разбираемый в настоящее время набор настроек документа
 *
 * @note Набор передаётся через хранилище, а не доводом: договор функции разбора
 *       у всех стендов один, и менять его ради одного стенда было бы неверно
 *
 * @note Поле `duplicates` у настроек самого читателя этим стендом не снимается:
 *       потоковый разбор имён пары не удерживает, и в `src/codec/yaml/reader.cpp`
 *       поле не читается ни одним местом - поверка живёт только у построения дерева
 *       (`src/codec/yaml/document.cpp:1898`, `:5679`)
 *
 */
static awh::codec::yaml::document_t::settings_t CURRENT;

/**
 * @brief Снимаемая возможность сборки дерева, выбираемая параметром `--ablation=`
 *
 */
typedef struct Ablation {
	// Название снимаемой возможности
	const char * name;
	// Функция снятия возможности с настроек документа
	void (* apply)(awh::codec::yaml::document_t::settings_t &);
} ablation_t;

/**
 * @brief Перечень снимаемых возможностей сборки дерева
 *
 */
static const ablation_t ABLATIONS[] = {
	{"исходное чтение", [](awh::codec::yaml::document_t::settings_t &) noexcept {}},
	{"без поверки повторяющихся имён", [](awh::codec::yaml::document_t::settings_t & settings) noexcept {
		// Переставляем правило обращения с повторяющимся именем на удержание всех
		settings.duplicates = awh::codec::yaml::duplicate_t::KEEP;
	}},
	{"без определения кодировки", [](awh::codec::yaml::document_t::settings_t & settings) noexcept {
		// Навязываем кодировку, вместо её определения по метке порядка байтов
		settings.encoding = awh::codec::yaml::encoding_t::UTF8;
	}}
};

/**
 * @brief Функция обхода собранного узла дерева настроек
 *
 * @details Обход ведётся рекурсией по построению дерева, а не стопою: сличаемые
 * реализации обходятся своими средствами обхода, и всякая из них рекурсивна,
 * поэтому стоимость самого обхода у стендов одинакова
 *
 * @param value обходимый узел дерева настроек
 *
 */
static void walk(const awh::codec::yaml::document_t::value_t & value) noexcept {
	// Получаем имя обходимого узла дерева
	const std::string_view name = value.name();
	/**
	 * Если обходимый узел является парой отображения
	 */
	if(!name.empty()){
		// Выполняем учёт обработанного имени пары
		rival::entry();
		// Выполняем учёт содержимого имени пары
		rival::consume(name.data(), name.size());
	}
	/**
	 * Если обходимый узел является построением
	 */
	if(value.is(awh::codec::yaml::type_t::SEQUENCE) || value.is(awh::codec::yaml::type_t::MAPPING)){
		/**
		 * Выполняем перебор всех детей обходимого узла
		 */
		for(awh::codec::yaml::document_t::value_t item = value.begin(); item.valid(); item = item.next())
			// Выполняем обход очередного ребёнка узла
			walk(item);
		// Выходим из обхода узла дерева
		return;
	}
	// Получаем содержимое обходимого скалярного значения
	const std::string_view text = value.text();
	// Выполняем учёт обработанного скалярного значения
	rival::entry();
	// Выполняем учёт содержимого скалярного значения
	rival::consume(text.data(), text.size());
}
/**
 * @brief Функция разбора одного файла настроек
 *
 * @param text разбираемый текст настроек
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект дерева настроек
	awh::codec::yaml::document_t document(CURRENT);
	/**
	 * Если разобрать текст настроек не удалось
	 */
	if(!document.parse(text))
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Выполняем перебор всех документов разобранного текста
	 */
	for(size_t i = 0; i < document.documents(); i++)
		// Выполняем обход корня очередного документа
		walk(document.root(i));
	// Выводим признак успешного разбора
	return true;
}

/**
 * @brief Перечень сценариев стенда
 *
 */
static const rival::scenario_t SCENARIOS[] = {
	{"service",   rival::TREE_SMALL_ROUNDS,   rival::service,   parse},
	{"large",     rival::TREE_LARGE_ROUNDS,   rival::large,     parse},
	{"strings",   rival::TREE_FOCUSED_ROUNDS, rival::strings,   parse},
	{"numbers",   rival::TREE_FOCUSED_ROUNDS, rival::numbers,   parse},
	{"arrays",    rival::TREE_FOCUSED_ROUNDS, rival::arrays,    parse},
	{"blocks",    rival::TREE_FOCUSED_ROUNDS, rival::blocks,    parse},
	{"anchors",   rival::TREE_FOCUSED_ROUNDS, rival::anchors,   parse},
	{"decorated", rival::TREE_FOCUSED_ROUNDS, rival::decorated, parse}
};

/**
 * @brief Главная функция стенда
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из стенда
 *
 */
int32_t main(int32_t argc, char * argv[]){
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	awh::fmk::initialize();
	// Номер снимаемой ступени, выбираемый параметром `--ablation=`
	size_t steps = 0;
	/**
	 * Выполняем перебор всех параметров запуска ради выбора ступени
	 */
	for(int32_t i = 1; i < argc; i++){
		/**
		 * Если параметром является номер снимаемой ступени
		 */
		if(::strncmp(argv[i], "--ablation=", 11) == 0){
			// Выполняем чтение номера снимаемой ступени
			const long number = ::strtol(argv[i] + 11, nullptr, 10);
			/**
			 * Если номер ступени неверен
			 */
			if((number < 0) || (number >= static_cast <long> (sizeof(ABLATIONS) / sizeof(ablation_t)))){
				// Выводим сообщение о неверном номере ступени
				::printf("ablation step must be within 0..%zu\n", sizeof(ABLATIONS) / sizeof(ablation_t) - 1);
				// Выводим код отказа разбора параметров запуска
				return EXIT_FAILURE;
			}
			// Запоминаем номер снимаемой ступени
			steps = static_cast <size_t> (number);
		}
	}
	/**
	 * Выполняем снятие всех ступеней до выбранной включительно
	 */
	for(size_t step = 0; step <= steps; step++)
		// Выполняем снятие очередной возможности с настроек документа
		ABLATIONS[step].apply(CURRENT);
	// Выводим название снимаемой ступени: ряд без подписи неотличим от ряда иной
	::printf("ступень: %s\n", ABLATIONS[steps].name);
	// Выполняем прогон всех сценариев стенда
	return rival::run(argc, argv, SCENARIOS, (sizeof(SCENARIOS) / sizeof(SCENARIOS[0])));
}
