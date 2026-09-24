"""Real loopback TCP checks. The server is a local single-client-at-a-time demo."""
import json, socket, subprocess, sys
server = subprocess.Popen([sys.argv[1], '0'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
checks = []
try:
    line = server.stdout.readline().strip()
    assert line.startswith('LISTENING 127.0.0.1 '), line
    port = int(line.split()[-1])
    def connect():
        return socket.create_connection(('127.0.0.1', port), timeout=3)
    with connect() as sock:
        stream = sock.makefile('rb')
        # The first command is split across TCP writes; next two share one write.
        sock.sendall(b'LIMIT 1 SE')
        sock.sendall(b'LL 10000 10\nLIMIT 2 BUY 10000 4\nBOOK\n')
        first, fill, book = [json.loads(stream.readline()) for _ in range(3)]
        assert first['ok'] and fill['trades'][0]['quantity'] == 4
        assert book['asks'][0]['quantity'] == 6
        checks += ['fragmented request', 'coalesced requests', 'FIFO response framing', 'matching over TCP']
        sock.sendall(b'LIMIT 3 BUY 1.2 5\nCANCEL 1\nBOOK\n')
        bad, cancel, empty = [json.loads(stream.readline()) for _ in range(3)]
        assert not bad['ok'] and cancel['ok'] and not empty['asks']
        checks += ['malformed command followed by valid command', 'cancellation over TCP']
        sock.sendall(b'QUIT\n'); assert json.loads(stream.readline())['code'] == 'bye'
        stream.close()
    with connect() as sock:
        sock.sendall(b'x' * 257)
        stream=sock.makefile('rb')
        assert json.loads(stream.readline())['code']=='line_too_long'
        stream.close()
        checks += ['oversized frame rejected']
    with connect() as sock:
        sock.sendall(b'LIMIT 99 BUY') # Disconnect in the middle of a command.
    with connect() as sock:
        sock.sendall(b'BOOK\nQUIT\n')
        stream=sock.makefile('rb')
        result=json.loads(stream.readline())
        assert not result['bids'] and not result['asks']
        assert json.loads(stream.readline())['code']=='bye'
        stream.close()
        checks += ['incomplete frame discarded on disconnect', 'server accepts subsequent client']
    print(json.dumps({'passed': True, 'checks': checks}, indent=2))
finally:
    server.terminate()
    try: server.wait(timeout=3)
    except subprocess.TimeoutExpired: server.kill();server.wait()
