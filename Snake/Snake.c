#include <p32xxxx.h>
#include <stdio.h>
#include <stdlib.h>

#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_8

#define TRUE 1
#define FALSE 0
#define GAME_COL 16
#define GAME_ROW 8
#define FONT_SIZE 8
#define UP 2
#define DOWN 8
#define LEFT 4
#define RIGHT 6

void Timer(void);
void initPortD(void);
void initPortB(void);
void initPortE(void);
void initPortF(void);
void initPortG(void);
void slowDelay(void);
void delay(int x);
void initLcd(void);
void writeXY(int x,int y,int lcd_cs);
void writeLcd(unsigned int num,int lcd_cs);
void cleanLCD(void);
void print_led(int x);	//not used
int scan_key(void);
void buzzOnce();
void initGame(void);
void gameOver(void);
int checkBounds(int direction);
void drawMatrix(void);
void appendSnake(int direction);
void moveSnake(int direction);
void addFruit(void);
void printToLCD(char* string);
void busy(void);
void initGLcd(void);


int keyVal,column;
int direction = RIGHT; // init snake moving right
int screen = 1;
int game_is_over = FALSE;
int matrix[GAME_ROW][GAME_COL] = {
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};


int tail_i=7;
int tail_j=0;
int head_i=7;
int head_j=1;
int score= 0;

unsigned int DATA_1[FONT_SIZE] = {
	0xff, //0b ########,
	0xff, //0b ########,
	0xff, //0b ########,
	0xff, //0b ########,
	0xff, //0b ########,
	0xff, //0b ########,
	0xff, //0b ########,
	0xff, //0b ########,
};
unsigned int DATA_0[FONT_SIZE] = {
	0x00, //0b 00000000,
	0x00, //0b 00000000,
	0x00, //0b 00000000,
	0x00, //0b 00000000,
	0x00, //0b 00000000,
	0x00, //0b 00000000,
	0x00, //0b 00000000,
	0x00, //0b 00000000,
};
unsigned int DATA_food[FONT_SIZE] = {
	0x00, //0b 00000000,
	0x00, //0b 00000000,
	0x18, //0b 000##000,
	0x3c, //0b 00####00,
	0x3c, //0b 00####00,
	0x18, //0b 000##000,
	0x00, //0b 00000000,
	0x00, //0b 00000000,
};

void main()
{
	unsigned int i ,k ,j ,linetab=0;
	unsigned int DATA_buf[8]; 
	char flag_scan=1;
	unsigned char s;
	int check;
	char s_score[20]  = "score - ";
	char temp_score_num[10];

	initPortD();
	initPortB();
	initPortE();
	initPortF();
	initPortG();
	initGLcd();
	initLcd();
	initGame();

	while(1)
	{
		drawMatrix();
		slowDelay();
		PORTG = 0x00;
		PORTF = 0x07;
		s=scan_key();
		if(s != 0xff)
			flag_scan=s;

		if (game_is_over == TRUE)
		{
			PORTGbits.RG15=1;
			for(i=0;i<80;i++)
			{
				Timer();
			}
			PORTGbits.RG15=0;
			break;
		}

		// prevent snake to go inside itself.
		if(flag_scan=='2')    
		{//up

			if(direction != DOWN )
			{
				direction = UP ;
			}
		}
		if(flag_scan=='4') 
		{//left
			if(direction != RIGHT)
			{
				direction = LEFT;
			}
		}  
		if(flag_scan == '6')
		{//right
			if(direction != LEFT)
			{
				direction = RIGHT;
			}
		}
		if(flag_scan == '8')
		{//down
			if(direction != UP)
			{
				direction = DOWN;
			}
		}

		check = checkBounds(direction);
		if(check == TRUE)
		{
			appendSnake(direction);//append 
			buzzOnce();
			addFruit();//add new rand fruit

			//increase score and print it to lcd
			score ++;
			sprintf(temp_score_num,"%d",score);
			strcat(s_score,temp_score_num);
			printToLCD(s_score);
			strcpy(s_score,"score - ");
			print_led(score);	
		}
		else if(check == FALSE)
		{
			//just move not append
			moveSnake(direction);
		}
		
	}
}

