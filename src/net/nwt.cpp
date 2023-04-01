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
 * zone Метод установки пользовательской зоны
 * @param zone пользовательская зона
 */
void awh::NWT::zone(const wstring & zone) noexcept {
	// Если зона передана и она не существует
	if(!zone.empty() && (this->_national.count(zone) < 1) && (this->_general.count(zone) < 1)){
		// Добавляем зону в список
		this->_user.emplace(zone);
	}
}
/**
 * zones Метод извлечения списка пользовательских зон интернета
 */
const set <wstring> & awh::NWT::zones() const noexcept {
	// Выводим список пользовательских зон интернета
	return this->_user;
}
/**
 * zones Метод установки списка пользовательских зон
 * @param zones список доменных зон интернета
 */
void awh::NWT::zones(const set <wstring> & zones) noexcept {
	// Если список зон не пустой
	if(!zones.empty()) this->_user = zones;
}
/**
 * clear Метод очистки результатов парсинга
 */
void awh::NWT::clear() noexcept {
	// Очищаем список пользовательских зон
	this->_user.clear();
}
/**
 * parse Метод парсинга URI строки
 * @param text текст для парсинга
 * @return     параметры полученные в результате парсинга
 */
const awh::NWT::data_t awh::NWT::parse(const wstring & text) noexcept {
	// Результат работы функции
	data_t result = {};
	// Если текст передан
	if(!text.empty()){
		/**
		 * emailFn Функция извлечения данных электронного адреса
		 * @param text текст для парсинга
		 */
		auto emailFn = [this](const wstring & text) noexcept {
			// Результат работы функции
			data_t result;
			// Если текст передан
			if(!text.empty()){
				// Результат работы регулярного выражения
				wsmatch match;
				// Выполняем проверку электронной почты
				regex_search(text, match, this->_expressEmail);
				// Если результат найден
				if(!match.empty() && (match.size() >= 5)){
					// Запоминаем тип параметра
					result.type = types_t::EMAIL;
					// Запоминаем uri адрес
					result.uri = match[1].str();
					// Запоминаем логин пользователя
					result.user = match[2].str();
					// Запоминаем название электронного ящика
					result.data = match[3].str();
					// Запоминаем домен верхнего уровня
					result.domain = match[4].str();
				}
			}
			// Выводим результат
			return result;
		};
		/**
		 * domainFn Функция извлечения данных доменного имени
		 * @param text текст для парсинга
		 */
		auto domainFn = [this](const wstring & text) noexcept {
			// Результат работы функции
			data_t result;
			// Если текст передан
			if(!text.empty()){
				// Результат работы регулярного выражения
				wsmatch match;
				// Выполняем проверку адреса сайта
				regex_search(text, match, this->_expressDomain);
				// Если результат найден
				if(!match.empty() && (match.size() >= 7)){
					// Запоминаем тип параметра
					result.type = types_t::DHOST;
					// Запоминаем uri адрес
					result.uri = match[0].str();
					// Запоминаем название домена
					result.data = match[2].str();
					// Запоминаем порт запроса
					result.port = match[4].str();
					// Запоминаем путь запроса
					result.path = match[5].str();
					// Запоминаем домен верхнего уровня
					result.domain = match[3].str();
					// Запоминаем параметры запроса
					result.params = match[6].str();
					// Запоминаем протокол
					result.protocol = match[1].str();
					// Если протокол не указан
					if(result.protocol.empty()){
						// Если домен верхнего уровня не является таковым, очищаем все
						if((this->_national.count(result.domain) < 1)
						&& (this->_general.count(result.domain) < 1)
						&& (this->_user.count(result.domain) < 1)){
							// Очищаем блок полученных данных
							result.path.clear();
							result.port.clear();
							result.user.clear();
							result.domain.clear();
							result.params.clear();
							result.protocol.clear();
							result.type = types_t::WRONG;
						}
					}
				}
			}
			// Выводим результат
			return result;
		};
		/**
		 * ipFn Функция извлечения данных ip адресов
		 * @param text текст для парсинга
		 */
		auto ipFn = [this](const wstring & text) noexcept {
			// Результат работы функции
			data_t result;
			// Если текст передан
			if(!text.empty()){
				// Результат работы регулярного выражения
				wsmatch match;
				// Выполняем проверку
				regex_search(text, match, this->_expressIP);
				// Если результат найден
				if(!match.empty() && (match.size() >= 5)){
					// Запоминаем uri адрес
					result.uri = match[0].str();
					// Извлекаем полученные данные
					const wstring mac = match[2].str();
					const wstring ip4 = match[4].str();
					const wstring ip6 = match[3].str();
					const wstring network = match[1].str();
					// Если это MAC адрес
					if(!mac.empty()){
						// Запоминаем сам параметр
						result.data = mac;
						// Запоминаем тип параметра
						result.type = types_t::MAC;
					// Если это IPv4 адрес
					} else if(!ip4.empty()){
						// Запоминаем сам параметр
						result.data = ip4;
						// Запоминаем тип параметра
						result.type = types_t::IPV4;
					// Если это IPv6 адрес
					} else if(!ip6.empty()) {
						// Запоминаем сам параметр
						result.data = ip6;
						// Запоминаем тип параметра
						result.type = types_t::IPV6;
					// Если это параметры сети
					} else if(!network.empty()) {
						// Запоминаем сам параметр
						result.data = network;
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
		// Запрашиваем данные электронной почты
		data_t email = emailFn(text);
		// Если тип не получен
		if(email.type == types_t::NONE){
			// Запрашиваем данные доменного имени
			data_t domain = domainFn(text);
			// Если данные домена не получены или протокол не найден
			if((domain.type == types_t::NONE)
			|| (domain.type == types_t::WRONG)){
				// Выполняем поиск ip адресов
				data_t ip = ipFn(text);
				// Если результат получен
				if(ip.type != types_t::NONE)
					// Устанавливаем IP адрес
					result = std::move(ip);
				// Если же ip адре не получен то возвращаем данные домена
				else result = std::move(domain);
			// Иначе запоминаем результат
			} else result = std::move(domain);
		// Иначе запоминаем результат
		} else result = std::move(email);
	}
	// Выводим результат
	return result;
}
/**
 * letters Метод добавления букв алфавита
 * @param letters список букв алфавита
 */
void awh::NWT::letters(const wstring & letters) noexcept {
	// Если буквы переданы запоминаем их
	if(!letters.empty()){
		// Устанавливаем буквы алфавита
		this->_letters = letters;
		// Устанавливаем регулярное выражение для проверки электронной почты
		this->_expressEmail = wregex(
			wstring(L"((?:([\\w\\-")
			+ this->_letters
			+ wstring(L"]+)\\@)(\\[(?:\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]|(?:\\d{1,3}(?:\\.\\d{1,3}){3})|(?:(?:xn\\-\\-[\\w\\d]+\\.){0,100}(?:xn\\-\\-[\\w\\d]+)|(?:[\\w\\-")
			+ this->_letters
			+ wstring(L"]+\\.){0,100}[\\w\\-")
			+ this->_letters
			+ wstring(L"]+)\\.(xn\\-\\-[\\w\\d]+|[a-z")
			+ this->_letters
			+ wstring(L"]+)))"),
			wregex::ECMAScript | wregex::icase
		);
		// Устанавливаем правило регулярного выражения
		this->_expressDomain = wregex(
			wstring(L"(?:(http[s]?)\\:\\/\\/)?(\\[(?:\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]|(?:\\d{1,3}(?:\\.\\d{1,3}){3})|(?:(?:xn\\-\\-[\\w\\d]+\\.){0,100}(?:xn\\-\\-[\\w\\d]+)|(?:[\\w\\-")
			+ this->_letters
			+ wstring(L"]+\\.){0,100}[\\w\\-")
			+ this->_letters
			+ wstring(L"]+)\\.(xn\\-\\-[\\w\\d]+|[a-z")
			+ this->_letters
			+ wstring(L"]+))(?:\\:(\\d+))?((?:\\/[\\w\\-]+){0,100}(?:$|\\/|\\.[\\w]+)|\\/)?(\\?(?:[\\w\\-\\.\\~\\:\\#\\[\\]\\@\\!\\$\\&\\'\\(\\)\\*\\+\\,\\;\\=]+)?)?"),
			wregex::ECMAScript | wregex::icase
		);
		// Устанавливаем правило регулярного выражения
		this->_expressIP = wregex(
			// Если это сеть
			L"(?:((?:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}[\\:]{2}|[\\:]{2})|[\\:]{2}))\\/(?:\\d{1,3}(?:\\.\\d{1,3}){3}|\\d+))|"
			// Определение мак адреса
			L"([a-f\\d]{2}(?:\\:[a-f\\d]{2}){5})|"
			// Определение ip6 адреса
			L"(?:(?:http[s]?\\:[\\/]{2})?(?:\\[?([\\:]{2}ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}[\\:]{2}|[\\:]{2})|[\\:]{2}))\\]?)(?:\\:\\d+)?\\/?)|"
			// Определение ip4 адреса
			L"(?:(?:http[s]?\\:[\\/]{2})?(\\d{1,3}(?:\\.\\d{1,3}){3})(?:\\:\\d+)?\\/?))",
			wregex::ECMAScript | wregex::icase
		);
	}
}
/**
 * NWT Конструктор
 * @param letters список букв алфавита
 * @param text    текст для парсинга
 */
awh::NWT::NWT(const wstring & letters, const wstring & text) noexcept : _letters(L"") {
	// Создаем список национальных доменов
	this->_national.emplace(L"ac");
	this->_national.emplace(L"ad");
	this->_national.emplace(L"ae");
	this->_national.emplace(L"af");
	this->_national.emplace(L"ag");
	this->_national.emplace(L"ai");
	this->_national.emplace(L"al");
	this->_national.emplace(L"am");
	this->_national.emplace(L"an");
	this->_national.emplace(L"ao");
	this->_national.emplace(L"aq");
	this->_national.emplace(L"ar");
	this->_national.emplace(L"as");
	this->_national.emplace(L"at");
	this->_national.emplace(L"au");
	this->_national.emplace(L"aw");
	this->_national.emplace(L"ax");
	this->_national.emplace(L"az");
	this->_national.emplace(L"ba");
	this->_national.emplace(L"bb");
	this->_national.emplace(L"bd");
	this->_national.emplace(L"be");
	this->_national.emplace(L"bf");
	this->_national.emplace(L"bg");
	this->_national.emplace(L"bh");
	this->_national.emplace(L"bi");
	this->_national.emplace(L"bj");
	this->_national.emplace(L"bm");
	this->_national.emplace(L"bn");
	this->_national.emplace(L"bo");
	this->_national.emplace(L"br");
	this->_national.emplace(L"bs");
	this->_national.emplace(L"bt");
	this->_national.emplace(L"bv");
	this->_national.emplace(L"bw");
	this->_national.emplace(L"by");
	this->_national.emplace(L"bz");
	this->_national.emplace(L"ca");
	this->_national.emplace(L"cc");
	this->_national.emplace(L"cd");
	this->_national.emplace(L"cf");
	this->_national.emplace(L"cg");
	this->_national.emplace(L"ch");
	this->_national.emplace(L"ci");
	this->_national.emplace(L"ck");
	this->_national.emplace(L"cl");
	this->_national.emplace(L"cm");
	this->_national.emplace(L"cn");
	this->_national.emplace(L"co");
	this->_national.emplace(L"cr");
	this->_national.emplace(L"cu");
	this->_national.emplace(L"cv");
	this->_national.emplace(L"cx");
	this->_national.emplace(L"cy");
	this->_national.emplace(L"cz");
	this->_national.emplace(L"dd");
	this->_national.emplace(L"de");
	this->_national.emplace(L"dj");
	this->_national.emplace(L"dk");
	this->_national.emplace(L"dm");
	this->_national.emplace(L"do");
	this->_national.emplace(L"dz");
	this->_national.emplace(L"ec");
	this->_national.emplace(L"ee");
	this->_national.emplace(L"eg");
	this->_national.emplace(L"er");
	this->_national.emplace(L"es");
	this->_national.emplace(L"et");
	this->_national.emplace(L"eu");
	this->_national.emplace(L"fi");
	this->_national.emplace(L"fj");
	this->_national.emplace(L"fk");
	this->_national.emplace(L"fm");
	this->_national.emplace(L"fo");
	this->_national.emplace(L"fr");
	this->_national.emplace(L"ga");
	this->_national.emplace(L"gb");
	this->_national.emplace(L"gd");
	this->_national.emplace(L"ge");
	this->_national.emplace(L"gf");
	this->_national.emplace(L"gg");
	this->_national.emplace(L"gh");
	this->_national.emplace(L"gi");
	this->_national.emplace(L"gl");
	this->_national.emplace(L"gm");
	this->_national.emplace(L"gn");
	this->_national.emplace(L"gp");
	this->_national.emplace(L"gq");
	this->_national.emplace(L"gr");
	this->_national.emplace(L"gs");
	this->_national.emplace(L"gt");
	this->_national.emplace(L"gu");
	this->_national.emplace(L"gw");
	this->_national.emplace(L"gy");
	this->_national.emplace(L"hk");
	this->_national.emplace(L"hm");
	this->_national.emplace(L"hn");
	this->_national.emplace(L"hr");
	this->_national.emplace(L"ht");
	this->_national.emplace(L"hu");
	this->_national.emplace(L"id");
	this->_national.emplace(L"ie");
	this->_national.emplace(L"il");
	this->_national.emplace(L"im");
	this->_national.emplace(L"in");
	this->_national.emplace(L"io");
	this->_national.emplace(L"iq");
	this->_national.emplace(L"ir");
	this->_national.emplace(L"is");
	this->_national.emplace(L"it");
	this->_national.emplace(L"je");
	this->_national.emplace(L"jm");
	this->_national.emplace(L"jo");
	this->_national.emplace(L"jp");
	this->_national.emplace(L"ke");
	this->_national.emplace(L"kg");
	this->_national.emplace(L"kh");
	this->_national.emplace(L"ki");
	this->_national.emplace(L"km");
	this->_national.emplace(L"kn");
	this->_national.emplace(L"kp");
	this->_national.emplace(L"kr");
	this->_national.emplace(L"kw");
	this->_national.emplace(L"ky");
	this->_national.emplace(L"kz");
	this->_national.emplace(L"la");
	this->_national.emplace(L"lb");
	this->_national.emplace(L"lc");
	this->_national.emplace(L"li");
	this->_national.emplace(L"lk");
	this->_national.emplace(L"lr");
	this->_national.emplace(L"ls");
	this->_national.emplace(L"lt");
	this->_national.emplace(L"lu");
	this->_national.emplace(L"lv");
	this->_national.emplace(L"ly");
	this->_national.emplace(L"ma");
	this->_national.emplace(L"mc");
	this->_national.emplace(L"md");
	this->_national.emplace(L"me");
	this->_national.emplace(L"mg");
	this->_national.emplace(L"mh");
	this->_national.emplace(L"mk");
	this->_national.emplace(L"ml");
	this->_national.emplace(L"mm");
	this->_national.emplace(L"mn");
	this->_national.emplace(L"mo");
	this->_national.emplace(L"mp");
	this->_national.emplace(L"mq");
	this->_national.emplace(L"mr");
	this->_national.emplace(L"ms");
	this->_national.emplace(L"mt");
	this->_national.emplace(L"mu");
	this->_national.emplace(L"mv");
	this->_national.emplace(L"mw");
	this->_national.emplace(L"mx");
	this->_national.emplace(L"my");
	this->_national.emplace(L"mz");
	this->_national.emplace(L"na");
	this->_national.emplace(L"nc");
	this->_national.emplace(L"ne");
	this->_national.emplace(L"nf");
	this->_national.emplace(L"ng");
	this->_national.emplace(L"ni");
	this->_national.emplace(L"nl");
	this->_national.emplace(L"no");
	this->_national.emplace(L"np");
	this->_national.emplace(L"nr");
	this->_national.emplace(L"nu");
	this->_national.emplace(L"nz");
	this->_national.emplace(L"om");
	this->_national.emplace(L"pa");
	this->_national.emplace(L"pe");
	this->_national.emplace(L"pf");
	this->_national.emplace(L"pg");
	this->_national.emplace(L"ph");
	this->_national.emplace(L"pk");
	this->_national.emplace(L"pl");
	this->_national.emplace(L"pm");
	this->_national.emplace(L"pn");
	this->_national.emplace(L"pr");
	this->_national.emplace(L"ps");
	this->_national.emplace(L"pt");
	this->_national.emplace(L"pw");
	this->_national.emplace(L"py");
	this->_national.emplace(L"qa");
	this->_national.emplace(L"re");
	this->_national.emplace(L"ro");
	this->_national.emplace(L"rs");
	this->_national.emplace(L"ru");
	this->_national.emplace(L"рф");
	this->_national.emplace(L"ру");
	this->_national.emplace(L"су");
	this->_national.emplace(L"rw");
	this->_national.emplace(L"sa");
	this->_national.emplace(L"sb");
	this->_national.emplace(L"sc");
	this->_national.emplace(L"sd");
	this->_national.emplace(L"se");
	this->_national.emplace(L"sg");
	this->_national.emplace(L"sh");
	this->_national.emplace(L"si");
	this->_national.emplace(L"sj");
	this->_national.emplace(L"sk");
	this->_national.emplace(L"sl");
	this->_national.emplace(L"sm");
	this->_national.emplace(L"sn");
	this->_national.emplace(L"so");
	this->_national.emplace(L"sr");
	this->_national.emplace(L"st");
	this->_national.emplace(L"su");
	this->_national.emplace(L"sv");
	this->_national.emplace(L"sy");
	this->_national.emplace(L"sz");
	this->_national.emplace(L"tc");
	this->_national.emplace(L"td");
	this->_national.emplace(L"tf");
	this->_national.emplace(L"tg");
	this->_national.emplace(L"th");
	this->_national.emplace(L"tj");
	this->_national.emplace(L"tk");
	this->_national.emplace(L"tl");
	this->_national.emplace(L"tm");
	this->_national.emplace(L"tn");
	this->_national.emplace(L"to");
	this->_national.emplace(L"tp");
	this->_national.emplace(L"tr");
	this->_national.emplace(L"tt");
	this->_national.emplace(L"tv");
	this->_national.emplace(L"tw");
	this->_national.emplace(L"tz");
	this->_national.emplace(L"ua");
	this->_national.emplace(L"ug");
	this->_national.emplace(L"uk");
	this->_national.emplace(L"us");
	this->_national.emplace(L"uy");
	this->_national.emplace(L"uz");
	this->_national.emplace(L"va");
	this->_national.emplace(L"vc");
	this->_national.emplace(L"ve");
	this->_national.emplace(L"vg");
	this->_national.emplace(L"vi");
	this->_national.emplace(L"vn");
	this->_national.emplace(L"vu");
	this->_national.emplace(L"wf");
	this->_national.emplace(L"ws");
	this->_national.emplace(L"ye");
	this->_national.emplace(L"yt");
	this->_national.emplace(L"za");
	this->_national.emplace(L"zm");
	this->_national.emplace(L"zw");
	this->_national.emplace(L"krd");
	this->_national.emplace(L"укр");
	this->_national.emplace(L"срб");
	this->_national.emplace(L"мон");
	this->_national.emplace(L"бел");
	this->_national.emplace(L"ком");
	this->_national.emplace(L"нет");
	this->_national.emplace(L"биз");
	this->_national.emplace(L"орг");
	this->_national.emplace(L"инфо");
	// Создаем список общих доменов
	this->_general.emplace(L"app");
	this->_general.emplace(L"biz");
	this->_general.emplace(L"cat");
	this->_general.emplace(L"com");
	this->_general.emplace(L"edu");
	this->_general.emplace(L"eus");
	this->_general.emplace(L"gov");
	this->_general.emplace(L"int");
	this->_general.emplace(L"mil");
	this->_general.emplace(L"net");
	this->_general.emplace(L"one");
	this->_general.emplace(L"ong");
	this->_general.emplace(L"onl");
	this->_general.emplace(L"ooo");
	this->_general.emplace(L"org");
	this->_general.emplace(L"pro");
	this->_general.emplace(L"red");
	this->_general.emplace(L"ren");
	this->_general.emplace(L"tel");
	this->_general.emplace(L"xxx");
	this->_general.emplace(L"xyz");
	this->_general.emplace(L"rent");
	this->_general.emplace(L"name");
	this->_general.emplace(L"aero");
	this->_general.emplace(L"mobi");
	this->_general.emplace(L"jobs");
	this->_general.emplace(L"info");
	this->_general.emplace(L"coop");
	this->_general.emplace(L"asia");
	this->_general.emplace(L"army");
	this->_general.emplace(L"pics");
	this->_general.emplace(L"pink");
	this->_general.emplace(L"plus");
	this->_general.emplace(L"porn");
	this->_general.emplace(L"post");
	this->_general.emplace(L"prof");
	this->_general.emplace(L"qpon");
	this->_general.emplace(L"rest");
	this->_general.emplace(L"rich");
	this->_general.emplace(L"site");
	this->_general.emplace(L"yoga");
	this->_general.emplace(L"zone");
	this->_general.emplace(L"rehab");
	this->_general.emplace(L"press");
	this->_general.emplace(L"poker");
	this->_general.emplace(L"parts");
	this->_general.emplace(L"party");
	this->_general.emplace(L"audio");
	this->_general.emplace(L"archi");
	this->_general.emplace(L"dance");
	this->_general.emplace(L"actor");
	this->_general.emplace(L"adult");
	this->_general.emplace(L"photo");
	this->_general.emplace(L"pizza");
	this->_general.emplace(L"place");
	this->_general.emplace(L"travel");
	this->_general.emplace(L"review");
	this->_general.emplace(L"repair");
	this->_general.emplace(L"report");
	this->_general.emplace(L"racing");
	this->_general.emplace(L"photos");
	this->_general.emplace(L"physio");
	this->_general.emplace(L"online");
	this->_general.emplace(L"museum");
	this->_general.emplace(L"agency");
	this->_general.emplace(L"active");
	this->_general.emplace(L"reviews");
	this->_general.emplace(L"rentals");
	this->_general.emplace(L"recipes");
	this->_general.emplace(L"organic");
	this->_general.emplace(L"academy");
	this->_general.emplace(L"auction");
	this->_general.emplace(L"plumbing");
	this->_general.emplace(L"pharmacy");
	this->_general.emplace(L"airforce");
	this->_general.emplace(L"attorney");
	this->_general.emplace(L"partners");
	this->_general.emplace(L"pictures");
	this->_general.emplace(L"feedback");
	this->_general.emplace(L"property");
	this->_general.emplace(L"republican");
	this->_general.emplace(L"associates");
	this->_general.emplace(L"apartments");
	this->_general.emplace(L"accountant");
	this->_general.emplace(L"properties");
	this->_general.emplace(L"photography");
	this->_general.emplace(L"accountants");
	this->_general.emplace(L"productions");
	// Если буквы переданы запоминаем их
	this->letters(letters);
	// Если текст передан то выполняем парсинг
	if(!text.empty()) this->parse(text);
}
