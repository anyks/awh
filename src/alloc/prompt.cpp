/**
 * @file prompt.cpp
 * @date 2026-09-15
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
 * @brief Файл приёма пароля с терминала без эха
 *
 * \~english
 * @brief Password intake from the terminal with the echo off
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/prompt.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cerrno>
#include <cstring>

/**
 * Если операционной системой является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	#include <fcntl.h>
	#include <termios.h>
	#if !defined(_MSC_VER)
		#include <unistd.h>
	#endif
#endif

/**
 * @brief Безымянное пространство имён
 *
 */
namespace {
	/**
	 * Знаки управления набором
	 */
	// Знак возврата на один шаг
	static constexpr uint8_t BACKSPACE = 0x08;
	/**
	 * Знак удаления предыдущего
	 *
	 * Имя взято НЕ `DELETE`: тот занят макросом у `<winnt.h>` MS Windows
	 * (`#define DELETE 0x00010000`), и объявление с таким именем не собирается вовсе -
	 * препроцессор подставляет на его место число. `RUBOUT` - историческое имя того же
	 * знака 0x7F, и макросом оно не занято нигде
	 */
	static constexpr uint8_t RUBOUT = 0x7F;
	/**
	 * Наибольшее число знаков, у каких помнится ширина
	 *
	 * Правка опечатки обязана снимать ЗНАК, а не байт: у кодировки UTF-8 буква занимает
	 * до четырёх байтов, и снятие байта оставило бы половину буквы - содержимое стало бы
	 * негодной последовательностью. Ширины помним затем, что прочесть содержимое
	 * приёмника нельзя: он выдаёт его лишь обработчику, и то намеренно.
	 *
	 * Набравшему больше этого числа знаков правка последнего недоступна - приём же идёт
	 * по-прежнему. Ширины сами по себе содержимого не выдают: длина набора видна и так
	 */
	static constexpr size_t WIDTHS = 256;
	/**
	 * @brief Метод определения байта продолжения кодировки UTF-8
	 *
	 * @param byte разбираемый байт
	 * @return     признак байта продолжения
	 *
	 */
	static bool trailing(const uint8_t byte) noexcept {
		// Выводим признак байта продолжения: старшие разряды его суть 10
		return ((byte & 0xC0u) == 0x80u);
	}
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		/**
		 * @brief Класс снятия эха у терминала
		 *
		 * @note Настройки возвращаются ДЕСТРУКТОРОМ: уход из приёма бывает и ранним, и
		 *       по исключению, а терминал, оставленный без эха, делает непригодной всю
		 *       сессию человека, а не только наше приложение
		 *
		 */
		class silence_t {
			private:
				// Описатель терминала
				int _tty;
				// Прежние настройки терминала
				struct termios _saved;
				// Признак снятого эха
				bool _muted;
			public:
				/**
				 * @brief Метод получения описателя терминала
				 *
				 * @return описатель терминала
				 *
				 */
				int tty() const noexcept {
					// Выводим описатель терминала
					return this->_tty;
				}
				/**
				 * @brief Метод проверки снятого эха
				 *
				 * @return признак снятого эха
				 *
				 */
				bool muted() const noexcept {
					// Выводим признак снятого эха
					return this->_muted;
				}
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				silence_t() noexcept : _tty(-1), _saved(), _muted(false) {
					/**
					 * Открываем ТЕРМИНАЛ, а не поток ввода
					 *
					 * Поток ввода вправе подменить кто угодно - конвейер, перенаправление
					 * из файла, чужой процесс, - и пароль пришёл бы не от человека, да ещё
					 * с включённым эхом
					 */
					this->_tty = ::open("/dev/tty", O_RDWR | O_NOCTTY);
					// Если терминал не открыт
					if(this->_tty < 0)
						// Снимать эхо нечему
						return;
					// Читаем прежние настройки терминала
					if(::tcgetattr(this->_tty, &this->_saved) != 0)
						// Снимать эхо нечем
						return;
					// Заводим новые настройки терминала
					struct termios muted = this->_saved;
					// Снимаем эхо набираемого
					muted.c_lflag = static_cast <tcflag_t> (muted.c_lflag & ~static_cast <tcflag_t> (ECHO));
					/**
					 * Строчный разбор оставляем системе, а знаки набора берём сами
					 *
					 * Снятие `ICANON` дало бы нам знаки поштучно, но вместе с ним ушла бы
					 * и обработка прерывания: нажатие отмены перестало бы убивать
					 * приложение, и человек лишился бы обычного выхода. Оттого канонический
					 * разбор остаётся, а строка приходит целиком
					 */
					// Применяем новые настройки терминала, дождавшись опустошения очереди
					if(::tcsetattr(this->_tty, TCSAFLUSH, &muted) != 0)
						// Эхо снять не удалось
						return;
					// Отмечаем эхо снятым
					this->_muted = true;
				}
				/**
				 * @brief Деструктор
				 *
				 */
				~silence_t() noexcept {
					// Если эхо было снято
					if(this->_muted)
						// Возвращаем прежние настройки терминала
						static_cast <void> (::tcsetattr(this->_tty, TCSAFLUSH, &this->_saved));
					// Если терминал был открыт
					if(this->_tty >= 0)
						// Закрываем терминал
						static_cast <void> (::close(this->_tty));
				}
			public:
				/**
				 * Копированию снятие эха не подлежит
				 *
				 * Копия вернула бы настройки дважды, а второй возврат пришёлся бы уже на
				 * закрытый описатель
				 */
				silence_t(const silence_t &) = delete;
				silence_t & operator = (const silence_t &) = delete;
		};
	#endif
};

