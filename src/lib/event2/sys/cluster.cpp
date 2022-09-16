/**
 * @file: server.cpp
 * @date: 2022-08-08
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

// Подключаем заголовочный файл
#include <lib/event2/sys/cluster.hpp>

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * child Функция обратного вызова при завершении работы процесса
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 * @param ctx   объект сервера
	 */
	void awh::Cluster::Worker::child(evutil_socket_t fd, short event, void * ctx) noexcept {
		// Идентификатор упавшего процесса
		pid_t pid = 0;
		// Статус упавшего процесса
		int status = 0;
		// Получаем объект кластера
		cluster_t * cluster = reinterpret_cast <cluster_t *> (ctx);
		// Выполняем получение идентификатора упавшего процесса
		while((pid = waitpid(-1, &status, WNOHANG)) > 0){
			// Выполняем перебор всех доступных воркеров
			for(auto & worker : cluster->_jacks){
				// Выполняем поиск завершившегося процесса
				for(auto & jack : worker.second){
					// Если процесс найден
					if(jack->pid == pid){
						// Выводим сообщение об ошибке, о невозможности отправкить сообщение
						cluster->_log->print("child process stopped, pid = %d, status = %x", log_t::flag_t::CRITICAL, jack->pid, status);
						// Если был завершён активный процесс и функция обратного вызова установлена
						if(cluster->_fn != nullptr)
							// Выводим функцию обратного вызова
							cluster->_fn(worker.first, pid, event_t::STOP);
						// Если статус сигнала, ручной остановкой процесса
						if(status == SIGINT){
							// Удаляем событие сигнала
							evsignal_del(jack->ev);
							// Выполняем очистку памяти сигнала
							event_free(jack->ev);
							// Выходим из приложения
							exit(SIGINT);
						// Если время жизни процесса составляет меньше 3-х минут
						} else if((cluster->_fmk->unixTimestamp() - jack->date) <= 180000) {
							// Удаляем событие сигнала
							evsignal_del(jack->ev);
							// Выполняем очистку памяти сигнала
							event_free(jack->ev);
							// Выходим из приложения
							exit(EXIT_FAILURE);
						}
						// Выходим из цикла
						break;
					}
				}
				// Выполняем поиск воркера
				auto it = cluster->_workers.find(worker.first);
				// Если запрашиваемый воркер найден и флаг автоматического перезапуска активен
				if((it != cluster->_workers.end()) && it->second.restart){
					// Создаём объект работника
					unique_ptr <jack_t> jack(new jack_t);
					// Получаем индекс упавшего процесса
					const uint16_t index = cluster->_pids.at(pid);
					// Удаляем процесс из списка процессов
					cluster->_pids.erase(pid);
					// Устанавливаем дочерний процесс
					worker.second.at(index) = move(jack);
					// Замораживаем поток на период в 5 секунд
					this_thread::sleep_for(5s);
					// Выполняем создание нового процесса
					cluster->fork(it->first, index, it->second.restart);
				// Просто удаляем процесс из списка процессов
				} else cluster->_pids.erase(pid);
			}
		}
	}
#endif
/**
 * fork Метод отделения от основного процесса (создание дочерних процессов)
 * @param wid   идентификатор воркера
 * @param index индекс инициализированного процесса
 * @param stop  флаг остановки итерации создания дочерних процессов
 */
