/**
 * @file: nwt.cpp
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
#include <net/nwt.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Оператор перемещения
 * @param url параметры падреса
 * @return    параметры URL-запроса
 */
awh::NWT::URL & awh::NWT::URL::operator = (url_t && url) noexcept {
	// Выполняем копирование тип URL-адреса
	this->type = url.type;
	// Выполняем копирование порта URL-адреса
	this->port = url.port;
	// Выполняем перемещение полного URI-параметров
	this->uri = ::move(url.uri);
	// Выполняем перемещение хоста URL-адреса
	this->host = ::move(url.host);
	// Выполняем перемещение пути URL-адреса
	this->path = ::move(url.path);
	// Выполняем перемещение ника пользователя (для электронной почты)
	this->user = ::move(url.user);
	// Выполняем перемещение пароля пользователя
	this->pass = ::move(url.pass);
	// Выполняем перемещение якоря URL-адреса
	this->anchor = ::move(url.anchor);
	// Выполняем перемещение домена верхнего уровня
	this->domain = ::move(url.domain);
	// Выполняем перемещение параметров URL-адреса
	this->params = ::move(url.params);
	// Выполняем перемещение протокола URL-адреса
	this->schema = ::move(url.schema);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор присванивания
 * @param url параметры падреса
 * @return    параметры URL-запроса
 */
awh::NWT::URL & awh::NWT::URL::operator = (const url_t & url) noexcept {
	// Выполняем копирование тип URL-адреса
	this->type = url.type;
	// Выполняем копирование порта URL-адреса
	this->port = url.port;
	// Выполняем копирование полного URI-параметров
	this->uri = url.uri;
	// Выполняем копирование хоста URL-адреса
	this->host = url.host;
	// Выполняем копирование пути URL-адреса
	this->path = url.path;
	// Выполняем копирование ника пользователя (для электронной почты)
	this->user = url.user;
	// Выполняем копирование пароля пользователя
	this->pass = url.pass;
	// Выполняем копирование якоря URL-адреса
	this->anchor = url.anchor;
	// Выполняем копирование домена верхнего уровня
	this->domain = url.domain;
	// Выполняем копирование параметров URL-адреса
	this->params = url.params;
	// Выполняем копирование протокола URL-адреса
	this->schema = url.schema;
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор сравнения
 * @param url параметры падреса
 * @return    результат сравнения
 */
bool awh::NWT::URL::operator == (const url_t & url) noexcept {
	// Выполняем сравнение параметров
	return (
		(this->type == url.type) &&
		(this->port == url.port) &&
		(this->uri.compare(url.uri) == 0) &&
		(this->host.compare(url.host) == 0) &&
		(this->path.compare(url.path) == 0) &&
		(this->user.compare(url.user) == 0) &&
		(this->pass.compare(url.pass) == 0) &&
		(this->anchor.compare(url.anchor) == 0) &&
		(this->domain.compare(url.domain) == 0) &&
		(this->params.compare(url.params) == 0) &&
		(this->schema.compare(url.schema) == 0)
	);
}
/**
 * URL Конструктор перемещения
 * @param uri параметры падреса
 */
awh::NWT::URL::URL(url_t && url) noexcept {
	// Выполняем копирование тип URL-адреса
	this->type = url.type;
	// Выполняем копирование порта URL-адреса
	this->port = url.port;
	// Выполняем перемещение полного URI-параметров
	this->uri = ::move(url.uri);
	// Выполняем перемещение хоста URL-адреса
	this->host = ::move(url.host);
	// Выполняем перемещение пути URL-адреса
	this->path = ::move(url.path);
	// Выполняем перемещение ника пользователя (для электронной почты)
	this->user = ::move(url.user);
	// Выполняем перемещение пароля пользователя
	this->pass = ::move(url.pass);
	// Выполняем перемещение якоря URL-адреса
	this->anchor = ::move(url.anchor);
	// Выполняем перемещение домена верхнего уровня
	this->domain = ::move(url.domain);
	// Выполняем перемещение параметров URL-адреса
	this->params = ::move(url.params);
	// Выполняем перемещение протокола URL-адреса
	this->schema = ::move(url.schema);
}
/**
 * URL Конструктор копирования
 * @param url параметры падреса
 */
awh::NWT::URL::URL(const url_t & url) noexcept {
	// Выполняем копирование тип URL-адреса
	this->type = url.type;
	// Выполняем копирование порта URL-адреса
	this->port = url.port;
	// Выполняем копирование полного URI-параметров
	this->uri = url.uri;
	// Выполняем копирование хоста URL-адреса
	this->host = url.host;
	// Выполняем копирование пути URL-адреса
	this->path = url.path;
	// Выполняем копирование ника пользователя (для электронной почты)
	this->user = url.user;
	// Выполняем копирование пароля пользователя
	this->pass = url.pass;
	// Выполняем копирование якоря URL-адреса
	this->anchor = url.anchor;
	// Выполняем копирование домена верхнего уровня
	this->domain = url.domain;
	// Выполняем копирование параметров URL-адреса
	this->params = url.params;
	// Выполняем копирование протокола URL-адреса
	this->schema = url.schema;
}
/**
 * URL Конструктор
 */
awh::NWT::URL::URL() noexcept :
 type(types_t::NONE), port(0), uri{""},
 host{""}, path{""}, user{""}, pass{""},
 anchor{""}, domain{""}, params{""}, schema{""} {}
/**
 * zone Метод установки пользовательской зоны
 * @param zone пользовательская зона
 */
void awh::NWT::zone(const string & zone) noexcept {
	// Если зона передана и она не существует
	if(!zone.empty() && (this->_national.find(zone) == this->_national.end()) && (this->_general.find(zone) == this->_general.end()))
		// Добавляем зону в список
		this->_user.emplace(zone);
}
/**
 * zones Метод извлечения списка пользовательских зон интернета
 */
const set <string> & awh::NWT::zones() const noexcept {
	// Выводим список пользовательских зон интернета
	return this->_user;
}
/**
 * zones Метод установки списка пользовательских зон
 * @param zones список доменных зон интернета
 */
void awh::NWT::zones(const set <string> & zones) noexcept {
	// Если список зон не пустой
	if(!zones.empty())
		// Выводим список пользовательских зон
		this->_user = zones;
}
/**
 * clear Метод очистки результатов парсинга
 */
void awh::NWT::clear() noexcept {
	// Очищаем список пользовательских зон
	this->_user.clear();
}
/**
 * parse Метод парсинга URI-строки
 * @param text текст для парсинга
 * @return     параметры полученные в результате парсинга
 */
awh::NWT::url_t awh::NWT::parse(const string & text) noexcept {
	// Результат работы функции
	url_t result;
	// Если текст передан
	if(!text.empty()){
		/**
		 * emailFn Функция извлечения данных электронного адреса
		 * @param text текст для парсинга
		 */
		auto emailFn = [this](const string & text) noexcept -> url_t {
			// Результат работы функции
			url_t result;
			// Если текст передан
			if(!text.empty()){
				// Выполняем проверку электронной почты
				const auto & match = this->_regexp.exec(text, this->_email);
				// Если результат найден
				if(!match.empty()){
					// Запоминаем тип параметра
					result.type = types_t::EMAIL;
					// Запоминаем uri адрес
					result.uri = match[1];
					// Запоминаем логин пользователя
					result.user = match[2];
					// Запоминаем название электронного ящика
					result.host = match[3];
					// Запоминаем домен верхнего уровня
					result.domain = match[4];
				}
			}
			// Выводим результат
			return result;
		};
		/**
		 * urlFn Функция извлечения данных URL адресов
		 * @param text текст для парсинга
		 */
		auto urlFn = [this](const string & text) noexcept -> url_t {
			// Результат работы функции
			url_t result;
			// Если текст передан
			if(!text.empty()){
				// Выполняем проверку URL адреса
				const auto & match = this->_regexp.exec(text, this->_url);
				// Если результат найден
				if(!match.empty()){
					// Запоминаем uri адрес
					result.uri = match[0];
					// Получаем логин пользователя
					result.user = match[2];
					// Получаем пароль пользователя
					result.pass = match[3];
					// Запоминаем название домена
					result.host = match[4];
					// Запоминаем путь запроса
					result.path = match[7];
					// Запоминаем протокол
					result.schema = match[1];
					// Запоминаем домен верхнего уровня
					result.domain = match[5];
					// Запоминаем параметры запроса
					result.params = match[8];
					// Запоминаем якорь запроса
					result.anchor = match[9];
					// Если порт получен
					if(!match[6].empty()){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Запоминаем порт запроса
							result.port = ::stoi(match[6]);
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Запоминаем порт запроса
							result.port = 0;
						}
					}
					// Запоминаем тип параметра
					result.type = types_t::URL;
				// Устанавливаем параметр неверных данных
				} else result.type = types_t::WRONG;
			}
			// Выводим результат
			return result;
		};
		/**
		 * ipFn Функция извлечения данных IP адресов
		 * @param text текст для парсинга
		 */
		auto ipFn = [this](const string & text) noexcept -> url_t {
			// Результат работы функции
			url_t result;
			// Если текст передан
			if(!text.empty()){
				// Выполняем проверку IP адреса
				const auto & match = this->_regexp.exec(text, this->_ip);
				// Если результат найден
				if(!match.empty()){
					// Запоминаем uri адрес
					result.uri = match.at(0);
					// Если это MAC адрес
					if(!match[2].empty()){
						// Запоминаем сам параметр
						result.host = match[2];
						// Запоминаем тип параметра
						result.type = types_t::MAC;
					// Если это IPv4 адрес
					} else if(!match[4].empty()) {
						// Запоминаем сам параметр
						result.host = match[4];
						// Запоминаем тип параметра
						result.type = types_t::IPV4;
					// Если это IPv6 адрес
					} else if(!match[3].empty()) {
						// Запоминаем сам параметр
						result.host = match[3];
						// Запоминаем тип параметра
						result.type = types_t::IPV6;
					// Если это параметры сети
					} else if(!match[1].empty()) {
						// Запоминаем сам параметр
						result.host = match[1];
						// Запоминаем тип параметра
						result.type = types_t::NETWORK;
					}
				}
			}
			// Выводим результат
			return result;
		};
		// Очищаем результаты предыдущей работы
		this->clear();
		// Запрашиваем данные URL адреса
		url_t url = urlFn(text);
		// Если мы получили какие-то достоверные параметры
		if((url.type == types_t::URL) && ((url.port > 0) ||
		   !url.path.empty() || !url.pass.empty() ||
		   !url.anchor.empty() || !url.params.empty() || !url.schema.empty()))
			// Устанавливаем полученный результат
			result = ::move(url);
		// Если URL адрес мы не получили
		else {
			// Выполняем извлечение E-Mail адреса
			url_t email = emailFn(text);
			// Если мы получили E-Mail адрес
			if(email.type == types_t::EMAIL)
				// Устанавливаем полученный результат
				result = ::move(email);
			// Если E-Mail адрес мы не получили
			else {
				// Выполняем извлечение IP адреса
				url_t ip = ipFn(text);
				// Если мы получили IP адрес
				if((ip.type == types_t::IPV4) || (ip.type == types_t::IPV6) || (ip.type == types_t::MAC) || (ip.type == types_t::NETWORK))
					// Устанавливаем полученный результат
					result = ::move(ip);
			}
		}	
	}
	// Выводим результат
	return result;
}
/**
 * letters Метод добавления букв алфавита
 * @param letters список букв алфавита
 */
void awh::NWT::letters(const string & letters) noexcept {
	// Если буквы переданы запоминаем их
	if(!letters.empty())
		// Устанавливаем буквы алфавита
		this->_letters = letters;
	// Устанавливаем регулярное выражение для проверки электронной почты
	this->_email = this->_regexp.build(
		"((?:([\\w\\-"
		+ this->_letters
		+ "]+)\\@)(\\[(?:\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]|(?:\\d{1,3}(?:\\.\\d{1,3}){3})|(?:(?:xn\\-\\-[\\w\\d]+\\.){0,100}(?:xn\\-\\-[\\w\\d]+)|(?:[\\w\\-"
		+ this->_letters
		+ "]+\\.){0,100}[\\w\\-"
		+ this->_letters
		+ "]+)\\.(xn\\-\\-[\\w\\d]+|[a-z"
		+ this->_letters
		+ "]+)))", {regexp_t::option_t::UTF8, regexp_t::option_t::CASELESS}
	);
	// Устанавливаем правило регулярного выражения для проверки URL адресов
	this->_url = this->_regexp.build(
		"(?:(http[s]?)\\:\\/\\/)?(?:([\\w+\\-]+)(?:\\:([\\w+\\-]+))?\\@)?(\\[(?:\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]|(?:\\d{1,3}(?:\\.\\d{1,3}){3})|(?:(?:xn\\-\\-[\\w\\d]+\\.){0,100}(?:xn\\-\\-[\\w\\d]+)|(?:[\\w\\-"
		+ this->_letters
		+ "]+\\.){0,100}[\\w\\-"
		+ this->_letters
		+ "]+)\\.(xn\\-\\-[\\w\\d]+|[a-z"
		+ this->_letters
		+ "]+))(?:\\:(\\d+))?((?:\\/[\\w\\-]+){0,100}(?:$|\\/|\\w+)|\\/)?(?:\\?([\\w\\-\\.\\~\\:\\[\\]\\@\\!\\$\\&\\'\\(\\)\\*\\+\\,\\;\\=]+))?(?:\\#([\\w\\-\\_]+))?", {regexp_t::option_t::UTF8, regexp_t::option_t::CASELESS}
	);
	// Устанавливаем правило регулярного выражения для проверки IP адресов
	this->_ip = this->_regexp.build(
		// Если это сеть
		"(?:((?:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}[\\:]{2}|[\\:]{2})|[\\:]{2}))\\/(?:\\d{1,3}(?:\\.\\d{1,3}){3}|\\d+))|"
		// Определение MAC адреса
		"([a-f\\d]{2}(?:\\:[a-f\\d]{2}){5})|"
		// Определение IPv6 адреса
		"(?:\\[?(\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:\\:\\:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){1,7})?)|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]?)|"
		// Определение IPv4 адреса
		"(\\d{1,3}(?:\\.\\d{1,3}){3})(?:\\:\\d+)?\\/?)", {regexp_t::option_t::UTF8, regexp_t::option_t::CASELESS}
	);
}
/**
 * NWT Конструктор
 * @param letters список букв алфавита
 */
awh::NWT::NWT(const string & letters) noexcept : _letters("") {
	// Создаем список национальных доменов
	this->_national.emplace("ac");
	this->_national.emplace("ad");
	this->_national.emplace("ae");
	this->_national.emplace("af");
	this->_national.emplace("ag");
	this->_national.emplace("ai");
	this->_national.emplace("al");
	this->_national.emplace("am");
	this->_national.emplace("an");
	this->_national.emplace("ao");
	this->_national.emplace("aq");
	this->_national.emplace("ar");
	this->_national.emplace("as");
	this->_national.emplace("at");
	this->_national.emplace("au");
	this->_national.emplace("aw");
	this->_national.emplace("ax");
	this->_national.emplace("az");
	this->_national.emplace("ba");
	this->_national.emplace("bb");
	this->_national.emplace("bd");
	this->_national.emplace("be");
	this->_national.emplace("bf");
	this->_national.emplace("bg");
	this->_national.emplace("bh");
	this->_national.emplace("bi");
	this->_national.emplace("bj");
	this->_national.emplace("bm");
	this->_national.emplace("bn");
	this->_national.emplace("bo");
	this->_national.emplace("br");
	this->_national.emplace("bs");
	this->_national.emplace("bt");
	this->_national.emplace("bv");
	this->_national.emplace("bw");
	this->_national.emplace("by");
	this->_national.emplace("bz");
	this->_national.emplace("ca");
	this->_national.emplace("cc");
	this->_national.emplace("cd");
	this->_national.emplace("cf");
	this->_national.emplace("cg");
	this->_national.emplace("ch");
	this->_national.emplace("ci");
	this->_national.emplace("ck");
	this->_national.emplace("cl");
	this->_national.emplace("cm");
	this->_national.emplace("cn");
	this->_national.emplace("co");
	this->_national.emplace("cr");
	this->_national.emplace("cu");
	this->_national.emplace("cv");
	this->_national.emplace("cx");
	this->_national.emplace("cy");
	this->_national.emplace("cz");
	this->_national.emplace("dd");
	this->_national.emplace("de");
	this->_national.emplace("dj");
	this->_national.emplace("dk");
	this->_national.emplace("dm");
	this->_national.emplace("do");
	this->_national.emplace("dz");
	this->_national.emplace("ec");
	this->_national.emplace("ee");
	this->_national.emplace("eg");
	this->_national.emplace("er");
	this->_national.emplace("es");
	this->_national.emplace("et");
	this->_national.emplace("eu");
	this->_national.emplace("fi");
	this->_national.emplace("fj");
	this->_national.emplace("fk");
	this->_national.emplace("fm");
	this->_national.emplace("fo");
	this->_national.emplace("fr");
	this->_national.emplace("ga");
	this->_national.emplace("gb");
	this->_national.emplace("gd");
	this->_national.emplace("ge");
	this->_national.emplace("gf");
	this->_national.emplace("gg");
	this->_national.emplace("gh");
	this->_national.emplace("gi");
	this->_national.emplace("gl");
	this->_national.emplace("gm");
	this->_national.emplace("gn");
	this->_national.emplace("gp");
	this->_national.emplace("gq");
	this->_national.emplace("gr");
	this->_national.emplace("gs");
	this->_national.emplace("gt");
	this->_national.emplace("gu");
	this->_national.emplace("gw");
	this->_national.emplace("gy");
	this->_national.emplace("hk");
	this->_national.emplace("hm");
	this->_national.emplace("hn");
	this->_national.emplace("hr");
	this->_national.emplace("ht");
	this->_national.emplace("hu");
	this->_national.emplace("id");
	this->_national.emplace("ie");
	this->_national.emplace("il");
	this->_national.emplace("im");
	this->_national.emplace("in");
	this->_national.emplace("io");
	this->_national.emplace("iq");
	this->_national.emplace("ir");
	this->_national.emplace("is");
	this->_national.emplace("it");
	this->_national.emplace("je");
	this->_national.emplace("jm");
	this->_national.emplace("jo");
	this->_national.emplace("jp");
	this->_national.emplace("ke");
	this->_national.emplace("kg");
	this->_national.emplace("kh");
	this->_national.emplace("ki");
	this->_national.emplace("km");
	this->_national.emplace("kn");
	this->_national.emplace("kp");
	this->_national.emplace("kr");
	this->_national.emplace("kw");
	this->_national.emplace("ky");
	this->_national.emplace("kz");
	this->_national.emplace("la");
	this->_national.emplace("lb");
	this->_national.emplace("lc");
	this->_national.emplace("li");
	this->_national.emplace("lk");
	this->_national.emplace("lr");
	this->_national.emplace("ls");
	this->_national.emplace("lt");
	this->_national.emplace("lu");
	this->_national.emplace("lv");
	this->_national.emplace("ly");
	this->_national.emplace("ma");
	this->_national.emplace("mc");
	this->_national.emplace("md");
	this->_national.emplace("me");
	this->_national.emplace("mg");
	this->_national.emplace("mh");
	this->_national.emplace("mk");
	this->_national.emplace("ml");
	this->_national.emplace("mm");
	this->_national.emplace("mn");
	this->_national.emplace("mo");
	this->_national.emplace("mp");
	this->_national.emplace("mq");
	this->_national.emplace("mr");
	this->_national.emplace("ms");
	this->_national.emplace("mt");
	this->_national.emplace("mu");
	this->_national.emplace("mv");
	this->_national.emplace("mw");
	this->_national.emplace("mx");
	this->_national.emplace("my");
	this->_national.emplace("mz");
	this->_national.emplace("na");
	this->_national.emplace("nc");
	this->_national.emplace("ne");
	this->_national.emplace("nf");
	this->_national.emplace("ng");
	this->_national.emplace("ni");
	this->_national.emplace("nl");
	this->_national.emplace("no");
	this->_national.emplace("np");
	this->_national.emplace("nr");
	this->_national.emplace("nu");
	this->_national.emplace("nz");
	this->_national.emplace("om");
	this->_national.emplace("pa");
	this->_national.emplace("pe");
	this->_national.emplace("pf");
	this->_national.emplace("pg");
	this->_national.emplace("ph");
	this->_national.emplace("pk");
	this->_national.emplace("pl");
	this->_national.emplace("pm");
	this->_national.emplace("pn");
	this->_national.emplace("pr");
	this->_national.emplace("ps");
	this->_national.emplace("pt");
	this->_national.emplace("pw");
	this->_national.emplace("py");
	this->_national.emplace("qa");
	this->_national.emplace("re");
	this->_national.emplace("ro");
	this->_national.emplace("rs");
	this->_national.emplace("ru");
	this->_national.emplace("рф");
	this->_national.emplace("ру");
	this->_national.emplace("су");
	this->_national.emplace("rw");
	this->_national.emplace("sa");
	this->_national.emplace("sb");
	this->_national.emplace("sc");
	this->_national.emplace("sd");
	this->_national.emplace("se");
	this->_national.emplace("sg");
	this->_national.emplace("sh");
	this->_national.emplace("si");
	this->_national.emplace("sj");
	this->_national.emplace("sk");
	this->_national.emplace("sl");
	this->_national.emplace("sm");
	this->_national.emplace("sn");
	this->_national.emplace("so");
	this->_national.emplace("sr");
	this->_national.emplace("st");
	this->_national.emplace("su");
	this->_national.emplace("sv");
	this->_national.emplace("sy");
	this->_national.emplace("sz");
	this->_national.emplace("tc");
	this->_national.emplace("td");
	this->_national.emplace("tf");
	this->_national.emplace("tg");
	this->_national.emplace("th");
	this->_national.emplace("tj");
	this->_national.emplace("tk");
	this->_national.emplace("tl");
	this->_national.emplace("tm");
	this->_national.emplace("tn");
	this->_national.emplace("to");
	this->_national.emplace("tp");
	this->_national.emplace("tr");
	this->_national.emplace("tt");
	this->_national.emplace("tv");
	this->_national.emplace("tw");
	this->_national.emplace("tz");
	this->_national.emplace("ua");
	this->_national.emplace("ug");
	this->_national.emplace("uk");
	this->_national.emplace("us");
	this->_national.emplace("uy");
	this->_national.emplace("uz");
	this->_national.emplace("va");
	this->_national.emplace("vc");
	this->_national.emplace("ve");
	this->_national.emplace("vg");
	this->_national.emplace("vi");
	this->_national.emplace("vn");
	this->_national.emplace("vu");
	this->_national.emplace("wf");
	this->_national.emplace("ws");
	this->_national.emplace("ye");
	this->_national.emplace("yt");
	this->_national.emplace("za");
	this->_national.emplace("zm");
	this->_national.emplace("zw");
	this->_national.emplace("krd");
	this->_national.emplace("укр");
	this->_national.emplace("срб");
	this->_national.emplace("мон");
	this->_national.emplace("бел");
	this->_national.emplace("ком");
	this->_national.emplace("нет");
	this->_national.emplace("биз");
	this->_national.emplace("орг");
	this->_national.emplace("инфо");
	// Создаем список общих доменов
	this->_general.emplace("app");
	this->_general.emplace("biz");
	this->_general.emplace("cat");
	this->_general.emplace("com");
	this->_general.emplace("edu");
	this->_general.emplace("eus");
	this->_general.emplace("gov");
	this->_general.emplace("int");
	this->_general.emplace("mil");
	this->_general.emplace("net");
	this->_general.emplace("one");
	this->_general.emplace("ong");
	this->_general.emplace("onl");
	this->_general.emplace("ooo");
	this->_general.emplace("org");
	this->_general.emplace("pro");
	this->_general.emplace("red");
	this->_general.emplace("ren");
	this->_general.emplace("tel");
	this->_general.emplace("xxx");
	this->_general.emplace("xyz");
	this->_general.emplace("gold");
	this->_general.emplace("rent");
	this->_general.emplace("name");
	this->_general.emplace("aero");
	this->_general.emplace("mobi");
	this->_general.emplace("jobs");
	this->_general.emplace("info");
	this->_general.emplace("coop");
	this->_general.emplace("asia");
	this->_general.emplace("army");
	this->_general.emplace("pics");
	this->_general.emplace("pink");
	this->_general.emplace("plus");
	this->_general.emplace("porn");
	this->_general.emplace("post");
	this->_general.emplace("prof");
	this->_general.emplace("qpon");
	this->_general.emplace("rest");
	this->_general.emplace("rich");
	this->_general.emplace("site");
	this->_general.emplace("yoga");
	this->_general.emplace("zone");
	this->_general.emplace("local");
	this->_general.emplace("rehab");
	this->_general.emplace("press");
	this->_general.emplace("poker");
	this->_general.emplace("parts");
	this->_general.emplace("party");
	this->_general.emplace("audio");
	this->_general.emplace("archi");
	this->_general.emplace("dance");
	this->_general.emplace("actor");
	this->_general.emplace("adult");
	this->_general.emplace("photo");
	this->_general.emplace("pizza");
	this->_general.emplace("place");
	this->_general.emplace("travel");
	this->_general.emplace("review");
	this->_general.emplace("repair");
	this->_general.emplace("report");
	this->_general.emplace("racing");
	this->_general.emplace("photos");
	this->_general.emplace("physio");
	this->_general.emplace("online");
	this->_general.emplace("museum");
	this->_general.emplace("agency");
	this->_general.emplace("active");
	this->_general.emplace("reviews");
	this->_general.emplace("rentals");
	this->_general.emplace("recipes");
	this->_general.emplace("organic");
	this->_general.emplace("academy");
	this->_general.emplace("auction");
	this->_general.emplace("plumbing");
	this->_general.emplace("pharmacy");
	this->_general.emplace("airforce");
	this->_general.emplace("attorney");
	this->_general.emplace("partners");
	this->_general.emplace("pictures");
	this->_general.emplace("feedback");
	this->_general.emplace("property");
	this->_general.emplace("republican");
	this->_general.emplace("associates");
	this->_general.emplace("apartments");
	this->_general.emplace("accountant");
	this->_general.emplace("properties");
	this->_general.emplace("photography");
	this->_general.emplace("accountants");
	this->_general.emplace("productions");
	// Если буквы переданы запоминаем их
	this->letters(letters);
}
