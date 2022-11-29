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
#include <server/sample.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * Server Класс объекта исполнителя
 */
class Server {
	private:
		// Объект логирования
		log_t * _log;
	public:
		/**
		 * accept Метод активации клиента на сервере
		 * @param ip     адрес интернет подключения
		 * @param mac    аппаратный адрес подключения
		 * @param port   порт подключения
		 * @param sample объект сервера
		 * @return       результат проверки
		 */
		bool accept(const string & ip, const string & mac, const u_int port, server::sample_t * sample){
			// Выводим информацию в лог
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			// Разрешаем подключение клиенту
			return true;
		}
		/**
		 * active Метод идентификации активности на сервере
		 * @param aid    идентификатор адъютанта (клиента)
		 * @param mode   режим события подключения
		 * @param sample объект сервера
		 */
		void active(const size_t aid, const server::sample_t::mode_t mode, server::sample_t * sample){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		/**
		 * message Метод получения сообщений
		 * @param aid    идентификатор адъютанта (клиента)
		 * @param buffer буфер входящих данных
		 * @param sample объект сервера
		 */
		void message(const size_t aid, const vector <char> & buffer, server::sample_t * sample){
			// Получаем сообщение
			const string message(buffer.begin(), buffer.end());
			// Выводим информацию в лог
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			// Отправляем сообщение обратно
			sample->send(aid, buffer.data(), buffer.size());
		}
	public:
		/**
		 * Server Конструктор
		 * @param log объект логирования
		 */
		Server(log_t * log) : _log(log) {}
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
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект исполнителя
	Server executor(&log);
	// Создаём биндинг
	server::core_t core(&fmk, &log);
	// Создаём объект SAMPLE запроса
	server::sample_t sample(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.name("SAMPLE Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	// sample.mode((uint8_t) server::sample_t::flag_t::WAITMESS);
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем адрес сертификата
	// core.ca("./ca/cert.pem");
	// Устанавливаем название сервера
	// core.nameServer("anyks");
	// Устанавливаем тип сокета unix-сокет
	// core.family(awh::scheme_t::family_t::NIX);
	// Устанавливаем тип сокета UDP TLS
	core.sonet(awh::scheme_t::sonet_t::DTLS);
	// core.sonet(awh::scheme_t::sonet_t::TLS);
	// core.sonet(awh::scheme_t::sonet_t::UDP);
	// core.sonet(awh::scheme_t::sonet_t::TCP);
	// core.sonet(awh::scheme_t::sonet_t::SCTP);
	// Отключаем валидацию сертификата
	core.verifySSL(false);
	// Выполняем инициализацию Sample сервера
	// sample.init(2222);
	// sample.init("anyks");
	sample.init(2222, "127.0.0.1");
	// Устанавливаем длительное подключение
	// sample.keepAlive(100, 30, 10);
	// Устанавливаем SSL сертификаты сервера
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");
	// Установливаем функцию обратного вызова на событие получения сообщений
	sample.on((function <void (const size_t, const vector <char> &, server::sample_t *)>) bind(&Server::message, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	sample.on((function <void (const size_t, const server::sample_t::mode_t, server::sample_t *)>) bind(&Server::active, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	sample.on((function <bool (const string &, const string &, const u_int, server::sample_t *)>) bind(&Server::accept, &executor, _1, _2, _3, _4));
	// Выполняем запуск SAMPLE сервер
	sample.start();
	// Выводим результат
	return 0;
}
