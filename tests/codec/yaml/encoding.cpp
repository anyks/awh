/**
 * @file encoding.cpp
 * @date 2026-08-17
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки приведения кодировок контейнера YAML — разбор последовательностей
 *        UTF-8, опознание кодировки по метке порядка байтов и независимость приведения
 *        от нарезки текста на куски
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/yaml/yaml.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Пространство имён помощников проверок приведения кодировок
 *
 * @note Помощники объявлены безымянным пространством имён намеренно: проверки всех
 *       кодеков собираются одной программой, и одноимённые помощники двух кодеков
 *       нарушили бы правило одного определения
 *
 */
namespace {
	/**
	 * @brief Функция сборки текста заданной кодировкой из знаков Юникода
	 *
	 * @param codes     собираемые знаки Юникода
	 * @param encoding  кодировка собираемого текста
	 * @param signature признак открытия текста меткой порядка байтов
	 * @return          собранный текст заданной кодировки
	 *
	 */
	string assemble(const vector <uint32_t> & codes, const yaml::encoding_t encoding, const bool signature) noexcept {
		// Собираемый текст заданной кодировки
		string result;
		// Признак обратного порядка байтов записи
		const bool little = ((encoding == yaml::encoding_t::UTF16LE) || (encoding == yaml::encoding_t::UTF32LE));
		// Признак записи знаков четвёрками байтов
		const bool wide = ((encoding == yaml::encoding_t::UTF32LE) || (encoding == yaml::encoding_t::UTF32BE));
		/**
		 * @brief Функция дописывания единицы записи парой байтов
		 *
		 * @param value дописываемая единица записи
		 *
		 */
		const auto unit = [&result, little](const uint32_t value) noexcept -> void {
			/**
			 * Если байты записываются обратным порядком
			 */
			if(little){
				// Выполняем запись младшего байта единицы записи
				result.push_back(static_cast <char> (value & 0xFF));
				// Выполняем запись старшего байта единицы записи
				result.push_back(static_cast <char> ((value >> 8) & 0xFF));
			/**
			 * Если байты записываются прямым порядком
			 */
			} else {
				// Выполняем запись старшего байта единицы записи
				result.push_back(static_cast <char> ((value >> 8) & 0xFF));
				// Выполняем запись младшего байта единицы записи
				result.push_back(static_cast <char> (value & 0xFF));
			}
		};
		/**
		 * @brief Функция дописывания знака четвёркой байтов
		 *
		 * @param value дописываемый знак Юникода
		 *
		 */
		const auto quad = [&result, little](const uint32_t value) noexcept -> void {
			/**
			 * Если байты записываются обратным порядком
			 */
			if(little){
				// Выполняем запись байтов знака обратным порядком
				result.push_back(static_cast <char> (value & 0xFF));
				result.push_back(static_cast <char> ((value >> 8) & 0xFF));
				result.push_back(static_cast <char> ((value >> 16) & 0xFF));
				result.push_back(static_cast <char> ((value >> 24) & 0xFF));
			/**
			 * Если байты записываются прямым порядком
			 */
			} else {
				// Выполняем запись байтов знака прямым порядком
				result.push_back(static_cast <char> ((value >> 24) & 0xFF));
				result.push_back(static_cast <char> ((value >> 16) & 0xFF));
				result.push_back(static_cast <char> ((value >> 8) & 0xFF));
				result.push_back(static_cast <char> (value & 0xFF));
			}
		};
		/**
		 * Если текст открывается меткой порядка байтов
		 */
		if(signature){
			/**
			 * Если текст записывается кодировкой UTF-8
			 */
			if(encoding == yaml::encoding_t::UTF8)
				// Выполняем запись метки порядка байтов кодировки UTF-8
				result.append("\xEF\xBB\xBF");
			/**
			 * Если текст записывается четвёрками байтов
			 */
			else if(wide)
				// Выполняем запись метки порядка байтов четвёркой байтов
				quad(0xFEFF);
			/**
			 * Если текст записывается парами байтов
			 */
			else unit(0xFEFF);
		}
		/**
		 * Выполняем перебор всех собираемых знаков Юникода
		 */
		for(const uint32_t code : codes){
			/**
			 * Если текст записывается кодировкой UTF-8
			 */
			if(encoding == yaml::encoding_t::UTF8)
				// Выполняем запись знака последовательностью UTF-8
				yaml::encode(code, result);
			/**
			 * Если текст записывается четвёрками байтов
			 */
			else if(wide)
				// Выполняем запись знака четвёркой байтов
				quad(code);
			/**
			 * Если знак укладывается в одну единицу записи
			 */
			else if(code < 0x10000)
				// Выполняем запись знака одной единицей записи
				unit(code);
			/**
			 * Если знак записывается парою суррогатов
			 */
			else {
				// Получаем значение знака за вычетом основной таблицы
				const uint32_t value = (code - 0x10000);
				// Выполняем запись старшего суррогата пары
				unit(0xD800 + (value >> 10));
				// Выполняем запись младшего суррогата пары
				unit(0xDC00 + (value & 0x3FF));
			}
		}
		// Выводим собранный текст заданной кодировки
		return result;
	}
	/**
	 * @brief Функция приведения текста к UTF-8 подачей кусками заданного размера
	 *
	 * @param source приводимый текст
	 * @param chunk  размер куска подачи
	 * @param result строка, куда помещается приведённый текст
	 * @return       признак успешного приведения текста
	 *
	 */
	bool piecemeal(const string & source, const size_t chunk, string & result) noexcept {
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Выполняем сброс собираемого приведённого текста
		result.clear();
		// Смещение очередного подаваемого куска
		size_t offset = 0;
		/**
		 * Выполняем подачу текста до его окончания
		 */
		do {
			// Получаем размер очередного подаваемого куска
			const size_t size = (((offset + chunk) > source.size()) ? (source.size() - offset) : chunk);
			/**
			 * Если привести очередной кусок не удалось
			 */
			if(!decoder.convert(source.data() + offset, size, ((offset + size) >= source.size()), result))
				// Выводим признак неудачного приведения текста
				return false;
			// Выполняем переход к следующему куску текста
			offset += size;
		// Выполняем подачу до исчерпания текста
		} while(offset < source.size());
		// Выводим признак успешного приведения текста
		return true;
	}
}

