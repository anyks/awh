/**
 * @file: throughput.cpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <string>
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков протокола QUIC
 */
#include "quic.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён протокола QUIC
 */
using namespace awh::quic;

/**
 * @brief Внутренние параметры и сценарии бенчмарков пропускной способности
 *
 */
namespace {
	/**
	 * @brief Объём передаваемых данных сценария в октетах
	 *
	 */
	static constexpr size_t PAYLOAD_SIZE = (64 * 1024 * 1024);
	/**
	 * @brief Размер блока, ставящегося приложением в очередь отправки
	 *
	 */
	static constexpr size_t BLOCK_SIZE = (64 * 1024);
	/**
	 * @brief Количество потоков сценария передачи по множеству потоков
	 *
	 */
	static constexpr size_t STREAM_COUNT = 64;
	/**
	 * @brief Порог пропускной способности передачи по одному потоку в мегабайтах в секунду
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки:
	 *          отладочная сборка библиотеки медленнее оптимизированной более чем
	 *          вдвое. Поэтому порог откалиброван по отладочной сборке с четырёхкратным
	 *          запасом - он ловит регрессии на порядок, а не колебания окружения.
	 *          Показательный пример такой регрессии: вырезание упакованных данных
	 *          из начала буфера отправки вместо продвижения курсора даёт квадратичную
	 *          стоимость и роняет показатель более чем в двадцать раз
	 */
	static constexpr double SINGLE_STREAM_THRESHOLD = 150.0;
	/**
	 * @brief Порог пропускной способности передачи по множеству потоков
	 *
	 */
	static constexpr double MULTI_STREAM_THRESHOLD = 80.0;
	/**
	 * @brief Порог количества выделений памяти на одну датаграмму
	 *
	 * @details Ограничение сверху: рост числа выделений на датаграмму означает
	 *          появление попакетного выделения в горячем пути. В отличие от
	 *          пропускной способности показатель от машины и режима сборки
	 *          не зависит, поэтому порог задан вплотную к измеренному значению
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 4.0;

	/**
	 * @brief Структура итогов прогона передачи данных
	 *
	 */
	typedef struct Transfer {
		// Количество переданных датаграмм
		size_t datagrams;
		// Количество принятых октетов
		size_t received;
		// Затраченное время в секундах
		double seconds;
		// Количество выполненных выделений памяти
		size_t allocations;
		// Суммарный объём выделенной памяти в октетах
		size_t bytes;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Transfer() noexcept :
		 datagrams(0), received(0), seconds(0.0), allocations(0), bytes(0) {}
	} transfer_t;

