#define _CRT_SECURE_NO_WARNINGS
#define PI 3.14159265358979323846

#include <stdio.h>
#include <Windows.h>
#include <locale.h>
#include <math.h> //standart math function 
#include <ctype.h> //functions to check if string contains a digit or a symbol (isdigit, isalpha)
#include <stdlib.h> //functions to convert str into digits

/*
план выполнения работы:
1) реализовать стек и очередь
2) реализовать структуру "токенов" в замену value для стек и очереди
3) реализовать токенизацию - разбиение по токенам входящей строки, чтобы привычное выражение типа 2+2 превратилось для компьютера в токен токен токен
4) реализовать алгоритм сортировочной станции Дейкстры, который бы обрабатывал выражение в токенизированном виде
5) реализовать вычисление выражения в польской нотации
*/
/*
устройство работы сортировочной станции Дейкстры:
сочетает в себе использование двух структур: очереди и стека, очередь будет являться буфером в который будет собираться изначальное выражение в инфиксной нотации в постфиксную
стек же будет стеком операторов, то есть будет содержать ТОЛЬКО операторы и ничего более
как обрабатывается запись: 
1) операнды (числа, переменные) сразу помещаются с очередь
2) функции (sin, cos, tg...) кладутся в стек и ждут своей закрывающей скобки
3) операторы попадают в стек, перед этим каждый оператор перед тем как попасть туда сравнивается с тем что уже находится на вершине стека: 
	- в случае >= приоритета или лево-ассоциативностью и > приоритета, оператор сразу выталкивается в очередь	|	лево-ассоциативность: операции выполняются слева направо, примеры: +, -, *, :
		- кладем текущий оператор в стек																		|	право-ассоциативность: операции выполняются справа налево, примеры: ^
4) открывающая скобка - кладется в стек и отсекает собой все предыдущие операторы								|------------------------------------------------------------------------------	
5) закрывающая скобка - выталкивает все операторы до открывающей скобки в очередь, если после исчезновения		| оператор | приоритет | унарность | ассоциативность | унарность - сколько операндов требуется
закрывающей скобки была функция - так же скидываем ее в очередь, после скобки *испаряются*						|	 !	   |	 4	   |  унарная  |	 правая	     | унарный => один
6) все что осталось в стеке переходит в очередь																	| функция  |	 4	   |  унарная  |	 правая		 | бинарный => два
																												|	 ^	   |	 3	   |  бинарная |     правая      |
																												|	*,/	   |	 2     |  бинарная |	 левая		 |
																												|   +,-    |     1     |  бинарная |     левая       |
*/
/*
как считать выражение в постфиксной нотации: 
1) число или переменная -> кладем в стек
2) оператор бинарный - достаем одно значение из стека, вставляем оператор, вставляем еще одно значение, считаем и засовываем обратно в стек
3) оператор унарный - достаем одно значение из стека, выполняем оператор или функцию и возвращаем в стек
4)если в стеке остался всего один элемент - мы нашли результат
*/
/*
пример - 2 + 2 * 3
1) Дейкстра: 
выражение: + 2 * 3  | выражение: 2 * 3     | выражение: * 3      | выражение: 3            | выражение:             | выражение:
стек операторов:    | стек операторов: +   | стек операторов: +  | стек операторов: *, +   | стек операторов: *, +  | стек операторов:
очередь: 2          | очередь: 2           | очередь: 2, 2       | очередь: 2, 2           | очередь: 2, 2, 3       | очередь: 2, 2, 3, *, +
2)рассчет выражения:
выражение: 2, 2, 3, *, +  | выражение: 2, 3, *, +   | выражение: 3, *, +   | выражение: *, +   | выражение: +  | между делом было: вытащили 3, вставили справа,                  | выражение: 
стек:                     | стек: 2                 | стек: 2, 2           | стек: 2, 2, 3     | стек: 2, 6    | вытащили *, вставили перед 3, вставили 2 и вернули результат: 6 | стек: 8
*/

typedef enum token_types {
	numbers,
	variables,
	operators,
	functions,
	left_brace,
	right_brace
}token_types;

