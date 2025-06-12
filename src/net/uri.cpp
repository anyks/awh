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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <net/uri.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

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
	// Выполняем освобождение памяти IP-адреса
	string().swap(this->ip);
	// Выполняем освобождение памяти хоста сервера
	string().swap(this->host);
	// Выполняем освобождение памяти имени пользователя
	string().swap(this->user);
	// Выполняем освобождение памяти пароля пользователя
	string().swap(this->pass);
	// Выполняем освобождение памяти доменного имени
	string().swap(this->domain);
	// Выполняем освобождение памяти протокола передачи данных
	string().swap(this->schema);
	// Выполняем освобождение памяти якоря URL-запроса
	string().swap(this->anchor);
	// Выполняем освобождение памяти пути URL-запроса
	vector <string> ().swap(this->path);
	// Выполняем освобождение памяти параметров URL-запроса
	vector <pair <string, string>> ().swap(this->params);
}
/**
 * empty Метод проверки на существование данных
 * @return результат проверки
 */
bool awh::URI::URL::empty() const noexcept {
	// Выполняем проверку на существование данных
	return (
		this->host.empty() && this->ip.empty() &&
		this->user.empty() && this->pass.empty() &&
		this->domain.empty() && this->path.empty() &&
		this->params.empty() && this->anchor.empty()
	);
}
/**
 * Оператор [=] перемещения параметров URL-адреса
 * @param url объект URL-адреса для получения параметров
 * @return    параметры URL-адреса
 */
awh::URI::URL & awh::URI::URL::operator = (url_t && url) noexcept {
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
			this->ip = ::move(url.ip);
		// Выполняем удаление IP-адреса
		else this->ip.clear();
		// Если хост сервера передан
		if(!url.host.empty())
			// Выполняем копирование хоста сервера
			this->host = ::move(url.host);
		// Выполняем удаление хоста сервера
		else this->host.clear();
		// Если доменное имя сервера передано
		if(!url.domain.empty())
			// Выполняем копирование доменного имени сервера
			this->domain = ::move(url.domain);
		// Выполняем удаление доменного имени сервера
		else this->domain.clear();
		// Если протокол передачи данных передан
		if(!url.schema.empty())
			// Выполняем копирование протокола передачи данных
			this->schema = ::move(url.schema);
		// Выполняем удаление протокола передачи данных
		else this->schema.clear();
		// Если якорь URL-запроса передан
		if(!url.anchor.empty())
			// Выполняем копирование якоря URL-запроса
			this->anchor = ::move(url.anchor);
		// Выполняем удаление якоря URL-запроса
		else this->anchor.clear();
		// Если пользователь передан
		if(!url.user.empty())
			// Выполняем копирование пользователя
			this->user = ::move(url.user);
		// Выполняем удаление пользователя
		else this->user.clear();
		// Если пароль передан
		if(!url.pass.empty())
			// Выполняем копирование пароля
			this->pass = ::move(url.pass);
		// Выполняем удаление пароля
		else this->pass.clear();
		// Если путь передан
		if(!url.path.empty())
			// Выполняем копирование пути
			this->path = ::move(url.path);
		// Выполняем удаление пути
		else this->path.clear();
		// Если параметры переданы
		if(!url.params.empty())
			// Выполняем копирование параметров
			this->params = ::move(url.params);
		// Выполняем удаление параметров
		else this->params.clear();
		// Выполняем копирование функции обратного вызова
		this->callback = url.callback;
	/**
	 * Если возникает ошибка
	 */
	} catch(const length_error & error) {
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
	return (* this);
}
/**
 * Оператор [=] присванивания параметров URL-адреса
 * @param url объект URL-адреса для получения параметров
 * @return    параметры URL-адреса
 */
awh::URI::URL & awh::URI::URL::operator = (const url_t & url) noexcept {
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
	} catch(const length_error & error) {
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
	return (* this);
}
/**
 * Оператор сравнения
 * @param url параметры URL-адреса
 * @return    результат сравнения
 */
