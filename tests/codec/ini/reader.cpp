/**
 * @file reader.cpp
 * @date 2026-08-09
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
 * @brief Автоматические тесты потокового чтения текста настроек INI — наречия записи,
 *        примечания и их расположение, продолжения строк, управляющие последовательности,
 *        отклонение неправильного построения, пределы разбора и подача текста кусками
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/ini/ini.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Снимаем на время реализации макросы, чьи имена заняты
 * членами перечислений AWH (возвращает их macro_pop.hpp в конце файла)
 */
#include <sys/macro_push.hpp>
#include <sys/log.hpp>

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
		 * @brief Функция получения объекта фреймворка проверок
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          проверки, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка проверок
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка проверок
			static awh::fmk_t fmk;
			// Выводим объект фреймворка проверок
			return fmk;
		}
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта журнала проверок
	 *
	 * @return объект журнала проверок
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект журнала проверок
		static Silent silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Метод разбора текста настроек в слепок потока событий
 *
 * @details Слепок собирается строкой ради сличения целиком: сравнение потока событий
 * знак в знак ловит и лишнее событие, и его недостачу, чего проверка отдельных полей
 * не даёт
 *
 * @param text     разбираемый текст настроек
 * @param settings настройки разбора текста настроек
 * @param step     размер куска подаваемого текста, нулевой для подачи целиком
 * @return         слепок потока событий разбора
 *
 */
static string dump(const string & text, const ini::reader_t::settings_t & settings, const size_t step = 0) noexcept {
	// Объект потокового чтения текста настроек
	ini::reader_t reader(::logger(), settings);
	// Собираемый слепок потока событий разбора
	string result;
	/**
	 * @brief Метод вычитывания накопленных событий разбора
	 *
	 */
	auto drain = [&reader, &result]() noexcept -> void {
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Определяем вид текущего события разбора
			 */
			switch(static_cast <uint8_t> (reader.event())){
				// Если событием является объявление раздела
				case static_cast <uint8_t> (ini::event_t::SECTION):
					// Выполняем добавление объявления раздела к слепку
					result.append("S[").append(reader.section().section).append("|").append(reader.section().subsection).append("]\n");
				break;
				// Если событием является свойство со значением
				case static_cast <uint8_t> (ini::event_t::PROPERTY): {
					// Выполняем добавление свойства к слепку
					result.append("P[").append(reader.key()).append("=").append(reader.text()).append("]");
					/**
					 * Если свойство записано без разделителя и значения
					 */
					if(reader.property().valueless)
						// Выполняем добавление признака свойства к слепку
						result.append("{novalue}");
					/**
					 * Если значение свойства было заключено в кавычки
					 */
					if(reader.property().quoted)
						// Выполняем добавление признака свойства к слепку
						result.append("{quoted}");
					/**
					 * Если свойство записано добавлением к перечню значений
					 */
					if(reader.property().append)
						// Выполняем добавление признака свойства к слепку
						result.append("{append}");
					// Выполняем добавление знака конца записи к слепку
					result.append("\n");
				} break;
				// Если событием является примечание
				case static_cast <uint8_t> (ini::event_t::COMMENT):
					// Выполняем добавление примечания к слепку
					result.append("C[").append(reader.text()).append("]").append(to_string(static_cast <uint8_t> (reader.comment().placement))).append("\n");
				break;
				// Если событием является пустая строка
				case static_cast <uint8_t> (ini::event_t::BLANK):
					// Выполняем добавление пустой строки к слепку
					result.append("B\n");
				break;
			}
		}
	};
	/**
	 * Если текст настроек подаётся целиком
	 */
	if(step == 0){
		// Выполняем передачу текста настроек целиком
		reader.feed(text);
		// Выполняем вычитывание накопленных событий разбора
		drain();
	/**
	 * Если текст настроек подаётся кусками
	 */
	} else {
		/**
		 * Выполняем перебор всех кусков текста настроек
		 */
		for(size_t i = 0; (i < text.size()) || (i == 0); i += step){
			// Получаем размер очередного куска текста настроек
			const size_t size = ((i + step) > text.size() ? (text.size() - i) : step);
			/**
			 * Если передачу очередного куска выполнить не удалось
			 */
			if(!reader.feed(text.data() + i, size, ((i + size) >= text.size())))
				// Выполняем прекращение подачи кусков текста настроек
				break;
			// Выполняем вычитывание накопленных событий разбора
			drain();
			/**
			 * Если текст настроек исчерпан
			 */
			if((i + size) >= text.size())
				// Выполняем прекращение подачи кусков текста настроек
				break;
		}
	}
	/**
	 * Если разбор прекращён ошибкой
	 */
	if(reader.state() == ini::state_t::FAILED)
		// Выполняем добавление сведений об ошибке к слепку
		result.append("E[").append(ini::message(reader.error())).append("]@")
		      .append(to_string(reader.errorLocation().line)).append(":")
		      .append(to_string(reader.errorLocation().column)).append("\n");
	// Выводим собранный слепок потока событий разбора
	return result;
}

/**
 * @brief Проверка разбора наречия, принятого по умолчанию
 *
 */
TEST(CodecIniReader, Default) {
	// Разбираемый текст настроек
	const string text = "; шапка\n[main]\nkey = value\nother=  spaced  \n\n# решётка\n";
	// Выполняем проверку слепка потока событий разбора
	ASSERT_EQ(::dump(text, ini::reader_t::settings_t()), "C[шапка]0\nS[main|]\nP[key=value]\nP[other=spaced]\nC[решётка]0\n");
}
/**
 * @brief Проверка совпадения потока событий при подаче текста кусками
 *
 * @details Слепок разбора обязан от нарезки исходного текста не зависеть: разрыв куска
 * допустим в любом месте, и поток событий обязан совпасть со слепком разбора текста
 * целиком знак в знак
 *
 */
