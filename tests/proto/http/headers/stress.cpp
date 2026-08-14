/**
 * @file stress.cpp
 * @date 2026-07-31
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
 * @brief Нагрузочные проверки контейнера HTTP-заголовков — случайные цепочки вызовов
 *        прикладного интерфейса со сверкой свойств контейнера после каждого шага
 *
 * @details Отдельные проверки задают вопрос «верно ли работает вот этот метод», и мимо них
 *          проходит целый род дефектов: тот, что живёт не в одном методе, а в сочетании
 *          состояний, до которого поштучная проверка не добирается. Оба дефекта, найденных
 *          в аудите 2026-07-31, были именно такими: строка переноса заголовка адресата
 *          в псевдозаголовок авторитета была покрыта проверками, а сочетание «провайдер
 *          ответа сервера рядом с заголовком адресата» - нет.
 *
 *          Поэтому здесь проверяются не методы, а свойства, которые контейнер обязан
 *          соблюдать в любом состоянии, до которого его вообще можно довести прикладным
 *          интерфейсом. Состояния перебираются случайными цепочками вызовов на паре
 *          объектов; последовательность задаётся постоянным зерном, поэтому прогон
 *          воспроизводим, а сообщение об отказе называет номер цепочки и шага
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "headers.hpp"
#include "../../../../include/proto/http/parser/http1/http.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Количество цепочек вызовов, выполняемых за прогон
 *
 * @note Значение подобрано так, чтобы прогон укладывался в единицы секунд и при этом
 *       доходил до сочетаний, которых отдельные проверки не создают
 *
 */
static constexpr size_t AWH_STRESS_CHAINS = 1500;

/**
 * @brief Класс источника воспроизводимой последовательности чисел
 *
 * @details Стандартный генератор не используется намеренно: его состояние
 *          и распределения зависят от реализации библиотеки, а прогон обязан
 *          давать одну и ту же последовательность состояний на всякой системе,
 *          иначе отказ невозможно повторить
 *
 */
