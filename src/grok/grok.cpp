/**
 * @file: grok.cpp
 * @date: 2026-08-04
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Файл реализации модуля Grok —
 *        реестр именованных шаблонов, разворот ссылок вида «%{NAME:поле:вид}»
 *        в регулярное выражение и извлечение именованных полей из текста
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <grok/grok.hpp>
#include <lexical/lexical.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Наибольшая допустимая глубина разворота ссылок
 *
 * @details Глубина ограничена затем, чтобы разворот набора шаблонов, круговой
 *          ссылки лишённого, но выстроенного цепочкой чрезмерно длинной, не
 *          исчерпал стек исполнения. Набор встроенный укладывается в глубину
 *          девяти уровней, поэтому запас взят с избытком.
 *
 */
static constexpr uint16_t MAX_DEPTH = 64;

/**
 * @brief Наибольший допустимый размер развёрнутого текста выражения
 *
 * @details Разворот ссылок текстом умножает размер выражения, и шаблон,
 *          ссылающийся на один и тот же тяжёлый шаблон многократно, способен
 *          выдать текст объёмом, сборке неподъёмным. Наибольший шаблон набора
 *          встроенного, «NETSCREENSESSIONLOG», разворачивается в 9128 байт.
 *
 */
static constexpr size_t MAX_LENGTH = 0x400000;

/**
 * @brief Оператор вычисления хэша ключа кэша
 *
 * @param key ключ кэша собранных шаблонов Grok
 * @return    вычисленное значение хэша ключа
 *
 */
size_t awh::Grok::Hash::operator () (const key_t & key) const noexcept {
	// Получаем хэш набора режимов сборки регулярного выражения
	const size_t first = std::hash <uint32_t> ()(key.first);
	// Получаем хэш текста шаблона Grok
	const size_t second = std::hash <string> ()(key.second);
	// Выводим смешанное значение хэша ключа кэша
	return (first ^ (second + 0x9E3779B9 + (first << 6) + (first >> 2)));
}
/**
 * @brief Метод подсчёта групп захвата участка текста выражения
 *
 * @param text   участок текста регулярного выражения
 * @param number номер очередной группы захвата
 *
 * @details Номер группы захвата определяется порядком открывающих скобок в
 *          тексте выражения окончательном, поэтому подсчёт ведёт не только
 *          скобки, порождённые ссылками с полем, но и скобки, записанные в
 *          тексте шаблона впрямую: набор встроенный несёт и группы
 *          безымянные, и группы именованные.
 *
 */
void awh::Grok::account(string_view text, uint32_t & number) const noexcept {
	// Признак экранирования очередного символа
	bool escaped = false;
	// Признак нахождения внутри класса символов
	bool klass = false;
	/**
	 * Выполняем перебор символов участка текста выражения
	 */
	for(size_t i = 0; i < text.size(); i++) {
		/**
		 * Если очередной символ экранирован
		 */
		if(escaped) {
			// Снимаем признак экранирования
			escaped = false;
			// Переходим к следующему символу
			continue;
		}
		/**
		 * Если очередной символ является обратной косой чертой
		 */
		if(text[i] == '\\') {
			// Устанавливаем признак экранирования
			escaped = true;
			// Переходим к следующему символу
			continue;
		}
		/**
		 * Если разбор ведётся внутри класса символов
		 */
		if(klass) {
			/**
			 * Если класс символов закрыт
			 */
			if(text[i] == ']')
				// Снимаем признак нахождения внутри класса символов
				klass = false;
			// Переходим к следующему символу
			continue;
		}
		/**
		 * Если класс символов открыт
		 */
		if(text[i] == '[') {
			// Устанавливаем признак нахождения внутри класса символов
			klass = true;
			// Переходим к следующему символу
			continue;
		}
		/**
		 * Если очередной символ круглой скобкой не является
		 */
		if(text[i] != '(')
			// Переходим к следующему символу
			continue;
		/**
		 * Если скобка закрывает выражение
		 */
		if((i + 1) >= text.size()) {
			// Выполняем увеличение номера группы захвата
			number++;
			// Завершаем разбор участка текста выражения
			break;
		}
		/**
		 * Если скобка группу особую не открывает
		 */
		if(text[i + 1] != '?') {
			// Выполняем увеличение номера группы захвата
			number++;
			// Переходим к следующему символу
			continue;
		}
		/**
		 * Если группа особая записана в виде «(?P<...>»
		 *
		 * @details Захват выполняют лишь три вида групп особых: «(?<имя>»,
		 *          «(?'имя'» и «(?P<имя>». Записи «(?<=» и «(?<!» открывают
		 *          проверку ретроспективную и захвата не выполняют, поэтому
		 *          отличаются по символу, за скобкой угловой следующему.
		 */
		if(((i + 2) < text.size()) && (text[i + 2] == 'P') && ((i + 3) < text.size()) && (text[i + 3] == '<')) {
			// Выполняем увеличение номера группы захвата
			number++;
			// Переходим к следующему символу
			continue;
		}
		/**
		 * Если группа особая записана в виде «(?'имя'»
		 */
		if(((i + 2) < text.size()) && (text[i + 2] == '\'')) {
			// Выполняем увеличение номера группы захвата
			number++;
			// Переходим к следующему символу
			continue;
		}
		/**
		 * Если группа особая записана в виде «(?<имя>»
		 */
		if(((i + 2) < text.size()) && (text[i + 2] == '<') && ((i + 3) < text.size()) && (text[i + 3] != '=') && (text[i + 3] != '!'))
			// Выполняем увеличение номера группы захвата
			number++;
	}
}
/**
 * @brief Метод разворота ссылок текста шаблона
 *
 * @param body   текст шаблона со ссылками
 * @param result развёрнутый текст регулярного выражения
 * @param fields набор полей шаблона
 * @param stack  набор шаблонов, разворот каких не завершён
 * @param number номер очередной группы захвата
 * @param depth  действующая глубина разворота
 * @return       результат разворота ссылок текста шаблона
 *
 */