/**
 * @brief Проверка определения длины последовательности по ведущему байту
 *
 */
TEST(CodecYamlEncoding, Sequence) {
	// Выполняем проверку длины последовательности знака US-ASCII
	ASSERT_EQ(yaml::sequence(0x41), 1u);
	// Выполняем проверку длины последовательности знака о двух байтах
	ASSERT_EQ(yaml::sequence(0xD0), 2u);
	// Выполняем проверку длины последовательности знака о трёх байтах
	ASSERT_EQ(yaml::sequence(0xE2), 3u);
	// Выполняем проверку длины последовательности знака о четырёх байтах
	ASSERT_EQ(yaml::sequence(0xF0), 4u);
	// Выполняем проверку продолжающего байта, ведущим не являющегося
	ASSERT_EQ(yaml::sequence(0x80), 0u);
	// Выполняем проверку байта, последовательностью не начинающегося вовсе
	ASSERT_EQ(yaml::sequence(0xFF), 0u);
}
/**
 * @brief Проверка разбора последовательностей UTF-8
 *
 */
TEST(CodecYamlEncoding, Inspect) {
	// Прочитанный знак Юникода
	uint32_t code = 0;
	// Количество байт, разбором пройденных
	size_t length = 0;
	// Выполняем проверку разбора знака US-ASCII
	ASSERT_EQ(yaml::inspect("A", 0, code, length), yaml::utf8_t::VALID);
	// Выполняем проверку прочитанного знака
	ASSERT_EQ(code, 0x41u);
	// Выполняем проверку длины разобранной последовательности
	ASSERT_EQ(length, 1u);
	// Выполняем проверку разбора знака кириллицы
	ASSERT_EQ(yaml::inspect("\xD0\xB7", 0, code, length), yaml::utf8_t::VALID);
	// Выполняем проверку прочитанного знака
	ASSERT_EQ(code, 0x437u);
	/**
	 * Выполняем проверку последовательности, оборванной концом текста
	 *
	 * @note Исход этот от ошибочного построения отделён намеренно: разбор обязан
	 *       дождаться продолжения, а не объявить отказ
	 */
	ASSERT_EQ(yaml::inspect("\xD0", 0, code, length), yaml::utf8_t::TRUNCATED);
	/**
	 * Выполняем проверку длины оборванной последовательности
	 *
	 * @details Длиною выдаётся число байт, налицо имеющихся: они и составляют наибольшую
	 *          часть последовательности, и знак замены при записи полагается на них один
	 *
	 * @note Прежде длина выдавалась нулевою, и запись `E1 80` давала два знака замены
	 *       вместо одного. Наружу это торчит счётом знаков и расхождением с кодеком JSON
	 */
	ASSERT_EQ(length, 1u);
	/**
	 * Выполняем проверку последовательности, непродолжающий байт несущей и оборванной
	 *
	 * @note Оборванной она **не** является: байт `41` продолжающим не бывает, и никакое
	 *       продолжение записи той уже не выправит. Прежде проверка нехватки байт стояла
	 *       раньше проверки продолжающих, и чтение потоковое ждало бы такую запись впустую
	 */
	ASSERT_EQ(yaml::inspect("\xE1\x41", 0, code, length), yaml::utf8_t::BROKEN);
	// Выполняем проверку длины наибольшей части негодной последовательности
	ASSERT_EQ(length, 1u);
	/**
	 * Выполняем проверку суррогата, за каким стоит годный знак
	 *
	 * @details Граница негодной части ложится на байте `A0`: ведущий `ED` отводит первому
	 *          продолжающему `80..9F`. Прочти разбор всю последовательность целиком - и
	 *          годная буква, следом стоящая, была бы съедена вместе с негодным байтом
	 */
	ASSERT_EQ(yaml::inspect("\xED\xA0\x41", 0, code, length), yaml::utf8_t::BROKEN);
	// Выполняем проверку того, что негодною признан лишь ведущий байт
	ASSERT_EQ(length, 1u);
	/**
	 * Выполняем проверку байтов, ведущими не бывающих вовсе
	 *
	 * @note Байты `C0` и `C1` открывали бы запись длиннее необходимой, а байты свыше `F4` -
	 *       точку свыше U+10FFFF
	 */
	ASSERT_EQ(yaml::sequence(0xC0), 0u);
	// Выполняем проверку байта, ведущим не бывающего вовсе
	ASSERT_EQ(yaml::sequence(0xC1), 0u);
	// Выполняем проверку байта, ведущим не бывающего вовсе
	ASSERT_EQ(yaml::sequence(0xF5), 0u);
	// Выполняем проверку последовательности с непродолжающим байтом
	ASSERT_EQ(yaml::inspect("\xD0\x41", 0, code, length), yaml::utf8_t::BROKEN);
	/**
	 * Выполняем проверку записи длиннее необходимой
	 *
	 * @note Запись эта изображает знак, записываемый короче, и признать её значило бы
	 *       принять два написания одного имени
	 */
	ASSERT_EQ(yaml::inspect("\xC0\x80", 0, code, length), yaml::utf8_t::BROKEN);
	// Выполняем проверку записи знака US-ASCII двумя байтами
	ASSERT_EQ(yaml::inspect("\xC1\xBF", 0, code, length), yaml::utf8_t::BROKEN);
	// Выполняем проверку записи суррогата последовательностью UTF-8
	ASSERT_EQ(yaml::inspect("\xED\xA0\x80", 0, code, length), yaml::utf8_t::BROKEN);
	// Выполняем проверку знака за пределом набора Юникода
	ASSERT_EQ(yaml::inspect("\xF5\x80\x80\x80", 0, code, length), yaml::utf8_t::BROKEN);
	// Выполняем проверку чтения за концом текста
	ASSERT_EQ(yaml::inspect("A", 1, code, length), yaml::utf8_t::TRUNCATED);
}
/**
 * @brief Проверка записи знаков Юникода и обратного чтения их
 *
 */
