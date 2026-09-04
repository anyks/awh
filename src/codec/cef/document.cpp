/**
 * @file document.cpp
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
 * @brief Реализация события CEF, удерживаемого целиком — укладки разбора в дерево контейнера ABC,
 *        обхода дерева по пути, розыска значений по полному имени словаря и сведения меток записи
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <fstream>
#include <iterator>
#include <vector>
#include <codec/cef/document.hpp>

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже
 */
#include <sys/macro/suppress.hpp>

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
	 * Пространство имён контейнера CEF
	 */
	using namespace awh::codec::cef;

	/**
	 * @brief Имена полей заголовка записи в дереве контейнера ABC
	 *
	 * @details Порядок отвечает порядку полей в записи и порядку членов перечня полей
	 */
	constexpr string_view FIELDS[] = {
		"version", "vendor", "product", "release", "signature", "name", "severity"
	};

	/**
	 * @brief Имя вместилища пар расширения в дереве события
	 *
	 * @details Обращение к паре ведётся именем внутри вместилища, а НЕ строковым путём
	 *          «/extension/<ключ>»: ось дерева - контейнер ABC, а ход `at` его
	 *          отменяющей записи RFC 6901 не понимает вовсе. Замерено 04.09.2026:
	 *          имя «a~b» разыскивается дословно, «a~0b» не находит ничего, а имя,
	 *          косую черту несущее, строковым путём недостижимо в принципе - путь
	 *          рвётся на два звена. Ключи же живых журналов косую черту несут
	 *
	 * @note Ворошитель нашёл это разомкнутостью обхода: `keys` выдавал звено с
	 *       отменяющей записью, а `at` по нему не находил потомка
	 */
	constexpr const char * EXTENSION = "extension";

	/**
	 * @brief Метод снятия отменяющей записи со звена пути
	 *
	 * @details Звено пути несёт отменяющую запись RFC 6901: «~1» есть косая черта, а
	 *          «~0» - сам знак отмены. Порядок снятия обратен порядку постановки:
	 *          сперва «~1», затем «~0», - иначе запись «~01» обращалась бы в косую
	 *          черту вместо «~1»
	 *
	 * @param link звено пути с отменяющей записью
	 * @return     имя, звеном пути обозначенное
	 */
	string decode(const string_view link) noexcept {
		// Результирующее имя, звеном обозначенное
		string result;
		// Выделяем память под результирующее имя
		result.reserve(link.size());
		/**
		 * Выполняем перебор всех знаков звена пути
		 */
		for(size_t i = 0; i < link.size(); i++){
			// Если знак отменяющей записью не является
			if((link[i] != '~') || ((i + 1) >= link.size())){
				// Добавляем знак в результирующее имя как есть
				result.append(1, link[i]);
				// Переходим к следующему знаку
				continue;
			}
			/**
			 * Определяем знак, отменяющей записью обозначенный
			 */
			switch(link[i + 1]){
				// Если отменяющей записью обозначена косая черта
				case '1': {
					// Добавляем косую черту в результирующее имя
					result.append(1, '/');
					// Пропускаем знак отменяющей записи
					i++;
				} break;
				// Если отменяющей записью обозначен сам знак отмены
				case '0': {
					// Добавляем знак отмены в результирующее имя
					result.append(1, '~');
					// Пропускаем знак отменяющей записи
					i++;
				} break;
				// Если запись отменяющей не является
				default: result.append(1, link[i]);
			}
		}
		// Выводим результирующее имя
		return result;
	}

	/**
	 * @brief Метод постановки отменяющей записи в звене пути
	 *
	 * @param name имя, звеном пути ставимое
	 * @return     звено пути с поставленной отменяющей записью
	 */
	string encode(const string_view name) noexcept {
		// Результирующее звено пути
		string result;
		// Выделяем память под результирующее звено пути
		result.reserve(name.size());
		/**
		 * Выполняем перебор всех знаков имени
		 */
		for(size_t i = 0; i < name.size(); i++){
			/**
			 * Определяем знак имени
			 */
			switch(name[i]){
				// Если знак является знаком отмены
				case '~': result.append("~0"); break;
				// Если знак является косой чертой
				case '/': result.append("~1"); break;
				// Если знак отменяющей записи не требует
				default: result.append(1, name[i]);
			}
		}
		// Выводим результирующее звено пути
		return result;
	}

	/**
	 * @brief Метод разбора пути на звенья
	 *
	 * @details Путь разбирается САМИМ кодеком, а не осью его - контейнером ABC: ход
	 *          `at` контейнера отменяющей записи RFC 6901 не понимает вовсе, и имя
	 *          ключа, косую черту несущее, было бы им недостижимо. Ключи же живых
	 *          журналов косую черту несут, и терять их нельзя
	 *
	 * @note Найдено ворошителем 04.09.2026: обход размыкался - `keys` выдавал звено,
	 *       а `at` по нему потомка не находил
	 *
	 * @param path разбираемый путь
	 * @return     звенья пути со снятой отменяющей записью
	 */
	vector <string> split(const string & path) noexcept {
		// Результирующие звенья пути
		vector <string> result;
		// Если путь пуст либо корнем является
		if(path.empty() || (path == "/"))
			// Выводим отсутствие звеньев пути
			return result;
		// Смещение начала очередного звена пути
		size_t begin = ((path.front() == '/') ? 1 : 0);
		/**
		 * Выполняем разбор пути на звенья
		 */
		while(begin <= path.size()){
			// Выполняем поиск конца очередного звена пути
			const size_t end = path.find('/', begin);
			// Получаем очередное звено пути
			const string_view link(path.data() + begin, ((end == string::npos) ? path.size() : end) - begin);
			// Добавляем звено пути со снятой отменяющей записью
			result.push_back(decode(link));
			// Если звено пути последним является
			if(end == string::npos)
				// Выходим из цикла разбора пути
				break;
			// Сдвигаем смещение начала очередного звена пути
			begin = (end + 1);
		}
		// Выводим результирующие звенья пути
		return result;
	}
}

