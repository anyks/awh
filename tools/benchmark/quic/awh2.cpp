/**
 * @file: awh.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения транспорта QUIC на основе библиотеки AWH —
 *        прогон пары соединений «клиент ↔ сервер» в памяти по синтетическим
 *        часам той же обвязкой замера, что и стенд ngtcp2
 *
 * @details Стенд присутствует не для дублирования собственного набора бенчмарков
 *          библиотеки, а потому, что иначе разница в обвязке замера осталась бы
 *          неотделима от разницы в самих движках транспорта: обе стороны сравнения
 *          проходят через один и тот же учёт выделений памяти и один и тот же
 *          критерий завершения передачи. Установление соединения, транспортные
 *          параметры и криптографический контекст берутся из окружения набора
 *          бенчмарков `benchmark/proto/quic`, поэтому нагрузка совпадает октет в октет
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

/**
 * Подключаем общее окружение стендов сравнения транспорта QUIC
 */
#include "common.hpp"

/**
 * Подключаем окружение набора бенчмарков транспорта QUIC библиотеки AWH
 */
#include "../../../benchmark/proto/quic/quic2.hpp"

/**
 * Подписываемся на пространство имён протокола QUIC
 */
using namespace awh::quic2;

/**
 * @brief Функция прогона передачи данных между клиентом и сервером
 *
 * @note Повторяет сценарий `benchmark/proto/quic/throughput.cpp` библиотеки AWH
 *       октет в октет: те же транспортные параметры, тот же порядок постановки
 *       блоков в очередь, тот же критерий завершения и тот же учёт выделений
 *
 * @param streams количество потоков передачи
 * @param output  итоги прогона передачи
 * @return        результат прогона (false - соединение не установлено)
 *
 */
static bool transfer(const size_t streams, rival::transfer_t & output) noexcept {
	// Объект фреймворка
	static awh::fmk_t fmk;
	// Объект логирования
	static awh::log_t log(&fmk);
	// Отключаем вывод логов: замер не должен перемежаться служебными сообщениями
	log.level(awh::log_t::level_t::NONE);
	// Получаем окружение транспортной безопасности бенчмарка
	auto & security = awh::benchmark::quic2::security();
	// Создаём соединение клиента
	connection_t client(endpoint_t::CLIENT, security.context(endpoint_t::CLIENT), security.coder(), &log);
	// Создаём соединение сервера
	connection_t server(endpoint_t::SERVER, security.context(endpoint_t::SERVER), security.coder(), &log);
	// Выполняем подготовку соединения клиента
	awh::benchmark::quic2::configure(client);
	// Выполняем подготовку соединения сервера
	awh::benchmark::quic2::configure(server);
	// Выполняем начало соединения клиентом
	if(client.connect() != status_t::OK)
		// Выводим отрицательный результат
		return false;
	// Часы бенчмарка в миллисекундах
	uint64_t now = 1000;
	// Выполняем установление соединения
	if(!awh::benchmark::quic2::establish(client, server, now))
		// Выводим отрицательный результат
		return false;
	// Список идентификаторов открытых потоков
	std::vector <uint64_t> identifiers;
	/**
	 * Открываем потоки передачи данных
	 */
	for(size_t i = 0; i < streams; i++){
		// Открываем двунаправленный поток на клиенте
		const uint64_t sid = client.open(false);
		// Если поток не открыт
		if(sid == connection_t::INVALID_STREAM)
			// Выводим отрицательный результат
			return false;
		// Сохраняем идентификатор открытого потока
		identifiers.push_back(sid);
	}
	// Блок данных, ставящийся приложением в очередь отправки
	const std::string block(rival::BLOCK_SIZE, 'x');
	// Объём данных на один поток
	const size_t target = (rival::PAYLOAD_SIZE / streams);
	// Буферы принятых данных по потокам
	std::vector <std::string> received(streams);
	// Отображение идентификатора потока в индекс буфера приёма: без него поиск
	// буфера по идентификатору идёт линейным перебором, и на множестве потоков
	// его квадратичная стоимость приписывалась бы движку, а не обвязке замера
	std::unordered_map <uint64_t, size_t> index;
	// Заполняем отображение идентификаторов потоков в индексы буферов
	for(size_t i = 0; i < streams; i++)
		// Сохраняем соответствие идентификатора потока индексу буфера
		index.emplace(identifiers[i], i);
	// Объёмы поставленных в очередь данных по потокам
	std::vector <size_t> queued(streams, 0);
	/**
	 * Резервируем память под принимаемые данные: рост буфера приёма
	 * к измеряемой стоимости обработки отношения не имеет
	 */
	for(auto & buffer : received)
		// Резервируем память под данные потока
		buffer.reserve(target + rival::BLOCK_SIZE);
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	// Список потоков с собранными данными
	std::vector <uint64_t> ready;
	// Количество принятых октетов
	size_t total = 0;
	// Включаем учёт выделений памяти
	rival::counting(true);
	// Запоминаем момент начала измерения
	const auto start = std::chrono::steady_clock::now();
	/**
	 * Выполняем передачу данных до приёма всего объёма
	 */
	while(total < rival::PAYLOAD_SIZE){
		/**
		 * Ставим очередные блоки данных в очередь отправки потоков
		 */
		for(size_t i = 0; i < streams; i++){
			// Если объём данных потока ещё не поставлен в очередь целиком
			if(queued[i] < target){
				// Вычисляем размер очередного блока данных
				const size_t length = ::std::min(rival::BLOCK_SIZE, target - queued[i]);
				// Ставим блок данных в очередь отправки потока (частичный приём: учитываем принятое)
				queued[i] += client.send(identifiers[i], std::string_view(block.data(), length), false);
			}
		}
		// Флаг передачи хотя бы одной датаграммы на шаге
		bool moved = false;
		/**
		 * Передаём датаграммы клиента серверу
		 */
		while(client.write(datagram, now)){
			// Продвигаем часы бенчмарка
			now += 1;
			// Считаем переданную датаграмму
			output.datagrams++;
			// Устанавливаем флаг передачи датаграммы
			moved = true;
			// Передаём датаграмму серверу
			server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now);
		}
		/**
		 * Выдаём принятые данные приложению: без выдачи лимиты приёма
		 * не продвигаются и передача упирается во flow control
		 */
		server.readable(ready);
		/**
		 * Перебираем список потоков с собранными данными
		 */
		for(auto & sid : ready){
			// Ищем индекс буфера приёма по идентификатору потока
			auto i = index.find(sid);
			// Если идентификатор потока найден
			if(i != index.end()){
				// Запоминаем объём принятых данных потока до выдачи
				const size_t before = received[i->second].size();
				// Флаг завершения потока
				bool fin = false;
				// Выдаём принятые данные приложению
				server.receive(sid, received[i->second], fin);
				// Учитываем принятый объём данных
				total += (received[i->second].size() - before);
			}
		}
		/**
		 * Передаём датаграммы сервера клиенту
		 */
		while(server.write(datagram, now)){
			// Продвигаем часы бенчмарка
			now += 1;
			// Считаем переданную датаграмму
			output.datagrams++;
			// Устанавливаем флаг передачи датаграммы
			moved = true;
			// Передаём датаграмму клиенту
			client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now);
		}
		// Если датаграммы на шаге не передавались
		if(!moved){
			// Продвигаем часы бенчмарка за дедлайн таймеров
			now += 10;
			// Обрабатываем просроченные таймеры клиента
			client.tick(now);
			// Обрабатываем просроченные таймеры сервера
			server.tick(now);
		}
	}
	// Запоминаем момент окончания измерения
	const auto finish = std::chrono::steady_clock::now();
	// Отключаем учёт выделений памяти
	rival::counting(false);
	// Устанавливаем количество принятых октетов
	output.received = total;
	// Устанавливаем затраченное время
	output.seconds = std::chrono::duration <double> (finish - start).count();
	// Устанавливаем количество выполненных выделений памяти
	output.allocations = rival::counter::count;
	// Устанавливаем объём выделенной памяти
	output.bytes = rival::counter::bytes;
	// Выводим положительный результат
	return true;
}
/**
 * @brief Функция выполнения сценария передачи данных
 *
 * @param name    название сценария
 * @param streams количество потоков передачи
 * @param metric  измеряемая характеристика (true - выделения, false - пропускная способность)
 * @param mask    фильтр названий сценариев
 *
 */
