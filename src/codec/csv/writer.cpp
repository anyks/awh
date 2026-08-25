/**
 * @file writer.cpp
 * @date 2026-08-12
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
 * @brief Реализация записи текста CSV — сборка текста полем за полем с обрамлением
 *        кавычками по правилам договора и выдача собранного целиком либо кусками
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <limits>
#include <cstdio>
#include <type_traits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <num/lexical/lexical.hpp>
#include <codec/csv/writer.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::codec::csv::Writer::Settings::Settings() noexcept :
 separator(','), quote('"'), comment('\0'), quoting(quoting_t::MINIMAL),
 escape(escape_t::DOUBLE), newline(newline_t::CRLF), signature(false) {}
/**
 * @brief Метод записи метки порядка байтов
 *
 */
void awh::codec::csv::Writer::mark() noexcept {
	/**
	 * Если метка порядка байтов ещё не записана
	 */
	if(!this->_marked){
		// Запоминаем признак записи метки порядка байтов
		this->_marked = true;
		/**
		 * Если запись метки порядка байтов затребована
		 */
		if(this->_settings.signature)
			// Записываем метку порядка байтов кодировки UTF-8
			this->_text.append("\xEF\xBB\xBF");
	}
}
/**
 * @brief Метод записи содержимого поля с обрамлением кавычками
 *
 * @details Кавычка внутри содержимого удваивается либо предваряется знаком отмены -
 * смотря по настройкам. Знак отмены при этом удваивается и сам: без этого содержимое,
 * оканчивающееся им, отменило бы закрывающую кавычку
 *
 * @param text содержимое записываемого поля
 *
 */
void awh::codec::csv::Writer::quoted(const string_view text) noexcept {
	// Записываем открывающую кавычку
	this->_text.push_back(this->_settings.quote);
	/**
	 * Выполняем перебор всех знаков содержимого поля
	 */
	for(const char letter : text){
		/**
		 * Если знаком является кавычка
		 */
		if(letter == this->_settings.quote){
			/**
			 * Если кавычка записывается знаком отмены
			 */
			if(this->_settings.escape == escape_t::BACKSLASH)
				// Записываем знак отмены
				this->_text.push_back('\\');
			/**
			 * Если кавычка записывается удвоением
			 */
			else this->_text.push_back(this->_settings.quote);
		/**
		 * Если знаком является знак отмены, а кавычка записывается им же
		 */
		/**
		 * Если знаком является знак отмены, а разбор знак отмены признаёт
		 *
		 * @note Удваивается он и при записи кавычки удвоением, коль скоро разбор
		 *       признаёт оба способа: одиночный знак отмены такой разбор снял бы,
		 *       забрав вместе с собою следующий за ним знак
		 */
		} else if((letter == '\\') && (this->_settings.escape != escape_t::DOUBLE))
			// Записываем знак отмены
			this->_text.push_back('\\');
		// Записываем знак содержимого поля
		this->_text.push_back(letter);
	}
	// Записываем закрывающую кавычку
	this->_text.push_back(this->_settings.quote);
}
/**
 * @brief Метод записи очередного поля записи
 *
 * @param text содержимое записываемого поля
 *
 */
