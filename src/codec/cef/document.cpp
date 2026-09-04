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
	 * @brief Метод постановки отменяющей записи в звене пути
	 *
	 * @details Звено пути отменяющей записи требует по RFC 6901: знак «~» пишется «~0»,
	 *          а косая черта - «~1». Без того имя ключа, косую черту несущее, разрывало
	 *          бы путь на два звена, и обход переставал бы быть замкнутым
	 *
	 * @param name имя, звеном пути ставимое
	 * @return     звено пути с поставленной отменяющей записью
	 */
	string pointer(const string_view name) noexcept {
		// Результирующее звено пути
		string result = "";
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
				// Если знак является знаком отменяющей записи
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
	// Получаем путь к паре расширения в дереве события
	const string path("/extension/" + pointer(key));
	// Если ключ расширения деревом уже объявлен
	if(this->_root.at("/extension").contains(key)){
		// Получаем значение, ключом уже объявленное
		abc::value_t & exists = this->_root.place(path);
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
	this->_root.place(path) = ::std::move(current);
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
	// Выполняем сборку записи CEF из дерева события
	const_cast <writer_t &> (this->_writer).write(this->_root, result);
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
	// Выводим значение дерева события по пути
	return this->_root.at(path);
}

/**
 * @brief Метод постановки значения дерева по пути
 *
 * @param path  путь к значению
 * @param value значение, по пути ставимое
 * @return      признак успешности постановки значения
 */
bool awh::codec::cef::Document::set(const string & path, const abc::value_t & value) noexcept {
	// Ставим значение в дерево события по пути
	this->_root.place(path) = value;
	// Выводим признак успешности постановки значения
	return this->_root.at(path).valid();
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
	// Замещаем значение пустой последовательностью знаков
	this->_root.place(path) = abc::value_t(string(""));
	// Выводим положительный признак сброса значения
	return true;
}

/**
 * @brief Метод сноса значения дерева по пути
 *
 * @param path путь к сносимому значению
 * @return     признак успешности сноса значения
 */
bool awh::codec::cef::Document::erase(const string & path) noexcept {
	// Выполняем поиск последнего звена пути
	const size_t pos = path.rfind('/');
	// Если путь звеньев не содержит
	if(pos == string::npos){
		// Запоминаем код ошибки отсутствия поля
		this->_error = error_t::UNKNOWN_FIELD;
		// Выводим отрицательный признак сноса значения
		return false;
	}
	// Получаем путь к вместилищу сносимого значения
	const string parent(path, 0, pos);
	// Получаем имя сносимого значения
	const string name(path, pos + 1, string::npos);
	// Получаем вместилище сносимого значения
	abc::value_t & value = this->_root.place(parent.empty() ? "/" : parent);
	// Если вместилище перечнем является
	if(value.type() == abc::type_t::ARRAY)
		// Выводим признак сноса значения перечня по номеру
		return value.erase(static_cast <size_t> (::std::stoull(name)));
	// Выводим признак сноса значения отображения по имени
	return value.erase(name);
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
	const abc::value_t & value = this->_root.at(path);
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
				result.push_back(pointer(value.key(i).text()));
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
	return this->_root.at(path).valid();
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
	return this->_root.at(path).contains(name);
}

/**
 * @brief Метод получения количества пар расширения события
 *
 * @return количество пар расширения события
 */
size_t awh::codec::cef::Document::size() const noexcept {
	// Выводим количество пар расширения события
	return this->_root.at("/extension").size();
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
	// Если полное имя ключа словарю неизвестно
	if(entry == nullptr)
		// Выводим значение расширения по имени как оно есть
		return this->_root.at("/extension/" + pointer(name));
	// Выводим значение расширения по ключу, словарём заданному
	return this->_root.at("/extension/" + pointer(entry->key));
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
	const abc::value_t & value = this->_root.at("/extension/" + pointer(name));
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
 _reader(fmk, log), _writer(fmk, log), _net(fmk, log), _error(error_t::NONE), _fmk(fmk), _log(log) {}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
