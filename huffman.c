#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define SYMBOLS 256

typedef struct tree
{
	char symbol;
	int frequency;
	struct tree* left;
	struct tree* right;
}
Tree;

typedef struct plot
{
	Tree* tree;
	struct plot* next;
}
Plot;

typedef struct forest
{
	Plot* first;
}
Forest;

typedef enum { READ, WRITE } mode;

typedef struct
{
    // 8-bit buffer
    unsigned char buffer;
    FILE* stream;
    // current location in buffer
    unsigned char ith;
    
    // number of valid bits in file's second-to-last byte
    unsigned char zth;

    // I/O mode
    mode io;

    // size of file
    off_t size;
}
Huffile;

typedef struct block
{
	char ch;
	int freq;
}
block;

typedef struct a
{
	char ch;
	int value;
	int ith;
	int freq;
	struct a* next;
}compressed;

Forest* mkforest(void);
Tree* pick(Forest* f);
bool plant(Forest* f, Tree* t);
bool rmforest(Forest* f);
Huffile* hfopen(const char* path, const char* mode);
bool trace(Tree* , Huffile*);
bool hfclose(Huffile* hf);
Forest* hftree(Forest* f);
block* hash(int ch, block* HASHTABLE);
bool print(Tree* t, int , int);
int bread(Huffile* hf);
Tree* mktree(void);
void rmtree(Tree* t);
compressed* create(void);
compressed* compress(Tree* t, int buffer, int i, compressed* first);
bool bwrite(int b, Huffile* hf);

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("Usage: %s [infile] [compressed_file]\n", argv[0]);
		return 1;
	}
	
	FILE* ifp = fopen(argv[1], "r");
	Huffile* hf = hfopen(argv[2], "r"); 
	block *table = (block*)malloc(sizeof(block)*SYMBOLS);
	compressed* c = create();
	c->freq = 0;
	Forest* f = mkforest();
	char ch; int count=0;

	while((ch = fgetc(ifp)) != EOF)
	{
		table = hash((int)ch, table);
		if ((int)ch < 0)
			printf("%c", ch);
	}
	fseek(ifp, 0, SEEK_SET);

	for (int i=0; i<SYMBOLS; i++)
	{
		if (table[i].freq)
		count++;
	}

	for (int i=0; i<SYMBOLS; i++)
	{
		Tree* node = mktree();
		node->symbol = table[i].ch;
		node->frequency = table[i].freq;
		plant(f, node);
	}

	/*Plot* temp = f->first;
	while(temp != NULL)
	{
		printf("%c (%03d) -->  %d\n", temp->tree->symbol, (int)temp->tree->symbol, temp->tree->frequency);
		temp = temp->next;
	}*/
	
	// makes huffman tree
	f = hftree(f);

	//print(f->first->tree, 0, 0);
	c = compress(f->first->tree, 0, 0, c);
/*	compressed* node = c;
	while (node != NULL)
	{
		printf("%05d(%c) -> ", node->freq, node->ch);
		for (int j=0; j<node->ith; j++)
		{
			int bit = (node->value & 1 << j) ? 1 : 0;
			printf("%d", bit);
		}
		node = node->next;
		printf("\n");
	}
	printf("\n");
	
	compressed* node = c;
	int bit = 0;
	while((ch = fgetc(ifp)) != EOF)
	{
		node = c;
		while(ch != node->ch)
		{
			node = node->next;
		}
		for (int j=0; j<node->ith; j++)
		{
			bit = (node->value & 1 << j) ? 1 : 0;
			bwrite(bit, hf);
		}
	}
*/
	trace(f->first->tree, hf);

	hfclose(hf);
	rmforest(f);
	free(table);
	fclose(ifp);
	return 0;
}

block* hash(int ch, block* HASHTABLE)
{
	HASHTABLE[ch].freq += 1;
	HASHTABLE[ch].ch = (char)ch;

	return HASHTABLE;
}

Tree* mktree(void)
{
	Tree* tree = malloc(sizeof(Tree));
	if (tree == NULL)
	{
		return NULL;
	}

	tree->symbol = 0x00;
	tree->frequency = 0;
	tree->left = NULL;
	tree->right = NULL;

	return tree;
}

void rmtree(Tree* t)
{
	if (t == NULL)
	{
		return;
	}

	if (t->left != NULL)
	{
		rmtree(t->left);
	}

	if (t->right != NULL)
	{
		rmtree(t->right);
	}

	free(t);
}

Forest* mkforest(void)
{
	Forest *f = malloc(sizeof(Forest));
	if (f == NULL)
	{
		return NULL;
	}

	f->first = NULL;

	return f;
}

