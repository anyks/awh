/**
 * @file: core.cpp
 * @date: 2025-03-12
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <sys/chrono.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Устанавливаем название дней недели
 */
static vector <pair <string, string>> nameDays = {
	{"Mon", "Monday"},
	{"Tue", "Tuesday"},
	{"Wed", "Wednesday"},
	{"Thu", "Thursday"},
	{"Fri", "Friday"},
	{"Sat", "Saturday"},
	{"Sun", "Sunday"}
};
/**
 * Устанавливаем название месяцев
 */
static vector <pair <string, string>> nameMonths = {
	{"Jan", "January"},
	{"Feb", "February"},
	{"Mar", "March"},
	{"Apr", "April"},
	{"May", "May"},
	{"Jun", "June"},
	{"Jul", "July"},
	{"Aug", "August"},
	{"Sep", "September"},
	{"Oct", "October"},
	{"Nov", "November"},
	{"Dec", "December"}
};
/**
 * Таблица множителей месяцев
 */
static vector <uint8_t> rateMonths = {6,2,2,5,0,3,5,1,4,6,2,4};
/**
 * Таблица количества дней в месяцах
 */
static vector <uint8_t> daysInMonths = {31,28,31,30,31,30,31,31,30,31,30,31};
/**
 * Таблица множителей високосных годов
 */
static map <uint16_t, uint8_t> rateLeapYears = {{0,6},{1,2},{2,5},{3,1},{4,4},{5,0},{6,3}};

/**
 * clear Метод очистку всех локальных данных
 */
void awh::Chrono::clear() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Устанавливаем типы данных для Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Устанавливаем временную зону по умолчанию
			_tzset();
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Устанавливаем временную зону по умолчанию
			::tzset();
		#endif
		// Выполняем очистку списка временных зон
		this->clearTimeZones();
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx.date);
		// Выполняем сброс локального объекта даты и времени
		this->_dt = dt_t();
		// Получаем текущий штамп времени
		const auto now = chrono::system_clock::now();
		// Получаем штамп времени в наносекундах
		const chrono::nanoseconds nanoseconds = chrono::duration_cast <chrono::nanoseconds> (now.time_since_epoch());
		// Получаем штамп времени в миллисекундах
		const chrono::milliseconds milliseconds = chrono::duration_cast <chrono::milliseconds> (now.time_since_epoch());
		// Получаем штамп времени в микросекундах
		const chrono::microseconds microseconds = chrono::duration_cast <chrono::microseconds> (now.time_since_epoch());
		// Устанавливаем количество микросекунд
		this->_dt.microseconds = (microseconds.count() % 1000);
		// Устанавливаем количество наносекунд
		this->_dt.nanoseconds = (nanoseconds.count() % 1000000);
		// Устанавливаем количество миллисекунд
		this->makeDate(static_cast <uint64_t> (milliseconds.count()), this->_dt);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
}
/**
 * makeDate Метод получения штампа времени из объекта даты
 * @param dt объект даты из которой необходимо получить штамп времени
 * @return   штамп времени в миллисекундах
 */
uint64_t awh::Chrono::makeDate(const dt_t & dt) const noexcept {
	// Результат работы функции
	uint64_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем количество прошедших лет
		const uint16_t lastYears = (dt.year > 0 ? (dt.year - 1970) : 0);
		// Определяем количество прошедших високосных лет
		const uint16_t leapCount = (lastYears > 0 ? static_cast <uint16_t> (::round((lastYears - 1) / 4.)) : 0);
		// Получаем штамп времени начала года
		result = (
			(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
			(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
		);
		// Выполняем подсчёт количества дней
		for(uint8_t i = 0; (i < daysInMonths.size()) && (i < (dt.month - 1)); i++){
			// Если месяц февраль и год високосный
			if((i == 1) && dt.leap)
				// Увеличиваем результат на один день
				result += static_cast <uint64_t> (86400000);
			// Увеличиваем смещение времени до указанного месяца
			result += (static_cast <uint64_t> (daysInMonths.at(i)) * static_cast <uint64_t> (86400000));
		}
		// Если дата нулевая
		if(dt.date == 0)
			// Выполняем компенсацию
			const_cast <dt_t &> (dt).date = 1;
		// Увеличиваем на количество прошедших дней
		result += (static_cast <uint64_t> (dt.date - 1) * static_cast <uint64_t> (86400000));
		// Увеличиваем на количество часов
		result += (static_cast <uint64_t> (dt.hour) * static_cast <uint64_t> (3600000));
		// Увеличиваем на указанное смещение времени
		result += static_cast <uint64_t> (dt.offset * 1000);
		// Увеличиваем на количество минут
		result += (static_cast <uint64_t> (dt.minutes) * static_cast <uint64_t> (60000));
		// Увеличиваем на количество секунд
		result += (static_cast <uint64_t> (dt.seconds) * static_cast <uint64_t> (1000));
		// Увеличиваем на количество миллисекунд
		result += static_cast <uint64_t> (dt.milliseconds);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
		// Выполняем сброс результата
		result = 0;
	}
	// Выводим результат
	return result;
}
/**
 * makeDate Метод заполнения объекта даты из штампа времени
 * @param date дата из которой необходимо заполнить объект
 * @param dt   объект даты который необходимо заполнить
 */
void awh::Chrono::makeDate(const uint64_t date, dt_t & dt) const noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем установку текущее значение года
			dt.year = this->year(date);
			// Устанавливаем флаг високосного года
			dt.leap = ((dt.year % 4) == 0);
			// Определяем количество прошедших лет
			const uint16_t lastYears = (dt.year - 1970);
			// Определяем количество прошедших високосных лет
			const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
			// Получаем штамп времени начала года
			const uint64_t beginYear = (
				(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
				(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
			);
			// Начало месяца и начало суток
			uint64_t beginMonth = 0, beginDay = 0;
			// Определяем сколько дней прошло с начала года
			dt.days = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
			{
				// Подсчитываем количество дней в предыдущих месяцах
				uint16_t count = 0, days = 0;
				// Выполняем перебор всех дней месяца
				for(uint8_t i = 0; i < daysInMonths.size(); i++){
					// Увеличиваем номер месяца
					dt.month = (i + 1);
					// Получаем текущее количество дней с компенсацией високосного года
					days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && dt.leap ? 1 : 0));
					// Если мы не дошли до предела
					if(dt.days >= (days + count))
						// Увеличиваем количество прошедших дней
						count += days;
					// Выходим из цикла
					else break;
				}
				// Если у нас на дворе лето, устанавливаем флаг
				dt.dst = ((dt.month > 5) && (dt.month < 9));
				// Устанавливаем текущее значение даты
				dt.date = static_cast <uint8_t> ((dt.days - count) + 1);
				// Получаем начало месяца указанной даты
				beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
				// Получаем начало суток указанной даты
				beginDay = (beginMonth + (static_cast <uint64_t> (dt.date - 1) * static_cast <uint64_t> (86400000)));
				// Получаем множитель текущего года
				auto i = rateLeapYears.find(static_cast <uint16_t> ((dt.year - (dt.year % 4)) % 7));
				// Если множитель получен
				if(i != rateLeapYears.end()){
					// Подробнее: https://habr.com/ru/articles/217389
					// Устанавливаем день недели
					dt.day = (((i->second + static_cast <uint8_t> (dt.year % 4) + rateMonths.at(dt.month - 1) + dt.date) - (((dt.month == 1) || (dt.month == 2)) && dt.leap ? 1 : 0)) % 7);
					// Если воскресенье установлен как нулевой
					if(dt.day == 0)
						// Выполняем компенсацию
						dt.day = 7;
				}
				// Получаем количество недель прошедших с начала года
				dt.weeks = static_cast <uint8_t> (::round((date - beginYear) / 604800000.L));
			}
			// Получаем количество миллисекунд
			dt.milliseconds = static_cast <uint32_t> (date % 1000);
			// Получаем количество часов
			dt.hour = static_cast <uint8_t> (::floor((date - beginDay) / 3600000.L));
			// Получаем количество минут
			dt.minutes = static_cast <uint8_t> (::floor(((date - beginDay) % 3600000) / 60000.));
			// Получаем количество секунд
			dt.seconds = static_cast <uint8_t> (::floor((((date - beginDay) % 3600000) % 60000) / 1000.));
			// Если время утреннее
			if(dt.hour < 12)
				// Устанавливаем статус времени до полудня
				dt.h12 = h12_t::AM;
			// Устанавливаем время после полудня
			else dt.h12 = h12_t::PM;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	// Выполняем сброс значения даты
	} else dt = dt_t();
}
/**
 * compile Метод компиляции регулярных выражений
 * @param expression регулярное выражение для компиляции
 * @param format     формат к которому относится регулярное выражение
 */
void awh::Chrono::compile(const string & expression, const format_t format) noexcept {
	// Если регулярное выражение передано
	if(!expression.empty()){
		// Выполняем поиск регулярного выражения
		auto i = this->_expressions.find(format);
		// Если регулярное выражение не существует
		if(i == this->_expressions.end()){
			// Выполняем создании записи кэша
			auto ret = this->_expressions.emplace(format, regex_t{});
			// Выполняем компиляцию регулярного выражения
			const int32_t error = ::pcre2_regcomp(&ret.first->second, expression.c_str(), REG_UTF);
			// Если возникла ошибка компиляции
			if(error > 0){
				// Создаём буфер данных для извлечения данных ошибки
				char buffer[256];
				// Выполняем заполнение нулями буфер данных
				::memset(buffer, '\0', sizeof(buffer));
				// Выполняем извлечение текста ошибки
				const size_t size = ::pcre2_regerror(error, &ret.first->second, buffer, sizeof(buffer) - 1);
				// Если текст ошибки получен
				if(size > 0){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, string(buffer, size).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "%s\n", string(buffer, size).c_str());
					#endif
				}
				// Выполняем удаление созданного объекта регулярного выражения
				this->_expressions.erase(format);
			}
		}
	}
}
/**
 * prepare Функция заполнения объекта даты и времени
 * @param dt     объект даты и времени для заполнения
 * @param text   текст в котором производится поиск
 * @param format формат выполнения поиска
 * @param pos    начальная позиция в тексте
 * @return       конечная позиция обработанных данных в тексте
 */
ssize_t awh::Chrono::prepare(dt_t & dt, const string & text, const format_t format, const size_t pos) const noexcept {
	// Результат работы функции
	ssize_t result = -1;
	// Если данные переданы
	if(!text.empty() && (pos < text.size()) && (format != format_t::NONE)){
		// Запоминаем оригинальное значение формата
		const format_t fmt = format;
		// Определяем нужный нам формат
		switch(static_cast <uint8_t> (format)){
			// Если формат получен как %w
			case static_cast <uint8_t> (format_t::w):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::u;
			break;
			// Если формат получен как %W
			case static_cast <uint8_t> (format_t::W):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::s;
			break;
			// Если формат получен как %H
			case static_cast <uint8_t> (format_t::H):
			// Если формат получен как %I
			case static_cast <uint8_t> (format_t::I):
			// Если формат получен как %M
			case static_cast <uint8_t> (format_t::M):
			// Если формат получен как %s
			case static_cast <uint8_t> (format_t::S):
			// Если формат получен как %m
			case static_cast <uint8_t> (format_t::m):
			// Если формат получен как %d
			case static_cast <uint8_t> (format_t::d):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::y;
			break;
			// Если формат получен как %b
			case static_cast <uint8_t> (format_t::b):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::a;
			break;
			// Если формат получен как %B
			case static_cast <uint8_t> (format_t::B):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::A;
			break;
		}
		// Выполняем поиск нужного нам регулярного выражения
		auto i = this->_expressions.find(format);
		// Если регулярное выражение получено
		if(i != this->_expressions.end()){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.parse);
			// Создаём объект матчинга
			regmatch_t match[i->second.re_nsub + 1];
			// Выполняем разбор регулярного выражения
			const int32_t error = ::pcre2_regexec(&i->second, text.c_str() + pos, i->second.re_nsub + 1, match, REG_NOTEMPTY);
			// Если ошибок не получено
			if(error == 0){
				// Определяем нужный нам формат
				switch(static_cast <uint8_t> (format)){
					// Если формат получен как %u
					case static_cast <uint8_t> (format_t::u):
					// Если формат получен как %j
					case static_cast <uint8_t> (format_t::j):
					// Если формат получен как %s
					case static_cast <uint8_t> (format_t::s):
					// Если формат получен как %y
					case static_cast <uint8_t> (format_t::y):
					// Если формат получен как %Y
					case static_cast <uint8_t> (format_t::Y):
					// Если формат получен как %p
					case static_cast <uint8_t> (format_t::p):
					// Если формат получен как %a
					case static_cast <uint8_t> (format_t::a):
					// Если формат получен как %A
					case static_cast <uint8_t> (format_t::A):
					// Если формат получен как %Z
					case static_cast <uint8_t> (format_t::Z):
					// Если формат получен как %W
					case static_cast <uint8_t> (format_t::W): {
						// Выполняем перебор всех полученных вариантов
						for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
							// Если результат получен
							if(match[j].rm_eo > match[j].rm_so){
								// Получаем смещение в тексте
								result = static_cast <ssize_t> (pos + match[j].rm_eo);
								// Определяем тип входящих данных
								switch(static_cast <uint8_t> (fmt)){
									// Если мы определяем номер дня недели %w
									case static_cast <uint8_t> (format_t::w): {
										// Устанавливаем номер дня недели
										dt.day = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
										// Если день установлен как нулевой
										if(dt.day == 0)
											// Устанавливаем номер дня недели
											dt.day = 7;
									} break;
									// Если мы определяем номер дня недели %W
									case static_cast <uint8_t> (format_t::W):
										// Устанавливаем количество недель прошедших с начала года
										dt.weeks = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									break;
									// Если мы определяем номер дня недели %j
									case static_cast <uint8_t> (format_t::j):
										// Устанавливаем номер дня недели
										dt.days = static_cast <uint16_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)) - 1);
									break;
									// Если мы определяем номер дня недели %u
									case static_cast <uint8_t> (format_t::u):
										// Устанавливаем номер дня недели
										dt.day = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									break;
									// Если формат получен как %y
									case static_cast <uint8_t> (format_t::y): {
										// Получаем значение указанного года
										const uint16_t num = static_cast <uint16_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
										// Устанавливаем год
										dt.year = (2000 + num);
										// Устанавливаем флаг високосного года
										dt.leap = ((dt.year % 4) == 0);
									} break;
									// Если формат получен как %Y
									case static_cast <uint8_t> (format_t::Y): {
										// Устанавливаем год
										dt.year = static_cast <uint16_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
										// Устанавливаем флаг високосного года
										dt.leap = ((dt.year % 4) == 0);
									} break;
									// Если формат получен как %d
									case static_cast <uint8_t> (format_t::d):
										// Устанавливаем число месяца
										dt.date = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									break;
									// Если формат получен как %m
									case static_cast <uint8_t> (format_t::m):
										// Получаем значение номера месяца
										dt.month = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									break;
									// Если формат получен как %I
									case static_cast <uint8_t> (format_t::I):
										// Устанавливаем полученный час времени
										dt.hour = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									break;
									// Если формат получен как %H
									case static_cast <uint8_t> (format_t::H):
										// Устанавливаем полученный час времени
										dt.hour = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									break;
									// Если формат получен как %M
									case static_cast <uint8_t> (format_t::M):
										// Устанавливаем значение указанного количества минут
										dt.minutes = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									break;
									// Если формат получен как %s
									case static_cast <uint8_t> (format_t::s):
										// Устанавливаем количество миллисекунд
										dt.milliseconds = static_cast <uint32_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									break;
									// Если формат получен как %S
									case static_cast <uint8_t> (format_t::S):
										// Устанавливаем значение указанного количества секунд
										dt.seconds = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									break;
									// Если формат получен как %Z
									case static_cast <uint8_t> (format_t::Z): {
										// Выполняем матчинг временной зоны
										dt.zone = this->matchTimeZone(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so));
										// Получаем название временной зоны
										dt.offset = this->getTimeZone(dt.zone);
									} break;
									// Если формат получен как %a
									case static_cast <uint8_t> (format_t::a): {
										// Получаем название дня недели
										const string day(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
										// Выполняем перебор всего списка дней недели
										for(size_t i = 0; i < nameDays.size(); i++){
											// Если мы нашли нужный нам день недели
											if(this->_fmk->compare(day, nameDays.at(i).first)){
												// Устанавливаем день недели
												dt.day = static_cast <uint8_t> (i + 1);
												// Выходим из цикла
												break;
											}
										}
									} break;
									// Если формат получен как %A
									case static_cast <uint8_t> (format_t::A): {
										// Получаем название дня недели
										const string day(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
										// Выполняем перебор всего списка дней недели
										for(size_t i = 0; i < nameDays.size(); i++){
											// Если мы нашли нужный нам день недели
											if(this->_fmk->compare(day, nameDays.at(i).second)){
												// Устанавливаем день недели
												dt.day = static_cast <uint8_t> (i + 1);
												// Выходим из цикла
												break;
											}
										}
									} break;
									// Если формат получен как %b
									case static_cast <uint8_t> (format_t::b): {
										// Получаем название месяца
										const string month(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
										// Выполняем перебор всего списка месяцев
										for(size_t i = 0; i < nameMonths.size(); i++){
											// Если мы нашли нужный нам месяц
											if(this->_fmk->compare(month, nameMonths.at(i).first)){
												// Устанавливаем месяц
												dt.month = static_cast <uint8_t> (i + 1);
												// Выходим из цикла
												break;
											}
										}
									} break;
									// Если формат получен как %B
									case static_cast <uint8_t> (format_t::B): {
										// Получаем название месяца
										const string month(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
										// Выполняем перебор всего списка месяцев
										for(size_t i = 0; i < nameMonths.size(); i++){
											// Если мы нашли нужный нам месяц
											if(this->_fmk->compare(month, nameMonths.at(i).second)){
												// Устанавливаем месяц
												dt.month = static_cast <uint8_t> (i + 1);
												// Выходим из цикла
												break;
											}
										}
									} break;
									// Если формат получен как %p
									case static_cast <uint8_t> (format_t::p): {
										// Получаем название времени суток
										const string name(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
										// Определяем 12-и часовой формат времени
										dt.h12 = (this->_fmk->compare("pm", name) ? h12_t::PM : h12_t::AM);
										// Если мы получили вечернее время
										if((dt.h12 == h12_t::PM) && (dt.hour < 12))
											// Уменьшаем полученный час времени
											dt.hour += 12;
										// Если мы получили утреннее время
										else if((dt.h12 == h12_t::AM) && (dt.hour == 12))
											// Уменьшаем полученный час времени
											dt.hour = 0;
									} break;
								}
							}
						}
					} break;
					// Если формат получен как %z
					case static_cast <uint8_t> (format_t::z): {
						// Если временная зона не установлена
						if(dt.zone == zone_t::NONE)
							// Выполняем сброс временной зоны
							dt.offset = 0;
						// Создаём массив собранных результатов
						vector <string> data(i->second.re_nsub + 1);
						// Выполняем перебор всех полученных вариантов
						for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
							// Если результат получен
							if(match[j].rm_eo > match[j].rm_so){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].rm_eo);
								// Выполняем установку результата
								data.at(j).assign(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
							}
						}
						// Если название временной зоны указано
						if(!data.at(1).empty() && (dt.zone == zone_t::UTC))
							// Выполняем установку временной зоны
							dt.zone = zone_t::UTC;
						// Получаем смещение времени
						const string & offset = data.at(2);
						// Если полученное смещение является числом
						if(this->_fmk->is(offset, fmk_t::check_t::NUMBER)){
							// Если указано 4 символа
							if(offset.size() == 4){
								// Получаем количество часов
								const uint8_t hour = static_cast <uint8_t> (::stoul(offset.substr(0, 2)));
								// Получаем количество минут
								const uint8_t minutes = static_cast <uint8_t> (::stoul(offset.substr(2)));
								// Если смещение времени положительное
								if(data.at(1).compare("+") == 0)
									// Получаем время смещения
									dt.offset += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
								// Устанавливаем отрицательное смещение времени
								else dt.offset -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
							// Если установлен всего один символ
							} else {
								// Если смещение времени положительное
								if(data.at(1).compare("+") == 0)
									// Получаем время смещения
									dt.offset += static_cast <int32_t> (::stoul(offset) * 60 * 60);
								// Устанавливаем отрицательное смещение времени
								else dt.offset -= static_cast <int32_t> (::stoul(offset) * 60 * 60);
							}
						// Если получено время в формате часов
						} else if((data.size() > 4) && !data.at(3).empty() && !data.at(4).empty()) {
							// Получаем количество часов
							const uint8_t hour = static_cast <uint8_t> (::stoul(data.at(3)));
							// Получаем количество минут
							const uint8_t minutes = static_cast <uint8_t> (::stoul(data.at(4)));
							// Если смещение времени положительное
							if(data.at(1).compare("+") == 0)
								// Получаем время смещения
								dt.offset += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
							// Устанавливаем отрицательное смещение времени
							else dt.offset -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
						}
					} break;
					// Если формат получен как %R
					case static_cast <uint8_t> (format_t::R): {
						// Выполняем перебор всех полученных вариантов
						for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
							// Если результат получен
							if(match[j].rm_eo > match[j].rm_so){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].rm_eo);
								// Если мы получили час
								else if(j == 1)
									// Устанавливаем полученный час времени
									dt.hour = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили минуты
								else if(j == 2)
									// Устанавливаем значение указанного количества минут
									dt.minutes = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
							}
						}
					} break;
					// Если формат получен как %D
					case static_cast <uint8_t> (format_t::D): {
						// Выполняем перебор всех полученных вариантов
						for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
							// Если результат получен
							if(match[j].rm_eo > match[j].rm_so){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].rm_eo);
								// Если мы получили номер месяца
								else if(j == 1)
									// Устанавливаем полученный номер месяца
									dt.month = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили число месяца
								else if(j == 2)
									// Устанавливаем число месяца
									dt.date = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили год
								else if(j == 3) {
									// Получаем значение указанного года
									const uint16_t num = static_cast <uint16_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									// Устанавливаем год
									dt.year = (2000 + num);
									// Устанавливаем флаг високосного года
									dt.leap = ((dt.year % 4) == 0);
								}
							}
						}
					} break;
					// Если формат получен как %F
					case static_cast <uint8_t> (format_t::F): {
						// Выполняем перебор всех полученных вариантов
						for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
							// Если результат получен
							if(match[j].rm_eo > match[j].rm_so){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].rm_eo);
								// Если мы получили год
								else if(j == 1) {
									// Устанавливаем год
									dt.year = static_cast <uint16_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									// Устанавливаем флаг високосного года
									dt.leap = ((dt.year % 4) == 0);
								// Если мы получили номер месяца
								} else if(j == 2)
									// Получаем значение номера месяца
									dt.month = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили число месяца
								else if(j == 3)
									// Устанавливаем число месяца
									dt.date = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
							}
						}
					} break;
					// Если формат получен как %T
					case static_cast <uint8_t> (format_t::T): {
						// Выполняем перебор всех полученных вариантов
						for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
							// Если результат получен
							if(match[j].rm_eo > match[j].rm_so){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].rm_eo);
								// Если мы получили час
								else if(j == 1)
									// Устанавливаем полученный час времени
									dt.hour = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили минуты
								else if(j == 2)
									// Устанавливаем значение указанного количества минут
									dt.minutes = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили секунды
								else if(j == 3)
									// Устанавливаем значение указанного количества секунд
									dt.seconds = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
							}
						}
					} break;
					// Если формат получен как %r
					case static_cast <uint8_t> (format_t::r): {
						// Выполняем перебор всех полученных вариантов
						for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
							// Если результат получен
							if(match[j].rm_eo > match[j].rm_so){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].rm_eo);
								// Если мы получили час
								else if(j == 1)
									// Устанавливаем полученный час времени
									dt.hour = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили минуты
								else if(j == 2)
									// Устанавливаем значение указанного количества минут
									dt.minutes = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили секунды
								else if(j == 3)
									// Устанавливаем значение указанного количества секунд
									dt.seconds = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили метку времени
								else if(j == 4) {
									// Получаем название времени суток
									const string name(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
									// Определяем 12-и часовой формат времени
									dt.h12 = (this->_fmk->compare("pm", name) ? h12_t::PM : h12_t::AM);
									// Если мы получили вечернее время
									if((dt.h12 == h12_t::PM) && (dt.hour < 12))
										// Уменьшаем полученный час времени
										dt.hour += 12;
									// Если мы получили утреннее время
									else if((dt.h12 == h12_t::AM) && (dt.hour == 12))
										// Уменьшаем полученный час времени
										dt.hour = 0;
								}
							}
						}
					} break;
					// Если формат получен как %c
					case static_cast <uint8_t> (format_t::c): {
						// Выполняем перебор всех полученных вариантов
						for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
							// Если результат получен
							if(match[j].rm_eo > match[j].rm_so){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].rm_eo);
								// Если мы получили название дня недели
								else if(j == 1) {
									// Получаем название дня недели
									const string day(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
									// Выполняем перебор всего списка дней недели
									for(size_t i = 0; i < nameDays.size(); i++){
										// Если мы нашли нужный нам день недели
										if(this->_fmk->compare(day, nameDays.at(i).first)){
											// Устанавливаем день недели
											dt.day = static_cast <uint8_t> (i + 1);
											// Выходим из цикла
											break;
										}
									}
								// Если мы получили название месяца
								} else if(j == 2) {
									// Получаем название месяца
									const string month(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
									// Выполняем перебор всего списка месяцев
									for(size_t i = 0; i < nameMonths.size(); i++){
										// Если мы нашли нужный нам месяц
										if(this->_fmk->compare(month, nameMonths.at(i).first)){
											// Устанавливаем месяц
											dt.month = static_cast <uint8_t> (i + 1);
											// Выходим из цикла
											break;
										}
									}
								// Если мы получили число месяца
								} else if(j == 3)
									// Устанавливаем число месяца
									dt.date = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили час
								else if(j == 4)
									// Устанавливаем полученный час времени
									dt.hour = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили минуты
								else if(j == 5)
									// Устанавливаем значение указанного количества минут
									dt.minutes = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили секунды
								else if(j == 6)
									// Устанавливаем значение указанного количества секунд
									dt.seconds = static_cast <uint8_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
								// Если мы получили год
								else if(j == 7) {
									// Устанавливаем год
									dt.year = static_cast <uint16_t> (::stoul(string(text.c_str() + pos + match[j].rm_so, match[j].rm_eo - match[j].rm_so)));
									// Устанавливаем флаг високосного года
									dt.leap = ((dt.year % 4) == 0);
								}
							}
						}
					} break;
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * abbreviation Метод перевода времени в аббревиатуру
 * @param date дата в UnixTimestamp
 * @return     сформированная аббревиатура даты
 */
