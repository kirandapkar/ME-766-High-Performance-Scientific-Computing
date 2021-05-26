default:
	g++ -fopenmp -o sudoku_omp Openmp/Sudoku_omp.cpp
	g++ -o sudoku_serial Serial/Serial_Sudoku.cpp
	mpicc -fopenmp -o sudoku_mpi Mpi/sudoku-mpi.c

clean:
	rm -rf  *.o *~ *.*~