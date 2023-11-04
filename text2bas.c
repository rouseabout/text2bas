#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* set to 1 for Colour Genie tokenizer, 0 for VZ200/300 */
#define CGENIE  0

typedef struct  tagLINE {
	unsigned short next;
	unsigned short num;
	unsigned char text[254];
}	LINE;

LINE line = {0, 0, {0, }};

unsigned short lineaddr = 0;
unsigned short linenum	= 1;
int flag_tokenize = 1;
int flag_squeeze_blanks = 1;

#if CGENIE
#define EXT ".cas"
#define ADR 0x5801

static char *token[128+26+1] = {
	"END","FOR","RESET","SET","CLS","CMD","RANDOM","NEXT",
	"DATA","INPUT","DIM","READ","LET","GOTO","RUN","IF",
	"RESTORE","GOSUB","RETURN","REM","STOP","ELSE","TRON","TROFF",
	"DEFSTR","DEFINT","DEFSNG","DEFDBL","LINE","EDIT","ERROR","RESUME",
	"OUT","ON","OPEN","FIELD","GET","PUT","CLOSE","LOAD",
	"MERGE","NAME","KILL","LSET","RSET","SAVE","SYSTEM","LPRINT",
	"DEF","POKE","PRINT","CONT","LIST","LLIST","DELETE","AUTO",
	"CLEAR","CLOAD","CSAVE","NEW","TAB(","TO","FN", "USING",
	"VARPTR","USR","ERL","ERR","STRING$","INSTR","CHECK","TIME$",
	"MEM","INKEY$","THEN","NOT","STEP","+","-","*",
	"/","[","AND","OR",">","=","<","SGN",
	"INT","ABS","FRE","INP","POS","SQR","RND","LOG",
	"EXP","COS","SIN","TAN","ATN","PEEK","CVI","CVS",
	"CVD","EOF","LOC","LOF","MKI$","MKS$","MKD$","CINT",
	"CSNG","CDBL","FIX","LEN","STR$","VAL","ASC","CHR$",
	"LEFT$","RIGHT$","MID$","'","","","","",
	"COLOUR","FCOLOU","KEYPAD","JOY","PLOT","FGR","LGR","FCLS",
	"PLAY","CIRCLE","SCALE","SHAPE","NSHAPE","XSHAPE","PAINT","CPOINT",
	"NPLOT","SOUND","CHAR","RENUM","SWAP","FKEY","CALL","VERIFY",
	"BGRD","NBGRD", NULL};
#else
#define EXT ".vz"
#define ADR 0x7ae9

static char *token[128+1] = {
	"END","FOR","RESET","SET","CLS",""/* CMD */,"RANDOM","NEXT",
	"DATA","INPUT","DIM","READ","LET","GOTO","RUN","IF",
	"RESTORE","GOSUB","RETURN","REM","STOP","ELSE","COPY","COLOR",
	"VERIFY","DEFINT","DEFSNG","DEFDBL","CRUN","MODE","SOUND","RESUME",
	"OUT","ON","OPEN","FIELD","GET","PUT","CLOSE","LOAD",
	"MERGE","NAME","KILL","LSET","RSET","SAVE","SYSTEM","LPRINT",
	"DEF","POKE","PRINT","CONT","LIST","LLIST","DELETE","AUTO",
	"CLEAR","CLOAD","CSAVE","NEW","TAB(","TO","FN", "USING",
	"VARPTR","USR","ERL","ERR","STRING$","INSTR","POINT","TIME$",
	"MEM","INKEY$","THEN","NOT","STEP","+","-","*",
	"/","^","AND","OR",">","=","<","SGN",
	"INT","ABS","FRE","INP","POS","SQR","RND","LOG",
	"EXP","COS","SIN","TAN","ATN","PEEK","CVI","CVS",
	"CVD","EOF","LOC","LOF","MKI$","MKS$","MKD$","CINT",
	"CSNG","CDBL","FIX","LEN","STR$","VAL","ASC","CHR$",
	"LEFT$","RIGHT$","MID$","'","","","","",NULL};
#endif

static void strmove(char * dst, char * src)
{
	memmove(dst, src, strlen(src) + 1);
}

void tokenize(int i)
{
	int t;
	int len;

	for( t = 0x80; token[t-0x80]; t++ )
	{
		len = strlen(token[t-0x80]);
		if( len && strncasecmp(line.text+i,token[t-0x80],len) == 0 )
		{
			if( t > 0xFF )
			{
				strmove(line.text + i + 2, line.text + i + len);
				line.text[i+0] = 0xFF;
				line.text[i+1] = t - 0x80;
				line.next -= len - 2;
			}
			else
			{
				strmove(line.text + i + 1, line.text + i + len);
				line.text[i+0] = t;
				line.next -= len - 1;
			}
		}
	}
}

void squeeze_blanks(int i)
{
	int j;

	if( line.text[i] != ' ' )
		return;

	for( j = i+1; j < line.next; j++ )
	{
		if( line.text[j] != ' ' )
			break;
	}
	if( j > i+1 )
	{
		strmove(line.text + i + 1, line.text + j);
		line.next -= j - i - 1;
	}
}