bool awh::Grok::expand(string_view body, string & result, vector <field_t> & fields, vector <string> & stack, uint32_t & number, const uint16_t depth) const noexcept {
	/**
	 * Если допустимая глубина разворота превышена
	 */
	if(depth >= MAX_DEPTH) {
		// Устанавливаем код ошибки разбора шаблона
		this->_error = error_t::NESTING_TOO_DEEP;
		// Выводим результат разворота ссылок текста шаблона
		return false;
	}
	// Позиция начала участка текста, ссылок не несущего
	size_t offset = 0;
	/**
	 * Выполняем перебор символов текста шаблона
	 */
	for(size_t i = 0; i < body.size(); i++) {
		/**
		 * Если очередной символ начало ссылки не открывает
		 */
		if((body[i] != '%') || ((i + 1) >= body.size()) || (body[i + 1] != '{'))
			// Переходим к следующему символу
			continue;
		/**
		 * Если открывающая скобка ссылки экранирована
		 *
		 * @details Знак процента экранируется обратной косой чертой, и
		 *          экранированный знак ссылки не открывает: текст «\%{» несёт
		 *          знак процента буквальный.
		 */
		if((i > 0) && (body[i - 1] == '\\'))
			// Переходим к следующему символу
			continue;
		// Выполняем поиск закрывающей скобки ссылки
		const size_t end = body.find('}', i + 2);
		/**
		 * Если закрывающая скобка ссылки не обнаружена
		 */
		if(end == string_view::npos) {
			// Устанавливаем код ошибки разбора шаблона
			this->_error = error_t::REFERENCE_UNCLOSED;
			// Выводим результат разворота ссылок текста шаблона
			return false;
		}
		// Выполняем перенос участка текста, ссылок не несущего
		result.append(body.substr(offset, (i - offset)));
		// Выполняем подсчёт групп захвата перенесённого участка текста
		this->account(body.substr(offset, (i - offset)), number);
		// Получаем текст ссылки на шаблон
		string_view reference = body.substr(i + 2, (end - i - 2));
		// Название поля ссылки
		string_view field;
		// Вид значения поля ссылки
		kind_t kind = kind_t::TEXT;
		// Выполняем поиск разделителя названия шаблона и названия поля
		const size_t colon = reference.find(':');
		/**
		 * Если ссылка несёт название поля
		 */
		if(colon != string_view::npos) {
			// Получаем название поля ссылки
			field = reference.substr(colon + 1);
			// Получаем название шаблона ссылки
			reference = reference.substr(0, colon);
			// Выполняем поиск разделителя названия поля и вида значения
			const size_t second = field.find(':');
			/**
			 * Если ссылка несёт вид значения поля
			 */
			if(second != string_view::npos) {
				// Получаем вид значения поля ссылки
				const string_view text = field.substr(second + 1);
				// Получаем название поля ссылки
				field = field.substr(0, second);
				/**
				 * Если вид значения поля прочитан числом целым
				 */
				if((text.compare("int") == 0) || (text.compare("integer") == 0) || (text.compare("long") == 0))
					// Устанавливаем вид значения поля числом целым
					kind = kind_t::INTEGER;
				/**
				 * Если вид значения поля прочитан числом дробным
				 */
				else if((text.compare("float") == 0) || (text.compare("double") == 0))
					// Устанавливаем вид значения поля числом дробным
					kind = kind_t::FLOATING;
				/**
				 * Если вид значения поля модулю неизвестен
				 */
				else {
					// Устанавливаем код ошибки разбора шаблона
					this->_error = error_t::KIND_UNKNOWN;
					// Выводим результат разворота ссылок текста шаблона
					return false;
				}
			}
			/**
			 * Если название поля ссылки пусто
			 */
			if(field.empty()) {
				// Устанавливаем код ошибки разбора шаблона
				this->_error = error_t::FIELD_EMPTY;
				// Выводим результат разворота ссылок текста шаблона
				return false;
			}
		}
		/**
		 * Если название шаблона ссылки пусто
		 */
		if(reference.empty()) {
			// Устанавливаем код ошибки разбора шаблона
			this->_error = error_t::REFERENCE_EMPTY;
			// Выводим результат разворота ссылок текста шаблона
			return false;
		}
		// Получаем название шаблона ссылки
		const string name(reference);
		// Выполняем поиск шаблона ссылки в реестре
		auto j = this->_patterns.find(name);
		/**
		 * Если шаблон ссылки реестру неизвестен
		 */
		if(j == this->_patterns.end()) {
			// Устанавливаем код ошибки разбора шаблона
			this->_error = error_t::REFERENCE_UNKNOWN;
			// Выводим результат разворота ссылок текста шаблона
			return false;
		}
		/**
		 * Выполняем перебор набора шаблонов, разворот каких не завершён
		 */
		for(const auto & item : stack) {
			/**
			 * Если шаблон ссылки развернуть уже пытались
			 */
			if(item.compare(name) == 0) {
				// Устанавливаем код ошибки разбора шаблона
				this->_error = error_t::REFERENCE_CIRCULAR;
				// Выводим результат разворота ссылок текста шаблона
				return false;
			}
		}
		/**
		 * Если ссылка несёт название поля
		 */
		if(!field.empty()) {
			// Создаём описание поля шаблона
			field_t record;
			// Устанавливаем вид значения поля
			record.kind = kind;
			// Устанавливаем название поля
			record.name.assign(field);
			// Устанавливаем номер группы захвата поля
			record.number = ++number;
			// Выполняем добавление описания поля в набор
			fields.push_back(::move(record));
			// Выполняем открытие группы захвата поля
			result.append(1, '(');
		/**
		 * Если ссылка названия поля не несёт
		 */
		} else
			// Выполняем открытие группы, захвата не выполняющей
			result.append("(?:");
		// Выполняем добавление шаблона ссылки в набор незавершённых
		stack.push_back(name);
		/**
		 * Если разворот текста шаблона ссылки не выполнен
		 */
		if(!this->expand(j->second, result, fields, stack, number, static_cast <uint16_t> (depth + 1)))
			// Выводим результат разворота ссылок текста шаблона
			return false;
		// Выполняем удаление шаблона ссылки из набора незавершённых
		stack.pop_back();
		// Выполняем закрытие группы шаблона ссылки
		result.append(1, ')');
		/**
		 * Если допустимый размер развёрнутого текста превышен
		 */
		if(result.size() > MAX_LENGTH) {
			// Устанавливаем код ошибки разбора шаблона
			this->_error = error_t::PATTERN_TOO_LARGE;
			// Выводим результат разворота ссылок текста шаблона
			return false;
		}
		// Выполняем перенос позиции начала участка текста
		offset = (end + 1);
		// Выполняем перенос позиции разбора текста шаблона
		i = end;
	}
	// Выполняем перенос остатка текста, ссылок не несущего
	result.append(body.substr(offset));
	// Выполняем подсчёт групп захвата перенесённого остатка текста
	this->account(body.substr(offset), number);
	// Выводим результат разворота ссылок текста шаблона
	return true;
}
/**
 * @brief Метод очистки реестра шаблонов
 *
 */
