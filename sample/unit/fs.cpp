/**
 * @file fs.cpp
 * @date 2026-02-25
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
 * @brief Пример работы с модулем наблюдения за файловой системой — демонстрация отслеживания создания, изменения,
 *        переименования и удаления файлов и каталогов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <unit/fs.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект узла файловой системы
	unit::fs_t fs(&fmk, &log);
	// Создаём новое событие работы с каталогом
	event::id_t did = fs.create(unit::fs_t::type_t::DIR);
	// Создаём новое событие работы с файлом
	event::id_t fid = fs.create(unit::fs_t::type_t::FILE);
	// Устанавливаем функцию обратного вызова на запись в событие
	fs.on <void (const event::id_t, const size_t)> ("write", [&log](const event::id_t eid, const size_t size) noexcept -> void {
		// Записываем в лог сообщение о записи в событие
		log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на чтение из события
	fs.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", [&log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const string message(reinterpret_cast <const char *> (data), size);
		// Записываем в лог сообщение о чтении из события
		log.print("Прочитано: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, size, message.c_str());
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на событие изменения состояния ноды файловой системы
	fs.on <void (const event::id_t, const event::status_t)> ("state", [&fs, &log](const event::id_t eid, const event::status_t status) noexcept -> void {
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (status)){
			// Если статус принятия
			case static_cast <uint8_t> (event::status_t::ACCEPTED):
				// Записываем в лог сообщение о принятии события
				log.print("Событие принято: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус уничтожения
			case static_cast <uint8_t> (event::status_t::DESTROYED):
				// Записываем в лог сообщение об уничтожении события
				log.print("Событие подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус инициализации
			case static_cast <uint8_t> (event::status_t::INITIAL):
				// Записываем в лог сообщение об инициализации события
				log.print("Событие инициализировано: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус запуска события
			case static_cast <uint8_t> (event::status_t::LAUNCHED):
				// Записываем в лог сообщение о запуске события
				log.print("Событие запущено: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус паузы события
			case static_cast <uint8_t> (event::status_t::PAUSED):
				// Записываем в лог сообщение о паузе события
				log.print("Событие на паузе: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус возобновления события
			case static_cast <uint8_t> (event::status_t::RESUMED):
				// Записываем в лог сообщение о возобновлении события
				log.print("Событие возобновлено: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус успешного выполнения события
			case static_cast <uint8_t> (event::status_t::SUCCESS):
				// Записываем в лог сообщение о успешном выполнении события
				log.print("Событие успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус неудачного выполнения события
			case static_cast <uint8_t> (event::status_t::FAILURE):
				// Записываем в лог сообщение о неудачном выполнении события
				log.print("Событие выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
			break;
			// Если статус выполнения события в ожидании
			case static_cast <uint8_t> (event::status_t::PENDING): {
				// Если тип узла события является файлом
				if(fs.type(eid) == unit::fs_t::type_t::FILE){
					// Записываем в лог сообщение о выполнении события в ожидании
					log.print("Событие в ожидании: ID=%u", log_t::flag_t::INFO, eid);
					// Устанавливаем смещение в файле
					// fs.setSeek(eid, event::seek_t::BEGIN, 1024);
					// Отправляем тестовое сообщение в файл
					fs.send(eid, "Hello World!!!", 14);
				}
			} break;
			// Если статус подключения события
			case static_cast <uint8_t> (event::status_t::CONNECTED):
				// Записываем в лог сообщение о подключении события
				log.print("Событие подключено: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус отмены события
			case static_cast <uint8_t> (event::status_t::CANCELLED):
				// Записываем в лог сообщение об отмене события
				log.print("Событие отменено: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус переподключения события
			case static_cast <uint8_t> (event::status_t::RECONNECTED):
				// Записываем в лог сообщение о переподключении события
				log.print("Событие переподключено: ID=%u", log_t::flag_t::INFO, eid);
			break;
			// Если статус прослушивания события
			case static_cast <uint8_t> (event::status_t::LISTENING):
				// Записываем в лог сообщение о прослушивании события
				log.print("Событие прослушивается: ID=%u", log_t::flag_t::INFO, eid);
			break;
		}
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на получений событий изменения состояния ноды файловой системы
	fs.on <void (const event::id_t, const event::action_t, const event::vnode_t, const std::string &)> ("vnode", [&log](const event::id_t eid, const event::action_t action, const event::vnode_t vnode, const std::string & path) noexcept -> void {
		/**
		 * Обрабатываем тип узла события
		 */
		switch(static_cast <uint8_t> (vnode)){
			// Если тип узла не определён
			case static_cast <uint8_t> (event::vnode_t::NONE):
				// Записываем в лог сообщение о типе узла события
				log.print("Тип узла события: Не определён, Путь=%s", log_t::flag_t::INFO, path.c_str());
			break;
			case static_cast <uint8_t> (event::vnode_t::CHR): {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является изменением
					case static_cast <uint8_t> (event::action_t::CHANGE):
						// Записываем в лог сообщение о изменении события
						log.print("Тип узла события: Символьный узел устройства добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						log.print("Тип узла события: Символьный узел устройства удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
				}
			} break;
			case static_cast <uint8_t> (event::vnode_t::BLK): {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является изменением
					case static_cast <uint8_t> (event::action_t::CHANGE):
						// Записываем в лог сообщение о изменении события
						log.print("Тип узла события: Блочный узел устройства добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						log.print("Тип узла события: Блочный узел устройства удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
				}
			} break;
			// Если тип узла является каналом FIFO
			case static_cast <uint8_t> (event::vnode_t::FIFO): {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является изменением
					case static_cast <uint8_t> (event::action_t::CHANGE):
						// Записываем в лог сообщение о изменении события
						log.print("Тип узла события: Канал FIFO добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						log.print("Тип узла события: Канал FIFO удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
				}
			} break;
			// Если тип узла является сокетом
			case static_cast <uint8_t> (event::vnode_t::SOCK): {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является изменением
					case static_cast <uint8_t> (event::action_t::CHANGE):
						// Записываем в лог сообщение о изменении события
						log.print("Тип узла события: Сокет добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						log.print("Тип узла события: Сокет удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
				}
			} break;
			// Если тип узла является файлом
			case static_cast <uint8_t> (event::vnode_t::FILE): {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является изменением
					case static_cast <uint8_t> (event::action_t::CHANGE):
						// Записываем в лог сообщение о изменении события
						log.print("Тип узла события: Файл добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						log.print("Тип узла события: Файл удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						log.print("Тип узла события: Файл переименован, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						log.print("Тип узла события: Файл атрибуты изменены, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа к событию
						log.print("Тип узла события: Файл доступ отозван, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (event::action_t::HDLINK):
						// Записываем в лог сообщение об изменении счётчика жёстких ссылок
						log.print("Тип узла события: Файл счётчик жёстких ссылок изменён, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
				}
			} break;
			// Если тип узла является каталогом
			case static_cast <uint8_t> (event::vnode_t::DIR): {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является изменением
					case static_cast <uint8_t> (event::action_t::CHANGE):
						// Записываем в лог сообщение о изменении события
						log.print("Тип узла события: Каталог добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (event::action_t::DELETE):
						// Записываем в лог сообщение о типе узла события
						log.print("Тип узла события: Каталог удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						log.print("Тип узла события: Каталог переименован, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						log.print("Тип узла события: Каталог атрибуты изменены, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа к событию
						log.print("Тип узла события: Каталог доступ отозван, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (event::action_t::HDLINK):
						// Записываем в лог сообщение об изменении счётчика жёстких ссылок
						log.print("Тип узла события: Каталог счётчик жёстких ссылок изменён, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
				}
			} break;
			// Если тип узла является символической ссылкой
			case static_cast <uint8_t> (event::vnode_t::LINK): {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является изменением
					case static_cast <uint8_t> (event::action_t::CHANGE):
						// Записываем в лог сообщение о изменении события
						log.print("Тип узла события: Символическая ссылка добавлена, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (event::action_t::DELETE):
						// Записываем в лог сообщение о типе узла события
						log.print("Тип узла события: Символическая ссылка удалена, Путь=%s", log_t::flag_t::INFO, path.c_str());
					break;
				}
			} break;
		}
	}, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
	// Устанавливаем адреса событий файловой системы
	if(fs.setAddress(did, "../tmp") && fs.setAddress(fid, "../README2.md")){
		// Устананавливаем опции события
		if(fs.setOptions(fid, event::options::AUTO_FOLLOW))
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Успешно установлены опции события!" << endl;
		// Записываем ошибку в лог установки опций события
		else cout << " Ошибка установки опций события!" << endl;
		// Запускаем работу события уведомителя
		fs.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
