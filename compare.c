/*compare.c cs214 project 22 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>













//linked list node

typedef struct WordNode {
    char          *word;        /* will dynamically allocated */
    int            count;       
    double         freq;        
    struct WordNode *next;


} WordNode;

/* WFD  */
typedef struct {
    char     *path;       
    WordNode *words;       /* sorted linked list */
    int       total_words; /*  wtokens*/

} WFD;

// one comparison
typedef struct {
    int    i, j;            /* indices into g_files[]     */

    int    combined_words;  /* word count of both files   */
    double jsd;
} Comparison;


//global vars
#define SUFFIX      ".txt"          /* file suffix we look at */
#define READ_BUF    4096            /* buffer*/
#define INIT_WORD   64              /* initial word buffer size*/

static WFD   **g_files      = NULL;





static int     g_file_count = 0;
static int     g_file_cap   = 0;

/* WFD helpers*/
/* Insert or increment a word (already lowercase) in a sorted linked list */
static void wfd_insert(WFD *wfd, const char *word) {
    WordNode **pp = &wfd->words;

    while (*pp) {

        int cmp = strcmp((*pp)->word, word);
        if (cmp == 0) {

            (*pp)->count++;

            wfd->total_words++;
            return;
        }

        if (cmp > 0) break;   /* insert before *pp */

        pp = &(*pp)->next;
    }

    /*  new node */
    WordNode *n = malloc(sizeof(WordNode));

    if (!n) { perror("malloc"); exit(EXIT_FAILURE); }                       //if we ever run out of memory we exit , critical failure

    n->word = strdup(word);

    if (!n->word) { perror("strdup"); exit(EXIT_FAILURE); }
    n->count = 1;

    n->freq  = 0.0;

    n->next  = *pp;

    *pp = n;

    wfd->total_words++;
}

/* count to freqs */
static void wfd_finalize(WFD *wfd) {

    if (wfd->total_words == 0) return;
    for (WordNode *n = wfd->words; n; n = n->next)
        n->freq = (double)n->count / (double)wfd->total_words;
}

/* Free a WFD and all its words */
static void wfd_free(WFD *wfd) {
    WordNode *n = wfd->words;
    while (n) {
        WordNode *tmp = n->next;
        free(n->word);
        free(n);
        n = tmp;
    }
    free(wfd->path);
    free(wfd);
}

// read file 

// Return true=1 if c is a word character as   in project sepc

static int is_word_char(int c) {
    return isalpha(c) || isdigit(c) || c == '-';
}

// Read file at patgh then it build and returns WFD || NULL if error opening
static WFD *read_file(const char *path) {           //Done I think??
    int fd = open(path, O_RDONLY);                              
    if (fd < 0) {
        perror(path);
        return NULL;
    }

    WFD *wfd = malloc(sizeof(WFD));

    if (!wfd){ 
        perror("malloc"); exit(EXIT_FAILURE); 
    }



    wfd->path        = strdup(path);
    wfd->words       = NULL;
    wfd->total_words = 0;

    char  buf[READ_BUF];
    ssize_t n;

    
    int   wlen = 0;
    int   wcap = INIT_WORD;
    char *wbuf = malloc(wcap);
    if (!wbuf){ perror("malloc"); exit(EXIT_FAILURE); }

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            int c = (unsigned char)buf[i];
            if (is_word_char(c)) {
                /* grows word buffer*/
                if (wlen + 1 >= wcap) {
                    wcap *= 2;
                    wbuf = realloc(wbuf, wcap);
                    if (!wbuf) { perror("realloc"); exit(EXIT_FAILURE); }
                }
                wbuf[wlen++] = tolower(c);
            } else {
                /* word boundary */
                if (wlen > 0) {
                    wbuf[wlen] = '\0';
                    wfd_insert(wfd, wbuf);
                    wlen = 0;
                }
            }
        }
    }
    if (n < 0) perror(path);

    // flush  trailing words
    if (wlen > 0) {
        wbuf[wlen] = '\0';

        wfd_insert(wfd, wbuf);
    }

    free(wbuf);

    close(fd);
    wfd_finalize(wfd);
    return wfd;
}

// File and directory collection

static void add_file(WFD *wfd) {
    if (g_file_count == g_file_cap) {

        g_file_cap = g_file_cap ? g_file_cap * 2 : 8;
        g_files = realloc(g_files, g_file_cap * sizeof(WFD *));
////
        if (!g_files) { perror("realloc"); exit(EXIT_FAILURE); }


    }

    g_files[g_file_count++] = wfd;
}