void awh::Grok::clear() noexcept {
	/**
	 * Если согласование доступа к реестру шаблонов установлено
	 */
	if(this->_threadSafety) {
		// Выполняем блокировку доступа к реестру шаблонов
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем очистку реестра шаблонов
		this->_patterns.clear();
		// Выполняем очистку кэша собранных шаблонов
		this->_cache.clear();
		// Выходим из метода
		return;
	}
	// Выполняем очистку реестра шаблонов
	this->_patterns.clear();
	// Выполняем очистку кэша собранных шаблонов
	this->_cache.clear();
}
/**
 * @brief Метод восстановления встроенного набора шаблонов
 *
 */
void awh::Grok::reset() noexcept {
	// Выполняем очистку реестра шаблонов
	this->clear();
	/**
	 * Если согласование доступа к реестру шаблонов установлено
	 */
	if(this->_threadSafety) {
		// Выполняем блокировку доступа к реестру шаблонов
		const lock_guard <mutex> lock(this->_mtx);
		/**
		 * Выполняем перебор встроенного набора шаблонов
		 */
		for(size_t i = 0; i < awh::grok::PATTERNS_COUNT; i++)
			// Выполняем добавление шаблона в реестр
			this->_patterns.emplace(awh::grok::PATTERNS[i].name, awh::grok::PATTERNS[i].body);
		// Выполняем очистку кэша собранных шаблонов
		this->_cache.clear();
		// Выходим из метода
		return;
	}
	/**
	 * Выполняем перебор встроенного набора шаблонов
	 */
	for(size_t i = 0; i < awh::grok::PATTERNS_COUNT; i++)
		// Выполняем добавление шаблона в реестр
		this->_patterns.emplace(awh::grok::PATTERNS[i].name, awh::grok::PATTERNS[i].body);
	// Выполняем очистку кэша собранных шаблонов
	this->_cache.clear();
}
/**
 * @brief Метод проверки наличия шаблона в реестре
 *
 * @param name название шаблона
 * @return     результат проверки наличия шаблона в реестре
 *
 */
