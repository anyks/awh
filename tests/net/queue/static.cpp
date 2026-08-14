/**
 * @file: static.cpp
 * @date: 2026-02-07
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты сетевой очереди — проверка создания и сброса объекта модуля,
 *        а также корректности буферизации данных, контроля ёмкости и корректности порядка извлечения записей
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "queue.hpp"

/**
 * Стандартные заголовочные файлы для бенчмарка памяти
 */
#include <vector>
#include <memory>
#include <cstdio>
#include <chrono>
#include <algorithm>

/**
 * Если операционной системой является macOS - подключаем mach API для измерения RSS
 */
#if defined(__APPLE__)
	#include <mach/mach.h>
#elif defined(__linux__)
	#include <cstdio>
#endif

/**
 * @brief Функция получения текущего резидентного размера памяти процесса (RSS) в байтах
 *
 * @return текущий RSS процесса в байтах (0 если измерение недоступно)
 *
 */
static size_t currentRSS() noexcept {
	/**
	 * Если операционной системой является macOS
	 */
	#if defined(__APPLE__)
		// Структура базовой информации о задаче
		mach_task_basic_info_data_t info;
		// Количество элементов информации о задаче
		mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
		// Получаем информацию о текущей задаче
		if(::task_info(::mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast <task_info_t> (&info), &count) != KERN_SUCCESS)
			// Возвращаем 0 при ошибке измерения
			return 0;
		// Возвращаем размер резидентной памяти
		return static_cast <size_t> (info.resident_size);
	/**
	 * Если операционной системой является Linux
	 */
	#elif defined(__linux__)
		// Количество страниц резидентной памяти
		long rss = 0;
		// Открываем файл статистики процесса
		FILE * file = ::fopen("/proc/self/statm", "r");
		// Если файл открыть не удалось
		if(file == nullptr)
			// Возвращаем 0 при ошибке измерения
			return 0;
		// Читаем размер резидентной памяти в страницах (второе поле)
		if(::fscanf(file, "%*s%ld", &rss) != 1)
			// Сбрасываем значение при ошибке чтения
			rss = 0;
		// Закрываем файл статистики
		::fclose(file);
		// Возвращаем размер резидентной памяти в байтах
		return (static_cast <size_t> (rss) * static_cast <size_t> (::sysconf(_SC_PAGESIZE)));
	/**
	 * Для остальных операционных систем
	 */
	#else
		// Измерение RSS не поддерживается
		return 0;
	#endif
}

/**
 * @brief Метод инициализации тестовой среды
 *
 */
