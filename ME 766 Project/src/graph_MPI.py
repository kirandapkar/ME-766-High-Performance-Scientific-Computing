import sys
import time
import os, glob
import subprocess 
import matplotlib.pyplot as plt


output,error = subprocess.Popen(['make'], stdout=subprocess.PIPE,stderr=subprocess.PIPE).communicate()

path = []
files = []

if len(sys.argv) > 1:
    if sys.argv[1].endswith('.txt'):
        [files.append(x) for x in sys.argv[1:]]
    else:
        files = glob.glob(os.path.join(sys.argv[1], '*.txt'))
else:
    path = 'TestCase/Thread/'
    files = glob.glob(os.path.join(path, '*.txt'))


files.sort()

X = [1,2,4,6,8]

S = []
MPI = [[],[],[]]
run_no_times = 5

for i,sudoku in enumerate(files):
    print("%s" % sudoku)
    one_sum = 0
    for x in range(1,run_no_times+1):
        output,error = subprocess.Popen(['./sudoku_serial', sudoku], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
        real = output.decode().strip().split()[-2]
        print("Serial\t, %d,%f" % (x,float(real)))
        one_sum = one_sum + float(real)
    one_avg = one_sum/run_no_times
    MPI[i].append(float(one_avg))

    Threads = [2,4,6,8]
    for t in Threads:
        one_sum = 0
        for x in range(1,run_no_times+1):
            output,error = subprocess.Popen(['time','mpirun','-np',str(t),'./sudoku_mpi', sudoku], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
            real = output.decode().strip().split()[-1]
            print("OMP\t thread-%d, %d,%f" % (t,x,float(real)))
            one_sum = one_sum + float(real)
        one_avg = one_sum/run_no_times
        MPI[i].append(float(one_avg))
    
print(X,MPI)

fig = plt.figure()        
plt.plot(X, MPI[0])
plt.plot(X, MPI[1])
plt.plot(X, MPI[2])


plt.legend(["Easy", "Medium","Hard"])

plt.xlabel('Threads')  
plt.ylabel('Time Taken') 

plt.title('MPI Parallel Sudoku Solver') 

plt.show()  