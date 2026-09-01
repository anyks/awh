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

/**
 * Подключаем заголовочный файл выдачи пути во временном каталоге системы
 */
#include "../temporary.hpp"
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
	 * @brief Функция извлечения объекта журнала проверок
	 *
	 * @details Журнал заводится единожды на весь набор и гасится: проверки отказов
	 *          выводили бы записью всякий свой отказ, а их тут большинство. Гашение
	 *          это - настройка журнала, а не молчание модуля: модуль доносит как
	 *          обычно, а показывать ли - решает журнал
	 *
	 * @return объект журнала проверок
	 *
	 */
	const log_t * logger() noexcept {
		// Объект фреймворка проверок
		static fmk_t fmk;
		// Объект журнала проверок
		static log_t log(& fmk);
		// Признак выполненной настройки журнала
		static const bool ready = [](){
			// Выполняем гашение вывода журнала проверок
			log.level(log_t::level_t::NONE);
			// Выводим признак выполненной настройки
			return true;
		}();
		// Снимаем неиспользуемый признак настройки
		(void) ready;
		// Выводим объект журнала проверок
		return & log;
	}
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
				/**
				 * Выполняем снос временного файла проверки, ПРОШЛЫМ прогоном оставленного
				 *
				 * @details Снос этот - главный из двух, и стоит он тут не ради опрятности.
				 *          Проверка, читающая файл, ею же записанный, при пережившем файле
				 *          с верным содержимым проходит и тогда, когда запись НЕ РАБОТАЕТ
				 *          вовсе. Замер 01.09.2026: запись, отдающая успех и ничего не
				 *          записывающая, валит `StoreAndLoad` на чистом каталоге - и
				 *          ПРОХОДИТ, если файл прошлого прогона подложен
				 *
				 * @warning Сторожа с одним лишь деструктором тут недостаточно: он снимает
				 *          за собою, но не за прогоном, прерванным отказом утверждения либо
				 *          снятым процессом извне. Ровно такой файл и обращает проверку
				 *          записи в ложное зелёное
				 */
				(void) ::remove(this->_filename.c_str());
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
		abc::assembler_t assembler(::logger());
		/**
		 * Выполняем перебор всех собираемых записей контейнера
		 */
		for(const string & text : records){
			// Выполняем сборку очередной записи
			const vector <uint8_t> item = abc::value_t(text).dump();
			// Выполняем внесение очередной записи в собираемый контейнер
			EXPECT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT));
		}
		// Октеты собранного контейнера
		vector <uint8_t> result;
		// Выполняем завершение сборки контейнера
		EXPECT_TRUE(assembler.complete(result));
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
	abc::storage_t storage(::logger());
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
	abc::loader_t loader(::logger());
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
	abc::storage_t storage(::logger());
	// Выполняем запись собранного контейнера в файл
	ASSERT_TRUE(storage.store(file.filename(), container.data(), container.size()))
		<< "код отказа: " << abc::message(storage.error());
	// Правщик контейнера
	abc::editor_t editor(::logger());
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
	abc::storage_t reopened(::logger());
	// Выполняем открытие правленого файла контейнера
	ASSERT_TRUE(reopened.open(file.filename())) << "код отказа: " << abc::message(reopened.error());
	// Выборщик записей контейнера
	abc::fetcher_t fetcher(::logger());
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
	abc::editor_t again(::logger());
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
	abc::fetcher_t last(::logger());
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
	/**
	 * Временный файл проверки, ОТСУТСТВИЕ какого проверке и нужно
	 *
	 * @note Сторож берётся здесь не ради уборки за собою, а ради ОБЕСПЕЧЕНИЯ отсутствия:
	 *       проверка стоит на том, что файла нет, а имя было постоянным и путь
	 *       относительным. Файл с таким именем, оставленный в каталоге запуска, обращал
	 *       проверку в ложное КРАСНОЕ - отказ, зависящий от каталога и от чужого мусора,
	 *       а не от кодека. Замер 01.09.2026: подложенный файл валит `Refusals`
	 */
	const Temporary absent("missing-file");
	// Хранилище контейнера в файле
	abc::storage_t storage(::logger());
	// Выполняем проверку отказа открытия несуществующего файла
	ASSERT_FALSE(storage.open(absent.filename()));
	// Выполняем проверку кода отказа чтения октетов контейнера
	ASSERT_EQ(storage.error(), abc::error_t::UNREADABLE_SOURCE);
	// Правщик контейнера
	abc::editor_t editor(::logger());
	// Выполняем проверку отказа открытия контейнера закрытого файла
	ASSERT_FALSE(storage.bind(editor));
	// Выполняем проверку кода отказа чтения октетов контейнера
	ASSERT_EQ(storage.error(), abc::error_t::UNREADABLE_SOURCE);
	// Сниматель контейнера
	abc::loader_t loader(::logger());
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
/**
 * @brief Проверка заведения файла и записи октетов отданной работой
 *
 * @details Работа записи и работа чтения выдаются наружу и ведут файл со смещением:
 * правка кладёт кадры в конец, а оглавление и подпись перезаписывает на месте.
 * Заведение файла обязано усекать прежний: убранный контейнер короче правленного,
 * и уцелевший хвост прежнего был бы прочитан как продолжение нового
 *
 */