void awh::Cluster::fork(const size_t wid, const uint16_t index, const bool stop) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем поиск воркера
		auto it = this->_workers.find(wid);
		// Если воркер найден
		if(it != this->_workers.end()){
			// Если не все форки созданы
			if(index < it->second.count){
				// Флаг первичной инициализации
				bool initialization = false;
				// Выполняем поиск работника
				auto jt = this->_jacks.find(it->first);
				// Выполняем проверку, проведена ли инициализация
				initialization = ((jt == this->_jacks.end()) || jt->second.empty());
				// Если список работников ещё пустой
				if(initialization){
					// Удаляем список дочерних процессов
					this->_pids.clear();
					// Если список работников еще не инициализирован
					if(jt == this->_jacks.end()){
						// Выполняем инициализацию списка работников
						this->_jacks.emplace(it->first, vector <unique_ptr <jack_t>> ());
						// Выполняем поиск работника
						jt = this->_jacks.find(it->first);
					}
					// Выполняем создание указанное количество работников
					for(size_t i = 0; i < it->second.count; i++){
						// Создаём объект работника
						unique_ptr <jack_t> jack(new jack_t);
						// Выполняем добавление работника в список работников
						jt->second.push_back(move(jack));
					}
					// Получаем объект текущего работника
					jack_t * jack = jt->second.at(index).get();
					// Устанавливаем базу событий для сигналов
					jack->ev = evsignal_new(this->_base, SIGCHLD, &worker_t::child, this);
					// Выполняем отслеживание возникающего сигнала
					evsignal_add(jack->ev, nullptr);
				}
				// Устанавливаем идентификатор процесса
				pid_t pid = -1;
				// Определяем тип потока
				switch((pid = ::fork())){
					// Если поток не создан
					case -1: {
						// Выводим в лог сообщение
						this->_log->print("child process could not be created", log_t::flag_t::CRITICAL);
						// Выходим принудительно из приложения
						exit(EXIT_FAILURE);
					} break;
					// Если - это дочерний поток значит все нормально
					case 0: {
						// Если процесс является дочерним
						if(this->_pid == (pid_t) getppid()){
							// Получаем идентификатор текущего процесса
							const pid_t pid = getpid();
							// Добавляем в список дочерних процессов, идентификатор процесса
							this->_pids.emplace(pid, index);
							// Активируем флаг запуска кластера
							it->second.working = true;
							{
								// Получаем объект текущего работника
								jack_t * jack = jt->second.at(index).get();
								// Устанавливаем идентификатор процесса
								jack->pid = pid;
								// Устанавливаем время начала жизни процесса
								jack->date = this->_fmk->unixTimestamp();
								// Если функция обратного вызова установлена, выводим её
								if(this->_fn != nullptr)
									// Выводим функцию обратного вызова
									this->_fn(it->first, pid, event_t::START);
							}
							// Выполняем активацию базы событий
							event_reinit(this->_base);
						// Если процесс превратился в зомби
						} else {
							// Процесс превратился в зомби, самоликвидируем его
							this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
							// Выходим из приложения
							exit(EXIT_FAILURE);
						}
					} break;
					// Если - это родительский процесс
					default: {
						// Добавляем в список дочерних процессов, идентификатор процесса
						this->_pids.emplace(pid, index);
						// Получаем объект текущего работника
						jack_t * jack = jt->second.at(index).get();
						// Устанавливаем PID процесса
						jack->pid = pid;
						// Устанавливаем время начала жизни процесса
						jack->date = this->_fmk->unixTimestamp();
						// Если функция обратного вызова установлена, выводим её
						if(initialization && (this->_fn != nullptr)){
							// Активируем флаг запуска кластера
							it->second.working = true;
							// Выводим функцию обратного вызова
							this->_fn(it->first, pid, event_t::START);
						}
						// Продолжаем дальше
						if(!stop) this->fork(it->first, index + 1, stop);
					}
				}
			}
		}
	#endif
}
/**
 * working Метод проверки на запуск работы кластера
 * @param wid идентификатор воркера
 * @return    результат работы проверки
 */
bool awh::Cluster::working(const size_t wid) const noexcept {
	// Выполняем поиск воркера
	auto it = this->_workers.find(wid);
	// Если воркер найден
	if(it != this->_workers.end())
		// Выводим результат проверки
		return it->second.working;
	// Сообщаем, что проверка не выполнена
	return false;
}
/**
 * clear Метод очистки всех выделенных ресурсов
 */
void awh::Cluster::clear() noexcept {
	// Удаляем список дочерних процессов
	this->_pids.clear();
	// Если список работников не пустой
	if(!this->_jacks.empty()){
		// Переходим по всем работникам
		for(auto & item : this->_jacks)
			// Выполняем остановку процессов
			this->stop(item.first);
		// Выполняем очистку списка работников
		this->_jacks.clear();
		// Выполняем освобождение выделенной памяти
		map <size_t, vector <unique_ptr <jack_t>>> ().swap(this->_jacks);
	}
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем освобождение выделенной памяти
	map <size_t, worker_t> ().swap(this->_workers);
}
/**
 * stop Метод остановки кластера
 * @param wid идентификатор воркера
 */
