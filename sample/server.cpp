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
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	private:
		// Объект Сервера
		server::sample_t * _sample;
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
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			// Разрешаем подключение клиенту
			return true;
		}
		/**
		 * active Метод идентификации активности на сервере
		 * @param bid  идентификатор брокера
		 * @param mode режим события подключения
		 */
		void active(const uint64_t bid, const server::sample_t::mode_t mode){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		/**
		 * message Метод получения сообщений
		 * @param bid    идентификатор брокера
		 * @param buffer буфер входящих данных
		 */
		void message(const uint64_t bid, const vector <char> & buffer){
			// Выводим информацию в лог
			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());
			// Отправляем сообщение обратно
			this->_sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		/**
		 * Server Конструктор
		 * @param fmk    объект фреймворка
		 * @param log    объект логирования
		 * @param sample объект сервера
		 */
		Server(const fmk_t * fmk, const log_t * log, server::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
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
	// Объект DNS-резолвера
	dns_t dns(&fmk, &log);
	// Создаём биндинг
	server::core_t core(&dns, &fmk, &log);
	// Создаём объект SAMPLE запроса
	server::sample_t sample(&core, &fmk, &log);
	// Создаём объект исполнителя
	Server executor(&fmk, &log, &sample);
	// Устанавливаем название сервиса
	log.name("SAMPLE Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	// sample.mode({server::sample_t::flag_t::WAIT_MESS});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Активируем максимальное количество рабочих процессов
	// core.clusterSize();
	// Устанавливаем адрес сертификата
	core.ca("./ca/cert.pem");
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
	// Разрешаем перехват сигналов
	core.signalInterception(core_t::mode_t::DISABLED);
	// Устанавливаем SSL сертификаты сервера
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");
	// Установливаем функцию обратного вызова на событие получения сообщений
	sample.callback <void (const uint64_t, const vector <char> &)> ("message", std::bind(&Server::message, &executor, _1, _2));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	sample.callback <void (const uint64_t, const server::sample_t::mode_t)> ("active", std::bind(&Server::active, &executor, _1, _2));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	sample.callback <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Server::accept, &executor, _1, _2, _3));
	// Выполняем запуск SAMPLE сервер
	sample.start();
	// Выводим результат
	return 0;
}
