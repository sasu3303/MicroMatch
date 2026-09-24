"""Readable walkthrough over the compiled C++ CLI (Python 3.9+ is enough)."""
import argparse,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--binary',default=str(root/'build/micromatch'));args=p.parse_args()
commands=[x for x in (root/'data/demo.txt').read_text().splitlines() if x and not x.startswith('#') and x!='QUIT']
run=subprocess.run([args.binary],input='\n'.join(commands)+'\n',text=True,capture_output=True,check=True)
responses=[json.loads(line) for line in run.stdout.splitlines()]
assert len(commands)==len(responses)
print('MICROMATCH | local C++ exchange simulator\nPrices are integer ticks. No real trading occurs.\n')
for command,r in zip(commands,responses):
    print('> '+command)
    if not r['ok']:print('  Rejected: '+r['code']);continue
    for trade in r['trades']:
        print(f"  Trade: maker {trade['maker']} -> taker {trade['taker']}; {trade['quantity']} units at {trade['price']} ticks")
    if r['code']=='book':
        print('  Bids: '+(', '.join(f"{x['quantity']} @ {x['price']} ({x['orders']} orders)" for x in r['bids']) or 'empty'))
        print('  Asks: '+(', '.join(f"{x['quantity']} @ {x['price']} ({x['orders']} orders)" for x in r['asks']) or 'empty'))
    else:print(f"  {r['code']}; remaining quantity: {r['remaining']}")
    print()