bool awh::Grok::has(string_view name) const noexcept {
	/**
	 * Если название шаблона пусто
	 */
	if(name.empty())
		// Выводим результат проверки наличия шаблона в реестре
		return false;
	/**
	 * Если согласование доступа к реестру шаблонов установлено
	 */
	if(this->_threadSafety) {
		// Выполняем блокировку доступа к реестру шаблонов
		const lock_guard <mutex> lock(this->_mtx);
		// Выводим результат проверки наличия шаблона в реестре
		return (this->_patterns.find(string(name)) != this->_patterns.end());
	}
	// Выводим результат проверки наличия шаблона в реестре
	return (this->_patterns.find(string(name)) != this->_patterns.end());
}
/**
 * @brief Метод удаления шаблона из реестра
 *
 * @param name название шаблона
 * @return     результат удаления шаблона из реестра
 *
 */
bool awh::Grok::erase(string_view name) noexcept {
	/**
	 * Если название шаблона пусто
	 */
	if(name.empty()) {
		// Устанавливаем код ошибки разбора шаблона
		this->_error = error_t::NAME_EMPTY;
		// Выводим результат удаления шаблона из реестра
		return false;
	}
	/**
	 * Если согласование доступа к реестру шаблонов установлено
	 */
	if(this->_threadSafety) {
		// Выполняем блокировку доступа к реестру шаблонов
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем очистку кэша собранных шаблонов
		this->_cache.clear();
		// Выводим результат удаления шаблона из реестра
		return (this->_patterns.erase(string(name)) > 0);
	}
	// Выполняем очистку кэша собранных шаблонов
	this->_cache.clear();
	// Выводим результат удаления шаблона из реестра
	return (this->_patterns.erase(string(name)) > 0);
}
/**
 * @brief Метод извлечения текста шаблона из реестра
 *
 * @param name название шаблона
 * @return     текст шаблона либо пустой текст при его отсутствии
 *
 */
string awh::Grok::pattern(string_view name) const noexcept {
	// Текст шаблона реестра
	string result;
	/**
	 * Если название шаблона пусто
	 */
	if(name.empty())
		// Выводим текст шаблона реестра
		return result;
	/**
	 * Выполняем извлечение текста шаблона из реестра
	 */
	auto extract = [&]() noexcept -> void {
		// Выполняем поиск шаблона в реестре
		auto i = this->_patterns.find(string(name));
		/**
		 * Если шаблон в реестре обнаружен
		 */
		if(i != this->_patterns.end())
			// Выполняем извлечение текста шаблона
			result.assign(i->second);
	};
	/**
	 * Если согласование доступа к реестру шаблонов установлено
	 */
	if(this->_threadSafety) {
		// Выполняем блокировку доступа к реестру шаблонов
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем извлечение текста шаблона из реестра
		extract();
		// Выводим текст шаблона реестра
		return result;
	}
	// Выполняем извлечение текста шаблона из реестра
	extract();
	// Выводим текст шаблона реестра
	return result;
}
/**
 * @brief Метод добавления шаблона в реестр
 *
 * @param name название шаблона
 * @param body текст шаблона, допускающий ссылки вида «%{NAME}»
 * @return     результат добавления шаблона в реестр
 *
 */
bool awh::Grok::pattern(string_view name, string_view body) noexcept {
	/**
	 * Если название шаблона пусто
	 */
	if(name.empty()) {
		// Устанавливаем код ошибки разбора шаблона
		this->_error = error_t::NAME_EMPTY;
		// Выводим результат добавления шаблона в реестр
		return false;
	}
	/**
	 * Если текст шаблона пуст
	 */
	if(body.empty()) {
		// Устанавливаем код ошибки разбора шаблона
		this->_error = error_t::PATTERN_EMPTY;
		// Выводим результат добавления шаблона в реестр
		return false;
	}
	/**
	 * Если согласование доступа к реестру шаблонов установлено
	 */
	if(this->_threadSafety) {
		// Выполняем блокировку доступа к реестру шаблонов
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем добавление шаблона в реестр
		this->_patterns[string(name)] = string(body);
		// Выполняем очистку кэша собранных шаблонов
		this->_cache.clear();
		// Выводим результат добавления шаблона в реестр
		return true;
	}
	// Выполняем добавление шаблона в реестр
	this->_patterns[string(name)] = string(body);
	// Выполняем очистку кэша собранных шаблонов
	this->_cache.clear();
	// Выводим результат добавления шаблона в реестр
	return true;
}
/**
 * @brief Метод извлечения названий шаблонов реестра
 *
 * @return набор названий шаблонов реестра
 *
 */
