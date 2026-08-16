/**
 * @file transfer-server.cpp
 * @date 2026-08-16
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
 * @brief Принимающая сторона проверки переноса файла по SCTP между системами
 *
 * @details Принимает подключение, считает всё пришедшее и по закрытии подключения
 *          выводит итог: количество октетов, контрольную сумму и число закрытых
 *          записей. Ничего не отвечает отправителю намеренно - встречный поток
 *          менял бы поведение очереди отправки, а проверяется именно она
 *
 *          Сличение сторон делается по выводу: отправитель печатает те же три
 *          числа о своём файле, и совпасть обязаны все три
 *
 * @par Запуск
 * @code
 *   transfer-server <STREAM|SEQPACKET> [порт] [адрес]
 * @endcode
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <string>
#include <cstdlib>
#include <cstring>
#include <iostream>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>

/**
 * Если операционная система несёт протокол SCTP
 *
 * @note Сторож тот же, каким объявлен и сам фасад протокола в io.hpp: на прочих
 *       системах средство собирается, но лишь сообщает о невозможности работы -
 *       молчаливое исключение из сборки выглядело бы пропущенной проверкой
 */
#if __linux__ || __FreeBSD__ || __sun

/**
 * Используем пространства имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Учёт принятого по одному подключению
 *
 */
typedef struct Counter {
	// Количество принятых октетов
	size_t bytes;
	// Количество закрытых записей
	size_t records;
	// Контрольная сумма принятых октетов
	uint64_t hash;
	/**
	 * @brief Конструктор
	 *
	 */
	Counter() noexcept : bytes(0), records(0), hash(0xCBF29CE484222325ULL) {}
} counter_t;

// Учёт принятого по всем подключениям
static map <event::id_t, counter_t> __counters__;

/**
 * @brief Функция досчёта контрольной суммы
 *
 * @details Считается FNV-1a: она проста, не требует таблиц и одинакова на всех
 *          системах и наборах команд, а значит годится доводом о равенстве
 *          отправленного и принятого
 *
 * @param hash   текущее значение контрольной суммы
 * @param buffer буфер данных
 * @param size   размер буфера данных
 * @return       новое значение контрольной суммы
 *
 */
static uint64_t digest(uint64_t hash, const uint8_t * buffer, const size_t size) noexcept {
	/**
	 * Перебираем все октеты буфера данных
	 */
	for(size_t i = 0; i < size; i++){
		// Подмешиваем очередной октет
		hash ^= static_cast <uint64_t> (buffer[i]);
		// Домножаем на простое число FNV
		hash *= 0x100000001B3ULL;
	}
	// Выводим новое значение контрольной суммы
	return hash;
}

/**
 * @brief Функция вывода итога по подключению
 *
 * @param log объект работы с логами
 * @param eid идентификатор события
 *
 */
static void report(const log_t & log, const event::id_t eid) noexcept {
	// Выполняем поиск учёта по подключению
	auto i = __counters__.find(eid);
	// Если учёт по подключению не ведётся, выводить нечего
	if(i == __counters__.end())
		// Выходим из функции
		return;
	// Выводим итог приёма
	log.print("ИТОГ: подключение=%u октетов=%zu записей=%zu сумма=%016llX", log_t::flag_t::INFO,
		eid, i->second.bytes, i->second.records, static_cast <unsigned long long> (i->second.hash));
	// Снимаем учёт по завершённому подключению
	__counters__.erase(i);
}

