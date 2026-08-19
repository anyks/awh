/**
 * @file: abc-storage.cpp
 * @date: 2026-08-19
 *
 * @brief Щуп файлового слоя хранилища ABC для стендов, где библиотека целиком не собрана
 *
 * @details Щуп проверяет только слой работы с файлом: заведение, запись, чтение,
 *          перемотку, длину и отказы. Работы `bind` и `load`, что уводят в правку и
 *          в разбор кадров, здесь НЕ проверяются — они тянут за собой сжатие и
 *          шифрование, ради чего пришлось бы собирать библиотеку целиком. Оттого
 *          опоры на них ниже и заведены пустыми: щуп их не зовёт, а связывание без
 *          них не проходит
 *
 * @copyright: Copyright © 2026
 */

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

#include <codec/abc/storage.hpp>

using namespace std;
using namespace awh;

/**
 * Пустые опоры работ, каких щуп не зовёт
 */
namespace awh {
	namespace codec {
		namespace abc {
			bool Editor::open(source_t source, sink_t sink, const uint64_t length) noexcept {
				(void) source; (void) sink; (void) length;
				return false;
			}
			bool Loader::feed(const void * buffer, const size_t size) noexcept {
				(void) buffer; (void) size;
				return false;
			}
			error_t Loader::error() const noexcept {
				return error_t::NONE;
			}
		}
	}
}

/**
 * Счётчики прогона
 */
static size_t passed = 0, failed = 0;

/**
 * @brief Функция проверки утверждения
 *
 * @param name   название проверки
 * @param result итог проверки
 *
 */
static void check(const char * name, const bool result) noexcept {
	// Если проверка пройдена
	if(result){
		// Увеличиваем счётчик пройденных
		passed++;
		// Выводим итог проверки
		::printf("[  OK  ] %s\n", name);
	// Если проверка провалена
	} else {
		// Увеличиваем счётчик проваленных
		failed++;
		// Выводим итог проверки
		::printf("[ FAIL ] %s\n", name);
	}
	// Выталкиваем вывод, ибо оболочка Windows держит его до конца работы
	::fflush(stdout);
}

/**
 * @brief Работа щупа
 *
 * @return код возврата
 *
 */
int main() noexcept {
	// Название файла хранилища
	const string filename = "abc-storage-probe.bin";
	// Образец записываемых октетов
	vector <uint8_t> sample(8192, 0);
	/**
	 * Заполняем образец узнаваемым содержимым
	 */
	for(size_t i = 0; i < sample.size(); i++)
		// Устанавливаем очередной октет образца
		sample.at(i) = static_cast <uint8_t> (i & 0xFF);
	/**
	 * Заведение хранилища и запись
	 */
	{
		// Хранилище контейнера
		codec::abc::storage_t storage;
		// Проверяем заведение файла хранилища
		check("создание файла", storage.create(filename));
		// Проверяем признак открытого хранилища
		check("признак открытого хранилища", storage.opened());
		// Проверяем название файла хранилища
		check("название файла хранилища", storage.filename() == filename);
		// Выполняем получение работы записи октетов
		codec::abc::editor_t::sink_t sink = storage.sink();
		// Проверяем запись октетов с начала файла
		check("запись октетов", sink(0, sample.data(), sample.size()));
		// Проверяем запись октетов со смещением
		check("запись октетов со смещением", sink(sample.size(), sample.data(), sample.size()));
		// Проверяем выталкивание записанного на носитель
		check("выталкивание на носитель", storage.flush());
		// Проверяем длину хранилища
		check("длина хранилища", storage.length() == static_cast <uint64_t> (sample.size() * 2));
	}
	/**
	 * Открытие уже заведённого хранилища и чтение
	 */
	{
		// Хранилище контейнера
		codec::abc::storage_t storage;
		// Проверяем открытие уже заведённого файла
		check("открытие заведённого файла", storage.open(filename));
		// Проверяем длину открытого хранилища
		check("длина открытого хранилища", storage.length() == static_cast <uint64_t> (sample.size() * 2));
		// Выполняем получение работы чтения октетов
		codec::abc::editor_t::source_t source = storage.source();
		// Буфер прочитанных октетов
		vector <uint8_t> buffer;
		// Проверяем чтение октетов с начала файла
		check("чтение октетов", source(0, sample.size(), buffer) && (buffer == sample));
		// Выполняем очистку буфера прочитанных октетов
		buffer.clear();
		// Проверяем чтение октетов со смещением
		check("чтение октетов со смещением", source(sample.size(), sample.size(), buffer) && (buffer == sample));
		// Выполняем очистку буфера прочитанных октетов
		buffer.clear();
		// Проверяем отказ чтения за концом файла
		check("отказ чтения за концом файла", !source(static_cast <uint64_t> (sample.size() * 4), sample.size(), buffer));
		// Выполняем закрытие хранилища
		storage.close();
		// Проверяем признак закрытого хранилища
		check("признак закрытого хранилища", !storage.opened());
	}
	/**
	 * Запись собранного контейнера целиком
	 */
	{
		// Хранилище контейнера
		codec::abc::storage_t storage;
		// Проверяем запись буфера в файл целиком
		check("запись буфера в файл", storage.store(filename, sample.data(), sample.size()));
		// Хранилище проверки записанного
		codec::abc::storage_t reopened;
		// Проверяем открытие записанного файла
		check("открытие записанного файла", reopened.open(filename));
		// Проверяем длину записанного файла
		check("длина записанного файла", reopened.length() == static_cast <uint64_t> (sample.size()));
	}
	/**
	 * Отказы
	 */
	{
		// Хранилище контейнера
		codec::abc::storage_t storage;
		// Проверяем отказ открытия несуществующего файла
		check("отказ открытия несуществующего файла", !storage.open("abc-storage-probe-missing.bin"));
		// Проверяем отказ работы записи у закрытого хранилища
		check("отказ записи у закрытого хранилища", !storage.sink()(0, sample.data(), sample.size()));
		// Буфер прочитанных октетов
		vector <uint8_t> buffer;
		// Проверяем отказ работы чтения у закрытого хранилища
		check("отказ чтения у закрытого хранилища", !storage.source()(0, sample.size(), buffer));
	}
	// Выполняем удаление файла хранилища
	::remove(filename.c_str());
	// Выводим итог прогона
	::printf("\nПройдено: %zu, провалено: %zu\n", passed, failed);
	// Выводим код возврата
	return (failed > 0 ? 1 : 0);
}