Tree* pick(Forest* f)
{
	if (f == NULL)
	{
		return NULL;
	}

	if (f->first == NULL)
	{
		return NULL;
	}

	Tree* tree = f->first->tree;

	Plot* plot = f->first;
	f->first = f->first->next;
	free(plot);

	return tree;
}

bool plant(Forest* f, Tree* t)
{
	if (f == NULL || t == NULL)
	{
		return false;
	}

	if (t->frequency == 0)
	{
		return false;
	}

	// prepare tree for forest
	Plot* plot = malloc(sizeof(Plot));
	if (plot == NULL)
	{
		return false;
	}
	plot->tree = t;

	// find tree's position in forest
	Plot* trailer = NULL;
	Plot* leader = f->first;
	while (leader != NULL)
	{
		// keep trees sorted first by frequency then by symbol
		if (t->frequency < leader->tree->frequency)
		{
			break;
		}
		else if (t->frequency == leader->tree->frequency
			&& t->symbol < leader->tree->symbol)
		{
			break;
		}
		else
		{
			trailer = leader;
			leader = leader->next;
		}
	}

	// either insert plot at start of forest
	if (trailer == NULL)
	{
		plot->next = f->first;
		f->first = plot;
	}

	// or insert plot in middle or at end of forest
	else
	{
		plot->next = trailer->next;
		trailer->next = plot;
	}

	return true;
}

bool rmforest(Forest* f)
{
	if (f == NULL)
	{
		return false;
	}

	while (f->first != NULL)
	{
		Plot* plot = f->first;
		f->first = f->first->next;
		rmtree(plot->tree);
		free(plot);
	}

	free(f);
	return true;
}

Forest* hftree(Forest* f)
{
	// ensures forest is not null
	if (f == NULL)
	{
		return NULL;
	}

	if (f->first == NULL)
    {
        return NULL;
    }
    // ensures that forest have more than one tree
    if (f->first->next == NULL)
    {
        return f;
    }

	// extract 2 minimum weighted tree

	Plot* f_plot = f->first;
	Plot* s_plot = f->first->next;

	// make new tree pointing towards above two trees and have frequency equal to the sum

	Tree* new_tree = mktree();
	
	if (new_tree == NULL)
	{
		return NULL;
	}

	new_tree->frequency = f_plot->tree->frequency + s_plot->tree->frequency;
	new_tree->left = f_plot->tree;
	new_tree->right = s_plot->tree;
	
	for (int i=0; i<2; i++)
	{
		pick(f);
	}
	
	plant(f, new_tree);

	return hftree(f);
}

bool print(Tree* t, int buffer, int i)
{
	Tree* temp = t;
	unsigned mask = 1 << i;
	if (temp->left == NULL && temp->right == NULL)
	{
		printf("%05d(%c) -> ", temp->frequency, temp->symbol);
		for (int j=0; j<i; j++)
		{
			int bit = (buffer & 1 << j) ? 1 : 0;
			printf("%d", bit);
		}
		printf("\n");
		return true;
	}
	else
	{
		if (temp->left != NULL)
		{
			buffer |= mask;
			print(temp->left, buffer, i+1);
		}
		if (temp->right != NULL)
		{
			buffer &= ~mask;
			print(temp->right, buffer, i+1);
		}
	}
	return true;
}

bool trace(Tree* t, Huffile* hf)
{
	if (t == NULL || hf == NULL)
	{
		return false;
	}
	Tree* temp = t;
	int bit;
	while(bit != EOF)
	{
		while(temp->left!=NULL && temp->right!=NULL)
		{
			bit = bread(hf);
			if (bit==1)
				temp = temp->left;
			else
				temp = temp->right;
		}
		printf("%c", temp->symbol);
		temp = t;
	}
	printf("\n");
	return true;
}

int bread(Huffile* hf)
{
	if (hf == NULL)
	{
		return EOF;
	}

	// check for file's end
	if (ftell(hf->stream) == hf->size - 1)
	{
		if (hf->ith == hf->zth)
		{
			return EOF;
		}
	}

	// read another buffer's worth of bits if necessary
	if (hf->ith == 0)
	{
		if (fread(&hf->buffer, sizeof(hf->buffer), 1, hf->stream) != 1)
		{
			return EOF;
		}
	}

	// prepare mask
	unsigned char mask = 1 << (7 - hf->ith);

	// advance to next bit
	hf->ith = (hf->ith + 1) % 8;

	// grab i'th bit
	return (hf->buffer & mask) ? 1 : 0;
}

