// Средство выдачи разобранной таблицы в виде, пригодном к сличению с эталоном
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <codec/csv/csv.hpp>

using namespace std;
using namespace awh::codec;

// Выполняем запись содержимого поля с отменой опасных знаков
static void quote(const string_view text){
	::fputc('"', stdout);
	for(const unsigned char letter : text){
		switch(letter){
			case '"': ::fputs("\\\"", stdout); break;
			case '\\': ::fputs("\\\\", stdout); break;
			case '\n': ::fputs("\\n", stdout); break;
			case '\r': ::fputs("\\r", stdout); break;
			case '\t': ::fputs("\\t", stdout); break;
			default: {
				if(letter < 0x20)
					::printf("\\u%04x", letter);
				else ::fputc(letter, stdout);
			}
		}
	}
	::fputc('"', stdout);
}

int main(int argc, char * argv[]){
	if(argc < 2)
		return 1;
	// Читаем содержимое файла таблицы
	ifstream file(argv[1], ios::binary);
	stringstream buffer;
	buffer << file.rdbuf();
	const string text = buffer.str();
	// Размер куска подачи, ноль означает подачу целиком
	const size_t chunk = ((argc > 2) ? static_cast <size_t> (::atoi(argv[2])) : 0);
	// Настройки разбора текста
	csv::reader_t::settings_t settings;
	csv::reader_t reader(settings);
	// Записи разобранной таблицы
	vector <vector <string>> records;
	vector <string> record;
	size_t offset = 0;
	do {
		const size_t size = ((chunk == 0) ? (text.size() - offset) : min(chunk, text.size() - offset));
		reader.feed(text.data() + offset, size, ((offset + size) >= text.size()));
		while(reader.next()){
			switch(static_cast <uint8_t> (reader.event())){
				case static_cast <uint8_t> (csv::event_t::FIELD):
					record.push_back(string(reader.field().value));
				break;
				case static_cast <uint8_t> (csv::event_t::RECORD): {
					// Запись из единственного пустого поля отбрасывается наравне с эталоном:
					// эталон пустую строку от записи «""» не отличает вовсе
					if(!((record.size() == 1) && record.front().empty()))
						records.push_back(record);
					record.clear();
				} break;
			}
		}
		offset += size;
	} while(offset < text.size());
	::fputc('[', stdout);
	for(size_t i = 0; i < records.size(); i++){
		if(i > 0)
			::fputc(',', stdout);
		::fputc('[', stdout);
		for(size_t j = 0; j < records.at(i).size(); j++){
			if(j > 0)
				::fputc(',', stdout);
			quote(records.at(i).at(j));
		}
		::fputc(']', stdout);
	}
	::fputs("]\n", stdout);
	if(reader.error() != csv::error_t::NONE){
		::fprintf(stderr, "%s\n", csv::message(reader.error()));
		return 2;
	}
	return 0;
}
