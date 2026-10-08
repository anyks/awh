/**
 * @file ablation.cpp
 * @date 2026-10-08
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
 * @brief Стенд поэлементного снятия возможностей потокового чтения текста разметки
 *
 * @details Стенд отвечает на вопрос, куда уходит время чтения: тот же текст
 *          читается несколько раз, и всякий раз снимается одна из возможностей,
 *          которых у сличаемых реализаций нет. Разница между прогонами и есть
 *          цена снятой возможности
 *
 * @note Возможности снимаются накопительно: всякий следующий прогон идёт без
 *       всех прежде снятых. Снятие по одной от исходного набора показало бы цену
 *       возможности в отрыве от прочих, а нужен путь от нашего чтения к чужому
 *
 * @note Пределы снимются именно нулём: всякая поверка объёма сверяется видом
 *       `maxEvent > 0`, поэтому ноль выключает и сравнение, а не только отказ
 *
 * @note Ступени, какие меняют наблюдаемый результат на этом наборе текстов,
 *       стендом не снимаются: `namespaces` переставляет самые имена узлов и
 *       оставляет объявления `xmlns` обычными атрибутами, `separateSpaces` и
 *       `mergeText` меняют состав выдаваемых событий. Равенство работы до всякого
 *       числа здесь - не вежливость, а условие годности меры: `--checksum` всякой
 *       ступени обязан сойтись с исходным чтением
 *
 * @note Снятие владеющего поддерева (сценарии `copy` стенда `awh.cpp`) здесь
 *       отсутствует: дерево собирается одной кэш-записью на текст, и настройки
 *       следующей ступени до него не доходят
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/xml/reader.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: посредник этот нужен одному лишь
 *       файлу, и вынос его наружу связал бы стенды между собою без нужды
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
 * @brief Разбираемый в настоящее время набор настроек чтения
 *
 * @note Набор передаётся через хранилище, а не доводом: договор функции разбора
 *       у всех стендов один, и менять его ради одного стенда было бы неверно
 *
 */
static awh::codec::xml::reader_t::settings_t CURRENT;

/**
 * @brief Функция разбора одного документа
 *
 * @param text разбираемый текст разметки
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект потокового чтения текста разметки
	awh::codec::xml::reader_t reader(CURRENT);
	/**
	 * Если передать текст разметки не удалось
	 */
	if(!reader.feed(text))
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		/**
		 * Определяем вид полученного события разбора
		 */
		switch(static_cast <uint8_t> (reader.event())){
			/**
			 * Если получено начало узла разметки
			 */
			case static_cast <uint8_t> (awh::codec::xml::event_t::ELEMENT_OPEN): {
				// Выполняем учёт обработанного узла разметки
				rival::node();
				// Выполняем чтение имени узла разметки
				rival::touch(reader.name().local.data(), reader.name().local.size());
				/**
				 * Выполняем перебор всех атрибутов узла разметки
				 */
				for(const awh::codec::xml::attribute_t & attribute : reader.attributes()){
					// Выполняем чтение имени очередного атрибута
					rival::touch(attribute.name.local.data(), attribute.name.local.size());
					// Выполняем чтение значения очередного атрибута
					rival::touch(attribute.value.data(), attribute.value.size());
				}
			} break;
			/**
			 * Если получен конец узла разметки
			 */
			case static_cast <uint8_t> (awh::codec::xml::event_t::ELEMENT_CLOSE):
				// Выполняем чтение имени узла разметки
				rival::touch(reader.name().local.data(), reader.name().local.size());
			break;
			/**
			 * Если получено текстовое содержимое узла
			 */
			case static_cast <uint8_t> (awh::codec::xml::event_t::TEXT):
			/**
			 * Если получен раздел дословного текста
			 */
			case static_cast <uint8_t> (awh::codec::xml::event_t::CDATA):
				// Выполняем учёт содержимого узла разметки
				rival::consume(reader.text().data(), reader.text().size());
			break;
		}
	}
	// Выводим признак успешного разбора по состоянию чтения
	return (reader.state() == awh::codec::xml::state_t::FINISHED);
}
/**
 * @brief Функция получения исходного набора настроек чтения
 *
 * @return исходный набор настроек чтения
 *
 */
static awh::codec::xml::reader_t::settings_t initial() noexcept {
	/**
	 * Собираемые настройки разбора текста разметки
	 *
	 * @note Исходным служит комплект по умолчанию - ровно тем читателя собирает
	 *       стенд `awh.cpp`, и базис обязан совпадать с его работой, иначе рядом
	 *       встанут две несравненные меры. Выдачу примечаний и обработок здесь НЕ
	 *       гасят, хотя стенд эти события не рассматривает: в эталонных текстах их
	 *       нет, и снятие только оторвало бы базис от опубликованного ряда
	 */
	awh::codec::xml::reader_t::settings_t result;
	// Выводим собранные настройки разбора
	return result;
}

/**
 * @brief Структура снимаемой возможности чтения
 *
 */
