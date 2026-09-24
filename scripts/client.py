"""Interactive TCP client: python3 scripts/client.py [--port 9000]."""
import argparse,json,socket
parser=argparse.ArgumentParser();parser.add_argument('--port',type=int,default=9000);args=parser.parse_args()
with socket.create_connection(('127.0.0.1',args.port),timeout=5) as connection:
    connection.settimeout(15)
    stream=connection.makefile('rb')
    print('MicroMatch: LIMIT id BUY|SELL price_ticks quantity; MARKET id BUY|SELL quantity; CANCEL id; BOOK; QUIT')
    print('Server closes an idle connection after 60 seconds. Reconnect if needed.')
    while True:
        try:line=input('order> ')
        except EOFError:break
        connection.sendall((line+'\n').encode())
        raw=stream.readline()
        if not raw:print('Server disconnected.');break
        print(json.dumps(json.loads(raw),indent=2))
        if line=='QUIT':break
