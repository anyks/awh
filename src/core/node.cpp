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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <core/node.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Оператор [=] перемещения SSL-параметров
 * @param ssl объект SSL-параметров
 * @return    объект текущий параметров
 */
awh::Node::SSL & awh::Node::SSL::operator = (ssl_t && ssl) noexcept {
	// Выполняем копирование флага валидации доменного имени
	this->verify = ssl.verify;
	// Выполняем перемещение ключа SSL-сертификата
	this->key = ::move(ssl.key);
	// Выполняем перемещение SSL-сертификата
	this->cert = ::move(ssl.cert);
	// Выполняем перемещение сертификата центра сертификации (CA-файла)
	this->ca = ::move(ssl.ca);
	// Выполняем перемещение сертификата отозванных сертификатов (CRL-файла)
	this->crl = ::move(ssl.crl);
	// Выполняем перемещение каталога с сертификатами центра сертификации (CA-файлами)
	this->capath = ::move(ssl.capath);
	// Выполняем перемещение списка алгоритмов шифрования
	this->ciphers = ::move(ssl.ciphers);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] присванивания SSL-параметров
 * @param ssl объект SSL-параметров
 * @return    объект текущий параметров
 */
awh::Node::SSL & awh::Node::SSL::operator = (const ssl_t & ssl) noexcept {
	// Выполняем копирование флага валидации доменного имени
	this->verify = ssl.verify;
	// Выполняем копирование ключа SSL-сертификата
	this->key = ssl.key;
	// Выполняем копирование SSL-сертификата
	this->cert = ssl.cert;
	// Выполняем копирование сертификата центра сертификации (CA-файла)
	this->ca = ssl.ca;
	// Выполняем копирование сертификата отозванных сертификатов (CRL-файла)
	this->crl = ssl.crl;
	// Выполняем копирование каталога с сертификатами центра сертификации (CA-файлами)
	this->capath = ssl.capath;
	// Выполняем копирование списка алгоритмов шифрования
	this->ciphers = ssl.ciphers;
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор сравнения
 * @param ssl объект SSL-параметров
 * @return    результат сравнения
 */
bool awh::Node::SSL::operator == (const ssl_t & ssl) noexcept {
	// Выполняем сравнения двух объектов SSL-параметров
	bool result = (
		(this->verify == ssl.verify) &&
		(this->key.compare(ssl.key) == 0) &&
		(this->cert.compare(ssl.cert) == 0) &&
		(this->ca.compare(ssl.ca) == 0) &&
		(this->crl.compare(ssl.crl) == 0) &&
		(this->capath.compare(ssl.capath) == 0)
	);
	// Если все параметры совпадают
	if(result && (result = (this->ciphers.size() == ssl.ciphers.size()))){
		// Если список шифрования не пустой
		if(!this->ciphers.empty()){
			// Выполняем перебор всего списка шифрования
			for(size_t i = 0; i < this->ciphers.size(); i++){
				// Выполняем сравнение каждой записи
				result = (this->ciphers.at(i).compare(ssl.ciphers.at(i)) == 0);
				// Если шифрвы не совпадают
				if(!result)
					// Выходим из функции
					return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * SSL Конструктор перемещения
 * @param ssl объект SSL-параметров
 */
awh::Node::SSL::SSL(ssl_t && ssl) noexcept {
	// Выполняем копирование флага валидации доменного имени
	this->verify = ssl.verify;
	// Выполняем перемещение ключа SSL-сертификата
	this->key = ::move(ssl.key);
	// Выполняем перемещение SSL-сертификата
	this->cert = ::move(ssl.cert);
	// Выполняем перемещение сертификата центра сертификации (CA-файла)
	this->ca = ::move(ssl.ca);
	// Выполняем перемещение сертификата отозванных сертификатов (CRL-файла)
	this->crl = ::move(ssl.crl);
	// Выполняем перемещение каталога с сертификатами центра сертификации (CA-файлами)
	this->capath = ::move(ssl.capath);
	// Выполняем перемещение списка алгоритмов шифрования
	this->ciphers = ::move(ssl.ciphers);
}
/**
 * SSL Конструктор копирования
 * @param ssl объект SSL-параметров
 */
awh::Node::SSL::SSL(const ssl_t & ssl) noexcept {
	// Выполняем копирование флага валидации доменного имени
	this->verify = ssl.verify;
	// Выполняем копирование ключа SSL-сертификата
	this->key = ssl.key;
	// Выполняем копирование SSL-сертификата
	this->cert = ssl.cert;
	// Выполняем копирование сертификата центра сертификации (CA-файла)
	this->ca = ssl.ca;
	// Выполняем копирование сертификата отозванных сертификатов (CRL-файла)
	this->crl = ssl.crl;
	// Выполняем копирование каталога с сертификатами центра сертификации (CA-файлами)
	this->capath = ssl.capath;
	// Выполняем копирование списка алгоритмов шифрования
	this->ciphers = ssl.ciphers;
}
/**
 * SSL Конструктор
 */
awh::Node::SSL::SSL() noexcept : verify(true), key{""}, cert{""}, ca{""}, crl{""}, capath{""} {}
/**
 * remove Метод удаления всех схем сети
 */
void awh::Node::remove() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock1(this->_mtx.main);
	const lock_guard <recursive_mutex> lock2(this->_mtx.send);
	// Выполняем удаление всей схемы сети
	this->_schemes.clear();
	// Выполняем удаление списка брокеров подключения
	this->_brokers.clear();
	// Выполняем удаление очередей полезной нагрузки
	this->_payloads.clear();
	// Выполняем удаление списка используемой памяти буферов полезной нагрузки
	this->_available.clear();
}
/**
 * remove Метод удаления схемы сети
 * @param sid идентификатор схемы сети
 */
void awh::Node::remove(const uint16_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock1(this->_mtx.main);
		const lock_guard <recursive_mutex> lock2(this->_mtx.send);
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден
		if(i != this->_schemes.end()){
			// Выполняем перебор всех брокеров схемы сети
			for(auto j = i->second->_brokers.begin(); j != i->second->_brokers.end();){
				// Выполняем удаление брокеров из локального списка
				this->_brokers.erase(j->first);
				// Выполняем удаление очереди полезной нагрузки
				this->_payloads.erase(j->first);
				// Выполняем удаление списка используемой памяти буфера полезной нагрузки
				this->_available.erase(j->first);
				// Выполняем удаление брокера подключения
				j = const_cast <scheme_t *> (i->second)->_brokers.erase(j);
			}
			// Выполняем удаление схему сети
			this->_schemes.erase(i);
		}
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
		const lock_guard <recursive_mutex> lock1(this->_mtx.main);
		const lock_guard <recursive_mutex> lock2(this->_mtx.send);
		// Выполняем поиск брокера подключения
		auto i = this->_brokers.find(bid);
		// Если брокер подключения найден
		if(i != this->_brokers.end()){
			// Выполняем поиск идентификатора схемы сети
			auto j = this->_schemes.find(i->second->sid());
			// Если идентификатор схемы сети найден
			if(j != this->_schemes.end()){
				// Выполняем поиск брокера подключений
				auto k = const_cast <scheme_t *> (j->second)->_brokers.find(i->first);
				// Если брокер подключения найден, удаляем его
				if(k != const_cast <scheme_t *> (j->second)->_brokers.end()){
					// Выполняем очистку брокера
					k->second.reset(nullptr);
					// Выполняем удаление брокера подключения
					const_cast <scheme_t *> (j->second)->_brokers.erase(k);
				}
			}
			// Выполняем удаление очереди полезной нагрузки
			this->_payloads.erase(i->first);
			// Выполняем удаление списка используемой памяти буфера полезной нагрузки
			this->_available.erase(i->first);
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
		auto i = this->_brokers.find(bid);
		// Если брокер подключения найден
		if(i != this->_brokers.end())
			// Выводим результат
			return i->second->sid();
	}
	// Выводим пустой результат
	return 0;
}
/**
 * initBuffer Метод инициализации буфера полезной нагрузки
 * @param bid идентификатор брокера
 */
void awh::Node::initBuffer(const uint64_t bid) noexcept {
	// Если идентификатор брокера подключений существует
	if((bid > 0) && this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < AWH_MAX_SOCKETS)){
			// Если буфер полезной нагрузки уже инициализирован
			if(broker->_buffer.size > 0){
				// Выполняем зануление размера буфера данных
				broker->_buffer.size = 0;
				// Выполняем удаление буфера данных
				broker->_buffer.data.reset(nullptr);
			}
			// Получаем максимальный размер буфера
			const int64_t size = broker->_ectx.buffer(engine_t::method_t::READ);
			// Если размер буфера получен
			if(size > 0){
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Устанавливаем размер буфера данных
					broker->_buffer.size = size;
					// Выполняем создание буфера данных
					broker->_buffer.data = unique_ptr <char []> (new char [size]);
				/**
				 * Если возникает ошибка
				 */
				} catch(const bad_alloc &) {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(bid), log_t::flag_t::CRITICAL, "Memory allocation error");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
					#endif
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
				// Получаем максимальный размер буфера
				this->_payloadSize = broker->_ectx.buffer(engine_t::method_t::WRITE);
			}
		}
	}
}
/**
 * erase Метод освобождение памяти занятой для хранение полезной нагрузки брокера
 * @param bid  идентификатор брокера
 * @param size размер байт удаляемых из буфера
 */
void awh::Node::erase(const uint64_t bid, const size_t size) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.send);
	// Значение оставшейся памяти в буфере
	size_t amount = 0;
	// Ещем для указанного потока очередь полезной нагрузки
	auto i = this->_payloads.find(bid);
	// Если для потока очередь полезной нагрузки получена
	if((i != this->_payloads.end()) && !i->second->empty()){
		// Выполняем блокировку потока
		this->_mtx.main.lock();
		// Увеличиваем общее количество переданных данных
		this->_memoryAvailableSize += size;
		// Выполняем поиск размера текущей полезной нагрузки для брокера
		auto j = this->_available.find(bid);
		// Если размер полезной нагрузки найден
		if(j != this->_available.end())
			// Выполняем уменьшение количества записанных байт
			amount = (j->second -= size);
		// Выполняем разблокировку потока
		this->_mtx.main.unlock();
		// Если идентификатор брокера подключений существует
		if((bid > 0) && this->has(bid)){
			// Выполняем удаление буфера буфера полезной нагрузки
			i->second->erase(size);
			// Если очередь полностью пустая
			if(i->second->empty())
				// Выполняем удаление всей очереди
				this->_payloads.erase(i);
		// Выполняем удаление всей очереди
		} else this->_payloads.erase(i);
	}
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("available"))
		// Выполняем функцию обратного вызова сообщая об освобождении памяти
		this->_callbacks.call <void (const uint64_t, const size_t)> ("available", bid, (this->_brokerAvailableSize < amount) ? 0 : min(this->_brokerAvailableSize - amount, this->_memoryAvailableSize));
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
	if(i != this->_brokers.end())
		// Выполняем вывод брокера подключения
		return i->second;
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
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
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
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку флага проверки домена
	this->_engine.verify(ssl.verify);
	// Выполняем установку алгоритмов шифрования
	this->_engine.ciphers(ssl.ciphers);
	// Устанавливаем адрес CRL-файла
	this->_engine.crl(ssl.crl);
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
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем DNS-резолвер
	this->_dns = dns;
}
/**
 * sockname Метод установки названия unix-сокета
 * @param name название unix-сокета
 * @return     результат установки названия unix-сокета
 */
