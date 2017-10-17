REPORT - PROGRAMMING ASSIGNMENT 1

DOCUMENTATION

This programming assignment is coding the travelling salesman problem based on genetic algorithm concept with a modified greedy crossover algorithm and parallelizing it with OpenMP. We use the omp.h library for parallelization. In this program we find the shortest trip through the 36 cities and also with lesser computation time.

for MAX_GENERATION { 

evaluate() : It calculates the fitness of each trip and uses qsort() to sort the trips in ascending order. It runs completely parallel. It has 2 portions – parallel compute the fitness of each chromosome, and qsort which is serialized. The distance is calculated with the helper function fitnessBetweenTwoCities() The qsort() requires the additional function compareFitness()
update and print the shortest path

select(): It selects the shortest TOP_X from the trips which is parallelized.

crossover(): Generates offsprings from parents. The loop_length is TOP_X / 2. Each parallel iteration picks i and i + 1 parents and generates i and i + 1 offsprings using the greedy crossover algorithm. I have used an array distances[][] for storing the results of distances between two cities (computeDistanceBetweenCities()). In this case the distance is never calculated more than once between two cities. Since the distance look-up is from an array, its computation is parallelized and is optimal. For retrieving the stored data, I have implemented two helper functions fitnessBetweenTwoCities().To help choose the next city for the offspring, I have implemented a boolean array of cities to know which city is already in the offspring. It gets the next random unvisited city by using the function getRandomUnvisitedCity(). The offspring i + 1 gets the complement city using the function getComplementCity() which calculates the complementIndex and retrieves the associated city char.

mutate(): randomly chooses two distinct cities in each trip with the given probability and swaps them.

populate(): populates the next generation by replacing the bottom half with the newly generated offsprings which is parallelized.
}

I have implemented other small helper functions like getCityFromIndex() which gets the char city from the int index and getCityIndex() which gets the int indx from the char city.

I have parallelized the crossover() and evaluate() functions indepth for inner for loops too,as they are the major regions to improve the performance. I was trying to understand OpenMP by applying it on for loops all throughout the program. When I applied it for mutate(), there was a performance decrement.

Probably the rand() function was causing that, as it cannot be parallelized. 
When I tried in select() and populate(), I got mixed results. At times there will be improvement and sometimes no.

In Tsp.cpp, I have added an extern function void computeDistanceBetweenCities(int coordinates[CITIES][2]) to calculate the distance between the cites.

DISCUSSION

Results:
One of the main requirements of this assignment is to have a performance improvement equal or greater than 2.1x. This code achieves a performance improvement of about 2.59 times.
The elapsed time for sequential code is 19167038 seconds.
The elapsed time for parallelized code is 7390456 seconds.
The second requirement was to have a shortest path with 449.658. This code usually gives the result of 449.658 with MUTATION_RATE 50. Now and then, rarely it will show a shortest path of 447.638. When I was running the script to store my execution output into typescript,I luckily got one of those shortest path (447.638) results to be captured. I still tried couple of more times with 4 threads and MUTATION_RATE 50, but I got only 449.658 again. When I reduced the MUTATION_RATE, I was able to get that shortest 447.638 distance constantly.
Possible Performance Improvement:
I think there will be a better performance, if the for loop for generating the maximum generations in Tsp.cpp could be parallelized. It could be done by dividing them into as many sections as the number of threads. If we could parallelize them separately by modifying the present code with different paramenters, it may increase the performance.
Computing Pi using Monte Carlo method of Lab1 explains that rand() doesn’t parallelize properly. We can try to implement one thread to generate rand() and other threads to pick from that, so other threads can all run in parallel.
We need to explore the correctness of implications – whether it needs this much MUTATION_RATE or CHROMOSOMES. When I tried with 24000 chromosomes and 12000 TOP_X, I was able to get the 449.658 shortest result at a much lesser time.
Qsort vs std sort : The std::sort works faster than the qsort. If some other type of sort, say merge sort, is used it can be parallelized to some extent. (http://stackoverflow.com/questions/4708105/performance-of-qsort-vs-stdsort).

Limitation:
Theoretically the time should be less by 4 times. But that doesn’t happen as all the parts of the code are not parallelized.
The rand() function cannot be parallelized. We use rand() for generating the unvisited city in crossover() and for shuffling in mutate(). The genetic algorithm requires the use of random selection and shuffling.
Sorting algorithm – qsort() used to sort the trips by distance, cannot be parallelized.
If we can add time stamps at the start and end of each function, we can find which function or part of the code takes long time and try to re-organize the code for more performance. This program notes the complete time and not individually for the functions.

EXECUTION OUTPUT

The below screen shot is another example for the output with Tsp 4 with MUTATION_RATE 50, showing the shortest path of 447. 388. 