vector <string> awh::Grok::patterns() const noexcept {
	// Набор названий шаблонов реестра
	vector <string> result;
	/**
	 * Выполняем извлечение названий шаблонов реестра
	 */
	auto extract = [&]() noexcept -> void {
		// Выполняем размещение набора названий шаблонов
		result.reserve(this->_patterns.size());
		/**
		 * Выполняем перебор реестра шаблонов
		 */
		for(const auto & item : this->_patterns)
			// Выполняем добавление названия шаблона в набор
			result.push_back(item.first);
	};
	/**
	 * Если согласование доступа к реестру шаблонов установлено
	 */
	if(this->_threadSafety) {
		// Выполняем блокировку доступа к реестру шаблонов
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем извлечение названий шаблонов реестра
		extract();
		// Выводим набор названий шаблонов реестра
		return result;
	}
	// Выполняем извлечение названий шаблонов реестра
	extract();
	// Выводим набор названий шаблонов реестра
	return result;
}
/**
 * @brief Метод сборки шаблона Grok
 *
 * @param pattern текст шаблона со ссылками вида «%{NAME}»
 * @param flags   набор режимов сборки регулярного выражения
 * @return        собранный шаблон либо пустая ссылка при отказе
 *
 */
awh::Grok::exp_t awh::Grok::build(string_view pattern, const uint32_t flags) const noexcept {
	// Собранный шаблон Grok
	exp_t result = nullptr;
	// Выполняем сброс кода ошибки разбора шаблона
	this->_error = error_t::NONE;
	/**
	 * Если текст шаблона пуст
	 */
	if(pattern.empty()) {
		// Устанавливаем код ошибки разбора шаблона
		this->_error = error_t::PATTERN_EMPTY;
		// Выводим собранный шаблон Grok
		return result;
	}
	// Создаём ключ кэша собранных шаблонов Grok
	const key_t key = make_pair(flags, string(pattern));
	/**
	 * Выполняем поиск шаблона в кэше собранных шаблонов
	 *
	 * @details Кэш удерживает слабую ссылку, поэтому шаблон находится в кэше
	 *          лишь до тех пор, пока его удерживает вызывающая сторона.
	 *
	 */
	{
		// Выполняем блокировку потока при согласовании доступа к реестру
		unique_lock <mutex> lock(this->_mtx, defer_lock);
		/**
		 * Если согласование доступа к реестру шаблонов установлено
		 */
		if(this->_threadSafety)
			// Выполняем блокировку потока
			lock.lock();
		// Выполняем поиск ключа в кэше собранных шаблонов
		auto i = this->_cache.find(key);
		/**
		 * Если ключ в кэше собранных шаблонов найден
		 */
		if(i != this->_cache.end()) {
			// Получаем собранный ранее шаблон Grok
			exp_t cached = i->second.lock();
			/**
			 * Если собранный ранее шаблон ещё удерживается
			 */
			if(cached)
				// Выводим собранный ранее шаблон Grok
				return cached;
			// Выполняем удаление освобождённого шаблона из кэша
			this->_cache.erase(i);
		}
	}
	// Создаём собранный шаблон Grok
	auto expression = make_shared <awh::grok::expression_t> ();
	// Устанавливаем исходный текст шаблона
	expression->pattern.assign(pattern);
	// Набор шаблонов, разворот каких не завершён
	vector <string> stack;
	// Номер очередной группы захвата
	uint32_t number = 0;
	/**
	 * Выполняем разворот ссылок текста шаблона
	 */
	auto unfold = [&]() noexcept -> bool {
		// Выводим результат разворота ссылок текста шаблона
		return this->expand(pattern, expression->expression, expression->fields, stack, number, 0);
	};
	// Признак выполнения разворота ссылок текста шаблона
	bool unfolded = false;
	/**
	 * Если согласование доступа к реестру шаблонов установлено
	 */
	if(this->_threadSafety) {
		// Выполняем блокировку доступа к реестру шаблонов
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем разворот ссылок текста шаблона
		unfolded = unfold();
	// Выполняем разворот ссылок текста шаблона
	} else unfolded = unfold();
	/**
	 * Если разворот ссылок текста шаблона не выполнен
	 */
	if(!unfolded)
		// Выводим собранный шаблон Grok
		return result;
	/**
	 * Выполняем сборку развёрнутого регулярного выражения
	 *
	 * @details Режим DUPNAMES устанавливается всегда: тексты шаблонов несут
	 *          именованные группы впрямую, и одно название встречается в
	 *          ветвях, объединяемых шаблоном вышестоящим.
	 */
	expression->exp = this->_regexp.build(expression->expression, (flags | static_cast <uint32_t> (flag_t::DUPNAMES)));
	/**
	 * Если сборка развёрнутого регулярного выражения не выполнена
	 */
	if(!expression->exp) {
		// Устанавливаем код ошибки разбора шаблона
		this->_error = error_t::EXPRESSION;
		// Выводим собранный шаблон Grok
		return result;
	}
	// Выполняем установку собранного шаблона Grok
	result = ::move(expression);
	/**
	 * Выполняем размещение собранного шаблона в кэше собранных шаблонов
	 */
	{
		// Выполняем блокировку потока при согласовании доступа к реестру
		unique_lock <mutex> lock(this->_mtx, defer_lock);
		/**
		 * Если согласование доступа к реестру шаблонов установлено
		 */
		if(this->_threadSafety)
			// Выполняем блокировку потока
			lock.lock();
		// Выполняем размещение собранного шаблона в кэше
		this->_cache[key] = result;
	}
	// Выводим собранный шаблон Grok
	return result;
}
/**
 * @brief Метод сборки шаблона Grok
 *
 * @param pattern текст шаблона со ссылками вида «%{NAME}»
 * @param flags   набор режимов сборки регулярного выражения
 * @return        собранный шаблон либо пустая ссылка при отказе
 *
 */
