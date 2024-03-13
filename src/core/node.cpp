/**
 * @file: node.cpp
 * @date: 2024-03-11
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

// Подключаем заголовочный файл
#include <core/node.hpp>

/**
 * remove Метод удаления всех схем сети
 */
void awh::Node::remove() noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем удаление всей схемы сети
	this->_schemes.clear();
	// Выполняем удаление списка брокеров подключения
	this->_brokers.clear();
}
/**
 * remove Метод удаления схемы сети
 * @param sid идентификатор схемы сети
 */
void awh::Node::remove(const uint16_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден
		if(it != this->_schemes.end())
			// Выполняем удаление схему сети
			this->_schemes.erase(it);
	}
}
/**
 * remove Метод удаления брокера подключения
 * @param bid идентификатор брокера
 */
void awh::Node::remove(const uint64_t bid) noexcept {
	// Если идентификатор брокера подключения передан
	if(bid > 0){
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем поиск брокера подключения
		auto i = this->_brokers.find(bid);
		// Если брокер подключения найден
		if(i != this->_brokers.end()){
			// Выполняем поиск идентификатора схемы сети
			auto j = this->_schemes.find(i->second);
			// Если идентификатор схемы сети найден
			if(j != this->_schemes.end()){
				// Выполняем поиск брокера подключений
				auto k = j->second->_brokers.find(bid);
				// Если брокер подключения найден, удаляем его
				if(k != j->second->_brokers.end())
					// Выполняем удаление брокера подключения
					const_cast <scheme_t *> (j->second)->_brokers.erase(k);
			}
			// Выполняем удаление брокера подключения
			this->_brokers.erase(i);
		}
	}
}
/**
 * has Метод проверки существования схемы сети
 * @param sid идентификатор схемы сети
 * @return    результат проверки
 */
bool awh::Node::has(const uint16_t sid) const noexcept {
	// Выполняем проверку существования схемы сети
	return (this->_schemes.find(sid) != this->_schemes.end());
}
/**
 * has Метод проверки существования брокера подключения
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::Node::has(const uint64_t bid) const noexcept {
	// Выполняем проверку существования идентификатора брокера
	return (this->_brokers.find(bid) != this->_brokers.end());
}
/**
 * sid Метод извлечения идентификатора схемы сети
 * @param bid идентификатор брокера
 * @return    идентификатор схемы сети
 */
uint16_t awh::Node::sid(const uint64_t bid) const noexcept {
	// Если идентификатор брокера подключения передан
	if(bid > 0){
		// Выполняем поиск брокера подключения
		auto it = this->_brokers.find(bid);
		// Если брокер подключения найден
		if(it != this->_brokers.end())
			// Выводим результат
			return it->second;
	}
	// Выводим пустой результат
	return 0;
}
/**
 * broker Метод извлечения брокера подключения
 * @param bid идентификатор брокера
 * @return    объект брокера подключения
 */
const awh::scheme_t::broker_t * awh::Node::broker(const uint64_t bid) const noexcept {
	// Выполняем поиск брокера подключения
	auto i = this->_brokers.find(bid);
	// Если брокер подключения найден
	if(i != this->_brokers.end()){
		// Выполняем поиск идентификатора схемы сети
		auto j = this->_schemes.find(i->second);
		// Если идентификатор схемы сети найден
		if(j != this->_schemes.end()){
			// Выполняем поиск брокера подключений
			auto k = j->second->_brokers.find(bid);
			// Если брокер подключения найден, удаляем его
			if(k != j->second->_brokers.end())
				// Выполняем вывод брокера подключения
				return k->second.get();
		}
	}
	// Выводим пустой результат
	return nullptr;
}
/**
 * scheme Метод добавления схемы сети
 * @param scheme схема рабочей сети
 * @return       идентификатор схемы сети
 */
