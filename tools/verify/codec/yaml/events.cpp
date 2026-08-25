#include <codec/yaml/yaml.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <sys/log.hpp>

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
		 * @brief Функция получения объекта фреймворка проверок
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          проверки, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка проверок
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка проверок
			static awh::fmk_t fmk;
			// Выводим объект фреймворка проверок
			return fmk;
		}
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта журнала проверок
	 *
	 * @return объект журнала проверок
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект журнала проверок
		static Silent silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}
using namespace awh::codec;
/**
 * Приведение содержимого к записи набора сверки: перевод строки, подача и обратная
 * косая передаются отменяющими последовательностями
 */
static std::string escaped(const std::string_view text){
	std::string result;
	for(const char letter : text){
		switch(letter){
			case '\n': result.append("\\n"); break;
			case '\t': result.append("\\t"); break;
			case '\r': result.append("\\r"); break;
			case '\b': result.append("\\b"); break;
			case '\\': result.append("\\\\"); break;
			default: result.push_back(letter);
		}
	}
	return result;
}
int main(int argc, char ** argv){
	std::ifstream file(argv[1], std::ios::binary);
	std::stringstream stream; stream << file.rdbuf();
	const std::string text = stream.str();
	yaml::reader_t::settings_t settings;
	yaml::reader_t reader(::logger(), settings);
	if(!reader.feed(text)){ std::cout << "ОТКАЗ: " << yaml::message(reader.error()) << "\n"; return 1; }
	/**
	 * Места событий открытия построений, проходом предварительным собранные
	 *
	 * @note Собираются они вторым чтением того же текста: сличение места события с местом
	 *       события следующего требует взгляда вперёд, а чтение потоковое его не даёт
	 */
	std::vector <uint64_t> starts;
	{
		yaml::reader_t::settings_t opening;
		yaml::reader_t scout(::logger(), opening);
		if(scout.feed(text)){
			while(scout.next()){
				if((scout.event() == yaml::event_t::MAPPING_START) ||
				   (scout.event() == yaml::event_t::SEQUENCE_START))
					starts.push_back(scout.value().location.offset);
			}
		}
	}
	// Номер разбираемого события открытия построения
	size_t started = 0;
	std::string result;
	size_t depth = 0;
	const auto indent = [&](){ result.append(depth, ' '); };
	const auto props = [&](){
		std::string out;
		if(!reader.value().anchor.empty() && (reader.event() != yaml::event_t::ALIAS))
			out.append(" &").append(reader.value().anchor);
		if(!reader.value().tag.empty())
			out.append(" <").append(reader.value().tag).append(">");
		return out;
	};
	/**
	 * Розыск первого незначащего знака от места события: набор сверки метит поточное
	 * построение скобками, а явную черту документа - самой чертой
	 */
	const auto marked = [&](const char * pair) noexcept -> std::string {
		/**
		 * Построение поточное, скобок своих не имеющее
		 *
		 * @note Запись `[ имя: значение ]` описание берёт правилом `ns-flow-pair`: отображение
		 *       там скобок не имеет вовсе, а набор сверки метит его скобками наравне с прочими.
		 *       Розыском по тексту его не опознать - место события стоит на имени пары
		 */
		if(reader.value().flow) return std::string(" ").append(1, pair[0]).append(1, pair[1]);
		/**
		 * @warning Розыск этот есть догадка щупа, а не сведение от чтения: признак
		 *          поточного построения выдаётся детям его, а самому построению - нет,
		 *          ибо оно не ВНУТРИ построения стоит, а построением и является. Догадка
		 *          ошибалась там, где имя пары само поточным построением является, и
		 *          правится она сличением мест ниже - но остаётся догадкою: расхождение
		 *          по ней есть изъян щупа, а не кодека, и первым делом проверять надлежит
		 *          её
		 */
		if(reader.value().location.offset == yaml::NO_OFFSET) return std::string();
		/**
		 * Построение блочное, место своё у имени поточного взявшее
		 *
		 * @note Отображение блочное стоит там же, где стоит имя первой пары его, и имя
		 *       это вправе быть построением поточным: скобка тогда одна на два события, и
		 *       принадлежит она внутреннему. Опознаётся это совпадением мест: события
		 *       открытия идут подряд с одного и того же места. Случай Q9WF
		 */
		if(((started + 1) < starts.size()) && (starts.at(started) == starts.at(started + 1)))
			return std::string();
		size_t offset = static_cast <size_t> (reader.value().location.offset);
		while((offset < text.size()) && ((text.at(offset) == ' ') || (text.at(offset) == '\t'))) offset++;
		if(offset >= text.size()) return std::string();
		if((text.at(offset) == pair[0]) || (text.at(offset) == pair[1]))
			return std::string(" ").append(1, pair[0]).append(1, pair[1]);
		return std::string();
	};
	const auto dashed = [&](const char * mark) noexcept -> std::string {
		if(reader.value().location.offset == yaml::NO_OFFSET) return std::string();
		size_t offset = static_cast <size_t> (reader.value().location.offset);
		while((offset < text.size()) && ((text.at(offset) == ' ') || (text.at(offset) == '\t'))) offset++;
		if((offset + 2) > text.size()) return std::string();
		/**
		 * Черта опознаётся лишь тогда, когда за нею стоит пробельный знак либо конец
		 * строки: запись `---word1` чертою начала документа не является вовсе, а есть
		 * простое значение целиком. Случай 82AN
		 */
		if((text.compare(offset, 3, mark) == 0) &&
		   (((offset + 3) >= text.size()) || (text.at(offset + 3) == ' ') ||
		    (text.at(offset + 3) == '\t') || (text.at(offset + 3) == '\n')))
			return std::string(" ").append(mark);
		return std::string();
	};
	result.append("+STR\n");
	depth = 1;
	while(reader.next()){
		switch(static_cast <uint8_t> (reader.event())){
			case static_cast <uint8_t> (yaml::event_t::DOCUMENT_START):
				indent(); result.append("+DOC").append(dashed("---")).append("\n"); depth++;
			break;
			case static_cast <uint8_t> (yaml::event_t::DOCUMENT_END):
				depth--; indent(); result.append("-DOC").append(dashed("...")).append("\n");
			break;
			case static_cast <uint8_t> (yaml::event_t::MAPPING_START):
				indent(); result.append("+MAP").append(marked("{}")).append(props()).append("\n"); depth++;
				started++;
			break;
			case static_cast <uint8_t> (yaml::event_t::MAPPING_END):
				depth--; indent(); result.append("-MAP\n");
			break;
			case static_cast <uint8_t> (yaml::event_t::SEQUENCE_START):
				indent(); result.append("+SEQ").append(marked("[]")).append(props()).append("\n"); depth++;
				started++;
			break;
			case static_cast <uint8_t> (yaml::event_t::SEQUENCE_END):
				depth--; indent(); result.append("-SEQ\n");
			break;
			case static_cast <uint8_t> (yaml::event_t::SCALAR): {
				const char * mark = ":";
				switch(static_cast <uint8_t> (reader.value().style)){
					case static_cast <uint8_t> (yaml::style_t::SINGLE): mark = "'"; break;
					case static_cast <uint8_t> (yaml::style_t::DOUBLE): mark = "\""; break;
					case static_cast <uint8_t> (yaml::style_t::LITERAL): mark = "|"; break;
					case static_cast <uint8_t> (yaml::style_t::FOLDED): mark = ">"; break;
				}
				indent(); result.append("=VAL").append(props()).append(" ").append(mark)
				 .append(escaped(reader.value().text)).append("\n");
			} break;
			case static_cast <uint8_t> (yaml::event_t::ALIAS):
				indent(); result.append("=ALI *").append(reader.value().text).append("\n");
			break;
		}
	}
	if(reader.state() == yaml::state_t::FAILED){ std::cout << "ОТКАЗ: " << yaml::message(reader.error()) << "\n"; return 1; }
	result.append("-STR\n");
	/**
	 * Выполняем сличение двух путей перезаписи на корпусе соответствия
	 *
	 * @details Дерево документа и снятое с него владеющее значение переписывают одно и
	 *          то же содержимое, и круговой ход через перезапись значения обязан
	 *          обойтись без потерь. Примечания при обоих путях говорят, что расходиться
	 *          им нечем, - а слово это ничем не подтверждено, покуда не проверено делом.
	 *          Корпус соответствия несёт то, чего ворошитель не построит: записи всякого
	 *          вида в настоящих сочетаниях
	 *
	 * @note Сличается и содержимое, и устойчивость перезаписи. Второе нужно отдельно:
	 *       равенство значений судит по содержимому, и потери оформления - ограды,
	 *       построения блоком, правила усечения переводов строк - им не ловятся вовсе.
	 *       Оба же текста собраны одним путём и примечаний не несут ни один, оттого
	 *       сличаются дословно
	 *
	 * @note Текст, чтением принятый, а деревом отвергнутый, обходится: у дерева правил
	 *       больше - пределы вложенности да обхождение с повторными именами, - и отказ
	 *       его законен. Обход этот НЕ молчалив: он докладывается в поток ошибок, и счёт
	 *       обойдённых снимается прогоном по корпусу целиком
	 */
	{
		// Дерево документа разбираемого текста
		yaml::document_t tree(::logger());
		/**
		 * Если разобрать текст деревом не удалось
		 */
		if(!tree.parse(text))
			// Выводим сообщение об обходе сличения путей перезаписи
			std::cerr << "ОБХОД сличения путей: дерево отвергло текст: " << yaml::message(tree.error()) << "\n";
		// Если разбор текста деревом удался
		else {
			// Владеющее значение, с дерева документа снятое
			const yaml::value_t lifted(tree.root());
			// Перезапись снятого значения
			const std::string written = lifted.dump();
			/**
			 * Если перезапись снятого значения не пуста
			 */
			if(!written.empty()){
				// Дерево документа перезаписи снятого значения
				yaml::document_t rebuilt(::logger());
				/**
				 * Если перезапись снятого значения разобрать не удалось
				 */
				if(!rebuilt.parse(written)){
					// Выводим сообщение об отказе разбора перезаписи
					std::cerr << "перезапись снятого значения не разобрана: " << yaml::message(rebuilt.error())
					 << "\n[" << written << "]\n";
					// Выходим из приложения с ошибкой
					return 1;
				}
				// Значение, с дерева перезаписи снятое
				const yaml::value_t back(rebuilt.root());
				/**
				 * Если прочтённое из перезаписи со снятым значением разошлось
				 */
				if(!(back == lifted)){
					// Выводим сообщение о расхождении содержимого
					std::cerr << "содержимое кругового хода разошлось\nдерево:\n[" << tree.dump()
					 << "]\nзначение:\n[" << written << "]\n";
					// Выходим из приложения с ошибкой
					return 1;
				}
				/**
				 * Если перезапись снятого значения неустойчива
				 */
				if(back.dump() != written){
					// Выводим сообщение о неустойчивости перезаписи снятого значения
					std::cerr << "перезапись снятого значения неустойчива\nпервая:\n[" << written
					 << "]\nвторая:\n[" << back.dump() << "]\n";
					// Выходим из приложения с ошибкой
					return 1;
				}
			}
		}
	}
	std::cout << result;
	return 0;
}
