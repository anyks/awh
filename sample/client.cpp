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
	private:
		// Объект основного модуля
		client::sample_t * _sample;
	public:
		/**
		 * active Метод идентификации активности на клиенте
		 * @param mode режим события подключения
		 */
		void active(const client::sample_t::mode_t mode){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
			// Если подключение выполнено
			if(mode == client::sample_t::mode_t::CONNECT){
				// Создаём текст сообщения для сервера
				const string message = "Hello World!!!";
				// Выполняем отправку сообщения серверу
				this->_sample->send(message.data(), message.size());
			}
		}
		/**
		 * message Метод получения сообщений
		 * @param buffer буфер входящих данных
		 */
		void message(const vector <char> & buffer){
			// Получаем сообщение
			const string message(buffer.begin(), buffer.end());
			// Выводим информацию в лог
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			// Останавливаем работу модуля
			this->_sample->stop();
		}
	public:
		/**
		 * Client Конструктор
		 * @param fmk    объект фреймворка
		 * @param log    объект логирования
		 * @param sample объект рабочего модуля
		 */
		Client(const fmk_t * fmk, const log_t * log, client::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
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
	client::core_t core(&dns, &fmk, &log);
	// Создаём объект SAMPLE запроса
	client::sample_t sample(&core, &fmk, &log);
	// Создаём объект исполнителя
	Client executor(&fmk, &log, &sample);
	// Устанавливаем название сервиса
	log.name("SAMPLE Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем отложенные вызовы
	 * 2. Устанавливаем ожидание входящих сообщений
	 * 3. Устанавливаем валидацию SSL сертификата
	 */
	sample.mode({
		// client::sample_t::flag_t::NOT_INFO,
		client::sample_t::flag_t::WAIT_MESS,
		// client::sample_t::flag_t::VERIFY_SSL
	});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
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
	// Устанавливаем длительное подключение
	// ws.keepAlive(100, 30, 10);
	// Подключаем сертификаты
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");
	// Отключаем валидацию сертификата
	core.verifySSL(false);
	// Создаём локальный контейнер функций обратного вызова
	fn_t callback(&log);
	// Подписываемся на событие получения сообщения
	callback.set <void (const vector <char> &)> ("message", std::bind(&Client::message, &executor, _1));
	// Подписываемся на событие коннекта и дисконнекта клиента
	callback.set <void (const client::sample_t::mode_t)> ("active", std::bind(&Client::active, &executor, _1));
	// Выполняем установку функций обратного вызова
	sample.callback(std::move(callback));
	// Выполняем инициализацию подключения
	sample.init(2222, "127.0.0.1");
	// sample.init("anyks");
	// Выполняем запуск работы клиента
	sample.start();
	// Выводим результат
	return 0;
}
