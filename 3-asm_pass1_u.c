/***********************************************************************/
/*  Program Name: 3-asm_pass1_u.c                                      */
/*  This program is the part of SIC/XE assembler Pass 1.	  		   */
/*  The program only identify the symbol, opcode and operand 		   */
/*  of a line of the asm file. The program do not build the            */
/*  SYMTAB.			                                               	   */
/*  2019.12.13                                                         */
/*  2021.03.26 Process error: format 1 & 2 instruction use + 		   */
/***********************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "2-optable.c"

/* Public variables and functions */
#define	ADDR_SIMPLE			0x01
#define	ADDR_IMMEDIATE		0x02
#define	ADDR_INDIRECT		0x04
#define	ADDR_INDEX			0x08

#define	LINE_EOF			(-1)
#define	LINE_COMMENT		(-2)
#define	LINE_ERROR			(0)
#define	LINE_CORRECT		(1)

typedef struct
{
	char		symbol[LEN_SYMBOL];
	char		op[LEN_SYMBOL];
	char		operand1[LEN_SYMBOL];
	char		operand2[LEN_SYMBOL];
	unsigned	code;
	unsigned	fmt;
	unsigned	addressing;	
} LINE;

typedef struct 
{
	char		symb[LEN_SYMBOL];
	unsigned	address;
} SYMTAB;

typedef struct 
{
	char		obcode[6];
} OBCODETAB;



int process_line(LINE *line);
/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT and Instruction information in *line*/

/* Private variable and function */

