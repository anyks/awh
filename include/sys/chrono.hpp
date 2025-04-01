/**
 * @file: core.hpp
 * @date: 2025-03-12
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_CHRONO__
#define __AWH_CHRONO__

/**
 * Подключаем зависимые заголовки
 */
#include <map>
#include <mutex>
#include <cmath>
#include <ctime>
#include <chrono>
#include <string>
#include <vector>
#include <cstdarg>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <pcre2/pcre2posix.h>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Chrono Структура модуля Chrono
	 */
	typedef class AWHSHARED_EXPORT Chrono {
		private:
			/**
			 * Формат парсинга даты
			 */
			enum class format_t : uint8_t {
				NONE = 0x00, // Формат не определён
				y    = 0x01, // Формат соответствует %y и %g (YY)
				Y    = 0x02, // Формат соответствует %Y и %G (YYYY)
				b    = 0x03, // Формат соответствует %b и %h (MMM)
				B    = 0x04, // Формат соответствует %B (MMMMM)
				m    = 0x05, // Формат соответствует %m (MM)
				d    = 0x06, // Формат соответствует %d (dd)
				a    = 0x07, // Формат соответствует %a (EEE)
				A    = 0x08, // Формат соответствует %A (EEEEE)
				j    = 0x09, // Формат соответствует %j
				u    = 0x0A, // Формат соответствует %u (u)
				w    = 0x0B, // Формат соответствует %w
				W    = 0x0C, // Формат соответствует %W
				D    = 0x0D, // Формат соответствует %D и %x (MM/dd/YYYY)
				F    = 0x0E, // Формат соответствует %F (YYYY-MM-dd)
				H    = 0x0F, // Формат соответствует %H (HH)
				I    = 0x10, // Формат соответствует %I (h)
				M    = 0x11, // Формат соответствует %M (mm)
				s    = 0x12, // Формат соответствует (SSS)
				S    = 0x13, // Формат соответствует %S (ss)
				p    = 0x14, // Формат соответствует %p (a)
				R    = 0x15, // Формат соответствует %R (HH:mm)
				T    = 0x16, // Формат соответствует %T и %X (HH:mm:ss)
				r    = 0x17, // Формат соответствует %r (h:mm:ss a)
				c    = 0x18, // Формат соответствует %c (EEE MMM dd HH:mm:ss YYYY)
				z    = 0x19, // Формат соответствует %z (Z)
				Z    = 0x1A, // Формат соответствует %Z (z)
				e    = 0x1B  // Формат соответствует %e (Zz)
			};
		public:
			/**
			 * 12-и часовой формат времени
			 */
			enum class h12_t : uint8_t {
				AM = 0x00, // До полудня
				PM = 0x01  // После полудня
			};
			/**
			 * Параметры смещения
			 */
			enum class offset_t : uint8_t {
				INCREMENT = 0x00, // Инкремент
				DECREMENT = 0x01  // Декремент
			};
			/**
			 * Параметры актуального состояния даты
			 */
			enum class actual_t : uint8_t {
				LEFT   = 0x00, // Сколько осталось времени
				PASSED = 0x01  // Сколько прошло времени
			};
			/**
			 * Тип хранимой даты
			 */
			enum class storage_t : uint8_t {
				NONE   = 0x00, // Хранение даты не установлено
				LOCAL  = 0x01, // Дата в формате локальных настроек
				GLOBAL = 0x02  // Дата в формате глобального времени
			};
			/**
			 * Тип штампа времени
			 */
			enum class type_t : uint8_t {
				NONE         = 0x00, // Не установлено
				YEAR         = 0x01, // Год
				MONTH        = 0x02, // Месяц
				WEEK         = 0x03, // Неделя
				DAY          = 0x04, // День
				HOUR         = 0x05, // Час
				MINUTES      = 0x06, // Минуты
				SECONDS      = 0x07, // Секунды
				MILLISECONDS = 0x08, // Миллисекунды
				MICROSECONDS = 0x09, // Микросекунды
				NANOSECONDS  = 0x0A  // Наносекунды
			};
			/**
			 * Тип элементов даты
			 */
			enum class unit_t : uint8_t {
				NONE         = 0x00, // Элемент даты не установлен
				DAY          = 0x01, // Номер текущего дня недели от 1 до 7
				DATE         = 0x02, // Число месяца от 1 до 31
				YEAR         = 0x03, // Полное обозначение года
				HOUR         = 0x04, // Количество часов от 0 до 23
				DAYS         = 0x05, // Количество прошедвших дней от 1 января
				MONTH        = 0x06, // Номер месяца от 1 до 12 (начиная с Января)
				WEEKS        = 0x07, // Количество недель прошедших с начала года
				OFFSET       = 0x08, // Смещение временной зоны в секундах относительно UTC
				MINUTES      = 0x09, // Количество минут от 0 до 59
				SECONDS      = 0x0A, // Количество секунд от 0 до 59
				NANOSECONDS  = 0x0B, // Количество наносекунд
				MICROSECONDS = 0x0C, // Количество микросекунд
				MILLISECONDS = 0x0D  // Количество миллисекунд
			};
			/**
			 * Временная зона
			 * @site https://24timezones.com/mirovoe_vremia3.php
			 */
			enum class zone_t : uint8_t {
				NONE   = 0x00, // Временная зона не установлена
				AT     = 0x01, // Атлантическое Время
				CT     = 0x02, // Северноамериканское Центральное Время
				ET     = 0x03, // Северноамериканское Восточное Время
				MT     = 0x04, // Северноамериканское Горное Время
				NT     = 0x05, // Время В Ньюфаундленде
				PT     = 0x06, // Северноамериканское Тихоокеанское Время
				MUT    = 0x07, // Стандартное Время На Острове Маврикий
				MVT    = 0x08, // Время На Мальдивах
				MYT    = 0x09, // Малайское Время
				NCT    = 0x0A, // Стандартное Время В Новой Каледонии
				NDT    = 0x0B, // Летнее Время В Ньюфаундленде
				NFT    = 0x0C, // Время На Острове Норфолк
				NPT    = 0x0D, // Непальськое Время
				NRT    = 0x0E, // Время На Острове Науру
				NST    = 0x0F, // Стандартное Время В Ньюфаундленде
				PWT    = 0x10, // Время На Острове Палау
				NUT    = 0x11, // Время На Острове Ниуэ
				FET    = 0x12, // Минское Время
				FJT    = 0x13, // Летнее Время На О. Фиджи
				PYT    = 0x14, // Парагвайское Стандартное Время
				RET    = 0x15, // Время На Острове Реюньон
				SBT    = 0x16, // Время На Соломоновых Островах
				SCT    = 0x17, // Время На Сейшелах
				SGT    = 0x18, // Сингапурское Время
				SRT    = 0x19, // Время В Суринаме
				SST    = 0x1A, // Стандартное Время На Острове Самоа
				TFT    = 0x1B, // Французское Южное И Антарктическое Время
				THA    = 0x1C, // Тайландское Время
				TJT    = 0x1D, // Время В Таджикистане
				TKT    = 0x1E, // Время На Островах Токелау
				TLT    = 0x1F, // Время В Восточном Тиморе
				TMT    = 0x20, // Стандартное Время В Туркмении
				TOT    = 0x21, // Время На Островах Тонга
				TRT    = 0x22, // Турецкое Время
				TVT    = 0x23, // Время На Островах Тувалу
				FKT    = 0x24, // Стандартное Время На Фолклендах
				FNT    = 0x25, // Стандартное Время На Фернанду-Ди-Норонья
				AFT    = 0x26, // Время В Афганистане
				ACT    = 0x27, // Амазонское Стандартное Время
				ADT    = 0x28, // Атлантическое Летнее Время
				AZT    = 0x29, // Азербайджанское Стандартное Время
				ART    = 0x2A, // Аргентинское Стандартное Время
				BDT    = 0x2B, // Время В Бруней-Даруссаламе
				BNT    = 0x2C, // Время В Бруней-Даруссаламе
				BOT    = 0x2D, // Боливийское Время
				BRT    = 0x2E, // Бразильское Стандартное Время
				BTT    = 0x2F, // Бутанское Время
				CAT    = 0x30, // Восточноафриканское Время
				CVT    = 0x31, // Стандартное Время На Островах Кабо-Верде
				CXT    = 0x33, // Время На Острове Рождества
				CCT    = 0x34, // Время На Кокосовые Островах
				CET    = 0x35, // Центральноевропейское Стандартное Время
				CIT    = 0x36, // Время В Центральной Индонезии
				CKT    = 0x37, // Стандартное Время На Островах Кука
				CLT    = 0x38, // Чилийское Стандартное Время
				COT    = 0x39, // Колумбийское Стандартное Время
				EAT    = 0x3A, // Восточноафриканский Час
				ECT    = 0x3B, // Эквадорское Время
				EDT    = 0x3C, // Северноамериканское Восточное Летнее Время
				EET    = 0x3D, // Восточноевропейское Стандартное Время
				EGT    = 0x3E, // Стандартное Время В Восточной Гренландии
				EIT    = 0x3F, // Время В Восточной Индонезии
				EST    = 0x40, // Северноамериканское Восточное Стандартное Время
				GET    = 0x41, // Грузинское Стандартное Время
				ICT    = 0x42, // Время В Индокитае
				IDT    = 0x43, // Израильское Летнее Время
				GFT    = 0x44, // Время В Французской Гвиане
				GIT    = 0x45, // Время На О. Гамбье
				GMT    = 0x46, // Среднее Время По Гринвичу
				GYT    = 0x47, // Время В Гайане
				HKT    = 0x48, // Гонконгское Стандартное Время
				JST    = 0x49, // Японское Стандартное Время
				KGT    = 0x4A, // Время В Киргизии
				KST    = 0x4B, // Корейское Стандартное Время
				MDT    = 0x4C, // Северноамериканское Горное Летнее Время
				MHT    = 0x4D, // Время На Маршалловых Островах
				MIT    = 0x4E, // Время На Маркизских Островах
				MMT    = 0x4F, // Время В Мьянме
				MSK    = 0x50, // Московское Время
				MSD    = 0x51, // Московское летнее время
				PST    = 0x52, // Северноамериканское Тихоокеанское Стандартное Время
				PDT    = 0x53, // Северноамериканское Тихоокеанское Летнее Время
				PET    = 0x54, // Стандартное Время В Перу
				PGT    = 0x55, // Время В Папуа-Новой Гвинее
				UTC    = 0x56, // Всемирное Координированное Время
				PHT    = 0x57, // Стандартное Время На Филлипинах
				PKT    = 0x58, // Пакистанское Стандартное Время
				UYT    = 0x59, // Стандартное Время В Уругвае
				UZT    = 0x5A, // Время В Узбекистане
				VET    = 0x5B, // Время В Венесуеле
				VUT    = 0x5C, // Стандартное Время На Островах Вануату
				WAT    = 0x5D, // Западноафриканское Стандартное Время
				WET    = 0x5E, // Западноевропейский Стандартний Час
				WFT    = 0x5F, // Время На Островах Уоллис И Футуна
				WIB    = 0x60, // Время В Западной Индонезии
				WIT    = 0x61, // Время В Восточной Индонезии
				ACDT   = 0x62, // Летнее Время В Центральной Австралии
				ACST   = 0x63, // Стандартное Время В Центральной Австралии
				AEDT   = 0x64, // Летнее Время В Восточной Австралии
				AEST   = 0x65, // Стандартное Время В Восточной Австралии
				AKDT   = 0x66, // Летнее Время На Аляске
				AKST   = 0x67, // Стандартное Время На Аляске
				AMST   = 0x68, // Амазонка, Стандартное Время
				AWST   = 0x69, // Стандартное Время В Западной Австралии
				AZOT   = 0x6A, // Стандартное Время На Азорских Островах
				BRST   = 0x6B, // Бразильское Летнее Время
				CLST   = 0x6C, // Чилийское Летнее Время
				CEST   = 0x6D, // Центральноевропейское Летнее Время
				CHOT   = 0x6E, // Стандартное Время В Чойлобалсане
				CHST   = 0x6F, // Час Чаморро
				CHUT   = 0x70, // Время На Островах Чуук
				COST   = 0x71, // Колумбийское Летнее Время
				DAVT   = 0x72, // Дейвис
				DDUT   = 0x73, // Дюмон-Д'юрвиль
				EGST   = 0x74, // Летнее Время В Восточной Гренландии
				EAST   = 0x75, // Стандартное Время На Острове Пасхи
				EEST   = 0x76, // Восточноевропейское Летнее Время
				FKST   = 0x77, // Летнее Время На Фолклендах
				GAMT   = 0x78, // Время На Острове Гамбье
				HOVT   = 0x79, // Стандартное Время В Ховде
				HADT   = 0x7A, // Гавайско-Алеутское Летнее Время
				HAST   = 0x7B, // Гавайско-Алеутское Стандартное Время
				IRDT   = 0x7C, // Иранское Летнее Время
				IRKT   = 0x7D, // Иркутское Стандартное Время
				IRST   = 0x7E, // Иранское Стандартное Время
				GILT   = 0x7F, // Время На Островах Гилберта
				GALT   = 0x80, // Время На Галапагосских Островах
				KOST   = 0x81, // Время На Острове Косраэ
				KRAT   = 0x82, // Красноярское Стандартное Время
				LHDT   = 0x83, // Летнее Время На Лорд-Хау
				LHST   = 0x84, // Стандартное Время На Лорд-Хау
				LINT   = 0x85, // Время На Острове Лайн
				MAGT   = 0x86, // Магаданское Стандартное Время
				MART   = 0x87, // Время На Маркизских Островах
				MIST   = 0x88, // Время На Станции Маккуори
				MAWT   = 0x89, // Время На Станции Моусон
				NZDT   = 0x8A, // Летнее Время В Новой Зеландии
				NZST   = 0x8B, // Стандартное Время В Новой Зеландии
				PYST   = 0x8C, // Парагвайское Летнее Время
				PETT   = 0x8D, // Камчатское Время
				PMDT   = 0x8E, // Летнее Время На Островах Сен-Пьер И Микелон
				PMST   = 0x8F, // Стандартное Время На Островах Сен-Пьер И Микелон
				PONT   = 0x90, // Время На Острове Понапе
				PHOT   = 0x91, // Время На Островах Феникс
				PhST   = 0x92, // Стандартное Время На Филлипинах
				ROTT   = 0x93, // Время На Станции Ротера
				SLST   = 0x94, // Стандартное Время В Шри-Ланке
				SAKT   = 0x95, // Сахалинское Стандартное Время
				SAMT   = 0x96, // Самарское Время
				SAST   = 0x97, // Южноафриканское Время
				SYOT   = 0x98, // Время На Станции Сёва
				TAHT   = 0x99, // Время На Острове Таити
				OMST   = 0x9A, // Омское Время
				ORAT   = 0x9B, // Время В Западном Казахстане
				VLAT   = 0x9C, // Владивостокское Время
				VOLT   = 0x9D, // Волгоградское Время
				VOST   = 0x9E, // Время На Станции Восток
				UYST   = 0x9F, // Летнее Время В Уругвае
				ULAT   = 0xA0, // Стандартное Время В Монголии
				USZ1   = 0xA1, // Калиниградское Время
				WAKT   = 0xA2, // Время На Острове Уэйк
				WAST   = 0xA3, // Западноафриканское Летнее Время
				WEST   = 0xA4, // Западноевропейское Летнее Время
				WGST   = 0xA5, // Стандартное Время В Западной Гренландии
				YAKT   = 0xA6, // Якутское Время
				YEKT   = 0xA7, // Екатеринбургское Время
				ACWST  = 0xA8, // Центрально-Западная Австралия, Стандартное Время
				AZOST  = 0xA9, // Летнее Время На Азорских Островах
				CHADT  = 0xAA, // Летнее Время На Архипелаге Чатем
				CHAST  = 0xAB, // Стандартное Время На Архипелаге Чатем
				CHOST  = 0xAC, // Летнее Время В Чойлобалсане
				EASST  = 0xAD, // Летнее Время На Острове Пасхи
				HOVST  = 0xAE, // Летнее Время В Ховде
				ULAST  = 0xAF, // Летнее Время В Монголии
				AMTAM  = 0xB0, // Амазонское Стандартное Время
				AMTAR  = 0xB1, // Армянское Стандартное Время
				ASTAL  = 0xB2, // Атлантическое Стандартное Время
				ASTSA  = 0xB3, // Стандартное Время В Саудовской Аравии
				BSTBR  = 0xB4, // Британское Летнее Время
				BSTBL  = 0xB5, // Стандартное Время В Бангладеш
				CDTNA  = 0xB6, // Северноамериканское Центральное Летнее Время
				CDTCB  = 0xB7, // Кубинское Летнее Время
				CSTNA  = 0xB8, // Северноамериканское Центральное Стандартное Время
				CSTKT  = 0xB9, // Китайское Стандартное Время
				CSTCB  = 0xBA, // Кубинское Стандартное Время
				GSTPG  = 0xBB, // Время В Персидском Заливе
				GSTSG  = 0xBC, // Время В Южной Георгии
				ISTID  = 0xBD, // Индийское Стандартное Время
				ISTIR  = 0xBE, // Ирландия, Летнее Время
				ISTIS  = 0xBF, // Израильское Стандартное Время
				MSTNA  = 0xC0, // Северноамериканское Горное Стандартное Время
				MSTMS  = 0xC1, // Время В Малайзии
				WGSTST = 0xC2  // Летнее Время В Западной Гренландии
			};
		private:
			/**
			 * Mutex структура рабочих мютексов
			 */
			typedef struct Mutex {
				mutex tz;              // Мютекс контроля добавления временной зоны
				mutex date;            // Мютекс контроля локального объекта даты
				recursive_mutex parse; // Мютекс контроля парсинга
			} mtx_t;
			/**
			 * DateTime Структура параметров даты и времени
			 */
			typedef struct DateTime {
				bool dst;              // Флаг летнего времени
				bool leap;             // Флаг високосного года
				h12_t h12;             // Статус 12-и часового формата времени
				zone_t zone;           // Идентификатор временной зоны
				uint8_t day;           // Номер текущего дня недели от 1 до 7
				uint8_t date;          // Число месяца от 1 до 31
				uint8_t hour;          // Количество часов от 0 до 23
				uint8_t month;         // Номер месяца от 1 до 12 (начиная с Января)
				uint8_t weeks;         // Количество недель прошедших с начала года
				uint8_t seconds;       // Количество секунд от 0 до 59
				uint8_t minutes;       // Количество минут от 0 до 59
				uint16_t year;         // Полное обозначение года
				uint16_t days;         // Количество прошедвших дней от 1 января
				int32_t offset;        // Смещение временной зоны в секундах относительно UTC
				uint32_t milliseconds; // Количество миллисекунд
				uint64_t nanoseconds;  // Количество наносекунд
				uint64_t microseconds; // Количество микросекунд
				/**
				 * DateTime Конструктор
				 */
				DateTime() noexcept :
				 dst(false), leap(false),
				 h12(h12_t::AM), zone(zone_t::NONE),
				 day(2), date(1), hour(0), month(1),
				 weeks(0), seconds(0), minutes(0),
				 year(1970), days(0), offset(0),
				 milliseconds(0), nanoseconds(0), microseconds(0) {}
			} __attribute__((packed)) dt_t;
		private:
			// Объект локального времени
			dt_t _dt;
		private:
			// Мютекс для блокировки потока
			mutable mtx_t _mtx;
		private:
			// Список скомпилированных регулярных выражений
			map <format_t, regex_t> _expressions;
			// Список внутренних временных зон
			unordered_map <string, int32_t> _timeZones;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
		public:
			/**
			 * clear Метод очистку всех локальных данных
			 */
			void clear() noexcept;
		private:
			/**
			 * makeDate Метод получения штампа времени из объекта даты
			 * @param dt объект даты из которой необходимо получить штамп времени
			 * @return   штамп времени в миллисекундах
			 */
			uint64_t makeDate(const dt_t & dt) const noexcept;
			/**
			 * makeDate Метод заполнения объекта даты из штампа времени
			 * @param date дата из которой необходимо заполнить объект
			 * @param dt   объект даты который необходимо заполнить
			 */
			void makeDate(const uint64_t date, dt_t & dt) const noexcept;
		private:
			/**
			 * compile Метод компиляции регулярных выражений
			 * @param expression регулярное выражение для компиляции
			 * @param format     формат к которому относится регулярное выражение
			 */
			void compile(const string & expression, const format_t format) noexcept;
		private:
			/**
			 * prepare Функция заполнения объекта даты и времени
			 * @param dt     объект даты и времени для заполнения
			 * @param text   текст в котором производится поиск
			 * @param format формат выполнения поиска
			 * @param pos    начальная позиция в тексте
			 * @return       конечная позиция обработанных данных в тексте
			 */
			ssize_t prepare(dt_t & dt, const string & text, const format_t format, const size_t pos = 0) const noexcept;
		public:
			/**
			 * abbreviation Метод перевода времени в аббревиатуру
			 * @param date дата в UnixTimestamp
			 * @return     сформированная аббревиатура даты
			 */
			pair <type_t, double> abbreviation(const uint64_t date) const noexcept;
		public:
			/**
			 * end Метод получения конца позиции указанной даты
			 * @param date дата для которой необходимо получить позицию
			 * @param type тип единиц измерений даты
			 * @return     конец указанной даты в формате UnixTimestamp
			 */
			uint64_t end(const uint64_t date, const type_t type) const noexcept;
			/**
			 * end Метод получения конца позиции текущей даты
			 * @param type    тип единиц измерений даты
			 * @param storage хранение значение времени
			 * @return        конец текущей даты в формате UnixTimestamp
			 */
			uint64_t end(const type_t type, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * begin Метод получения начала позиции указанной даты
			 * @param date дата для которой необходимо получить позицию
			 * @param type тип единиц измерений даты
			 * @return     начало указанной даты в формате UnixTimestamp
			 */
			uint64_t begin(const uint64_t date, const type_t type) const noexcept;
			/**
			 * begin Метод получения начала позиции текущей даты
			 * @param type    тип единиц измерений даты
			 * @param storage хранение значение времени
			 * @return        начало текущей даты в формате UnixTimestamp
			 */
			uint64_t begin(const type_t type, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * actual Метод актуализации прошедшего и оставшегося времени
			 * @param date   дата относительно которой производятся расчёты
			 * @param value  тип определяемых единиц измерений времени
			 * @param type   тип единиц измерений даты
			 * @param actual направление актуализации
			 * @return       результат вычисления
			 */
			uint64_t actual(const uint64_t date, const type_t value, const type_t type, const actual_t actual) const noexcept;
			/**
			 * actual Метод актуализации прошедшего и оставшегося времени
			 * @param value   тип определяемых единиц измерений времени
			 * @param type    тип единиц измерений даты
			 * @param actual  направление актуализации
			 * @param storage хранение значение времени
			 * @return        результат вычисления
			 */
			uint64_t actual(const type_t value, const type_t type, const actual_t actual, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * offset Метод смещения на указанное количество единиц времени
			 * @param date   дата относительно которой производится смещение
			 * @param value  значение на которое производится смещение
			 * @param type   тип единиц измерений даты
			 * @param offset направление смещения
			 * @return       результат вычисления в формате UnixTimestamp
			 */
			uint64_t offset(const uint64_t date, const uint64_t value, const type_t type, const offset_t offset) const noexcept;
			/**
			 * offset Метод смещения текущей даты на указанное количество единиц времени
			 * @param value   значение на которое производится смещение
			 * @param type    тип единиц измерений даты
			 * @param offset  направление смещения
			 * @param storage хранение значение времени
			 * @return        результат вычисления в формате UnixTimestamp
			 */
			uint64_t offset(const uint64_t value, const type_t type, const offset_t offset, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * seconds Метод получения текстового значения времени 
			 * @param seconds количество секунд для конвертации
			 * @return        обозначение времени с указанием размерности
			 */
			string seconds(const double seconds) const noexcept;
			/**
			 * seconds Метод получения размера в секундах из строки
			 * @param seconds строка обозначения размерности (s, m, h, d, w, M, y)
			 * @return        размер в секундах
			 */
			double seconds(const string & seconds) const noexcept;
		public:
			/**
			 * h12 Метод извлечения статуса 12-и часового формата времени
			 * @param date дата для проверки
			 */
			h12_t h12(const uint64_t date) const noexcept;
			/**
			 * h12 Метод извлечения текущего статуса 12-и часового формата времени
			 * @param storage хранение значение времени
			 * @return        текущее установленное значение статуса 12-и часового формата времени
			 */
			h12_t h12(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * year Метод извлечения значения года
			 * @param date дата для извлечения года
			 */
			uint16_t year(const uint64_t date) const noexcept;
			/**
			 * year Метод получение текущего значения года
			 * @param storage хранение значение времени
			 * @return        текущее значение года
			 */
			uint16_t year(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * dst Метод проверки принадлежит ли дата к лету
			 * @param date дата для проверки
			 * @return     результат проверки
			 */
			bool dst(const uint64_t date) const noexcept;
			/**
			 * dst Метод проверки стоит ли на дворе лето
			 * @param storage хранение значение времени
			 * @return        результат проверки
			 */
			bool dst(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * leap Метод проверки является ли год високосным
			 * @param date дата для проверки
			 * @return     результат проверки
			 */
			bool leap(const uint64_t date) const noexcept;
			/**
			 * leap Метод проверки является ли текущий год високосным
			 * @param storage хранение значение времени
			 * @return        результат проверки
			 */
			bool leap(const storage_t storage = storage_t::GLOBAL) const noexcept;
		private:
			/**
			 * set Метод установки данных даты и времени
			 * @param buffer бинарный буфер данных
			 * @param size   размер бинарного буфера
			 * @param unit   элементы данных для установки
			 * @param text   данные переданы в виде текста
			 */
			void set(const void * buffer, const size_t size, const unit_t unit, const bool text) noexcept;
		public:
			/**
			 * set Шаблон метода установки данных даты и времени
			 * @tparam T тип данных в котором устанавливаются данные
			 */
			template <typename T>
			/**
			 * set Метод установки данных даты и времени
			 * @param value дата для обработки
			 * @param unit  элементы данных для установки
			 */
			void set(const T value, const unit_t unit) noexcept {
				// Выполняем установку данных
				this->set(&value, sizeof(value), unit, is_class_v <T>);
			}
		private:
			/**
			 * get Метод извлечения данных даты и времени
			 * @param buffer бинарный буфер данных
			 * @param size   размер бинарного буфера
			 * @param date   дата для обработки
			 * @param unit   элементы данных для установки
			 * @param text   данные переданы в виде текста
			 */
			void get(void * buffer, const size_t size, const uint64_t date, const unit_t unit, const bool text) const noexcept;
			/**
			 * get Метод извлечения данных даты и времени
			 * @param buffer  бинарный буфер данных
			 * @param size    размер бинарного буфера
			 * @param unit    элементы данных для установки
			 * @param text    данные переданы в виде текста
			 * @param storage хранение значение времени
			 */
			void get(void * buffer, const size_t size, const unit_t unit, const bool text, const storage_t storage) const noexcept;
		public:
			/**
			 * get Шаблон метода извлечения данных даты и времени
			 * @tparam T тип данных в котором извлекаются данные
			 */
			template <typename T>
			/**
			 * get Метод извлечения данных даты и времени
			 * @param date дата для обработки
			 * @param unit элементы данных для установки
			 * @return     значение данных даты и времени
			 */
			T get(const uint64_t date, const unit_t unit) const noexcept {
				// Результат работы функции
				T result;
				// Если данные являются основными
				if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
					// Буфер результата по умолчанию
					uint8_t buffer[sizeof(T)];
					// Заполняем нулями буфер данных
					::memset(buffer, 0, sizeof(T));
					// Выполняем установку результата по умолчанию
					::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
				}
				// Выполняем извлечение данных
				this->get(&result, sizeof(result), date, unit, is_class_v <T>);
				// Выводим полученный результат
				return result;
			}
			/**
			 * get Шаблон метода извлечения данных даты и времени
			 * @tparam T тип данных в котором извлекаются данные
			 */
			template <typename T>
			/**
			 * get Метод извлечения данных даты и времени
			 * @param unit    элементы данных для установки
			 * @param storage хранение значение времени
			 * @return        значение данных даты и времени
			 */
			T get(const unit_t unit, const storage_t storage = storage_t::GLOBAL) const noexcept {
				// Результат работы функции
				T result;
				// Если данные являются основными
				if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
					// Буфер результата по умолчанию
					uint8_t buffer[sizeof(T)];
					// Заполняем нулями буфер данных
					::memset(buffer, 0, sizeof(T));
					// Выполняем установку результата по умолчанию
					::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
				}
				// Выполняем извлечение данных
				this->get(&result, sizeof(result), unit, is_class_v <T>, storage);
				// Выводим полученный результат
				return result;
			}
		public:
			/**
			 * setTimeZone Метод установки временной зоны
			 * @param zone временная зона для установки (в секундах)
			 */
			void setTimeZone(const int32_t zone) noexcept;
			/**
			 * setTimeZone Метод установки временной зоны
			 * @param zone временная зона для установки
			 */
			void setTimeZone(const zone_t zone) noexcept;
			/**
			 * setTimeZone Метод установки временной зоны
			 * @param zone временная зона для установки
			 */
			void setTimeZone(const string & zone) noexcept;
		public:
			/**
			 * matchTimeZone Метод выполнения матчинга временной зоны
			 * @param zone временная зона для конвертации
			 * @return     определённая временная зона
			 */
			zone_t matchTimeZone(const string & zone) const noexcept;
			/**
			 * matchTimeZone Метод выполнения матчинга временной зоны
			 * @param storage хранение значение времени
			 * @return        определённая временная зона
			 */
			zone_t matchTimeZone(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * getTimeZone Метод перевода временной зоны в числовой эквивалент
			 * @param zone временная зона для конвертации
			 * @return     временная зона в секундах
			 */
			int32_t getTimeZone(const zone_t zone) const noexcept;
			/**
			 * getTimeZone Метод перевода временной зоны в числовой эквивалент
			 * @param zone временная зона для конвертации
			 * @return     временная зона в секундах
			 */
			int32_t getTimeZone(const string & zone) const noexcept;
		public:
			/**
			 * getTimeZone Метод определения текущей временной зоны относительно летнего времени
			 * @param std временная зона стандартного времени
			 * @param sum временная зона летнего времени
			 * @return    текущее значение временной зоны
			 */
			int32_t getTimeZone(const zone_t std, const zone_t sum) const noexcept;
		public:
			/**
			 * getTimeZone Метод получения установленной временной зоны
			 * @param storage хранение значение времени
			 * @return        установленное значение временной зоны
			 */
			int32_t getTimeZone(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * clearTimeZones Метод очистки списка временных зон
			 */
			void clearTimeZones() noexcept;
			/**
			 * addTimeZone Метод установки собственной временной зоны
			 * @param name   название временной зоны
			 * @param offset смещение времени в миллисекундах
			 */
			void addTimeZone(const string & name, const int32_t offset) noexcept;
			/**
			 * setTimeZones Метод установки своего списка временных зон
			 * @param zones список временных зон для установки
			 */
			void setTimeZones(const unordered_map <string, int32_t> & zones) noexcept;
		public:
			/**
			 * timestamp Метод установки штампа времени в указанных единицах измерения
			 * @param date дата для установки
			 * @param type единицы измерения штампа времени
			 */
			void timestamp(const uint64_t date, const type_t type) noexcept;
			/**
			 * timestamp Метод получения штампа времени в указанных единицах измерения
			 * @param type    единицы измерения штампа времени
			 * @param storage хранение значение времени
			 * @return        штамп времени в указанных единицах измерения
			 */
			uint64_t timestamp(const type_t type, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * parse Метод парсинга строки в UnixTimestamp
			 * @param date    строка даты
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        дата в UnixTimestamp
			 */
			uint64_t parse(const string & date, const string & format, const storage_t storage = storage_t::GLOBAL) noexcept;
		public:
			/**
			 * format Метод генерации формата временной зоны
			 * @param zone временная зона (в секундах) в которой нужно получить результат
			 * @return     строковое обозначение временной зоны
			 */
			string format(const int32_t zone) const noexcept;
			/**
			 * format Метод генерации формата временной зоны
			 * @param zone временная зона в которой нужно получить результат
			 * @return     строковое обозначение временной зоны
			 */
			string format(const zone_t zone) const noexcept;
		private:
			/**
			 * format Метод формирования UnixTimestamp в строку
			 * @param dt     объект даты и времени
			 * @param format формат даты
			 * @return       строка содержащая дату
			 */
			string format(const dt_t & dt, const string & format) const noexcept;
		public:
			/**
			 * format Метод формирования UnixTimestamp в строку
			 * @param date   дата в UnixTimestamp
			 * @param format формат даты
			 * @return       строка содержащая дату
			 */
			string format(const uint64_t date, const string & format) const noexcept;
			/**
			 * format Метод формирования UnixTimestamp в строку
			 * @param date   дата в UnixTimestamp
			 * @param zone   временная зона в которой нужно получить дату (в секундах)
			 * @param format формат даты
			 * @return       строка содержащая дату
			 */
			string format(const uint64_t date, const int32_t zone, const string & format) const noexcept;
			/**
			 * format Метод формирования UnixTimestamp в строку
			 * @param date   дата в UnixTimestamp
			 * @param zone   временная зона в которой нужно получить дату
			 * @param format формат даты
			 * @return       строка содержащая дату
			 */
			string format(const uint64_t date, const zone_t zone, const string & format) const noexcept;
			/**
			 * format Метод формирования UnixTimestamp в строку
			 * @param date   дата в UnixTimestamp
			 * @param zone   временная зона в которой нужно получить дату
			 * @param format формат даты
			 * @return       строка содержащая дату
			 */
			string format(const uint64_t date, const string & zone, const string & format) const noexcept;
		public:
			/**
			 * format Метод формирования UnixTimestamp в строку
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        строка содержащая дату
			 */
			string format(const string & format, const storage_t storage = storage_t::GLOBAL) const noexcept;
			/**
			 * format Метод формирования UnixTimestamp в строку
			 * @param zone    временная зона в которой нужно получить дату (в секундах)
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        строка содержащая дату
			 */
			string format(const int32_t zone, const string & format, const storage_t storage = storage_t::GLOBAL) const noexcept;
			/**
			 * format Метод формирования UnixTimestamp в строку
			 * @param zone    временная зона в которой нужно получить дату
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        строка содержащая дату
			 */
			string format(const zone_t zone, const string & format, const storage_t storage = storage_t::GLOBAL) const noexcept;
			/**
			 * format Метод формирования UnixTimestamp в строку
			 * @param zone    временная зона в которой нужно получить дату
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        строка содержащая дату
			 */
			string format(const string & zone, const string & format, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * strip Метод получения UnixTimestamp из строки
			 * @param date    строка даты
			 * @param format1 форматы даты из которой нужно получить дату
			 * @param format2 форматы даты в который нужно перевести дату
			 * @param storage хранение значение времени
			 * @return        результат работы
			 */
			string strip(const string & date, const string & format1, const string & format2, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * Chrono Конструктор
			 * @param fmk объект фреймворка
			 */
			Chrono(const fmk_t * fmk) noexcept;
		public:
			/**
			 * ~Chrono Деструктор
			 */
			~Chrono() noexcept;
	} chrono_t;
};

#endif // __AWH_CHRONO__