awh::Grok::exp_t awh::Grok::build(string_view pattern, const vector <flag_t> & flags) const noexcept {
	// Набор режимов сборки регулярного выражения
	uint32_t mode = 0;
	/**
	 * Выполняем перебор набора режимов сборки
	 */
	for(const auto & flag : flags)
		// Выполняем объединение режима сборки с набором
		mode |= static_cast <uint32_t> (flag);
	// Выводим собранный шаблон Grok
	return this->build(pattern, mode);
}
/**
 * @brief Метод проверки соответствия текста шаблону
 *
 * @param text текст для сопоставления
 * @param exp  собранный шаблон
 * @return     результат проверки соответствия текста шаблону
 *
 */
bool awh::Grok::test(string_view text, const exp_t & exp) const noexcept {
	/**
	 * Если собранный шаблон не получен
	 */
	if(!exp || !exp->exp)
		// Выводим результат проверки соответствия текста шаблону
		return false;
	// Выводим результат проверки соответствия текста шаблону
	return this->_regexp.test(text, exp->exp);
}
/**
 * @brief Метод извлечения границ совпадения и захваченных групп
 *
 * @param text текст для сопоставления
 * @param exp  собранный шаблон
 * @return     набор границ совпадения и захваченных групп
 *
 */
vector <pair <size_t, size_t>> awh::Grok::match(string_view text, const exp_t & exp) const noexcept {
	/**
	 * Если собранный шаблон не получен
	 */
	if(!exp || !exp->exp)
		// Выводим набор границ совпадения и захваченных групп
		return vector <pair <size_t, size_t>> ();
	// Выводим набор границ совпадения и захваченных групп
	return this->_regexp.match(text, exp->exp);
}
/**
 * @brief Метод извлечения именованных полей из текста
 *
 * @param text   текст для сопоставления
 * @param exp    собранный шаблон
 * @param result набор извлечённых полей
 * @return       результат извлечения именованных полей из текста
 *
 */
bool awh::Grok::exec(string_view text, const exp_t & exp, unordered_map <string, string> & result) const noexcept {
	// Выполняем очистку набора извлечённых полей
	result.clear();
	/**
	 * Если собранный шаблон не получен
	 */
	if(!exp || !exp->exp)
		// Выводим результат извлечения именованных полей из текста
		return false;
	// Получаем набор границ совпадения и захваченных групп
	const auto & bounds = this->_regexp.match(text, exp->exp);
	/**
	 * Если совпадение с текстом не обнаружено
	 */
	if(bounds.empty())
		// Выводим результат извлечения именованных полей из текста
		return false;
	/**
	 * Выполняем перебор набора полей собранного шаблона
	 */
	for(const auto & field : exp->fields) {
		/**
		 * Если номер группы захвата поля набору границ не принадлежит
		 */
		if(static_cast <size_t> (field.number) >= bounds.size())
			// Переходим к следующему полю собранного шаблона
			continue;
		// Получаем границы захвата группой поля
		const auto & bound = bounds.at(field.number);
		/**
		 * Если захват группой поля не выполнен
		 */
		if((bound.first == string_view::npos) || (bound.second == string_view::npos) || (bound.second < bound.first))
			// Переходим к следующему полю собранного шаблона
			continue;
		// Выполняем добавление извлечённого поля в набор
		result[field.name] = string(text.substr(bound.first, (bound.second - bound.first)));
	}
	// Выводим результат извлечения именованных полей из текста
	return true;
}
/**
 * @brief Метод извлечения значений полей из текста
 *
 * @param text   текст для сопоставления
 * @param exp    собранный шаблон
 * @param result набор извлечённых значений полей
 * @return       результат извлечения значений полей из текста
 *
 */