TEST(CodecIniReader, Chunked) {
	// Перечень разбираемых текстов настроек
	const string texts[] = {
		"; шапка\n[main]\nkey = value\n\n# решётка\n",
		"[remote \"origin\"]\n\turl = git@host:repo.git ; примечание\n\tbare\n",
		"[srv]\nhosts:\n  first\n  second\nport = 80\n",
		"[Service]\nExecStart=/bin/sh \\\n  -c true\nEnvironment=A=1\\sB=2\n",
		"[a]\r\nk=v\r\n[b]\rj=w\r"
	};
	// Перечень наборов настроек разбора
	const ini::reader_t::settings_t settings[] = {
		ini::reader_t::settings_t(),
		ini::reader_t::settings_t::git(),
		ini::reader_t::settings_t::python(),
		ini::reader_t::settings_t::systemd(),
		ini::reader_t::settings_t()
	};
	/**
	 * Выполняем перебор всех разбираемых текстов настроек
	 */
	for(size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); i++){
		// Получаем слепок разбора текста настроек целиком
		const string expected = ::dump(texts[i], settings[i]);
		/**
		 * Выполняем перебор размеров куска подаваемого текста
		 */
		for(size_t step = 1; step < 8; step++)
			// Выполняем проверку совпадения слепка потока событий
			ASSERT_EQ(::dump(texts[i], settings[i], step), expected) << "text " << i << " step " << step;
	}
}
/**
 * @brief Проверка разбора наречия настроек Git
 *
 */
TEST(CodecIniReader, Git) {
	// Разбираемый текст настроек
	const string text = "[remote \"origin\"]\n\turl = git@host:repo.git ; примечание\n\tbare\n[core]\n\tpath = \"a;b\"\n";
	// Выполняем проверку слепка потока событий разбора
	ASSERT_EQ(::dump(text, ini::reader_t::settings_t::git()),
		"S[remote|origin]\nP[url=git@host:repo.git]\nC[примечание]1\nP[bare=]{novalue}\nS[core|]\nP[path=a;b]{quoted}\n");
}
/**
 * @brief Проверка разбора наречия MS Windows
 *
 * @details Точка с запятой внутри значения этим наречием примечания не начинает, а
 * кавычки частью значения остаются
 *
 */
TEST(CodecIniReader, Windows) {
	// Разбираемый текст настроек
	const string text = "[paths]\nPATH=c:\\bin;c:\\sbin\nquoted=\"value\"\n";
	// Выполняем проверку слепка потока событий разбора
	ASSERT_EQ(::dump(text, ini::reader_t::settings_t::windows()), "S[paths|]\nP[PATH=c:\\bin;c:\\sbin]\nP[quoted=\"value\"]\n");
}
/**
 * @brief Проверка разбора наречия configparser языка Python
 *
 */
TEST(CodecIniReader, Python) {
	// Разбираемый текст настроек
	const string text = "[srv]\nhosts:\n  first\n  second\nport = 80\n";
	// Выполняем проверку слепка потока событий разбора
	ASSERT_EQ(::dump(text, ini::reader_t::settings_t::python()), "S[srv|]\nP[hosts=\nfirst\nsecond]\nP[port=80]\n");
}
/**
 * @brief Проверка разбора наречия описания служб systemd
 *
 */
TEST(CodecIniReader, Systemd) {
	// Разбираемый текст настроек
	const string text = "[Service]\nExecStart=/bin/sh \\\n  -c true\nEnvironment=A=1\\sB=2\n";
	// Выполняем проверку слепка потока событий разбора
	ASSERT_EQ(::dump(text, ini::reader_t::settings_t::systemd()), "S[Service|]\nP[ExecStart=/bin/sh   -c true]\nP[Environment=A=1 B=2]\n");
}
/**
 * @brief Проверка разбора имени подраздела разделителем
 *
 */
TEST(CodecIniReader, Subsection) {
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Устанавливаем построение имени подраздела разделителем
	settings.subsections = ini::subsection_t::DELIMITED;
	// Выполняем проверку слепка потока событий разбора
	ASSERT_EQ(::dump("[a.b.c]\nk=v\n", settings), "S[a|b.c]\nP[k=v]\n");
	// Устанавливаем предел глубины вложенности подразделов
	settings.maxDepth = 1;
	// Выполняем проверку отклонения превышения глубины вложенности
	ASSERT_EQ(::dump("[a.b]\nk=v\n", settings), "E[subsection depth exceeded]@1:1\n");
}
/**
 * @brief Проверка разбора управляющих последовательностей значения
 *
 */
TEST(CodecIniReader, Escapes) {
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Устанавливаем разбор управляющих последовательностей в значении
	settings.escapes = true;
	// Выполняем проверку разбора управляющих последовательностей
	ASSERT_EQ(::dump("[a]\nk=first\\nsecond\\tend\n", settings), "S[a|]\nP[k=first\nsecond\tend]\n");
	// Выполняем проверку разбора записи знака кодовым значением Юникода
	ASSERT_EQ(::dump("[a]\nk=\\u0424\n", settings), "S[a|]\nP[k=Ф]\n");
	// Выполняем проверку отклонения неизвестной управляющей последовательности
	ASSERT_EQ(::dump("[a]\nk=\\q\n", settings), "S[a|]\nE[invalid escape sequence]@2:3\n");
}
/**
 * @brief Проверка обращения с кавычками вокруг значения
 *
 */
TEST(CodecIniReader, Quotes) {
	// Выполняем проверку сохранения пробельной обвязки внутри кавычек
	ASSERT_EQ(::dump("[a]\nk =  \" v \"  \n", ini::reader_t::settings_t()), "S[a|]\nP[k= v ]{quoted}\n");
	// Выполняем проверку склеивания частей значения в кавычках и вне их
	ASSERT_EQ(::dump("[a]\nk = \"first\"second\n", ini::reader_t::settings_t()), "S[a|]\nP[k=firstsecond]{quoted}\n");
	// Выполняем проверку отклонения незакрытой кавычки значения
	ASSERT_EQ(::dump("[a]\nk = \"value\n", ini::reader_t::settings_t()), "S[a|]\nE[unterminated quoted value]@2:5\n");
	/**
	 * Выполняем проверку того, что одиночная кавычка значения не ограждает
	 *
	 * @note Кавычкой признаётся лишь двойная: одиночная слишком часто встречается
	 *       в значениях как знак сокращения, и снятие её теряло бы часть значения
	 */
	ASSERT_EQ(::dump("[a]\nk = 'value'\n", ini::reader_t::settings_t()), "S[a|]\nP[k='value']\n");
}
/**
 * @brief Проверка признания примечания в конце строки
 *
 */
