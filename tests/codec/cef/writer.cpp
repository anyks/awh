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
		// Объект фреймворка проверок
		awh::fmk_t fmk;
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		EnvCefWriter() noexcept : log(&this->fmk) {
			// Выполняем отключение вывода логов
			this->log.mode({});
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
	cef::document_t doc(&::writerEnvironment().fmk, &::writerEnvironment().log);
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
	cef::writer_t writer(&::writerEnvironment().fmk, &::writerEnvironment().log);
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
	cef::writer_t writer(&::writerEnvironment().fmk, &::writerEnvironment().log);
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
		cef::document_t document(&::writerEnvironment().fmk, &::writerEnvironment().log);
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
