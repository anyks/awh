/**
 * @file tunnel.cpp
 * @date 2026-08-08
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
 * @brief Реализация модуля туннельных устройств MS Windows
 *
 * @details Разбор устройства обоих драйверов и доводы к принятым решениям вынесены в
 *          заголовочный файл модуля, здесь же остаётся одно исполнение
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include <cstring>
#include <unordered_map>
/**
 * @brief Средства блокировок фреймворка
 *
 * @note Замок берётся свой, а не стандартный: стандартный, захваченный в миг
 *       ветвления процесса, остаётся в потомке замкнутым навсегда - владелец в
 *       потомок не переходит, и отпереть его некому. AWH ветвится кластером, и
 *       такой замок остановил бы потомка намертво
 *
 */
#include <sys/lib.hpp>
#include <sys/locker.hpp>

/**
 * Подключаем заголовочный файл модуля
 */
#include <net/backend/win/tunnel.hpp>

/**
 * @brief Средства опроса и настройки сетевых устройств
 *
 */
#include <iphlpapi.h>
#include <netioapi.h>

/**
 * @brief Средства управления драйверами устройств
 *
 * @details Отсюда берётся сборка управляющих кодов CTL_CODE, какими драйверу
 *          tap-windows6 сообщается вид переноса и подключение устройства
 *
 */
#include <winioctl.h>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Название модуля для записей в журнале
 *
 */
static constexpr const char * __AWH_TUNNEL_BACKEND__ = "MS Windows tunnel backend";

/**
 * @brief Приставка названий устройств туннеля, заводимых для фреймворка
 *
 * @details Устройства драйвера tap-windows6 заводит УСТАНОВЩИК заранее, и движок
 *          берёт из готовых - назвать своё он не может. Приставка нужна затем,
 *          чтобы среди чужих устройств распорядителя машины предпочитались наши:
 *          «awh_tap0», «awh_tap1» и далее
 *
 * @note Берётся из краткого имени библиотеки, а не пишется рядом вторым разом:
 *       сменится имя библиотеки - сменится и приставка. Тем же порядком названы
 *       связи у Sun (`awh_tun{N}`), и одно устройство названий на все системы
 *
 * @warning Предпочтение это МЯГКОЕ: не нашлось устройств с приставкой - берётся
 *          любое свободное. Иначе фреймворк перестал бы работать на машинах, где
 *          распорядитель завёл устройства под своими названиями
 */
static constexpr const char * __AWH_TUNNEL_PREFIX__ = AWH_SHORT_NAME "_tap";

/**
 * @brief Инкапсулируем состояние модуля в пространство имён
 *
 */
namespace {
	/**
	 * @brief Описатели устройства и сеанса драйвера Wintun
	 *
	 * @details Оба непрозрачны, и объявлять их точнее указателя незачем. Взяты они
	 *          отсюда, а не из заголовка поставщика, чтобы набор не обзаводился
	 *          сторонним заголовком ради десятка объявлений
	 *
	 */
	typedef void * wintun_adapter_t;
	typedef void * wintun_session_t;
	/**
	 * @brief Подписи вызовов драйвера Wintun
	 *
	 */
	typedef wintun_adapter_t (WINAPI * wintun_create_t)(LPCWSTR, LPCWSTR, const GUID *);
	typedef wintun_adapter_t (WINAPI * wintun_open_t)(LPCWSTR);
	typedef void (WINAPI * wintun_close_t)(wintun_adapter_t);
	typedef void (WINAPI * wintun_luid_t)(wintun_adapter_t, NET_LUID *);
	typedef wintun_session_t (WINAPI * wintun_start_t)(wintun_adapter_t, DWORD);
	typedef void (WINAPI * wintun_end_t)(wintun_session_t);
	typedef HANDLE (WINAPI * wintun_event_t)(wintun_session_t);
	typedef BYTE * (WINAPI * wintun_receive_t)(wintun_session_t, DWORD *);
	typedef void (WINAPI * wintun_release_t)(wintun_session_t, const BYTE *);
	typedef BYTE * (WINAPI * wintun_allocate_t)(wintun_session_t, DWORD);
	typedef void (WINAPI * wintun_send_t)(wintun_session_t, const BYTE *);
	/**
	 * @brief Набор вызовов драйвера Wintun
	 *
	 * @details Библиотека подключается по ходу работы, а не связыванием: машина вправе
	 *          обойтись вовсе без неё, и отсутствие её обязано отвечать внятным
	 *          отказом, а не срывом запуска всего приложения
	 *
	 */
	struct wintun_t {
		HMODULE dll;                  // Описатель подключённой библиотеки
		wintun_create_t create;       // Заведение устройства
		wintun_open_t open;           // Занятие уже заведённого устройства
		wintun_close_t close;         // Устранение устройства
		wintun_luid_t luid;           // Получение местного номера устройства
		wintun_start_t start;         // Открытие сеанса обмена
		wintun_end_t end;             // Закрытие сеанса обмена
		wintun_event_t event;         // Событие готовности к чтению
		wintun_receive_t receive;     // Приём пакета из кольца
		wintun_release_t release;     // Возврат места в кольцо
		wintun_allocate_t allocate;   // Отведение места под отправку
		wintun_send_t send;           // Отправка пакета
		/**
		 * @brief Конструктор
		 *
		 */
		wintun_t() noexcept :
		 dll(nullptr), create(nullptr), open(nullptr), close(nullptr),
		 luid(nullptr), start(nullptr), end(nullptr), event(nullptr),
		 receive(nullptr), release(nullptr), allocate(nullptr), send(nullptr) {}
	};
	/**
	 * @brief Запись реестра заведённых туннельных устройств
	 *
	 */
	struct entry_t {
		awh::win::tunnel::driver_t driver;   // Драйвер, каким устройство заведено
		string name;                         // Название устройства в виде «{GUID}»
		wintun_adapter_t adapter;            // Описатель устройства Wintun
		wintun_session_t session;            // Описатель сеанса Wintun
		HANDLE handle;                       // Дескриптор файла устройства tap-windows6
		HANDLE event;                        // Событие готовности к чтению
		bool pending;                        // Признак поданного упреждающего приёма
		OVERLAPPED overlapped;               // Описатель упреждающего приёма
		std::vector <uint8_t> buffer;        // Буфер упреждающего приёма
		/**
		 * @brief Конструктор
		 *
		 */
		entry_t() noexcept :
		 driver(awh::win::tunnel::driver_t::NONE), adapter(nullptr),
		 session(nullptr), handle(INVALID_HANDLE_VALUE), event(nullptr),
		 pending(false), overlapped{} {}
	};
	/**
	 * @brief Состояние блокировок, погашенное при заведении
	 *
	 * @details `LockState` заводится ВКЛЮЧЁННЫМ: он зовёт `onEnabledChanged(true)` в теле
	 *          своего конструктора. Работа же в один поток - обычный расклад, и платить
	 *          за захват замка на каждом обращении незачем
	 *
	 * @note Гашение стоит здесь, а не отдельной строкой рядом с объявлением: на уровне
	 *       пространства имён оператор не пишется вовсе - там разрешены одни объявления.
	 *       Собственный конструктор - единственное место, где гашение оказывается ЧАСТЬЮ
	 *       заведения замка и не зависит ни от чего постороннего
	 *
	 * @tparam MutexType вид мьютекса, каким состояние распоряжается
	 *
	 */
	template <typename MutexType = std::mutex>
	struct muted_state_t : public awh::lock_state_t <MutexType> {
		/**
		 * @brief Конструктор
		 *
		 */
		muted_state_t() noexcept {
			// Гасим замок при заведении
			this->enabled = false;
		}
	};
	// Замок, оберегающий реестр заведённых устройств
	static muted_state_t <std::shared_mutex> __awh_mutex__;

