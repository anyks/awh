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
		// Объект логирования
		log_t * _log;
	public:
		/**
		 * active Метод идентификации активности на клиенте
		 * @param mode   режим события подключения
		 * @param sample объект client-а
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
		 * @param sample объект client-а
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
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект исполнителя
	Client executor(&log);
	// Создаём биндинг
	client::core_t core(awh::core_t::affiliation_t::PRIMARY, &fmk, &log);
	// Создаём объект SAMPLE запроса
	client::sample_t sample(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.name("SAMPLE Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем отложенные вызовы
	 * 2. Устанавливаем ожидание входящих сообщений
	 * 3. Устанавливаем валидацию SSL сертификата
	 */
	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOINFO |
		(uint8_t) client::sample_t::flag_t::WAITMESS |
		(uint8_t) client::sample_t::flag_t::VERIFYSSL
	);
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем адрес сертификата
	core.ca("./ca/cert.pem");
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
	// Устанавливаем длительное подключение
	// ws.keepAlive(100, 30, 10);
	// Подключаем сертификаты
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");
	// Отключаем валидацию сертификата
	core.verifySSL(false);
	// Подписываемся на событие коннекта и дисконнекта клиента
	sample.on(bind(&Client::active, &executor, _1, _2));
	// Подписываемся на событие получения сообщения
	sample.on(bind(&Client::message, &executor, _1, _2));
	// Выполняем инициализацию подключения
	sample.init(2222, "127.0.0.1");
	// sample.init("anyks");
	// Выполняем запуск работы клиента
	sample.start();
	// Выводим результат
	return 0;
}
