/**
 * @file reader.cpp
 * @date 2026-09-04
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
 * @brief Автоматические тесты потокового чтения записей CEF — отделения приставки syslog,
 *        разбора полей заголовка, отмены знаков порознь по областям записи, пустых значений,
 *        повторяющихся ключей, отклонения неправильного построения и подачи текста кусками
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/cef.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений AWH
 */
#include <sys/macro/suppress.hpp>
#include <sys/log.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Метод разбора записей в слепок потока событий
 *
 * @details Слепок собирается строкой ради сличения целиком: сравнение потока событий
 * знак в знак ловит и лишнее событие, и его недостачу, чего проверка отдельных полей
 * не даёт
 *
 * @param text     разбираемый текст записей
 * @param settings настройки разбора записей
 * @param step     размер куска подаваемого текста, нулевой для подачи целиком
 * @return         слепок потока событий разбора
 *
 */
static string dumpCef(const string & text, const cef::reader_t::settings_t & settings, const size_t step = 0) noexcept {
	// Объект потокового чтения записей
	cef::reader_t reader;
	// Устанавливаем настройки разбора записей
	reader.settings(settings);
	// Собираемый слепок потока событий разбора
	string result;
	// Смещение подачи текста записей
	size_t offset = 0;
	/**
	 * Выполняем подачу текста записей, пока он не исчерпан
	 */
	do {
		// Получаем размер очередного куска подаваемого текста
		const size_t size = ((step == 0) ? (text.size() - offset) : ::std::min(step, text.size() - offset));
		// Выполняем подачу очередного куска текста записей
		reader.feed(text.data() + offset, size, (offset + size) >= text.size());
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
				// Если событием является приставка syslog
				case static_cast <uint8_t> (cef::event_t::SYSLOG):
					// Добавляем приставку syslog в слепок
					result.append("S{").append(reader.value()).append("}");
				break;
				// Если событием является поле заголовка записи
				case static_cast <uint8_t> (cef::event_t::HEADER):
					// Добавляем поле заголовка в слепок
					result.append("H{").append(reader.value()).append("}");
				break;
				// Если событием является пара расширения
				case static_cast <uint8_t> (cef::event_t::EXTENSION):
					// Добавляем пару расширения в слепок
					result.append("E{").append(reader.key()).append("=").append(reader.value()).append("}");
				break;
				// Если событием является окончание записи
				case static_cast <uint8_t> (cef::event_t::RECORD):
					// Добавляем окончание записи в слепок
					result.append("R;");
				break;
			}
		}
		// Если разбор прекращён ошибкой
		if(reader.state() == cef::state_t::FAILED){
			// Добавляем код отказа разбора в слепок
			result.append("F{").append(::std::to_string(static_cast <uint32_t> (reader.error()))).append("}");
			// Выходим из цикла подачи текста
			break;
		}
	} while(offset < text.size());
	// Выводим слепок потока событий разбора
	return result;
}

/**
 * @brief Проверка разбора записи с приставкой syslog
 *
 */
TEST(CodecCefReader, Syslog) {
	// Настройки разбора записей
	const cef::reader_t::settings_t settings;
	// Выполняем проверку разбора записи с приставкой syslog
	EXPECT_EQ(
		::dumpCef("Feb 17 15:30:15 vnetids emerg CEF:0|InfoTeCS|IDS|2.4|1:9|ET POLICY|1|src=1.2.3.4", settings),
		"S{Feb 17 15:30:15 vnetids emerg}H{0}H{InfoTeCS}H{IDS}H{2.4}H{1:9}H{ET POLICY}H{1}E{src=1.2.3.4}R;"
	);
	// Выполняем проверку разбора записи без приставки syslog
	EXPECT_EQ(
		::dumpCef("CEF:0|InfoTeCS|IDS|2.4|1:9|ET POLICY|1|src=1.2.3.4", settings),
		"H{0}H{InfoTeCS}H{IDS}H{2.4}H{1:9}H{ET POLICY}H{1}E{src=1.2.3.4}R;"
	);
}

/**
 * @brief Проверка независимости разбора от нарезки текста на куски
 *
 * @details Обрыв куска допустим в любом месте, в том числе посреди имени ключа и
 *          посреди отменяющей последовательности; события выдаются те же и в тех же
 *          местах, что и при подаче текста целиком
 *
 */