uint16_t awh::Node::scheme(const scheme_t * scheme) noexcept {
	// Результат работы функции
	uint16_t result = 0;
	// Если схема сети передана и URL адрес существует
	if(scheme != nullptr){
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Получаем объект схемы сети
		scheme_t * shm = const_cast <scheme_t *> (scheme);
		// Устанавливаем идентификатор схемы сети
		shm->id = result = (this->_schemes.size() + 1);
		// Добавляем схему сети в список
		this->_schemes.emplace(result, shm);
	}
	// Выводим результат
	return result;
}
/**
 * ssl Метод установки SSL-параметров
 * @param ssl параметры SSL для установки
 */
void awh::Node::ssl(const ssl_t & ssl) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем установку флага проверки домена
	this->_engine.verify(ssl.verify);
	// Выполняем установку алгоритмов шифрования
	this->_engine.ciphers(ssl.ciphers);
	// Устанавливаем адрес CA-файла
	this->_engine.ca(ssl.ca, ssl.capath);
	// Устанавливаем файлы сертификата
	this->_engine.certificate(ssl.cert, ssl.key);
}
/**
 * resolver Метод установки объекта DNS-резолвера
 * @param dns объект DNS-резолвер
 */
void awh::Node::resolver(const dns_t * dns) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Устанавливаем DNS-резолвер
	this->_dns = dns;
}
/**
 * sockname Метод установки адреса файла unix-сокета
 * @param name адрес файла unix-сокета
 * @return     результат установки unix-сокета
 */
bool awh::Node::sockname(const string & name) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Адрес файла сокета
		string filename = "";
		// Если адрес unix-сокета передан
		if(!name.empty())
			// Получаем адрес файла
			filename = name;
		// Если адрес unix-сокета не передан
		else filename = AWH_SHORT_NAME;
		// Устанавливаем адрес файла unix-сокета
		this->_settings.sockname = this->_fmk->format("/tmp/%s.sock", this->_fmk->transform(filename, fmk_t::transform_t::LOWER).c_str());
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Выводим в лог сообщение
		this->_log->print("Microsoft Windows does not support Unix sockets", log_t::flag_t::CRITICAL);
		// Выходим принудительно из приложения
		::exit(EXIT_FAILURE);
	#endif
	// Выводим результат
	return !this->_settings.sockname.empty();
}
/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @return поддерживаемый протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 */
awh::engine_t::proto_t awh::Node::proto() const noexcept {
	// Выполняем вывод поддерживаемого протокола подключения
	return this->_settings.proto;
}
/**
 * proto Метод извлечения активного протокола подключения
 * @param bid идентификатор брокера
 * @return    активный протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 */
awh::engine_t::proto_t awh::Node::proto(const uint64_t bid) const noexcept {
	// Если данные переданы верные
	if(bid > 0){
		// Выполняем поиск брокера подключения
		auto i = this->_brokers.find(bid);
		// Если брокер подключения найден
		if(i != this->_brokers.end()){
			// Выполняем поиск идентификатора схемы сети
			auto j = this->_schemes.find(i->second);
			// Если идентификатор схемы сети найден
			if(j != this->_schemes.end()){
				// Выполняем поиск брокера подключений
				auto k = j->second->_brokers.find(bid);
				// Если брокер подключения найден, удаляем его
				if(k != j->second->_brokers.end())
					// Выполняем извлечение активного протокола подключения
					return this->_engine.proto(const_cast <awh::scheme_t::broker_t *> (k->second.get())->_ectx);
			}
		}
	}
	// Выводим результат
	return engine_t::proto_t::NONE;
}
/**
 * proto Метод установки поддерживаемого протокола подключения
 * @param proto устанавливаемый протокол (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 */
void awh::Node::proto(const engine_t::proto_t proto) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем установку поддерживаемого протокола подключения
	this->_settings.proto = proto;
}
/**
 * sonet Метод извлечения типа сокета подключения
 * @return тип сокета подключения (TCP / UDP / SCTP)
 */
awh::scheme_t::sonet_t awh::Node::sonet() const noexcept {
	// Выполняем вывод тип сокета подключения
	return this->_settings.sonet;
}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
 */