bool awh::Node::sockname(const string & name) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если название unix-сокета передано
		if(!name.empty())
			// Устанавливаем название unix-сокета
			this->_settings.sockname = name;
		// Если название unix-сокета не передан
		else {
			// Устанавливаем название unix-сокета по умолчанию
			this->_settings.sockname = AWH_SHORT_NAME;
			// Переводим название unix-сокета в нижний регистр
			this->_fmk->transform(this->_settings.sockname, fmk_t::transform_t::LOWER);
		}
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
 * sockpath Метод установки адреса каталога где хранится unix-сокет
 * @param path адрес каталога в файловой системе где хранится unix-сокет
 * @return     результат установки адреса каталога где хранится unix-сокет
 */
bool awh::Node::sockpath(const string & path) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если адрес каталога в файловой системе где хранится unix-сокет передан
		if(!path.empty())
			// Устанавливаем адрес каталога в файловой системе где хранится unix-сокет
			this->_settings.sockpath = path;
		// Если адрес каталога в файловой системе где хранится unix-сокет не передан
		else {
			// Устанавливаем адрес каталога в файловой системе где хранится unix-сокет по умолчанию
			this->_settings.sockpath = "/tmp";
		}
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
	return !this->_settings.sockpath.empty();
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
		if(i != this->_brokers.end())
			// Выполняем извлечение активного протокола подключения
			return this->_engine.proto(const_cast <awh::scheme_t::broker_t *> (i->second)->_ectx);
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
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
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
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем тип сокета
	this->_settings.sonet = sonet;
	/**
	 * Если операционной системой не является Linux или FreeBSD
	 */
	#if !defined(__linux__) && !defined(__FreeBSD__)
		// Если установлен протокол SCTP
		if(this->_settings.sonet == scheme_t::sonet_t::SCTP){
			// Выводим в лог сообщение
			this->_log->print("\"SCTP-protocol\" is allowed to be used only in Linux or FreeBSD operating system", log_t::flag_t::CRITICAL);
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
	this->_mtx.main.lock();
	// Устанавливаем тип активного интернет-подключения
	this->_settings.family = family;
	// Выполняем разблокировку потока
	this->_mtx.main.unlock();
	// Если тип сокета подключения - unix-сокет
	if(this->_settings.family == scheme_t::family_t::NIX){
		// Если название unix-сокета ещё не инициализированно
		if(this->_settings.sockname.empty())
			// Выполняем установку названия unix-сокета
			this->sockname();
	// Если тип сокета подключения соответствует хосту и порту
	} else if(!this->_settings.sockname.empty() &&
			 (this->_type == engine_t::type_t::SERVER) &&
			 (this->_settings.family != scheme_t::family_t::NIX)) {
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.main);
			// Получаем адрес файла unix-сокет
			const string & filename = this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_fs.isSock(filename))
				// Удаляем файл сокета
				::unlink(filename.c_str());
			// Выполняем очистку unix-сокета
			this->_settings.sockname.clear();
		#endif
	}
}
/**
 * sending Метод получения режима отправки сообщений
 * @return установленный режим отправки сообщений
 */