TEST(CodecIniReader, InlineComment) {
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Устанавливаем признание примечания в конце строки свойства
	settings.inlineComments = true;
	// Выполняем проверку выдачи примечания конца строки отдельным событием
	ASSERT_EQ(::dump("[a] ; шапка\nk = v # хвост\n", settings), "S[a|]\nC[шапка]2\nP[k=v]\nC[хвост]1\n");
	/**
	 * Выполняем проверку того, что знак примечания внутри значения его не обрывает
	 *
	 * @note Примечанием знак признаётся лишь отделённым пробельным знаком: запись
	 *       цвета «#RRGGBB» иначе теряла бы всё своё содержимое
	 */
	ASSERT_EQ(::dump("[a]\nk = value#tail\n", settings), "S[a|]\nP[k=value#tail]\n");
}
/**
 * @brief Проверка выдачи пустых строк отдельным событием
 *
 */
TEST(CodecIniReader, Blanks) {
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Устанавливаем выдачу пустых строк отдельным событием
	settings.emitBlanks = true;
	// Выполняем проверку слепка потока событий разбора
	ASSERT_EQ(::dump("[a]\n\n   \nk=v\n", settings), "S[a|]\nB\nB\nP[k=v]\n");
}
/**
 * @brief Проверка признания записи добавления к перечню значений
 *
 */
TEST(CodecIniReader, Arrays) {
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Устанавливаем признание записи добавления к перечню значений
	settings.arrays = true;
	// Выполняем проверку слепка потока событий разбора
	ASSERT_EQ(::dump("[a]\nk[] = first\nk[] = second\n", settings), "S[a|]\nP[k=first]{append}\nP[k=second]{append}\n");
}
/**
 * @brief Проверка отклонения неправильного построения текста настроек
 *
 */
TEST(CodecIniReader, Malformed) {
	// Выполняем проверку отклонения повторного объявления свойства
	ASSERT_EQ(::dump("[a]\nk=1\nk=2\n", ini::reader_t::settings_t::strict()), "S[a|]\nP[k=1]\nE[duplicate property]@3:1\n");
	// Выполняем проверку отклонения повторного объявления раздела
	ASSERT_EQ(::dump("[a]\n[a]\n", ini::reader_t::settings_t::strict()), "S[a|]\nE[duplicate section]@2:1\n");
	// Выполняем проверку отклонения свойства, объявленного до первого раздела
	ASSERT_EQ(::dump("k=1\n", ini::reader_t::settings_t::strict()), "E[property outside of any section]@1:1\n");
	// Выполняем проверку отклонения незакрытого объявления раздела
	ASSERT_EQ(::dump("[a\n", ini::reader_t::settings_t::strict()), "E[unclosed section header]@1:1\n");
	// Выполняем проверку отклонения пустого имени раздела
	ASSERT_EQ(::dump("[  ]\n", ini::reader_t::settings_t::strict()), "E[empty section name]@1:1\n");
	// Выполняем проверку отклонения содержимого за закрывающей скобкой
	ASSERT_EQ(::dump("[a] хвост\n", ini::reader_t::settings_t::strict()), "E[unexpected content after section header]@1:1\n");
	// Выполняем проверку отклонения строки без разделителя имени и значения
	ASSERT_EQ(::dump("[a]\nkey\n", ini::reader_t::settings_t::strict()), "S[a|]\nE[missing name and value separator]@2:1\n");
	// Выполняем проверку отклонения пустого имени свойства
	ASSERT_EQ(::dump("[a]\n = v\n", ini::reader_t::settings_t::strict()), "S[a|]\nE[empty property name]@2:1\n");
}
/**
 * @brief Проверка соблюдения пределов разбора
 *
 */
TEST(CodecIniReader, Limits) {
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Устанавливаем предел длины логической строки
	settings.maxLine = 8;
	// Выполняем проверку отклонения превышения длины логической строки
	ASSERT_EQ(::dump("[a]\nkey = очень длинное значение\n", settings), "S[a|]\nE[line is too long]@2:1\n");
	// Восстанавливаем предел длины логической строки
	settings.maxLine = ini::MAX_LINE;
	// Устанавливаем предел длины имени раздела или свойства
	settings.maxName = 2;
	// Выполняем проверку отклонения превышения длины имени свойства
	ASSERT_EQ(::dump("[a]\nkey = v\n", settings), "S[a|]\nE[name is too long]@2:1\n");
	// Восстанавливаем предел длины имени
	settings.maxName = ini::MAX_NAME;
	// Устанавливаем склеивание строк, продолженных обратной косой чертой
	settings.continuations = true;
	// Устанавливаем предел количества строк продолжения
	settings.maxContinuation = 1;
	// Выполняем проверку отклонения превышения количества строк продолжения
	ASSERT_EQ(::dump("[a]\nk = a\\\nb\\\nc\n", settings), "S[a|]\nE[line continuation limit exceeded]@4:1\n");
	/**
	 * Выполняем проверку снятия пределов нулевым значением
	 *
	 * @note Ноль снимает предел, а не запрещает строки и имена вовсе: строка нулевой
	 *       длины смысла не имеет, и запрещающее толкование оставило бы предел без
	 *       способа его снять. Тем же образом ноль толкуется у кодека разметки
	 */
	{
		// Собираемые настройки разбора со снятыми пределами
		ini::reader_t::settings_t released;
		// Выполняем снятие предела длины логической строки
		released.maxLine = 0;
		// Выполняем снятие предела длины имени раздела или свойства
		released.maxName = 0;
		// Выполняем проверку разбора текста при снятых пределах
		ASSERT_EQ(::dump("[a]\nkey = очень длинное значение\n", released), "S[a|]\nP[key=очень длинное значение]\n");
	}
}
/**
 * @brief Проверка разбора значения свойства числом
 *
 */
