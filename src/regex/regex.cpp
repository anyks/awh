/**
 * @file regex.cpp
 * @date 2026-07-31
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
 * @brief Реализация открытого интерфейса модуля регулярных выражений — сборка
 *        разделяемых регулярных выражений с кэшем собранного, сопоставление
 *        выражения с текстом и извлечение захваченных групп по номеру и по имени
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>
#include <regex/text.hpp>
#include <encoding/unicode/utf8.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Метод извлечения движка сопоставления потока исполнения
 *
 * @details Движок хранит рабочее состояние сопоставления и потоками исполнения
 *          не разделяется, тогда как собранное выражение после сборки не
 *          изменяется и разделяется ими свободно, благодаря чему сопоставление
 *          согласования доступа не требует.
 *
 * @return движок сопоставления потока исполнения
 *
 */
static awh::regex::engine_t & engine() noexcept {
	/**
	 * Создаём движок сопоставления потока исполнения
	 *
	 * @note Связка передаётся пустою, и передаётся ЯВНО: движок этот - рабочее
	 *       состояние потока, у него нет владельца, чей журнал он мог бы взять.
	 *       Отказы его молчанием не пропадают: их читает выражение вызовами
	 *       error(), offset() и message() и само пишет их в свой журнал
	 */
	static thread_local awh::regex::engine_t result(nullptr);
	// Выводим движок сопоставления потока исполнения
	return result;
}
/**
 * @brief Оператор вычисления хэша ключа кэша
 *
 * @param key ключ кэша собранных регулярных выражений
 * @return    вычисленное значение хэша ключа
 *
 */
size_t awh::RegularExpression::Hash::operator () (const key_t & key) const noexcept {
	// Получаем хэш набора режимов сборки регулярного выражения
	const size_t first = std::hash <uint32_t> ()(key.first);
	// Получаем хэш текста регулярного выражения
	const size_t second = std::hash <string> ()(key.second);
	// Выводим смешанное значение хэша ключа кэша
	return (first ^ (second + 0x9E3779B9 + (first << 6) + (first >> 2)));
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 *
 */
awh::RegularExpression::RegularExpression(const log_t * log) noexcept :
 _error(error_t::NONE), _offset(0), _log(log), _limit(awh::regex::MAX_STEPS) {}
/**
 * @brief Метод извлечения кода ошибки последней сборки
 *
 * @return код ошибки последней операции сборки
 *
 */
awh::RegularExpression::error_t awh::RegularExpression::error() const noexcept {
	// Выводим код ошибки последней операции сборки
	return this->_error;
}
/**
 * @brief Метод извлечения смещения ошибки последней сборки
 *
 * @return смещение ошибки в тексте регулярного выражения
 *
 */
size_t awh::RegularExpression::offset() const noexcept {
	// Выводим смещение ошибки в тексте регулярного выражения
	return this->_offset;
}
/**
 * @brief Метод извлечения текста ошибки последней сборки
 *
 * @return текст ошибки последней операции сборки
 *
 */
const string & awh::RegularExpression::message() const noexcept {
	// Выводим текст ошибки последней операции сборки
	return this->_message;
}
/**
 * @brief Метод очистки кэша собранных регулярных выражений
 *
 */
void awh::RegularExpression::clear() noexcept {
	// Выполняем очистку кэша собранных выражений
	this->_cache.clear();
}
/**
 * @brief Метод установки наибольшего допустимого объёма работы сопоставления
 *
 * @param limit наибольшее допустимое количество шагов сопоставления
 *
 */
void awh::RegularExpression::limit(const size_t limit) noexcept {
	// Выполняем установку наибольшего допустимого объёма работы сопоставления
	this->_limit = limit;
}
/**
 * @brief Метод сборки регулярного выражения
 *
 * @param pattern текст регулярного выражения
 * @param flags   набор режимов сборки регулярного выражения
 * @return        собранное регулярное выражение
 *
 */
awh::RegularExpression::exp_t awh::RegularExpression::build(string_view pattern, const uint32_t flags) const noexcept {
	// Выполняем сброс кода ошибки последней операции сборки
	this->_error = error_t::NONE;
	// Выполняем сброс смещения ошибки в тексте выражения
	this->_offset = 0;
	// Выполняем очистку текста ошибки последней операции сборки
	this->_message.clear();
	// Создаём ключ кэша собранных регулярных выражений
	const key_t key = make_pair(flags, string(pattern));
	/**
	 * Выполняем поиск выражения в кэше собранных выражений
	 *
	 * @details Кэш удерживает слабую ссылку, поэтому выражение находится в кэше
	 *          лишь до тех пор, пока его удерживает вызывающая сторона.
	 *
	 */
	{
		// Выполняем поиск ключа в кэше собранных выражений
		auto i = this->_cache.find(key);
		/**
		 * Если ключ в кэше собранных выражений найден
		 */
		if(i != this->_cache.end()) {
			// Получаем собранное ранее регулярное выражение
			exp_t result = i->second.lock();
			/**
			 * Если собранное ранее выражение ещё удерживается
			 */
			if(result)
				// Выводим собранное ранее регулярное выражение
				return result;
			// Выполняем удаление освобождённого выражения из кэша
			this->_cache.erase(i);
		}
	}
	// Создаём собираемое регулярное выражение
	auto result = make_shared <awh::regex::expression_t> ();
	/**
	 * Если сборка регулярного выражения не выполнена
	 */
	if(!engine().build(pattern, flags, * result)) {
		// Выполняем установку кода ошибки последней операции сборки
		this->_error = engine().error();
		// Выполняем установку смещения ошибки в тексте выражения
		this->_offset = engine().offset();
		// Выполняем установку текста ошибки последней операции сборки
		this->_message = engine().message();
		/**
		 * Если объект журнала событий передан
		 */
		if(this->_log != nullptr) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Regular expression could not be built at offset %zu: %s", __PRETTY_FUNCTION__, make_tuple(string(pattern), flags), log_t::flag_t::WARNING, this->_offset, this->_message.c_str());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Regular expression could not be built at offset %zu: %s", log_t::flag_t::WARNING, this->_offset, this->_message.c_str());
			#endif
		}
		// Выводим отсутствие собранного регулярного выражения
		return exp_t();
	}
	// Создаём собранное регулярное выражение
	exp_t expression = result;
	/**
	 * Выполняем размещение собранного выражения в кэше собранных выражений
	 */
	{
		// Выполняем размещение собранного выражения в кэше
		this->_cache[key] = expression;
	}
	// Выводим собранное регулярное выражение
	return expression;
}
/**
 * @brief Метод сборки регулярного выражения
 *
 * @param pattern текст регулярного выражения
 * @param flags   набор режимов сборки регулярного выражения
 * @return        собранное регулярное выражение
 *
 */
