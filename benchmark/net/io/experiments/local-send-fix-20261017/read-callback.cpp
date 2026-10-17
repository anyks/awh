/**
 * @file read-callback.cpp
 * @date 2026-10-17
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
 * @brief Реализация проверки отправки из настоящего обработчика чтения IOCP
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Переименовываем точку входа сохранённой оснастки для повторного использования
 */
#define main preservedMain
// Подключаем оснастку с настоящим движком и собственным TCP-собеседником
#include "local-send-probe.cpp"
// Восстанавливаем имя единственной точки входа процесса проверки
#undef main

/**
 * @brief Пространство проверки допуска отправки между завершением READ и перевзведением
 *
 */
namespace {
	/**
	 * @brief Метод проверки принадлежности живому событию без чтения чужой памяти
	 *
	 * @param fixture собственное соединение проверки
	 *
	 */
	static void checkOwner(fixture_t & fixture) noexcept {
		// Получаем существующую запись собственного сокета
		auto & state = ::kernel::registry.at(fixture.sock);
		// Сохраняем идентификатор текущего допуска
		const awh::event::id_t id = state.localOwner;
		// Проверяем отсутствие допуска у нулевого идентификатора
		state.localOwner = 0;
		// Нулевой идентификатор не разрешает обычную отправку
		::require(!::kernel::localSendable(fixture.sock, &state), "zero-owner-rejected");
		// Подставляем живого слушателя с другим адресом узла
		state.localOwner = fixture.server;
		// Чужой идентификатор не разрешает отправку из этой записи
		::require(!::kernel::localSendable(fixture.sock, &state), "different-owner-rejected");
		// Восстанавливаем исходный идентификатор
		state.localOwner = id;
		// Сохраняем непрозрачный адрес владельца
		void * const owner = state.udata;
		// Подставляем адрес, который нельзя разыменовывать
		state.udata = reinterpret_cast <void *> (static_cast <uintptr_t> (1));
		// Адрес сравнивается с живым узлом без чтения памяти по этому адресу
		::require(!::kernel::localSendable(fixture.sock, &state), "stale-pointer-rejected");
		// Восстанавливаем точного владельца
		state.udata = owner;
		// Находим живого владельца в таблице событий
		const auto entry = ::__awh_nodes__.find(id);
		// Подтверждаем наличие узла до обращения к его состоянию
		::require(entry != ::__awh_nodes__.end(), "owner-node-present");
		// Получаем живой узел из найденной записи
		auto * node = entry->second.get();
		// Сохраняем действующий статус узла
		const auto status = node->state.status;
		// Проверяем запрет отправки после начала уничтожения
		node->state.status = awh::event::status_t::DESTROYED;
		// Закрываемый узел не сохраняет право локальной отправки
		::require(!::kernel::localSendable(fixture.sock, &state), "destroyed-owner-rejected");
		// Проверяем узел, ожидающий освобождения памяти
		node->state.status = awh::event::status_t::GARBAGE;
		// Отложенное освобождение не продлевает допуск
		::require(!::kernel::localSendable(fixture.sock, &state), "garbage-owner-rejected");
		// Восстанавливаем статус до дальнейшей работы движка
		node->state.status = status;
		// Сохраняем действующий порт подписки
		const HANDLE attached = state.attached;
		// Снимаем привязку только для проверки условия допуска
		state.attached = nullptr;
		// Отсутствие привязки исключает локальную отправку
		::require(!::kernel::localSendable(fixture.sock, &state), "detached-owner-rejected");
		// Восстанавливаем привязку перед настоящей отправкой
		state.attached = attached;
		// Все временные нарушения устранены, исходный допуск вновь действует
		::require(::kernel::localSendable(fixture.sock, &state), "restored-owner-enabled");
	}
	/**
	 * @brief Метод проверки отправки из настоящего обработчика чтения
	 *
	 * @param mode сценарий доставки, отмены, уничтожения либо смены поколения
	 *
	 */
	static void fromRead(const std::string & mode) noexcept {
		// Создаём собственное соединение с явно разрешённой локальной отправкой
		fixture_t fixture;
		// Дожидаемся настоящего ожидания чтения в IOCP
		fixture.open();
		// Сохраняем число вызовов настоящего обработчика чтения
		size_t reads = 0;
		// Сохраняем старую и новую метки для проверки времени жизни слота
		uint64_t token = ::inflight::INVALID, fresh = ::inflight::INVALID;
		// Устанавливаем отправку в обработчик реально принятого байта
		fixture.engine.on(fixture.sender, static_cast <awh::engine::callback::read_t> ([&]([[maybe_unused]] const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept {
			// Учитываем вход в обработчик, не подменяя состояние registered вручную
			reads++;
			// Проверяем единственный переданный собеседником байт
			::require((reads == 1) && (size == 1) && (buffer[0] == 'R'), "actual-read-byte");
			// Получаем запись после снятия одноразового ожидания самим движком
			auto & state = ::kernel::registry.at(fixture.sock);
			// Фиксируем именно окно, в котором прежний критерий запрещал локальный SEND
			::require(!state.registered, "read-must-have-no-active-wait");
			// Проверяем право отправки до повторного взведения чтения
			::require(::kernel::localSendable(fixture.sock, &state), "read-callback-send-eligible");
			// Проверяем независимые ограничения по времени жизни владельца
			::checkOwner(fixture);
			// Передаём узнаваемый поток публичным интерфейсом прямо из READ
			::require(fixture.send(::payload(64)) == 64, "read-send-accepted");
			// Проверяем реальный локальный слот, а не только результат предиката
			token = ::localToken(fixture);
			// Подтверждение положительного продвижения остаётся отложенным
			::require(fixture.written == 0, "read-positive-write-deferred");
			// Отменяем локальный исход до разбора следующей пачки
			if(mode == "read-cancel")
				// Сохраняем живой узел для проверки отменённого исхода
				(void) ::post::cancel(token);
			// Уничтожаем событие через публичный интерфейс внутри READ
			else if(mode == "read-destroy") {
				// Запрещаем оснастке любые последующие WRITE этого события
				fixture.closed = true;
				// Удаляем владельца до доставки локального исхода
				::require(fixture.engine.destroy(fixture.sender), "read-public-destroy");
			// Проверяем старый исход после переиспользования индекса слота
			} else if(mode == "read-generation") {
				// Освобождаем слот, оставляя его прежнюю метку в локальной очереди
				::inflight::release(token);
				// Занимаем тот же индекс с новым поколением
				fresh = ::inflight::acquire(::inflight::kind_t::SEND, fixture.sock, nullptr);
				// Проверяем именно смену поколения одного индекса
				::require((fresh != token) && (static_cast <uint32_t> (fresh) == static_cast <uint32_t> (token)), "read-generation-reused-index");
			// Снимаем допуск закрываемого дескриптора тем же путём, что использует backend
			} else if(mode == "read-forget") {
				// Забываем операции и явный допуск собственного сокета
				(void) ::kernel::forget(fixture.sock);
				// Запись остаётся доступной, но не наследует допуск или идентификатор
				::require(!state.localSend && (state.localOwner == 0) && !::kernel::localSendable(fixture.sock, &state), "forgotten-owner-rejected");
			}
		}));
		// Настоящий сокет передаёт байт, вызывающий READ через IOCP
		::require(::send(fixture.peer, "R", 1, 0) == 1, "trigger-read");
		// Ограничиваем ожидание настоящего чтения
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		/**
		 * Ожидаем обработчик чтения, прокручивая штатный цикл событий
		 */
		while((reads == 0) && (std::chrono::steady_clock::now() < deadline))
			// Получаем настоящие завершения порта
			::require(fixture.engine.poll(1), "read-poll");
		// Отсутствие READ считается ошибкой проверки
		::require(reads == 1, "read-deadline");
		// Обычная доставка проверяется полным совпадением потока и числа WRITE
		if(mode == "read-send")
			// Дожидаемся ровно 64 байт без повторных публичных отправок
			fixture.drain();
		// Остальные сценарии обязаны подавить положительный WRITE
		else {
			/**
			 * Пропускаем достаточно оборотов для разбора отменённого исхода
			 */
			for(uint32_t i = 0; i < 20; i++)
				// Разбираем завершения штатным потребителем
				::require(fixture.engine.poll(1), "read-lifecycle-poll");
			// Отменённый или устаревший исход не становится успешной записью
			::require(fixture.written == 0, "read-cancelled-write-delivered");
		}
		// Прежняя метка должна быть освобождена либо признана устаревшей
		::require(::inflight::get(token) == nullptr, "read-old-slot-released");
		// После смены поколения новая метка не принадлежит старому исходу
		if(fresh != ::inflight::INVALID){
			// Проверяем, что старое завершение не освободило новую операцию
			::require(::inflight::get(fresh) != nullptr, "read-new-generation-survives");
			// Освобождаем созданную только для проверки операцию
			::inflight::release(fresh);
		}
		// Проверяем отсутствие потерянных локальных исходов
		::require(::port::localCount == 0, "read-local-queue-drained");
		// Закрываем собственные события и сокеты
		fixture.finish();
		// Публикуем подтверждённый путь отдельно от измерений производительности
		::fprintf(stdout, "READ_CALLBACK_OK case=%s registered=0 reads=%zu bytes=%zu queued=%llu delivered=%llu cancelled=%llu\n", mode.c_str(), reads, fixture.written, static_cast <unsigned long long> (::port::localStats.queued), static_cast <unsigned long long> (::port::localStats.delivered), static_cast <unsigned long long> (::port::localStats.cancelled));
	}
};

/**
 * @brief Метод запуска одного сценария в отдельном процессе
 *
 * @param argc число аргументов
 * @param argv аргументы запуска
 * @return     код завершения
 *
 */
int main(const int argc, char * argv[]) {
	// Проверяем наличие названия сценария
	::require(argc == 2, "read-case-argument");
	// Подготавливаем библиотеку до создания событий
	awh::fmk::initialize();
	// Получаем один явно выбранный сценарий
	const std::string mode(argv[1]);
	// Не допускаем молчаливого запуска неизвестного сценария
	::require((mode == "read-send") || (mode == "read-cancel") || (mode == "read-destroy") || (mode == "read-generation") || (mode == "read-forget"), "read-known-case");
	// Проверяем отправку и время жизни из настоящего READ
	::fromRead(mode);
	// Возвращаем успешный результат только после всех обязательных проверок
	return EXIT_SUCCESS;
}