pair <awh::Chrono::type_t, double> awh::Chrono::abbreviation(const uint64_t date) const noexcept {
	// Результат работы функции
	pair <type_t, double> result = {type_t::MILLISECONDS, 0.};
	// Если число передано
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если число больше года
			if(date >= 29030400000)
				// Выполняем формирование результата
				result = ::make_pair(type_t::YEAR, static_cast <double> (date / 29030400000.L));
			// Если число больше месяца
			else if(date >= 2419200000)
				// Выполняем формирование результата
				result = ::make_pair(type_t::MONTH, static_cast <double> (date / 2419200000.L));
			// Если число больше недели
			else if(date >= 604800000)
				// Выполняем формирование результата
				result = ::make_pair(type_t::WEEK, static_cast <double> (date / 604800000.L));
			// Если число больше дня
			else if(date >= 86400000)
				// Выполняем формирование результата
				result = ::make_pair(type_t::DAY, static_cast <double> (date / 86400000.L));
			// Если число больше часа
			else if(date >= 3600000)
				// Выполняем формирование результата
				result = ::make_pair(type_t::HOUR, static_cast <double> (date / 3600000.L));
			// Если число больше минуты
			else if(date >= 60000)
				// Выполняем формирование результата
				result = ::make_pair(type_t::MINUTES, static_cast <double> (date / 60000.L));
			// Если число ольше секунды
			else if(date >= 1000)
				// Выполняем формирование результата
				result = ::make_pair(type_t::SECONDS, static_cast <double> (date / 1000.L));
			// Иначе выводим как есть
			else result = ::make_pair(type_t::MILLISECONDS, static_cast <double> (date));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * end Метод получения конца позиции указанной даты
 * @param date дата для которой необходимо получить позицию
 * @param type тип единиц измерений даты
 * @return     конец указанной даты в формате UnixTimestamp
 */
uint64_t awh::Chrono::end(const uint64_t date, const type_t type) const noexcept {
	// Результат работы функции
	uint64_t result = 0;
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем тип единиц измерений
			switch(static_cast <uint8_t> (type)){
				// Если нам нужно получить конец года
				case static_cast <uint8_t> (type_t::YEAR): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Определяем количество прошедших лет
					const uint16_t lastYears = (year - 1970);
					// Определяем количество прошедших високосных лет
					const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
					// Получаем штамп времени начала года
					result = (
						(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
						(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
					);
					// Выполняем перебор всех месяцев в году
					for(size_t i = 0; i < daysInMonths.size(); i++)
						// Увеличиваем количество дней в месяце
						result += (static_cast <uint64_t> (daysInMonths.at(i)) * static_cast <uint64_t> (86400000));
					// Если год високосный
					if(((lastYears + 1970) % 4) == 0)
						// Добавляем ещё один день
						result += 86400000;
				} break;
				// Если нам нужно получить конец месяца
				case static_cast <uint8_t> (type_t::MONTH): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Определяем количество прошедших лет
					const uint16_t lastYears = (year - 1970);
					// Определяем количество прошедших високосных лет
					const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
					// Получаем штамп времени начала года
					const uint64_t beginYear = (
						(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
						(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
					);
					// Определяем сколько дней прошло с начала года
					const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
					{
						// Подсчитываем количество дней в предыдущих месяцах
						uint16_t count = 0, days = 0;
						// Устанавливаем флаг високосного года
						const bool leap = (((lastYears + 1970) % 4) == 0);
						// Выполняем перебор всех дней месяца
						for(uint8_t i = 0; i < daysInMonths.size(); i++){
							// Получаем текущее количество дней с компенсацией високосного года
							days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
							// Если мы не дошли до предела
							if(lastDays > (days + count))
								// Увеличиваем количество прошедших дней
								count += days;
							// Выходим из цикла
							else break;
						}
						// Получаем начало месяца указанной даты
						result = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
						// Увеличиваем количество дней до конца месяца
						result += (static_cast <uint64_t> (days) * static_cast <uint64_t> (86400000));
					}
				} break;
				// Если нам нужно получить конец недели
				case static_cast <uint8_t> (type_t::WEEK): {
					// Получаем количество миллисекунд с начала текущей недели
					result = this->begin(date, type);
					// Увеличиваем количество дней до конца недели
					result += (static_cast <uint64_t> (7) * static_cast <uint64_t> (86400000));
				} break;
				// Если нам нужно получить конец дня
				case static_cast <uint8_t> (type_t::DAY):
					// Получаем количество миллисекунд конца текущего дня
					result = (this->begin(date, type) + 86400000);
				break;
				// Если нам нужно получить конец часа
				case static_cast <uint8_t> (type_t::HOUR):
					// Получаем количество миллисекунд конца текущего часа
					result = (this->begin(date, type) + 3600000);
				break;
				// Если нам нужно получить конец минуты
				case static_cast <uint8_t> (type_t::MINUTES):
					// Получаем количество миллисекунд конца текущей минуты
					result = (this->begin(date, type) + 60000);
				break;
				// Если нам нужно получить конец секунды
				case static_cast <uint8_t> (type_t::SECONDS):
					// Получаем количество миллисекунд конца текущей секунды
					result = (this->begin(date, type) + 1000);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * end Метод получения конца позиции текущей даты
 * @param type    тип единиц измерений даты
 * @param storage хранение значение времени
 * @return        конец текущей даты в формате UnixTimestamp
 */
uint64_t awh::Chrono::end(const type_t type, const storage_t storage) const noexcept {
	// Выполняем получение конца позиции текущей даты
	return this->end(this->timestamp(type_t::MILLISECONDS, storage), type);
}
/**
 * end Метод получения начала позиции указанной даты
 * @param date дата для которой необходимо получить позицию
 * @param type тип единиц измерений даты
 * @return     начало указанной даты в формате UnixTimestamp
 */
uint64_t awh::Chrono::begin(const uint64_t date, const type_t type) const noexcept {
	// Результат работы функции
	uint64_t result = 0;
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем тип единиц измерений
			switch(static_cast <uint8_t> (type)){
				// Если нам нужно получить начало года
				case static_cast <uint8_t> (type_t::YEAR): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Определяем количество прошедших лет
					const uint16_t lastYears = (year - 1970);
					// Определяем количество прошедших високосных лет
					const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
					// Получаем штамп времени начала года
					result = (
						(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
						(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
					);
				} break;
				// Если нам нужно получить начало месяца
				case static_cast <uint8_t> (type_t::MONTH): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Определяем количество прошедших лет
					const uint16_t lastYears = (year - 1970);
					// Определяем количество прошедших високосных лет
					const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
					// Получаем штамп времени начала года
					const uint64_t beginYear = (
						(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
						(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
					);
					// Определяем сколько дней прошло с начала года
					const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
					{
						// Подсчитываем количество дней в предыдущих месяцах
						uint16_t count = 0, days = 0;
						// Устанавливаем флаг високосного года
						const bool leap = (((lastYears + 1970) % 4) == 0);
						// Выполняем перебор всех дней месяца
						for(uint8_t i = 0; i < daysInMonths.size(); i++){
							// Получаем текущее количество дней с компенсацией високосного года
							days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
							// Если мы не дошли до предела
							if(lastDays > (days + count))
								// Увеличиваем количество прошедших дней
								count += days;
							// Выходим из цикла
							else break;
						}
						// Получаем начало месяца указанной даты
						result = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
					}
				} break;
				// Если нам нужно получить начало недели
				case static_cast <uint8_t> (type_t::WEEK): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Определяем количество прошедших лет
					const uint16_t lastYears = (year - 1970);
					// Определяем количество прошедших високосных лет
					const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
					// Получаем штамп времени начала года
					const uint64_t beginYear = (
						(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
						(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
					);
					// Определяем сколько дней прошло с начала года
					const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
					{
						// Номер текущего месяца
						uint8_t month = 0;
						// Подсчитываем количество дней в предыдущих месяцах
						uint16_t count = 0, days = 0;
						// Устанавливаем флаг високосного года
						const bool leap = ((year % 4) == 0);
						// Выполняем перебор всех дней месяца
						for(uint8_t i = 0; i < daysInMonths.size(); i++){
							// Увеличиваем номер месяца
							month = (i + 1);
							// Получаем текущее количество дней с компенсацией високосного года
							days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
							// Если мы не дошли до предела
							if(lastDays > (days + count))
								// Увеличиваем количество прошедших дней
								count += days;
							// Выходим из цикла
							else break;
						}
						// Устанавливаем текущее значение даты
						const uint8_t date = static_cast <uint8_t> ((lastDays - count) + 1);
						// Получаем начало месяца указанной даты
						const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
						// Получаем начало суток указанной даты
						const uint64_t beginDay = (beginMonth + (static_cast <uint64_t> (date - 1) * static_cast <uint64_t> (86400000)));
						// Получаем множитель текущего года
						auto i = rateLeapYears.find(static_cast <uint16_t> ((year - (year % 4)) % 7));
						// Если множитель получен
						if(i != rateLeapYears.end()){
							// Подробнее: https://habr.com/ru/articles/217389
							// Устанавливаем день недели
							uint8_t day = (((i->second + static_cast <uint8_t> (year % 4) + rateMonths.at(month - 1) + date) - (((month == 1) || (month == 2)) && leap ? 1 : 0)) % 7);
							// Если воскресенье установлен как нулевой
							if(day == 0)
								// Выполняем компенсацию
								day = 6;
							// Уменьшаем день на один
							else day--;
							// Получаем начало недели
							result = (beginDay - (static_cast <uint64_t> (day) * static_cast <uint64_t> (86400000)));
						}
					}
				} break;
				// Если нам нужно получить начало дня
				case static_cast <uint8_t> (type_t::DAY):
					// Выполняем определение начала дня
					result = (date - (date % 86400000));
				break;
				// Если нам нужно получить начало часа
				case static_cast <uint8_t> (type_t::HOUR):
					// Выполняем определение начала часа
					result = (date - (date % 3600000));
				break;
				// Если нам нужно получить начало минуты
				case static_cast <uint8_t> (type_t::MINUTES):
					// Выполняем определение начала минут
					result = (date - (date % 60000));
				break;
				// Если нам нужно получить начало секунды
				case static_cast <uint8_t> (type_t::SECONDS):
					// Выполняем определение начала секунд
					result = (date - (date % 1000));
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * end Метод получения начала позиции текущей даты
 * @param type    тип единиц измерений даты
 * @param storage хранение значение времени
 * @return        начало текущей даты в формате UnixTimestamp
 */
uint64_t awh::Chrono::begin(const type_t type, const storage_t storage) const noexcept {
	// Выполняем получение начала позиции текущей даты
	return this->begin(this->timestamp(type_t::MILLISECONDS, storage), type);
}
/**
 * actual Метод актуализации прошедшего и оставшегося времени
 * @param date   дата относительно которой производятся расчёты
 * @param value  тип определяемых единиц измерений времени
 * @param type   тип единиц измерений даты
 * @param actual направление актуализации
 * @return       результат вычисления
 */
uint64_t awh::Chrono::actual(const uint64_t date, const type_t value, const type_t type, const actual_t actual) const noexcept {
	// Результат работы функции
	uint64_t result = 0;
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем направление актуализации
			switch(static_cast <uint8_t> (actual)){
				// Если нужно определить сколько осталось времени
				case static_cast <uint8_t> (actual_t::LEFT): {
					// Определяем тип единиц измерений
					switch(static_cast <uint8_t> (type)){
						// Если нам нужно получить количество оставшего времени в году
						case static_cast <uint8_t> (type_t::YEAR): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся месяцев
								case static_cast <uint8_t> (type_t::MONTH): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Формируем полученный результат
										result = static_cast <uint64_t> (12 - month);
									}
								} break;
								// Если нам нужно получить количество оставшихся недель
								case static_cast <uint8_t> (type_t::WEEK): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Количество недель в году
									uint8_t weeks = 0;
									// Если год високосный
									if(((lastYears + 1970) % 4) == 0)
										// Получаем количество недель в году
										weeks = static_cast <uint8_t> (::round(31622400000 / 604800000.L));
									// Если год не високосный
									else weeks = static_cast <uint8_t> (::round(31536000000 / 604800000.L));
									// Получаем количество недель оставшихся в году
									result = static_cast <uint64_t> (weeks - static_cast <uint8_t> (::round((date - beginYear) / 604800000.L)));
								} break;
								// Если нам нужно получить количество оставшихся дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									// Если год високосный
									if(((lastYears + 1970) % 4) == 0)
										// Определяем сколько осталось дней в году
										result = static_cast <uint64_t> (366 - (lastDays + 1));
									// Если год не високосный
									else result = static_cast <uint64_t> (365 - (lastDays + 1));
								} break;
								// Если нам нужно получить количество оставшихся часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Получаем количество часов прошедших с начала года
									const uint32_t hours = static_cast <uint32_t> (::ceil((date - beginYear) / 3600000.L));
									// Если год високосный
									if(((lastYears + 1970) % 4) == 0)
										// Определяем количество оставшихся часов
										result = static_cast <uint64_t> (static_cast <uint32_t> (31622400000 / 3600000) - hours);
									// Если год не високосный
									else result = static_cast <uint64_t> (static_cast <uint32_t> (31536000000 / 3600000) - hours);
								} break;
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Получаем количество минут прошедших с начала года
									const uint64_t minutes = static_cast <uint64_t> (::ceil((date - beginYear) / 60000.));
									// Если год високосный
									if(((lastYears + 1970) % 4) == 0)
										// Определяем количество оставшихся минут
										result = (static_cast <uint64_t> (31622400000 / 60000) - minutes);
									// Если год не високосный
									else result = (static_cast <uint64_t> (31536000000 / 60000) - minutes);
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Получаем количество секунд прошедших с начала года
									const uint64_t seconds = static_cast <uint64_t> (::ceil((date - beginYear) / 1000.));
									// Если год високосный
									if(((lastYears + 1970) % 4) == 0)
										// Определяем количество оставшихся секунд
										result = (static_cast <uint64_t> (31622400000 / 1000) - seconds);
									// Если год не високосный
									else result = (static_cast <uint64_t> (31536000000 / 1000) - seconds);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Получаем количество миллисекунд прошедших с начала года
									const uint64_t milliseconds = static_cast <uint64_t> (date - beginYear);
									// Если год високосный
									if(((lastYears + 1970) % 4) == 0)
										// Определяем количество оставшихся миллисекунд
										result = (static_cast <uint64_t> (31622400000) - (milliseconds + 1));
									// Если год не високосный
									else result = (static_cast <uint64_t> (31536000000) - (milliseconds + 1));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
									// Если текущее значение даты передано в микросекундах
									if(current == (actual + 3)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Определяем количество прошедших лет
										const uint16_t lastYears = (year - 1970);
										// Определяем количество прошедших високосных лет
										const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
										// Получаем штамп времени начала года
										const uint64_t beginYear = (
											(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000000)) +
											(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000000))
										);
										// Получаем количество микросекунд прошедших с начала года
										const uint64_t microseconds = static_cast <uint64_t> (date - beginYear);
										// Если год високосный
										if(((lastYears + 1970) % 4) == 0)
											// Определяем количество оставшихся микросекунд
											result = (static_cast <uint64_t> (31622400000000) - (microseconds + 1));
										// Если год не високосный
										else result = (static_cast <uint64_t> (31536000000000) - (microseconds + 1));
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
										// Увеличиваем размер количества миллисекунд до микросекунд
										result *= 1000;
									}
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
									// Если текущее значение даты передано в наносекундах
									if(current == (actual + 6)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Определяем количество прошедших лет
										const uint16_t lastYears = (year - 1970);
										// Определяем количество прошедших високосных лет
										const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
										// Получаем штамп времени начала года
										const uint64_t beginYear = (
											(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000000000)) +
											(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000000000))
										);
										// Получаем количество наносекунд прошедших с начала года
										const uint64_t nanoseconds = static_cast <uint64_t> (date - beginYear);
										// Если год високосный
										if(((lastYears + 1970) % 4) == 0)
											// Определяем количество оставшихся наносекунд
											result = (static_cast <uint64_t> (31622400000000000) - (nanoseconds + 1));
										// Если год не високосный
										else result = (static_cast <uint64_t> (31536000000000000) - (nanoseconds + 1));
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
										// Увеличиваем размер количества миллисекунд до наносекунд
										result *= 1000000;
									}
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в месяце
						case static_cast <uint8_t> (type_t::MONTH): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся недель
								case static_cast <uint8_t> (type_t::WEEK): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2)){
											// Получаем количество недель в месяце
											const uint8_t weeks = static_cast <uint8_t> (::ceil((daysInMonths.at(month - 1) + 1) / 7.));
											// Получаем количество оставшихся недель в месяце
											result = static_cast <uint64_t> (weeks - static_cast <uint8_t> (::round((date - beginMonth) / 604800000.L)));
										// Если год не високосный или месяц не февраль
										} else {
											// Получаем количество недель в месяце
											const uint8_t weeks = static_cast <uint8_t> (::ceil(daysInMonths.at(month - 1) / 7.));
											// Получаем количество оставшихся недель в месяце
											result = static_cast <uint64_t> (weeks - static_cast <uint8_t> (::round((date - beginMonth) / 604800000.L)));
										}
									}
								} break;
								// Если нам нужно получить количество оставшихся дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся дней в месяце
											result = static_cast <uint64_t> ((daysInMonths.at(month - 1) + 1) - static_cast <uint8_t> (::round((date - beginMonth) / 86400000.L)));
										// Получаем количество оставшихся дней в месяце
										else result = static_cast <uint64_t> (daysInMonths.at(month - 1) - static_cast <uint8_t> (::round((date - beginMonth) / 86400000.L)));
									}
								} break;
								// Если нам нужно получить количество оставшихся часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся часов в месяце
											result = static_cast <uint64_t> ((static_cast <uint32_t> ((daysInMonths.at(month - 1) + 1) * 86400000) - static_cast <uint32_t> (date - beginMonth)) / 3600000);
										// Получаем количество оставшихся часов в месяце
										else result = static_cast <uint64_t> ((static_cast <uint32_t> (daysInMonths.at(month - 1) * 86400000) - static_cast <uint32_t> (date - beginMonth)) / 3600000);
									}
								} break;
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся минут в месяце
											result = static_cast <uint64_t> ((static_cast <uint32_t> ((daysInMonths.at(month - 1) + 1) * 86400000) - static_cast <uint32_t> (date - beginMonth)) / 60000);
										// Получаем количество оставшихся минут в месяце
										else result = static_cast <uint64_t> ((static_cast <uint32_t> (daysInMonths.at(month - 1) * 86400000) - static_cast <uint32_t> (date - beginMonth)) / 60000);
									}
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся секунд в месяце
											result = static_cast <uint64_t> ((static_cast <uint32_t> ((daysInMonths.at(month - 1) + 1) * 86400000) - static_cast <uint32_t> (date - beginMonth)) / 1000);
										// Получаем количество оставшихся секунд в месяце
										else result = static_cast <uint64_t> ((static_cast <uint32_t> (daysInMonths.at(month - 1) * 86400000) - static_cast <uint32_t> (date - beginMonth)) / 1000);
									}
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся миллисекунд в месяце
											result = static_cast <uint64_t> (static_cast <uint32_t> ((daysInMonths.at(month - 1) + 1) * 86400000) - static_cast <uint32_t> (date - beginMonth));
										// Получаем количество оставшихся миллисекунд в месяце
										else result = static_cast <uint64_t> (static_cast <uint32_t> (daysInMonths.at(month - 1) * 86400000) - static_cast <uint32_t> (date - beginMonth));
									}
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
									// Если текущее значение даты передано в микросекундах
									if(current == (actual + 3)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Определяем количество прошедших лет
										const uint16_t lastYears = (year - 1970);
										// Определяем количество прошедших високосных лет
										const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
										// Получаем штамп времени начала года
										const uint64_t beginYear = (
											(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000000)) +
											(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000000))
										);
										// Определяем сколько дней прошло с начала года
										const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000000.L));
										{
											// Номер текущего месяца
											uint8_t month = 0;
											// Подсчитываем количество дней в предыдущих месяцах
											uint16_t count = 0, days = 0;
											// Устанавливаем флаг високосного года
											const bool leap = ((year % 4) == 0);
											// Выполняем перебор всех дней месяца
											for(uint8_t i = 0; i < daysInMonths.size(); i++){
												// Увеличиваем номер месяца
												month = (i + 1);
												// Получаем текущее количество дней с компенсацией високосного года
												days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
												// Если мы не дошли до предела
												if(lastDays > (days + count))
													// Увеличиваем количество прошедших дней
													count += days;
												// Выходим из цикла
												else break;
											}
											// Получаем начало месяца указанной даты
											const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000000)));
											// Если год високосный и месяц февраль
											if(leap && (month == 2))
												// Получаем количество оставшихся микросекунд в месяце
												result = static_cast <uint64_t> (static_cast <uint64_t> ((daysInMonths.at(month - 1) + 1) * 86400000000) - static_cast <uint64_t> (date - beginMonth));
											// Получаем количество оставшихся микросекунд в месяце
											else result = static_cast <uint64_t> (static_cast <uint64_t> (daysInMonths.at(month - 1) * 86400000000) - static_cast <uint64_t> (date - beginMonth));
										}
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
										// Увеличиваем размер количества миллисекунд до микросекунд
										result *= 1000;
									}
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
									// Если текущее значение даты передано в наносекундах
									if(current == (actual + 6)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Определяем количество прошедших лет
										const uint16_t lastYears = (year - 1970);
										// Определяем количество прошедших високосных лет
										const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
										// Получаем штамп времени начала года
										const uint64_t beginYear = (
											(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000000000)) +
											(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000000000))
										);
										// Определяем сколько дней прошло с начала года
										const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000000000.L));
										{
											// Номер текущего месяца
											uint8_t month = 0;
											// Подсчитываем количество дней в предыдущих месяцах
											uint16_t count = 0, days = 0;
											// Устанавливаем флаг високосного года
											const bool leap = ((year % 4) == 0);
											// Выполняем перебор всех дней месяца
											for(uint8_t i = 0; i < daysInMonths.size(); i++){
												// Увеличиваем номер месяца
												month = (i + 1);
												// Получаем текущее количество дней с компенсацией високосного года
												days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
												// Если мы не дошли до предела
												if(lastDays > (days + count))
													// Увеличиваем количество прошедших дней
													count += days;
												// Выходим из цикла
												else break;
											}
											// Получаем начало месяца указанной даты
											const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000000000)));
											// Если год високосный и месяц февраль
											if(leap && (month == 2))
												// Получаем количество оставшихся наносекунд в месяце
												result = static_cast <uint64_t> (static_cast <uint64_t> ((daysInMonths.at(month - 1) + 1) * 86400000000000) - static_cast <uint64_t> (date - beginMonth));
											// Получаем количество оставшихся наносекунд в месяце
											else result = static_cast <uint64_t> (static_cast <uint64_t> (daysInMonths.at(month - 1) * 86400000000000) - static_cast <uint64_t> (date - beginMonth));
										}
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
										// Увеличиваем размер количества наносекунд до микросекунд
										result *= 1000000;
									}
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в неделе
						case static_cast <uint8_t> (type_t::WEEK): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших дней
									result = static_cast <uint64_t> ((static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(date - begin))) / 86400000.L);
								} break;
								// Если нам нужно получить количество оставшихся часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших часов
									result = static_cast <uint64_t> ((static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(date - begin))) / 3600000.L);
								} break;
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших минут
									result = static_cast <uint64_t> ((static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(date - begin))) / 60000.);
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> ((static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(date - begin))) / 1000.);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(date - begin)));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в дне
						case static_cast <uint8_t> (type_t::DAY): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших часов
									result = static_cast <uint64_t> ((static_cast <uint64_t> (86400000) - static_cast <uint64_t> (::floor(date - begin))) / 3600000.L);
								} break;
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших минут
									result = static_cast <uint64_t> ((static_cast <uint64_t> (86400000) - static_cast <uint64_t> (::floor(date - begin))) / 60000.);
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> ((static_cast <uint64_t> (86400000) - static_cast <uint64_t> (::floor(date - begin))) / 1000.);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (86400000) - static_cast <uint64_t> (::floor(date - begin)));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в часе
						case static_cast <uint8_t> (type_t::HOUR): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших минут
									result = static_cast <uint64_t> ((static_cast <uint64_t> (3600000) - static_cast <uint64_t> (::floor(date - begin))) / 60000.);
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> ((static_cast <uint64_t> (3600000) - static_cast <uint64_t> (::floor(date - begin))) / 1000.);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (3600000) - static_cast <uint64_t> (::floor(date - begin)));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в минуте
						case static_cast <uint8_t> (type_t::MINUTES): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> ((static_cast <uint64_t> (60000) - static_cast <uint64_t> (::floor(date - begin))) / 1000.);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (60000) - static_cast <uint64_t> (::floor(date - begin)));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в секунде
						case static_cast <uint8_t> (type_t::SECONDS): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (1000) - static_cast <uint64_t> (::floor(date - begin)));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
					}
				} break;
				// Если нужно определить сколько прошло времени
				case static_cast <uint8_t> (actual_t::PASSED): {
					// Определяем тип единиц измерений
					switch(static_cast <uint8_t> (type)){
						// Если нам нужно получить количество прошедшего времени в году
						case static_cast <uint8_t> (type_t::YEAR): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших месяцев
								case static_cast <uint8_t> (type_t::MONTH): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Формируем полученный результат
										result = static_cast <uint64_t> (month - 1);
									}
								} break;
								// Если нам нужно получить количество прошедших недель
								case static_cast <uint8_t> (type_t::WEEK): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Получаем количество недель прошедших в году
									result = static_cast <uint64_t> (::round((date - beginYear) / 604800000.L));
								} break;
								// Если нам нужно получить количество прошедших дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									result = static_cast <uint64_t> (::floor((date - beginYear) / 86400000.L));
								} break;
								// Если нам нужно получить количество прошедших часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Получаем количество часов прошедших с начала года
									result = static_cast <uint64_t> (::floor((date - beginYear) / 3600000.L));
								} break;
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Получаем количество минут прошедших с начала года
									result = static_cast <uint64_t> (::floor((date - beginYear) / 60000.));
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Получаем количество секунд прошедших с начала года
									result = static_cast <uint64_t> (::floor((date - beginYear) / 1000.));
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Получаем количество миллисекунд прошедших с начала года
									result = (date - beginYear);
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
									// Если текущее значение даты передано в микросекундах
									if(current == (actual + 3)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Определяем количество прошедших лет
										const uint16_t lastYears = (year - 1970);
										// Определяем количество прошедших високосных лет
										const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
										// Получаем штамп времени начала года
										const uint64_t beginYear = (
											(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000000)) +
											(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000000))
										);
										// Получаем количество микросекунд прошедших с начала года
										result = static_cast <uint64_t> (date - beginYear);
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
										// Увеличиваем размер количества миллисекунд до микросекунд
										result *= 1000;
									}
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
									// Если текущее значение даты передано в наносекундах
									if(current == (actual + 6)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Определяем количество прошедших лет
										const uint16_t lastYears = (year - 1970);
										// Определяем количество прошедших високосных лет
										const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
										// Получаем штамп времени начала года
										const uint64_t beginYear = (
											(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000000000)) +
											(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000000000))
										);
										// Получаем количество наносекунд прошедших с начала года
										result = static_cast <uint64_t> (date - beginYear);
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
										// Увеличиваем размер количества миллисекунд до наносекунд
										result *= 1000000;
									}
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в месяце
						case static_cast <uint8_t> (type_t::MONTH): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших недель
								case static_cast <uint8_t> (type_t::WEEK): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество оставшихся недель в месяце
										result = static_cast <uint64_t> (::round((date - beginMonth) / 604800000.L));
									}
								} break;
								// Если нам нужно получить количество прошедших дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество оставшихся дней в месяце
										result = static_cast <uint64_t> (::round((date - beginMonth) / 86400000.L) - 1);
									}
								} break;
								// Если нам нужно получить количество прошедших часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество оставшихся часов в месяце
										result = static_cast <uint64_t> (static_cast <uint32_t> (date - beginMonth) / 3600000);
									}
								} break;
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество оставшихся минут в месяце
										result = static_cast <uint64_t> (static_cast <uint32_t> (date - beginMonth) / 60000);
									}
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество оставшихся секунд в месяце
										result = static_cast <uint64_t> (static_cast <uint32_t> (date - beginMonth) / 1000);
									}
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Определяем количество прошедших лет
									const uint16_t lastYears = (year - 1970);
									// Определяем количество прошедших високосных лет
									const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
									// Получаем штамп времени начала года
									const uint64_t beginYear = (
										(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
										(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
									);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = ((year % 4) == 0);
										// Выполняем перебор всех дней месяца
										for(uint8_t i = 0; i < daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays > (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество оставшихся миллисекунд в месяце
										result = static_cast <uint64_t> (date - beginMonth);
									}
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
									// Если текущее значение даты передано в микросекундах
									if(current == (actual + 3)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Определяем количество прошедших лет
										const uint16_t lastYears = (year - 1970);
										// Определяем количество прошедших високосных лет
										const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
										// Получаем штамп времени начала года
										const uint64_t beginYear = (
											(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000000)) +
											(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000000))
										);
										// Определяем сколько дней прошло с начала года
										const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000000.L));
										{
											// Номер текущего месяца
											uint8_t month = 0;
											// Подсчитываем количество дней в предыдущих месяцах
											uint16_t count = 0, days = 0;
											// Устанавливаем флаг високосного года
											const bool leap = ((year % 4) == 0);
											// Выполняем перебор всех дней месяца
											for(uint8_t i = 0; i < daysInMonths.size(); i++){
												// Увеличиваем номер месяца
												month = (i + 1);
												// Получаем текущее количество дней с компенсацией високосного года
												days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
												// Если мы не дошли до предела
												if(lastDays > (days + count))
													// Увеличиваем количество прошедших дней
													count += days;
												// Выходим из цикла
												else break;
											}
											// Получаем начало месяца указанной даты
											const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000000)));
											// Получаем количество оставшихся микросекунд в месяце
											result = static_cast <uint64_t> (date - beginMonth);
										}
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
										// Увеличиваем размер количества миллисекунд до микросекунд
										result *= 1000;
									}
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
									// Если текущее значение даты передано в наносекундах
									if(current == (actual + 6)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Определяем количество прошедших лет
										const uint16_t lastYears = (year - 1970);
										// Определяем количество прошедших високосных лет
										const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
										// Получаем штамп времени начала года
										const uint64_t beginYear = (
											(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000000000)) +
											(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000000000))
										);
										// Определяем сколько дней прошло с начала года
										const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000000000.L));
										{
											// Номер текущего месяца
											uint8_t month = 0;
											// Подсчитываем количество дней в предыдущих месяцах
											uint16_t count = 0, days = 0;
											// Устанавливаем флаг високосного года
											const bool leap = ((year % 4) == 0);
											// Выполняем перебор всех дней месяца
											for(uint8_t i = 0; i < daysInMonths.size(); i++){
												// Увеличиваем номер месяца
												month = (i + 1);
												// Получаем текущее количество дней с компенсацией високосного года
												days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
												// Если мы не дошли до предела
												if(lastDays > (days + count))
													// Увеличиваем количество прошедших дней
													count += days;
												// Выходим из цикла
												else break;
											}
											// Получаем начало месяца указанной даты
											const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000000000)));
											// Получаем количество оставшихся наносекунд в месяце
											result = static_cast <uint64_t> (date - beginMonth);
										}
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
										// Увеличиваем размер количества миллисекунд до наносекунд
										result *= 1000000;
									}
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в неделе
						case static_cast <uint8_t> (type_t::WEEK): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших дней
									result = static_cast <uint64_t> (::floor(date - begin) / 86400000.L);
								} break;
								// Если нам нужно получить количество прошедших часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших часов
									result = static_cast <uint64_t> (::floor(date - begin) / 3600000.L);
								} break;
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших минут
									result = static_cast <uint64_t> (::floor(date - begin) / 60000.);
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(date - begin) / 1000.);
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(date - begin));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в дне
						case static_cast <uint8_t> (type_t::DAY): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших часов
									result = static_cast <uint64_t> (::floor(date - begin) / 3600000.L);
								} break;
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших минут
									result = static_cast <uint64_t> (::floor(date - begin) / 60000.);
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(date - begin) / 1000.);
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(date - begin));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в часе
						case static_cast <uint8_t> (type_t::HOUR): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших минут
									result = static_cast <uint64_t> (::floor(date - begin) / 60000.);
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(date - begin) / 1000.);
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(date - begin));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в минуте
						case static_cast <uint8_t> (type_t::MINUTES): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(date - begin) / 1000.);
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(date - begin));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в секунде
						case static_cast <uint8_t> (type_t::SECONDS): {
							// Определяем тип определяемых единиц измерений
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало дня недели
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(date - begin));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * actual Метод актуализации прошедшего и оставшегося времени
 * @param value   тип определяемых единиц измерений времени
 * @param type    тип единиц измерений даты
 * @param actual  направление актуализации
 * @param storage хранение значение времени
 * @return        результат вычисления
 */
uint64_t awh::Chrono::actual(const type_t value, const type_t type, const actual_t actual, const storage_t storage) const noexcept {
	// Выполняем актуализацию текущей даты на указанное количество единиц времени
	return this->actual(this->timestamp(type_t::MILLISECONDS, storage), value, type, actual);
}
/**
 * offset Метод смещения на указанное количество единиц времени
 * @param date   дата относительно которой производится смещение
 * @param value  значение на которое производится смещение
 * @param type   тип единиц измерений даты
 * @param offset направление смещения
 * @return       результат вычисления в формате UnixTimestamp
 */