typedef struct Ablation {
	// Название снимаемой возможности
	const char * name;
	// Функция снятия возможности с настроек чтения
	void (* apply)(awh::codec::xml::reader_t::settings_t &);
} ablation_t;

/**
 * @brief Перечень снимаемых возможностей чтения
 *
 */
static const ablation_t ABLATIONS[] = {
	{"исходное чтение", [](awh::codec::xml::reader_t::settings_t &) noexcept {}},
	{"без предела события", [](awh::codec::xml::reader_t::settings_t & settings) noexcept {
		// Снимаем поверку наибольшего объёма события
		settings.maxEvent = 0;
	}},
	{"без пределов разметки", [](awh::codec::xml::reader_t::settings_t & settings) noexcept {
		// Снимаем поверку длины имени узла
		settings.maxName = 0;
		// Снимаем поверку числа атрибутов узла
		settings.maxAttributes = 0;
		// Снимаем поверку глубины вложенности узлов
		settings.maxDepth = 0;
	}},
	{"без сущностей", [](awh::codec::xml::reader_t::settings_t & settings) noexcept {
		// Снимаем подстановку объявленных сущностей и поиск разметки в ссылках
		settings.entities = false;
		// Снимаем поверку числа объявленных сущностей
		settings.maxEntities = 0;
		// Снимаем поверку общего объёма подстановки
		settings.maxExpansion = 0;
	}},
	{"без значений по умолчанию", [](awh::codec::xml::reader_t::settings_t & settings) noexcept {
		// Снимаем подстановку значений атрибутов, объявленных по умолчанию
		settings.defaults = false;
	}},
	{"без определения кодировки", [](awh::codec::xml::reader_t::settings_t & settings) noexcept {
		// Навязываем кодировку, вместо её определения по метке порядка байтов
		settings.encoding = awh::codec::xml::encoding_t::UTF8;
	}}
};

/**
 * @brief Структура сценария стенда
 *
 */
struct scenario_t {
	// Название сценария
	const char * name;
	// Количество прогонов разбора
	size_t rounds;
	// Функция получения разбираемого текста разметки
	const std::string & (* text)() noexcept;
};

/**
 * @brief Метод получения перечня сценариев стенда
 *
 * @return перечень сценариев стенда
 *
 * @note Состав и число прогонов совпадают со стендом `awh.cpp`, кроме снятий
 *       владеющего поддерева: те настройки чтения не трогают
 */
static const std::vector <scenario_t> & scenarios() noexcept {
	// Перечень сценариев стенда
	static const std::vector <scenario_t> result = {
		{"soap",       rival::SMALL_ROUNDS,     rival::soap},
		{"device",     rival::SMALL_ROUNDS / 4, rival::device},
		{"large",      rival::LARGE_ROUNDS,     rival::large},
		{"attributes", rival::FOCUSED_ROUNDS,   rival::attributes},
		{"content",    rival::FOCUSED_ROUNDS,   rival::content},
		{"nested",     rival::SMALL_ROUNDS,     rival::nested}
	};
	// Выводим перечень сценариев стенда
	return result;
}

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
	// Собираемые настройки разбора текста разметки
	awh::codec::xml::reader_t::settings_t settings = initial();
	// Получаем отбор прогонов по вхождению в название
	const char * chosen = rival::filter(argc, argv);
	// Итоги прогона одного сценария
	rival::outcome_t outcome{0, 0, 0.0, 0, 0};
	/**
	 * Выполняем перебор всех снимаемых возможностей чтения
	 */
	for(const ablation_t & ablation : ABLATIONS){
		// Выполняем снятие очередной возможности с настроек чтения
		ablation.apply(settings);
		// Запоминаем настройки разбора текущего прогона
		CURRENT = settings;
		/**
		 * Выполняем перебор всех эталонных текстов разметки
		 */
		for(const scenario_t & scenario : scenarios()){
			/**
			 * Собираем название прогона: эталонный текст и снятая возможность разом
			 *
			 * @note Разделителем служит косая черта, и отбор параметром `--filter=`
			 *       берёт либо одну ступень по всем текстам, либо один текст по всем
			 *       ступеням - и то и другое нужно для сверки
			 */
			const std::string name = std::string(scenario.name) + "/" + ablation.name;
			/**
			 * Если прогон отбором не выбран
			 */
			if(!rival::selected(name.c_str(), chosen))
				// Выполняем переход к следующему эталонному тексту
				continue;
			/**
			 * Если прогон сценария выполнить не удалось
			 */
			if(!rival::parsing(parse, scenario.text(), scenario.rounds, outcome)){
				// Выводим сообщение о пропуске прогона
				rival::skip(name.c_str(), "parsing failed");
				// Выполняем переход к следующему эталонному тексту
				continue;
			}
			// Выводим результат прогона
			rival::report(name.c_str(), outcome);
		}
	}
	// Выводим контрольную сумму работы, выполненной стендом
	rival::digest(argc, argv);
	// Выводим успешный код выхода из стенда
	return EXIT_SUCCESS;
}