/**
 * @brief Метод обращения значения расширения в значение дерева
 *
 * @param entry  запись словаря расширений либо ничто
 * @param value  значение пары расширения знаками
 * @param result значение дерева контейнера ABC
 * @return       признак успешности обращения значения
 */
bool awh::codec::cef::Document::convert(const entry_t * entry, const string & value, abc::value_t & result) noexcept {
	// Получаем строгость сличения ключей расширения со словарём
	const mode_t mode = this->_reader.settings().mode;
	// Если значение пары расширения пусто
	if(value.empty()){
		/**
		 * Определяем обращение с пустым значением расширения
		 */
		switch(static_cast <uint8_t> (this->_reader.settings().empty)){
			// Если пустое значение обращается в логическую истину
			case static_cast <uint8_t> (empty_t::BOOLEAN): {
				// Устанавливаем логическую истину значением дерева
				result = abc::value_t(true);
				// Выводим положительный признак обращения значения
				return true;
			}
			// Если пустое значение обращается в пустоту
			case static_cast <uint8_t> (empty_t::NUL): {
				// Устанавливаем пустоту значением дерева
				result = abc::value_t(nullptr);
				// Выводим положительный признак обращения значения
				return true;
			}
			// Если пустое значение остаётся пустой последовательностью знаков
			default: {
				// Устанавливаем пустую последовательность знаков значением дерева
				result = abc::value_t(value);
				// Выводим положительный признак обращения значения
				return true;
			}
		}
	}
	// Если вид значения словарём не задан либо сличение видов не ведётся
	if((entry == nullptr) || (mode == mode_t::NONE) || (mode == mode_t::LOW)){
		// Устанавливаем последовательность знаков значением дерева
		result = abc::value_t(value);
		// Выводим положительный признак обращения значения
		return true;
	}
	/**
	 * Определяем вид значения, словарём заданный
	 */
	switch(static_cast <uint8_t> (entry->type)){
		// Если значение является целым числом
		case static_cast <uint8_t> (type_t::INTEGER):
		// Если значение является целым числом без знака
		case static_cast <uint8_t> (type_t::UNSIGNED): {
			// Если значение целым числом не является
			if(!this->_fmk->is(value, fmk_t::check_t::NUMBER)){
				// Устанавливаем код ошибки несоответствия значения виду
				this->_error = error_t::INVALID_NUMBER;
				// Выводим отрицательный признак обращения значения
				return false;
			}
			// Устанавливаем целое число значением дерева
			result = abc::value_t(static_cast <int64_t> (::std::stoll(value)));
			// Выводим положительный признак обращения значения
			return true;
		}
		// Если значение является дробным числом
		case static_cast <uint8_t> (type_t::DOUBLE): {
			// Если значение числом не является
			if(!this->_fmk->is(value, fmk_t::check_t::NUMBER) && !this->_fmk->is(value, fmk_t::check_t::DECIMAL)){
				// Устанавливаем код ошибки несоответствия значения виду
				this->_error = error_t::INVALID_NUMBER;
				// Выводим отрицательный признак обращения значения
				return false;
			}
			// Устанавливаем дробное число значением дерева
			result = abc::value_t(::std::stod(value));
			// Выводим положительный признак обращения значения
			return true;
		}
		// Если значение является логическим
		case static_cast <uint8_t> (type_t::BOOLEAN): {
			// Устанавливаем логическое значение значением дерева
			result = abc::value_t(this->_fmk->compare(value, "true") || this->_fmk->compare(value, "yes"));
			// Выводим положительный признак обращения значения
			return true;
		}
		// Если значение является меткой времени
		case static_cast <uint8_t> (type_t::TIMESTAMP): {
			// Если сличение всех видов значений не ведётся
			if(mode != mode_t::STRONG){
				// Устанавливаем последовательность знаков значением дерева
				result = abc::value_t(value);
				// Выводим положительный признак обращения значения
				return true;
			}
			// Признак числовой записи метки времени
			bool numeric = true;
			/**
			 * Выполняем перебор всех знаков метки времени
			 */
			for(size_t i = 0; i < value.size(); i++){
				// Если знак метки времени цифрой не является
				if((value[i] < '0') || (value[i] > '9')){
					// Запоминаем отсутствие числовой записи метки времени
					numeric = false;
					// Выходим из цикла перебора
					break;
				}
			}
			// Если метка времени записана числом
			if(numeric){
				// Устанавливаем штамп времени значением дерева
				result = abc::value_t(static_cast <uint64_t> (::std::stoull(value)));
				// Выводим положительный признак обращения значения
				return true;
			}
			/**
			 * Выполняем разбор записи метки времени модулем работы с датой и временем
			 *
			 * @warning Признака успешности разбор этот не выдаёт: ход `parse` с доводом
			 * `valid` у модуля `chrono_t` закрыт, а открытый его вид об отказе молчит.
			 * Запись, в которой не нашлось ни одного поля формата, выдаётся им ТЕКУЩИМ
			 * МОМЕНТОМ, и отличить её от разобранной кодеку нечем. Оттого отказом здесь
			 * считается один лишь нулевой штамп - пустая запись либо пустая запись
			 * формата, - а испорченная метка недоверенного журнала обращается в момент
			 * разбора молча. Ограничение это лежит НЕ у кодека, и снимается оно
			 * открытием признака успешности у самого модуля
			 */
			// Признак того, что метка времени долю секунды несёт
			bool fractional = false;
			/**
			 * Выполняем перебор знаков метки времени в поисках доли секунды
			 *
			 * @details Доля секунды опознаётся точкой, цифрами с обеих сторон окружённой:
			 * запись «23:30:15.734» её несёт, а «2.4.3-371989» - нет
			 */
			for(size_t i = 1; (i + 1) < value.size(); i++){
				// Если знак точкой является, а соседи его цифрами
				if((value[i] == '.') &&
				   (value[i - 1] >= '0') && (value[i - 1] <= '9') &&
				   (value[i + 1] >= '0') && (value[i + 1] <= '9')){
					// Запоминаем наличие доли секунды у метки времени
					fractional = true;
					// Выходим из цикла перебора
					break;
				}
			}
			// Получаем запись даты, метке времени отвечающую
			const string & pattern = (fractional ? this->_reader.settings().fraction : this->_reader.settings().timestamp);
			const uint64_t stamp = this->_chrono.parse(value, pattern, chrono_t::storage_t::GLOBAL);
			// Если разбор записи метки времени отказом завершился
			if(stamp == 0){
				// Устанавливаем код ошибки несоответствия значения виду
				this->_error = error_t::INVALID_TIMESTAMP;
				// Выводим отрицательный признак обращения значения
				return false;
			}
			// Устанавливаем штамп времени значением дерева
			result = abc::value_t(stamp);
			// Выводим положительный признак обращения значения
			return true;
		}
		// Если значение является адресом устройства сети
		case static_cast <uint8_t> (type_t::MAC):
		// Если значение является адресом IPv4
		case static_cast <uint8_t> (type_t::IPV4):
		// Если значение является адресом IPv6
		case static_cast <uint8_t> (type_t::IPV6):
		// Если значение является адресом сети любого из двух видов
		case static_cast <uint8_t> (type_t::ADDRESS): {
			// Если сличение всех видов значений не ведётся
			if(mode != mode_t::STRONG){
				// Устанавливаем последовательность знаков значением дерева
				result = abc::value_t(value);
				// Выводим положительный признак обращения значения
				return true;
			}
			// Выполняем определение вида адреса сети
			const net_addr_t::type_t type = this->_net.host(value);
			// Признак соответствия адреса виду, словарём заданному
			bool matched = false;
			/**
			 * Определяем вид значения, словарём заданный
			 */
			switch(static_cast <uint8_t> (entry->type)){
				// Если значение является адресом устройства сети
				case static_cast <uint8_t> (type_t::MAC):
					// Запоминаем соответствие адреса виду
					matched = (type == net_addr_t::type_t::MAC);
				break;
				// Если значение является адресом IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Запоминаем соответствие адреса виду
					matched = (type == net_addr_t::type_t::IPV4);
				break;
				// Если значение является адресом IPv6
				case static_cast <uint8_t> (type_t::IPV6):
					// Запоминаем соответствие адреса виду
					matched = (type == net_addr_t::type_t::IPV6);
				break;
				// Если значение является адресом сети любого из двух видов
				default: matched = ((type == net_addr_t::type_t::IPV4) || (type == net_addr_t::type_t::IPV6));
			}
			// Если адрес виду, словарём заданному, не отвечает
			if(!matched){
				// Устанавливаем код ошибки несоответствия значения адресом
				this->_error = error_t::INVALID_ADDRESS;
				// Выводим отрицательный признак обращения значения
				return false;
			}
			// Устанавливаем адрес сети значением дерева
			result = abc::value_t(value);
			// Выводим положительный признак обращения значения
			return true;
		}
	}
	// Устанавливаем последовательность знаков значением дерева
	result = abc::value_t(value);
	// Выводим положительный признак обращения значения
	return true;
}