TEST(CodecIniReader, Numeric) {
	// Объект потокового чтения текста настроек
	ini::reader_t reader(::logger());
	// Выполняем передачу текста настроек целиком
	ASSERT_TRUE(reader.feed("[a]\nport = 8080\nratio = 0.25\nenabled = on\nwide = 70000\n"));
	// Выполняем переход к объявлению раздела
	ASSERT_TRUE(reader.next());
	// Выполняем переход к свойству с целым числом
	ASSERT_TRUE(reader.next());
	// Значение целого числа без знака
	uint16_t port = 0;
	// Выполняем разбор значения свойства числом
	ASSERT_TRUE(reader.value(port));
	// Выполняем проверку разобранного значения свойства
	ASSERT_EQ(port, 8080);
	// Выполняем переход к свойству с числом с плавающей точкой
	ASSERT_TRUE(reader.next());
	// Значение числа с плавающей точкой
	double ratio = 0.;
	// Выполняем разбор значения свойства числом
	ASSERT_TRUE(reader.value(ratio));
	// Выполняем проверку разобранного значения свойства
	ASSERT_DOUBLE_EQ(ratio, 0.25);
	// Выполняем переход к свойству с логическим значением
	ASSERT_TRUE(reader.next());
	// Логическое значение свойства
	bool enabled = false;
	// Выполняем разбор значения свойства логическим значением
	ASSERT_TRUE(reader.value(enabled));
	// Выполняем проверку разобранного значения свойства
	ASSERT_TRUE(enabled);
	/**
	 * Выполняем проверку отклонения записи «on» при строгом разборе
	 */
	ASSERT_FALSE(reader.value(enabled, ini::boolean_t::STRICT));
	// Выполняем переход к свойству с числом за пределами запрошенного типа
	ASSERT_TRUE(reader.next());
	// Значение целого числа без знака
	uint16_t wide = 0;
	/**
	 * Выполняем проверку переноса младшими разрядами числа, в запрошенный тип не
	 * помещающегося
	 *
	 * @note Прежде здесь стоял отказ. Отменён он владельцем 20.08.2026: приведение языка
	 *       не отказывает нигде, и признак успешности отведён одному лишь случаю, когда
	 *       значение числом не является вовсе
	 */
	ASSERT_TRUE(reader.value(wide));
	// Выполняем проверку разобранного значения свойства
	ASSERT_EQ(wide, 4464);
}
/**
 * @brief Проверка сброса разбора в исходное состояние
 *
 */
/**
 * @brief Проверка отключения обрезки пробельной обвязки значения
 *
 * @note Обвязка значения - это хвост всей логической строки, и обрезка её до
 *       разбора отнимала у настройки всякий смысл: значение приходило обрезанным
 *       независимо от неё
 *
 */
TEST(CodecIniReader, Untrimmed) {
	{
		// Собираемые настройки разбора текста настроек
		ini::reader_t::settings_t settings;
		// Снимаем обрезку пробельной обвязки имён и значений
		settings.trim = false;
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), settings);
		// Выполняем передачу текста настроек
		ASSERT_TRUE(reader.feed(string("[a]\n  k = v   \n")));
		// Признак получения свойства со значением
		bool received = false;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если получено свойство со значением
			 */
			if(reader.event() == ini::event_t::PROPERTY){
				// Запоминаем признак получения свойства со значением
				received = true;
				// Выполняем проверку имени свойства
				ASSERT_EQ(reader.key(), "k");
				// Выполняем проверку сохранности хвостовой обвязки значения
				ASSERT_EQ(reader.text(), "v   ");
			}
		}
		// Выполняем проверку получения свойства со значением
		ASSERT_TRUE(received);
	}{
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger());
		// Выполняем передачу текста настроек
		ASSERT_TRUE(reader.feed(string("[a]\n  k = v   \n")));
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если получено свойство со значением
			 */
			if(reader.event() == ini::event_t::PROPERTY)
				// Выполняем проверку обрезки хвостовой обвязки значения
				ASSERT_EQ(reader.text(), "v");
		}
	}
}
/**
 * @brief Проверка указания столбца ошибки в строке с отступом
 *
 */
TEST(CodecIniReader, ErrorColumn) {
	// Объект потокового чтения текста настроек
	ini::reader_t reader(::logger());
	/**
	 * Выполняем передачу текста настроек с незакрытой кавычкой значения
	 *
	 * @note Прежде здесь стояло имя свойства с квадратною скобкою посреди, но запись эта
	 *       законна: образцовые разборы наречий берут «k[x» именем свойства, и запрет её
	 *       был строже образца
	 */
	ASSERT_TRUE(reader.feed(string("[a]\n      k = \"v\n")));
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next());
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(reader.error(), ini::error_t::UNTERMINATED_QUOTE);
	// Выполняем проверку номера строки ошибки
	ASSERT_EQ(reader.errorLocation().line, 2u);
	/**
	 * Выполняем проверку столбца ошибки
	 *
	 * @note Столбец считается по строке, как она в файле записана: шесть знаков
	 *       отступа, имя свойства с разделителем и незакрытая кавычка одиннадцатым знаком
	 */
	ASSERT_EQ(reader.errorLocation().column, 11u);
}
/**
 * @brief Проверка отклонения пустого имени подраздела за знаком-разделителем
 *
 */
