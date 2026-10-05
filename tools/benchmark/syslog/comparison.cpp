/**
 * @file comparison.cpp
 * @date 2026-10-05
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Стенд сличения разбора записей системного журнала — прежний модуль ANYKS против
 *        кодека AWH, с поверкой равенства работы впереди показателей
 *
 * @details Разборщиков syslog на C++ в открытом виде, годных к рядом поставке, нет:
 *          формат разбирают либо сборки демонов (rsyslog, syslog-ng), либо надстройки над
 *          сборщиками журналов, либо связки на Python и Go. Сличать их рядом значило бы
 *          мерить языки и устройства, а не разборы. Оттого участник один — прежний модуль
 *          ANYKS, тот самый, что дан был для аналогии устройства
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <string>

/**
 * Подключаем заголовочные файлы прежнего модуля
 */
#include <syslog.hpp>

/**
 * Подключаем заголовочные файлы кодека AWH
 */
#include <codec/syslog/document.hpp>
#include <codec/syslog/reader.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Запись RFC 5424 с двумя блоками данных и повторяющимся именем поля
 *
 * @note Значения полей блоков обнесены кавычками всюду: описание (RFC 5424, раздел 6.3.3)
 *       требует их и для единственного знака, а прежний модуль ненакрытое значение
 *       принимал молча. На записи `ip=192.168.59.39` кодек AWH отвечал отказом
 *       «значение поля не обнесено кавычками» в столбце 77, а сверка шла бы не одного и
 *       того же текста, а разного поведения на браке
 */
static const string MODERN = "<34>1 2003-10-11T22:14:15.003Z hostname application 1 ID47 "
	"[origin@32473 ip=\"192.168.59.39\" enterpriseid=\"9f2c\"][example@42 k=\"первое\" k=\"второе\"] "
	"текст сообщения системного журнала";

/**
 * Запись RFC 3164 прежней раскладки служб журнала
 */
static const string LEGACY = "<34>Oct 11 22:14:15 hostname application[1]: текст сообщения";

/**
 * @brief Метод замера времени указанного числа оборотов
 *
 * @param rounds число оборотов замера
 * @param run    выполняемая операция
 * @return       затраченное время в секундах
 */
template <typename F> static double measure(const size_t rounds, F run) noexcept {
	// Выполняем разогрев без замера
	run();
	// Начало окна замера
	const auto begin = chrono::steady_clock::now();
	/**
	 * Выполняем перебор всех оборотов замера
	 */
	for(size_t i = 0; i < rounds; i++)
		// Выполняем прогон измеряемой операции
		run();
	// Конец окна замера
	const auto finish = chrono::steady_clock::now();
	// Выводим затраченное время в секундах
	return chrono::duration <double> (finish - begin).count();
}

/**
 * @brief Метод поверки того, что оба модуля делают одну и ту же работу
 *
 * @note Поверка печатается ДО чисел и не по обычаю, а по необходимости: обе реализации
 *       строят дерево, но итоги их сходятся не во всём, и расхождение, спрятанное за
 *       средним значением, годно лишь на то, чтобы вводить в заблуждение
 */
static void proof(void) noexcept {
	// Объект разбора прежнего модуля
	anyks::syslog_t before;
	// Выполняем разбор прежним модулем
	before.parse(MODERN, anyks::syslog_t::std_t::AUTO);
	// Объект разбора кодека AWH
	awh::codec::syslog::document_t after;
	// Выполняем разбор кодеком AWH
	const bool parsed = after.parse(MODERN);
	// Выполняем вывод вердикта разбора кодека AWH
	printf("== поверка равенства работы ==\n");
	// Выводим число блоков данных, разобранных каждым модулем
	printf("прежний: блоков %zu  AWH: блоков %zu (годен: %d)\n",
		static_cast <size_t> (before.has("origin@32473") + before.has("example@42")), after.size(), (parsed ? 1 : 0));
	// Выводим опознаватель, имя службы, процесса и сообщения
	printf("прежний: host=%s app=%s mid=%s\n", before.host().c_str(), before.application().c_str(), before.mid().c_str());
	printf("AWH    : host=%s app=%s mid=%s\n",
		after.at("/header/hostname").text().c_str(), after.at("/header/application").text().c_str(), after.at("/header/messageId").text().c_str());
	/**
	 * Выводим поле, повторяющееся в блоке данных дважды
	 *
	 * @note Повтор имени поля обращения в перечень (RFC 5424, раздел 6.3.3): прежний модуль
	 *       держит отображение имён в одном значении и второе теряет, кодек AWH несёт оба
	 */
	printf("прежний: example@42/k=%s\n", before.sd("example@42").count("k") ? before.sd("example@42").at("k").c_str() : "нет");
	printf("AWH    : example@42/k/0=%s /k/1=%s\n", after.at("/structures/example@42/k/0").text().c_str(),
		after.at("/structures/example@42/k/1").text().c_str());
	// Выводим содержимое сообщения, снятое каждым модулем
	printf("прежний: msg=%s\n", before.message().c_str());
	printf("AWH    : msg=%s\n\n", after.at("/message").text().c_str());
}

/**
 * @brief Функция запуска сличения разбора записей системного журнала
 *
 * @param argc количество доводов вызова
 * @param argv перечень доводов вызова: первым — число кругов
 * @return     код выхода из сличения
 */