uint64_t awh::Chrono::offset(const uint64_t date, const uint64_t value, const type_t type, const offset_t offset) const noexcept {
	// Результат работы функции
	uint64_t result = 0;
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем направление смещения
			switch(static_cast <uint8_t> (offset)){
				// Если необходимо выполнить инкремент
				case static_cast <uint8_t> (offset_t::INCREMENT): {
					// Определяем тип единиц измерений
					switch(static_cast <uint8_t> (type)){
						// Если нам нужно получить начало года
						case static_cast <uint8_t> (type_t::YEAR): {
							// Устанавливаем текущее значение даты
							result = date;
							// Получаем значение текущего года
							const uint16_t year = this->year(result);
							// Выполняем перебор всех лет
							for(size_t i = 0; i < static_cast <size_t> (value); i++){
								// Если будущий год является високосным
								if(((year + (i + 1)) % 4) == 0)
									// Увеличиваем текущее значение года на 366 дней
									result += 31622400000;
								// Увеличиваем текущее значение года на 365 дней
								else result += 31536000000;
							}
						} break;
						// Если нам нужно получить начало месяца
						case static_cast <uint8_t> (type_t::MONTH): {
							// Устанавливаем текущее значение даты
							result = date;
							// Получаем значение текущего года
							const uint16_t year = this->year(result);
							// Определяем количество прошедших лет
							const uint16_t lastYears = (year - 1970);
							// Определяем количество прошедших високосных лет
							const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
							// Получаем штамп времени начала года
							const uint64_t beginYear = (
								(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
								(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
							);
							// Определяем сколько дней прошло с начала года
							const uint16_t lastDays = static_cast <uint16_t> (::floor((result - beginYear) / 86400000.L));
							{
								// Номер текущего месяца
								uint8_t month = 0;
								// Подсчитываем количество дней в предыдущих месяцах
								uint16_t count = 0, days = 0;
								// Устанавливаем флаг високосного года
								const bool leap = ((year % 4) == 0);
								// Выполняем перебор всех дней месяца
								for(uint8_t i = 0; i < daysInMonths.size(); i++){
									// Увеличиваем номер месяца
									month = (i + 1);
									// Получаем текущее количество дней с компенсацией високосного года
									days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
									// Если мы не дошли до предела
									if(lastDays > (days + count))
										// Увеличиваем количество прошедших дней
										count += days;
									// Выходим из цикла
									else break;
								}
								// Выполняем перебор всех месяцев
								for(size_t i = 0; i < static_cast <size_t> (value); i++){
									// Увеличиваем текущее значение месяца на указанное количество дней
									result += (static_cast <uint64_t> (daysInMonths.at(month - 1)) * static_cast <uint64_t> (86400000));
									// Если месяц февраль и год является високосным
									if((month == 2) && this->leap(result))
										// Увеличиваем текущее значение года на один день
										result += 86400000;
									// Увеличиваем текущее значение месяца
									month++;
									// Если месяц выше 12-го
									if(month > 12)
										// Выполняем сброс месяца на начало
										month = 1;
								}
							}
						} break;
						// Если нам нужно получить начало недели
						case static_cast <uint8_t> (type_t::WEEK): {
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех недель
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Увеличиваем значение даты на указанное количество недель
								result += 604800000;
						} break;
						// Если нам нужно получить начало дня
						case static_cast <uint8_t> (type_t::DAY):
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех дней
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Увеличиваем значение даты на указанное количество дней
								result += 86400000;
						break;
						// Если нам нужно получить начало часа
						case static_cast <uint8_t> (type_t::HOUR):
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех часов
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Увеличиваем значение даты на указанное количество часов
								result += 3600000;
						break;
						// Если нам нужно получить начало минуты
						case static_cast <uint8_t> (type_t::MINUTES):
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех минут
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Увеличиваем значение даты на указанное количество минут
								result += 60000;
						break;
						// Если нам нужно получить начало секунды
						case static_cast <uint8_t> (type_t::SECONDS):
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех секунд
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Увеличиваем значение даты на указанное количество секунд
								result += 1000;
						break;
						// Если нам нужно получить начало миллисекунды
						case static_cast <uint8_t> (type_t::MILLISECONDS):
							// Увеличиваем значение даты на указанное количество миллисекунд
							result = (date + value);
						break;
						// Если нам нужно получить начало микросекунды
						case static_cast <uint8_t> (type_t::MICROSECONDS): {
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
							// Если текущее значение даты передано в микросекундах
							if(current == (actual + 3))
								// Увеличиваем значение даты на указанное количество микросекунд
								result = (date + value);
							// Если текущее значение даты передано в других единицах
							else {
								// Устанавливаем текущее значение даты
								result = date;
								// Увеличиваем размер даты на указанное количество микросекунд
								result *= 1000;
								// Увеличиваем значение даты на указанное количество микросекунд
								result += value;
							}
						} break;
						// Если нам нужно получить начало наносекунды
						case static_cast <uint8_t> (type_t::NANOSECONDS): {
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
							// Если текущее значение даты передано в наносекундах
							if(current == static_cast <uint8_t> (actual + 6))
								// Увеличиваем значение даты на указанное количество наносекунд
								result = (date + value);
							// Если текущее значение даты передано в других единицах
							else {
								// Устанавливаем текущее значение даты
								result = date;
								// Увеличиваем размер даты на указанное количество наносекунд
								result *= 1000000;
								// Увеличиваем значение даты на указанное количество наносекунд
								result += value;
							}
						} break;
					}
				} break;
				// Если необходимо выполнить декремент
				case static_cast <uint8_t> (offset_t::DECREMENT): {
					// Определяем тип единиц измерений
					switch(static_cast <uint8_t> (type)){
						// Если нам нужно получить начало года
						case static_cast <uint8_t> (type_t::YEAR): {
							// Устанавливаем текущее значение даты
							result = date;
							// Получаем значение текущего года
							const uint16_t year = this->year(result);
							// Выполняем перебор всех лет
							for(size_t i = 0; i < static_cast <size_t> (value); i++){
								// Если предыдущий год является високосным
								if(((year - (i + 1)) % 4) == 0)
									// Уменьшаем текущее значение года на 366 дней
									result -= (result >= 31622400000 ? 31622400000 : 0);
								// Уменьшаем текущее значение года на 365 дней
								else result -= (result >= 31536000000 ? 31536000000 : 0);
							}
						} break;
						// Если нам нужно получить начало месяца
						case static_cast <uint8_t> (type_t::MONTH): {
							// Устанавливаем текущее значение даты
							result = date;
							// Получаем значение текущего года
							const uint16_t year = this->year(result);
							// Определяем количество прошедших лет
							const uint16_t lastYears = (year - 1970);
							// Определяем количество прошедших високосных лет
							const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
							// Получаем штамп времени начала года
							const uint64_t beginYear = (
								(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
								(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
							);
							// Определяем сколько дней прошло с начала года
							const uint16_t lastDays = static_cast <uint16_t> (::floor((result - beginYear) / 86400000.L));
							{
								// Номер текущего месяца
								uint8_t month = 0;
								// Подсчитываем количество дней в предыдущих месяцах
								uint16_t count = 0, days = 0;
								// Устанавливаем флаг високосного года
								const bool leap = ((year % 4) == 0);
								// Выполняем перебор всех дней месяца
								for(uint8_t i = 0; i < daysInMonths.size(); i++){
									// Увеличиваем номер месяца
									month = (i + 1);
									// Получаем текущее количество дней с компенсацией високосного года
									days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
									// Если мы не дошли до предела
									if(lastDays > (days + count))
										// Увеличиваем количество прошедших дней
										count += days;
									// Выходим из цикла
									else break;
								}
								// Выполняем перебор всех месяцев
								for(size_t i = 0; i < static_cast <size_t> (value); i++){
									// Уменьшаем текущее значение месяца
									month--;
									// Если месяц ниже 1-го
									if(month < 1)
										// Выполняем сброс месяца на начало
										month = 12;
									// Уменьшаем текущее значение месяца на указанное количество дней
									result -= (static_cast <uint64_t> (daysInMonths.at(month - 1)) * static_cast <uint64_t> (86400000));
									// Если месяц февраль и год является високосным
									if((month == 2) && this->leap(result))
										// Уменьшаем текущее значение года на один день
										result -= (result >= 86400000 ? 86400000 : 0);
								}
							}
						} break;
						// Если нам нужно получить начало недели
						case static_cast <uint8_t> (type_t::WEEK): {
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех недель
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Уменьшаем значение даты на указанное количество недель
								result -= (result >= 604800000 ? 604800000 : 0);
						} break;
						// Если нам нужно получить начало дня
						case static_cast <uint8_t> (type_t::DAY):
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех дней
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Уменьшаем значение даты на указанное количество дней
								result -= (result >= 86400000 ? 86400000 : 0);
						break;
						// Если нам нужно получить начало часа
						case static_cast <uint8_t> (type_t::HOUR):
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех часов
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Уменьшаем значение даты на указанное количество часов
								result -= (result >= 3600000 ? 3600000 : 0);
						break;
						// Если нам нужно получить начало минуты
						case static_cast <uint8_t> (type_t::MINUTES):
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех минут
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Уменьшаем значение даты на указанное количество минут
								result -= (result >= 60000 ? 60000 : 0);
						break;
						// Если нам нужно получить начало секунды
						case static_cast <uint8_t> (type_t::SECONDS):
							// Устанавливаем текущее значение даты
							result = date;
							// Выполняем перебор всех секунд
							for(size_t i = 0; i < static_cast <size_t> (value); i++)
								// Уменьшаем значение даты на указанное количество секунд
								result -= (result >= 1000 ? 1000 : 0);
						break;
						// Если нам нужно получить начало миллисекунды
						case static_cast <uint8_t> (type_t::MILLISECONDS):
							// Уменьшаем значение даты на указанное количество миллисекунд
							result = (date >= value ? (date - value) : 0);
						break;
						// Если нам нужно получить начало микросекунды
						case static_cast <uint8_t> (type_t::MICROSECONDS): {
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
							// Если текущее значение даты передано в микросекундах
							if(current == (actual + 3))
								// Уменьшаем значение даты на указанное количество микросекунд
								result = (date >= value ? (date - value) : 0);
							// Если текущее значение даты передано в других единицах
							else {
								// Устанавливаем текущее значение даты
								result = date;
								// Увеличиваем размер даты на указанное количество микросекунд
								result *= 1000;
								// Уменьшаем значение даты на указанное количество микросекунд
								result -= (result >= value ? value : 0);
							}
						} break;
						// Если нам нужно получить начало наносекунды
						case static_cast <uint8_t> (type_t::NANOSECONDS): {
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (::floor(::log10(date)));
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
							// Если текущее значение даты передано в наносекундах
							if(current == (actual + 6))
								// Уменьшаем значение даты на указанное количество наносекунд
								result = (date >= value ? (date - value) : 0);
							// Если текущее значение даты передано в других единицах
							else {
								// Устанавливаем текущее значение даты
								result = date;
								// Увеличиваем размер даты на указанное количество наносекунд
								result *= 1000000;
								// Уменьшаем значение даты на указанное количество наносекунд
								result -= (result >= value ? value : 0);
							}
						} break;
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * offset Метод смещения текущей даты на указанное количество единиц времени
 * @param value   значение на которое производится смещение
 * @param type    тип единиц измерений даты
 * @param offset  направление смещения
 * @param storage хранение значение времени
 * @return        результат вычисления в формате UnixTimestamp
 */
uint64_t awh::Chrono::offset(const uint64_t value, const type_t type, const offset_t offset, const storage_t storage) const noexcept {
	// Выполняем смещение текущей даты на указанное количество единиц времени
	return this->offset(this->timestamp(type_t::MILLISECONDS, storage), value, type, offset);
}
/**
 * seconds Метод получения текстового значения времени 
 * @param seconds количество секунд для конвертации
 * @return        обозначение времени с указанием размерности
 */
string awh::Chrono::seconds(const double seconds) const noexcept {
	// Результат работы функции
	string result = "0s";
	// Если количество секунд передано
	if(seconds > 0.){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Шаблон минуты
			const double minute = 60.;
			// Шаблон часа
			const double hour = 3600.;
			// Шаблон дня
			const double day = 86400.;
			// Шаблон недели
			const double week = 604800.;
			// Шаблон месяца
			const double month = 2628000.;
			// Шаблон года
			const double year = 31536000.;
			// Если переданное значение соответствует году
			if(seconds >= year){
				// Выполняем преобразование в количество лет
				result = this->_fmk->noexp(seconds / year, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'y');
			// Если переданное значение соответствует месяцу
			} else if((seconds >= month) && (seconds < year)) {
				// Выполняем преобразование в количество месяцев
				result = this->_fmk->noexp(seconds / month, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'M');
			// Если переданное значение соответствует недели
			} else if((seconds >= week) && (seconds < month)) {
				// Выполняем преобразование в количество недель
				result = this->_fmk->noexp(seconds / week, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'w');
			// Если переданное значение соответствует дням
			} else if((seconds >= day) && (seconds < week)) {
				// Выполняем преобразование в количество дней
				result = this->_fmk->noexp(seconds / day, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'd');
			// Если переданное значение соответствует часам
			} else if((seconds >= hour) && (seconds < day)) {
				// Выполняем преобразование в количество часов
				result = this->_fmk->noexp(seconds / hour, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'h');
			// Если переданное значение соответствует минут
			} else if((seconds >= minute) && (seconds < hour)) {
				// Выполняем преобразование в количество минут
				result = this->_fmk->noexp(seconds / minute, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'm');
			// Если переданное значение соответствует секундам
			} else {
				// Выполняем преобразование в количество секунд
				result = this->_fmk->noexp(seconds, true);
				// Добавляем наименование единицы измерения
				result.append(1, 's');
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * seconds Метод получения размера в секундах из строки
 * @param seconds строка обозначения размерности (s, m, h, d, w, M, y)
 * @return        размер в секундах
 */
double awh::Chrono::seconds(const string & seconds) const noexcept {
	// Количество секунд
	double result = 0.;
	// Если строка с секундами передана
	if(!seconds.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск нужного нам регулярного выражения
			auto i = this->_expressions.find(format_t::S);
			// Если регулярное выражение получено
			if(i != this->_expressions.end()){
				// Выполняем блокировку потока
				const lock_guard <recursive_mutex> lock(this->_mtx.parse);
				// Создаём объект матчинга
				regmatch_t match[i->second.re_nsub + 1];
				// Выполняем разбор регулярного выражения
				const int32_t error = ::pcre2_regexec(&i->second, seconds.c_str(), i->second.re_nsub + 1, match, REG_NOTEMPTY);
				// Если ошибок не получено
				if(error == 0){
					// Обозначение размерности числа
					string label = "";
					// Размерность времени и размерность секунд
					double dimension = 1., value = 0.;
					// Выполняем перебор всех полученных вариантов
					for(uint8_t j = 1; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
						// Если результат получен
						if(match[j].rm_eo > match[j].rm_so){
							// Определяем номер найденного элемента
							switch(j){
								// Если мы получили само число
								case 1:
									// Получаем значение числа
									value = ::stod(string(seconds.c_str() + match[j].rm_so, match[j].rm_eo - match[j].rm_so));
								break;
								// Если мы получили размерность числа
								case 2: {
									// Получаем обозначение размерности числа
									label.assign(seconds.c_str() + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
									// Если мы получили секунды
									if(label.front() == 's')
										// Выполняем установку множителя
										dimension = 1.;
									// Если мы получили минуты
									else if(label.front() == 'm')
										// Выполняем установку множителя
										dimension = 60.;
									// Если мы получили часы
									else if(label.front() == 'h')
										// Выполняем установку множителя
										dimension = 3600.;
									// Если мы получили дни
									else if(label.front() == 'd')
										// Выполняем установку множителя
										dimension = 86400.;
									// Если мы получили недели
									else if(label.front() == 'w')
										// Выполняем установку множителя
										dimension = 604800.;
									// Если мы получили месяцы
									else if(label.front() == 'M')
										// Выполняем установку множителя
										dimension = 2629746.;
									// Если мы получили годы
									else if(label.front() == 'y')
										// Выполняем установку множителя
										dimension = 31536000.;
								} break;
							}
						}
					}
					// Если время установлено тогда расчитываем количество секунд
					if(value > -1.)
						// Выполняем получение количества секунд
						result = (value * dimension);
					// Размер буфера по умолчанию
					else result = value;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * h12 Метод извлечения статуса 12-и часового формата времени
 * @param date дата для проверки
 */
awh::Chrono::h12_t awh::Chrono::h12(const uint64_t date) const noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаем структуру времени
			dt_t dt;
			// Устанавливаем количество миллисекунд
			this->makeDate(date, dt);
			// Получаем текущий статус 12-и часового формата времени
			return dt.h12;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return h12_t::AM;
}
/**
 * h12 Метод извлечения текущего статуса 12-и часового формата времени
 * @param storage хранение значение времени
 * @return        текущее установленное значение статуса 12-и часового формата времени
 */
awh::Chrono::h12_t awh::Chrono::h12(const storage_t storage) const noexcept {
	// Результат работы функции
	h12_t result = h12_t::AM;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL):
				// Получаем текущий статус 12-и часового формата времени
				result = this->_dt.h12;
			break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL):
				// Выполняем извлечение текущего статуса 12-и часового формата времени
				result = this->h12(this->timestamp(type_t::MILLISECONDS, storage));
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * year Метод извлечения значения года
 * @param date дата для извлечения года
 */
uint16_t awh::Chrono::year(const uint64_t date) const noexcept {
	// Выполняем извлечение значения годы из даты
	return static_cast <uint16_t> (
		static_cast <uint32_t> (
			::floor(
				(
					date - (
						static_cast <uint64_t> (
							::ceil(date / 126489600000.L)
						) * static_cast <uint64_t> (86400000)
					)
				) / 31536000000.L
			)
		) + 1970
	);
}
/**
 * year Метод получение текущего значения года
 * @param storage хранение значение времени
 * @return        текущее значение года
 */
uint16_t awh::Chrono::year(const storage_t storage) const noexcept {
	// Результат работы функции
	uint16_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL):
				// Получаем установленное значение года
				result = this->_dt.year;
			break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL):
				// Выполняем извлечение текущее значение года
				result = this->year(this->timestamp(type_t::MILLISECONDS, storage));
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * dst Метод проверки принадлежит ли дата к лету
 * @param date дата для проверки
 * @return     результат проверки
 */
bool awh::Chrono::dst(const uint64_t date) const noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем значение текущего года
			const uint16_t year = this->year(date);
			// Определяем количество прошедших лет
			const uint16_t lastYears = (year - 1970);
			// Определяем количество прошедших високосных лет
			const uint16_t leapCount = static_cast <uint16_t> (::round((lastYears - 1) / 4.));
			// Получаем штамп времени начала года
			const uint64_t beginYear = (
				(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
				(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
			);
			// Определяем сколько дней прошло с начала года
			const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
			{
				// Номер текущего месяца
				uint8_t month = 0;
				// Подсчитываем количество дней в предыдущих месяцах
				uint16_t count = 0, days = 0;
				// Устанавливаем флаг високосного года
				const bool leap = ((year % 4) == 0);
				// Выполняем перебор всех дней месяца
				for(uint8_t i = 0; i < daysInMonths.size(); i++){
					// Увеличиваем номер месяца
					month = (i + 1);
					// Получаем текущее количество дней с компенсацией високосного года
					days = (static_cast <uint16_t> (daysInMonths.at(i)) + ((i == 1) && leap ? 1 : 0));
					// Если мы не дошли до предела
					if(lastDays > (days + count))
						// Увеличиваем количество прошедших дней
						count += days;
					// Выходим из цикла
					else break;
				}
				// Если у нас на дворе лето, устанавливаем флаг
				return ((month > 5) && (month < 9));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * dst Метод проверки стоит ли на дворе лето
 * @param storage хранение значение времени
 * @return        результат проверки
 */
bool awh::Chrono::dst(const storage_t storage) const noexcept {
	// Выполняем проверку стоит ли на дворе лето
	return this->dst(this->timestamp(type_t::MILLISECONDS, storage));
}
/**
 * leap Метод проверки является ли год високосным
 * @param date дата для проверки
 * @return     результат проверки
 */
bool awh::Chrono::leap(const uint64_t date) const noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Устанавливаем флаг високосного года
			return ((this->year(date) % 4) == 0);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * leap Метод проверки является ли текущий год високосным
 * @param storage хранение значение времени
 * @return        результат проверки
 */
bool awh::Chrono::leap(const storage_t storage) const noexcept {
	// Выполняем проверку является ли текущий год високосным
	return this->leap(this->timestamp(type_t::MILLISECONDS, storage));
}
/**
 * set Метод установки данных даты и времени
 * @param buffer бинарный буфер данных
 * @param size   размер бинарного буфера
 * @param unit   элементы данных для установки
 * @param text   данные переданы в виде текста
 */
void awh::Chrono::set(const void * buffer, const size_t size, const unit_t unit, const bool text) noexcept {
	// Если данные переданы правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard <mutex> lock(this->_mtx.date);
			// Определяем элементы устанавливаемых данных
			switch(static_cast <uint8_t> (unit)){
				// Если требуется установить номер текущего дня недели от 1 до 7
				case static_cast <uint8_t> (unit_t::DAY): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер дня недели
						const string * day = reinterpret_cast <const string *> (buffer);
						// Если номер дня недели передан
						if(!day->empty()){
							// Если день передан в виде числа
							if(this->_fmk->is(* day, fmk_t::check_t::NUMBER)){
								// День для установки
								const uint8_t num = static_cast <uint8_t> (::stoul(* day));
								// Если номер дня недели передан
								if((num > 0) && (num < 8))
									// Устанавливаем номер дня недели
									this->_dt.day = num;
							// Если день передан в виде названия
							} else {
								// Выполняем перебор всего списка дней недели
								for(size_t i = 0; i < nameDays.size(); i++){
									// Получаем название дня
									const auto & name = nameDays.at(i);
									// Если мы нашли нужный нам день недели
									if(this->_fmk->compare(* day, name.first) || this->_fmk->compare(* day, name.second)){
										// Выполняем установку номера дня недели
										this->_dt.day = static_cast <uint8_t> (i + 1);
										// Выходим из цикла
										break;
									}
								}
							}
						}
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Номер текущего дня недели
							uint8_t day = 0;
							// Выполняем получение номера текущего дня недели
							::memcpy(&day, buffer, sizeof(day));
							// Если номер дня недели передан
							if((day > 0) && (day < 8))
								// Устанавливаем номер дня недели
								this->_dt.day = day;
						}
					}
				} break;
				// Если требуется установить число месяца от 1 до 31
				case static_cast <uint8_t> (unit_t::DATE): {
					// Если данные переданы в виде текста
					if(text){
						// Дата для установки
						const uint8_t date = static_cast <uint8_t> (::stoul(* reinterpret_cast <const string *> (buffer)));
						// Если дата передана в нужном виде
						if((date > 0) && (date < 32))
							// Устанавливаем дату
							this->_dt.date = date;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Дата для установки
							uint8_t date = 0;
							// Выполняем получение даты
							::memcpy(&date, buffer, sizeof(date));
							// Если дата передана в нужном виде
							if((date > 0) && (date < 32))
								// Устанавливаем дату
								this->_dt.date = date;
						}
					}
				} break;
				// Если требуется установить полное обозначение года
				case static_cast <uint8_t> (unit_t::YEAR): {
					// Если данные переданы в виде текста
					if(text){
						// Год для установки
						const uint16_t year = static_cast <uint16_t> (::stoul(* reinterpret_cast <const string *> (buffer)));
						// Если год передан
						if(year > 0){
							// Устанавливаем год
							this->_dt.year = year;
							// Устанавливаем флаг високосного года
							this->_dt.leap = ((this->_dt.year % 4) == 0);
						}
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint16_t)){
							// Год для установки
							uint16_t year = 0;
							// Выполняем получение года
							::memcpy(&year, buffer, sizeof(year));
							// Если год передан
							if(year > 0){
								// Устанавливаем год
								this->_dt.year = year;
								// Устанавливаем флаг високосного года
								this->_dt.leap = ((this->_dt.year % 4) == 0);
							}
						}
					}
				} break;
				// Если требуется установить количество часов от 0 до 23
				case static_cast <uint8_t> (unit_t::HOUR): {
					// Если данные переданы в виде текста
					if(text){
						// Час времени для установки
						const uint8_t hour = static_cast <uint8_t> (::stoul(* reinterpret_cast <const string *> (buffer)));
						// Если количество часов передано
						if(hour < 24)
							// Устанавливаем количество часов
							this->_dt.hour = hour;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Час времени для установки
							uint8_t hour = 0;
							// Выполняем получение часа
							::memcpy(&hour, buffer, sizeof(hour));
							// Если количество часов передано
							if(hour < 24)
								// Устанавливаем количество часов
								this->_dt.hour = hour;
						}
					}
				} break;
				// Если требуется установить количество прошедвших дней от 1 января
				case static_cast <uint8_t> (unit_t::DAYS): {
					// Если данные переданы в виде текста
					if(text){
						// Количество прошедвших дней для установки
						const uint16_t days = static_cast <uint16_t> (::stoul(* reinterpret_cast <const string *> (buffer)));
						// Если количество прошедвших дней от 1 января
						if(days < 366)
							// Устанавливаем количество количество прошедвших дней
							this->_dt.days = days;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint16_t)){
							// Количество прошедвших дней для установки
							uint16_t days = 0;
							// Выполняем получение количество прошедвших дней
							::memcpy(&days, buffer, sizeof(days));
							// Если количество прошедвших дней от 1 января
							if(days < 366)
								// Устанавливаем количество количество прошедвших дней
								this->_dt.days = days;
						}
					}
				} break;
				// Если требуется установить номер месяца от 1 до 12 (начиная с Января)
				case static_cast <uint8_t> (unit_t::MONTH): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем название месяца
						const string * month = reinterpret_cast <const string *> (buffer);
						// Если месяц передан
						if(!month->empty()){
							// Если месяц передан в виде числа
							if(this->_fmk->is(* month, fmk_t::check_t::NUMBER)){
								// Месяц для установки
								const uint8_t num = static_cast <uint8_t> (::stoul(* month));
								// Если месяц передан
								if((num > 0) && (num < 13))
									// Устанавливаем месяц
									this->_dt.month = num;
							// Если день передан в виде названия
							} else {
								// Выполняем перебор всего списка месяцев
								for(size_t i = 0; i < nameMonths.size(); i++){
									// Получаем название месяца
									const auto & name = nameMonths.at(i);
									// Если мы нашли нужный нам месяц
									if(this->_fmk->compare(* month, name.first) || this->_fmk->compare(* month, name.second)){
										// Устанавливаем месяц
										this->_dt.month = static_cast <uint8_t> (i + 1);
										// Выходим из цикла
										break;
									}
								}
							}
						}
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Номер месяца для установки
							uint8_t month = 0;
							// Выполняем получение номера месяца
							::memcpy(&month, buffer, sizeof(month));
							// Если месяц передан
							if((month > 0) && (month < 13))
								// Устанавливаем месяц
								this->_dt.month = month;
						}
					}
				} break;
				// Если требуется установить количество недель прошедших с начала года
				case static_cast <uint8_t> (unit_t::WEEKS): {
					// Если данные переданы в виде текста
					if(text){
						// Номер дня недели для установки
						const uint8_t weeks = static_cast <uint8_t> (::stoul(* reinterpret_cast <const string *> (buffer)));
						// Если количество недель прошедших с начала года
						if(weeks < 53)
							// Устанавливаем количество недель прошедших с начала года
							this->_dt.weeks = weeks;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Номер дня недели для установки
							uint8_t weeks = 0;
							// Выполняем получение номера дня недели
							::memcpy(&weeks, buffer, sizeof(weeks));
							// Если количество недель прошедших с начала года
							if(weeks < 53)
								// Устанавливаем количество недель прошедших с начала года
								this->_dt.weeks = weeks;
						}
					}
				} break;
				// Если требуется установить количество смещение временной зоны в секундах относительно UTC
				case static_cast <uint8_t> (unit_t::OFFSET): {
					// Если данные переданы в виде текста
					if(text)
						// Устанавливаем смещение временной зоны в секундах относительно UTC
						this->_dt.offset = static_cast <int32_t> (::stoul(* reinterpret_cast <const string *> (buffer)));
					// Если данные переданы в виде числа
					else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(int32_t)){
							// Смещение временной зоны в секундах относительно UTC
							int32_t offset = 0;
							// Выполняем смещение временной зоны в секундах относительно UTC
							::memcpy(&offset, buffer, sizeof(offset));
							// Устанавливаем смещение временной зоны в секундах относительно UTC
							this->_dt.offset = offset;
						}
					}
				} break;
				// Если требуется установить количество минут от 0 до 59
				case static_cast <uint8_t> (unit_t::MINUTES): {
					// Если данные переданы в виде текста
					if(text){
						// Количество минут для установки
						const uint8_t minutes = static_cast <uint8_t> (::stoul(* reinterpret_cast <const string *> (buffer)));
						// Если количество минут передано
						if(minutes < 60)
							// Устанавливаем количество минут
							this->_dt.minutes = minutes;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Количество минут для установки
							uint8_t minutes = 0;
							// Выполняем получение минут
							::memcpy(&minutes, buffer, sizeof(minutes));
							// Если количество минут передано
							if(minutes < 60)
								// Устанавливаем количество минут
								this->_dt.minutes = minutes;
						}
					}
				} break;
				// Если требуется установить количество секунд от 0 до 59
				case static_cast <uint8_t> (unit_t::SECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Количество секунд для установки
						const uint8_t seconds = static_cast <uint8_t> (::stoul(* reinterpret_cast <const string *> (buffer)));
						// Если количество секунд передано
						if(seconds < 60)
							// Устанавливаем количество секунд
							this->_dt.seconds = seconds;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Количество секунд для установки
							uint8_t seconds = 0;
							// Выполняем получение секунд
							::memcpy(&seconds, buffer, sizeof(seconds));
							// Если количество секунд передано
							if(seconds < 60)
								// Устанавливаем количество секунд
								this->_dt.seconds = seconds;
						}
					}
				} break;
				// Если требуется установить количество наносекунд
				case static_cast <uint8_t> (unit_t::NANOSECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Количество наносекунд для установки
						const uint64_t nanoseconds = static_cast <uint64_t> (::stoull(* reinterpret_cast <const string *> (buffer)));
						// Получаем текущее значение размерности даты
						const uint8_t current = static_cast <uint8_t> (::floor(::log10(nanoseconds)));
						// Получаем размерность актуальной размерности даты
						const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
						// Если текущее значение даты передано в наносекундах
						if(current >= (actual + 6))
							// Устанавливаем количество наносекунд
							this->_dt.nanoseconds = (nanoseconds % 1000000);
						// Устанавливаем количество наносекунд
						else this->_dt.nanoseconds = nanoseconds;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint64_t)){
							// Количество наносекунд для установки
							uint64_t nanoseconds = 0;
							// Выполняем получение наносекунд
							::memcpy(&nanoseconds, buffer, sizeof(nanoseconds));
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (::floor(::log10(nanoseconds)));
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
							// Если текущее значение даты передано в наносекундах
							if(current >= (actual + 6))
								// Устанавливаем количество наносекунд
								this->_dt.nanoseconds = (nanoseconds % 1000000);
							// Устанавливаем количество наносекунд
							else this->_dt.nanoseconds = nanoseconds;
						}
					}
				} break;
				// Если требуется установить количество микросекунд
				case static_cast <uint8_t> (unit_t::MICROSECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Количество микросекунд для установки
						const uint64_t microseconds = static_cast <uint64_t> (::stoull(* reinterpret_cast <const string *> (buffer)));
						// Получаем текущее значение размерности даты
						const uint8_t current = static_cast <uint8_t> (::floor(::log10(microseconds)));
						// Получаем размерность актуальной размерности даты
						const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
						// Если текущее значение даты передано в микросекундах
						if(current >= (actual + 3))
							// Устанавливаем количество микросекунд
							this->_dt.microseconds = (microseconds % 1000);
						// Устанавливаем количество микросекунд
						else this->_dt.microseconds = microseconds;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint64_t)){
							// Количество микросекунд для установки
							uint64_t microseconds = 0;
							// Выполняем получение микросекунд
							::memcpy(&microseconds, buffer, sizeof(microseconds));
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (::floor(::log10(microseconds)));
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (::floor(::log10(this->timestamp(type_t::MILLISECONDS))));
							// Если текущее значение даты передано в микросекундах
							if(current >= (actual + 3))
								// Устанавливаем количество микросекунд
								this->_dt.microseconds = (microseconds % 1000);
							// Устанавливаем количество микросекунд
							else this->_dt.microseconds = microseconds;
						}
					}
				} break;
				// Если требуется установить количество миллисекунд
				case static_cast <uint8_t> (unit_t::MILLISECONDS): {
					// Если данные переданы в виде текста
					if(text)
						// Устанавливаем количество миллисекунд
						this->_dt.milliseconds = static_cast <uint32_t> (::stoul(* reinterpret_cast <const string *> (buffer)));
					// Если данные переданы в виде числа
					else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint32_t)){
							// Количество миллисекунд для установки
							uint32_t milliseconds = 0;
							// Выполняем получение миллисекунд
							::memcpy(&milliseconds, buffer, sizeof(milliseconds));
							// Устанавливаем количество миллисекунд
							this->_dt.milliseconds = milliseconds;
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
}
/**
 * get Метод извлечения данных даты и времени
 * @param buffer бинарный буфер данных
 * @param size   размер бинарного буфера
 * @param date   дата для обработки
 * @param unit   элементы данных для установки
 * @param text   данные переданы в виде текста
 */
void awh::Chrono::get(void * buffer, const size_t size, const uint64_t date, const unit_t unit, const bool text) const noexcept {
	// Если данные переданы правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем элементы устанавливаемых данных
			switch(static_cast <uint8_t> (unit)){
				// Если требуется установить номер текущего дня недели от 1 до 7
				case static_cast <uint8_t> (unit_t::DAY): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение текущего дня недели
							(* result) = nameDays.at(dt.day - 1).second;
						// Выполняем копирование текущего дня недели
						} else ::memcpy(buffer, &dt.day, sizeof(dt.day));
					}
				} break;
				// Если требуется установить число месяца от 1 до 31
				case static_cast <uint8_t> (unit_t::DATE): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение текущего значения даты
							(* result) = std::to_string(dt.date);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем копирование текущего значения даты
						} else ::memcpy(buffer, &dt.date, sizeof(dt.date));
					}
				} break;
				// Если требуется установить полное обозначение года
				case static_cast <uint8_t> (unit_t::YEAR): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint16_t)){
						// Получаем значение текущего года
						const uint16_t year = this->year(date);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение текущего значения года
							(* result) = std::to_string(year);
						// Выполняем копирование текущего значения года
						} else ::memcpy(buffer, &year, sizeof(year));
					}
				} break;
				// Если требуется установить количество часов от 0 до 23
				case static_cast <uint8_t> (unit_t::HOUR): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количество часов от 0 до 23
							(* result) = std::to_string(dt.hour);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем копирование количество часов от 0 до 23
						} else ::memcpy(buffer, &dt.hour, sizeof(dt.hour));
					}
				} break;
				// Если требуется установить количество прошедвших дней от 1 января
				case static_cast <uint8_t> (unit_t::DAYS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint16_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количество прошедвших дней от 1 января
							(* result) = std::to_string(dt.days);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем копирование количество прошедвших дней от 1 января
						} else ::memcpy(buffer, &dt.days, sizeof(dt.days));
					}
				} break;
				// Если требуется установить номер месяца от 1 до 12 (начиная с Января)
				case static_cast <uint8_t> (unit_t::MONTH): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение названия месяца
							(* result) = nameMonths.at(dt.month - 1).second;
						// Выполняем копирование текущего значения месяца
						} else ::memcpy(buffer, &dt.month, sizeof(dt.month));
					}
				} break;
				// Если требуется установить количество недель прошедших с начала года
				case static_cast <uint8_t> (unit_t::WEEKS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количество недель прошедших с начала года
							(* result) = std::to_string(dt.weeks);
						// Выполняем копирование количество недель прошедших с начала года
						} else ::memcpy(buffer, &dt.weeks, sizeof(dt.weeks));
					}
				} break;
				// Если требуется установить смещение временной зоны в секундах относительно UTC
				case static_cast <uint8_t> (unit_t::OFFSET): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(int32_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение смещения временной зоны в секундах относительно UTC
							(* result) = std::to_string(dt.offset);
						// Выполняем копирование смещения временной зоны в секундах относительно UTC
						} else ::memcpy(buffer, &dt.offset, sizeof(dt.offset));
					}
				} break;
				// Если требуется установить количество минут от 0 до 59
				case static_cast <uint8_t> (unit_t::MINUTES): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества минут от 0 до 59
							(* result) = std::to_string(dt.minutes);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества минут от 0 до 59
						} else ::memcpy(buffer, &dt.minutes, sizeof(dt.minutes));
					}
				} break;
				// Если требуется установить количество секунд от 0 до 59
				case static_cast <uint8_t> (unit_t::SECONDS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества секунд от 0 до 59
							(* result) = std::to_string(dt.seconds);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества секунд от 0 до 59
						} else ::memcpy(buffer, &dt.seconds, sizeof(dt.seconds));
					}
				} break;
				// Если требуется установить количество наносекунд
				case static_cast <uint8_t> (unit_t::NANOSECONDS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint64_t)){
						// Количество наносекунд
						const uint64_t nanoseconds = (date % 1000000);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества наносекунд
							(* result) = std::to_string(nanoseconds);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 5, '0');
							// Если первого нуля нет
							else if(result->length() == 2)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 4, '0');
							// Если первого нуля нет
							else if(result->length() == 3)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 3, '0');
							// Если первого нуля нет
							else if(result->length() == 4)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 2, '0');
							// Если первого нуля нет
							else if(result->length() == 5)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества наносекунд
						} else ::memcpy(buffer, &nanoseconds, sizeof(nanoseconds));
					}
				} break;
				// Если требуется установить количество микросекунд
				case static_cast <uint8_t> (unit_t::MICROSECONDS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint64_t)){
						// Количество микросекунд
						const uint64_t microseconds = (date % 1000);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества микросекунд
							(* result) = std::to_string(microseconds);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 2, '0');
							// Если первого нуля нет
							else if(result->length() == 2)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества микросекунд
						} else ::memcpy(buffer, &microseconds, sizeof(microseconds));
					}
				} break;
				// Если требуется установить количество миллисекунд
				case static_cast <uint8_t> (unit_t::MILLISECONDS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint32_t)){
						// Создаем структуру времени
						dt_t dt;
						// Устанавливаем количество миллисекунд
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества миллисекунд
							(* result) = std::to_string(dt.milliseconds);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 2, '0');
							// Если первого нуля нет
							else if(result->length() == 2)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества миллисекунд
						} else ::memcpy(buffer, &dt.milliseconds, sizeof(dt.milliseconds));
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
}
/**
 * get Метод извлечения данных даты и времени
 * @param buffer  бинарный буфер данных
 * @param size    размер бинарного буфера
 * @param unit    элементы данных для установки
 * @param text    данные переданы в виде текста
 * @param storage хранение значение времени
 */