void awh::codec::csv::Writer::field(const string_view text) noexcept {
	// Выполняем запись метки порядка байтов
	this->mark();
	// Запоминаем признак того, что поле начинает запись
	const bool started = this->_started;
	/**
	 * Если поле начинает запись, запоминаем её начало в собранном тексте
	 *
	 * @note Нужно для различения записи без полей от записи из одного пустого поля:
	 *       знаков не дают обе, а поступать с ними договор велит по-разному
	 */
	if(!started)
		// Запоминаем положение начала текущей записи
		this->_origin = this->_text.size();
	/**
	 * Если запись уже содержит поля
	 */
	if(this->_started)
		// Записываем знак-разделитель полей
		this->_text.push_back(this->_settings.separator);
	// Запоминаем признак наличия полей у записи
	this->_started = true;
	/**
	 * Признак необходимости кавычек, вызванной знаком начала строки примечания
	 *
	 * @note Проверяется лишь первое поле записи: разбор признаёт примечанием строку,
	 *       этим знаком начатую, и поле такое без кавычек унесло бы за собою всю запись.
	 *       Знак этот - настройка разбора, а не часть договора, и записи он сообщается
	 *       затем, чтобы записанное читалось обратно теми же настройками
	 */
	const bool commented = (
		!started && (this->_settings.comment != '\0') &&
		(this->_settings.quoting != quoting_t::NONE) &&
		!text.empty() && (text.front() == this->_settings.comment)
	);
	/**
	 * Признак необходимости кавычек, вызванной меткой порядка байтов
	 *
	 * @note Проверяется лишь самое начало собираемого текста: метку, стоящую в начале,
	 *       разбор снимает как признак кодировки, и поле, ею начатое, опустело бы. Поле
	 *       из одного лишь пустого содержимого записывается пустой строкой, а пустые
	 *       строки разбор пропускает - запись пропала бы целиком. Найдено ворошителем:
	 *       круговой ход давал три записи против четырёх
	 */
	const bool signatured = (
		!started && this->_text.empty() &&
		(this->_settings.quoting != quoting_t::NONE) &&
		(text.length() >= 3) && (static_cast <uint8_t> (text[0]) == 0xEF) &&
		(static_cast <uint8_t> (text[1]) == 0xBB) && (static_cast <uint8_t> (text[2]) == 0xBF)
	);
	/**
	 * Признак необходимости кавычек, вызванной знаком отмены в содержимом поля
	 *
	 * @note Поле, содержащее знак отмены, при разборе, знак отмены признающем, теряет
	 *       его вместе со следующим за ним знаком. Правило необходимости кавычек знать
	 *       о способе записи кавычки не обязано, а потому знак этот проверяется здесь
	 */
	const bool escaped = (
		(this->_settings.escape != escape_t::DOUBLE) &&
		(this->_settings.quoting != quoting_t::NONE) &&
		(text.find('\\') != string_view::npos)
	);
	/**
	 * Если поле требуется заключить в кавычки
	 */
	if(commented || signatured || escaped || quotable(text, this->_settings.separator, this->_settings.quote, this->_settings.quoting)){
		// Записываем содержимое поля с обрамлением кавычками
		this->quoted(text);
		// Выходим из метода
		return;
	}
	/**
	 * Если кавычки не ставятся вовсе
	 *
	 * @note Знак-разделитель и знак конца строки внутри поля при этом предваряются
	 *       знаком отмены: иначе поле разорвало бы запись надвое. Разбор такую запись
	 *       читает лишь при способе записи знаком отмены, и правило это - уговор
	 *       между записью и разбором, а не часть договора
	 */
	if(this->_settings.quoting == quoting_t::NONE){
		/**
		 * Выполняем перебор всех знаков содержимого поля
		 */
		for(const char letter : text){
			/**
			 * Если знак разрывает запись
			 */
			if((letter == this->_settings.separator) || (letter == '\r') || (letter == '\n') || (letter == '\\'))
				// Записываем знак отмены
				this->_text.push_back('\\');
			// Записываем знак содержимого поля
			this->_text.push_back(letter);
		}
		// Выходим из метода
		return;
	}
	// Записываем содержимое поля как есть
	this->_text.append(text);
}
/**
 * @brief Метод записи числового поля записи
 *
 * @details Число записывается по правилам местности «C» и от установленной в
 * приложении местности не зависит: разделитель дробной части местности сделал бы
 * текст непереносимым, а в записи, где запятая служит разделителем полей, ещё и
 * неразбираемым
 *
 * @tparam T тип записываемого значения
 * @param value записываемое значение
 *
 */