typedef struct Tokens {
	token_types type;
	char text[20];
	double value;
}Tokens;

typedef struct node {
	Tokens token;
	struct node* next;
}node;

typedef struct stack {
	struct node* top;
}stack;

typedef struct queue {
	struct node* head;
	struct node* tail;
}queue ;

//functions to put a value in the struct (push to stack and en(ter)queue for queue)
void push(stack* s, Tokens t) {
	node* s_node = (node*)malloc(sizeof(node));
	if (!s_node) {
		printf("Memory allocation error!");
		exit(0);
	}
	s_node->next = NULL;
	s_node->token = t;
	s_node->next = s->top;
	s->top = s_node;
}

void enqueue(queue* q, Tokens t) {
	node* q_node = (node*)malloc(sizeof(node));
	if (!q_node) {
		printf("Memory allocation error!");
		exit(0);
	}
	q_node->token = t;
	q_node->next = NULL;
	if (q->head == NULL) q->head = q_node;
	else q->tail->next = q_node;
	q->tail = q_node;
}

//functions to get a value from struct (because stack work on LIFO and queue FIFO, the value gets from the top of the stack and from the head of the queue)
Tokens pop(stack* s) {
	if (s == NULL || s->top == NULL) {
		printf("Стэк пустой!\n");
		exit(0);
	}
	node* temp = s->top;
	Tokens x = temp->token;
	s->top = temp->next;
	free(temp);
	return x;
}

Tokens dequeue(queue* q) {
	if (q == NULL || q->head == NULL) {
		printf("Очередь пуста!\n");
		exit(0);
	}
	node* temp = q->head;
	Tokens x = temp->token;
	q->head = temp->next;
	if (q->head == NULL) q->tail = NULL; //with the queue its quite harder, we need to check if after our manipulations the head is empty, if it is we know that the whole queue is empty now and need to null the tail too
	free(temp);
	return x;
}

int tokenizer(const char* str, Tokens list[500]) {
	int tokens_count = 0;
	int i = 0;
	while (str[i] != '\0') {

		if (isspace(str[i]) != 0) { //if symbol is space - skip we dont need it
			i++;
			continue;
		}
		//braces
		if (str[i] == '(') {
			list[tokens_count++] = (Tokens){ left_brace, "(", 0 };
			i++;
		}
		else if (str[i] == ')') {
			list[tokens_count++] = (Tokens){ right_brace, ")", 0 };
			i++;
		}
		//operators
		else if (strchr("+-*/^!", str[i]) != NULL)
		{
			char operator[] = { str[i], '\0' };
			list[tokens_count] = (Tokens){ operators, "", 0}; //we cant directy put operator in text because after leaving this function the pointer on char will disappear
			strcpy(list[tokens_count].text, operator);
			tokens_count++;
			i++;
		}

		//digit
		else if (isdigit(str[i]) || (str[i]) == '.' && isdigit(str[i + 1]))
		{
			char num_buf[32];
			int j = 0;

			// puting number in a buffer
			while (isdigit(str[i]) || str[i] == '.') 
			{
				num_buf[j++] = str[i++];
			}
			num_buf[j] = '\0';

			list[tokens_count].type = numbers;
			strcpy(list[tokens_count].text, num_buf);
			list[tokens_count].value = atof(num_buf); // atof - str to double | atoi - str to int
			tokens_count++;
		}

		// symbols - they create functions or variables
		else if (isalpha(str[i]))
		{
			char word_buf[16];
			int j = 0;
			while (isalpha(str[i]))
			{
				word_buf[j++] = str[i++];
			}
			word_buf[j] = '\0';

			const char* funcs[] = { "sin", "cos", "tan", "cot", "arcsin", "arccos", "arctan", "arccot", "sqrt" };
			int found = 0;

			for (int i = 0; i < sizeof(funcs) / sizeof(funcs[0]); i++) {
				if (strcmp(word_buf, funcs[i]) == 0) {
					found = 1;
					break;
				}
			}
			if (found == 1) {
				list[tokens_count].type = functions;
				strcpy(list[tokens_count].text, word_buf);
				tokens_count++;
			}
			else 
			{
				list[tokens_count].type = variables;
				strcpy(list[tokens_count].text, word_buf);
				tokens_count++;
			}
		}
		// неизвестный символ
		else 
		{
			printf("неизвестный символ: '%c'\n", str[i]);
			i++;
		}
	}

	return tokens_count;
}