void outline(FILE * out)
{
	int i;
	int len;
	char str = 0;

	if( line.next )
	{
		if( line.next == 1 )			/* avoid empty lines */
		{
			line.text[0] = 'R';
			line.text[1] = 'E';
			line.text[2] = 'M';
			line.text[3] = 0;
			line.next	 = 4;
		}
		for (i = 0; i < line.next; i++)
		{
			if (str && (line.text[i] != str))
				continue;
			if( line.text[i] == str )
			{
				str = 0;
			}
			else
			if( line.text[i] == 0x22 )
			{
				str = 0x22;
			}
			else
			if( line.text[i] == 0x27 )
			{
				str = 0x27;
			}
			else
			if( flag_tokenize )
				tokenize(i);
			if( flag_squeeze_blanks )
				squeeze_blanks(i);
		}

		len = 4 + line.next;
		lineaddr += len;
		line.next = lineaddr;
		line.num = linenum++;
		fwrite(&line, 1, len, out);
		line.next = 0;
	}
}

void outbyte(FILE * out, char c)
{
	if( c == 0 && line.next > 0 && line.text[line.next-1] == 0 )
		return;

	line.text[line.next] = c;

	if( ++line.next == 254 )
		outline(out);
}

int main(int ac, char ** av)
{
	char inpfilename[256];
	char outfilename[256];
	char *p;
	FILE *inp, *out;
	int num = 1;
	char str = 0;
	char c;

	memset(inpfilename, 0, sizeof(inpfilename));
	memset(outfilename, 0, sizeof(outfilename));

    if (ac < 2)
	{
		fprintf(stderr, "usage: text2bas input_filename [output_filename[%s]]\n", EXT);
		fprintf(stderr, "\tconvert text file to basic image (extension %s)\n", EXT);
		return 1;
	}

	strcpy(inpfilename, av[1]);
	if (ac < 3)
	{
		strcpy(outfilename, av[1]);
		p = strrchr(outfilename, '.');
		if (p)
			strcpy(p, EXT);
	}
	else
	{
		strcpy(outfilename, av[2]);
	}

	p = strrchr(outfilename, '.');
	if (!p)
		strcat(outfilename, EXT);

	p = strrchr(outfilename, '/');
	if (!p)
		p = strrchr(outfilename, '/');
	if (!p)
		p = outfilename - 1;

	inp = fopen(inpfilename, "rb");
	if( !inp )
	{
		fprintf(stderr, "%s not found\n", inpfilename);
		return 1;
	}

	out = fopen(outfilename,"wb");
	if( !out )
	{
		fprintf(stderr, "could not create %s\n", outfilename);
		return 1;
	}

#if CGENI
	/* Colour Genie header */
    fputc(0x66, out);               
	/* Basic name is first char of outfilename */
    fputc(toupper(p[1]), out);      
#else
	/* VZ magic header */
    fwrite("VZF0", 1, 4, out);      
	/* write input filename */
	for( p = inpfilename; *p != '.'; p++ )
		*p = toupper(*p);
	memset(p, 0, sizeof(inpfilename) - (int)(p - inpfilename));
    fwrite(inpfilename, 1, 17, out); 
	/* VZ magic value 0xf0 */
    fputc(0xf0, out);
	/* VZ basic load address **/
	fputc(ADR & 0xff, out);
	fputc(ADR >> 8, out);
#endif

    lineaddr = ADR;
	while( !feof(inp) )
	{
		c = fgetc(inp);
		if( str )					/* inside string/comment ? */
		{
			if( c == 0x0d ) 		/* new line ? */
			{
				outbyte(out, 0x00);
				outline(out);
				num = 1;
			}
			else
			{
#if CGENIE
				/* put any char */
                outbyte(out, c);
#else
				/* upper case letters only */
                outbyte(out, toupper(c));
#endif
            }
			if( c == str )			/* end of string/comment ? */
				str = 0;
		}
		else						/* normal text */
		{
			if( num )				/* expecting line number ? */
			{
				if( isdigit(c) )	/* digit follows ?? */
				{
					linenum = c - '0';
					while( isdigit(c = fgetc(inp)) )
					{
						linenum *= 10;
						linenum += c - '0';
					}
					if( c == ' ' )  /* skip one space after the line number */
						c = fgetc(inp);
				}
				num = 0;
			}
			switch (c)
			{
			case 0x0a:				/* no linefeeds */
				num = 1;
                break;
			case 0x0d:				/* convert newline into EOL */
			case 0x1a:				/* convert end of text into EOL */
				outbyte(out, 0x00);
				outline(out);
				num = 1;
				break;
			case 9: 				/* expand tabs */
				outbyte(out, 0x20);
				while( line.next & 7 )
					outbyte(out, 0x20);
				break;
			case 0x22:				/* inside string ? */
			case 0x27:
				str = c;
				outbyte(out, c);
				break;
			case ';':               /* comment line ? */
				str = 0x0d; 		/* skip until next return */
				outbyte(out, c);
				break;
			default:
#if CGENIE
				outbyte(out, c);
#else
                outbyte(out, toupper(c));
#endif
                break;
			}
		}
	}

    line.next = 0;
	fwrite(&line, 1, 2, out);

    fclose(inp);
	fclose(out);

	return 0;
}