void awh::Node::sonet(const scheme_t::sonet_t sonet) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Устанавливаем тип сокета
	this->_settings.sonet = sonet;
	/**
	 * Если операционной системой не является Linux или FreeBSD
	 */
	#if !defined(__linux__) && !defined(__FreeBSD__)
		// Если установлен протокол SCTP
		if(this->_settings.sonet == scheme_t::sonet_t::SCTP){
			// Выводим в лог сообщение
			this->_log->print("SCTP protocol is allowed to be used only in the Linux or FreeBSD operating system", log_t::flag_t::CRITICAL);
			// Выходим принудительно из приложения
			::exit(EXIT_FAILURE);
		}
	#endif
}
/**
 * family Метод извлечения типа протокола интернета
 * @return тип протокола интернета (IPV4 / IPV6 / NIX)
 */
awh::scheme_t::family_t awh::Node::family() const noexcept {
	// Выполняем вывод тип протокола интернета
	return this->_settings.family;
}
/**
 * family Метод установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::Node::family(const scheme_t::family_t family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Устанавливаем тип активного интернет-подключения
	this->_settings.family = family;
	// Если тип сокета подключения - unix-сокет
	if(this->_settings.family == scheme_t::family_t::NIX){
		// Если адрес файла unix-сокета ещё не инициализированно
		if(this->_settings.sockname.empty())
			// Выполняем активацию адреса файла unix-сокета
			this->sockname();
	// Если тип сокета подключения соответствует хосту и порту
	} else if((this->_settings.family != scheme_t::family_t::NIX) && (this->_type == engine_t::type_t::SERVER)) {
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_fs.isSock(this->_settings.sockname))
				// Удаляем файл сокета
				::unlink(this->_settings.sockname.c_str());
			// Выполняем очистку unix-сокета
			this->_settings.sockname.clear();
		#endif
	}
}
/**
 * bandwidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::Node::bandwidth(const uint64_t bid, const string & read, const string & write) noexcept {
	// Если идентификатор брокера подключений существует
	if((bid > 0) && this->has(bid)){
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Создаём бъект активного брокера подключения
			awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
			// Устанавливаем размер буфера
			broker->_ectx.buffer(
				(!read.empty() ? this->_fmk->sizeBuffer(read) : 0),
				(!write.empty() ? this->_fmk->sizeBuffer(write) : 0), 1
			);
		/**
		 * Если операционной системой является MS Windows
		 */
		#else
			// Блокируем вывод переменных
			(void) read;
			(void) write;
		#endif
	}
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::Node::network(const vector <string> & ips, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Устанавливаем тип сокета
	this->_settings.sonet = sonet;
	// Устанавливаем тип активного интернет-подключения
	this->_settings.family = family;
	// Если тип сокета подключения - unix-сокет
	if(this->_settings.family == scheme_t::family_t::NIX){
		// Если адрес файла unix-сокета ещё не инициализированно
		if(this->_settings.sockname.empty())
			// Выполняем активацию адреса файла сокета
			this->sockname();
	// Если тип сокета подключения соответствует хосту и порту
	} else if((this->_settings.family != scheme_t::family_t::NIX) && (this->_type == engine_t::type_t::SERVER)) {
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_fs.isSock(this->_settings.sockname))
				// Удаляем файл сокета
				::unlink(this->_settings.sockname.c_str());
			// Выполняем очистку unix-сокета
			this->_settings.sockname.clear();
		#endif
	}
	// Если IP-адреса переданы
	if(!ips.empty()){
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr)
			// Выполняем установку параметров сети для DNS-резолвера
			const_cast <dns_t *> (this->_dns)->network(ips);
		// Переходим по всему списку полученных адресов
		for(auto & host : ips){
			// Определяем к какому адресу относится полученный хост
			switch(static_cast <uint8_t> (this->_net.host(host))){
				// Если IP-адрес является IPv4 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Если IP-адрес является IPv6 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV6):
					// Устанавливаем полученные IP-адреса
					this->_settings.network.push_back(host);
				break;
				// Для всех остальных адресов
				default: {
					// Если объект DNS-резолвера установлен
					if(this->_dns != nullptr){
						// Определяем тип интернет-протокола
						switch(static_cast <uint8_t> (family)){
							// Если тип протокола интернета IPv4
							case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
								// Выполняем получение IP-адреса для IPv4
								const string & ip = const_cast <dns_t *> (this->_dns)->host(AF_INET, host);
								// Если IP-адрес успешно получен
								if(!ip.empty())
									// Выполняем добавление полученного хоста в список
									this->_settings.network.push_back(ip);
							} break;
							// Если тип протокола интернета IPv6
							case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
								// Выполняем получение IP-адреса для IPv6
								const string & ip = const_cast <dns_t *> (this->_dns)->host(AF_INET6, host);
								// Если результат получен, выполняем пинг
								if(!ip.empty())
									// Выполняем добавление полученного хоста в список
									this->_settings.network.push_back(ip);
							} break;
						}
					}
				}
			}
		}
	}
}
/**
 * operator Оператор извлечения поддерживаемого протокола подключения
 * @return поддерживаемый протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 */