/*************************************************
*the method adds a fruit to the matrix in a      *
*				place random					 *
*************************************************/
void addFruit(void)
{
	int i, j;
	i = rand()%8;
	j = rand()%16;
	while(matrix[i][j] != 0)
	{
		i = rand()%8;
		j = rand()%16;
	}
	matrix[i][j] = -1; //add a fruit in a random place
}

/************************************************
*the method check the current location of snake *
*       if it reaches bounds end game           *
*       Returns :					            *
*       -1 : if game over   		            *
*       0 (FALSE) :	regular move	            *
*       1 (TRUE)  :	eat fruit		            *
*************************************************/
int checkBounds(int direction)
{
	if (direction == UP)
	{
		//check if out of array, < 0 because our matrix is 0-7, so if row < 0 is out of boundary.
		if(head_i-1 <0)
		{
			gameOver();
			return -1;
		}
		//check if hit snake
		else if (matrix[head_i-1][head_j] > 1)
		{
			gameOver();
			return -1;
		}
		else if(matrix[head_i-1][head_j] == -1)
		{
			return  TRUE;//fruit
		}
		else 
			return FALSE; // just move not append
	}
	else if(direction == DOWN)
	{
		//check if out of array, 7 because our matrix is 0-7, so if row > 7 is out of boundary.
		if(head_i+1 > 7)
		{
			gameOver();
			return -1;
		}
		//check if hit snake
		else if (matrix[head_i+1][head_j] > 1)
		{
			gameOver();
			return -1;
		}
		else if(matrix[head_i+1][head_j] == -1)
		{
			return  TRUE ;//fruit
		}
		else 
			return FALSE ; // just move not append
	}
	else if (direction == LEFT)
	{
		//check if out of array, j < 0 because matrix is 0-7, when moving left we must be not below column 0.
		if(head_j-1 <0)
		{
			gameOver();
			return -1;
		}
		//check if hit snake
		else if (matrix[head_i][head_j-1] > 1)
		{
			gameOver();
			return -1;
		}
		else if(matrix[head_i][head_j-1] == -1)
		{
			return  TRUE ;//fruit
		}
		else 
			return FALSE ; // just move not append
	}
	else //direction == RIGHT
	{
		//check if out of array, we have 2 screens, of 8, so it's 0-7 and 8-15; if column > 15 - out of boundary.
		if(head_j+1 > 15)
		{
			gameOver();
			return -1;
		}
		//check if hit snake
		else if (matrix[head_i][head_j+1] > 1)
		{
			gameOver();
			return -1;
		}
		else if(matrix[head_i][head_j+1] == -1)
		{
			return  TRUE ;//fruit
		}
		else 
			return FALSE ; // just move not append
	}
}

/***********************************************
* method to move append snake to next location *
************************************************/
void appendSnake(int direction)
{
	// save current tail values
	int old_tail_i = tail_i;
	int old_tail_j = tail_j;

	// move snake with it's current direction.
	moveSnake(direction);

	// set old tail as new tail 
	matrix[old_tail_i][old_tail_j] = matrix[tail_i][tail_j]+1;
	tail_i = old_tail_i;
	tail_j = old_tail_j;

}

/*************************************************
* method to move only the snake to next location *
**************************************************/
void moveSnake(int direction)
{
	int i, j, max = matrix[tail_i][tail_j];
	//update all of snake counters
	matrix[tail_i][tail_j] = 0;
	for(i=0; i< GAME_ROW; i++)
	{
		for(j=0; j< GAME_COL; j++)
		{
			if(matrix[i][j] > 0)
			{
				matrix[i][j]++;
				if(matrix[i][j] == max)
				{//update tail
					tail_i = i;
					tail_j = j;
				}
			}	
		}
	}
	//update head to next place
	if (direction == UP)
	{
		matrix[head_i-1][head_j] = 1;
		head_i--;
	}
	else if(direction == DOWN )
	{
		matrix[head_i+1][head_j] = 1;
		head_i++;
	}
	else if (direction == LEFT)
	{
		matrix[head_i][head_j-1] = 1;
		head_j--;
	}
	else
	{//RIGHT
		matrix[head_i][head_j+1] = 1;
		head_j++;
	}
}