bool awh::URI::URL::operator == (const url_t & url) noexcept {
	// Выполняем проверку на соответствие основным параметрам
	bool result = (
		(this->port == url.port) &&
		(this->family == url.family) &&
		(this->ip.compare(url.ip) == 0) &&
		(this->host.compare(url.host) == 0) &&
		(this->user.compare(url.user) == 0) &&
		(this->pass.compare(url.pass) == 0) &&
		(this->domain.compare(url.domain) == 0) &&
		(this->schema.compare(url.schema) == 0) &&
		(this->anchor.compare(url.anchor) == 0)
	);
	// Если параметры соответствуют
	if(result){
		// Если пути заполненны
		if((result = (this->path.size() == url.path.size()))){
			// Выполняем проверку соответствия путей
			for(size_t i = 0; i < this->path.size(); i++){
				// Выполняем проверку соответствия путей
				result = (this->path.at(i).compare(url.path.at(i)) == 0);
				// Если пути не соответствуют
				if(!result)
					// Выходим из функции
					return result;
			}
			// Если размеры параметров соответствуют
			if((result = (this->params.size() == url.params.size()))){
				// Выполняем перебор всего списка параметров
				for(size_t i = 0; i < this->params.size(); i++){
					// Получаем текущее значение параметра
					const auto & params = this->params.at(i);
					// Если параметры соответствуют
					result = (
						(params.first.compare(params.first) == 0) &&
						(params.second.compare(params.second) == 0)
					);
					// Если пути не соответствуют
					if(!result)
						// Выходим из функции
						return result;
				}
			}
		}
	}
	// Выполняем сравнение URL-параметров
	return result;
}
/**
 * URL Конструктор перемещения
 * @param url параметры URL-адреса
 */