	/**
	 * @brief Замок согласования подключения библиотеки драйвера
	 *
	 * @details Заведён он ОТДЕЛЬНЫМ от замка реестра намеренно: подключение библиотеки
	 *          ведётся из обращений, какие сами держат замок реестра, и общий замок
	 *          обернулся бы повторным захватом. Тем же порядком разведены замки у
	 *          подсистемы качества обслуживания (`win::qos`)
	 *
	 * @warning Подключение прежде велось БЕЗ всякого согласования, на паре простых
	 *          значений: признак попытки ставился прежде самого подключения, и поток
	 *          соседний, увидав его поставленным, уходил с набором вызовов, заполненным
	 *          наполовину, - либо с пустым описателем библиотеки (это означало бы для
	 *          него «драйвера нет вовсе»), либо с пустыми адресами вызовов при живом
	 *          описателе, а такой набор зовущая сторона разбирает уже не проверяя
	 *
	 */
	static muted_state_t <> __awh_guard__;
	// Реестр заведённых туннельных устройств
	static std::unordered_map <awh::net::socket_t, entry_t> __awh_registry__;

	/**
	 * @brief Признак безопасной работы с потоками
	 *
	 * @details Умолчание здесь ОТКЛЮЧЕНО намеренно: работа в один поток - обычный
	 *          расклад, и платить за захват замка на каждом обращении незачем.
	 *          Включается признак принудительно, обращением `threadSafety`
	 *
	 */
	static awh::event::mode_t __awh_thread_safety__ = awh::event::mode_t::DISABLED;