/**********************************************
* method to end game and print to lcd message *
***********************************************/
void gameOver()
{
	delay(30000);
	printToLCD("GAME OVER LOSER!");
	delay(200000);
	game_is_over = TRUE;

}

/*****************************************
* method to reset the game to begin      *
******************************************/
void initGame(void)
{
	int i,j;
	cleanLCD();

	tail_i=7;
	tail_j=0;
	head_i=7;
	head_j=1;
	for(i=0; i<GAME_ROW; i++)
	{
		for(j=0; j<GAME_COL; j++)
		{
			matrix[i][j] = 0;	
		}
	}

	matrix[tail_i][tail_j] = 2;
	matrix[head_i][head_j] = 1;

	addFruit();
	//print score - 0 first
	printToLCD("score - 0");
	score = 0;
}


/****************************************************
* the method get matrix of game and print it to lcd *
*****************************************************/
void drawMatrix()
{
	int i,j,k;
	//run on all matrix
	for(i=0;i<GAME_ROW;i++)
	{
		//first lcd
		for(j=0;j<GAME_COL/2;j++)
		{
			writeXY(j*8,i,1);
			//if 1 draw snake
			if(matrix[i][j] > 0)
			{
				for(k = 0; k < FONT_SIZE; k++)
				{//print snake on screen   	
					writeLcd(DATA_1[k],1);
				}
			}
			else if(matrix[i][j] == -1)
			{
				for(k = 0; k < FONT_SIZE; k++)
				{//print snake on screen   	
					writeLcd(DATA_food[k],1);
				}
			}
			//draw nothing
			else
			{
				for(k = 0; k < FONT_SIZE; k++)
				{//print nothing on screen   	
					writeLcd(DATA_0[k],1);
				}
			}
		}
		//second lcd
		for(j=GAME_COL/2;j<GAME_COL;j++)
		{
			writeXY(j*8,i,2);
			//if 1 draw snake
			if(matrix[i][j] > 0)
			{
				for(k = 0; k < FONT_SIZE; k++)
				{//print snake on screen   	
					writeLcd(DATA_1[k],2);
				}
			}
			else if(matrix[i][j] == -1)
			{//draw food
				for(k = 0; k < FONT_SIZE; k++)
				{//print snake on screen   	
					writeLcd(DATA_food[k],2);
				}
			}
			//draw nothing
			else
			{
				for(k = 0; k < FONT_SIZE; k++)
				{//print nothing on screen   	
					writeLcd(DATA_0[k],2);
				}
			}
		}
	}
}

/************************************************
* the method get number for delay the system    *
*************************************************/
void delay(int x)
{
	unsigned int i;

	for(i=0;i<x;i++);
}

void slowDelay(void) // moving speed
{
	unsigned int i;

	for(i=0;i<150000;i++);
}

/*****************************************************
* the method set the selected lcd to x and y it gets *
******************************************************/
void writeXY(int x,int y,int lcd_cs)
{
	PORTDbits.RD5 = 0;
	PORTBbits.RB15 = 0;
	PORTF = lcd_cs;
	PORTE =0x40+ x;
	PORTDbits.RD4 = 1;//enable=1
	PORTDbits.RD4 = 0;//enable=0
	delay(64);
	PORTE =0xb8+ y;
	PORTDbits.RD4 = 1;//enable=1
	PORTDbits.RD4 = 0;//enable=0
	delay(64);
	PORTFbits.RF8 = 1;
}

/************************************************
* the method get number and print it on glcd    *
*************************************************/
void writeLcd(unsigned int num,int lcd_cs)
{
	int i;

	PORTDbits.RD5 = 0;
	PORTBbits.RB15 = 1;
	PORTF = lcd_cs;
	PORTE = num;
	PORTDbits.RD4 = 1;//enable=1
	PORTDbits.RD4 = 0;//enable=0
	delay(64);
	PORTFbits.RF8 = 1;
}