TEST(CodecIniReader, EmptySubsection) {
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Устанавливаем построение имени подраздела разделителем
	settings.subsections = ini::subsection_t::DELIMITED;
	{
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), settings);
		// Выполняем передачу текста настроек
		ASSERT_TRUE(reader.feed(string("[a.]\nk = v\n")));
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next());
		// Выполняем проверку отклонения пустого имени подраздела
		ASSERT_EQ(reader.error(), ini::error_t::INVALID_SUBSECTION);
	}{
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), settings);
		// Выполняем передачу текста настроек
		ASSERT_TRUE(reader.feed(string("[a.b]\nk = v\n")));
		// Признак получения объявления раздела
		bool received = false;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если получено объявление раздела
			 */
			if(reader.event() == ini::event_t::SECTION){
				// Запоминаем признак получения объявления раздела
				received = true;
				// Выполняем проверку имени раздела
				ASSERT_EQ(reader.section().section, "a");
				// Выполняем проверку имени подраздела
				ASSERT_EQ(reader.section().subsection, "b");
			}
		}
		// Выполняем проверку получения объявления раздела
		ASSERT_TRUE(received);
	}
}
/**
 * @brief Проверка сброса свойства при выдаче примечания конца строки
 *
 * @note Договор велит держать поля свойства пустыми у всякого события, кроме
 *       события свойства
 *
 */
TEST(CodecIniReader, CommentProperty) {
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Устанавливаем признание примечания в конце строки свойства
	settings.inlineComments = true;
	// Объект потокового чтения текста настроек
	ini::reader_t reader(::logger(), settings);
	// Выполняем передачу текста настроек
	ASSERT_TRUE(reader.feed(string("[a]\nk = v ; хвост\n")));
	// Признак получения примечания конца строки
	bool received = false;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		/**
		 * Если получено примечание
		 */
		if(reader.event() == ini::event_t::COMMENT){
			// Запоминаем признак получения примечания
			received = true;
			// Выполняем проверку содержимого примечания
			ASSERT_EQ(reader.comment().text, "хвост");
			// Выполняем проверку пустоты имени свойства
			ASSERT_TRUE(reader.key().empty());
			// Выполняем проверку пустоты значения свойства
			ASSERT_TRUE(reader.property().value.empty());
		}
	}
	// Выполняем проверку получения примечания конца строки
	ASSERT_TRUE(received);
}
/**
 * @brief Проверка поиска закрывающей скобки объявления раздела
 *
 * @note Скобка ищется разбором, а не поиском с конца: с конца она находилась бы в
 *       примечании за объявлением и молча портила имя раздела
 *
 */
TEST(CodecIniReader, SectionClosing) {
	{
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger());
		// Выполняем передачу текста настроек с квадратной скобкой в примечании
		ASSERT_TRUE(reader.feed(string("[a] ; см. раздел [docs]\nk = v\n")));
		// Признак получения объявления раздела
		bool received = false;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если получено объявление раздела
			 */
			if(reader.event() == ini::event_t::SECTION){
				// Запоминаем признак получения объявления раздела
				received = true;
				// Выполняем проверку имени раздела
				ASSERT_EQ(reader.section().section, "a");
			}
		}
		// Выполняем проверку получения объявления раздела
		ASSERT_TRUE(received);
		// Выполняем проверку отсутствия ошибок разбора
		ASSERT_EQ(reader.error(), ini::error_t::NONE);
	}{
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), ini::reader_t::settings_t::git());
		// Выполняем передачу текста настроек со скобкой внутри имени подраздела
		ASSERT_TRUE(reader.feed(string("[remote \"a]b\"]\n\turl = x\n")));
		// Признак получения объявления раздела
		bool received = false;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если получено объявление раздела
			 */
			if(reader.event() == ini::event_t::SECTION){
				// Запоминаем признак получения объявления раздела
				received = true;
				// Выполняем проверку имени раздела
				ASSERT_EQ(reader.section().section, "remote");
				/**
				 * Выполняем проверку имени подраздела
				 *
				 * @note Скобка внутри кавычек закрывающей не считается
				 */
				ASSERT_EQ(reader.section().subsection, "a]b");
			}
		}
		// Выполняем проверку получения объявления раздела
		ASSERT_TRUE(received);
	}
}
/**
 * @brief Проверка отклонения смены настроек посреди разбора
 *
 * @note Смена посреди текста применилась бы к остатку, но не к разобранному
 *       началу, и один файл читался бы двумя наречиями сразу
 *
 */
TEST(CodecIniReader, SettingsLocked) {
	// Объект потокового чтения текста настроек
	ini::reader_t reader(::logger());
	// Выполняем передачу первого куска исходного текста
	ASSERT_TRUE(reader.feed("[a]\nk = v\n", 9, false));
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Устанавливаем разбор управляющих последовательностей в значении
	settings.escapes = true;
	// Выполняем установку настроек разбора
	reader.settings(settings);
	// Выполняем проверку того, что настройки остались прежними
	ASSERT_FALSE(reader.settings().escapes);
	// Выполняем сброс состояния чтения
	reader.reset();
	// Выполняем установку настроек разбора
	reader.settings(settings);
	// Выполняем проверку принятия настроек после сброса
	ASSERT_TRUE(reader.settings().escapes);
}
TEST(CodecIniReader, Reset) {
	// Объект потокового чтения текста настроек
	ini::reader_t reader(::logger(), ini::reader_t::settings_t::strict());
	// Выполняем передачу текста настроек целиком
	ASSERT_TRUE(reader.feed("[a]\nk=1\nk=2\n"));
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next());
	// Выполняем проверку прекращения разбора ошибкой
	ASSERT_EQ(reader.state(), ini::state_t::FAILED);
	// Выполняем сброс разбора в исходное состояние
	reader.reset();
	// Выполняем проверку сброса кода ошибки разбора
	ASSERT_EQ(reader.error(), ini::error_t::NONE);
	// Выполняем передачу нового текста настроек целиком
	ASSERT_TRUE(reader.feed("[b]\nk=1\n"));
	// Выполняем переход к объявлению раздела
	ASSERT_TRUE(reader.next());
	// Выполняем проверку имени объявленного раздела
	ASSERT_TRUE(reader.section().is("b"));
	// Выполняем проверку сохранения настроек разбора при сбросе
	ASSERT_EQ(reader.settings().duplicates, ini::duplicate_t::ERROR);
}

/**
 * @brief Проверка выдачи событий, разобранных до отказа приведения кодировки
 *
 */
