/**
 * @file container.cpp
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
 * @brief Замеры контейнера ABC — сборка кадров, сжатие, шифрование, выборка записи по
 *        номеру и правка контейнера на месте
 *
 * @details Возможностей этих у сличаемых реализаций двоичной записи нет вовсе, и
 *          сличать их не с чем: набор этот стережёт их от собственных регрессий
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "abc.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../../include/sys/fmk.hpp"
#include "../../../include/sys/log.hpp"
#include "../../../include/cryptography/crypto.hpp"
#include "../../../include/compressor/block.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::benchmark::binary;

/**
 * @brief Внутренние параметры сценариев контейнера
 *
 */
namespace {
	/**
	 * @brief Количество записей собираемого контейнера
	 *
	 */
	static constexpr size_t RECORD_COUNT = 5000;
	/**
	 * @brief Количество собираемых контейнеров
	 *
	 */
	static constexpr size_t BUILD_ROUNDS = 8;
	/**
	 * @brief Количество выбираемых записей контейнера
	 *
	 */
	static constexpr size_t FETCH_ROUNDS = 5000;
	/**
	 * @brief Количество вносимых правок контейнера
	 *
	 */
	static constexpr size_t EDIT_ROUNDS = 500;

	/**
	 * @brief Порог пропускной способности сборки контейнера без сжатия
	 *
	 */
	static constexpr double BUILD_PLAIN_THRESHOLD = 6.0;
	/**
	 * @brief Порог пропускной способности сборки контейнера со сжатием
	 *
	 */
	static constexpr double BUILD_PACKED_THRESHOLD = 12.0;
	/**
	 * @brief Порог пропускной способности сборки контейнера с шифрованием
	 *
	 */
	static constexpr double BUILD_CIPHERED_THRESHOLD = 12.0;
	/**
	 * @brief Порог сжатия содержимого контейнера
	 *
	 * @details Мерится отношение размера несжатого контейнера к размеру сжатого.
	 *          Показатель этот стережёт не скорость, а то, что сжатие вообще ведётся:
	 *          отношение около единицы означало бы, что кадры уходят на носитель как
	 *          есть - скажем, оттого что модуль сжатия отвалился молча
	 *
	 */
	static constexpr double PACK_RATIO_THRESHOLD = 1.5;
	/**
	 * @brief Порог задержки выборки записи контейнера по номеру в микросекундах
	 *
	 * @details Выборка ведётся оглавлением и читает лишь тот кадр, где запись лежит:
	 *          показатель этот стережёт устройство - рост его до величин, отвечающих
	 *          чтению всего содержимого, означал бы, что оглавление перестало работать
	 *
	 */
	static constexpr double FETCH_LATENCY_THRESHOLD = 1500.0;
	/**
	 * @brief Порог задержки правки контейнера на месте в микросекундах
	 *
	 */
	static constexpr double EDIT_LATENCY_THRESHOLD = 8000.0;

