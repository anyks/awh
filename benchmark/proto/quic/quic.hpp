/**
 * @file: quic.hpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл окружения бенчмарков протокола QUIC —
 *        объявление вспомогательных средств подготовки соединений,
 *        криптографического контекста и общих параметров сценариев измерения
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_PROTO_QUIC_BENCHMARK__
#define __AWH_PROTO_QUIC_BENCHMARK__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/fmk.hpp"
#include "../../../include/sys/log.hpp"
#include "../../../include/net/tls/coder.hpp"
#include "../../../include/proto/quic/connection.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён бенчмарков
	 *
	 */
	namespace benchmark {
		/**
		 * @brief Пространство имён бенчмарков транспортного протокола QUIC
		 *
		 */
		namespace quic {
			/**
			 * @brief Функция подсчёта выделений памяти
			 *
			 * @note Учёт ведётся перегруженными операторами выделения памяти
			 *       в единице трансляции точки входа бенчмарков QUIC
			 *
			 * @param count количество выполненных выделений
			 * @param bytes суммарный объём выделенной памяти в октетах
			 *
			 */
			void allocations(size_t & count, size_t & bytes) noexcept;
			/**
			 * @brief Функция управления учётом выделений памяти
			 *
			 * @param mode режим учёта выделений памяти
			 *
			 */
			void counting(const bool mode) noexcept;
			/**
			 * @brief Класс окружения транспортной безопасности бенчмарка
			 *
			 * @details Криптография соединения QUIC задаётся шаблоном контекста
			 *          кодера, поэтому бенчмарку нужна готовая пара шаблонов -
			 *          клиентский и серверный. Окружение создаётся однократно на
			 *          весь прогон: генерация сертификата и построение контекста
			 *          к измеряемой работе отношения не имеют и в замер попадать
			 *          не должны
			 *
			 */
			class Security {
				private:
					// Путь к файлу сертификата узла бенчмарка
					std::string _certificate;
					// Путь к файлу приватного ключа узла бенчмарка
					std::string _privateKey;
				private:
					// Идентификатор шаблона контекста безопасности клиента
					awh::tls::Coder::id_t _client;
					// Идентификатор шаблона контекста безопасности сервера
					awh::tls::Coder::id_t _server;
				private:
					// Объект кодера транспортной безопасности
					awh::tls::Coder _coder;
				public:
					/**
					 * @brief Метод доступа к объекту кодера транспортной безопасности
					 *
					 * @return объект кодера транспортной безопасности
					 *
					 */
					awh::tls::Coder & coder() noexcept;
					/**
					 * @brief Метод извлечения шаблона контекста безопасности роли эндпоинта
					 *
					 * @param endpoint роль эндпоинта
					 * @return         идентификатор шаблона контекста безопасности
					 *
					 */
					awh::tls::Coder::id_t context(const awh::quic::endpoint_t endpoint) const noexcept;
				public:
					/**
					 * Запрещаем копирование и перемещение (окружение владеет контекстами кодера)
					 */
					Security(const Security &) = delete;
					Security(Security &&) = delete;
					Security & operator = (const Security &) = delete;
					Security & operator = (Security &&) = delete;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 *
					 */
					explicit Security(const awh::fmk_t * fmk, const awh::log_t * log) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Security() noexcept;
			};
			/**
			 * @brief Функция получения окружения транспортной безопасности бенчмарка
			 *
			 * @return окружение транспортной безопасности бенчмарка
			 *
			 */
			Security & security() noexcept;
			/**
			 * @brief Функция подготовки соединения к бенчмарку
			 *
			 * @note Криптография задана шаблоном контекста кодера, из которого
			 *       создано соединение, поэтому подготовка сводится к
			 *       транспортным параметрам
			 *
			 * @param connection объект соединения
			 *
			 */
			void configure(awh::quic::connection_t & connection) noexcept;
			/**
			 * @brief Функция выполнения хендшейка между клиентом и сервером
			 *
			 * @param client эндпоинт клиента
			 * @param server эндпоинт сервера
			 * @param now    текущее время тестовых часов в миллисекундах
			 * @return       результат установления соединения
			 *
			 */
			bool establish(awh::quic::connection_t & client, awh::quic::connection_t & server, uint64_t & now) noexcept;
		};
	};
};

#endif // __AWH_PROTO_QUIC_BENCHMARK__