TEST_F(NetworkQueueFixture, CreateQueueTest){
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
	// Очищаем объект очереди
	this->_queue.reset();
	// Проверяем удаление объекта очереди
	ASSERT_TRUE(this->_queue == nullptr);
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
TEST_F(NetworkQueueFixture, ResetAndCreateQueueTest){
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
	// Очищаем объект очереди
	this->_queue.reset();
	// Проверяем удаление объекта очереди
	ASSERT_TRUE(this->_queue == nullptr);
	// Создаём объект очереди
	this->_queue = std::make_unique <awh::net_queue_t> (this->_fmk.get(), this->_log.get());
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
TEST_F(NetworkQueueFixture, ReCreateQueueTest){
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
	// Cоздаём объект очереди заново
	this->_queue = std::make_unique <awh::net_queue_t> (this->_fmk.get(), this->_log.get());
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
}

/**
 * @brief Тест полной очистки TCP-очереди через pop() с большим размером
 *
 */
TEST_F(NetworkQueueFixture, TcpPopFullClearTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для потоков данных (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Подготавливаем блок данных для добавления в очередь
	std::vector <uint8_t> data(1024, 0x7F);
	// Добавляем данные в очередь
	ASSERT_EQ(this->_queue->push(data.data(), data.size()), data.size());
	// Проверяем что очередь не пустая
	ASSERT_FALSE(this->_queue->empty());
	// Для TCP количество записей отслеживает количество байт
	ASSERT_EQ(this->_queue->count(), this->_queue->size());
	// Удаляем все данные через pop() с размером не меньше размера буфера
	ASSERT_TRUE(this->_queue->pop(AWH_NETWORK_QUEUE_BUFFER_SIZE));
	// Проверяем что очередь опустела
	ASSERT_TRUE(this->_queue->empty());
	// Проверяем что размер данных обнулён
	ASSERT_EQ(this->_queue->size(), 0u);
	// Проверяем что инвариант count() == size() сохранён после полной очистки
	ASSERT_EQ(this->_queue->count(), this->_queue->size());
	// Проверяем что счётчик записей обнулён
	ASSERT_EQ(this->_queue->count(), 0u);
}

/**
 * @brief Тест частичного удаления данных из TCP-очереди через pop()
 *
 */
TEST_F(NetworkQueueFixture, TcpPopPartialTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для потоков данных (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Подготавливаем блок данных для добавления в очередь
	std::vector <uint8_t> data(1000, 0x21);
	// Добавляем данные в очередь
	ASSERT_EQ(this->_queue->push(data.data(), data.size()), data.size());
	// Удаляем часть данных из очереди
	ASSERT_TRUE(this->_queue->pop(400));
	// Проверяем что в очереди остались корректные данные
	ASSERT_EQ(this->_queue->size(), 600u);
	// Проверяем что инвариант count() == size() сохранён после частичного удаления
	ASSERT_EQ(this->_queue->count(), this->_queue->size());
	// Получаем указатель на оставшиеся данные
	const void * ptr = nullptr;
	// Размер оставшихся данных
	size_t size = 0;
	// Получаем данные из очереди
	ASSERT_TRUE(this->_queue->front(&ptr, size));
	// Проверяем что размер оставшихся данных корректен
	ASSERT_EQ(size, 600u);
}

/**
 * @brief Тест частичного добавления данных в TCP-очередь
 *
 * @details Поток делится где угодно, поэтому очередь обязана принять СКОЛЬКО ВЛЕЗАЕТ и
 *          сообщить принятое, а не отвергать порцию целиком. Отказ целиком заставлял бы
 *          отправителя терять всю порцию вместо её остатка, а вытягивающую модель отправки
 *          - терять данные безвозвратно: копии у источника нет
 *
 * @note Правило «всё или ничего» принадлежит ТОЛЬКО очереди границ сообщений: дейтаграмма
 *       неделима, и половина сообщения смысла не имеет. Смешение этих двух правил
 *       закрепляется здесь намеренно, чтобы очередной разбор кода не свёл их к одному
 *
 */
TEST_F(NetworkQueueFixture, TcpPushPartialTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для потоков данных (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Подготавливаем небольшой блок данных, который гарантированно помещается
	std::vector <uint8_t> head(1000, 0x11);
	// Добавляем данные в очередь
	ASSERT_EQ(this->_queue->push(head.data(), head.size()), head.size());
	// Запоминаем свободное место, оставшееся в очереди
	const size_t available = this->_queue->available();
	// Проверяем что свободное место в очереди осталось
	ASSERT_GT(available, 0u);
	// Подготавливаем блок данных размером со весь буфер - он заведомо крупнее свободного места
	std::vector <uint8_t> big(AWH_NETWORK_QUEUE_BUFFER_SIZE, 0x22);
	// Очередь обязана принять ровно столько, сколько влезает, и сообщить принятое
	ASSERT_EQ(this->_queue->push(big.data(), big.size()), available);
	// Проверяем что принятое действительно легло в очередь
	ASSERT_EQ(this->_queue->size(), (head.size() + available));
	// Проверяем что инвариант count() == size() для TCP сохранён
	ASSERT_EQ(this->_queue->count(), this->_queue->size());
	// Проверяем что свободного места в очереди больше не осталось
	ASSERT_EQ(this->_queue->available(), 0u);
}

/**
 * @brief Тест переключения регионов A/B (wrap) bip-буфера для TCP с проверкой целостности данных
 *
 */
TEST_F(NetworkQueueFixture, TcpBipWrapTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для потоков данных (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Первый блок данных заполняем шаблоном 0xAA - он ляжет в регион A
	std::vector <uint8_t> blockA(60000, 0xAA);
	// Добавляем первый блок в очередь
	ASSERT_EQ(this->_queue->push(blockA.data(), blockA.size()), blockA.size());
	// Удаляем часть данных из начала региона A (освобождаем голову буфера)
	ASSERT_TRUE(this->_queue->pop(40000));
	// В регионе A осталось 20000 байт
	ASSERT_EQ(this->_queue->size(), 20000u);
	// Второй блок данных заполняем шаблоном 0xBB - в хвосте места мало, он откроет регион B в начале буфера
	std::vector <uint8_t> blockB(30000, 0xBB);
	// Добавляем второй блок в очередь
	ASSERT_EQ(this->_queue->push(blockB.data(), blockB.size()), blockB.size());
	// Суммарный размер данных в очереди
	ASSERT_EQ(this->_queue->size(), 50000u);
	// Указатель на данные в очереди
	const void * ptr = nullptr;
	// Размер непрерывного региона
	size_t size = 0;
	// Читаем первый непрерывный регион (остаток региона A)
	ASSERT_TRUE(this->_queue->front(&ptr, size));
	// Регион A должен вернуть ровно 20000 байт
	ASSERT_EQ(size, 20000u);
	// Проверяем что содержимое региона A соответствует шаблону 0xAA
	ASSERT_EQ(std::count(reinterpret_cast <const uint8_t *> (ptr), reinterpret_cast <const uint8_t *> (ptr) + size, 0xAA), static_cast <ptrdiff_t> (size));
	// Удаляем остаток региона A (произойдёт переключение региона B в регион A)
	ASSERT_TRUE(this->_queue->pop(size));
	// В очереди остались данные региона B
	ASSERT_EQ(this->_queue->size(), 30000u);
	// Читаем второй непрерывный регион (бывший регион B)
	ASSERT_TRUE(this->_queue->front(&ptr, size));
	// Регион B должен вернуть ровно 30000 байт
	ASSERT_EQ(size, 30000u);
	// Проверяем что содержимое региона B соответствует шаблону 0xBB
	ASSERT_EQ(std::count(reinterpret_cast <const uint8_t *> (ptr), reinterpret_cast <const uint8_t *> (ptr) + size, 0xBB), static_cast <ptrdiff_t> (size));
	// Удаляем остаток данных
	ASSERT_TRUE(this->_queue->pop(size));
	// Очередь должна опустеть
	ASSERT_TRUE(this->_queue->empty());
}

/**
 * @brief Тест гарантии available(): запись ровно на available() байт всегда успешна (bip-буфер TCP)
 *
 */
TEST_F(NetworkQueueFixture, TcpBipAvailableGuaranteeTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для потоков данных (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Заполняем регион A крупным блоком
	std::vector <uint8_t> blockA(60000, 0x01);
	// Добавляем блок в очередь
	ASSERT_EQ(this->_queue->push(blockA.data(), blockA.size()), blockA.size());
	// Освобождаем голову буфера, создавая фрагментацию (хвост маленький, голова большая)
	ASSERT_TRUE(this->_queue->pop(40000));
	// Запрашиваем размер наибольшего непрерывного свободного региона
	const size_t free = this->_queue->available();
	// Свободный непрерывный регион должен быть больше нуля
	ASSERT_GT(free, 0u);
	// Подготавливаем блок ровно на доступный непрерывный размер
	std::vector <uint8_t> fit(free, 0x02);
	// Запись ровно на available() байт обязана пройти полностью
	ASSERT_EQ(this->_queue->push(fit.data(), fit.size()), free);
	// Любая следующая запись хотя бы на 1 байт обязана провалиться (места больше нет)
	uint8_t extra = 0x03;
	// Проверяем что переполнение корректно отвергается
	ASSERT_EQ(this->_queue->push(&extra, sizeof(extra)), 0u);
}

/**
 * @brief Тест компактности объекта очереди (буфер выделяется лениво, а не встроен по значению)
 *
 */
TEST_F(NetworkQueueFixture, QueueMemoryFootprintTest){
	// Размер объекта очереди должен быть небольшим (буфер вынесен в кучу и выделяется лениво)
	ASSERT_LE(sizeof(awh::net_queue_t), static_cast <size_t> (256));
	// Размер объекта должен быть существенно меньше размера буфера данных
	ASSERT_LT(sizeof(awh::net_queue_t), static_cast <size_t> (AWH_NETWORK_QUEUE_BUFFER_SIZE));
}

/**
 * @brief Тест чтения заголовков UDP-записей по невыровненным смещениям
 *
 */
TEST_F(NetworkQueueFixture, UdpUnalignedRecordTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для границ сообщений (например, UDP)
	this->_queue->type(awh::net_queue_t::type_t::UDP);
	// Список записей разной длины для создания невыровненных смещений заголовков
	const std::vector <std::string> records = {"a", "bc", "def", "ghij", "klmno", "pqrstu", "vwxyz012"};
	/**
	 * Добавляем все записи в очередь
	 */
	for(auto & record : records)
		// Добавляем запись в очередь
		ASSERT_EQ(this->_queue->push(record.data(), record.size()), record.size());
	// Проверяем что количество записей совпадает
	ASSERT_EQ(this->_queue->count(), records.size());
	// Индекс текущей записи
	size_t index = 0;
	/**
	 * Обходим очередь пока она не опустеет
	 */
	while(!this->_queue->empty()){
		// Указатель на данные в очереди
		const void * ptr = nullptr;
		// Размер данных записи
		size_t size = 0;
		// Получаем указатель на данные в очереди и их размер
		ASSERT_TRUE(this->_queue->front(&ptr, size));
		// Проверяем что размер записи прочитан корректно (невыровненный заголовок)
		ASSERT_EQ(size, records.at(index).size());
		// Проверяем что содержимое записи совпадает
		ASSERT_EQ(std::string(reinterpret_cast <const char *> (ptr), size), records.at(index));
		// Удаляем запись из очереди
		ASSERT_TRUE(this->_queue->pop());
		// Увеличиваем индекс текущей записи
		index++;
	}
	// Проверяем что обработаны все записи
	ASSERT_EQ(index, records.size());
}

/**
 * @brief Тест игнорирования некорректных аргументов push (нулевой размер и нулевой указатель)
 *
 */
TEST_F(NetworkQueueFixture, PushInvalidArgumentsTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для потоков данных (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Блок данных для проверки
	uint8_t byte = 0x7E;
	// Добавление с нулевым указателем должно вернуть 0
	ASSERT_EQ(this->_queue->push(nullptr, sizeof(byte)), 0u);
	// Добавление с нулевым размером должно вернуть 0
	ASSERT_EQ(this->_queue->push(&byte, 0), 0u);
	// Очередь должна остаться пустой
	ASSERT_TRUE(this->_queue->empty());
	// Размер данных должен быть нулевым
	ASSERT_EQ(this->_queue->size(), 0u);
	// Счётчик записей должен быть нулевым
	ASSERT_EQ(this->_queue->count(), 0u);
}

/**
 * @brief Тест операций чтения и удаления на пустой очереди (должны корректно отклоняться)
 *
 */
TEST_F(NetworkQueueFixture, EmptyQueueOperationsTest){
	// Очищаем очередь
	this->_queue->clear();
	// Указатель на данные в очереди
	const void * ptr = nullptr;
	// Размер данных при чтении
	size_t size = 0;
	/**
	 * Проверяем поведение пустой очереди для обоих типов
	 */
	for(const awh::net_queue_t::type_t type : {awh::net_queue_t::type_t::TCP, awh::net_queue_t::type_t::UDP}){
		// Устанавливаем тип очереди
		this->_queue->type(type);
		// Очередь должна быть пустой
		ASSERT_TRUE(this->_queue->empty());
		// Чтение из пустой очереди должно вернуть false
		ASSERT_FALSE(this->_queue->front(&ptr, size));
		// Удаление записи из пустой очереди должно вернуть false
		ASSERT_FALSE(this->_queue->pop());
		// Удаление по размеру из пустой очереди должно вернуть false
		ASSERT_FALSE(this->_queue->pop(1024));
	}
}

/**
 * @brief Тест цикла ленивого выделения и возврата буфера в пул (многократное наполнение/опустошение)
 *
 * @note При опустошении очереди буфер возвращается в потоко-локальный пул, а следующая запись
 *       переиспользует блок из пула. Тест прогоняет множество циклов и проверяет целостность данных.
 *
 */
TEST_F(NetworkQueueFixture, BufferPoolReuseCycleTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для потоков данных (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Количество циклов наполнения/опустошения
	constexpr size_t CYCLES = 256;
	// Размер блока данных в каждом цикле
	constexpr size_t BLOCK = 2048;
	/**
	 * Прогоняем циклы наполнения и полного опустошения очереди
	 */
	for(size_t cycle = 0; cycle < CYCLES; cycle++){
		// Уникальный для цикла байт-заполнитель (для проверки целостности после переиспользования блока)
		const uint8_t filler = static_cast <uint8_t> (cycle & 0xFF);
		// Блок данных текущего цикла
		std::vector <uint8_t> data(BLOCK, filler);
		// Добавляем блок данных в очередь (ленивое выделение/переиспользование блока из пула)
		ASSERT_EQ(this->_queue->push(data.data(), data.size()), data.size());
		// Очередь не должна быть пустой
		ASSERT_FALSE(this->_queue->empty());
		// Размер данных должен совпадать с записанным блоком
		ASSERT_EQ(this->_queue->size(), BLOCK);
		// Указатель на данные в очереди
		const void * ptr = nullptr;
		// Размер непрерывного региона
		size_t size = 0;
		// Читаем данные из очереди
		ASSERT_TRUE(this->_queue->front(&ptr, size));
		// Размер непрерывного региона должен совпадать с блоком
		ASSERT_EQ(size, BLOCK);
		// Содержимое блока должно соответствовать байту-заполнителю текущего цикла
		ASSERT_EQ(std::count(reinterpret_cast <const uint8_t *> (ptr), reinterpret_cast <const uint8_t *> (ptr) + size, filler), static_cast <ptrdiff_t> (size));
		// Полностью опустошаем очередь (буфер возвращается в пул)
		ASSERT_TRUE(this->_queue->pop(BLOCK));
		// Очередь должна опустеть
		ASSERT_TRUE(this->_queue->empty());
		// Размер данных должен обнулиться
		ASSERT_EQ(this->_queue->size(), 0u);
	}
}

/**
 * @brief Тест атомарности переполнения UDP-очереди (запись не помещается целиком - очередь не меняется)
 *
 */
TEST_F(NetworkQueueFixture, UdpPushOverflowAtomicTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для границ сообщений (например, UDP)
	this->_queue->type(awh::net_queue_t::type_t::UDP);
	// Крупная запись, чтобы быстро заполнить буфер
	std::vector <uint8_t> record(8000, 0x44);
	// Количество успешно добавленных записей
	size_t pushed = 0;
	/**
	 * Заполняем очередь записями до отказа
	 */
	while(this->_queue->push(record.data(), record.size()) == record.size())
		// Увеличиваем счётчик добавленных записей
		pushed++;
	// Хотя бы одна запись должна была добавиться
	ASSERT_GT(pushed, 0u);
	// Фиксируем состояние очереди на момент отказа
	const size_t countBefore = this->_queue->count();
	// Фиксируем размер данных на момент отказа
	const size_t sizeBefore = this->_queue->size();
	// Количество записей должно совпадать с числом успешных добавлений
	ASSERT_EQ(countBefore, pushed);
	// Повторная попытка добавления должна провалиться (всё-или-ничего)
	ASSERT_EQ(this->_queue->push(record.data(), record.size()), 0u);
	// Состояние очереди не должно измениться после неудачного добавления
	ASSERT_EQ(this->_queue->count(), countBefore);
	// Размер данных не должен измениться после неудачного добавления
	ASSERT_EQ(this->_queue->size(), sizeBefore);
}

/**
 * @brief Тест дефрагментации UDP-очереди (compact): запись, помещающаяся только после сжатия буфера
 *
 */
TEST_F(NetworkQueueFixture, UdpCompactIntegrityTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для границ сообщений (например, UDP)
	this->_queue->type(awh::net_queue_t::type_t::UDP);
	// Размер полезной нагрузки одной записи
	constexpr size_t PAYLOAD = 10000;
	// Количество предварительно добавляемых записей
	constexpr size_t RECORDS = 6;
	/**
	 * Добавляем записи, заполняя буфер так, чтобы в хвосте осталось мало места
	 */
	for(size_t i = 0; i < RECORDS; i++){
		// Запись с уникальным байтом-заполнителем
		std::vector <uint8_t> record(PAYLOAD, static_cast <uint8_t> (i + 1));
		// Добавляем запись в очередь
		ASSERT_EQ(this->_queue->push(record.data(), record.size()), record.size());
	}
	// Удаляем первую запись из начала очереди (освобождаем место в голове, а не в хвосте)
	ASSERT_TRUE(this->_queue->pop());
	// Запись, которая не поместится в хвост, но поместится после сжатия (compact)
	std::vector <uint8_t> tail(9000, 0x99);
	// Добавление должно сработать только за счёт дефрагментации буфера
	ASSERT_EQ(this->_queue->push(tail.data(), tail.size()), tail.size());
	// В очереди должны остаться записи 2..6 и новая запись
	ASSERT_EQ(this->_queue->count(), RECORDS);
	// Ожидаемые записи после удаления первой и добавления хвостовой (байт-заполнитель и размер)
	std::vector <std::pair <uint8_t, size_t>> expected;
	/**
	 * Записи со 2-й по 6-ю сохраняют исходный порядок и содержимое
	 */
	for(size_t i = 1; i < RECORDS; i++)
		// Добавляем ожидаемую запись
		expected.emplace_back(static_cast <uint8_t> (i + 1), PAYLOAD);
	// Последней идёт добавленная после сжатия запись
	expected.emplace_back(static_cast <uint8_t> (0x99), tail.size());
	// Индекс текущей ожидаемой записи
	size_t index = 0;
	/**
	 * Обходим очередь и проверяем целостность данных после дефрагментации
	 */
	while(!this->_queue->empty()){
		// Указатель на данные в очереди
		const void * ptr = nullptr;
		// Размер данных записи
		size_t size = 0;
		// Получаем данные из очереди
		ASSERT_TRUE(this->_queue->front(&ptr, size));
		// Размер записи должен совпадать с ожидаемым
		ASSERT_EQ(size, expected.at(index).second);
		// Содержимое записи должно соответствовать ожидаемому байту-заполнителю
		ASSERT_EQ(std::count(reinterpret_cast <const uint8_t *> (ptr), reinterpret_cast <const uint8_t *> (ptr) + size, expected.at(index).first), static_cast <ptrdiff_t> (size));
		// Удаляем запись из очереди
		ASSERT_TRUE(this->_queue->pop());
		// Увеличиваем индекс ожидаемой записи
		index++;
	}
	// Проверяем, что обработаны все ожидаемые записи
	ASSERT_EQ(index, expected.size());
}

/**
 * @brief Тест заполнения вторичного региона B bip-буфера TCP и семантики available() при активном B
 *
 */
TEST_F(NetworkQueueFixture, TcpRegionBFillTest){
	// Очищаем очередь
	this->_queue->clear();
	// Устанавливаем тип очереди для потоков данных (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Крупный блок данных заполняет регион A
	std::vector <uint8_t> blockA(60000, 0xA0);
	// Добавляем блок в регион A
	ASSERT_EQ(this->_queue->push(blockA.data(), blockA.size()), blockA.size());
	// Освобождаем большую часть головы буфера (хвост остаётся маленьким)
	ASSERT_TRUE(this->_queue->pop(50000));
	// В регионе A осталось 10000 байт
	ASSERT_EQ(this->_queue->size(), 10000u);
	// Первый блок региона B (в хвосте места мало - откроется регион B в начале буфера)
	std::vector <uint8_t> blockB1(5000, 0xB1);
	// Добавляем первый блок в регион B
	ASSERT_EQ(this->_queue->push(blockB1.data(), blockB1.size()), blockB1.size());
	// При активном регионе B available() возвращает непрерывное место между концом B и началом A
	const size_t freeB = this->_queue->available();
	// Свободного места должно хватать на крупный второй блок
	ASSERT_GT(freeB, 0u);
	// Второй блок региона B ровно на доступный размер
	std::vector <uint8_t> blockB2(freeB, 0xB2);
	// Запись ровно на available() должна пройти полностью
	ASSERT_EQ(this->_queue->push(blockB2.data(), blockB2.size()), freeB);
	// После заполнения региона B свободного непрерывного места не остаётся
	ASSERT_EQ(this->_queue->available(), 0u);
	// Любая следующая запись обязана провалиться
	uint8_t extra = 0x03;
	// Проверяем что переполнение корректно отвергается
	ASSERT_EQ(this->_queue->push(&extra, sizeof(extra)), 0u);
	// Суммарный размер данных в очереди (остаток A + оба блока B)
	ASSERT_EQ(this->_queue->size(), (10000u + 5000u + freeB));
	// Указатель на данные в очереди
	const void * ptr = nullptr;
	// Размер непрерывного региона
	size_t size = 0;
	// Читаем остаток региона A
	ASSERT_TRUE(this->_queue->front(&ptr, size));
	// Регион A должен вернуть ровно 10000 байт
	ASSERT_EQ(size, 10000u);
	// Содержимое региона A должно соответствовать шаблону 0xA0
	ASSERT_EQ(std::count(reinterpret_cast <const uint8_t *> (ptr), reinterpret_cast <const uint8_t *> (ptr) + size, 0xA0), static_cast <ptrdiff_t> (size));
	// Удаляем остаток региона A (произойдёт переключение региона B в регион A)
	ASSERT_TRUE(this->_queue->pop(size));
	// В очереди остаются данные региона B
	ASSERT_EQ(this->_queue->size(), (5000u + freeB));
	// Читаем непрерывный бывший регион B
	ASSERT_TRUE(this->_queue->front(&ptr, size));
	// Бывший регион B должен вернуть весь свой объём одним непрерывным прогоном
	ASSERT_EQ(size, (5000u + freeB));
	// Первые 5000 байт региона B соответствуют шаблону 0xB1
	ASSERT_EQ(std::count(reinterpret_cast <const uint8_t *> (ptr), reinterpret_cast <const uint8_t *> (ptr) + 5000, 0xB1), static_cast <ptrdiff_t> (5000));
	// Оставшиеся байты региона B соответствуют шаблону 0xB2
	ASSERT_EQ(std::count(reinterpret_cast <const uint8_t *> (ptr) + 5000, reinterpret_cast <const uint8_t *> (ptr) + size, 0xB2), static_cast <ptrdiff_t> (freeB));
	// Удаляем остаток данных
	ASSERT_TRUE(this->_queue->pop(size));
	// Очередь должна опустеть
	ASSERT_TRUE(this->_queue->empty());
}

/**
 * @brief Тест переиспользования очереди после clear() со сменой типа (UDP -> TCP)
 *
 */
TEST_F(NetworkQueueFixture, ClearAndSwitchTypeTest){
	// Устанавливаем тип очереди для границ сообщений (например, UDP)
	this->_queue->type(awh::net_queue_t::type_t::UDP);
	// Записи разной длины для UDP-режима
	const std::vector <std::string> records = {"alpha", "beta", "gamma"};
	/**
	 * Добавляем записи в очередь
	 */
	for(auto & record : records)
		// Добавляем запись в очередь
		ASSERT_EQ(this->_queue->push(record.data(), record.size()), record.size());
	// Количество записей должно совпадать
	ASSERT_EQ(this->_queue->count(), records.size());
	// Очищаем очередь (буфер возвращается в пул)
	this->_queue->clear();
	// После очистки очередь пуста
	ASSERT_TRUE(this->_queue->empty());
	// Размер данных обнулён
	ASSERT_EQ(this->_queue->size(), 0u);
	// Счётчик записей обнулён
	ASSERT_EQ(this->_queue->count(), 0u);
	// Переключаем тип очереди на потоковый (например, TCP)
	this->_queue->type(awh::net_queue_t::type_t::TCP);
	// Тип очереди должен корректно переключиться
	ASSERT_EQ(this->_queue->type(), awh::net_queue_t::type_t::TCP);
	// Блок данных для потокового режима
	std::vector <uint8_t> data(4096, 0x5C);
	// Очередь должна корректно работать после смены типа
	ASSERT_EQ(this->_queue->push(data.data(), data.size()), data.size());
	// Для TCP счётчик записей отслеживает количество байт
	ASSERT_EQ(this->_queue->count(), this->_queue->size());
	// Размер данных должен совпадать с записанным блоком
	ASSERT_EQ(this->_queue->size(), data.size());
	// Указатель на данные в очереди
	const void * ptr = nullptr;
	// Размер непрерывного региона
	size_t size = 0;
	// Читаем данные из очереди
	ASSERT_TRUE(this->_queue->front(&ptr, size));
	// Размер региона должен совпадать с блоком
	ASSERT_EQ(size, data.size());
	// Содержимое блока должно соответствовать шаблону 0x5C
	ASSERT_EQ(std::count(reinterpret_cast <const uint8_t *> (ptr), reinterpret_cast <const uint8_t *> (ptr) + size, 0x5C), static_cast <ptrdiff_t> (size));
}

/**
 * @brief Бенчмарк экономии резидентной памяти (RSS) при ленивом выделении буфера
 *
 * @note Помечен префиксом DISABLED_ и не входит в обычный прогон из-за крупных аллокаций - запускать явно:
 *       ./unit-tests/net --gtest_also_run_disabled_tests --gtest_filter='*RssFootprintBenchmark*'
 *
 */
TEST_F(NetworkQueueFixture, DISABLED_RssFootprintBenchmark){
	// Количество эмулируемых соединений (каждое со своей очередью отправки)
	constexpr size_t CONNECTIONS = 100000;
	// Доля соединений, у которых очередь под backpressure (буфер реально выделен)
	constexpr size_t ACTIVE = (CONNECTIONS / 20); // 5%
	// Базовый замер резидентной памяти до создания очередей
	const size_t rssBase = currentRSS();
	// Контейнер очередей (хранятся в куче по указателю, как реальные узлы соединений)
	std::vector <std::unique_ptr <awh::net_queue_t>> queues;
	// Резервируем место под указатели заранее, чтобы не мерить рост самого вектора
	queues.reserve(CONNECTIONS);
	/**
	 * Создаём очереди для всех эмулируемых соединений (все пустые - буфер не выделяется)
	 */
	for(size_t i = 0; i < CONNECTIONS; i++)
		// Создаём очередь отправки для соединения
		queues.push_back(std::make_unique <awh::net_queue_t> (this->_fmk.get(), this->_log.get()));
	// Замер резидентной памяти при N пустых очередях (ленивое выделение - буферы ещё не созданы)
	const size_t rssEmpty = currentRSS();
	// Блок данных для активации буфера у части соединений
	std::vector <uint8_t> payload(4096, 0x5A);
	/**
	 * Активируем буфер у доли соединений (эмуляция backpressure)
	 */
	for(size_t i = 0; i < ACTIVE; i++){
		// Устанавливаем тип потоковой очереди
		queues[i]->type(awh::net_queue_t::type_t::TCP);
		// Кладём данные - это вызывает ленивое выделение блока буфера из пула
		queues[i]->push(payload.data(), payload.size());
	}
	// Замер резидентной памяти при активной доле очередей
	const size_t rssActive = currentRSS();
	// Теоретический объём памяти при старой реализации (буфер 64 КБ встроен в каждый объект)
	const size_t legacy = (static_cast <size_t> (CONNECTIONS) * static_cast <size_t> (AWH_NETWORK_QUEUE_BUFFER_SIZE));
	// Вспомогательный делитель для перевода в мегабайты
	constexpr double MB = (1024.0 * 1024.0);
	// Выводим результаты замера
	::printf("\n========== RSS Footprint Benchmark ==========\n");
	// Параметры сценария
	::printf("Connections: %zu, active (backpressure): %zu (%.1f%%)\n", CONNECTIONS, ACTIVE, (100.0 * ACTIVE / CONNECTIONS));
	// Размер одного объекта очереди
	::printf("sizeof(net_queue_t): %zu bytes\n", sizeof(awh::net_queue_t));
	// Базовый RSS
	::printf("RSS base:                       %8.2f MB\n", (rssBase / MB));
	// RSS при N пустых очередях
	::printf("RSS with %zu empty queues:   %8.2f MB (delta %.2f MB)\n", CONNECTIONS, (rssEmpty / MB), ((rssEmpty - rssBase) / MB));
	// RSS при активной доле
	::printf("RSS with %zu active buffers:    %8.2f MB (delta %.2f MB)\n", ACTIVE, (rssActive / MB), ((rssActive - rssEmpty) / MB));
	// Теоретический объём старой реализации
	::printf("Legacy inline (64KB x N):       %8.2f MB\n", (legacy / MB));
	// Экономия памяти относительно старой реализации (при текущей активной доле)
	if(rssActive > rssBase)
		// Выводим коэффициент экономии
		::printf("Savings vs legacy:              %8.1fx\n", (static_cast <double> (legacy) / static_cast <double> (rssActive - rssBase)));
	// Завершаем вывод результатов
	::printf("=============================================\n");
	// Проверяем, что измерение RSS доступно на данной платформе
	if((rssBase > 0) && (rssEmpty > 0)){
		// Прирост памяти от N пустых очередей должен быть кардинально меньше старой реализации (буферы не выделены)
		ASSERT_LT((rssEmpty - rssBase), (legacy / 10));
	}
}

/**
 * @brief Функция вычисления перцентиля по отсортированному массиву задержек
 *
 * @param sorted отсортированный по возрастанию массив задержек (нс)
 * @param p      перцентиль в диапазоне [0.0, 1.0]
 * @return       значение задержки на заданном перцентиле
 *
 */
static double percentile(const std::vector <double> & sorted, const double p) noexcept {
	// Если массив пуст - возвращаем 0
	if(sorted.empty())
		// Возвращаем значение по умолчанию
		return 0.0;
	// Вычисляем индекс перцентиля
	size_t index = static_cast <size_t> (p * (sorted.size() - 1));
	// Возвращаем значение на заданном перцентиле
	return sorted[index];
}

/**
 * @brief Бенчмарк латентности push под фрагментацией: TCP-bip (без memmove) против UDP-linear (с compact)
 *
 * @note Помечен префиксом DISABLED_ и не входит в обычный прогон - запускать явно:
 *       ./unit-tests/net --gtest_also_run_disabled_tests --gtest_filter='*LatencyBenchmark*'
 *
 */
TEST_F(NetworkQueueFixture, DISABLED_LatencyBenchmark){
	// Размер одного блока данных в байтах
	constexpr size_t BLOCK = 1024;
	// Количество измеряемых итераций push
	constexpr size_t ITERS = 500000;
	// Целевой объём заполнения буфера для создания фрагментации (близко к размеру буфера)
	constexpr size_t FILL = 60000;
	// Блок данных для записи
	std::vector <uint8_t> payload(BLOCK, 0x33);
	// Указатель на данные в очереди
	const void * ptr = nullptr;
	// Размер данных при чтении
	size_t size = 0;
	/**
	 * @brief Лямбда измерения латентности push для заданного типа очереди
	 *
	 */
	auto bench = [&](const awh::net_queue_t::type_t type, const char * label){
		// Очищаем очередь
		this->_queue->clear();
		// Устанавливаем тип очереди
		this->_queue->type(type);
		/**
		 *  Предварительно заполняем очередь до целевого объёма
		 */
		while(this->_queue->size() + BLOCK <= FILL)
			// Добавляем блок данных в очередь
			this->_queue->push(payload.data(), payload.size());
		// Массив измеренных задержек push (нс)
		std::vector <double> samples;
		// Резервируем место под измерения
		samples.reserve(ITERS);
		// Контрольная сумма для предотвращения оптимизации измерений
		size_t sink = 0;
		/**
		 * Выполняем измеряемые итерации
		 */
		for(size_t i = 0; i < ITERS; i++){
			// Удаляем один блок из начала очереди (освобождаем место)
			if(type == awh::net_queue_t::type_t::UDP)
				// Для UDP удаляем целую запись
				this->_queue->pop();
			// Для TCP удаляем ровно размер блока
			else this->_queue->pop(BLOCK);
			// Фиксируем момент начала измерения
			const auto start = std::chrono::high_resolution_clock::now();
			// Добавляем блок данных в очередь (измеряемая операция)
			sink += this->_queue->push(payload.data(), payload.size());
			// Фиксируем момент окончания измерения
			const auto finish = std::chrono::high_resolution_clock::now();
			// Сохраняем измеренную задержку в наносекундах
			samples.push_back(std::chrono::duration <double, std::nano> (finish - start).count());
		}
		// Используем контрольную сумму, чтобы компилятор не выбросил измеряемый код
		ASSERT_GT(sink, 0u);
		// Сортируем измерения по возрастанию
		std::sort(samples.begin(), samples.end());
		// Вычисляем среднее значение задержки
		double sum = 0.0;
		/**
		 * Суммируем все измерения
		 */
		for(double value : samples) sum += value;
		// Выводим результаты
		::printf("%-18s p50=%7.1f  p90=%7.1f  p99=%7.1f  p99.9=%8.1f  max=%9.1f  mean=%7.1f  (ns)\n",
			label,
			percentile(samples, 0.50), percentile(samples, 0.90),
			percentile(samples, 0.99), percentile(samples, 0.999),
			samples.back(), (sum / samples.size()));
	};
	// Выводим заголовок результатов
	::printf("\n========== push() Latency Benchmark (fragmented, block=%zu, iters=%zu) ==========\n", BLOCK, ITERS);
	// Прогоняем бенчмарк для TCP (bip-буфер, без memmove)
	bench(awh::net_queue_t::type_t::TCP, "TCP bip:");
	// Прогоняем бенчмарк для UDP (линейный буфер с compact/memmove)
	bench(awh::net_queue_t::type_t::UDP, "UDP linear:");
	// Завершаем вывод результатов
	::printf("====================================================================================\n");
}