TEST(CodecAbcStorage, CreateAndWrite){
	// Временный файл проверки
	Temporary file("creation");
	// Образец записываемых октетов
	vector <uint8_t> sample(256, 0);
	/**
	 * Выполняем сборку образца записываемых октетов
	 */
	for(size_t i = 0; i < sample.size(); i++)
		// Устанавливаем очередной октет образца
		sample.at(i) = static_cast <uint8_t> (i & 0xFF);
	// Хранилище контейнера в файле
	abc::storage_t storage(::logger());
	// Выполняем заведение файла контейнера
	ASSERT_TRUE(storage.create(file.filename())) << "код отказа: " << abc::message(storage.error());
	// Выполняем проверку признака открытого хранилища
	ASSERT_TRUE(storage.opened());
	// Выполняем проверку названия файла контейнера
	ASSERT_EQ(storage.filename(), file.filename());
	// Выполняем проверку того, что заведённый файл пуст
	ASSERT_EQ(storage.length(), 0ull);
	// Выполняем получение работы записи октетов контейнера
	const abc::editor_t::sink_t sink = storage.sink();
	// Выполняем запись октетов образца с начала файла
	ASSERT_TRUE(sink(0, sample.data(), sample.size()));
	// Выполняем запись октетов образца со смещением
	ASSERT_TRUE(sink(sample.size(), sample.data(), sample.size()));
	// Выполняем сброс записанного на носитель
	ASSERT_TRUE(storage.flush());
	// Выполняем проверку полной длины контейнера на носителе
	ASSERT_EQ(storage.length(), static_cast <uint64_t> (sample.size() * 2));
	/**
	 * Выполняем проверку того, что запись внутри прежнего конца длины не наращивает:
	 * правка перезаписывает оглавление на месте, и рост длины на этом был бы ложным
	 */
	ASSERT_TRUE(sink(0, sample.data(), sample.size()));
	// Выполняем проверку неизменности полной длины контейнера
	ASSERT_EQ(storage.length(), static_cast <uint64_t> (sample.size() * 2));
	// Выполняем закрытие файла контейнера
	storage.close();
	// Выполняем проверку признака закрытого хранилища
	ASSERT_FALSE(storage.opened());
	// Хранилище проверки записанного
	abc::storage_t reopened(::logger());
	// Выполняем открытие записанного файла контейнера
	ASSERT_TRUE(reopened.open(file.filename())) << "код отказа: " << abc::message(reopened.error());
	// Выполняем проверку полной длины открытого контейнера
	ASSERT_EQ(reopened.length(), static_cast <uint64_t> (sample.size() * 2));
	// Выполняем получение работы чтения октетов контейнера
	const abc::editor_t::source_t source = reopened.source();
	// Буфер прочитанных октетов контейнера
	vector <uint8_t> buffer;
	// Выполняем чтение октетов с начала файла контейнера
	ASSERT_TRUE(source(0, sample.size(), buffer));
	// Выполняем проверку прочитанных октетов
	ASSERT_EQ(buffer, sample);
	// Выполняем чтение октетов со смещением
	ASSERT_TRUE(source(sample.size(), sample.size(), buffer));
	// Выполняем проверку прочитанных со смещением октетов
	ASSERT_EQ(buffer, sample);
	// Выполняем закрытие файла контейнера
	reopened.close();
	/**
	 * Выполняем проверку того, что заведение усекает прежний файл: длина
	 * заведённого наново хранилища обязана обнулиться
	 */
	abc::storage_t truncated(::logger());
	// Выполняем заведение файла контейнера поверх записанного
	ASSERT_TRUE(truncated.create(file.filename())) << "код отказа: " << abc::message(truncated.error());
	// Выполняем проверку того, что прежнее содержимое файла усечено
	ASSERT_EQ(truncated.length(), 0ull);
	// Выполняем сброс записанного на носитель
	ASSERT_TRUE(truncated.flush());
	// Выполняем закрытие файла контейнера
	truncated.close();
	// Хранилище проверки усечения
	abc::storage_t empty(::logger());
	// Выполняем открытие усечённого файла контейнера
	ASSERT_TRUE(empty.open(file.filename())) << "код отказа: " << abc::message(empty.error());
	// Выполняем проверку того, что хвост прежнего содержимого не уцелел
	ASSERT_EQ(empty.length(), 0ull);
}
/**
 * @brief Проверка отказов работ чтения и записи закрытого хранилища
 *
 * @details Работы эти выдаются наружу и переживают закрытие файла: потребитель
 * вправе держать их у себя, и отказ обязан быть внятным, а не обращением к
 * снесённому потоку
 *
 */