TEST(CodecYamlEncoding, Roundtrip) {
	/**
	 * Знаки, охватывающие все длины записи и границы отрезков
	 */
	const vector <uint32_t> codes = {
		0x00, 0x41, 0x7F, 0x80, 0x7FF, 0x800, 0x437, 0xD7FF, 0xE000, 0xFFFD,
		0xFFFF, 0x10000, 0x1F600, 0x10FFFF
	};
	/**
	 * Выполняем перебор всех проверяемых знаков Юникода
	 */
	for(const uint32_t code : codes){
		// Собираемая последовательность знака
		string text;
		// Выполняем проверку успешной записи знака
		ASSERT_TRUE(yaml::encode(code, text)) << "знак " << code;
		// Прочитанный обратно знак Юникода
		uint32_t mirror = 0;
		// Выполняем обратное чтение записанного знака
		const size_t length = yaml::decode(text, 0, mirror);
		// Выполняем проверку того, что знак прочитан
		ASSERT_EQ(length, text.size()) << "знак " << code;
		// Выполняем проверку того, что знак прочитан тем же самым
		ASSERT_EQ(mirror, code);
	}
	// Собираемая последовательность знака
	string text;
	// Выполняем проверку отказа записи суррогата
	ASSERT_FALSE(yaml::encode(0xD800, text));
	// Выполняем проверку отказа записи знака за пределом набора Юникода
	ASSERT_FALSE(yaml::encode(0x110000, text));
	// Выполняем проверку того, что отказ записи ничего не дописал
	ASSERT_TRUE(text.empty());
}
/**
 * @brief Проверка принадлежности знаков к печатным
 *
 */