TEST(CodecCefReader, Chunked) {
	// Настройки разбора записей
	const cef::reader_t::settings_t settings;
	// Разбираемый текст записей
	const string text = R"(Feb 17 15:30:15 host CEF:0|Vendor|Product|1.0|100|detected a \| in message|10|src=10.0.0.1 act=blocked a | msg=Detected.\nNo action. dst=1.1.1.1
CEF:0|A|B|C|D|E|F|cs3= cs3Label=CVEID)" "\n";
	// Получаем слепок разбора текста, поданного целиком
	const string expected = ::dumpCef(text, settings);
	/**
	 * Выполняем перебор размеров куска подаваемого текста
	 */
	for(const size_t step : {static_cast <size_t> (1), static_cast <size_t> (2), static_cast <size_t> (3), static_cast <size_t> (7), static_cast <size_t> (16), static_cast <size_t> (64)})
		// Выполняем проверку совпадения слепка разбора текста, поданного кусками
		EXPECT_EQ(::dumpCef(text, settings, step), expected) << "размер куска: " << step;
}

/**
 * @brief Проверка отмены знаков порознь по областям записи
 *
 * @details Описание ArcSight требует отменять в заголовке прямую черту и обратную
 *          косую, а в расширении - знак равенства, и прямо оговаривает, что черта и
 *          косая в расширении отмены не требуют. Свод обеих областей к одному правилу
 *          разбирал бы неверно обе
 *
 */
TEST(CodecCefReader, Escaping) {
	// Настройки разбора записей
	const cef::reader_t::settings_t settings;
	// Выполняем проверку отмены прямой черты в заголовке и её отсутствия в расширении
	EXPECT_EQ(
		::dumpCef(R"(CEF:0|security|threatmanager|1.0|100|detected a \| in message|10|src=10.0.0.1 act=blocked a | dst=1.1.1.1)", settings),
		"H{0}H{security}H{threatmanager}H{1.0}H{100}H{detected a | in message}H{10}E{src=10.0.0.1}E{act=blocked a |}E{dst=1.1.1.1}R;"
	);
	// Выполняем проверку отмены обратной косой в заголовке и её отсутствия в расширении
	EXPECT_EQ(
		::dumpCef(R"(CEF:0|security|threatmanager|1.0|100|detected a \\ in packet|10|src=10.0.0.1 action=blocked a \ dst=1.1.1.1)", settings),
		"H{0}H{security}H{threatmanager}H{1.0}H{100}H{detected a \\ in packet}H{10}E{src=10.0.0.1}E{action=blocked a \\}E{dst=1.1.1.1}R;"
	);
	// Выполняем проверку отмены знака равенства в расширении
	EXPECT_EQ(
		::dumpCef(R"(CEF:0|A|B|C|D|E|1|originsicname=CN\=chr-cpsg-01,O\=stal dst=1.1.1.1)", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{originsicname=CN=chr-cpsg-01,O=stal}E{dst=1.1.1.1}R;"
	);
	// Выполняем проверку отмены перевода строки в значении расширения
	EXPECT_EQ(
		::dumpCef(R"(CEF:0|A|B|C|D|E|1|msg=Detected a threat.\nNo action needed.)", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{msg=Detected a threat.\nNo action needed.}R;"
	);
	// Выполняем проверку сохранения обратной косой перед знаком, отмене не подлежащим
	EXPECT_EQ(
		::dumpCef(R"(CEF:0|A|B|C|D|E|1|fileName=c:\Program Files\ArcSight)", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{fileName=c:\\Program Files\\ArcSight}R;"
	);
}

/**
 * @brief Проверка разбора пустых значений расширения
 *
 * @details Запись «cs3=» описанием не оговорена вовсе, но в живых журналах обычна и
 *          образует пару с меткой «cs3Label=CVEID». Разбор держит её полноценной
 *          записью с пустым значением
 *
 */
TEST(CodecCefReader, EmptyValue) {
	// Настройки разбора записей
	const cef::reader_t::settings_t settings;
	// Выполняем проверку разбора пустого значения посреди записи
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|cs3= cs3Label=CVEID cs5= cs5Label=IDSTags", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{cs3=}E{cs3Label=CVEID}E{cs5=}E{cs5Label=IDSTags}R;"
	);
	// Выполняем проверку разбора пустого значения в конце записи
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|src=1.2.3.4 msg=", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{src=1.2.3.4}E{msg=}R;"
	);
}

/**
 * @brief Проверка разбора значений, пробелы несущих
 *
 * @details Концом значения служит начало следующей пары, а не первый же пробел:
 *          описание прямо дозволяет пробел внутри значения
 *
 */
TEST(CodecCefReader, SpacedValue) {
	// Настройки разбора записей
	const cef::reader_t::settings_t settings;
	// Выполняем проверку разбора значения, пробелы несущего
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|rt=Feb 17 2023 23:30:15.734 YEKT smac=eb:11:0e:37:28:65", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{rt=Feb 17 2023 23:30:15.734 YEKT}E{smac=eb:11:0e:37:28:65}R;"
	);
	// Выполняем проверку разбора последнего значения, весь остаток занимающего
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|msg=HTTPS post request from 188.43.251.186:59420 to 10.77.194.51:80", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{msg=HTTPS post request from 188.43.251.186:59420 to 10.77.194.51:80}R;"
	);
}