TEST(CodecAbcStorage, ClosedWorks){
	// Временный файл проверки
	Temporary file("closed");
	// Образец записываемых октетов
	const vector <uint8_t> sample(64, 0x5A);
	// Хранилище контейнера в файле
	abc::storage_t storage(::logger());
	// Выполняем заведение файла контейнера
	ASSERT_TRUE(storage.create(file.filename())) << "код отказа: " << abc::message(storage.error());
	// Выполняем получение работы записи октетов контейнера
	const abc::editor_t::sink_t sink = storage.sink();
	// Выполняем получение работы чтения октетов контейнера
	const abc::editor_t::source_t source = storage.source();
	// Выполняем запись октетов образца в заведённый файл
	ASSERT_TRUE(sink(0, sample.data(), sample.size()));
	// Выполняем сброс записанного на носитель
	ASSERT_TRUE(storage.flush());
	// Выполняем закрытие файла контейнера
	storage.close();
	// Выполняем проверку отказа записи у закрытого хранилища
	ASSERT_FALSE(sink(0, sample.data(), sample.size()));
	// Буфер прочитанных октетов контейнера
	vector <uint8_t> buffer;
	// Выполняем проверку отказа чтения у закрытого хранилища
	ASSERT_FALSE(source(0, sample.size(), buffer));
	// Выполняем проверку того, что буфер прочитанных октетов остался пуст
	ASSERT_TRUE(buffer.empty());
}
/**
 * @brief Проверка уборки мусора контейнера в отдельный файл
 *
 * @details Уборка ведётся в ЗАВЕДЁННЫЙ файл, а не поверх правимого: убранный
 * контейнер короче, и запись его поверх прежнего оставила бы хвост, читаемый
 * выборкой как продолжение оглавления
 *
 */
/**
 * @brief Проверка договора работ чтения и записи октетов хранилища
 *
 * @details Работы эти отдаются наружу и зовутся правкою, а не самим хранилищем, - оттого
 * договор их поверяется прямым зовом, а не через правку: пустой размер, отсутствующий
 * буфер да чтение за концом файла лежат на дорогах, куда правка не заходит вовсе
 *
 * @note Сторож границ чтения закрепляется здесь ИМЕННО потому, что заведён он замером:
 * длина приходит из самого контейнера и недоверенна, а порча её заставляла заводить
 * место под всё затребованное прежде отказа - 4 ГиБ на файле в две сотни октетов
 *
 */