int get_priority(const char* operator) { //functions returns priority of operators that is important to shunting yard
	char c = operator[0];
	if (c == '!') return 4;
	if (c == '^') return 3;
	if (c == '*' || c == '/') return 2;
	if (c == '+' || c == '-') return 1;
	return 0;
}

typedef enum associativity {left, right} associativity;
//using enum ecxept of standart 0, 1 makes understading what associativity have operator easier
associativity get_associativity(const char* operator) {
	char c = operator[0];
	if (c == '!' || c == '^') return right;
	return left;
}
/* debug functions
void print_queue(queue* q) {
	node* current = q->head;
	printf("Очередь: ");
	while (current != NULL) {
		printf("%s ", current->token.text); // предполагаем, что text — строка в Tokens
		current = current->next;
	}
	printf("\n");
}

void print_stack(stack* s) {
	node* current = s->top;
	printf("Стек: ");
	while (current != NULL) {
		if (current->token.type == numbers) printf("%lf ", current->token.value);
		else printf("%s ", current->token.text);
		current = current->next;
	}
	printf("\n");
}
*/
void shunting_yard(queue* out, stack* oper_stack, Tokens list[500], int tokens_count) {
	for (int i = 0; i < tokens_count; i++) 
	{
		if (list[i].type == numbers || list[i].type == variables) enqueue(out, list[i]);
		else if (list[i].type == functions) 
		{
			push(oper_stack, list[i]);
		}
		else if (list[i].type == operators)
		{ //подробное объяснение случая с операторами
		   //если вершина стека пуста то все просто - банально кладем новоприбывший оператор в стек 
			while ((oper_stack->top != NULL) && //если же нет, ситуация становится сложнее - нам нужно сравнить на приоритетность и ассоциативность новый и старый (в данный момент лежит на вершине стека)
					(oper_stack->top->token.type == operators || oper_stack->top->token.type == functions) &&  //если на вершине стека оператор или функция - нужно провести сравнение по двум случаям
					(get_priority(oper_stack->top->token.text) > get_priority(list[i].text) ||  // 1 случай - новый оператор имеет приоритет меньше старого, тогда старый оператор переходит в очередь
					(get_priority(oper_stack->top->token.text) == get_priority(list[i].text) &&   // 2 случай - приоритеты равны, но тогда мы проводим проверку на ассоциативность
					get_associativity(list[i].text) == left))) // если новый оператор лево-ассоциативен, старый оператор идет в очередь
			{
				enqueue(out, pop(oper_stack));
			}
			push(oper_stack, list[i]); //как только провели все нужные проверки - наконец толкаем новый оператор в стек
		}
		else if (list[i].type == left_brace) push(oper_stack, list[i]);
		else if (list[i].type == right_brace) 
		{
			while (oper_stack->top != NULL && oper_stack->top->token.type != left_brace) 
			{
				enqueue(out, pop(oper_stack));
			}
			if (oper_stack->top == NULL) {
				printf("Неверно введенная скобочная последовательность!\n");
				exit(0);
			}
			pop(oper_stack);
			if (oper_stack->top != NULL && oper_stack->top->token.type == functions) enqueue(out, pop(oper_stack));
		}
	}
	while (oper_stack->top != NULL) enqueue(out, pop(oper_stack));
}

