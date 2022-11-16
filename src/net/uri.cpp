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
	// Зануляем функцию генерации цифровой подписи запроса
	this->sign = nullptr;
	// Выполняем сброс протокола интернета AF_INET или AF_INET6
	this->family = AF_INET;
	// Выполняем очистку IP адреса
	this->ip.clear();
	// Выполняем очистку хоста сервера
	this->host.clear();
	// Выполняем очистку имени пользователя
	this->user.clear();
	// Выполняем очистку пароля пользователя
	this->pass.clear();
	// Выполняем очистку пути URL запроса
	this->path.clear();
	// Выполняем очистку доменного имени
	this->domain.clear();
	// Выполняем очистку протокола передачи данных
	this->schema.clear();
	// Выполняем очистку якоря URL запроса
	this->anchor.clear();
	// Выполняем очистку параметров URL запроса
	this->params.clear();
}
/**
 * empty Метод проверки на существование данных
 * @return результат проверки
 */
bool awh::URI::URL::empty() const noexcept {
	// Выполняем проверку на существование данных
	return (this->schema.empty() && this->host.empty() && this->ip.empty() && this->domain.empty());
}
/**
 * parse Метод получения параметров URL запроса
 * @param url строка URL запроса для получения параметров
 * @return    параметры URL запроса
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
			auto it = uri.find(flag_t::SCHEMA);
			// Если схема протокола получена
			if(it != uri.end())
				// Выполняем извлечение схемы протокола
				result.schema = std::forward <const string> (it->second);
			// Выполняем поиск пути запроса
			it = uri.find(flag_t::PATH);
			// Если путь запроса получен
			if(it != uri.end()){
				// Выполняем извлечение пути запроса
				result.path = this->splitPath(it->second);
				// Если схема протокола принадлежит unix-сокету
				if(result.schema.compare("unix") == 0)
					// Устанавливаем доменное имя
					result.host = std::forward <const string> (it->second);
			}
			// Выполняем поиск параметров запроса
			it = uri.find(flag_t::PARAMS);
			// Если параметры запроса получены
			if(it != uri.end())
				// Выполняем извлечение параметров запроса
				result.params = this->splitParams(it->second);
			// Выполняем поиск якоря запроса
			it = uri.find(flag_t::ANCHOR);
			// Если якорь запроса получен
			if(it != uri.end())
				// Выполняем извлечение якоря запроса
				result.anchor = std::forward <const string> (it->second);
			// Выполняем поиск порта запроса
			it = uri.find(flag_t::PORT);
			// Если порт запроса получен
			if(it != uri.end())
				// Выполняем извлечение порта запроса
				result.port = stoi(it->second);
			// Выполняем поиск пользователя запроса
			it = uri.find(flag_t::LOGIN);
			// Если пользователь запроса получен
			if(it != uri.end())
				// Выполняем извлечение пользователя запроса
				result.user = std::forward <const string> (it->second);
			// Выполняем поиск пароля пользователя запроса
			it = uri.find(flag_t::PASS);
			// Если пароль пользователя запроса получен
			if(it != uri.end())
				// Выполняем извлечение пароля пользователя запроса
				result.pass = std::forward <const string> (it->second);
			// Выполняем поиск хоста запроса
			it = uri.find(flag_t::HOST);
			// Если хост запроса получен
			if(it != uri.end()){
				// Выполняем извлечение хоста запроса
				result.host = std::forward <const string> (it->second);
				// Определяем тип домена
				switch(this->_nwk->checkNetworkByIp(result.host)){
					// Если - это доменное имя
					case 0: result.domain = result.host; break;
					// Если - это IP адрес сети v4
					case 4: {
						// Устанавливаем IP адрес
						result.ip = result.host;
						// Устанавливаем тип сети
						result.family = AF_INET;
					} break;
					// Если - это IP адрес сети v6
					case 6: {
						// Устанавливаем IP адрес
						result.ip = result.host;
						// Устанавливаем тип сети
						result.family = AF_INET6;
						// Если у хоста обнаружены скобки
						if((result.ip.front() == '[') && (result.ip.back() == ']'))
							// Удаляем скобки вокруг IP адреса
							result.ip = result.ip.substr(1, result.ip.length() - 2);
					} break;
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
		const string & sha1 = this->_fmk->sha1(text);
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
 * encode Метод кодирования строки в url адресе
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
			escaped << '%' << setw(2) << int((u_char) c);
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
 * decode Метод декодирования строки в url адресе
 * @param str строка для декодирования
 * @return    результат декодирования
 */
