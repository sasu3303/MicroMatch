"""Run three trials of each local microbenchmark; preserve every trial."""
import argparse,json,platform,statistics,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--build-dir',default='build');p.add_argument('--output',default='evidence/benchmarks.json');args=p.parse_args()
build=(root/args.build_dir).resolve()
def trial(name):return json.loads(subprocess.check_output([str(build/name)],text=True))
core=[trial('micromatch_bench') for _ in range(3)]
cancel=[trial('micromatch_cancel_bench') for _ in range(3)]
cpu='Not recorded'
if Path('/proc/cpuinfo').exists():
    for line in Path('/proc/cpuinfo').read_text().splitlines():
        if line.startswith('model name'):cpu=line.split(':',1)[1].strip();break
result={'platform':platform.platform(),'cpu':cpu,'compiler':subprocess.check_output(['c++','--version'],text=True).splitlines()[0],'build':'CMake Release (verify build-dir configuration); no sanitizers','core_trials':core,'cancellation_trials':cancel,'summary':{'core_p95_ns_median_of_trials':statistics.median(r['p95_ns'] for r in core),'core_throughput_median':statistics.median(r['throughput_ops_per_second'] for r in core),'vector_cancel_p95_ns_median':statistics.median(r['vector_scan_erase_p95_ns'] for r in cancel),'indexed_cancel_p95_ns_median':statistics.median(r['indexed_list_erase_p95_ns'] for r in cancel)},'limitations':'Local synthetic microbenchmarks. No CPU pinning or production load. Core trial has at most one resting order; cancellation trial shrinks 20K to zero. Per-operation clock overhead is included in percentiles; core throughput uses a separate pass without per-operation timers. Not exchange/network latency.'}
output=root/args.output;output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(result,indent=2))
print(json.dumps(result['summary'],indent=2))
