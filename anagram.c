/*
  Play against engine ==> feed database

  
  
  Create a strategy engine
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LEN       50
#define MAX_ROWS  400000

#define MAX_WORDS 10

#define max(a, b) (a > b) ? a : b
#define min(a, b) (a < b) ? a : b

//
typedef unsigned long long uint64;

//
typedef struct node_s { int state; char word[MAX_LEN]; int len; struct node_s *next; } node_t;
 
//
int score;
int maxlen = 5, minlen = 5;
int nb_inserts = 0;
int nb_fail_gen = 0;
int nb_collisions = 0;

//
int nb_words;
node_t *words[MAX_WORDS];

//
int randxy(int a, int b)
{  return a + (rand() % (a - b)); }

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
      strncpy(d[h].word, str, len);
      d[h].len = len;
      d[h].next = NULL;
    }
  else
    {
      node_t *tmp = d[h].next;
      node_t *new_n = malloc(sizeof(node_t));

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


//
void gen_perm(node_t *d, char *str, int b, int e)
{
  int pos, found = 0;
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
      for (int i = b; i < e; i++)
	{
	  swap(str + b, str + i);
	  gen_perm(d, str, b + 1, e);
	  swap(str + b, str + i);
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
int main(int argc, char **argv)
{
  char *str = NULL;
  node_t *dico = load_dico("words.txt");
  
  nb_words = 0;
  
  srand(getpid());
  
  printf("\nControl characters:\n"
	 "\t! : forfait the game\n"
	 "\t+ : request a hint (loose two points)\n\n");
  
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
	  
	  gen_perm(dico, str, 0, nb_chars);
	  
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
		  
		  score -= 1;
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