awh::Node::sending_t awh::Node::sending() const noexcept {
	// Выполняем получение режима отправки сообщений
	return this->_sending;
}
/**
 * sending Метод установки режима отправки сообщений
 * @param sending режим отправки сообщений для установки
 */
void awh::Node::sending(const sending_t sending) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку режима отправки сообщений
	this->_sending = sending;
}
/**
 * memoryAvailableSize Метод получения максимального рамзера памяти для хранения полезной нагрузки всех брокеров
 * @return размер памяти для хранения полезной нагрузки всех брокеров
 */
size_t awh::Node::memoryAvailableSize() const noexcept {
	// Выводим размер памяти для хранения полезной нагрузки всех брокеров
	return this->_memoryAvailableSize;
}
/**
 * memoryAvailableSize Метод установки максимального рамзера памяти для хранения полезной нагрузки всех брокеров
 * @param size размер памяти для хранения полезной нагрузки всех брокеров
 */
void awh::Node::memoryAvailableSize(const size_t size) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку размера памяти для хранения полезной нагрузки всех брокеров
	this->_memoryAvailableSize = size;
}
/**
 * brokerAvailableSize Метод получения максимального размера хранимой полезной нагрузки для одного брокера
 * @return размер хранимой полезной нагрузки для одного брокера
 */