static void execute(const char * name, const size_t streams, const bool metric, const char * mask) noexcept {
	// Если название сценария не соответствует фильтру
	if(!rival::selected(name, mask))
		// Выходим без выполнения сценария
		return;
	// Итоги прогона передачи данных
	rival::transfer_t transfer;
	// Если прогон передачи данных не выполнен
	if(!::transfer(streams, transfer)){
		// Выводим сообщение о неудачном прогоне сценария
		rival::skip(name, "прогон не выполнен: соединение не установлено");
		// Выходим из сценария
		return;
	}
	// Если измеряется количество выделений памяти
	if(metric)
		// Выводим результат прогона выделений памяти
		rival::allocations(name, transfer);
	// Если измеряется пропускная способность
	else rival::throughput(name, transfer);
}
/**
 * @brief Точка входа стенда сравнения транспорта QUIC на основе библиотеки AWH
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код возврата
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Получаем фильтр названий сценариев
	const char * mask = rival::filter(argc, argv);
	// Выполняем сценарий передачи по одному потоку
	execute("quic/throughput/single-stream", 1, false, mask);
	// Выполняем сценарий передачи по множеству потоков
	execute("quic/throughput/multi-stream", rival::STREAM_COUNT, false, mask);
	// Выполняем сценарий количества выделений памяти на датаграмму
	execute("quic/allocations/per-datagram", 1, true, mask);
	// Выводим успешный код возврата
	return 0;
}

/**
 * @brief Оператор выделения памяти с учётом статистики
 *
 * @note Оператор подменяется на уровне программы. Стенд собирается без аллокатора
 *       TcMalloc, поэтому учёт ведётся штатными операторами языка, как и стенд
 *       ngtcp2 ведёт его штатным распределителем: сравниваются выделения самих
 *       движков транспорта на одном и том же системном распределителе
 *
 * @param size размер выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new (size_t size){
	// Учитываем выполненное выделение памяти
	rival::account(size);
	// Выполняем выделение памяти
	void * result = ::malloc(size > 0 ? size : 1);
	// Если память не выделена
	if(result == nullptr)
		// Завершаем приложение - обработка нехватки памяти в стенде не предусмотрена
		::abort();
	// Выводим указатель на выделенную память
	return result;
}
/**
 * @brief Оператор выделения памяти под массив с учётом статистики
 *
 * @param size размер выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new [] (size_t size){
	// Выполняем выделение памяти
	return operator new (size);
}
/**
 * @brief Оператор освобождения памяти
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти массива
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти с указанием размера
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти массива с указанием размера
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
