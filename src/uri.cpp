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
#include <uri.hpp>

/**
 * clear Метод очистки
 */
void awh::URI::URL::clear() noexcept {
	// Выполняем сброс порта
	this->port = 0;
	// Зануляем передаваемый промежуточный контекст
	this->ctx = nullptr;
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
	return (this->schema.empty() || this->host.empty());
}
/**
 * etag Метод генерации ETag хэша текста
 * @param text текст для перевода в строку
 * @return     хэш etag
 */
const string awh::URI::etag(const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если текст передан
	if(!text.empty()){
		// Получаем sha1 хэш строки
		const string & sha1 = this->fmk->sha1(text);
		// Если строка получена
		if(!sha1.empty()){
			// Извлекаем первую часть хэша
			const string & first = sha1.substr(0, 8);
			// Извлекаем вторую часть хэша
			const string & second = sha1.substr(35);
			// Формируем результат
			result = this->fmk->format("W/\"%s-%s\"", first.c_str(), second.c_str());
		}
	}
	// Выводим результат
	return result;
}
/**
 * urlEncode Метод кодирования строки в url адресе
 * @param str строка для кодирования
 * @return    результат кодирования
 */
const string awh::URI::urlEncode(const string & str) const noexcept {
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
 * urlDecode Метод декодирования строки в url адресе
 * @param str строка для декодирования
 * @return    результат декодирования
 */
const string awh::URI::urlDecode(const string & str) const noexcept {
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
 * createUrl Метод создания строки URL запросы из параметров
 * @param url параметры URL запроса
 * @return    URL запрос в виде строки
 */
const string awh::URI::createUrl(const url_t & url) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные получены
	if(!url.schema.empty() && !url.host.empty()){
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
				case AF_INET6: host = this->fmk->format("[%s]", url.ip.c_str()); break;
			}
		}
		// Если параметры авторизации указаны
		if(!url.pass.empty() || !url.user.empty())
			// Формируем параметры авторизации
			auth = (!url.pass.empty() ? this->fmk->format("%s:%s@", url.user.c_str(), url.pass.c_str()) : this->fmk->format("%s@", url.user.c_str()));
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
		const string & query = this->createQuery(url);
		// Если порт не установлен, формируем URL строку запроса без порта
		if(port == 0) result = this->fmk->format("%s://%s%s%s", url.schema.c_str(), auth.c_str(), host.c_str(), query.c_str());
		// Если порт установлен, формируем URL строку запроса с указанием порта
		else result = this->fmk->format("%s://%s%s:%u%s", url.schema.c_str(), auth.c_str(), host.c_str(), url.port, query.c_str());
	}
	// Выводим результат
	return result;
}
/**
 * createQuery Метод создания строки запроса из параметров
 * @param url параметры URL запроса
 * @return    URL запрос в виде строки
 */
const string awh::URI::createQuery(const url_t & url) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные получены
	if(!url.empty()){
		// Выполняем сборку пути запроса
		const string & path = this->joinPath(url.path);
		// Выполняем сборку параметров запроса
		const string & params = this->joinParams(url.params);
		// Выполняем генерацию сигнатуры
		const string & signature = (url.sign != nullptr ? this->fmk->format("&%s", url.sign(&url, this, url.ctx).c_str()) : "");
		// Иначе порт не устанавливаем
		result = this->fmk->format("%s%s%s%s", path.c_str(), params.c_str(), signature.c_str(), url.anchor.c_str());
	}
	// Выводим результат
	return result;
}
/**
 * createOrigin Метод создания заголовка [origin], для HTTP запроса
 * @param url параметры URL запроса
 * @return    заголовок [origin]
 */