class Random {
	private:
		// Текущее состояние источника
		uint64_t _state;
	public:
		/**
		 * @brief Метод получения очередного числа
		 *
		 * @return очередное число последовательности
		 *
		 */
		uint32_t next() noexcept {
			// Смешиваем состояние сдвигами (xorshift64)
			this->_state ^= (this->_state << 13);
			// Продолжаем смешивание состояния
			this->_state ^= (this->_state >> 7);
			// Завершаем смешивание состояния
			this->_state ^= (this->_state << 17);
			// Выводим старшую половину состояния
			return static_cast <uint32_t> (this->_state >> 32);
		}
		/**
		 * @brief Метод получения очередного числа в заданных границах
		 *
		 * @param limit верхняя граница (не включается)
		 * @return      очередное число последовательности
		 *
		 */
		uint32_t next(const uint32_t limit) noexcept {
			// Приводим очередное число к заданным границам
			return ((limit == 0) ? 0 : (this->next() % limit));
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 * @param seed начальное состояние источника
		 *
		 */
		explicit Random(const uint64_t seed) noexcept : _state(seed) {}
};

/**
 * @brief Названия заголовков, участвующие в цепочках вызовов
 *
 * @note Набор подобран так, чтобы в цепочки попадали заголовки всех родов: обычные,
 *       управляющие соединением, адресат, псевдозаголовки и поля с законной кратностью
 *
 */
static const std::vector <std::string> AWH_STRESS_NAMES = {
	"Host", "Connection", "TE", "Set-Cookie", "Content-Length", "X-Custom",
	"Upgrade", "Transfer-Encoding", "Keep-Alive", "Proxy-Connection",
	":authority", ":method", ":path", ":scheme", ":status", "User-Agent", "Server", "Date"
};

/**
 * @brief Значения заголовков, участвующие в цепочках вызовов
 *
 */
static const std::vector <std::string> AWH_STRESS_VALUES = {
	"", "trailers", "close", "TE, close", "example.com", "user:pass@example.com",
	"  padded  ", "a=1", "b=2", "42", "gzip", "keep-alive", "GET", "/x", "https", "200"
};

/**
 * @brief Протоколы, участвующие в цепочках вызовов
 *
 */
static const std::vector <proto_t> AWH_STRESS_PROTOS = {
	proto_t::NONE, proto_t::HTTP1, proto_t::HTTP2, proto_t::HTTP3,
	proto_t::PROXY1, proto_t::PROXY2, proto_t::WEBSOCKET1, proto_t::WEBSOCKET2
};

/**
 * @brief Стартовые строки, участвующие в цепочках вызовов
 *
 */
static const std::vector <std::string> AWH_STRESS_STARTLINES = {
	"GET /x HTTP/1.1", "HTTP/1.1 200 OK", "POST http://a.b/c HTTP/1.0",
	"", "BROKEN", "HTTP/1.1 20 OK", "PUT /y HTTP/9.9"
};

/**
 * @brief Метод пересчёта объёма полезной нагрузки набора
 *
 * @param headers проверяемый контейнер заголовков
 * @return        пересчитанный объём полезной нагрузки
 *
 */
static size_t recount(const headers_t & headers) noexcept {
	// Результат работы функции
	size_t result = 0;
	/**
	 * Проходим по всем заголовкам набора
	 */
	for(auto i = headers.begin(); i != headers.end(); ++i)
		// Увеличиваем объём полезной нагрузки на объём очередного заголовка
		result += (i->name.size() + i->value.size());
	// Выводим пересчитанный объём полезной нагрузки
	return result;
}

/**
 * @brief Метод регистронезависимого сравнения названий заголовков
 *
 * @param first  первое название заголовка
 * @param second второе название заголовка
 * @return       результат сравнения без учёта регистра
 *
 */
static bool sameName(const std::string & first, const std::string & second) noexcept {
	// Выполняем регистронезависимое сравнение названий заголовков
	return ((first.size() == second.size()) && (::strcasecmp(first.c_str(), second.c_str()) == 0));
}

/**
 * @brief Класс нагрузочных проверок контейнера HTTP-заголовков
 *
 */
class HeadersStressFixture : public HeadersFixture {
	protected:
		// Количество сообщений, доведённых до разборщика HTTP/1
		size_t _roundtrips = 0;
	protected:
		/**
		 * @brief Метод сверки свойств, обязательных для всякого состояния контейнера
		 *
		 * @param headers проверяемый контейнер заголовков
		 * @param step    описание шага цепочки для сообщения об отказе
		 *
		 */
		void verify(const headers_t & headers, const std::string & step) noexcept {
			/**
			 * Свойство 1: учёт занимаемой памяти согласован с содержимым набора.
			 * Расхождение означает путь изменения набора в обход учёта, а вместе
			 * с учётом перестают защищать и оба ограничения
			 */
			ASSERT_EQ(headers.memory(), recount(headers)) << step;
			/**
			 * Свойство 2: ограничения соблюдаются. Ограничения существуют против
			 * недружелюбного входа, и превышение любого из них означает, что вход
			 * нашёл дорогу мимо проверки
			 */
			ASSERT_LE(headers.size(), headers.maxRecords()) << step;
			// Проверяем соблюдение ограничения по объёму полезной нагрузки
			ASSERT_LE(headers.memory(), headers.maxMemory()) << step;
			/**
			 * Свойство 3: способы узнать размер набора согласованы между собой
			 */
			ASSERT_EQ(headers.size(), headers.count("")) << step;
			// Проверяем согласованность признака пустоты с размером набора
			ASSERT_EQ(headers.empty(), (headers.size() == 0)) << step;
			/**
			 * Свойство 4: сравнение рефлексивно, а копия неотличима от источника.
			 * Копия, отличимая от источника, означает поле состояния, которое
			 * копированием не переносится
			 */
			ASSERT_TRUE(headers == headers) << step;
			// Создаём копию контейнера
			const headers_t copy = headers;
			// Проверяем что копия равна источнику
			ASSERT_TRUE(copy == headers) << step;
			// Проверяем что учёт занимаемой памяти перенесён копированием
			ASSERT_EQ(copy.memory(), headers.memory()) << step;
			// Проверяем что вид сообщения у копии тот же
			ASSERT_EQ(copy.print(), headers.print()) << step;
			/**
			 * Свойство 5: получение вида сообщения объект не меняет
			 */
			const size_t memory = headers.memory();
			// Запоминаем размер набора до получения вида сообщения
			const size_t records = headers.size();
			// Получаем вид сообщения под протокол контейнера
			const std::string message = headers.print();
			// Получаем вид сообщения под бинарный протокол
			(void) headers.print(proto_t::HTTP2);
			// Проверяем что учёт занимаемой памяти не сдвинулся
			ASSERT_EQ(headers.memory(), memory) << step;
			// Проверяем что размер набора не изменился
			ASSERT_EQ(headers.size(), records) << step;
			/**
			 * Свойство 6: перечень названий согласован с поиском и кратностью
			 */
			for(const auto & name : headers.names()){
				// Проверяем что название из перечня находится поиском
				ASSERT_TRUE(headers.has(name)) << step << ": " << name;
				// Проверяем что кратность согласована с перечнем значений
				ASSERT_EQ(headers.count(name), headers.range(name).size()) << step << ": " << name;
				// Проверяем что первое значение перечня совпадает с извлечённым по названию
				ASSERT_EQ(headers.at(name), headers.range(name).front()) << step << ": " << name;
			}
			/**
			 * Свойство 7: вид сообщения неподвижен. Собранный набор, положенный
			 * в другой контейнер того же протокола, обязан дать при сборке тот же
			 * набор - иначе объект расходится с собственным видом сообщения
			 */
			const headers_t::fields_t fields = static_cast <headers_t::fields_t> (headers);
			// Создаём контейнер того же протокола для обратного присваивания
			headers_t back(headers.proto(), this->_fmk.get(), this->_log.get());
			// Поднимаем ограничения, чтобы обратное присваивание не резалось ими
			back.maxRecords(headers.maxRecords() + 8);
			// Поднимаем ограничение по объёму полезной нагрузки
			back.maxMemory(headers.maxMemory() + 4096);
			// Присваиваем собранный набор
			back = fields;
			/**
			 * Возвращаем протокол источника: присваивание набора определяет протокол
			 * заново по составу полей, и расхождение по нему к неподвижности вида
			 * сообщения отношения не имеет
			 */
			back.proto(headers.proto());
			// Собираем вид сообщения повторно
			const headers_t::fields_t again = static_cast <headers_t::fields_t> (back);
			// Проверяем что состав набора при обходе сохранён
			ASSERT_EQ(again.size(), fields.size()) << step;
			/**
			 * Проходим по обоим наборам, сверяя их поэлементно
			 */
			for(size_t i = 0; i < again.size(); i++){
				// Проверяем что название заголовка сохранено с точностью до регистра
				ASSERT_TRUE(sameName(again.at(i).name, fields.at(i).name)) << step << ": " << fields.at(i).name;
				// Проверяем что значение заголовка сохранено дословно
				ASSERT_EQ(again.at(i).value, fields.at(i).value) << step << ": " << fields.at(i).name;
			}
			/**
			 * Свойство 8: вид сообщения под бинарный протокол отвечает запретам
			 * HTTP/2 и HTTP/3 - заголовков управления соединением и заголовка
			 * адресата в нём быть не может, а перенос адресата второго
			 * псевдозаголовка авторитета не создаёт
			 */
			headers_t binary = headers;
			// Переводим копию на бинарный протокол
			binary.proto(proto_t::HTTP2);
			// Количество псевдозаголовков авторитета, полученных переносом
			size_t transferred = 0;
			// Количество псевдозаголовков авторитета, положенных вызывающей стороной
			size_t stored = 0;
			/**
			 * Считаем псевдозаголовки авторитета, положенные вызывающей стороной
			 */
			for(auto i = headers.begin(); i != headers.end(); ++i)
				// Увеличиваем счётчик при совпадении названия
				stored += (sameName(i->name, ":authority") ? 1 : 0);
			/**
			 * Определяем направление сообщения: заголовок адресата снимается только
			 * у запроса, где его место занимает псевдозаголовок авторитета. В ответе
			 * авторитет не формируется, а к управляющим соединением адресат не отнесён
			 * (RFC 9113 §8.2.2), и снятие теряло бы поле, передавать которое не запрещено
			 */
			const bool response = ((headers.provider() != nullptr) ?
				(headers.provider()->direct == direct_t::RESPONSE) :
				headers.has(":status")
			);
			/**
			 * Проходим по виду сообщения под бинарный протокол
			 */
			for(const auto & header : static_cast <headers_t::fields_t> (binary)){
				// Считаем псевдозаголовки авторитета вида сообщения
				transferred += (header.name == ":authority" ? 1 : 0);
				// Проверяем что заголовок адресата в вид сообщения запроса не попал
				if(!response)
					// Сличаем название заголовка с адресатом
					ASSERT_NE(header.name, "host") << step;
				// Проверяем что заголовок управления соединением в вид сообщения не попал
				ASSERT_NE(header.name, "connection") << step;
				// Проверяем что заголовок обновления протокола в вид сообщения не попал
				ASSERT_NE(header.name, "upgrade") << step;
				// Проверяем что заголовок удержания соединения в вид сообщения не попал
				ASSERT_NE(header.name, "keep-alive") << step;
				// Проверяем что заголовок соединения с посредником в вид сообщения не попал
				ASSERT_NE(header.name, "proxy-connection") << step;
				// Проверяем что заголовок кодирования передачи в вид сообщения не попал
				ASSERT_NE(header.name, "transfer-encoding") << step;
			}
			/**
			 * Проверяем что перенос заголовка адресата лишнего авторитета не добавил:
			 * их в виде сообщения не больше, чем положила вызывающая сторона, либо
			 * ровно один, полученный переносом
			 */
			ASSERT_LE(transferred, ((stored > 0) ? stored : 1)) << step;
			/**
			 * Свойство 9: вид сообщения ответа сервера несёт единственный
			 * допустимый ему псевдозаголовок кода ответа (RFC 9113 §8.3.2)
			 */
			if((headers.provider() != nullptr) && (headers.provider()->direct == direct_t::RESPONSE)){
				/**
				 * Проходим по виду сообщения под бинарный протокол
				 */
				for(const auto & header : static_cast <headers_t::fields_t> (binary)){
					// Если поле псевдозаголовком не является - переходим к следующему
					if(header.name.empty() || (header.name.front() != ':'))
						// Переходим к следующему полю вида сообщения
						continue;
					// Проверяем что единственным псевдозаголовком ответа является код ответа
					ASSERT_EQ(header.name, ":status") << step;
				}
			}
			/**
			 * Свойство 10: обмен дважды возвращает объект к прежнему виду
			 */
			headers_t left = headers;
			// Создаём пустой контейнер для обмена
			headers_t right(this->_fmk.get(), this->_log.get());
			// Выполняем обмен содержимым
			left.swap(right);
			// Выполняем обмен содержимым повторно
			left.swap(right);
			// Проверяем что вид сообщения после двух обменов прежний
			ASSERT_EQ(left.print(), message) << step;
			/**
			 * Свойство 11: вид хранилища согласован с самим собой - мультикарта
			 * отдаёт ровно то, что лежит в наборе, с сохранением кратности
			 */
			const headers_t::multimap_t storage = static_cast <headers_t::multimap_t> (headers);
			// Проверяем что мультикарта отдала весь набор
			ASSERT_EQ(storage.size(), headers.size()) << step;
			/**
			 * Проходим по всем названиям набора
			 */
			for(const auto & name : headers.names())
				// Проверяем что кратность в мультикарте совпадает с кратностью в наборе
				ASSERT_EQ(storage.count(name), headers.count(name)) << step << ": " << name;
			/**
			 * Свойство 12: напечатанное сообщение разбирается настоящим разборщиком
			 * HTTP/1 обратно. Проверяется только там, где сообщение полное: без
			 * стартовой строки разбирать нечего, а поля с управляющими символами
			 * отправляющая сторона отвергает целиком - это её договор, записанный
			 * в README модуля
			 */
			this->roundtrip(headers, step);
		}
		/**
		 * @brief Метод сверки напечатанного сообщения с разобранным обратно
		 *
		 * @param headers проверяемый контейнер заголовков
		 * @param step    описание шага цепочки для сообщения об отказе
		 *
		 */
		void roundtrip(const headers_t & headers, const std::string & step) noexcept {
			// Если объект провайдера не установлен - полного сообщения не собрать
			if(headers.provider() == nullptr)
				// Выходим из метода
				return;
			// Получаем вид сообщения под протокол HTTP/1
			const std::string message = headers.print(proto_t::HTTP1);
			// Если стартовая строка не собрана - разбирать нечего
			if(message.empty() || (message.compare(0, 2, "\r\n") == 0))
				// Выходим из метода
				return;
			/**
			 * Проходим по всем полям вида сообщения, отсеивая непригодные к отправке:
			 * октеты вне набора значения поля отвергаются отправляющей стороной
			 * целиком, и такое сообщение до разборщика не доходит вовсе
			 */
			for(const auto & header : static_cast <headers_t::fields_t> (headers)){
				// Собираем название и значение заголовка в одну строку для проверки
				const std::string payload = (header.name + header.value);
				/**
				 * Проходим по всем октетам названия и значения заголовка
				 */
				for(const auto letter : payload){
					// Получаем беззнаковое значение октета
					const uint8_t octet = static_cast <uint8_t> (letter);
					// Если октет управляющим не является либо является табуляцией - переходим к следующему
					if((octet == '\t') || ((octet >= 0x20) && (octet != 0x7F)))
						// Переходим к следующему октету
						continue;
					// Сообщение с управляющим октетом до разборщика не доходит
					return;
				}
				// Если название заголовка несёт пробел либо двоеточие - сообщение непригодно
				if((header.name.find(' ') != std::string::npos) || (header.name.find(':') != std::string::npos))
					// Выходим из метода
					return;
			}
			// Определяем направление трафика по объекту провайдера
			const direct_t direct = headers.provider()->direct;
			// Создаём объект разборщика HTTP/1 соответствующего направления
			auto parser = std::make_unique <parser_http_t> (direct, this->_fmk.get(), this->_log.get());
			// Собранный разборщиком набор заголовков
			std::vector <std::pair <std::string, std::string>> parsed;
			// Устанавливаем функцию обратного вызова, собирающую разобранные заголовки
			parser->on(parser_http_t::header_callback_t([&parsed](const uint32_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
				// Запоминаем разобранный заголовок
				parsed.emplace_back(std::string(name), std::string(value));
				// Продолжаем разбор сообщения
				return true;
			}));
			// Выполняем разбор напечатанного сообщения
			parser->parse(message.data(), message.size());
			// Если сообщение разборщиком отвергнуто - дальше сверять нечего
			if(parser->status() == parser_t::status_t::ERROR)
				// Выходим из метода
				return;
			/**
			 * Собираем кратность названий вида сообщения, отбрасывая заголовки
			 * с пустым значением: разборщик пустое значение поля сохраняет,
			 * а печать отделяет его пробелом, и обратно оно приходит пустым
			 */
			std::map <std::string, size_t> expected;
			/**
			 * Проходим по всем полям вида сообщения под протокол HTTP/1
			 */
			for(const auto & header : static_cast <headers_t::fields_t> (headers)){
				// Приводим название заголовка к нижнему регистру для сверки
				std::string name = header.name;
				// Приводим каждую букву названия к нижнему регистру
				for(auto & letter : name)
					// Приводим очередную букву названия к нижнему регистру
					letter = static_cast <char> (((letter >= 'A') && (letter <= 'Z')) ? (letter + 32) : letter);
				// Увеличиваем кратность названия
				expected[name]++;
			}
			// Собираем кратность названий, полученных от разборщика
			std::map <std::string, size_t> received;
			/**
			 * Проходим по всем разобранным заголовкам
			 */
			for(const auto & item : parsed){
				// Приводим название заголовка к нижнему регистру для сверки
				std::string name = item.first;
				// Приводим каждую букву названия к нижнему регистру
				for(auto & letter : name)
					// Приводим очередную букву названия к нижнему регистру
					letter = static_cast <char> (((letter >= 'A') && (letter <= 'Z')) ? (letter + 32) : letter);
				// Увеличиваем кратность названия
				received[name]++;
			}
			// Отмечаем сообщение, доведённое до разборщика
			this->_roundtrips++;
			// Проверяем что разборщик получил ровно те же названия с той же кратностью
			ASSERT_EQ(expected, received) << step << ": [" << message << "]";
		}
};

/**
 * @brief Метод сверки свойств контейнера на случайных цепочках вызовов
 *
 */
TEST_F(HeadersStressFixture, RandomCallChainsTest){
	/**
	 * Отключаем вывод логов на время прогона: понижение ограничений и отказ слияния
	 * записываются предупреждением, а прогон доводит контейнер до них тысячи раз -
	 * поток предупреждений скрыл бы собственный вывод набора проверок
	 */
	this->_log->level(awh::log_t::level_t::NONE);
	// Создаём источник воспроизводимой последовательности чисел
	Random random(0x12345678ABCDEFULL);
	// Количество сообщений, доведённых до разборщика HTTP/1
	this->_roundtrips = 0;
	/**
	 * Выполняем перебор цепочек вызовов
	 */
	for(size_t chain = 0; chain < AWH_STRESS_CHAINS; chain++){
		// Создаём первый контейнер заголовков цепочки
		headers_t first(this->_fmk.get(), this->_log.get());
		// Создаём второй контейнер заголовков цепочки
		headers_t second(this->_fmk.get(), this->_log.get());
		// Определяем длину очередной цепочки вызовов
		const size_t length = (4 + random.next(14));
		/**
		 * Выполняем перебор шагов цепочки вызовов
		 */
		for(size_t step = 0; step < length; step++){
			// Выбираем контейнер, над которым выполняется очередной шаг
			headers_t & target = ((random.next(4) == 0) ? second : first);
			// Выбираем второй контейнер цепочки
			headers_t & other = ((&target == &first) ? second : first);
			// Собираем описание шага для сообщения об отказе
			const std::string title = ("цепь " + std::to_string(chain) + " шаг " + std::to_string(step));
			/**
			 * Выполняем очередной шаг цепочки вызовов
			 */
			switch(random.next(16)){
				// Добавляем либо заменяем заголовок
				case 0: case 1: case 2:
					target.emplace(
						AWH_STRESS_NAMES.at(random.next(static_cast <uint32_t> (AWH_STRESS_NAMES.size()))),
						AWH_STRESS_VALUES.at(random.next(static_cast <uint32_t> (AWH_STRESS_VALUES.size()))),
						((random.next(2) == 0) ? headers_t::mode_t::REPLACE : headers_t::mode_t::APPEND)
					);
				break;
				// Снимаем заголовок по названию
				case 3: target.erase(AWH_STRESS_NAMES.at(random.next(static_cast <uint32_t> (AWH_STRESS_NAMES.size())))); break;
				// Переводим контейнер на другой протокол
				case 4: target.proto(AWH_STRESS_PROTOS.at(random.next(static_cast <uint32_t> (AWH_STRESS_PROTOS.size())))); break;
				// Очищаем набор заголовков
				case 5: target.clear(); break;
				// Выполняем полный сброс контейнера
				case 6: target.reset(); break;
				// Опускаем ограничение по количеству записей
				case 7: target.maxRecords(1 + random.next(12)); break;
				// Опускаем ограничение по объёму полезной нагрузки
				case 8: target.maxMemory(1 + random.next(400)); break;
				// Выполняем слияние со вторым контейнером цепочки
				case 9: target.merge(other, ((random.next(2) == 0) ? headers_t::mode_t::REPLACE : headers_t::mode_t::APPEND)); break;
				// Обмениваемся содержимым со вторым контейнером цепочки
				case 10: target.swap(other); break;
				// Устанавливаем стартовую строку сообщения
				case 11: target.startline(AWH_STRESS_STARTLINES.at(random.next(static_cast <uint32_t> (AWH_STRESS_STARTLINES.size())))); break;
				// Устанавливаем объект провайдера сообщения
				case 12: {
					// Если очередным сообщением является запрос клиента
					if(random.next(2) == 0){
						// Создаём объект запроса клиента
						request_t request(version_t::HTTP1_1, method_t::GET, std::string("https://user:pass@example.com/path?q=1#f"));
						// Устанавливаем провайдер запроса
						target.provider(&request);
					// Если очередным сообщением является ответ сервера
					} else {
						// Создаём объект ответа сервера
						response_t response(version_t::HTTP1_1, static_cast <uint16_t> (200 + random.next(200)), std::string("OK"));
						// Устанавливаем провайдер ответа
						target.provider(&response);
					}
				} break;
				// Добавляем заголовки по умолчанию
				case 13: target.addDefaultHeaders(); break;
				// Присваиваем вид хранилища второго контейнера цепочки
				case 14: target = static_cast <headers_t::multimap_t> (other); break;
				// Выполняем перенос контейнера и возврат его обратно
				case 15: {
					// Переносим содержимое контейнера во временный объект
					headers_t moved = ::std::move(target);
					// Сверяем свойства временного объекта
					this->verify(moved, title + " (перенос)");
					// Если сверка отказала - прерываем цепочку
					if(this->HasFatalFailure())
						// Выходим из метода
						return;
					// Возвращаем содержимое обратно
					target = ::std::move(moved);
				} break;
			}
			// Сверяем свойства первого контейнера цепочки
			this->verify(first, title + " (первый)");
			// Если сверка отказала - прерываем прогон
			if(this->HasFatalFailure())
				// Выходим из метода
				return;
			// Сверяем свойства второго контейнера цепочки
			this->verify(second, title + " (второй)");
			// Если сверка отказала - прерываем прогон
			if(this->HasFatalFailure())
				// Выходим из метода
				return;
		}
	}
	/**
	 * Проверяем что обход через разборщик выполнялся: свойство, ни разу не дошедшее
	 * до сверки, ничего не проверяет, и отсев непригодных сообщений не должен
	 * съедать его целиком
	 */
	ASSERT_GT(this->_roundtrips, 100u);
}