TEST(CodecAbcStorage, BoundedWorks){
	// Временный файл проверки
	Temporary file("bounded");
	// Образец записываемых октетов
	const vector <uint8_t> sample(64, 0x5A);
	// Хранилище контейнера в файле
	abc::storage_t storage(::logger());
	// Выполняем заведение файла контейнера
	ASSERT_TRUE(storage.create(file.filename())) << "код отказа: " << abc::message(storage.error());
	// Выполняем получение работы записи октетов контейнера
	const abc::editor_t::sink_t sink = storage.sink();
	// Выполняем получение работы чтения октетов контейнера
	const abc::editor_t::source_t source = storage.source();
	// Выполняем запись октетов образца в заведённый файл
	ASSERT_TRUE(sink(0, sample.data(), sample.size()));
	// Выполняем сброс записанного на носитель
	ASSERT_TRUE(storage.flush());
	/**
	 * Записывать нечего - работа записи отвечает согласием, ничего не делая
	 */
	ASSERT_TRUE(sink(0, sample.data(), 0));
	/**
	 * Буфера записываемых октетов нет - работа записи отвечает отказом
	 */
	ASSERT_FALSE(sink(0, nullptr, sample.size()));
	// Буфер прочитанных октетов контейнера
	vector <uint8_t> buffer;
	/**
	 * Читать нечего - работа чтения отвечает согласием, буфера не касаясь
	 */
	ASSERT_TRUE(source(0, 0, buffer));
	// Буфер прочитанных октетов обязан остаться пустым
	ASSERT_TRUE(buffer.empty());
	/**
	 * Выполняем поверку сторожа границ чтения объявленной длиною в четыре гигабайта
	 *
	 * @details Утверждается ВРЕМЯ, а не исход: со снятым сторожем зов этот тоже отвечает
	 * отказом - место заводится, чтение возвращает недочитанное, и работа выходит ложью.
	 * Проверка на один исход проходила бы при снятом стороже, поверяя ровно ничего; разница
	 * же вся в том, заводится ли место. Замер 30.08.2026: со сторожем 9 мс, без него -
	 * 23 022 мс на заведении и очистке четырёх гигабайт
	 *
	 * @note Порог взят с двухсоткратным запасом к доброму пути и десятикратным к худому:
	 * времени тут мерить нечего вовсе, отказ обязан прийти немедля
	 */
	// Миг начала поверки сторожа границ чтения
	const auto started = chrono::steady_clock::now();
	// Работа чтения обязана ответить отказом на объявленную длину в четыре гигабайта
	ASSERT_FALSE(source(0, static_cast <size_t> (0xFFFFFFFF), buffer));
	// Выполняем получение времени, потраченного работою чтения на отказ
	const auto spent = chrono::duration_cast <chrono::milliseconds> (chrono::steady_clock::now() - started);
	// Буфер прочитанных октетов обязан остаться пустым
	ASSERT_TRUE(buffer.empty());
	// Отказ обязан прийти без заведения места, то есть немедля
	ASSERT_LT(spent.count(), 2000) << "отказ занял " << spent.count() << " мс: сторож границ чтения не сработал";
	/**
	 * Чтение за концом файла отвечает отказом и по смещению
	 */
	ASSERT_FALSE(source(static_cast <uint64_t> (sample.size()), 1, buffer));
	// Буфер прочитанных октетов обязан остаться пустым
	ASSERT_TRUE(buffer.empty());
	/**
	 * Чтение, выходящее за конец файла хвостом, отвечает отказом
	 */
	ASSERT_FALSE(source(static_cast <uint64_t> (sample.size() - 1), 2, buffer));
	// Буфер прочитанных октетов обязан остаться пустым
	ASSERT_TRUE(buffer.empty());
	/**
	 * Чтение в пределах файла отвечает согласием
	 */
	ASSERT_TRUE(source(0, sample.size(), buffer));
	// Прочитанные октеты обязаны отвечать записанным
	ASSERT_EQ(buffer, sample);
}
TEST(CodecAbcStorage, CompactToFile){
	// Временный файл правимого контейнера
	Temporary origin("compact-origin");
	// Временный файл убранного контейнера
	Temporary target("compact-target");
	// Выполняем сборку контейнера с четырьмя записями
	const vector <uint8_t> container = build({"первая", "вторая", "третья", "четвёртая"});
	// Хранилище правимого контейнера
	abc::storage_t source(::logger());
	// Выполняем запись собранного контейнера в файл
	ASSERT_TRUE(source.store(origin.filename(), container.data(), container.size()))
		<< "код отказа: " << abc::message(source.error());
	// Правщик контейнера
	abc::editor_t editor(::logger());
	// Выполняем открытие контейнера файла правщиком
	ASSERT_TRUE(source.bind(editor)) << "код отказа: " << abc::message(editor.error());
	// Выполняем снос третьей записи контейнера
	ASSERT_TRUE(editor.erase(2)) << "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Выполняем сброс записанного на носитель
	ASSERT_TRUE(source.flush());
	// Выполняем проверку того, что мусор правкой накопился
	ASSERT_GT(editor.garbage(), 0ull);
	// Хранилище убранного контейнера
	abc::storage_t cleaned(::logger());
	// Выполняем заведение файла убранного контейнера
	ASSERT_TRUE(cleaned.create(target.filename())) << "код отказа: " << abc::message(cleaned.error());
	// Полная длина убранного контейнера
	uint64_t length = 0;
	// Выполняем уборку мусора перестройкой контейнера в заведённый файл
	ASSERT_TRUE(editor.compact(cleaned.sink(), abc::payload_t::TEXT, length))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем сброс записанного на носитель
	ASSERT_TRUE(cleaned.flush());
	// Выполняем проверку того, что длина убранного совпала с длиной файла
	ASSERT_EQ(cleaned.length(), length);
	// Выполняем проверку того, что убранный контейнер вышел короче правленного
	ASSERT_LT(cleaned.length(), source.length());
	// Выполняем закрытие файла правимого контейнера
	source.close();
	// Выполняем закрытие файла убранного контейнера
	cleaned.close();
	// Хранилище проверки убранного контейнера
	abc::storage_t reopened(::logger());
	// Выполняем открытие убранного файла контейнера
	ASSERT_TRUE(reopened.open(target.filename())) << "код отказа: " << abc::message(reopened.error());
	// Выполняем проверку полной длины убранного контейнера
	ASSERT_EQ(reopened.length(), length);
	// Выборщик записей убранного контейнера
	abc::fetcher_t fetcher(::logger());
	// Выполняем открытие контейнера отданной работой чтения
	ASSERT_TRUE(fetcher.open(reopened.source())) << "код отказа: " << abc::message(fetcher.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем выборку первой записи убранного контейнера
	ASSERT_TRUE(fetcher.record(0, picked)) << "код отказа: " << abc::message(fetcher.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, abc::value_t(string{"первая"}).dump());
	/**
	 * Выполняем проверку того, что номера записей уборкой не сдвинулись: строка
	 * снесённой записи сохранена пустою, а номера живут и вне контейнера
	 */
	ASSERT_TRUE(fetcher.record(3, picked)) << "код отказа: " << abc::message(fetcher.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, abc::value_t(string{"четвёртая"}).dump());
	// Выполняем проверку отказа выборки снесённой записи из убранного контейнера
	ASSERT_FALSE(fetcher.record(2, picked));
	// Выполняем проверку кода отказа выборки снесённой записи
	ASSERT_EQ(fetcher.error(), abc::error_t::MISSING_RECORD);
}
/**
 * @brief Проверка договора хранилища при незаведённом файле и недоступном пути
 *
 * @details Пути отказа ввода-вывода покрытием не брались вовсе: работы хранилища
 *          отвечали отказом молча, и разойдись договор их с объявленным - никто бы
 *          того не заметил. Проверка эта берёт те из них, что достижимы без порчи
 *          носителя: обращение к незаведённому файлу и заведение по пути, какого нет
 *
 * @note Пути, требующие ОТКАЗА самой записи на носитель (переполнение раздела, отказ
 *       `fwrite`), сюда не входят: воспроизвести их без чужой оснастки нечем, и
 *       проверка на них вышла бы либо ненадёжной, либо непереносимой
 */
TEST(CodecAbcStorage, RefusalsWithoutMedium){
	/**
	 * Выполняем проверку того, что подача незаведённого файла отвечена отказом
	 *
	 * @note Подача переводит поток файла в начало первым же делом, и у незаведённого
	 * перевод этот отвечает отказом. Дорога взята подачею, а не переводом напрямую:
	 * перевод есть работа внутренняя, и проверке до неё дела нет
	 */
	{
		// Хранилище контейнера на носителе
		abc::storage_t storage(::logger());
		// Снимальщик контейнера с носителя
		abc::loader_t loader(::logger());
		// Файл хранилища заведён быть не должен
		ASSERT_FALSE(storage.opened());
		// Полная длина незаведённого контейнера обязана быть нулевой
		ASSERT_EQ(storage.length(), static_cast <uint64_t> (0));
		// Подача незаведённого файла обязана быть отвечена отказом
		ASSERT_FALSE(storage.load(loader));
		// Отказ обязан быть объявлен невозможностью чтения
		ASSERT_EQ(storage.error(), abc::error_t::UNREADABLE_SOURCE);
	}
	/**
	 * Выполняем проверку того, что заведение файла по несуществующему пути отвечено отказом
	 */
	{
		// Хранилище контейнера на носителе
		abc::storage_t storage(::logger());
		// Заведение файла по несуществующему пути обязано быть отвечено отказом
		ASSERT_FALSE(storage.create("/несуществующий-каталог-проверки/контейнер.abc"));
		// Отказ обязан быть объявлен невозможностью записи
		ASSERT_EQ(storage.error(), abc::error_t::UNWRITABLE_SINK);
		// Файл хранилища заведён быть не должен
		ASSERT_FALSE(storage.opened());
	}
	/**
	 * Выполняем проверку того, что открытие несуществующего файла отвечено отказом
	 */
	{
		// Хранилище контейнера на носителе
		abc::storage_t storage(::logger());
		// Открытие несуществующего файла обязано быть отвечено отказом
		ASSERT_FALSE(storage.open("/несуществующий-каталог-проверки/контейнер.abc"));
		// Отказ обязан быть объявлен невозможностью чтения
		ASSERT_EQ(storage.error(), abc::error_t::UNREADABLE_SOURCE);
	}
	/**
	 * Выполняем проверку того, что укладка без буфера отвечена внутренним отказом
	 *
	 * @note Поверка буфера стоит ПРЕЖДЕ заведения файла, и это существенно: заведи
	 * работа файл прежде поверки - на носителе оставался бы пустой файл от вызова,
	 * отвергнутого заведомо
	 */
	{
		// Хранилище контейнера на носителе
		abc::storage_t storage(::logger());
		// Укладка без буфера обязана быть отвечена отказом
		ASSERT_FALSE(storage.store(temporary("abc-проверка-без-буфера.abc"), nullptr, 16));
		// Отказ обязан быть объявлен внутренним
		ASSERT_EQ(storage.error(), abc::error_t::INTERNAL);
		// Файл хранилища заведён быть не должен
		ASSERT_FALSE(storage.opened());
	}
	/**
	 * Выполняем проверку того, что укладка по несуществующему пути отвечена отказом
	 */
	{
		// Хранилище контейнера на носителе
		abc::storage_t storage(::logger());
		// Буфер укладываемых октетов
		const vector <uint8_t> record(16, 0xAB);
		// Укладка по несуществующему пути обязана быть отвечена отказом
		ASSERT_FALSE(storage.store("/несуществующий-каталог-проверки/контейнер.abc",
		 record.data(), record.size()));
		// Отказ обязан быть объявлен невозможностью записи
		ASSERT_EQ(storage.error(), abc::error_t::UNWRITABLE_SINK);
	}
}
