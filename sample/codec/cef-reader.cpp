/**
 * @file cef-reader.cpp
 * @date 2026-09-05
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
 * @brief Пример потокового чтения записей CEF — подача текста кусками произвольного
 *        размера, перебор выдаваемых событий разбора и разбор потока записей, поданных
 *        одна за другой, без удержания события целиком в памяти
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <cstring>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/reader.hpp>
#include <sys/log.hpp>

/**
 * @brief Пространство имён образца
 *
 */
namespace {
	/**
	 * @brief Функция получения объекта фреймворка
	 *
	 * @details Кодек связку берёт конструктором, а построения образца стоят и вне
	 *          main(): объект заводится статикою местною, дабы всякое построение
	 *          образца работало с одним и тем же фреймворком
	 *
	 * @return объект фреймворка
	 *
	 */
	const awh::fmk_t * framework() noexcept {
		// Объект фреймворка
		static awh::fmk_t fmk;
		// Выводим объект фреймворка
		return &fmk;
	}
	/**
	 * @brief Функция получения объекта для работы с логами
	 *
	 * @return объект для работы с логами
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект для работы с логами
		static awh::log_t log(::framework());
		// Выводим объект для работы с логами
		return &log;
	}
}

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Разбираемый поток записей
 *
 * @details Первая запись несёт приставку syslog, метки имён и адреса сети, вторая -
 *          отмену знаков в значении: и черта, и знак равенства внутри значения
 *          отменяются косой чертой, а в заголовке отменяется лишь черта
 *
 */
static const char * TEXT =
	"Feb 17 15:30:15 vnetids emerg CEF:0|InfoTeCS|IDS|2.4.3|1:905590:7|RDP connection|1|"
	"cat=1 src=192.168.59.39 spt=8082 dst=10.0.0.1 cs1=not-suspicious cs1Label=IDSClass "
	"rt=Feb 17 2023 23:30:15.734 YEKT\n"
	"CEF:0|Check Point|VPN-1|Check Point|Log|cp_udp_85FA60B6|Unknown|"
	"act=Drop msg=connection dropped\\= reason\\|early ifname=bond5\n";

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Блокируем неиспользуемую переменную
	(void) argc;
	// Блокируем неиспользуемую переменную
	(void) argv;
	// Создаём объект потокового чтения записей
	codec::cef::reader_t reader(::framework(), ::logger());
	// Настройки потокового чтения записей
	codec::cef::reader_t::settings_t settings;
	// Устанавливаем приём приставки syslog перед заголовком записи
	settings.syslog = true;
	// Устанавливаем снятие отмены знаков со значений расширения
	settings.unescape = true;
	// Устанавливаем настройки потокового чтения записей
	reader.settings(settings);
	// Размер подаваемого куска текста
	const size_t step = 64;
	// Смещение подачи текста записей
	size_t offset = 0;
	// Длина разбираемого текста записей
	const size_t length = ::strlen(TEXT);
	/**
	 * Выполняем подачу текста записей кусками, пока он не исчерпан
	 *
	 * @note Разбор от нарезки на куски не зависит: события выдаются в тех же местах,
	 *       каким бы размером кусок ни подавался, - хоть по одному октету
	 */
	do {
		// Получаем размер очередного куска подаваемого текста
		const size_t size = ::std::min(step, length - offset);
		/**
		 * Если подача очередного куска текста отказом завершилась
		 *
		 * @note Признак конца подачи выставляется на последнем куске: без него читатель
		 *       ждёт продолжения записи и последнего события не выдаёт
		 */
		if(!reader.feed(TEXT + offset, size, (offset + size) >= length)){
			// Выводим сведения об обнаруженной ошибке разбора
			cout << "ошибка: " << codec::cef::message(reader.error())
			     << " в " << reader.errorPosition().line << ":" << reader.errorPosition().column << endl;
			// Выводим код выхода из приложения с ошибкой
			return EXIT_FAILURE;
		}
		// Сдвигаем смещение подачи текста записей
		offset += size;
		/**
		 * Выполняем перебор всех событий разбора, поданным куском выданных
		 */
		while(reader.next()){
			/**
			 * Определяем разновидность выданного события разбора
			 */
			switch(static_cast <uint8_t> (reader.event())){
				// Если выдана приставка syslog
				case static_cast <uint8_t> (codec::cef::event_t::SYSLOG):
					// Выводим приставку syslog записи
					cout << "syslog: " << reader.value() << endl;
				break;
				// Если выдано очередное поле заголовка записи
				case static_cast <uint8_t> (codec::cef::event_t::HEADER): {
					/**
					 * Определяем разновидность выданного поля заголовка
					 *
					 * @note Число разновидности поля и есть порядок его в заголовке:
					 *       заголовок читается слева направо и полей несёт ровно семь
					 */
					switch(static_cast <uint8_t> (reader.field())){
						// Если выдана степень важности события
						case static_cast <uint8_t> (codec::cef::field_t::SEVERITY):
							// Выводим степень важности события числом
							cout << "  важность: " << reader.severity()
							     << " (" << reader.value() << ")" << endl;
						break;
						// Если выдана редакция описания записи
						case static_cast <uint8_t> (codec::cef::field_t::VERSION):
							// Выводим редакцию описания записи числом
							cout << "  редакция: " << reader.version() << endl;
						break;
						// Если выдано прочее поле заголовка записи
						default:
							// Выводим значение очередного поля заголовка
							cout << "  заголовок: " << reader.value() << endl;
					}
				} break;
				// Если выдана очередная пара расширения записи
				case static_cast <uint8_t> (codec::cef::event_t::EXTENSION):
					// Выводим имя и значение очередной пары расширения
					cout << "  " << reader.key() << " = " << reader.value() << endl;
				break;
				// Если выдан конец записи
				case static_cast <uint8_t> (codec::cef::event_t::RECORD):
					// Выводим обозначение конца очередной записи
					cout << "-- запись прочитана, строка " << reader.position().line << endl;
				break;
				// Если выдан конец потока записей
				case static_cast <uint8_t> (codec::cef::event_t::FINISH):
					// Выводим обозначение конца потока записей
					cout << "== поток исчерпан ==" << endl;
				break;
			}
		}
	} while(offset < length);
	/**
	 * Если чтение потока записей отказом завершилось
	 *
	 * @note Состояние читателя и есть итог разбора: FINISHED значит, что поток прочитан
	 *       до конца, а HUNGRY - что читатель ждёт продолжения незавершённой записи
	 */
	if(reader.state() == codec::cef::state_t::FAILED){
		// Выводим сведения об обнаруженной ошибке разбора
		cout << "ошибка: " << codec::cef::message(reader.error()) << endl;
		// Выводим код выхода из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Выводим код успешного выхода из приложения
	return EXIT_SUCCESS;
}
