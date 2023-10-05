/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <server/ws.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;
using namespace awh::server;

/**
 * Executor Класс объекта исполнителя
 */
class Executor {
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	private:
		// Объект WebSocket-сервера
		server::websocket_t * _ws;
	public:
		/**
		 * password Метод извлечения пароля (для авторизации методом Digest)
		 * @param login логин пользователя
		 * @return      пароль пользователя хранящийся в базе данных
		 */
		string password(const string & login){
			// Выводим информацию в лог
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), "password");
			// Выводим пароль
			return "password";
		}
		/**
		 * auth Метод проверки авторизации пользователя (для авторизации методом Basic)
		 * @param login    логин пользователя (от клиента)
		 * @param password пароль пользователя (от клиента)
		 * @return         результат авторизации
		 */
		bool auth(const string & login, const string & password){
			// Выводим информацию в лог
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());
			// Разрешаем авторизацию
			return true;
		}
	public:
		/**
		 * accept Метод активации клиента на сервере
		 * @param ip   адрес интернет подключения
		 * @param mac  аппаратный адрес подключения
		 * @param port порт подключения
		 * @return     результат проверки
		 */
		bool accept(const string & ip, const string & mac, const u_int port){
			// Выводим информацию в лог
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			// Разрешаем подключение клиенту
			return true;
		}
		/**
		 * active Метод событий сервера
		 * @param sid  идентификатор потока
		 * @param aid  идентификатор адъютанта (клиента)
		 * @param mode флаг события
		 */
		void active(const int32_t sid, const uint64_t aid, const server::web_t::mode_t mode){
			// Блокируем неиспользуемую переменную
			(void) sid;
			// Определяем флаг события сервера
			switch(static_cast <uint8_t> (mode)){
				// Если клиент подключился к серверу
				case static_cast <uint8_t> (server::web_t::mode_t::OPEN): {
					// Выводим информацию в лог
					this->_log->print("CONNECT", log_t::flag_t::INFO);
				} break;
				// Если клиент отключился от сервера
				case static_cast <uint8_t> (server::web_t::mode_t::CLOSE):
					// Выводим информацию в лог
					this->_log->print("DISCONNECT", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * error Метод вывода ошибок WebSocket сервера
		 * @param aid  идентификатор адъютанта (клиента)
		 * @param code код ошибки
		 * @param mess сообщение ошибки
		 */
		void error(const uint64_t aid, const u_int code, const string & mess){
			// Выводим информацию в лог
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		/**
		 * message Метод получения сообщений
		 * @param aid    идентификатор адъютанта (клиента)
		 * @param buffer бинарный буфер сообщения
		 * @param text   тип буфера сообщения
		 */
		void message(const uint64_t aid, const vector <char> & buffer, const bool text){
			// Если даныне получены
			if(!buffer.empty()){
				// Выводим информацию в лог
				this->_log->print("Message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), this->_ws->sub(aid).c_str());
				// Отправляем сообщение обратно
				this->_ws->send(aid, buffer.data(), buffer.size(), text);
			}
		}
	public:
		/**
		 * Executor Конструктор
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 * @param ws  объект WebSocket-сервера
		 */
		Executor(const fmk_t * fmk, const log_t * log, server::websocket_t * ws) : _fmk(fmk), _log(log), _ws(ws) {}
};

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int main(int argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём биндинг
	server::core_t core(&fmk, &log);
	// Создаём объект REST запроса
	websocket_t ws(&core, &fmk, &log);
	// Создаём объект исполнителя
	Executor executor(&fmk, &log, &ws);
	// Устанавливаем название сервиса
	log.name("WebSocket Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем валидацию SSL сертификата
	 * 2. Устанавливаем флаг перехвата контекста декомпрессии
	 * 3. Устанавливаем флаг перехвата контекста компрессии
	 */
	ws.mode({
		// server::web_t::flag_t::NOT_STOP,
		// server::web_t::flag_t::NOT_INFO,
		// server::web_t::flag_t::WAIT_MESS,
		server::web_t::flag_t::VERIFY_SSL,
		server::web_t::flag_t::TAKEOVER_CLIENT,
		server::web_t::flag_t::TAKEOVER_SERVER
	});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем активный протокол подключения
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Устанавливаем тип сокета unix-сокет
	// core.family(awh::scheme_t::family_t::NIX);
	// Устанавливаем тип сокета UDP TLS
	// core.sonet(awh::scheme_t::sonet_t::DTLS);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	// core.sonet(awh::scheme_t::sonet_t::UDP);
	// core.sonet(awh::scheme_t::sonet_t::TCP);
	// core.sonet(awh::scheme_t::sonet_t::SCTP);
	// Активируем максимальное количество рабочих процессов
	// core.clusterSize();
	// Отключаем валидацию сертификата
	// core.verifySSL(false);
	// Разрешаем выполняем автоматический перезапуск упавшего процесса
	// ws.clusterAutoRestart(true);
	// Выполняем активацию многопоточности
	// ws.multiThreads(22);
	// Устанавливаем название сервера
	// ws.realm("ANYKS");
	// Устанавливаем временный ключ сессии
	// ws.opaque("keySession");
	// Устанавливаем тип авторизации
	// ws.authType(awh::auth_t::type_t::BASIC);
	// ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::SHA256);
	// Выполняем инициализацию WebSocket сервера
	// ws.init(2222, "127.0.0.1", awh::http_t::compress_t::DEFLATE);
	ws.init(2222, "", awh::http_t::compress_t::DEFLATE);
	// ws.init("anyks", awh::http_t::compress_t::DEFLATE);
	// Устанавливаем длительное подключение
	// ws.keepAlive(100, 30, 10);
	
	// Устанавливаем SSL сертификаты сервера
	core.certificate(
		"/usr/local/etc/letsencrypt/live/anyks.net/fullchain.pem",
		"/usr/local/etc/letsencrypt/live/anyks.net/privkey.pem"
	);

	// core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");
	// Устанавливаем шифрование
	// ws.crypto("PASS");
	// Устанавливаем сабпротоколы
	ws.subs({"test1", "test2", "test3"});
	// Устанавливаем функцию извлечения пароля
	// ws.on((function <string (const string &)>) std::bind(&Executor::password, &executor, _1));
	// Устанавливаем функцию проверки авторизации
	// ws.on((function <bool (const string &, const string &)>) std::bind(&Executor::auth, &executor, _1, _2));
	// Установливаем функцию обратного вызова на событие получения ошибок
	ws.on((function <void (const uint64_t, const u_int, const string &)>) std::bind(&Executor::error, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	ws.on((function <bool (const string &, const string &, const u_int)>) std::bind(&Executor::accept, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие получения сообщений
	ws.on((function <void (const uint64_t, const vector <char> &, const bool)>) bind(&Executor::message, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	ws.on((function <void (const int32_t, const uint64_t, const server::web_t::mode_t)>) std::bind(&Executor::active, &executor, _1, _2, _3));
	// Выполняем запуск WebSocket сервер
	ws.start();
	// Выводим результат
	return 0;
}