	/**
	 * @brief Класс окружения замеров контейнера
	 *
	 * @details Модули сжатия и шифрования заводятся однократно: заведение их внутри
	 *          измеряемого цикла мерило бы их сборку, а не работу контейнера
	 *
	 */
	class Environment {
		private:
			// Объект фреймворка
			fmk_t _fmk;
		private:
			// Объект журнала
			log_t _log;
		private:
			// Объект сжатия данных
			compressor::block_t _compressor;
		private:
			// Объект шифрования данных
			crypto_t _crypto;
		public:
			/**
			 * @brief Метод извлечения модуля сжатия данных
			 *
			 * @return модуль сжатия данных
			 *
			 */
			const compressor::block_t * compressor() noexcept {
				// Выводим модуль сжатия данных
				return &this->_compressor;
			}
			/**
			 * @brief Метод извлечения модуля шифрования данных
			 *
			 * @return модуль шифрования данных
			 *
			 */
			const crypto_t * crypto() noexcept {
				// Выводим модуль шифрования данных
				return &this->_crypto;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			Environment() noexcept : _log(&this->_fmk), _compressor(&this->_log), _crypto(&this->_fmk, &this->_log) {
				// Выполняем установку соли шифрования
				this->_crypto.salt("соль замеров контейнера");
				// Выполняем установку пароля шифрования
				this->_crypto.password("пароль замеров контейнера");
			}
	};
	/**
	 * @brief Функция получения окружения замеров контейнера
	 *
	 * @return окружение замеров контейнера
	 *
	 */
	static Environment & environment() noexcept {
		// Окружение замеров контейнера
		static Environment result;
		// Выводим окружение замеров контейнера
		return result;
	}
	/**
	 * @brief Функция получения эталонной записи контейнера
	 *
	 * @return эталонная запись контейнера
	 *
	 */
	static const vector <uint8_t> & item() noexcept {
		// Эталонная запись контейнера
		static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
			// Сборка бинарной записи
			awh::codec::abc::writer_t writer;
			// Выполняем укладку эталонной записи контейнера
			if(!(writer.mapBegin(static_cast <uint64_t> (4)) &&
			     writer.text("city") && writer.text("Москва") &&
			     writer.text("id") && writer.number(static_cast <uint64_t> (17)) &&
			     writer.text("name") && writer.text("Товар обыкновенный") &&
			     writer.text("note") && writer.text("описание записи контейнера") && writer.mapEnd()))
				// Выводим пустую запись
				return vector <uint8_t> ();
			// Выводим собранную запись
			return writer.record();
		}();
		// Выводим эталонную запись контейнера
		return result;
	}
	/**
	 * @brief Функция сборки контейнера
	 *
	 * @param packed   признак сжатия содержимого кадров
	 * @param ciphered признак шифрования содержимого кадров
	 * @param result   буфер, куда следует положить собранный контейнер
	 * @return         размер собранного контейнера в октетах
	 *
	 */
	static uint64_t assemble(const bool packed, const bool ciphered, vector <uint8_t> & result) noexcept {
		// Сборщик контейнера
		awh::codec::abc::assembler_t assembler;
		/**
		 * Если содержимое кадров следует сжимать
		 */
		if(packed)
			// Выполняем установку модуля сжатия данных
			assembler.compressor(environment().compressor());
		/**
		 * Если содержимое кадров следует шифровать
		 */
		if(ciphered){
			// Выполняем установку модуля шифрования данных
			assembler.crypto(environment().crypto());
			// Выполняем получение настроек упаковки кадров
			awh::codec::abc::packer_t::settings_t packing = assembler.packer().settings();
			// Выполняем установку признака шифрования содержимого кадров
			packing.encrypt = true;
			// Выполняем установку настроек упаковки кадров
			assembler.packer().settings(packing);
		}
		// Выполняем получение эталонной записи контейнера
		const vector <uint8_t> & record = ::item();
		/**
		 * Выполняем внесение всех записей собираемого контейнера
		 */
		for(size_t i = 0; i < RECORD_COUNT; i++){
			// Если внести очередную запись не удалось
			if(!assembler.append(record.data(), record.size(), awh::codec::abc::payload_t::TEXT))
				// Выводим нулевой размер собранного контейнера
				return 0;
		}
		// Если завершить сборку контейнера не удалось
		if(!assembler.complete(result))
			// Выводим нулевой размер собранного контейнера
			return 0;
		// Выводим размер собранного контейнера
		return static_cast <uint64_t> (result.size());
	}
	/**
	 * @brief Функция получения размера содержимого записей контейнера
	 *
	 * @return размер содержимого записей контейнера в октетах
	 *
	 */
	static size_t payload() noexcept {
		// Выводим размер содержимого записей контейнера
		return (::item().size() * RECORD_COUNT);
	}
	/**
	 * @brief Функция прогона сценария сборки контейнера
	 *
	 * @param packed    признак сжатия содержимого кадров
	 * @param ciphered  признак шифрования содержимого кадров
	 * @return          результат измерения
	 *
	 */
	static awh::benchmark::result_t building(const bool packed, const bool ciphered) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(::payload(), BUILD_ROUNDS, [packed, ciphered]() noexcept {
			// Буфер собираемого контейнера
			vector <uint8_t> container;
			// Выполняем сборку контейнера
			return ::assemble(packed, ciphered, container);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки контейнера без сжатия
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildPlain() noexcept {
		// Выполняем прогон сценария сборки контейнера без сжатия
		return ::building(false, false);
	}
	/**
	 * @brief Функция прогона сценария сборки контейнера со сжатием
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildPacked() noexcept {
		// Выполняем прогон сценария сборки контейнера со сжатием
		return ::building(true, false);
	}
	/**
	 * @brief Функция прогона сценария сборки контейнера с шифрованием
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildCiphered() noexcept {
		// Выполняем прогон сценария сборки контейнера с шифрованием
		return ::building(true, true);
	}
	/**
	 * @brief Функция прогона сценария сжатия содержимого контейнера
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t packRatio() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Буферы несжатого и сжатого контейнеров
		vector <uint8_t> plain, packed;
		// Выполняем сборку несжатого контейнера
		(void) ::assemble(false, false, plain);
		// Выполняем сборку сжатого контейнера
		(void) ::assemble(true, false, packed);
		/**
		 * Если собрать контейнеры не удалось
		 */
		if(plain.empty() || packed.empty()){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "сборка контейнера отвечена отказом";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное отношение размеров контейнеров
		result.value = (static_cast <double> (plain.size()) / static_cast <double> (packed.size()));
		// Устанавливаем сведения о прогоне
		result.details = (to_string(plain.size()) + " окт. против " + to_string(packed.size()) + " окт.");
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария выборки записи контейнера по номеру
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t fetchRecord() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Буфер собранного контейнера
		static vector <uint8_t> container;
		// Выполняем сборку контейнера со сжатием
		(void) ::assemble(true, false, container);
		/**
		 * Если собрать контейнер не удалось
		 */
		if(container.empty()){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "сборка контейнера отвечена отказом";
			// Выводим результат измерения
			return result;
		}
		// Выборщик записей контейнера
		awh::codec::abc::fetcher_t fetcher;
		// Выполняем установку модуля сжатия данных
		fetcher.compressor(environment().compressor());
		/**
		 * Если открыть контейнер выборщиком не удалось
		 */
		if(!fetcher.open([](const uint64_t offset, const size_t size, vector <uint8_t> & data) noexcept -> bool {
			// Если затребованные октеты за концом контейнера
			if((offset + static_cast <uint64_t> (size)) > static_cast <uint64_t> (container.size()))
				// Выводим признак неудачного чтения
				return false;
			// Выполняем выдачу затребованных октетов контейнера
			data.assign(container.begin() + static_cast <ptrdiff_t> (offset),
			 container.begin() + static_cast <ptrdiff_t> (offset) + static_cast <ptrdiff_t> (size));
			// Выводим признак успешного чтения
			return true;
		})){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "открытие контейнера выборщиком отвечено отказом";
			// Выводим результат измерения
			return result;
		}
		// Номер выбираемой записи контейнера
		static uint64_t number = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(::item().size(), FETCH_ROUNDS, [&fetcher]() noexcept {
			// Буфер выбранной записи контейнера
			vector <uint8_t> picked;
			/**
			 * Выполняем выборку записи вразнобой, а не подряд: выборка соседних записей
			 * попадала бы в один и тот же кадр, а кадр выборщиком удерживается
			 */
			number = ((number + 7919) % static_cast <uint64_t> (RECORD_COUNT));
			// Если выбрать запись контейнера не удалось
			if(!fetcher.record(number, picked))
				// Выводим нулевой размер выбранной записи
				return static_cast <uint64_t> (0);
			// Выводим размер выбранной записи
			return static_cast <uint64_t> (picked.size());
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную задержку выборки записи
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария правки контейнера на месте
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t editRecord() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Буфер собранного контейнера
		static vector <uint8_t> container;
		// Выполняем сборку контейнера со сжатием
		(void) ::assemble(true, false, container);
		/**
		 * Если собрать контейнер не удалось
		 */
		if(container.empty()){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "сборка контейнера отвечена отказом";
			// Выводим результат измерения
			return result;
		}
		// Правщик контейнера
		awh::codec::abc::editor_t editor;
		// Выполняем установку модуля сжатия данных
		editor.compressor(environment().compressor());
		/**
		 * Если открыть контейнер правщиком не удалось
		 */
		if(!editor.open([](const uint64_t offset, const size_t size, vector <uint8_t> & data) noexcept -> bool {
			// Если затребованные октеты за концом контейнера
			if((offset + static_cast <uint64_t> (size)) > static_cast <uint64_t> (container.size()))
				// Выводим признак неудачного чтения
				return false;
			// Выполняем выдачу затребованных октетов контейнера
			data.assign(container.begin() + static_cast <ptrdiff_t> (offset),
			 container.begin() + static_cast <ptrdiff_t> (offset) + static_cast <ptrdiff_t> (size));
			// Выводим признак успешного чтения
			return true;
		}, [](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Если запись выходит за конец контейнера, наращиваем его
			if((offset + static_cast <uint64_t> (size)) > static_cast <uint64_t> (container.size()))
				// Выполняем наращивание контейнера
				container.resize(static_cast <size_t> (offset) + size, 0);
			// Выполняем запись поданных октетов контейнера
			::memcpy(container.data() + static_cast <size_t> (offset), buffer, size);
			// Выводим признак успешной записи
			return true;
		}, static_cast <uint64_t> (container.size()))){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "открытие контейнера правщиком отвечено отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем получение эталонной записи контейнера
		const vector <uint8_t> & record = ::item();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(record.size(), EDIT_ROUNDS, [&editor, &record]() noexcept {
			// Если дописать запись в конец контейнера не удалось
			if(!editor.append(record.data(), record.size(), awh::codec::abc::payload_t::TEXT))
				// Выводим нулевое количество записей контейнера
				return static_cast <uint64_t> (0);
			// Если зафиксировать накопленные правки не удалось
			if(!editor.commit())
				// Выводим нулевое количество записей контейнера
				return static_cast <uint64_t> (0);
			// Выводим количество записей контейнера
			return editor.records();
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную задержку правки контейнера
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки контейнера без сжатия
	 */
	static const bool PLAIN_REGISTERED = awh::benchmark::add(
		"codec/abc: сборка контейнера без сжатия", "МБ/с", BUILD_PLAIN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildPlain
	);
	/**
	 * Выполняем регистрацию сценария сборки контейнера со сжатием
	 */
	static const bool PACKED_REGISTERED = awh::benchmark::add(
		"codec/abc: сборка контейнера со сжатием", "МБ/с", BUILD_PACKED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildPacked
	);
	/**
	 * Выполняем регистрацию сценария сборки контейнера с шифрованием
	 */
	static const bool CIPHERED_REGISTERED = awh::benchmark::add(
		"codec/abc: сборка контейнера с шифрованием", "МБ/с", BUILD_CIPHERED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildCiphered
	);
	/**
	 * Выполняем регистрацию сценария сжатия содержимого контейнера
	 */
	static const bool RATIO_REGISTERED = awh::benchmark::add(
		"codec/abc: сжатие содержимого контейнера", "раз", PACK_RATIO_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, packRatio
	);
	/**
	 * Выполняем регистрацию сценария выборки записи контейнера по номеру
	 */
	static const bool FETCH_REGISTERED = awh::benchmark::add(
		"codec/abc: выборка записи по номеру", "мкс/зап.", FETCH_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, fetchRecord
	);
	/**
	 * Выполняем регистрацию сценария правки контейнера на месте
	 */
	static const bool EDIT_REGISTERED = awh::benchmark::add(
		"codec/abc: правка контейнера на месте", "мкс/правку", EDIT_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, editRecord
	);
};
