CS 214 Project 2

Aadarsh Kumar NetID: ak3053



make produces executable ./compare.

To build with AddressSanitizer and UBSan you should uncomment the CFLAGS += line in the Makefile then $ make clean && make




./compare <file|directory> [<file|directory> ...]

Files given directly will be included in the analysis set.
Directories are then traveled recursively and only files
that match the suffix will be considered.

Output
JSD  path1  path2
output shjould be sorted by decreasing word counts


Word Frequency Distribution (WFD)
Each file's WFD is stored as a sorted linked list of WordNode
structs.  Keeping the list sorted enables O(N+M)
merge-iteration when computing JSD

Words are tokenised by scanning bytes read with open()/read()/close().
EVERYTHINGg that isn't a word character is treated as delimiter.
Words -> lowercase as they are read.
Word storage is dynamically allocated, no max length (in theory).


The WFD lists are iterated simultaneously so that
every distinct word is considered once.  KLD accumulators for
both files are updated per word, then combined as per equation (3) in
the spec.  sqrt() and log2() from <math.h> are used


\
stat() distinguishes regular files from directories.
Arguments that are files are added directly (as said before).
Arguments that are directories == recursive traversal 
/ readdir().  Entries starting with '.' are skipped.







A flat array of Comparison structs is filled (one per unordered pair),
then sorted with qsort() by decreasing combined word count.

Error handling
Any file/directory that cannot be opened triggers perror()
If fewer than two files are collected, error is printed and then  EXIT_FAILURE.
malloc()/realloc() failures also exit immediately


My Testing Plan


1) already Known Example
file1.txt  
    "hi there hi there"
file2.txt  
          "hi hi out there"
Expected WFDs and JSD should be about 0.3945
Verify match

Identical files
two copies should Yield JSD == 0.00

No similariities
Two files don't share anything
JSD = 1.0 (max)

Single-word files
  One-word files where frequency = 1.0, JSD computed correctly.

ignoring case sensitivity
"Hello HELLO hello" should treat all three tokens as the same word 'hello'.

ignoring punctuationm 
"can't" "cant"  (apostrophe ignored and our words are all treated as 'cant')
   

dashes
"well-known" is one token.

recursive dir traversal
testdir/
alpha.txt
.hidden.txt          //has to skip
  subdir/
    beta.txt
      data.csv           //has to skip

./compare testdir
only alpha.txt and beta.txt in the analysis

Mixed explicit files and directories
./compare somefile.md testdir
somefile.md must appear

Fewer than two files
./compare only_one.txt  
we get EXIT_FAILURE

Non-existent path
perror but continues

large files or many files
check for memory leaks with valgrind and memory sanitizer

empty file
empty file with any other file should produce JSD = 1



these tests should ensure that compare works as expected