TEST(CodecYamlEncoding, Printable) {
	// Выполняем проверку знаков, описанием дозволенных
	ASSERT_TRUE(yaml::printable('\t'));
	// Выполняем проверку перевода строки
	ASSERT_TRUE(yaml::printable('\n'));
	// Выполняем проверку возврата каретки
	ASSERT_TRUE(yaml::printable('\r'));
	// Выполняем проверку пробела
	ASSERT_TRUE(yaml::printable(' '));
	// Выполняем проверку знака смены строки второго набора управляющих знаков
	ASSERT_TRUE(yaml::printable(0x85));
	// Выполняем проверку знака за основной таблицей
	ASSERT_TRUE(yaml::printable(0x1F600));
	// Выполняем проверку нулевого знака
	ASSERT_FALSE(yaml::printable(0x00));
	// Выполняем проверку управляющего знака
	ASSERT_FALSE(yaml::printable(0x01));
	// Выполняем проверку знака удаления
	ASSERT_FALSE(yaml::printable(0x7F));
	// Выполняем проверку знаков, описанием печатными не признаваемых
	ASSERT_FALSE(yaml::printable(0xFFFE));
	// Выполняем проверку знака за пределом набора Юникода
	ASSERT_FALSE(yaml::printable(0x110000));
}
/**
 * @brief Проверка опознания кодировки и независимости приведения от нарезки
 *
 * @details Проверка эта стережёт главное свойство потокового чтения: исход разбора не
 * вправе зависеть от того, как текст нарезан на куски при подаче. Нарезка перебирается
 * всеми размерами куска, а не одним
 *
 */
TEST(CodecYamlEncoding, Conversion) {
	/**
	 * Образцовые знаки: US-ASCII, кириллица и знак за основной таблицей
	 */
	const vector <uint32_t> codes = {
		'k', 'e', 'y', ':', ' ', 0x437, 0x43D, 0x430, 0x447, '\n', '-', ' ', 0x1F600, '\n'
	};
	// Получаем ожидаемый исход приведения
	const string expected = assemble(codes, yaml::encoding_t::UTF8, false);
	/**
	 * @brief Структура образца приводимого текста
	 *
	 */
	struct sample_t {
		// Кодировка образца приводимого текста
		yaml::encoding_t encoding;
		// Признак открытия образца меткой порядка байтов
		bool signature;
	};
	/**
	 * Образцы приводимого текста всеми признаваемыми кодировками
	 */
	const vector <sample_t> samples = {
		{yaml::encoding_t::UTF8, false}, {yaml::encoding_t::UTF8, true},
		{yaml::encoding_t::UTF16LE, false}, {yaml::encoding_t::UTF16LE, true},
		{yaml::encoding_t::UTF16BE, false}, {yaml::encoding_t::UTF16BE, true},
		{yaml::encoding_t::UTF32LE, false}, {yaml::encoding_t::UTF32LE, true},
		{yaml::encoding_t::UTF32BE, false}, {yaml::encoding_t::UTF32BE, true}
	};
	/**
	 * Выполняем перебор всех образцов приводимого текста
	 */
	for(const sample_t & sample : samples){
		// Собираем образец приводимого текста
		const string source = assemble(codes, sample.encoding, sample.signature);
		// Собираемый приведённый текст
		string whole;
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Выполняем приведение образца, поданного целиком
		ASSERT_TRUE(decoder.convert(source.data(), source.size(), true, whole))
			<< "кодировка " << static_cast <unsigned> (sample.encoding) << ": " << yaml::message(decoder.error());
		// Выполняем проверку опознанной кодировки
		ASSERT_EQ(decoder.encoding(), sample.encoding);
		// Выполняем проверку признака открытия текста меткой порядка байтов
		ASSERT_EQ(decoder.signature(), sample.signature);
		// Выполняем проверку приведённого текста
		ASSERT_EQ(whole, expected) << "кодировка " << static_cast <unsigned> (sample.encoding);
		/**
		 * Выполняем перебор всех размеров куска подачи
		 */
		for(size_t chunk = 1; chunk <= source.size(); chunk++){
			// Собираемый приведённый текст
			string piece;
			// Выполняем приведение образца, поданного кусками
			ASSERT_TRUE(piecemeal(source, chunk, piece))
				<< "кодировка " << static_cast <unsigned> (sample.encoding) << ", кусок " << chunk;
			// Выполняем проверку того, что нарезка исхода не изменила
			ASSERT_EQ(piece, expected)
				<< "кодировка " << static_cast <unsigned> (sample.encoding) << ", кусок " << chunk;
		}
	}
}
/**
 * @brief Проверка отказов приведения кодировки
 *
 */
