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
				auto k = j->second->_brokers.find(i->first);
				// Если брокер подключения найден, удаляем его
				if(k != j->second->_brokers.end())
					// Выполняем удаление брокера подключения
					const_cast <scheme_t *> (j->second)->_brokers.erase(k);
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
 * available Метод освобождение памяти занятой для хранение полезной нагрузки брокера
 * @param bid идентификатор брокера
 */
void awh::Node::available(const uint64_t bid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.send);
	// Ещем для указанного потока очередь полезной нагрузки
	auto i = this->_payloads.find(bid);
	// Если для потока очередь полезной нагрузки получена
	if((i != this->_payloads.end()) && !i->second.empty()){
		// Выполняем блокировку потока
		this->_mtx.main.lock();
		// Выполняем поиск размера текущей полезной нагрузки для брокера
		auto j = this->_available.find(bid);
		// Если размер полезной нагрузки найден
		if(j != this->_available.end())
			// Выполняем уменьшение количества записанных байт
			j->second -= i->second.front().offset;
		// Увеличиваем общее количество переданных данных
		this->_memoryAvailableSize += i->second.front().offset;
		// Выполняем разблокировку потока
		this->_mtx.main.unlock();
		// Если идентификатор брокера подключений существует
		if((bid > 0) && this->has(bid)){
			// Выполняем удаление буфера буфера полезной нагрузки
			i->second.pop();
			// Если очередь полностью пустая
			if(i->second.empty())
				// Выполняем удаление всей очереди
				this->_payloads.erase(i);
		// Выполняем удаление всей очереди
		} else this->_payloads.erase(i);
	}
	// Если опередей полезной нагрузки нет, выполняем функцию обратного вызова
	if(this->_payloads.find(bid) == this->_payloads.end()){
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("available")){
			// Выполняем поиск размера текущей полезной нагрузки для брокера
			auto i = this->_available.find(bid);
			// Если размер полезной нагрузки найден
			if(i != this->_available.end())
				// Выполняем функцию обратного вызова сообщая об освобождении памяти
				this->_callbacks.call <void (const uint64_t, const size_t)> ("available", bid, (this->_brokerAvailableSize < i->second) ? 0 : min(this->_brokerAvailableSize - i->second, this->_memoryAvailableSize));
		}
	}
}
/**
 * createBuffer Метод инициализации буфера полезной нагрузки
 * @param bid идентификатор брокера
 */
