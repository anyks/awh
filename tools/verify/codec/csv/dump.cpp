/**
 * @file dump.cpp
 * @date 2026-08-13
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
 * @brief Средство выдачи разобранной таблицы в виде, пригодном к сличению с эталоном
 *
 * @details Таблица выдаётся записью JSON из перечня записей, чтобы сличение читало её
 *          одним разбирателем. Второй довод задаёт размер куска подачи: при нуле текст
 *          подаётся читателю целиком, иначе - частями, и разбор обязан сойтись к тому же
 *          перечню. Третий довод `header` включает разбор первой записи заголовком, и
 *          имена его полей выдаются первой записью
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <codec/csv/csv.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в
 *       одно, порождая порчу вдали от места её причины
 *
 */
namespace {
	/**
	 * @brief Объект журнала проверок с отключённым выводом
	 *
	 * @details Вывод отключается назначением пустого перечня приёмников: отказы
	 *          разбора проверки наводят намеренно, и журнал их засорял бы выдачу
	 *
	 */
	struct Silent {
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept {
			// Выполняем отключение вывода логов
			awh::log::mode({});
		}
	};
}

using namespace std;

/**
 * Подписываемся на пространство имён кодеков
 */
using namespace awh::codec;

// Выполняем запись содержимого поля с отменой опасных знаков
static void quote(const string_view text){
	::fputc('"', stdout);
	/**
	 * Выполняем перебор всех знаков записываемого поля
	 */
	for(const unsigned char letter : text){
		/**
		 * Выполняем выбор записи знака по его роду
		 */
		switch(letter){
			case '"': ::fputs("\\\"", stdout); break;
			case '\\': ::fputs("\\\\", stdout); break;
			case '\n': ::fputs("\\n", stdout); break;
			case '\r': ::fputs("\\r", stdout); break;
			case '\t': ::fputs("\\t", stdout); break;
			default: {
				// Если знак является управляющим, выдаём его отменяющей последовательностью
				if(letter < 0x20)
					::printf("\\u%04x", letter);
				else ::fputc(letter, stdout);
			}
		}
	}
	::fputc('"', stdout);
}

int main(int argc, char * argv[]){
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 *
	 */
	awh::fmk::initialize();
	/**
	 * Выполняем отключение выдачи журнала СРАЗУ ПОСЛЕ заведения модуля
	 *
	 * @note Порядок этот и есть смысл: `initialize` назначает приёмники сами, и
	 *       отключение, поставленное до него, оказалось бы перезаписанным. Без него щуп
	 *       на отказе выдаёт в stdout баннер с записью журнала перед строками таблицы,
	 *       и сличение reading этот поток как JSON отвечало «Expecting value» на всяком
	 *       намеренном обрыве
	 *
	 */
	Silent silent;
	// Если путь к разбираемому файлу не задан
	if(argc < 2)
		// Выходим из приложения с отказом
		return 1;
	// Читаем содержимое файла таблицы
	ifstream file(argv[1], ios::binary);
	// Хранилище содержимого разбираемого файла
	stringstream buffer;
	// Выполняем считывание содержимого файла в хранилище
	buffer << file.rdbuf();
	// Разбираемый текст таблицы
	const string text = buffer.str();
	// Размер куска подачи, ноль означает подачу целиком
	const size_t chunk = ((argc > 2) ? static_cast <size_t> (::atoi(argv[2])) : 0);
	/**
	 * Признак разбора первой записи заголовком таблицы
	 *
	 * @note Режим этот сличается с `csv.DictReader` эталона, тогда как обычный - с
	 *       `csv.reader`: ветвь заголовка прежде не сличалась с эталоном вовсе, и
	 *       правила её - пустое имя, повторное имя, отсутствие заголовка - держались
	 *       на одних лишь собственных проверках
	 *
	 */
	const bool heading = ((argc > 3) && (string(argv[3]) == "header"));
	// Настройки разбора текста
	csv::reader_t::settings_t settings;
	// Устанавливаем признак разбора первой записи заголовком таблицы
	settings.header = (heading ? csv::header_t::PRESENT : csv::header_t::NONE);
	// Объект чтения текста таблицы
	csv::reader_t reader(settings);
	// Собираемые имена полей заголовка таблицы
	vector <string> heads;
	// Записи разобранной таблицы
	vector <vector <string>> records;
	// Поля собираемой записи таблицы
	vector <string> record;
	// Смещение начала подаваемого читателю куска
	size_t offset = 0;
	/**
	 * Выполняем подачу текста читателю кусками до самого конца
	 */
	do {
		// Выполняем расчёт размера подаваемого куска
		const size_t size = ((chunk == 0) ? (text.size() - offset) : min(chunk, text.size() - offset));
		// Выполняем подачу очередного куска читателю
		reader.feed(text.data() + offset, size, ((offset + size) >= text.size()));
		/**
		 * Выполняем перебор всех событий разбора поданного куска
		 */
		while(reader.next()){
			/**
			 * Выполняем обработку события по его роду
			 */
			switch(static_cast <uint8_t> (reader.event())){
				case static_cast <uint8_t> (csv::event_t::HEADER):
					// Заносим очередное имя поля заголовка таблицы
					heads.push_back(string(reader.field().value));
				break;
				case static_cast <uint8_t> (csv::event_t::FIELD):
					record.push_back(string(reader.field().value));
				break;
				case static_cast <uint8_t> (csv::event_t::RECORD): {
					// Запись из единственного пустого поля отбрасывается наравне с эталоном:
					// эталон пустую строку от записи «""» не отличает вовсе
					//
					// Запись без единого поля отбрасывается тоже: событие окончания записи
					// приходит и по строке заголовка, чьи поля выданы событиями HEADER
					if(!record.empty() && !((record.size() == 1) && record.front().empty()))
						// Выполняем занесение собранной записи в перечень
						records.push_back(record);
					// Выполняем очистку полей собранной записи
					record.clear();
				} break;
			}
		}
		// Выполняем сдвиг смещения на размер поданного куска
		offset += size;
	} while(offset < text.size());
	/**
	 * Если разбор вёлся с заголовком, выдаём имена его первой записью
	 *
	 * @note Эталон имена эти отдаёт полем `fieldnames`, а записи - отображениями:
	 *       сличать удобнее одинаковым видом, и сличение разворачивает отображения
	 *       обратно в перечни
	 *
	 */
	if(heading && !heads.empty())
		// Выполняем вставку имён полей заголовка первой записью
		records.insert(records.begin(), heads);
	::fputc('[', stdout);
	/**
	 * Выполняем перебор всех записей разобранной таблицы
	 */
	for(size_t i = 0; i < records.size(); i++){
		// Если запись не первая, выдаём разделитель перечня
		if(i > 0)
			::fputc(',', stdout);
		::fputc('[', stdout);
		/**
		 * Выполняем перебор всех полей очередной записи
		 */
		for(size_t j = 0; j < records.at(i).size(); j++){
			// Если поле не первое, выдаём разделитель перечня
			if(j > 0)
				::fputc(',', stdout);
			// Выполняем запись очередного поля с отменой опасных знаков
			quote(records.at(i).at(j));
		}
		::fputc(']', stdout);
	}
	::fputs("]\n", stdout);
	/**
	 * Если разбор таблицы завершился ошибкой
	 */
	if(reader.error() != csv::error_t::NONE){
		::fprintf(stderr, "%s\n", csv::message(reader.error()));
		// Выходим из приложения с кодом отказа разбора
		return 2;
	}
	// Выводим признак успешного завершения
	return 0;
}
