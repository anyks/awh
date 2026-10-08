/**
 * @brief Проверка владения записью события в настоящем пуле волокон IOCP
 *
 * @details Backend включён напрямую; проверки действуют при NDEBUG.
 *
 */
/**
 * Подключаем заголовки автономной проверки
 */
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
/**
 * Если путь к проверяемому исходнику не задан
 */
#if !defined(AWH_IOCP_SOURCE)
	/**
	 * @brief Подключаем текущую реализацию движка
	 *
	 */
	#define AWH_IOCP_SOURCE "src/net/backend/win/iocp.cpp"
#endif
/**
 * Подключаем настоящий backend вместо копии его логики
 */
#include AWH_IOCP_SOURCE

/**
 * @brief Инкапсулируем статические локальные функции в пространство имён
 *
 */
namespace {
	/**
	 * @brief Функция проверки условия с безусловным завершением при отказе
	 *
	 * @param value   проверяемое условие
	 * @param message название проверки
	 *
	 */
	static void require(const bool value, const char * message) noexcept {
		// Если условие нарушено
		if(!value){
			// Сообщаем причину отказа
			::fprintf(stderr, "WORKER_PROBE_FAIL: %s\n", message);
			::fflush(stderr);
			// Завершаем проверку независимо от NDEBUG
			::_Exit(EXIT_FAILURE);
		}
	}
	/**
	 * @brief Функция создания различимой записи события
	 *
	 * @param index номер записи
	 * @param owner владелец записи
	 * @return      заполненная запись
	 *
	 */
	static ::change::record_t record(const size_t index, void * owner) noexcept {
		// Создаём запись со всеми различимыми полями
		return ::change::make(1000 + index, ::change::filter_t::READ, static_cast <uint16_t> (index + 1), static_cast <uint32_t> (index * 17 + 3), static_cast <int64_t> (index * 131 + 7), owner);
	}
	/**
	 * @brief Функция сравнения всех полей записи без чтения padding
	 *
	 * @param first  первая запись
	 * @param second вторая запись
	 * @return       признак совпадения
	 *
	 */
	static bool equal(const ::change::record_t & first, const ::change::record_t & second) noexcept {
		// Сравниваем все поля записи
		return ((first.ident == second.ident) && (first.filter == second.filter) && (first.flags == second.flags) && (first.fflags == second.fflags) && (first.data == second.data) && (first.udata == second.udata));
	}
	/**
	 * @brief Функция проверки копии через уступку управления и повторное использование пачки
	 *
	 */
	static void lifetime() noexcept {
		// Состояние ожидающего обработчика
		size_t stage = 0;
		awh::fiber::ctx_t * held = nullptr;
		// Исходная и ожидаемая записи
		::change::record_t source = ::record(5, &stage);
		const ::change::record_t expected = source;
		// Запускаем обработчик, который уснёт посреди разбора
		::require(::fibers::run([&](::change::record_t & value) noexcept {
			// Проверяем первоначальную копию
			::require(::equal(value, expected), "initial copy");
			held = awh::fiber::current();
			stage++;
			// Возвращаем управление циклу до завершения обработки
			awh::fiber::yield();
			// Проверяем запись после чужих событий
			::require(::equal(value, expected), "record survives yield and overwrite");
			stage++;
		}, source), "lifetime dispatch");
		::require((stage == 1) && (held != nullptr), "first suspension");
		::require(::fibers::pool.front()->busy, "suspended worker remains busy");
		// Перезаписываем исходную пачку и многократно используем другой worker
		for(size_t index = 0; index < 128; index++){
			source = ::record(index + 100, &source);
			::require(::fibers::run([&](::change::record_t & value) noexcept {
				// Проверяем отсутствие смешивания с приостановленным событием
				::require(::equal(value, source), "independent dispatch");
			}, source), "independent worker");
		}
		// Завершаем первоначальную работу ровно один раз
		::require(awh::fiber::resume(held), "lifetime resume");
		::require(stage == 2, "lifetime completion once");
		::fibers::clear();
		::require(::fibers::pool.empty(), "lifetime pool cleanup");
	}
	/**
	 * @brief Функция проверки исчерпания пула и прямого разбора таймера
	 *
	 * @param engine проверяемый движок
	 *
	 */
	static void exhaustion(awh::engine::io_t & engine) noexcept {
		// Контексты и число входов каждого обработчика
		std::array <awh::fiber::ctx_t *, 64> contexts{};
		std::array <uint8_t, 64> stages{};
		// Занимаем весь пул работами, уступающими управление
		for(size_t index = 0; index < contexts.size(); index++){
			::change::record_t source = ::record(index, &stages[index]);
			::require(::fibers::run([&](::change::record_t & value) noexcept {
				// Сохраняем номер записи в кадре волокна до уступки
				const size_t number = (value.ident - 1000);
				::require(number < contexts.size(), "worker record index");
				const ::change::record_t expected = ::record(number, &stages[number]);
				::require(::equal(value, expected), "worker record before yield");
				contexts[number] = awh::fiber::current();
				stages[number]++;
				awh::fiber::yield();
				// Проверяем независимую копию после заполнения всего пула
				::require(::equal(value, expected), "worker record after saturation");
				stages[number]++;
			}, source), "occupy worker");
			::require(stages[index] == 1, "worker entered once");
			// Исходная запись больше не принадлежит callback
			source = {};
		}
		::require(::fibers::pool.size() == contexts.size(), "64 occupied workers");
		// 65-я работа не должна исполняться или портить спящие записи
		uint8_t rejected = 0;
		::require(!::fibers::run([&](::change::record_t &) noexcept {
			// Отмечаем ошибочное исполнение отказавшей работы
			rejected++;
		}), "65th dispatch refused");
		::require(rejected == 0, "refused callback not called");
		// Проверяем настоящий fallback таймера внутри IO::poll при занятом пуле
		uint8_t fired = 0;
		bool outside = false;
		const auto interval = engine.event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
		::require(interval > 0, "fallback interval");
		engine.setTimeout(interval, awh::event::action_t::NONE, 10);
		::require(engine.commit(interval), "fallback commit");
		engine.on(interval, [&](const awh::event::id_t, const awh::event::status_t status) noexcept {
			// Учитываем только успешную доставку таймера
			if(status == awh::event::status_t::SUCCESS){
				fired++;
				outside = (awh::fiber::current() == nullptr);
			}
		});
		::require(engine.launch(interval), "fallback launch");
		const auto deadline = (std::chrono::steady_clock::now() + std::chrono::seconds(5));
		while((fired == 0) && (std::chrono::steady_clock::now() < deadline))
			::require(engine.poll(1), "fallback poll");
		::require((fired == 1) && outside, "actual timer fallback once outside fiber");
		::require(engine.destroy(interval), "fallback destroy");
		// Очистка не должна сносить ни одного занятого worker
		::fibers::clear();
		::require(::fibers::pool.size() == contexts.size(), "clear preserves suspended workers");
		// Пробуждаем в обратном порядке и проверяем однократность завершения
		for(size_t index = contexts.size(); index > 0; index--){
			::require(awh::fiber::resume(contexts[index - 1]), "saturated resume");
			::require(stages[index - 1] == 2, "saturated completion once");
		}
		::fibers::clear();
		::require(::fibers::pool.empty(), "saturation pool cleanup");
	}
	/**
	 * @brief Функция проверки разбора движка из работающего callback
	 *
	 * @param engine проверяемый движок
	 *
	 */
	static void teardown(awh::engine::io_t & engine) noexcept {
		// Контекст, состояние и запись обработчика
		awh::fiber::ctx_t * held = nullptr;
		uint8_t stage = 0;
		::change::record_t source = ::record(77, &stage);
		const ::change::record_t expected = source;
		::require(::fibers::run([&](::change::record_t & value) noexcept {
			held = awh::fiber::current();
			::require(held != nullptr, "teardown inside fiber");
			// Разбираем движок с текущего рабочего стека
			::require(engine.deinitialize(), "deinitialize from callback");
			::require((::fibers::pool.size() == 1) && ::fibers::pool.front()->busy, "current worker survives deinitialize");
			stage++;
			awh::fiber::yield();
			::require(::equal(value, expected), "record survives deinitialize and yield");
			stage++;
		}, source), "teardown dispatch");
		::require(stage == 1, "teardown suspension");
		source = {};
		::require(awh::fiber::resume(held), "teardown resume");
		::require(stage == 2, "teardown completion once");
		::fibers::clear();
		::require(::fibers::pool.empty(), "teardown final cleanup");
		// Повторное заведение и опрос не должны доставлять старые callbacks
		::require(engine.initialize(), "reinitialize");
		for(uint8_t index = 0; index < 16; index++)
			::require(engine.poll(0), "poll after reinitialize");
		::require(stage == 2, "no late callback");
		::require(engine.deinitialize(), "final deinitialize");
	}
};

/**
 * @brief Точка входа автономной проверки
 *
 * @return результат выполнения проверки
 *
 */
int main() {
	// Инициализируем память и настоящий движок
	awh::fmk::initialize();
	awh::engine::io_t engine;
	::require(engine.initialize(), "initialize");
	::lifetime();
	::exhaustion(engine);
	::teardown(engine);
	::fprintf(stdout, "WORKER_PROBE_OK: copy, yield, overwrite, 64 workers, actual timer fallback, deinitialize, reinitialize\n");
	return EXIT_SUCCESS;
}