bool awh::Grok::exec(string_view text, const exp_t & exp, vector <value_t> & result) const noexcept {
	// Выполняем очистку набора извлечённых значений полей
	result.clear();
	/**
	 * Если собранный шаблон не получен
	 */
	if(!exp || !exp->exp)
		// Выводим результат извлечения значений полей из текста
		return false;
	// Получаем набор границ совпадения и захваченных групп
	const auto & bounds = this->_regexp.match(text, exp->exp);
	/**
	 * Если совпадение с текстом не обнаружено
	 */
	if(bounds.empty())
		// Выводим результат извлечения значений полей из текста
		return false;
	// Выполняем размещение набора извлечённых значений полей
	result.reserve(exp->fields.size());
	/**
	 * Выполняем перебор набора полей собранного шаблона
	 */
	for(const auto & field : exp->fields) {
		/**
		 * Если номер группы захвата поля набору границ не принадлежит
		 */
		if(static_cast <size_t> (field.number) >= bounds.size())
			// Переходим к следующему полю собранного шаблона
			continue;
		// Получаем границы захвата группой поля
		const auto & bound = bounds.at(field.number);
		/**
		 * Если захват группой поля не выполнен
		 */
		if((bound.first == string_view::npos) || (bound.second == string_view::npos) || (bound.second < bound.first))
			// Переходим к следующему полю собранного шаблона
			continue;
		// Создаём извлечённое значение поля
		value_t value;
		// Устанавливаем вид значения поля
		value.kind = field.kind;
		// Устанавливаем название поля
		value.name.assign(field.name);
		// Устанавливаем извлечённое значение поля
		value.value.assign(text.substr(bound.first, (bound.second - bound.first)));
		// Выполняем добавление извлечённого значения поля в набор
		result.push_back(::move(value));
	}
	// Выводим результат извлечения значений полей из текста
	return true;
}
/**
 * @brief Пространство имён вспомогательных функций вывода записи JSON
 *
 */
namespace {
	/**
	 * @brief Функция проверки соответствия захвата объявленному виду значения
	 *
	 * @param text текст захвата поля
	 * @param kind вид значения поля
	 * @return     результат проверки соответствия захвата виду значения
	 *
	 * @details Разбор ведётся до конца текста: запись «42abc» числом объявлена
	 *          быть вправе, но числом не является, и вывод её числом дал бы
	 *          запись JSON неправильную.
	 *
	 */
	bool numeric(const string & text, const awh::grok::kind_t kind) noexcept {
		/**
		 * Если текст захвата пуст
		 */
		if(text.empty())
			// Выводим результат проверки соответствия захвата виду значения
			return false;
		/**
		 * Определяем вид значения поля
		 */
		switch(static_cast <uint8_t> (kind)) {
			/**
			 * Если значение поля прочитано числом целым
			 */
			case static_cast <uint8_t> (awh::grok::kind_t::INTEGER): {
				// Разбираемое целое число
				int64_t value = 0;
				// Выполняем разбор целого числа
				const auto result = awh::Lexical::fromChars(text.data(), (text.data() + text.size()), value);
				// Выводим результат разбора целого числа до конца текста
				return (static_cast <bool> (result) && (result.ptr == (text.data() + text.size())));
			}
			/**
			 * Если значение поля прочитано числом дробным
			 */
			case static_cast <uint8_t> (awh::grok::kind_t::FLOATING): {
				// Разбираемое дробное число
				double value = 0.;
				// Выполняем разбор дробного числа
				const auto result = awh::Lexical::fromChars(text.data(), (text.data() + text.size()), value);
				// Выводим результат разбора дробного числа до конца текста
				return (static_cast <bool> (result) && (result.ptr == (text.data() + text.size())));
			}
		}
		// Выводим результат проверки соответствия захвата виду значения
		return false;
	}
	/**
	 * @brief Функция вывода текста строкой записи JSON
	 *
	 * @param text   выводимый текст
	 * @param result запись JSON, куда ведётся вывод
	 *
	 * @details Управляющие символы выводятся последовательностями
	 *          экранированными, а последовательности UTF-8 переносятся как
	 *          есть: запись JSON записывается в UTF-8, и разбирать её ради
	 *          вывода той же записью проку нет.
	 *
	 */
	void quoted(const string & text, string & result) noexcept {
		// Выполняем открытие строки записи JSON
		result.append(1, '"');
		/**
		 * Выполняем перебор символов выводимого текста
		 */
		for(const char letter : text) {
			/**
			 * Определяем символ выводимого текста
			 */
			switch(letter) {
				// Выполняем вывод кавычки
				case '"': result.append("\\\""); break;
				// Выполняем вывод обратной косой черты
				case '\\': result.append("\\\\"); break;
				// Выполняем вывод забоя
				case '\b': result.append("\\b"); break;
				// Выполняем вывод подачи страницы
				case '\f': result.append("\\f"); break;
				// Выполняем вывод перевода строки
				case '\n': result.append("\\n"); break;
				// Выполняем вывод возврата каретки
				case '\r': result.append("\\r"); break;
				// Выполняем вывод горизонтальной табуляции
				case '\t': result.append("\\t"); break;
				/**
				 * Если символ является прочим символом выводимого текста
				 */
				default: {
					/**
					 * Если символ является управляющим
					 */
					if(static_cast <uint8_t> (letter) < 0x20) {
						// Запись кодового значения управляющего символа
						char record[7];
						// Выполняем запись кодового значения управляющего символа
						::snprintf(record, sizeof(record), "\\u%04X", static_cast <uint32_t> (static_cast <uint8_t> (letter)));
						// Выполняем вывод кодового значения управляющего символа
						result.append(record);
					// Выполняем вывод символа выводимого текста
					} else result.append(1, letter);
				}
			}
		}
		// Выполняем закрытие строки записи JSON
		result.append(1, '"');
	}
};

/**
 * @brief Метод вывода набора значений полей записью JSON
 *
 * @param values набор значений полей
 * @param pretty признак вывода записи с отступами
 * @return       запись JSON набора значений полей
 *
 */
