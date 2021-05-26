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
    path = 'TestCase/Levels/'
    files = glob.glob(os.path.join(path, '*.txt'))


files.sort()

X = [1,2,3,4,5,6,7,8,9,10]

S = []
OMP = []
MPI = []


times = []
times_all = {}
run_no_times = 5


for sudoku in files:
    one_sum = 0
    print("%s" % sudoku)
    for x in range(1,run_no_times+1):
        output,error = subprocess.Popen(['./sudoku_omp', sudoku,'8'], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
        real = output.decode().strip().split()[-2]
        print("\t%d,%f" % (x,float(real)))
        one_sum = one_sum + float(real)

    one_avg = one_sum/run_no_times
    times.append(one_avg)
    times_all[(sudoku)] = float(one_avg)
    OMP.append(float(one_avg))


for sudoku in files:
    one_sum = 0
    t = 2
    print("%s" % sudoku)    
    for x in range(1,run_no_times+1):
        output,error = subprocess.Popen(['time','mpirun','-np','8','./sudoku_mpi', sudoku], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
        real = output.decode().strip().split()[-1]         
        print("\t%d,%f" % (x,float(real)))
        one_sum = one_sum + float(real)

    one_avg = one_sum/run_no_times
    times.append(one_avg)
    times_all[(sudoku)] = float(one_avg)
    MPI.append(float(one_avg))


for sudoku in files:
    one_sum = 0
    print("%s" % sudoku)
    for x in range(1,run_no_times+1):
        output,error = subprocess.Popen(['./sudoku_serial', sudoku], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
        real = output.decode().strip().split()[-2]
        print("\t%d,%f" % (x,float(real)))
        one_sum = one_sum + float(real)

    one_avg = one_sum/run_no_times
    times.append(one_avg)
    times_all[(sudoku)] = float(one_avg)
    S.append(float(one_avg))           



print(X,OMP,MPI,S)

fig = plt.figure()        

plt.plot(X, S)

plt.plot(X, OMP)
plt.plot(X, MPI)


plt.legend(["Serial", "OMP 8 Threads", "MPI 8 Threads"])

plt.xlabel('Easy to Hard Level Sudokus')  
plt.ylabel('Time Taken') 

 
plt.title('Parallel Sudoku Solver') 
  
plt.show()  