TEST(CodecYamlEncoding, Refusals) {
	/**
	 * Выполняем проверку последовательности, оборванной концом текста
	 */
	{
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Собираемый приведённый текст
		string result;
		// Выполняем проверку отказа приведения оборванной последовательности
		ASSERT_FALSE(decoder.convert("\xD0", 1, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), yaml::error_t::INVALID_ENCODING);
		/**
		 * Выполняем проверку того, что отказ приведения терминален
		 *
		 * @note Кодировка сама собою не восстанавливается, и разбор дальнейших кусков
		 *       выдавал бы мусор вместо отказа
		 */
		ASSERT_FALSE(decoder.convert("A", 1, true, result));
	}
	/**
	 * Выполняем проверку одинокого старшего суррогата записи парами байтов
	 */
	{
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Собираемый приведённый текст
		string result;
		// Задаём кодировку исходного текста
		ASSERT_TRUE(decoder.encoding(yaml::encoding_t::UTF16LE));
		// Выполняем проверку отказа приведения одинокого суррогата
		ASSERT_FALSE(decoder.convert("\x00\xD8\x41\x00", 4, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), yaml::error_t::UNPAIRED_SURROGATE);
	}
	/**
	 * Выполняем проверку одинокого младшего суррогата записи парами байтов
	 */
	{
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Собираемый приведённый текст
		string result;
		// Задаём кодировку исходного текста
		ASSERT_TRUE(decoder.encoding(yaml::encoding_t::UTF16BE));
		// Выполняем проверку отказа приведения одинокого суррогата
		ASSERT_FALSE(decoder.convert("\xDC\x00\x00\x41", 4, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), yaml::error_t::UNPAIRED_SURROGATE);
	}
	/**
	 * Выполняем проверку старшего суррогата, пары так и не дождавшегося
	 */
	{
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Собираемый приведённый текст
		string result;
		// Задаём кодировку исходного текста
		ASSERT_TRUE(decoder.encoding(yaml::encoding_t::UTF16LE));
		// Выполняем проверку отказа приведения оборванной пары суррогатов
		ASSERT_FALSE(decoder.convert("\x00\xD8", 2, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), yaml::error_t::UNPAIRED_SURROGATE);
	}
}
/**
 * @brief Проверка навязывания кодировки исходного текста
 *
 * @details Навязанная кодировка старше опознания: текст без метки порядка байтов
 * опознаётся по расположению нулевых байтов, а опознание это ошибается на тексте, чей
 * первый знак записан одним байтом, а второй - многими
 *
 */
