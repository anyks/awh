/**
 * @file: proxy.cpp
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
#include <server/proxy.hpp>

/**
 * runCallback Функция обратного вызова при активации ядра сервера
 * @param mode флаг запуска/остановки
 * @param core объект биндинга TCP/IP
 */
void awh::server::Proxy::runCallback(const bool mode, awh::core_t * core) noexcept {

}
/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::server::Proxy::chunking(const vector <char> & chunk, const awh::http_t * http) noexcept {

}
/**
 * persistServerCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Proxy::persistCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * openServerCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Proxy::openServerCallback(const size_t wid, awh::core_t * core) noexcept {

}
/**
 * connectClientCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Proxy::connectClientCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * connectServerCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Proxy::connectServerCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Proxy::disconnectClientCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * disconnectServerCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Proxy::disconnectServerCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param port порт подключившегося адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @return     результат разрешения к подключению клиента
 */
bool awh::server::Proxy::acceptServerCallback(const string & ip, const string & mac, const u_int port, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::server::Proxy::readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * readServerCallback Функция обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::server::Proxy::readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * writeServerCallback Функция обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 */
void awh::server::Proxy::writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * prepare Метод обработки входящих данных
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
void awh::server::Proxy::prepare(const size_t aid, const size_t wid, awh::core_t * core) noexcept {

}
/**
 * init Метод инициализации WebSocket адъютанта
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Proxy::init(const string & socket, const http_t::compress_t compress) noexcept {

}
/**
 * init Метод инициализации WebSocket клиента
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Proxy::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {

}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const size_t, const mode_t, Proxy *)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const size_t, const event_t, http_t *, Proxy *)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const size_t, const event_t, const char *, const size_t, Proxy *)> callback) noexcept {

}
/**
 * on Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Proxy::on(function <string (const string &)> callback) noexcept {

}
/**
 * on Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Proxy::on(function <bool (const string &, const string &)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const vector <char> &, const http_t *)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const string &, const string &, Proxy *)> callback) noexcept {

}
/**
 * reject Метод отправки сообщения об ошибке
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для клиента
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Proxy::reject(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {

}
/**
 * response Метод отправки сообщения клиенту
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для клиента
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Proxy::response(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {

}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::Proxy::port(const size_t aid) const noexcept {

}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::Proxy::ip(const size_t aid) const noexcept {

}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::Proxy::mac(const size_t aid) const noexcept {

}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::alive(const bool mode) noexcept {

}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Proxy::alive(const size_t time) noexcept {

}
/**
 * alive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::alive(const size_t aid, const bool mode) noexcept {

}
/**
 * stop Метод остановки клиента
 */
void awh::server::Proxy::stop() noexcept {
	
}
/**
 * start Метод запуска клиента
 */
void awh::server::Proxy::start() noexcept {

}
/**
 * close Метод закрытия подключения клиента
 * @param aid идентификатор адъютанта
 */
void awh::server::Proxy::close(const size_t aid) noexcept {

}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Proxy::waitTimeDetect(const time_t read, const time_t write) noexcept {

}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::Proxy::bytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {

}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Proxy::realm(const string & realm) noexcept {

}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Proxy::opaque(const string & opaque) noexcept {

}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Proxy::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {

}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::Proxy::mode(const u_short flag) noexcept {

}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Proxy::total(const u_short total) noexcept {

}
/**
 * clusterSize Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::server::Proxy::clusterSize(const size_t size) noexcept {

}
/**
 * ipV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::Proxy::ipV6only(const bool mode) noexcept {

}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Proxy::keepAlive(const int cnt, const int idle, const int intvl) noexcept {

}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
 */
void awh::server::Proxy::sonet(const worker_t::sonet_t sonet) noexcept {

}
/**
 * family Метод установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::server::Proxy::family(const worker_t::family_t family) noexcept {

}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Proxy::bandWidth(const size_t aid, const string & read, const string & write) noexcept {

}
/**
 * network Метод установки параметров сети
 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::server::Proxy::network(const vector <string> & ip, const vector <string> & ns, const worker_t::family_t family, const worker_t::sonet_t sonet) noexcept {

}
/**
 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::server::Proxy::verifySSL(const bool mode) noexcept {

}
/**
 * ciphers Метод установки алгоритмов шифрования
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::server::Proxy::ciphers(const vector <string> & ciphers) noexcept {

}
/**
 * ca Метод установки доверенного сертификата (CA-файла)
 * @param trusted адрес доверенного сертификата (CA-файла)
 * @param path    адрес каталога где находится сертификат (CA-файл)
 */
void awh::server::Proxy::ca(const string & trusted, const string & path) noexcept {

}
/**
 * certificate Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата
 */
void awh::server::Proxy::certificate(const string & chain, const string & key) noexcept {

}
/**
 * serverName Метод добавления названия сервера
 * @param name название сервера для добавления
 */
void awh::server::Proxy::serverName(const string & name) noexcept {

}
/**
 * chunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Proxy::chunkSize(const size_t size) noexcept {

}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Proxy::maxRequests(const size_t max) noexcept {

}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::Proxy::compress(const http_t::compress_t compress) noexcept {

}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Proxy::serv(const string & id, const string & name, const string & ver) noexcept {

}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Proxy::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {

}
/**
 * Proxy Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Proxy::Proxy(const fmk_t * fmk, const log_t * log) noexcept :
 _port(SERVER_PORT), _host(""), _usock(""), _core(fmk, log), _worker(fmk, log),
 _sid(AWH_SHORT_NAME), _ver(AWH_VERSION), _name(AWH_NAME), _realm(""), _opaque(""),
 _pass(""), _salt(""), _cipher(hash_t::cipher_t::AES128), _authHash(auth_t::hash_t::MD5),
 _authType(auth_t::type_t::NONE), _crypt(false), _alive(false), _noConnect(false),
 _chunkSize(BUFFER_CHUNK), _timeAlive(KEEPALIVE_TIMEOUT), _maxRequests(SERVER_MAX_REQUESTS), _fmk(fmk), _log(log) {

}










/**
 * runCallback Функция обратного вызова при активации ядра сервера
 * @param mode флаг запуска/остановки
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
/*
void awh::server::Proxy::runCallback(const bool mode, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Выполняем биндинг базы событий для клиента
		if(mode) proxy->_core.server.bind(reinterpret_cast <awh::core_t *> (&proxy->_core.client));
		// Выполняем анбиндинг базы событий клиента
		else proxy->_core.server.unbind(reinterpret_cast <awh::core_t *> (&proxy->_core.client));
	}
}
*/
/**
 * chunkingCallback Функция обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 * @param ctx   передаваемый контекст модуля
 */
/*
void awh::server::Proxy::chunkingCallback(const vector <char> & chunk, const http_t * http, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) ctx;
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <http_t *> (http)->addBody(chunk.data(), chunk.size());
}
*/
/**
 * openServerCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
/*
void awh::server::Proxy::openServerCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(wid, proxy->_port, proxy->_host);
		// Выполняем запуск сервера
		reinterpret_cast <server::core_t *> (core)->run(wid);
	}
}
*/
/**
 * persistServerCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
/*
void awh::server::Proxy::persistServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && ((adj->method != web_t::method_t::CONNECT) || adj->close) && ((!adj->_alive && !proxy->_alive) || adj->close)){
			// Если клиент давно должен был быть отключён, отключаем его
			if(adj->close || !adj->srv.isAlive()) proxy->close(aid);
			// Иначе проверяем прошедшее время
			else {
				// Получаем текущий штамп времени
				const time_t stamp = proxy->fmk->unixTimestamp();
				// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
				if((stamp - adj->checkPoint) >= proxy->_keepAlive)
					// Завершаем работу
					proxy->close(aid);
			}
		}
	}
}
*/
/**
 * connectClientCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
/*
void awh::server::Proxy::connectClientCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->_worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->_worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->_worker.getAdj(it->second));
			// Если подключение не выполнено
			if(!adj->connect){
				// Разрешаем обработки данных
				adj->locked = false;
				// Запоминаем, что подключение выполнено
				adj->connect = true;
				// Выполняем сброс состояния HTTP парсера
				adj->srv.clear();
				// Выполняем сброс состояния HTTP парсера
				adj->srv.reset();
				// Если метод подключения CONNECT
				if(adj->method == web_t::method_t::CONNECT){
					// Формируем ответ клиенту
					const auto & response = adj->srv.response((u_int) 200);
					// Если ответ получен
					if(!response.empty()){
						// Тело полезной нагрузки
						vector <char> payload;
						// Отправляем ответ клиенту
						reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(response.data(), response.size(), it->second);
						// Получаем данные тела запроса
						while(!(payload = adj->srv.payload()).empty()){
							// Отправляем тело на сервер
							reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(payload.data(), payload.size(), it->second);
						}
					// Выполняем отключение клиента
					} else proxy->close(it->second);
				// Отправляем сообщение на сервер, так-как оно пришло от клиента
				} else proxy->prepare(it->second, proxy->_worker.wid, reinterpret_cast <awh::core_t *> (&proxy->_core.server));
			}
		}
	}
}
*/
/**
 * connectServerCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
/*
void awh::server::Proxy::connectServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Создаём адъютанта
		proxy->_worker.createAdj(aid);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Устанавливаем размер чанка
		adj->cli.setChunkSize(proxy->_chunkSize);
		adj->srv.setChunkSize(proxy->_chunkSize);
		// Устанавливаем данные сервиса
		adj->cli.setServ(proxy->_sid, proxy->_name, proxy->_version);
		adj->srv.setServ(proxy->_sid, proxy->_name, proxy->_version);
		// Если функция обратного вызова для обработки чанков установлена
		if(proxy->_chunkingFn != nullptr)
			// Устанавливаем внешнюю функцию обработки вызова для получения чанков
			adj->cli.setChunkingFn(proxy->ctx.at(5), proxy->_chunkingFn);
		// Устанавливаем функцию обработки вызова для получения чанков
		else adj->cli.setChunkingFn(proxy, &chunkingCallback);
		// Устанавливаем функцию обработки вызова для получения чанков
		adj->srv.setChunkingFn(proxy, &chunkingCallback);
		// Устанавливаем метод компрессии поддерживаемый клиентом
		adj->cli.setCompress(proxy->_worker.compress);
		// Устанавливаем метод компрессии поддерживаемый сервером
		adj->srv.setCompress(proxy->_worker.compress);
		// Если данные будем передавать в зашифрованном виде
		if(proxy->_crypt){
			// Устанавливаем параметры шифрования
			adj->cli.setCrypt(proxy->_pass, proxy->_salt, proxy->_aes);
			adj->srv.setCrypt(proxy->_pass, proxy->_salt, proxy->_aes);
		}
		// Если сервер требует авторизацию
		if(proxy->_authType != auth_t::type_t::NONE){
			// Определяем тип авторизации
			switch((uint8_t) proxy->_authType){
				// Если тип авторизации Basic
				case (uint8_t) auth_t::type_t::BASIC: {
					// Устанавливаем параметры авторизации
					adj->srv.setAuthType(proxy->_authType);
					// Устанавливаем функцию проверки авторизации
					adj->srv.setAuthCallback(proxy->ctx.at(4), proxy->_checkAuthFn);
				} break;
				// Если тип авторизации Digest
				case (uint8_t) auth_t::type_t::DIGEST: {
					// Устанавливаем название сервера
					adj->srv.setRealm(proxy->_realm);
					// Устанавливаем временный ключ сессии сервера
					adj->srv.setOpaque(proxy->_opaque);
					// Устанавливаем параметры авторизации
					adj->srv.setAuthType(proxy->_authType, proxy->_authHash);
					// Устанавливаем функцию извлечения пароля
					adj->srv.setExtractPassCallback(proxy->ctx.at(3), proxy->_extractPassFn);
				} break;
			}
		}
		// Устанавливаем контекст сообщения
		adj->_worker.ctx = proxy;
		// Устанавливаем флаг ожидания входящих сообщений
		adj->_worker.wait = proxy->_worker.wait;
		// Устанавливаем количество секунд на чтение
		adj->_worker.timeRead = proxy->_worker.timeRead;
		// Устанавливаем количество секунд на запись
		adj->_worker.timeWrite = proxy->_worker.timeWrite;
		// Устанавливаем функцию чтения данных
		adj->_worker.readFn = readClientCallback;
		// Устанавливаем событие подключения
		adj->_worker.connectFn = connectClientCallback;
		// Устанавливаем событие отключения
		adj->_worker.disconnectFn = disconnectClientCallback;
		// Добавляем воркер в биндер TCP/IP
		proxy->_core.client.add(&adj->_worker);
		// Создаём пару клиента и сервера
		proxy->_worker.pairs.emplace(adj->_worker.wid, aid);
		// Если функция обратного вызова установлена, выполняем
		if(proxy->_activeFn != nullptr) proxy->_activeFn(aid, mode_t::CONNECT, proxy, proxy->ctx.at(0));
	}
}
*/
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
/*
void awh::server::Proxy::disconnectClientCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->_worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->_worker.pairs.end()){
			// Получаем идентификатор адъютанта
			const size_t aid = it->second;
			// Удаляем пару клиента и сервера
			proxy->_worker.pairs.erase(it);
			// Получаем параметры подключения адъютанта
			workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->_worker.getAdj(aid));
			// Если подключение не выполнено, отправляем ответ клиенту
			if(!adj->connect)
				// Выполняем реджект
				proxy->reject(aid, 404);
			// Устанавливаем флаг отключения клиента
			else adj->close = true;
		}
	}
}
*/
/**
 * disconnectServerCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
/*
void awh::server::Proxy::disconnectServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Выполняем отключение клиента от стороннего сервера
		if(adj != nullptr) adj->close = true;
	}
}
*/
/**
 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
 * @param ip   адрес интернет подключения клиента
 * @param mac  мак-адрес подключившегося клиента
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 * @return     результат разрешения к подключению клиента
 */
/*
bool awh::server::Proxy::acceptServerCallback(const string & ip, const string & mac, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Если функция обратного вызова установлена, проверяем
		if(proxy->_acceptFn != nullptr) result = proxy->_acceptFn(ip, mac, proxy, proxy->ctx.at(6));
	}
	// Разрешаем подключение клиенту
	return result;
}
*/
/**
 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
/*
void awh::server::Proxy::readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Ищем идентификатор адъютанта пары
		auto it = proxy->_worker.pairs.find(wid);
		// Если адъютант получен
		if(it != proxy->_worker.pairs.end()){
			// Получаем параметры подключения адъютанта
			workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->_worker.getAdj(it->second));
			// Если подключение выполнено, отправляем ответ клиенту
			if(adj->connect){
				// Если указан метод не CONNECT и функция обработки сообщения установлена
				if((proxy->_messageFn != nullptr) && (adj->method != web_t::method_t::CONNECT)){
					// Добавляем полученные данные в буфер
					adj->client.insert(adj->client.end(), buffer, buffer + size);
					// Выполняем обработку полученных данных
					while(!adj->close && !adj->client.empty()){
						// Выполняем парсинг полученных данных
						size_t bytes = adj->cli.parse(adj->client.data(), adj->client.size());
						// Если все данные получены
						if(adj->cli.isEnd()){
							// Получаем параметры запроса
							const auto & query = adj->cli.getQuery();
							// Если включён режим отладки
							#if defined(DEBUG_MODE)
								// Получаем данные ответа
								const auto & response = adj->cli.response(true);
								// Если параметры ответа получены
								if(!response.empty()){
									// Выводим заголовок ответа
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры ответа
									cout << string(response.begin(), response.end()) << endl;
									// Если тело ответа существует
									if(!adj->cli.getBody().empty())
										// Выводим сообщение о выводе чанка тела
										cout << proxy->fmk->format("<body %u>", adj->cli.getBody().size()) << endl << endl;
									// Иначе устанавливаем перенос строки
									else cout << endl;
								}
							#endif
							// Выводим сообщение
							if(proxy->_messageFn(it->second, event_t::RESPONSE, &adj->cli, proxy, proxy->ctx.at(1))){
								// Получаем данные ответа
								const auto & response = adj->cli.response();
								// Если данные ответа получены
								if(!response.empty()){
									// Тело REST сообщения
									vector <char> entity;
									// Отправляем ответ клиенту
									reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(response.data(), response.size(), it->second);
									// Получаем данные тела ответа
									while(!(entity = adj->cli.payload()).empty()){
										// Отправляем тело клиенту
										reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(entity.data(), entity.size(), it->second);
									}
								}
							}
						}
						// Устанавливаем метку продолжения обработки пайплайна
						Next:
						// Если парсер обработал какое-то количество байт
						if((bytes > 0) && !adj->client.empty()){
							// Если размер буфера больше количества удаляемых байт
							if(adj->client.size() >= bytes)
								// Удаляем количество обработанных байт
								vector <decltype(adj->client)::value_type> (adj->client.begin() + bytes, adj->client.end()).swap(adj->client);
							// Если байт в буфере меньше, просто очищаем буфер
							else adj->client.clear();
							// Если данных для обработки не осталось, выходим
							if(adj->client.empty()) break;
						// Если данных для обработки недостаточно, выходим
						} else break;
					}
				// Передаём данные так-как они есть
				} else {
					// Если функция обратного вызова установлена, выполняем
					if(proxy->_binaryFn != nullptr){
						// Выводим сообщение
						if(proxy->_binaryFn(it->second, event_t::RESPONSE, buffer, size, proxy, proxy->ctx.at(2)))
							// Отправляем ответ клиенту
							reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(buffer, size, it->second);
					// Отправляем ответ клиенту
					} else reinterpret_cast <awh::core_t *> (&proxy->_core.server)->write(buffer, size, it->second);
				}
			}
		}
	}
}
*/
/**
 * readServerCallback Функция обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
/*
void awh::server::Proxy::readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (buffer != nullptr) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если указан метод CONNECT
			if(adj->connect && (adj->method == web_t::method_t::CONNECT)){
				// Получаем идентификатор адъютанта
				const size_t aid = adj->_worker.getAid();
				// Отправляем запрос на внешний сервер
				if(aid > 0){
					// Если функция обратного вызова установлена, выполняем
					if(proxy->_binaryFn != nullptr){
						// Выводим сообщение
						if(proxy->_binaryFn(aid, event_t::REQUEST, buffer, size, proxy, proxy->ctx.at(2)))
							// Отправляем запрос на внешний сервер
							reinterpret_cast <awh::core_t *> (&proxy->_core.client)->write(buffer, size, aid);
					// Отправляем запрос на внешний сервер
					} else reinterpret_cast <awh::core_t *> (&proxy->_core.client)->write(buffer, size, aid);
				}
			// Если подключение ещё не выполнено
			} else {
				// Добавляем полученные данные в буфер
				adj->server.insert(adj->server.end(), buffer, buffer + size);
				// Выполняем обработку полученных данных
				proxy->prepare(aid, wid, core);
			}
		}
	}
}
*/
/**
 * writeServerCallback Функция обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
/*
void awh::server::Proxy::writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		proxy_t * proxy = reinterpret_cast <proxy_t *> (ctx);
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (proxy->_worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && (adj->stopBytes > 0)){
			// Запоминаем количество прочитанных байт
			adj->readBytes += size;
			// Если размер полученных байт соответствует
			adj->close = (adj->stopBytes >= adj->readBytes);
		}
	}
}
*/
/**
 * prepare Метод обработки входящих данных
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 */
/*
void awh::server::Proxy::prepare(const size_t aid, const size_t wid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (wid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->_worker.getAdj(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && !adj->locked){
			// Выполняем обработку полученных данных
			while(!adj->close && !adj->server.empty()){
				// Выполняем парсинг полученных данных
				size_t bytes = adj->srv.parse(adj->server.data(), adj->server.size());
				// Если все данные получены
				if(adj->srv.isEnd()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Получаем данные запроса
						const auto & request = adj->srv.request(true);
						// Если параметры запроса получены
						if(!request.empty()){
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(request.begin(), request.end()) << endl;
							// Если тело запроса существует
							if(!adj->srv.getBody().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->fmk->format("<body %u>", adj->srv.getBody().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Если подключение не установлено как постоянное
					if(!this->_alive && !adj->_alive){
						// Увеличиваем количество выполненных запросов
						adj->requests++;
						// Если количество выполненных запросов превышает максимальный
						if(adj->requests >= this->_maxRequests)
							// Устанавливаем флаг закрытия подключения
							adj->close = true;
						// Получаем текущий штамп времени
						else adj->checkPoint = this->fmk->unixTimestamp();
					// Выполняем сброс количества выполненных запросов
					} else adj->requests = 0;
					// Выполняем проверку авторизации
					switch((uint8_t) adj->srv.getAuth()){
						// Если запрос выполнен удачно
						case (uint8_t) http_t::stath_t::GOOD: {
							// Получаем флаг шифрованных данных
							adj->_crypt = adj->srv.isCrypt();
							// Получаем поддерживаемый метод компрессии
							adj->compress = adj->srv.getCompress();
							// Если подключение не выполнено
							if(!adj->connect){
								// Получаем данные запроса
								const auto & query = adj->srv.getQuery();
								// Выполняем проверку разрешено ли нам выполнять подключение
								const bool allow = (!this->_noConnect || (query.method != web_t::method_t::CONNECT));
								// Получаем хост сервера
								const auto & host = adj->srv.getHeader("host");
								// Сообщение запрета подключения
								const string message = (allow ? "" : "Connect method prohibited");
								// Если хост сервера получен
								if(allow && (adj->locked = !host.empty())){
									// Запоминаем метод подключения
									adj->method = query.method;
									// Формируем адрес подключения
									adj->_worker.url = this->_worker.uri.parseUrl(this->fmk->format("http://%s", host.c_str()));
									// Выполняем запрос на сервер
									this->_core.client.open(adj->_worker.wid);
									// Выходим из функции
									return;
								// Если хост не получен
								} else {
									// Выполняем сброс состояния HTTP парсера
									adj->srv.clear();
									// Выполняем сброс состояния HTTP парсера
									adj->srv.reset();
									// Выполняем очистку буфера полученных данных
									adj->server.clear();
									// Формируем запрос реджекта
									const auto & response = adj->srv.reject(403, message);
									// Если ответ получен
									if(!response.empty()){
										// Тело полезной нагрузки
										vector <char> payload;
										// Отправляем ответ клиенту
										core->write(response.data(), response.size(), aid);
										// Получаем данные тела запроса
										while(!(payload = adj->srv.payload()).empty()){
											// Отправляем тело на сервер
											core->write(payload.data(), payload.size(), aid);
										}
									// Выполняем отключение клиента
									} else this->close(aid);
									// Выходим из функции
									return;
								}
							// Если подключение выполнено
							} else {
								// Выполняем удаление заголовка авторизации на прокси-сервере
								adj->srv.rmHeader("proxy-authorization");
								{
									// Получаем данные запроса
									const auto & query = adj->srv.getQuery();
									// Получаем данные заголовка Via
									string via = adj->srv.getHeader("via");
									// Если заголовок получен
									if(!via.empty())
										// Устанавливаем Via заголовок
										via = this->fmk->format("%s, %.1f %s:%u", via.c_str(), query.ver, this->_host.c_str(), this->_port);
									// Иначе просто формируем заголовок Via
									else via = this->fmk->format("%.1f %s:%u", query.ver, this->_host.c_str(), this->_port);
									// Устанавливаем заголовок Via
									adj->srv.addHeader("Via", via);
								}{
									// Название операционной системы
									const char * os = nullptr;
									// Определяем название операционной системы
									switch((uint8_t) this->fmk->os()){
										// Если операционной системой является Unix
										case (uint8_t) fmk_t::os_t::UNIX: os = "Unix"; break;
										// Если операционной системой является Linux
										case (uint8_t) fmk_t::os_t::LINUX: os = "Linux"; break;
										// Если операционной системой является неизвестной
										case (uint8_t) fmk_t::os_t::NONE: os = "Unknown"; break;
										// Если операционной системой является Windows
										case (uint8_t) fmk_t::os_t::WIND32:
										case (uint8_t) fmk_t::os_t::WIND64: os = "Windows"; break;
										// Если операционной системой является MacOS X
										case (uint8_t) fmk_t::os_t::MACOSX: os = "MacOS X"; break;
										// Если операционной системой является FreeBSD
										case (uint8_t) fmk_t::os_t::FREEBSD: os = "FreeBSD"; break;
									}
									// Устанавливаем наименование агента
									adj->srv.addHeader("X-Proxy-Agent", this->fmk->format("(%s; %s) %s/%s", os, this->_name.c_str(), this->_sid.c_str(), this->_version.c_str()));
								}
								// Если функция обратного вызова установлена, выполняем
								if(this->_messageFn != nullptr){
									// Выводим сообщение
									if(this->_messageFn(aid, event_t::REQUEST, &adj->srv, this, this->ctx.at(1))){
										// Получаем данные запроса
										const auto & request = adj->srv.request();
										// Если данные запроса получены
										if(!request.empty()){
											// Получаем идентификатор адъютанта
											const size_t aid = adj->_worker.getAid();
											// Отправляем запрос на внешний сервер
											if(aid > 0){
												// Тело REST сообщения
												vector <char> entity;
												// Отправляем серверу сообщение
												reinterpret_cast <awh::core_t *> (&this->_core.client)->write(request.data(), request.size(), aid);
												// Получаем данные тела запроса
												while(!(entity = adj->srv.payload()).empty()){
													// Отправляем тело на сервер
													reinterpret_cast <awh::core_t *> (&this->_core.client)->write(entity.data(), entity.size(), aid);
												}
											}
										}
									}
								// Если функция обратного вызова не установлена
								} else {
									// Получаем идентификатор адъютанта
									const size_t aid = adj->_worker.getAid();
									// Отправляем запрос на внешний сервер
									if(aid > 0){
										// Получаем данные запроса
										const auto & request = adj->srv.request();
										// Если данные запроса получены
										if(!request.empty()){
											// Тело REST сообщения
											vector <char> entity;
											// Если функция обратного вызова установлена, выполняем
											if(this->_binaryFn != nullptr){
												// Выводим сообщение
												if(this->_binaryFn(aid, event_t::REQUEST, request.data(), request.size(), this, this->ctx.at(2)))
													// Отправляем запрос на внешний сервер
													reinterpret_cast <awh::core_t *> (&this->_core.client)->write(request.data(), request.size(), aid);
											// Отправляем запрос на внешний сервер
											} else reinterpret_cast <awh::core_t *> (&this->_core.client)->write(request.data(), request.size(), aid);
											// Получаем данные тела запроса
											while(!(entity = adj->srv.payload()).empty()){
												// Отправляем тело на сервер
												reinterpret_cast <awh::core_t *> (&this->_core.client)->write(entity.data(), entity.size(), aid);
											}
										}
									}
								}
							}
							// Выполняем сброс состояния HTTP парсера
							adj->srv.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->srv.reset();
							// Завершаем обработку
							goto Next;
						} break;
						// Если запрос неудачный
						case (uint8_t) http_t::stath_t::FAULT: {
							// Выполняем сброс состояния HTTP парсера
							adj->srv.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->srv.reset();
							// Выполняем очистку буфера полученных данных
							adj->server.clear();
							// Формируем запрос авторизации
							const auto & response = adj->srv.reject(407);
							// Если ответ получен
							if(!response.empty()){
								// Тело полезной нагрузки
								vector <char> payload;
								// Устанавливаем размер стопбайт
								if(!adj->srv.isAlive()) adj->stopBytes = response.size();
								// Отправляем ответ клиенту
								core->write(response.data(), response.size(), aid);
								// Получаем данные тела запроса
								while(!(payload = adj->srv.payload()).empty()){
									// Устанавливаем размер стопбайт
									if(!adj->srv.isAlive()) adj->stopBytes += payload.size();
									// Отправляем тело на сервер
									core->write(payload.data(), payload.size(), aid);
								}
							// Выполняем отключение клиента
							} else this->close(aid);
							// Выходим из функции
							return;
						}
					}
				}
				// Устанавливаем метку продолжения обработки пайплайна
				Next:
				// Если парсер обработал какое-то количество байт
				if((bytes > 0) && !adj->server.empty()){
					// Если размер буфера больше количества удаляемых байт
					if(adj->server.size() >= bytes)
						// Удаляем количество обработанных байт
						vector <decltype(adj->server)::value_type> (adj->server.begin() + bytes, adj->server.end()).swap(adj->server);
					// Если байт в буфере меньше, просто очищаем буфер
					else adj->server.clear();
					// Если данных для обработки не осталось, выходим
					if(adj->server.empty()) break;
				// Если данных для обработки недостаточно, выходим
				} else break;
			}
		}
	}
}
*/
/**
 * init Метод инициализации WebSocket клиента
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
/*
void awh::server::Proxy::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Устанавливаем порт сервера
	this->_port = port;
	// Устанавливаем хост сервера
	this->_host = host;
	// Устанавливаем тип компрессии
	this->_worker.compress = compress;
}
*/
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
/*
void awh::server::Proxy::on(function <void (const size_t, const mode_t, Proxy *, void *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_activeFn = callback;
}
*/
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
/*
void awh::server::Proxy::on(function <bool (const size_t, const event_t, http_t *, Proxy *, void *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений с сервера
	this->_messageFn = callback;
}
*/
/**
 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
 * @param callback функция обратного вызова
 */
/*
void awh::server::Proxy::on(function <bool (const size_t, const event_t, const char *, const size_t, Proxy *, void *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений в бинарном виде с сервера
	this->_binaryFn = callback;
}
*/
/**
 * on Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
/*
void awh::server::Proxy::on(function <string (const string &, void *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для извлечения пароля
	this->_extractPassFn = callback;
}
*/
/**
 * on Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
/*
void awh::server::Proxy::on(function <bool (const string &, const string &, void *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->_checkAuthFn = callback;
}
*/
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
/*
void awh::server::Proxy::on(function <void (const vector <char> &, const http_t *, void *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения чанков
	this->_chunkingFn = callback;
}
*/
/**
 * on Метод установки функции обратного вызова на событие активации клиента на сервере
 * @param callback функция обратного вызова
 */
/*
void awh::server::Proxy::on(function <bool (const string &, const string &, Proxy *, void *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_acceptFn = callback;
}
*/
/**
 * reject Метод отправки сообщения об ошибке
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для клиента
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
/*
void awh::server::Proxy::reject(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->_core.server.working()){
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->_worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->srv.setBody(entity);
			// Устанавливаем заголовки ответа
			adj->srv.setHeaders(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->_alive && !adj->_alive && adj->srv.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->srv.addHeader("Keep-Alive", this->fmk->format("timeout=%d, max=%d", this->_keepAlive / 1000, this->_maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->srv.reject(code, mess);
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем размер стопбайт
			if(!adj->srv.isAlive()) adj->stopBytes = response.size();
			// Отправляем серверу сообщение
			((awh::core_t *) const_cast <server::core_t *> (&this->_core.server))->write(response.data(), response.size(), aid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = adj->srv.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Устанавливаем размер стопбайт
				if(!adj->srv.isAlive()) adj->stopBytes += payload.size();
				// Отправляем тело на сервер
				((awh::core_t *) const_cast <server::core_t *> (&this->_core.server))->write(payload.data(), payload.size(), aid);
			}
		}
	}
}
*/
/**
 * response Метод отправки сообщения клиенту
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для клиента
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
/*
void awh::server::Proxy::response(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) const noexcept {
	// Если подключение выполнено
	if(this->_core.server.working()){
		// Получаем параметры подключения адъютанта
		workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->_worker.getAdj(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->srv.setBody(entity);
			// Устанавливаем заголовки ответа
			adj->srv.setHeaders(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->_alive && !adj->_alive && adj->srv.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->srv.addHeader("Keep-Alive", this->fmk->format("timeout=%d, max=%d", this->_keepAlive / 1000, this->_maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->srv.response(code, mess);
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем размер стопбайт
			if(!adj->srv.isAlive()) adj->stopBytes = response.size();
			// Отправляем серверу сообщение
			((awh::core_t *) const_cast <server::core_t *> (&this->_core.server))->write(response.data(), response.size(), aid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = adj->srv.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Устанавливаем размер стопбайт
				if(!adj->srv.isAlive()) adj->stopBytes += payload.size();
				// Отправляем тело на сервер
				((awh::core_t *) const_cast <server::core_t *> (&this->_core.server))->write(payload.data(), payload.size(), aid);
			}
		}
	}
}
*/
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
/*
const string & awh::server::Proxy::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->_worker.ip(aid);
}
*/
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
/*
const string & awh::server::Proxy::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->_worker.mac(aid);
}
*/
/**
 * setAlive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
/*
void awh::server::Proxy::setAlive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->_alive = mode;
}
*/
/**
 * setAlive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
/*
void awh::server::Proxy::setAlive(const size_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->_worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг пдолгоживущего подключения
	if(adj != nullptr) adj->_alive = mode;
}
*/
/**
 * start Метод запуска клиента
 */
/*
void awh::server::Proxy::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core.server.working())
		// Выполняем запуск биндинга
		this->_core.server.start();
}
*/
/**
 * stop Метод остановки клиента
 */
/*
void awh::server::Proxy::stop() noexcept {
	// Если подключение выполнено
	if(this->_core.server.working())
		// Завершаем работу, если разрешено остановить
		this->_core.server.stop();
}
*/
/**
 * close Метод закрытия подключения клиента
 * @param aid идентификатор адъютанта
 */
/*
void awh::server::Proxy::close(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	workerProxy_t::adjp_t * adj = const_cast <workerProxy_t::adjp_t *> (this->_worker.getAdj(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Выполняем отключение всех дочерних клиентов
		reinterpret_cast <awh::core_t *> (&this->_core.client)->close(adj->_worker.getAid());
		// Выполняем удаление параметров адъютанта
		this->_worker.removeAdj(aid);
	}
	// Отключаем клиента от сервера
	reinterpret_cast <awh::core_t *> (&this->_core.server)->close(aid);
	// Если функция обратного вызова установлена, выполняем
	if(this->_activeFn != nullptr) this->_activeFn(aid, mode_t::DISCONNECT, this, this->ctx.at(0));
}
*/
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
/*
void awh::server::Proxy::setRealm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->_realm = realm;
}
*/
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
/*
void awh::server::Proxy::setOpaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->_opaque = opaque;
}
*/
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
/*
void awh::server::Proxy::setAuthType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->_authHash = hash;
	// Устанавливаем тип авторизации
	this->_authType = type;
}
*/
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
/*
void awh::server::Proxy::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг запрещающий метод CONNECT
	this->_noConnect = (flag & (uint8_t) flag_t::NOCONNECT);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг отложенных вызовов событий сокета
	this->_core.client.setDefer(flag & (uint8_t) flag_t::DEFER);
	this->_core.server.setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_core.client.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
	this->_core.server.setNoInfo(flag & (uint8_t) flag_t::NOINFO);
}
*/
/**
 * setTotal Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
/*
void awh::server::Proxy::setTotal(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	this->_core.server.setTotal(this->_worker.wid, total);
}
*/
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
/*
void awh::server::Proxy::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_chunkSize = (size > 0 ? size : BUFFER_CHUNK);
}
*/
/**
 * setKeepAlive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
/*
void awh::server::Proxy::setKeepAlive(const size_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->_keepAlive = time;
}
*/
/**
 * setMaxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
/*
void awh::server::Proxy::setMaxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->_maxRequests = max;
}
*/
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
/*
void awh::server::Proxy::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_worker.compress = compress;
}
*/
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
/*
void awh::server::Proxy::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_worker.timeRead = read;
	// Устанавливаем количество секунд на запись
	this->_worker.timeWrite = write;
}
*/
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
/*
void awh::server::Proxy::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификатор сервера
	this->_sid = id;
	// Устанавливаем название сервера
	this->_name = name;
	// Устанавливаем версию сервера
	this->_version = ver;
}
*/
/**
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
/*
void awh::server::Proxy::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->_worker.markWrite = write;
}
*/
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
/*
void awh::server::Proxy::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем флаг шифрования
	if((this->_crypt = !pass.empty())){
		// Размер шифрования передаваемых данных
		this->_aes = aes;
		// Пароль шифрования передаваемых данных
		this->_pass = pass;
		// Соль шифрования передаваемых данных
		this->_salt = salt;
	}
}
*/
/**
 * Proxy Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
/*
awh::server::Proxy::Proxy(const fmk_t * fmk, const log_t * log) noexcept : core(fmk, log), worker(fmk, log), fmk(fmk), log(log) {
	// Устанавливаем контекст сообщения
	this->_worker.ctx = this;
	// Устанавливаем событие на запуск системы
	this->_worker.openFn = openServerCallback;
	// Устанавливаем функцию чтения данных
	this->_worker.readFn = readServerCallback;
	// Устанавливаем функцию записи данных
	this->_worker.writeFn = writeServerCallback;
	// Добавляем событие аццепта клиента
	this->_worker.acceptFn = acceptServerCallback;
	// Устанавливаем функцию персистентного вызова
	this->_worker.persistFn = persistServerCallback;
	// Устанавливаем событие подключения
	this->_worker.connectFn = connectServerCallback;
	// Устанавливаем событие отключения
	this->_worker.disconnectFn = disconnectServerCallback;
	// Активируем персистентный запуск для работы пингов
	this->_core.server.setPersist(true);
	// Добавляем воркер в биндер TCP/IP
	this->_core.server.add(&this->_worker);
	// Устанавливаем функцию активации ядра сервера
	this->_core.server.setCallback(this, &runCallback);
	// Устанавливаем интервал персистентного таймера для работы пингов
	this->_core.server.setPersistInterval(KEEPALIVE_TIMEOUT / 2);
}
*/