/**
 * @brief Метод укладки пары расширения в дерево события
 *
 * @param key   имя ключа пары расширения
 * @param value значение пары расширения
 * @return      признак успешности укладки пары
 */
bool awh::codec::cef::Document::inject(const string & key, const string & value) noexcept {
	// Выполняем розыск записи словаря по ключу расширения
	const entry_t * entry = dictionary::find(key);
	// Если ключ расширения словарю неизвестен, а сличение ведётся строго
	if((entry == nullptr) && (this->_reader.settings().mode == mode_t::STRONG)){
		// Устанавливаем код ошибки неизвестного ключа расширения
		this->_error = error_t::UNKNOWN_KEY;
		// Выводим в лог сообщение о неизвестном ключе расширения
		this->_log->print("CEF extension key \"%s\" is unknown to the dictionary", log_t::flag_t::CRITICAL, key.c_str());
		// Выводим отрицательный признак укладки пары
		return false;
	}
	// Значение пары расширения деревом контейнера ABC
	abc::value_t current;
	// Если обращение значения расширения отказом завершилось
	if(!this->convert(entry, value, current)){
		// Выводим в лог сообщение о несоответствии значения виду
		this->_log->print(
			"CEF extension key \"%s\" holds a value of another kind: %s",
			log_t::flag_t::CRITICAL, key.c_str(), awh::codec::cef::message(this->_error)
		);
		// Выводим отрицательный признак укладки пары
		return false;
	}
	// Если обращение пустого значения велит пару пропустить
	if(value.empty() && (this->_reader.settings().empty == empty_t::SKIP))
		// Выводим положительный признак укладки пары
		return true;
	// Получаем вместилище пар расширения дерева события
	abc::value_t & extension = this->_root.place(string("/") + EXTENSION);
	// Если ключ расширения деревом уже объявлен
	if(extension.contains(key)){
		// Получаем значение, ключом уже объявленное
		abc::value_t & exists = extension[key];
		// Если объявленное значение перечнем не является
		if(exists.type() != abc::type_t::ARRAY){
			// Заводим перечень значений одного ключа
			abc::value_t list(abc::kind_t::ARRAY);
			// Добавляем объявленное ранее значение в перечень
			if(!list.push(exists))
				// Выводим отрицательный признак укладки пары
				return false;
			// Добавляем новое значение в перечень
			if(!list.push(current))
				// Выводим отрицательный признак укладки пары
				return false;
			// Ставим перечень значений на место объявленного значения
			exists = ::std::move(list);
			// Выводим положительный признак укладки пары
			return true;
		}
		// Выводим признак добавления значения в перечень
		return exists.push(current);
	}
	// Ставим значение пары расширения в дерево события
	extension[key] = ::std::move(current);
	// Выводим положительный признак укладки пары
	return true;
}