awh::Node::operator awh::engine_t::proto_t() const noexcept {
	// Выполняем вывод поддерживаемого протокола подключения
	return this->_settings.proto;
}
/**
 * operator Оператор извлечения типа сокета подключения
 * @return тип сокета подключения (TCP / UDP / SCTP)
 */
awh::Node::operator awh::scheme_t::sonet_t() const noexcept {
	// Выполняем вывод тип сокета подключения
	return this->_settings.sonet;
}
/**
 * operator Оператор извлечения типа протокола интернета
 * @return тип протокола интернета (IPV4 / IPV6 / NIX)
 */
awh::Node::operator awh::scheme_t::family_t() const noexcept {
	// Выполняем вывод тип протокола интернета
	return this->_settings.family;
}
/**
 * Оператор [=] установки SSL-параметров
 * @param ssl параметры SSL для установки
 * @return    текущий объект
 */
awh::Node & awh::Node::operator = (const ssl_t & ssl) noexcept {
	// Выполняем установку SSL-параметров
	this->ssl(ssl);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] установки объекта DNS-резолвера
 * @param dns объект DNS-резолвер
 * @return    текущий объект
 */
awh::Node & awh::Node::operator = (const dns_t & dns) noexcept {
	// Выполняем установку объекта DNS-резолвера
	this->resolver(&dns);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] установки поддерживаемого протокола подключения
 * @param proto устанавливаемый протокол (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
 * @return      текущий объект
 */
awh::Node & awh::Node::operator = (const engine_t::proto_t proto) noexcept {
	// Выполняем установку поддерживаемого протокола подключения
	this->proto(proto);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] установки типа сокета подключения
 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
 * @return      текущий объект
 */
awh::Node & awh::Node::operator = (const scheme_t::sonet_t sonet) noexcept {
	// Выполняем установку типа сокета подключения
	this->sonet(sonet);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @return       текущий объект
 */
awh::Node & awh::Node::operator = (const scheme_t::family_t family) noexcept {
	// Выполняем установку типа протокола интернета
	this->family(family);
	// Выводим текущий объект
	return (* this);
}
/**
 * ~Node Деструктор
 */
awh::Node::~Node() noexcept {
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Выполняем удаление списка активных схем сети
	this->_schemes.clear();
	// Если требуется использовать unix-сокет и ядро является сервером
	if((this->_settings.family == scheme_t::family_t::NIX) && (this->_type == engine_t::type_t::SERVER)){
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_fs.isSock(this->_settings.sockname))
				// Удаляем файл сокета
				::unlink(this->_settings.sockname.c_str());
		#endif
	}
	// Выполняем разблокировку потока
	this->_mtx.unlock();
}