/**
 * @brief Метод проверки наличия терминала
 *
 * @return признак доступного терминала
 *
 */
bool awh::alloc::Prompt::available() noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Получаем описатель ввода консоли
		::HANDLE console = ::GetStdHandle(STD_INPUT_HANDLE);
		// Если описатель ввода консоли не получен
		if((console == nullptr) || (console == INVALID_HANDLE_VALUE))
			// Терминала нет
			return false;
		// Прежний режим консоли
		::DWORD mode = 0;
		/**
		 * Спрашиваем режим консоли, а не вид описателя
		 *
		 * У перенаправленного ввода описатель есть, а режима у него нет: отказ здесь и
		 * означает «ввод не консольный»
		 */
		return (::GetConsoleMode(console, &mode) != FALSE);
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Открываем терминал
		const int tty = ::open("/dev/tty", O_RDWR | O_NOCTTY);
		// Если терминал не открыт
		if(tty < 0)
			// Терминала нет
			return false;
		// Закрываем терминал
		static_cast <void> (::close(tty));
		// Терминал на месте
		return true;
	#endif
}
/**
 * @brief Метод приёма пароля с терминала без эха
 *
 * @param vessel приёмник, куда ложится пароль
 * @param hint   подсказка, печатаемая человеку, либо nullptr
 * @return       исход приёма пароля
 *
 */
