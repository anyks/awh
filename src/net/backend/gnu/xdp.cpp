/**
 * @file xdp.cpp
 * @date 2026-11-06
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
 * @brief Реализация низкого уровня датапути AF_XDP — пространство имён eth::xdp: заведение сокетов
 *        AF_XDP с областью UMEM и кольцами, приём и отправка кадров канального уровня в обход
 *        обычного сетевого стека, обобщённый режим generic SKB первым
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <exception>
#include <algorithm>

/**
 * Системные заголовочные файлы
 */
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <linux/if_xdp.h>
#include <linux/if_link.h>

/**
 * Заголовочные файлы libbpf/libxdp
 */
#include <xdp/xsk.h>
#include <bpf/libbpf.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <net/eth/xdp.hpp>

/**
 * Спутник датапути AF_XDP включается сборкой (-DCMAKE_BUILD_XDP=ON), умолчанием выключен
 */
#if defined(AWH_XDP)
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Основное пространство имён
	 *
	 */
	namespace awh {
		/**
		 * @brief Пространство имён сетевых сущностей
		 *
		 */
		namespace net {
			/**
			 * @brief Пространство имён датапути AF_XDP
			 *
			 */
			namespace xdp {
				/**
				 * @brief Структура состояния одного сокета AF_XDP
				 *
				 * @details Указатели libxdp на сокет и UMEM, область кадров, четыре кольца
				 *          (приём, отправка, наполнение, завершение) и пул свободных кадров
				 *          принадлежат одному сокету. Состояние непрозрачно для заголовков и
				 *          узлов движка: полное определение живёт лишь здесь, чтобы заголовки
				 *          libxdp не растекались по прочим единицам трансляции.
				 *
				 */
				typedef struct Socket {
					// Размер области кадров в байтах
					uint64_t size = 0;
					// Кольцо приёма
					struct xsk_ring_cons rx;
					// Кольцо отправки
					struct xsk_ring_prod tx;
					// Кольцо наполнения
					struct xsk_ring_prod fill;
					// Кольцо завершения
					struct xsk_ring_cons comp;
					// Пул свободных кадров (смещения в UMEM)
					vector <uint64_t> freeFrames;
					// Выделенная область кадров памяти
					void * area;
					// Область UMEM глазами libxdp
					struct xsk_umem * umem;
					// Сокет AF_XDP глазами libxdp
					struct xsk_socket * xsk;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Socket() noexcept :
					 size(0), rx{}, tx{}, fill{}, comp{},
					 area(nullptr), umem(nullptr), xsk(nullptr) {}
				} socket_t;
			};
		};
	};

	/**
	 * @brief Пространство имён внутренних помощников датапути
	 *
	 * @details Помощники колец видны одной лишь единице трансляции спутника; состояние
	 *          сокета принадлежит узлу движка, потому своего реестра и мьютекса здесь нет.
	 *
	 */
	namespace {
		/**
		 * @brief Число кадров области UMEM (вдвое больше кольца, чтобы кадры оставались и на отправку)
		 *
		 */
		constexpr uint32_t FRAME_COUNT = 4096;
		/**
		 * @brief Размер одного кадра памяти UMEM
		 *
		 */
		constexpr uint32_t FRAME_SIZE = XSK_UMEM__DEFAULT_FRAME_SIZE;
		/**
		 * @brief Число дескрипторов каждого кольца
		 *
		 */
		constexpr uint32_t RING_SIZE = XSK_RING_CONS__DEFAULT_NUM_DESCS;

		/**
		 * @brief Функция наполнения кольца FILL свободными кадрами
		 *
		 * @details Отдаёт ядру свободные кадры, чтобы оно могло класть в них принятые
		 *          датаграммы; берёт не больше, чем есть в пуле и вмещает кольцо.
		 *
		 * @param sock состояние сокета
		 *
		 */
		static __AWH_INLINE__ void refillFill(net::xdp::socket_t & sock) noexcept {
			// Сколько кадров можно отдать кольцу наполнения
			const uint32_t want = std::min <uint32_t> (RING_SIZE, static_cast <uint32_t> (sock.freeFrames.size()));
			// Если отдавать нечего
			if(want == 0)
				// Выходим из функции
				return;
			// Индекс первой зарезервированной ячейки кольца
			uint32_t idx = 0;
			// Резервируем ячейки кольца наполнения
			const uint32_t got = ::xsk_ring_prod__reserve(&sock.fill, want, &idx);
			/**
			 * Раскладываем свободные кадры по зарезервированным ячейкам
			 */
			for(uint32_t i = 0; i < got; i++){
				// Кладём смещение свободного кадра в ячейку наполнения
				*::xsk_ring_prod__fill_addr(&sock.fill, (idx + i)) = sock.freeFrames.back();
				// Снимаем кадр с пула свободных
				sock.freeFrames.pop_back();
			}
			// Подтверждаем наполнение кольца
			::xsk_ring_prod__submit(&sock.fill, got);
		}
		/**
		 * @brief Функция жатвы кольца завершения
		 *
		 * @details Возвращает в пул свободных кадры, отправку которых ядро завершило.
		 *
		 * @param sock состояние сокета
		 *
		 */
		static __AWH_INLINE__ void reapComp(net::xdp::socket_t & sock) noexcept {
			// Индекс первой готовой ячейки кольца завершения
			uint32_t idx = 0;
			// Снимаем число завершённых кадров
			const uint32_t got = ::xsk_ring_cons__peek(&sock.comp, RING_SIZE, &idx);
			/**
			 * Возвращаем завершённые кадры в пул свободных
			 */
			for(uint32_t i = 0; i < got; i++)
				// Кладём смещение завершённого кадра в пул
				sock.freeFrames.push_back(*::xsk_ring_cons__comp_addr(&sock.comp, (idx + i)));
			// Если завершённые кадры были
			if(got > 0)
				// Освобождаем ячейки кольца завершения
				::xsk_ring_cons__release(&sock.comp, got);
		}
		/**
		 * @brief Функция освобождения ресурсов состояния сокета
		 *
		 * @details Освобождает сокет, кольца, область UMEM и область кадров памяти в
		 *          обратном порядке заведения; вызывается и при отказе заведения, и при
		 *          уничтожении, потому сосредоточена в одном месте.
		 *
		 * @param sock состояние сокета
		 *
		 */
		static __AWH_INLINE__ void release(net::xdp::socket_t & sock) noexcept {
			// Если сокет AF_XDP заведён
			if(sock.xsk != nullptr)
				// Выполняем удаление сокета
				::xsk_socket__delete(sock.xsk);
			// Если область UMEM заведена
			if(sock.umem != nullptr)
				// Выполняем удаление области UMEM
				::xsk_umem__delete(sock.umem);
			// Если область кадров памяти выделена
			if(sock.area != nullptr)
				// Выполняем освобождение области
				::munmap(sock.area, sock.size);
		}
	};

	/**
	 * @brief Функция проверки поддержки AF_XDP ядром
	 *
	 * @details Пробует завести сокет семейства AF_XDP: успех означает, что ядро несёт
	 *          этот датапуть. Отказ по праву (EPERM/EACCES) тоже означает поддержку:
	 *          семейство ядром ОПОЗНАНО, но вызывающему не хватает привилегии
	 *          CAP_NET_ADMIN - заведение сокета AF_XDP требует её, как и прочих сырых
	 *          сокетов. Лишь отказ EAFNOSUPPORT/EPROTONOSUPPORT значит, что ядро не
	 *          несёт AF_XDP вовсе. Проба не требует ни привязки к устройству, ни UMEM.
	 *
	 * @return признак поддержки AF_XDP ядром
	 *
	 */
	bool awh::eth::xdp::supported() noexcept {
		// Пробуем завести пробный сокет семейства AF_XDP
		const int32_t fd = ::socket(AF_XDP, SOCK_RAW, 0);
		// Если сокет заведён, ядро несёт датапуть AF_XDP
		if(fd >= 0){
			// Закрываем пробный сокет
			::close(fd);
			// Возвращаем положительный результат поддержки
			return true;
		}
		// Если отказ по праву, семейство ядром опознано и датапуть есть
		if((errno == EPERM) || (errno == EACCES))
			// Возвращаем положительный результат поддержки
			return true;
		// Возвращаем отрицательный результат поддержки
		return false;
	}
	/**
	 * @brief Функция получения описателя сокета AF_XDP
	 *
	 * @details Описатель пригоден для опроса готовности (poll/epoll): по нему движок
	 *          узнаёт о приходе кадров, не обращаясь к кольцам на всякой итерации.
	 *          Описатель внутренний, уровня eth, и публичному фасаду движка не выдаётся.
	 *
	 * @param state состояние сокета
	 * @return      описатель сокета, либо неверный сокет при отказе
	 *
	 */
	net::socket_t awh::eth::xdp::fd(const net::xdp::socket_t * state) noexcept {
		// Если состояние не подано
		if(state == nullptr)
			// Возвращаем неверный сокет
			return net::invalid_socket_t;
		// Возвращаем описатель сокета AF_XDP
		return static_cast <net::socket_t> (::xsk_socket__fd(state->xsk));
	}
	/**
	 * @brief Функция уничтожения сокета AF_XDP
	 *
	 * @details Освобождает сокет, кольца и область UMEM, заведённые create(), и освобождает
	 *          само состояние.
	 *
	 * @param state состояние сокета
	 * @return      признак успешного уничтожения
	 *
	 */
	bool awh::eth::xdp::destroy(net::xdp::socket_t * state) noexcept {
		// Если состояние не подано
		if(state == nullptr)
			// Возвращаем отказ уничтожения
			return false;
		// Освобождаем ресурсы сокета
		release(* state);
		// Освобождаем само состояние
		delete state;
		// Возвращаем успешный результат уничтожения
		return true;
	}
	/**
	 * @brief Функция приёма одной датаграммы из кольца RX
	 *
	 * @details Снимает с кольца приёма ровно один готовый кадр, копирует его датаграмму
	 *          в поданный буфер, возвращает кадр в пул свободных и дочитывает кольцо
	 *          наполнения. Прочие готовые кадры остаются в кольце: движок дозовёт их
	 *          следующими вызовами, потому датаграммы не теряются.
	 *
	 * @param state  состояние сокета
	 * @param buffer буфер под принятую датаграмму
	 * @param size   размер буфера в байтах
	 * @return       число принятых байт, ноль если датаграмм нет, минус единица при отказе
	 *
	 */
	int32_t awh::eth::xdp::receive(net::xdp::socket_t * state, uint8_t * buffer, const uint32_t size) noexcept {
		// Если состояние не подано
		if(state == nullptr)
			// Возвращаем отказ приёма
			return -1;
		// Индекс первой готовой ячейки кольца приёма
		uint32_t idx = 0;
		// Снимаем готовность ровно одного кадра приёма
		const uint32_t got = ::xsk_ring_cons__peek(&state->rx, 1, &idx);
		// Если готовых кадров нет
		if(got == 0)
			// Возвращаем отсутствие датаграмм
			return 0;
		// Дескриптор принятого кадра
		const struct xdp_desc * desc = ::xsk_ring_cons__rx_desc(&state->rx, idx);
		// Смещение кадра в области UMEM
		const uint64_t addr = desc->addr;
		// Число копируемых байт (не больше буфера)
		const uint32_t n = std::min <uint32_t> (desc->len, size);
		// Указатель на данные кадра
		void * data = ::xsk_umem__get_data(state->area, addr);
		// Копируем датаграмму в буфер
		::memcpy(buffer, data, n);
		// Возвращаем кадр в пул свободных
		state->freeFrames.push_back(addr);
		// Освобождаем ячейку кольца приёма
		::xsk_ring_cons__release(&state->rx, 1);
		// Дочитываем кольцо наполнения свободными кадрами
		refillFill(* state);
		// Возвращаем число принятых байт
		return static_cast <int32_t> (n);
	}
	/**
	 * @brief Функция отправки одной датаграммы через кольцо TX
	 *
	 * @details Берёт свободный кадр, копирует в него датаграмму, кладёт дескриптор в
	 *          кольцо отправки и побуждает ядро к отправке.
	 *
	 * @param state состояние сокета
	 * @param data  данные датаграммы
	 * @param size  размер датаграммы в байтах
	 * @return      признак успешной отправки
	 *
	 */
	bool awh::eth::xdp::send(net::xdp::socket_t * state, const uint8_t * data, const uint32_t size) noexcept {
		// Если состояние не подано
		if(state == nullptr)
			// Возвращаем отказ отправки
			return false;
		// Датаграмма больше кадра не помещается
		if(size > FRAME_SIZE)
			// Возвращаем отказ отправки
			return false;
		// Возвращаем в пул кадры завершённых отправок
		reapComp(* state);
		// Если свободных кадров нет
		if(state->freeFrames.empty())
			// Возвращаем отказ отправки
			return false;
		// Индекс зарезервированной ячейки кольца отправки
		uint32_t idx = 0;
		// Резервируем одну ячейку кольца отправки
		if(::xsk_ring_prod__reserve(&state->tx, 1, &idx) != 1)
			// Возвращаем отказ отправки
			return false;
		// Берём свободный кадр из пула
		const uint64_t addr = state->freeFrames.back();
		// Снимаем кадр с пула свободных
		state->freeFrames.pop_back();
		// Дескриптор отправляемого кадра
		struct xdp_desc * desc = ::xsk_ring_prod__tx_desc(&state->tx, idx);
		// Смещение кадра в области UMEM
		desc->addr = addr;
		// Длина датаграммы
		desc->len = size;
		// Копируем датаграмму в кадр
		::memcpy(::xsk_umem__get_data(state->area, addr), data, size);
		// Подтверждаем отправку кадра
		::xsk_ring_prod__submit(&state->tx, 1);
		// Побуждаем ядро к отправке (описатель сокета AF_XDP)
		::sendto(::xsk_socket__fd(state->xsk), nullptr, 0, MSG_DONTWAIT, nullptr, 0);
		// Возвращаем успешный результат отправки
		return true;
	}
	/**
	 * @brief Функция заведения сокета AF_XDP
	 *
	 * @details Выделяет область UMEM, заводит кольца и привязывает сокет к устройству
	 *          и очереди. Умолчанием программа BPF не крепится (признак inhibit):
	 *          заведение не перенаправляет трафик и безопасно; признак redirect крепит
	 *          программу перенаправления. Режим обобщённый (generic SKB).
	 *
	 * @param iface    название сетевого устройства
	 * @param queue    номер очереди устройства
	 * @param redirect крепить ли программу BPF-редиректа трафика в сокет
	 * @return         состояние сокета, либо пустой указатель при отказе
	 *
	 */
	net::xdp::socket_t * awh::eth::xdp::create(const string & iface, const uint32_t queue, const bool redirect) noexcept {
		// Состояние заводимого сокета
		net::xdp::socket_t * state = nullptr;
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выделяем состояние сокета
			state = new net::xdp::socket_t();
			// Резервируем пул свободных кадров на весь объём области
			state->freeFrames.reserve(FRAME_COUNT);
			// Размер области кадров памяти
			state->size = static_cast <uint64_t> (FRAME_SIZE) * FRAME_COUNT;
			// Выделяем область кадров памяти
			state->area = ::mmap(nullptr, state->size, (PROT_READ | PROT_WRITE), (MAP_PRIVATE | MAP_ANONYMOUS), -1, 0);
			// Если область выделить не удалось
			if(state->area == MAP_FAILED){
				// Освобождаем состояние при отказе
				delete state;
				// Возвращаем отказ заведения
				return nullptr;
			}
			// Настройки области UMEM
			struct xsk_umem_config ucfg{};
			// Размер кольца наполнения
			ucfg.fill_size = RING_SIZE;
			// Размер кольца завершения
			ucfg.comp_size = RING_SIZE;
			// Размер кадра памяти
			ucfg.frame_size = FRAME_SIZE;
			// Запас перед кадром
			ucfg.frame_headroom = 0;
			// Признаки области UMEM
			ucfg.flags = 0;
			// Заводим область UMEM и кольца наполнения/завершения
			if(::xsk_umem__create(&state->umem, state->area, state->size, &state->fill, &state->comp, &ucfg) != 0){
				// Освобождаем ресурсы состояния при отказе
				release(* state);
				// Освобождаем само состояние
				delete state;
				// Возвращаем отказ заведения
				return nullptr;
			}
			// Настройки сокета AF_XDP
			struct xsk_socket_config scfg{};
			// Размер кольца приёма
			scfg.rx_size = RING_SIZE;
			// Размер кольца отправки
			scfg.tx_size = RING_SIZE;
			// Крепить программу BPF лишь по признаку redirect, иначе inhibit
			scfg.libxdp_flags = (redirect ? 0 : XSK_LIBXDP_FLAGS__INHIBIT_PROG_LOAD);
			// Обобщённый режим generic SKB
			scfg.xdp_flags = XDP_FLAGS_SKB_MODE;
			// Копирующий режим привязки без нулевого копирования
			scfg.bind_flags = XDP_COPY;
			// Заводим сокет AF_XDP и кольца приёма/отправки
			if(::xsk_socket__create(&state->xsk, iface.c_str(), queue, state->umem, &state->rx, &state->tx, &scfg) != 0){
				// Освобождаем ресурсы состояния при отказе
				release(* state);
				// Освобождаем само состояние
				delete state;
				// Возвращаем отказ заведения
				return nullptr;
			}
			/**
			 * Кладём все кадры области в пул свободных
			 */
			for(uint32_t i = 0; i < FRAME_COUNT; i++)
				// Смещение кадра в области UMEM
				state->freeFrames.push_back(static_cast <uint64_t> (i) * FRAME_SIZE);
			// Наполняем кольцо FILL, чтобы ядро могло класть принятые кадры
			refillFill(* state);
			// Возвращаем состояние заведённого сокета
			return state;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если состояние успело выделиться, освобождаем его
			if(state != nullptr){
				// Освобождаем ресурсы состояния при отказе
				release(* state);
				// Освобождаем само состояние
				delete state;
			}
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {iface, queue, redirect}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
		// Возвращаем отказ заведения
		return nullptr;
	}
#endif