/**
 * @brief Исполняемая функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Если вид сокета не назван
	if(argc < 2){
		// Выводим порядок запуска
		cout << " Запуск: " << argv[0] << " <STREAM|SEQPACKET> [порт] [адрес]" << endl;
		// Выходим с ошибкой
		return EXIT_FAILURE;
	}
	// Определяем вид сокета
	const bool stream = (::strcasecmp(argv[1], "SEQPACKET") != 0);
	// Определяем порт прослушивания
	const uint16_t port = static_cast <uint16_t> ((argc > 2) ? ::atoi(argv[2]) : 2222);
	// Определяем адрес прослушивания
	const string address = ((argc > 3) ? argv[3] : "0.0.0.0");
	// Объект фреймворка
	fmk_t fmk;
	// Объект работы с логами
	log_t log(&fmk);
	// Устанавливаем объект работы с логами
	fmk.setLogger(&log);
	// Объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Объект работы с протоколом SCTP
	engine::sctp_t sctp(&fmk, &log);
	// Создаём событие сервера
	event::id_t eid = io.event(
		event::node_t::SERVER, event::family_t::IPV4,
		(stream ? event::type_t::STREAM : event::type_t::SEQPACKET),
		event::protocol_t::SCTP
	);
	// Устанавливаем порт прослушивания
	io.setSourcePort(eid, port);
	// Если завести движок не удалось
	if(!io.initialize()){
		// Выводим сообщение об ошибке
		log.print("Движок завести не удалось", log_t::flag_t::CRITICAL);
		// Выходим с ошибкой
		return EXIT_FAILURE;
	}
	// Устанавливаем опции события сервера
	io.setOptions(eid,
		event::options::NO_SIGILL | event::options::NO_SIGPIPE |
		event::options::REUSE_ADDR | event::options::REUSE_PORT |
		event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC
	);
	/**
	 * Подписываемся на события протокола
	 *
	 * @note Подписка нужна не ради самих событий, а ради сведений о сообщении: у части
	 *       систем метаданные приходят только по ней
	 */
	sctp.eventsSubscribe(eid, {
		net::sctp::event_type_t::DATA_IO,
		net::sctp::event_type_t::ASSOC_CHANGE,
		net::sctp::event_type_t::SHUTDOWN_EVENT
	});
	// Если установить адрес прослушивания не удалось
	if(!io.setAddress(eid, event::address_t::IPV4, address)){
		// Выводим сообщение об ошибке
		log.print("Адрес прослушивания установить не удалось: %s", log_t::flag_t::CRITICAL, address.c_str());
		// Выходим с ошибкой
		return EXIT_FAILURE;
	}
	// Устанавливаем отклик принятия подключения
	io.on(eid, static_cast <engine::callback::accept_t> ([&io, &sctp, &log](const event::id_t sid, const event::id_t cid) noexcept -> void {
		// Выводим сообщение о принятом подключении
		log.print("Принято подключение: %u", log_t::flag_t::INFO, cid);
		// Заводим учёт по принятому подключению
		__counters__.emplace(cid, counter_t());
		// Устанавливаем опции принятого подключения
		io.setOptions(cid,
			event::options::NO_SIGILL | event::options::NO_SIGPIPE |
			event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC
		);
		/**
		 * Устанавливаем отклик чтения данных вместе с метаданными
		 *
		 * @note Отклик этот взят намеренно вместо общего: только он приносит признак
		 *       границы записи, а без него число записей не сосчитать
		 */
		sctp.on(cid, static_cast <engine::callback::sctp::message_t> ([&log](const event::id_t eid, const uint8_t * data, const size_t size, const net::sctp::rinfo_t & info) noexcept -> void {
			// Выполняем поиск учёта по подключению
			auto i = __counters__.find(eid);
			// Если учёт по подключению не ведётся
			if(i == __counters__.end())
				// Выходим из функции
				return;
			// Если получено известие протокола, а не данные - в счёт оно не идёт
			if(info.flags.find(net::sctp::receipt_t::NOTIFICATION) != info.flags.end())
				// Выходим из функции
				return;
			// Досчитываем контрольную сумму принятого
			i->second.hash = digest(i->second.hash, data, size);
			// Увеличиваем количество принятых октетов
			i->second.bytes += size;
			// Если запись закрыта границей
			if(info.flags.find(net::sctp::receipt_t::END_OF_RECORD) != info.flags.end())
				// Увеличиваем количество закрытых записей
				i->second.records++;
		}));
		// Устанавливаем отклик состояния принятого подключения
		io.on(cid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
			/**
			 * Определяем состояние события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если подключение закрыто либо подлежит уничтожению
				case static_cast <uint8_t> (event::status_t::DESTROYED):
				case static_cast <uint8_t> (event::status_t::GARBAGE):
					// Выводим итог приёма
					report(log, eid);
				break;
			}
		});
		// Устанавливаем отклик действий принятого подключения
		io.on(cid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
			/**
			 * Определяем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если подключение закрыто
				case static_cast <uint8_t> (event::action_t::CLOSE):
				case static_cast <uint8_t> (event::action_t::DISCONNECT):
					// Выводим итог приёма
					report(log, eid);
				break;
			}
		});
	}));
	// Если зафиксировать настройки события не удалось
	if(!io.commit(eid)){
		// Выводим сообщение об ошибке
		log.print("Настройки события зафиксировать не удалось", log_t::flag_t::CRITICAL);
		// Выходим с ошибкой
		return EXIT_FAILURE;
	}
	// Если включить прослушивание не удалось
	if(!io.listen(eid, 64)){
		// Выводим сообщение об ошибке
		log.print("Прослушивание включить не удалось", log_t::flag_t::CRITICAL);
		// Выходим с ошибкой
		return EXIT_FAILURE;
	}
	// Если запустить событие не удалось
	if(!io.launch(eid)){
		// Выводим сообщение об ошибке
		log.print("Событие запустить не удалось", log_t::flag_t::CRITICAL);
		// Выходим с ошибкой
		return EXIT_FAILURE;
	}
	// Выводим сообщение о запуске
	log.print("Сервер запущен: %s:%u вид сокета %s", log_t::flag_t::INFO,
		address.c_str(), port, (stream ? "STREAM" : "SEQPACKET"));
	// Крутим цикл событий
	while(io.poll());
	// Выходим успешно
	return EXIT_SUCCESS;
}

/**
 * Если операционная система протокола SCTP не несёт
 */
#else

/**
 * @brief Исполняемая функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main([[maybe_unused]] int32_t argc, [[maybe_unused]] char * argv[]){
	// Выводим сообщение о невозможности работы
	std::cout << " Протокол SCTP этой операционной системой не поддерживается" << std::endl;
	// Выходим с ошибкой
	return EXIT_FAILURE;
}

#endif