awh::RegularExpression::exp_t awh::RegularExpression::build(string_view pattern, const vector <flag_t> & flags) const noexcept {
	// Набор режимов сборки регулярного выражения
	uint32_t options = 0;
	/**
	 * Выполняем перебор набора режимов сборки регулярного выражения
	 */
	for(const auto & flag : flags)
		// Выполняем установку режима сборки регулярного выражения
		options |= static_cast <uint32_t> (flag);
	// Выводим результат сборки регулярного выражения
	return this->build(pattern, options);
}
/**
 * @brief Функция проверки правильности записи текста сопоставления
 *
 * @param text текст для сопоставления
 * @param exp  собранное регулярное выражение
 * @return     результат проверки правильности записи текста
 *
 * @details Под режимом «UTF» текст разбирается посимвольно, и запись,
 *          кодировке UTF-8 не отвечающая, разбору не поддаётся: сопоставление
 *          по такому тексту выдаёт границы, символам не отвечающие, а извлечение
 *          по ним - обрубки записей. Эталонная реализация отвечает на такой
 *          текст отказом, и модуль отвечает им же.
 *
 *          Проверка ведётся лишь под режимом «UTF»: без него текст разбирается
 *          побайтно, и понятия правильности записи для него нет вовсе. Снимает
 *          её режим «UNCHECKED»: проверка проходит текст целиком, и проход по
 *          тексту повторными вызовами от очередной позиции обходится
 *          квадратично, поэтому там, где текст проверен единожды снаружи,
 *          проверку следует снимать.
 *
 */
static bool verified(string_view text, const awh::RegularExpression::exp_t & exp) noexcept {
	/**
	 * Если режим разбора текста посимвольно не установлен
	 */
	if(!awh::regex::hasFlag(exp->forward.flags, awh::regex::flag_t::UTF))
		// Выводим результат проверки правильности записи текста
		return true;
	/**
	 * Если проверка правильности записи текста снята
	 */
	if(awh::regex::hasFlag(exp->forward.flags, awh::regex::flag_t::UNCHECKED))
		// Выводим результат проверки правильности записи текста
		return true;
	// Выводим результат проверки правильности записи текста
	return awh::utf8::valid(text);
}
/**
 * @brief Метод проверки наличия совпадения в тексте
 *
 * @param text текст для сопоставления
 * @param exp  собранное регулярное выражение
 * @return     результат проверки наличия совпадения
 *
 */