awh::URI::URL::URL(url_t && url) noexcept {
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
			this->ip = ::move(url.ip);
		// Выполняем удаление IP-адреса
		else this->ip.clear();
		// Если хост сервера передан
		if(!url.host.empty())
			// Выполняем копирование хоста сервера
			this->host = ::move(url.host);
		// Выполняем удаление хоста сервера
		else this->host.clear();
		// Если доменное имя сервера передано
		if(!url.domain.empty())
			// Выполняем копирование доменного имени сервера
			this->domain = ::move(url.domain);
		// Выполняем удаление доменного имени сервера
		else this->domain.clear();
		// Если протокол передачи данных передан
		if(!url.schema.empty())
			// Выполняем копирование протокола передачи данных
			this->schema = ::move(url.schema);
		// Выполняем удаление протокола передачи данных
		else this->schema.clear();
		// Если якорь URL-запроса передан
		if(!url.anchor.empty())
			// Выполняем копирование якоря URL-запроса
			this->anchor = ::move(url.anchor);
		// Выполняем удаление якоря URL-запроса
		else this->anchor.clear();
		// Если пользователь передан
		if(!url.user.empty())
			// Выполняем копирование пользователя
			this->user = ::move(url.user);
		// Выполняем удаление пользователя
		else this->user.clear();
		// Если пароль передан
		if(!url.pass.empty())
			// Выполняем копирование пароля
			this->pass = ::move(url.pass);
		// Выполняем удаление пароля
		else this->pass.clear();
		// Если путь передан
		if(!url.path.empty())
			// Выполняем копирование пути
			this->path = ::move(url.path);
		// Выполняем удаление пути
		else this->path.clear();
		// Если параметры переданы
		if(!url.params.empty())
			// Выполняем копирование параметров
			this->params = ::move(url.params);
		// Выполняем удаление параметров
		else this->params.clear();
		// Выполняем копирование функции обратного вызова
		this->callback = url.callback;
	/**
	 * Если возникает ошибка
	 */
	} catch(const length_error & error) {
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
 * URL Конструктор копирования
 * @param url параметры URL-адреса
 */
awh::URI::URL::URL(const url_t & url) noexcept {
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
	} catch(const length_error & error) {
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
 * URL Конструктор
 */
awh::URI::URL::URL() noexcept :
 port(0), family(AF_INET), ip{""}, host{""},
 user{""}, pass{""}, domain{""}, schema{""},
 anchor{""}, callback(nullptr) {}
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
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем парсинг URL адреса
			auto uri = this->split(url);
			// Если сплит URL адреса, прошёл успешно
			if(!uri.empty()){
				// Выполняем поиск схемы протокола
				auto i = uri.find(flag_t::SCHEMA);
				// Если схема протокола получена
				if(i != uri.end())
					// Выполняем извлечение схемы протокола
					result.schema = ::move(i->second);
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
						result.host = ::move(i->second);
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
					result.anchor = this->decode(i->second);
				// Выполняем поиск порта запроса
				i = uri.find(flag_t::PORT);
				// Если порт запроса получен
				if(i != uri.end()){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем извлечение порта запроса
						result.port = ::stoi(i->second);
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception &) {
						// Выполняем извлечение порта запроса
						result.port = 0;
					}
				}
				// Выполняем поиск пользователя запроса
				i = uri.find(flag_t::LOGIN);
				// Если пользователь запроса получен
				if(i != uri.end())
					// Выполняем извлечение пользователя запроса
					result.user = ::move(i->second);
				// Выполняем поиск пароля пользователя запроса
				i = uri.find(flag_t::PASS);
				// Если пароль пользователя запроса получен
				if(i != uri.end())
					// Выполняем извлечение пароля пользователя запроса
					result.pass = ::move(i->second);
				// Выполняем поиск хоста запроса
				i = uri.find(flag_t::HOST);
				// Если хост запроса получен
				if(i != uri.end()){
					// Выполняем извлечение хоста запроса
					result.host = ::move(i->second);
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
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(url), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
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
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем sha1 хэш строки
			const string & sha1 = this->_hash.hashing <string> (text, hash_t::type_t::SHA1);
			// Если строка получена
			if(!sha1.empty()){
				// Извлекаем первую часть хэша
				const string & first = sha1.substr(0, 8);
				// Извлекаем вторую часть хэша
				const string & second = sha1.substr(35);
				// Формируем результат
				result = this->_fmk->format("W/\"%s-%s\"", first.c_str(), second.c_str());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * encode Метод кодирования строки в URL-адресе
 * @param text строка текста для кодирования
 * @return     результат кодирования
 */
string awh::URI::encode(const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если строка передана
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём поток
			ostringstream ss;
			// Заполняем поток нулями
			ss.fill('0');
			// Переключаемся на 16-ю систему счисления
			ss << hex;
			// Перебираем все символы
			for(char letter : text){
				// Не трогаем буквенно-цифровые и другие допустимые символы.
				if(isalnum(letter) || (letter == '-') || (letter == '_') || (letter == '.') || (letter == '~') || (letter == '@') ||
				 ((letter >= '0') && (letter <= '9')) || ((letter >= 'A') && (letter <= 'Z')) || ((letter >= 'a') && (letter <= 'z'))){
					// Записываем в поток символ, как он есть
					ss << letter;
					// Пропускаем итерацию
					continue;
				}
				/**
				 * Любые другие символы закодированы в процентах
				 */
				// Переводим символы в верхний регистр
				ss << uppercase;
				// Записываем в поток, код символа
				ss << '%' << setw(2) << static_cast <int16_t> (static_cast <uint8_t> (letter));
				// Убираем верхний регистр
				ss << nouppercase;
			}
			// Получаем результат
			result = ss.str();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * decode Метод декодирования строки в URL-адресе
 * @param text строка текста для декодирования
 * @return     результат декодирования
 */
string awh::URI::decode(const string & text) const noexcept {
	// Результат работы функции
	string result = "";
	// Если строка передана
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём бинарный буфер данных
			char buffer[3];
			// Устанавливаем завершение строки
			buffer[2] = '\0';
			// Код символа в 16-м виде
			uint16_t hex = 0;
			// Смещение в текстовом буфере
			const char * offset = nullptr;
			// Выделяем память для строки
			result.reserve(text.capacity());
			// Переходим по всей длине строки
			for(size_t i = 0; i < text.length(); i++){
				// Получаем текущее смещение в текстовом буфере
				offset = (text.c_str() + i);
				// Если это не проценты
				if(offset[0] != '%'){
					// Если это объединение двух слов
					if(offset[0] == '+')
						// Выполняем добавление разделителя
						result.append(1, ' ');
					// Иначе копируем букву как она есть
					else result.append(1, offset[0]);
				// Если же это проценты
				} else {
					// Выполняем копирование в бинарный буфер полученных байт
					::memcpy(buffer, offset + 1, 2);
					// Извлекаем из 16-х символов наш код числа
					sscanf(buffer, "%hx", &hex);
					// Запоминаем полученный символ
					result.append(1, static_cast <char> (hex));
					// Смещаем итератор
					i += 2;
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
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
		/**
		 * Выполняем отлов ошибок
		 */
		try {
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
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(url), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
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
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем сборку пути запроса
			const string & path = this->joinPath(url.path);
			// Выполняем сборку параметров запроса
			const string & params = this->joinParams(url.params);
			// Выполняем сборку якоря запроса
			const string & anchor = (!url.anchor.empty() ? this->_fmk->format("#%s", this->encode(url.anchor).c_str()) : "");
			// Выполняем генерацию URL адреса
			const string & uri = ((url.callback != nullptr) ? this->_fmk->format("&%s", url.callback(&url, this).c_str()) : "");
			// Иначе порт не устанавливаем
			result = this->_fmk->format("%s%s%s%s", path.c_str(), params.c_str(), uri.c_str(), anchor.c_str());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(url), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
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
		/**
		 * Выполняем отлов ошибок
		 */
		try {
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
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(url), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
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
	/**
	 * Выполняем отлов ошибок
	 */
	try {
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
		// Если фукнция обратного вызова не указана
		if(dest.callback == nullptr)
			// Выполняем установку функции обратного вызова
			dest.callback = src.callback;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(dest, src), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
}
/**
 * combine Метод комбинации двух адресов
 * @param dest адрес места назначения
 * @param src  исходный адрес для объединения
 */
void awh::URI::combine(url_t & dest, const url_t & src) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
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
		// Выполняем установку функции обратного вызова
		dest.callback = src.callback;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(dest, src), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
}
/**
 * append Метод добавления к URL адресу параметров запроса
 * @param url    параметры URL-запроса
 * @param params параметры для добавления
 */
void awh::URI::append(url_t & url, const string & params) const noexcept {
	// Если данные переданы
	if(!url.empty() && !params.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем парсинг URL адреса
			auto uri = this->split(params);
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
						url.host = ::move(i->second);
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
					url.anchor = ::move(i->second);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(url, params), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
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
map <awh::URI::flag_t, string> awh::URI::split(const string & uri) const noexcept {
	// Результат работы функции
	map <flag_t, string> result;
	// Если URI передан
	if(!uri.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
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
							} else result.emplace(flag_t::LOGIN, user);
						// Переводим название хоста в нижний регистр
						} else i->second = this->_fmk->transform(i->second, fmk_t::transform_t::LOWER);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(uri), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
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
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Флаг поиска амперсанда
			bool ampersand = false;
			// Текущая позиция в строке и начальная позиция поиска
			size_t pos = string::npos, begin = 0;
			/**
			 * Выполняем перебор всех параметров
			 */
			do {
				// Если мы амперсанд не ищем
				if(!ampersand){
					// Выполняем поиск разделителя ключа и значения
					if((pos = uri.find('=', begin)) != string::npos){
						// Добавляем полученный результат
						result.push_back(make_pair(this->decode(uri.substr(begin, pos - begin)), ""));
						// Запоминаем позицию поиска следующего элемента
						begin = (pos + 1);
						// Устанавливаем флаг поиска амперсанда
						ampersand = !ampersand;
					// Запрещаем извлекать значение, так-как строка уже поломана
					} else begin = uri.size();
				// Если мы ищем амперсанд
				} else {
					// Выполняем поиск разделителя параметров
					if((pos = uri.find('&', begin)) != string::npos){
						// Добавляем полученное значение ключа
						result.back().second = this->decode(uri.substr(begin, pos - begin));
						// Запоминаем позицию поиска следующего элемента
						begin = (pos + 1);
						// Снимаем флаг поиска амперсанда
						ampersand = !ampersand;
					}
				}
			/**
			 * Если искать дальше некуда, выходим
			 */
			} while(pos != string::npos);
			// Если значение получено для ключа
			if(begin < uri.size())
				// Добавляем полученное значение ключа
				result.back().second = this->decode(uri.substr(begin));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(uri), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
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
vector <string> awh::URI::splitPath(const string & path, const char delim) const noexcept {
	// Результат работы функции
	vector <string> result;
	// Если данные переданы
	if(!path.empty() && (delim > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём бинарный буфер данных
			char buffer[3];
			// Устанавливаем завершение строки
			buffer[2] = '\0';
			// Текст названия каталога
			string name = "";
			// Код символа в 16-м виде
			uint16_t hex = 0;
			// Смещение в текстовом буфере
			const char * offset = nullptr;
			// Переходим по всей длине строки
			for(size_t i = 0; i < path.length(); i++){
				// Получаем текущее смещение в текстовом буфере
				offset = (path.c_str() + i);
				// Определяем текущий символ
				switch(offset[0]){
					// Если символом является знак процента %
					case '%': {
						// Выполняем копирование в бинарный буфер полученных байт
						::memcpy(buffer, offset + 1, 2);
						// Извлекаем из 16-х символов наш код числа
						sscanf(buffer, "%hx", &hex);
						// Запоминаем полученный символ
						name.append(1, static_cast <char> (hex));
						// Смещаем итератор
						i += 2;
					} break;
					// Если мы получили разделитель двух слов
					case '+':
						// Выполняем добавление разделителя
						name.append(1, ' ');
					break;
					// Если мы получили любой другой символ
					default: {
						// Если мы получили разделитель каталога
						if(offset[0] == delim){
							// Если это не первый символ
							if(i > 0){
								// Если название адреса получено
								if(!name.empty() && (name.front() != '.'))
									// Выполняем добавление полученного названия в список
									result.push_back(name);
								// Выполняем очистку результата
								name.clear();
							}
						// Выполняем формирование названия пути
						} else name.append(1, offset[0]);
					}
				}
			}
			// Если название адреса получено
			if(!name.empty())
				// Выполняем добавление полученного названия в список
				result.push_back(name);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(path, delim), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
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
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Добавляем системный параметр
			result.append(1, '?');
			// Переходим по всему списку параметров
			for(auto & item : uri){
				// Если параметры уже были добавлены
				if(result.length() > 1)
					// Выполняем добавление разделителя
					result.append(1, '&');
				// Если значение не пустое
				if(!item.second.empty())
					// Добавляем собранные параметры
					result.append(this->_fmk->format("%s=%s", this->encode(item.first).c_str(), this->encode(item.second).c_str()));
				// Если значение пустое, проставляем как есть
				else result.append(this->_fmk->format("%s", this->encode(item.first).c_str()));
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
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
string awh::URI::joinPath(const vector <string> & path, const char delim) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные переданы
	if(!path.empty() && (delim > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Переходим по всему списку параметров
			for(auto & item : path){
				// Если параметр существует
				if(!item.empty()){
					// Добавляем разделитель
					result.append(1, delim);
					// Добавляем собранные параметры
					result.append(this->encode(item));
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	// Выводим путь из одного разделителя
	} else result.assign(1, delim);
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
		/**
		 * Выполняем отлов ошибок
		 */
		try {
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
				if(!port.empty()){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем установку порта
						result.port = ::stoi(port);
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception &) {
						// Выполняем установку порта
						result.port = 0;
					}
				// Если порт не получен но указана схема
				} else if(!schema.empty()) {
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
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(uri, schema), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
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
 * @param log объект для работы с логами
 */
awh::URI::URI(const fmk_t * fmk, const log_t * log) noexcept : _net(log), _hash(log), _fmk(fmk), _log(log) {
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
	// Создаём объект для работы с логами
	log_t log(&fmk);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем создание объекта для работы с URI
		uri_t uri(&fmk, &log);
		// Записываем в поток сгенерированный URL-адрес
		os << (uri = url);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			log.debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			log.print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return os;
}