size_t awh::Node::brokerAvailableSize() const noexcept {
	// Выводим размер хранимой полезной нагрузки для одного брокера
	return this->_brokerAvailableSize;
}
/**
 * brokerAvailableSize Метод получения размера хранимой полезной нагрузки для текущего брокера
 * @param bid идентификатор брокера
 * @return    размер хранимой полезной нагрузки для текущего брокера
 */
size_t awh::Node::brokerAvailableSize(const uint64_t bid) const noexcept {
	// Выполняем поиск размера текущей полезной нагрузки для брокера
	auto i = this->_available.find(bid);
	// Если размер полезной нагрузки найден
	if(i != this->_available.end())
		// Выводим размер осташейся памяти
		return i->second;
	// Сообщаем, что ничего не найдено
	return 0;
}
/**
 * brokerAvailableSize Метод установки максимального размера хранимой полезной нагрузки для одного брокера
 * @param size размер хранимой полезной нагрузки для одного брокера
 */
void awh::Node::brokerAvailableSize(const size_t size) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку размера хранимой полезной нагрузки для одного брокера
	this->_brokerAvailableSize = size;
}
/**
 * cork Метод отключения/включения алгоритма TCP/CORK
 * @param bid  идентификатор брокера
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::Node::cork(const uint64_t bid, const engine_t::mode_t mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock1(this->_mtx.main);
	const lock_guard <recursive_mutex> lock2(this->_mtx.send);
	// Если идентификатор брокера подключений существует
	if((bid > 0) && this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < AWH_MAX_SOCKETS))
			// Выполняем активирование/деактивирование алгоритма TCP/CORK
			return broker->_ectx.cork(mode);
	}
	// Сообщаем, что ничего не установлено
	return false;
}
/**
 * nodelay Метод отключения/включения алгоритма Нейгла
 * @param bid  идентификатор брокера
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::Node::nodelay(const uint64_t bid, const engine_t::mode_t mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock1(this->_mtx.main);
	const lock_guard <recursive_mutex> lock2(this->_mtx.send);
	// Если идентификатор брокера подключений существует
	if((bid > 0) && this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < AWH_MAX_SOCKETS))
			// Выполняем активирование/деактивирование алгоритма Нейгла
			return broker->_ectx.nodelay(mode);
	}
	// Сообщаем, что ничего не установлено
	return false;
}
/**
 * send Метод асинхронной отправки буфера данных в сокет
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param bid    идентификатор брокера
 * @return       результат отправки сообщения
 */