/**
 * @brief Проверка разбора повторяющихся ключей расширения
 *
 * @details Повтор ключа в живых журналах настоящий, и чтение выдаёт всякое его
 *          появление своим событием, порядок следования сохраняя
 *
 */
TEST(CodecCefReader, DuplicateKeys) {
	// Настройки разбора записей
	const cef::reader_t::settings_t settings;
	// Выполняем проверку разбора повторяющихся ключей расширения
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|ad.prog-id=128394 ad.prog-id=128394 ad.prog-id=555 deviceExternalId=1 deviceExternalId=2", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{ad.prog-id=128394}E{ad.prog-id=128394}E{ad.prog-id=555}E{deviceExternalId=1}E{deviceExternalId=2}R;"
	);
}

/**
 * @brief Проверка разбора потока из многих записей
 *
 */
TEST(CodecCefReader, Stream) {
	// Настройки разбора записей
	const cef::reader_t::settings_t settings;
	// Выполняем проверку разбора потока из трёх записей
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|a=1\nCEF:0|A|B|C|D|E|2|b=2\r\nCEF:0|A|B|C|D|E|3|c=3\n", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{a=1}R;H{0}H{A}H{B}H{C}H{D}H{E}H{2}E{b=2}R;H{0}H{A}H{B}H{C}H{D}H{E}H{3}E{c=3}R;"
	);
	// Выполняем проверку пропуска пустых строк между записями
	EXPECT_EQ(
		::dumpCef("\n\nCEF:0|A|B|C|D|E|1|a=1\n\n   \nCEF:0|A|B|C|D|E|2|b=2\n", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{a=1}R;H{0}H{A}H{B}H{C}H{D}H{E}H{2}E{b=2}R;"
	);
}

/**
 * @brief Проверка отклонения неправильного построения записи
 *
 */
TEST(CodecCefReader, Failures) {
	// Настройки разбора записей
	cef::reader_t::settings_t settings;
	// Выполняем проверку отклонения записи без слова «CEF:»
	EXPECT_EQ(
		::dumpCef("Feb 17 15:30:15 vnetids emerg src=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::MISSING_SIGNATURE)) + "}"
	);
	// Выполняем проверку отклонения записи с неполным заголовком
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|src=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::INCOMPLETE_HEADER)) + "}"
	);
	// Выполняем проверку отклонения записи с ошибочным номером редакции
	EXPECT_EQ(
		::dumpCef("CEF:x|A|B|C|D|E|1|src=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::INVALID_VERSION)) + "}"
	);
	// Отключаем признание приставки syslog перед словом «CEF:»
	settings.syslog = false;
	// Выполняем проверку отклонения приставки syslog при выключенном её признании
	EXPECT_EQ(
		::dumpCef("Feb 17 15:30:15 host CEF:0|A|B|C|D|E|1|src=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::MISSING_SIGNATURE)) + "}"
	);
}

/**
 * @brief Проверка выдачи полей заголовка порознь и по счёту
 *
 */