template <typename T>
void awh::codec::csv::Writer::number(const T value) noexcept {
	// Хранилище записи числового значения поля
	char buffer[64];
	// Длина записи числового значения поля
	int32_t length = 0;
	/**
	 * Если записывается логическое значение
	 *
	 * @note Сличение ведётся прежде целых чисел намеренно: логический тип языком
	 *       причислен к целым, и без этого истина записалась бы единицей
	 */
	if constexpr(is_same <T, bool>::value){
		// Выполняем запись поля с логическим значением
		this->field(value ? "true" : "false");
		// Выходим из метода
		return;
	/**
	 * Если записывается число с плавающей точкой
	 */
	} else if constexpr(is_floating_point <T>::value) {
		/**
		 * Выполняем запись числа с плавающей точкой кратчайшим обратимым представлением
		 *
		 * @note Подбор точности с обратным чтением каждой пробы более не нужен: модуль
		 *       разбора чисел вычисляет количество значащих цифр сразу, отчего запись
		 *       «0.1» выходит как «0.1», а не как «0.10000000000000001»
		 */
		const awh::lexical::output_t <char> output = awh::lexical_t::toChars(
			buffer, buffer + sizeof(buffer),
			/**
			 * Запись ведётся числом его собственного типа: кратчайшая запись
			 * зависит от разрядности мантиссы, и приведение float к double дало бы
			 * «0.10000000149011612» вместо «0.1». Тип long double модулем не
			 * поддержан и приводится к double, как это делалось и прежде
			 */
			static_cast <typename conditional <is_same <T, float>::value, float, double>::type> (value)
		);
		// Определяем длину записи числа с плавающей точкой
		length = (static_cast <bool> (output) ? static_cast <int32_t> (output.ptr - buffer) : 0);
	/**
	 * Если записывается целое число со знаком
	 */
	} else if constexpr(is_signed <T>::value)
		// Выполняем запись целого числа со знаком
		length = ::snprintf(buffer, sizeof(buffer), "%lld", static_cast <long long> (value));
	/**
	 * Если записывается целое число без знака
	 */
	else length = ::snprintf(buffer, sizeof(buffer), "%llu", static_cast <unsigned long long> (value));
	/**
	 * Если запись числового значения выполнить не удалось
	 *
	 * @note Заслон этот НЕДОСТИЖИМ и оттого не покрыт: буфера в 64 байта довольно
	 *       всякому родному виду - целое без знака занимает не более 20 знаков, а
	 *       дробное кратчайшим обратимым представлением не более 24, считая знак,
	 *       точку и порядок. Проверено записью краевых значений: `nan`, `inf`,
	 *       `1.7976931348623157e+308` и `5e-324` укладываются с большим запасом
	 *
	 * @warning Снимать его нельзя: он оберегает обращение к буферу по длине, какую
	 *          вернуло преобразование, а преобразование это внешнее - смена его
	 *          заслон может понадобиться в тот же день
	 */
	if((length <= 0) || (static_cast <size_t> (length) >= sizeof(buffer))){
		// Выполняем запись пустого поля
		this->field("");
		// Выходим из метода
		return;
	}
	// Выполняем запись поля с числовым значением
	this->field(string_view(buffer, static_cast <size_t> (length)));
}
/**
 * Выполняем явное порождение метода записи числового поля для всех числовых типов
 */
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <bool> (const bool) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <int8_t> (const int8_t) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <uint8_t> (const uint8_t) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <int16_t> (const int16_t) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <uint16_t> (const uint16_t) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <int32_t> (const int32_t) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <uint32_t> (const uint32_t) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <int64_t> (const int64_t) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <uint64_t> (const uint64_t) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <float> (const float) noexcept;
template __AWH_SHARED_EXPORT__ void awh::codec::csv::Writer::number <double> (const double) noexcept;
/**
 * @brief Метод завершения текущей записи
 *
 */
void awh::codec::csv::Writer::record() noexcept {
	// Выполняем запись метки порядка байтов
	this->mark();
	/**
	 * Если запись содержит поля, но знаков не дала вовсе
	 *
	 * @note Случай этот один: запись из единственного ПУСТОГО поля. Пустою строкою она
	 *       неотличима от записи БЕЗ полей, а разбор пустые строки пропускает, и запись
	 *       круга не переживала бы. Пара кавычек её сохраняет - так же поступает
	 *       эталонная реализация языка Python
	 *
	 * @note При запрещённых настройкою кавычках писать нечем, и запись теряется
	 *       законно: договор без кавычек различить эти два случая не позволяет
	 */
	if(this->_started && (this->_text.size() == this->_origin) && (this->_settings.quoting != quoting_t::NONE)){
		// Записываем открывающий знак кавычек
		this->_text.push_back(this->_settings.quote);
		// Записываем закрывающий знак кавычек
		this->_text.push_back(this->_settings.quote);
	}
	// Записываем знак конца строки
	this->_text.append(newline(this->_settings.newline));
	// Снимаем признак наличия полей у записи
	this->_started = false;
}
/**
 * @brief Метод записи целой записи полем за полем
 *
 * @param fields поля записываемой записи
 *
 */
