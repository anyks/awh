/**
 * @file generic.cpp
 * @date 2026-11-16
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
 * @brief Пример узла AF_XDP обобщённого режима — демонстрация спутника engine::xdp
 *        без перенаправления трафика: проверка поддержки ядром, заведение узла,
 *        отправка кадров через кольцо TX и ручной приём из кольца RX
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Спутник AF_XDP объявлен только под Linux и лишь когда он включён сборкой
 *
 * @details Признак AWH_XDP ставит выбор сборки -DCMAKE_BUILD_XDP=ON. Вне этого
 *          условия пример печатает причину и выходит: звать engine::xdp неоткуда,
 *          а молча собранный пустой образ ничего бы не показал
 *
 */
#if defined(__linux__) && defined(AWH_XDP)
	/**
	 * Стандартные модули
	 */
	#include <iostream>
	#include <cstring>
	#include <cstdlib>
	#include <cstdint>
	#include <string>
	#include <unistd.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>
	#include <net/if.h>
	#include <linux/if_ether.h>
	/**
	 * Подключаем заголовочные файлы проекта
	 */
	#include <sys/fmk.hpp>
	#include <sys/log.hpp>
	#include <net/engine.hpp>
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;
	/**
	 * @brief Параметры запуска примера
	 *
	 * @details Устройство называется доводом обязательно: выбирать его самому
	 *          нельзя, на живом аплинке даже обобщённый узел занимает очередь
	 *          устройства, и молчаливый выбор подменил бы названную карту чужой
	 *
	 */
	typedef struct Params {
		// Название сетевого устройства
		string iface;
		// Номер очереди устройства
		uint32_t queue;
		// Число кадров отправки
		uint32_t count;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Params() noexcept : iface{""}, queue(0), count(3) {}
	} params_t;
	/**
	 * @brief Функция печати способа запуска
	 *
	 * @param name название исполняемого файла
	 *
	 */
	static void usage(const char * name) noexcept {
		// Печатаем заголовок примера
		cout << "Узел AF_XDP обобщённого режима AWH" << endl << endl;
		// Печатаем строку запуска
		cout << "Запуск: " << name << " --iface <имя> [доводы]" << endl << endl;
		// Печатаем довод названия устройства
		cout << "  --iface <имя>     сетевое устройство (обязательно), например eth0" << endl;
		// Печатаем довод номера очереди
		cout << "  --queue <номер>   очередь устройства (по умолчанию 0)" << endl;
		// Печатаем довод числа кадров
		cout << "  --count <число>   число кадров отправки (по умолчанию 3)" << endl;
		// Печатаем довод справки
		cout << "  --help            напечатать этот текст" << endl << endl;
		// Печатаем пояснение о правах
		cout << "Права суперпользователя обязательны: заведение узла AF_XDP требует CAP_NET_ADMIN." << endl;
		// Печатаем пояснение о безопасности режима
		cout << "Программа BPF не крепится: трафик не перенаправляется, кольцо приёма тихо, связь машины не нарушается." << endl;
	}
	/**
	 * @brief Функция разбора доводов запуска
	 *
	 * @param argc   длина массива параметров
	 * @param argv   массив параметров
	 * @param params параметры запуска
	 * @return       результат разбора
	 *
	 */
	static bool parse(int32_t argc, char * argv[], params_t & params) noexcept {
		/**
		 * Перебираем все переданные доводы
		 */
		for(int32_t i = 1; i < argc; i++){
			// Получаем название довода
			const string arg(argv[i]);
			// Если запрошен способ запуска
			if(arg.compare("--help") == 0){
				// Печатаем способ запуска
				usage(argv[0]);
				// Возвращаем отказ: работать дальше незачем
				return false;
			// Иначе довод требует значения
			} else {
				// Если значение довода не передано
				if((i + 1) >= argc){
					// Сообщаем о доводе без значения
					cout << "Довод \"" << arg << "\" требует значения" << endl;
					// Возвращаем отказ разбора
					return false;
				}
				// Получаем значение довода
				const string value(argv[++i]);
				// Если названо сетевое устройство
				if(arg.compare("--iface") == 0)
					// Запоминаем название устройства
					params.iface = value;
				// Если назван номер очереди
				else if(arg.compare("--queue") == 0)
					// Запоминаем номер очереди
					params.queue = static_cast <uint32_t> (::strtoul(value.c_str(), nullptr, 10));
				// Если названо число кадров отправки
				else if(arg.compare("--count") == 0)
					// Запоминаем число кадров
					params.count = static_cast <uint32_t> (::strtoul(value.c_str(), nullptr, 10));
				// Иначе довод неизвестен
				else {
					// Сообщаем о неизвестном доводе
					cout << "Неизвестный довод \"" << arg << "\"" << endl;
					// Возвращаем отказ разбора
					return false;
				}
			}
		}
		// Если устройство не названо, работать не на чем
		if(params.iface.empty()){
			// Сообщаем об обязательном доводе
			cout << "Довод --iface обязателен" << endl;
			// Возвращаем отказ разбора
			return false;
		}
		// Возвращаем успех разбора
		return true;
	}
	/**
	 * @brief Функция извлечения канального адреса устройства
	 *
	 * @details Адрес нужен кадру отправки: кадр адресуется самому устройству,
	 *          чтобы демонстрационная отправка не тревожила соседей по сети
	 *
	 * @param iface название сетевого устройства
	 * @param mac   буфер приёма канального адреса
	 * @return      результат извлечения адреса
	 *
	 */
	static bool deviceMac(const string & iface, uint8_t * mac) noexcept {
		// Заводим сокет для запроса параметров устройства
		const int query = ::socket(AF_INET, SOCK_DGRAM, 0);
		// Если сокет не заведён, адрес не получить
		if(query < 0)
			// Возвращаем отказ извлечения
			return false;
		// Объект запроса параметров устройства
		struct ifreq ifr{};
		// Устанавливаем название устройства в запрос
		::strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
		// Спрашиваем канальный адрес устройства вызовом SIOCGIFHWADDR
		const bool ok = (::ioctl(query, SIOCGIFHWADDR, &ifr) == 0);
		// Если адрес получен, переносим его в буфер приёмника
		if(ok)
			// Копируем канальный адрес
			::memcpy(mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
		// Закрываем сокет запроса
		::close(query);
		// Возвращаем результат извлечения
		return ok;
	}
	/**
	 * @brief Функция главного цикла примера
	 *
	 * @param params параметры запуска
	 * @return       код выхода из приложения
	 *
	 */
	static int32_t run(const params_t & params) noexcept {
		// Если ядро не несёт AF_XDP, показывать нечего
		if(!engine::xdp::supported()){
			// Сообщаем об отсутствии поддержки ядром
			log::print("Ядро не поддерживает AF_XDP", log::flag_t::CRITICAL);
			// Возвращаем отказ
			return EXIT_FAILURE;
		}
		// Сообщаем о поддержке AF_XDP ядром
		log::print("Ядро поддерживает AF_XDP", log::flag_t::INFO);
		// Инициализируем асинхронный движок ввода-вывода
		if(!engine::io::initialize()){
			// Сообщаем об отказе инициализации движка
			log::print("Ошибка инициализации движка ввода-вывода", log::flag_t::CRITICAL);
			// Возвращаем отказ
			return EXIT_FAILURE;
		}
		// Заводим узел обобщённого режима без перенаправления трафика
		const event::id_t id = engine::xdp::create(params.iface, params.queue, false, net::xdp::mode_t::GENERIC);
		// Если узел не заведён, датапуть недоступен
		if(id == 0){
			// Сообщаем об отказе заведения узла
			log::print("Узел AF_XDP не заведён: устройство=%s, очередь=%u (нужны права CAP_NET_ADMIN)", log::flag_t::CRITICAL, params.iface.c_str(), params.queue);
			// Возвращаем отказ
			return EXIT_FAILURE;
		}
		// Сообщаем о заведении узла и его режиме
		log::print("Узел заведён: ID=%u, устройство=%s, очередь=%u, режим=%s", log::flag_t::INFO, id, params.iface.c_str(), params.queue, ((engine::xdp::mode(id) == net::xdp::mode_t::NATIVE) ? "native" : "generic"));
		// Канальный адрес устройства-получателя кадра
		uint8_t mac[ETH_ALEN];
		// Если канальный адрес не получен, кадр не адресовать
		if(!deviceMac(params.iface, mac)){
			// Сообщаем об отказе извлечения адреса
			log::print("Канальный адрес устройства %s не получен", log::flag_t::CRITICAL, params.iface.c_str());
			// Сносим заведённый узел
			engine::xdp::destroy(id);
			// Возвращаем отказ
			return EXIT_FAILURE;
		}
		// Буфер кадра отправки наименьшей длины Ethernet
		uint8_t frame[64];
		// Обнуляем кадр целиком
		::memset(frame, 0, sizeof(frame));
		// Копируем адрес устройства в поле получателя кадра
		::memcpy(frame, mac, ETH_ALEN);
		// Копируем адрес устройства в поле отправителя кадра
		::memcpy(frame + ETH_ALEN, mac, ETH_ALEN);
		// Устанавливаем старший октет типа протокола IPv4
		frame[12] = 0x08;
		// Устанавливаем младший октет типа протокола IPv4
		frame[13] = 0x00;
		// Полезная нагрузка демонстрационного кадра
		const char payload[] = "AWH-XDP-GENERIC";
		// Копируем полезную нагрузку в кадр вслед за заголовком
		::memcpy(frame + 14, payload, sizeof(payload));
		/**
		 * Отправляем названное число кадров через кольцо TX узла
		 */
		for(uint32_t i = 0; i < params.count; i++){
			// Отправляем кадр через узел
			if(engine::xdp::send(id, frame, static_cast <uint32_t> (sizeof(frame))))
				// Сообщаем об отправке кадра
				log::print("Кадр отправлен: ID=%u, номер=%u, %zu байт", log::flag_t::INFO, id, (i + 1), sizeof(frame));
			// Иначе сообщаем об отказе отправки
			else log::print("Ошибка отправки кадра: ID=%u, номер=%u", log::flag_t::CRITICAL, id, (i + 1));
		}
		// Буфер ручного приёма из кольца RX
		uint8_t buffer[2048];
		// Число кадров, снятых ручным приёмом
		uint32_t received = 0;
		/**
		 * Дочитываем кольцо приёма ручным вызовом до его опустошения
		 *
		 * @details Перенаправление не закреплено, потому кадров в кольце нет и
		 *          первый же вызов отвечает нулём. Упражнение показывает сам
		 *          способ ручного приёма: в цикле событий кадры приходят сами
		 *          через подписку engine::io::on(id, callback::read_t)
		 *
		 */
		while(true){
			// Принимаем один кадр из кольца приёма
			const int32_t size = engine::xdp::receive(id, buffer, static_cast <uint32_t> (sizeof(buffer)));
			// Если кадров больше нет либо приём отказал, дочитывать нечего
			if(size <= 0)
				// Прекращаем дочитывание кольца
				break;
			// Учитываем принятый кадр
			received++;
			// Сообщаем о принятном кадре
			log::print("Кадр принят вручную: ID=%u, %d байт", log::flag_t::INFO, id, size);
		}
		// Сообщаем итог ручного приёма
		log::print("Ручной приём завершён: ID=%u, кадров=%u", log::flag_t::INFO, id, received);
		// Сносим узел датапути AF_XDP
		if(engine::xdp::destroy(id))
			// Сообщаем об уничтожении узла
			log::print("Узел уничтожен: ID=%u", log::flag_t::INFO, id);
		// Иначе сообщаем об отказе уничтожения
		else log::print("Ошибка уничтожения узла: ID=%u", log::flag_t::CRITICAL, id);
		// Возвращаем успех
		return EXIT_SUCCESS;
	}
	/**
	 * @brief Функция входа приложения
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 * @return     код выхода из приложения
	 *
	 */
	int32_t main(int32_t argc, char * argv[]){
		/**
		 * Заводим модуль ядра первым делом
		 *
		 * @note Заведение захватывает выдачу памяти процесса и обязано идти
		 *       ДО всякой выдачи и ДО порождения потоков
		 *
		 */
		fmk::initialize();
		// Параметры запуска примера
		params_t params;
		// Если доводы разобраны, показываем датапуть
		if(parse(argc, argv, params))
			// Возвращаем результат показа датапути
			return run(params);
		// Возвращаем отказ разбора доводов
		return EXIT_FAILURE;
	}
	/**
	 * Вне собранного спутника пример лишь называет причину
	 *
	 * @details Спутник engine::xdp объявлен только под Linux с признаком AWH_XDP,
	 *          и на прочих системах либо при выключенном спутнике звать его
	 *          неоткуда. Образ при этом собирается и запускается везде, как
	 *          всякий пример проекта, а не выпадает из сборки по системе
	 *
	 */
#else
	/**
	 * Стандартные модули
	 */
	#include <iostream>
	/**
	 * Подключаем заголовочные файлы проекта
	 */
	#include <sys/fmk.hpp>
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;
	/**
	 * @brief Функция входа приложения вне собранного спутника AF_XDP
	 *
	 * @return код выхода из приложения
	 *
	 */
	int32_t main(){
		/**
		 * Заводим модуль ядра первым делом
		 *
		 * @note Заведение захватывает выдачу памяти процесса и обязано идти
		 *       ДО всякой выдачи и ДО порождения потоков
		 *
		 */
		fmk::initialize();
		// Сообщаем о причине отсутствия примера
		cout << "Спутник AF_XDP не собран: он есть только под Linux и включается" << endl;
		// Сообщаем о выборе сборки, каком спутник включается
		cout << "выбором -DCMAKE_BUILD_XDP=ON при наличии libbpf и libxdp." << endl;
		// Возвращаем успех: отказ здесь означает лишь отсутствие спутника
		return EXIT_SUCCESS;
	}
#endif