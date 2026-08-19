#include <codec/yaml/yaml.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
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
	yaml::reader_t reader(settings);
	if(!reader.feed(text)){ std::cout << "ОТКАЗ: " << yaml::message(reader.error()) << "\n"; return 1; }
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
		 *          ошибается там, где имя пары само поточным построением является: место
		 *          события блочного отображения стоит тогда на скобке имени, и щуп метит
		 *          скобками отображение блочное. Случай Q9WF расходится по этой причине,
		 *          а не по вине кодека
		 */
		if(reader.value().location.offset == yaml::NO_OFFSET) return std::string();
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
			break;
			case static_cast <uint8_t> (yaml::event_t::MAPPING_END):
				depth--; indent(); result.append("-MAP\n");
			break;
			case static_cast <uint8_t> (yaml::event_t::SEQUENCE_START):
				indent(); result.append("+SEQ").append(marked("[]")).append(props()).append("\n"); depth++;
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
	std::cout << result;
	return 0;
}