	/**
	 * @brief Функция подключения библиотеки драйвера Wintun
	 *
	 * @details Подключается она единожды за время работы и остаётся подключённой:
	 *          отключение её при живых устройствах устранило бы их разом
	 *
	 * @param log объект ведения журнала
	 * @return    набор вызовов драйвера либо пустое значение при отказе
	 *
	 */
	static const wintun_t * __awh_wintun__(const awh::log_t * log) noexcept {
		// Набор вызовов драйвера
		static wintun_t result;
		// Признак уже выполненной попытки подключения
		static bool attempted = false;
		// Выполняем блокировку замка согласования подключения библиотеки
		const awh::locker_t <> lock(::__awh_guard__);
		// Если попытка подключения уже выполнялась
		if(attempted)
			// Выводим набор вызовов, если подключение удалось
			return (result.dll != nullptr ? &result : nullptr);
		// Запоминаем выполнение попытки подключения
		attempted = true;
		/**
		 * Библиотека драйвера в поставку НЕ входит
		 *
		 * @details Ни `wintun.dll`, ни драйвер OpenVPN фреймворк с собою не несёт:
		 *          ставит их пользователь сам по описанию. Оттого отсутствие
		 *          библиотеки - это обычный расклад, а не отказ: движок молча
		 *          переходит на tap-windows6, и лишь когда нет и его - отвечает
		 *          отказом заведения
		 *
		 * @warning Из этого следует и то, что набор проверок по умолчанию идёт
		 *          ОДНИМ драйвером - тем, какой на машине есть. Путь Wintun
		 *          проверяется, только если библиотеку положить рядом с двоичным
		 *          файлом руками; сделано так 21.08.2026, и все пять проверок
		 *          туннеля прошли обоими драйверами
		 *
		 * @note Решено владельцем 21.08.2026
		 */
		// Выполняем подключение библиотеки драйвера
		result.dll = ::LoadLibraryExW(L"wintun.dll", nullptr, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
		// Если подключить библиотеку драйвера не удалось
		if(result.dll == nullptr){
			// Если объект ведения журнала передан
			if(log != nullptr)
				// Выводим в журнал сообщение о невозможности подключения
				log->print("%s: wintun.dll could not be loaded, error %lu", awh::log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__, ::GetLastError());
			// Выводим пустое значение
			return nullptr;
		}
		/**
		 * @brief Снятие вызова из подключённой библиотеки
		 *
		 */
		#define __AWH_BIND__(field, type, symbol) \
			result.field = reinterpret_cast <type> (reinterpret_cast <void *> (::GetProcAddress(result.dll, symbol)))
		// Выполняем снятие вызовов драйвера
		__AWH_BIND__(create, wintun_create_t, "WintunCreateAdapter");
		__AWH_BIND__(open, wintun_open_t, "WintunOpenAdapter");
		__AWH_BIND__(close, wintun_close_t, "WintunCloseAdapter");
		__AWH_BIND__(luid, wintun_luid_t, "WintunGetAdapterLUID");
		__AWH_BIND__(start, wintun_start_t, "WintunStartSession");
		__AWH_BIND__(end, wintun_end_t, "WintunEndSession");
		__AWH_BIND__(event, wintun_event_t, "WintunGetReadWaitEvent");
		__AWH_BIND__(receive, wintun_receive_t, "WintunReceivePacket");
		__AWH_BIND__(release, wintun_release_t, "WintunReleaseReceivePacket");
		__AWH_BIND__(allocate, wintun_allocate_t, "WintunAllocateSendPacket");
		__AWH_BIND__(send, wintun_send_t, "WintunSendPacket");
		// Снимаем объявление снятия вызовов
		#undef __AWH_BIND__
		// Если хотя бы один вызов драйвера снять не удалось
		if((result.create == nullptr) || (result.close == nullptr) || (result.luid == nullptr) ||
		   (result.start == nullptr) || (result.end == nullptr) || (result.event == nullptr) ||
		   (result.receive == nullptr) || (result.release == nullptr) ||
		   (result.allocate == nullptr) || (result.send == nullptr)){
			/**
			 * Обращаемся к журналу лишь при переданном объекте
			 *
			 * @warning Соседние ветви этой же функции проверку имеют, а эта её не имела,
			 *          тогда как трое зовущих из пяти передают сюда ПУСТОЙ объект журнала:
			 *          проверка доступности драйвера, приём пакета и отправка. Библиотека
			 *          с неполным составом вызовов роняла бы на них приложение обращением
			 *          по пустому указателю - там, где расклад этот обязан всего лишь
			 *          отвести движок на драйвер tap-windows6
			 *
			 */
			if(log != nullptr)
				// Выводим в журнал сообщение о несовпадении состава библиотеки
				log->print("%s: wintun.dll does not export the expected entry points", awh::log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__);
			// Выполняем отключение библиотеки драйвера
			::FreeLibrary(result.dll);
			// Сбрасываем описатель подключённой библиотеки
			result.dll = nullptr;
			// Выводим пустое значение
			return nullptr;
		}
		// Выводим набор вызовов драйвера
		return &result;
	}
	/**
	 * @brief Функция получения названия устройства по его местному номеру
	 *
	 * @details Названием устройства слой сетевых устройств считает неизменное имя вида
	 *          «{GUID}», а не переименуемое описание, и здесь оно приводится к тому же
	 *          виду - иначе заведённое устройство нельзя было бы найти опросом
	 *
	 * @param luid местный номер устройства
	 * @return     название устройства в виде «{GUID}»
	 *
	 */
	static string __awh_name__(const NET_LUID & luid) noexcept {
		// Уникальный номер устройства
		GUID guid{};
		// Если получить уникальный номер устройства не удалось
		if(::ConvertInterfaceLuidToGuid(&luid, &guid) != NO_ERROR)
			// Выводим пустое название устройства
			return string{};
		// Буфер под название устройства
		char buffer[64];
		// Выполняем сборку названия устройства
		const int32_t size = ::snprintf(
			buffer, sizeof(buffer),
			"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
			static_cast <unsigned long> (guid.Data1), guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]
		);
		// Выводим собранное название устройства
		return (size > 0 ? string(buffer, static_cast <size_t> (size)) : string{});
	}
	/**
	 * @brief Функция поиска свободного устройства драйвера tap-windows6
	 *
	 * @details Устройства эти ставит установщик драйвера, и завести новое из работающего
	 *          приложения нельзя. Перечень их система держит в ветви настроек класса
	 *          сетевых устройств, откуда и снимается уникальный номер каждого
	 *
	 * @param name  название занятого устройства
	 * @param log   объект ведения журнала
	 * @return      дескриптор занятого устройства
	 *
	 */
	/**
	 * @brief Функция получения сетевого названия устройства по его уникальному номеру
	 *
	 * @details Уникальный номер устройства и его сетевое название лежат в РАЗНЫХ ветвях
	 *          настроек: первый - в ветви класса устройств, второе - в ветви сетевых
	 *          соединений. Связывает их сам номер, и потому название спрашивается
	 *          отдельным заходом
	 *
	 * @param guid уникальный номер устройства
	 * @return     сетевое название устройства либо пустая строка
	 *
	 */
	static string __awh_title__(const wchar_t * guid) noexcept {
		// Название устройства
		string result = "";
		// Буфер под путь к ветви настроек соединения
		wchar_t path[256];
		// Выполняем сборку пути к ветви настроек соединения
		::_snwprintf(path, sizeof(path) / sizeof(path[0]), L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%ls\\Connection", guid);
		/**
		 * Досылаем окончание строки своей рукой
		 *
		 * @note Обращение это при усечении окончания НЕ ставит: у него договор иной,
		 *       нежели у `snprintf`. Путь складывается из отрезка постоянной длины и
		 *       уникального номера, и усечение здесь мыслимо лишь при негодном номере,
		 *       - но полагаться на это незачем
		 */
		path[(sizeof(path) / sizeof(path[0])) - 1] = L'\0';
		// Описатель ветви настроек соединения
		HKEY branch = nullptr;
		// Если открыть ветвь настроек соединения не удалось
		if(::RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &branch) != ERROR_SUCCESS)
			// Выводим пустое название
			return result;
		// Буфер под название соединения
		wchar_t title[256];
		// Размер названия соединения
		DWORD length = static_cast <DWORD> (sizeof(title));
		// Если название соединения получено
		if(::RegQueryValueExW(branch, L"Name", nullptr, nullptr, reinterpret_cast <LPBYTE> (title), &length) == ERROR_SUCCESS){
			/**
			 * Досылаем окончание строки своей рукой
			 *
			 * @warning Значение реестра вида REG_SZ система выдаёт БЕЗ обязательного
			 *          окончания строки, если оно так и было записано: договор обращения
			 *          прямо велит вызывающей стороне доводить строку до окончания самой,
			 *          даже когда обращение ответило успехом. Без этого сличение и перевод
			 *          записи читают за конец буфера - до первого попавшегося нуля
			 *
			 * @note Длина берётся выданная, но не больше буфера: выдай система ровно
			 *       столько, сколько тот вмещает, - место под окончание пришлось бы взять
			 *       у последнего знака
			 *
			 */
			title[((length / sizeof(wchar_t)) < 256) ? (length / sizeof(wchar_t)) : 255] = L'\0';
			// Буфер под название соединения в узкой кодировке
			char buffer[256];
			// Выполняем перевод названия соединения в узкую кодировку
			const int32_t count = ::WideCharToMultiByte(CP_UTF8, 0, title, -1, buffer, static_cast <int32_t> (sizeof(buffer)), nullptr, nullptr);
			// Если перевод названия соединения выполнен
			if(count > 1)
				// Запоминаем название соединения
				result.assign(buffer, static_cast <size_t> (count - 1));
		}
		// Выполняем закрытие ветви настроек соединения
		::RegCloseKey(branch);
		// Выводим название устройства
		return result;
	}
	static HANDLE __awh_tap__(string & name, const awh::log_t * log) noexcept {
		/**
		 * @brief Ветвь настроек класса сетевых устройств
		 *
		 */
		static constexpr const wchar_t * BRANCH = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
		// Описатель ветви настроек класса сетевых устройств
		HKEY branch = nullptr;
		// Если открыть ветвь настроек класса сетевых устройств не удалось
		if(::RegOpenKeyExW(HKEY_LOCAL_MACHINE, BRANCH, 0, KEY_READ, &branch) != ERROR_SUCCESS){
			// Если объект ведения журнала передан
			if(log != nullptr)
				// Выводим в журнал сообщение о невозможности опроса устройств
				log->print("%s: network adapter class registry branch could not be opened", awh::log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__);
			// Выводим пустой дескриптор
			return INVALID_HANDLE_VALUE;
		}
		// Результат занятия устройства
		HANDLE result = INVALID_HANDLE_VALUE;
		/**
		 * Соблюдаем заказанное название устройства
		 *
		 * @details Названное потребителем берётся как есть: молчаливая подмена выдала
		 *          бы ему не то устройство, какое он назвал. Порядок тот же, что у
		 *          связей Sun - там имя тоже либо названо, либо отыскивается перебором
		 *
		 * @note Заказ запоминается ДО перебора: тот же довод служит и возвратом
		 *       уникального номера занятого устройства, и к концу работы заказ в нём
		 *       уже затёрт
		 */
		const string ordered = name;
		/**
		 * Перебор идёт ДВАЖДЫ, если заказа не было
		 *
		 * @details Первым заходом спрашиваются устройства с нашей приставкой
		 *          («awh_tap0», «awh_tap1» и далее), вторым - любые свободные.
		 *          Так наши устройства предпочитаются чужим, но работа не встаёт
		 *          на машине, где распорядитель завёл их под своими названиями
		 */
		for(uint8_t pass = 0; (pass < 2) && (result == INVALID_HANDLE_VALUE); pass++){
		// Если заказ был, второго захода не требуется вовсе
		if((pass > 0) && !ordered.empty()) break;
		/**
		 * Выполняем перебор всех устройств класса сетевых устройств
		 */
		for(DWORD index = 0; result == INVALID_HANDLE_VALUE; index++){
			// Буфер под название записи устройства
			wchar_t key[256];
			// Размер названия записи устройства
			DWORD size = static_cast <DWORD> (sizeof(key) / sizeof(key[0]));
			// Если записи устройств исчерпаны
			if(::RegEnumKeyExW(branch, index, key, &size, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
				// Завершаем перебор записей устройств
				break;
			// Описатель записи устройства
			HKEY entry = nullptr;
			// Если открыть запись устройства не удалось
			if(::RegOpenKeyExW(branch, key, 0, KEY_READ, &entry) != ERROR_SUCCESS)
				// Переходим к следующей записи устройства
				continue;
			// Буфер под опознаватель драйвера устройства
			wchar_t component[64];
			// Размер опознавателя драйвера устройства
			DWORD length = static_cast <DWORD> (sizeof(component));
			// Признак принадлежности устройства драйверу tap-windows6
			bool matched = false;
			// Если опознаватель драйвера устройства получен
			if(::RegQueryValueExW(entry, L"ComponentId", nullptr, nullptr, reinterpret_cast <LPBYTE> (component), &length) == ERROR_SUCCESS){
				/**
				 * Досылаем окончание строки своей рукой
				 *
				 * @warning Значение реестра вида REG_SZ система выдаёт БЕЗ обязательного
				 *          окончания строки, если оно так и было записано: договор обращения
				 *          прямо велит вызывающей стороне доводить строку до окончания самой,
				 *          даже когда обращение ответило успехом. Без этого сличение и перевод
				 *          записи читают за конец буфера - до первого попавшегося нуля
				 *
				 * @note Длина берётся выданная, но не больше буфера: выдай система ровно
				 *       столько, сколько тот вмещает, - место под окончание пришлось бы взять
				 *       у последнего знака
				 *
				 */
				component[((length / sizeof(wchar_t)) < 64) ? (length / sizeof(wchar_t)) : 63] = L'\0';
				// Запоминаем принадлежность устройства драйверу tap-windows6
				matched = ((::wcscmp(component, L"tap0901") == 0) || (::wcscmp(component, L"root\\tap0901") == 0));
			}
			// Если устройство драйверу tap-windows6 не принадлежит
			if(!matched){
				// Выполняем закрытие записи устройства
				::RegCloseKey(entry);
				// Переходим к следующей записи устройства
				continue;
			}
			// Буфер под уникальный номер устройства
			wchar_t guid[64];
			// Размер уникального номера устройства
			length = static_cast <DWORD> (sizeof(guid));
			// Если уникальный номер устройства получен
			if(::RegQueryValueExW(entry, L"NetCfgInstanceId", nullptr, nullptr, reinterpret_cast <LPBYTE> (guid), &length) == ERROR_SUCCESS){
				/**
				 * Досылаем окончание строки своей рукой
				 *
				 * @warning Значение реестра вида REG_SZ система выдаёт БЕЗ обязательного
				 *          окончания строки, если оно так и было записано: договор обращения
				 *          прямо велит вызывающей стороне доводить строку до окончания самой,
				 *          даже когда обращение ответило успехом. Без этого сличение и перевод
				 *          записи читают за конец буфера - до первого попавшегося нуля
				 *
				 * @note Длина берётся выданная, но не больше буфера: выдай система ровно
				 *       столько, сколько тот вмещает, - место под окончание пришлось бы взять
				 *       у последнего знака
				 *
				 */
				guid[((length / sizeof(wchar_t)) < 64) ? (length / sizeof(wchar_t)) : 63] = L'\0';
				// Сетевое название этого устройства
				const string title = ::__awh_title__(guid);
				/**
				 * Отсеиваем устройство по названию
				 *
				 * @note Заказ соблюдается совпадением; при первом заходе без заказа
				 *       берутся лишь устройства с нашей приставкой, при втором - любые
				 *
				 * @warning Сличение ведётся БЕЗ УЧЁТА РЕГИСТРА намеренно: краткое имя
				 *          библиотеки записано заглавными («AWH»), а распорядитель машины
				 *          называет устройства как ему привычно - «awh_tap1». Сличение
				 *          по регистру давало ложный отказ, и предпочтение не работало
				 *          вовсе: движок брал первое попавшееся чужое устройство
				 */
				const bool suitable = (!ordered.empty() ? (::_stricmp(title.c_str(), ordered.c_str()) == 0) :
					((pass > 0) || (::_strnicmp(title.c_str(), ::__AWH_TUNNEL_PREFIX__, ::strlen(::__AWH_TUNNEL_PREFIX__)) == 0)));
				// Если устройство не подходит, переходим к следующему
				if(!suitable){
					// Выполняем закрытие записи устройства
					::RegCloseKey(entry);
					// Переходим к следующей записи устройства
					continue;
				}
				// Буфер под путь к устройству
				wchar_t path[128];
				// Выполняем сборку пути к устройству
				::_snwprintf(path, sizeof(path) / sizeof(path[0]), L"\\\\.\\Global\\%ls.tap", guid);
				// Досылаем окончание строки своей рукой: при усечении обращение его не ставит
				path[(sizeof(path) / sizeof(path[0])) - 1] = L'\0';
				/**
				 * Открывается устройство обязательно с перекрытым обменом
				 *
				 * @note На дескрипторе без перекрытия система обмен упорядочивает, и
				 *       незавершённое чтение задержало бы запись из другого потока
				 *
				 */
				result = ::CreateFileW(
					path, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
					OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, nullptr
				);
				// Если устройство занять удалось
				if(result != INVALID_HANDLE_VALUE){
					// Буфер под название занятого устройства
					char buffer[64];
					// Выполняем перевод названия устройства в узкую кодировку
					const int32_t count = ::WideCharToMultiByte(CP_UTF8, 0, guid, -1, buffer, static_cast <int32_t> (sizeof(buffer)), nullptr, nullptr);
					// Если название устройства переведено
					if(count > 0)
						// Запоминаем название занятого устройства
						name.assign(buffer, static_cast <size_t> (count - 1));
				}
			}
			// Выполняем закрытие записи устройства
			::RegCloseKey(entry);
		}
		// Выполняем закрытие ветви настроек класса сетевых устройств
		}
		// Завершаем перебор и закрываем ветвь настроек
		::RegCloseKey(branch);
		// Если свободного устройства найти не удалось
		if((result == INVALID_HANDLE_VALUE) && (log != nullptr))
			// Выводим в журнал сообщение об отсутствии свободных устройств
			log->print("%s: no free tap-windows6 adapter is available, the driver installer creates them", awh::log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__);
		// Выводим результат занятия устройства
		return result;
	}
	/**
	 * @brief Функция перевода устройства драйвера tap-windows6 в рабочее состояние
	 *
	 * @details Устройство после открытия молчит, покуда драйверу не сообщено, что оно
	 *          подключено. Вид же переноса - пакеты сетевого уровня либо кадры
	 *          канального - задаётся тому же драйверу отдельно
	 *
	 * @param handle дескриптор устройства
	 * @param tun    признак переноса пакетов сетевого уровня
	 * @return       результат выполнения перевода
	 *
	 */
	static bool __awh_activate__(HANDLE handle, const bool tun) noexcept {
		/**
		 * @brief Сборка управляющего кода драйвера tap-windows6
		 *
		 */
		#define __AWH_TAP_CONTROL__(code) CTL_CODE(FILE_DEVICE_UNKNOWN, (code), METHOD_BUFFERED, FILE_ANY_ACCESS)
		// Размер переданных драйверу данных
		DWORD size = 0;
		// Если устройство переносит пакеты сетевого уровня
		if(tun){
			/**
			 * Адреса эти драйвер требует, но занятыми их не считает: они лишь
			 * очерчивают сеть, пакеты которой устройство принимает. Настоящие адреса
			 * туннеля сообщаются позже, обращением `configure`, - в миг заведения
			 * устройства их ещё нет и взять их неоткуда
			 *
			 * @warning Второе поле - это СЕТЬ, а не адрес узла в ней, и драйвер сличает
			 *          её с маской: пара, где часть узла отлична от нуля, отвергается
			 *          кодом 87 (`ERROR_INVALID_PARAMETER`). Прежде здесь стояло
			 *          10.0.0.1 при маске 255.255.255.0 - сеть невозможная, - и
			 *          устройство не заводилось ВОВСЕ: перевод в рабочее состояние
			 *          отвечал отказом, а заведение обрывалось следом. То есть туннель
			 *          поверх tap-windows6 не работал никогда, и выдавал себя отказ
			 *          лишь записью в журнале
			 *
			 * @note Установлено щупом на стенде Windows ARM64 20.08.2026: те же числа,
			 *       поданные драйверу отдельной программой, отвергаются кодом 87, а с
			 *       сетью 10.0.0.0 настройка принимается
			 */
			ULONG config[3] = { 0x0100000A, 0x0000000A, 0x00FFFFFF };
			// Если сообщить драйверу о переносе пакетов сетевого уровня не удалось
			if(!::DeviceIoControl(handle, __AWH_TAP_CONTROL__(10), config, sizeof(config), config, sizeof(config), &size, nullptr))
				// Выводим отрицательный результат перевода
				return false;
		}
		// Признак подключения устройства
		ULONG status = 1;
		// Выполняем сообщение драйверу о подключении устройства
		const bool result = ::DeviceIoControl(handle, __AWH_TAP_CONTROL__(6), &status, sizeof(status), &status, sizeof(status), &size, nullptr);
		// Снимаем объявление сборки управляющего кода
		#undef __AWH_TAP_CONTROL__
		// Выводим результат выполнения перевода
		return result;
	}
	/**
	 * @brief Функция подачи упреждающего приёма устройству tap-windows6
	 *
	 * @details Драйвер этот события готовности не выдаёт вовсе, а событие завершения
	 *          обмена взводит: значит готовность приходится изображать самим приёмом,
	 *          поданным заранее. Событие поданного приёма взводится, едва пакет лёг в
	 *          буфер, и годится в ожидание наравне с событием Wintun
	 *
	 * @warning Приём подаётся ЗАРАНЕЕ, а не по готовности: у Wintun пакет ждёт разбора
	 *          в кольце, и спросить о нём можно когда угодно, а tap-windows6 отдаёт
	 *          пакет лишь тому, кто попросил его прежде прихода. Без упреждающего
	 *          приёма событие не взводится никогда, и туннель молчит навсегда
	 *
	 * @param entry запись устройства, какому подаётся приём
	 * @return      признак принятой системой подачи
	 *
	 */
	static bool __awh_arm__(entry_t & entry) noexcept {
		// Если приём уже подан либо устройство к приёму не готово
		if(entry.pending || (entry.handle == INVALID_HANDLE_VALUE) || (entry.event == nullptr))
			// Выводим признак поданности приёма
			return entry.pending;
		// Выполняем сброс описателя упреждающего приёма
		entry.overlapped = OVERLAPPED{};
		// Устанавливаем событие завершения упреждающего приёма
		entry.overlapped.hEvent = entry.event;
		// Выполняем сброс события завершения упреждающего приёма
		::ResetEvent(entry.event);
		// Размер принятого пакета
		DWORD length = 0;
		// Выполняем подачу упреждающего приёма
		if(::ReadFile(entry.handle, entry.buffer.data(), static_cast <DWORD> (entry.buffer.size()), &length, &entry.overlapped)){
			/**
			 * Взводим событие сами: приём выполнен разом
			 *
			 * @note Система событие в этом случае взводит не всегда, а ждущая сторона
			 *       обязана узнать о готовности одинаково при обоих исходах подачи
			 *
			 */
			::SetEvent(entry.event);
			// Запоминаем поданность упреждающего приёма
			entry.pending = true;
		// Если подача упреждающего приёма принята системой
		} else if(::GetLastError() == ERROR_IO_PENDING)
			// Запоминаем поданность упреждающего приёма
			entry.pending = true;
		// Выводим признак поданности приёма
		return entry.pending;
	}
}

/**
 * @brief Функция установки режима безопасной работы с потоками
 *
 * @details Замки модуля по умолчанию ПОГАШЕНЫ: работа в один поток - обычный расклад,
 *          и платить за захват на каждом обращении незачем. Включаются они отсюда,
 *          тем же порядком, каким это сделано у наречий POSIX
 *
 * @param mode устанавливаемый режим безопасной работы с потоками
 *
 */
void awh::win::tunnel::threadSafety(const bool mode) noexcept {
	// Запоминаем режим безопасной работы с потоками
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
	// Приводим замок к установленному режиму
	::__awh_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Приводим замок согласования подключения библиотеки к установленному режиму
	::__awh_guard__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
}

/**
 * @brief Функция сообщения драйверу адресов туннеля
 *
 * @details Драйвер переводится в режим точки-точки настоящими адресами туннеля. Прежде
 *          режим этот задавался в миг заведения устройства числами, зашитыми в коде
 *          (10.0.0.1 в сети 10.0.0.0/24), - взять настоящие тогда было неоткуда, они
 *          ещё не назначены. Числа эти определяли не только обёртку, но и ОТБОР: в
 *          режиме переноса пакетов сетевого уровня драйвер пропускает лишь те пакеты,
 *          что адресованы настроенной сети
 *
 * @details Установлено щупом на стенде Windows ARM64 20.08.2026. Туннель заведён на
 *          10.77.0.1, встречная сторона 10.77.0.2, за пять секунд послано четыре
 *          запроса эха:
 *
 *          | режим драйвера | пакетов к встречной стороне |
 *          |---|---|
 *          | зашитые числа 10.0.0.x | 0 |
 *          | настоящие адреса сетью | 4 |
 *          | настоящие адреса точкой-точкой | 4 |
 *
 *          То есть с зашитыми числами обмен по IPv4 через устройство не шёл ВОВСЕ, и
 *          выдавала себя беда лишь молчанием туннеля
 *
 * @note Обёртка от режима не зависит: голый пакет драйвер отдаёт в любом настроенном
 *       режиме, а кадр канального уровня - лишь у устройства, не настроенного вовсе.
 *       Проверено тем же щупом
 *
 * @note Пакеты IPv6 отбору этому не подвержены: в том же опыте их приходило по три
 *       десятка при любом режиме, включая зашитые числа. Управляющие коды режима
 *       туннеля у tap-windows6 знают один лишь IPv4
 *
 * @param sock   дескриптор туннельного устройства
 * @param local  адрес своего конца туннеля
 * @param remote адрес встречного конца туннеля
 * @param log    объект ведения журнала
 * @return       результат выполнения сообщения
 *
 */
bool awh::win::tunnel::configure(const net::socket_t sock, const uint32_t local, const uint32_t remote, const log_t * log) noexcept {
	// Дескриптор устройства, каким оно заведено драйвером tap-windows6
	HANDLE handle = INVALID_HANDLE_VALUE;
	{
		// Выполняем блокировку реестра заведённых устройств
		const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск устройства в реестре
		auto i = ::__awh_registry__.find(sock);
		// Если устройство в реестре не значится
		if(i == ::__awh_registry__.end())
			// Выводим отрицательный результат сообщения
			return false;
		/**
		 * Устройству Wintun сообщать нечего
		 *
		 * @note Отбора по настроенной сети у него нет: кольцо переносит всё, что в него
		 *       положено, и согласие здесь означает не сделанную работу, а её ненужность
		 */
		if(i->second.driver != driver_t::TAP)
			// Выводим положительный результат сообщения
			return true;
		// Запоминаем дескриптор устройства
		handle = i->second.handle;
	}
	// Если дескриптор устройства не заведён
	if(handle == INVALID_HANDLE_VALUE)
		// Выводим отрицательный результат сообщения
		return false;
	/**
	 * Собираем управляющий код драйвера tap-windows6
	 */
	#define __AWH_TAP_CONTROL__(code) CTL_CODE(FILE_DEVICE_UNKNOWN, (code), METHOD_BUFFERED, FILE_ANY_ACCESS)
	// Адреса концов туннеля, драйверу передаваемые
	ULONG config[2] = {static_cast <ULONG> (local), static_cast <ULONG> (remote)};
	// Размер переданных драйверу данных
	DWORD size = 0;
	// Выполняем сообщение драйверу адресов туннеля
	const bool result = ::DeviceIoControl(handle, __AWH_TAP_CONTROL__(5), config, sizeof(config), config, sizeof(config), &size, nullptr);
	// Снимаем объявление сборки управляющего кода
	#undef __AWH_TAP_CONTROL__
	// Если сообщить драйверу адреса туннеля не удалось
	if(!result && (log != nullptr))
		// Выводим в журнал сообщение о невозможности сообщения адресов
		log->print("%s: tap-windows6 adapter could not be configured for the point-to-point mode, error %lu", log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__, ::GetLastError());
	// Выводим результат выполнения сообщения
	return result;
}
/**
 * @brief Функция заведения туннельного устройства
 *
 * @param type   вид заводимого устройства
 * @param driver драйвер, каким устройство заводится
 * @param name   название заводимого устройства
 * @param log    объект ведения журнала
 * @return       дескриптор заведённого устройства
 *
 */
awh::net::socket_t awh::win::tunnel::create(const event::eth_t type, const driver_t driver, string & name, const log_t * log) noexcept {
	// Если вид заводимого устройства не поддерживается
	if((type != event::eth_t::TUN) && (type != event::eth_t::TAP)){
		// Выводим в журнал сообщение о неподдерживаемом виде устройства
		log->print("%s: only TUN and TAP devices are supported", log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__);
		// Выводим пустой дескриптор
		return net::invalid_socket_t;
	}
	// Если кадры канального уровня запрошены у драйвера, какой их не переносит
	if((type == event::eth_t::TAP) && (driver == driver_t::WINTUN)){
		// Выводим в журнал сообщение о несовместимости драйвера с видом устройства
		log->print("%s: Wintun carries network layer packets only, TAP requires tap-windows6", log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__);
		// Выводим пустой дескриптор
		return net::invalid_socket_t;
	}
	// Заводимая запись реестра устройств
	entry_t entry;
	/**
	 * Определяем драйвер, каким устройство заводится
	 */
	switch(static_cast <uint8_t> (driver)){
		// Если устройство заводится драйвером Wintun
		case static_cast <uint8_t> (driver_t::WINTUN): {
			// Выполняем подключение библиотеки драйвера
			const wintun_t * wintun = ::__awh_wintun__(log);
			// Если подключить библиотеку драйвера не удалось
			if(wintun == nullptr)
				// Выводим пустой дескриптор
				return net::invalid_socket_t;
			// Описание заводимого устройства
			const string & title = (name.empty() ? string{"AWH"} : name);
			// Буфер под описание заводимого устройства
			std::vector <wchar_t> buffer(title.size() + 1, 0);
			// Выполняем перевод описания устройства в широкую кодировку
			::MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, buffer.data(), static_cast <int32_t> (buffer.size()));
			// Выполняем заведение устройства
			entry.adapter = wintun->create(buffer.data(), L"AWH", nullptr);
			// Если завести устройство не удалось
			if(entry.adapter == nullptr){
				// Выводим в журнал сообщение о невозможности заведения устройства
				log->print("%s: Wintun adapter could not be created, error %lu", log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__, ::GetLastError());
				// Выводим пустой дескриптор
				return net::invalid_socket_t;
			}
			/**
			 * Объём кольца берётся наибольшим из допустимых
			 *
			 * @note Кольцо это - единственное место, где пакеты ждут разбора, и
			 *       переполнение его отбрасывает их молча. Памяти под наибольший
			 *       объём уходит четыре мегабайта на устройство
			 *
			 */
			entry.session = wintun->start(entry.adapter, 0x400000);
			// Если открыть сеанс обмена не удалось
			if(entry.session == nullptr){
				// Выводим в журнал сообщение о невозможности открытия сеанса обмена
				log->print("%s: Wintun session could not be started, error %lu", log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__, ::GetLastError());
				// Выполняем устранение заведённого устройства
				wintun->close(entry.adapter);
				// Выводим пустой дескриптор
				return net::invalid_socket_t;
			}
			// Местный номер заведённого устройства
			NET_LUID luid{};
			// Выполняем получение местного номера заведённого устройства
			wintun->luid(entry.adapter, &luid);
			// Запоминаем название заведённого устройства
			entry.name = ::__awh_name__(luid);
			// Запоминаем событие готовности устройства к чтению
			entry.event = wintun->event(entry.session);
			/**
			 * Событие готовности обязано быть получено
			 *
			 * @warning Дескриптором устройства у Wintun служит именно это событие, а
			 *          недопустимым дескриптором у MS Windows считается `~0`. Пустое
			 *          событие дало бы НОЛЬ - величину допустимую, - и устройство без
			 *          события легло бы в реестр как живое: проверка вызывающего
			 *          `== invalid_socket_t` его бы пропустила, а ждать готовности было
			 *          бы не на чем. Прочие шаги заведения проверены все до одного,
			 *          этот же оставался без проверки
			 *
			 * @note Замером отказ этот НЕ пойман: драйвер отдаёт событие всякому живому
			 *       сеансу, и повода отказать у него нет. Проверка стоит здесь не как
			 *       лечение замеренной беды, а как та же проверка, какую имеют все
			 *       соседние шаги
			 */
			if(entry.event == nullptr){
				// Выводим в журнал сообщение о невозможности получения события готовности
				log->print("%s: Wintun readiness event could not be obtained, error %lu", log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__, ::GetLastError());
				// Выполняем закрытие открытого сеанса обмена
				wintun->end(entry.session);
				// Выполняем устранение заведённого устройства
				wintun->close(entry.adapter);
				// Выводим пустой дескриптор
				return net::invalid_socket_t;
			}
			// Запоминаем драйвер, каким устройство заведено
			entry.driver = driver_t::WINTUN;
		} break;
		// Если устройство заводится драйвером tap-windows6
		case static_cast <uint8_t> (driver_t::TAP): {
			/**
			 * Заказанное название устройства передаём отбору
			 *
			 * @warning Прежде отбору уходила ПУСТАЯ строка свежей записи, а заказ
			 *          оставался в доводе нетронутым: заказ принимался, но брался
			 *          всё равно первый свободный адаптер. Замер этого не показывал -
			 *          первым свободным как раз и оказывался заказанный
			 */
			entry.name = name;
			// Выполняем занятие свободного устройства
			entry.handle = ::__awh_tap__(entry.name, log);
			// Если занять свободное устройство не удалось
			if(entry.handle == INVALID_HANDLE_VALUE)
				// Выводим пустой дескриптор
				return net::invalid_socket_t;
			// Если перевести устройство в рабочее состояние не удалось
			if(!::__awh_activate__(entry.handle, (type == event::eth_t::TUN))){
				// Выводим в журнал сообщение о невозможности перевода устройства
				log->print("%s: tap-windows6 adapter could not be brought up, error %lu", log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__, ::GetLastError());
				// Выполняем освобождение занятого устройства
				::CloseHandle(entry.handle);
				// Выводим пустой дескриптор
				return net::invalid_socket_t;
			}
			/**
			 * Заводим событие готовности и буфер упреждающего приёма
			 *
			 * @note Событие заводится СБРАСЫВАЕМЫМ вручную: ждущих у него вправе
			 *       оказаться несколько, а сбрасывает его подача следующего приёма
			 *
			 */
			entry.event = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
			// Если завести событие готовности не удалось
			if(entry.event == nullptr){
				// Выводим в журнал сообщение о невозможности заведения события
				log->print("%s: tap-windows6 readiness event could not be created, error %lu", log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__, ::GetLastError());
				// Выполняем освобождение занятого устройства
				::CloseHandle(entry.handle);
				// Выводим пустой дескриптор
				return net::invalid_socket_t;
			}
			/**
			 * Объём буфера берётся с запасом на кадр канального уровня
			 *
			 * @note Буфер этот - место, куда драйвер кладёт пакет ещё до того, как о
			 *       нём спросили, и меньше наибольшего кадра он быть не вправе:
			 *       короткий буфер обрывает приём отказом `ERROR_MORE_DATA`
			 *
			 */
			entry.buffer.resize(0xFFFF, 0);
			/**
			 * Запоминаем драйвер, каким устройство заведено
			 *
			 * @warning Упреждающий приём здесь НЕ подаётся: запись эта переезжает в
			 *          реестр перемещением, а драйвер держит адрес описателя приёма,
			 *          какой при переезде меняется. Подаётся приём уже над записью,
			 *          лежащей в реестре - у выдачи события готовности и у самого приёма
			 *
			 */
			entry.driver = driver_t::TAP;
		} break;
		// Если драйвер заведения устройства не определён
		default: {
			// Выводим в журнал сообщение о неопределённом драйвере
			log->print("%s: tunnel driver is not specified", log_t::flag_t::WARNING, ::__AWH_TUNNEL_BACKEND__);
			// Выводим пустой дескриптор
			return net::invalid_socket_t;
		}
	}
	/**
	 * Дескриптором устройства служит событие готовности у Wintun и дескриптор файла
	 * у tap-windows6 - оба уникальны и оба ложатся в net::socket_t без потерь
	 *
	 */
	const net::socket_t result = reinterpret_cast <net::socket_t> (
		entry.driver == driver_t::WINTUN ? entry.event : entry.handle
	);
	// Запоминаем название заведённого устройства
	name = entry.name;
	// Выполняем блокировку реестра заведённых устройств
	const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем занесение устройства в реестр
	::__awh_registry__.emplace(result, ::std::move(entry));
	// Выводим дескриптор заведённого устройства
	return result;
}
/**
 * @brief Функция устранения туннельного устройства
 *
 * @param sock дескриптор устраняемого устройства
 * @param log  объект ведения журнала
 * @return     результат выполнения устранения
 *
 */
bool awh::win::tunnel::destroy(const net::socket_t sock, const log_t * log) noexcept {
	// Устраняемая запись реестра устройств
	entry_t entry;
	{
		// Выполняем блокировку реестра заведённых устройств
		const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Выполняем поиск устройства в реестре
		auto i = ::__awh_registry__.find(sock);
		// Если устройство в реестре не значится
		if(i == ::__awh_registry__.end())
			// Выводим отрицательный результат устранения
			return false;
		/**
		 * Отменяем поданный упреждающий приём прежде переезда записи
		 *
		 * @warning Порядок здесь значим: драйвер держит адрес описателя приёма, а
		 *          переезд записи его меняет. Отмена по новому адресу драйверу
		 *          неизвестна, и приём остаётся поданным над памятью, какой уже нет
		 *
		 */
		if(i->second.pending){
			// Выполняем отмену поданного упреждающего приёма
			::CancelIoEx(i->second.handle, &i->second.overlapped);
			// Размер принятого пакета
			DWORD length = 0;
			// Выполняем ожидание завершения отменяемого приёма
			::GetOverlappedResult(i->second.handle, &i->second.overlapped, &length, TRUE);
			// Снимаем признак поданности упреждающего приёма
			i->second.pending = false;
		}
		// Снимаем запись устраняемого устройства
		entry = ::std::move(i->second);
		// Выполняем изъятие устройства из реестра
		::__awh_registry__.erase(i);
	}
	/**
	 * Определяем драйвер, каким устройство заведено
	 */
	switch(static_cast <uint8_t> (entry.driver)){
		// Если устройство заведено драйвером Wintun
		case static_cast <uint8_t> (driver_t::WINTUN): {
			// Выполняем подключение библиотеки драйвера
			const wintun_t * wintun = ::__awh_wintun__(log);
			// Если библиотека драйвера подключена
			if(wintun != nullptr){
				// Выполняем закрытие сеанса обмена
				wintun->end(entry.session);
				// Выполняем устранение заведённого устройства
				wintun->close(entry.adapter);
			}
		} break;
		// Если устройство заведено драйвером tap-windows6
		case static_cast <uint8_t> (driver_t::TAP): {
			// Выполняем освобождение занятого устройства
			::CloseHandle(entry.handle);
			// Если событие готовности заведено
			if(entry.event != nullptr)
				// Выполняем закрытие события готовности
				::CloseHandle(entry.event);
		} break;
	}
	// Выводим положительный результат устранения
	return true;
}
/**
 * @brief Функция проверки доступности драйвера туннельных устройств
 *
 * @param driver проверяемый драйвер
 * @return       признак доступности драйвера
 *
 */
bool awh::win::tunnel::available(const driver_t driver) noexcept {
	/**
	 * Определяем проверяемый драйвер
	 */
	switch(static_cast <uint8_t> (driver)){
		// Если проверяется драйвер Wintun
		case static_cast <uint8_t> (driver_t::WINTUN):
			// Выводим признак доступности библиотеки драйвера
			return (::__awh_wintun__(nullptr) != nullptr);
		// Если проверяется драйвер tap-windows6
		case static_cast <uint8_t> (driver_t::TAP): {
			// Название занятого для проверки устройства
			string name;
			/**
			 * Доступность проверяется занятием свободного устройства
			 *
			 * @note Иного способа узнать, осталось ли свободное устройство, драйвер
			 *       не даёт: перечень настроек показывает все, а занято ли каждое из
			 *       них, выясняется лишь попыткой его открыть
			 *
			 */
			HANDLE handle = ::__awh_tap__(name, nullptr);
			// Если свободное устройство нашлось
			if(handle != INVALID_HANDLE_VALUE){
				// Выполняем освобождение занятого устройства
				::CloseHandle(handle);
				// Выводим признак доступности драйвера
				return true;
			}
		} break;
	}
	// Выводим признак недоступности драйвера
	return false;
}
/**
 * @brief Функция поиска туннельного устройства по его названию
 *
 * @param name название искомого устройства
 * @return     дескриптор найденного устройства
 *
 */
awh::net::socket_t awh::win::tunnel::find(const string & name) noexcept {
	// Выполняем блокировку реестра заведённых устройств
	const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::SHARED);
	/**
	 * Выполняем перебор всех заведённых туннельных устройств
	 */
	for(auto & item : ::__awh_registry__){
		// Если название устройства с искомым совпало
		if(item.second.name.compare(name) == 0)
			// Выводим дескриптор найденного устройства
			return item.first;
	}
	// Выводим пустой дескриптор
	return net::invalid_socket_t;
}
/**
 * @brief Функция проверки принадлежности дескриптора туннельному устройству
 *
 * @param sock проверяемый дескриптор
 * @return     признак принадлежности дескриптора туннелю
 *
 */