TEST(CodecYamlEncoding, Forced) {
	// Объект приведения кодировки исходного текста
	yaml::decoder_t decoder;
	// Собираемый приведённый текст
	string result;
	// Задаём кодировку исходного текста
	ASSERT_TRUE(decoder.encoding(yaml::encoding_t::UTF16BE));
	// Выполняем проверку того, что навязанная кодировка принята
	ASSERT_EQ(decoder.encoding(), yaml::encoding_t::UTF16BE);
	// Выполняем приведение текста, записанного парами байтов
	ASSERT_TRUE(decoder.convert("\x00\x41\x00\x42", 4, true, result));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "AB");
	/**
	 * Выполняем проверку того, что сброс навязанной кодировки не снимает
	 *
	 * @note Задаётся она до подачи текста, и сброс, её снимающий, отменял бы решение
	 *       потребителя молча
	 */
	decoder.reset();
	// Выполняем проверку того, что навязанная кодировка пережила сброс
	ASSERT_EQ(decoder.encoding(), yaml::encoding_t::UTF16BE);
	// Выполняем сброс собираемого приведённого текста
	result.clear();
	// Выполняем приведение текста, записанного парами байтов
	ASSERT_TRUE(decoder.convert("\x00\x43", 2, true, result));
	// Выполняем проверку приведённого текста
	ASSERT_EQ(result, "C");
}
/**
 * @brief Проверка признака ненадобности приведения
 *
 * @details Текст в UTF-8 приведения не требует вовсе, и признак этот дозволяет чтению
 * разбирать поданный кусок на месте, не платя за копирование его
 *
 */
TEST(CodecYamlEncoding, Direct) {
	// Объект приведения кодировки исходного текста
	yaml::decoder_t decoder;
	// Собираемый приведённый текст
	string result;
	/**
	 * Выполняем проверку того, что до опознания кодировки прямой разбор не обещается
	 */
	ASSERT_FALSE(decoder.direct());
	// Выполняем приведение текста, записанного кодировкой UTF-8
	ASSERT_TRUE(decoder.convert("key: value\n", 11, true, result));
	// Выполняем проверку того, что прямой разбор дозволен
	ASSERT_TRUE(decoder.direct());
	// Объект приведения кодировки исходного текста, записанного парами байтов
	yaml::decoder_t doubled;
	// Выполняем сброс собираемого приведённого текста
	result.clear();
	// Выполняем приведение текста, записанного парами байтов
	ASSERT_TRUE(doubled.convert("\x00\x41\x00\x42", 4, true, result));
	// Выполняем проверку того, что прямой разбор не дозволен
	ASSERT_FALSE(doubled.direct());
}
/**
 * @brief Проверка запрета знаков, описанием тексту не дозволенных
 *
 * @details Описание дозволяет тексту одни лишь печатные знаки, и знак иной есть отказ
 * приведения, а не содержимое: пропустив его, приведение выдало бы разбору знак,
 * которого в тексте YAML быть не может вовсе
 *
 * @note Проверка стоит в приведении, а не в разборе: приведение есть единственное
 *       место, через которое проходит всякий знак текста любой кодировки
 *
 */
