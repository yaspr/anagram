#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "rdtsc.h"

#define MAX_LEN       50
#define MAX_ROWS   40000

#define MAX_WORDS 10

#define max(a, b) (a > b) ? a : b
#define min(a, b) (a < b) ? a : b

//
typedef unsigned long long uint64;

//
typedef struct node_s { int state; int nb_links; char word[MAX_LEN]; int len; struct node_s *next; } node_t;
 
//
int score;
int nb_inserts = 0;
int nb_fail_gen = 0;
int nb_collisions = 0;
int maxlen = 5, minlen = 5;

//
int nb_words;
node_t *words[MAX_WORDS];

//
int randxy(int a, int b)
{ return (rand() % (b - a + 1)) + a; }

//
int fact(int n)
{
  if (n == 0 || n == 1)
    return 1;
  else
    return fact(n - 1) * n;
}

//
int is_upper(char c)
{ return (c >= 'A' && c <= 'Z'); }

//
int is_lower(char c)
{ return (c >= 'a' && c <= 'z'); }

//
int is_alpha(char c)
{ return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

//
void to_lower(char *str)
{
  int i = 0;
  
  while (str[i])
    {
      if (is_alpha(str[i]))
	if (is_upper(str[i]))
	  str[i] += ' ';

      i++;
    }
}

//Bernstein's algorithm
unsigned long hash(unsigned char *str)
{
  int c;
  unsigned long hash = 5381;
  
  while (c = *str++)
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  
  return hash;
}

//
void swap(char *x, char *y)
{
  char tmp = *x;
  *x = *y;
  *y = tmp;
}

//
int insert_node(node_t *d, char *str)
{
  int len = strlen(str);
  unsigned h = hash(str) % MAX_ROWS;
  
  //If spot is free
  if (d[h].state == 0)
    {
      d[h].state = 1;
      d[h].nb_links = 0;
      strncpy(d[h].word, str, len);
      d[h].len = len;
      d[h].next = NULL;
    }
  else
    {
      node_t *tmp = d[h].next;
      node_t *new_n = malloc(sizeof(node_t));

      d[h].nb_links++;
      
      new_n->state = 1;
      strncpy(new_n->word, str, len);
      new_n->len = len;
      new_n->next = NULL;

      while (tmp && tmp->next)
	tmp = tmp->next;
      
      if (tmp)
	tmp->next = new_n;
      else
	d[h].next = new_n;
      
      nb_collisions++;
    }
  
  maxlen = max(maxlen, len);
  minlen = min(minlen, len);

  nb_inserts++;
}

//
node_t *load_dico(char *fname)
{
  char ret = 1, tmp[MAX_LEN];
  FILE *fd = fopen(fname, "rb");
  node_t *d = malloc(sizeof(node_t) * MAX_ROWS);
  
  if (fd)
    {
      //Load each word in the dictionary
      while (ret != EOF)
	{
	  ret = fscanf(fd, "%s\n", tmp);

	  to_lower(tmp);
	  
	  insert_node(d, tmp);
	}
      
      fclose(fd);
      
      return d;
    }
  else
    return NULL;
}

//Lookup a string in the dictionary
node_t *lookup(node_t *d, char *str)
{
  unsigned h = hash(str) % MAX_ROWS;
  node_t *n = &d[h];
  int found = 0, len = strlen(str);

  while (!found && n)
    {
      if (n->len == len)
	found = !strncmp(n->word, str, len);

      if (!found)
	n = n->next;
    }

  if (found && n)
    return n;
  else
    return NULL;
}


//Recursive version
void gen_perm1(node_t *d, char *str, int b, int e)
{
  int found = 0;
  node_t *tmp = NULL;
  
  if (b == e)
    {
      //Look up word in dictionary
      if (tmp = lookup(d, str))
	{
	  for (int i = 0; i < nb_words && !found; i++)
	    found = (words[i] == tmp);

	  if (!found && nb_words < MAX_WORDS)
	    words[nb_words++] = tmp;
	}
    }
  else
    {
      //
      for (int i = b; i < e; i++)
	{
	  swap(str + b, str + i);
	  gen_perm1(d, str, b + 1, e);
	  swap(str + b, str + i);
	}
    }
}

//Heap's algorithm
void gen_perm2(node_t *d, char *str, int n)
{
  int found = 0;
  node_t *tmp = NULL;

  if (n == 1)
    {      
      //Look up word in dictionary
      if (tmp = lookup(d, str))
	{
	  for (int i = 0; i < nb_words && !found; i++)
	    found = (words[i] == tmp);
	  
	  if (!found && nb_words < MAX_WORDS)
	    words[nb_words++] = tmp;
	}
    }
  else
    {
      for (int i = 0; i < n - 1; i++)
	{
	  gen_perm2(d, str, n - 1);

	  if (n & 1) //Odd
	    swap(str, str + n - 1);
	  else       //Even
	    swap(str + i, str + n - 1);
	  
	  gen_perm2(d, str, n - 1);
	}
    }
}

//Iterative version of the Heap's permutaion algorithm - much faster
void gen_perm3(node_t *d, char *str)
{
  node_t *tmp = NULL;
  int n = strlen(str);
  int c[n], found = 0;
  
  for (int i = 0; i < n; i++)
    c[i] = 0;
  
  for (int i = 0; i < n;)
    {
      if  (c[i] < i)
	{
	  if (i % 2)
	    swap(&str[c[i]], &str[i]);
	  else
	    swap(&str[0], &str[i]);
	  
	  //Look up word in dictionary
	  if (tmp = lookup(d, str))
	    {
	      for (int i = 0; i < nb_words && !found; i++)
		found = (words[i] == tmp);
	  
	      if (!found && nb_words < MAX_WORDS)
		words[nb_words++] = tmp;
	    }	      
	  
	  c[i]++;
	  i = 0;
	}
      else
	{
	  c[i] = 0;
	  i++;
        }
    }
}

//
void gen_perm_n(node_t *d, char *str, int n)
{
  node_t *tmp = NULL;
  int len = strlen(str);
  int c[len], found = 0, j = 0;
  
  for (int i = 0; i < len; i++)
    c[i] = 0;
  
  for (int i = 0; i < len && !found; )
    {
      if  (c[i] < i)
	{
	  if (i % 2)
	    swap(&str[c[i]], &str[i]);
	  else
	    swap(&str[0], &str[i]);
	    
	  found = (j == n); j++;
	    
	  c[i]++;
	  i = 0;
	}
      else
	{
	  c[i] = 0;
	  i++;
        }
    }
}

//
void dump_db(char *str)
{
  FILE *fd = fopen("db.txt",  "a");

  fprintf(fd, "%s: ", str);

  for (int i = 0; i < nb_words; i++)
    fprintf(fd, "%s; ", words[i]->word);

  fprintf(fd, "%d;\n", score);
  
  fclose(fd);
}

//
int v1(node_t *dico)
{
  char *str = NULL;
  double after, before;
  
  nb_words = 0;
  
  srand(getpid());
  
  printf("WARNING: generating a sequence may take a while when going over 9 characters.\n\n");
  
  if (dico)
    {
      int nb_chars;
      int hint = 0;
      
      printf("How many characters? ");
      scanf("%d", &nb_chars);
      
      char in[nb_chars];
      str = malloc(sizeof(char) * nb_chars);
      
      score = nb_chars;
      
      do
	{
	  for (int i = 0; i < nb_chars; i++)
	    str[i] = randxy('a', 'z');
	  
	  str[nb_chars] = 0;
	  
	  //gen_perm(dico, str, 0, nb_chars);
	  //gen_perm2(dico, str, strlen(str));	  
	  gen_perm3(dico, str);
	  
	  nb_fail_gen++;
	}
      while (!nb_words);
      
      printf("Characters: %s\n\n", str);
      
      int found = 0;
      
      do
	{
	  printf("Word (score: %2d)\t: ", score);
	  scanf("%s", in);

	  to_lower(in);
	  
	  if (strlen(in) == nb_chars)
	    {
	      for (int i = 0; i < nb_words && !found; i++)
		found = !strncmp(in, words[i]->word, nb_chars);
	    }
	  else
	    if (in[0] == '!')
	      {
		score = 1;
	      }
	    else
	      if (in[0] == '+')
		{
		  if (hint < words[0]->len)
		    printf(" HINT %2d: ", hint);
		  
		  for (int i = 0; i < words[0]->len; i++)
		    if (i <= hint)
		      printf("%c", words[0]->word[i]);
		    else
		      printf("?");
		  
		  printf("\n\n");

		  hint++;
		  
		  score--;
		}
	  
	  score -= !found;
	}
      while (!found && score > 0);

      printf("\nAll possible words:\n");
      
      for (int i = 0; i < nb_words; i++)
	printf(" %2d: %s\n", i, words[i]->word);
      
      if (found && score)
	printf("\n ### Win (score: %d/%d) ###\n", score, nb_chars);
      
      if (score == 0)
	printf("\n ### Game Over (score: 0/%d) ###\n", nb_chars);
    }
  
  dump_db(str);
  
  return 0;
}

//
int v2(node_t *dico)
{
  srand(getpid());

  int h = randxy(0, MAX_ROWS), found = 0, hint = 0;
  node_t *tmp = &dico[h];
  char in[tmp->len], str[tmp->len];
  
  int w  = randxy(0, 1);  //Go through linked list (1) or not (0)
  int hh = randxy(0, tmp->nb_links); 

  while (w && hh && tmp)
    {
      tmp = tmp->next;
      hh--;
    }

  strcpy(str, tmp->word);
  
  gen_perm_n(dico, str, randxy(0, fact(tmp->len)));
  
  score = tmp->len;

  printf("Characters: %s\n\n", str);

  do
    {
      printf("Word (score: %2d)\t: ", score);
      scanf("%s", in);
      
      to_lower(in);
      
      if (strlen(in) == tmp->len)
	{
	  found = !strncmp(tmp->word, in, tmp->len);
	}
      else
	if (in[0] == '!')
	  {
	    score = 1;
	  }
	else
	  if (in[0] == '+')
	    {
	      if (hint < tmp->len)
		printf(" HINT %2d: ", hint);
	      
	      for (int i = 0; i < tmp->len; i++)
		if (i <= hint)
		  printf("%c", tmp->word[i]);
		else
		  printf("?");
	      
	      printf("\n\n");
	      
	      hint++;
	      
	      score--;
	    }
      
	  score -= !found;
    }
  while (!found && score);

  printf("\nCorrect answer: %s\n", tmp->word);
  
  if (found && score)
    printf("\n ### Win (score: %d/%d) ###\n", score, tmp->len);
  
  if (score == 0)
    printf("\n ### Game Over (score: 0/%d) ###\n", tmp->len);

  nb_words = 1;
  words[0] = tmp;
  
  dump_db(str);
  
  return 0;
}

//
int main(int argc, char **argv)
{
  node_t *dico = load_dico("words.txt");

  printf("\nControl characters:\n"
	 "\t! : forfait the game\n"
	 "\t+ : request a hint (lose two points)\n\n");
	 
  v2(dico);
  
  return 0;
}
