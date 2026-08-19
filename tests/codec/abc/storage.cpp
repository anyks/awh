/**
 * @file storage.cpp
 * @date 2026-08-19
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
 * @brief Проверки хранения бинарного контейнера ABC в файле
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstdio>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
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
	 * @brief Класс временного файла проверки
	 *
	 * @details Файл сносится деструктором: проверка, оставившая файл на диске, портит
	 * соседние прогоны, а прогон под правкой сноса и вовсе застаёт чужое содержимое
	 *
	 */
	class Temporary {
		private:
			// Название временного файла проверки
			string _filename;
		public:
			/**
			 * @brief Метод извлечения названия временного файла
			 *
			 * @return название временного файла проверки
			 *
			 */
			const string & filename() const noexcept {
				// Выводим название временного файла проверки
				return this->_filename;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param name отличительная часть названия временного файла
			 *
			 */
			explicit Temporary(const string & name) noexcept {
				// Выполняем сборку названия временного файла проверки
				this->_filename = ("abc-storage-" + name + ".bin");
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Temporary() noexcept {
				// Выполняем снос временного файла проверки
				(void) ::remove(this->_filename.c_str());
			}
	};
	/**
	 * @brief Функция сборки контейнера с поданными записями
	 *
	 * @param records собираемые записи контейнера
	 * @return        октеты собранного контейнера
	 *
	 */
	vector <uint8_t> build(const vector <string> & records) noexcept {
		// Сборщик контейнера
		abc::assembler_t assembler;
		/**
		 * Выполняем перебор всех собираемых записей контейнера
		 */
		for(const string & text : records){
			// Выполняем сборку очередной записи
			const vector <uint8_t> item = abc::value_t(text).dump();
			// Выполняем внесение очередной записи в собираемый контейнер
			assembler.append(item.data(), item.size(), abc::payload_t::TEXT);
		}
		// Октеты собранного контейнера
		vector <uint8_t> result;
		// Выполняем завершение сборки контейнера
		assembler.complete(result);
		// Выводим октеты собранного контейнера
		return result;
	}
};

/**
 * @brief Проверка записи собранного контейнера в файл и снятия его обратно
 *
 */
TEST(CodecAbcStorage, StoreAndLoad){
	// Временный файл проверки
	Temporary file("roundtrip");
	// Выполняем сборку контейнера с двумя записями
	const vector <uint8_t> container = build({"первая", "вторая"});
	// Хранилище контейнера в файле
	abc::storage_t storage;
	// Выполняем запись собранного контейнера в файл
	ASSERT_TRUE(storage.store(file.filename(), container.data(), container.size()))
		<< "код отказа: " << abc::message(storage.error());
	// Выполняем проверку полной длины контейнера на носителе
	ASSERT_EQ(storage.length(), static_cast <uint64_t> (container.size()));
	// Выполняем проверку названия файла контейнера
	ASSERT_EQ(storage.filename(), file.filename());
	// Выполняем закрытие файла контейнера
	storage.close();
	// Выполняем проверку закрытости файла контейнера
	ASSERT_FALSE(storage.opened());
	// Выполняем открытие записанного файла контейнера
	ASSERT_TRUE(storage.open(file.filename())) << "код отказа: " << abc::message(storage.error());
	// Выполняем проверку полной длины открытого контейнера
	ASSERT_EQ(storage.length(), static_cast <uint64_t> (container.size()));
	// Сниматель контейнера
	abc::loader_t loader;
	/**
	 * Выполняем подачу файла контейнера снимателю кусками по семь октетов: снятие
	 * ведётся потоком, и от нарезки на куски оно зависеть не вправе
	 */
	ASSERT_TRUE(storage.load(loader, 7)) << "код отказа: " << abc::message(storage.error());
	// Снятое содержимое кадров контейнера
	vector <uint8_t> taken;
	// Количество снятых кадров контейнера
	size_t count = 0;
	// Буфер содержимого очередного кадра
	vector <uint8_t> content;
	// Снятые сведения о кадре
	abc::chunk_t chunk;
	/**
	 * Выполняем снятие всех кадров контейнера
	 */
	while(loader.next(content, chunk)){
		// Выполняем учёт снятого кадра контейнера
		count++;
		// Выполняем накопление содержимого снятого кадра
		taken.insert(taken.end(), content.begin(), content.end());
	}
	// Выполняем проверку того, что кадры контейнера сняты
	ASSERT_GT(count, 0ul);
	// Собираемое ожидаемое содержимое кадров
	vector <uint8_t> expected;
	/**
	 * Выполняем сборку ожидаемого содержимого кадров.
	 *
	 * Кадр несёт записи подряд, а не по одной: сборка копит их до размера кадра, и
	 * снятие выдаёт кадры, а не записи. Выборка записи по номеру ведётся оглавлением
	 */
	for(const string & text : {string{"первая"}, string{"вторая"}}){
		// Выполняем сборку очередной записи
		const vector <uint8_t> item = abc::value_t(text).dump();
		// Выполняем накопление записи в ожидаемом содержимом
		expected.insert(expected.end(), item.begin(), item.end());
	}
	// Выполняем проверку снятого содержимого кадров контейнера
	ASSERT_EQ(taken, expected);
}
/**
 * @brief Проверка правки контейнера в файле на месте
 *
 */
TEST(CodecAbcStorage, EditInPlace){
	// Временный файл проверки
	Temporary file("editing");
	// Выполняем сборку контейнера с двумя записями
	const vector <uint8_t> container = build({"первая", "вторая"});
	// Хранилище контейнера в файле
	abc::storage_t storage;
	// Выполняем запись собранного контейнера в файл
	ASSERT_TRUE(storage.store(file.filename(), container.data(), container.size()))
		<< "код отказа: " << abc::message(storage.error());
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера файла правщиком
	ASSERT_TRUE(storage.bind(editor)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку количества записей открытого контейнера
	ASSERT_EQ(editor.records(), 2ull);
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"третья"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Выполняем сброс записанного на носитель
	ASSERT_TRUE(storage.flush());
	/**
	 * Выполняем проверку того, что контейнер в файле вырос, а не переписан целиком:
	 * правка ведётся дописыванием, и прежние кадры остаются на месте
	 */
	ASSERT_GT(storage.length(), static_cast <uint64_t> (container.size()));
	// Выполняем закрытие файла контейнера
	storage.close();
	// Хранилище правленого контейнера
	abc::storage_t reopened;
	// Выполняем открытие правленого файла контейнера
	ASSERT_TRUE(reopened.open(file.filename())) << "код отказа: " << abc::message(reopened.error());
	// Выборщик записей контейнера
	abc::fetcher_t fetcher;
	// Выполняем открытие контейнера отданной работой чтения
	ASSERT_TRUE(fetcher.open(reopened.source())) << "код отказа: " << abc::message(fetcher.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем выборку дописанной записи с носителя
	ASSERT_TRUE(fetcher.record(2, picked)) << "код отказа: " << abc::message(fetcher.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
	// Выполняем выборку записи, лежавшей в контейнере прежде правки
	ASSERT_TRUE(fetcher.record(0, picked)) << "код отказа: " << abc::message(fetcher.error());
	// Выполняем проверку того, что прежняя запись правкой не тронута
	ASSERT_EQ(picked, abc::value_t(string{"первая"}).dump());
	// Правщик открытого наново контейнера
	abc::editor_t again;
	/**
	 * Выполняем открытие правщиком контейнера, открытого работой открытия, а не
	 * заведения: правка ведётся на месте, и одного чтения ей мало - файл обязан быть
	 * открыт на чтение ВМЕСТЕ с записью
	 */
	ASSERT_TRUE(reopened.bind(again)) << "код отказа: " << abc::message(again.error());
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> fourth = abc::value_t(string{"четвёртая"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(again.append(fourth.data(), fourth.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(again.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(again.commit()) << "код отказа: " << abc::message(again.error());
	// Выполняем сброс записанного на носитель
	ASSERT_TRUE(reopened.flush());
	// Выборщик записей правленого наново контейнера
	abc::fetcher_t last;
	// Выполняем открытие контейнера отданной работой чтения
	ASSERT_TRUE(last.open(reopened.source())) << "код отказа: " << abc::message(last.error());
	// Выполняем выборку дописанной наново записи
	ASSERT_TRUE(last.record(3, picked)) << "код отказа: " << abc::message(last.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, fourth);
}
/**
 * @brief Проверка отказов работы с файлом контейнера
 *
 */
TEST(CodecAbcStorage, Refusals){
	// Хранилище контейнера в файле
	abc::storage_t storage;
	// Выполняем проверку отказа открытия несуществующего файла
	ASSERT_FALSE(storage.open("abc-storage-missing-file.bin"));
	// Выполняем проверку кода отказа чтения октетов контейнера
	ASSERT_EQ(storage.error(), abc::error_t::UNREADABLE_SOURCE);
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем проверку отказа открытия контейнера закрытого файла
	ASSERT_FALSE(storage.bind(editor));
	// Выполняем проверку кода отказа чтения октетов контейнера
	ASSERT_EQ(storage.error(), abc::error_t::UNREADABLE_SOURCE);
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем проверку отказа подачи закрытого файла снимателю
	ASSERT_FALSE(storage.load(loader));
	// Временный файл проверки
	Temporary file("refusals");
	// Выполняем сборку контейнера с одной записью
	const vector <uint8_t> container = build({"единственная"});
	// Выполняем запись собранного контейнера в файл
	ASSERT_TRUE(storage.store(file.filename(), container.data(), container.size()));
	// Выполняем проверку отказа подачи нулевыми кусками
	ASSERT_FALSE(storage.load(loader, 0));
	// Выполняем проверку кода внутреннего отказа
	ASSERT_EQ(storage.error(), abc::error_t::INTERNAL);
	// Буфер прочитанных октетов контейнера
	vector <uint8_t> result;
	// Выполняем получение работы чтения октетов контейнера
	const abc::editor_t::source_t source = storage.source();
	/**
	 * Выполняем проверку отказа чтения за концом контейнера: прочитано будет меньше
	 * затребованного, а усечённая выдача была бы неотличима от целой, и потребитель
	 * принял бы обрывок за запись
	 */
	ASSERT_FALSE(source(storage.length() - 4, 8, result));
	// Выполняем проверку того, что буфер прочитанных октетов остался пуст
	ASSERT_TRUE(result.empty());
	// Выполняем проверку чтения октетов, лежащих в пределах контейнера
	ASSERT_TRUE(source(0, 8, result));
	// Выполняем проверку размера прочитанных октетов
	ASSERT_EQ(result.size(), 8ul);
}
