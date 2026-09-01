#!/usr/bin/env python3
##
# @file clash.py
# @date 2026-09-01
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @telegram{forman}
# @phone{+7 (910) 983-95-90}
#
# @email forman@anyks.com
# @site https://anyks.com
#
# @brief Сличение имён членов перечислений AWH с макросами системных заголовков
#
# @details Препроцессор области видимости не разбирает и заменяет имя даже внутри
#          enum class: объявление `CS5 = 0x28` при пришедшем <termios.h> обращается
#          в `(0x00000000) = 0x28`, и сборка отвечает отказом. Лечится это парой
#          include/sys/macro_push.hpp и include/sys/macro_pop.hpp, но лишь для тех
#          имён, что в неё внесены
#
#          Беда капкана в том, что вскрывается он не у нас: дерево собирается, пока
#          сталкивающийся заголовок в него никто не включает, а у потребителя
#          библиотеки он приходит первым же подключением. Оттого сличение идёт по
#          ШИРОКОМУ набору системных заголовков, а не по тем, что дерево включает само
#
# @note Метка вывода взята ASCII намеренно. Кириллическая метка молчала на GCC: тот
#       экранирует не-ASCII в \\U00000421, и поиск её не находил - сличение отвечало
#       нулём столкновений там, где их три
#
# @note Столкнувшееся имя раскрывается в своё значение и метку с собою съедает,
#       поэтому в метку идёт НОМЕР имени, а не само имя
#
# @copyright Copyright © 2026
#
##
import os, re, sys, subprocess, tempfile, platform

# Каталоги, откуда берутся объявления перечислений
ROOTS = ('include',)

##
# Набор системных заголовков сличения
#
# Первая часть общая для всех систем, вторая - свойственная отдельным: заголовки
# указателей сегментов Sun, параметров машины BSD, колец и оповещений Linux. Тех,
# каких у системы нет, сличение не берёт - наличие спрашивается пробной сборкой
##
HEADERS = """
termios.h fcntl.h signal.h netdb.h netinet/in.h netinet/tcp.h netinet/ip.h
sys/socket.h sys/stat.h sys/mman.h sys/wait.h sys/resource.h sys/time.h sys/param.h
sys/ioctl.h sys/uio.h sys/un.h sys/types.h sys/select.h sys/file.h sys/utsname.h
sys/statvfs.h sys/mount.h sched.h dlfcn.h limits.h math.h errno.h syslog.h dirent.h
poll.h pthread.h regex.h time.h stdio.h stdlib.h string.h unistd.h arpa/inet.h
net/if.h ifaddrs.h grp.h pwd.h locale.h ctype.h wchar.h semaphore.h sysexits.h
ucontext.h utime.h glob.h netinet/udp.h net/route.h nl_types.h aio.h fnmatch.h
monetary.h
sys/regset.h sys/procset.h sys/machelf.h port.h sys/port.h procfs.h
machine/param.h sys/event.h sys/sysctl.h sys/user.h sys/cpuset.h
sys/epoll.h sys/inotify.h sys/eventfd.h sys/signalfd.h sys/timerfd.h
linux/if_packet.h linux/netlink.h linux/rtnetlink.h netinet/sctp.h
netpacket/packet.h sys/prctl.h sys/syscall.h sys/vfs.h ucred.h sys/ucred.h
""".split()

# Выражение выборки тела перечисления и его членов
ENUM = re.compile(r'enum\s+class\s+\w+\s*(?::\s*[\w:]+\s*)?\{(.*?)\}\s*;', re.S)
MEMBER = re.compile(r'^\s*([A-Z_][A-Z0-9_]*)\s*(?:=|,|$)', re.M)

##
# @brief Метод сбора имён членов перечислений
#
# @param base корень дерева исходников
# @return     перечень имён по порядку
##
def collect(base):
	result = set()
	for root in ROOTS:
		for path, _, files in os.walk(os.path.join(base, root)):
			for name in files:
				if not name.endswith('.hpp'):
					continue
				body = open(os.path.join(path, name), encoding = 'utf-8', errors = 'replace').read()
				for block in ENUM.findall(body):
					result.update(MEMBER.findall(block))
	return sorted(result)

##
# @brief Метод отбора заголовков, какие у системы есть
#
# @param cc собиратель
# @return   перечень доступных заголовков
##
def available(cc):
	result = []
	with tempfile.TemporaryDirectory() as tmp:
		probe = os.path.join(tmp, 'probe.c')
		for header in dict.fromkeys(HEADERS):
			open(probe, 'w').write('#include <%s>\n' % header)
			if subprocess.call([cc, '-E', probe], stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL) == 0:
				result.append(header)
	return result

##
# @brief Метод чтения имён, снимаемых парой macro_push.hpp
#
# @param base корень дерева исходников
# @return     множество снятых имён
##
def guarded(base):
	path = os.path.join(base, 'include', 'sys', 'macro_push.hpp')
	body = open(path, encoding = 'utf-8', errors = 'replace').read()
	return set(re.findall(r'#pragma\s+push_macro\("([A-Za-z_][A-Za-z0-9_]*)"\)', body))

##
# @brief Точка входа
##
def main():
	base = sys.argv[1] if (len(sys.argv) > 1) else '.'
	cc = os.environ.get('CC', 'cc')
	names = collect(base)
	headers = available(cc)
	with tempfile.TemporaryDirectory() as tmp:
		source = os.path.join(tmp, 'clash.c')
		with open(source, 'w') as file:
			for header in headers:
				file.write('#include <%s>\n' % header)
			for index, name in enumerate(names):
				file.write('#ifdef %s\nAWHCLASH %d ENDCLASH\n#endif\n' % (name, index))
		out = subprocess.run([cc, '-E', '-P', source], capture_output = True, text = True).stdout
	found = sorted(set(int(i) for i in re.findall(r'AWHCLASH (\d+) ENDCLASH', out)))
	known = guarded(base)
	covered = [names[i] for i in found if names[i] in known]
	naked = [names[i] for i in found if names[i] not in known]
	print('система: %s | заголовков: %d | членов перечислений: %d' % (platform.system(), len(headers), len(names)))
	print('столкновений: %d, из них снято парой: %d' % (len(found), len(covered)))
	if covered:
		print('снято:')
		for name in covered:
			print('   %s' % name)
	##
	# Сличение обязано УТВЕРЖДАТЬ, а не печатать
	#
	# Печатающая проверка проходит и тогда, когда щуп сломан. Оттого непокрытое имя
	# отвечает отказом с кодом 1: молчаливое «посмотрите глазами» было бы тем же
	# отключением проверки
	##
	if naked:
		print('НЕ СНЯТО ПАРОЙ - столкновение открыто:')
		for name in naked:
			print('   %s' % name)
		return 1
	print('непокрытых столкновений нет')
	return 0

if __name__ == '__main__':
	sys.exit(main())
