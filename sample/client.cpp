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
#include <client/sample.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * Client Класс объекта исполнителя
 */
class Client {
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	public:
		/**
		 * active Метод идентификации активности на клиенте
		 * @param mode   режим события подключения
		 * @param sample объект активного клиента
		 */
		void active(const client::sample_t::mode_t mode, client::sample_t * sample){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
			// Если подключение выполнено
			if(mode == client::sample_t::mode_t::CONNECT){
				// Создаём текст сообщения для сервера
				const string message = "Hello World!!!";
				// Выполняем отправку сообщения серверу
				sample->send(message.data(), message.size());
			}
		}
		/**
		 * message Метод получения сообщений
		 * @param buffer буфер входящих данных
		 * @param sample объект активного клиента
		 */
		void message(const vector <char> & buffer, client::sample_t * sample){
			// Получаем сообщение
			const string message(buffer.begin(), buffer.end());
			// Выводим информацию в лог
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			// Останавливаем работу модуля
			sample->stop();
		}
	public:
		/**
		 * Client Конструктор
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 */
		Client(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект параметров SSL-шифрования
	node_t::ssl_t ssl;
	// Объект DNS-резолвера
	dns_t dns(&fmk, &log);
	// Создаём биндинг
	client::core_t core(&dns, &fmk, &log);
	// Создаём объект SAMPLE запроса
	client::sample_t sample(&core, &fmk, &log);
	// Создаём объект исполнителя
	Client executor(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("SAMPLE Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * Запрет вывода информационных сообщений
	 */
	// sample.mode({client::sample_t::flag_t::NOT_INFO});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Отключаем валидацию сертификата
	ssl.verify = false;
	// Устанавливаем SSL сертификаты сервера
	ssl.key  = "./certs/certificates/client-key.pem";
	ssl.cert = "./certs/certificates/client-cert.pem";
	// Выполняем установку параметров SSL-шифрования
	core.ssl(ssl);
	// Устанавливаем тип сокета unix-сокет
	// core.family(awh::scheme_t::family_t::NIX);
	// Устанавливаем тип сокета
	core.sonet(awh::scheme_t::sonet_t::DTLS);
	// core.sonet(awh::scheme_t::sonet_t::TLS);
	// core.sonet(awh::scheme_t::sonet_t::UDP);
	// core.sonet(awh::scheme_t::sonet_t::TCP);
	// core.sonet(awh::scheme_t::sonet_t::SCTP);
	// Устанавливаем длительное подключение
	// ws.keepAlive(100, 30, 10);
	// Устанавливаем таймеры ожидания по одной секунде на чтение и запись
	sample.waitTimeDetect(1, 1);
	// Подписываемся на событие получения сообщения
	sample.callback <void (const vector <char> &)> ("message", std::bind(&Client::message, &executor, _1, &sample));
	// Подписываемся на событие коннекта и дисконнекта клиента
	sample.callback <void (const client::sample_t::mode_t)> ("active", std::bind(&Client::active, &executor, _1, &sample));
	// Выполняем инициализацию подключения
	sample.init(2222, "127.0.0.1");
	// sample.init("anyks");
	// Выполняем запуск работы клиента
	sample.start();
	// Выводим результат
	return 0;
}