void init_LINE(LINE *line)
{
	line->symbol[0] = '\0';
	line->op[0] = '\0';
	line->operand1[0] = '\0';
	line->operand2[0] = '\0';
	line->code = 0x0;
	line->fmt = 0x0;
	line->addressing = ADDR_SIMPLE;
}
int	fmt=0;
int process_line(LINE *line)
/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT */
{
	char		buf[LEN_SYMBOL];
	int			c;
	int			state;
	int			ret;
	Instruction	*op;
	
	c = ASM_token(buf);		/* get the first token of a line */
	if(c == EOF)
		return LINE_EOF;
	else if((c == 1) && (buf[0] == '\n'))	/* blank line */
		return LINE_COMMENT;
	else if((c == 1) && (buf[0] == '.'))	/* a comment line */
	{
		do
		{
			c = ASM_token(buf);
		} while((c != EOF) && (buf[0] != '\n'));
		return LINE_COMMENT;
	}
	else
	{
		init_LINE(line);
		ret = LINE_ERROR;
		state = 0;
		while(state < 8)
		{
			switch(state)
			{
				case 0:
				case 1:
				case 2:
					op = is_opcode(buf);
					if((state < 2) && (buf[0] == '+'))	/* + */
					{
						line->fmt = FMT4;
						state = 2;
					}
					else	if(op != NULL)	/* INSTRUCTION */
					{
						strcpy(line->op, op->op);
						line->code = op->code;
						state = 3;
						if(line->fmt != FMT4)
						{
							line->fmt = op->fmt & (FMT1 | FMT2 | FMT3);
						}
						else if((line->fmt == FMT4) && ((op->fmt & FMT4) == 0)) /* INSTRUCTION is FMT1 or FMT 2*/
						{	/* ERROR 20210326 added */
							printf("ERROR at token %s, %s cannot use format 4 \n", buf, buf);
							ret = LINE_ERROR;
							state = 7;		/* skip following tokens in the line */
						}
					}				
					else	if(state == 0)	/* SYMBOL */
					{
						strcpy(line->symbol, buf);
						state = 1;
					}
					else		/* ERROR */
					{
						printf("ERROR at token %s\n", buf);
						ret = LINE_ERROR;
						state = 7;		/* skip following tokens in the line */
					}
					break;	
				case 3:
					if(line->fmt == FMT1 || line->code == 0x4C)	/* no operand needed */
					{
						if(c == EOF || buf[0] == '\n')
						{
							ret = LINE_CORRECT;
							state = 8;
						}
						else		/* COMMENT */
						{
							ret = LINE_CORRECT;
							state = 7;
						}
					}
					else
					{
						if(c == EOF || buf[0] == '\n')
						{
							ret = LINE_ERROR;
							state = 8;
						}
						else	if(buf[0] == '@' || buf[0] == '#')
						{
							line->addressing = (buf[0] == '#') ? ADDR_IMMEDIATE : ADDR_INDIRECT;
							state = 4;
						}
						else	/* get a symbol */
						{
							op = is_opcode(buf);
							if(op != NULL)
							{
								printf("Operand1 cannot be a reserved word\n");
								ret = LINE_ERROR;
								state = 7; 		/* skip following tokens in the line */
							}
							else
							{
								strcpy(line->operand1, buf);
								state = 5;
							}
						}
					}			
					break;		
				case 4:
					op = is_opcode(buf);
					if(op != NULL)
					{
						printf("Operand1 cannot be a reserved word\n");
						ret = LINE_ERROR;
						state = 7;		/* skip following tokens in the line */
					}
					else
					{
						strcpy(line->operand1, buf);
						state = 5;
					}
					break;
				case 5:
					if(c == EOF || buf[0] == '\n')
					{
						ret = LINE_CORRECT;
						state = 8;
					}
					else if(buf[0] == ',')
					{
						state = 6;
					}
					else	/* COMMENT */
					{
						ret = LINE_CORRECT;
						state = 7;		/* skip following tokens in the line */
					}
					break;
				case 6:
					if(c == EOF || buf[0] == '\n')
					{
						ret = LINE_ERROR;
						state = 8;
					}
					else	/* get a symbol */
					{
						op = is_opcode(buf);
						if(op != NULL)
						{
							printf("Operand2 cannot be a reserved word\n");
							ret = LINE_ERROR;
							state = 7;		/* skip following tokens in the line */
						}
						else
						{
							if(line->fmt == FMT2)
							{
								strcpy(line->operand2, buf);
								ret = LINE_CORRECT;
								state = 7;
							}
							else if((c == 1) && (buf[0] == 'x' || buf[0] == 'X'))
							{
								line->addressing = line->addressing | ADDR_INDEX;
								ret = LINE_CORRECT;
								state = 7;		/* skip following tokens in the line */
							}
							else
							{
								printf("Operand2 exists only if format 2  is used\n");
								ret = LINE_ERROR;
								state = 7;		/* skip following tokens in the line */
							}
						}
					}
					break;
				case 7:	/* skip tokens until '\n' || EOF */
					if(c == EOF || buf[0] =='\n')
						state = 8;
					break;										
			}
			if(state < 8)
				c = ASM_token(buf);  /* get the next token */
		}
		return ret;
	}
}


unsigned findSymbolAddress(SYMTAB symtab[], int num_symbols, const char *search_symbol) {
    for (int i = 0; i <= num_symbols; i++) {
        if (strcmp(symtab[i].symb, search_symbol) == 0) {
            // 找到匹配的symbol，取出address的后三位数
            return symtab[i].address % 4096  ;
        }
    }
    return 0;
}