	/**
	 * @brief Функция прогона передачи данных между клиентом и сервером
	 *
	 * @param streams количество потоков передачи
	 * @param output  итоги прогона передачи
	 * @return        результат прогона (false - соединение не установлено)
	 */
	static bool transfer(const size_t streams, transfer_t & output) noexcept {
		// Объект фреймворка
		static awh::fmk_t fmk;
		// Объект логирования
		static awh::log_t log(&fmk);
		// Получаем окружение транспортной безопасности бенчмарка
		auto & security = awh::benchmark::quic::security();
		// Создаём соединение клиента
		connection_t client(endpoint_t::CLIENT, security.context(endpoint_t::CLIENT), security.coder(), &log);
		// Создаём соединение сервера
		connection_t server(endpoint_t::SERVER, security.context(endpoint_t::SERVER), security.coder(), &log);
		// Выполняем подготовку соединения клиента
		awh::benchmark::quic::configure(client);
		// Выполняем подготовку соединения сервера
		awh::benchmark::quic::configure(server);
		// Выполняем начало соединения клиентом
		if(client.connect() != status_t::OK)
			// Выводим отрицательный результат
			return false;
		// Часы бенчмарка в миллисекундах
		uint64_t now = 1000;
		// Выполняем установление соединения
		if(!awh::benchmark::quic::establish(client, server, now))
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
		const std::string block(BLOCK_SIZE, 'x');
		// Объём данных на один поток
		const size_t target = (PAYLOAD_SIZE / streams);
		// Буферы принятых данных по потокам
		std::vector <std::string> received(streams);
		// Объёмы поставленных в очередь данных по потокам
		std::vector <size_t> queued(streams, 0);
		/**
		 * Резервируем память под принимаемые данные: рост буфера приёма
		 * к измеряемой стоимости обработки отношения не имеет
		 */
		for(auto & buffer : received)
			// Резервируем память под данные потока
			buffer.reserve(target + BLOCK_SIZE);
		// Буфер передаваемой датаграммы
		std::string datagram = "";
		// Список потоков с собранными данными
		std::vector <uint64_t> ready;
		// Количество принятых октетов
		size_t total = 0;
		// Включаем учёт выделений памяти
		awh::benchmark::quic::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем передачу данных до приёма всего объёма
		 */
		while(total < PAYLOAD_SIZE){
			/**
			 * Ставим очередные блоки данных в очередь отправки потоков
			 */
			for(size_t i = 0; i < streams; i++){
				// Если объём данных потока ещё не поставлен в очередь целиком
				if(queued[i] < target){
					// Вычисляем размер очередного блока данных
					const size_t length = ::min(BLOCK_SIZE, target - queued[i]);
					// Ставим блок данных в очередь отправки потока
					if(client.send(identifiers[i], std::string_view(block.data(), length), false) == status_t::OK)
						// Учитываем поставленный в очередь объём данных
						queued[i] += length;
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
				/**
				 * Перебираем список идентификаторов открытых потоков
				 */
				for(size_t i = 0; i < streams; i++){
					// Если идентификатор потока найден
					if(identifiers[i] == sid){
						// Запоминаем объём принятых данных потока до выдачи
						const size_t before = received[i].size();
						// Флаг завершения потока
						bool fin = false;
						// Выдаём принятые данные приложению
						server.receive(sid, received[i], fin);
						// Учитываем принятый объём данных
						total += (received[i].size() - before);
						// Прекращаем поиск идентификатора потока
						break;
					}
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
		awh::benchmark::quic::counting(false);
		// Получаем статистику выделений памяти
		awh::benchmark::quic::allocations(output.allocations, output.bytes);
		// Устанавливаем количество принятых октетов
		output.received = total;
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция формирования результата измерения пропускной способности
	 *
	 * @param streams количество потоков передачи
	 * @return        результат измерения
	 */
	static awh::benchmark::result_t throughput(const size_t streams) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона передачи данных
		transfer_t transfer;
		// Выполняем прогон передачи данных
		if(!::transfer(streams, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: соединение не установлено";
			// Выводим результат измерения
			return result;
		}
		// Объём переданных данных в мегабайтах
		const double megabytes = (static_cast <double> (transfer.received) / (1024.0 * 1024.0));
		// Вычисляем пропускную способность в мегабайтах в секунду
		result.value = ((transfer.seconds > 0.0) ? (megabytes / transfer.seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details),
			"датаграмм: %zu, выделений: %zu (%.2f на датаграмму), выделено: %.1f МБ (%.2f× от переданного)",
			transfer.datagrams, transfer.allocations,
			(transfer.datagrams > 0 ? (static_cast <double> (transfer.allocations) / static_cast <double> (transfer.datagrams)) : 0.0),
			(static_cast <double> (transfer.bytes) / (1024.0 * 1024.0)),
			(transfer.received > 0 ? (static_cast <double> (transfer.bytes) / static_cast <double> (transfer.received)) : 0.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения количества выделений памяти на датаграмму
	 *
	 * @return результат измерения
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона передачи данных
		transfer_t transfer;
		// Выполняем прогон передачи данных по одному потоку
		if(!::transfer(1, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: соединение не установлено";
			// Устанавливаем заведомо превышающее порог значение
			result.value = 1000.0;
			// Выводим результат измерения
			return result;
		}
		// Вычисляем количество выделений памяти на датаграмму
		result.value = ((transfer.datagrams > 0) ? (static_cast <double> (transfer.allocations) / static_cast <double> (transfer.datagrams)) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "выделено: %.1f МБ (%.2f× от переданного)",
			(static_cast <double> (transfer.bytes) / (1024.0 * 1024.0)),
			(transfer.received > 0 ? (static_cast <double> (transfer.bytes) / static_cast <double> (transfer.received)) : 0.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий передачи по одному потоку
	static const bool gSingle = awh::benchmark::add(
		"quic/throughput/single-stream", "МБ/с", SINGLE_STREAM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::throughput(1); }
	);
	// Регистрируем сценарий передачи по множеству потоков
	static const bool gMulti = awh::benchmark::add(
		"quic/throughput/multi-stream", "МБ/с", MULTI_STREAM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::throughput(STREAM_COUNT); }
	);
	// Регистрируем сценарий количества выделений памяти на датаграмму
	static const bool gAllocations = awh::benchmark::add(
		"quic/allocations/per-datagram", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
};