void awh::Chrono::get(void * buffer, const size_t size, const unit_t unit, const bool text, const storage_t storage) const noexcept {
	// Если данные переданы правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем элементы устанавливаемых данных
			switch(static_cast <uint8_t> (unit)){
				// Если требуется установить номер текущего дня недели от 1 до 7
				case static_cast <uint8_t> (unit_t::DAY): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем получение номера текущего дня недели
								(* result) = nameDays.at(this->_dt.day - 1).second;
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Текущий номер дня недели
								uint8_t day = 0;
								// Выполняем извлечение текущего дня недели
								this->get(&day, sizeof(day), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем получение номера текущего дня недели
								(* result) = nameDays.at(day - 1).second;
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.day))
									// Выполняем копирование текущего дня недели
									::memcpy(buffer, &this->_dt.day, sizeof(this->_dt.day));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Текущий номер дня недели
									uint8_t day = 0;
									// Выполняем извлечение текущего дня недели
									this->get(&day, sizeof(day), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование текущего дня недели
									::memcpy(buffer, &day, sizeof(day));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить число месяца от 1 до 31
				case static_cast <uint8_t> (unit_t::DATE): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущего значения даты
								(* result) = std::to_string(this->_dt.date);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Число месяца от 1 до 31
								uint8_t date = 0;
								// Выполняем извлечение числа месяца
								this->get(&date, sizeof(date), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущего значения даты
								(* result) = std::to_string(date);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.date))
									// Выполняем копирование текущего значения даты
									::memcpy(buffer, &this->_dt.date, sizeof(this->_dt.date));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Число месяца от 1 до 31
									uint8_t date = 0;
									// Выполняем извлечение числа месяца
									this->get(&date, sizeof(date), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование числа месяца
									::memcpy(buffer, &date, sizeof(date));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить полное обозначение года
				case static_cast <uint8_t> (unit_t::YEAR): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущего значения года
								(* result) = std::to_string(this->_dt.year);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Полное обозначение года
								uint16_t year = 0;
								// Выполняем извлечение года
								this->get(&year, sizeof(year), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущего значения года
								(* result) = std::to_string(year);
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.year))
									// Выполняем копирование текущего значения года
									::memcpy(buffer, &this->_dt.year, sizeof(this->_dt.year));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint16_t)){
									// Полное обозначение года
									uint16_t year = 0;
									// Выполняем извлечение года
									this->get(&year, sizeof(year), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование текущего значения года
									::memcpy(buffer, &year, sizeof(year));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество часов от 0 до 23
				case static_cast <uint8_t> (unit_t::HOUR): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущее значение часа
								(* result) = std::to_string(this->_dt.hour);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Текущее значение часа
								uint8_t hour = 0;
								// Выполняем извлечение текущее значение часа
								this->get(&hour, sizeof(hour), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущее значение часа
								(* result) = std::to_string(hour);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.hour))
									// Выполняем копирование текущее значение часа
									::memcpy(buffer, &this->_dt.hour, sizeof(this->_dt.hour));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Текущее значение часа
									uint8_t hour = 0;
									// Выполняем извлечение текущее значение часа
									this->get(&hour, sizeof(hour), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование текущее значение часа
									::memcpy(buffer, &hour, sizeof(hour));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество прошедвших дней от 1 января
				case static_cast <uint8_t> (unit_t::DAYS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование количество прошедвших дней от 1 января
								(* result) = std::to_string(this->_dt.days);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество прошедвших дней от 1 января
								uint16_t days = 0;
								// Выполняем извлечение количества прошедвших дней от 1 января
								this->get(&days, sizeof(days), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование количество прошедвших дней от 1 января
								(* result) = std::to_string(days);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.days))
									// Выполняем копирование количество прошедвших дней от 1 января
									::memcpy(buffer, &this->_dt.days, sizeof(this->_dt.days));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint16_t)){
									// Количество прошедвших дней от 1 января
									uint16_t days = 0;
									// Выполняем извлечение количества прошедвших дней от 1 января
									this->get(&days, sizeof(days), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количество прошедвших дней от 1 января
									::memcpy(buffer, &days, sizeof(days));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить номер месяца от 1 до 12 (начиная с Января)
				case static_cast <uint8_t> (unit_t::MONTH): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем название месяца
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем получение названия месяца
								(* result) = nameMonths.at(this->_dt.month - 1).second;
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Текущее значение месяца
								uint8_t month = 0;
								// Выполняем извлечение текущего значения месяца
								this->get(&month, sizeof(month), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем получение названия месяца
								(* result) = nameMonths.at(month - 1).second;
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.month))
									// Выполняем копирование текущего значения месяца
									::memcpy(buffer, &this->_dt.month, sizeof(this->_dt.month));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Текущее значение месяца
									uint8_t month = 0;
									// Выполняем извлечение текущего значения месяца
									this->get(&month, sizeof(month), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование текущего значения месяца
									::memcpy(buffer, &month, sizeof(month));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество недель прошедших с начала года
				case static_cast <uint8_t> (unit_t::WEEKS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущего количества недель прошедших с начала года
								(* result) = std::to_string(this->_dt.weeks);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество недель прошедших с начала года
								uint8_t weeks = 0;
								// Выполняем извлечение количества недель прошедших с начала года
								this->get(&weeks, sizeof(weeks), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущего количества недель прошедших с начала года
								(* result) = std::to_string(weeks);
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.weeks))
									// Выполняем копирование текущего количества недель прошедших с начала года
									::memcpy(buffer, &this->_dt.weeks, sizeof(this->_dt.weeks));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Количество недель прошедших с начала года
									uint8_t weeks = 0;
									// Выполняем извлечение количества недель прошедших с начала года
									this->get(&weeks, sizeof(weeks), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количества недель прошедших с начала года
									::memcpy(buffer, &weeks, sizeof(weeks));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить смещение временной зоны в секундах относительно UTC
				case static_cast <uint8_t> (unit_t::OFFSET): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование смещение временной зоны в секундах относительно UTC
								(* result) = std::to_string(this->_dt.offset);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Смещение временной зоны в секундах относительно UTC
								int32_t offset = 0;
								// Выполняем извлечение смещения временной зоны в секундах относительно UTC
								this->get(&offset, sizeof(offset), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование смещение временной зоны в секундах относительно UTC
								(* result) = std::to_string(offset);
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.offset))
									// Выполняем копирование смещение временной зоны в секундах относительно UTC
									::memcpy(buffer, &this->_dt.offset, sizeof(this->_dt.offset));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(int32_t)){
									// Смещение временной зоны в секундах относительно UTC
									int32_t offset = 0;
									// Выполняем извлечение смещения временной зоны в секундах относительно UTC
									this->get(&offset, sizeof(offset), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование смещение временной зоны в секундах относительно UTC
									::memcpy(buffer, &offset, sizeof(offset));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество минут от 0 до 59
				case static_cast <uint8_t> (unit_t::MINUTES): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущее количество минут
								(* result) = std::to_string(this->_dt.minutes);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество минут от 0 до 59
								uint8_t minutes = 0;
								// Выполняем извлечение количество минут от 0 до 59
								this->get(&minutes, sizeof(minutes), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущее количество минут
								(* result) = std::to_string(minutes);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.minutes))
									// Выполняем копирование текущее количество минут
									::memcpy(buffer, &this->_dt.minutes, sizeof(this->_dt.minutes));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Количество минут от 0 до 59
									uint8_t minutes = 0;
									// Выполняем извлечение количество минут от 0 до 59
									this->get(&minutes, sizeof(minutes), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количество минут от 0 до 59
									::memcpy(buffer, &minutes, sizeof(minutes));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество секунд от 0 до 59
				case static_cast <uint8_t> (unit_t::SECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущее количество секунд
								(* result) = std::to_string(this->_dt.seconds);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество секунд от 0 до 59
								uint8_t seconds = 0;
								// Выполняем извлечение количество секунд от 0 до 59
								this->get(&seconds, sizeof(seconds), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущее количество секунд
								(* result) = std::to_string(seconds);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.seconds))
									// Выполняем копирование текущее количество секунд
									::memcpy(buffer, &this->_dt.seconds, sizeof(this->_dt.seconds));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Количество секунд от 0 до 59
									uint8_t seconds = 0;
									// Выполняем извлечение количество секунд от 0 до 59
									this->get(&seconds, sizeof(seconds), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количество секунд от 0 до 59
									::memcpy(buffer, &seconds, sizeof(seconds));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество наносекунд
				case static_cast <uint8_t> (unit_t::NANOSECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование количество наносекунд
								(* result) = std::to_string(this->_dt.nanoseconds);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL):
								// Выполняем копирование количество наносекунд
								(* result) = std::to_string(this->timestamp(type_t::NANOSECONDS) % 1000000);
							break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 5, '0');
						// Если первого нуля нет
						else if(result->length() == 2)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 4, '0');
						// Если первого нуля нет
						else if(result->length() == 3)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 3, '0');
						// Если первого нуля нет
						else if(result->length() == 4)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 2, '0');
						// Если первого нуля нет
						else if(result->length() == 5)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.nanoseconds))
									// Выполняем копирование количество наносекунд
									::memcpy(buffer, &this->_dt.nanoseconds, sizeof(this->_dt.nanoseconds));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint64_t)){
									// Количество наносекунд
									const uint64_t nanoseconds = (this->timestamp(type_t::NANOSECONDS) % 1000000);
									// Выполняем копирование количество наносекунд
									::memcpy(buffer, &nanoseconds, sizeof(nanoseconds));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество микросекунд
				case static_cast <uint8_t> (unit_t::MICROSECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Выполняем копирование количество микросекунд
								(* result) = std::to_string(this->_dt.microseconds);
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL):
								// Выполняем копирование количество микросекунд
								(* result) = std::to_string(this->timestamp(type_t::MICROSECONDS) % 1000);
							break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 2, '0');
						// Если первого нуля нет
						else if(result->length() == 2)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.microseconds))
									// Получаем текущее количество микросекунд
									::memcpy(buffer, &this->_dt.microseconds, sizeof(this->_dt.microseconds));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint64_t)){
									// Количество микросекунд
									const uint64_t microseconds = (this->timestamp(type_t::MICROSECONDS) % 1000);
									// Выполняем копирование количество микросекунд
									::memcpy(buffer, &microseconds, sizeof(microseconds));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество миллисекунд
				case static_cast <uint8_t> (unit_t::MILLISECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование количество миллисекунд
								(* result) = std::to_string(this->_dt.milliseconds);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество миллисекунд
								uint32_t milliseconds = 0;
								// Выполняем извлечение количество миллисекунд
								this->get(&milliseconds, sizeof(milliseconds), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование количество миллисекунд
								(* result) = std::to_string(milliseconds);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 2, '0');
						// Если первого нуля нет
						else if(result->length() == 2)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.milliseconds))
									// Получаем текущее количество миллисекунд
									::memcpy(buffer, &this->_dt.milliseconds, sizeof(this->_dt.milliseconds));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint32_t)){
									// Количество миллисекунд
									uint32_t milliseconds = 0;
									// Выполняем извлечение количество миллисекунд
									this->get(&milliseconds, sizeof(milliseconds), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количество миллисекунд
									::memcpy(buffer, &milliseconds, sizeof(milliseconds));
								}
							} break;
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
}
/**
 * setTimeZone Метод установки временной зоны
 * @param zone временная зона для установки (в секундах)
 */
void awh::Chrono::setTimeZone(const int32_t zone) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx.date);
	// Устанавливаем временную зону в секундах
	this->_dt.offset = zone;
	// Устанавливаем идентификатор временной зоны
	this->_dt.zone = zone_t::UTC;
	// Выполняем перерасчёт локальной версии даты
	this->makeDate(this->makeDate(this->_dt), this->_dt);
}
/**
 * setTimeZone Метод установки временной зоны
 * @param zone временная зона для установки
 */
void awh::Chrono::setTimeZone(const zone_t zone) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx.date);
	// Устанавливаем идентификатор временной зоны
	this->_dt.zone = zone;
	// Устанавливаем временную зону в секундах
	this->_dt.offset = this->getTimeZone(zone);
	// Выполняем перерасчёт локальной версии даты
	this->makeDate(this->makeDate(this->_dt), this->_dt);
}
/**
 * setTimeZone Метод установки временной зоны
 * @param zone временная зона для установки
 */
void awh::Chrono::setTimeZone(const string & zone) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx.date);
	// Устанавливаем идентификатор временной зоны
	this->_dt.zone = this->matchTimeZone(zone);
	// Устанавливаем временную зону в секундах
	this->_dt.offset = this->getTimeZone(zone);
	// Выполняем перерасчёт локальной версии даты
	this->makeDate(this->makeDate(this->_dt), this->_dt);
}
/**
 * matchTimeZone Метод выполнения матчинга временной зоны
 * @param zone временная зона для конвертации
 * @return     определённая временная зона
 */
