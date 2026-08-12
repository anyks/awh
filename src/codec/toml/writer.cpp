/**
 * @file: writer.cpp
 * @date: 2026-08-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация записи текста настроек TOML — проверка построения имён ключей,
 *        ограждение строковых значений, запись чисел, отметок времени, перечней,
 *        встроенных таблиц, объявлений таблиц и примечаний
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <cstdio>
#include <limits>
#include <clocale>
#include <cstring>
#include <type_traits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <encoding/ascii.hpp>
#include <num/lexical/lexical.hpp>
#include <codec/toml/writer.hpp>

/**
 * Снимаем на время реализации макросы, чьи имена заняты
 * членами перечислений AWH (возвращает их macro_pop.hpp в конце файла)
 */
#include <sys/macro_push.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;
	/**
	 * Пространство имён контейнера TOML
	 */
	using namespace awh::codec::toml;

	/**
	 * @brief Метод проверки знака на принадлежность к управляющим
	 *
	 * @details Описание отводит тексту настроек знаки печатные, знак горизонтальной
	 * табуляции и знаки конца строки: прочие управляющие знаки в записи дословной
	 * оградить нечем, а в основной они записываются последовательностями
	 *
	 * @param letter проверяемый знак
	 * @return       результат проверки
	 *
	 */
	static bool controlled(const char letter) noexcept {
		// Получаем беззнаковое значение проверяемого знака
		const uint8_t value = static_cast <uint8_t> (letter);
		// Выводим результат проверки знака на принадлежность к управляющим
		return (((value < 0x20) && (letter != '\t') && (letter != '\n') && (letter != '\r')) || (value == 0x7F));
	}
	/**
	 * @brief Метод замены знака десятичной точки локали точкой
	 *
	 * @details Запись числа средствами языка знак десятичной точки берёт из локали, а
	 * текст настроек - запись протокольная: под локалью de_DE «0.1» вышло бы как «0,1»
	 * и обратно уже не прочиталось бы
	 *
	 * @param buffer буфер записи числа
	 * @param length длина записи числа
	 *
	 */
	static void pointed(char * buffer, int32_t & length) noexcept {
		// Получаем знак десятичной точки, принятый текущей локалью
		const char * point = ::localeconv()->decimal_point;
		/**
		 * Если знаком десятичной точки локали является точка
		 */
		if((point == nullptr) || (point[0] == '\0') || (::strcmp(point, ".") == 0))
			// Выходим из функции
			return;
		/**
		 * Длина знака десятичной точки локали
		 *
		 * @warning Знак этот однобайтовым быть не обязан: локаль пушту «ps_AF» несёт
		 *          десятичным знаком «٫» (U+066B), занимающий в UTF-8 два байта
		 */
		const size_t size = ::strlen(point);
		/**
		 * Если знак десятичной точки локали длиннее самой записи числа
		 */
		if((size == 0) || (static_cast <int32_t> (size) > length))
			// Выходим из функции
			return;
		/**
		 * Выполняем перебор всех знаков записи числа
		 *
		 * @note Десятичный знак в записи числа встречается не более одного раза, и
		 *       поиск прекращается на первом же его появлении
		 */
		for(int32_t i = 0; i <= (length - static_cast <int32_t> (size)); i++){
			/**
			 * Если с этого места стоит знак десятичной точки локали
			 */
			if(::memcmp(buffer + i, point, size) == 0){
				// Выполняем замену знака десятичной точки локали точкой
				buffer[i] = '.';
				// Выполняем сдвиг остатка записи на место снятых байтов
				::memmove((buffer + i + 1), (buffer + i + size), static_cast <size_t> (length) - (static_cast <size_t> (i) + size));
				// Уменьшаем длину записи на количество снятых байтов
				length -= static_cast <int32_t> (size - 1);
				// Выполняем завершение записи числа нулевым байтом
				buffer[length] = '\0';
				// Выходим из цикла перебора знаков записи числа
				break;
			}
		}
	}
	/**
	 * @brief Метод чтения записи числа с плавающей точкой
	 *
	 * @details Чтение ведётся средствами модуля разбора чисел, а не языка: чтение
	 * средствами языка знак десятичной точки берёт из локали, и под локалью de_DE
	 * запись «0.1» прочиталась бы единицей
	 *
	 * @param text   читаемая запись числа с плавающей точкой
	 * @param result полученное число с плавающей точкой
	 * @return       результат выполнения операции
	 *
	 */
	static bool numeric(const string_view text, double & result) noexcept {
		// Выполняем разбор записи числа с плавающей точкой
		const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), (text.data() + text.length()), result);
		// Выводим результат разбора записи числа целиком
		return (static_cast <bool> (res) && (res.ptr == (text.data() + text.length())));
	}
};

/**
 * @brief Конструктор
 *
 */
awh::codec::toml::Writer::Settings::Settings() noexcept :
 maxLine(MAX_LINE), maxKey(MAX_KEY), maxDepth(MAX_DEPTH), maxParts(MAX_PARTS),
 unicode(false), promote(true), spaces(true), indent(false), separated(true),
 newline(newline_t::LF) {}

/**
 * @brief Метод записи знака конца строки
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::newline() noexcept {
	/**
	 * Если длина собранной логической строки превышает допустимую
	 */
	if((this->_settings.maxLine > 0) && (this->_length > static_cast <size_t> (this->_settings.maxLine))){
		// Запоминаем код ошибки записи
		this->_error = error_t::LINE_TOO_LONG;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем запись знака конца строки
	this->_text.append(toml::newline(this->_settings.newline));
	// Выполняем сброс длины собираемой логической строки
	this->_length = 0;
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи знака конца строки внутри записи
 *
 */
void awh::codec::toml::Writer::fold() noexcept {
	/**
	 * Выполняем запись знака конца строки, длину записи не сбрасывая
	 *
	 * @note Разбор меряет пределом длины запись целиком - вместе со всеми строками
	 *       многострочного значения, - и сброс счёта здесь дал бы обход предела:
	 *       собранный текст читающий отверг бы длиной записи
	 */
	this->append(toml::newline(this->_settings.newline));
}
/**
 * @brief Метод дописывания последовательности знаков к собираемому тексту
 *
 * @param text дописываемая последовательность знаков
 *
 */
void awh::codec::toml::Writer::append(const string_view text) noexcept {
	// Выполняем дописывание последовательности знаков к собираемому тексту
	this->_text.append(text);
	// Увеличиваем длину собираемой логической строки
	this->_length += text.length();
}
/**
 * @brief Метод получения вида окружения записываемого значения
 *
 * @return вид окружения, в котором ведётся запись
 *
 */
awh::codec::toml::Writer::context_t awh::codec::toml::Writer::context() const noexcept {
	// Выводим вид окружения, в котором ведётся запись
	return (this->_levels.empty() ? context_t::ROOT : this->_levels.back().context);
}
/**
 * @brief Метод проверки готовности записи очередной строки текста
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::ready() noexcept {
	/**
	 * Если запись ведётся строками текста настроек
	 */
	if(this->_levels.empty())
		// Выводим положительный результат выполнения операции
		return true;
	/**
	 * Запоминаем код ошибки записи
	 *
	 * @note Незаписанное значение отличается от незакрытого перечня: первое означает
	 *       пропущенный вызов записи значения, второе - порядок вызовов, строке текста
	 *       места не оставляющий
	 */
	this->_error = ((this->_levels.back().context == context_t::KEYED) ? error_t::MISSING_VALUE : error_t::INTERNAL);
	// Выводим отрицательный результат выполнения операции
	return false;
}
/**
 * @brief Метод записи отступа перед парой объявленной таблицы
 *
 */
