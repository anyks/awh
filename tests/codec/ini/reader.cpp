/**
 * @file: reader.cpp
 * @date: 2026-08-09
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты потокового чтения текста настроек INI — наречия записи,
 *        примечания и их расположение, продолжения строк, управляющие последовательности,
 *        отклонение неправильного построения, пределы разбора и подача текста кусками
 *
 * @copyright: Copyright © 2026
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
	ini::reader_t reader(settings);
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
}
/**
 * @brief Проверка разбора значения свойства числом
 *
 */
TEST(CodecIniReader, Numeric) {
	// Объект потокового чтения текста настроек
	ini::reader_t reader;
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
	 * Выполняем проверку отклонения числа, в запрошенный тип не помещающегося
	 */
	ASSERT_FALSE(reader.value(wide));
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
		ini::reader_t reader(settings);
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
		ini::reader_t reader;
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
	ini::reader_t reader;
	// Выполняем передачу текста настроек
	ASSERT_TRUE(reader.feed(string("[a]\n      k[x = 1\n")));
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next());
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(reader.error(), ini::error_t::INVALID_KEY);
	// Выполняем проверку номера строки ошибки
	ASSERT_EQ(reader.errorLocation().line, 2u);
	/**
	 * Выполняем проверку столбца ошибки
	 *
	 * @note Столбец считается по строке, как она в файле записана: шесть знаков
	 *       отступа, имя свойства и недопустимая скобка восьмым знаком
	 */
	ASSERT_EQ(reader.errorLocation().column, 8u);
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
		ini::reader_t reader(settings);
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
		ini::reader_t reader(settings);
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
	ini::reader_t reader(settings);
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
		ini::reader_t reader;
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
		ini::reader_t reader(ini::reader_t::settings_t::git());
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
	ini::reader_t reader;
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
	ini::reader_t reader(ini::reader_t::settings_t::strict());
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
 * Возвращаем макросы, снятые в начале файла
 */
#include <sys/macro_pop.hpp>