TEST(CodecCefReader, HeaderFields) {
	// Объект потокового чтения записей
	cef::reader_t reader;
	// Выполняем подачу записи целиком
	ASSERT_TRUE(reader.feed("CEF:0|InfoTeCS|IDS|2.4.3|1:905590:7|ET POLICY|7|src=1.2.3.4"));
	// Ожидаемые значения полей заголовка записи
	const string expected[] = {"0", "InfoTeCS", "IDS", "2.4.3", "1:905590:7", "ET POLICY", "7"};
	// Номер очередного поля заголовка записи
	size_t index = 0;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		// Если событием является поле заголовка записи
		if(reader.event() == cef::event_t::HEADER){
			// Выполняем проверку номера поля заголовка записи
			EXPECT_EQ(static_cast <size_t> (reader.field()), index);
			// Выполняем проверку значения поля заголовка записи
			EXPECT_EQ(reader.value(), expected[index]);
			// Переходим к следующему полю заголовка записи
			index++;
		}
	}
	// Выполняем проверку количества полей заголовка записи
	EXPECT_EQ(index, static_cast <size_t> (cef::HEADER_FIELDS));
	// Выполняем проверку номера редакции записи
	EXPECT_EQ(reader.version(), 0u);
	// Выполняем проверку важности события записи
	EXPECT_EQ(reader.severity(), 7u);
}

/**
 * @brief Проверка выдачи окончания текста событием, а не признаком состояния
 *
 * @details Событие окончания объявлено перечнем наравне с прочими, а перебор ведётся
 *          циклом «покуда next()». Выставь его читатель при отказе - и потребителю оно
 *          не досталось бы вовсе, оставаясь мёртвым обещанием перечня
 *
 */
TEST(CodecCefReader, StreamFinishIsDeliveredAsAnEvent) {
	// Объект потокового чтения записей
	cef::reader_t reader;
	// Выполняем подачу записи целиком с признаком конца подачи
	ASSERT_TRUE(reader.feed("CEF:0|InfoTeCS|IDS|2.4.3|1:905590:7|ET POLICY|7|src=1.2.3.4"));
	// Количество выданных событий окончания текста
	size_t finishes = 0;
	// Вид последнего выданного события разбора
	cef::event_t last = cef::event_t::NONE;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		// Запоминаем вид очередного выданного события разбора
		last = reader.event();
		// Если событием является окончание текста
		if(last == cef::event_t::FINISH)
			// Наращиваем количество выданных событий окончания текста
			finishes++;
	}
	// Выполняем проверку того, что окончание текста выдано событием
	EXPECT_EQ(finishes, static_cast <size_t> (1));
	// Выполняем проверку того, что окончание текста выдано ПОСЛЕДНИМ событием
	EXPECT_EQ(last, cef::event_t::FINISH);
	// Выполняем проверку состояния читателя окончанием разбора
	EXPECT_EQ(reader.state(), cef::state_t::FINISHED);
	// Выполняем проверку того, что повторный спрос события отвечает отказом
	EXPECT_FALSE(reader.next());
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>

/**
 * @brief Проверка договора подачи текста и смены настроек
 *
 * @details Чтение ведётся потоком, и договор подачи стережёт три случая: подачу после
 *          отказа, подачу после последнего куска и смену настроек посреди разбора.
 *          Первые два отвечаются отказом, а не тихим принятием: текст, поданный за
 *          последним куском, разобран не будет вовсе, и молчаливое согласие обещало бы
 *          потребителю разбор, какого не случится
 *
 * @note Смена настроек посреди потока запрещена оттого, что записи, разными правилами
 *       прочтённые, ничем друг от друга не отличаются: потребитель не имел бы способа
 *       узнать, какая чем прочтена
 *
 * @warning Подача целиком - `feed(string_view)` - объявляет кусок ПОСЛЕДНИМ сама, и
 *          всякая следующая подача ей отвечалась бы отказом. Проверка потока обязана
 *          звать подачу тремя доводами, иначе поверяла бы не то
 *
 * @note Заведено 09.09.2026 сличением наборов двух своих кодеков: у syslog проверка эта
 *       стояла, у CEF её не было вовсе, при том что договор подачи у них общий и код
 *       уплотнения хранилища тоже
 *
 */