/* Returns 1 if name ends with SUFFIX */
static int has_suffix(const char *name) {
    size_t nlen = strlen(name);
    size_t slen = strlen(SUFFIX);
    if (nlen < slen) return 0;
    return strcmp(name + nlen - slen, SUFFIX) == 0;
}


static void traverse_dir(const char *path);

static void process_entry(const char *dirpath, const char *name) {
    if (name[0] == '.') return;   //skip files that start with .

    /* build path */
    size_t plen  = strlen(dirpath);
    size_t nlen  = strlen(name);

    char  *full  = malloc(plen + nlen + 2);
    if (!full) { perror("malloc"); exit(EXIT_FAILURE); }

    memcpy(full, dirpath, plen);
    full[plen] = '/';

    memcpy(full + plen + 1, name, nlen + 1);

    struct stat st;
    if (stat(full, &st) < 0) { perror(full); free(full); return; }

    if (S_ISDIR(st.st_mode)) {
        traverse_dir(full);

    } else if (S_ISREG(st.st_mode) && has_suffix(name)) {

        WFD *wfd = read_file(full);

        if (wfd){

            add_file(wfd);
        }
    }
    free(full);
}

static void traverse_dir(const char *path) {

    DIR *d = opendir(path);
    if (!d) { perror(path); return; }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {

        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        process_entry(path, ent->d_name);
    }

    closedir(d);
}

/* JSD  */
/* fi * log2(fi / fm)   where fm = mean freq.  If fi === 0, contribution is 0. */
static double kld_term(double fi, double fm) {
    if (fi <= 0.0) return 0.0;


    assert(fm > 0.0);

    return fi * log2(fi / fm);
}

static double compute_jsd(const WFD *a, const WFD *b) {     //CHECK MATH FOR THIS*******
    double kld1 = 0.0, kld2 = 0.0;

    const WordNode *pa = a->words;
    const WordNode *pb = b->words;

    /* Merge-iterate sorted lists */
    while (pa || pb) {
        double f1, f2, fm;
        int cmp;

        if      (!pa) cmp =  1;
        else if (!pb) cmp = -1;
        else          cmp = strcmp(pa->word, pb->word);

        if (cmp < 0) {
            
            f1 = pa->freq; f2 = 0.0;
            pa = pa->next;
        } else if (cmp > 0) {
            
            f1 = 0.0; f2 = pb->freq;
            pb = pb->next;
        } else {
            
            f1 = pa->freq; f2 = pb->freq;
            pa = pa->next; pb = pb->next;
        }

        fm    = 0.5 * (f1 + f2);
        kld1 += kld_term(f1, fm);
        kld2 += kld_term(f2, fm);
    }

    double jsd = sqrt(0.5 * kld1 + 0.5 * kld2);
    return jsd;
}

//Sorting comparisons
static int cmp_comparison(const void *va, const void *vb) {
    const Comparison *a = va;
    const Comparison *b = vb;
    

    if (b->combined_words != a->combined_words)
        return b->combined_words - a->combined_words;
    return 0;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file|directory> ...\n", argv[0]);
        return EXIT_FAILURE;        //i think
    }

    int had_error = 0;

    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) < 0) {
            perror(argv[i]);
            had_error = 1;
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            traverse_dir(argv[i]);
        } else if (S_ISREG(st.st_mode)) {\

            WFD *wfd = read_file(argv[i]);



            if (wfd) add_file(wfd);
            else had_error = 1;
        } else {

            fprintf(stderr, "%s: not a regular file or directory\n", argv[i]);

            had_error = 1;
        }
    }

    if (g_file_count < 2) {
        fprintf(stderr, "error: fewer than two files collected\n");
    
        for (int i = 0; i < g_file_count; i++) wfd_free(g_files[i]);
        free(g_files);
        return EXIT_FAILURE;
    }

    
    int n    = g_file_count;
    int npairs = n * (n - 1) / 2;

    Comparison *comps = malloc(npairs * sizeof(Comparison));
    if (!comps) { perror("malloc"); exit(EXIT_FAILURE); }

    int idx = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {

            comps[idx].i             = i;
            comps[idx].j             = j;
            comps[idx].combined_words = g_files[i]->total_words   +   g_files[j]->total_words;

            comps[idx].jsd= compute_jsd(g_files[i], g_files[j]);
            idx++;

        }
    }

    qsort(comps, npairs, sizeof(Comparison), cmp_comparison);

    for (int k = 0; k < npairs; k++) {

        int i = comps[k].i, j = comps[k].j;

        printf("%.5f %s %s\n",comps[k].jsd,g_files[i]->path,   g_files[j]->path);
    }

    
    free(comps);

    for (int i = 0; i < g_file_count; i++) wfd_free(g_files[i]);

    free(g_files);

    return had_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
