/**
 * @file abc-container.cpp
 * @date 2026-09-01
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
 * @brief Инструмент фаззинга СЛОЯ КОНТЕЙНЕРА бинарного кодека ABC — заголовка опознания,
 *        кадров, строк оглавления и записи подписи
 *
 * @details Ворошитель `abc.cpp` трогает слой ЗАПИСИ: сборку, разбор и дерево документа.
 *          Слой контейнера он не трогает вовсе - ни заголовка, ни кадров, ни оглавления,
 *          ни подписи. Между тем именно этот слой читает октеты с НОСИТЕЛЯ, то есть с
 *          чужой стороны: заголовок разбирается прежде всего прочего и всяким, кто держит
 *          контейнер в руках
 *
 * @details Устройство ворошителя: собирается годный контейнер, затем октеты его портятся
 *          точечной заменой, и всякая работа слоя обязана ответить ОТКАЗОМ либо выдать
 *          прежнее содержимое. Третьего - выдачи содержимого ИНОГО под видом годного -
 *          быть не должно, и оно ловится сличением с исходными записями
 *
 * @note Порча ведётся точечной заменой октета, а не случайным потоком: случайный поток
 *       отвергается опознавательной записью на первом же октете, и слои за нею не
 *       достигаются вовсе
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <random>
#include <memory>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <codec/abc/abc.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Зерно источника случайных чисел по умолчанию
	 *
	 * @note Зерно постоянно намеренно: прогон обязан быть повторимым, а иное зерно
	 *       подаётся вторым доводом
	 *
	 */
	constexpr uint64_t SEED = 0x41424331ull;

	/**
	 * @brief Количество записей в собираемом контейнере
	 *
	 */
	constexpr size_t RECORDS = 12;

	/**
	 * @brief Счётчики прогона ворошителя
	 *
	 */
	struct Tally {
		// Количество прогонов порчи
		uint64_t rounds;
		// Количество отказов слоя контейнера
		uint64_t refused;
		// Количество прогонов, где порча пришлась на место безразличное
		uint64_t survived;
		// Количество расхождений содержимого
		uint64_t diverged;
		/**
		 * @brief Конструктор
		 *
		 */
		Tally() noexcept : rounds(0), refused(0), survived(0), diverged(0) {}
	};

	/**
	 * @brief Объект фреймворка
	 *
	 */
	unique_ptr <fmk_t> Fmk;

	/**
	 * @brief Объект журнала
	 *
	 */
	unique_ptr <log_t> Journal;

	/**
	 * @brief Объект сжатия данных
	 *
	 */
	unique_ptr <compressor::block_t> Squeezer;

	/**
	 * @brief Объект шифрования данных
	 *
	 */
	unique_ptr <crypto_t> Cipher;

	/**
	 * @brief Функция сборки записи по её номеру
	 *
	 * @param number номер собираемой записи
	 * @return       собранная запись
	 *
	 */
	vector <uint8_t> item(const size_t number) noexcept {
		// Выводим собранную запись владеющего значения
		return abc::value_t(string{"запись номер "} + to_string(number)).dump();
	}

	/**
	 * @brief Функция сборки годного контейнера
	 *
	 * @param signed   признак того, что контейнер следует подписать
	 * @param buffer   буфер, куда следует уложить собранный контейнер
	 * @param original буфер, куда следует уложить исходные записи контейнера
	 * @return         признак успешно собранного контейнера
	 *
	 */
	bool assemble(const bool sign, vector <uint8_t> & buffer, vector <uint8_t> & original) noexcept {
		// Сборщик контейнера
		abc::assembler_t assembler(Journal.get());
		// Выполняем установку модуля сжатия сборщику контейнера
		assembler.compressor(Squeezer.get());
		/**
		 * Если контейнер следует подписать
		 */
		if(sign)
			// Выполняем объявление подписи собираемого контейнера
			assembler.sign(Cipher.get(), "владелец");
		// Получаем настройки сборки контейнера
		abc::assembler_t::settings_t settings = assembler.settings();
		// Выполняем установку порога накопления, дающего несколько кадров
		settings.block = 64;
		// Выполняем установку настроек сборки контейнера
		assembler.settings(settings);
		// Выполняем очистку исходного содержимого записей
		original.clear();
		/**
		 * Выполняем перебор собираемых записей контейнера
		 */
		for(size_t i = 0; i < RECORDS; i++){
			// Собираемая запись контейнера
			const vector <uint8_t> record = item(i);
			// Выполняем внесение записи в исходное содержимое
			original.insert(original.end(), record.begin(), record.end());
			/**
			 * Если внесение записи в собираемый контейнер отвечено отказом
			 */
			if(!assembler.append(record.data(), record.size(), abc::payload_t::TEXT))
				// Выводим признак неудачной сборки контейнера
				return false;
		}
		// Выводим результат завершения сборки контейнера
		return assembler.complete(buffer);
	}

	/**
	 * @brief Функция вычерпывания записей контейнера загрузчиком
	 *
	 * @param buffer  октеты разбираемого контейнера
	 * @param records буфер, куда следует уложить вычерпанные записи
	 * @return        признак того, что контейнер вычерпан без отказа
	 *
	 */
	bool drain(const vector <uint8_t> & buffer, vector <uint8_t> & records) noexcept {
		// Загрузчик контейнера
		abc::loader_t loader(Journal.get());
		// Выполняем установку модуля сжатия загрузчику контейнера
		loader.compressor(Squeezer.get());
		// Выполняем очистку собираемого содержимого кадров
		records.clear();
		/**
		 * Если подача октетов контейнера отвечена отказом
		 */
		if(!loader.feed(buffer.data(), buffer.size()))
			// Выводим признак неудачного вычерпывания
			return false;
		// Содержимое очередного кадра контейнера
		vector <uint8_t> payload;
		// Сведения об очередном кадре контейнера
		abc::chunk_t chunk;
		/**
		 * Выполняем вычерпывание кадров контейнера
		 */
		while(loader.next(payload, chunk))
			// Выполняем внесение содержимого кадра в собираемое содержимое
			records.insert(records.end(), payload.begin(), payload.end());
		// Выводим признак того, что контейнер вычерпан без отказа
		return (loader.error() == abc::error_t::NONE);
	}

	/**
	 * @brief Функция подачи испорченных октетов работам слоя контейнера
	 *
	 * @details Работы эти читают октеты с носителя, и всякая обязана ответить отказом
	 *          либо снять годное. Падение, чтение за концом буфера и выдача мусора за
	 *          годное суть дефекты, и первые два ловит надзиратель памяти
	 *
	 * @param buffer подаваемые октеты
	 *
	 */
	void submit(const vector <uint8_t> & buffer) noexcept {
		// Код отказа снятия заголовка опознания
		abc::error_t error = abc::error_t::NONE;
		// Снимаемый заголовок опознания контейнера
		abc::header_t header;
		// Выполняем быстрое опознание поданных октетов
		(void) abc::probe(buffer.data(), buffer.size());
		// Выполняем снятие заголовка опознания контейнера
		const bool taken = header.unpack(buffer.data(), buffer.size(), error);
		/**
		 * Если заголовок опознания снят, идём по объявленным им местам
		 */
		if(taken){
			// Укладчик кадров контейнера
			abc::packer_t packer(Journal.get());
			// Выполняем установку модуля сжатия укладчику кадров
			packer.compressor(Squeezer.get());
			// Смещение снятия кадра
			size_t offset = static_cast <size_t> (abc::HEADER_LENGTH);
			// Снятое содержимое кадра
			vector <uint8_t> payload;
			// Снятые сведения о кадре
			abc::chunk_t chunk;
			/**
			 * Выполняем подрядное снятие кадров тела контейнера
			 */
			while((offset < buffer.size()) && packer.unpack(buffer.data(), buffer.size(), offset, payload, chunk))
				// Продолжаем снятие кадров тела контейнера
				continue;
			/**
			 * Если заголовок объявил место оглавления
			 */
			if((header.index >= static_cast <uint64_t> (abc::HEADER_LENGTH)) &&
			 (header.index < static_cast <uint64_t> (buffer.size()))){
				// Оглавление контейнера
				abc::index_t index(Journal.get());
				// Смещение снятия кадра оглавления
				size_t place = static_cast <size_t> (header.index);
				// Снятое содержимое кадра оглавления
				vector <uint8_t> content;
				// Сведения о кадре оглавления
				abc::chunk_t entry;
				/**
				 * Если кадр оглавления снят, разбираем строки его
				 */
				if(packer.unpack(buffer.data(), buffer.size(), place, content, entry))
					// Выполняем снятие строк оглавления контейнера
					(void) index.unpack(content.data(), content.size(), error);
			}
			/**
			 * Если заголовок объявил место подписи
			 */
			if((header.signature >= static_cast <uint64_t> (abc::HEADER_LENGTH)) &&
			 (header.signature < static_cast <uint64_t> (buffer.size()))){
				// Смещение записи подписи за заголовком кадра её
				const size_t start = static_cast <size_t> (header.signature) + abc::CHUNK_HEADER;
				/**
				 * Если октетов достаёт на запись подписи
				 */
				if(start < buffer.size()){
					// Снятая запись подписи контейнера
					abc::sign_t sign;
					// Выполняем снятие записи подписи контейнера
					(void) abc::unpack(buffer.data() + start, buffer.size() - start, sign, error);
				}
			}
		}
	}

	/**
	 * @brief Функция прогона одного круга порчи контейнера
	 *
	 * @param origin   октеты годного контейнера
	 * @param original исходное содержимое записей контейнера
	 * @param source   источник случайных чисел
	 * @param tally    счётчики прогона ворошителя
	 * @return         признак того, что круг прошёл без расхождений
	 *
	 */
	bool round(const vector <uint8_t> & origin, const vector <uint8_t> & original,
	 mt19937_64 & source, Tally & tally) noexcept {
		// Испорченные октеты контейнера
		vector <uint8_t> broken = origin;
		// Количество портимых октетов: от одного до трёх
		const size_t count = (1 + (source() % 3));
		/**
		 * Выполняем порчу выбранных октетов контейнера
		 */
		for(size_t i = 0; i < count; i++){
			// Смещение портимого октета
			const size_t place = static_cast <size_t> (source() % broken.size());
			// Выполняем порчу выбранного октета
			broken.at(place) = static_cast <uint8_t> (source() & 0xFF);
		}
		// Выполняем учёт прогона порчи
		tally.rounds++;
		// Выполняем подачу испорченных октетов работам слоя контейнера
		submit(broken);
		// Вычерпанное содержимое испорченного контейнера
		vector <uint8_t> records;
		/**
		 * Если испорченный контейнер вычерпан с отказом, дефекта нет: отказ есть
		 * законный ответ на порчу, и ради него сторожа и стоят
		 */
		if(!drain(broken, records)){
			// Выполняем учёт отказа слоя контейнера
			tally.refused++;
			// Выводим признак того, что круг прошёл без расхождений
			return true;
		}
		/**
		 * Если испорченный контейнер вычерпан БЕЗ отказа, содержимое его обязано
		 * отвечать исходному: порча пришлась на место безразличное
		 */
		if(records == original){
			// Выполняем учёт прогона, где порча пришлась на место безразличное
			tally.survived++;
			// Выводим признак того, что круг прошёл без расхождений
			return true;
		}
		// Выполняем учёт найденного расхождения содержимого
		tally.diverged++;
		// Выводим признак расхождения содержимого
		return false;
	}
};