bool awh::win::tunnel::exists(const net::socket_t sock) noexcept {
	// Выполняем блокировку реестра заведённых устройств
	const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выводим признак наличия устройства в реестре
	return (::__awh_registry__.find(sock) != ::__awh_registry__.end());
}
/**
 * @brief Функция получения драйвера туннельного устройства
 *
 * @param sock дескриптор туннельного устройства
 * @return     драйвер, каким устройство заведено
 *
 */
awh::win::tunnel::driver_t awh::win::tunnel::driver(const net::socket_t sock) noexcept {
	// Выполняем блокировку реестра заведённых устройств
	const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем поиск устройства в реестре
	auto i = ::__awh_registry__.find(sock);
	// Выводим драйвер, каким устройство заведено
	return (i != ::__awh_registry__.end() ? i->second.driver : driver_t::NONE);
}
/**
 * @brief Функция получения названия туннельного устройства
 *
 * @param sock дескриптор туннельного устройства
 * @return     название туннельного устройства
 *
 */
string awh::win::tunnel::name(const net::socket_t sock) noexcept {
	// Выполняем блокировку реестра заведённых устройств
	const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем поиск устройства в реестре
	auto i = ::__awh_registry__.find(sock);
	// Выводим название туннельного устройства
	return (i != ::__awh_registry__.end() ? i->second.name : string{});
}
/**
 * @brief Функция получения события готовности к чтению
 *
 * @param sock дескриптор туннельного устройства
 * @return     событие готовности устройства к чтению
 *
 */