bool awh::Node::send(const char * buffer, const size_t size, const uint64_t bid) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.send);
		// Если идентификатор брокера подключений существует
		if((bid > 0) && this->has(bid) && (buffer != nullptr) && (size > 0)){
			// Флаг недоступности свободной памяти в буфере обмена данными
			bool unavailable = false;
			// Выполняем поиск размера текущей полезной нагрузки для брокера
			auto i = this->_available.find(bid);
			// Если размер полезной нагрузки найден
			if(i != this->_available.end())
				// Если места не достаточно
				unavailable = ((this->_brokerAvailableSize < i->second) || (min(this->_brokerAvailableSize - i->second, this->_memoryAvailableSize) < size));
			// Если память ещё не потрачена для отправки
			else unavailable = (min(this->_brokerAvailableSize, this->_memoryAvailableSize) < size);
			// Если свободной памяти в буфере обмена данными достаточно
			if((result = !unavailable)){
				// Ещем для указанного потока очередь полезной нагрузки
				auto i = this->_payloads.find(bid);
				// Если для потока очередь полезной нагрузки получена
				if(i != this->_payloads.end())
					// Выполняем добавление полезной нагрузки
					i->second->push(buffer, size);
				// Если для потока почередь полезной нагрузки ещё не сформированна
				else {
					// Создаём новую очередь полезной нагрузки
					auto ret = this->_payloads.emplace(bid, unique_ptr <buffer_t> (new buffer_t(this->_log)));
					// Выполняем добавление полезной нагрузки
					ret.first->second->push(buffer, size);
				}
				// Выполняем блокировку потока
				this->_mtx.main.lock();
				// Уменьшаем общее количество переданных данных
				this->_memoryAvailableSize -= size;
				// Выполняем поиск размера используемой памяти для хранения полезной нагрузки брокера
				auto j = this->_available.find(bid);
				// Если размер полезной нагрузки найден
				if(j != this->_available.end())
					// Выполняем увеличение количество записанных байт
					j->second += size;
				// Если память ещё не потрачена для отправки, добавляем размер передаваемых данных
				else this->_available.emplace(bid, size);
				// Выполняем разблокировку потока
				this->_mtx.main.unlock();
			// Если функция обратного вызова установлена
			} else if(this->_callbacks.is("unavailable"))
				// Выводим функцию обратного вызова сигнализирующая о том, что передаваемые данные небыли отправленны
				this->_callbacks.call <void (const uint64_t, const char *, const size_t)> ("unavailable", bid, buffer, size);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const bad_alloc &) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, bid), log_t::flag_t::CRITICAL, "Memory allocation error");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, bid), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
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
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Устанавливаем размер буфера
		broker->_ectx.buffer(
			static_cast <int32_t> (!read.empty() ? this->_fmk->sizeBuffer(read) : AWH_BUFFER_SIZE_RCV),
			static_cast <int32_t> (!write.empty() ? this->_fmk->sizeBuffer(write) : AWH_BUFFER_SIZE_SND)
		);
		// Выполняем создание буфера полезной нагрузки
		this->initBuffer(bid);
	}
}
/**
 * events Метод активации/деактивации метода события сокета
 * @param bid    идентификатор брокера
 * @param mode   сигнал активации сокета
 * @param method метод режима работы
 */
