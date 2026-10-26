import os, re, socket, subprocess, sys, tempfile, time

img = sys.argv[1] if len(sys.argv) > 1 else 'build/textile.img'
qemu = os.environ.get('QEMU', 'qemu-system-i386')
work = tempfile.mkdtemp()
sock = os.path.join(work, 'mon.sock')
log = os.path.join(work, 'serial.log')

proc = subprocess.Popen([qemu, '-fda', img, '-display', 'none', '-no-reboot',
                         '-netdev', 'user,id=n0',
                         '-device', 'rtl8139,netdev=n0',
                         '-serial', 'file:' + log,
                         '-monitor', 'unix:' + sock + ',server,nowait'])
ok = True
try:
    for _ in range(100):
        if os.path.exists(sock):
            break
        time.sleep(0.1)
    mon = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    mon.connect(sock)
    time.sleep(5)

    keys = {' ': 'spc', '\n': 'ret'}

    def type_line(text):
        for ch in text + '\n':
            mon.sendall(('sendkey %s\n' % keys.get(ch, ch)).encode())
            time.sleep(0.12)
        time.sleep(0.8)

    for cmd in ['help', 'time', 'uptime', 'in 60', 'echo hello textile', 'sqr 10 5', 'net']:
        type_line(cmd)
    time.sleep(0.5)
    text = open(log, errors='ignore').read()
finally:
    proc.kill()

checks = [
    ('kernel banner', 'Textile OS 0.2'),
    ('help list', 'Commands:'),
    ('help lists net', 'net            show'),
    ('clock', re.compile(r'20\d\d-\d\d-\d\d \d\d:\d\d:\d\d')),
    ('uptime from timer', re.compile(r'up \d+ s')),
    ('port read', re.compile(r'port 0x0060 = 0x[0-9A-F]{2}')),
    ('echo', 'hello textile'),
    ('sqr draws square', 'square at 10 5'),
    ('net finds rtl8139', re.compile(r'rtl8139 at io 0x[0-9A-F]{4}')),
    ('net mac', re.compile(r'mac (?:[0-9A-F]{2}:){5}[0-9A-F]{2}')),
    ('net link state', re.compile(r'link (up|down)')),
]
for name, pat in checks:
    found = bool(pat.search(text)) if hasattr(pat, 'search') else pat in text
    ok = ok and found
    print(('PASS ' if found else 'FAIL ') + name)
print('--- serial output ---')
print(text[-1000:])
sys.exit(0 if ok else 1)
