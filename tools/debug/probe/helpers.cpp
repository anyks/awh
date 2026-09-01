// Щуп сплошной проверки помощников описателей
//
// Поля проверяемые лежат в СЕРЕДИНЕ, а соседями им заведомо ненулевые значения:
// в хвосте мусор совпадает с ожидаемым и щуп подтверждает неверный помощник
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <cstdio>

using namespace std;

struct probe_t {
	uint64_t guard_head;
	string small;
	string large;
	vector <int> numbers;
	unordered_map <int, int> pairs;
	shared_ptr <int> owned;
	shared_ptr <int> empty;
	atomic <size_t> counter;
	atomic_bool raised;
	atomic_bool lowered;
	deque <int> tasks;
	uint64_t guard_tail;
};

int main(){
	probe_t probe;
	probe.guard_head = 0xAAAAAAAAAAAAAAAAull;
	probe.small = "Юрий";
	probe.large = string(200, 'w');
	probe.numbers = {10, 20, 30, 40, 50};
	probe.pairs = {{1, 1}, {2, 2}, {3, 3}};
	probe.owned = make_shared <int> (42);
	probe.counter.store(230700);
	probe.raised.store(true);
	probe.lowered.store(false);
	probe.tasks = {1, 2, 3, 4};
	probe.guard_tail = 0xBBBBBBBBBBBBBBBBull;
	printf("small=%zu large=%zu numbers=%zu pairs=%zu tasks=%zu\n",
		probe.small.size(), probe.large.size(), probe.numbers.size(), probe.pairs.size(), probe.tasks.size());
	return 0;
}