TEST(CodecIniReader, DecodingPrefix) {
	// Разбираемый текст настроек с испорченной последовательностью UTF-8
	const string text = string("[a]\nk=1\nm=bad\xC3\x28 tail\n");
	/**
	 * Выполняем перебор размеров куска подачи текста настроек
	 */
	for(size_t chunk : {static_cast <size_t> (0), static_cast <size_t> (1), static_cast <size_t> (7)}){
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), ini::reader_t::settings_t::strict());
		// Размер подаваемого куска текста настроек
		const size_t size = (chunk > 0 ? chunk : text.length());
		// Смещение начала очередного куска подачи
		size_t offset = 0;
		// Количество выданных разбором событий
		uint32_t count = 0;
		/**
		 * Выполняем подачу текста настроек кусками
		 */
		do {
			// Размер подаваемого куска текста настроек
			const size_t length = ((text.length() - offset) < size ? (text.length() - offset) : size);
			// Выполняем подачу очередного куска текста настроек
			if(!reader.feed(text.data() + offset, length, ((offset + length) >= text.length())))
				// Выходим из цикла подачи текста настроек
				break;
			// Выполняем смещение начала очередного куска подачи
			offset += length;
			/**
			 * Выполняем перебор всех выданных разбором событий
			 */
			while(reader.next())
				// Выполняем учёт выданного разбором события
				count++;
			// Если разбор прекращён ошибкой
			if(reader.state() == ini::state_t::FAILED)
				// Выходим из цикла подачи текста настроек
				break;
		// Выполняем подачу до исчерпания текста настроек
		} while(offset < text.length());
		// Выполняем проверку выдачи событий, разобранных до отказа приведения
		ASSERT_EQ(count, 2u);
		// Выполняем проверку прекращения разбора ошибкой
		ASSERT_EQ(reader.state(), ini::state_t::FAILED);
		// Выполняем проверку кода ошибки приведения кодировки
		ASSERT_EQ(reader.error(), ini::error_t::INVALID_ENCODING);
		// Выполняем проверку указания места отказа на испорченную строку
		ASSERT_EQ(reader.errorLocation().line, 3u);
	}
}

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include <sys/macro_pop.hpp>
/**
 * @brief Проверка изъятия разобранного начала текста настроек
 *
 * @details Накопленное начало текста изымается из буфера, когда разобранного набралось
 *          свыше 64 КиБ: положения разбора при этом сдвигаются, а место события
 *          считается уже с учётом изъятого. Тексты короче порога этот ход не задевают
 *          вовсе, и до сих пор его не исполняла ни одна проверка
 *
 */
TEST(CodecIniReader, Compaction) {
	/**
	 * @brief Метод сборки слепка выдачи разбора текста настроек
	 *
	 * @param text     разбираемый текст настроек
	 * @param chunk    размер куска подачи, ноль подаёт текст целиком
	 * @param settings настройки разбора текста настроек
	 * @return         собранный слепок выдачи разбора
	 *
	 */
	const auto events = [](const string & text, const size_t chunk, const ini::reader_t::settings_t & settings) noexcept -> string {
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), settings);
		// Собираемый слепок выдачи разбора
		string result;
		// Положение подачи в разбираемом тексте настроек
		size_t offset = 0;
		/**
		 * Выполняем сборку слепка до исчерпания выдачи разбора
		 */
		while(true){
			/**
			 * Выполняем подачу текста настроек до получения очередного события
			 */
			while(!reader.next()){
				// Если разбор события не ждёт либо подавать больше нечего
				if((reader.state() != ini::state_t::HUNGRY) || (offset >= text.size()))
					// Выводим собранный слепок вместе с итогом разбора
					return result.append("=").append(::std::to_string(static_cast <uint32_t> (reader.state())))
						.append("/").append(::std::to_string(static_cast <uint32_t> (reader.error())));
				// Получаем размер очередного куска подачи текста настроек
				const size_t size = (((chunk == 0) || ((offset + chunk) > text.size())) ? (text.size() - offset) : chunk);
				// Выполняем подачу очередного куска текста настроек
				reader.feed(text.data() + offset, size, ((offset + size) >= text.size()));
				// Выполняем смещение положения подачи текста настроек
				offset += size;
			}
			// Дописываем к слепку разновидность полученного события
			result.append(::std::to_string(static_cast <uint32_t> (reader.event()))).append(":");
			// Дописываем к слепку имя свойства и длину его значения
			result.append(reader.key()).append("=").append(::std::to_string(reader.text().size())).append("@");
			// Дописываем к слепку место начала полученного события
			result.append(::std::to_string(reader.location().line)).append(",");
			result.append(::std::to_string(reader.location().column)).append(",");
			result.append(::std::to_string(reader.location().offset)).append("|");
		}
	};
	// Собираемые настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Выполняем включение выдачи примечаний отдельными событиями
	settings.emitComments = true;
	// Выполняем включение выдачи пустых строк отдельными событиями
	settings.emitBlanks = true;
	// Выполняем включение склеивания строк по обратной косой черте
	settings.continuations = true;
	// Выполняем снятие предела длины логической строки
	settings.maxLine = 0;
	// Выполняем снятие предела количества строк продолжения
	settings.maxContinuation = 0xFFFFFFFF;
	/**
	 * Выполняем проверку изъятия начала на множестве свойств
	 */
	{
		// Собираемый текст настроек длиннее порога изъятия
		string text("[раздел]\n");
		/**
		 * Выполняем сборку множества свойств раздела
		 */
		for(uint32_t i = 0; i < 4000; i++){
			// Дописываем к тексту настроек очередное свойство со значением
			text.append("; примечание ").append(::std::to_string(i)).append("\n");
			text.append("ключ").append(::std::to_string(i)).append(" = значение ").append(::std::to_string(i)).append("\n");
		}
		// Выполняем проверку того, что собранный текст предел изъятия превысил
		ASSERT_GT(text.size(), static_cast <size_t> (0x10000));
		// Слепок выдачи разбора текста настроек, поданного целиком
		const string whole = events(text, 0, settings);
		// Выполняем проверку того, что разбор текста настроек удался
		ASSERT_NE(whole.find("=2/0"), string::npos);
		/**
		 * Выполняем перебор размеров куска подачи текста настроек
		 */
		for(const size_t chunk : {static_cast <size_t> (997), static_cast <size_t> (4096), static_cast <size_t> (0x10000)})
			// Выполняем проверку независимости выдачи разбора от нарезки на куски
			ASSERT_EQ(events(text, chunk, settings), whole) << chunk;
	}
	/**
	 * Выполняем проверку изъятия начала под одной логической строкой
	 *
	 * @note Логическая строка, собранная из продолжений, перекрывает порог изъятия
	 *       целиком: изымать начало под нею нельзя, пока она не дочитана
	 */
	{
		// Собираемый текст настроек с длинной логической строкой
		string text("[a]\nk = начало");
		/**
		 * Выполняем сборку строк продолжения логической строки
		 */
		for(uint32_t i = 0; i < 4000; i++)
			// Дописываем к тексту настроек очередную строку продолжения
			text.append(" \\\n продолжение ").append(::std::to_string(i));
		// Выполняем завершение собираемого текста настроек
		text.append("\n[b]\nm = v\n");
		// Выполняем проверку того, что собранный текст предел изъятия превысил
		ASSERT_GT(text.size(), static_cast <size_t> (0x10000));
		// Слепок выдачи разбора текста настроек, поданного целиком
		const string whole = events(text, 0, settings);
		// Выполняем проверку того, что разбор текста настроек удался
		ASSERT_NE(whole.find("=2/0"), string::npos);
		/**
		 * Выполняем перебор размеров куска подачи текста настроек
		 */
		for(const size_t chunk : {static_cast <size_t> (997), static_cast <size_t> (0x10000)})
			// Выполняем проверку независимости выдачи разбора от нарезки на куски
			ASSERT_EQ(events(text, chunk, settings), whole) << chunk;
	}
}