bool awh::RegularExpression::test(string_view text, const exp_t & exp) const noexcept {
	/**
	 * Если собранное регулярное выражение не установлено
	 */
	if(!exp)
		// Выводим результат проверки наличия совпадения
		return false;
	/**
	 * Если запись текста сопоставления кодировке UTF-8 не отвечает
	 */
	if(!verified(text, exp)) {
		// Устанавливаем ошибку неверной записи текста сопоставления
		this->_error = error_t::BAD_UTF8_SUBJECT;
		// Выводим результат проверки наличия совпадения
		return false;
	}
	/**
	 * Выполняем установку наибольшего допустимого объёма работы сопоставления
	 *
	 * @note Потолок ставится движку перед каждым сопоставлением, а не единожды:
	 *       движок принадлежит потоку исполнения, и поток, впервые обратившийся
	 *       к выражению, взял бы иначе умолчание вместо настройки объекта
	 */
	engine().limit(this->_limit);
	// Выводим результат проверки наличия совпадения в тексте
	return engine().test(* exp, text, 0);
}
/**
 * @brief Метод извлечения границ совпадения и захваченных групп
 *
 * @param text текст для сопоставления
 * @param exp  собранное регулярное выражение
 * @return     набор границ совпадения и захваченных групп
 *
 */
vector <pair <size_t, size_t>> awh::RegularExpression::match(string_view text, const exp_t & exp) const noexcept {
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> result;
	// Выполняем извлечение границ совпадения и захваченных групп
	this->match(text, exp, result);
	// Выводим набор границ совпадения и захваченных групп
	return result;
}
/**
 * @brief Метод извлечения границ совпадения и захваченных групп
 *
 * @param text   текст для сопоставления
 * @param exp    собранное регулярное выражение
 * @param result набор границ совпадения и захваченных групп
 * @return       результат поиска совпадения
 *
 */
bool awh::RegularExpression::match(string_view text, const exp_t & exp, vector <pair <size_t, size_t>> & result) const noexcept {
	// Выполняем очистку набора границ совпадения
	result.clear();
	/**
	 * Если собранное регулярное выражение не установлено
	 */
	if(!exp)
		// Выводим результат поиска совпадения
		return false;
	/**
	 * Если запись текста сопоставления кодировке UTF-8 не отвечает
	 */
	if(!verified(text, exp)) {
		// Устанавливаем ошибку неверной записи текста сопоставления
		this->_error = error_t::BAD_UTF8_SUBJECT;
		// Выводим результат поиска совпадения
		return false;
	}
	// Выполняем установку наибольшего допустимого объёма работы сопоставления
	engine().limit(this->_limit);
	/**
	 * Если совпадение в тексте не обнаружено
	 */
	if(!engine().exec(* exp, text, 0, result)) {
		// Выполняем очистку набора границ совпадения
		result.clear();
		// Выводим результат поиска совпадения
		return false;
	}
	// Выводим результат поиска совпадения
	return true;
}
/**
 * @brief Метод извлечения текста совпадения и захваченных групп
 *
 * @param text текст для сопоставления
 * @param exp  собранное регулярное выражение
 * @return     набор текста совпадения и захваченных групп
 *
 */
vector <string> awh::RegularExpression::exec(string_view text, const exp_t & exp) const noexcept {
	// Набор текста совпадения и захваченных групп
	vector <string> result;
	// Получаем набор границ совпадения и захваченных групп
	const auto & bounds = this->match(text, exp);
	// Выполняем размещение набора текста совпадения
	result.reserve(bounds.size());
	/**
	 * Выполняем перебор набора границ совпадения и захваченных групп
	 */
	for(const auto & bound : bounds) {
		/**
		 * Если захват группой не выполнен
		 */
		if((bound.first == string_view::npos) || (bound.second == string_view::npos) || (bound.second < bound.first)) {
			// Выполняем добавление пустого текста захвата
			result.emplace_back();
			// Переходим к следующим границам захвата
			continue;
		}
		// Выполняем добавление текста захвата группой
		result.emplace_back(text.substr(bound.first, (bound.second - bound.first)));
	}
	// Выводим набор текста совпадения и захваченных групп
	return result;
}
/**
 * @brief Метод извлечения номера именованной группы
 *
 * @param exp  собранное регулярное выражение
 * @param name имя именованной группы выражения
 * @return     номер именованной группы либо ноль при её отсутствии
 *
 */
uint32_t awh::RegularExpression::group(const exp_t & exp, string_view name) const noexcept {
	/**
	 * Если собранное регулярное выражение не установлено
	 */
	if(!exp)
		// Выводим отсутствие именованной группы выражения
		return 0;
	// Выполняем поиск имени в соответствии имён групп наборам их номеров
	auto i = exp->names.find(string(name));
	/**
	 * Если имя в соответствии имён групп не найдено
	 */
	if((i == exp->names.end()) || i->second.empty())
		// Выводим отсутствие именованной группы выражения
		return 0;
	// Выводим номер именованной группы, объявленной первой
	return i->second.front();
}
/**
 * @brief Метод извлечения соответствия имён групп наборам их номеров
 *
 * @param exp собранное регулярное выражение
 * @return    соответствие имён именованных групп наборам их номеров
 *
 */