/**
 * @brief Функция запуска ворошителя слоя контейнера
 *
 * @param argc количество передаваемых аргументов
 * @param argv буфер параметров
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Количество прогонов порчи
	uint64_t count = 20000;
	/**
	 * Если количество прогонов задано доводом
	 */
	if(argc > 1)
		// Выполняем снятие количества прогонов порчи
		count = static_cast <uint64_t> (::strtoull(argv[1], nullptr, 10));
	// Зерно источника случайных чисел
	uint64_t seed = SEED;
	/**
	 * Если зерно источника случайных чисел задано доводом
	 */
	if(argc > 2)
		// Выполняем снятие зерна источника случайных чисел
		seed = static_cast <uint64_t> (::strtoull(argv[2], nullptr, 0));
	// Выполняем заведение объекта фреймворка
	Fmk = make_unique <fmk_t> ();
	// Выполняем заведение объекта журнала
	Journal = make_unique <log_t> (Fmk.get());
	// Выполняем отключение вывода журнала: отказы здесь ожидаемы и часты
	Journal->mode({});
	// Выполняем заведение объекта сжатия данных
	Squeezer = make_unique <compressor::block_t> (Journal.get());
	// Выполняем заведение объекта шифрования данных
	Cipher = make_unique <crypto_t> (Fmk.get(), Journal.get());
	// Выполняем установку соли шифрования
	Cipher->salt("соль ворошителя");
	// Выполняем установку пароля шифрования
	Cipher->password("пароль ворошителя");
	/**
	 * Если ключ владельца контейнера завести не вышло
	 */
	if(!Cipher->generateKey("владелец", crypto_t::signature_t::ED25519)){
		// Выводим сообщение о неудачном заведении ключа владельца
		::fprintf(stderr, "abc container fuzz: the key of the owner has not been generated\n");
		// Выходим из приложения с признаком неудачи
		return 1;
	}
	// Источник случайных чисел
	mt19937_64 source(seed);
	// Счётчики прогона ворошителя
	Tally tally;
	/**
	 * Выполняем перебор двух видов контейнера: подписанного и без подписи
	 */
	for(size_t kind = 0; kind < 2; kind++){
		// Октеты годного контейнера
		vector <uint8_t> origin;
		// Исходное содержимое записей контейнера
		vector <uint8_t> original;
		/**
		 * Если годный контейнер собрать не вышло
		 */
		if(!assemble((kind > 0), origin, original)){
			// Выводим сообщение о неудачной сборке контейнера
			::fprintf(stderr, "abc container fuzz: the assembling of a container has failed\n");
			// Выходим из приложения с признаком неудачи
			return 1;
		}
		// Вычерпанное содержимое годного контейнера
		vector <uint8_t> records;
		/**
		 * Если годный контейнер вычерпан с отказом либо содержимое его разошлось
		 */
		if(!drain(origin, records) || (records != original)){
			// Выводим сообщение о негодном опорном контейнере
			::fprintf(stderr, "abc container fuzz: the pristine container does not drain\n");
			// Выходим из приложения с признаком неудачи
			return 1;
		}
		/**
		 * Выполняем перебор прогонов порчи контейнера
		 */
		for(uint64_t i = 0; i < count; i++){
			/**
			 * Если круг порчи дал расхождение содержимого
			 */
			if(!round(origin, original, source, tally)){
				// Выводим сообщение о найденном расхождении содержимого
				::fprintf(stderr, "abc container fuzz: a corrupted container has drained into a DIFFERENT content "
				 "(round %llu, kind %zu, seed 0x%llx)\n", static_cast <unsigned long long> (i), kind,
				 static_cast <unsigned long long> (seed));
				// Выходим из приложения с признаком неудачи
				return 1;
			}
		}
	}
	// Выводим итоги прогона ворошителя
	::printf("abc container fuzz: прогонов %llu, отказов %llu, безразличных %llu, расхождений %llu\n",
	 static_cast <unsigned long long> (tally.rounds), static_cast <unsigned long long> (tally.refused),
	 static_cast <unsigned long long> (tally.survived), static_cast <unsigned long long> (tally.diverged));
	// Выходим из приложения с признаком успеха
	return 0;
}