/**
 * @brief Проверка признания примечания в конце строки без разделителя
 *
 * @details Примечание в конце строки признавалось лишь разбором значения, а тот
 * исполняется только при найденном разделителе имени и значения. Оттого свойство без
 * значения забирало примечание себе в имя, а при разделителях обоих видов разделителем
 * записи становился знак внутри самого примечания. Объявление раздела страдало тем же:
 * поиск закрывающей скобки уходил внутрь примечания
 *
 * @note Имя, несущее знак примечания, обратной записью невыразимо: собственный разбор
 *       прочтёт такую строку иначе. Проверка потому утверждает и чистоту имён, и выдачу
 *       самого примечания отдельным событием
 *
 */
TEST(CodecIniReader, InlineCommentWithoutSeparator) {
	// Настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Признаём примечание в конце строки
	settings.inlineComments = true;
	// Признаём свойство, записанное без разделителя и значения
	settings.valueless = true;
	// Признаём разделителями имени и значения оба знака
	settings.separators = ini::separator_t::BOTH;
	// Выполняем проверку примечания за именем свойства без значения
	ASSERT_EQ(::dump("k ; примечание\n", settings), "P[k=]{novalue}\nC[примечание]1\n");
	/**
	 * Выполняем проверку примечания, несущего знак разделителя
	 *
	 * @note Двоеточие внутри примечания разделителем записи не является: примечание
	 *       начато прежде него и до конца строки простирается
	 */
	ASSERT_EQ(::dump("k # см. далее: текст\n", settings), "P[k=]{novalue}\nC[см. далее: текст]1\n");
	/**
	 * Выполняем проверку объявления раздела, скобку потерявшего
	 *
	 * @note Примечание объявление обрывает, и скобка, стоящая внутри примечания,
	 *       закрывающей не является: объявление остаётся незакрытым
	 */
	ASSERT_EQ(::dump("[раздел ; см. далее [тут]\n", settings), "E[unclosed section header]@1:1\n");
	// Выполняем проверку того, что знак примечания без отступа именем остаётся
	ASSERT_EQ(::dump("[раздел;часть]\n", settings), "S[раздел;часть|]\n");
}

/**
 * @brief Проверка места примечания, приписанного к объявлению раздела
 *
 * @details Местом примечания служит сам знак, которым оно начато: примечание за
 * свойством выдавалось именно так, а примечание за объявлением раздела указывало на
 * начало строки. Два одинаковых по существу примечания выдавались разными местами
 *
 */
TEST(CodecIniReader, HeaderCommentLocation) {
	// Настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Признаём примечание в конце строки
	settings.inlineComments = true;
	// Объект потокового чтения текста настроек
	ini::reader_t reader(::logger(), settings);
	// Выполняем передачу текста настроек целиком
	reader.feed(string_view("[раздел] ; примечание\nk = v ; хвост\n"));
	// Выполняем переход к объявлению раздела
	ASSERT_TRUE(reader.next());
	// Выполняем проверку вида полученного события
	ASSERT_EQ(reader.event(), ini::event_t::SECTION);
	// Выполняем переход к примечанию за объявлением раздела
	ASSERT_TRUE(reader.next());
	// Выполняем проверку вида полученного события
	ASSERT_EQ(reader.event(), ini::event_t::COMMENT);
	// Выполняем проверку того, что место указывает на знак примечания
	ASSERT_EQ(reader.location().column, static_cast <uint32_t> (10));
	// Выполняем проверку номера строки примечания
	ASSERT_EQ(reader.location().line, static_cast <uint32_t> (1));
	// Выполняем переход к свойству со значением
	ASSERT_TRUE(reader.next());
	// Выполняем проверку вида полученного события
	ASSERT_EQ(reader.event(), ini::event_t::PROPERTY);
	// Выполняем переход к примечанию за свойством
	ASSERT_TRUE(reader.next());
	// Выполняем проверку вида полученного события
	ASSERT_EQ(reader.event(), ini::event_t::COMMENT);
	// Выполняем проверку того, что место указывает на знак примечания
	ASSERT_EQ(reader.location().column, static_cast <uint32_t> (7));
}

