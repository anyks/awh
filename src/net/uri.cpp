/**
 * @file: uri.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <net/uri.hpp>

/**
 * clear Метод очистки
 */
void awh::URI::URL::clear() noexcept {
	// Выполняем сброс порта
	this->port = 0;
	// Выполняем очистку IP-адреса
	this->ip.clear();
	// Выполняем очистку хоста сервера
	this->host.clear();
	// Выполняем очистку имени пользователя
	this->user.clear();
	// Выполняем очистку пароля пользователя
	this->pass.clear();
	// Выполняем очистку пути URL-запроса
	this->path.clear();
	// Выполняем очистку доменного имени
	this->domain.clear();
	// Выполняем очистку протокола передачи данных
	this->schema.clear();
	// Выполняем очистку якоря URL-запроса
	this->anchor.clear();
	// Выполняем очистку параметров URL-запроса
	this->params.clear();
	// Выполняем сброс протокола интернета AF_INET или AF_INET6
	this->family = AF_INET;
	// Зануляем функцию выполняемую при генерации URL адреса
	this->callback = nullptr;
}
/**
 * empty Метод проверки на существование данных
 * @return результат проверки
 */
bool awh::URI::URL::empty() const noexcept {
	// Выполняем проверку на существование данных
	return (
		this->host.empty()   && this->ip.empty()     &&
		this->user.empty()   && this->pass.empty()   &&
		this->domain.empty() && this->path.empty()   &&
		this->params.empty() && this->anchor.empty()
	);
}
/**
 * Оператор [=] получения параметров URL-запроса
 * @param url объект URL-запроса для получения параметров
 * @return    параметры URL-запроса
 */
awh::URI::URL & awh::URI::URL::operator = (const URL & url) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем копирование порта
		this->port = url.port;
		// Выполняем копирование протокола интернета
		this->family = url.family;
		// Если IP-адрес передан
		if(!url.ip.empty())
			// Выполняем копирование IP-адреса
			this->ip.assign(url.ip.begin(), url.ip.end());
		// Выполняем удаление IP-адреса
		else this->ip.clear();
		// Если хост сервера передан
		if(!url.host.empty())
			// Выполняем копирование хоста сервера
			this->host.assign(url.host.begin(), url.host.end());
		// Выполняем удаление хоста сервера
		else this->host.clear();
		// Если доменное имя сервера передано
		if(!url.domain.empty())
			// Выполняем копирование доменного имени сервера
			this->domain.assign(url.domain.begin(), url.domain.end());
		// Выполняем удаление доменного имени сервера
		else this->domain.clear();
		// Если протокол передачи данных передан
		if(!url.schema.empty())
			// Выполняем копирование протокола передачи данных
			this->schema.assign(url.schema.begin(), url.schema.end());
		// Выполняем удаление протокола передачи данных
		else this->schema.clear();
		// Если якорь URL-запроса передан
		if(!url.anchor.empty())
			// Выполняем копирование якоря URL-запроса
			this->anchor.assign(url.anchor.begin(), url.anchor.end());
		// Выполняем удаление якоря URL-запроса
		else this->anchor.clear();
		// Если пользователь передан
		if(!url.user.empty())
			// Выполняем копирование пользователя
			this->user.assign(url.user.begin(), url.user.end());
		// Выполняем удаление пользователя
		else this->user.clear();
		// Если пароль передан
		if(!url.pass.empty())
			// Выполняем копирование пароля
			this->pass.assign(url.pass.begin(), url.pass.end());
		// Выполняем удаление пароля
		else this->pass.clear();
		// Если путь передан
		if(!url.path.empty())
			// Выполняем копирование пути
			this->path.assign(url.path.begin(), url.path.end());
		// Выполняем удаление пути
		else this->path.clear();
		// Если параметры переданы
		if(!url.params.empty())
			// Выполняем копирование параметров
			this->params.assign(url.params.begin(), url.params.end());
		// Выполняем удаление параметров
		else this->params.clear();
		// Выполняем копирование функции обратного вызова
		this->callback = url.callback;
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::length_error &) {
		/** Игнорируем полученную ошибку */
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception &) {
		/** Игнорируем полученную ошибку */
	}
	// Выводим результат
	return (* this);
}
/**
 * parse Метод получения параметров URL-запроса
 * @param url строка URL-запроса для получения параметров
 * @return    параметры URL-запроса
 */