Huffile* hfopen(const char* path, const char* mode)
{
	if (path == NULL || mode == NULL)
	{
		return NULL;
	}

	if (strcmp(mode, "r") != 0 && strcmp(mode, "w") != 0)
	{
		return NULL;
	}

	Huffile* hf = malloc(sizeof(Huffile));
	if (hf == NULL)
	{
		return NULL;
	}

	hf->stream = fopen(path, mode);
	if (hf->stream == NULL)
	{
		free(hf);
		return NULL;
	}

	hf->io = (strcmp(mode, "w") == 0) ? WRITE : READ;

	// remember size and final buffer's index if reading
	if (hf->io == READ)
	{
		// fast-forward to end of file
		if (fseek(hf->stream, -1, SEEK_END) != 0)
		{
			fclose(hf->stream);
			free(hf);
			return NULL;
		}

		// remember size
		if ((hf->size = ftell(hf->stream) + 1) == 0)
		{
			fclose(hf->stream);
			free(hf);
			return NULL;
		}

		// remember final buffer's index
		if (fread(&hf->zth, sizeof(hf->zth), 1, hf->stream) != 1)
		{
			fclose(hf->stream);
			free(hf);
			return NULL;
		}

		if (fseek(hf->stream, 0, SEEK_SET) != 0)
		{
			fclose(hf->stream);
			free(hf);
			return NULL;
		}
	}

	// initialize state
	hf->buffer = 0x00;
	hf->ith = 0;

	return hf;
}

bool hfclose(Huffile* hf)
{
	if (hf == NULL)
	{
		return false;
	}

	if (hf->stream == NULL)
	{
		return false;
	}

	// be sure final buffer and its index get written out if writing
	if (hf->io == WRITE)
	{
		// write out final buffer if necessary
		if (hf->ith > 0)
		{
			if (fwrite(&hf->buffer, sizeof(hf->buffer), 1, hf->stream) != 1)
			{
				return false;
			}
		}

		// write out final buffer's index
		if (fwrite(&hf->ith, sizeof(hf->ith), 1, hf->stream) != 1)
			return false;
	}

	if (fclose(hf->stream) != 0)
	{
		return false;
	}

	free(hf);
	return true;
}

compressed* create(void)
{
	compressed* node = (compressed*)malloc(sizeof(compressed*));
	node->next = NULL;
	node->freq = 0;
	node->value = 0;
	node->ith = 0;
	return node;
}

compressed* fit(compressed* first, compressed* node)
{
	compressed* temp1 = first;
	compressed* temp2 = NULL;

	while(temp1 != NULL)
	{
		if (temp1->freq <= node->freq)
		{
			break;
		}
		temp2 = temp1;
		temp1 = temp1->next;
	}

	if (temp2 == NULL)
	{
		first = node;
		node->next = temp1;
	}
	else
	{
		temp2->next = node;
		node->next = temp1;
	}
	return first;
}

compressed* compress(Tree* t, int buffer, int i, compressed* first)
{
	Tree* temp = t;
	unsigned mask = 1 << i;
	if (temp->left == NULL && temp->right == NULL)
	{
		compressed* node = create();
		node->ith = i;
		node->ch = temp->symbol;
		node->value = buffer;
		node->freq = temp->frequency;
		first = fit(first, node);
		return first;
	}
	else
	{
		if (temp->left != NULL)
		{
			buffer |= mask;
			first = compress(temp->left, buffer, i+1, first);
		}
		if (temp->right != NULL)
		{
			buffer &= ~mask;
			first = compress(temp->right, buffer, i+1, first);
		}
	}
	return first;
}

bool bwrite(int b, Huffile* hf)
{       
	if (hf == NULL)
	{
		return false;
	}   

	if (hf->io != WRITE)
	{
		return false; 
	}

	// ensure bit is valid
	if (b != 0 && b != 1)
	{
		return false;
	}

	// prepare mask
	unsigned mask = 1 << (7 - hf->ith);

	// set i'th bit in buffer if 1
	if (b == true)
	{
		// set i'th bit in buffer
		hf->buffer |= mask;
	}

	// clear i'th bit in buffer if 0
	else
	{
		// clear i'th bit in buffer
		hf->buffer &= ~mask;
	}

	// advance to next bit
	hf->ith = hf->ith + 1;

	// write buffer to disk if full
	if (hf->ith == 8)
	{
		if (fwrite(&hf->buffer, sizeof(hf->buffer), 1, hf->stream) == 1)
		{
			hf->buffer = 0x00;
			hf->ith = 0;
		}
		else
		{
			return false;
		}
	}
	return true;
}