/**
 * @brief Проверка места события внутри склеенной логической строки
 *
 * @details Логическая строка вправе собираться из нескольких физических - продолжением
 * обратной косой чертой либо отступом. Столбец при этом считался от начала логической
 * строки сквозь переносы, а строкой выдавалась первая из них: примечание, приписанное
 * ко второй физической строке, получало столбец больше длины первой - место, которого
 * в тексте нет вовсе
 *
 * @note Место незакрытой кавычки указывает на саму кавычку, а не на начало значения:
 *       искать её читающему следует там, где она стоит
 *
 */
TEST(CodecIniReader, ContinuationLocation) {
	// Настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Признаём примечание в конце строки
	settings.inlineComments = true;
	// Признаём склеивание строк обратной косой чертой
	settings.continuations = true;
	// Признаём управляющие последовательности значения
	settings.escapes = true;
	{
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), settings);
		// Выполняем передачу текста настроек целиком
		reader.feed(string_view("k = начало \\\nпродолжение ; заметка\n"));
		// Выполняем переход к свойству со значением
		ASSERT_TRUE(reader.next());
		// Выполняем проверку вида полученного события
		ASSERT_EQ(reader.event(), ini::event_t::PROPERTY);
		// Выполняем переход к примечанию за значением свойства
		ASSERT_TRUE(reader.next());
		// Выполняем проверку вида полученного события
		ASSERT_EQ(reader.event(), ini::event_t::COMMENT);
		// Выполняем проверку того, что примечание отнесено ко второй физической строке
		ASSERT_EQ(reader.location().line, static_cast <uint32_t> (2));
		// Выполняем проверку того, что столбец отсчитан от начала своей строки
		ASSERT_EQ(reader.location().column, static_cast <uint32_t> (13));
	}
	{
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), settings);
		// Выполняем передачу текста настроек целиком
		reader.feed(string_view("k = начало \\\nхвост \\q\n"));
		// Выполняем проверку того, что разбор отвергнут негодной последовательностью
		ASSERT_FALSE(reader.next());
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), ini::error_t::INVALID_ESCAPE);
		// Выполняем проверку того, что отказ отнесён ко второй физической строке
		ASSERT_EQ(reader.errorLocation().line, static_cast <uint32_t> (2));
		// Выполняем проверку того, что столбец указывает на обратную косую черту
		ASSERT_EQ(reader.errorLocation().column, static_cast <uint32_t> (7));
	}
	{
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), settings);
		// Выполняем передачу текста настроек целиком
		reader.feed(string_view("k = начало \\\nхвост \"открыта\n"));
		// Выполняем проверку того, что разбор отвергнут незакрытой кавычкой
		ASSERT_FALSE(reader.next());
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), ini::error_t::UNTERMINATED_QUOTE);
		// Выполняем проверку того, что отказ отнесён ко второй физической строке
		ASSERT_EQ(reader.errorLocation().line, static_cast <uint32_t> (2));
		// Выполняем проверку того, что столбец указывает на саму кавычку
		ASSERT_EQ(reader.errorLocation().column, static_cast <uint32_t> (7));
	}
	{
		// Настройки разбора с продолжением значения отступом
		ini::reader_t::settings_t indented = settings;
		// Признаём продолжение значения отступом
		indented.indents = true;
		// Объект потокового чтения текста настроек
		ini::reader_t reader(::logger(), indented);
		// Выполняем передачу текста настроек целиком
		reader.feed(string_view("k = начало\n\tпродолжение ; заметка\n"));
		// Выполняем переход к свойству со значением
		ASSERT_TRUE(reader.next());
		// Выполняем проверку вида полученного события
		ASSERT_EQ(reader.event(), ini::event_t::PROPERTY);
		// Выполняем переход к примечанию за значением свойства
		ASSERT_TRUE(reader.next());
		// Выполняем проверку того, что примечание отнесено ко второй физической строке
		ASSERT_EQ(reader.location().line, static_cast <uint32_t> (2));
		// Выполняем проверку того, что отступ учтён в столбце примечания
		ASSERT_EQ(reader.location().column, static_cast <uint32_t> (14));
	}
}

/**
 * @brief Проверка знака примечания в начале имени раздела
 *
 * @details Примечание в конце строки открывается знаком, стоящим в начале строки либо
 * за знаком пробельным. Имени раздела предшествует скобка, а имени подраздела -
 * знак-разделитель: примечания знак в начале такого имени не открывает, и запись `[#]`
 * есть раздел с именем из одного знака решётки. Имени же свойства предшествует начало
 * строки, и там знак этот обращает в примечание всю строку
 *
 * @note Закрепляется поведение, за которое кодек винила проверка ворошителя: она
 *       считала начало всякого имени началом строки. Кодек был прав, строгой была
 *       проверка
 *
 */
TEST(CodecIniReader, CommentSignOpeningName) {
	// Настройки разбора текста настроек
	ini::reader_t::settings_t settings;
	// Признаём примечание в конце строки
	settings.inlineComments = true;
	// Признаём подразделы, знаком-разделителем отделяемые
	settings.subsections = ini::subsection_t::DELIMITED;
	// Выполняем проверку раздела, из одного знака примечания состоящего
	ASSERT_EQ(::dump("[#]\n", settings), "S[#|]\n");
	// Выполняем проверку раздела, знаком примечания начатого
	ASSERT_EQ(::dump("[#a]\n", settings), "S[#a|]\n");
	// Выполняем проверку подраздела, знаком примечания начатого
	ASSERT_EQ(::dump("[a.#b]\n", settings), "S[a|#b]\n");
	/**
	 * Выполняем проверку раздела, где знаку примечания предшествует пробел
	 *
	 * @note Здесь примечание открывается по правилу, и объявление раздела остаётся
	 *       незакрытым
	 */
	ASSERT_NE(::dump("[ #]\n", settings).find("E["), string::npos);
	/**
	 * Выполняем проверку того, что строка, знаком примечания начатая, есть примечание
	 *
	 * @note Примечание это собственное, а не приписанное записи, - оттого и нуль
	 */
	ASSERT_EQ(::dump("#b = 1\n", settings), "C[b = 1]0\n");
}
