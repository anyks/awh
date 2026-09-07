/**
 * @file syslog-reader.cpp
 * @date 2026-09-07
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
 * @brief Образец потокового чтения сообщений системного журнала — подачи текста кусками и
 *        выдачи полей заголовка, структурированных данных и текста сообщения событиями порознь
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/syslog/reader.hpp>
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
 * @details Записи обоих описаний идут вперемешку: сборщик журналов принимает поток от
 *          устройств разных поколений разом, и описание всякой записи опознаётся
 *          порознь. Третья записи приставки приоритета не несёт вовсе - живые
 *          устройства шлют и такие, и отвергать их значило бы терять события
 *
 */
static const char * TEXT =
	"<45>Oct 22 12:34:56 freebsd-log syslog-ng[8763]: [notice]syslog-ng starting up; version='4.7.1'\n"
	"<165>1 2023-04-11T23:29:33.003Z mymachine.example.com evntslog 1093 ID47 "
	"[exampleSDID@32473 iut=\"3\" eventSource=\"Application\"][examplePriority@32473 class=\"high\"] "
	"An application event log entry\n"
	"Oct 22 10:52:01 scapegoat.dmz.example.org sched[222]: That's All Folks!\n"
	"<13>Sat Jan  8 20:07:41 2011 myhostname myapp[1234]: This is a sample syslog message.\n";

/**
 * @brief Функция получения имени поля заголовка
 *
 * @param field поле заголовка записи
 * @return      имя поля заголовка
 *
 */
static const char * name(const codec::syslog::field_t field) noexcept {
	/**
	 * Определяем поле заголовка записи
	 */
	switch(static_cast <uint8_t> (field)){
		// Если полем является номер описания записи
		case static_cast <uint8_t> (codec::syslog::field_t::VERSION): return "версия";
		// Если полем является дата сообщения
		case static_cast <uint8_t> (codec::syslog::field_t::TIMESTAMP): return "дата";
		// Если полем является имя узла
		case static_cast <uint8_t> (codec::syslog::field_t::HOSTNAME): return "узел";
		// Если полем является название приложения
		case static_cast <uint8_t> (codec::syslog::field_t::APPLICATION): return "приложение";
		// Если полем является опознаватель работы
		case static_cast <uint8_t> (codec::syslog::field_t::PROCESS): return "работа";
		// Если полем является опознаватель сообщения
		case static_cast <uint8_t> (codec::syslog::field_t::MESSAGE_ID): return "опознаватель";
	}
	// Выводим имя неопределённого поля заголовка
	return "поле";
}

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код полученного результата
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Отключаем неиспользуемые переменные
	(void) argc;
	(void) argv;
	// Объект потокового чтения записей
	codec::syslog::reader_t reader(::framework(), ::logger());
	// Настройки разбора записей
	codec::syslog::reader_t::settings_t settings;
	/**
	 * Устанавливаем строгое сличение разбираемой записи с описанием
	 *
	 * @note Строгость сличает вид полей, длины и набор знаков имён. Без неё поля
	 *       кладутся знаками как есть, и запись, описанию не отвечающая, разбирается
	 *       молча - что для приёма от чужих устройств бывает и нужно
	 */
	settings.mode = codec::syslog::mode_t::STRONG;
	// Устанавливаем настройки разбора записей
	reader.settings(settings);
	// Длина разбираемого потока записей
	const size_t length = ::strlen(TEXT);
	// Смещение подачи текста записей
	size_t offset = 0;
	/**
	 * Выполняем подачу текста записей кусками
	 *
	 * @note Размер куска взят нарочно неудобным: обрыв приходится и на середину даты,
	 *       и на середину значения структурированных данных. Разбор от нарезки НЕ
	 *       зависит - ряд событий выходит один и тот же при любом размере куска
	 */
	while(offset < length){
		// Получаем размер очередного куска подаваемого текста
		const size_t size = ((offset + 37) < length ? 37 : (length - offset));
		// Выполняем подачу очередного куска текста записей
		reader.feed(TEXT + offset, size, (offset + size) >= length);
		// Сдвигаем смещение подачи текста записей
		offset += size;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Определяем вид события разбора
			 */
			switch(static_cast <uint8_t> (reader.event())){
				// Если событием является поле заголовка записи
				case static_cast <uint8_t> (codec::syslog::event_t::HEADER):
					// Выводим поле заголовка записи
					cout << "  " << ::name(reader.field()) << ": " << reader.value() << endl;
				break;
				// Если событием является опознаватель блока структурированных данных
				case static_cast <uint8_t> (codec::syslog::event_t::STRUCTURE):
					// Выводим опознаватель блока структурированных данных
					cout << "  блок [" << reader.key() << "]" << endl;
				break;
				// Если событием является поле структурированных данных
				case static_cast <uint8_t> (codec::syslog::event_t::PARAM):
					// Выводим поле блока структурированных данных
					cout << "    " << reader.key() << " = " << reader.value() << endl;
				break;
				// Если событием является текст сообщения
				case static_cast <uint8_t> (codec::syslog::event_t::MESSAGE):
					// Выводим текст сообщения
					cout << "  текст: " << reader.value() << endl;
				break;
				// Если событием является окончание записи
				case static_cast <uint8_t> (codec::syslog::event_t::RECORD): {
					// Выводим описание, каким запись прочтена
					cout << "  == описание: "
					     << ((reader.standard() == codec::syslog::standard_t::RFC5424) ? "RFC 5424" : "RFC 3164");
					/**
					 * Если приоритет записью объявлен
					 *
					 * @note Объявленность спрашивается ходом `prioritized`, а не сличением
					 *       приоритета с нулём: ноль есть законный приоритет - сообщение
					 *       ядра наивысшей важности
					 */
					if(reader.prioritized())
						// Выводим приоритет записи с источником и важностью
						cout << ", приоритет " << reader.priority()
						     << " (" << codec::syslog::name(reader.facility())
						     << "." << codec::syslog::name(reader.severity()) << ")";
					// Если приоритет записью не объявлен
					else cout << ", приоритет не объявлен";
					// Завершаем вывод сведений о записи
					cout << endl << endl;
				} break;
			}
		}
	}
	/**
	 * Если разбор потока записей прекращён ошибкой
	 */
	if(reader.state() == codec::syslog::state_t::FAILED)
		// Выводим сообщение об ошибке разбора с местом её обнаружения
		cout << "ОТКАЗ: " << codec::syslog::message(reader.error())
		     << " в строке " << reader.errorPosition().line
		     << ", столбце " << reader.errorPosition().column << endl;
	// Выводим результат работы приложения
	return EXIT_SUCCESS;
}
