/**
 * @file fiber.cpp
 * @date 2026-08-26
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Пример работы волокон — асинхронный обмен, поданный синхронным видом:
 *        запрос отправляется, волокно засыпает, цикл событий продолжает работу,
 *        отклик будит волокно, и вызов возвращает ответ обычным return
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fiber.hpp>
#include <net/io.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Класс объекта исполнителя
 *
 */
class Executor {
	private:
		// Объект работы с логами
		const log_t * _log;
		// Объект асинхронного движка ввода-вывода
		engine::io_t * _io;
		// Идентификатор события клиента
		event::id_t _client;
		// Волокно, в котором идут синхронные на вид обмены
		fiber::ctx_t * _fiber;
	private:
		// Ответ, полученный на последний запрос
		std::string _answer;
		// Признак завершения всей работы
		bool _done;
	public:
		/**
		 * @brief Метод проверки завершения работы
		 *
		 * @return результат проверки
		 *
		 */
		bool done() const noexcept {
			// Выводим признак завершения работы
			return this->_done;
		}
	public:
		/**
		 * @brief Метод синхронного на вид обмена с сервером
		 *
		 * @details Внутри - обычная последовательность: отправили, получили, вернули.
		 *          Ожидание при этом НЕ блокирует ни поток, ни цикл опроса: волокно
		 *          засыпает на своём стеке, а цикл всё это время обслуживает прочие узлы
		 *
		 * @param message сообщение для отправки серверу
		 * @return        ответ сервера
		 *
		 */
		std::string request(const std::string & message) noexcept {
			// Сбрасываем прежний ответ
			this->_answer.clear();
			// Отправляем запрос серверу
			if(this->_io->send(this->_client, message.data(), message.size()) == 0){
				// Записываем ошибку в лог
				this->_log->print("Запрос отправить не удалось", log_t::flag_t::CRITICAL);
				// Выводим пустой ответ
				return "";
			}
			/**
			 * Отдаём управление циклу событий
			 *
			 * @note Кадр этого вызова со всеми его переменными остаётся жив: он лежит
			 *       на стеке волокна, а не на стеке цикла
			 */
			fiber::yield();
			// Сюда управление попадает уже с ответом
			return this->_answer;
		}
		/**
		 * @brief Метод обработки события чтения данных
		 *
		 * @param eid  идентификатор события
		 * @param data бинарный буфер данных
		 * @param size размер данных
		 *
		 */
		void read([[maybe_unused]] const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
			// Запоминаем полученный ответ
			this->_answer.assign(reinterpret_cast <const char *> (data), size);
			/**
			 * Будим волокно: управление вернётся в request() сразу за yield()
			 *
			 * @warning Правило одного направления: этот отклик идёт на стеке цикла и
			 *          засыпать сам НЕ ВПРАВЕ - ему позволено только будить
			 */
			fiber::resume(this->_fiber);
		}
		/**
		 * @brief Метод обработки события подключения к серверу
		 *
		 * @param ok результат подключения
		 *
		 */
		void connect(const bool ok) noexcept {
			// Если подключение не выполнено
			if(!ok){
				// Записываем ошибку в лог
				this->_log->print("Подключиться к серверу не удалось", log_t::flag_t::CRITICAL);
				// Выходим из функции обработки
				return;
			}
			/**
			 * Заводим волокно и запускаем в нём пользовательский код
			 *
			 * @note Пользовательский код обязан идти в волокне: усыпить можно лишь то,
			 *       что на своём стеке. Прямо со стека цикла усыплять нечего
			 */
			this->_fiber = fiber::spawn([this]() noexcept -> void {
				// Выполняем первый обмен
				const std::string first = this->request("SELECT 1");
				// Записываем ответ в лог
				this->_log->print("Первый ответ: %s", log_t::flag_t::INFO, first.c_str());
				// Выполняем второй обмен, опираясь на итог первого
				const std::string second = this->request("SELECT " + std::to_string(first.size()));
				// Записываем ответ в лог
				this->_log->print("Второй ответ: %s", log_t::flag_t::INFO, second.c_str());
				// Записываем в лог сообщение о завершении обменов
				this->_log->print("Оба обмена выполнены последовательно, цикл при этом не стоял", log_t::flag_t::INFO);
				// Отмечаем работу выполненной
				this->_done = true;
			}, this->_log);
			// Запускаем волокно
			fiber::resume(this->_fiber);
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 * @param io     объект асинхронного движка ввода-вывода
		 * @param client идентификатор события клиента
		 * @param log    объект работы с логами
		 *
		 */
		Executor(engine::io_t * io, const event::id_t client, const log_t * log) noexcept :
		 _log(log), _io(io), _client(client), _fiber(nullptr), _answer{""}, _done(false) {}
		/**
		 * @brief Деструктор
		 *
		 */
		~Executor() noexcept {
			// Уничтожаем волокно, если оно доработало
			fiber::destroy(this->_fiber);
		}
};

/**
 * @brief Главная функция приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Порт, на котором работает встроенный эхо-сервер образца
	constexpr uint16_t PORT = 12345;
	// Набор опций событий
	constexpr uint16_t OPTIONS = (
		event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR |
		event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY
	);
	// Добавляем событие сервера
	const event::id_t server = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	// Добавляем событие клиента
	const event::id_t client = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	// Устанавливаем порт события сервера
	io.setSourcePort(server, PORT);
	// Устанавливаем порт назначения события клиента
	io.setTargetPort(client, PORT);
	// Инициализируем движок
	if(!io.initialize()){
		// Записываем ошибку в лог
		log.print("Движок инициализировать не удалось", log_t::flag_t::CRITICAL);
		// Выходим из приложения
		return EXIT_FAILURE;
	}
	// Устанавливаем опции события сервера
	io.setOptions(server, OPTIONS);
	// Устанавливаем опции события клиента
	io.setOptions(client, OPTIONS);
	// Устанавливаем адрес события сервера
	io.setAddress(server, event::address_t::IPV4, "127.0.0.1");
	/**
	 * Заводим встроенный эхо-сервер: он возвращает принятое обратно
	 *
	 * @note Сервер нужен образцу лишь затем, чтобы обмен был настоящим. Всё
	 *       существенное происходит на стороне клиента, в волокне
	 */
	io.on(server, static_cast <engine::callback::accept_t> ([&io, &log]([[maybe_unused]] const event::id_t sid, const event::id_t cid) noexcept -> void {
		// Записываем в лог сообщение о принятом подключении
		log.print("Сервер принял подключение", log_t::flag_t::INFO);
		// Устанавливаем функцию обратного вызова на чтение данных сервером
		io.on(cid, [&io](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Возвращаем принятое обратно отправителю
			io.send(eid, data, size);
		});
	}));
	// Выполняем фиксацию настроек события сервера
	io.commit(server);
	// Переводим событие сервера в режим прослушивания
	io.listen(server, 16);
	// Запускаем событие сервера
	io.launch(server);
	// Создаём объект исполнителя
	Executor executor(&io, client, &log);
	// Устанавливаем адрес назначения события клиента
	io.setTarget(client, "127.0.0.1");
	// Устанавливаем функцию обратного вызова на чтение данных клиентом
	io.on(client, [&executor](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		// Выполняем обработку чтения данных
		executor.read(eid, data, size);
	});
	// Устанавливаем функцию обратного вызова на подключение к серверу
	io.on(client, static_cast <engine::callback::connect_t> ([&executor]([[maybe_unused]] const event::id_t eid, const bool ok) noexcept -> void {
		// Выполняем обработку подключения
		executor.connect(ok);
	}));
	// Выполняем фиксацию настроек события клиента
	io.commit(client);
	// Выполняем подключение к серверу
	io.connect(client);
	// Запускаем событие клиента
	io.launch(client);
	/**
	 * Крутим цикл опроса событий
	 *
	 * @note Цикл здесь самый обычный, ничем не отличающийся от любого другого.
	 *       Волокна ему не требуют НИЧЕГО: ни вложенных вызовов, ни потоков
	 */
	while(!executor.done() && io.poll(100));
	// Уничтожаем все события
	io.deinitialize();
	// Выводим результат
	return EXIT_SUCCESS;
}
