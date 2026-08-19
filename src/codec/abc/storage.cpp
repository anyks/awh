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
 * \~russian
 * @brief Реализация хранения бинарного контейнера ABC в файле
 *
 * \~english
 * @brief Implementation of the storing of the ABC binary container in a file
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <codec/abc/storage.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cerrno>

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
	 * @brief Функция перевода потока файла на заданное смещение
	 *
	 * @details Смещение берётся шириною в восемь октетов: контейнер вправе перевалить
	 * за два гигабайта, а работа `fseek` берёт лишь длинное целое, какого на то у иных
	 * систем не хватает
	 *
	 * @param stream переводимый поток файла
	 * @param offset смещение в файле
	 * @return       признак успешности перевода
	 *
	 */
	bool position(FILE * stream, const uint64_t offset) noexcept {
		/**
		 * Если сборка выполняется под Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем перевод потока файла на заданное смещение
			return (::_fseeki64(stream, static_cast <__int64> (offset), SEEK_SET) == 0);
		/**
		 * Если сборка выполняется под операционную систему семейства POSIX
		 */
		#else
			// Выполняем перевод потока файла на заданное смещение
			return (::fseeko(stream, static_cast <off_t> (offset), SEEK_SET) == 0);
		#endif
	}
	/**
	 * @brief Функция получения размера файла
	 *
	 * @note Название `extent` здесь занято стандартной библиотекой: свойство типа
	 *       `std::extent` видно через подписку на пространство имён, и вызов ушёл бы
	 *       не туда молча
	 *
	 * @param stream поток файла
	 * @param result размер файла в октетах
	 * @return       признак успешности получения размера
	 *
	 */
	bool measure(FILE * stream, uint64_t & result) noexcept {
		/**
		 * Если сборка выполняется под Windows
		 */
		#if _WIN32 || _WIN64
			// Если перевод потока файла в конец не удался
			if(::_fseeki64(stream, 0, SEEK_END) != 0)
				// Выводим признак неудачного получения размера
				return false;
			// Выполняем получение смещения конца файла
			const __int64 offset = ::_ftelli64(stream);
		/**
		 * Если сборка выполняется под операционную систему семейства POSIX
		 */
		#else
			// Если перевод потока файла в конец не удался
			if(::fseeko(stream, 0, SEEK_END) != 0)
				// Выводим признак неудачного получения размера
				return false;
			// Выполняем получение смещения конца файла
			const off_t offset = ::ftello(stream);
		#endif
		// Если смещение конца файла получить не удалось
		if(offset < 0)
			// Выводим признак неудачного получения размера
			return false;
		// Выполняем установку размера файла
		result = static_cast <uint64_t> (offset);
		// Выводим признак успешного получения размера
		return true;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Storage::Storage() noexcept :
 _stream(nullptr), _length(0), _error(error_t::NONE) {}
/**
 * @brief Деструктор
 *
 */
awh::codec::abc::Storage::~Storage() noexcept {
	// Выполняем закрытие файла контейнера
	this->close();
}
/**
 * @brief Метод перевода потока файла на заданное смещение
 *
 * @param offset смещение в файле контейнера
 * @return       признак успешности перевода
 *
 */
bool awh::codec::abc::Storage::seek(const uint64_t offset) noexcept {
	// Если файл контейнера не открыт
	if(this->_stream == nullptr)
		// Выводим признак неудачного перевода
		return false;
	// Выводим результат перевода потока файла на заданное смещение
	return ::position(this->_stream, offset);
}
/**
 * @brief Метод открытия существующего файла контейнера
 *
 * @param filename название открываемого файла контейнера
 * @return         признак успешно открытого файла
 *
 */
bool awh::codec::abc::Storage::open(const string & filename) noexcept {
	// Выполняем закрытие ранее открытого файла контейнера
	this->close();
	// Выполняем сброс кода отказа работы с файлом
	this->_error = error_t::NONE;
	// Выполняем открытие файла контейнера на чтение вместе с записью
	this->_stream = ::fopen(filename.c_str(), "r+b");
	/**
	 * Если открыть файл контейнера не удалось
	 */
	if(this->_stream == nullptr){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачно открытого файла
		return false;
	}
	/**
	 * Если получить размер файла контейнера не удалось
	 */
	if(!::measure(this->_stream, this->_length)){
		// Выполняем закрытие файла контейнера
		this->close();
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачно открытого файла
		return false;
	}
	// Выполняем запоминание названия файла контейнера
	this->_filename = filename;
	// Выводим признак успешно открытого файла
	return true;
}
/**
 * @brief Метод заведения нового файла контейнера
 *
 * @param filename название заводимого файла контейнера
 * @return         признак успешно заведённого файла
 *
 */
bool awh::codec::abc::Storage::create(const string & filename) noexcept {
	// Выполняем закрытие ранее открытого файла контейнера
	this->close();
	// Выполняем сброс кода отказа работы с файлом
	this->_error = error_t::NONE;
	// Выполняем заведение файла контейнера на чтение вместе с записью
	this->_stream = ::fopen(filename.c_str(), "w+b");
	/**
	 * Если завести файл контейнера не удалось
	 */
	if(this->_stream == nullptr){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выводим признак неудачно заведённого файла
		return false;
	}
	// Выполняем запоминание названия файла контейнера
	this->_filename = filename;
	// Выполняем сброс полной длины контейнера
	this->_length = 0;
	// Выводим признак успешно заведённого файла
	return true;
}
/**
 * @brief Метод закрытия файла контейнера
 *
 */
void awh::codec::abc::Storage::close() noexcept {
	/**
	 * Если файл контейнера открыт
	 */
	if(this->_stream != nullptr){
		// Выполняем закрытие файла контейнера
		(void) ::fclose(this->_stream);
		// Выполняем сброс потока файла контейнера
		this->_stream = nullptr;
	}
	// Выполняем очистку названия файла контейнера
	this->_filename.clear();
	// Выполняем сброс полной длины контейнера
	this->_length = 0;
}
/**
 * @brief Метод сброса записанного на носитель
 *
 * @return признак успешности сброса
 *
 */
bool awh::codec::abc::Storage::flush() noexcept {
	// Если файл контейнера не открыт
	if(this->_stream == nullptr)
		// Выводим признак неудачного сброса
		return false;
	/**
	 * Если сбросить записанное на носитель не удалось
	 */
	if(::fflush(this->_stream) != 0){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выводим признак неудачного сброса
		return false;
	}
	// Выводим признак успешного сброса
	return true;
}
/**
 * @brief Метод выдачи работы чтения октетов контейнера
 *
 * @return работа чтения октетов контейнера
 *
 */
awh::codec::abc::editor_t::source_t awh::codec::abc::Storage::source() noexcept {
	/**
	 * Выводим работу чтения октетов контейнера
	 *
	 * @param offset смещение читаемых октетов
	 * @param size   размер читаемых октетов
	 * @param result буфер, куда следует положить прочитанное
	 * @return       признак успешности чтения
	 *
	 */
	return [this](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем очистку буфера прочитанных октетов
		result.clear();
		// Если файл контейнера не открыт
		if(this->_stream == nullptr)
			// Выводим признак неудачного чтения
			return false;
		// Если читать нечего
		if(size == 0)
			// Выводим признак успешного чтения
			return true;
		// Если перевод потока файла на заданное смещение не удался
		if(!this->seek(offset))
			// Выводим признак неудачного чтения
			return false;
		// Выполняем заведение места под прочитанные октеты
		result.resize(size);
		/**
		 * Если прочитать затребованные октеты не удалось.
		 *
		 * Сюда же уходит и чтение за концом контейнера: прочитано будет меньше
		 * затребованного, а усечённая выдача была бы неотличима от целой, и потребитель
		 * принял бы обрывок за запись. Отдельной проверки на длину оттого и нет - она
		 * ничего бы не решала
		 */
		if(::fread(result.data(), 1, size, this->_stream) != size){
			// Выполняем очистку буфера прочитанных октетов
			result.clear();
			// Выводим признак неудачного чтения
			return false;
		}
		// Выводим признак успешного чтения
		return true;
	};
}
/**
 * @brief Метод выдачи работы записи октетов контейнера
 *
 * @return работа записи октетов контейнера
 *
 */
awh::codec::abc::editor_t::sink_t awh::codec::abc::Storage::sink() noexcept {
	/**
	 * Выводим работу записи октетов контейнера
	 *
	 * @param offset смещение записываемых октетов
	 * @param buffer буфер записываемых октетов
	 * @param size   размер записываемых октетов
	 * @return       признак успешности записи
	 *
	 */
	return [this](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Если файл контейнера не открыт
		if(this->_stream == nullptr)
			// Выводим признак неудачной записи
			return false;
		// Если записывать нечего
		if(size == 0)
			// Выводим признак успешной записи
			return true;
		// Если буфер записываемых октетов не существует
		if(buffer == nullptr)
			// Выводим признак неудачной записи
			return false;
		// Если перевод потока файла на заданное смещение не удался
		if(!this->seek(offset))
			// Выводим признак неудачной записи
			return false;
		// Если записать поданные октеты не удалось
		if(::fwrite(buffer, 1, size, this->_stream) != size)
			// Выводим признак неудачной записи
			return false;
		/**
		 * Если запись вышла за прежний конец контейнера, длина его растёт: правка
		 * дописывает кадры в конец, и длина эта нужна ей при следующем открытии
		 */
		if((offset + static_cast <uint64_t> (size)) > this->_length)
			// Выполняем наращивание полной длины контейнера
			this->_length = (offset + static_cast <uint64_t> (size));
		// Выводим признак успешной записи
		return true;
	};
}
/**
 * @brief Метод открытия контейнера файла правкой
 *
 * @param editor правка, какой открывается контейнер
 * @return       признак успешно открытого контейнера
 *
 */
bool awh::codec::abc::Storage::bind(editor_t & editor) noexcept {
	/**
	 * Если файл контейнера не открыт
	 */
	if(this->_stream == nullptr){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачно открытого контейнера
		return false;
	}
	// Выполняем открытие контейнера файла правкой
	return editor.open(this->source(), this->sink(), this->_length);
}
/**
 * @brief Метод записи собранного контейнера в файл
 *
 * @param filename название заводимого файла контейнера
 * @param buffer   буфер октетов собранного контейнера
 * @param size     размер октетов собранного контейнера
 * @return         признак успешности записи
 *
 */
bool awh::codec::abc::Storage::store(const string & filename, const void * buffer, const size_t size) noexcept {
	// Если буфер октетов собранного контейнера не существует
	if((buffer == nullptr) && (size > 0)){
		// Выполняем установку кода внутреннего отказа
		this->_error = error_t::INTERNAL;
		// Выводим признак неудачной записи
		return false;
	}
	// Если завести файл контейнера не удалось
	if(!this->create(filename))
		// Выводим признак неудачной записи
		return false;
	/**
	 * Если записать октеты собранного контейнера не удалось
	 */
	if((size > 0) && (::fwrite(buffer, 1, size, this->_stream) != size)){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выполняем закрытие файла контейнера
		this->close();
		// Выводим признак неудачной записи
		return false;
	}
	// Выполняем установку полной длины контейнера
	this->_length = static_cast <uint64_t> (size);
	// Выводим результат сброса записанного на носитель
	return this->flush();
}
/**
 * @brief Метод подачи файла контейнера снимателю
 *
 * @param loader сниматель, какому подаётся файл контейнера
 * @param block  размер подаваемого куска в октетах
 * @return       признак успешности подачи
 *
 */
bool awh::codec::abc::Storage::load(loader_t & loader, const size_t block) noexcept {
	/**
	 * Если файл контейнера не открыт
	 */
	if(this->_stream == nullptr){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачной подачи
		return false;
	}
	// Если размер подаваемого куска не задан
	if(block == 0){
		// Выполняем установку кода внутреннего отказа
		this->_error = error_t::INTERNAL;
		// Выводим признак неудачной подачи
		return false;
	}
	// Если перевод потока файла в начало не удался
	if(!this->seek(0)){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачной подачи
		return false;
	}
	// Буфер подаваемого куска контейнера
	vector <uint8_t> buffer(block, 0);
	/**
	 * Выполняем подачу файла контейнера кусками
	 */
	while(true){
		// Выполняем чтение очередного куска контейнера
		const size_t size = ::fread(buffer.data(), 1, buffer.size(), this->_stream);
		/**
		 * Если файл контейнера исчерпан. Кусок, прочитанный не целиком, подаётся как
		 * есть, а конец опознаётся следующим чтением: отдельной проверки на неполный
		 * кусок оттого и нет - она ничего бы не решала
		 */
		if(size == 0)
			// Выполняем прекращение подачи
			break;
		/**
		 * Если подача очередного куска отвечена отказом
		 */
		if(!loader.feed(buffer.data(), size)){
			// Выполняем установку кода отказа снятия контейнера
			this->_error = loader.error();
			// Выводим признак неудачной подачи
			return false;
		}
	}
	// Выводим признак успешной подачи
	return true;
}
/**
 * @brief Метод проверки открытости файла контейнера
 *
 * @return признак открытого файла контейнера
 *
 */
bool awh::codec::abc::Storage::opened() const noexcept {
	// Выводим признак открытого файла контейнера
	return (this->_stream != nullptr);
}
/**
 * @brief Метод извлечения полной длины контейнера на носителе
 *
 * @return полная длина контейнера в октетах
 *
 */
uint64_t awh::codec::abc::Storage::length() const noexcept {
	// Выводим полную длину контейнера на носителе
	return this->_length;
}
/**
 * @brief Метод извлечения названия файла контейнера
 *
 * @return название файла контейнера
 *
 */
const string & awh::codec::abc::Storage::filename() const noexcept {
	// Выводим название файла контейнера
	return this->_filename;
}
/**
 * @brief Метод извлечения кода отказа работы с файлом контейнера
 *
 * @return код отказа работы с файлом контейнера
 *
 */
awh::codec::abc::error_t awh::codec::abc::Storage::error() const noexcept {
	// Выводим код отказа работы с файлом контейнера
	return this->_error;
}