awh::alloc::Prompt::result_t awh::alloc::Prompt::read(vessel_t & vessel, const char * hint) noexcept {
	// Если приёмник не заведён
	if(!vessel.exists())
		// Принимать некуда
		return result_t::FAILED;
	/**
	 * Прежнее содержимое приёмника снимаем
	 *
	 * Приём идёт с начала, а недобранный прежде пароль лежал бы хвостом за набранным
	 * теперь - и ушёл бы вместе с ним туда, где его не ждут
	 */
	while(vessel.size() > 0)
		// Снимаем последний принятый байт
		static_cast <void> (vessel.drop());
	/**
	 * Для операционной системы MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Получаем описатель ввода консоли
		::HANDLE console = ::GetStdHandle(STD_INPUT_HANDLE);
		// Если описатель ввода консоли не получен
		if((console == nullptr) || (console == INVALID_HANDLE_VALUE))
			// Читать неоткуда
			return result_t::NOTERM;
		// Прежний режим консоли
		::DWORD saved = 0;
		// Если режим консоли не получен
		if(::GetConsoleMode(console, &saved) == FALSE)
			// Ввод не консольный: читать неоткуда
			return result_t::NOTERM;
		// Если подсказка задана
		if(hint != nullptr){
			// Печатаем подсказку человеку
			::fputs(hint, stderr);
			// Отдаём подсказку немедля
			static_cast <void> (::fflush(stderr));
		}
		/**
		 * Снимаем эхо, оставив строчный разбор
		 *
		 * Строчный разбор оставляем затем, что вместе с ним ушла бы и обработка
		 * прерывания: нажатие отмены перестало бы убивать приложение
		 */
		if(::SetConsoleMode(console, (saved & ~static_cast <::DWORD> (ENABLE_ECHO_INPUT))) == FALSE)
			// Эхо снять не удалось
			return result_t::FAILED;
		// Исход приёма пароля
		result_t result = result_t::ACCEPTED;
		// Ширины принятых знаков в байтах
		uint8_t widths[WIDTHS] = {0};
		// Число принятых знаков
		size_t taken_symbols = 0;
		/**
		 * Выполняем приём знаков набора
		 */
		while(true){
			// Принимаемый знак
			char symbol = 0;
			// Число прочитанных знаков
			::DWORD taken = 0;
			// Читаем очередной знак набора
			if((::ReadConsoleA(console, &symbol, 1, &taken, nullptr) == FALSE) || (taken == 0)){
				// Набор кончился
				break;
			}
			// Если набор завершён переводом строки
			if((symbol == '\n') || (symbol == '\r'))
				// Набор кончился
				break;
			// Если знак снимает предыдущий
			if((static_cast <uint8_t> (symbol) == BACKSPACE) || (static_cast <uint8_t> (symbol) == RUBOUT)){
				// Если снимать нечего
				if(taken_symbols == 0)
					// Переходим к следующему знаку
					continue;
				// Получаем ширину последнего принятого знака
				uint8_t width = widths[(taken_symbols - 1u) % WIDTHS];
				/**
				 * Ширину неизвестного знака числим одним байтом
				 *
				 * Так бывает у набора длиннее помнимого: снимется байт, а не знак, - и
				 * это лучше, чем не снять ничего вовсе
				 */
				if(width == 0)
					// Числим знак одним байтом
					width = 1;
				/**
				 * Выполняем снятие всех байтов знака
				 */
				for(uint8_t i = 0; i < width; i++)
					// Снимаем очередной байт знака
					static_cast <void> (vessel.drop());
				// Уменьшаем число принятых знаков
				taken_symbols--;
				// Переходим к следующему знаку
				continue;
			}
			// Принимаем очередной знак набора
			if(!vessel.pour(static_cast <uint8_t> (symbol))){
				// Запоминаем превышение ёмкости приёмника
				result = result_t::TOOLONG;
				// Набор кончился
				break;
			}
			// Если принятый байт продолжает знак
			if(::trailing(static_cast <uint8_t> (symbol))){
				// Если знак, какой он продолжает, помнится
				if((taken_symbols > 0) && (((taken_symbols - 1u) % WIDTHS) < WIDTHS))
					// Расширяем последний принятый знак
					widths[(taken_symbols - 1u) % WIDTHS]++;
			// Если принятый байт начинает знак
			} else {
				// Запоминаем ширину нового знака
				widths[taken_symbols % WIDTHS] = 1;
				// Увеличиваем число принятых знаков
				taken_symbols++;
			}
		}
		/**
		 * Затираем ширины знаков
		 *
		 * Содержимого они не несут, но состав набора обрисовывают: сколько знаков и
		 * какой ширины. Оставлять их на стеке незачем
		 */
		for(size_t i = 0; i < WIDTHS; i++)
			// Затираем ширину очередного знака
			widths[i] = 0;
		// Возвращаем прежний режим консоли
		static_cast <void> (::SetConsoleMode(console, saved));
		// Переводим строку за человека: эха у набора не было
		::fputs("\n", stderr);
		// Выводим исход приёма пароля
		return result;
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Снимаем эхо у терминала
		silence_t silence;
		// Если терминал не открыт
		if(silence.tty() < 0)
			// Читать неоткуда
			return result_t::NOTERM;
		// Если эхо снять не удалось
		if(!silence.muted())
			/**
			 * Читать с включённым эхом НЕ БУДЕМ
			 *
			 * Пароль лёг бы на экран, а человек об этом не узнал бы вовсе: подсказку он
			 * увидел ту же самую. Отказ здесь честнее показанного пароля
			 */
			return result_t::FAILED;
		// Если подсказка задана
		if(hint != nullptr){
			// Печатаем подсказку человеку
			static_cast <void> (::write(silence.tty(), hint, ::strlen(hint)));
		}
		// Исход приёма пароля
		result_t result = result_t::ACCEPTED;
		// Ширины принятых знаков в байтах
		uint8_t widths[WIDTHS] = {0};
		// Число принятых знаков
		size_t symbols = 0;
		/**
		 * Выполняем приём знаков набора
		 */
		while(true){
			// Принимаемый знак
			uint8_t symbol = 0;
			// Читаем очередной знак набора
			const ssize_t taken = ::read(silence.tty(), &symbol, 1);
			// Если чтение прервано сигналом
			if((taken < 0) && (errno == EINTR))
				// Продолжаем приём
				continue;
			// Если набор кончился
			if(taken <= 0)
				// Набор кончился
				break;
			// Если набор завершён переводом строки
			if((symbol == '\n') || (symbol == '\r'))
				// Набор кончился
				break;
			// Если знак снимает предыдущий
			if((symbol == BACKSPACE) || (symbol == RUBOUT)){
				// Если снимать нечего
				if(symbols == 0)
					// Переходим к следующему знаку
					continue;
				// Получаем ширину последнего принятого знака
				uint8_t width = widths[(symbols - 1u) % WIDTHS];
				/**
				 * Ширину неизвестного знака числим одним байтом
				 *
				 * Так бывает у набора длиннее помнимого: снимется байт, а не знак, - и
				 * это лучше, чем не снять ничего вовсе
				 */
				if(width == 0)
					// Числим знак одним байтом
					width = 1;
				/**
				 * Выполняем снятие всех байтов знака
				 */
				for(uint8_t i = 0; i < width; i++)
					// Снимаем очередной байт знака
					static_cast <void> (vessel.drop());
				// Уменьшаем число принятых знаков
				symbols--;
				// Переходим к следующему знаку
				continue;
			}
			// Принимаем очередной знак набора
			if(!vessel.pour(symbol)){
				// Запоминаем превышение ёмкости приёмника
				result = result_t::TOOLONG;
				// Набор кончился
				break;
			}
			// Если принятый байт продолжает знак
			if(::trailing(symbol)){
				// Если знак, какой он продолжает, помнится
				if(symbols > 0)
					// Расширяем последний принятый знак
					widths[(symbols - 1u) % WIDTHS]++;
			// Если принятый байт начинает знак
			} else {
				// Запоминаем ширину нового знака
				widths[symbols % WIDTHS] = 1;
				// Увеличиваем число принятых знаков
				symbols++;
			}
		}
		/**
		 * Затираем ширины знаков
		 *
		 * Содержимого они не несут, но состав набора обрисовывают: сколько знаков и
		 * какой ширины. Оставлять их на стеке незачем
		 */
		for(size_t i = 0; i < WIDTHS; i++)
			// Затираем ширину очередного знака
			widths[i] = 0;
		// Переводим строку за человека: эха у набора не было
		static_cast <void> (::write(silence.tty(), "\n", 1));
		// Выводим исход приёма пароля
		return result;
	#endif
}