HANDLE awh::win::tunnel::event(const net::socket_t sock) noexcept {
	// Выполняем блокировку реестра заведённых устройств
	const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем поиск устройства в реестре
	auto i = ::__awh_registry__.find(sock);
	// Если устройство в реестре не значится
	if(i == ::__awh_registry__.end())
		// Выводим пустое событие готовности
		return nullptr;
	/**
	 * Устройству tap-windows6 подаём упреждающий приём
	 *
	 * @details Событие его взводится завершением обмена, а не приходом пакета:
	 *          неподанный приём не взводит событие никогда, сколько бы пакетов
	 *          устройству ни пришло
	 *
	 */
	if(i->second.driver == driver_t::TAP)
		// Выполняем подачу упреждающего приёма
		::__awh_arm__(i->second);
	// Выводим событие готовности устройства к чтению
	return i->second.event;
}
/**
 * @brief Функция приёма пакета из туннельного устройства
 *
 * @param sock   дескриптор туннельного устройства
 * @param buffer буфер, в который принимается пакет
 * @param size   размер буфера приёма
 * @return       размер принятого пакета, ноль если пакетов нет, -1 при отказе
 *
 */
int64_t awh::win::tunnel::read(const net::socket_t sock, void * buffer, const size_t size) noexcept {
	// Если буфер приёма не передан
	if((buffer == nullptr) || (size == 0))
		// Выводим признак отказа приёма
		return -1;
	// Снятая запись устройства
	entry_t entry;
	{
		// Выполняем блокировку реестра заведённых устройств
		const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск устройства в реестре
		auto i = ::__awh_registry__.find(sock);
		// Если устройство в реестре не значится
		if(i == ::__awh_registry__.end())
			// Выводим признак отказа приёма
			return -1;
		// Запоминаем драйвер, каким устройство заведено
		entry.driver = i->second.driver;
		// Запоминаем описатель сеанса обмена
		entry.session = i->second.session;
		// Запоминаем дескриптор устройства
		entry.handle = i->second.handle;
	}
	// Если устройство заведено драйвером Wintun
	if(entry.driver == driver_t::WINTUN){
		// Выполняем подключение библиотеки драйвера
		const wintun_t * wintun = ::__awh_wintun__(nullptr);
		// Если библиотека драйвера не подключена
		if(wintun == nullptr)
			// Выводим признак отказа приёма
			return -1;
		// Размер принятого пакета
		DWORD length = 0;
		// Выполняем приём пакета из кольца
		BYTE * packet = wintun->receive(entry.session, &length);
		// Если пакетов в кольце не осталось
		if(packet == nullptr)
			// Выводим отсутствие принятых пакетов либо признак отказа приёма
			return (::GetLastError() == ERROR_NO_MORE_ITEMS ? 0 : -1);
		// Определяем размер переносимых данных
		const size_t count = (static_cast <size_t> (length) < size ? static_cast <size_t> (length) : size);
		// Выполняем перенос принятого пакета в буфер приёма
		::memcpy(buffer, packet, count);
		// Выполняем возврат места в кольцо
		wintun->release(entry.session, packet);
		// Выводим размер принятого пакета
		return static_cast <int64_t> (count);
	}
	/**
	 * Приём у tap-windows6 снимается с упреждающего, а не подаётся заново
	 *
	 * @details Приём подан прежде прихода пакета, и здесь остаётся лишь спросить,
	 *          завершился ли он. Спрашивается это БЕЗ ожидания: место вызова -
	 *          цикл разбора событий, и остановить его на приходе пакета нельзя
	 *
	 * @warning Прежде здесь подавался свой приём с ожиданием `TRUE`, и вызов
	 *          останавливался навсегда, покуда пакет не придёт. В цикле разбора
	 *          событий это останавливало и все прочие узлы вместе с ним
	 *
	 * @note Запись берётся под замком целиком, а не снимается копией: признак
	 *       поданности приёма живёт в реестре, и копия его теряет
	 *
	 */
	const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем поиск устройства в реестре
	auto i = ::__awh_registry__.find(sock);
	// Если устройство в реестре не значится
	if(i == ::__awh_registry__.end())
		// Выводим признак отказа приёма
		return -1;
	// Если упреждающий приём не подан
	if(!i->second.pending && !::__awh_arm__(i->second))
		// Выводим признак отказа приёма
		return -1;
	// Размер принятого пакета
	DWORD length = 0;
	// Если упреждающий приём ещё не завершён
	if(!::GetOverlappedResult(i->second.handle, &i->second.overlapped, &length, FALSE)){
		// Если приём попросту не завершён, ждём его дальше
		if(::GetLastError() == ERROR_IO_INCOMPLETE)
			// Выводим отсутствие принятых пакетов
			return 0;
		/**
		 * Снимаем признак поданности отказавшего приёма
		 *
		 * @warning Приём этот завершился, пусть и отказом, и поданным он больше не
		 *          числится у системы. Оставленный же признак поданности не давал
		 *          подать следующий НИКОГДА: подача отсеивается им и здесь, и у
		 *          выдачи события готовности, а событие без поданного приёма не
		 *          взводится вовсе. То есть один-единственный отказ приёма - хоть
		 *          отмена обмена, хоть `ERROR_MORE_DATA` от короткого буфера -
		 *          заставлял устройство замолчать навсегда, и выдавала себя беда
		 *          лишь тем, что туннель переставал принимать
		 *
		 * @note Следующий приём здесь НЕ подаётся: отказ вправе оказаться и
		 *       окончательным - устройство закрыто, - и подача по кругу из него
		 *       мостила бы отказы один за другим. Подаст его следующий заход,
		 *       ветвью неподанного приёма выше
		 *
		 */
		i->second.pending = false;
		// Выводим признак отказа приёма
		return -1;
	}
	// Снимаем признак поданности упреждающего приёма
	i->second.pending = false;
	// Определяем размер переносимых данных
	const size_t count = (static_cast <size_t> (length) < size ? static_cast <size_t> (length) : size);
	// Выполняем перенос принятого пакета в буфер приёма
	::memcpy(buffer, i->second.buffer.data(), count);
	// Выполняем подачу следующего упреждающего приёма
	::__awh_arm__(i->second);
	// Выводим размер принятого пакета
	return static_cast <int64_t> (count);
}
/**
 * @brief Функция отправки пакета в туннельное устройство
 *
 * @param sock   дескриптор туннельного устройства
 * @param buffer буфер отправляемого пакета
 * @param size   размер отправляемого пакета
 * @return       размер отправленного пакета, -1 при отказе
 *
 */
