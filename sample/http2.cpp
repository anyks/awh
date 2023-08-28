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
#include <client/http2.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * Client Класс объекта исполнителя
 */
class Client {
	private:
		// Объект логирования
		log_t * _log;
	public:
		/**
		 * active Метод идентификации активности на клиенте
		 * @param mode  режим события подключения
		 * @param http2 объект client-а
		 */
		void active(const client::http2_t::mode_t mode, client::http2_t * http2){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::http2_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
			// Если подключение выполнено
			if(mode == client::http2_t::mode_t::CONNECT){
				/*
				// Создаём текст сообщения для сервера
				const string message = "Hello World!!!";
				// Выполняем отправку сообщения серверу
				http2->send(message.data(), message.size());
				*/
			}
		}
		/**
		 * message Метод получения сообщений
		 * @param buffer буфер входящих данных
		 * @param http2  объект client-а
		 */
		void message(const vector <char> & buffer, client::http2_t * http2){
			// Получаем сообщение
			const string message(buffer.begin(), buffer.end());
			// Выводим информацию в лог
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			// Останавливаем работу модуля
			http2->stop();
		}
	public:
		/**
		 * Client Конструктор
		 * @param log объект логирования
		 */
		Client(log_t * log) : _log(log) {}
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
	// Создаём объект исполнителя
	Client executor(&log);
	// Создаём биндинг
	client::core_t core(&fmk, &log);
	// Создаём объект HTTP/2 запроса
	client::http2_t http2(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.name("HTTTP2 Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем отложенные вызовы
	 * 2. Устанавливаем ожидание входящих сообщений
	 * 3. Устанавливаем валидацию SSL сертификата
	 */
	http2.mode(
		// (uint8_t) client::http2_t::flag_t::NOT_INFO |
		(uint8_t) client::http2_t::flag_t::WAIT_MESS |
		(uint8_t) client::http2_t::flag_t::VERIFY_SSL
	);
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем адрес сертификата
	core.ca("./ca/cert.pem");
	// Устанавливаем тип сокета UDP TLS
	// core.sonet(awh::scheme_t::sonet_t::DTLS);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	// core.sonet(awh::scheme_t::sonet_t::UDP);
	// core.sonet(awh::scheme_t::sonet_t::TCP);
	// core.sonet(awh::scheme_t::sonet_t::SCTP);
	// Устанавливаем длительное подключение
	// ws.keepAlive(100, 30, 10);
	// Подключаем сертификаты
	// core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");
	// Отключаем валидацию сертификата
	core.verifySSL(false);
	// Подписываемся на событие коннекта и дисконнекта клиента
	http2.on(bind(&Client::active, &executor, _1, _2));
	// Подписываемся на событие получения сообщения
	http2.on(bind(&Client::message, &executor, _1, _2));
	// Выполняем запуск работы клиента
	http2.start();
	// Выводим результат
	return 0;
}