/*****************************
* method to clean the glcd   *
******************************/
void cleanLCD(void)
{
	int i,j;
	for(i = 0;i <8;i++)
	{
		for(j = 0;j < 64;j++)
		{
			writeXY(j,i,1);
			writeLcd(0x00,1);
			writeXY(j,i,2);
			writeLcd(0x00,2);
		}
	}	
}

void print_led(int x)
{
	PORTF=4;
	PORTE=x;
	PORTDbits.RD4=1;
	PORTDbits.RD4=0;
	delay(32000);
}

void buzzOnce()
{
	TRISG=0;
	PORTGbits.RG15=1;
	delay(320000);
	PORTGbits.RG15=0;
}

/********************************************************
* method to scan and return the key pressed in keyboard *
*********************************************************/
int scan_key(void)
{
	int RUN_ZERO[4] = {0xee,0xdd,0xbb,0x77}; // pre-setup to define "0 raz, hibur matrix".
	int scan_key_code_ascii[32]={
		0xee,'1',0xde,'2',0xbe,'3',0x7e,'A',
		0xed,'4',0xdd,'5',0xbd,'6',0x7d,'B',
		0xeb,'7',0xdb,'8',0xbb,'9',0x7b,'C',
		0xe7,'*',0xd7,'0',0xb7,'#',0x77,'D'};  

		int i,num_code;
		int column=0;
		int flag=0;  
		
		// init for keypad.
		PORTG = 0x00;
		PORTF = 0x07;

		for(i=0;i<4;i++)
		{
			PORTE = RUN_ZERO[column];
			delay(10);	
			keyVal = PORTB & 0x0F;
			if(keyVal != 0x0f)
			{ 
				flag=1;
				break;
			}
			column++;
			if(column==4)
				column = 0;
		}

		if(flag==1)
		{				  
			num_code=((RUN_ZERO[column]&0xf0)|(keyVal));
			for(i=0;i<32;i+=2)
			{               
				if(num_code==scan_key_code_ascii[i])
				{
					i=scan_key_code_ascii[i+1];  
					break;
				}		 
			}
		}
		else
			i=0xff;
		return(i);
}
/****************************************
* method get string and print it to lcd *
*****************************************/
void printToLCD(char* string)
{
	int i;
	unsigned int portMap,temp_porte;
	char control[2]={0x1,0x80};

	//save state of current portE so we wont lose it in print
	temp_porte = PORTE;

	portMap = TRISE;
	portMap &= 0xFFFFFF00;
	TRISE = portMap;

	//clear screen
	PORTFbits.RF8 = 1;
	PORTDbits.RD5 = 0;//w/r	
	PORTBbits.RB15 = 0;//rs	
	PORTF = 0x00;
	for(i = 0;i < 2;i++)
	{
		PORTE=control[i];
		PORTDbits.RD4=1;//enable=1
		PORTDbits.RD4=0;//enable=0
		busy();
	}


	//print the string
	PORTFbits.RF8 = 1;
	PORTDbits.RD5 = 0;//w/r
	PORTBbits.RB15 = 1;//rs
	PORTF = 0x00;
	for(i = 0;i < strlen(string) ;i++)
	{
		PORTE=string[i];
		PORTDbits.RD4=1;//enable=1
		PORTDbits.RD4=0;//enable=0
		busy();
	} 

	//now return PORTE to old status
	PORTE = temp_porte;
}

void busy(void)
{
	char RD,RS;
	int STATUS_TRISE;
	unsigned int portMap;
	RD=PORTDbits.RD5;
	RS=PORTBbits.RB15;
	STATUS_TRISE=TRISE;
	PORTDbits.RD5 = 1;//w/r
	PORTBbits.RB15 = 0;//rs 
	portMap = TRISE;
	portMap |= 0x80;
	TRISE = portMap;
	do
	{
		PORTDbits.RD4=1;//enable=1
		PORTDbits.RD4=0;//enable=0
	}
	while(PORTEbits.RE7); // BF בדיקה דגל
	PORTDbits.RD5=RD; 
	PORTBbits.RB15=RS;
	TRISE=STATUS_TRISE;   
}