TEST(CodecYamlEncoding, Forbidden) {
	/**
	 * Выполняем проверку запрета пустого знака в кодировке UTF-8
	 *
	 * @note Кодировка навязана извне нарочно: опознание по началу текста приняло бы
	 *       пустой байт за половину знака UTF-16 и отказало бы иначе - записью битой
	 */
	{
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Выполняем навязывание кодировки приведению
		decoder.encoding(yaml::encoding_t::UTF8);
		// Приведённый к UTF-8 текст
		string result;
		// Выполняем проверку отказа приведения пустого знака
		ASSERT_FALSE(decoder.convert("a\0b", 3, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), yaml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Выполняем проверку запрета управляющего знака в кодировке UTF-8
	 */
	{
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Выполняем навязывание кодировки приведению
		decoder.encoding(yaml::encoding_t::UTF8);
		// Приведённый к UTF-8 текст
		string result;
		// Выполняем проверку отказа приведения управляющего знака
		ASSERT_FALSE(decoder.convert("a\x01" "b", 3, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), yaml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Выполняем проверку запрета пустого знака в кодировке UTF-16
	 */
	{
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Приведённый к UTF-8 текст
		string result;
		// Выполняем проверку отказа приведения пустого знака парами байтов
		ASSERT_FALSE(decoder.convert("\xFF\xFE" "a\0\0\0", 6, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), yaml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Выполняем проверку признания знаков, описанием дозволенных
	 *
	 * @note Горизонтальная подача, перевод строки и возврат каретки печатными
	 *       признаются: без них не было бы ни строк, ни отступов
	 */
	{
		// Объект приведения кодировки исходного текста
		yaml::decoder_t decoder;
		// Выполняем навязывание кодировки приведению
		decoder.encoding(yaml::encoding_t::UTF8);
		// Приведённый к UTF-8 текст
		string result;
		// Выполняем проверку приведения знаков, описанием дозволенных
		ASSERT_TRUE(decoder.convert("a:\tб\r\nв\n", 10, true, result));
		// Выполняем проверку кода ошибки приведения
		ASSERT_EQ(decoder.error(), yaml::error_t::NONE);
	}
}

/**
 * @brief Проверка снятия метки порядка байтов при навязанной кодировке
 *
 * @details Навязывание кодировки значит «не гадай о кодировке», а не «оставь метку в
 *          тексте». Прежде снятие метки шло лишь по опознанию, и текст с меткою,
 *          кодировка которого навязана извне, отвергался разбором знаком U+FEFF в первом
 *          же значении своём. Проверка держит все пять кодировок и ведётся вдобавок
 *          подачей по одному байту: снятие метки обязано быть от нарезки на куски
 *          независимым
 *
 */
TEST(CodecYamlEncoding, ForcedSignature) {
	/**
	 * @brief Строение проверяемого случая
	 *
	 */
	struct Sample {
		// Навязываемая кодировка исходного текста
		yaml::encoding_t encoding;
		// Исходный текст вместе с меткою порядка байтов
		string text;
	};
	// Перечень проверяемых случаев
	const vector <Sample> samples = {
		{yaml::encoding_t::UTF8, string("\xEF\xBB\xBF" "a: 1\n")},
		{yaml::encoding_t::UTF16LE, string("\xFF\xFE" "a\0:\0 \0" "1\0\n\0", 12)},
		{yaml::encoding_t::UTF16BE, string("\xFE\xFF" "\0a\0:\0 \0" "1\0\n", 12)},
		{yaml::encoding_t::UTF32LE, string("\xFF\xFE\0\0" "a\0\0\0:\0\0\0 \0\0\0" "1\0\0\0\n\0\0\0", 24)},
		{yaml::encoding_t::UTF32BE, string("\0\0\xFE\xFF" "\0\0\0a\0\0\0:\0\0\0 \0\0\0" "1\0\0\0\n", 24)}
	};
	/**
	 * Выполняем перебор всех проверяемых случаев
	 */
	for(auto & sample : samples){
		/**
		 * Выполняем перебор размеров куска подачи
		 */
		for(const size_t chunk : {static_cast <size_t> (0), static_cast <size_t> (1), static_cast <size_t> (3)}){
			// Объект приведения кодировки исходного текста
			yaml::decoder_t decoder;
			// Выполняем навязывание кодировки приведению
			ASSERT_TRUE(decoder.encoding(sample.encoding));
			// Приведённый к UTF-8 текст
			string result;
			// Получаем размер куска подачи
			const size_t step = ((chunk == 0) ? sample.text.size() : chunk);
			/**
			 * Выполняем подачу текста кусками заданного размера
			 */
			for(size_t i = 0; i < sample.text.size(); i += step){
				// Получаем размер очередного куска подачи
				const size_t length = (((i + step) > sample.text.size()) ? (sample.text.size() - i) : step);
				// Выполняем приведение очередного куска
				ASSERT_TRUE(decoder.convert((sample.text.data() + i), length, ((i + length) >= sample.text.size()), result))
					<< "кодировка " << static_cast <unsigned> (sample.encoding) << ", кусок " << chunk;
			}
			// Выполняем проверку опознания метки порядка байтов
			ASSERT_TRUE(decoder.signature())
				<< "кодировка " << static_cast <unsigned> (sample.encoding) << ", кусок " << chunk;
			// Выполняем проверку того, что метка в приведённый текст не попала
			ASSERT_EQ(result, "a: 1\n")
				<< "кодировка " << static_cast <unsigned> (sample.encoding) << ", кусок " << chunk;
		}
	}
	/**
	 * Выполняем проверку того, что разбор текста с меткою при навязанной кодировке проходит
	 */
	{
		// Настройки разбора дерева с навязанной кодировкой
		yaml::document_t::settings_t settings;
		// Выполняем навязывание кодировки разбору
		settings.encoding = yaml::encoding_t::UTF8;
		// Объект дерева документа
		yaml::document_t doc(settings);
		// Выполняем разбор текста, меткою порядка байтов открытого
		ASSERT_TRUE(doc.parse(string("\xEF\xBB\xBF") + "имя: значение\n"));
		// Выполняем проверку собранного значения пары
		ASSERT_EQ(doc.root().at("/имя").text(), "значение");
	}
}