void awh::codec::toml::Writer::indent() noexcept {
	/**
	 * Если запись отступа перед парами таблицы настройками задана
	 */
	if(this->_settings.indent && this->_tabled)
		// Выполняем запись отступа перед именем ключа
		this->append("\t");
}
/**
 * @brief Метод записи разделителя очередного значения перечня
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::separate() noexcept {
	// Получаем уровень вложенности, на котором ведётся запись
	level_t & level = this->_levels.back();
	/**
	 * Если на уровне вложенности уже записано значение
	 */
	if(level.count > 0)
		// Выполняем запись разделителя значений
		this->append(",");
	/**
	 * Если перечень записывается несколькими строками
	 */
	if(level.multiline){
		// Выполняем запись знака конца строки внутри записи
		this->fold();
		/**
		 * Выполняем запись отступа перед строкой продолжения перечня
		 *
		 * @note Отступ этот украшающий и ставится наравне с отступом строки самой
		 *       пары: без него продолжение перечня уходило бы левее её начала
		 */
		this->indent();
		/**
		 * Выполняем запись отступа по глубине вложенности значения
		 *
		 * @note Уровень пары глубиной не считается: скобок он не несёт, и отступа
		 *       ему не полагается
		 */
		for(size_t i = 0; i < this->_levels.size(); i++){
			/**
			 * Если уровень вложенности несёт составное значение
			 */
			if(this->_levels.at(i).context != context_t::KEYED)
				// Выполняем запись отступа очередного уровня вложенности
				this->append("\t");
		}
	/**
	 * Если запись ведётся в одну строку
	 */
	} else if((level.count > 0) || (level.context == context_t::INLINE))
		// Выполняем запись пробела, отделяющего очередное значение
		this->append(" ");
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод завершения записи значения
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::complete() noexcept {
	/**
	 * Если записанное значение принадлежит паре
	 */
	if(!this->_levels.empty() && (this->_levels.back().context == context_t::KEYED)){
		// Выполняем снятие уровня вложенности записанной пары
		this->_levels.pop_back();
		/**
		 * Если пара записана строкой текста настроек
		 */
		if(this->_levels.empty()){
			// Запоминаем, что к записанной строке можно дописать примечание
			this->_trailable = true;
			// Выполняем запись знака конца строки
			return this->newline();
		}
		// Выполняем учёт записанной пары встроенной таблицы
		this->_levels.back().count++;
		// Выводим положительный результат выполнения операции
		return true;
	}
	/**
	 * Если записанное значение принадлежит перечню
	 */
	if(!this->_levels.empty() && (this->_levels.back().context == context_t::ARRAY)){
		// Выполняем учёт записанного значения перечня
		this->_levels.back().count++;
		// Выводим положительный результат выполнения операции
		return true;
	}
	// Запоминаем код ошибки записи
	this->_error = error_t::INTERNAL;
	// Выводим отрицательный результат выполнения операции
	return false;
}
/**
 * @brief Метод записи составной части имени ключа
 *
 * @param part записываемая составная часть имени ключа
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::naming(const part_t & part) noexcept {
	/**
	 * Если длина составной части имени превышает допустимую
	 */
	if((this->_settings.maxKey > 0) && (part.name.length() > static_cast <size_t> (this->_settings.maxKey))){
		// Запоминаем код ошибки записи
		this->_error = error_t::KEY_TOO_LONG;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Получаем запись, которой записывается составная часть имени
	naming_t naming = part.naming;
	/**
	 * Если имя записывается без кавычек
	 */
	if(naming == naming_t::BARE){
		// Признак пригодности имени к записи без кавычек
		bool bared = !part.name.empty();
		/**
		 * Выполняем перебор всех знаков имени
		 */
		for(size_t i = 0; bared && (i < part.name.length()); i++)
			/**
			 * Выполняем проверку знака на допустимость в имени без кавычек
			 *
			 * @note Знаки Юникода в имени без кавычек отводит лишь черновик следующей
			 *       версии описания, и признаются они по настройке читающего
			 */
			bared = (toml::bare(part.name[i]) ||
			         (this->_settings.unicode && (static_cast <uint8_t> (part.name[i]) >= 0x80)));
		/**
		 * Если имя к записи без кавычек непригодно
		 */
		if(!bared){
			/**
			 * Если смена ограды имени настройками не дозволена
			 */
			if(!this->_settings.promote){
				// Запоминаем код ошибки записи
				this->_error = (part.name.empty() ? error_t::EMPTY_KEY : error_t::INVALID_KEY);
				// Выводим отрицательный результат выполнения операции
				return false;
			}
			// Выполняем смену ограды имени на основную строку
			naming = naming_t::BASIC;
		}
	}
	/**
	 * Выполняем выбор записи составной части имени
	 */
	switch(static_cast <uint8_t> (naming)){
		// Если имя записывается без кавычек
		case static_cast <uint8_t> (naming_t::BARE): {
			// Выполняем запись имени без кавычек
			this->append(part.name);
			// Выводим положительный результат выполнения операции
			return true;
		}
		// Если имя записывается дословной строкой
		case static_cast <uint8_t> (naming_t::LITERAL):
			// Выполняем запись имени дословной строкой
			return this->quoted(part.name, string_t::LITERAL);
	}
	// Выполняем запись имени основной строкой
	return this->quoted(part.name, string_t::BASIC);
}
/**
 * @brief Метод записи составного имени ключа
 *
 * @param path записываемое составное имя ключа
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::naming(const vector <part_t> & path) noexcept {
	/**
	 * Если составное имя ключа пусто
	 */
	if(path.empty()){
		// Запоминаем код ошибки записи
		this->_error = error_t::EMPTY_KEY;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если количество составных частей имени превышает допустимое
	 */
	if((this->_settings.maxParts > 0) && (path.size() > static_cast <size_t> (this->_settings.maxParts))){
		// Запоминаем код ошибки записи
		this->_error = error_t::PARTS_EXCEEDED;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Выполняем перебор всех составных частей имени ключа
	 */
	for(size_t i = 0; i < path.size(); i++){
		/**
		 * Если записывается не первая составная часть имени
		 */
		if(i > 0)
			// Выполняем запись разделителя составных частей имени
			this->append(".");
		/**
		 * Если записать составную часть имени не удалось
		 */
		if(!this->naming(path.at(i)))
			// Выводим отрицательный результат выполнения операции
			return false;
	}
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод проверки возможности записи строки выбранной оградой
 *
 * @param text    проверяемое строковое значение
 * @param quoting проверяемая запись строкового значения
 * @return        результат проверки
 *
 */
bool awh::codec::toml::Writer::carried(const string_view text, const string_t quoting) const noexcept {
	/**
	 * Выполняем выбор проверяемой ограды строкового значения
	 */
	switch(static_cast <uint8_t> (quoting)){
		// Если строка записывается дословной оградой
		case static_cast <uint8_t> (string_t::LITERAL): {
			/**
			 * Выполняем перебор всех знаков строкового значения
			 */
			for(size_t i = 0; i < text.length(); i++){
				/**
				 * Если знаком является одинарная кавычка, знак конца строки либо управляющий знак
				 *
				 * @note Управляющих последовательностей дословная строка не признаёт, и
				 *       записать ими знак, оградой не несомый, там нечем
				 */
				if((text[i] == '\'') || (text[i] == '\n') || (text[i] == '\r') || controlled(text[i]))
					// Выводим отрицательный результат проверки
					return false;
			}
			// Выводим положительный результат проверки
			return true;
		}
		// Если строка записывается многострочной дословной оградой
		case static_cast <uint8_t> (string_t::MULTILINE_LITERAL): {
			/**
			 * Если строковое значение несёт возврат каретки
			 *
			 * @note Разбор приводит пару «возврат каретки - перевод строки» к одному
			 *       переводу, а управляющих последовательностей дословная ограда не
			 *       признаёт: записать возврат каретки ею нечем
			 */
			if(text.find('\r') != string_view::npos)
				// Выводим отрицательный результат проверки
				return false;
			/**
			 * Если строковое значение несёт закрывающую ограду
			 */
			if(text.find("'''") != string_view::npos)
				// Выводим отрицательный результат проверки
				return false;
			/**
			 * Если строковое значение оканчивается одинарной кавычкой
			 *
			 * @note Кавычка эта слилась бы с закрывающей оградой в четвёртую, и разбор
			 *       прочитал бы её частью значения - но лишь при огражденном значении в
			 *       одну кавычку, а не в две
			 */
			if(!text.empty() && (text.back() == '\''))
				// Выводим отрицательный результат проверки
				return false;
			/**
			 * Выполняем перебор всех знаков строкового значения
			 */
			for(size_t i = 0; i < text.length(); i++){
				/**
				 * Если знаком является управляющий знак
				 */
				if(controlled(text[i]))
					// Выводим отрицательный результат проверки
					return false;
			}
			// Выводим положительный результат проверки
			return true;
		}
	}
	/**
	 * Выводим положительный результат проверки
	 *
	 * @note Основная строка несёт всякое содержимое: знак, оградой не несомый,
	 *       записывается там управляющей последовательностью
	 */
	return true;
}
/**
 * @brief Метод записи строкового значения выбранной оградой
 *
 * @param text    записываемое строковое значение
 * @param quoting запись строкового значения
 * @return        результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::quoted(const string_view text, const string_t quoting) noexcept {
	// Получаем ограду, которой записывается строковое значение
	string_t selected = quoting;
	/**
	 * Если выбранная ограда содержимого строки не несёт
	 */
	if(!this->carried(text, selected)){
		/**
		 * Если смена ограды строки настройками не дозволена
		 */
		if(!this->_settings.promote){
			// Запоминаем код ошибки записи
			this->_error = error_t::INVALID_VALUE;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		/**
		 * Выполняем смену ограды строки на ближайшую, содержимое несущую
		 */
		selected = ((selected == string_t::MULTILINE_LITERAL) ? string_t::MULTILINE_BASIC : string_t::BASIC);
	}
	// Признак многострочной записи строкового значения
	const bool multiline = ((selected == string_t::MULTILINE_BASIC) || (selected == string_t::MULTILINE_LITERAL));
	// Признак дословной записи строкового значения
	const bool literal = ((selected == string_t::LITERAL) || (selected == string_t::MULTILINE_LITERAL));
	/**
	 * Если строка записывается дословной оградой
	 */
	if(literal){
		// Выполняем запись открывающей ограды строкового значения
		this->append(multiline ? "'''" : "'");
		/**
		 * Если строка записывается несколькими строками
		 *
		 * @note Знак конца строки за открывающей оградой разбор отбрасывает, и запись
		 *       его ставит всегда: без него значение, начинающееся знаком конца строки,
		 *       обратно прочиталось бы без него
		 */
		if(multiline)
			// Выполняем запись знака конца строки внутри записи
			this->fold();
		// Выполняем запись содержимого строкового значения
		this->append(text);
		// Выполняем запись закрывающей ограды строкового значения
		this->append(multiline ? "'''" : "'");
		// Выводим положительный результат выполнения операции
		return true;
	}
	// Выполняем запись открывающей ограды строкового значения
	this->append(multiline ? "\"\"\"" : "\"");
	/**
	 * Если строка записывается несколькими строками
	 */
	if(multiline)
		// Выполняем запись знака конца строки внутри записи
		this->fold();
	// Хранилище записи знака управляющей последовательностью
	char buffer[8];
	/**
	 * Выполняем перебор всех знаков строкового значения
	 */
	for(size_t i = 0; i < text.length(); i++){
		// Получаем очередной записываемый знак строкового значения
		const char letter = text[i];
		/**
		 * Выполняем выбор записываемого знака строкового значения
		 */
		switch(letter){
			// Если знаком является обратная косая черта
			case '\\': {
				// Выполняем запись управляющей последовательности знака
				this->append("\\\\");
				// Выполняем переход к следующему знаку
				continue;
			}
			// Если знаком является двойная кавычка
			case '"': {
				/**
				 * Если строка записывается несколькими строками
				 *
				 * @note Кавычка внутри многострочной ограды ограждения не требует:
				 *       требует его лишь та, что слилась бы с закрывающей оградой
				 */
				if(multiline && (text.compare(i, 3, "\"\"\"") != 0) && ((i + 1) < text.length())){
					// Выполняем запись двойной кавычки как она есть
					this->append(string_view(&text[i], 1));
					// Выполняем переход к следующему знаку
					continue;
				}
				// Выполняем запись управляющей последовательности знака
				this->append("\\\"");
				// Выполняем переход к следующему знаку
				continue;
			}
			// Если знаком является горизонтальная табуляция
			case '\t': {
				/**
				 * Если строка записывается несколькими строками
				 */
				if(multiline)
					// Выполняем запись горизонтальной табуляции как она есть
					this->append("\t");
				// Выполняем запись управляющей последовательности знака
				else this->append("\\t");
				// Выполняем переход к следующему знаку
				continue;
			}
			// Если знаком является перевод строки
			case '\n': {
				/**
				 * Если строка записывается несколькими строками
				 */
				if(multiline){
					// Выполняем запись знака конца строки внутри записи
					this->fold();
					// Выполняем переход к следующему знаку
					continue;
				}
				// Выполняем запись управляющей последовательности знака
				this->append("\\n");
				// Выполняем переход к следующему знаку
				continue;
			}
			// Если знаком является возврат каретки
			case '\r': {
				/**
				 * Выполняем запись управляющей последовательности знака
				 *
				 * @note Последовательность ставится и в многострочной ограде: разбор
				 *       приводит пару «возврат каретки - перевод строки» к одному
				 *       переводу, и записанный собою возврат каретки при обратном
				 *       чтении пропадал бы молча
				 */
				this->append("\\r");
				// Выполняем переход к следующему знаку
				continue;
			}
			// Если знаком является забой
			case '\b': {
				// Выполняем запись управляющей последовательности знака
				this->append("\\b");
				// Выполняем переход к следующему знаку
				continue;
			}
			// Если знаком является перевод страницы
			case '\f': {
				// Выполняем запись управляющей последовательности знака
				this->append("\\f");
				// Выполняем переход к следующему знаку
				continue;
			}
		}
		/**
		 * Если знак является управляющим
		 */
		if(controlled(letter)){
			// Выполняем запись управляющей последовательности знака его кодом
			const int32_t length = ::snprintf(buffer, sizeof(buffer), "\\u%04X", static_cast <uint32_t> (static_cast <uint8_t> (letter)));
			/**
			 * Если запись управляющей последовательности выполнить не удалось
			 */
			if(length <= 0){
				// Запоминаем код ошибки записи
				this->_error = error_t::INTERNAL;
				// Выводим отрицательный результат выполнения операции
				return false;
			}
			// Выполняем запись управляющей последовательности знака
			this->append(string_view(buffer, static_cast <size_t> (length)));
			// Выполняем переход к следующему знаку
			continue;
		}
		// Выполняем запись знака строкового значения как он есть
		this->append(string_view(&text[i], 1));
	}
	// Выполняем запись закрывающей ограды строкового значения
	this->append(multiline ? "\"\"\"" : "\"");
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи отметки времени
 *
 * @param stamp записываемая отметка времени
 * @param type  тип записываемой отметки времени
 * @return      результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::stamped(const stamp_t & stamp, const type_t type) noexcept {
	// Хранилище записи отметки времени
	char buffer[64];
	// Признак записи даты отметки времени
	const bool dated = ((type == type_t::LOCAL_DATE) || (type == type_t::LOCAL_DATETIME) || (type == type_t::OFFSET_DATETIME));
	// Признак записи времени отметки
	const bool timed = ((type == type_t::LOCAL_TIME) || (type == type_t::LOCAL_DATETIME) || (type == type_t::OFFSET_DATETIME));
	/**
	 * Если записывается дата отметки времени
	 */
	if(dated){
		/**
		 * Если построение даты ошибочно
		 */
		if((stamp.date.year > 9999) || (stamp.date.month < 1) || (stamp.date.month > 12) ||
		   (stamp.date.day < 1) || (stamp.date.day > 31)){
			// Запоминаем код ошибки записи
			this->_error = error_t::INVALID_DATETIME;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Выполняем запись даты отметки времени
		const int32_t length = ::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u",
		 static_cast <uint32_t> (stamp.date.year), static_cast <uint32_t> (stamp.date.month), static_cast <uint32_t> (stamp.date.day));
		/**
		 * Если запись даты отметки времени выполнить не удалось
		 */
		if(length <= 0){
			// Запоминаем код ошибки записи
			this->_error = error_t::INTERNAL;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Выполняем запись даты отметки времени
		this->append(string_view(buffer, static_cast <size_t> (length)));
	}
	/**
	 * Если записываются и дата, и время отметки
	 */
	if(dated && timed)
		// Выполняем запись разделителя даты и времени отметки
		this->append(stamp.spaced ? " " : "T");
	/**
	 * Если записывается время отметки
	 */
	if(timed){
		/**
		 * Если построение времени ошибочно
		 *
		 * @note Секунда в шестьдесят принимается намеренно: описание дозволяет ею
		 *       записывать добавочную секунду координации
		 */
		if((stamp.time.hour > 23) || (stamp.time.minute > 59) || (stamp.time.second > 60) ||
		   (stamp.time.digits > MAX_FRACTION) || (stamp.time.nanosecond > 999999999)){
			// Запоминаем код ошибки записи
			this->_error = error_t::INVALID_DATETIME;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Выполняем запись времени отметки
		int32_t length = ::snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u",
		 static_cast <uint32_t> (stamp.time.hour), static_cast <uint32_t> (stamp.time.minute), static_cast <uint32_t> (stamp.time.second));
		/**
		 * Если запись времени отметки выполнить не удалось
		 */
		if(length <= 0){
			// Запоминаем код ошибки записи
			this->_error = error_t::INTERNAL;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Выполняем запись времени отметки
		this->append(string_view(buffer, static_cast <size_t> (length)));
		/**
		 * Если у времени отметки записана доля секунды
		 */
		if(stamp.time.digits > 0){
			// Хранилище записи доли секунды наносекундами
			char fraction[16];
			// Выполняем запись доли секунды наносекундами
			length = ::snprintf(fraction, sizeof(fraction), "%09u", static_cast <uint32_t> (stamp.time.nanosecond));
			/**
			 * Если запись доли секунды выполнить не удалось
			 */
			if(length != static_cast <int32_t> (MAX_FRACTION)){
				// Запоминаем код ошибки записи
				this->_error = error_t::INTERNAL;
				// Выводим отрицательный результат выполнения операции
				return false;
			}
			// Выполняем запись разделителя доли секунды
			this->append(".");
			/**
			 * Выполняем запись доли секунды разрядами, записанными человеком
			 *
			 * @note Разряды эти значащи: «01:02:03.100» и «01:02:03.1» задают одно и то
			 *       же время, но выбраны они человеком, и подменять их не следует
			 */
			this->append(string_view(fraction, static_cast <size_t> (stamp.time.digits)));
		}
	}
	/**
	 * Если записывается смещение часового пояса
	 */
	if(type == type_t::OFFSET_DATETIME){
		/**
		 * Если смещение часового пояса отсутствует
		 */
		if(stamp.offset == NO_TIMEZONE){
			// Запоминаем код ошибки записи
			this->_error = error_t::INVALID_DATETIME;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		/**
		 * Если часовой пояс записывается знаком «Z»
		 */
		if(stamp.zulu && (stamp.offset == 0)){
			// Выполняем запись часового пояса UTC
			this->append("Z");
			// Выводим положительный результат выполнения операции
			return true;
		}
		/**
		 * Если смещение часового пояса выходит за отведённый ему отрезок значений
		 */
		if((stamp.offset < -(23 * 60 + 59)) || (stamp.offset > (23 * 60 + 59))){
			// Запоминаем код ошибки записи
			this->_error = error_t::INVALID_DATETIME;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Получаем величину смещения часового пояса в минутах
		const uint32_t value = static_cast <uint32_t> (stamp.offset < 0 ? -stamp.offset : stamp.offset);
		// Выполняем запись смещения часового пояса
		const int32_t length = ::snprintf(buffer, sizeof(buffer), "%c%02u:%02u",
		 ((stamp.offset < 0) ? '-' : '+'), (value / 60), (value % 60));
		/**
		 * Если запись смещения часового пояса выполнить не удалось
		 */
		if(length <= 0){
			// Запоминаем код ошибки записи
			this->_error = error_t::INTERNAL;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Выполняем запись смещения часового пояса
		this->append(string_view(buffer, static_cast <size_t> (length)));
	}
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи объявления таблицы
 *
 * @param path  записываемое имя таблицы
 * @param array признак объявления очередной таблицы набора таблиц
 * @return      результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::declare(const vector <part_t> & path, const bool array) noexcept {
	/**
	 * Если запись очередной строки текста не готова
	 */
	if(!this->ready())
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если запись пустой строки перед объявлением таблицы настройками задана
	 *
	 * @note Пустая строка не ставится ни в начале текста, ни следом за уже
	 *       записанной пустой строкой: иначе текст рос бы пустыми строками с каждой
	 *       перезаписью
	 */
	if(this->_settings.separated && !this->_text.empty()){
		// Получаем последовательность знаков конца строки собираемого текста
		const string_view newline = toml::newline(this->_settings.newline);
		// Получаем длину пары знаков конца строки
		const size_t length = (newline.length() * 2);
		/**
		 * Если собранный текст пустой строкой не оканчивается
		 */
		if((this->_text.length() < length) || (this->_text.compare(this->_text.length() - length, length, string(newline).append(newline)) != 0)){
			/**
			 * Если записать пустую строку не удалось
			 */
			if(!this->blank())
				// Выводим отрицательный результат выполнения операции
				return false;
		}
	}
	// Выполняем запись открывающей скобки объявления таблицы
	this->append(array ? "[[" : "[");
	/**
	 * Если записать имя таблицы не удалось
	 */
	if(!this->naming(path))
		// Выводим отрицательный результат выполнения операции
		return false;
	// Выполняем запись закрывающей скобки объявления таблицы
	this->append(array ? "]]" : "]");
	// Запоминаем, что таблица текста настроек объявлена
	this->_tabled = true;
	// Запоминаем, что к записанной строке можно дописать примечание
	this->_trailable = true;
	// Выполняем запись знака конца строки
	return this->newline();
}
/**
 * @brief Метод получения текущих настроек записи
 *
 * @return текущие настройки записи текста настроек
 *
 */
const awh::codec::toml::Writer::settings_t & awh::codec::toml::Writer::settings() const noexcept {
	// Выводим текущие настройки записи текста настроек
	return this->_settings;
}
/**
 * @brief Метод установки настроек записи
 *
 * @param settings настройки записи текста настроек
 *
 */
void awh::codec::toml::Writer::settings(const settings_t & settings) noexcept {
	// Выполняем установку настроек записи текста настроек
	this->_settings = settings;
}
/**
 * @brief Метод записи объявления таблицы
 *
 * @param path записываемое составное имя таблицы
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::table(const vector <part_t> & path) noexcept {
	// Выполняем запись объявления таблицы
	return this->declare(path, false);
}
/**
 * @brief Метод записи объявления таблицы
 *
 * @param name записываемое имя таблицы
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::table(const string_view name) noexcept {
	// Составное имя записываемой таблицы
	vector <part_t> path(1);
	// Устанавливаем имя записываемой таблицы
	path.front().name = name;
	// Выполняем запись объявления таблицы
	return this->declare(path, false);
}
/**
 * @brief Метод записи объявления очередной таблицы набора таблиц
 *
 * @param path записываемое составное имя набора таблиц
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::arrayTable(const vector <part_t> & path) noexcept {
	// Выполняем запись объявления очередной таблицы набора таблиц
	return this->declare(path, true);
}
/**
 * @brief Метод записи объявления очередной таблицы набора таблиц
 *
 * @param name записываемое имя набора таблиц
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::arrayTable(const string_view name) noexcept {
	// Составное имя записываемого набора таблиц
	vector <part_t> path(1);
	// Устанавливаем имя записываемого набора таблиц
	path.front().name = name;
	// Выполняем запись объявления очередной таблицы набора таблиц
	return this->declare(path, true);
}
/**
 * @brief Метод записи имени ключа пары
 *
 * @param path записываемое составное имя ключа
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::key(const vector <part_t> & path) noexcept {
	/**
	 * Выполняем выбор окружения, в котором ведётся запись
	 */
	switch(static_cast <uint8_t> (this->context())){
		// Если запись ведётся строками текста настроек
		case static_cast <uint8_t> (context_t::ROOT): {
			// Выполняем запись отступа перед именем ключа
			this->indent();
		} break;
		// Если запись ведётся внутри встроенной таблицы
		case static_cast <uint8_t> (context_t::INLINE): {
			/**
			 * Если записать разделитель пар встроенной таблицы не удалось
			 */
			if(!this->separate())
				// Выводим отрицательный результат выполнения операции
				return false;
		} break;
		// Если запись ведётся в ином окружении
		default: {
			/**
			 * Запоминаем код ошибки записи
			 *
			 * @note Имени ключа посреди перечня места нет: значения перечня имён не
			 *       имеют, а незаписанное значение пары означает пропущенный вызов
			 */
			this->_error = ((this->context() == context_t::KEYED) ? error_t::MISSING_VALUE : error_t::INVALID_KEY);
			// Выводим отрицательный результат выполнения операции
			return false;
		}
	}
	/**
	 * Если записать имя ключа не удалось
	 */
	if(!this->naming(path))
		// Выводим отрицательный результат выполнения операции
		return false;
	// Выполняем запись знака равенства с пробелами вокруг него
	this->append(this->_settings.spaces ? " = " : "=");
	// Выполняем добавление уровня вложенности записываемой пары
	this->_levels.push_back(level_t(context_t::KEYED, false));
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи имени ключа пары
 *
 * @param name записываемое имя ключа
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::key(const string_view name) noexcept {
	// Составное имя записываемого ключа
	vector <part_t> path(1);
	// Устанавливаем имя записываемого ключа
	path.front().name = name;
	// Выполняем запись имени ключа пары
	return this->key(path);
}
/**
 * @brief Метод записи значения
 *
 * @param value записываемое значение
 * @return      результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::value(const value_t & value) noexcept {
	/**
	 * Выполняем выбор типа записываемого значения
	 */
	switch(static_cast <uint8_t> (value.type)){
		// Если записывается последовательность знаков
		case static_cast <uint8_t> (type_t::STRING):
			// Выполняем запись строкового значения
			return this->text(value.text, value.quoting);
		// Если записывается логическое значение
		case static_cast <uint8_t> (type_t::BOOLEAN):
			// Выполняем запись логического значения
			return this->boolean(value.boolean);
		// Если записывается целое число
		case static_cast <uint8_t> (type_t::INTEGER):
			// Выполняем запись целого числа
			return this->integer(value.integer, value.radix);
		// Если записывается число с плавающей точкой
		case static_cast <uint8_t> (type_t::FLOAT):
			// Выполняем запись числа с плавающей точкой
			return this->real(value.real);
		// Если записывается отметка времени со смещением часового пояса
		case static_cast <uint8_t> (type_t::OFFSET_DATETIME):
		// Если записывается отметка времени без смещения часового пояса
		case static_cast <uint8_t> (type_t::LOCAL_DATETIME):
		// Если записывается местная дата
		case static_cast <uint8_t> (type_t::LOCAL_DATE):
		// Если записывается местное время
		case static_cast <uint8_t> (type_t::LOCAL_TIME):
			// Выполняем запись отметки времени
			return this->stamp(value.stamp, value.type);
	}
	/**
	 * Запоминаем код ошибки записи
	 *
	 * @note Перечень и таблица значениями не записываются: составное значение
	 *       собирается парой вызовов открытия и закрытия
	 */
	this->_error = error_t::INVALID_VALUE;
	// Выводим отрицательный результат выполнения операции
	return false;
}
/**
 * @brief Метод записи строкового значения
 *
 * @param text    записываемое строковое значение
 * @param quoting запись строкового значения
 * @return        результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::text(const string_view text, const string_t quoting) noexcept {
	/**
	 * Если запись значения в текущем окружении невозможна
	 */
	if(this->context() == context_t::ARRAY){
		/**
		 * Если записать разделитель значений перечня не удалось
		 */
		if(!this->separate())
			// Выводим отрицательный результат выполнения операции
			return false;
	// Если запись значения ведётся вне пары и вне перечня
	} else if(this->context() != context_t::KEYED) {
		// Запоминаем код ошибки записи
		this->_error = error_t::UNEXPECTED_CONTENT;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если записать строковое значение не удалось
	 */
	if(!this->quoted(text, quoting))
		// Выводим отрицательный результат выполнения операции
		return false;
	// Выполняем завершение записи значения
	return this->complete();
}
/**
 * @brief Метод записи логического значения
 *
 * @param value записываемое логическое значение
 * @return      результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::boolean(const bool value) noexcept {
	/**
	 * Если запись значения ведётся внутри перечня
	 */
	if(this->context() == context_t::ARRAY){
		/**
		 * Если записать разделитель значений перечня не удалось
		 */
		if(!this->separate())
			// Выводим отрицательный результат выполнения операции
			return false;
	// Если запись значения ведётся вне пары и вне перечня
	} else if(this->context() != context_t::KEYED) {
		// Запоминаем код ошибки записи
		this->_error = error_t::UNEXPECTED_CONTENT;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем запись логического значения
	this->append(value ? "true" : "false");
	// Выполняем завершение записи значения
	return this->complete();
}
/**
 * @brief Метод записи целого числа
 *
 * @param value записываемое целое число
 * @param radix система счисления записи числа
 * @return      результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::integer(const int64_t value, const radix_t radix) noexcept {
	/**
	 * Если запись значения ведётся внутри перечня
	 */
	if(this->context() == context_t::ARRAY){
		/**
		 * Если записать разделитель значений перечня не удалось
		 */
		if(!this->separate())
			// Выводим отрицательный результат выполнения операции
			return false;
	// Если запись значения ведётся вне пары и вне перечня
	} else if(this->context() != context_t::KEYED) {
		// Запоминаем код ошибки записи
		this->_error = error_t::UNEXPECTED_CONTENT;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если число записывается не десятичной системой счисления и оно отрицательно
	 *
	 * @note Описание отводит знак числа лишь десятичной записи: записать
	 *       отрицательное число шестнадцатеричным нечем, и молча сменить систему
	 *       счисления запись не вправе - человек выбрал её сам
	 */
	if((radix != radix_t::DECIMAL) && (value < 0)){
		// Запоминаем код ошибки записи
		this->_error = error_t::INVALID_NUMBER;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Хранилище записи целого числа
	char buffer[80];
	// Длина записи целого числа
	int32_t length = 0;
	/**
	 * Выполняем выбор системы счисления записи целого числа
	 */
	switch(static_cast <uint8_t> (radix)){
		// Если число записывается шестнадцатеричной системой счисления
		case static_cast <uint8_t> (radix_t::HEX):
			// Выполняем запись числа шестнадцатеричной системой счисления
			length = ::snprintf(buffer, sizeof(buffer), "0x%llX", static_cast <unsigned long long> (value));
		break;
		// Если число записывается восьмеричной системой счисления
		case static_cast <uint8_t> (radix_t::OCTAL):
			// Выполняем запись числа восьмеричной системой счисления
			length = ::snprintf(buffer, sizeof(buffer), "0o%llo", static_cast <unsigned long long> (value));
		break;
		// Если число записывается двоичной системой счисления
		case static_cast <uint8_t> (radix_t::BINARY): {
			// Получаем записываемое двоичной системой счисления число
			uint64_t number = static_cast <uint64_t> (value);
			// Выполняем запись приставки двоичной системы счисления
			buffer[length++] = '0';
			// Выполняем запись обозначения двоичной системы счисления
			buffer[length++] = 'b';
			/**
			 * Если записываемое число равно нулю
			 */
			if(number == 0)
				// Выполняем запись нулевого значения числа
				buffer[length++] = '0';
			/**
			 * Если записываемое число нулю не равно
			 */
			else {
				// Получаем номер старшего значащего разряда числа
				int32_t bit = 63;
				/**
				 * Выполняем поиск старшего значащего разряда числа
				 */
				while((bit > 0) && (((number >> bit) & 1) == 0))
					// Выполняем переход к следующему разряду числа
					bit--;
				/**
				 * Выполняем запись всех значащих разрядов числа
				 */
				for(; bit >= 0; bit--)
					// Выполняем запись очередного разряда числа
					buffer[length++] = static_cast <char> ('0' + ((number >> bit) & 1));
			}
		} break;
		// Если число записывается десятичной системой счисления
		default: length = ::snprintf(buffer, sizeof(buffer), "%lld", static_cast <long long> (value));
	}
	/**
	 * Если запись целого числа выполнить не удалось
	 */
	if((length <= 0) || (static_cast <size_t> (length) >= sizeof(buffer))){
		// Запоминаем код ошибки записи
		this->_error = error_t::INTERNAL;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем запись целого числа
	this->append(string_view(buffer, static_cast <size_t> (length)));
	// Выполняем завершение записи значения
	return this->complete();
}
/**
 * @brief Метод записи числа с плавающей точкой
 *
 * @param value записываемое число с плавающей точкой
 * @return      результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::real(const double value) noexcept {
	/**
	 * Если запись значения ведётся внутри перечня
	 */
	if(this->context() == context_t::ARRAY){
		/**
		 * Если записать разделитель значений перечня не удалось
		 */
		if(!this->separate())
			// Выводим отрицательный результат выполнения операции
			return false;
	// Если запись значения ведётся вне пары и вне перечня
	} else if(this->context() != context_t::KEYED) {
		// Запоминаем код ошибки записи
		this->_error = error_t::UNEXPECTED_CONTENT;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если записываемое число не является числом
	 */
	if(std::isnan(value)){
		/**
		 * Выполняем запись обозначения нечисла
		 *
		 * @note Знак у нечисла описанием дозволен, но смысла не несёт, и запись его
		 *       не ставит
		 */
		this->append("nan");
		// Выполняем завершение записи значения
		return this->complete();
	}
	/**
	 * Если записываемое число бесконечно
	 */
	if(std::isinf(value)){
		// Выполняем запись обозначения бесконечности
		this->append((value < 0) ? "-inf" : "inf");
		// Выполняем завершение записи значения
		return this->complete();
	}
	// Хранилище записи числа с плавающей точкой
	char buffer[64];
	// Длина записи числа с плавающей точкой
	int32_t length = 0;
	/**
	 * Выполняем подбор кратчайшей записи числа с плавающей точкой
	 *
	 * @note Точность наращивается от единицы до наибольшей, какую тип несёт, и
	 *       берётся первая запись, читающаяся обратно тем же числом: запись
	 *       наибольшей точностью оборот переживает, но выдаёт «0.1» как
	 *       «0.10000000000000001» - в файле настроек, писанном для человека, такое
	 *       чтению не подлежит
	 */
	for(int32_t digits = 1; digits <= static_cast <int32_t> (numeric_limits <double>::max_digits10); digits++){
		// Выполняем запись числа с плавающей точкой очередной точностью
		length = ::snprintf(buffer, sizeof(buffer), "%.*g", digits, value);
		/**
		 * Если запись числового значения выполнить не удалось
		 */
		if((length <= 0) || (static_cast <size_t> (length) >= sizeof(buffer)))
			// Выполняем прекращение подбора точности записи
			break;
		// Выполняем замену знака десятичной точки локали точкой
		::pointed(buffer, length);
		// Прочитанное обратно значение записанного числа
		double back = 0.0;
		/**
		 * Если запись читается обратно тем же самым числом
		 */
		if(::numeric(string_view(buffer, static_cast <size_t> (length)), back) && (back == value))
			// Выполняем прекращение подбора точности записи
			break;
	}
	/**
	 * Если запись числа выполнить не удалось
	 */
	if((length <= 0) || (static_cast <size_t> (length) >= sizeof(buffer))){
		// Запоминаем код ошибки записи
		this->_error = error_t::INTERNAL;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем запись числа с плавающей точкой
	this->append(string_view(buffer, static_cast <size_t> (length)));
	/**
	 * Если запись числа не несёт ни десятичной точки, ни показателя
	 *
	 * @note Дробная часть здесь не украшение: «1» описание читает целым числом, и
	 *       без неё число с плавающей точкой при обратном чтении сменило бы тип
	 */
	if((::memchr(buffer, '.', static_cast <size_t> (length)) == nullptr) &&
	   (::memchr(buffer, 'e', static_cast <size_t> (length)) == nullptr) &&
	   (::memchr(buffer, 'E', static_cast <size_t> (length)) == nullptr))
		// Выполняем запись нулевой дробной части числа
		this->append(".0");
	// Выполняем завершение записи значения
	return this->complete();
}
/**
 * @brief Метод записи отметки времени
 *
 * @param stamp записываемая отметка времени
 * @param type  тип записываемой отметки времени
 * @return      результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::stamp(const stamp_t & stamp, const type_t type) noexcept {
	/**
	 * Если запись значения ведётся внутри перечня
	 */
	if(this->context() == context_t::ARRAY){
		/**
		 * Если записать разделитель значений перечня не удалось
		 */
		if(!this->separate())
			// Выводим отрицательный результат выполнения операции
			return false;
	// Если запись значения ведётся вне пары и вне перечня
	} else if(this->context() != context_t::KEYED) {
		// Запоминаем код ошибки записи
		this->_error = error_t::UNEXPECTED_CONTENT;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если записываемый тип отметкой времени не является
	 */
	if((type != type_t::OFFSET_DATETIME) && (type != type_t::LOCAL_DATETIME) &&
	   (type != type_t::LOCAL_DATE) && (type != type_t::LOCAL_TIME)){
		// Запоминаем код ошибки записи
		this->_error = error_t::INVALID_DATETIME;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если записать отметку времени не удалось
	 */
	if(!this->stamped(stamp, type))
		// Выводим отрицательный результат выполнения операции
		return false;
	// Выполняем завершение записи значения
	return this->complete();
}
/**
 * @brief Метод записи начала перечня значений
 *
 * @param multiline признак записи перечня несколькими строками
 * @return          результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::arrayOpen(const bool multiline) noexcept {
	/**
	 * Если запись значения ведётся внутри перечня
	 */
	if(this->context() == context_t::ARRAY){
		/**
		 * Если записать разделитель значений перечня не удалось
		 */
		if(!this->separate())
			// Выводим отрицательный результат выполнения операции
			return false;
	// Если запись значения ведётся вне пары и вне перечня
	} else if(this->context() != context_t::KEYED) {
		// Запоминаем код ошибки записи
		this->_error = error_t::UNEXPECTED_CONTENT;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если глубина вложенности значения превышает допустимую
	 */
	if((this->_settings.maxDepth > 0) && (this->_levels.size() >= static_cast <size_t> (this->_settings.maxDepth))){
		// Запоминаем код ошибки записи
		this->_error = error_t::DEPTH_EXCEEDED;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем запись открывающей скобки перечня значений
	this->append("[");
	// Выполняем добавление уровня вложенности записываемого перечня
	this->_levels.push_back(level_t(context_t::ARRAY, multiline));
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи конца перечня значений
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::arrayClose() noexcept {
	/**
	 * Если запись ведётся вне перечня значений
	 */
	if(this->context() != context_t::ARRAY){
		// Запоминаем код ошибки записи
		this->_error = error_t::UNCLOSED_ARRAY;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если перечень записан несколькими строками и значения в нём записаны
	 */
	if(this->_levels.back().multiline && (this->_levels.back().count > 0)){
		// Выполняем запись знака конца строки внутри записи
		this->fold();
		// Выполняем запись отступа перед закрывающей скобкой перечня
		this->indent();
		/**
		 * Выполняем запись отступа по глубине вложенности закрываемого перечня
		 *
		 * @note Отступ закрывающей скобки на один уровень мельче отступа значений:
		 *       скобка эта принадлежит объемлющему уровню, а не закрываемому
		 */
		for(size_t i = 0; (i + 1) < this->_levels.size(); i++){
			/**
			 * Если уровень вложенности несёт составное значение
			 */
			if(this->_levels.at(i).context != context_t::KEYED)
				// Выполняем запись отступа очередного уровня вложенности
				this->append("\t");
		}
	}
	// Выполняем запись закрывающей скобки перечня значений
	this->append("]");
	// Выполняем снятие уровня вложенности записанного перечня
	this->_levels.pop_back();
	// Выполняем завершение записи значения
	return this->complete();
}
/**
 * @brief Метод записи начала встроенной таблицы
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::inlineOpen() noexcept {
	/**
	 * Если запись значения ведётся внутри перечня
	 */
	if(this->context() == context_t::ARRAY){
		/**
		 * Если записать разделитель значений перечня не удалось
		 */
		if(!this->separate())
			// Выводим отрицательный результат выполнения операции
			return false;
	// Если запись значения ведётся вне пары и вне перечня
	} else if(this->context() != context_t::KEYED) {
		// Запоминаем код ошибки записи
		this->_error = error_t::UNEXPECTED_CONTENT;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если глубина вложенности значения превышает допустимую
	 */
	if((this->_settings.maxDepth > 0) && (this->_levels.size() >= static_cast <size_t> (this->_settings.maxDepth))){
		// Запоминаем код ошибки записи
		this->_error = error_t::DEPTH_EXCEEDED;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем запись открывающей скобки встроенной таблицы
	this->append("{");
	/**
	 * Выполняем добавление уровня вложенности записываемой встроенной таблицы
	 *
	 * @note Многострочной встроенная таблица не бывает: описание переноса строки
	 *       внутри неё не дозволяет вовсе
	 */
	this->_levels.push_back(level_t(context_t::INLINE, false));
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод записи конца встроенной таблицы
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::inlineClose() noexcept {
	/**
	 * Если запись ведётся вне встроенной таблицы
	 */
	if(this->context() != context_t::INLINE){
		// Запоминаем код ошибки записи
		this->_error = error_t::UNCLOSED_INLINE_TABLE;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Если во встроенной таблице записаны пары
	 */
	if(this->_levels.back().count > 0)
		// Выполняем запись пробела перед закрывающей скобкой
		this->append(" ");
	// Выполняем запись закрывающей скобки встроенной таблицы
	this->append("}");
	// Выполняем снятие уровня вложенности записанной встроенной таблицы
	this->_levels.pop_back();
	// Выполняем завершение записи значения
	return this->complete();
}
/**
 * @brief Метод записи примечания
 *
 * @param text содержимое записываемого примечания
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::comment(const string_view text) noexcept {
	/**
	 * Если запись очередной строки текста не готова
	 */
	if(!this->ready())
		// Выводим отрицательный результат выполнения операции
		return false;
	// Положение начала очередной строки примечания
	size_t start = 0;
	/**
	 * Выполняем запись всех строк примечания
	 */
	do {
		// Выполняем поиск конца очередной строки примечания
		size_t stop = text.find('\n', start);
		/**
		 * Если конец очередной строки примечания не найден
		 */
		if(stop == string_view::npos)
			// Запоминаем концом строки конец содержимого примечания
			stop = text.length();
		// Получаем содержимое очередной строки примечания
		string_view line = text.substr(start, stop - start);
		/**
		 * Если строка примечания оканчивается возвратом каретки
		 */
		if(!line.empty() && (line.back() == '\r'))
			// Выполняем отбрасывание возврата каретки строки примечания
			line.remove_suffix(1);
		/**
		 * Выполняем перебор всех знаков строки примечания
		 */
		for(size_t i = 0; i < line.length(); i++){
			/**
			 * Если знак строки примечания является управляющим
			 *
			 * @note Оградить управляющий знак в примечании нечем: управляющих
			 *       последовательностей примечание не признаёт, а обрезать содержимое
			 *       запись не вправе
			 */
			if(controlled(line[i])){
				// Запоминаем код ошибки записи
				this->_error = error_t::INVALID_CHARACTER;
				// Выводим отрицательный результат выполнения операции
				return false;
			}
		}
		// Выполняем запись знака начала примечания
		this->append("#");
		/**
		 * Если содержимое строки примечания не пусто
		 */
		if(!line.empty()){
			// Выполняем запись пробела, отделяющего содержимое примечания
			this->append(" ");
			// Выполняем запись содержимого строки примечания
			this->append(line);
		}
		/**
		 * Если записать знак конца строки не удалось
		 */
		if(!this->newline())
			// Выводим отрицательный результат выполнения операции
			return false;
		// Выполняем переход к следующей строке примечания
		start = (stop + 1);
	} while(start <= text.length());
	/**
	 * Запоминаем, что к записанной строке примечание дописать нельзя
	 *
	 * @note Строка примечания примечанием и является: дописывать к ней второе
	 *       примечание нечем - знак его начала достался бы содержимому первого
	 */
	this->_trailable = false;
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод дописывания примечания к последней записанной строке
 *
 * @param text содержимое дописываемого примечания
 * @return     результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::trailing(const string_view text) noexcept {
	/**
	 * Если запись очередной строки текста не готова
	 */
	if(!this->ready())
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если к последней записанной строке примечание дописать нельзя
	 */
	if(!this->_trailable || this->_text.empty()){
		// Запоминаем код ошибки записи
		this->_error = error_t::UNEXPECTED_CONTENT;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Выполняем перебор всех знаков дописываемого примечания
	 */
	for(size_t i = 0; i < text.length(); i++){
		/**
		 * Если знаком примечания является знак конца строки либо управляющий знак
		 *
		 * @note Примечание в конце строки занимает одну строку по устройству своему:
		 *       знак конца строки внутри него оборвал бы его на первой же строке, а
		 *       хвост достался бы тексту настроек содержимым
		 */
		if((text[i] == '\n') || controlled(text[i])){
			// Запоминаем код ошибки записи
			this->_error = error_t::INVALID_CHARACTER;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
	}
	// Получаем последовательность знаков конца строки собираемого текста
	const string_view newline = toml::newline(this->_settings.newline);
	/**
	 * Если собранный текст оканчивается знаком конца строки
	 */
	if((this->_text.length() >= newline.length()) &&
	   (this->_text.compare(this->_text.length() - newline.length(), newline.length(), newline) == 0)){
		// Выполняем снятие знака конца строки собранного текста
		this->_text.erase(this->_text.length() - newline.length());
		// Выполняем поиск начала последней записанной строки текста
		const size_t position = this->_text.rfind('\n');
		// Восстанавливаем длину собираемой логической строки
		this->_length = ((position == string::npos) ? this->_text.length() : (this->_text.length() - (position + 1)));
	}
	// Выполняем запись пробела, отделяющего примечание от содержимого строки
	this->append(" #");
	/**
	 * Если содержимое дописываемого примечания не пусто
	 */
	if(!text.empty()){
		// Выполняем запись пробела, отделяющего содержимое примечания
		this->append(" ");
		// Выполняем запись содержимого примечания
		this->append(text);
	}
	// Запоминаем, что к записанной строке примечание дописать нельзя
	this->_trailable = false;
	// Выполняем запись знака конца строки
	return this->newline();
}
/**
 * @brief Метод записи пустой строки
 *
 * @return результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::blank() noexcept {
	/**
	 * Если запись очередной строки текста не готова
	 */
	if(!this->ready())
		// Выводим отрицательный результат выполнения операции
		return false;
	// Запоминаем, что к записанной строке примечание дописать нельзя
	this->_trailable = false;
	// Выполняем запись знака конца строки
	return this->newline();
}
/**
 * @brief Метод получения кода ошибки записи
 *
 * @return код ошибки последней операции записи
 *
 */
awh::codec::toml::error_t awh::codec::toml::Writer::error() const noexcept {
	// Выводим код ошибки последней операции записи
	return this->_error;
}
/**
 * @brief Метод получения собранного текста настроек
 *
 * @return собранный текст настроек
 *
 */
const string & awh::codec::toml::Writer::text() noexcept {
	// Пустой результат выдачи собранного текста настроек
	static const string result;
	/**
	 * Если собираемый текст настроек не дописан
	 */
	if(!this->ready())
		// Выводим пустой результат выдачи собранного текста настроек
		return result;
	// Выводим собранный текст настроек
	return this->_text;
}
/**
 * @brief Метод сброса записи в исходное состояние
 *
 */
void awh::codec::toml::Writer::clear() noexcept {
	// Выполняем сброс кода ошибки записи
	this->_error = error_t::NONE;
	// Выполняем сброс признака объявления таблицы
	this->_tabled = false;
	// Выполняем сброс признака возможности дописать примечание
	this->_trailable = false;
	// Выполняем сброс длины собираемой логической строки
	this->_length = 0;
	// Выполняем очистку собираемого текста настроек
	this->_text.clear();
	// Выполняем очистку стопы уровней вложенности
	this->_levels.clear();
}
/**
 * @brief Конструктор
 *
 */
awh::codec::toml::Writer::Writer() noexcept :
 _error(error_t::NONE), _tabled(false), _trailable(false), _length(0) {}
/**
 * @brief Конструктор
 *
 * @param settings настройки записи текста настроек
 *
 */
awh::codec::toml::Writer::Writer(const settings_t & settings) noexcept :
 _error(error_t::NONE), _tabled(false), _trailable(false), _length(0), _settings(settings) {}
/**
 * @brief Деструктор
 *
 */
awh::codec::toml::Writer::~Writer() noexcept {
	// Выполняем очистку собираемого текста настроек
	this->_text.clear();
	// Выполняем очистку стопы уровней вложенности
	this->_levels.clear();
}

/**
 * @brief Шаблон типа записываемого числа
 *
 * @tparam T тип записываемого числа
 *
 */
template <typename T>
/**
 * @brief Метод записи пары с числовым значением
 *
 * @param key   имя записываемого ключа
 * @param value значение записываемой пары
 * @return      результат выполнения операции
 *
 */
bool awh::codec::toml::Writer::number(const string_view key, const T value) noexcept {
	/**
	 * Если записать имя ключа пары не удалось
	 */
	if(!this->key(key))
		// Выводим отрицательный результат выполнения операции
		return false;
	/**
	 * Если записывается логическое значение
	 *
	 * @note Сличение ведётся прежде целых чисел намеренно: логический тип языком
	 *       причислен к целым, и без этого истина записалась бы единицей
	 */
	if constexpr(is_same <T, bool>::value)
		// Выполняем запись логического значения
		return this->boolean(value);
	/**
	 * Если записывается число с плавающей точкой
	 */
	else if constexpr(is_floating_point <T>::value)
		// Выполняем запись числа с плавающей точкой
		return this->real(static_cast <double> (value));
	/**
	 * Если записывается целое число без знака
	 */
	else if constexpr(is_unsigned <T>::value) {
		/**
		 * Если целое число без знака выходит за отведённый ему отрезок значений
		 *
		 * @note Описание отводит целому числу шестьдесят четыре разряда со знаком, и
		 *       записать число сверх того нечем: приведение по кругу выдало бы
		 *       отрицательное значение
		 */
		if(static_cast <uint64_t> (value) > static_cast <uint64_t> (numeric_limits <int64_t>::max())){
			// Запоминаем код ошибки записи
			this->_error = error_t::NUMBER_OVERFLOW;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		// Выполняем запись целого числа без знака
		return this->integer(static_cast <int64_t> (value));
	}
	// Выполняем запись целого числа со знаком
	else return this->integer(static_cast <int64_t> (value));
}

/**
 * Выполняем порождение метода записи пары с числовым значением для всех поддерживаемых типов
 */
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <bool> (const string_view, const bool) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <int8_t> (const string_view, const int8_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <uint8_t> (const string_view, const uint8_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <int16_t> (const string_view, const int16_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <uint16_t> (const string_view, const uint16_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <int32_t> (const string_view, const int32_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <uint32_t> (const string_view, const uint32_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <int64_t> (const string_view, const int64_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <uint64_t> (const string_view, const uint64_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <float> (const string_view, const float) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::toml::Writer::number <double> (const string_view, const double) noexcept;

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include <sys/macro_pop.hpp>