/****************************************************************
*					init methods								*
*****************************************************************/
void initPortB(void)
{
	unsigned int portMap;
	portMap = TRISB;
	portMap &= 0xFFFF7FFF;
	portMap |= 0xF;
	TRISB = portMap;
	AD1PCFG = 0x7fff; //Select PORTB to be digital port input
	CNCONbits.ON = 0;
	CNEN = 0x3C;
	CNPUE = 0x3C;//Set RB0 - RB3 as inputs with weak pull-up
	CNCONbits.ON = 1;
}

void initPortD(void)
{
	unsigned int portMap;
	portMap = TRISD;
	portMap &= 0xFFFFFF4F;
	TRISD = portMap;
	PORTDbits.RD4 = 0;
	PORTDbits.RD5 = 0;

}

void initPortE(void)
{
	unsigned int portMap;
	portMap = TRISE;
	portMap &= 0xFFFFFF00;
	TRISE = portMap;
	PORTE = 0x00;
}

void initPortF(void)
{
	unsigned int portMap;
	portMap = TRISF;
	portMap &= 0xFFFFFEF8;
	TRISF = portMap;
	PORTFbits.RF8 = 1;
}

void initPortG(void)
{	
	unsigned int portMap;
	portMap = TRISG;
	portMap &= 0xFFFFFFFC;
	TRISG = portMap;
	PORTG = 0x00;
}

/************************************************
* the method initialize the graphic lcd to start *
*************************************************/
void initGLcd(void)
{	
	int CONTROL[4] = {0x40,0xB8,0xFF,0x3F};
	int i;

	PORTDbits.RD5 = 0;
	PORTBbits.RB15 = 0;
	PORTF = 0x01;
	PORTDbits.RD7 = 0;
	PORTDbits.RD7 = 1;
	PORTF = 0x02;
	PORTDbits.RD7 = 0;
	PORTDbits.RD7 = 1;
	PORTFbits.RF8 = 1;

	for(i = 0;i < 4;i++)
	{
		PORTE = CONTROL[i];
		PORTF = 0x01;
		PORTDbits.RD4 = 1;//enable=1
		PORTDbits.RD4 = 0;//enable=0
		delay(64);
		PORTF = 0x02;
		PORTDbits.RD4 = 1;//enable=1
		PORTDbits.RD4 = 0;//enable=0
		delay(64);	
	}
	PORTFbits.RF8 = 1;
}

void initLcd(void)
{
	unsigned int portMap;
	char control[6]={0x38,0x38,0x38,0x0e,0x06,0x1};
	int i;
	//control to lcd
	portMap = TRISB;
	portMap &= 0xFFFF7FFF;
	TRISB = portMap;

	AD1PCFG |=0x8000; // RB15-Digital

	portMap = TRISF;
	portMap &= 0xFFFFFEF8;
	TRISF = portMap;
	PORTFbits.RF8 = 1;

	portMap = TRISD;
	portMap &= 0xFFFFFFCF;
	TRISD = portMap;

	PORTDbits.RD5 = 0;//w/r
	PORTBbits.RB15 = 0;//rs

	portMap = TRISE;
	portMap &= 0xFFFFFF00;
	TRISE = portMap;

	PORTF = 0x00;
	for(i = 0;i < 6;i++)
	{
		PORTE=control[i];
		PORTDbits.RD4=1;//enable=1
		PORTDbits.RD4=0;//enable=0
		busy();
	}
	//************************************//
}

void Timer(void)
{
		T1CONbits.ON=0;
		T1CONbits.TGATE=0;
		T1CONbits.TCS=0;//in clock
		T1CONbits.TCKPS0=1;
		T1CONbits.TCKPS1=1;
		T1CONbits.TSYNC=1;
		TMR1=0;
		PR1=0X04FF;
		T1CONbits.ON=1;
		IFS0bits.T1IF=0;
		while(!IFS0bits.T1IF);
}