int32_t main(const int32_t argc, char ** argv) noexcept {
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	awh::fmk::initialize();
	// Отключаем выдачу журнала: отказ кодека печатался бы внутрь замерного хода
	awh::log::mode({});
	// Выполняем поверку равенства работы прежде показателей
	proof();
	// Получаем число кругов замера
	const size_t rounds = ((argc > 1) ? strtoul(argv[1], nullptr, 10) : 20000);
	// Получаем величина записей, приходящаяся на круг замера
	const double bytes = static_cast <double> ((MODERN.size() + LEGACY.size()) * rounds) / 1048576.0;
	/**
	 * Выполняем замер прежнего модуля на обеих записях
	 */
	{
		// Объект разбора прежнего модуля
		anyks::syslog_t syslog;
		// Выполняем замер времени разбора обеих записей
		const double seconds = measure(rounds, [&syslog]() noexcept {
			syslog.clear();
			syslog.parse(MODERN, anyks::syslog_t::std_t::AUTO);
			syslog.clear();
			syslog.parse(LEGACY, anyks::syslog_t::std_t::AUTO);
		});
		// Выводим показатели прежнего модуля
		printf("прежний ANYKS syslog (AUTO):   %8.2f МБ/с  %8.2f мкс/запись\n", bytes / seconds, seconds * 1e6 / (rounds * 2));
	}
	/**
	 * Выполняем замер прежнего модуля вместе с постройкою дерева
	 *
	 * @note Разбор одного лишь значения кодеку `document_t` не пара: тот укладывает разбор
	 *       в дерево `abc::value_t`, а прежний модуль держит свои поля отдельно и деревом
	 *       отдаёт их лишь через `dump()`. Строка «дерево к дереву» без этого хода
	 *       сличала бы разное количество работы
	 */
	{
		// Объект разбора прежнего модуля
		anyks::syslog_t syslog;
		// Дерево, куда прежний модуль кладёт разобранное
		anyks::json tree;
		// Выполняем замер времени разбора обеих записей с постройкою дерева
		const double seconds = measure(rounds, [&syslog, &tree]() noexcept {
			syslog.clear();
			syslog.parse(MODERN, anyks::syslog_t::std_t::AUTO);
			tree = syslog.dump();
			syslog.clear();
			syslog.parse(LEGACY, anyks::syslog_t::std_t::AUTO);
			tree = syslog.dump();
		});
		// Выводим показатели прежнего модуля с постройкою дерева
		printf("прежний ANYKS syslog (с деревом):%8.2f МБ/с  %8.2f мкс/запись\n", bytes / seconds, seconds * 1e6 / (rounds * 2));
	}
	/**
	 * Выполняем замер сборки записи прежним модулем
	 */
	{
		// Объект разбора прежнего модуля
		anyks::syslog_t syslog;
		// Выполняем разбор обеих записей единократно
		syslog.parse(MODERN, anyks::syslog_t::std_t::AUTO);
		// Получаем собранную запись прежнего модуля
		const string rebuilt = syslog.syslog();
		// Выполняем замер времени сборки записи
		const double seconds = measure(rounds, [&syslog]() noexcept { syslog.syslog(); });
		// Выводим показатели сборки прежнего модуля
		printf("прежний ANYKS syslog (сборка): %8.2f МБ/с  %8.2f мкс/запись  (собрано %zu октетов)\n",
			static_cast <double> (rebuilt.size() * rounds) / 1048576.0 / seconds, seconds * 1e6 / rounds, rebuilt.size());
	}
	/**
	 * Выполняем замер кодека AWH на обеих записях
	 */
	{
		// Объект разбора кодека AWH
		awh::codec::syslog::document_t document;
		// Выполняем замер времени разбора обеих записей
		const double seconds = measure(rounds, [&document]() noexcept { document.parse(MODERN); document.parse(LEGACY); });
		// Выводим показатели кодека AWH
		printf("AWH codec::syslog (дерево):    %8.2f МБ/с  %8.2f мкс/запись\n", bytes / seconds, seconds * 1e6 / (rounds * 2));
	}
	/**
	 * Выполняем замер кодека AWH на потоковом чтении событий
	 */
	{
		// Объект потокового чтения записей журнала
		awh::codec::syslog::reader_t reader;
		// Выполняем замер времени потокового чтения обеих записей
		const double seconds = measure(rounds, [&reader]() noexcept {
			reader.reset();
			reader.feed(MODERN);
			while(reader.next());
			reader.reset();
			reader.feed(LEGACY);
			while(reader.next());
		});
		// Выводим показатели потокового чтения
		printf("AWH codec::syslog (поток):     %8.2f МБ/с  %8.2f мкс/запись\n", bytes / seconds, seconds * 1e6 / (rounds * 2));
	}
	/**
	 * Выполняем замер сборки записи кодеком AWH
	 */
	{
		// Объект разбора кодека AWH
		awh::codec::syslog::document_t document;
		// Выполняем разбор обеих записей единократно
		document.parse(MODERN);
		// Выполняем замер времени сборки записи
		const double seconds = measure(rounds, [&document]() noexcept { document.dump(); });
		// Выводим показатели сборки кодека AWH
		printf("AWH codec::syslog (сборка):    %8.2f МБ/с  %8.2f мкс/запись  (собрано %zu октетов)\n",
			static_cast <double> (document.dump().size() * rounds) / 1048576.0 / seconds, seconds * 1e6 / rounds, document.dump().size());
	}
	// Выводим успешный код выхода
	return 0;
}