double result(queue* out) {
	stack* temp_stack = (stack*)malloc(sizeof(stack));
	temp_stack->top = NULL;
	while (out->head != NULL) 
	{
		Tokens new = dequeue(out);
		if (new.type == numbers) push(temp_stack, new);
		else if (new.type == variables) 
		{
			printf("Введите значение переменной %s: ", new.text);
			scanf("%lf", &new.value);
			push(temp_stack, new);
		}
		else if (new.type == functions) 
		{
			double x = pop(temp_stack).value;
			if (strcmp(new.text, "sin") == 0) x = sin(x);
			else if (strcmp(new.text, "cos") == 0) x = cos(x);
			else if (strcmp(new.text, "tan") == 0) x = tan(x);
			else if (strcmp(new.text, "cot") == 0) x = (cos(x) / sin(x));
			else if (strcmp(new.text, "arcsin") == 0) x = asin(x);
			else if (strcmp(new.text, "arccos") == 0) x = acos(x);
			else if (strcmp(new.text, "arctan") == 0) x = atan(x);
			else if (strcmp(new.text, "arccot") == 0) x = PI / 2.0 - atan(x);
			else if (strcmp(new.text, "sqrt") == 0) 
			{
				if (x < 0) 
				{
					printf("Корень отрицательного числа не может быть высчитан\n");
					exit(0);
				}
				else x = sqrt(x);
			}
			Tokens resTok = { numbers, "", x};
			push(temp_stack, resTok);
		}
		else if (new.type == operators)
		{
			double res = 1;
			if (strcmp(new.text, "!") != 0)
			{
				double right = pop(temp_stack).value;
				double left = pop(temp_stack).value;
				if (strcmp(new.text, "+") == 0) res = left + right;
				else if (strcmp(new.text, "-") == 0) res = left - right;
				else if (strcmp(new.text, "*") == 0) res = left * right;
				else if (strcmp(new.text, "/") == 0)
				{
					if (right == 0) 
					{
						printf("Делить на ноль нельзя\n");
						exit(0);
					}
					res = left / right;
				}
				else if (strcmp(new.text, "^") == 0) res = pow(left, right);
			}
			else 
			{
				double x = pop(temp_stack).value;
				if (x < 0) 
				{
					printf("Факториал отрицательного числа не существует\n");
					exit(0);
				}
				for (int i = 2; i <= x; i++) res *= i;
			}
			Tokens resTok = { numbers, "", res };
			push(temp_stack, resTok);

		}
	}
	double answer = pop(temp_stack).value;
	if (temp_stack->top != NULL) 
	{
		printf("Ошибка: стек не пустой после вычислений\n");
		exit(0);
	}
	return answer;
}

void solve() {
	char str[1000];

	//initializing all main structures
	Tokens token_list[500];
	stack* main_stack = (stack*)malloc(sizeof(stack));
	main_stack->top = NULL;
	queue* main_queue = (queue*)malloc(sizeof(queue));
	main_queue->head = NULL;
	main_queue->tail = NULL;

	printf("Введите ваше выражение: ");
	fgets(str, sizeof(str), stdin);
	str[strcspn(str, "\n")] = '\0'; //another fixing of "\n", not getchar cause of using fgets except of scanf: strcspn find the first "\n" in str and replaces it with "\0"
	//main solving part
	int tokens_count = tokenizer(str, token_list); //converting string into tokens
	shunting_yard(main_queue, main_stack, token_list, tokens_count);
	double answer = result(main_queue);
	printf("Ответ: %lf\n", answer);
}

void print_func() {
	printf("\tСписок доступных функций:\n"
	"\t sin, cos, tan, cot - тригонометрические функции\n"
	"\t arcsin, arccos, arctan, arccot - обратные тригонометрические функции\n"
	"\t sqrt - корень числа\n"
	);
}

int main() {
	setlocale(LC_ALL, "rus");
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	unsigned short option;
	while (TRUE) {
		printf("\t-------------Меню-------------\n"
			   "\t| 1. Ввести выражение        |\n"
			   "\t| 2. Список доступных функций|\n"
			   "\t| 0. Выход из программы      |\n"
			   "\t------------------------------\n"
		"\tВведите свой выбор: ");
		scanf("%hu", &option);
		int ch;
		while ((ch = getchar()) != '\n' && ch != EOF) {} //fixing "\n" after choice: scanf gets all except "\n" and it remains in stdin, when getchar takes it and stdin stays clear
		switch (option) {
		case 0: exit(0);
			break;
		case 1: solve();
			break;
		case 2: print_func();
			break;
		default: printf("Неверно введенная команда\n");
		}
	}
}