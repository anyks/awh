/**
 * @file tls.cpp
 * @date 2026-07-22
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
 * @brief Реализация тестовой фикстуры модуля транспортного уровня безопасности —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>

/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/macro/win32.hpp>
#endif

/**
 * Подключаем заголовочный файл тестов кодера
 */
#include "tls.hpp"
#include "../../cert.hpp"
#include <sys/log.hpp>

/**
 * @brief Внутренние вспомогательные средства тестов кодера
 *
 */
namespace {
	/**
	 * @brief Функция получения пути к временному каталогу
	 *
	 * @return путь к временному каталогу
	 *
	 * @note У MS Windows путь берётся у самой системы, а не из окружения. Оболочка MSYS2
	 *       выставляет там TEMP и TMP по правилам POSIX (значением "/tmp"), но собранный
	 *       под MinGW двоичный файл - родной для Windows, и такой путь ему непонятен:
	 *       он раскрывается в "C:\\tmp", какого на машине нет вовсе
	 *
	 */
	static std::string directory() noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Буфер под путь к временному каталогу
			char buffer[MAX_PATH + 1]{0};
			// Получаем путь к временному каталогу у системы
			const DWORD size = ::GetTempPathA(static_cast <DWORD> (sizeof(buffer)), buffer);
			// Если путь к временному каталогу получен
			if((size > 0) && (size <= MAX_PATH)){
				// Формируем путь к временному каталогу
				std::string result(buffer, static_cast <size_t> (size));
				// Удаляем завершающий разделитель, добавляемый системой
				while(!result.empty() && ((result.back() == '\\') || (result.back() == '/')))
					// Удаляем завершающий разделитель
					result.pop_back();
				// Выводим путь к временному каталогу
				return result;
			}
			// Выводим запасной путь к временному каталогу
			return std::string(".");
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Получаем путь к временному каталогу из окружения
			const char * result = ::getenv("TMPDIR");
			// Выводим путь к временному каталогу
			return ((result != nullptr) ? std::string(result) : std::string("/tmp"));
		#endif
	}
	/**
	 * @brief Функция записи данных во временный файл
	 *
	 * @param path путь к создаваемому файлу
	 * @param data записываемые данные
	 * @return     результат записи
	 *
	 */
	static bool store(const std::string & path, const std::string & data) noexcept {
		// Открываем файл на запись
		FILE * file = ::fopen(path.c_str(), "wb");
		// Если файл не открыт
		if(file == nullptr)
			// Выводим отрицательный результат
			return false;
		// Записываем данные в файл
		const bool result = (::fwrite(data.data(), 1, data.size(), file) == data.size());
		// Закрываем файл
		::fclose(file);
		// Выводим результат записи
		return result;
	}
};

/**
 * @brief Метод настройки тестового окружения
 *
 */
void TlsFixture::SetUp(){
	/**
	 * Отключаем вывод логов в тестах: часть проверок намеренно подаёт кодеру
	 * некорректные данные, и его диагностика забивала бы вывод теста
	 */
	awh::log::level(awh::log::level_t::NONE);
	// Выполняем генерацию самоподписанного сертификата тестового узла
	this->makeCertificate(this->_certificate, this->_privateKey);
}
/**
 * @brief Метод очистки тестового окружения
 *
 */
void TlsFixture::TearDown(){
	// Если файл сертификата создан
	if(!this->_certificate.empty())
		// Удаляем файл сертификата
		::remove(this->_certificate.c_str());
	// Если файл приватного ключа создан
	if(!this->_privateKey.empty())
		// Удаляем файл приватного ключа
		::remove(this->_privateKey.c_str());
	// Освобождаем объект для работы с логами
	// Освобождаем объект фреймворка
}
/**
 * @brief Метод записи самоподписанного сертификата во временные файлы
 *
 * @param certificate путь к созданному файлу сертификата
 * @param privateKey  путь к созданному файлу приватного ключа
 * @param host        доменное имя субъекта сертификата
 * @return            результат генерации
 *
 */
	bool TlsFixture::makeCertificate(std::string & certificate, std::string & privateKey, const std::string & host) const noexcept {
		// Формируем основу пути к временным файлам
		const std::string base = (::directory() + "/awh-tls-" + host + "-" + std::to_string(reinterpret_cast <uintptr_t> (this)));
		// Устанавливаем путь к файлу сертификата
		certificate = (base + ".crt");
		// Устанавливаем путь к файлу приватного ключа
		privateKey = (base + ".key");
		// Записываем сертификат и приватный ключ во временные файлы
		return (::store(certificate, std::string(::testCertificate())) && ::store(privateKey, std::string(::testPrivateKey())));
	}
