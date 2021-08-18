# uniqify
The job of **uniqify** is to read a text file and output the unique words in the file, sorted in alphabetic order. The input is from **stdin**, and the output is to **stdout**. In one case, you will be using distinct input and output processes. In the other case, you will have a single process, with two threads: one thread for input, and one for output. 

# Build
* For process version
```
./build.sh
```
* For thread version
```
./build.sh thread
```

# Run
```
./msort k [-c] < input [> output]
```
Where **k** is number of processes or threads, **-c** is an option to count the number of duplicates of words, **input** is a text file, **output** is a text file that has been sorted and deduplicated. For large files you should increase the number of ***k***.
```
./msort 2 < data1.txt
./msort 4 -c < data2.txt
./msort 4 < data2.txt > output.txt
```
# Execution time
./test_time.sh file is used to get the execution time
```
./test_time.sh 8 data1.txt exec_time.csv
```

# Contact
If you want to change or upgrade this project, you can contact me by email bachns.dev@gmail.com