void awh::Node::events(const uint64_t bid, const awh::scheme_t::mode_t mode, const engine_t::method_t method) noexcept {
	// Если идентификатор брокера подключений существует
	if((bid > 0) && this->has(bid)){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Выполняем активацию/деактивацию метода события сокета
		broker->events(mode, method);
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
	this->_mtx.main.lock();
	// Устанавливаем тип сокета
	this->_settings.sonet = sonet;
	// Устанавливаем тип активного интернет-подключения
	this->_settings.family = family;
	// Выполняем разблокировку потока
	this->_mtx.main.unlock();
	// Если тип сокета подключения - unix-сокет
	if(this->_settings.family == scheme_t::family_t::NIX){
		// Если название unix-сокета ещё не инициализированно
		if(this->_settings.sockname.empty())
			// Выполняем установку названия unix-сокета
			this->sockname();
	// Если тип сокета подключения соответствует хосту и порту
	} else if(!this->_settings.sockname.empty() &&
			 (this->_type == engine_t::type_t::SERVER) &&
			 (this->_settings.family != scheme_t::family_t::NIX)) {
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.main);
			// Получаем адрес файла unix-сокет
			const string & filename = this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_fs.isSock(filename))
				// Удаляем файл сокета
				::unlink(filename.c_str());
			// Выполняем очистку unix-сокета
			this->_settings.sockname.clear();
		#endif
	}
	// Если IP-адреса переданы
	if(!ips.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
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
	// Выполняем удаление всех созданных объектов
	this->remove();
	// Выполняем блокировку потока
	this->_mtx.main.lock();
	// Если требуется использовать unix-сокет и ядро является сервером
	if(!this->_settings.sockname.empty() &&
	  (this->_type == engine_t::type_t::SERVER) &&
	  (this->_settings.family == scheme_t::family_t::NIX)){
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Получаем адрес файла unix-сокет
			const string & filename = this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_fs.isSock(filename))
				// Удаляем файл сокета
				::unlink(filename.c_str());
		#endif
	}
	// Выполняем разблокировку потока
	this->_mtx.main.unlock();
}