TEST(CodecCefReader, FeedContract) {
	// Годная запись событий безопасности
	constexpr string_view RECORD = "CEF:0|security|threatmanager|1.0|100|detected|10|src=10.0.0.1\n";
	// Объект потокового чтения записей
	cef::reader_t reader;
	// Настройки разбора записей
	cef::reader_t::settings_t settings;
	// Выполняем проверку того, что настройки принимаются до начала разбора
	EXPECT_TRUE(reader.settings(settings));
	// Выполняем подачу записи куском, последним не являющимся
	ASSERT_TRUE(reader.feed(RECORD.data(), RECORD.size(), false));
	// Выполняем проверку отказа смены настроек посреди разбора
	EXPECT_FALSE(reader.settings(settings));
	/**
	 * Выполняем перебор событий разбора записи
	 */
	while(reader.next()){}
	// Выполняем подачу последнего куска текста
	ASSERT_TRUE(reader.feed(nullptr, 0, true));
	// Выполняем проверку отказа подачи текста после последнего куска
	EXPECT_FALSE(reader.feed(RECORD.data(), RECORD.size(), false));
	/**
	 * Запись с приставкой, описанию не отвечающей
	 *
	 * @note Знак конца строки ставится непременно: без него и без последнего куска
	 *       запись завершённой не считается, и разбор её не начинается вовсе
	 */
	constexpr string_view BROKEN = "НЕ CEF ВОВСЕ|security|threatmanager\n";
	// Объект потокового чтения записей, отказом останавливаемый
	cef::reader_t failed;
	// Выполняем подачу записи, описанию не отвечающей
	ASSERT_TRUE(failed.feed(BROKEN.data(), BROKEN.size(), false));
	/**
	 * Выполняем перебор событий разбора записи
	 */
	while(failed.next()){}
	// Выполняем проверку того, что чтение отказом завершилось
	ASSERT_EQ(failed.state(), cef::state_t::FAILED);
	// Выполняем проверку отказа подачи текста чтению, отказом остановленному
	EXPECT_FALSE(failed.feed(RECORD.data(), RECORD.size(), false));
	// Объект потокового чтения записей для уплотнения хранилища
	cef::reader_t compact;
	// Количество прочтённых записей событий безопасности
	size_t records = 0;
	/**
	 * Выполняем подачу записей объёмом, порог уплотнения превышающим
	 *
	 * @note Порог уплотнения равен 64 КиБ, и записи подаются кусками, покуда разобранное
	 *       начало хранилища его не перерастёт: уплотнение снимает разобранное начало и
	 *       сбрасывает смещение разбора в нуль. Проверка на малом объёме ветвь эту не
	 *       заводит вовсе и зеленела бы, ничего не поверяя
	 */
	for(size_t i = 0; i < 2048; i++){
		// Выполняем подачу очередной записи событий безопасности
		ASSERT_TRUE(compact.feed(RECORD.data(), RECORD.size(), false));
		/**
		 * Выполняем перебор событий разбора поданных записей
		 */
		while(compact.next()){
			// Если событием является окончание записи
			if(compact.event() == cef::event_t::RECORD)
				// Выполняем счёт прочтённых записей
				records++;
		}
	}
	// Выполняем проверку того, что уплотнение хранилища записей не потеряло
	EXPECT_EQ(records, static_cast <size_t> (2048));
	// Выполняем проверку того, что чтение отказом не завершилось
	EXPECT_NE(compact.state(), cef::state_t::FAILED);
}

/**
 * @brief Проверка отклонения записей превышением пределов
 *
 * @details Пределы разбора стояли в чтении все до одного непроверенными: карта покрытия
 *          держала пустыми ветви FIELD_TOO_LONG, NAME_TOO_LONG, RECORD_TOO_LONG,
 *          OVERFLOW_LIMIT, EMPTY_KEY и INVALID_SEVERITY. Предел, поверкой не тронутый,
 *          от предела снятого неотличим
 *
 */