/**
 * @brief Метод разбора записи CEF
 *
 * @param text текст записи CEF
 * @return     результат выполнения операции
 */
bool awh::codec::cef::Document::parse(const string_view text) noexcept {
	// Выполняем очистку дерева события
	this->clear();
	// Выполняем сброс состояния чтения записей
	this->_reader.reset();
	// Если передача текста чтению отказом завершилась
	if(!this->_reader.feed(text)){
		// Запоминаем код ошибки последней операции
		this->_error = this->_reader.error();
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Заводим дерево события отображением
	this->_root = abc::value_t(abc::kind_t::MAP);
	/**
	 * Выполняем перебор событий разбора первой записи
	 */
	while(this->_reader.next()){
		/**
		 * Определяем вид события разбора
		 */
		switch(static_cast <uint8_t> (this->_reader.event())){
			// Если событием является приставка syslog
			case static_cast <uint8_t> (event_t::SYSLOG): {
				// Ставим приставку syslog в дерево события
				this->_root.place("/syslog") = abc::value_t(this->_reader.value());
			} break;
			// Если событием является поле заголовка записи
			case static_cast <uint8_t> (event_t::HEADER): {
				// Получаем имя поля заголовка записи
				const string name(FIELDS[static_cast <size_t> (this->_reader.field())]);
				// Получаем путь к полю заголовка в дереве события
				const string path("/header/" + name);
				// Если полем заголовка является номер редакции записи
				if(this->_reader.field() == field_t::VERSION)
					// Ставим номер редакции записи целым числом
					this->_root.place(path) = abc::value_t(static_cast <int64_t> (this->_reader.version()));
				// Если полем заголовка является важность события
				else if(this->_reader.field() == field_t::SEVERITY){
					// Если важность события числом записана
					if(this->_fmk->is(this->_reader.value(), fmk_t::check_t::NUMBER))
						// Ставим важность события целым числом
						this->_root.place(path) = abc::value_t(static_cast <int64_t> (this->_reader.severity()));
					// Если важность события словом записана
					else this->_root.place(path) = abc::value_t(this->_reader.value());
				// Если полем заголовка является поле знаками
				} else this->_root.place(path) = abc::value_t(this->_reader.value());
			} break;
			// Если событием является пара расширения
			case static_cast <uint8_t> (event_t::EXTENSION): {
				// Если укладка пары расширения отказом завершилась
				if(!this->inject(this->_reader.key(), this->_reader.value()))
					// Выводим отрицательный результат выполнения операции
					return false;
			} break;
			// Если событием является окончание записи
			case static_cast <uint8_t> (event_t::RECORD): {
				// Выводим положительный результат выполнения операции
				return true;
			}
		}
	}
	// Если разбор записи прекращён ошибкой
	if(this->_reader.state() == state_t::FAILED){
		// Запоминаем код ошибки последней операции
		this->_error = this->_reader.error();
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выводим положительный результат выполнения операции
	return true;
}

/**
 * @brief Метод чтения записи CEF из файла
 *
 * @param filename адрес файла записи CEF
 * @return         результат выполнения операции
 */
bool awh::codec::cef::Document::load(const string & filename) noexcept {
	// Выполняем открытие файла записи CEF
	::std::ifstream file(filename, ::std::ios::binary);
	// Если файл записи CEF открыть не удалось
	if(!file.is_open()){
		// Запоминаем код ошибки открытия файла
		this->_error = error_t::FILE_NOT_OPENED;
		// Выводим в лог сообщение об ошибке открытия файла
		this->_log->print("CEF file \"%s\" could not be opened", log_t::flag_t::CRITICAL, filename.c_str());
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем чтение содержимого файла записи CEF
	const string content((::std::istreambuf_iterator <char> (file)), ::std::istreambuf_iterator <char> ());
	// Выполняем закрытие файла записи CEF
	file.close();
	// Выводим результат разбора содержимого файла записи CEF
	return this->parse(content);
}

/**
 * @brief Метод записи события в файл
 *
 * @param filename адрес файла записи CEF
 * @return         результат выполнения операции
 */
bool awh::codec::cef::Document::save(const string & filename) const noexcept {
	// Выполняем сбор записи CEF из дерева события
	const string & content = this->dump();
	// Если сбор записи CEF отказом завершился
	if(content.empty())
		// Выводим отрицательный результат выполнения операции
		return false;
	// Выполняем открытие файла записи CEF
	::std::ofstream file(filename, ::std::ios::binary);
	// Если файл записи CEF открыть не удалось
	if(!file.is_open()){
		// Выводим в лог сообщение об ошибке открытия файла
		this->_log->print("CEF file \"%s\" could not be written", log_t::flag_t::CRITICAL, filename.c_str());
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	// Выполняем запись собранной записи CEF в файл
	file.write(content.data(), static_cast <::std::streamsize> (content.size()));
	// Выполняем закрытие файла записи CEF
	file.close();
	// Выводим положительный результат выполнения операции
	return true;
}

/**
 * @brief Метод сбора записи CEF из дерева события
 *
 * @return собранная запись CEF
 */
string awh::codec::cef::Document::dump() const noexcept {
	// Собранная запись CEF
	string result = "";
	/**
	 * Если сборка записи CEF отказом завершилась
	 *
	 * @details Итог сборки проверяется НЕПРЕМЕННО: писатель наполняет запись по ходу
	 * обхода дерева, и на отказе в ней остаётся собранное до места отказа. Выдача
	 * такого обрубка отдавала бы потребителю запись, которая разбирается, но несёт
	 * лишь часть события - и молча
	 *
	 * @note Найдено ворошителем 04.09.2026: запись с непредставимым ключом выдавалась
	 *       одним заголовком, а расширение пропадало без всякого знака о том
	 */
	if(!const_cast <writer_t &> (this->_writer).write(this->_root, result)){
		// Запоминаем код ошибки сборки записи
		const_cast <Document *> (this)->_error = this->_writer.error();
		// Выводим пустую запись CEF
		return string("");
	}
	// Выводим собранную запись CEF
	return result;
}

/**
 * @brief Метод извлечения значения дерева по пути
 *
 * @param path путь к значению
 * @return     ссылка на значение либо ссылка на отсутствующее значение
 */
const awh::codec::abc::value_t & awh::codec::cef::Document::at(const string & path) const noexcept {
	// Получаем звенья разбираемого пути
	const vector <string> links = split(path);
	// Текущее значение обхода дерева события
	const abc::value_t * result = &this->_root;
	/**
	 * Выполняем обход дерева события по звеньям пути
	 */
	for(const auto & link : links){
		/**
		 * Определяем вид текущего значения обхода
		 */
		switch(static_cast <uint32_t> (result->type())){
			// Если значение является отображением
			case static_cast <uint32_t> (abc::type_t::MAP):
				// Переходим к полю отображения по имени
				result = &(*result)[link];
			break;
			// Если значение является перечнем
			case static_cast <uint32_t> (abc::type_t::ARRAY): {
				// Номер значения перечня, звеном обозначенный
				size_t index = 0;
				// Признак числового вида звена пути
				bool numeric = !link.empty();
				/**
				 * Выполняем перебор всех знаков звена пути
				 */
				for(size_t i = 0; numeric && (i < link.size()); i++){
					// Если знак звена цифрой не является
					if((link[i] < '0') || (link[i] > '9'))
						// Запоминаем отсутствие числового вида звена
						numeric = false;
					// Если знак звена цифрой является
					else index = ((index * 10) + static_cast <size_t> (link[i] - '0'));
				}
				// Если звено пути числовым не является
				if(!numeric)
					// Выводим отсутствующее значение дерева
					return abc::value_t::scrap();
				// Переходим к значению перечня по номеру
				result = &(*result)[index];
			} break;
			// Если значение вместилищем не является
			default: return abc::value_t::scrap();
		}
	}
	// Выводим значение дерева события по пути
	return * result;
}

/**
 * @brief Метод постановки значения дерева по пути
 *
 * @param path  путь к значению
 * @param value значение, по пути ставимое
 * @return      признак успешности постановки значения
 */
bool awh::codec::cef::Document::set(const string & path, const abc::value_t & value) noexcept {
	// Получаем звенья разбираемого пути
	const vector <string> links = split(path);
	// Если путь звеньев не содержит
	if(links.empty()){
		// Запоминаем код ошибки отсутствия поля
		this->_error = error_t::UNKNOWN_FIELD;
		// Выводим отрицательный признак постановки значения
		return false;
	}
	// Текущее значение обхода дерева события
	abc::value_t * current = &this->_root;
	/**
	 * Выполняем обход дерева события по звеньям пути, кроме последнего
	 */
	for(size_t i = 0; (i + 1) < links.size(); i++){
		// Если вместилище звена пути отображением не является
		if(current->type() != abc::type_t::MAP)
			// Заводим вместилище звена пути отображением
			(* current) = abc::value_t(abc::kind_t::MAP);
		// Переходим к полю отображения по имени
		current = &(*current)[links.at(i)];
	}
	// Если вместилище последнего звена отображением не является
	if(current->type() != abc::type_t::MAP)
		// Заводим вместилище последнего звена отображением
		(* current) = abc::value_t(abc::kind_t::MAP);
	// Ставим значение в дерево события по последнему звену пути
	(* current)[links.back()] = value;
	// Выводим признак успешности постановки значения
	return (* current)[links.back()].valid();
}

/**
 * @brief Метод сброса значения дерева по пути
 *
 * @param path путь к сбрасываемому значению
 * @return     признак успешности сброса значения
 */
bool awh::codec::cef::Document::reset(const string & path) noexcept {
	// Если значение по пути деревом не объявлено
	if(!this->has(path)){
		// Запоминаем код ошибки отсутствия поля
		this->_error = error_t::UNKNOWN_FIELD;
		// Выводим отрицательный признак сброса значения
		return false;
	}
	// Выводим признак замещения значения пустой последовательностью знаков
	return this->set(path, abc::value_t(string("")));
}

/**
 * @brief Метод сноса значения дерева по пути
 *
 * @param path путь к сносимому значению
 * @return     признак успешности сноса значения
 */
bool awh::codec::cef::Document::erase(const string & path) noexcept {
	// Получаем звенья разбираемого пути
	const vector <string> links = split(path);
	// Если путь звеньев не содержит
	if(links.empty()){
		// Запоминаем код ошибки отсутствия поля
		this->_error = error_t::UNKNOWN_FIELD;
		// Выводим отрицательный признак сноса значения
		return false;
	}
	// Текущее значение обхода дерева события
	abc::value_t * current = &this->_root;
	/**
	 * Выполняем обход дерева события по звеньям пути, кроме последнего
	 */
	for(size_t i = 0; (i + 1) < links.size(); i++){
		// Если вместилище звена пути отображением не является
		if(current->type() != abc::type_t::MAP){
			// Запоминаем код ошибки отсутствия поля
			this->_error = error_t::UNKNOWN_FIELD;
			// Выводим отрицательный признак сноса значения
			return false;
		}
		// Переходим к полю отображения по имени
		current = &(*current)[links.at(i)];
	}
	// Если вместилище сносимого значения перечнем является
	if(current->type() == abc::type_t::ARRAY){
		// Номер сносимого значения перечня
		size_t index = 0;
		// Признак числового вида последнего звена пути
		bool numeric = !links.back().empty();
		/**
		 * Выполняем перебор всех знаков последнего звена пути
		 */
		for(size_t i = 0; numeric && (i < links.back().size()); i++){
			// Если знак звена цифрой не является
			if((links.back()[i] < '0') || (links.back()[i] > '9'))
				// Запоминаем отсутствие числового вида звена
				numeric = false;
			// Если знак звена цифрой является
			else index = ((index * 10) + static_cast <size_t> (links.back()[i] - '0'));
		}
		// Если последнее звено пути числовым не является
		if(!numeric){
			// Запоминаем код ошибки отсутствия поля
			this->_error = error_t::UNKNOWN_FIELD;
			// Выводим отрицательный признак сноса значения
			return false;
		}
		// Выводим признак сноса значения перечня по номеру
		return current->erase(index);
	}
	// Выводим признак сноса значения отображения по имени
	return current->erase(links.back());
}

/**
 * @brief Метод извлечения звеньев пути, у значения объявленных
 *
 * @param path путь к значению
 * @return     звенья пути, у значения объявленные
 */
vector <string> awh::codec::cef::Document::keys(const string & path) const noexcept {
	// Результирующий перечень звеньев пути
	vector <string> result;
	// Получаем значение дерева события по пути
	const abc::value_t & value = this->at(path);
	/**
	 * Определяем вид значения дерева события
	 */
	switch(static_cast <uint32_t> (value.type())){
		// Если значение является отображением
		case static_cast <uint32_t> (abc::type_t::MAP): {
			// Выделяем память под звенья пути
			result.reserve(value.size());
			/**
			 * Выполняем перебор всех полей отображения
			 */
			for(size_t i = 0; i < value.size(); i++)
				// Добавляем имя поля отображения звеном пути
				result.push_back(encode(value.key(i).text()));
		} break;
		// Если значение является перечнем
		case static_cast <uint32_t> (abc::type_t::ARRAY): {
			// Выделяем память под звенья пути
			result.reserve(value.size());
			/**
			 * Выполняем перебор всех значений перечня
			 */
			for(size_t i = 0; i < value.size(); i++)
				// Добавляем номер значения перечня звеном пути
				result.push_back(::std::to_string(i));
		} break;
	}
	// Выводим результирующий перечень звеньев пути
	return result;
}

/**
 * @brief Метод проверки наличия значения по пути
 *
 * @param path путь к значению
 * @return     признак наличия значения по пути
 */
bool awh::codec::cef::Document::has(const string & path) const noexcept {
	// Выводим признак наличия значения дерева события по пути
	return this->at(path).valid();
}

/**
 * @brief Метод проверки наличия вложенного значения по имени
 *
 * @param path путь к значению
 * @param name имя вложенного значения
 * @return     признак наличия вложенного значения
 */
bool awh::codec::cef::Document::contains(const string & path, const string & name) const noexcept {
	// Выводим признак наличия вложенного значения по имени
	return this->at(path).contains(name);
}

/**
 * @brief Метод получения количества пар расширения события
 *
 * @return количество пар расширения события
 */
size_t awh::codec::cef::Document::size() const noexcept {
	// Выводим количество пар расширения события
	return this->_root.at(string("/") + EXTENSION).size();
}

/**
 * @brief Метод очистки дерева события
 *
 */
void awh::codec::cef::Document::clear() noexcept {
	// Выполняем очистку дерева события
	this->_root.clear();
	// Сбрасываем код ошибки последней операции
	this->_error = error_t::NONE;
}

/**
 * @brief Метод извлечения значения расширения по ПОЛНОМУ имени ключа
 *
 * @param name полное имя ключа расширения
 * @return     ссылка на значение либо ссылка на отсутствующее значение
 */
const awh::codec::abc::value_t & awh::codec::cef::Document::field(const string & name) const noexcept {
	// Выполняем розыск записи словаря по полному имени ключа
	const entry_t * entry = dictionary::search(name);
	// Получаем вместилище пар расширения дерева события
	const abc::value_t & extension = this->_root.at(string("/") + EXTENSION);
	// Если полное имя ключа словарю неизвестно
	if(entry == nullptr)
		// Выводим значение расширения по имени как оно есть
		return extension[name];
	// Выводим значение расширения по ключу, словарём заданному
	return extension[string(entry->key)];
}

/**
 * @brief Метод получения человеческого имени ключа расширения
 *
 * @param key имя ключа расширения, в записи стоящее
 * @return    человеческое имя ключа расширения
 */
string awh::codec::cef::Document::label(const string & key) const noexcept {
	// Получаем имя ключа, метку имени несущего
	const string name(key + string(LABEL_SUFFIX));
	// Получаем значение метки имени ключа
	const abc::value_t & value = this->_root.at(string("/") + EXTENSION)[name];
	// Если метка имени ключа записью объявлена
	if(value.valid() && (value.type() == abc::type_t::STRING))
		// Выводим человеческое имя ключа, меткой записи заданное
		return value.text();
	// Выполняем розыск записи словаря по ключу расширения
	const entry_t * entry = dictionary::find(key);
	// Выводим человеческое имя ключа, словарём заданное
	return (entry != nullptr ? string(entry->name) : string(""));
}

/**
 * @brief Метод получения метки времени записью заданного вида
 *
 * @param key    имя ключа расширения, метку времени несущего
 * @param format запись даты, выдаваемой метке назначаемая
 * @return       метка времени записью заданного вида
 */
string awh::codec::cef::Document::timestamp(const string & key, const string & format) const noexcept {
	// Получаем значение пары расширения, метку времени несущей
	const abc::value_t & value = this->_root.at(string("/") + EXTENSION)[key];
	// Если значение пары расширения деревом не объявлено
	if(!value.valid())
		// Выводим пустую метку времени
		return "";
	// Если значение пары расширения последовательностью знаков является
	if(value.type() == abc::type_t::STRING)
		// Выводим метку времени как она в записи стоит: разбор её не вёлся
		return value.text();
	// Штамп времени, деревом удерживаемый
	uint64_t stamp = 0;
	// Если извлечение штампа времени отказом завершилось
	if(!value.value(stamp))
		// Выводим пустую метку времени
		return "";
	// Выводим метку времени записью заданного вида
	return this->_chrono.format(stamp, format);
}

/**
 * @brief Метод получения вида значения ключа расширения
 *
 * @param key имя ключа расширения, в записи стоящее
 * @return    вид значения, словарём заданный
 */
awh::codec::cef::type_t awh::codec::cef::Document::type(const string & key) const noexcept {
	// Выполняем розыск записи словаря по ключу расширения
	const entry_t * entry = dictionary::find(key);
	// Выводим вид значения, словарём заданный
	return (entry != nullptr ? entry->type : type_t::NONE);
}

/**
 * @brief Метод получения дерева события целиком
 *
 * @return дерево разобранного события контейнером ABC
 */
const awh::codec::abc::value_t & awh::codec::cef::Document::root() const noexcept {
	// Выводим дерево разобранного события
	return this->_root;
}

/**
 * @brief Метод получения кода ошибки последней операции
 *
 * @return код ошибки последней операции
 */
awh::codec::cef::error_t awh::codec::cef::Document::error() const noexcept {
	// Выводим код ошибки последней операции
	return this->_error;
}

/**
 * @brief Метод получения места обнаружения ошибки разбора
 *
 * @return положение обнаруженной ошибки в исходном тексте
 */
const awh::codec::cef::pos_t & awh::codec::cef::Document::errorPosition() const noexcept {
	// Выводим положение обнаруженной ошибки в исходном тексте
	return this->_reader.errorPosition();
}

/**
 * @brief Метод получения настроек разбора записей
 *
 * @return настройки разбора записей
 */
const awh::codec::cef::reader_t::settings_t & awh::codec::cef::Document::settings() const noexcept {
	// Выводим настройки разбора записей
	return this->_reader.settings();
}

/**
 * @brief Метод установки настроек разбора записей
 *
 * @param settings настройки разбора записей
 * @return         результат выполнения операции
 */
bool awh::codec::cef::Document::settings(const reader_t::settings_t & settings) noexcept {
	// Выполняем сброс состояния чтения записей
	this->_reader.reset();
	// Выводим результат установки настроек разбора записей
	return this->_reader.settings(settings);
}

/**
 * @brief Метод установки настроек записи событий
 *
 * @param settings настройки записи событий
 */
void awh::codec::cef::Document::settings(const writer_t::settings_t & settings) noexcept {
	// Устанавливаем настройки записи событий
	this->_writer.settings(settings);
}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::codec::cef::Document::Document(const fmk_t * fmk, const log_t * log) noexcept :
 _reader(fmk, log), _writer(fmk, log), _net(fmk, log), _chrono(fmk, log),
 _error(error_t::NONE), _fmk(fmk), _log(log) {}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