const unordered_map <string, vector <uint32_t>> & awh::RegularExpression::groups(const exp_t & exp) const noexcept {
	// Создаём пустое соответствие имён групп для несобранного выражения
	static const unordered_map <string, vector <uint32_t>> empty;
	/**
	 * Если собранное регулярное выражение не установлено
	 */
	if(!exp)
		// Выводим пустое соответствие имён именованных групп
		return empty;
	// Выводим соответствие имён именованных групп наборам их номеров
	return exp->names;
}
/**
 * @brief Метод извлечения текста, захваченного именованной группой
 *
 * @param text   текст, с которым выполнялось сопоставление
 * @param bounds набор границ совпадения и захваченных групп
 * @param exp    собранное регулярное выражение
 * @param name   имя именованной группы выражения
 * @return       текст, захваченный именованной группой
 *
 */
string_view awh::RegularExpression::capture(string_view text, const vector <pair <size_t, size_t>> & bounds, const exp_t & exp, string_view name) const noexcept {
	/**
	 * Если собранное регулярное выражение не установлено
	 */
	if(!exp)
		// Выводим отсутствие захваченного текста
		return string_view();
	// Выполняем поиск имени в соответствии имён групп наборам их номеров
	auto i = exp->names.find(string(name));
	/**
	 * Если имя в соответствии имён групп не найдено
	 */
	if(i == exp->names.end())
		// Выводим отсутствие захваченного текста
		return string_view();
	/**
	 * Выполняем перебор номеров групп, объявленных этим именем
	 *
	 * @details Одно имя объявляется несколькими группами лишь в режиме «DUPNAMES»,
	 *          и захват выполняет та из них, что участвовала в совпадении, поэтому
	 *          перебор прекращается на первой выполнившей захват.
	 *
	 */
	for(const uint32_t number : i->second) {
		/**
		 * Если номер группы находится за пределами набора границ
		 */
		if(static_cast <size_t> (number) >= bounds.size())
			// Переходим к следующему номеру группы
			continue;
		// Получаем границы захвата очередной группы
		const auto & bound = bounds.at(static_cast <size_t> (number));
		/**
		 * Если группа захвата не выполнила
		 */
		if((bound.first == string_view::npos) || (bound.second == string_view::npos) || (bound.second < bound.first))
			// Переходим к следующему номеру группы
			continue;
		// Выводим текст, захваченный именованной группой
		return text.substr(bound.first, (bound.second - bound.first));
	}
	// Выводим отсутствие захваченного текста
	return string_view();
}
/**
 * @brief Метод сопоставления с извлечением именованных групп
 *
 * @param text   текст для сопоставления
 * @param exp    собранное регулярное выражение
 * @param result соответствие имён именованных групп захваченному тексту
 * @return       результат поиска совпадения
 *
 */
bool awh::RegularExpression::exec(string_view text, const exp_t & exp, unordered_map <string, string> & result) const noexcept {
	// Выполняем очистку соответствия имён групп захваченному тексту
	result.clear();
	/**
	 * Если собранное регулярное выражение не установлено
	 */
	if(!exp)
		// Выводим результат поиска совпадения
		return false;
	// Создаём набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	/**
	 * Если совпадение в тексте не обнаружено
	 */
	if(!this->match(text, exp, bounds))
		// Выводим результат поиска совпадения
		return false;
	/**
	 * Выполняем перебор имён именованных групп выражения
	 */
	for(const auto & item : exp->names) {
		// Получаем текст, захваченный очередной именованной группой
		const string_view value = this->capture(text, bounds, exp, item.first);
		/**
		 * Если группа захвата не выполнила
		 *
		 * @details Группа, захвата не выполнившая, от группы, захватившей текст
		 *          нулевой длины, отличается лишь отсутствием в выводимом
		 *          соответствии, поэтому пустой текст в него не помещается.
		 *
		 */
		if(value.data() == nullptr)
			// Переходим к следующей именованной группе
			continue;
		// Выполняем размещение захваченного текста в соответствии имён групп
		result.emplace(item.first, string(value));
	}
	// Выводим результат поиска совпадения
	return true;
}
/**
 * @brief Метод извлечения количества захватывающих групп
 *
 * @param exp собранное регулярное выражение
 * @return    количество захватывающих групп выражения
 *
 */
uint32_t awh::RegularExpression::captures(const exp_t & exp) const noexcept {
	/**
	 * Если собранное регулярное выражение не установлено
	 */
	if(!exp)
		// Выводим отсутствие захватывающих групп выражения
		return 0;
	// Выводим количество захватывающих групп выражения
	return exp->forward.captures;
}