void awh::codec::csv::Writer::record(const vector <string> & fields) noexcept {
	/**
	 * Выполняем перебор всех полей записи
	 */
	for(const string & value : fields)
		// Записываем очередное поле записи
		this->field(value);
	// Завершаем текущую запись
	this->record();
}
/**
 * @brief Метод записи целой записи полем за полем
 *
 * @param fields поля записываемой записи
 *
 */
void awh::codec::csv::Writer::record(const vector <string_view> & fields) noexcept {
	/**
	 * Выполняем перебор всех полей записи
	 */
	for(const string_view value : fields)
		// Записываем очередное поле записи
		this->field(value);
	// Завершаем текущую запись
	this->record();
}
/**
 * @brief Метод записи текста целиком
 *
 * @param records записываемые записи
 *
 */
void awh::codec::csv::Writer::write(const vector <vector <string>> & records) noexcept {
	/**
	 * Выполняем перебор всех записываемых записей
	 */
	for(const vector <string> & fields : records)
		// Записываем очередную запись
		this->record(fields);
}
/**
 * @brief Метод получения собранного текста
 *
 * @return собранный текст
 *
 */
const string & awh::codec::csv::Writer::text() const noexcept {
	// Выводим собранный текст
	return this->_text;
}
/**
 * @brief Метод получения размера собранного текста
 *
 * @return размер собранного текста в байтах
 *
 */
size_t awh::codec::csv::Writer::size() const noexcept {
	// Выводим размер собранного текста
	return this->_text.size();
}
/**
 * @brief Метод изъятия собранного текста
 *
 * @return изъятый текст
 *
 */
string awh::codec::csv::Writer::take() noexcept {
	/**
	 * Если изъятие затребовано посреди записи
	 *
	 * @note Изъятое окончилось бы полем без завершения записи, и склеенное обратно
	 *       дало бы запись, разорванную надвое. Отдать здесь нечего, и пустота
	 *       честнее половины записи
	 */
	if(this->_started)
		// Выводим пустой текст
		return string();
	// Изымаемый текст
	string result;
	// Выполняем изъятие собранного текста
	result.swap(this->_text);
	// Выводим изъятый текст
	return result;
}
/**
 * @brief Метод очистки собранного текста
 *
 * @details Очищается лишь собранный текст: настройки записи и признак записанной метки
 * порядка байтов сохраняются, иначе метка эта появилась бы посреди потока
 *
 */
void awh::codec::csv::Writer::clear() noexcept {
	// Очищаем собранный текст
	this->_text.clear();
	// Сбрасываем положение начала текущей записи
	this->_origin = 0;
	// Снимаем признак наличия полей у записи
	this->_started = false;
}
/**
 * @brief Метод получения настроек записи текста
 *
 * @return настройки записи текста
 *
 */
const awh::codec::csv::Writer::settings_t & awh::codec::csv::Writer::settings() const noexcept {
	// Выводим настройки записи текста
	return this->_settings;
}
/**
 * @brief Метод установки настроек записи текста
 *
 * @param settings настройки записи текста
 *
 */
void awh::codec::csv::Writer::settings(const settings_t & settings) noexcept {
	// Запоминаем настройки записи текста
	this->_settings = settings;
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 *
 */
awh::codec::csv::Writer::Writer(const log_t * log) noexcept :
 _log(log), _origin(0), _started(false), _marked(false) {}
/**
 * @brief Конструктор
 *
 * @param log      объект для работы с логами
 * @param settings настройки записи текста
 *
 */
awh::codec::csv::Writer::Writer(const log_t * log, const settings_t & settings) noexcept :
 _log(log),
 _settings(settings), _origin(0), _started(false), _marked(false) {}