TEST(CodecCefReader, LimitFailures) {
	// Настройки разбора записей
	cef::reader_t::settings_t settings;
	// Выполняем проверку отклонения записи пустым номером редакции
	EXPECT_EQ(
		::dumpCef("CEF:|A|B|C|D|E|1|src=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::INVALID_VERSION)) + "}"
	);
	// Выполняем проверку отклонения записи неподдерживаемым номером редакции
	EXPECT_EQ(
		::dumpCef("CEF:2|A|B|C|D|E|1|src=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::UNSUPPORTED_VERSION)) + "}"
	);
	/**
	 * @note Важность события за пределом отвечается отказом лишь при строгом сличении:
	 *       при вольном она сбрасывается нулём, а запись выдаётся, ибо описание ArcSight
	 *       поле это словом тоже допускает
	 */
	// Выполняем проверку выдачи записи с важностью события за пределом при вольном сличении
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|11|src=1.2.3.4", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{11}E{src=1.2.3.4}R;"
	);
	// Устанавливаем строгое сличение ключей расширения со словарём
	settings.mode = cef::mode_t::STRONG;
	// Выполняем проверку приёма записи с важностью события ровно в предел
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|10|src=1.2.3.4", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{10}E{src=1.2.3.4}R;"
	);
	// Выполняем проверку отклонения записи превышением важности события
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|11|src=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::INVALID_SEVERITY)) + "}"
	);
	// Возвращаем вольное сличение ключей расширения со словарём
	settings.mode = cef::reader_t::settings_t().mode;
	// Выполняем проверку отклонения записи пустым именем ключа расширения
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::EMPTY_KEY)) + "}"
	);
	/**
	 * @note Всякий предел проверяется ОБОИМИ краями: запись ровно на пределе годна, и
	 *       лишь запись сверх него отвергается. Одного наблюдения мало - проверка «сверх
	 *       предела отказ» проходит и при заслоне, срабатывающем на единицу раньше. Так
	 *       09.09.2026 и вскрылась ошибка на единицу у предела числа пар расширения
	 */
	// Выполняем проверку приёма записи с полем заголовка длиною ровно в предел
	EXPECT_EQ(
		::dumpCef("CEF:0|" + string(cef::MAX_HEADER_FIELD, 'A') + "|B|C|D|E|1|src=1.2.3.4", settings).substr(0, 5),
		"H{0}H"
	);
	// Выполняем проверку отклонения записи превышением длины поля заголовка
	EXPECT_EQ(
		::dumpCef("CEF:0|" + string(cef::MAX_HEADER_FIELD + 1, 'A') + "|B|C|D|E|1|src=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::FIELD_TOO_LONG)) + "}"
	);
	// Выполняем проверку приёма записи с именем ключа длиною ровно в предел
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|" + string(cef::MAX_NAME, 'k') + "=1.2.3.4", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{" + string(cef::MAX_NAME, 'k') + "=1.2.3.4}R;"
	);
	// Выполняем проверку отклонения записи превышением длины имени ключа расширения
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|" + string(cef::MAX_NAME + 1, 'k') + "=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::NAME_TOO_LONG)) + "}"
	);
	// Запись событий безопасности, длиною предел поверяющая
	const string record = "CEF:0|A|B|C|D|E|1|src=1.2.3.4";
	// Устанавливаем наибольшую длину записи ровно по длине её самой
	settings.maxRecord = static_cast <uint32_t> (record.size());
	// Выполняем проверку приёма записи длиною ровно в предел
	EXPECT_EQ(
		::dumpCef(record, settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{src=1.2.3.4}R;"
	);
	// Устанавливаем наибольшую длину записи на один байт меньше её самой
	settings.maxRecord = static_cast <uint32_t> (record.size() - 1);
	// Выполняем проверку отклонения записи превышением её длины
	EXPECT_EQ(
		::dumpCef(record, settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::RECORD_TOO_LONG)) + "}"
	);
}

/**
 * @brief Проверка предела количества пар расширения
 *
 */
TEST(CodecCefReader, ExtensionsLimit) {
	// Настройки разбора записей
	cef::reader_t::settings_t settings;
	// Устанавливаем наибольшее допустимое количество пар расширения
	settings.maxExtensions = 2;
	// Выполняем проверку выдачи записи с количеством пар, пределу равным
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|src=1.2.3.4 dst=5.6.7.8", settings),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{1}E{src=1.2.3.4}E{dst=5.6.7.8}R;"
	);
	// Выполняем проверку отклонения записи превышением количества пар расширения
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|1|src=1.2.3.4 dst=5.6.7.8 spt=1024", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::OVERFLOW_LIMIT)) + "}"
	);
}