void awh::Cluster::stop(const size_t wid) noexcept {
	// Выполняем поиск работников
	auto jt = this->_jacks.find(wid);
	// Если работник найден
	if((jt != this->_jacks.end()) && !jt->second.empty()){
		// Выполняем поиск воркера
		auto it = this->_workers.find(jt->first);
		// Если процесс является родительским
		if(this->_pid == getpid()){
			// Флаг перезапуска
			bool restart = false;
			// Если воркер найден, получаем флаг перезапуска
			if(it != this->_workers.end()){
				// Получаем флаг перезапуска
				restart = it->second.restart;
				// Снимаем флаг перезапуска процесса
				it->second.restart = false;
			}
			// Переходим по всему списку работников
			for(auto & jack : jt->second){
				/**
				 * Если операционной системой не является Windows
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Если объект работы с дочерними процессами установлен
					if(jack->ev != nullptr){
						// Удаляем объект работы с дочерними процессами
						evsignal_del(jack->ev);
						// Выполняем очистку памяти сигнала
						event_free(jack->ev);
						// Зануляем объект события
						jack->ev = nullptr;
					}
				#endif
			}
			// Если воркер найден, возвращаем флаг перезапуска
			if(it != this->_workers.end())
				// Возвращаем значение флага автоматического перезапуска процесса
				it->second.restart = restart;
			// Очищаем список работников
			jt->second.clear();
		// Если процесс является дочерним
		} else if(this->_pid == (pid_t) getppid())
			// Очищаем список работников
			jt->second.clear();
		// Если процесс превратился в зомби
		else {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}
		// Если воркер найден, снимаем флаг запуска кластера
		if(it != this->_workers.end())
			// Снимаем флаг запуска кластера
			it->second.working = false;
		// Удаляем список дочерних процессов
		this->_pids.clear();
	}
}
/**
 * start Метод запуска кластера
 * @param wid идентификатор воркера
 */
void awh::Cluster::start(const size_t wid) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если вокер найден
	if(it != this->_workers.end())
		// Выполняем запуск процесса
		this->fork(it->first);
}
/**
 * restart Метод установки флага перезапуска процессов
 * @param wid  идентификатор воркера
 * @param mode флаг перезапуска процессов
 */
void awh::Cluster::restart(const size_t wid, const bool mode) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если вокер найден
	if(it != this->_workers.end())
		// Устанавливаем флаг автоматического перезапуска процесса
		it->second.restart = mode;
}
/**
 * base Метод установки базы событий
 * @param base база событий для установки
 */
void awh::Cluster::base(struct event_base * base) noexcept {
	// Переходим по всем активным воркерам
	for(auto & worker : this->_workers)
		// Выполняем остановку процессов
		this->stop(worker.first);
	// Выполняем установку базы событий
	this->_base = base;
}
/**
 * count Метод получения максимального количества процессов
 * @param wid идентификатор воркера
 * @return    максимальное количество процессов
 */
uint16_t awh::Cluster::count(const size_t wid) const noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если вокер найден
	if(it != this->_workers.end())
		// Выводим максимально-возможное количество процессов
		return it->second.count;
	// Выводим результат
	return 0;
}
/**
 * count Метод установки максимального количества процессов
 * @param wid   идентификатор воркера
 * @param count максимальное количество процессов
 */
void awh::Cluster::count(const size_t wid, const uint16_t count) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если вокер найден
	if(it != this->_workers.end()){
		// Если количество процессов не передано
		if(count == 0)
			// Устанавливаем максимальное количество ядер доступных в системе
			it->second.count = std::thread::hardware_concurrency();
		// Устанавливаем максимальное количество процессов
		else it->second.count = count;
	}
}
/**
 * init Метод инициализации воркера
 * @param wid   идентификатор воркера
 * @param count максимальное количество процессов
 */
void awh::Cluster::init(const size_t wid, const uint16_t count) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если воркер не найден
	if(it == this->_workers.end()){
		// Добавляем воркер в список воркеров
		auto ret = this->_workers.emplace(wid, worker_t());
		// Устанавливаем идентификатор воркера
		ret.first->second.wid = wid;
		// Устанавливаем родительский объект кластера
		ret.first->second.cluster = this;
	}
	// Выполняем установку максимально-возможного количества процессов
	this->count(wid, count);
}
/**
 * onMessage Метод установки функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
 * @param callback функция обратного вызова
 */
void awh::Cluster::on(function <void (const size_t, const pid_t, const event_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_fn = callback;
}