string awh::URI::decode(const string & str) const noexcept {
	// Результат работы функции
	string result = "";
	// Если строка передана
	if(!str.empty()){
		// Код символа
		int ii;
		// Символ строки
		char ch;
		// Переходим по всей длине строки
		for(size_t i = 0; i < str.length(); i++){
			// Если это не проценты
			if(str[i] != '%'){
				// Если это объединение двух слов
				if(str[i] == '+') result.append(1, ' ');
				// Иначе копируем букву как она есть
				else result.append(1, str[i]);
			// Если же это проценты
			} else {
				// Копируем символ
				sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
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
 * url Метод создания строки URL запросы из параметров
 * @param url параметры URL запроса
 * @return    URL запрос в виде строки
 */
string awh::URI::url(const url_t & url) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные получены
	if(!url.schema.empty() && !url.host.empty()){
		// Если схема протокола является unix
		if(url.schema.compare("unix") == 0){
			// Получаем строку HTTP запроса
			const string & query = this->query(url);
			// Формируем адрес строки unix-сокета
			result = this->_fmk->format("%s:%s", url.schema.c_str(), query.c_str());
		// Иначе формируем URI адрес в обычном виде
		} else {
			// Порт сервера для URL запроса
			u_int port = 0;
			// Хост URL запроса и параметры авторизации
			string host = url.domain, auth = "";
			// Если хост не существует
			if(host.empty()){
				// Определяем тип хоста
				switch(url.family){
					// Если - это IPv4
					case AF_INET: host = url.ip; break;
					// Если - это IPv6
					case AF_INET6: host = this->_fmk->format("[%s]", url.ip.c_str()); break;
				}
			}
			// Если параметры авторизации указаны
			if(!url.pass.empty() || !url.user.empty())
				// Формируем параметры авторизации
				auth = (!url.pass.empty() ? this->_fmk->format("%s:%s@", url.user.c_str(), url.pass.c_str()) : this->_fmk->format("%s@", url.user.c_str()));
			// Если порт был указан
			if(url.port > 0){
				// Определяем указанный порт
				switch(url.port){
					// Если указан 80 порт
					case 80: port = (url.schema.compare("http") == 0 || url.schema.compare("ws") == 0 ? 0 : url.port); break;
					// Если указан 443 порт
					case 443: port = (url.schema.compare("https") == 0 || url.schema.compare("wss") == 0 ? 0 : url.port); break;
					// Если - это, любой другой порт
					default: port = url.port;
				}
			}
			// Получаем строку HTTP запроса
			const string & query = this->query(url);
			// Если порт не установлен, формируем URL строку запроса без порта
			if(port == 0) result = this->_fmk->format("%s://%s%s%s", url.schema.c_str(), auth.c_str(), host.c_str(), query.c_str());
			// Если порт установлен, формируем URL строку запроса с указанием порта
			else result = this->_fmk->format("%s://%s%s:%u%s", url.schema.c_str(), auth.c_str(), host.c_str(), url.port, query.c_str());
		}
	}
	// Выводим результат
	return result;
}
/**
 * query Метод создания строки запроса из параметров
 * @param url параметры URL запроса
 * @return    URL запрос в виде строки
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
		const string & anchor = (!url.anchor.empty() ? this->_fmk->format("#%s", url.anchor.c_str()) : "");
		// Выполняем генерацию сигнатуры
		const string signature = ((url.sign != nullptr) ? this->_fmk->format("&%s", url.sign(&url, this).c_str()) : "");
		// Иначе порт не устанавливаем
		result = this->_fmk->format("%s%s%s%s", path.c_str(), params.c_str(), signature.c_str(), anchor.c_str());
	}
	// Выводим результат
	return result;
}
/**
 * origin Метод создания заголовка [origin], для HTTP запроса
 * @param url параметры URL запроса
 * @return    заголовок [origin]
 */
string awh::URI::origin(const url_t & url) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные получены
	if(!url.schema.empty() && !url.host.empty()){
		// Хост URL запроса
		string host = url.domain;
		// Если IP адрес существует
		if(host.empty()){
			// Определяем тип хоста
			switch(url.family){
				// Если - это IPv4
				case AF_INET: host = url.ip; break;
				// Если - это IPv6
				case AF_INET6: host = this->_fmk->format("[%s]", url.ip.c_str()); break;
			}
		}
		// Порт сервера для URL запроса
		u_int port = url.port;
		// Определяем указанный порт
		switch(port){
			// Если указан 80 порт
			case 80: port = ((url.schema.compare("http") == 0) || (url.schema.compare("ws") == 0) ? 0 : port); break;
			// Если указан 443 порт
			case 443: port = ((url.schema.compare("https") == 0) || (url.schema.compare("wss") == 0) ? 0 : port); break;
		}
		// Выполняем формирование URL адреса
		if(port > 0) result = this->_fmk->format("%s://%s:%u", url.schema.c_str(), host.c_str(), port);
		// Иначе порт не устанавливаем
		else result = this->_fmk->format("%s://%s", url.schema.c_str(), host.c_str());
	}
	// Выводим результат
	return result;
}
/**
 * append Метод добавления к URL адресу параметров запроса
 * @param url    параметры URL запроса
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
			auto it = uri.find(flag_t::PATH);
			// Если путь запроса получен
			if((it != uri.end()) && (it->second.compare("/") != 0)){
				// Выполняем извлечение пути запроса
				url.path = this->splitPath(it->second);
				// Если схема протокола принадлежит unix-сокету
				if(url.schema.compare("unix") == 0)
					// Устанавливаем доменное имя
					url.host = std::forward <const string> (it->second);
			}
			// Выполняем поиск параметров запроса
			it = uri.find(flag_t::PARAMS);
			// Если параметры запроса получены
			if(it != uri.end())
				// Выполняем извлечение параметров запроса
				url.params = this->splitParams(it->second);
			// Выполняем поиск якоря запроса
			it = uri.find(flag_t::ANCHOR);
			// Если якорь запроса получен
			if(it != uri.end())
				// Выполняем извлечение якоря запроса
				url.anchor = std::forward <const string> (it->second);
		}
	}
}
/**
 * split Метод сплита URI на составные части
 * @param uri строка URI для сплита
 * @return    список полученных частей URI
 */
map <awh::URI::flag_t, string> awh::URI::split(const string & uri) const noexcept {
	// Результат работы функции
	map <flag_t, string> result;
	// Если URI передан
	if(!uri.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^(([^:/?#]+):)?(\\/\\/([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?", regex::ECMAScript | regex::icase);
		// Выполняем поиск ip адреса и префикса сети
		regex_search(uri, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Если данные пришли в правильном формате
			if(!match[4].str().empty()){
				// Переходим по всему списку полученных данных
				for(size_t i = 0; i < match.size(); i++){
					// Если запись получена
					if(!match[i].str().empty()){
						// Определяем тип записи
						switch(i){
							// Если типом записи является путём запроса
							case 5: result.emplace(flag_t::PATH, match[i].str()); break;
							// Если типом записи является параметрами запроса
							case 7: result.emplace(flag_t::PARAMS, match[i].str()); break;
							// Если типом записи является якорем запроса
							case 9: result.emplace(flag_t::ANCHOR, match[i].str()); break;
							// Если типом записи является доменным именем
							case 4: result.emplace(flag_t::HOST, this->_fmk->toLower(match[i].str())); break;
							// Если типом записи является протокол
							case 2: result.emplace(flag_t::SCHEMA, this->_fmk->toLower(match[i].str())); break;
						}
					}
				}
				// Выполняем поиск хоста
				auto it = result.find(flag_t::HOST);
				// Если хост запроса найден
				if(it != result.end()){
					// Выполняем поиск разделителя порта
					const size_t pos = it->second.rfind(":");
					// Если разделитель порта найден
					if(pos != string::npos){
						// Получаем данные порта
						const string & port = it->second.substr(pos + 1);
						// Если данные являются портом
						if(this->_fmk->isNumber(port)){
							// Устанавливаем данные порта
							result.emplace(flag_t::PORT, port);
							// Формируем правильный хост
							it->second = it->second.substr(0, pos);
						}
					}
				}
			// Если данные пришли в неправильном формате
			} else {
				// Переходим по всему списку полученных данных
				for(size_t i = 0; i < match.size(); i++){
					// Если запись получена
					if(!match[i].str().empty()){
						// Определяем тип записи
						switch(i){
							// Если типом записи является путём запроса
							case 5: result.emplace(flag_t::PATH, match[i].str()); break;
							// Если типом записи является параметрами запроса
							case 7: result.emplace(flag_t::PARAMS, match[i].str()); break;
							// Если типом записи является якорем запроса
							case 9: result.emplace(flag_t::ANCHOR, match[i].str()); break;
							// Если типом записи является доменным именем
							case 2: result.emplace(flag_t::HOST, this->_fmk->toLower(match[i].str())); break;
						}
					}
				}
				// Выполняем поиск пути запроса
				auto it = result.find(flag_t::PATH);
				// Если путь запроса найден
				if(it != result.end()){
					// Выполняем поиск разделителя пути
					const size_t pos = it->second.find("/");
					// Если разделитель пути найден
					if(pos != string::npos){
						// Если позиция не нулевая и порт является числом
						if(pos > 0){
							// Получаем данные хоста или порта
							const string & data = it->second.substr(0, pos);
							// Если данные являются портом
							if(this->_fmk->isNumber(data))
								// Устанавливаем данные порта
								result.emplace(flag_t::PORT, data);
							// Иначе если хост не установлен
							else if(result.count(flag_t::HOST) < 1) {								
								// Определяем тип домена
								switch(this->_nwk->checkNetworkByIp(data)){
									// Если - это доменное имя
									case 0:
									// Если - это IP адрес сети v4
									case 4:
									// Если - это IP адрес сети v6
									case 6: result.emplace(flag_t::HOST, this->_fmk->toLower(data)); break;
								}
							}
						}
						// Формируем правильный путь запроса
						it->second = it->second.substr(pos);
					}
				}
			}{
				// Выполняем поиск протокола запроса
				auto it = result.find(flag_t::SCHEMA);
				// Если протокол не обнаружен
				if(it == result.end())
					// Устанавливаем протокол запроса
					result.emplace(flag_t::SCHEMA, "http");
			}{
				// Выполняем поиск пути запроса
				auto it = result.find(flag_t::PATH);
				// Если путь запроса не обнаружен
				if(it == result.end())
					// Устанавливаем протокол запроса
					result.emplace(flag_t::PATH, "/");
			}{
				// Выполняем поиск хоста
				auto it = result.find(flag_t::HOST);
				// Если хост запроса обнаружен
				if((it != result.end()) && it->second.compare("unix") == 0){
					// Извлекаем путь запроса
					const string & path = result.at(flag_t::PATH);
					// Если в пути найдено расширение
					if(path.rfind(".") != string::npos){
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "0");
						// Устанавливаем схему протокола
						result.at(flag_t::SCHEMA) = it->second;
						// Заменяем хост, на путь сокета в файловой системе
						result.at(flag_t::HOST) = path;
					}
				}
			}{
				// Выполняем поиск порта
				auto it = result.find(flag_t::PORT);
				// Если порт не найден
				if(it == result.end()){
					// Получаем схему протокола интернета
					const string & schema = result.at(flag_t::SCHEMA);
					// Если протокол является HTTPS
					if(schema.compare("https") == 0)
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "443");
					// Если протокол является HTTP
					else if(schema.compare("http") == 0)
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "80");
					// Если протокол является WSS
					else if(schema.compare("wss") == 0)
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "443");
					// Если протокол является WS
					else if(schema.compare("ws") == 0)
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "80");
					// Если протокол является FTP
					else if(schema.compare("ftp") == 0)
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "21");
					// Если протокол является MQTT
					else if(schema.compare("mqtt") == 0)
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "1883");
					// Если протокол является REDIS
					else if(schema.compare("redis") == 0)
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "6379");
					// Если протокол является Socks5
					else if(schema.compare("socks5") == 0)
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "1080");
					// Если протокол является PostgreSQL
					else if(schema.compare("postgresql") == 0)
						// Устанавливаем данные порта
						result.emplace(flag_t::PORT, "5432");
					// Иначе устанавливаем порт открытого HTTP протокола
					else result.emplace(flag_t::PORT, "80");
				// Если порт установлен как 443
				} else if(it->second.compare("443") == 0) {
					// Если протокол является HTTP
					if(result.at(flag_t::SCHEMA).compare("http") == 0)
						// Устанавливаем протокол
						result.at(flag_t::SCHEMA) = "https";
				}
			}{
				// Выполняем поиск хоста
				auto it = result.find(flag_t::HOST);
				// Если хост запроса найден
				if(it != result.end()){
					// Выполняем поиск разделителя данных пользователя и хоста
					size_t pos = it->second.rfind("@");
					// Если разделитель порта найден
					if(pos != string::npos){
						// Получаем данные пользователя
						const string user = it->second.substr(0, pos);
						// Формируем правильный хост
						it->second = it->second.substr(pos + 1);
						// Выполняем поиск разделителя логина и пароля
						pos = user.find(":");
						// Если разделитель логина и пароля найден
						if(pos != string::npos){
							// Устанавливаем пароль пользователя
							result.emplace(flag_t::PASS, user.substr(pos + 1));
							// Устанавливаем логин пользователя
							result.emplace(flag_t::LOGIN, user.substr(0, pos));
						// Устанавливаем данные пользователя как они есть
						} else result.emplace(flag_t::LOGIN, std::forward <const string> (user));
					}
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
vector <pair <string, string>> awh::URI::splitParams(const string & uri) const noexcept {
	// Результат работы функции
	vector <pair <string, string>> result;
	// Если URI передано
	if(!uri.empty()){
		// Параметры URI
		vector <wstring> params;
		// Выполняем сплит URI параметров
		this->_fmk->split(uri, "&", params);
		// Если параметры получены
		if(!params.empty()){
			// Данные параметров
			vector <wstring> data;
			// Переходим по всему списку параметров
			for(auto & param : params){
				// Выполняем сплит данных URI параметров
				this->_fmk->split(param, L"=", data);
				// Если данные получены
				if(data.size() == 2)
					// Добавляем полученные данные
					result.push_back(make_pair(
						this->decode(this->_fmk->convert(data.front())),
						this->decode(this->_fmk->convert(data.back()))
					));
				// Если значения параметр не имеет
				else if(data.size() == 1)
					// Добавляем полученные данные
					result.push_back(make_pair(this->decode(this->_fmk->convert(data.front())), ""));
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
		this->_fmk->split(path.substr(1), delim, params);
		// Если параметры получены
		if(!params.empty()){
			// Переходим по всему списку параметров
			for(auto & param : params)
				// Добавляем в список наши параметры
				result.push_back(this->decode(this->_fmk->convert(param)));
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
string awh::URI::joinParams(const vector <pair <string, string>> & uri) const noexcept {
	// Результат работы функции
	string result = "";
	// Если параметры URI переданы
	if(!uri.empty()){
		// Добавляем системный параметр
		result.append("?");
		// Переходим по всему списку параметров
		for(auto & param : uri){
			// Если параметры уже были добавлены
			if(result.length() > 1) result.append("&");
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
 * @param uri    для получения параметров
 * @param schema протокол передачи данных
 * @return       параметры полученные из URI
 */
awh::URI::params_t awh::URI::params(const string & uri, const string & schema) const noexcept {
	// Результат работы функции
	params_t result;
	// Если URI передан
	if(!uri.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e(
			"^(?:(?:(.+)\\:)?(?:(.+)\\@))?((?:[^\\:]+|(?:\\[?(?:\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]?)))(?:\\:(\\d+))?$",
			regex::ECMAScript | regex::icase
		);
		// Выполняем поиск ip адреса и префикса сети
		regex_search(uri, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Получаем пользователя
			result.user = match[1].str();
			// Получаем пароль пользователя
			result.pass = match[2].str();
			// Получаем хост
			result.host = this->_fmk->toLower(match[3].str());
			// Получаем порт
			const string & port = match[4].str();
			// Если порт получен
			if(!port.empty()) result.port = stoi(port);
			// Если порт не получен но указана схема
			else if(!schema.empty()){
				// Если - это зашифрованный протокол
				if(schema.compare("https") == 0) result.port = 443;
				// Если - это не зашифрованный протокол
				else if(schema.compare("http") == 0) result.port = 80;
				// Если - это зашифрованный протокол WebSocket
				else if(schema.compare("wss") == 0) result.port = 443;
				// Если - это не зашифрованный протокол WebSocket
				else if(schema.compare("ws") == 0) result.port = 80;
				// Если - это FTP
				else if(schema.compare("ftp") == 0) result.port = 21;
				// Если - это MQTT
				else if(schema.compare("mqtt") == 0) result.port = 1883;
				// Если - это Redis
				else if(schema.compare("redis") == 0) result.port = 6379;
				// Если - это SOCKS5 прокси
				else if(schema.compare("socks5") == 0) result.port = 1080;
				// Если - это PostgreSQL
				else if(schema.compare("postgresql") == 0) result.port = 5432;
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