string awh::Grok::json(const vector <value_t> & values, const bool pretty) const noexcept {
	// Запись JSON набора значений полей
	string result;
	// Набор названий полей в порядке вывода
	vector <string> names;
	// Набор значений полей по названиям
	unordered_map <string, const value_t *> records;
	/**
	 * Выполняем перебор набора значений полей
	 *
	 * @details Повторное название поля выигрывает последним, а место в порядке
	 *          вывода занимает первым: объект JSON повторов ключа не несёт.
	 */
	for(const auto & value : values) {
		/**
		 * Если название поля выводится впервые
		 */
		if(records.emplace(value.name, &value).second)
			// Выполняем добавление названия поля в порядок вывода
			names.push_back(value.name);
		// Выполняем замену значения поля выведенным позже
		else records.at(value.name) = &value;
	}
	// Выполняем открытие объекта записи JSON
	result.append(1, '{');
	/**
	 * Выполняем перебор набора названий полей
	 */
	for(size_t i = 0; i < names.size(); i++) {
		/**
		 * Если выводимое поле объекта первым не является
		 */
		if(i > 0)
			// Выполняем вывод разделителя полей объекта
			result.append(1, ',');
		/**
		 * Если запись выводится с отступами
		 */
		if(pretty)
			// Выполняем вывод перевода строки с отступом
			result.append("\n\t");
		// Получаем значение выводимого поля
		const value_t * value = records.at(names.at(i));
		// Выполняем вывод названия поля
		quoted(value->name, result);
		// Выполняем вывод разделителя названия и значения
		result.append(pretty ? ": " : ":");
		/**
		 * Если захват объявленному виду значения отвечает
		 */
		if((value->kind != kind_t::TEXT) && numeric(value->value, value->kind))
			// Выполняем вывод значения поля числом
			result.append(value->value);
		// Выполняем вывод значения поля текстом
		else quoted(value->value, result);
	}
	/**
	 * Если запись выводится с отступами и объект не пуст
	 */
	if(pretty && !names.empty())
		// Выполняем вывод перевода строки
		result.append(1, '\n');
	// Выполняем закрытие объекта записи JSON
	result.append(1, '}');
	// Выводим запись JSON набора значений полей
	return result;
}
/**
 * @brief Метод вывода извлечённых полей записью JSON
 *
 * @param text   текст для сопоставления
 * @param exp    собранный шаблон
 * @param result запись JSON извлечённых полей
 * @param pretty признак вывода записи с отступами
 * @return       результат вывода извлечённых полей записью JSON
 *
 */
bool awh::Grok::json(string_view text, const exp_t & exp, string & result, const bool pretty) const noexcept {
	// Выполняем очистку записи JSON извлечённых полей
	result.clear();
	// Набор извлечённых значений полей
	vector <value_t> values;
	/**
	 * Если извлечение значений полей из текста не выполнено
	 */
	if(!this->exec(text, exp, values))
		// Выводим результат вывода извлечённых полей записью JSON
		return false;
	// Выполняем вывод набора значений полей записью JSON
	result = this->json(values, pretty);
	// Выводим результат вывода извлечённых полей записью JSON
	return true;
}
/**
 * @brief Метод извлечения набора полей собранного шаблона
 *
 * @param exp собранный шаблон
 * @return    набор полей собранного шаблона
 *
 */
const vector <awh::Grok::field_t> & awh::Grok::fields(const exp_t & exp) const noexcept {
	/**
	 * @brief Пустой набор полей собранного шаблона
	 *
	 */
	static const vector <field_t> empty;
	/**
	 * Если собранный шаблон не получен
	 */
	if(!exp)
		// Выводим пустой набор полей собранного шаблона
		return empty;
	// Выводим набор полей собранного шаблона
	return exp->fields;
}
/**
 * @brief Метод извлечения кода ошибки разбора шаблона
 *
 * @return код ошибки разбора шаблона
 *
 */
awh::Grok::error_t awh::Grok::error() const noexcept {
	// Выводим код ошибки разбора шаблона
	return this->_error;
}
/**
 * @brief Метод установки согласования доступа к реестру шаблонов
 *
 * @param mode режим согласования доступа к реестру шаблонов
 *
 */
void awh::Grok::threadSafety(const bool mode) noexcept {
	// Выполняем установку согласования доступа к реестру шаблонов
	this->_threadSafety = mode;
}
/**
 * @brief Конструктор
 *
 */
awh::Grok::Grok() noexcept : _error(error_t::NONE), _threadSafety(false) {
	// Выполняем размещение реестра шаблонов
	this->_patterns.reserve(awh::grok::PATTERNS_COUNT);
	/**
	 * Выполняем перебор встроенного набора шаблонов
	 */
	for(size_t i = 0; i < awh::grok::PATTERNS_COUNT; i++)
		// Выполняем добавление шаблона в реестр
		this->_patterns.emplace(awh::grok::PATTERNS[i].name, awh::grok::PATTERNS[i].body);
}