const string awh::URI::createOrigin(const url_t & url) const noexcept {
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
				case AF_INET6: host = this->fmk->format("[%s]", url.ip.c_str()); break;
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
		if(port > 0) result = this->fmk->format("%s://%s:%u", url.schema.c_str(), host.c_str(), port);
		// Иначе порт не устанавливаем
		else result = this->fmk->format("%s://%s", url.schema.c_str(), host.c_str());
	}
	// Выводим результат
	return result;
}
/**
 * parseUrl Метод получения параметров URL запроса
 * @param url строка URL запроса для получения параметров
 * @param ctx промежуточный передаваемый контекст (если требуется)
 * @return    параметры URL запроса
 */
const awh::URI::url_t awh::URI::parseUrl(const string & url, void * ctx) const noexcept {
	// Результат работы функции
	url_t result;
	// Если URL адрес передан
	if(!url.empty()){
		// Устанавливаем промежуточный передаваемый контекст
		result.ctx = ctx;
		// Выполняем парсинг URL адреса
		const auto & split = this->split(url);
		// Если список параметров получен
		if(!split.empty() && (split.size() > 1)){
			// Устанавливаем протокол запроса
			result.schema = this->fmk->toLower(split.front());
			// Определяем сколько элементов мы получили
			switch(split.size()){
				// Если количество элементов равно 3
				case 3: {
					// Проверяем путь запроса
					const string & value = split.back();
					// Если первый символ является путём запроса
					if(value.front() == '/') result.path = this->splitPath(value);
					// Если же первый символ является параметром запросов
					else if(value.front() == '?') result.params = this->splitParams(value);
					// Если же первый символ является якорем
					else if(value.front() == '#') result.anchor = value;
				} break;
				// Если количество элементов равно 4
				case 4: {
					// Устанавливаем путь запроса
					result.path = this->splitPath(split.at(2));
					// Проверяем параметры запроса
					const string & value = split.back();
					// Если первый символ является параметром запросов
					if(value.front() == '?') result.params = this->splitParams(value);
					// Если же первый символ является якорем
					else if(value.front() == '#') result.anchor = value;
				} break;
				// Если количество элементов равно 5
				case 5: {
					// Устанавливаем якорь запроса
					result.anchor = split.back();
					// Устанавливаем путь запроса
					result.path = this->splitPath(split.at(2));
					// Устанавливаем параметры запроса
					result.params = this->splitParams(split.at(3));
				} break;
			}
			// Разбиваем доменное имя на параметры
			const auto & params = this->params(split.at(1), result.schema);
			// Устанавливаем хост сервера
			result.host = params.host;
			// Устанавливаем порт сервера
			result.port = params.port;
			// Если пользователь получен
			if(!params.user.empty()) result.user = params.user;
			// Если пароль получен
			if(!params.pass.empty()) result.pass = params.pass;
			// Определяем тип домена
			switch(this->nwk->checkNetworkByIp(params.host)){
				// Если - это доменное имя
				case 0: result.domain = this->fmk->toLower(params.host); break;
				// Если - это IP адрес сети v4
				case 4: {
					// Устанавливаем IP адрес
					result.ip = params.host;
					// Устанавливаем тип сети
					result.family = AF_INET;
				} break;
				// Если - это IP адрес сети v6
				case 6: {
					// Устанавливаем IP адрес
					result.ip = params.host;
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
	// Выводим результат
	return result;
}
/**
 * split Метод сплита URI на составные части
 * @param uri строка URI для сплита
 * @return    список полученных частей URI
 */
const vector <string> awh::URI::split(const string & uri) const noexcept {
	// Результат работы функции
	vector <string> result;
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
			// Полученная строка
			string value = "";
			// Если данные пришли в правильном формате
			if(!match[4].str().empty()){
				// Переходим по всему списку полученных данных
				for(size_t i = 0; i < match.size(); i++){
					// Получаем значение
					value = match[i].str();
					/**
					 * schema [2]
					 * domain [4]
					 * path [5]
					 * params [6]
					 * anchor [8]
					 */
					// Если стркоа существует и индекс соответствует
					if(!value.empty() && ((i == 2) || (i == 4) || (i == 5) || (i == 6) || (i == 8)))
						// Если индекс соответствует
						result.push_back(value);
				}
			// Если данные пришли в неправильном формате
			} else {
				// Переходим по всему списку полученных данных
				for(size_t i = 0; i < match.size(); i++){
					// Получаем значение
					value = match[i].str();
					/**
					 * domain [2]
					 * path [5]
					 * params [6]
					 * anchor [8]
					 */
					// Если стркоа существует и индекс соответствует
					if(!value.empty() && ((i == 2) || (i == 5) || (i == 6) || (i == 8)))
						// Если индекс соответствует
						result.push_back(value);
				}
				// Если путь запроса получен
				if(result.size() >= 2){
					// Порт сервера
					string port = "";
					// Выполняем поиск порта
					for(auto & letter : result.at(1)){
						// Если число не получено, выходим
						if(!this->fmk->isNumber(string(1, letter))) break;
						// Если число получено, собираем его
						else port.append(1, letter);
					}
					// Если мы нашли порт сервера
					if(!port.empty()){
						// Удаляем порт сервера из пути
						result.at(1).erase(0, port.size());
						// Устанавливаем порт хоста
						result.front() = this->fmk->format("%s:%s", result.front().c_str(), port.c_str());
						// Если порт указан для зашифрованного подключения
						if(port.compare("443") == 0){
							// Устанавливаем схему протокола
							result.insert(result.begin(), "https");
							// Выходим из функции
							goto Next;
						}
					}
				}
				// Устанавливаем схему протокола
				result.insert(result.begin(), "http");
			}
		}
	}
	// Устанавливаем метку выхода
	Next:
	// Выводим результат
	return result;
}
/**
 * splitParams Метод выполнения сплита параметров URI
 * @param uri строка URI для сплита
 * @return    параметры полученные при сплите
 */
const unordered_map <string, string> awh::URI::splitParams(const string & uri) const noexcept {
	// Результат работы функции
	unordered_map <string, string> result;
	// Если URI передано
	if(!uri.empty()){
		// Параметры URI
		vector <wstring> params;
		// Выполняем сплит URI параметров
		this->fmk->split(uri.substr(1), "&", params);
		// Если параметры получены
		if(!params.empty()){
			// Данные параметров
			vector <wstring> data;
			// Переходим по всему списку параметров
			for(auto & param : params){
				// Выполняем сплит данных URI параметров
				this->fmk->split(param, L"=", data);
				// Если данные получены
				if(!data.empty()){
					// Добавляем полученные данные
					result.emplace(
						this->urlDecode(this->fmk->convert(data.front())),
						this->urlDecode(this->fmk->convert(data.back()))
					);
				}
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
const vector <string> awh::URI::splitPath(const string & path, const string & delim) const noexcept {
	// Результат работы функции
	vector <string> result;
	// Если данные переданы
	if(!path.empty() && !delim.empty()){
		// Параметры пути
		vector <wstring> params;
		// Выполняем сплит параметров пути
		this->fmk->split(path.substr(1), delim, params);
		// Если параметры получены
		if(!params.empty()){
			// Переходим по всему списку параметров
			for(auto & param : params){
				// Добавляем в список наши параметры
				result.push_back(this->urlDecode(this->fmk->convert(param)));
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
const string awh::URI::joinParams(const unordered_map <string, string> & uri) const noexcept {
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
			// Добавляем собранные параметры
			result.append(this->fmk->format("%s=%s", this->urlEncode(param.first).c_str(), this->urlEncode(param.second).c_str()));
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
const string awh::URI::joinPath(const vector <string> & path, const string & delim) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные переданы
	if(!path.empty() && !delim.empty()){
		// Переходим по всему списку параметров
		for(auto & param : path){
			// Добавляем разделитель
			result.append(delim);
			// Добавляем собранные параметры
			result.append(this->urlEncode(param));
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
const awh::URI::params_t awh::URI::params(const string & uri, const string & schema) const noexcept {
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
			result.host = this->fmk->toLower(match[3].str());
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
