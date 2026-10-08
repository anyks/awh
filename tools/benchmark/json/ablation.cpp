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
 * @brief Стенд поэлементного снятия возможностей разбора текста JSON
 *
 * @details Стенд отвечает на вопрос, куда уходит время разбора: тот же текст
 *          читается несколько раз, и всякий раз снимается одна из возможностей,
 *          которых у сличаемых реализаций нет. Разница между прогонами и есть
 *          цена снятой возможности
 *
 * @note Возможности снимаются накопительно: всякий следующий прогон идёт без
 *       всех прежде снятых. Снятие по одной от исходного набора показало бы цену
 *       возможности в отрыве от прочих, а нужен путь от нашего чтения к чужому
 *
 * @note Долю хода, которого настройки не касают, этим стендом не снять: поверку
 *       кодировки, разбор записей отмены и преобразование чисел выключают
 *       правкой исходника по `tools/benchmark/ablate.sh`, и число, снятое тем
 *       порядком, называют потолком, а не обещанием правки
 *
 * @note Послабления разбора - примечания, одинарные кавычки, запятая перед
 *       закрывающей скобкой, записи NaN и Infinity, поток NDJSON - гасятся
 *       НАСТРОЙКАМИ ПО УМОЛЧАНИЮ, и снимать с исходного чтения стенду нечего:
 *       их цена видна лишь прогоном с поднятыми признаками, а не снятием
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/json/document.hpp>

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
 * @brief Разбираемый в настоящее время набор настроек документа
 *
 * @note Набор передаётся через хранилище, а не доводом: договор функции разбора
 *       у всех стендов один, и менять его ради одного стенда было бы неверно
 *
 */
static awh::codec::json::document_t::settings_t CURRENT;

/**
 * @brief Функция обхода собранного дерева документа
 *
 * @param value обходимое значение документа
 *
 */
static void walk(const awh::codec::json::document_t::value_t & value) noexcept {
	/**
	 * Определяем вид обходимого значения документа
	 */
	switch(static_cast <uint8_t> (value.kind())){
		// Если значение является пустым
		case static_cast <uint8_t> (awh::codec::json::kind_t::NUL):
			// Выполняем учёт прочитанного пустого значения
			rival::nothing();
		break;
		/**
		 * Если значение является логическим
		 */
		case static_cast <uint8_t> (awh::codec::json::kind_t::BOOL): {
			// Прочитанное логическое значение
			bool result = false;
			// Выполняем извлечение логического значения
			value.value(result);
			// Выполняем учёт прочитанного логического значения
			rival::consume(result);
		} break;
		/**
		 * Если значение является числом
		 */
		case static_cast <uint8_t> (awh::codec::json::kind_t::NUMBER): {
			// Прочитанное число
			double result = 0.;
			// Выполняем извлечение числа
			value.value(result);
			// Выполняем учёт прочитанного числа
			rival::consume(result);
		} break;
		/**
		 * Если значение является строкой
		 */
		case static_cast <uint8_t> (awh::codec::json::kind_t::STRING): {
			// Получаем прочитанное строковое значение
			const std::string_view result = value.text();
			// Выполняем учёт прочитанного строкового значения
			rival::consume(result.data(), result.size());
		} break;
		/**
		 * Если значение является вместилищем
		 */
		case static_cast <uint8_t> (awh::codec::json::kind_t::ARRAY):
		case static_cast <uint8_t> (awh::codec::json::kind_t::OBJECT): {
			/**
			 * Выполняем обход всех значений вместилища
			 */
			for(auto item = value.begin(); item.valid(); item = item.next()){
				// Получаем имя поля объекта
				const std::string_view name = item.name();
				/**
				 * Если значение является полем объекта
				 */
				if(!name.empty())
					// Выполняем учёт прочитанного имени поля объекта
					rival::consume(name.data(), name.size());
				// Выполняем обход очередного значения вместилища
				walk(item);
			}
		} break;
	}
}
/**
 * @brief Функция разбора одного документа
 *
 * @param text разбираемый текст документа
 * @return     признак успешного разбора
 *
 */
static bool parse(const std::string & text) noexcept {
	// Объект документа
	awh::codec::json::document_t doc;
	// Выполняем установку настроек разбора текущего прогона
	doc.settings(CURRENT);
	/**
	 * Если разбор текста документа выполнить не удалось
	 */
	if(!doc.parse(text))
		// Выводим признак неудачного разбора
		return false;
	// Выполняем обход собранного дерева документа
	walk(doc.root());
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Функция получения исходного набора настроек документа
 *
 * @return исходный набор настроек документа
 *
 */
static awh::codec::json::document_t::settings_t initial() noexcept {
	/**
	 * Собираемые настройки разбора текста документа
	 *
	 * @note Исходным служит комплект по умолчанию: снятие предела или гашение
	 *       поверки в него не входят, ибо ровно так читает кодек и без стенда
	 */
	awh::codec::json::document_t::settings_t result;
	// Выводим собранные настройки разбора
	return result;
}

/**
 * @brief Структура снимаемой возможности разбора
 *
 */
typedef struct Ablation {
	// Название снимаемой возможности
	const char * name;
	// Функция снятия возможности с настроек документа
	void (* apply)(awh::codec::json::document_t::settings_t &);
} ablation_t;

/**
 * @brief Перечень снимаемых возможностей разбора
 *
 */
static const ablation_t ABLATIONS[] = {
	{"исходное чтение", [](awh::codec::json::document_t::settings_t &) noexcept {}},
	{"без пределов длины записей", [](awh::codec::json::document_t::settings_t & settings) noexcept {
		// Снимаем предел длины строкового значения
		settings.reader.maxString = 0;
		// Снимаем предел длины записи числа
		settings.reader.maxNumber = 0;
	}},
	{"без предела вложенности", [](awh::codec::json::document_t::settings_t & settings) noexcept {
		// Снимаем предел вложенности вместилищ
		settings.reader.maxDepth = 0;
	}},
	{"без поверки повторяющихся имён", [](awh::codec::json::document_t::settings_t & settings) noexcept {
		// Переставляем правило обращения с повторяющимся именем на удержание всех
		settings.duplicates = awh::codec::json::duplicate_t::KEEP;
	}},
	{"без определения кодировки", [](awh::codec::json::document_t::settings_t & settings) noexcept {
		// Навязываем кодировку, вместо её определения по метке порядка байтов
		settings.reader.encoding = awh::codec::json::encoding_t::UTF8;
	}}
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
	// Собираемые настройки разбора текста документа
	awh::codec::json::document_t::settings_t settings = initial();
	// Отбор прогонов по вхождению снятой возможности в название
	const char * chosen = rival::filter(argc, argv);
	// Итоги прогона одного сценария
	rival::outcome_t outcome{0, 0, 0.0};
	/**
	 * Выполняем перебор всех снимаемых возможностей разбора
	 */
	for(const ablation_t & ablation : ABLATIONS){
		// Выполняем снятие очередной возможности с настроек документа
		ablation.apply(settings);
		// Запоминаем настройки разбора текущего прогона
		CURRENT = settings;
		/**
		 * Выполняем перебор всех эталонных текстов документа
		 */
		for(const rival::scenario_t & scenario : rival::scenarios()){
			/**
			 * Собираем название прогона: эталонный текст и снятая возможность разом
			 *
			 * @note Разделителем служит косая черта, и отбор параметром `--filter=`
			 *       берёт либо один текст по всем ступеням, либо одну ступень по всем
			 *       текстам - и то и другое нужно для сверки
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
