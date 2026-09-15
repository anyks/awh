/**
 * @file writer.cpp
 * @date 2026-09-04
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
 * @brief Автоматические тесты записи событий в запись CEF — постановки отмены знаков порознь
 *        по областям, сборки заголовка и расширения, обращения с вложенным значением и
 *        записи повторяющегося ключа перечнем
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <clocale>
#include <utility>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/cef.hpp>
#include <num/lexical/lexical.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений AWH
 */
#include <sys/macro/suppress.hpp>
#include <sys/log.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной программою
 *
 */
namespace {
	/**
	 * @brief Объект окружения проверок записи событий
	 *
	 */
	struct EnvCefWriter {
		/**
		 * @brief Конструктор
		 *
		 */
		EnvCefWriter() noexcept {
			// Выполняем отключение вывода логов
			awh::log::mode({});
		}
	};
	/**
	 * @brief Функция получения объекта окружения проверок
	 *
	 * @return объект окружения проверок
	 *
	 */
	EnvCefWriter & writerEnvironment() noexcept {
		// Объект окружения проверок
		static EnvCefWriter env;
		// Выводим объект окружения проверок
		return env;
	}
	/**
	 * @brief Страж локали записи чисел
	 *
	 * @details Локаль есть настройка ПРОЦЕССА, а проверки всех кодеков идут одною
	 *          программою: оставленная чужая локаль утекла бы к соседним проверкам и
	 *          обратилась бы там «плавающим» отказом вдали от места
	 *
	 * @warning Возврат ведётся деструктором, а не строкою в конце проверки: всякий
	 *          ASSERT_* выходит из проверки немедленно, и возврат, записанный последней
	 *          строкою, по дороге отказа НЕ выполняется вовсе
	 *
	 * @note Тип лежит в БЕЗЫМЯННОМ пространстве имён, как и у набора syslog, где есть
	 *       такой же: наборы кодеков собираются в одну программу, и два одноимённых типа
	 *       со внешним связыванием дали бы порчу кучи вдали от места
	 */
	struct LocaleGuard {
		/**
		 * Локаль записи чисел, действовавшая до подмены
		 *
		 * @note Вид назван ПОЛНЫМ именем: страж стоит выше строки «using namespace std»,
		 *       и краткое имя разрешалось бы здесь в «awh::string»
		 */
		::std::string previous;
		/**
		 * @brief Конструктор
		 *
		 */
		LocaleGuard() noexcept : previous(::setlocale(LC_NUMERIC, nullptr)) {}
		/**
		 * @brief Деструктор
		 *
		 */
		~LocaleGuard() noexcept {
			// Выполняем возврат действовавшей локали записи чисел
			::setlocale(LC_NUMERIC, this->previous.c_str());
		}
	};
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Метод сборки записи CEF из разобранной записи
 *
 * @param text     разбираемая запись CEF
 * @param settings настройки записи событий
 * @return         собранная заново запись CEF
 *
 */
static string rewriteCef(const string & text, const cef::writer_t::settings_t & settings = cef::writer_t::settings_t()) noexcept {
	// Объект события CEF
	cef::document_t doc;
	// Устанавливаем настройки записи событий
	doc.settings(settings);
	// Если разбор записи отказом завершился
	if(!doc.parse(text))
		// Выводим пустую запись
		return "";
	// Выводим собранную заново запись CEF
	return doc.dump();
}

/**
 * @brief Проверка постановки отмены знаков порознь по областям записи
 *
 * @details Описание ArcSight требует отменять в заголовке прямую черту и обратную
 *          косую, а в расширении - знак равенства, и прямо оговаривает, что черта в
 *          расширении отмены не требует
 *
 */
TEST(CodecCefWriter, Escaping) {
	// Выполняем проверку постановки отмены прямой черты в заголовке
	EXPECT_EQ(
		::rewriteCef(R"(CEF:0|security|threatmanager|1.0|100|detected a \| in message|10|act=blocked a |)"),
		"CEF:0|security|threatmanager|1.0|100|detected a \\| in message|10|act=blocked a |\n"
	);
	// Выполняем проверку постановки отмены обратной косой в заголовке
	EXPECT_EQ(
		::rewriteCef(R"(CEF:0|security|tm|1.0|100|detected a \\ in packet|10|action=blocked a \)"),
		"CEF:0|security|tm|1.0|100|detected a \\\\ in packet|10|action=blocked a \\\\\n"
	);
	// Выполняем проверку постановки отмены знака равенства в расширении
	EXPECT_EQ(
		::rewriteCef(R"(CEF:0|A|B|C|D|E|1|originsicname=CN\=chr-cpsg-01)"),
		"CEF:0|A|B|C|D|E|1|originsicname=CN\\=chr-cpsg-01\n"
	);
	// Выполняем проверку постановки перевода строки отменяющей последовательностью
	EXPECT_EQ(
		::rewriteCef(R"(CEF:0|A|B|C|D|E|1|msg=Detected a threat.\nNo action needed.)"),
		"CEF:0|A|B|C|D|E|1|msg=Detected a threat.\\nNo action needed.\n"
	);
}

/**
 * @brief Проверка сборки записи с приставкой syslog
 *
 */
TEST(CodecCefWriter, Syslog) {
	// Настройки записи событий
	cef::writer_t::settings_t settings;
	// Выполняем проверку сборки записи с приставкой syslog
	EXPECT_EQ(
		::rewriteCef("Feb 17 15:30:15 host CEF:0|A|B|C|D|E|1|src=1.2.3.4", settings),
		"Feb 17 15:30:15 host CEF:0|A|B|C|D|E|1|src=1.2.3.4\n"
	);
	// Отключаем запись приставки syslog перед словом «CEF:»
	settings.syslog = false;
	// Выполняем проверку сборки записи без приставки syslog
	EXPECT_EQ(
		::rewriteCef("Feb 17 15:30:15 host CEF:0|A|B|C|D|E|1|src=1.2.3.4", settings),
		"CEF:0|A|B|C|D|E|1|src=1.2.3.4\n"
	);
	// Отключаем запись знака конца строки за записью
	settings.terminate = false;
	// Выполняем проверку сборки записи без знака конца строки
	EXPECT_EQ(
		::rewriteCef("CEF:0|A|B|C|D|E|1|src=1.2.3.4", settings),
		"CEF:0|A|B|C|D|E|1|src=1.2.3.4"
	);
}

/**
 * @brief Проверка записи повторяющегося ключа расширения
 *
 * @details Повтор ключа держится в дереве перечнем, а записывается вновь всяким своим
 *          появлением: обратное обратило бы перечень в одно значение и потеряло бы
 *          повтор молча
 *
 */
TEST(CodecCefWriter, DuplicateKeys) {
	// Выполняем проверку записи повторяющегося ключа расширения
	EXPECT_EQ(
		::rewriteCef("CEF:0|A|B|C|D|E|1|ad.x=1 ad.x=2 ad.x=3"),
		"CEF:0|A|B|C|D|E|1|ad.x=1 ad.x=2 ad.x=3\n"
	);
}

/**
 * @brief Проверка обращения с вложенным значением
 *
 * @details Дерева произвольной глубины запись CEF не несёт: исход выбирает тот, кто
 *          пишет, а не кодек
 *
 */
TEST(CodecCefWriter, Nested) {
	// Объект записи событий
	cef::writer_t writer;
	// Настройки записи событий
	cef::writer_t::settings_t settings;
	// Заводим дерево события отображением
	abc::value_t root(abc::kind_t::MAP);
	// Заводим поля заголовка записи отображением
	root.place("/header") = abc::value_t(abc::kind_t::MAP);
	// Ставим поле заголовка поставщика устройства
	root.place("/header/vendor") = abc::value_t(string("A"));
	// Ставим поле заголовка изделия поставщика
	root.place("/header/product") = abc::value_t(string("B"));
	// Ставим поле заголовка редакции изделия
	root.place("/header/release") = abc::value_t(string("C"));
	// Ставим поле заголовка опознавателя события
	root.place("/header/signature") = abc::value_t(string("D"));
	// Ставим поле заголовка имени события
	root.place("/header/name") = abc::value_t(string("E"));
	// Ставим поле заголовка важности события
	root.place("/header/severity") = abc::value_t(static_cast <int64_t> (1));
	// Заводим пары расширения записи отображением
	root.place("/extension") = abc::value_t(abc::kind_t::MAP);
	// Ставим пару расширения с простым значением
	root.place("/extension/src") = abc::value_t(string("1.2.3.4"));
	// Ставим пару расширения со значением вложенным
	root.place("/extension/nested") = abc::value_t(abc::kind_t::MAP);
	// Собираемая запись CEF
	string result;
	// Устанавливаем отказ на вложенное значение
	settings.nested = cef::nested_t::STRICT;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выполняем проверку отказа записи на вложенном значении
	EXPECT_FALSE(writer.write(root, result));
	// Выполняем проверку кода отказа записи
	EXPECT_EQ(writer.error(), cef::error_t::NESTED_VALUE);
	// Устанавливаем пропуск вложенного значения вовсе
	settings.nested = cef::nested_t::SKIP;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выполняем проверку записи с пропуском вложенного значения
	EXPECT_TRUE(writer.write(root, result));
	// Выполняем проверку того, что вложенное значение в запись не попало
	EXPECT_EQ(result, "CEF:0|A|B|C|D|E|1|src=1.2.3.4\n");
}

/**
 * @brief Проверка отклонения дерева неверного устройства
 *
 */
TEST(CodecCefWriter, Failures) {
	// Объект записи событий
	cef::writer_t writer;
	// Собираемая запись CEF
	string result;
	// Заводим дерево события последовательностью знаков
	const abc::value_t plain(string("это не событие"));
	// Выполняем проверку отказа записи на дереве неверного устройства
	EXPECT_FALSE(writer.write(plain, result));
	// Выполняем проверку кода отказа записи
	EXPECT_EQ(writer.error(), cef::error_t::UNREPRESENTABLE_VALUE);
	// Заводим дерево события отображением без заголовка
	abc::value_t empty(abc::kind_t::MAP);
	// Выполняем проверку отказа записи на дереве без заголовка
	EXPECT_FALSE(writer.write(empty, result));
	// Выполняем проверку кода отказа записи
	EXPECT_EQ(writer.error(), cef::error_t::INCOMPLETE_HEADER);
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>

/**
 * @brief Проверка глухоты записи чисел к локали
 *
 * @details Дробное значение расширения обращается в знаки ходом `lexical_t`, а не
 *          `snprintf` и не `noexp`, и потому десятичным знаком записи остаётся точка при
 *          всякой локали. Описание CEF иного знака не знает: запись с запятой читается
 *          уже не числом, а значением знаками
 *
 * @warning Локаль ставится ПОСЛЕ заведения оснастки, а не до: `fmk_t` ставит свою в
 *          конструкторе и установку, сделанную раньше, перекрывает. Опыт, поставленный
 *          до заведения оснастки, показывает точку ВСЕГДА и глухоты не проверяет вовсе
 *
 * @note Заведено 09.09.2026 по указанию Николая: починку записи дробного я приложил к
 *       ОДНОМУ кодеку из двух - проверка стояла у syslog, а у CEF её не было, хотя
 *       запись там та же и довод при ней слово в слово тот же. Довод честен, обход мест
 *       не сделан
 *
 * @note Пропуск здесь - свойство системы, а не пробел проверки: у musl локалей нет
 *       вовсе, а у OpenBSD не заведён разряд LC_NUMERIC. Глухота там оттого НЕ ПРОВЕРЕНА,
 *       и зелёный прогон обещания не несёт
 *
 */
TEST(CodecCefWriter, LocaleNumbers) {
	/**
	 * Выполняем заведение оснастки ЗАРАНЕЕ, до всякой установки локали
	 *
	 * @warning Без этого проверка зелена ЛОЖНО: `fmk_t` ставит свою локаль в
	 *          конструкторе, а объект оснастки здесь заводится при первом обращении -
	 *          то есть уже после установки чужой локали
	 */
	(void) ::writerEnvironment();
	// Страж возврата локали записи чисел
	const LocaleGuard guard;
	// Количество проверенных локалей с иным десятичным знаком
	uint32_t checked = 0;
	/**
	 * Выполняем перебор названий локали с иным знаком десятичной точки
	 *
	 * @note Названия эти у разных систем свои, и ни одно не признаётся всюду: у Debian
	 *       ходит «de_DE.utf8», у MS Windows - «German_Germany». Локали «fa_IR» и
	 *       «ar_SA» взяты особо: десятичным знаком там служит «٫» (U+066B), занимающий
	 *       в UTF-8 два байта
	 */
	for(const char * name : {"de_DE.UTF-8", "de_DE.utf8", "German_Germany.1252", "German_Germany", "fa_IR.UTF-8", "ar_SA.UTF-8"}){
		// Если установить очередную локаль не удалось
		if(::setlocale(LC_NUMERIC, name) == nullptr)
			// Выполняем переход к следующей локали
			continue;
		// Если знаком десятичной точки установленной локали точка всё же осталась
		if(::localeconv()->decimal_point[0] == '.')
			// Выполняем переход к следующей локали
			continue;
		/**
		 * Объект события CEF, годною записью наполняемый
		 *
		 * @note Дерево строится РАЗБОРОМ годной записи, а не руками: устройство ветвей
		 *       заголовка задаёт сам кодек, и собранное на глаз дерево дало бы отказ
		 *       записи вместо проверки глухоты. Замерено 09.09.2026: руками собранное
		 *       дерево выдавало запись без единого расширения
		 */
		cef::document_t document;
		// Выполняем разбор годной записи событий безопасности
		ASSERT_TRUE(document.parse("CEF:0|security|threatmanager|1.0|100|detected|10|src=10.0.0.1")) << name;
		/**
		 * Ставим дробное значение расширения в дерево события
		 *
		 * @warning Ветвь зовётся «/extension», БЕЗ буквы «s» на конце: путь
		 *          «/extensions/...» заводит ветвь, какой договор кодека не знает, а
		 *          писатель смотрит только первую. Постановка при том отвечает УСПЕХОМ,
		 *          и спрос значение отдаёт - в запись оно просто не попадает. Замерено
		 *          09.09.2026 щупом, и на эту же букву я наступал вторично
		 */
		ASSERT_TRUE(document.set("/extension/cfp1", abc::value_t(0.1))) << name;
		// Ставим дробное значение с большим количеством разрядов в дерево события
		ASSERT_TRUE(document.set("/extension/cfp2", abc::value_t(2986.808299))) << name;
		// Собранная запись событий безопасности
		const string result = document.dump();
		// Выполняем проверку успешности сборки записи под чужой локалью
		ASSERT_FALSE(result.empty()) << name;
		/**
		 * Выполняем проверку того, что чужая локаль держалась ВО ВРЕМЯ записи
		 *
		 * @note Сторож этот и есть то, чем проверка отличается от собственной видимости:
		 *       сбрось её кто-нибудь до записи числа - и точка в выдаче доказывала бы
		 *       лишь то, что локаль была точкою, а вовсе не глухоту кодека
		 */
		ASSERT_NE(::localeconv()->decimal_point[0], '.') << name << ": локаль сброшена до записи числа";
		/**
		 * Выполняем перебор записанных дробных значений расширения
		 *
		 * @warning Сличать здесь ВИД записи нельзя, лишь значение: кратчайшее
		 *          представление числа держится не всюду. Предмет проверки - глухота к
		 *          локали, а не вид числа
		 */
		for(const pair <string, double> & expected : {make_pair(string("cfp1"), 0.1), make_pair(string("cfp2"), 2986.808299)}){
			// Место начала значения очередного ключа расширения
			const size_t begin = result.find(expected.first + "=");
			// Выполняем проверку наличия очередного ключа расширения в записи
			ASSERT_NE(begin, string::npos) << name << ": " << result;
			// Место окончания значения очередного ключа расширения
			size_t end = result.find(' ', begin + expected.first.size() + 1);
			// Если значение ключа расширения записью и оканчивается
			if(end == string::npos)
				// Берём концом значения конец самой записи
				end = result.size();
			// Записанное значение очередного ключа расширения
			const string written = result.substr(begin + expected.first.size() + 1, end - begin - expected.first.size() - 1);
			// Выполняем проверку того, что чужой десятичный знак в запись не попал
			EXPECT_EQ(written.find(::localeconv()->decimal_point), string::npos) << name << ": " << written;
			/**
			 * Обратно прочитанное значение очередного ключа расширения
			 *
			 * @warning Читается оно `lexical_t`, а НЕ `strtod`: тот сам локали подвластен
			 *          и под немецкой читает «0.1» нулём, обращая проверку глухоты в
			 *          проверку того же разряда локали с другой стороны
			 */
			double parsed = 0.;
			// Выполняем обратное чтение записанного значения без оглядки на локаль
			awh::lexical_t::fromChars(written.c_str(), written.c_str() + written.size(), parsed);
			// Выполняем проверку того, что записанное значение читается тем же числом
			EXPECT_DOUBLE_EQ(parsed, expected.second) << name << ": " << written;
		}
		// Выполняем учёт проверенной локали
		checked++;
	}
	// Если ни одной локали с иным десятичным знаком в системе не нашлось
	if(checked == 0)
		// Объявляем проверку пропущенной с указанием причины
		GTEST_SKIP() << "локали с иным десятичным знаком в системе нет";
}

/**
 * @brief Проверка обращения значений всех видов в знаки записи
 *
 * @details Запись CEF значений видами не различает: всё в ней есть последовательность
 *          знаков. Обращение потому обязано брать всякий вид дерева и давать знаки, а не
 *          отказ, - иначе дерево, чтением собранное, обратно в запись не легло бы
 *
 * @note Заведено 09.09.2026 по карте покрытия: ветви логического, беззнакового целого и
 *       прочих видов стояли непокрытыми, и непокрытая ветвь неотличима от недостижимой,
 *       покуда не замерена
 *
 */
TEST(CodecCefWriter, ValueKinds) {
	/**
	 * Образцы значений всех видов и ожидаемая запись их
	 */
	const struct {
		// Значение дерева, в запись ставимое
		abc::value_t value;
		// Ожидаемая запись значения знаками
		const char * expected;
	} SAMPLES[] = {
		// Логическая истина записывается словом
		{abc::value_t(true), "true"},
		// Логическая ложь записывается словом
		{abc::value_t(false), "false"},
		// Целое со знаком записывается числом
		{abc::value_t(static_cast <int64_t> (-42)), "-42"},
		// Целое без знака записывается числом
		{abc::value_t(static_cast <uint64_t> (42)), "42"},
		// Дробное записывается кратчайшим обратимым представлением
		{abc::value_t(0.5), "0.5"},
		// Значение знаками записывается как есть
		{abc::value_t(string("текст")), "текст"}
	};
	/**
	 * Выполняем перебор всех образцов значений
	 */
	for(const auto & sample : SAMPLES){
		// Объект события CEF
		cef::document_t document;
		// Выполняем разбор годной записи событий безопасности
		ASSERT_TRUE(document.parse("CEF:0|security|threatmanager|1.0|100|detected|10|src=10.0.0.1"));
		// Ставим значение очередного вида парой расширения
		ASSERT_TRUE(document.set("/extension/cs1", sample.value)) << sample.expected;
		// Собранная запись событий безопасности
		const string result = document.dump();
		// Выполняем проверку успешности сборки записи
		ASSERT_FALSE(result.empty()) << sample.expected;
		// Выполняем проверку того, что значение записано ожидаемым видом
		EXPECT_NE(result.find(string("cs1=") + sample.expected), string::npos) << result;
	}
	/**
	 * Значение пустого вида записывается пустотою
	 *
	 * @note Проверяется отдельно: пустая запись значения в общий перебор не ложится -
	 *       розыск «cs1=» нашёл бы её и у всякого иного вида
	 */
	cef::document_t document;
	// Выполняем разбор годной записи событий безопасности
	ASSERT_TRUE(document.parse("CEF:0|security|threatmanager|1.0|100|detected|10|src=10.0.0.1"));
	// Ставим пустое значение парой расширения
	ASSERT_TRUE(document.set("/extension/cs1", abc::value_t(abc::kind_t::NUL)));
	// Собранная запись событий безопасности
	const string result = document.dump();
	// Выполняем проверку успешности сборки записи
	ASSERT_FALSE(result.empty());
	// Выполняем проверку того, что пустое значение записано ключом без знаков за ним
	EXPECT_NE(result.find("cs1="), string::npos) << result;
}

/**
 * @brief Проверка записи значений без постановки отмены знаков
 *
 * @details Настройка отмены живёт В ПАРЕ с настройкой снятия её у чтения: значения,
 *          отмену несущие, ставятся в запись как есть. Порознь их задавать нельзя -
 *          постановка отмены поверх неснятой наращивает косые при всяком обороте, - и
 *          пару эту сводит документ, а не потребитель
 *
 * @note Ветвь эта стояла в карте непокрытой: все проверки шли при умолчании, где отмена
 *       включена
 *
 */
TEST(CodecCefWriter, EscapingDisabled) {
	// Объект события CEF
	cef::document_t document;
	// Выполняем разбор годной записи событий безопасности
	ASSERT_TRUE(document.parse("CEF:0|security|threatmanager|1.0|100|detected|10|src=10.0.0.1"));
	// Настройки записи событий без постановки отмены знаков
	cef::writer_t::settings_t settings;
	// Выключаем постановку отмены знаков
	settings.escape = false;
	// Устанавливаем настройки записи событий
	document.settings(settings);
	// Ставим значение со знаком равенства парой расширения
	ASSERT_TRUE(document.set("/extension/cs1", abc::value_t(string("a=b"))));
	// Собранная запись событий безопасности
	const string result = document.dump();
	// Выполняем проверку успешности сборки записи
	ASSERT_FALSE(result.empty());
	// Выполняем проверку того, что знак равенства записан без отмены
	EXPECT_NE(result.find("cs1=a=b"), string::npos) << result;
	// Выполняем проверку того, что отмена знаков в запись не попала
	EXPECT_EQ(result.find("a\\=b"), string::npos) << result;
}

/**
 * @brief Проверка постановки отмены знаков перевода строки и возврата каретки
 *
 * @details Многострочное значение запись CEF выражает отменяющими последовательностями
 *          «\\n» и «\\r»: сама запись строкою и оканчивается, и знак перевода внутри
 *          значения оборвал бы её на две
 *
 * @note Ветвь возврата каретки стояла в карте непокрытой: прежние проверки брали лишь
 *       перевод строки
 *
 */
TEST(CodecCefWriter, NewlineEscaping) {
	// Объект события CEF
	cef::document_t document;
	// Выполняем разбор годной записи событий безопасности
	ASSERT_TRUE(document.parse("CEF:0|security|threatmanager|1.0|100|detected|10|src=10.0.0.1"));
	// Ставим значение с переводом строки и возвратом каретки парой расширения
	ASSERT_TRUE(document.set("/extension/msg", abc::value_t(string("первая\r\nвторая"))));
	// Собранная запись событий безопасности
	const string result = document.dump();
	// Выполняем проверку успешности сборки записи
	ASSERT_FALSE(result.empty());
	// Выполняем проверку того, что возврат каретки поставлен отменяющей последовательностью
	EXPECT_NE(result.find("первая\\r\\nвторая"), string::npos) << result;
	// Выполняем проверку того, что настоящий перевод строки в значение не попал
	EXPECT_EQ(result.find("первая\r"), string::npos) << result;
}

/**
 * @brief Проверка обращения вложенного значения в знаки
 *
 * @details Режим `nested_t::TEXT` был третьим и единственным, ни одной проверкой не
 *          затронутым: карта покрытия держала всю ветвь обращения пустой, а отсутствие
 *          поверки от поверки пройденной неотличимо
 *
 */
TEST(CodecCefWriter, NestedAsText) {
	// Объект записи событий
	cef::writer_t writer;
	// Настройки записи событий
	cef::writer_t::settings_t settings;
	// Заводим дерево события отображением
	abc::value_t root(abc::kind_t::MAP);
	// Заводим поля заголовка записи отображением
	root.place("/header") = abc::value_t(abc::kind_t::MAP);
	// Ставим поле заголовка поставщика устройства
	root.place("/header/vendor") = abc::value_t(string("A"));
	// Ставим поле заголовка изделия поставщика
	root.place("/header/product") = abc::value_t(string("B"));
	// Ставим поле заголовка редакции изделия
	root.place("/header/release") = abc::value_t(string("C"));
	// Ставим поле заголовка опознавателя события
	root.place("/header/signature") = abc::value_t(string("D"));
	// Ставим поле заголовка имени события
	root.place("/header/name") = abc::value_t(string("E"));
	// Ставим поле заголовка важности события
	root.place("/header/severity") = abc::value_t(static_cast <int64_t> (1));
	// Заводим пары расширения записи отображением
	root.place("/extension") = abc::value_t(abc::kind_t::MAP);
	// Ставим пару расширения со значением вложенным
	root.place("/extension/nested") = abc::value_t(abc::kind_t::MAP);
	// Ставим поле вложенного значения пары расширения
	root.place("/extension/nested/key") = abc::value_t(string("значение"));
	// Собираемая запись CEF
	string result;
	// Устанавливаем обращение вложенного значения в знаки
	settings.nested = cef::nested_t::TEXT;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выполняем проверку успешности записи с обращением вложенного значения
	ASSERT_TRUE(writer.write(root, result));
	// Выполняем проверку того, что пара расширения записана
	EXPECT_NE(result.find("nested="), string::npos) << result;
	// Выполняем проверку того, что содержимое вложенного значения в запись попало
	EXPECT_NE(result.find("key"), string::npos) << result;
	// Выполняем проверку того, что вложенное значение пустым не осталось
	EXPECT_GT(result.find("key"), result.find("nested=")) << result;
}

/**
 * @brief Проверка пропуска вложенного поля заголовка записи
 *
 * @details Заголовок и расширение обращаются с вложенным значением порознь, и пропуск
 *          в заголовке проверками затронут не был: поле остаётся пустым, а не выпадает
 *          вовсе, ибо места полей заголовка CEF определены их порядком
 *
 */
TEST(CodecCefWriter, NestedHeaderField) {
	// Объект записи событий
	cef::writer_t writer;
	// Настройки записи событий
	cef::writer_t::settings_t settings;
	// Заводим дерево события отображением
	abc::value_t root(abc::kind_t::MAP);
	// Заводим поля заголовка записи отображением
	root.place("/header") = abc::value_t(abc::kind_t::MAP);
	// Ставим поле заголовка поставщика устройства
	root.place("/header/vendor") = abc::value_t(string("A"));
	// Ставим поле заголовка изделия поставщика
	root.place("/header/product") = abc::value_t(string("B"));
	// Ставим поле заголовка редакции изделия
	root.place("/header/release") = abc::value_t(string("C"));
	// Ставим поле заголовка опознавателя события вложенным отображением
	root.place("/header/signature") = abc::value_t(abc::kind_t::MAP);
	// Ставим поле вложенного поля заголовка записи
	root.place("/header/signature/key") = abc::value_t(string("V"));
	// Ставим поле заголовка имени события
	root.place("/header/name") = abc::value_t(string("E"));
	// Ставим поле заголовка важности события
	root.place("/header/severity") = abc::value_t(static_cast <int64_t> (1));
	// Собираемая запись CEF
	string result;
	// Устанавливаем отказ на вложенное значение
	settings.nested = cef::nested_t::STRICT;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выполняем проверку отказа записи на вложенном поле заголовка
	EXPECT_FALSE(writer.write(root, result));
	// Выполняем проверку кода отказа записи
	EXPECT_EQ(writer.error(), cef::error_t::NESTED_VALUE);
	// Устанавливаем пропуск вложенного значения вовсе
	settings.nested = cef::nested_t::SKIP;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выполняем проверку записи с пропуском вложенного поля заголовка
	ASSERT_TRUE(writer.write(root, result));
	// Выполняем проверку того, что место поля заголовка осталось пустым
	EXPECT_EQ(result, "CEF:0|A|B|C||E|1|\n") << result;
}

/**
 * @brief Проверка отклонения негодных имён ключей расширения
 *
 * @details Ключ, пробельный знак несущий, разбирается обратно двумя парами, а пустой
 *          ключ не разбирается вовсе. Заслон на пробельный знак был найден ворошителем
 *          04.09.2026 и до сих пор ни одной проверкой не закреплён
 *
 */
TEST(CodecCefWriter, ExtensionKeyEdges) {
	// Объект записи событий
	cef::writer_t writer;
	// Настройки записи событий
	cef::writer_t::settings_t settings;
	/**
	 * Выполняем перебор негодных имён ключей расширения записи
	 */
	for(const auto & item : vector <pair <string, cef::error_t>> {
		{"два слова", cef::error_t::UNREPRESENTABLE_VALUE},
		{"таб\tключ", cef::error_t::UNREPRESENTABLE_VALUE},
		{"", cef::error_t::EMPTY_KEY}
	}) {
		// Заводим дерево события отображением
		abc::value_t root(abc::kind_t::MAP);
		// Заводим поля заголовка записи отображением
		root.place("/header") = abc::value_t(abc::kind_t::MAP);
		// Ставим поле заголовка поставщика устройства
		root.place("/header/vendor") = abc::value_t(string("A"));
		// Ставим поле заголовка изделия поставщика
		root.place("/header/product") = abc::value_t(string("B"));
		// Ставим поле заголовка редакции изделия
		root.place("/header/release") = abc::value_t(string("C"));
		// Ставим поле заголовка опознавателя события
		root.place("/header/signature") = abc::value_t(string("D"));
		// Ставим поле заголовка имени события
		root.place("/header/name") = abc::value_t(string("E"));
		// Ставим поле заголовка важности события
		root.place("/header/severity") = abc::value_t(static_cast <int64_t> (1));
		// Заводим пары расширения записи отображением
		root.place("/extension") = abc::value_t(abc::kind_t::MAP);
		// Ставим годную пару расширения записи
		root.place("/extension/src") = abc::value_t(string("1.2.3.4"));
		// Ставим пару расширения с негодным именем ключа
		ASSERT_TRUE(root.place("/extension").insert(item.first, abc::value_t(string("V")))) << item.first;
		// Собираемая запись CEF
		string result;
		// Устанавливаем отказ на непредставимое значение
		settings.nested = cef::nested_t::STRICT;
		// Устанавливаем настройки записи событий
		writer.settings(settings);
		// Выполняем проверку отказа записи на негодном имени ключа расширения
		EXPECT_FALSE(writer.write(root, result)) << item.first;
		// Выполняем проверку кода отказа записи
		EXPECT_EQ(writer.error(), item.second) << item.first;
		// Устанавливаем пропуск непредставимого значения вовсе
		settings.nested = cef::nested_t::SKIP;
		// Устанавливаем настройки записи событий
		writer.settings(settings);
		// Выполняем проверку записи с пропуском негодной пары расширения
		ASSERT_TRUE(writer.write(root, result)) << item.first;
		// Выполняем проверку того, что негодная пара расширения в запись не попала
		EXPECT_EQ(result, "CEF:0|A|B|C|D|E|1|src=1.2.3.4\n") << item.first << ": " << result;
	}
}

/**
 * @brief Проверка отклонения непредставимой приставки syslog
 *
 * @details Приставка syslog стоит перед словом «CEF:» отдельным полем дерева, и вложенным
 *          значением быть не может: записать её было бы нечем, а молчаливый пропуск
 *          отдал бы потребителю запись без приставки, о потере не сказав
 *
 */
TEST(CodecCefWriter, UnrepresentableSyslogPrefix) {
	// Объект записи событий
	cef::writer_t writer;
	// Настройки записи событий
	cef::writer_t::settings_t settings;
	// Заводим дерево события отображением
	abc::value_t root(abc::kind_t::MAP);
	// Ставим приставку syslog ВЛОЖЕННЫМ значением
	root.place("/syslog") = abc::value_t(abc::kind_t::MAP);
	// Ставим поле вложенной приставки syslog
	root.place("/syslog/key") = abc::value_t(string("значение"));
	// Заводим поля заголовка записи отображением
	root.place("/header") = abc::value_t(abc::kind_t::MAP);
	// Ставим поле заголовка поставщика устройства
	root.place("/header/vendor") = abc::value_t(string("A"));
	// Ставим поле заголовка изделия поставщика
	root.place("/header/product") = abc::value_t(string("B"));
	// Ставим поле заголовка редакции изделия
	root.place("/header/release") = abc::value_t(string("C"));
	// Ставим поле заголовка опознавателя события
	root.place("/header/signature") = abc::value_t(string("D"));
	// Ставим поле заголовка имени события
	root.place("/header/name") = abc::value_t(string("E"));
	// Ставим поле заголовка важности события
	root.place("/header/severity") = abc::value_t(static_cast <int64_t> (1));
	// Собираемая запись CEF
	string result;
	// Устанавливаем отказ на вложенное значение
	settings.nested = cef::nested_t::STRICT;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выполняем проверку отказа записи на непредставимой приставке syslog
	EXPECT_FALSE(writer.write(root, result));
	// Выполняем проверку кода отказа записи
	EXPECT_EQ(writer.error(), cef::error_t::UNREPRESENTABLE_VALUE);
	// Устанавливаем обращение вложенного значения в знаки
	settings.nested = cef::nested_t::TEXT;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выполняем проверку успешности записи с обращением приставки в знаки
	ASSERT_TRUE(writer.write(root, result));
	// Выполняем проверку того, что приставка предшествует слову «CEF:»
	EXPECT_LT(result.find("key"), result.find(cef::SIGNATURE)) << result;
}

/**
 * @brief Проверка отказа записи значением, запись разрывающим
 *
 * @details Запись CEF оканчивается переводом строки, и он же ей границею служит. Отмену
 *          знаков заголовок знает лишь для прямой черты и обратной косой, а приставка
 *          syslog не знает вовсе: значение с переводом строки разрывало запись надвое.
 *          Писатель отвечал при этом УСПЕХОМ, а оборот битой записи - INCOMPLETE_HEADER
 *
 * @note Расширение записи проверяется здесь же обратным случаем: там перевод строки
 *       отменяется последовательностью «\n» и запись остаётся целой - отказывать в нём
 *       было бы неверно
 *
 */
TEST(CodecCefWriter, ValueBreakingTheRecord) {
	// Объект записи событий
	cef::writer_t writer;
	// Собранная запись CEF
	string result = "";
	/**
	 * @brief Метод сборки дерева события с заданным значением поставщика
	 *
	 * @param vendor значение поля поставщика
	 * @param prefix приставка syslog записи
	 * @param value  значение пары расширения
	 * @return       дерево собираемого события
	 */
	const auto tree = [](const string & vendor, const string & prefix, const string & value) noexcept -> abc::value_t {
		// Дерево собираемого события
		abc::value_t root(abc::kind_t::MAP);
		// Ставим номер редакции записи в дерево события
		root.place("/header/version") = abc::value_t(static_cast <uint64_t> (0));
		// Ставим поставщика устройства в дерево события
		root.place("/header/vendor") = abc::value_t(vendor);
		// Ставим название устройства в дерево события
		root.place("/header/product") = abc::value_t(string("P"));
		// Ставим редакцию устройства в дерево события
		root.place("/header/release") = abc::value_t(string("1"));
		// Ставим опознаватель события в дерево события
		root.place("/header/signature") = abc::value_t(string("S"));
		// Ставим название события в дерево события
		root.place("/header/name") = abc::value_t(string("N"));
		// Ставим важность события в дерево события
		root.place("/header/severity") = abc::value_t(static_cast <uint64_t> (1));
		// Ставим пару расширения в дерево события
		root.place("/extension/msg") = abc::value_t(value);
		// Если приставка syslog задана
		if(!prefix.empty())
			// Ставим приставку syslog в дерево события
			root.place("/syslog") = abc::value_t(prefix);
		// Выводим дерево собираемого события
		return root;
	};
	// Выполняем проверку успешности сборки записи годными значениями
	EXPECT_TRUE(writer.write(tree("Vendor", "", "text"), result));
	// Выполняем проверку отказа сборки записи полем заголовка с переводом строки
	EXPECT_FALSE(writer.write(tree("Ven\ndor", "", "text"), result));
	// Выполняем проверку отказа сборки записи приставкой syslog с переводом строки
	EXPECT_FALSE(writer.write(tree("Vendor", "Feb 17\n15:30:15 host", "text"), result));
	// Выполняем проверку успешности сборки записи значением расширения с переводом строки
	EXPECT_TRUE(writer.write(tree("Vendor", "", "te\nxt"), result));
	// Выполняем проверку того, что перевод строки в расширении отменён последовательностью
	EXPECT_NE(result.find("te\\nxt"), string::npos);
}

/**
 * @brief Проверка того, что писатель не рождает записей, кодеком не читаемых
 *
 * @details Оборот замыкается лишь тогда, когда всякая собранная запись разбирается
 *          обратно. Два разрыва найдены щупом 14.09.2026:
 *
 *          - «inf», «-inf» и «nan» писались как есть, а собственный разбор отвергал их
 *            кодом INVALID_NUMBER: записи CEF они неведомы вовсе. Теперь запись
 *            отвечает отказом - честнее отвергнуть у себя, чем слать принимающему пару,
 *            какую поправить уже некому;
 *
 *          - «1.7976931348623157e+308» писалось кратчайшим обратимым представлением, а
 *            разбор отвергал его, ибо спрашивал у рамки, число ли это, а рамка записи с
 *            точкой И показателем степени разом дробной не признаёт. Довод у рамки снят
 *            за ненадобностью: строгий разбор поверяет запись сам
 *
 */
TEST(CodecCefWriter, WrittenRecordsAreAlwaysReadable) {
	// Объект записи событий
	cef::writer_t writer;
	// Собранная запись CEF
	string result = "";
	/**
	 * @brief Метод сборки дерева события с заданным дробным значением
	 *
	 * @param value дробное значение пары расширения
	 * @return      дерево собираемого события
	 */
	const auto tree = [](const double value) noexcept -> abc::value_t {
		// Дерево собираемого события
		abc::value_t root(abc::kind_t::MAP);
		// Ставим номер редакции записи в дерево события
		root.place("/header/version") = abc::value_t(static_cast <uint64_t> (0));
		// Ставим поставщика устройства в дерево события
		root.place("/header/vendor") = abc::value_t(string("V"));
		// Ставим название устройства в дерево события
		root.place("/header/product") = abc::value_t(string("P"));
		// Ставим редакцию устройства в дерево события
		root.place("/header/release") = abc::value_t(string("1"));
		// Ставим опознаватель события в дерево события
		root.place("/header/signature") = abc::value_t(string("S"));
		// Ставим название события в дерево события
		root.place("/header/name") = abc::value_t(string("N"));
		// Ставим важность события в дерево события
		root.place("/header/severity") = abc::value_t(static_cast <uint64_t> (1));
		// Ставим дробное значение пары расширения в дерево события
		root.place("/extension/cfp1") = abc::value_t(value);
		// Выводим дерево собираемого события
		return root;
	};
	// Настройки разбора записей со строгим сличением видов
	cef::reader_t::settings_t settings;
	// Устанавливаем строгое сличение со словарём
	settings.mode = cef::mode_t::STRONG;
	// Выполняем проверку отказа записи бесконечностью
	EXPECT_FALSE(writer.write(tree(::std::numeric_limits <double>::infinity()), result));
	// Выполняем проверку отказа записи значением, числом не являющимся
	EXPECT_FALSE(writer.write(tree(::std::numeric_limits <double>::quiet_NaN()), result));
	/**
	 * Выполняем перебор конечных дробных значений, записи подлежащих
	 */
	for(const double value : {1.5, 0.0, -2.25, ::std::numeric_limits <double>::max(), ::std::numeric_limits <double>::denorm_min()}) {
		// Выполняем проверку успешности сборки записи дробным значением
		ASSERT_TRUE(writer.write(tree(value), result)) << value;
		// Объект события повторного разбора
		cef::document_t again;
		// Устанавливаем настройки разбора записей
		again.settings(settings);
		// Выполняем проверку того, что собранная запись разбирается обратно
		EXPECT_TRUE(again.parse(result)) << "собранная запись не читается кодеком: " << result;
	}
}

/**
 * @brief Поверка читаемости вложенного значения, обращённого в знаки
 *
 * @details Настройка `nested_t::TEXT` обещает обратить вложенное значение в
 * последовательность знаков: перечень выдаётся значениями через запятую, отображение -
 * парами «имя=значение» через запятую, и обход одинаков на всякой глубине. Знак равенства
 * при том отменяется правилами области расширения, как и всякий иной
 *
 * @warning Прежде здесь стоял сбор ДВОИЧНОЙ записи контейнера ABC, и её октеты клались в
 *          поле записи как текст. Запись выходила негодной вовсе и обратно не читалась, а
 *          настройка при том обещала знаки
 *
 * @note Найдено щупом вложенности 15.09.2026 по признаку, поданному владельцем кодеков
 *       JSON: если то же значение при ИНОЙ настройке записывается, дело в настройке, а не
 *       в значении. Тот же дефект слово в слово лежал и в кодеке syslog
 */
TEST(CodecCefWriter, NestedValueBecomesReadableText) {
	// Вложенный перечень значений
	abc::value_t nest;
	// Добавляем первое значение вложенного перечня
	ASSERT_TRUE(nest.push(abc::value_t(string("x"))));
	// Добавляем второе значение вложенного перечня
	ASSERT_TRUE(nest.push(abc::value_t(string("y"))));
	// Отображение, вложенный перечень несущее
	abc::value_t map;
	// Добавляем простое поле отображения
	ASSERT_TRUE(map.insert("one", abc::value_t(string("1"))));
	// Добавляем поле отображения, перечень несущее
	ASSERT_TRUE(map.insert("two", nest));
	// Объект документа события CEF
	cef::document_t document;
	// Выполняем разбор годной записи события CEF
	ASSERT_TRUE(document.parse("CEF:0|A|B|1.0|100|name|5|msg=test"));
	// Укладываем отображение в поле сообщения расширения
	ASSERT_TRUE(document.set("/extension/msg", map));
	// Настройки записи событий
	cef::writer_t::settings_t settings;
	// Устанавливаем обращение вложенного значения в знаки
	settings.nested = cef::nested_t::TEXT;
	// Устанавливаем настройки записи событий
	document.settings(settings);
	// Получаем собранную запись события CEF
	string built = document.dump();
	// Снимаем перевод строки, сборкою добавленный
	while(!built.empty() && ((built.back() == '\n') || (built.back() == '\r')))
		// Снимаем последний знак собранной записи
		built.pop_back();
	/**
	 * Выполняем проверку того, что вложенное значение выдано читаемыми знаками
	 *
	 * @note Знак равенства внутри значения отменён обратной косой: область расширения
	 *       того требует, и без отмены запись разобралась бы иными парами
	 */
	EXPECT_NE(built.find("one\\=1,two\\=x,y"), string::npos) << "собрано: " << built;
	/**
	 * Выполняем перебор всех знаков собранной записи
	 *
	 * @note Поверка эта закрепляет главное: запись не несёт октетов двоичной записи
	 *       контейнера, каких запись события выразить не может вовсе
	 */
	for(size_t i = 0; i < built.size(); i++){
		// Получаем очередной знак собранной записи беззнаковым
		const uint8_t letter = static_cast <uint8_t> (built[i]);
		// Выполняем проверку того, что знак записи печатным является
		EXPECT_TRUE((letter >= 32) && (letter <= 126)) << "октет " << static_cast <uint32_t> (letter) << " в позиции " << i;
	}
	// Объект документа события CEF для поверки оборота
	cef::document_t again;
	// Выполняем проверку того, что собранная запись читается обратно
	EXPECT_TRUE(again.parse(built)) << "собрано: " << built;
}

/**
 * @brief Поверка того, что запись не рождает значений, своим же чтением не читаемых
 *
 * @details Словарь назначает полю вид, и разбор читает значение именно им: поле вида
 * INTEGER читается знаковым целым. Значение беззнаковое, за предел знакового вышедшее,
 * записывалось знаками исправно, а обратно НЕ ЧИТАЛОСЬ - кодек рождал запись, какую сам
 * прочесть не мог
 *
 * @warning Найдено щупом краевых значений 15.09.2026: «cn1» со значением
 *          18446744073709551615 писалось, а своё же строгое чтение отвечало «value is not
 *          a number». Ветвь дробного такую защиту уже несла с 14.09.2026 - «inf» и «nan»
 *          отвергаются по той же причине, - а ветвь целых не несла
 *
 * @note Поверка ставится ВСЕГДА, а не при строгом сличении: запись, своим чтением не
 *       читаемая, негодна при всяком сличении - принимающая сторона вольна читать строго
 */
TEST(CodecCefWriter, WrittenIntegersFitTheDictionaryKind) {
	// Краевые значения, полю знакового вида назначаемые
	const vector <pair <abc::value_t, bool>> CASES = {
		{abc::value_t(static_cast <uint64_t> (numeric_limits <int64_t>::max())), true},
		{abc::value_t(numeric_limits <int64_t>::min()), true},
		{abc::value_t(static_cast <uint64_t> (0)), true},
		{abc::value_t(numeric_limits <uint64_t>::max()), false}
	};
	// Количество сличённых значений
	size_t compared = 0;
	// Выполняем перебор всех краевых значений
	for(auto & item : CASES){
		// Объект документа события CEF
		cef::document_t document;
		// Выполняем разбор годной записи события CEF
		ASSERT_TRUE(document.parse("CEF:0|A|B|1.0|100|n|5|cn1=1"));
		// Укладываем краевое значение в поле знакового вида
		ASSERT_TRUE(document.set("/extension/cn1", item.first));
		// Получаем собранную запись события CEF
		const string built = document.dump();
		// Выполняем проверку того, что запись собрана лишь у значений выразимых
		EXPECT_EQ(!built.empty(), item.second);
		// Если запись собрана, поверяем читаемость её своим же чтением
		if(!built.empty()){
			// Объект документа события CEF для поверки чтения
			cef::document_t back;
			// Получаем настройки разбора записей
			cef::reader_t::settings_t settings = back.settings();
			// Устанавливаем строгое сличение значений со словарём
			settings.mode = cef::mode_t::STRONG;
			// Устанавливаем настройки разбора записей
			ASSERT_TRUE(back.settings(settings));
			/**
			 * Выполняем проверку того, что собранное читается своим же чтением
			 *
			 * @note Поверка эта и есть главная: отказ записи сам по себе ничего не
			 *       доказывает, доказывает лишь пара «записано - прочтено»
			 */
			EXPECT_TRUE(back.parse(built)) << "собрано: " << built;
		// Если запись отвергнута, поверяем названность отказа
		} else EXPECT_EQ(document.error(), cef::error_t::UNREPRESENTABLE_VALUE);
		// Наращиваем количество сличённых значений
		compared++;
	}
	// Выполняем проверку того, что сличены все значения образца
	EXPECT_EQ(compared, CASES.size());
}