int main(int argc, char *argv[])
{
	int			i, c, line_count,lenghth = 0,pro_count = 0;
	char		buf[LEN_SYMBOL];
	LINE		line;
	int 		hexNumber=0,space=0;
	SYMTAB 		symtab[10000];
	char 		Trecord[60] = " ";
	int         addresstab[10000];
	OBCODETAB	obcode[10000];
	int 		NUM_SYMBOLS = 0;
	if(argc < 2)
	{
		printf("Usage: %s fname.asm\n", argv[0]);
	}
	else
	{
		if(ASM_open(argv[1]) == NULL)
			printf("File not found!!\n");
		else
		{
			for(line_count = 1 ; (c = process_line(&line)) != LINE_EOF; line_count++)
			{
				
				pro_count += 1;
				if(line.op[0]=='S' && line.op[4] == 'T'){
					sscanf(line.operand1, "%x", &hexNumber);
					fmt=hexNumber;
				}	
				if(c == LINE_ERROR)
					printf("%06d   Error\n", line_count);
				else if(c == LINE_COMMENT)
					printf("\n");
				else{
					
					addresstab[line_count] = fmt; 
					printf("%06X  %12s %12s %12s %12s    %X %s\n", fmt, line.symbol, line.op, line.operand1, line.operand2, line.code,obcode[line_count].obcode);
					if(line.symbol[0] != '\0'){
						strcpy(symtab[line_count].symb, line.symbol);
						symtab[line_count].address = fmt;
						NUM_SYMBOLS++;
					}
					if(line.fmt == FMT3){
						fmt += 3;
					}
					else if(line.fmt == FMT4){
						fmt += 4;
					}
					else if(line.fmt == FMT2){
						fmt += 2;
					}
					else if(line.fmt == FMT1){
						fmt += 1;
					}
					
				}
				if(line.fmt == FMT0){
					if(line.op[3] == 'D'){
						fmt += 3;
						space = 3;
					}
					else if(line.op[3] == 'W'){
						fmt += atoi(line.operand1)*3;
						space = atoi(line.operand1)*3;
					}
						
					else if(line.op[3] == 'E'){
						fmt += 1;
						space = 1;
					}
					else if(line.op[3] == 'B'){
						fmt += atoi(line.operand1);
						space = atoi(line.operand1);
					}
						
				}
						
			}
			pro_count--;
			if(line.fmt == FMT3)
				lenghth = fmt - (space) - hexNumber;
			else if (line.fmt == FMT0)
				lenghth = fmt - (space * 2) - hexNumber;
			printf("program length: %X  %d\n",lenghth,pro_count);
			
			for(int coun=1;coun<line_count;coun++){
				if(symtab[coun].symb[0]!='\0'){
					printf("%s : %06X\n",symtab[coun].symb,symtab[coun].address);
				}
			}	
			ASM_close();
		}
		if(ASM_open(argv[1]) == NULL)
			printf("File not found!!\n");
		else
		{
			int Taddress = addresstab[1];
			int Tfmt = 0;
			int Tcount = 0;
			unsigned TA = 0;
			int pc = 0;
			int complement = 4096;
			unsigned disp = 0;
			char dispchar[3];
			char wordspace[6];
			char wordint[6];
			char bytespace[6];
			char  byteint[6];
			int j = 0;
			for(line_count = 1 ; (c = process_line(&line)) != LINE_EOF; line_count++){
				unsigned	recode = line.code;
				char	hex_string[2];
				if(line_count == 1){
					printf("H%-6s%06X%06X\n",symtab[1].symb,symtab[1].address,lenghth);
				}
				if(c == LINE_ERROR)
					continue;
				else if(c == LINE_COMMENT)
					continue;
				else {
					if(line_count > 1){
						if(line.fmt == FMT3 && line.operand1[0] != '\0'){
							TA = findSymbolAddress(symtab, pro_count, line.operand1);
							pc = Tfmt + 3;
							recode += 3;
							if(pc > TA)
								disp = TA + (complement-pc);
							else
								disp = TA - pc;
							sprintf(hex_string, "%X", recode);
							strcpy(obcode[line_count].obcode, hex_string);
							obcode[line_count].obcode[2] = '2';
							sprintf(dispchar, "%X", disp);
							if(strlen(dispchar) == 2){
								obcode[line_count].obcode[3] = '0';
								strcpy(obcode[line_count].obcode + 4, dispchar);
							}
							else if(strlen(dispchar) == 1){
								obcode[line_count].obcode[3] = '0';
								obcode[line_count].obcode[4] = '0';
								strcpy(obcode[line_count].obcode + 5, dispchar);
							}
							else if(strlen(dispchar) == 3){
								strcpy(obcode[line_count].obcode + 3, dispchar);
							}
							//printf("%s %s %s %d\n",line.op,line.operand1 ,obcode[line_count].obcode,Tcount);
							strcpy(Trecord + Tcount, obcode[line_count].obcode);
							Tcount += 6;
						}
						else if(line.fmt == FMT0 && line.op[0] == 'B'){
							
							if(line.operand1[0] == 'X'){
								j = 0;
								for(int i = 2;i<strlen(line.operand1)-1;i++){
									bytespace[j] = line.operand1[i];
									j++;
								}
								strcpy(Trecord + Tcount, bytespace);
								Tcount += strlen(bytespace);
							}
							else if(line.operand1[0] == 'C'){
								for(int i = 2;i<strlen(line.operand1)-1;i++){
									sprintf(&byteint[i * 2], "%02X", line.operand1[i]);
								}
								strcpy(Trecord + Tcount, bytespace);
								Tcount += strlen(bytespace);
							}
								
						}
						else if(line.fmt == FMT0 && line.op[0] == 'W'){
							int wordlength = strlen(line.operand1);
							int i;
							for(i = 0;i < (6 - wordlength);i ++){
								wordspace[i] = '0';
							}
							strcpy(wordspace + i, line.operand1);
							strcpy(Trecord + Tcount, wordspace);
							Tcount += 6;
							
						}
						else if(line.fmt == FMT3 && line.operand1[0] == '\0'){
							recode += 3;
							sprintf(hex_string, "%X", recode);
							strcpy(obcode[line_count].obcode, hex_string);
							obcode[line_count].obcode[2] = '0';
							obcode[line_count].obcode[3] = '0';
							obcode[line_count].obcode[4] = '0';
							obcode[line_count].obcode[5] = '0';
							//printf("%s %s  %d\n",line.operand1 ,obcode[line_count].obcode,Tcount);
							strcpy(Trecord + Tcount, obcode[line_count].obcode);
							Tcount += 6;
						}
						
					}
					
					if(line.fmt == FMT3){
						Tfmt += 3;
					}
					else if(line.fmt == FMT2){
						Tfmt += 2;
					}
					else if(line.fmt == FMT1){
						Tfmt += 1;
					}
					else if(line.fmt == FMT0){
						if(line.op[3] == 'D')
							Tfmt += 3;
						else if(line.op[3] == 'E')
							Tfmt += 1;
						else if(line.op[3] == 'B' && Tfmt > 0){
							printf("T%06X%02X%s\n",Taddress,Tfmt,Trecord);
							memset(Trecord, '\0', sizeof(Trecord));
							Taddress = addresstab[line_count+1];	
							Tfmt = 0;
							Tcount = 0;
						}
						else if(line.op[3] == 'W' && Tfmt > 0){
							printf("T%06X%02X%s\n",Taddress,Tfmt,Trecord);
							memset(Trecord, '\0', sizeof(Trecord));
							Taddress = addresstab[line_count+1];	
							Tfmt = 0;
							Tcount = 0;
						} 	
					}
					if(Tcount == 60){
						printf("T%06X%02X%s\n",Taddress,Tfmt,Trecord);	
						Taddress = addresstab[line_count+1];
						memset(Trecord, '\0', sizeof(Trecord));
						Tfmt = 0;
						Tcount = 0;
					}
				}
				if(line_count == pro_count){
					Taddress = addresstab[line_count];
					printf("T%06X%02X%s\n",Taddress,Tfmt,Trecord);
					memset(Trecord, '\0', sizeof(Trecord));
					Tfmt = 0;
					Tcount = 0;
				}
				
				
			}

			printf("E%06X\n",lenghth);
			ASM_close();
		}
	}
}