int64_t awh::win::tunnel::write(const net::socket_t sock, const void * buffer, const size_t size) noexcept {
	// Если буфер отправки не передан
	if((buffer == nullptr) || (size == 0))
		// Выводим признак отказа отправки
		return -1;
	// Снятая запись устройства
	entry_t entry;
	{
		// Выполняем блокировку реестра заведённых устройств
		const awh::locker_t <std::shared_mutex> lock(::__awh_mutex__, awh::locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск устройства в реестре
		auto i = ::__awh_registry__.find(sock);
		// Если устройство в реестре не значится
		if(i == ::__awh_registry__.end())
			// Выводим признак отказа отправки
			return -1;
		// Запоминаем драйвер, каким устройство заведено
		entry.driver = i->second.driver;
		// Запоминаем описатель сеанса обмена
		entry.session = i->second.session;
		// Запоминаем дескриптор устройства
		entry.handle = i->second.handle;
	}
	// Если устройство заведено драйвером Wintun
	if(entry.driver == driver_t::WINTUN){
		// Выполняем подключение библиотеки драйвера
		const wintun_t * wintun = ::__awh_wintun__(nullptr);
		// Если библиотека драйвера не подключена
		if(wintun == nullptr)
			// Выводим признак отказа отправки
			return -1;
		// Выполняем отведение места под отправку
		BYTE * packet = wintun->allocate(entry.session, static_cast <DWORD> (size));
		// Если места под отправку не осталось
		if(packet == nullptr)
			// Выводим признак отказа отправки
			return -1;
		// Выполняем перенос отправляемого пакета в кольцо
		::memcpy(packet, buffer, size);
		// Выполняем отправку пакета
		wintun->send(entry.session, packet);
		// Выводим размер отправленного пакета
		return static_cast <int64_t> (size);
	}
	// Описатель перекрытого обмена
	OVERLAPPED overlapped{};
	// Выполняем заведение события завершения обмена
	overlapped.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
	// Если завести событие завершения обмена не удалось
	if(overlapped.hEvent == nullptr)
		// Выводим признак отказа отправки
		return -1;
	// Размер отправленного пакета
	DWORD length = 0;
	// Результат выполнения отправки
	int64_t result = -1;
	// Если отправка пакета выполнена либо начата
	if(::WriteFile(entry.handle, buffer, static_cast <DWORD> (size), &length, &overlapped) || (::GetLastError() == ERROR_IO_PENDING)){
		// Если отправка пакета завершена
		if(::GetOverlappedResult(entry.handle, &overlapped, &length, TRUE))
			// Запоминаем размер отправленного пакета
			result = static_cast <int64_t> (length);
	}
	// Выполняем закрытие события завершения обмена
	::CloseHandle(overlapped.hEvent);
	// Выводим результат выполнения отправки
	return result;
}
