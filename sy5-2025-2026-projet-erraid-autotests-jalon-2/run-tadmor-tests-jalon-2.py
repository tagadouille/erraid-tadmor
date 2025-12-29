import os
import itertools
from common import *
#from multiprocessing.pool import ThreadPool

if not os.path.isfile("../tadmor"):
  print(f"Erreur : La compilation n'a pas produit le fichier exécutable `tadmor`. Abandon.")
  exit(1)

exec_path = f'{os.getcwd()}/../tadmor'
recording_prefix = "tadmor-tests/tadmor-jalon-2-test-"

for i in itertools.count(start=1):
  path = f'{recording_prefix}{i+1}'
  if not os.path.isdir(path):
    nb_tests = i
    break


prefix_dir = f'/tmp/{pwd.getpwuid(os.getuid()).pw_name}/erraid-tests'
os.makedirs(prefix_dir, exist_ok = True)
with tempfile.TemporaryDirectory(dir = prefix_dir) as run_dir:
  os.makedirs(f'{run_dir}/pipes', exist_ok = True)
  os.mkfifo(f'{run_dir}/pipes/erraid-request-pipe', 0o600)
  os.mkfifo(f'{run_dir}/pipes/erraid-reply-pipe', 0o600)
  
  tests = [ (lambda i=i: lambda : client_replay(exec_path, run_dir, f'{recording_prefix}{i}', (2,i),
                                                use_valgrind=os.path.exists(f'{path}/valgrind'))) ()
            # Ugly hack because python closures capture references, not values
            for i in range(1, nb_tests+1) ]
  
  #tests = [ tests[0] ] # For testing
  #with ThreadPool(processes=len(tests)) as pool:
  #  reports_async = [ pool.apply_async(test, ()) for test in tests ]
  #  reports = [ ar.get() for ar in reports_async ]
  
  reports = []
  for test in tests:
    report = test()
    reports.append(report)
    if report['timed_out']:
      break

report = {'totalok': 0, 'totaltests': 0, 'totalfailures':0, 'text': ''}
report['totaltests'] = nb_tests
for r in reports:
  report['totalfailures'] += r['failures']
  if r['failures'] == 0:
    report['totalok'] += 1

if report['totalok'] != report['totaltests']:
  for r in reports:
    report['text'] += r['text']

jalonsummary(report, jalon="2 (client)")

if ENV_GRADE_PROJECT:
  penalty = int(os.getenv("GRADE_PROJECT_PENALTY", "0")) / 100
  grade = report['totalok']*10/report['totaltests']-penalty
  if grade < 0:
    grade = 0
  if grade == int(grade):
    grade_str = f"{int(grade)}"
  else:
    grade_str = f"{grade:.1f}"
  report['text'] += f"\nNote : {grade_str} / 10\n"

print(report['text'])