awh::URI::url_t awh::URI::parse(const string & url) const noexcept {
	// Результат работы функции
	url_t result;
	// Если URL адрес передан
	if(!url.empty()){
		// Выполняем парсинг URL адреса
		const auto & uri = this->split(url);
		// Если сплит URL адреса, прошёл успешно
		if(!uri.empty()){
			// Выполняем поиск схемы протокола
			auto i = uri.find(flag_t::SCHEMA);
			// Если схема протокола получена
			if(i != uri.end())
				// Выполняем извлечение схемы протокола
				result.schema = std::forward <const string> (i->second);
			// Выполняем поиск пути запроса
			i = uri.find(flag_t::PATH);
			// Если путь запроса получен
			if(i != uri.end()){
				// Если схема протокола принадлежит unix-сокету
				if(this->_fmk->compare(result.schema, "unix")){
					// Если пусть не получен
					if(i->second.empty() || (i->second.compare("/") == 0) && (uri.count(flag_t::HOST) > 0))
						// Выполняем установку пути
						(* const_cast <string *> (&i->second)) = uri.at(flag_t::HOST);
					// Выполняем извлечение пути запроса
					result.path = this->splitPath(i->second);
					// Устанавливаем доменное имя
					result.host = std::forward <const string> (i->second);
				// Выполняем извлечение пути запроса
				} else result.path = this->splitPath(i->second);
			}
			// Выполняем поиск параметров запроса
			i = uri.find(flag_t::PARAMS);
			// Если параметры запроса получены
			if(i != uri.end())
				// Выполняем извлечение параметров запроса
				result.params = this->splitParams(i->second);
			// Выполняем поиск якоря запроса
			i = uri.find(flag_t::ANCHOR);
			// Если якорь запроса получен
			if(i != uri.end())
				// Выполняем извлечение якоря запроса
				result.anchor = std::forward <const string> (i->second);
			// Выполняем поиск порта запроса
			i = uri.find(flag_t::PORT);
			// Если порт запроса получен
			if(i != uri.end())
				// Выполняем извлечение порта запроса
				result.port = stoi(i->second);
			// Выполняем поиск пользователя запроса
			i = uri.find(flag_t::LOGIN);
			// Если пользователь запроса получен
			if(i != uri.end())
				// Выполняем извлечение пользователя запроса
				result.user = std::forward <const string> (i->second);
			// Выполняем поиск пароля пользователя запроса
			i = uri.find(flag_t::PASS);
			// Если пароль пользователя запроса получен
			if(i != uri.end())
				// Выполняем извлечение пароля пользователя запроса
				result.pass = std::forward <const string> (i->second);
			// Выполняем поиск хоста запроса
			i = uri.find(flag_t::HOST);
			// Если хост запроса получен
			if(i != uri.end()){
				// Выполняем извлечение хоста запроса
				result.host = std::forward <const string> (i->second);
				// Определяем тип домена
				switch(static_cast <uint8_t> (this->_net.host(result.host))){
					// Если домен является адресом в файловой системе
					case static_cast <uint8_t> (net_t::type_t::FS):
					// Если домен является аппаратным адресом сетевого интерфейса
					case static_cast <uint8_t> (net_t::type_t::MAC):
					// Если домен является HTTP адресом
					case static_cast <uint8_t> (net_t::type_t::URL):
					// Если домен является адресом/Маски сети
					case static_cast <uint8_t> (net_t::type_t::NETWORK): break;
					// Если мы получили доменное имя
					case static_cast <uint8_t> (net_t::type_t::FQDN):
						// Устанавливаем доменное имя
						result.domain = result.host;
					break;
					// Если мы получили IP-адрес сети IPv4
					case static_cast <uint8_t> (net_t::type_t::IPV4): {
						// Устанавливаем IP-адрес
						result.ip = result.host;
						// Устанавливаем тип сети
						result.family = AF_INET;
					} break;
					// Если мы получили IP-адрес сети IPv6
					case static_cast <uint8_t> (net_t::type_t::IPV6): {
						// Устанавливаем IP-адрес
						result.ip = result.host;
						// Устанавливаем тип сети
						result.family = AF_INET6;
						// Если у хоста обнаружены скобки
						if((result.ip.front() == '[') && (result.ip.back() == ']'))
							// Удаляем скобки вокруг IP-адреса
							result.ip = result.ip.substr(1, result.ip.length() - 2);
					} break;
					// Если хост не распознан, устанавливаем его как есть
					default: result.domain = result.host;
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * etag Метод генерации ETag хэша текста
 * @param text текст для перевода в строку
 * @return     хэш etag
 */
string awh::URI::etag(const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!text.empty()){
		// Получаем sha1 хэш строки
		const string & sha1 = this->_fmk->hash(text, fmk_t::hash_t::SHA1);
		// Если строка получена
		if(!sha1.empty()){
			// Извлекаем первую часть хэша
			const string & first = sha1.substr(0, 8);
			// Извлекаем вторую часть хэша
			const string & second = sha1.substr(35);
			// Формируем результат
			result = this->_fmk->format("W/\"%s-%s\"", first.c_str(), second.c_str());
		}
	}
	// Выводим результат
	return result;
}
/**
 * encode Метод кодирования строки в URL-адресе
 * @param str строка для кодирования
 * @return    результат кодирования
 */
string awh::URI::encode(const string & str) const noexcept {
	// Результат работы функции
	string result = "";
	// Если строка передана
	if(!str.empty()){
		// Создаём поток
		ostringstream escaped;
		// Заполняем поток нулями
		escaped.fill('0');
		// Переключаемся на 16-ю систему счисления
		escaped << hex;
		// Перебираем все символы
		for(auto i = str.cbegin(), n = str.cend(); i != n; ++i){
			// Получаем символ
			string::value_type c = (* i);
			// Не трогаем буквенно-цифровые и другие допустимые символы.
			if(isalnum(c) || (c == '-') || (c == '_') || (c == '.') || (c == '~') || (c == '@') ||
			  ((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))){
				// Записываем в поток символ, как он есть
				escaped << c;
				// Пропускаем итерацию
				continue;
			}
			/**
			 * Любые другие символы закодированы в процентах
			 */
			// Переводим символы в верхний регистр
			escaped << uppercase;
			// Записываем в поток, код символа
			escaped << '%' << setw(2) << static_cast <uint32_t> (c);
			// Убираем верхний регистр
			escaped << nouppercase;
		}
		// Получаем результат
		result = escaped.str();
	}
	// Выводим результат
	return result;
}
/**
 * decode Метод декодирования строки в URL-адресе
 * @param str строка для декодирования
 * @return    результат декодирования
 */
string awh::URI::decode(const string & str) const noexcept {
	// Результат работы функции
	string result = "";
	// Если строка передана
	if(!str.empty()){
		// Символ строки
		char ch = 0;
		// Код символа
		int32_t ii = 0;
		// Переходим по всей длине строки
		for(size_t i = 0; i < str.length(); i++){
			// Если это не проценты
			if(str[i] != '%'){
				// Если это объединение двух слов
				if(str[i] == '+')
					// Выполняем добавление разделителя
					result.append(1, ' ');
				// Иначе копируем букву как она есть
				else result.append(1, str[i]);
			// Если же это проценты
			} else {
				// Копируем символ
				std::sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
				// Переводим символ в из числа в букву
				ch = static_cast <char> (ii);
				// Запоминаем полученный символ
				result.append(1, ch);
				// Смещаем итератор
				i = (i + 2);
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * url Метод создания строки URL-запросы из параметров
 * @param url параметры URL-запроса
 * @return    URL-запрос в виде строки
 */
string awh::URI::url(const url_t & url) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные получены
	if(!url.schema.empty() && !url.host.empty()){
		// Если схема протокола является unix
		if(this->_fmk->compare(url.schema, "unix")){
			// Получаем строку HTTP запроса
			const string & query = this->query(url);
			// Формируем адрес строки unix-сокета
			result = this->_fmk->format("%s:%s", url.schema.c_str(), query.c_str());
		// Иначе формируем URI адрес в обычном виде
		} else {
			// Хост URL-запроса и параметры авторизации
			string host = url.domain, auth = "";
			// Если хост не существует
			if(host.empty()){
				// Определяем тип хоста
				switch(url.family){
					// Если мы получили IPv4
					case AF_INET: host = url.ip; break;
					// Если мы получили IPv6
					case AF_INET6: host = this->_fmk->format("[%s]", url.ip.c_str()); break;
				}
			}
			// Если параметры авторизации указаны
			if(!url.pass.empty() || !url.user.empty())
				// Формируем параметры авторизации
				auth = (!url.pass.empty() ? this->_fmk->format("%s:%s@", url.user.c_str(), url.pass.c_str()) : this->_fmk->format("%s@", url.user.c_str()));
			// Порт сервера для URL-запроса
			uint32_t port = url.port;
			// Определяем указанный порт
			switch(port){
				// Если указан 25 порт
				case 25:
				// Если указан 587 порт
				case 587:
				// Если указан 465 порт
				case 465: {
					// Если схема принадлежит E-Mail
					if(this->_fmk->compare(url.schema, "mailto"))
						// Формируем URI адрес по умолчанию
						return this->_fmk->format("%s%s", auth.c_str(), host.c_str());
				} break;
				// Если указан 80 порт
				case 80: port = (this->_fmk->compare(url.schema, "http") || this->_fmk->compare(url.schema, "ws") ? 0 : url.port); break;
				// Если указан 443 порт
				case 443: port = (this->_fmk->compare(url.schema, "https") || this->_fmk->compare(url.schema, "wss") ? 0 : url.port); break;
			}
			// Получаем строку HTTP запроса
			const string & query = this->query(url);
			// Если порт не установлен
			if(port == 0)
				// Формируем URL строку запроса без порта
				result = this->_fmk->format("%s://%s%s%s", url.schema.c_str(), auth.c_str(), host.c_str(), query.c_str());
			// Если порт установлен, формируем URL строку запроса с указанием порта
			else result = this->_fmk->format("%s://%s%s:%u%s", url.schema.c_str(), auth.c_str(), host.c_str(), url.port, query.c_str());
		}
	}
	// Выводим результат
	return result;
}
/**
 * query Метод создания строки запроса из параметров
 * @param url параметры URL-запроса
 * @return    URL-запрос в виде строки
 */
string awh::URI::query(const url_t & url) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные получены
	if(!url.empty()){
		// Выполняем сборку пути запроса
		const string & path = this->joinPath(url.path);
		// Выполняем сборку параметров запроса
		const string & params = this->joinParams(url.params);
		// Выполняем сборку якоря запроса
		const string anchor = (!url.anchor.empty() ? this->_fmk->format("#%s", url.anchor.c_str()) : "");
		// Выполняем генерацию URL адреса
		const string uri = ((url.callback != nullptr) ? this->_fmk->format("&%s", url.callback(&url, this).c_str()) : "");
		// Иначе порт не устанавливаем
		result = this->_fmk->format("%s%s%s%s", path.c_str(), params.c_str(), uri.c_str(), anchor.c_str());
	}
	// Выводим результат
	return result;
}
/**
 * origin Метод создания заголовка [origin], для HTTP запроса
 * @param url параметры URL-запроса
 * @return    заголовок [origin]
 */
string awh::URI::origin(const url_t & url) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные получены
	if(!url.schema.empty() && !url.host.empty()){
		// Хост URL-запроса
		string host = url.domain;
		// Если IP-адрес существует
		if(host.empty()){
			// Определяем тип хоста
			switch(url.family){
				// Если мы получили IPv4
				case AF_INET: host = url.ip; break;
				// Если мы получили IPv6
				case AF_INET6: host = this->_fmk->format("[%s]", url.ip.c_str()); break;
			}
		}
		// Порт сервера для URL-запроса
		uint32_t port = url.port;
		// Определяем указанный порт
		switch(port){
			// Если указан 25 порт
			case 25:
			// Если указан 587 порт
			case 587:
			// Если указан 465 порт
			case 465: {
				// Если пользователь установлен и схема принадлежит E-Mail
				if(!url.user.empty() && this->_fmk->compare(url.schema, "mailto"))
					// Формируем URI адрес по умолчанию
					return this->_fmk->format("%s@%s", url.user.c_str(), host.c_str());
			} break;
			// Если указан 80 порт
			case 80: port = (this->_fmk->compare(url.schema, "http") || this->_fmk->compare(url.schema, "ws") ? 0 : port); break;
			// Если указан 443 порт
			case 443: port = (this->_fmk->compare(url.schema, "https") || this->_fmk->compare(url.schema, "wss") ? 0 : port); break;
		}
		// Если порт установлен
		if(port > 0)
			// Выполняем формирование URL адреса
			result = this->_fmk->format("%s://%s:%u", url.schema.c_str(), host.c_str(), port);
		// Иначе порт не устанавливаем
		else result = this->_fmk->format("%s://%s", url.schema.c_str(), host.c_str());
	}
	// Выводим результат
	return result;
}
/**
 * create Метод создания полного адреса
 * @param dest адрес места назначения
 * @param src  исходный адрес для объединения
 */
void awh::URI::create(url_t & dest, const url_t & src) const noexcept {
	// Если доменное доменное имя или IP-адрес установлены
	if(!src.domain.empty() || !src.ip.empty()){
		// Если IP-адрес не установлен
		if(dest.ip.empty())
			// Выполняем устновку IP-адреса
			dest.ip = src.ip;
		// Если хост не установлен
		if(dest.host.empty())
			// Выполняем установку хоста
			dest.host = src.host;
		// Если порт передан
		if((dest.port == 0) || (src.port != 0))
			// Выполняем установку порта
			dest.port = src.port;
		// Если доменное имя не установлено
		if(dest.domain.empty())
			// Выполняем установку доменного имени
			dest.domain = src.domain;
		// Если схема протокола не установлена
		if(dest.schema.empty() || !src.schema.empty())
			// Выполняем установку схемы протокола
			dest.schema = src.schema;
		// Если IP-адрес установлен
		if(!src.ip.empty())
			// Выполняем установку семейство протоколов
			dest.family = src.family;
	}
	// Если якорь установлен
	if(dest.anchor.empty() && !src.anchor.empty())
		// Выполняем установку якоря
		dest.anchor = src.anchor;
	// Если логин пользователя указан
	if(!src.user.empty()){
		// Выполняем установку логина пользователя
		dest.user = src.user;
		// Выполняем установку пароля пользователя
		dest.pass = src.pass;
	}
	// Если путь запроса указан
	if(dest.path.empty() && !src.path.empty())
		// Выполняем установку пути запроса
		dest.path.assign(src.path.begin(), src.path.end());
	// Если параметры запроса указаны
	if(dest.params.empty() && !src.params.empty())
		// Выполняем установку параметров запроса
		dest.params.assign(src.params.begin(), src.params.end());
}
/**
 * combine Метод комбинации двух адресов
 * @param dest адрес места назначения
 * @param src  исходный адрес для объединения
 */
void awh::URI::combine(url_t & dest, const url_t & src) const noexcept {
	// Если доменное доменное имя или IP-адрес установлены
	if(!src.domain.empty() || !src.ip.empty()){
		// Выполняем устновку IP-адреса
		dest.ip = src.ip;
		// Выполняем установку хоста
		dest.host = src.host;
		// Выполняем установку порта
		dest.port = src.port;
		// Выполняем установку доменного имени
		dest.domain = src.domain;
		// Выполняем установку схемы протокола
		dest.schema = src.schema;
		// Выполняем установку семейство протоколов
		dest.family = src.family;
	}
	// Если якорь установлен
	if(!src.anchor.empty())
		// Выполняем установку якоря
		dest.anchor = src.anchor;
	// Выполняем очистку якоря запроса
	else dest.anchor.clear();
	// Если логин пользователя указан
	if(!src.user.empty()){
		// Выполняем установку логина пользователя
		dest.user = src.user;
		// Выполняем установку пароля пользователя
		dest.pass = src.pass;
	// Если логин пользователя не указан
	} else {
		// Выполняем очистку логина пользователя
		dest.user.clear();
		// Выполняем очистку пароля пользователя
		dest.pass.clear();
	}
	// Если путь запроса указан
	if(!src.path.empty())
		// Выполняем установку пути запроса
		dest.path.assign(src.path.begin(), src.path.end());
	// Выполняем очистку пути запроса
	else dest.path.clear();
	// Если параметры запроса указаны
	if(!src.params.empty())
		// Выполняем установку параметров запроса
		dest.params.assign(src.params.begin(), src.params.end());
	// Выполняем очистку параметров запроса
	else dest.params.clear();
}
/**
 * append Метод добавления к URL адресу параметров запроса
 * @param url    параметры URL-запроса
 * @param params параметры для добавления
 */
void awh::URI::append(url_t & url, const string & params) const noexcept {
	// Если данные переданы
	if(!url.empty() && !params.empty()){
		// Выполняем парсинг URL адреса
		const auto & uri = this->split(params);
		// Если сплит URL адреса, прошёл успешно
		if(!uri.empty()){
			// Выполняем поиск пути запроса
			auto i = uri.find(flag_t::PATH);
			// Если путь запроса получен
			if((i != uri.end()) && (i->second.compare("/") != 0)){
				// Выполняем извлечение пути запроса
				url.path = this->splitPath(i->second);
				// Если схема протокола принадлежит unix-сокету
				if(this->_fmk->compare(url.schema, "unix"))
					// Устанавливаем доменное имя
					url.host = std::forward <const string> (i->second);
			}
			// Выполняем поиск параметров запроса
			i = uri.find(flag_t::PARAMS);
			// Если параметры запроса получены
			if(i != uri.end())
				// Выполняем извлечение параметров запроса
				url.params = this->splitParams(i->second);
			// Выполняем поиск якоря запроса
			i = uri.find(flag_t::ANCHOR);
			// Если якорь запроса получен
			if(i != uri.end())
				// Выполняем извлечение якоря запроса
				url.anchor = std::forward <const string> (i->second);
		}
	}
}
/**
 * concat Объединение двух адресов путём создания третьего
 * @param dest адрес назначения
 * @param src  исходный адрес для объединения
 * @return     результирующий адрес
 */
awh::URI::URL awh::URI::concat(const url_t & dest, const url_t & src) const noexcept {
	// Результат работы функции
	url_t result = dest;
	// Выполняем объединение двух адресов
	this->combine(result, src);
	// Выводим результат
	return result;
}
/**
 * split Метод сплита URI на составные части
 * @param uri строка URI для сплита
 * @return    список полученных частей URI
 */
std::map <awh::URI::flag_t, string> awh::URI::split(const string & uri) const noexcept {
	// Результат работы функции
	std::map <flag_t, string> result;
	// Если URI передан
	if(!uri.empty()){
		// Выполняем проверку строки URI для сплита
		const auto & match = this->_regexp.exec(this->_fmk->replace(uri, " ", "%20"), this->_uri);
		// Если результат получен
		if(!match.empty()){
			// Если данные пришли в правильном формате
			if(!match[4].empty()){
				// Переходим по всему списку полученных данных
				for(size_t i = 0; i < match.size(); i++){
					// Если запись получена
					if(!match[i].empty()){
						// Определяем тип записи
						switch(i){
							// Если типом записи является доменным именем
							case 4: result.emplace(flag_t::HOST, match[i]); break;
							// Если типом записи является путём запроса
							case 5: result.emplace(flag_t::PATH, match[i]); break;
							// Если типом записи является параметрами запроса
							case 7: result.emplace(flag_t::PARAMS, match[i]); break;
							// Если типом записи является якорем запроса
							case 9: result.emplace(flag_t::ANCHOR, match[i]); break;
							// Если типом записи является протокол
							case 2: result.emplace(flag_t::SCHEMA, this->_fmk->transform(match[i], fmk_t::transform_t::LOWER)); break;
						}
					}
				}
				// Выполняем поиск хоста
				auto i = result.find(flag_t::HOST);
				// Если хост запроса найден
				if(i != result.end()){
					// Выполняем поиск разделителя порта
					const size_t pos = i->second.rfind(":");
					// Если разделитель порта найден
					if(pos != string::npos){
						// Получаем данные порта
						const string & port = i->second.substr(pos + 1);
						// Если данные являются портом
						if(this->_fmk->is(port, fmk_t::check_t::NUMBER)){
							// Устанавливаем данные порта
							result.emplace(flag_t::PORT, port);
							// Формируем правильный хост
							i->second = i->second.substr(0, pos);
						}
					}
				}
			// Если данные пришли в неправильном формате
			} else {
				// Переходим по всему списку полученных данных
				for(size_t i = 0; i < match.size(); i++){
					// Если запись получена
					if(!match[i].empty()){
						// Если мы обрабатываем путь запроса
						if(i == 5){
							// Если данные являются портом
							if(this->_fmk->is(match[i], fmk_t::check_t::NUMBER)){
								// Устанавливаем данные порта
								result.emplace(flag_t::PORT, match[i]);
								// Выходим из цикла
								break;
							// Если мы получили URL адрес
							} else if(this->_regexp.test(match[i], this->_email)) {
								// Устанавливаем порт по умолчанию
								result.emplace(flag_t::PORT, "25");
								// Устанавливаем схему протокола
								result.emplace(flag_t::SCHEMA, "mailto");
								// Устанавливаем тип хоста
								result.emplace(flag_t::HOST, match[i]);
								// Выходим из цикла
								break;
							}
						}
						// Определяем тип записи
						switch(i){
							// Если типом записи является доменным именем
							case 2: result.emplace(flag_t::HOST, match[i]); break;
							// Если типом записи является путём запроса
							case 5: result.emplace(flag_t::PATH, match[i]); break;
							// Если типом записи является параметрами запроса
							case 7: result.emplace(flag_t::PARAMS, match[i]); break;
							// Если типом записи является якорем запроса
							case 9: result.emplace(flag_t::ANCHOR, match[i]); break;
						}
					}
				}
				// Выполняем поиск пути запроса
				auto i = result.find(flag_t::PATH);
				// Если путь запроса найден
				if(i != result.end()){
					// Выполняем поиск разделителя пути
					const size_t pos = i->second.find("/");
					// Если разделитель пути найден
					if(pos != string::npos){
						// Если позиция не нулевая и порт является числом
						if(pos > 0){
							// Получаем данные хоста или порта
							const string & data = i->second.substr(0, pos);
							// Если данные являются портом
							if(this->_fmk->is(data, fmk_t::check_t::NUMBER))
								// Устанавливаем данные порта
								result.emplace(flag_t::PORT, data);
							// Иначе если хост не установлен
							else if(result.count(flag_t::HOST) < 1) {
								// Определяем тип домена
								switch(static_cast <uint8_t> (this->_net.host(data))){
									// Если мы получили IP-адрес сети IPv4
									case static_cast <uint8_t> (net_t::type_t::IPV4):
									// Если мы получили IP-адрес сети IPv6
									case static_cast <uint8_t> (net_t::type_t::IPV6):
										// Устанавливаем результат хоста
										result.emplace(flag_t::HOST, data);
									break;
									// Если мы получили доменное имя
									case static_cast <uint8_t> (net_t::type_t::FQDN):
										// Устанавливаем результат хоста
										result.emplace(flag_t::HOST, this->_fmk->transform(data, fmk_t::transform_t::LOWER));
									break;
								}
							}
						}
						// Формируем правильный путь запроса
						i->second = i->second.substr(pos);
					}
				}
			}{
				// Выполняем поиск протокола запроса
				auto i = result.find(flag_t::SCHEMA);
				// Если протокол не обнаружен
				if(i == result.end())
					// Устанавливаем протокол запроса
					result.emplace(flag_t::SCHEMA, "http");
			}{
				// Выполняем поиск пути запроса
				auto i = result.find(flag_t::PATH);
				// Если путь запроса не обнаружен
				if(i == result.end())
					// Устанавливаем протокол запроса
					result.emplace(flag_t::PATH, "/");
			}{
				// Выполняем поиск хоста
				auto i = result.find(flag_t::HOST);
				// Если хост запроса обнаружен
				if((i != result.end()) && this->_fmk->compare(i->second, "unix")){
					// Извлекаем путь запроса
					const string & path = result.at(flag_t::PATH);
					// Если в пути найдено расширение
					if(path.rfind(".") != string::npos){
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "0");
						// Устанавливаем схему протокола
						result.at(flag_t::SCHEMA) = i->second;
						// Заменяем хост, на путь сокета в файловой системе
						result.at(flag_t::HOST) = path;
					}
				}
			}{
				// Выполняем поиск порта
				auto i = result.find(flag_t::PORT);
				// Если порт не найден
				if(i == result.end()){
					// Получаем схему протокола интернета
					const string & schema = result.at(flag_t::SCHEMA);
					// Если протокол является HTTPS
					if(this->_fmk->compare(schema, "https"))
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "443");
					// Если протокол является HTTP
					else if(this->_fmk->compare(schema, "http"))
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "80");
					// Если протокол является WSS
					else if(this->_fmk->compare(schema, "wss"))
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "443");
					// Если протокол является WS
					else if(this->_fmk->compare(schema, "ws"))
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "80");
					// Если протокол является FTP
					else if(this->_fmk->compare(schema, "ftp"))
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "21");
					// Если протокол является MQTT
					else if(this->_fmk->compare(schema, "mqtt"))
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "1883");
					// Если протокол является REDIS
					else if(this->_fmk->compare(schema, "redis"))
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "6379");
					// Если протокол является Socks5
					else if(this->_fmk->compare(schema, "socks5"))
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "1080");
					// Если протокол является PostgreSQL
					else if(this->_fmk->compare(schema, "postgresql"))
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "5432");
					// Иначе устанавливаем порт открытого HTTP протокола
					else result.emplace(flag_t::PORT, "80");
				// Если порт установлен как 443
				} else if(this->_fmk->compare(i->second, "443")) {
					// Если протокол является HTTP
					if(this->_fmk->compare(result.at(flag_t::SCHEMA), "http"))
						// Устанавливаем протокол
						result.at(flag_t::SCHEMA) = "https";
				}
			}{
				// Выполняем поиск хоста
				auto i = result.find(flag_t::HOST);
				// Если хост запроса найден
				if(i != result.end()){
					// Выполняем поиск разделителя данных пользователя и хоста
					size_t pos = i->second.rfind('@');
					// Если разделитель порта найден
					if(pos != string::npos){
						// Получаем данные пользователя
						const string user = i->second.substr(0, pos);
						// Формируем правильный хост
						i->second = this->_fmk->transform(i->second.substr(pos + 1), fmk_t::transform_t::LOWER);
						// Выполняем поиск разделителя логина и пароля
						pos = user.find(':');
						// Если разделитель логина и пароля найден
						if(pos != string::npos){
							// Устанавливаем пароль пользователя
							result.emplace(flag_t::PASS, user.substr(pos + 1));
							// Устанавливаем логин пользователя
							result.emplace(flag_t::LOGIN, user.substr(0, pos));
						// Устанавливаем данные пользователя как они есть
						} else result.emplace(flag_t::LOGIN, std::forward <const string> (user));
					// Переводим название хоста в нижний регистр
					} else i->second = this->_fmk->transform(i->second, fmk_t::transform_t::LOWER);
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * splitParams Метод выполнения сплита параметров URI
 * @param uri строка URI для сплита
 * @return    параметры полученные при сплите
 */
vector <std::pair <string, string>> awh::URI::splitParams(const string & uri) const noexcept {
	// Результат работы функции
	vector <std::pair <string, string>> result;
	// Если URI передано
	if(!uri.empty()){
		// Параметры URI
		vector <wstring> params;
		// Если параметры получены
		if(!this->_fmk->split(this->_fmk->convert(uri), L"&", params).empty()){
			// Данные параметров
			vector <wstring> data;
			// Переходим по всему списку параметров
			for(auto & param : params){
				// Выполняем сплит данных URI параметров
				this->_fmk->split(param, L"=", data);
				// Если данные получены
				if(data.size() == 2)
					// Добавляем полученные данные
					result.push_back(
						std::make_pair(
							this->decode(this->_fmk->convert(data.front())),
							this->decode(this->_fmk->convert(data.back()))
						)
					);
				// Если значения параметр не имеет
				else if(data.size() == 1)
					// Добавляем полученные данные
					result.push_back(std::make_pair(this->decode(this->_fmk->convert(data.front())), ""));
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * splitPath Метод выполнения сплита пути
 * @param path  путь для выполнения сплита
 * @param delim сепаратор-разделитель для сплита
 * @return      список параметров пути
 */
vector <string> awh::URI::splitPath(const string & path, const string & delim) const noexcept {
	// Результат работы функции
	vector <string> result;
	// Если данные переданы
	if(!path.empty() && !delim.empty()){
		// Параметры пути
		vector <wstring> params;
		// Выполняем сплит параметров пути
		if(!this->_fmk->split(this->_fmk->convert(path.front() == '/' ? path.substr(1) : path), this->_fmk->convert(delim), params).empty()){
			// Переходим по всему списку параметров
			for(auto & param : params){
				// Выполняем конвертирование полученной строки
				const string & item = this->_fmk->convert(param);
				// Если в адресе не найдена точка
				if(item.front() != '.')
					// Добавляем в список наши параметры
					result.push_back(this->decode(item));
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * joinParams Метод сборки параметров URI
 * @param uri параметры URI для сборки
 * @return    строка полученная при сборке параметров URI
 */
string awh::URI::joinParams(const vector <std::pair <string, string>> & uri) const noexcept {
	// Результат работы функции
	string result = "";
	// Если параметры URI переданы
	if(!uri.empty()){
		// Добавляем системный параметр
		result.append(1, '?');
		// Переходим по всему списку параметров
		for(auto & param : uri){
			// Если параметры уже были добавлены
			if(result.length() > 1)
				// Выполняем добавление разделителя
				result.append(1, '&');
			// Если значение не пустое
			if(!param.second.empty())
				// Добавляем собранные параметры
				result.append(this->_fmk->format("%s=%s", this->encode(param.first).c_str(), this->encode(param.second).c_str()));
			// Если значение пустое, проставляем как есть
			else result.append(this->_fmk->format("%s", this->encode(param.first).c_str()));
		}
	}
	// Выводим результат
	return result;
}
/**
 * joinPath Метод сборки пути запроса
 * @param path  список параметров пути запроса
 * @param delim сепаратор-разделитель для сплита
 * @return      строка собранного пути
 */
string awh::URI::joinPath(const vector <string> & path, const string & delim) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные переданы
	if(!path.empty() && !delim.empty()){
		// Переходим по всему списку параметров
		for(auto & param : path){
			// Добавляем разделитель
			result.append(delim);
			// Если параметр существует
			if(!param.empty())
				// Добавляем собранные параметры
				result.append(this->encode(param));
		}
	// Выводим путь из одного разделителя
	} else result = delim;
	// Выводим результат
	return result;
}
/**
 * params Метод получения параметров URI
 * @param uri    URI для получения параметров
 * @param schema протокол передачи данных
 * @return       параметры полученные из URI
 */
awh::URI::params_t awh::URI::params(const string & uri, const string & schema) const noexcept {
	// Результат работы функции
	params_t result;
	// Если URI передан
	if(!uri.empty()){
		// Выполняем проверку URI для получения параметров
		const auto & match = this->_regexp.exec(uri, this->_params);
		// Если результат получен
		if(!match.empty()){
			// Получаем пользователя
			result.user = match[1];
			// Получаем пароль пользователя
			result.pass = match[2];
			// Получаем хост запроса
			result.host = this->_fmk->transform(match[3], fmk_t::transform_t::LOWER);
			// Получаем порт запроса
			const string & port = match[4];
			// Если порт получен
			if(!port.empty())
				// Выполняем установку порта
				result.port = ::stoi(port);
			// Если порт не получен но указана схема
			else if(!schema.empty()) {
				// Если схема принадлежит зашифрованному HTTP серверу
				if(this->_fmk->compare(schema, "https"))
					// Выполняем установку порта по умолчанию
					result.port = 443;
				// Если схема принадлежит не зашифрованному HTTP серверу
				else if(this->_fmk->compare(schema, "http"))
					// Выполняем установку порта по умолчанию
					result.port = 80;
				// Если схема принадлежит зашифрованному WebSocket серверу
				else if(this->_fmk->compare(schema, "wss"))
					// Выполняем установку порта по умолчанию
					result.port = 443;
				// Если схема принадлежит не зашифрованному WebSocket серверу
				else if(this->_fmk->compare(schema, "ws"))
					// Выполняем установку порта по умолчанию
					result.port = 80;
				// Если схема принадлежит FTP серверу
				else if(this->_fmk->compare(schema, "ftp"))
					// Выполняем установку порта по умолчанию
					result.port = 21;
				// Если схема принадлежит MQTT брокеру сообщений
				else if(this->_fmk->compare(schema, "mqtt"))
					// Выполняем установку порта по умолчанию
					result.port = 1883;
				// Если схема принадлежит адресу электронной почты
				else if(this->_fmk->compare(schema, "mailto"))
					// Выполняем установку порта по умолчанию
					result.port = 25;
				// Если схема принадлежит Redis базе данных
				else if(this->_fmk->compare(schema, "redis"))
					// Выполняем установку порта по умолчанию
					result.port = 6379;
				// Если схема принадлежит SOCKS5 прокси серверу
				else if(this->_fmk->compare(schema, "socks5"))
					// Выполняем установку порта по умолчанию
					result.port = 1080;
				// Если схема принадлежит PostgreSQL базе данных
				else if(this->_fmk->compare(schema, "postgresql"))
					// Выполняем установку порта по умолчанию
					result.port = 5432;
			// Устанавливаем порт по умолчанию
			} else result.port = 80;
			// Если пароль получен а пользователь нет
			if(!result.pass.empty() && result.user.empty()){
				// Устанавливаем пользователя
				result.user = result.pass;
				// Удаляем пароль
				result.pass.clear();
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * Оператор [=] получения параметров URL-запроса
 * @param url строка URL-запроса для получения параметров
 * @return    параметры URL-запроса
 */
awh::URI::url_t awh::URI::operator = (const string & url) const noexcept {
	// Выполняем парсинг URL-запроса
	return this->parse(url);
}
/**
 * Оператор [=] создания строки URL-запросы из параметров
 * @param url параметры URL-запроса
 * @return    URL-запрос в виде строки
 */
string awh::URI::operator = (const url_t & url) const noexcept {
	// Выполняем сборку URL-адреса запросов
	return this->url(url);
}
/**
 * URI Конструктор
 * @param fmk объект фреймворка
 */
awh::URI::URI(const fmk_t * fmk) noexcept : _fmk(fmk) {
	// Устанавливаем регулярное выражение для парсинга URI
	this->_uri = this->_regexp.build("^(([^:/?#]+):)?(\\/\\/([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?", {regexp_t::option_t::UTF8, regexp_t::option_t::CASELESS});
	// Устанавливаем регулярное выражение для парсинга E-Mail
	this->_email = this->_regexp.build(
		"((?:([\\w\\-абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
		"]+)\\@)(\\[(?:\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]|(?:\\d{1,3}(?:\\.\\d{1,3}){3})|(?:(?:xn\\-\\-[\\w\\d]+\\.){0,100}(?:xn\\-\\-[\\w\\d]+)|(?:[\\w\\-"
		"абвгдеёжзийклмнопрстуфхцчшщъыьэюя]+\\.){0,100}[\\w\\-абвгдеёжзийклмнопрстуфхцчшщъыьэюя]+)\\.(xn\\-\\-[\\w\\d]+|[a-zабвгдеёжзийклмнопрстуфхцчшщъыьэюя]+)))", {regexp_t::option_t::UTF8, regexp_t::option_t::CASELESS,}
	);
	// Устанавливаем регулярное выражение для парсинга параметров
	this->_params = this->_regexp.build("^(?:(?:(.+)\\:)?(?:(.+)\\@))?((?:[^\\:]+|(?:\\[?(?:\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]?)))(?:\\:(\\d+))?$", {regexp_t::option_t::UTF8, regexp_t::option_t::CASELESS});
}
/**
 * Оператор [<<] вывода в поток IP адреса
 * @param os  поток куда нужно вывести данные
 * @param url параметры URL-запроса
 */
ostream & awh::operator << (ostream & os, const uri_t::url_t & url) noexcept {
	// Выполняем создание объекта фреймворка
	fmk_t fmk{};
	// Выполняем создание объекта для работы с URI
	uri_t uri(&fmk);
	// Записываем в поток сгенерированный URL-адрес
	os << (uri = url);
	// Выводим результат
	return os;
}