/**
 * @brief Проверка набора чисел записи без обращения по кругу
 *
 * @details Номер редакции и важность события набираются в «uint32_t», а поле заголовка
 *          длиною до MAX_HEADER_FIELD знаков: без сторожа набор обращался по кругу, и
 *          «4294967297» есть 2^32 + 1 - в uint32_t он ложился ЕДИНИЦЕЙ. Запись с
 *          заведомо негодною редакцией принималась как редакция 1, молча и без отказа;
 *          важность «4294967298» выходила двойкою - годной
 *
 * @note Проверка закрепляет ОТСУТСТВИЕ обращения по кругу, а не место отказа: место
 *       осталось прежним - поверкою редакции в next() и разбором важности своим ходом
 *
 */
TEST(CodecCefReader, NumberOverflowWrap) {
	// Настройки разбора записей с умолчаниями
	const cef::reader_t::settings_t defaults;
	// Выполняем проверку отклонения записи редакцией, за предел вышедшей
	EXPECT_EQ(
		::dumpCef("CEF:4294967297|A|B|C|D|E|1|src=1.2.3.4", defaults),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::UNSUPPORTED_VERSION)) + "}"
	);
	// Настройки разбора записей со строгим сличением
	cef::reader_t::settings_t settings;
	// Устанавливаем строгое сличение со словарём
	settings.mode = cef::mode_t::STRONG;
	// Выполняем проверку отклонения записи важностью, за предел вышедшей
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|4294967298|src=1.2.3.4", settings),
		"F{" + ::std::to_string(static_cast <uint32_t> (cef::error_t::INVALID_SEVERITY)) + "}"
	);
	/**
	 * Выполняем проверку записи важностью, за предел вышедшей, при мягком сличении
	 *
	 * @note Важность сбрасывается в нуль, как и при важности, записанной словом: числом
	 *       она здесь не является, и трактовка их одинакова намеренно
	 */
	EXPECT_EQ(
		::dumpCef("CEF:0|A|B|C|D|E|4294967298|src=1.2.3.4", defaults),
		"H{0}H{A}H{B}H{C}H{D}H{E}H{4294967298}E{src=1.2.3.4}R;"
	);
}

/**
 * @brief Проверка предела длины записи у текста без границы
 *
 * @details Поверка длины стояла ЛИШЬ за найденной границей записи, и текст без перевода
 *          строки её не встречал вовсе: хранилище росло, пока подаёт подающий. Щуп
 *          14.09.2026 скормил 64 МБ при пределе записи в 1 МБ - ни отказа, ни жалобы
 *
 * @warning Разряд этот опаснее прочих: отправитель, границы не ставящий, исчерпывает
 *          память принимающего, и никакая настройка того не останавливала
 *
 * @note Сличение строгое: кусок длиною РОВНО в предел годен и отказа не наводит - иначе
 *       законная запись предельной длины стала бы неразбираемой
 *
 */
TEST(CodecCefReader, UnboundedFeedIsRefused) {
	// Настройки разбора записей
	cef::reader_t::settings_t settings;
	// Устанавливаем малый предел длины записи
	settings.maxRecord = 1024;
	// Выполняем создание объекта чтения записей
	cef::reader_t reader;
	// Устанавливаем настройки разбора записей
	ASSERT_TRUE(reader.settings(settings));
	// Кусок текста длиною РОВНО в предел, границы записи не несущий
	const string chunk(settings.maxRecord, 'A');
	// Выполняем подачу куска текста длиною ровно в предел
	ASSERT_TRUE(reader.feed(chunk.data(), chunk.size(), false));
	// Выполняем перебор событий разбора
	while(reader.next()){}
	// Выполняем проверку того, что кусок длиною ровно в предел отказа не навёл
	EXPECT_EQ(reader.error(), cef::error_t::NONE);
	// Выполняем подачу второго куска текста, предел превосходящего
	ASSERT_TRUE(reader.feed(chunk.data(), chunk.size(), false));
	// Выполняем перебор событий разбора
	while(reader.next()){}
	// Выполняем проверку того, что превышение предела отвечено отказом
	EXPECT_EQ(reader.error(), cef::error_t::RECORD_TOO_LONG);
	// Выполняем проверку того, что чтение записей прекращено отказом
	EXPECT_EQ(reader.state(), cef::state_t::FAILED);
}