awh::Chrono::zone_t awh::Chrono::matchTimeZone(const string & zone) const noexcept {
	// Результат работы функции
	zone_t result = zone_t::NONE;
	// Если временная зона для матчинга передана
	if(!zone.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск нужного нам регулярного выражения
			auto i = this->_expressions.find(format_t::Z);
			// Если регулярное выражение получено
			if(i != this->_expressions.end()){
				// Выполняем блокировку потока
				const lock_guard <recursive_mutex> lock(this->_mtx.parse);
				// Создаём объект матчинга
				regmatch_t match[i->second.re_nsub + 1];
				// Выполняем разбор регулярного выражения
				const int32_t error = ::pcre2_regexec(&i->second, zone.c_str(), i->second.re_nsub + 1, match, REG_NOTEMPTY);
				// Если ошибок не получено
				if(error == 0){
					// Создаём массив собранных результатов
					vector <string> data(i->second.re_nsub + 1);
					// Выполняем перебор всех полученных вариантов
					for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
						// Если результат получен
						if(match[j].rm_eo > match[j].rm_so)
							// Выполняем установку результата
							data.at(j).assign(zone.c_str() + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
					}
					// Если временная зона извлечена
					if(!data.empty() && !data.front().empty()){
						// Если название временной зоны Z
						if(this->_fmk->compare("z", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::UTC;
						// Если название временной зоны получено как CT
						else if(this->_fmk->compare("ct", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CT;
						// Если название временной зоны получено как ET
						else if(this->_fmk->compare("et", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ET;
						// Если название временной зоны получено как MT
						else if(this->_fmk->compare("mt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MT;
						// Если название временной зоны получено как NT
						else if(this->_fmk->compare("nt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NT;
						// Если название временной зоны получено как PT
						else if(this->_fmk->compare("pt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PT;
						// Если название временной зоны получено как GMT
						else if(this->_fmk->compare("gmt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GMT;
						// Если название временной зоны получено как UTC
						else if(this->_fmk->compare("utc", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::UTC;
						// Если название временной зоны получено как AT
						else if(this->_fmk->compare("at", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AT;
						// Если название временной зоны получено как ACDT
						else if(this->_fmk->compare("acdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ACDT;
						// Если название временной зоны получено как ACST
						else if(this->_fmk->compare("acst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ACST;
						// Если название временной зоны получено как ACT
						else if(this->_fmk->compare("act", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ACT;
						// Если название временной зоны получено как ADT
						else if(this->_fmk->compare("adt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ADT;
						// Если название временной зоны получено как AFT
						else if(this->_fmk->compare("aft", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AFT;
						// Если название временной зоны получено как ART
						else if(this->_fmk->compare("art", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ART;
						// Если название временной зоны получено как AZT
						else if(this->_fmk->compare("azt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AZT;
						// Если название временной зоны получено как BDT
						else if(this->_fmk->compare("bdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::BDT;
						// Если название временной зоны получено как BOT
						else if(this->_fmk->compare("bot", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::BOT;
						// Если название временной зоны получено как BRT
						else if(this->_fmk->compare("brt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::BRT;
						// Если название временной зоны получено как BTT
						else if(this->_fmk->compare("btt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::BTT;
						// Если название временной зоны получено как CAT
						else if(this->_fmk->compare("cat", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CAT;
						// Если название временной зоны получено как CCT
						else if(this->_fmk->compare("cct", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CCT;
						// Если название временной зоны получено как CET
						else if(this->_fmk->compare("cet", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CET;
						// Если название временной зоны получено как CIT
						else if(this->_fmk->compare("cit", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CIT;
						// Если название временной зоны получено как CKT
						else if(this->_fmk->compare("ckt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CKT;
						// Если название временной зоны получено как CLT
						else if(this->_fmk->compare("clt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CLT;
						// Если название временной зоны получено как COT
						else if(this->_fmk->compare("cot", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::COT;
						// Если название временной зоны получено как CVT
						else if(this->_fmk->compare("cvt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CVT;
						// Если название временной зоны получено как CXT
						else if(this->_fmk->compare("cxt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CXT;
						// Если название временной зоны получено как EAT
						else if(this->_fmk->compare("eat", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EAT;
						// Если название временной зоны получено как ECT
						else if(this->_fmk->compare("ect", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ECT;
						// Если название временной зоны получено как EDT
						else if(this->_fmk->compare("edt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EDT;
						// Если название временной зоны получено как EET
						else if(this->_fmk->compare("eet", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EET;
						// Если название временной зоны получено как EGT
						else if(this->_fmk->compare("egt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EGT;
						// Если название временной зоны получено как EIT
						else if(this->_fmk->compare("eit", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EIT;
						// Если название временной зоны получено как EST
						else if(this->_fmk->compare("est", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EST;
						// Если название временной зоны получено как FET
						else if(this->_fmk->compare("fet", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::FET;
						// Если название временной зоны получено как FJT
						else if(this->_fmk->compare("fjt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::FJT;
						// Если название временной зоны получено как FKT
						else if(this->_fmk->compare("fkt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::FKT;
						// Если название временной зоны получено как FNT
						else if(this->_fmk->compare("fnt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::FNT;
						// Если название временной зоны получено как GET
						else if(this->_fmk->compare("get", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GET;
						// Если название временной зоны получено как GFT
						else if(this->_fmk->compare("gft", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GFT;
						// Если название временной зоны получено как GIT
						else if(this->_fmk->compare("git", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GIT;
						// Если название временной зоны получено как GMT
						else if(this->_fmk->compare("gmt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GMT;
						// Если название временной зоны получено как GYT
						else if(this->_fmk->compare("gyt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GYT;
						// Если название временной зоны получено как HKT
						else if(this->_fmk->compare("hkt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::HKT;
						// Если название временной зоны получено как ICT
						else if(this->_fmk->compare("ict", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ICT;
						// Если название временной зоны получено как IDT
						else if(this->_fmk->compare("idt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::IDT;
						// Если название временной зоны получено как JST
						else if(this->_fmk->compare("jst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::JST;
						// Если название временной зоны получено как KGT
						else if(this->_fmk->compare("kgt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::KGT;
						// Если название временной зоны получено как KST
						else if(this->_fmk->compare("kst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::KST;
						// Если название временной зоны получено как MDT
						else if(this->_fmk->compare("mdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MDT;
						// Если название временной зоны получено как MHT
						else if(this->_fmk->compare("mht", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MHT;
						// Если название временной зоны получено как MIT
						else if(this->_fmk->compare("mit", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MIT;
						// Если название временной зоны получено как MMT
						else if(this->_fmk->compare("mmt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MMT;
						// Если название временной зоны получено как MSK
						else if(this->_fmk->compare("msk", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MSK;
						// Если название временной зоны получено как MSD
						else if(this->_fmk->compare("msd", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MSD;
						// Если название временной зоны получено как MUT
						else if(this->_fmk->compare("mut", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MUT;
						// Если название временной зоны получено как MVT
						else if(this->_fmk->compare("mvt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MVT;
						// Если название временной зоны получено как MYT
						else if(this->_fmk->compare("myt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MYT;
						// Если название временной зоны получено как NCT
						else if(this->_fmk->compare("nct", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NCT;
						// Если название временной зоны получено как NDT
						else if(this->_fmk->compare("ndt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NDT;
						// Если название временной зоны получено как NFT
						else if(this->_fmk->compare("nft", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NFT;
						// Если название временной зоны получено как NPT
						else if(this->_fmk->compare("npt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NPT;
						// Если название временной зоны получено как NRT
						else if(this->_fmk->compare("nrt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NRT;
						// Если название временной зоны получено как NST
						else if(this->_fmk->compare("nst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NST;
						// Если название временной зоны получено как NUT
						else if(this->_fmk->compare("nut", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NUT;
						// Если название временной зоны получено как PDT
						else if(this->_fmk->compare("pdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PDT;
						// Если название временной зоны получено как PET
						else if(this->_fmk->compare("pet", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PET;
						// Если название временной зоны получено как PGT
						else if(this->_fmk->compare("pgt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PGT;
						// Если название временной зоны получено как PHT
						else if(this->_fmk->compare("pht", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PHT;
						// Если название временной зоны получено как PKT
						else if(this->_fmk->compare("pkt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PKT;
						// Если название временной зоны получено как PST
						else if(this->_fmk->compare("pst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PST;
						// Если название временной зоны получено как PWT
						else if(this->_fmk->compare("pwt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PWT;
						// Если название временной зоны получено как PYT
						else if(this->_fmk->compare("pyt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PYT;
						// Если название временной зоны получено как RET
						else if(this->_fmk->compare("ret", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::RET;
						// Если название временной зоны получено как SBT
						else if(this->_fmk->compare("sbt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SBT;
						// Если название временной зоны получено как SCT
						else if(this->_fmk->compare("sct", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SCT;
						// Если название временной зоны получено как SGT
						else if(this->_fmk->compare("sgt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SGT;
						// Если название временной зоны получено как SRT
						else if(this->_fmk->compare("srt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SRT;
						// Если название временной зоны получено как SST
						else if(this->_fmk->compare("sst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SST;
						// Если название временной зоны получено как TFT
						else if(this->_fmk->compare("tft", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::TFT;
						// Если название временной зоны получено как THA
						else if(this->_fmk->compare("tha", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::THA;
						// Если название временной зоны получено как TJT
						else if(this->_fmk->compare("tjt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::TJT;
						// Если название временной зоны получено как TKT
						else if(this->_fmk->compare("tkt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::TKT;
						// Если название временной зоны получено как TLT
						else if(this->_fmk->compare("tlt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::TLT;
						// Если название временной зоны получено как TMT
						else if(this->_fmk->compare("tmt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::TMT;
						// Если название временной зоны получено как TOT
						else if(this->_fmk->compare("tot", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::TOT;
						// Если название временной зоны получено как TRT
						else if(this->_fmk->compare("trt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::TRT;
						// Если название временной зоны получено как TVT
						else if(this->_fmk->compare("tvt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::TVT;
						// Если название временной зоны получено как UTC
						else if(this->_fmk->compare("utc", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::UTC;
						// Если название временной зоны получено как UYT
						else if(this->_fmk->compare("uyt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::UYT;
						// Если название временной зоны получено как UZT
						else if(this->_fmk->compare("uzt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::UZT;
						// Если название временной зоны получено как VET
						else if(this->_fmk->compare("vet", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::VET;
						// Если название временной зоны получено как VUT
						else if(this->_fmk->compare("vut", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::VUT;
						// Если название временной зоны получено как WAT
						else if(this->_fmk->compare("wat", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::WAT;
						// Если название временной зоны получено как WET
						else if(this->_fmk->compare("wet", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::WET;
						// Если название временной зоны получено как WFT
						else if(this->_fmk->compare("wft", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::WFT;
						// Если название временной зоны получено как WIB
						else if(this->_fmk->compare("wib", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::WIB;
						// Если название временной зоны получено как WIT
						else if(this->_fmk->compare("wit", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::WIT;
						// Если название временной зоны получено как AMT
						else if(this->_fmk->compare("amt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AMTAM;
						// Если название временной зоны получено как AST
						else if(this->_fmk->compare("ast", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ASTAL;
						// Если название временной зоны получено как BST
						else if(this->_fmk->compare("bst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::BSTBR;
						// Если название временной зоны получено как CDT
						else if(this->_fmk->compare("cdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CDTNA;
						// Если название временной зоны получено как CST
						else if(this->_fmk->compare("cst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CSTNA;
						// Если название временной зоны получено как GST
						else if(this->_fmk->compare("gst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GSTPG;
						// Если название временной зоны получено как IST
						else if(this->_fmk->compare("ist", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ISTID;
						// Если название временной зоны получено как MST
						else if(this->_fmk->compare("mst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MSTNA;
						// Если название временной зоны получено как AEDT
						else if(this->_fmk->compare("aedt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AEDT;
						// Если название временной зоны получено как AKDT
						else if(this->_fmk->compare("akdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AKDT;
						// Если название временной зоны получено как AKST
						else if(this->_fmk->compare("akst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AKST;
						// Если название временной зоны получено как AMST
						else if(this->_fmk->compare("amst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AMST;
						// Если название временной зоны получено как AWST
						else if(this->_fmk->compare("awst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AWST;
						// Если название временной зоны получено как AZOT
						else if(this->_fmk->compare("azot", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AZOT;
						// Если название временной зоны получено как BRST
						else if(this->_fmk->compare("brst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::BRST;
						// Если название временной зоны получено как CEST
						else if(this->_fmk->compare("cest", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CEST;
						// Если название временной зоны получено как AEST
						else if(this->_fmk->compare("aest", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AEST;
						// Если название временной зоны получено как CHOT
						else if(this->_fmk->compare("chot", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CHOT;
						// Если название временной зоны получено как CHST
						else if(this->_fmk->compare("chst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CHST;
						// Если название временной зоны получено как CHUT
						else if(this->_fmk->compare("chut", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CHUT;
						// Если название временной зоны получено как CLST
						else if(this->_fmk->compare("clst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CLST;
						// Если название временной зоны получено как COST
						else if(this->_fmk->compare("cost", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::COST;
						// Если название временной зоны получено как DAVT
						else if(this->_fmk->compare("davt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::DAVT;
						// Если название временной зоны получено как DDUT
						else if(this->_fmk->compare("ddut", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::DDUT;
						// Если название временной зоны получено как EAST
						else if(this->_fmk->compare("east", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EAST;
						// Если название временной зоны получено как EEST
						else if(this->_fmk->compare("eest", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EEST;
						// Если название временной зоны получено как EGST
						else if(this->_fmk->compare("egst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EGST;
						// Если название временной зоны получено как FKST
						else if(this->_fmk->compare("fkst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::FKST;
						// Если название временной зоны получено как GALT
						else if(this->_fmk->compare("galt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GALT;
						// Если название временной зоны получено как GAMT
						else if(this->_fmk->compare("gamt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GAMT;
						// Если название временной зоны получено как GILT
						else if(this->_fmk->compare("gilt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::GILT;
						// Если название временной зоны получено как HADT
						else if(this->_fmk->compare("hadt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::HADT;
						// Если название временной зоны получено как HAST
						else if(this->_fmk->compare("hast", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::HAST;
						// Если название временной зоны получено как HOVT
						else if(this->_fmk->compare("hovt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::HOVT;
						// Если название временной зоны получено как IRDT
						else if(this->_fmk->compare("irdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::IRDT;
						// Если название временной зоны получено как IRKT
						else if(this->_fmk->compare("irkt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::IRKT;
						// Если название временной зоны получено как IRST
						else if(this->_fmk->compare("irst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::IRST;
						// Если название временной зоны получено как KOST
						else if(this->_fmk->compare("kost", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::KOST;
						// Если название временной зоны получено как KRAT
						else if(this->_fmk->compare("krat", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::KRAT;
						// Если название временной зоны получено как LHDT
						else if(this->_fmk->compare("lhdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::LHDT;
						// Если название временной зоны получено как LHST
						else if(this->_fmk->compare("lhst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::LHST;
						// Если название временной зоны получено как LINT
						else if(this->_fmk->compare("lint", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::LINT;
						// Если название временной зоны получено как MAGT
						else if(this->_fmk->compare("magt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MAGT;
						// Если название временной зоны получено как MART
						else if(this->_fmk->compare("mart", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MART;
						// Если название временной зоны получено как MAWT
						else if(this->_fmk->compare("mawt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MAWT;
						// Если название временной зоны получено как MIST
						else if(this->_fmk->compare("mist", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::MIST;
						// Если название временной зоны получено как NZDT
						else if(this->_fmk->compare("nzdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NZDT;
						// Если название временной зоны получено как NZST
						else if(this->_fmk->compare("nzst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::NZST;
						// Если название временной зоны получено как OMST
						else if(this->_fmk->compare("omst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::OMST;
						// Если название временной зоны получено как ORAT
						else if(this->_fmk->compare("orat", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ORAT;
						// Если название временной зоны получено как PETT
						else if(this->_fmk->compare("pett", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PETT;
						// Если название временной зоны получено как PHOT
						else if(this->_fmk->compare("phot", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PHOT;
						// Если название временной зоны получено как PhST
						else if(this->_fmk->compare("phst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PhST;
						// Если название временной зоны получено как PMDT
						else if(this->_fmk->compare("pmdt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PMDT;
						// Если название временной зоны получено как PMST
						else if(this->_fmk->compare("pmst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PMST;
						// Если название временной зоны получено как PONT
						else if(this->_fmk->compare("pont", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PONT;
						// Если название временной зоны получено как PYST
						else if(this->_fmk->compare("pyst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::PYST;
						// Если название временной зоны получено как ROTT
						else if(this->_fmk->compare("rott", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ROTT;
						// Если название временной зоны получено как SAKT
						else if(this->_fmk->compare("sakt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SAKT;
						// Если название временной зоны получено как SAMT
						else if(this->_fmk->compare("samt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SAMT;
						// Если название временной зоны получено как SAST
						else if(this->_fmk->compare("sast", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SAST;
						// Если название временной зоны получено как SLST
						else if(this->_fmk->compare("slst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SLST;
						// Если название временной зоны получено как SYOT
						else if(this->_fmk->compare("syot", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::SYOT;
						// Если название временной зоны получено как TAHT
						else if(this->_fmk->compare("taht", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::TAHT;
						// Если название временной зоны получено как ULAT
						else if(this->_fmk->compare("ulat", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ULAT;
						// Если название временной зоны получено как USZ1
						else if(this->_fmk->compare("usz1", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::USZ1;
						// Если название временной зоны получено как UYST
						else if(this->_fmk->compare("uyst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::UYST;
						// Если название временной зоны получено как VLAT
						else if(this->_fmk->compare("vlat", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::VLAT;
						// Если название временной зоны получено как VOLT
						else if(this->_fmk->compare("volt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::VOLT;
						// Если название временной зоны получено как VOST
						else if(this->_fmk->compare("vost", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::VOST;
						// Если название временной зоны получено как WAKT
						else if(this->_fmk->compare("wakt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::WAKT;
						// Если название временной зоны получено как WAST
						else if(this->_fmk->compare("wast", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::WAST;
						// Если название временной зоны получено как WEST
						else if(this->_fmk->compare("west", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::WEST;
						// Если название временной зоны получено как YAKT
						else if(this->_fmk->compare("yakt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::YAKT;
						// Если название временной зоны получено как YEKT
						else if(this->_fmk->compare("yekt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::YEKT;
						// Если название временной зоны получено как WGST
						else if(this->_fmk->compare("wgst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::WGSTST;
						// Если название временной зоны получено как CHADT
						else if(this->_fmk->compare("chadt", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CHADT;
						// Если название временной зоны получено как CHAST
						else if(this->_fmk->compare("chast", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CHAST;
						// Если название временной зоны получено как CHOST
						else if(this->_fmk->compare("chost", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::CHOST;
						// Если название временной зоны получено как ACWST
						else if(this->_fmk->compare("acwst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ACWST;
						// Если название временной зоны получено как AZOST
						else if(this->_fmk->compare("azost", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::AZOST;
						// Если название временной зоны получено как EASST
						else if(this->_fmk->compare("easst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::EASST;
						// Если название временной зоны получено как HOVST
						else if(this->_fmk->compare("hovst", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::HOVST;
						// Если название временной зоны получено как ULAST
						else if(this->_fmk->compare("ulast", data.front()))
							// Выполняем получение временной зоны
							result = zone_t::ULAST;
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
			// Результат работы функции
			result = zone_t::NONE;
		}
	}
	// Выводим результат
	return result;
}
/**
 * matchTimeZone Метод выполнения матчинга временной зоны
 * @param storage хранение значение времени
 * @return        определённая временная зона
 */
awh::Chrono::zone_t awh::Chrono::matchTimeZone(const storage_t storage) const noexcept {
	// Результат работы функции
	zone_t result = zone_t::NONE;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL):
				// Получаем локальное значение временной зоны
				result = this->_dt.zone;
			break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL):
				// Получаем глобальное значение временной зоны
				result = zone_t::UTC;
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * getTimeZone Метод перевода временной зоны в числовой эквивалент
 * @param zone временная зона для конвертации
 * @return     временная зона в секундах
 */
int32_t awh::Chrono::getTimeZone(const zone_t zone) const noexcept {
	// Определяем временную зону
	switch(static_cast <uint8_t> (zone)){
		// Если временная зона не установлена
		case static_cast <uint8_t> (zone_t::NONE):
			// Выводим значение локальной временной зоны
			return this->_dt.offset;
		// Если временная зона установлена как (Атлантическое Время)
		case static_cast <uint8_t> (zone_t::AT):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::ASTAL, zone_t::ADT);
		// Если временная зона установлена как (Северноамериканское Центральное Время)
		case static_cast <uint8_t> (zone_t::CT):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::CSTNA, zone_t::CDTNA);
		// Если временная зона установлена как (Северноамериканское Восточное Время)
		case static_cast <uint8_t> (zone_t::ET):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::EST, zone_t::EDT);
		// Если временная зона установлена как (Северноамериканское Горное Время)
		case static_cast <uint8_t> (zone_t::MT):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::MSTNA, zone_t::MDT);
		// Если временная зона установлена как (Северноамериканское Тихоокеанское Время)
		case static_cast <uint8_t> (zone_t::PT):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::PST, zone_t::PDT);
		// Если временная зона установлена как (Время На Острове Ниуэ)
		case static_cast <uint8_t> (zone_t::NUT):
		// Если временная зона установлена как (Стандартное Время На Острове Самоа)
		case static_cast <uint8_t> (zone_t::SST):
			// Формируем смещение временной зоны (UTC-11)
			return -39600;
		// Если временная зона установлена как (Стандартное Время На Островах Кука)
		case static_cast <uint8_t> (zone_t::CKT):
		// Если временная зона установлена как (Гавайско-Алеутское Стандартное Время)
		case static_cast <uint8_t> (zone_t::HAST):
		// Если временная зона установлена как (Время На Острове Таити)
		case static_cast <uint8_t> (zone_t::TAHT):
			// Формируем смещение временной зоны (UTC-10)
			return -36000;
		// Если временная зона установлена как (Время На Маркизских Островах)
		case static_cast <uint8_t> (zone_t::MIT):
		// Если временная зона установлена как (Время На Маркизских Островах)
		case static_cast <uint8_t> (zone_t::MART):
			// Формируем смещение временной зоны (UTC-9:30)
			return -34200;
		// Если временная зона установлена как (Время На О. Гамбье)
		case static_cast <uint8_t> (zone_t::GIT):
		// Если временная зона установлена как (Стандартное Время На Аляске)
		case static_cast <uint8_t> (zone_t::AKST):
		// Если временная зона установлена как (Время На Острове Гамбье)
		case static_cast <uint8_t> (zone_t::GAMT):
		// Если временная зона установлена как (Гавайско-Алеутское Летнее Время)
		case static_cast <uint8_t> (zone_t::HADT):
			// Формируем смещение временной зоны (UTC-9)
			return -32400;
		// Если временная зона установлена как (Северноамериканское Тихоокеанское Стандартное Время)
		case static_cast <uint8_t> (zone_t::PST):
		// Если временная зона установлена как (Летнее Время На Аляске)
		case static_cast <uint8_t> (zone_t::AKDT):
			// Формируем смещение временной зоны (UTC-8)
			return -28800;
		// Если временная зона установлена как (Северноамериканское Тихоокеанское Летнее Время)
		case static_cast <uint8_t> (zone_t::PDT):
		// Если временная зона установлена как (Северноамериканское Горное Стандартное Время)
		case static_cast <uint8_t> (zone_t::MSTNA):
			// Формируем смещение временной зоны (UTC-7)
			return -25200;
		// Если временная зона установлена как (Северноамериканское Горное Летнее Время)
		case static_cast <uint8_t> (zone_t::MDT):
		// Если временная зона установлена как (Стандартное Время На Острове Пасхи)
		case static_cast <uint8_t> (zone_t::EAST):
		// Если временная зона установлена как (Время На Галапагосских Островах)
		case static_cast <uint8_t> (zone_t::GALT):
		// Если временная зона установлена как (Северноамериканское Центральное Стандартное Время)
		case static_cast <uint8_t> (zone_t::CSTNA):
			// Формируем смещение временной зоны (UTC-6)
			return -21600;
		// Если временная зона установлена как (Амазонское Стандартное Время)
		case static_cast <uint8_t> (zone_t::ACT):
		// Если временная зона установлена как (Колумбийское Стандартное Время)
		case static_cast <uint8_t> (zone_t::COT):
		// Если временная зона установлена как (Эквадорское Время)
		case static_cast <uint8_t> (zone_t::ECT):
		// Если временная зона установлена как (Северноамериканское Восточное Стандартное Время)
		case static_cast <uint8_t> (zone_t::EST):
		// Если временная зона установлена как (Стандартное Время В Перу)
		case static_cast <uint8_t> (zone_t::PET):
		// Если временная зона установлена как (Летнее Время На Острове Пасхи)
		case static_cast <uint8_t> (zone_t::EASST):
		// Если временная зона установлена как (Северноамериканское Центральное Летнее Время)
		case static_cast <uint8_t> (zone_t::CDTNA):
		// Если временная зона установлена как (Кубинское Стандартное Время)
		case static_cast <uint8_t> (zone_t::CSTCB):
			// Формируем смещение временной зоны (UTC-5)
			return -18000;
		// Если временная зона установлена как (Парагвайское Стандартное Время)
		case static_cast <uint8_t> (zone_t::PYT):
		// Если временная зона установлена как (Стандартное Время На Фолклендах)
		case static_cast <uint8_t> (zone_t::FKT):
		// Если временная зона установлена как (Боливийское Время)
		case static_cast <uint8_t> (zone_t::BOT):
		// Если временная зона установлена как (Чилийское Стандартное Время)
		case static_cast <uint8_t> (zone_t::CLT):
		// Если временная зона установлена как (Северноамериканское Восточное Летнее Время)
		case static_cast <uint8_t> (zone_t::EDT):
		// Если временная зона установлена как (Время В Гайане)
		case static_cast <uint8_t> (zone_t::GYT):
		// Если временная зона установлена как (Время В Венесуеле)
		case static_cast <uint8_t> (zone_t::VET):
		// Если временная зона установлена как (Колумбийское Летнее Время)
		case static_cast <uint8_t> (zone_t::COST):
		// Если временная зона установлена как (Амазонское Стандартное Время)
		case static_cast <uint8_t> (zone_t::AMTAM):
		// Если временная зона установлена как (Атлантическое Стандартное Время)
		case static_cast <uint8_t> (zone_t::ASTAL):
		// Если временная зона установлена как (Кубинское Летнее Время)
		case static_cast <uint8_t> (zone_t::CDTCB):
			// Формируем смещение временной зоны (UTC-4)
			return -14400;
		// Если временная зона установлена как (Время В Ньюфаундленде)
		case static_cast <uint8_t> (zone_t::NT):
		// Если временная зона установлена как (Стандартное Время В Ньюфаундленде)
		case static_cast <uint8_t> (zone_t::NST):
			// Формируем смещение временной зоны (UTC-3:30)
			return -12600;
		// Если временная зона установлена как (Время В Суринаме)
		case static_cast <uint8_t> (zone_t::SRT):
		// Если временная зона установлена как (Атлантическое Летнее Время)
		case static_cast <uint8_t> (zone_t::ADT):
		// Если временная зона установлена как (Аргентинское Стандартное Время)
		case static_cast <uint8_t> (zone_t::ART):
		// Если временная зона установлена как (Бразильское Стандартное Время)
		case static_cast <uint8_t> (zone_t::BRT):
		// Если временная зона установлена как (Время В Французской Гвиане)
		case static_cast <uint8_t> (zone_t::GFT):
		// Если временная зона установлена как (Стандартное Время В Уругвае)
		case static_cast <uint8_t> (zone_t::UYT):
		// Если временная зона установлена как (Амазонка, Стандартное Время)
		case static_cast <uint8_t> (zone_t::AMST):
		// Если временная зона установлена как (Чилийское Летнее Время)
		case static_cast <uint8_t> (zone_t::CLST):
		// Если временная зона установлена как (Летнее Время На Фолклендах)
		case static_cast <uint8_t> (zone_t::FKST):
		// Если временная зона установлена как (Парагвайское Летнее Время)
		case static_cast <uint8_t> (zone_t::PYST):
		// Если временная зона установлена как (Стандартное Время На Островах Сен-Пьер И Микелон)
		case static_cast <uint8_t> (zone_t::PMST):
		// Если временная зона установлена как (Время На Станции Ротера)
		case static_cast <uint8_t> (zone_t::ROTT):
		// Если временная зона установлена как (Стандартное Время В Западной Гренландии)
		case static_cast <uint8_t> (zone_t::WGST):
			// Формируем смещение временной зоны (UTC-3)
			return -10800;
		// Если временная зона установлена как (Летнее Время В Ньюфаундленде)
		case static_cast <uint8_t> (zone_t::NDT):
			// Формируем смещение временной зоны (UTC-2:30)
			return -9000;
		// Если временная зона установлена как (Стандартное Время На Фернанду-Ди-Норонья)
		case static_cast <uint8_t> (zone_t::FNT):
		// Если временная зона установлена как (Бразильское Летнее Время)
		case static_cast <uint8_t> (zone_t::BRST):
		// Если временная зона установлена как (Летнее Время На Островах Сен-Пьер И Микелон)
		case static_cast <uint8_t> (zone_t::PMDT):
		// Если временная зона установлена как (Летнее Время В Уругвае)
		case static_cast <uint8_t> (zone_t::UYST):
		// Если временная зона установлена как (Время В Южной Георгии)
		case static_cast <uint8_t> (zone_t::GSTSG):
		// Если временная зона установлена как (Летнее Время В Западной Гренландии)
		case static_cast <uint8_t> (zone_t::WGSTST):
			// Формируем смещение временной зоны (UTC-2)
			return -7200;
		// Если временная зона установлена как (Стандартное Время На Островах Кабо-Верде)
		case static_cast <uint8_t> (zone_t::CVT):
		// Если временная зона установлена как (Стандартное Время В Восточной Гренландии)
		case static_cast <uint8_t> (zone_t::EGT):
		// Если временная зона установлена как (Стандартное Время На Азорских Островах)
		case static_cast <uint8_t> (zone_t::AZOT):
			// Формируем смещение временной зоны (UTC-1)
			return -3600;
		// Если временная зона установлена как (Среднее Время По Гринвичу)
		case static_cast <uint8_t> (zone_t::GMT):
		// Если временная зона установлена как (Всемирное Координированное Время)
		case static_cast <uint8_t> (zone_t::UTC):
		// Если временная зона установлена как (Западноевропейский Стандартний Час)
		case static_cast <uint8_t> (zone_t::WET):
		// Если временная зона установлена как (Летнее Время В Восточной Гренландии)
		case static_cast <uint8_t> (zone_t::EGST):
		// Если временная зона установлена как (Летнее Время На Азорских Островах)
		case static_cast <uint8_t> (zone_t::AZOST):
			// Формируем смещение временной зоны (UTC+0)
			return 0;
		// Если временная зона установлена как (Центральноевропейское Стандартное Время)
		case static_cast <uint8_t> (zone_t::CET):
		// Если временная зона установлена как (Западноафриканское Стандартное Время)
		case static_cast <uint8_t> (zone_t::WAT):
		// Если временная зона установлена как (Западноевропейское Летнее Время)
		case static_cast <uint8_t> (zone_t::WEST):
		// Если временная зона установлена как (Британское Летнее Время)
		case static_cast <uint8_t> (zone_t::BSTBR):
		// Если временная зона установлена как (Ирландия, Летнее Время)
		case static_cast <uint8_t> (zone_t::ISTIR):
			// Формируем смещение временной зоны (UTC+1)
			return 3600;
		// Если временная зона установлена как (Восточноафриканское Время)
		case static_cast <uint8_t> (zone_t::CAT):
		// Если временная зона установлена как (Восточноевропейское Стандартное Время)
		case static_cast <uint8_t> (zone_t::EET):
		// Если временная зона установлена как (Центральноевропейское Летнее Время)
		case static_cast <uint8_t> (zone_t::CEST):
		// Если временная зона установлена как (Южноафриканское Время)
		case static_cast <uint8_t> (zone_t::SAST):
		// Если временная зона установлена как (Калиниградское Время)
		case static_cast <uint8_t> (zone_t::USZ1):
		// Если временная зона установлена как (Западноафриканское Летнее Время)
		case static_cast <uint8_t> (zone_t::WAST):
		// Если временная зона установлена как (Израильское Стандартное Время)
		case static_cast <uint8_t> (zone_t::ISTIS):
			// Формируем смещение временной зоны (UTC+2)
			return 7200;
		// Если временная зона установлена как (Минское Время)
		case static_cast <uint8_t> (zone_t::FET):
		// Если временная зона установлена как (Турецкое Время)
		case static_cast <uint8_t> (zone_t::TRT):
		// Если временная зона установлена как (Восточноафриканский Час)
		case static_cast <uint8_t> (zone_t::EAT):
		// Если временная зона установлена как (Израильское Летнее Время)
		case static_cast <uint8_t> (zone_t::IDT):
		// Если временная зона установлена как (Московское Время)
		case static_cast <uint8_t> (zone_t::MSK):
		// Если временная зона установлена как (Восточноевропейское Летнее Время)
		case static_cast <uint8_t> (zone_t::EEST):
		// Если временная зона установлена как (Время На Станции Сёва)
		case static_cast <uint8_t> (zone_t::SYOT):
		// Если временная зона установлена как (Стандартное Время В Саудовской Аравии)
		case static_cast <uint8_t> (zone_t::ASTSA):
			// Формируем смещение временной зоны (UTC+3)
			return 10800;
		// Если временная зона установлена как (Иранское Стандартное Время)
		case static_cast <uint8_t> (zone_t::IRST):
			// Формируем смещение временной зоны (UTC+3:30)
			return 12600;
		// Если временная зона установлена как (Стандартное Время На Острове Маврикий)
		case static_cast <uint8_t> (zone_t::MUT):
		// Если временная зона установлена как (Время На Острове Реюньон)
		case static_cast <uint8_t> (zone_t::RET):
		// Если временная зона установлена как (Время На Сейшелах)
		case static_cast <uint8_t> (zone_t::SCT):
		// Если временная зона установлена как (Азербайджанское Стандартное Время)
		case static_cast <uint8_t> (zone_t::AZT):
		// Если временная зона установлена как (Грузинское Стандартное Время)
		case static_cast <uint8_t> (zone_t::GET):
		// Если временная зона установлена как (Московское Летнее Время)
		case static_cast <uint8_t> (zone_t::MSD):
		// Если временная зона установлена как (Самарское Время)
		case static_cast <uint8_t> (zone_t::SAMT):
		// Если временная зона установлена как (Волгоградское Время)
		case static_cast <uint8_t> (zone_t::VOLT):
		// Если временная зона установлена как (Армянское Стандартное Время)
		case static_cast <uint8_t> (zone_t::AMTAR):
		// Если временная зона установлена как (Время В Персидском Заливе)
		case static_cast <uint8_t> (zone_t::GSTPG):
			// Формируем смещение временной зоны (UTC+4)
			return 14400;
		// Если временная зона установлена как (Время В Афганистане)
		case static_cast <uint8_t> (zone_t::AFT):
		// Если временная зона установлена как (Иранское Летнее Время)
		case static_cast <uint8_t> (zone_t::IRDT):
			// Формируем смещение временной зоны (UTC+4:30)
			return 16200;
		// Если временная зона установлена как (Время На Мальдивах)
		case static_cast <uint8_t> (zone_t::MVT):
		// Если временная зона установлена как (Французское Южное И Антарктическое Время)
		case static_cast <uint8_t> (zone_t::TFT):
		// Если временная зона установлена как (Время В Таджикистане)
		case static_cast <uint8_t> (zone_t::TJT):
		// Если временная зона установлена как (Стандартное Время В Туркмении)
		case static_cast <uint8_t> (zone_t::TMT):
		// Если временная зона установлена как (Пакистанское Стандартное Время)
		case static_cast <uint8_t> (zone_t::PKT):
		// Если временная зона установлена как (Время В Узбекистане)
		case static_cast <uint8_t> (zone_t::UZT):
		// Если временная зона установлена как (Время На Станции Моусон)
		case static_cast <uint8_t> (zone_t::MAWT):
		// Если временная зона установлена как (Время В Западном Казахстане)
		case static_cast <uint8_t> (zone_t::ORAT):
		// Если временная зона установлена как (Екатеринбургское Время)
		case static_cast <uint8_t> (zone_t::YEKT):
			// Формируем смещение временной зоны (UTC+5)
			return 18000;
		// Если временная зона установлена как (Стандартное Время В Шри-Ланке)
		case static_cast <uint8_t> (zone_t::SLST):
		// Если временная зона установлена как (Индийское Стандартное Время)
		case static_cast <uint8_t> (zone_t::ISTID):
			// Формируем смещение временной зоны (UTC+5:30)
			return 19800;
		// Если временная зона установлена как (Непальськое Время)
		case static_cast <uint8_t> (zone_t::NPT):
			// Формируем смещение временной зоны (UTC+5:45)
			return 20700;
		// Если временная зона установлена как (Бутанское Время)
		case static_cast <uint8_t> (zone_t::BTT):
		// Если временная зона установлена как (Время В Киргизии)
		case static_cast <uint8_t> (zone_t::KGT):
		// Если временная зона установлена как (Омское Время)
		case static_cast <uint8_t> (zone_t::OMST):
		// Если временная зона установлена как (Время На Станции Восток)
		case static_cast <uint8_t> (zone_t::VOST):
		// Если временная зона установлена как (Стандартное Время В Бангладеш)
		case static_cast <uint8_t> (zone_t::BSTBL):
			// Формируем смещение временной зоны (UTC+6)
			return 21600;
		// Если временная зона установлена как (Время На Кокосовые Островах)
		case static_cast <uint8_t> (zone_t::CCT):
		// Если временная зона установлена как (Время В Мьянме)
		case static_cast <uint8_t> (zone_t::MMT):
			// Формируем смещение временной зоны (UTC+6:30)
			return 23400;
		// Если временная зона установлена как (Тайландское Время)
		case static_cast <uint8_t> (zone_t::THA):
		// Если временная зона установлена как (Время На Острове Рождества)
		case static_cast <uint8_t> (zone_t::CXT):
		// Если временная зона установлена как (Время В Индокитае)
		case static_cast <uint8_t> (zone_t::ICT):
		// Если временная зона установлена как (Время В Западной Индонезии)
		case static_cast <uint8_t> (zone_t::WIB):
		// Если временная зона установлена как (Дейвис)
		case static_cast <uint8_t> (zone_t::DAVT):
		// Если временная зона установлена как (Стандартное Время В Ховде)
		case static_cast <uint8_t> (zone_t::HOVT):
		// Если временная зона установлена как (Красноярское Стандартное Время)
		case static_cast <uint8_t> (zone_t::KRAT):
			// Формируем смещение временной зоны (UTC+7)
			return 25200;
		// Если временная зона установлена как (Малайское Время)
		case static_cast <uint8_t> (zone_t::MYT):
		// Если временная зона установлена как (Сингапурское Время)
		case static_cast <uint8_t> (zone_t::SGT):
		// Если временная зона установлена как (Время В Бруней-Даруссаламе)
		case static_cast <uint8_t> (zone_t::BDT):
		// Если временная зона установлена как (Время В Бруней-Даруссаламе)
		case static_cast <uint8_t> (zone_t::BNT):
		// Если временная зона установлена как (Время В Центральной Индонезии)
		case static_cast <uint8_t> (zone_t::CIT):
		// Если временная зона установлена как (Гонконгское Стандартное Время)
		case static_cast <uint8_t> (zone_t::HKT):
		// Если временная зона установлена как (Стандартное Время На Филлипинах)
		case static_cast <uint8_t> (zone_t::PHT):
		// Если временная зона установлена как (Стандартное Время В Западной Австралии)
		case static_cast <uint8_t> (zone_t::AWST):
		// Если временная зона установлена как (Стандартное Время В Чойлобалсане)
		case static_cast <uint8_t> (zone_t::CHOT):
		// Если временная зона установлена как (Иркутское Стандартное Время)
		case static_cast <uint8_t> (zone_t::IRKT):
		// Если временная зона установлена как (Стандартное Время На Филлипинах)
		case static_cast <uint8_t> (zone_t::PhST):
		// Если временная зона установлена как (Стандартное Время В Монголии)
		case static_cast <uint8_t> (zone_t::ULAT):
		// Если временная зона установлена как (Летнее Время В Ховде)
		case static_cast <uint8_t> (zone_t::HOVST):
		// Если временная зона установлена как (Китайское Стандартное Время)
		case static_cast <uint8_t> (zone_t::CSTKT):
		// Если временная зона установлена как (Время В Малайзии)
		case static_cast <uint8_t> (zone_t::MSTMS):
			// Формируем смещение временной зоны (UTC+8)
			return 28800;
		// Если временная зона установлена как (Центрально-Западная Австралия, Стандартное Время)
		case static_cast <uint8_t> (zone_t::ACWST):
			// Формируем смещение временной зоны (UTC+8:45)
			return 31500;
		// Если временная зона установлена как (Время На Острове Палау)
		case static_cast <uint8_t> (zone_t::PWT):
		// Если временная зона установлена как (Время В Восточном Тиморе)
		case static_cast <uint8_t> (zone_t::TLT):
		// Если временная зона установлена как (Время В Восточной Индонезии)
		case static_cast <uint8_t> (zone_t::EIT):
		// Если временная зона установлена как (Японское Стандартное Время)
		case static_cast <uint8_t> (zone_t::JST):
		// Если временная зона установлена как (Корейское Стандартное Время)
		case static_cast <uint8_t> (zone_t::KST):
		// Если временная зона установлена как (Время В Восточной Индонезии)
		case static_cast <uint8_t> (zone_t::WIT):
		// Если временная зона установлена как (Якутское Время)
		case static_cast <uint8_t> (zone_t::YAKT):
		// Если временная зона установлена как (Летнее Время В Чойлобалсане)
		case static_cast <uint8_t> (zone_t::CHOST):
		// Если временная зона установлена как (Летнее Время В Монголии)
		case static_cast <uint8_t> (zone_t::ULAST):
			// Формируем смещение временной зоны (UTC+9)
			return 32400;
		// Если временная зона установлена как (Стандартное Время В Центральной Австралии)
		case static_cast <uint8_t> (zone_t::ACST):
			// Формируем смещение временной зоны (UTC+9:30)
			return 34200;
		// Если временная зона установлена как (Время В Папуа-Новой Гвинее)
		case static_cast <uint8_t> (zone_t::PGT):
		// Если временная зона установлена как (Стандартное Время В Восточной Австралии)
		case static_cast <uint8_t> (zone_t::AEST):
		// Если временная зона установлена как (Час Чаморро)
		case static_cast <uint8_t> (zone_t::CHST):
		// Если временная зона установлена как (Время На Островах Чуук)
		case static_cast <uint8_t> (zone_t::CHUT):
		// Если временная зона установлена как (Дюмон-Д'юрвиль)
		case static_cast <uint8_t> (zone_t::DDUT):
		// Если временная зона установлена как (Владивостокское Время)
		case static_cast <uint8_t> (zone_t::VLAT):
			// Формируем смещение временной зоны (UTC+10)
			return 36000;
		// Если временная зона установлена как (Летнее Время В Центральной Австралии)
		case static_cast <uint8_t> (zone_t::ACDT):
		// Если временная зона установлена как (Стандартное Время На Лорд-Хау)
		case static_cast <uint8_t> (zone_t::LHST):
			// Формируем смещение временной зоны (UTC+10:30)
			return 37800;
		// Если временная зона установлена как (Стандартное Время В Новой Каледонии)
		case static_cast <uint8_t> (zone_t::NCT):
		// Если временная зона установлена как (Время На Острове Норфолк)
		case static_cast <uint8_t> (zone_t::NFT):
		// Если временная зона установлена как (Время На Соломоновых Островах)
		case static_cast <uint8_t> (zone_t::SBT):
		// Если временная зона установлена как (Стандартное Время На Островах Вануату)
		case static_cast <uint8_t> (zone_t::VUT):
		// Если временная зона установлена как (Летнее Время В Восточной Австралии)
		case static_cast <uint8_t> (zone_t::AEDT):
		// Если временная зона установлена как (Время На Острове Косраэ)
		case static_cast <uint8_t> (zone_t::KOST):
		// Если временная зона установлена как (Летнее Время На Лорд-Хау)
		case static_cast <uint8_t> (zone_t::LHDT):
		// Если временная зона установлена как (Магаданское Стандартное Время)
		case static_cast <uint8_t> (zone_t::MAGT):
		// Если временная зона установлена как (Время На Станции Маккуори)
		case static_cast <uint8_t> (zone_t::MIST):
		// Если временная зона установлена как (Время На Острове Понапе)
		case static_cast <uint8_t> (zone_t::PONT):
		// Если временная зона установлена как (Сахалинское Стандартное Время)
		case static_cast <uint8_t> (zone_t::SAKT):
			// Формируем смещение временной зоны (UTC+11)
			return 39600;
		// Если временная зона установлена как (Время На Острове Науру)
		case static_cast <uint8_t> (zone_t::NRT):
		// Если временная зона установлена как (Летнее Время На О. Фиджи)
		case static_cast <uint8_t> (zone_t::FJT):
		// Если временная зона установлена как (Время На Островах Тувалу)
		case static_cast <uint8_t> (zone_t::TVT):
		// Если временная зона установлена как (Время На Маршалловых Островах)
		case static_cast <uint8_t> (zone_t::MHT):
		// Если временная зона установлена как (Время На Островах Уоллис И Футуна)
		case static_cast <uint8_t> (zone_t::WFT):
		// Если временная зона установлена как (Время На Островах Гилберта)
		case static_cast <uint8_t> (zone_t::GILT):
		// Если временная зона установлена как (Стандартное Время В Новой Зеландии)
		case static_cast <uint8_t> (zone_t::NZST):
		// Если временная зона установлена как (Камчатское Время)
		case static_cast <uint8_t> (zone_t::PETT):
		// Если временная зона установлена как (Время На Острове Уэйк)
		case static_cast <uint8_t> (zone_t::WAKT):
			// Формируем смещение временной зоны (UTC+12)
			return 43200;
		// Если временная зона установлена как (Стандартное Время На Архипелаге Чатем)
		case static_cast <uint8_t> (zone_t::CHAST):
			// Формируем смещение временной зоны (UTC+12:45)
			return 45900;
		// Если временная зона установлена как (Время На Островах Токелау)
		case static_cast <uint8_t> (zone_t::TKT):
		// Если временная зона установлена как (Время На Островах Тонга)
		case static_cast <uint8_t> (zone_t::TOT):
		// Если временная зона установлена как (Летнее Время В Новой Зеландии)
		case static_cast <uint8_t> (zone_t::NZDT):
		// Если временная зона установлена как (Время На Островах Феникс)
		case static_cast <uint8_t> (zone_t::PHOT):
			// Формируем смещение временной зоны (UTC+13)
			return 46800;
		// Если временная зона установлена как (Летнее Время На Архипелаге Чатем)
		case static_cast <uint8_t> (zone_t::CHADT):
			// Формируем смещение временной зоны (UTC+13:45)
			return 49500;
		// Если временная зона установлена как (Время На Острове Лайн)
		case static_cast <uint8_t> (zone_t::LINT):
			// Формируем смещение временной зоны (UTC+14)
			return 50400;
	}
	// Выводим результат
	return this->_dt.offset;
}
/**
 * getTimeZone Метод перевода временной зоны в числовой эквивалент
 * @param zone временная зона для конвертации
 * @return     временная зона в секундах
 */
int32_t awh::Chrono::getTimeZone(const string & zone) const noexcept {
	// Результат работы функции
	int32_t result = this->_dt.offset;
	// Если временная зона указана
	if(!zone.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск нужного нам регулярного выражения
			auto i = this->_expressions.find(format_t::e);
			// Если регулярное выражение получено
			if(i != this->_expressions.end()){
				// Выполняем блокировку потока
				const lock_guard <recursive_mutex> lock(this->_mtx.parse);
				// Создаём объект матчинга
				regmatch_t match[i->second.re_nsub + 1];
				// Выполняем разбор регулярного выражения
				const int32_t error = ::pcre2_regexec(&i->second, zone.c_str(), i->second.re_nsub + 1, match, REG_NOTEMPTY);
				// Если ошибок не получено
				if(error == 0){
					// Создаём массив собранных результатов
					vector <string> data(i->second.re_nsub + 1);
					// Выполняем перебор всех полученных вариантов
					for(uint8_t j = 0; j < static_cast <uint8_t> (i->second.re_nsub + 1); j++){
						// Если результат получен
						if(match[j].rm_eo > match[j].rm_so)
							// Выполняем установку результата
							data.at(j).assign(zone.c_str() + match[j].rm_so, match[j].rm_eo - match[j].rm_so);
					}
					// Если временная зона извлечена
					if(!data.empty() && (data.size() > 1)){
						// Получаем название временной зоны
						string name = data.at(1);
						// Выполняем поиск временной зоны в списке временных зон
						auto i = this->_timeZones.find(this->_fmk->transform(name, fmk_t::transform_t::LOWER));
						// Если временная зона найдена
						if(i != this->_timeZones.end())
							// Устанавливаем значение временной зоны
							result = i->second;
						// Если временная зона не найдена
						else {
							// Если название временной зоны не получено
							if(name.empty()){
								// Если смещение времени указано
								if((data.size() > 5) && !data.at(4).empty()){
									// Получаем смещение времени
									const string & offset = data.at(4);
									// Если полученное смещение является числом
									if(this->_fmk->is(offset, fmk_t::check_t::NUMBER)){
										// Если указано 4 символа
										if(offset.size() == 4){
											// Получаем количество часов
											const uint8_t hour = static_cast <uint8_t> (::stoul(offset.substr(0, 2)));
											// Получаем количество минут
											const uint8_t minutes = static_cast <uint8_t> (::stoul(offset.substr(2)));
											// Если смещение времени положительное
											if(data.at(3).compare("+") == 0)
												// Получаем время смещения
												result += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
											// Устанавливаем отрицательное смещение времени
											else result -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
										// Если установлен всего один символ
										} else {
											// Если смещение времени положительное
											if(data.at(3).compare("+") == 0)
												// Получаем время смещения
												result += static_cast <int32_t> (::stoul(offset) * 60 * 60);
											// Устанавливаем отрицательное смещение времени
											else result -= static_cast <int32_t> (::stoul(offset) * 60 * 60);
										}
									// Если получено время в формате часов
									} else if((data.size() > 6) && !data.at(5).empty() && !data.at(6).empty()) {
										// Получаем количество часов
										const uint8_t hour = static_cast <uint8_t> (::stoul(data.at(5)));
										// Получаем количество минут
										const uint8_t minutes = static_cast <uint8_t> (::stoul(data.at(6)));
										// Если смещение времени положительное
										if(data.at(3).compare("+") == 0)
											// Получаем время смещения
											result += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
										// Устанавливаем отрицательное смещение времени
										else result -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
									}
								}
							// Если название временной зоны является числом
							} else if(this->_fmk->is(name, fmk_t::check_t::NUMBER))
								// Получаем время смещения
								result += static_cast <int32_t> (::stoul(name) * 60 * 60);
							// Если временная зона получена в виде названия
							else {
								// Если название временной зоны Z
								if(this->_fmk->compare("z", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::UTC);
								// Если название временной зоны получено как CT
								else if(this->_fmk->compare("ct", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CT);
								// Если название временной зоны получено как ET
								else if(this->_fmk->compare("et", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ET);
								// Если название временной зоны получено как MT
								else if(this->_fmk->compare("mt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MT);
								// Если название временной зоны получено как NT
								else if(this->_fmk->compare("nt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NT);
								// Если название временной зоны получено как PT
								else if(this->_fmk->compare("pt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PT);
								// Если название временной зоны получено как GMT
								else if(this->_fmk->compare("gmt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GMT);
								// Если название временной зоны получено как UTC
								else if(this->_fmk->compare("utc", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::UTC);
								// Если название временной зоны получено как AT
								else if(this->_fmk->compare("at", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AT);
								// Если название временной зоны получено как ACDT
								else if(this->_fmk->compare("acdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ACDT);
								// Если название временной зоны получено как ACST
								else if(this->_fmk->compare("acst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ACST);
								// Если название временной зоны получено как ACT
								else if(this->_fmk->compare("act", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ACT);
								// Если название временной зоны получено как ADT
								else if(this->_fmk->compare("adt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ADT);
								// Если название временной зоны получено как AFT
								else if(this->_fmk->compare("aft", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AFT);
								// Если название временной зоны получено как ART
								else if(this->_fmk->compare("art", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ART);
								// Если название временной зоны получено как AZT
								else if(this->_fmk->compare("azt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AZT);
								// Если название временной зоны получено как BDT
								else if(this->_fmk->compare("bdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::BDT);
								// Если название временной зоны получено как BOT
								else if(this->_fmk->compare("bot", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::BOT);
								// Если название временной зоны получено как BRT
								else if(this->_fmk->compare("brt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::BRT);
								// Если название временной зоны получено как BTT
								else if(this->_fmk->compare("btt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::BTT);
								// Если название временной зоны получено как CAT
								else if(this->_fmk->compare("cat", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CAT);
								// Если название временной зоны получено как CCT
								else if(this->_fmk->compare("cct", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CCT);
								// Если название временной зоны получено как CET
								else if(this->_fmk->compare("cet", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CET);
								// Если название временной зоны получено как CIT
								else if(this->_fmk->compare("cit", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CIT);
								// Если название временной зоны получено как CKT
								else if(this->_fmk->compare("ckt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CKT);
								// Если название временной зоны получено как CLT
								else if(this->_fmk->compare("clt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CLT);
								// Если название временной зоны получено как COT
								else if(this->_fmk->compare("cot", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::COT);
								// Если название временной зоны получено как CVT
								else if(this->_fmk->compare("cvt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CVT);
								// Если название временной зоны получено как CXT
								else if(this->_fmk->compare("cxt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CXT);
								// Если название временной зоны получено как EAT
								else if(this->_fmk->compare("eat", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EAT);
								// Если название временной зоны получено как ECT
								else if(this->_fmk->compare("ect", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ECT);
								// Если название временной зоны получено как EDT
								else if(this->_fmk->compare("edt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EDT);
								// Если название временной зоны получено как EET
								else if(this->_fmk->compare("eet", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EET);
								// Если название временной зоны получено как EGT
								else if(this->_fmk->compare("egt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EGT);
								// Если название временной зоны получено как EIT
								else if(this->_fmk->compare("eit", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EIT);
								// Если название временной зоны получено как EST
								else if(this->_fmk->compare("est", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EST);
								// Если название временной зоны получено как FET
								else if(this->_fmk->compare("fet", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::FET);
								// Если название временной зоны получено как FJT
								else if(this->_fmk->compare("fjt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::FJT);
								// Если название временной зоны получено как FKT
								else if(this->_fmk->compare("fkt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::FKT);
								// Если название временной зоны получено как FNT
								else if(this->_fmk->compare("fnt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::FNT);
								// Если название временной зоны получено как GET
								else if(this->_fmk->compare("get", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GET);
								// Если название временной зоны получено как GFT
								else if(this->_fmk->compare("gft", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GFT);
								// Если название временной зоны получено как GIT
								else if(this->_fmk->compare("git", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GIT);
								// Если название временной зоны получено как GMT
								else if(this->_fmk->compare("gmt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GMT);
								// Если название временной зоны получено как GYT
								else if(this->_fmk->compare("gyt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GYT);
								// Если название временной зоны получено как HKT
								else if(this->_fmk->compare("hkt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::HKT);
								// Если название временной зоны получено как ICT
								else if(this->_fmk->compare("ict", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ICT);
								// Если название временной зоны получено как IDT
								else if(this->_fmk->compare("idt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::IDT);
								// Если название временной зоны получено как JST
								else if(this->_fmk->compare("jst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::JST);
								// Если название временной зоны получено как KGT
								else if(this->_fmk->compare("kgt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::KGT);
								// Если название временной зоны получено как KST
								else if(this->_fmk->compare("kst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::KST);
								// Если название временной зоны получено как MDT
								else if(this->_fmk->compare("mdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MDT);
								// Если название временной зоны получено как MHT
								else if(this->_fmk->compare("mht", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MHT);
								// Если название временной зоны получено как MIT
								else if(this->_fmk->compare("mit", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MIT);
								// Если название временной зоны получено как MMT
								else if(this->_fmk->compare("mmt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MMT);
								// Если название временной зоны получено как MSK
								else if(this->_fmk->compare("msk", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MSK);
								// Если название временной зоны получено как MSD
								else if(this->_fmk->compare("msd", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MSD);
								// Если название временной зоны получено как MUT
								else if(this->_fmk->compare("mut", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MUT);
								// Если название временной зоны получено как MVT
								else if(this->_fmk->compare("mvt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MVT);
								// Если название временной зоны получено как MYT
								else if(this->_fmk->compare("myt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MYT);
								// Если название временной зоны получено как NCT
								else if(this->_fmk->compare("nct", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NCT);
								// Если название временной зоны получено как NDT
								else if(this->_fmk->compare("ndt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NDT);
								// Если название временной зоны получено как NFT
								else if(this->_fmk->compare("nft", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NFT);
								// Если название временной зоны получено как NPT
								else if(this->_fmk->compare("npt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NPT);
								// Если название временной зоны получено как NRT
								else if(this->_fmk->compare("nrt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NRT);
								// Если название временной зоны получено как NST
								else if(this->_fmk->compare("nst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NST);
								// Если название временной зоны получено как NUT
								else if(this->_fmk->compare("nut", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NUT);
								// Если название временной зоны получено как PDT
								else if(this->_fmk->compare("pdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PDT);
								// Если название временной зоны получено как PET
								else if(this->_fmk->compare("pet", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PET);
								// Если название временной зоны получено как PGT
								else if(this->_fmk->compare("pgt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PGT);
								// Если название временной зоны получено как PHT
								else if(this->_fmk->compare("pht", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PHT);
								// Если название временной зоны получено как PKT
								else if(this->_fmk->compare("pkt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PKT);
								// Если название временной зоны получено как PST
								else if(this->_fmk->compare("pst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PST);
								// Если название временной зоны получено как PWT
								else if(this->_fmk->compare("pwt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PWT);
								// Если название временной зоны получено как PYT
								else if(this->_fmk->compare("pyt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PYT);
								// Если название временной зоны получено как RET
								else if(this->_fmk->compare("ret", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::RET);
								// Если название временной зоны получено как SBT
								else if(this->_fmk->compare("sbt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SBT);
								// Если название временной зоны получено как SCT
								else if(this->_fmk->compare("sct", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SCT);
								// Если название временной зоны получено как SGT
								else if(this->_fmk->compare("sgt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SGT);
								// Если название временной зоны получено как SRT
								else if(this->_fmk->compare("srt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SRT);
								// Если название временной зоны получено как SST
								else if(this->_fmk->compare("sst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SST);
								// Если название временной зоны получено как TFT
								else if(this->_fmk->compare("tft", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::TFT);
								// Если название временной зоны получено как THA
								else if(this->_fmk->compare("tha", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::THA);
								// Если название временной зоны получено как TJT
								else if(this->_fmk->compare("tjt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::TJT);
								// Если название временной зоны получено как TKT
								else if(this->_fmk->compare("tkt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::TKT);
								// Если название временной зоны получено как TLT
								else if(this->_fmk->compare("tlt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::TLT);
								// Если название временной зоны получено как TMT
								else if(this->_fmk->compare("tmt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::TMT);
								// Если название временной зоны получено как TOT
								else if(this->_fmk->compare("tot", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::TOT);
								// Если название временной зоны получено как TRT
								else if(this->_fmk->compare("trt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::TRT);
								// Если название временной зоны получено как TVT
								else if(this->_fmk->compare("tvt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::TVT);
								// Если название временной зоны получено как UTC
								else if(this->_fmk->compare("utc", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::UTC);
								// Если название временной зоны получено как UYT
								else if(this->_fmk->compare("uyt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::UYT);
								// Если название временной зоны получено как UZT
								else if(this->_fmk->compare("uzt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::UZT);
								// Если название временной зоны получено как VET
								else if(this->_fmk->compare("vet", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::VET);
								// Если название временной зоны получено как VUT
								else if(this->_fmk->compare("vut", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::VUT);
								// Если название временной зоны получено как WAT
								else if(this->_fmk->compare("wat", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::WAT);
								// Если название временной зоны получено как WET
								else if(this->_fmk->compare("wet", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::WET);
								// Если название временной зоны получено как WFT
								else if(this->_fmk->compare("wft", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::WFT);
								// Если название временной зоны получено как WIB
								else if(this->_fmk->compare("wib", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::WIB);
								// Если название временной зоны получено как WIT
								else if(this->_fmk->compare("wit", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::WIT);
								// Если название временной зоны получено как AMT
								else if(this->_fmk->compare("amt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AMTAM);
								// Если название временной зоны получено как AST
								else if(this->_fmk->compare("ast", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ASTAL);
								// Если название временной зоны получено как BST
								else if(this->_fmk->compare("bst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::BSTBR);
								// Если название временной зоны получено как CDT
								else if(this->_fmk->compare("cdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CDTNA);
								// Если название временной зоны получено как CST
								else if(this->_fmk->compare("cst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CSTNA);
								// Если название временной зоны получено как GST
								else if(this->_fmk->compare("gst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GSTPG);
								// Если название временной зоны получено как IST
								else if(this->_fmk->compare("ist", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ISTID);
								// Если название временной зоны получено как MST
								else if(this->_fmk->compare("mst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MSTNA);
								// Если название временной зоны получено как AEDT
								else if(this->_fmk->compare("aedt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AEDT);
								// Если название временной зоны получено как AKDT
								else if(this->_fmk->compare("akdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AKDT);
								// Если название временной зоны получено как AKST
								else if(this->_fmk->compare("akst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AKST);
								// Если название временной зоны получено как AMST
								else if(this->_fmk->compare("amst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AMST);
								// Если название временной зоны получено как AWST
								else if(this->_fmk->compare("awst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AWST);
								// Если название временной зоны получено как AZOT
								else if(this->_fmk->compare("azot", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AZOT);
								// Если название временной зоны получено как BRST
								else if(this->_fmk->compare("brst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::BRST);
								// Если название временной зоны получено как CEST
								else if(this->_fmk->compare("cest", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CEST);
								// Если название временной зоны получено как AEST
								else if(this->_fmk->compare("aest", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AEST);
								// Если название временной зоны получено как CHOT
								else if(this->_fmk->compare("chot", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CHOT);
								// Если название временной зоны получено как CHST
								else if(this->_fmk->compare("chst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CHST);
								// Если название временной зоны получено как CHUT
								else if(this->_fmk->compare("chut", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CHUT);
								// Если название временной зоны получено как CLST
								else if(this->_fmk->compare("clst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CLST);
								// Если название временной зоны получено как COST
								else if(this->_fmk->compare("cost", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::COST);
								// Если название временной зоны получено как DAVT
								else if(this->_fmk->compare("davt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::DAVT);
								// Если название временной зоны получено как DDUT
								else if(this->_fmk->compare("ddut", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::DDUT);
								// Если название временной зоны получено как EAST
								else if(this->_fmk->compare("east", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EAST);
								// Если название временной зоны получено как EEST
								else if(this->_fmk->compare("eest", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EEST);
								// Если название временной зоны получено как EGST
								else if(this->_fmk->compare("egst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EGST);
								// Если название временной зоны получено как FKST
								else if(this->_fmk->compare("fkst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::FKST);
								// Если название временной зоны получено как GALT
								else if(this->_fmk->compare("galt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GALT);
								// Если название временной зоны получено как GAMT
								else if(this->_fmk->compare("gamt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GAMT);
								// Если название временной зоны получено как GILT
								else if(this->_fmk->compare("gilt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::GILT);
								// Если название временной зоны получено как HADT
								else if(this->_fmk->compare("hadt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::HADT);
								// Если название временной зоны получено как HAST
								else if(this->_fmk->compare("hast", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::HAST);
								// Если название временной зоны получено как HOVT
								else if(this->_fmk->compare("hovt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::HOVT);
								// Если название временной зоны получено как IRDT
								else if(this->_fmk->compare("irdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::IRDT);
								// Если название временной зоны получено как IRKT
								else if(this->_fmk->compare("irkt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::IRKT);
								// Если название временной зоны получено как IRST
								else if(this->_fmk->compare("irst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::IRST);
								// Если название временной зоны получено как KOST
								else if(this->_fmk->compare("kost", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::KOST);
								// Если название временной зоны получено как KRAT
								else if(this->_fmk->compare("krat", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::KRAT);
								// Если название временной зоны получено как LHDT
								else if(this->_fmk->compare("lhdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::LHDT);
								// Если название временной зоны получено как LHST
								else if(this->_fmk->compare("lhst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::LHST);
								// Если название временной зоны получено как LINT
								else if(this->_fmk->compare("lint", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::LINT);
								// Если название временной зоны получено как MAGT
								else if(this->_fmk->compare("magt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MAGT);
								// Если название временной зоны получено как MART
								else if(this->_fmk->compare("mart", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MART);
								// Если название временной зоны получено как MAWT
								else if(this->_fmk->compare("mawt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MAWT);
								// Если название временной зоны получено как MIST
								else if(this->_fmk->compare("mist", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::MIST);
								// Если название временной зоны получено как NZDT
								else if(this->_fmk->compare("nzdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NZDT);
								// Если название временной зоны получено как NZST
								else if(this->_fmk->compare("nzst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::NZST);
								// Если название временной зоны получено как OMST
								else if(this->_fmk->compare("omst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::OMST);
								// Если название временной зоны получено как ORAT
								else if(this->_fmk->compare("orat", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ORAT);
								// Если название временной зоны получено как PETT
								else if(this->_fmk->compare("pett", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PETT);
								// Если название временной зоны получено как PHOT
								else if(this->_fmk->compare("phot", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PHOT);
								// Если название временной зоны получено как PhST
								else if(this->_fmk->compare("phst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PhST);
								// Если название временной зоны получено как PMDT
								else if(this->_fmk->compare("pmdt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PMDT);
								// Если название временной зоны получено как PMST
								else if(this->_fmk->compare("pmst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PMST);
								// Если название временной зоны получено как PONT
								else if(this->_fmk->compare("pont", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PONT);
								// Если название временной зоны получено как PYST
								else if(this->_fmk->compare("pyst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::PYST);
								// Если название временной зоны получено как ROTT
								else if(this->_fmk->compare("rott", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ROTT);
								// Если название временной зоны получено как SAKT
								else if(this->_fmk->compare("sakt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SAKT);
								// Если название временной зоны получено как SAMT
								else if(this->_fmk->compare("samt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SAMT);
								// Если название временной зоны получено как SAST
								else if(this->_fmk->compare("sast", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SAST);
								// Если название временной зоны получено как SLST
								else if(this->_fmk->compare("slst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SLST);
								// Если название временной зоны получено как SYOT
								else if(this->_fmk->compare("syot", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::SYOT);
								// Если название временной зоны получено как TAHT
								else if(this->_fmk->compare("taht", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::TAHT);
								// Если название временной зоны получено как ULAT
								else if(this->_fmk->compare("ulat", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ULAT);
								// Если название временной зоны получено как USZ1
								else if(this->_fmk->compare("usz1", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::USZ1);
								// Если название временной зоны получено как UYST
								else if(this->_fmk->compare("uyst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::UYST);
								// Если название временной зоны получено как VLAT
								else if(this->_fmk->compare("vlat", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::VLAT);
								// Если название временной зоны получено как VOLT
								else if(this->_fmk->compare("volt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::VOLT);
								// Если название временной зоны получено как VOST
								else if(this->_fmk->compare("vost", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::VOST);
								// Если название временной зоны получено как WAKT
								else if(this->_fmk->compare("wakt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::WAKT);
								// Если название временной зоны получено как WAST
								else if(this->_fmk->compare("wast", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::WAST);
								// Если название временной зоны получено как WEST
								else if(this->_fmk->compare("west", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::WEST);
								// Если название временной зоны получено как YAKT
								else if(this->_fmk->compare("yakt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::YAKT);
								// Если название временной зоны получено как YEKT
								else if(this->_fmk->compare("yekt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::YEKT);
								// Если название временной зоны получено как WGST
								else if(this->_fmk->compare("wgst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::WGSTST);
								// Если название временной зоны получено как CHADT
								else if(this->_fmk->compare("chadt", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CHADT);
								// Если название временной зоны получено как CHAST
								else if(this->_fmk->compare("chast", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CHAST);
								// Если название временной зоны получено как CHOST
								else if(this->_fmk->compare("chost", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::CHOST);
								// Если название временной зоны получено как ACWST
								else if(this->_fmk->compare("acwst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ACWST);
								// Если название временной зоны получено как AZOST
								else if(this->_fmk->compare("azost", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::AZOST);
								// Если название временной зоны получено как EASST
								else if(this->_fmk->compare("easst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::EASST);
								// Если название временной зоны получено как HOVST
								else if(this->_fmk->compare("hovst", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::HOVST);
								// Если название временной зоны получено как ULAST
								else if(this->_fmk->compare("ulast", name))
									// Выполняем получение временной зоны
									result = this->getTimeZone(zone_t::ULAST);
								// Если смещение времени указано
								if((data.size() > 5) && !data.at(4).empty()){
									// Получаем смещение времени
									const string & offset = data.at(4);
									// Если полученное смещение является числом
									if(this->_fmk->is(offset, fmk_t::check_t::NUMBER)){
										// Если указано 4 символа
										if(offset.size() == 4){
											// Получаем количество часов
											const uint8_t hour = static_cast <uint8_t> (::stoul(offset.substr(0, 2)));
											// Получаем количество минут
											const uint8_t minutes = static_cast <uint8_t> (::stoul(offset.substr(2)));
											// Если смещение времени положительное
											if(data.at(3).compare("+") == 0)
												// Получаем время смещения
												result += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
											// Устанавливаем отрицательное смещение времени
											else result -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
										// Если установлен всего один символ
										} else {
											// Если смещение времени положительное
											if(data.at(3).compare("+") == 0)
												// Получаем время смещения
												result += static_cast <int32_t> (::stoul(offset) * 60 * 60);
											// Устанавливаем отрицательное смещение времени
											else result -= static_cast <int32_t> (::stoul(offset) * 60 * 60);
										}
									// Если получено время в формате часов
									} else if((data.size() > 6) && !data.at(5).empty() && !data.at(6).empty()) {
										// Получаем количество часов
										const uint8_t hour = static_cast <uint8_t> (::stoul(data.at(5)));
										// Получаем количество минут
										const uint8_t minutes = static_cast <uint8_t> (::stoul(data.at(6)));
										// Если смещение времени положительное
										if(data.at(3).compare("+") == 0)
											// Получаем время смещения
											result += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
										// Устанавливаем отрицательное смещение времени
										else result -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
									}
								}
							}
						}
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
			// Результат работы функции
			result = this->_dt.offset;
		}
	}
	// Выводим результат
	return result;
}
/**
 * getTimeZone Метод определения текущей временной зоны относительно летнего времени
 * @param std временная зона стандартного времени
 * @param sum временная зона летнего времени
 * @return    текущее значение временной зоны
 */
int32_t awh::Chrono::getTimeZone(const zone_t std, const zone_t sum) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаем структуру времени
		dt_t dt;
		// Получаем стандартное время
		const int32_t result = this->getTimeZone(std);
		// Устанавливаем количество миллисекунд
		this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
		// Если время летнее
		if(dt.dst)
			// Получаем результат для летнего времени
			return this->getTimeZone(sum);
		// Выводим результат
		return result;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return this->_dt.offset;
}
/**
 * getTimeZone Метод получения установленной временной зоны
 * @param storage хранение значение времени
 * @return        установленное значение временной зоны
 */
int32_t awh::Chrono::getTimeZone(const storage_t storage) const noexcept {
	// Результат работы функции
	int32_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL):
				// Получаем локальное значение временной зоны в секундах
				result = this->_dt.offset;
			break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				/**
				 * Устанавливаем типы данных для Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Устанавливаем временную зону по умолчанию
					_tzset();
					// Получаем глобальное значение временной зоны в секундах
					result = (_timezone * -1);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Устанавливаем временную зону по умолчанию
					::tzset();
					// Получаем глобальное значение временной зоны в секундах
					result = (timezone * -1);
				#endif
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * clearTimeZones Метод очистки списка временных зон
 */
void awh::Chrono::clearTimeZones() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx.tz);
		// Выполняем очистку списка временных зон
		this->_timeZones.clear();
		// Выполняем освобождение выделенной памяти
		unordered_map <string, int32_t> ().swap(this->_timeZones);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
}
/**
 * addTimeZone Метод установки собственной временной зоны
 * @param name   название временной зоны
 * @param offset смещение времени в миллисекундах
 */
void awh::Chrono::addTimeZone(const string & name, const int32_t offset) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx.tz);
		// Выполняем добавление временной зоны в список временных зон
		this->_timeZones.emplace(this->_fmk->transform(name, fmk_t::transform_t::LOWER), offset);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
}
/**
 * setTimeZones Метод установки собственной временной зоны
 * @param zones список временных зон для установки
 */
void awh::Chrono::setTimeZones(const unordered_map <string, int32_t> & zones) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx.tz);
	// Название временной зоны
	string name = "";
	// Выполняем перебор всего списка временных зон
	for(auto & zone : zones){
		// Получаем название временной зоны
		name = zone.first;
		// Выполняем добавление временной зоны в список временных зон
		this->_timeZones.emplace(this->_fmk->transform(name, fmk_t::transform_t::LOWER), zone.second);
	}
}
/**
 * timestamp Метод установки штампа времени в указанных единицах измерения
 * @param date дата для установки
 * @param type единицы измерения штампа времени
 */
void awh::Chrono::timestamp(const uint64_t date, const type_t type) noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Текущий штамп времени
			uint64_t stamp = 0;
			// Определяем единицы измерения штампа времени
			switch(static_cast <uint8_t> (type)){
				// Если единицы измерения штампа времени требуется установить в годах
				case static_cast <uint8_t> (type_t::YEAR):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 31536000000);
				break;
				// Если единицы измерения штампа времени требуется установить в месяцах
				case static_cast <uint8_t> (type_t::MONTH):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 2629746000);
				break;
				// Если единицы измерения штампа времени требуется установить в неделях
				case static_cast <uint8_t> (type_t::WEEK):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 604800000);
				break;
				// Если единицы измерения штампа времени требуется установить в днях
				case static_cast <uint8_t> (type_t::DAY):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 86400000);
				break;
				// Если единицы измерения штампа времени требуется установить в часах
				case static_cast <uint8_t> (type_t::HOUR):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 3600000);
				break;
				// Если единицы измерения штампа времени требуется установить в минутах
				case static_cast <uint8_t> (type_t::MINUTES):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 60000);
				break;
				// Если единицы измерения штампа времени требуется установить в секундах
				case static_cast <uint8_t> (type_t::SECONDS):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 1000);
				break;
				// Если единицы измерения штампа времени требуется установить в миллисекундах
				case static_cast <uint8_t> (type_t::MILLISECONDS):
					// Получаем текущий штамп времени
					stamp = date;
				break;
				// Если единицы измерения штампа времени требуется установить в микросекундах
				case static_cast <uint8_t> (type_t::MICROSECONDS): {
					// Устанавливаем количество микросекунд
					this->_dt.microseconds = (date % 1000);
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date / 1000.L);
				} break;
				// Если единицы измерения штампа времени требуется установить в наносекундах
				case static_cast <uint8_t> (type_t::NANOSECONDS): {
					// Устанавливаем количество наносекунд
					this->_dt.nanoseconds = (date % 1000000);
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date / 1000000.L);
				} break;
			}
			// Выполняем блокировку потока
			const lock_guard <mutex> lock(this->_mtx.date);
			// Устанавливаем количество миллисекунд
			this->makeDate(stamp, this->_dt);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
}
/**
 * timestamp Метод получения штампа времени в указанных единицах измерения
 * @param type    единицы измерения штампа времени
 * @param storage хранение значение времени
 * @return        штамп времени в указанных единицах измерения
 */
uint64_t awh::Chrono::timestamp(const type_t type, const storage_t storage) const noexcept {
	// Результат работы функции
	uint64_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем единицы измерения штампа времени
		switch(static_cast <uint8_t> (type)){
			// Если единицы измерения штампа времени требуется получить в годы
			case static_cast <uint8_t> (type_t::YEAR): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 31536000000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / 8760.L);
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в месяцах
			case static_cast <uint8_t> (type_t::MONTH): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 2629746000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в часы
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (seconds.count() / 2629746.L);
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в неделях
			case static_cast <uint8_t> (type_t::WEEK): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 604800000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / 168.L);
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в днях
			case static_cast <uint8_t> (type_t::DAY): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 86400000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / 24.L);
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в часах
			case static_cast <uint8_t> (type_t::HOUR): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 3600000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в минутах
			case static_cast <uint8_t> (type_t::MINUTES): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 60000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в минуты
						chrono::minutes minutes = chrono::duration_cast <chrono::minutes> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (minutes.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в секундах
			case static_cast <uint8_t> (type_t::SECONDS): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 1000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в секундах
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (seconds.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в миллисекундах
			case static_cast <uint8_t> (type_t::MILLISECONDS): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = this->makeDate(this->_dt);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в миллисекундах
						chrono::milliseconds milliseconds = chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (milliseconds.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в микросекундах
			case static_cast <uint8_t> (type_t::MICROSECONDS): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = ((this->makeDate(this->_dt) * 1000) + this->_dt.microseconds);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в микросекунды
						chrono::microseconds microseconds = chrono::duration_cast <chrono::microseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (microseconds.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в наносекундах
			case static_cast <uint8_t> (type_t::NANOSECONDS): {
				// Определяем хранилизе значение времени
				switch(static_cast <uint8_t> (storage)){
					// Если хранилизе локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = ((this->makeDate(this->_dt) * 1000000) + this->_dt.nanoseconds);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в наносекундах
						chrono::nanoseconds nanoseconds = chrono::duration_cast <chrono::nanoseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (nanoseconds.count());
					} break;
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * parse Метод парсинга строки в UnixTimestamp
 * @param date    строка даты
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        дата в UnixTimestamp
 */
uint64_t awh::Chrono::parse(const string & date, const string & format, const storage_t storage) noexcept {
	// Результат работы функции
	uint64_t result = 0;
	// Если дата для парсинга и формат переданы
	if(!date.empty() && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Начальная позиция данных в тексте
		ssize_t pos = 0;
		// Символ для обработки
		char letter = 0;
		// Текущее количество минут прошедших с 1970-го года
		uint64_t lastMinutes = 0;
		// Флаги установки параметров
		bool flags[6] = {
			false, // Флаг установки временной зоны
			false, // Флаг установки года
			false, // Флаг установки часа
			false, // Флаг установки минут
			false, // Флаг установки секунд
			false  // Флаг установки миллисекунд
		};
		// Буфер содержащий переменную
		char buffer[2];
		// Сбрасываем буфер переменной
		::memset(buffer, 0, 2);
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Выполняем блокировку потока
				const lock_guard <mutex> lock(this->_mtx.date);
				// Выполняем сброс временной зоны
				this->_dt.offset = 0;
				// Выполняем сброс количества наносекунд
				this->_dt.nanoseconds = 0;
				// Выполняем сброс количества микросекунд
				this->_dt.microseconds = 0;
				// Получаем текущее значение штампа времени
				result = this->makeDate(this->_dt);
				// Получаем количество минут прошедших с 1970-го года
				lastMinutes = static_cast <uint64_t> (result / 1000.L / 60.L);
			} break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Получаем текущее значение штампа времени
				result = this->timestamp(type_t::MILLISECONDS);
				// Устанавливаем количество миллисекунд
				this->makeDate(result, dt);
				// Получаем количество минут прошедших с 1970-го года
				lastMinutes = static_cast <uint64_t> (result / 1000.L / 60.L);
			} break;
		}
		// Выполняем перебор формата
		for(size_t i = 0; i < format.length(); i++){
			// Получаем символ для обработки
			letter = format.at(i);
			// Определяем символ парсинга
			switch(letter){
				// Если мы нашли идентификатор переменной
				case '%': buffer[0] = '%'; break;
				// Если мы нашли переменную (y)
				case 'y':
				// Если мы нашли переменную (g)
				case 'g':
				// Если мы нашли переменную (Y)
				case 'Y':
				// Если мы нашли переменную (G)
				case 'G':
				// Если мы нашли переменную (b)
				case 'b':
				// Если мы нашли переменную (h)
				case 'h':
				// Если мы нашли переменную (B)
				case 'B':
				// Если мы нашли переменную (m)
				case 'm':
				// Если мы нашли переменную (d)
				case 'd':
				// Если мы нашли переменную (e)
				case 'e':
				// Если мы нашли переменную (a)
				case 'a':
				// Если мы нашли переменную (A)
				case 'A':
				// Если мы нашли переменную (j)
				case 'j':
				// Если мы нашли переменную (u)
				case 'u':
				// Если мы нашли переменную (U)
				case 'U':
				// Если мы нашли переменную (w)
				case 'w':
				// Если мы нашли переменную (W)
				case 'W':
				// Если мы нашли переменную (D)
				case 'D':
				// Если мы нашли переменную (x)
				case 'x':
				// Если мы нашли переменную (F)
				case 'F':
				// Если мы нашли переменную (H)
				case 'H':
				// Если мы нашли переменную (I)
				case 'I':
				// Если мы нашли переменную (M)
				case 'M':
				// Если мы нашли переменную (s)
				case 's':
				// Если мы нашли переменную (S)
				case 'S':
				// Если мы нашли переменную (p)
				case 'p':
				// Если мы нашли переменную (R)
				case 'R':
				// Если мы нашли переменную (T)
				case 'T':
				// Если мы нашли переменную (X)
				case 'X':
				// Если мы нашли переменную (r)
				case 'r':
				// Если мы нашли переменную (c)
				case 'c':
				// Если мы нашли переменную (o)
				case 'o':
				// Если мы нашли переменную (z)
				case 'z':
				// Если мы нашли переменную (Z)
				case 'Z': {
					// Если мы ищем переменную
					if(buffer[0] == '%'){
						// Определяем хранилизе значение времени
						switch(static_cast <uint8_t> (storage)){
							// Если хранилизе локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Выполняем блокировку потока
								const lock_guard <mutex> lock(this->_mtx.date);
								// Определяем символ парсинга
								switch(letter){
									// Если мы нашли переменную (y)
									case 'y':
									// Если мы нашли переменную (g)
									case 'g': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::y, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (Y)
									case 'Y':
									// Если мы нашли переменную (G)
									case 'G': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::Y, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (b)
									case 'b':
									// Если мы нашли переменную (h)
									case 'h':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::b, pos);
									break;
									// Если мы нашли переменную (B)
									case 'B':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::B, pos);
									break;
									// Если мы нашли переменную (m)
									case 'm':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::m, pos);
									break;
									// Если мы нашли переменную (d)
									case 'd':
									// Если мы нашли переменную (e)
									case 'e':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::d, pos);
									break;
									// Если мы нашли переменную (a)
									case 'a':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::a, pos);
									break;
									// Если мы нашли переменную (A)
									case 'A':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::A, pos);
									break;
									// Если мы нашли переменную (j)
									case 'j':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::j, pos);
									break;
									// Если мы нашли переменную (u)
									case 'u':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::u, pos);
									break;
									// Если мы нашли переменную (U)
									case 'U':
									// Если мы нашли переменную (W)
									case 'W':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::W, pos);
									break;
									// Если мы нашли переменную (w)
									case 'w':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::w, pos);
									break;
									// Если мы нашли переменную (D)
									case 'D':
									// Если мы нашли переменную (x)
									case 'x': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::D, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (F)
									case 'F': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::F, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (H)
									case 'H': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::H, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
									} break;
									// Если мы нашли переменную (I)
									case 'I': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::I, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
									} break;
									// Если мы нашли переменную (M)
									case 'M': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::M, pos);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
									} break;
									// Если мы нашли переменную (s)
									case 's': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::s, pos);
										// Устанавливаем флаг установки миллисекунд
										flags[5] = (pos > -1);
									} break;
									// Если мы нашли переменную (S)
									case 'S': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::S, pos);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (p)
									case 'p':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::p, pos);
									break;
									// Если мы нашли переменную (R)
									case 'R': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::R, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
									} break;
									// Если мы нашли переменную (T)
									case 'T':
									// Если мы нашли переменную (X)
									case 'X': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::T, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (r)
									case 'r': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::r, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (c)
									case 'c': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::c, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (z)
									case 'z': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::z, pos);
										// Если флаг ещё не установлен
										if(!flags[0])
											// Устанавливаем флаг установки смещения временной зоны
											flags[0] = (pos > -1);
									} break;
									// Если мы нашли переменную (Z)
									case 'Z': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::Z, pos);
										// Устанавливаем флаг установки смещения временной зоны
										flags[0] = (pos > -1);
									} break;
									// Если пришёл любой другой формат, завершаем работу
									default: pos = -1;
								}
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Определяем символ парсинга
								switch(letter){
									// Если мы нашли переменную (y)
									case 'y':
									// Если мы нашли переменную (g)
									case 'g': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::y, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (Y)
									case 'Y':
									// Если мы нашли переменную (G)
									case 'G': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::Y, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (b)
									case 'b':
									// Если мы нашли переменную (h)
									case 'h':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::b, pos);
									break;
									// Если мы нашли переменную (B)
									case 'B':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::B, pos);
									break;
									// Если мы нашли переменную (m)
									case 'm':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::m, pos);
									break;
									// Если мы нашли переменную (d)
									case 'd':
									// Если мы нашли переменную (e)
									case 'e':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::d, pos);
									break;
									// Если мы нашли переменную (a)
									case 'a':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::a, pos);
									break;
									// Если мы нашли переменную (A)
									case 'A':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::A, pos);
									break;
									// Если мы нашли переменную (j)
									case 'j':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::j, pos);
									break;
									// Если мы нашли переменную (u)
									case 'u':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::u, pos);
									break;
									// Если мы нашли переменную (U)
									case 'U':
									// Если мы нашли переменную (W)
									case 'W':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::W, pos);
									break;
									// Если мы нашли переменную (w)
									case 'w':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::w, pos);
									break;
									// Если мы нашли переменную (D)
									case 'D':
									// Если мы нашли переменную (x)
									case 'x': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::D, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (F)
									case 'F': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::F, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (H)
									case 'H': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::H, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
									} break;
									// Если мы нашли переменную (I)
									case 'I': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::I, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
									} break;
									// Если мы нашли переменную (M)
									case 'M': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::M, pos);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
									} break;
									// Если мы нашли переменную (s)
									case 's': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::s, pos);
										// Устанавливаем флаг установки миллисекунд
										flags[5] = (pos > -1);
									} break;
									// Если мы нашли переменную (S)
									case 'S': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::S, pos);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (p)
									case 'p':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::p, pos);
									break;
									// Если мы нашли переменную (R)
									case 'R': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::R, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
									} break;
									// Если мы нашли переменную (T)
									case 'T':
									// Если мы нашли переменную (X)
									case 'X': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::T, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (r)
									case 'r': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::r, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (c)
									case 'c': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::c, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (o)
									case 'o':
									// Если мы нашли переменную (z)
									case 'z': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::z, pos);
										// Если флаг ещё не установлен
										if(!flags[0])
											// Устанавливаем флаг установки смещения временной зоны
											flags[0] = (pos > -1);
									} break;
									// Если мы нашли переменную (Z)
									case 'Z': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::Z, pos);
										// Устанавливаем флаг установки смещения временной зоны
										flags[0] = (pos > -1);
									} break;
									// Если пришёл любой другой формат, завершаем работу
									default: pos = -1;
								}
							} break;
						}
						// Если позиция не определена
						if(pos < 0)
							// Завершаем перебор
							i = format.length();
					}
					// Сбрасываем буфер переменной
					::memset(buffer, 0, 2);
				} break;
				// Если получен любой другой символ
				default: ::memset(buffer, 0, 2);
			}
		}
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Выполняем блокировку потока
				const lock_guard <mutex> lock(this->_mtx.date);
				// Если флаг смещения временной зоны не передан
				if(!flags[0]){
					// Устанавливаем идентификатор временной зоны
					this->_dt.zone = zone_t::UTC;
					// Устанавливаем смещение временной зоны по умолчанию
					this->_dt.offset = this->getTimeZone();
				}
				// Получаем смещение временной зоны
				const int32_t offset = this->_dt.offset;
				// Если смещение временной зоны установлено
				if(offset != 0)
					// Выполняем инверсию
					this->_dt.offset *= -1;
				// Если час или минуты установлены а секунды нет
				if((flags[2] || flags[3]) && !flags[4])
					// Выполняем сброс секунд
					this->_dt.seconds = 0;
				// Если часы, минуты или секунды установлены а миллисекунды нет
				if((flags[2] || flags[3] || flags[4]) && !flags[5])
					// Выполняем сброс миллисекунд
					this->_dt.milliseconds = 0;
				// Выполняем формирование UnixTimestamp
				result = this->makeDate(this->_dt);
				// Если смещение временной зоны установлено
				if(offset != 0){
					// Выполняем установку пересчитанной временной зоны обратно
					this->makeDate(result, this->_dt);
					// Возвращаем значение временной зоны обратно
					this->_dt.offset = offset;
				}
				// Если количество минут переданной даты с начала 1970-го года выше чем текущее количество минут
				if(!flags[1] && (static_cast <uint64_t> (result / 1000.L / 60.L) > lastMinutes)){
					// Уменьшаем значение текущего года
					this->_dt.year--;
					// Устанавливаем флаг високосного года
					this->_dt.leap = ((this->_dt.year % 4) == 0);
					// Выполняем формирование UnixTimestamp
					result = this->makeDate(this->_dt);
					// Выполняем установку пересчитанной временной зоны обратно
					this->makeDate(result, this->_dt);
				}
			} break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Если флаг смещения временной зоны не передан
				if(!flags[0]){
					// Устанавливаем идентификатор временной зоны
					dt.zone = zone_t::UTC;
					// Устанавливаем смещение временной зоны по умолчанию
					dt.offset = this->getTimeZone();
				}
				// Если смещение временной зоны установлено
				if(dt.offset != 0)
					// Выполняем инверсию
					dt.offset *= -1;
				// Если час или минуты установлены а секунды нет
				if((flags[2] || flags[3]) && !flags[4])
					// Выполняем сброс секунд
					dt.seconds = 0;
				// Если часы, минуты или секунды установлены а миллисекунды нет
				if((flags[2] || flags[3] || flags[4]) && !flags[5])
					// Выполняем сброс миллисекунд
					dt.milliseconds = 0;
				// Выполняем формирование UnixTimestamp
				result = this->makeDate(dt);
				// Если количество минут переданной даты с начала 1970-го года выше чем текущее количество минут
				if(!flags[1] && (static_cast <uint64_t> (result / 1000.L / 60.L) > lastMinutes)){
					// Уменьшаем значение текущего года
					dt.year--;
					// Устанавливаем флаг високосного года
					dt.leap = ((dt.year % 4) == 0);
					// Выполняем формирование UnixTimestamp
					result = this->makeDate(dt);
				}
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * format Метод генерации формата временной зоны
 * @param zone временная зона (в секундах) в которой нужно получить результат
 * @return     строковое обозначение временной зоны
 */
string awh::Chrono::format(const int32_t zone) const noexcept {
	// Результат работы функции
	string result = "UTC";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если переданная зона больше нуля
		if(zone >= 0)
			// Добавляем плюс
			result.append(1, '+');
		// Добавляем минус
		else result.append(1, '-');
		// Временное значение переменной
		double intpart = 0;
		// Выполняем конвертацию часов в секунду
		const double seconds = (::abs(zone) / 3600.L);
		// Выполняем проверку есть ли дробная часть у числа
		if(::modf(seconds, &intpart) == 0)
			// Добавляем переданную зону
			result.append(std::to_string(static_cast <uint32_t> (seconds)));
		// Если мы нашли дробную часть числа
		else {
			// Добавляем первую часть часа
			result.append(std::to_string(static_cast <uint32_t> (intpart)));
			// Добавляем разделитель времени
			result.append(1, ':');
			// Добавляем дробную часть часа
			result.append(std::to_string(static_cast <uint32_t> ((seconds - intpart) * 60)));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
		// Выводим результат по умолчанию
		return "UTC+0";
	}
	// Выводим результат
	return result;
}
/**
 * format Метод генерации формата временной зоны
 * @param zone временная зона в которой нужно получить результат
 * @return     строковое обозначение временной зоны
 */
string awh::Chrono::format(const zone_t zone) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем временную зону
		switch(static_cast <uint8_t> (zone)){
			// Если временная зона не установлена
			case static_cast <uint8_t> (zone_t::NONE):
				// Выводим временную зону по умолчанию
				return this->format(this->_dt.offset);
			// Если временная зона установлена как (Атлантическое Время)
			case static_cast <uint8_t> (zone_t::AT): {
				// Создаем структуру времени
				dt_t dt;
				// Устанавливаем количество миллисекунд
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "ADT";
				// Если инверсия не включена
				return "UTC-4";
			}
			// Если временная зона установлена как (Северноамериканское Центральное Время)
			case static_cast <uint8_t> (zone_t::CT): {
				// Создаем структуру времени
				dt_t dt;
				// Устанавливаем количество миллисекунд
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "CDT";
				// Если инверсия не включена
				return "UTC-6";
			}
			// Если временная зона установлена как (Северноамериканское Восточное Время)
			case static_cast <uint8_t> (zone_t::ET): {
				// Создаем структуру времени
				dt_t dt;
				// Устанавливаем количество миллисекунд
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "EDT";
				// Выводим результат
				return "EST";
			}
			// Если временная зона установлена как (Северноамериканское Горное Время)
			case static_cast <uint8_t> (zone_t::MT): {
				// Создаем структуру времени
				dt_t dt;
				// Устанавливаем количество миллисекунд
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "MDT";
				// Выводим результат
				return "UTC-7";
			}
			// Если временная зона установлена как (Северноамериканское Тихоокеанское Время)
			case static_cast <uint8_t> (zone_t::PT): {
				// Создаем структуру времени
				dt_t dt;
				// Устанавливаем количество миллисекунд
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "PDT";
				// Выводим результат
				return "PST";
			}
			// Если временная зона установлена как (Время В Ньюфаундленде)
			case static_cast <uint8_t> (zone_t::NT):
				// Формируем временную зону
				return "NT";
			// Если временная зона установлена как (Стандартное Время На Острове Маврикий)
			case static_cast <uint8_t> (zone_t::MUT):
				// Формируем временную зону
				return "MUT";
			// Если временная зона установлена как (Время На Мальдивах)
			case static_cast <uint8_t> (zone_t::MVT):
				// Формируем временную зону
				return "MVT";
			// Если временная зона установлена как (Малайское Время)
			case static_cast <uint8_t> (zone_t::MYT):
				// Формируем временную зону
				return "MYT";
			// Если временная зона установлена как (Стандартное Время В Новой Каледонии)
			case static_cast <uint8_t> (zone_t::NCT):
				// Формируем временную зону
				return "NCT";
			// Если временная зона установлена как (Летнее Время В Ньюфаундленде)
			case static_cast <uint8_t> (zone_t::NDT):
				// Формируем временную зону
				return "NDT";
			// Если временная зона установлена как (Время На Острове Норфолк)
			case static_cast <uint8_t> (zone_t::NFT):
				// Формируем временную зону
				return "NFT";
			// Если временная зона установлена как (Непальськое Время)
			case static_cast <uint8_t> (zone_t::NPT):
				// Формируем временную зону
				return "NPT";
			// Если временная зона установлена как (Время На Острове Науру)
			case static_cast <uint8_t> (zone_t::NRT):
				// Формируем временную зону
				return "NRT";
			// Если временная зона установлена как (Стандартное Время В Ньюфаундленде)
			case static_cast <uint8_t> (zone_t::NST):
				// Формируем временную зону
				return "NST";
			// Если временная зона установлена как (Время На Острове Палау)
			case static_cast <uint8_t> (zone_t::PWT):
				// Формируем временную зону
				return "PWT";
			// Если временная зона установлена как (Время На Острове Ниуэ)
			case static_cast <uint8_t> (zone_t::NUT):
				// Формируем временную зону
				return "NUT";
			// Если временная зона установлена как (Минское Время)
			case static_cast <uint8_t> (zone_t::FET):
				// Формируем временную зону
				return "FET";
			// Если временная зона установлена как (Летнее Время На О. Фиджи)
			case static_cast <uint8_t> (zone_t::FJT):
				// Формируем временную зону
				return "FJT";
			// Если временная зона установлена как (Парагвайское Стандартное Время)
			case static_cast <uint8_t> (zone_t::PYT):
				// Формируем временную зону
				return "PYT";
			// Если временная зона установлена как (Время На Острове Реюньон)
			case static_cast <uint8_t> (zone_t::RET):
				// Формируем временную зону
				return "RET";
			// Если временная зона установлена как (Время На Соломоновых Островах)
			case static_cast <uint8_t> (zone_t::SBT):
				// Формируем временную зону
				return "SBT";
			// Если временная зона установлена как (Время На Сейшелах)
			case static_cast <uint8_t> (zone_t::SCT):
				// Формируем временную зону
				return "SCT";
			// Если временная зона установлена как (Сингапурское Время)
			case static_cast <uint8_t> (zone_t::SGT):
				// Формируем временную зону
				return "SGT";
			// Если временная зона установлена как (Время В Суринаме)
			case static_cast <uint8_t> (zone_t::SRT):
				// Формируем временную зону
				return "SRT";
			// Если временная зона установлена как (Стандартное Время На Острове Самоа)
			case static_cast <uint8_t> (zone_t::SST):
				// Формируем временную зону
				return "SST";
			// Если временная зона установлена как (Французское Южное И Антарктическое Время)
			case static_cast <uint8_t> (zone_t::TFT):
				// Формируем временную зону
				return "TFT";
			// Если временная зона установлена как (Тайландское Время)
			case static_cast <uint8_t> (zone_t::THA):
				// Формируем временную зону
				return "THA";
			// Если временная зона установлена как (Время В Таджикистане)
			case static_cast <uint8_t> (zone_t::TJT):
				// Формируем временную зону
				return "TJT";
			// Если временная зона установлена как (Время На Островах Токелау)
			case static_cast <uint8_t> (zone_t::TKT):
				// Формируем временную зону
				return "TKT";
			// Если временная зона установлена как (Время В Восточном Тиморе)
			case static_cast <uint8_t> (zone_t::TLT):
				// Формируем временную зону
				return "TLT";
			// Если временная зона установлена как (Стандартное Время В Туркмении)
			case static_cast <uint8_t> (zone_t::TMT):
				// Формируем временную зону
				return "TMT";
			// Если временная зона установлена как (Время На Островах Тонга)
			case static_cast <uint8_t> (zone_t::TOT):
				// Формируем временную зону
				return "TOT";
			// Если временная зона установлена как (Турецкое Время)
			case static_cast <uint8_t> (zone_t::TRT):
				// Формируем временную зону
				return "TRT";
			// Если временная зона установлена как (Время На Островах Тувалу)
			case static_cast <uint8_t> (zone_t::TVT):
				// Формируем временную зону
				return "TVT";
			// Если временная зона установлена как (Стандартное Время На Фолклендах)
			case static_cast <uint8_t> (zone_t::FKT):
				// Формируем временную зону
				return "FKT";
			// Если временная зона установлена как (Стандартное Время На Фернанду-Ди-Норонья)
			case static_cast <uint8_t> (zone_t::FNT):
				// Формируем временную зону
				return "FNT";
			// Если временная зона установлена как (Время В Афганистане)
			case static_cast <uint8_t> (zone_t::AFT):
				// Формируем временную зону
				return "AFT";
			// Если временная зона установлена как (Амазонское Стандартное Время)
			case static_cast <uint8_t> (zone_t::ACT):
				// Формируем временную зону
				return "ACT";
			// Если временная зона установлена как (Атлантическое Летнее Время)
			case static_cast <uint8_t> (zone_t::ADT):
				// Формируем временную зону
				return "ADT";
			// Если временная зона установлена как (Азербайджанское Стандартное Время)
			case static_cast <uint8_t> (zone_t::AZT):
				// Формируем временную зону
				return "AZT";
			// Если временная зона установлена как (Аргентинское Стандартное Время)
			case static_cast <uint8_t> (zone_t::ART):
				// Формируем временную зону
				return "ART";
			// Если временная зона установлена как (Время В Бруней-Даруссаламе)
			case static_cast <uint8_t> (zone_t::BDT):
				// Формируем временную зону
				return "BDT";
			// Если временная зона установлена как (Время В Бруней-Даруссаламе)
			case static_cast <uint8_t> (zone_t::BNT):
				// Формируем временную зону
				return "BNT";
			// Если временная зона установлена как (Боливийское Время)
			case static_cast <uint8_t> (zone_t::BOT):
				// Формируем временную зону
				return "BOT";
			// Если временная зона установлена как (Бразильское Стандартное Время)
			case static_cast <uint8_t> (zone_t::BRT):
				// Формируем временную зону
				return "BRT";
			// Если временная зона установлена как (Бутанское Время)
			case static_cast <uint8_t> (zone_t::BTT):
				// Формируем временную зону
				return "BTT";
			// Если временная зона установлена как (Восточноафриканское Время)
			case static_cast <uint8_t> (zone_t::CAT):
				// Формируем временную зону
				return "CAT";
			// Если временная зона установлена как (Стандартное Время На Островах Кабо-Верде)
			case static_cast <uint8_t> (zone_t::CVT):
				// Формируем временную зону
				return "CVT";
			// Если временная зона установлена как (Время На Острове Рождества)
			case static_cast <uint8_t> (zone_t::CXT):
				// Формируем временную зону
				return "CXT";
			// Если временная зона установлена как (Время На Кокосовые Островах)
			case static_cast <uint8_t> (zone_t::CCT):
				// Формируем временную зону
				return "CCT";
			// Если временная зона установлена как (Центральноевропейское Стандартное Время)
			case static_cast <uint8_t> (zone_t::CET):
				// Формируем временную зону
				return "CET";
			// Если временная зона установлена как (Время В Центральной Индонезии)
			case static_cast <uint8_t> (zone_t::CIT):
				// Формируем временную зону
				return "CIT";
			// Если временная зона установлена как (Стандартное Время На Островах Кука)
			case static_cast <uint8_t> (zone_t::CKT):
				// Формируем временную зону
				return "CKT";
			// Если временная зона установлена как (Чилийское Стандартное Время)
			case static_cast <uint8_t> (zone_t::CLT):
				// Формируем временную зону
				return "CLT";
			// Если временная зона установлена как (Колумбийское Стандартное Время)
			case static_cast <uint8_t> (zone_t::COT):
				// Формируем временную зону
				return "COT";
			// Если временная зона установлена как (Восточноафриканский Час)
			case static_cast <uint8_t> (zone_t::EAT):
				// Формируем временную зону
				return "EAT";
			// Если временная зона установлена как (Эквадорское Время)
			case static_cast <uint8_t> (zone_t::ECT):
				// Формируем временную зону
				return "ECT";
			// Если временная зона установлена как (Северноамериканское Восточное Летнее Время)
			case static_cast <uint8_t> (zone_t::EDT):
				// Формируем временную зону
				return "EDT";
			// Если временная зона установлена как (Восточноевропейское Стандартное Время)
			case static_cast <uint8_t> (zone_t::EET):
				// Формируем временную зону
				return "EET";
			// Если временная зона установлена как (Стандартное Время В Восточной Гренландии)
			case static_cast <uint8_t> (zone_t::EGT):
				// Формируем временную зону
				return "EGT";
			// Если временная зона установлена как (Время В Восточной Индонезии)
			case static_cast <uint8_t> (zone_t::EIT):
				// Формируем временную зону
				return "EIT";
			// Если временная зона установлена как (Северноамериканское Восточное Стандартное Время)
			case static_cast <uint8_t> (zone_t::EST):
				// Формируем временную зону
				return "EST";
			// Если временная зона установлена как (Грузинское Стандартное Время)
			case static_cast <uint8_t> (zone_t::GET):
				// Формируем временную зону
				return "GET";
			// Если временная зона установлена как (Время В Индокитае)
			case static_cast <uint8_t> (zone_t::ICT):
				// Формируем временную зону
				return "ICT";
			// Если временная зона установлена как (Израильское Летнее Время)
			case static_cast <uint8_t> (zone_t::IDT):
				// Формируем временную зону
				return "IDT";
			// Если временная зона установлена как (Время В Французской Гвиане)
			case static_cast <uint8_t> (zone_t::GFT):
				// Формируем временную зону
				return "GFT";
			// Если временная зона установлена как (Время На О. Гамбье)
			case static_cast <uint8_t> (zone_t::GIT):
				// Формируем временную зону
				return "GIT";
			// Если временная зона установлена как (Среднее Время По Гринвичу)
			case static_cast <uint8_t> (zone_t::GMT):
				// Формируем временную зону
				return "GMT";
			// Если временная зона установлена как (Время В Гайане)
			case static_cast <uint8_t> (zone_t::GYT):
				// Формируем временную зону
				return "GYT";
			// Если временная зона установлена как (Гонконгское Стандартное Время)
			case static_cast <uint8_t> (zone_t::HKT):
				// Формируем временную зону
				return "HKT";
			// Если временная зона установлена как (Японское Стандартное Время)
			case static_cast <uint8_t> (zone_t::JST):
				// Формируем временную зону
				return "JST";
			// Если временная зона установлена как (Время В Киргизии)
			case static_cast <uint8_t> (zone_t::KGT):
				// Формируем временную зону
				return "KGT";
			// Если временная зона установлена как (Корейское Стандартное Время)
			case static_cast <uint8_t> (zone_t::KST):
				// Формируем временную зону
				return "KST";
			// Если временная зона установлена как (Северноамериканское Горное Летнее Время)
			case static_cast <uint8_t> (zone_t::MDT):
				// Формируем временную зону
				return "MDT";
			// Если временная зона установлена как (Время На Маршалловых Островах)
			case static_cast <uint8_t> (zone_t::MHT):
				// Формируем временную зону
				return "MHT";
			// Если временная зона установлена как (Время На Маркизских Островах)
			case static_cast <uint8_t> (zone_t::MIT):
				// Формируем временную зону
				return "MIT";
			// Если временная зона установлена как (Время В Мьянме)
			case static_cast <uint8_t> (zone_t::MMT):
				// Формируем временную зону
				return "MMT";
			// Если временная зона установлена как (Московское Время)
			case static_cast <uint8_t> (zone_t::MSK):
				// Формируем временную зону
				return "MSK";
			// Если временная зона установлена как (Московское Летнее Время)
			case static_cast <uint8_t> (zone_t::MSD):
				// Формируем временную зону
				return "MSD";
			// Если временная зона установлена как (Северноамериканское Тихоокеанское Стандартное Время)
			case static_cast <uint8_t> (zone_t::PST):
				// Формируем временную зону
				return "PST";
			// Если временная зона установлена как (Северноамериканское Тихоокеанское Летнее Время)
			case static_cast <uint8_t> (zone_t::PDT):
				// Формируем временную зону
				return "PDT";
			// Если временная зона установлена как (Стандартное Время В Перу)
			case static_cast <uint8_t> (zone_t::PET):
				// Формируем временную зону
				return "PET";
			// Если временная зона установлена как (Время В Папуа-Новой Гвинее)
			case static_cast <uint8_t> (zone_t::PGT):
				// Формируем временную зону
				return "PGT";
			// Если временная зона установлена как (Всемирное Координированное Время)
			case static_cast <uint8_t> (zone_t::UTC):
				// Формируем временную зону
				return "UTC";
			// Если временная зона установлена как (Стандартное Время На Филлипинах)
			case static_cast <uint8_t> (zone_t::PHT):
				// Формируем временную зону
				return "PHT";
			// Если временная зона установлена как (Пакистанское Стандартное Время)
			case static_cast <uint8_t> (zone_t::PKT):
				// Формируем временную зону
				return "PKT";
			// Если временная зона установлена как (Стандартное Время В Уругвае)
			case static_cast <uint8_t> (zone_t::UYT):
				// Формируем временную зону
				return "UYT";
			// Если временная зона установлена как (Время В Узбекистане)
			case static_cast <uint8_t> (zone_t::UZT):
				// Формируем временную зону
				return "UZT";
			// Если временная зона установлена как (Время В Венесуеле)
			case static_cast <uint8_t> (zone_t::VET):
				// Формируем временную зону
				return "VET";
			// Если временная зона установлена как (Стандартное Время На Островах Вануату)
			case static_cast <uint8_t> (zone_t::VUT):
				// Формируем временную зону
				return "VUT";
			// Если временная зона установлена как (Западноафриканское Стандартное Время)
			case static_cast <uint8_t> (zone_t::WAT):
				// Формируем временную зону
				return "WAT";
			// Если временная зона установлена как (Западноевропейский Стандартний Час)
			case static_cast <uint8_t> (zone_t::WET):
				// Формируем временную зону
				return "WET";
			// Если временная зона установлена как (Время На Островах Уоллис И Футуна)
			case static_cast <uint8_t> (zone_t::WFT):
				// Формируем временную зону
				return "WFT";
			// Если временная зона установлена как (Время В Западной Индонезии)
			case static_cast <uint8_t> (zone_t::WIB):
				// Формируем временную зону
				return "WIB";
			// Если временная зона установлена как (Время В Восточной Индонезии)
			case static_cast <uint8_t> (zone_t::WIT):
				// Формируем временную зону
				return "WIT";
			// Если временная зона установлена как (Летнее Время В Центральной Австралии)
			case static_cast <uint8_t> (zone_t::ACDT):
				// Формируем временную зону
				return "ACDT";
			// Если временная зона установлена как (Стандартное Время В Центральной Австралии)
			case static_cast <uint8_t> (zone_t::ACST):
				// Формируем временную зону
				return "ACST";
			// Если временная зона установлена как (Летнее Время В Восточной Австралии)
			case static_cast <uint8_t> (zone_t::AEDT):
				// Формируем временную зону
				return "AEDT";
			// Если временная зона установлена как (Стандартное Время В Восточной Австралии)
			case static_cast <uint8_t> (zone_t::AEST):
				// Формируем временную зону
				return "AEST";
			// Если временная зона установлена как (Летнее Время На Аляске)
			case static_cast <uint8_t> (zone_t::AKDT):
				// Формируем временную зону
				return "AKDT";
			// Если временная зона установлена как (Стандартное Время На Аляске)
			case static_cast <uint8_t> (zone_t::AKST):
				// Формируем временную зону
				return "AKST";
			// Если временная зона установлена как (Амазонка, Стандартное Время)
			case static_cast <uint8_t> (zone_t::AMST):
				// Формируем временную зону
				return "AMST";
			// Если временная зона установлена как (Стандартное Время В Западной Австралии)
			case static_cast <uint8_t> (zone_t::AWST):
				// Формируем временную зону
				return "AWST";
			// Если временная зона установлена как (Стандартное Время На Азорских Островах)
			case static_cast <uint8_t> (zone_t::AZOT):
				// Формируем временную зону
				return "AZOT";
			// Если временная зона установлена как (Бразильское Летнее Время)
			case static_cast <uint8_t> (zone_t::BRST):
				// Формируем временную зону
				return "BRST";
			// Если временная зона установлена как (Чилийское Летнее Время)
			case static_cast <uint8_t> (zone_t::CLST):
				// Формируем временную зону
				return "CLST";
			// Если временная зона установлена как (Центральноевропейское Летнее Время)
			case static_cast <uint8_t> (zone_t::CEST):
				// Формируем временную зону
				return "CEST";
			// Если временная зона установлена как (Стандартное Время В Чойлобалсане)
			case static_cast <uint8_t> (zone_t::CHOT):
				// Формируем временную зону
				return "CHOT";
			// Если временная зона установлена как (Час Чаморро)
			case static_cast <uint8_t> (zone_t::CHST):
				// Формируем временную зону
				return "CHST";
			// Если временная зона установлена как (Время На Островах Чуук)
			case static_cast <uint8_t> (zone_t::CHUT):
				// Формируем временную зону
				return "CHUT";
			// Если временная зона установлена как (Колумбийское Летнее Время)
			case static_cast <uint8_t> (zone_t::COST):
				// Формируем временную зону
				return "COST";
			// Если временная зона установлена как (Дейвис)
			case static_cast <uint8_t> (zone_t::DAVT):
				// Формируем временную зону
				return "DAVT";
			// Если временная зона установлена как (Дюмон-Д'юрвиль)
			case static_cast <uint8_t> (zone_t::DDUT):
				// Формируем временную зону
				return "DDUT";
			// Если временная зона установлена как (Летнее Время В Восточной Гренландии)
			case static_cast <uint8_t> (zone_t::EGST):
				// Формируем временную зону
				return "EGST";
			// Если временная зона установлена как (Стандартное Время На Острове Пасхи)
			case static_cast <uint8_t> (zone_t::EAST):
				// Формируем временную зону
				return "EAST";
			// Если временная зона установлена как (Восточноевропейское Летнее Время)
			case static_cast <uint8_t> (zone_t::EEST):
				// Формируем временную зону
				return "EEST";
			// Если временная зона установлена как (Летнее Время На Фолклендах)
			case static_cast <uint8_t> (zone_t::FKST):
				// Формируем временную зону
				return "FKST";
			// Если временная зона установлена как (Время На Острове Гамбье)
			case static_cast <uint8_t> (zone_t::GAMT):
				// Формируем временную зону
				return "GAMT";
			// Если временная зона установлена как (Стандартное Время В Ховде)
			case static_cast <uint8_t> (zone_t::HOVT):
				// Формируем временную зону
				return "HOVT";
			// Если временная зона установлена как (Гавайско-Алеутское Летнее Время)
			case static_cast <uint8_t> (zone_t::HADT):
				// Формируем временную зону
				return "HADT";
			// Если временная зона установлена как (Гавайско-Алеутское Стандартное Время)
			case static_cast <uint8_t> (zone_t::HAST):
				// Формируем временную зону
				return "HAST";
			// Если временная зона установлена как (Иранское Летнее Время)
			case static_cast <uint8_t> (zone_t::IRDT):
				// Формируем временную зону
				return "IRDT";
			// Если временная зона установлена как (Иркутское Стандартное Время)
			case static_cast <uint8_t> (zone_t::IRKT):
				// Формируем временную зону
				return "IRKT";
			// Если временная зона установлена как (Иранское Стандартное Время)
			case static_cast <uint8_t> (zone_t::IRST):
				// Формируем временную зону
				return "IRST";
			// Если временная зона установлена как (Время На Островах Гилберта)
			case static_cast <uint8_t> (zone_t::GILT):
				// Формируем временную зону
				return "GILT";
			// Если временная зона установлена как (Время На Галапагосских Островах)
			case static_cast <uint8_t> (zone_t::GALT):
				// Формируем временную зону
				return "GALT";
			// Если временная зона установлена как (Время На Острове Косраэ)
			case static_cast <uint8_t> (zone_t::KOST):
				// Формируем временную зону
				return "KOST";
			// Если временная зона установлена как (Красноярское Стандартное Время)
			case static_cast <uint8_t> (zone_t::KRAT):
				// Формируем временную зону
				return "KRAT";
			// Если временная зона установлена как (Летнее Время На Лорд-Хау)
			case static_cast <uint8_t> (zone_t::LHDT):
				// Формируем временную зону
				return "LHDT";
			// Если временная зона установлена как (Стандартное Время На Лорд-Хау)
			case static_cast <uint8_t> (zone_t::LHST):
				// Формируем временную зону
				return "LHST";
			// Если временная зона установлена как (Время На Острове Лайн)
			case static_cast <uint8_t> (zone_t::LINT):
				// Формируем временную зону
				return "LINT";
			// Если временная зона установлена как (Магаданское Стандартное Время)
			case static_cast <uint8_t> (zone_t::MAGT):
				// Формируем временную зону
				return "MAGT";
			// Если временная зона установлена как (Время На Маркизских Островах)
			case static_cast <uint8_t> (zone_t::MART):
				// Формируем временную зону
				return "MART";
			// Если временная зона установлена как (Время На Станции Маккуори)
			case static_cast <uint8_t> (zone_t::MIST):
				// Формируем временную зону
				return "MIST";
			// Если временная зона установлена как (Время На Станции Моусон)
			case static_cast <uint8_t> (zone_t::MAWT):
				// Формируем временную зону
				return "MAWT";
			// Если временная зона установлена как (Летнее Время В Новой Зеландии)
			case static_cast <uint8_t> (zone_t::NZDT):
				// Формируем временную зону
				return "NZDT";
			// Если временная зона установлена как (Стандартное Время В Новой Зеландии)
			case static_cast <uint8_t> (zone_t::NZST):
				// Формируем временную зону
				return "NZST";
			// Если временная зона установлена как (Парагвайское Летнее Время)
			case static_cast <uint8_t> (zone_t::PYST):
				// Формируем временную зону
				return "PYST";
			// Если временная зона установлена как (Камчатское Время)
			case static_cast <uint8_t> (zone_t::PETT):
				// Формируем временную зону
				return "PETT";
			// Если временная зона установлена как (Летнее Время На Островах Сен-Пьер И Микелон)
			case static_cast <uint8_t> (zone_t::PMDT):
				// Формируем временную зону
				return "PMDT";
			// Если временная зона установлена как (Стандартное Время На Островах Сен-Пьер И Микелон)
			case static_cast <uint8_t> (zone_t::PMST):
				// Формируем временную зону
				return "PMST";
			// Если временная зона установлена как (Время На Острове Понапе)
			case static_cast <uint8_t> (zone_t::PONT):
				// Формируем временную зону
				return "PONT";
			// Если временная зона установлена как (Время На Островах Феникс)
			case static_cast <uint8_t> (zone_t::PHOT):
				// Формируем временную зону
				return "PHOT";
			// Если временная зона установлена как (Стандартное Время На Филлипинах)
			case static_cast <uint8_t> (zone_t::PhST):
				// Формируем временную зону
				return "PhST";
			// Если временная зона установлена как (Время На Станции Ротера)
			case static_cast <uint8_t> (zone_t::ROTT):
				// Формируем временную зону
				return "ROTT";
			// Если временная зона установлена как (Стандартное Время В Шри-Ланке)
			case static_cast <uint8_t> (zone_t::SLST):
				// Формируем временную зону
				return "SLST";
			// Если временная зона установлена как (Сахалинское Стандартное Время)
			case static_cast <uint8_t> (zone_t::SAKT):
				// Формируем временную зону
				return "SAKT";
			// Если временная зона установлена как (Самарское Время)
			case static_cast <uint8_t> (zone_t::SAMT):
				// Формируем временную зону
				return "SAMT";
			// Если временная зона установлена как (Южноафриканское Время)
			case static_cast <uint8_t> (zone_t::SAST):
				// Формируем временную зону
				return "SAST";
			// Если временная зона установлена как (Время На Станции Сёва)
			case static_cast <uint8_t> (zone_t::SYOT):
				// Формируем временную зону
				return "SYOT";
			// Если временная зона установлена как (Время На Острове Таити)
			case static_cast <uint8_t> (zone_t::TAHT):
				// Формируем временную зону
				return "TAHT";
			// Если временная зона установлена как (Омское Время)
			case static_cast <uint8_t> (zone_t::OMST):
				// Формируем временную зону
				return "OMST";
			// Если временная зона установлена как (Время В Западном Казахстане)
			case static_cast <uint8_t> (zone_t::ORAT):
				// Формируем временную зону
				return "ORAT";
			// Если временная зона установлена как (Владивостокское Время)
			case static_cast <uint8_t> (zone_t::VLAT):
				// Формируем временную зону
				return "VLAT";
			// Если временная зона установлена как (Волгоградское Время)
			case static_cast <uint8_t> (zone_t::VOLT):
				// Формируем временную зону
				return "VOLT";
			// Если временная зона установлена как (Время На Станции Восток)
			case static_cast <uint8_t> (zone_t::VOST):
				// Формируем временную зону
				return "VOST";
			// Если временная зона установлена как (Летнее Время В Уругвае)
			case static_cast <uint8_t> (zone_t::UYST):
				// Формируем временную зону
				return "UYST";
			// Если временная зона установлена как (Стандартное Время В Монголии)
			case static_cast <uint8_t> (zone_t::ULAT):
				// Формируем временную зону
				return "ULAT";
			// Если временная зона установлена как (Калиниградское Время)
			case static_cast <uint8_t> (zone_t::USZ1):
				// Формируем временную зону
				return "USZ1";
			// Если временная зона установлена как (Время На Острове Уэйк)
			case static_cast <uint8_t> (zone_t::WAKT):
				// Формируем временную зону
				return "WAKT";
			// Если временная зона установлена как (Западноафриканское Летнее Время)
			case static_cast <uint8_t> (zone_t::WAST):
				// Формируем временную зону
				return "WAST";
			// Если временная зона установлена как (Западноевропейское Летнее Время)
			case static_cast <uint8_t> (zone_t::WEST):
				// Формируем временную зону
				return "WEST";
			// Если временная зона установлена как (Стандартное Время В Западной Гренландии)
			case static_cast <uint8_t> (zone_t::WGST):
				// Формируем временную зону
				return "UTC-3";
			// Если временная зона установлена как (Якутское Время)
			case static_cast <uint8_t> (zone_t::YAKT):
				// Формируем временную зону
				return "YAKT";
			// Если временная зона установлена как (Екатеринбургское Время)
			case static_cast <uint8_t> (zone_t::YEKT):
				// Формируем временную зону
				return "YEKT";
			// Если временная зона установлена как (Центрально-Западная Австралия, Стандартное Время)
			case static_cast <uint8_t> (zone_t::ACWST):
				// Формируем временную зону
				return "ACWST";
			// Если временная зона установлена как (Летнее Время На Азорских Островах)
			case static_cast <uint8_t> (zone_t::AZOST):
				// Формируем временную зону
				return "AZOST";
			// Если временная зона установлена как (Летнее Время На Архипелаге Чатем)
			case static_cast <uint8_t> (zone_t::CHADT):
				// Формируем временную зону
				return "CHADT";
			// Если временная зона установлена как (Стандартное Время На Архипелаге Чатем)
			case static_cast <uint8_t> (zone_t::CHAST):
				// Формируем временную зону
				return "CHAST";
			// Если временная зона установлена как (Летнее Время В Чойлобалсане)
			case static_cast <uint8_t> (zone_t::CHOST):
				// Формируем временную зону
				return "CHOST";
			// Если временная зона установлена как (Летнее Время На Острове Пасхи)
			case static_cast <uint8_t> (zone_t::EASST):
				// Формируем временную зону
				return "EASST";
			// Если временная зона установлена как (Летнее Время В Ховде)
			case static_cast <uint8_t> (zone_t::HOVST):
				// Формируем временную зону
				return "HOVST";
			// Если временная зона установлена как (Летнее Время В Монголии)
			case static_cast <uint8_t> (zone_t::ULAST):
				// Формируем временную зону
				return "ULAST";
			// Если временная зона установлена как (Амазонское Стандартное Время)
			case static_cast <uint8_t> (zone_t::AMTAM):
				// Если инверсия не включена
				return "UTC-4";
			// Если временная зона установлена как (Армянское Стандартное Время)
			case static_cast <uint8_t> (zone_t::AMTAR):
				// Если инверсия не включена
				return "UTC+4";
			// Если временная зона установлена как (Атлантическое Стандартное Время)
			case static_cast <uint8_t> (zone_t::ASTAL):
				// Если инверсия не включена
				return "UTC-4";
			// Если временная зона установлена как (Стандартное Время В Саудовской Аравии)
			case static_cast <uint8_t> (zone_t::ASTSA):
				// Если инверсия не включена
				return "UTC+3";
			// Если временная зона установлена как (Британское Летнее Время)
			case static_cast <uint8_t> (zone_t::BSTBR):
				// Если инверсия не включена
				return "UTC+1";
			// Если временная зона установлена как (Стандартное Время В Бангладеш)
			case static_cast <uint8_t> (zone_t::BSTBL):
				// Если инверсия не включена
				return "UTC+6";
			// Если временная зона установлена как (Северноамериканское Центральное Летнее Время)
			case static_cast <uint8_t> (zone_t::CDTNA):
				// Если инверсия не включена
				return "UTC-5";
			// Если временная зона установлена как (Кубинское Летнее Время)
			case static_cast <uint8_t> (zone_t::CDTCB):
				// Если инверсия не включена
				return "UTC-4";
			// Если временная зона установлена как (Северноамериканское Центральное Стандартное Время)
			case static_cast <uint8_t> (zone_t::CSTNA):
				// Если инверсия не включена
				return "UTC-6";
			// Если временная зона установлена как (Китайское Стандартное Время)
			case static_cast <uint8_t> (zone_t::CSTKT):
				// Если инверсия не включена
				return "UTC+8";
			// Если временная зона установлена как (Кубинское Стандартное Время)
			case static_cast <uint8_t> (zone_t::CSTCB):
				// Если инверсия не включена
				return "UTC-5";
			// Если временная зона установлена как (Время В Персидском Заливе)
			case static_cast <uint8_t> (zone_t::GSTPG):
				// Если инверсия не включена
				return "UTC+4";
			// Если временная зона установлена как (Время В Южной Георгии)
			case static_cast <uint8_t> (zone_t::GSTSG):
				// Если инверсия не включена
				return "UTC-2";
			// Если временная зона установлена как (Индийское Стандартное Время)
			case static_cast <uint8_t> (zone_t::ISTID):
				// Если инверсия не включена
				return "UTC+5:30";
			// Если временная зона установлена как (Ирландия, Летнее Время)
			case static_cast <uint8_t> (zone_t::ISTIR):
				// Если инверсия не включена
				return "UTC+1";
			// Если временная зона установлена как (Израильское Стандартное Время)
			case static_cast <uint8_t> (zone_t::ISTIS):
				// Если инверсия не включена
				return "UTC+2";
			// Если временная зона установлена как (Северноамериканское Горное Стандартное Время)
			case static_cast <uint8_t> (zone_t::MSTNA):
				// Если инверсия не включена
				return "UTC-7";
			// Если временная зона установлена как (Время В Малайзии)
			case static_cast <uint8_t> (zone_t::MSTMS):
				// Если инверсия не включена
				return "UTC+8";
			// Если временная зона установлена как (Летнее Время В Западной Гренландии)
			case static_cast <uint8_t> (zone_t::WGSTST):
				// Если инверсия не включена
				return "UTC-2";
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", error.what());
		#endif
	}
	// Выводим результат
	return "UTC";
}
/**
 * format Метод формирования UnixTimestamp в строку
 * @param dt     объект даты и времени
 * @param format формат даты
 * @return       строка содержащая дату
 */
string awh::Chrono::format(const dt_t & dt, const string & format) const noexcept {
	// Результат работы функции
	string result = "";
	// Если формат даты передан
	if(!format.empty()){
		// Символ для обработки
		char letter = 0;
		// Буфер содержащий переменную
		char buffer[2];
		// Сбрасываем буфер переменной
		::memset(buffer, 0, 2);
		// Выполняем перебор формата
		for(size_t i = 0; i < format.length(); i++){
			// Получаем символ для обработки
			letter = format.at(i);
			// Определяем символ парсинга
			switch(letter){
				// Если мы нашли идентификатор переменной
				case '%': buffer[0] = '%'; break;
				// Если мы нашли переменную (y)
				case 'y':
				// Если мы нашли переменную (g)
				case 'g':
				// Если мы нашли переменную (Y)
				case 'Y':
				// Если мы нашли переменную (G)
				case 'G':
				// Если мы нашли переменную (b)
				case 'b':
				// Если мы нашли переменную (h)
				case 'h':
				// Если мы нашли переменную (B)
				case 'B':
				// Если мы нашли переменную (m)
				case 'm':
				// Если мы нашли переменную (d)
				case 'd':
				// Если мы нашли переменную (e)
				case 'e':
				// Если мы нашли переменную (a)
				case 'a':
				// Если мы нашли переменную (A)
				case 'A':
				// Если мы нашли переменную (j)
				case 'j':
				// Если мы нашли переменную (u)
				case 'u':
				// Если мы нашли переменную (U)
				case 'U':
				// Если мы нашли переменную (w)
				case 'w':
				// Если мы нашли переменную (W)
				case 'W':
				// Если мы нашли переменную (D)
				case 'D':
				// Если мы нашли переменную (x)
				case 'x':
				// Если мы нашли переменную (F)
				case 'F':
				// Если мы нашли переменную (H)
				case 'H':
				// Если мы нашли переменную (I)
				case 'I':
				// Если мы нашли переменную (M)
				case 'M':
				// Если мы нашли переменную (s)
				case 's':
				// Если мы нашли переменную (S)
				case 'S':
				// Если мы нашли переменную (p)
				case 'p':
				// Если мы нашли переменную (R)
				case 'R':
				// Если мы нашли переменную (T)
				case 'T':
				// Если мы нашли переменную (X)
				case 'X':
				// Если мы нашли переменную (r)
				case 'r':
				// Если мы нашли переменную (c)
				case 'c':
				// Если мы нашли переменную (o)
				case 'o':
				// Если мы нашли переменную (z)
				case 'z':
				// Если мы нашли переменную (Z)
				case 'Z': {
					// Если мы ищем переменную
					if(buffer[0] == '%'){
						// Определяем символ парсинга
						switch(letter){
							// Если мы нашли переменную (y)
							case 'y':
							// Если мы нашли переменную (g)
							case 'g':
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year).substr(2));
							break;
							// Если мы нашли переменную (Y)
							case 'Y':
							// Если мы нашли переменную (G)
							case 'G':
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year));
							break;
							// Если мы нашли переменную (b)
							case 'b':
							// Если мы нашли переменную (h)
							case 'h':
								// Выполняем формирование названия месяца
								result.append(nameMonths.at(dt.month - 1).first);
							break;
							// Если мы нашли переменную (B)
							case 'B':
								// Выполняем формирование названия месяца
								result.append(nameMonths.at(dt.month - 1).second);
							break;
							// Если мы нашли переменную (m)
							case 'm': {
								// Получаем номер месяца
								string month = std::to_string(dt.month);
								// Если первого нуля нет
								if(month.length() == 1)
									// Добавляем предстоящий ноль
									month.insert(month.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(month));
							} break;
							// Если мы нашли переменную (d)
							case 'd': {
								// Получаем число месяца
								string date = std::to_string(dt.date);
								// Если первого нуля нет
								if(date.length() == 1)
									// Добавляем предстоящий ноль
									date.insert(date.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(date));
							} break;
							// Если мы нашли переменную (e)
							case 'e':
								// Добавляем полученный результат
								result.append(std::to_string(dt.date));
							break;
							// Если мы нашли переменную (a)
							case 'a':
								// Выполняем формирование названия дня недели
								result.append(nameDays.at(dt.day - 1).first);
							break;
							// Если мы нашли переменную (A)
							case 'A':
								// Выполняем формирование названия дня недели
								result.append(nameDays.at(dt.day - 1).second);
							break;
							// Если мы нашли переменную (u)
							case 'u':
								// Добавляем полученный результат
								result.append(std::to_string(dt.day));
							break;
							// Если мы нашли переменную (w)
							case 'w':
								// Добавляем полученный результат
								result.append(std::to_string(dt.day == 7 ? 0 : dt.day));
							break;
							// Если мы нашли переменную (W)
							case 'W':
							// Если мы нашли переменную (U)
							case 'U': {
								// Получаем количество недель с начала года
								string weeks = std::to_string(dt.weeks);
								// Если первого нуля нет
								if(weeks.length() == 1)
									// Добавляем предстоящий ноль
									weeks.insert(weeks.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(weeks));
							} break;
							// Если мы нашли переменную (j)
							case 'j': {
								// Получаем количество дней с начала года
								string days = std::to_string(dt.days + 1);
								// Если первого нуля нет
								if(days.length() == 1)
									// Добавляем предстоящий ноль
									days.insert(days.begin(), 2, '0');
								// Если второго нуля нет
								else if(days.length() == 2)
									// Добавляем предстоящий ноль
									days.insert(days.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(days));
							} break;
							// Если мы нашли переменную (D)
							case 'D':
							// Если мы нашли переменную (x)
							case 'x': {
								// Получаем номер месяца
								string num = std::to_string(dt.month);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, '/');
								// Получаем число месяца
								num = std::to_string(dt.date);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, '/');
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year).substr(2));
							} break;
							// Если мы нашли переменную (F)
							case 'F': {
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year));
								// Добавляем разделитель
								result.append(1, '-');
								// Получаем номер месяца
								string num = std::to_string(dt.month);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, '-');
								// Получаем число месяца
								num = std::to_string(dt.date);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
							} break;
							// Если мы нашли переменную (H)
							case 'H': {
								// Получаем час времени
								string hour = std::to_string(dt.hour);
								// Если первого нуля нет
								if(hour.length() == 1)
									// Добавляем предстоящий ноль
									hour.insert(hour.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(hour));
							} break;
							// Если мы нашли переменную (I)
							case 'I': {
								// Номер часа времени
								string hour = "";
								// Если время до полудня
								if(dt.h12 == h12_t::AM)
									// Получаем час времени
									hour = std::to_string(dt.hour);
								// Добавляем время с учётом 12-и часового формата
								else hour = std::to_string(dt.hour - 12);
								// Если первого нуля нет
								if(hour.length() == 1)
									// Добавляем предстоящий ноль
									hour.insert(hour.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(hour));
							} break;
							// Если мы нашли переменную (M)
							case 'M': {
								// Получаем количество минут времени
								string minutes = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(minutes.length() == 1)
									// Добавляем предстоящий ноль
									minutes.insert(minutes.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(minutes));
							} break;
							// Если мы нашли переменную (s)
							case 's': {
								// Получаем количество миллисекунд времени
								string milliseconds = std::to_string(dt.milliseconds);
								// Если первого нуля нет
								if(milliseconds.length() == 1)
									// Добавляем предстоящий ноль
									milliseconds.insert(milliseconds.begin(), 2, '0');
								// Если второго нуля нет
								else if(milliseconds.length() == 2)
									// Добавляем предстоящий ноль
									milliseconds.insert(milliseconds.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(milliseconds));
							} break;
							// Если мы нашли переменную (S)
							case 'S': {
								// Получаем количество секунд времени
								string seconds = std::to_string(dt.seconds);
								// Если первого нуля нет
								if(seconds.length() == 1)
									// Добавляем предстоящий ноль
									seconds.insert(seconds.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(seconds));
							} break;
							// Если мы нашли переменную (p)
							case 'p':
								// Добавляем формат 12-и часового времени
								result.append(dt.h12 == h12_t::AM ? "AM" : "PM");
							break;
							// Если мы нашли переменную (R)
							case 'R': {
								// Получаем час времени
								string num = std::to_string(dt.hour);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество минут времени
								num = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
							} break;
							// Если мы нашли переменную (T)
							case 'T':
							// Если мы нашли переменную (X)
							case 'X': {
								// Получаем час времени
								string num = std::to_string(dt.hour);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество минут времени
								num = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество секунд времени
								num = std::to_string(dt.seconds);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
							} break;
							// Если мы нашли переменную (r)
							case 'r': {
								// Номер часа времени
								string num = "";
								// Если время до полудня
								if(dt.h12 == h12_t::AM)
									// Получаем час времени
									num = std::to_string(dt.hour);
								// Добавляем время с учётом 12-и часового формата
								else num = std::to_string(dt.hour - 12);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество минут времени
								num = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество секунд времени
								num = std::to_string(dt.seconds);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ' ');
								// Добавляем формат 12-и часового времени
								result.append(dt.h12 == h12_t::AM ? "AM" : "PM");
							} break;
							// Если мы нашли переменную (c)
							case 'c': {
								// Выполняем формирование названия дня недели
								result.append(nameDays.at(dt.day - 1).first);
								// Добавляем разделитель
								result.append(1, ' ');
								// Выполняем формирование названия месяца
								result.append(nameMonths.at(dt.month - 1).first);
								// Добавляем разделитель
								result.append(1, ' ');
								// Добавляем полученный результат
								result.append(std::to_string(dt.date));
								// Добавляем разделитель
								result.append(1, ' ');
								// Получаем час времени
								string num = std::to_string(dt.hour);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество минут времени
								num = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество секунд времени
								num = std::to_string(dt.seconds);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ' ');
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year));
							} break;
							// Если мы нашли переменную (o)
							case 'o': {
								// Если переданная зона больше нуля
								if(dt.offset >= 0)
									// Добавляем плюс
									result.append(1, '+');
								// Добавляем минус
								else result.append(1, '-');
								// Временное значение переменной
								double intpart = 0;
								// Выполняем конвертацию часов в секунду
								const double seconds = (::abs(dt.offset) / 3600.);
								// Выполняем проверку есть ли дробная часть у числа
								if(::modf(seconds, &intpart) == 0) {
									// Получаем количество секунд времени
									string num = std::to_string(static_cast <uint32_t> (seconds));
									// Если первого нуля нет
									if(num.length() == 1)
										// Добавляем предстоящий ноль
										num.insert(num.begin(), 1, '0');
									// Добавляем полученный результат
									result.append(::move(num));
									// Добавляем разделитель
									result.append(1, ':');
									// Добавляем конечные нули
									result.append(2, '0');
								// Если мы нашли дробную часть числа
								} else {
									// Добавляем первую часть часа
									result.append(std::to_string(static_cast <uint32_t> (intpart)));
									// Добавляем разделитель
									result.append(1, ':');
									// Добавляем дробную часть часа
									result.append(std::to_string(static_cast <uint32_t> ((seconds - intpart) * 60)));
								}
							} break;
							// Если мы нашли переменную (z)
							case 'z': {
								// Если переданная зона больше нуля
								if(dt.offset >= 0)
									// Добавляем плюс
									result.append(1, '+');
								// Добавляем минус
								else result.append(1, '-');
								// Временное значение переменной
								double intpart = 0;
								// Выполняем конвертацию часов в секунду
								const double seconds = (::abs(dt.offset) / 3600.);
								// Выполняем проверку есть ли дробная часть у числа
								if(::modf(seconds, &intpart) == 0) {
									// Получаем количество секунд времени
									string num = std::to_string(static_cast <uint32_t> (seconds));
									// Если первого нуля нет
									if(num.length() == 1)
										// Добавляем предстоящий ноль
										num.insert(num.begin(), 1, '0');
									// Добавляем полученный результат
									result.append(::move(num));
									// Добавляем конечные нули
									result.append(2, '0');
								// Если мы нашли дробную часть числа
								} else {
									// Добавляем первую часть часа
									result.append(std::to_string(static_cast <uint32_t> (intpart)));
									// Добавляем дробную часть часа
									result.append(std::to_string(static_cast <uint32_t> ((seconds - intpart) * 60)));
								}
							} break;
							// Если мы нашли переменную (Z)
							case 'Z': {
								// Если временная зона установлена пустая
								if(dt.zone == zone_t::NONE)
									// Выполняем формирование временной зоны
									result.append(this->format(dt.offset));
								// Выводим установленную временную зону
								else result.append(this->format(dt.zone));
							} break;
							// Добавляем полученный символ в результат
							default: result.append(1, letter);
						}
					// Добавляем полученный символ в результат
					} else result.append(1, letter);
					// Сбрасываем буфер переменной
					::memset(buffer, 0, 2);
				} break;
				// Если получен любой другой символ
				default: {
					// Сбрасываем буфер переменной
					::memset(buffer, 0, 2);
					// Добавляем полученный символ в результат
					result.append(1, letter);
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * format Метод формирования UnixTimestamp в строку
 * @param date   дата в UnixTimestamp
 * @param format формат даты
 * @return       строка содержащая дату
 */
string awh::Chrono::format(const uint64_t date, const string & format) const noexcept {
	// Если формат даты передан
	if((date > 0) && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Устанавливаем количество миллисекунд
		this->makeDate(date, dt);
		// Устанавливаем локальную временную зону
		dt.offset = this->getTimeZone();
		// Если смещение выше нуля
		if(dt.offset != 0){
			// Выполняем замену полученной даты
			const_cast <uint64_t &> (date) = this->makeDate(dt);
			// Устанавливаем количество миллисекунд
			this->makeDate(date, dt);
		}
		// Выполняем формирование формата даты
		return this->format(dt, format);
	}
	// Выводим результат
	return "";
}
/**
 * format Метод формирования UnixTimestamp в строку
 * @param date   дата в UnixTimestamp
 * @param zone   временная зона в которой нужно получить дату (в секундах)
 * @param format формат даты
 * @return       строка содержащая дату
 */
string awh::Chrono::format(const uint64_t date, const int32_t zone, const string & format) const noexcept {
	// Если формат даты передан
	if((date > 0) && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Устанавливаем количество миллисекунд
		this->makeDate(date, dt);
		// Выполняем установку смещения временной зоны
		dt.offset = zone;
		// Если смещение выше нуля
		if(dt.offset != 0){
			// Выполняем замену полученной даты
			const_cast <uint64_t &> (date) = this->makeDate(dt);
			// Устанавливаем количество миллисекунд
			this->makeDate(date, dt);
		}
		// Выполняем формирование формата даты
		return this->format(dt, format);
	}
	// Выводим результат
	return "";
}
/**
 * format Метод формирования UnixTimestamp в строку
 * @param date   дата в UnixTimestamp
 * @param zone   временная зона в которой нужно получить дату
 * @param format формат даты
 * @return       строка содержащая дату
 */
string awh::Chrono::format(const uint64_t date, const zone_t zone, const string & format) const noexcept {
	// Если формат даты передан
	if((date > 0) && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Устанавливаем количество миллисекунд
		this->makeDate(date, dt);
		// Устанавливаем временную зону
		dt.zone = zone;
		// Выполняем установку смещения временной зоны
		dt.offset = this->getTimeZone(zone);
		// Если смещение выше нуля
		if(dt.offset != 0){
			// Выполняем замену полученной даты
			const_cast <uint64_t &> (date) = this->makeDate(dt);
			// Устанавливаем количество миллисекунд
			this->makeDate(date, dt);
		}
		// Выполняем формирование формата даты
		return this->format(dt, format);
	}
	// Выводим результат
	return "";
}
/**
 * format Метод формирования UnixTimestamp в строку
 * @param date   дата в UnixTimestamp
 * @param zone   временная зона в которой нужно получить дату
 * @param format формат даты
 * @return       строка содержащая дату
 */
string awh::Chrono::format(const uint64_t date, const string & zone, const string & format) const noexcept {
	// Если формат даты передан
	if((date > 0) && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Устанавливаем количество миллисекунд
		this->makeDate(date, dt);
		// Устанавливаем временную зону
		dt.zone = this->matchTimeZone(zone);
		// Выполняем установку смещения временной зоны
		dt.offset = this->getTimeZone(zone);
		// Если смещение выше нуля
		if(dt.offset != 0){
			// Выполняем замену полученной даты
			const_cast <uint64_t &> (date) = this->makeDate(dt);
			// Устанавливаем количество миллисекунд
			this->makeDate(date, dt);
		}
		// Выполняем формирование формата даты
		return this->format(dt, format);
	}
	// Выводим результат
	return "";
}
/**
 * format Метод формирования UnixTimestamp в строку
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        строка содержащая дату
 */
string awh::Chrono::format(const string & format, const storage_t storage) const noexcept {
	// Если формат даты передан
	if(!format.empty()){
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Создаем структуру времени
				dt_t dt = this->_dt;
				// Если временная зона не установлена
				if((dt.offset == 0) && (dt.zone == zone_t::NONE))
					// Устанавливаем смещение временной зоны по умолчанию
					dt.offset = this->getTimeZone();
				// Устанавливаем количество миллисекунд
				this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Создаем структуру времени
				dt_t dt;
				// Устанавливаем количество миллисекунд
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Устанавливаем локальную временную зону
				dt.offset = this->getTimeZone(storage);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Устанавливаем количество миллисекунд
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
		}
	}
	// Выводим результат
	return "";
}
/**
 * format Метод формирования UnixTimestamp в строку
 * @param zone    временная зона в которой нужно получить дату (в секундах)
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        строка содержащая дату
 */
string awh::Chrono::format(const int32_t zone, const string & format, const storage_t storage) const noexcept {
	// Если формат даты передан
	if(!format.empty()){
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Создаем структуру времени
				dt_t dt = this->_dt;
				// Выполняем установку смещения временной зоны
				dt.offset = zone;
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Устанавливаем количество миллисекунд
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Создаем структуру времени
				dt_t dt;
				// Устанавливаем количество миллисекунд
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Выполняем установку смещения временной зоны
				dt.offset = zone;
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Устанавливаем количество миллисекунд
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
		}
	}
	// Выводим результат
	return "";
}
/**
 * format Метод формирования UnixTimestamp в строку
 * @param zone    временная зона в которой нужно получить дату
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        строка содержащая дату
 */
string awh::Chrono::format(const zone_t zone, const string & format, const storage_t storage) const noexcept {
	// Если формат даты передан
	if(!format.empty()){
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Создаем структуру времени
				dt_t dt = this->_dt;
				// Устанавливаем временную зону
				dt.zone = zone;
				// Выполняем установку смещения временной зоны
				dt.offset = this->getTimeZone(zone);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Устанавливаем количество миллисекунд
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Создаем структуру времени
				dt_t dt;
				// Устанавливаем количество миллисекунд
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Устанавливаем временную зону
				dt.zone = zone;
				// Выполняем установку смещения временной зоны
				dt.offset = this->getTimeZone(zone);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Устанавливаем количество миллисекунд
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
		}
	}
	// Выводим результат
	return "";
}
/**
 * format Метод формирования UnixTimestamp в строку
 * @param zone    временная зона в которой нужно получить дату
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        строка содержащая дату
 */
string awh::Chrono::format(const string & zone, const string & format, const storage_t storage) const noexcept {
	// Если формат даты передан
	if(!format.empty()){
		// Определяем хранилизе значение времени
		switch(static_cast <uint8_t> (storage)){
			// Если хранилизе локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Создаем структуру времени
				dt_t dt = this->_dt;
				// Устанавливаем временную зону
				dt.zone = this->matchTimeZone(zone);
				// Выполняем установку смещения временной зоны
				dt.offset = this->getTimeZone(zone);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Устанавливаем количество миллисекунд
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Создаем структуру времени
				dt_t dt;
				// Устанавливаем количество миллисекунд
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Устанавливаем временную зону
				dt.zone = this->matchTimeZone(zone);
				// Выполняем установку смещения временной зоны
				dt.offset = this->getTimeZone(zone);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Устанавливаем количество миллисекунд
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
		}
	}
	// Выводим результат
	return "";
}
/**
 * strip Метод получения UnixTimestamp из строки
 * @param date    строка даты
 * @param format1 форматы даты из которой нужно получить дату
 * @param format2 форматы даты в который нужно перевести дату
 * @param storage хранение значение времени
 * @return        результат работы
 */
string awh::Chrono::strip(const string & date, const string & format1, const string & format2, const storage_t storage) const noexcept {
	// Если данные переданы
	if(!date.empty() && !format1.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем парсинг даты
			const uint64_t stamp = const_cast <chrono_t *> (this)->parse(date, format1, storage);
			// Если штамп времени получен
			if(stamp > 0)
				// Выполняем формирование формата даты и времени
				return this->format(stamp, this->getTimeZone(storage), format2);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "%s\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return "";
}
/**
 * Chrono Конструктор
 * @param fmk объект фреймворка
 */
awh::Chrono::Chrono(const fmk_t * fmk) noexcept : _fmk(fmk) {
	// Выполняем инициализацию локального объекта даты и времени
	this->clear();
	// Выполняем компиляцию регулярных выражений
	this->compile("\\d+", format_t::s);
	this->compile("\\w+", format_t::Z);
	this->compile("\\d{3}", format_t::j);
	this->compile("\\d{1}", format_t::u);
	this->compile("\\d{4}", format_t::Y);
	this->compile("\\d{1,2}", format_t::y);
	this->compile("[A-Za-z]{2}", format_t::p);
	this->compile("[A-Z][a-z]{2}", format_t::a);
	this->compile("[A-Z][a-z]{2,}", format_t::A);
	this->compile("(\\d{1,2})\\:(\\d{1,2})", format_t::R);
	this->compile("([\\d\\.\\,]+)\\s*(s|m|h|d|w|M|y)$", format_t::S);
	this->compile("(\\d{1,2})\\/(\\d{1,2})\\/(\\d{2})", format_t::D);
	this->compile("(\\d{4})\\-(\\d{1,2})\\-(\\d{1,2})", format_t::F);
	this->compile("(\\d{1,2})\\:(\\d{1,2})\\:(\\d{1,2})", format_t::T);
	this->compile("(\\+|\\-)((\\d{1,2})\\:(\\d{1,2})|(\\d{1,4}))", format_t::z);
	this->compile("(\\w+)?((\\+|\\-)((\\d{1,2})\\:(\\d{1,2})|\\d{1,4}))?", format_t::e);
	this->compile("(\\d{1,2})\\:(\\d{1,2})\\:(\\d{1,2})\\s+([A-Za-z]{2})", format_t::r);
	this->compile("([A-Z][a-z]{2})\\s+([A-Z][a-z]{2})\\s+(\\d{1,2})\\s+(\\d{1,2})\\:(\\d{1,2})\\:(\\d{1,2})\\s+(\\d{4})", format_t::c);
}
/**
 * ~Chrono Деструктор
 */
awh::Chrono::~Chrono() noexcept {
	// Выполняем перебор всего списка скомпилированных регулярных выражений
	for(auto i = this->_expressions.begin(); i != this->_expressions.end();){
		// Выполняем удаление выделенной памяти
		::pcre2_regfree(&i->second);
		// Удаляем регулярное выражение
		i = this->_expressions.erase(i);
	}
}