void awh::Node::createBuffer(const uint64_t bid) noexcept {
	// Если идентификатор брокера подключений существует
	if((bid > 0) && this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS)){
			// Если буфер полезной нагрузки уже инициализирован
			if(broker->_payload.size != 0){
				// Выполняем зануление размера буфера данных
				broker->_payload.size = 0;
				// Выполняем удаление буфера данных
				broker->_payload.data.reset(nullptr);
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
					broker->_payload.size = size;
					// Выполняем создание буфера данных
					broker->_payload.data = unique_ptr <char []> (new char [size]);
				/**
				 * Если возникает ошибка
				 */
				} catch(const bad_alloc &) {
					// Выводим в лог сообщение
					this->_log->print("Memory allocation error", log_t::flag_t::CRITICAL);
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
 * emplace Метод добавления нового буфера полезной нагрузки
 * @param buffer бинарный буфер полезной нагрузки
 * @param size   размер бинарного буфера полезной нагрузки
 * @param bid    идентификатор брокера
 */
void awh::Node::emplace(const char * buffer, const size_t size, const uint64_t bid) noexcept {
	// Если идентификатор брокера подключений существует
	if((bid > 0) && this->has(bid) && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Смещение в переданном буфере данных
			size_t offset = 0, actual = 0;
			/**
			 * Выполняем создание нужного количества буферов
			 */
			do {
				// Объект полезной нагрузки для отправки
				payload_t payload;
				// Устанавливаем размер буфера данных
				payload.size = this->_payloadSize;
				// Выполняем создание буфера данных
				payload.data = unique_ptr <char []> (new char [this->_payloadSize]);
				// Получаем размер данных который возможно скопировать
				actual = ((this->_payloadSize >= (size - offset)) ? (size - offset) : this->_payloadSize);
				// Выполняем копирование буфера полезной нагрузки
				::memcpy(payload.data.get(), buffer + offset, size - offset);
				// Выполняем смещение в буфере
				offset += actual;
				// Увеличиваем смещение в буфере полезной нагрузки
				payload.offset += actual;
				// Ещем для указанного потока очередь полезной нагрузки
				auto i = this->_payloads.find(bid);
				// Если для потока очередь полезной нагрузки получена
				if(i != this->_payloads.end())
					// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
					i->second.push(std::move(payload));
				// Если для потока почередь полезной нагрузки ещё не сформированна
				else {
					// Создаём новую очередь полезной нагрузки
					auto ret = this->_payloads.emplace(bid, queue <payload_t> ());
					// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
					ret.first->second.push(std::move(payload));
				}
			/**
			 * Если не все данные полезной нагрузки установлены, создаём новый буфер
			 */
			} while(offset < size);
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			// Выводим в лог сообщение
			this->_log->print("Memory allocation error", log_t::flag_t::CRITICAL);
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
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
 * sockname Метод установки адреса файла unix-сокета
 * @param name адрес файла unix-сокета
 * @return     результат установки unix-сокета
 */
bool awh::Node::sockname(const string & name) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
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
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.main);
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
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS))
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
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS))
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
			if(i != this->_payloads.end()){
				// Если ещё есть место в буфере данных
				if(i->second.back().offset < i->second.back().size){
					// Смещение в переданном буфере данных
					size_t offset = 0;
					// Получаем размер данных который возможно скопировать
					const size_t actual = ((i->second.back().size - i->second.back().offset) >= size ? size : (i->second.back().size - i->second.back().offset));
					// Выполняем копирование переданного буфера данных в временный буфер данных
					::memcpy(i->second.back().data.get() + i->second.back().offset, buffer, actual);
					// Выполняем смещение в переданном буфере данных
					offset += actual;
					// Выполняем смещение в основном буфере данных
					i->second.back().offset += actual;
					// Если не все данные были скопированы
					if(offset < size)
						// Выполняем создание нового фрейма
						this->emplace(buffer + offset, size - offset, bid);
				// Если места в буфере данных больше нет
				} else this->emplace(buffer, size, bid);
			// Если для потока почередь полезной нагрузки ещё не сформированна
			} else this->emplace(buffer, size, bid);
			// Выполняем блокировку потока
			this->_mtx.main.lock();
			// Выполняем поиск размера используемой памяти для хранения полезной нагрузки брокера
			auto j = this->_available.find(bid);
			// Если размер полезной нагрузки найден
			if(j != this->_available.end())
				// Выполняем увеличение количество записанных байт
				j->second += size;
			// Если память ещё не потрачена для отправки, добавляем размер передаваемых данных
			else this->_available.emplace(bid, size);
			// Уменьшаем общее количество переданных данных
			this->_memoryAvailableSize -= size;
			// Выполняем разблокировку потока
			this->_mtx.main.unlock();
		// Если функция обратного вызова установлена
		} else if(this->_callbacks.is("unavailable"))
			// Выводим функцию обратного вызова сигнализирующая о том, что передаваемые данные небыли отправленны
			this->_callbacks.call <void (const uint64_t, const char *, const size_t)> ("unavailable", bid, buffer, size);
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
			static_cast <int> (!read.empty() ? this->_fmk->sizeBuffer(read) : AWH_BUFFER_SIZE_RCV),
			static_cast <int> (!write.empty() ? this->_fmk->sizeBuffer(write) : AWH_BUFFER_SIZE_SND)
		);
		// Выполняем создание буфера полезной нагрузки
		this->createBuffer(bid);
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
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.main);
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
	this->_mtx.main.unlock();
}
