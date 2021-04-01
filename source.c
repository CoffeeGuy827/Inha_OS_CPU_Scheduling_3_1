#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>

#define TRUE 1
#define FALSE 0

typedef struct _process {
	int id; // process ID
	int return_time; 
	int waiting_time;
	int arrive_time;
	int response_time;
	int turnaround_time;
	int priority; // �켱 ����
	int completed; // process �۾��� ó���Ǿ����� ����
	int burst; // ���� �ð�
}Process;

int p_count = 0; //process�� ����
int i_q1 = 0; // Queue 1 �� ���μ��� ����
int i_q2 = 0; // Queue 2 �� ���μ��� ����
int i_q3 = 0; // Queue 3 �� ���μ��� ����

int class_num; // ���μ��� �Է� �� ���° ť�� ���� ����
// �Էµ� ���μ��� ������ �ޱ� ���� �ӽ� ����
int sub_id;
int sub_arrive_time;
int sub_burst;
int sub_priority;

Process *q1; // Queue 1
Process *q2; // Queue 2
Process *q3; // Queue 3

int idx = 0; // thread �迭�� ���� index
pthread_t thread[5]; // ������

sem_t semaphore; // ��������


void p_init(Process q[], int length)
{
	int i;

	for (i = 0; i < length; i++)
	{
		q[i].waiting_time = 0;
		q[i].return_time = 0;
		q[i].response_time = 0;
		q[i].completed = FALSE;
	}
}

void print_table(Process p[], int n)
{
	int i;

	puts("\t*-----*------------*-------------*----------*-------------*-----------------*--------------*-----------------*");
	puts("\t| PID | Burst Time | Arrive Time | Priority | Return Time |  Response Time  | Waiting Time | Turnaround Time |");
	puts("\t*-----*------------*-------------*----------*-------------*-----------------*--------------*-----------------*");

	for (i = 0; i < n; i++)
	{
		printf("\t| %3d |     %3d    |     %3d     |    %3d   |     %3d     |      %3d        |      %3d     |        %3d      |\n",
			p[i].id, p[i].burst, p[i].arrive_time, p[i].priority, p[i].return_time, p[i].response_time, p[i].waiting_time, p[i].turnaround_time);

		puts("\t*-----*------------*-------------*----------*-------------*-----------------*--------------*-----------------*");
	}

	puts("\n");
}

int compare_by_return_time(const void *a, const void *b)
{
	//�޾ƿ� ���ڸ� (Process *)�� ����ȯ ���ݴϴ�.
	Process *pA = (Process *)a;
	Process *pB = (Process *)b;

	// return_time�� ��Ұ��迡 ���� ���ϰ��� �ٸ��� �����մϴ�.
	if (pA->return_time < pB->return_time)
		return -1;

	else if (pA->return_time > pB->return_time)
		return 1;

	else
		return 0;
}

void quick_sort_by_return_time(Process q[], int len)
{//len : Queue�� ����
	qsort(q, len, sizeof(Process), compare_by_return_time);
}

int compare_by_priority(Process *a, Process *b)
{
	//�޾ƿ� ���ڸ� (Process *)�� ����ȯ ���ݴϴ�.
	Process *pA = (Process *)a;
	Process *pB = (Process *)b;

	//priority�� ���� ���ٸ� ���� �ð��� �� ª�� ���μ����� �켱������ ó���մϴ�.
	if (a->priority == b->priority) {
		if (a->burst != b->burst) {
			if (a->burst < b->burst)
				return -1;
			else
				return 1;
		}
		else
			return 0;
	}
	else {// priority�� ��Ұ��迡 ���� ���ϰ��� �ٸ��� �����մϴ�.
		if (a->priority < b->priority)
			return -1;
		else if (a->priority > b->priority)
			return 1;
	}
}

void quick_sort_by_priority_time()
{// Queue 1 �� ���μ����� �켱������ ���� ������ �� �����մϴ�.
	qsort(q1, i_q1, sizeof(Process), compare_by_priority);
}

void gantt_chart(Process p[], int len)
{
	int i, j;
	printf("\t ");

	for (i = 0; i < len; i++)
	{
		for (j = 0; j < p[i].burst; j++)
		{
			printf("--");
		}
		printf(" ");
	}

	printf("\n\t|");

	for (i = 0; i < len; i++)
	{
		for (j = 0; j < p[i].burst - 1; j++)
		{
			printf(" ");
		}
		printf("%2d", p[i].id);

		for (j = 0; j < p[i].burst - 1; j++)
		{
			printf(" ");
		}

		printf("|");
	}

	printf("\n\t ");

	for (i = 0; i < len; i++)
	{
		for (j = 0; j < p[i].burst; j++)
		{
			printf("--");
		}
		printf(" ");
	}
	printf("\n\t");
	printf("0");

	for (i = 0; i < len; i++)
	{
		for (j = 0; j < p[i].burst; j++)
		{
			printf("  ");
		}
		if (p[i].return_time > 9)
		{
			printf("\b");
		}
		printf("%d", p[i].return_time);
	}
	printf("\n");
}

//**************************************************//
// Q1 : Non-preemptive Priority Scheduling Algorithm
void Cal_for_npps(Process p[], int len)
{
	int i, j;
	int check; // TRUE : ��� ���μ����� �Ϸ���� ����, FALSE : ��� ���μ��� �Ϸ�
	int min; // Priority�� ���� ���� index ����
	int time = 0; // ���� �ð�

	// Queue 1�� ù��° process�� ������ �����մϴ�.
	p[0].return_time = p[0].burst;
	p[0].turnaround_time = p[0].return_time - p[0].arrive_time;
	p[0].response_time = 0;
	p[0].completed = TRUE;
	
	//���� �ð��� Queue 1�� ù��° process�� burst�� �����մϴ�.
	time = p[0].burst;

	for (j = 1; j < i_q1; j++)
	{
		p[j].response_time = time - p[j].arrive_time;
		p[j].return_time = time + p[j].burst;
		p[j].turnaround_time = p[j].return_time - p[j].arrive_time;
		p[j].waiting_time = time - p[min].arrive_time;
		p[j].completed = TRUE;

		// ���� �ð��� ���� �ְ� �켱���� process�� ���� �ð��� ���Ͽ� ����
		time += p[min].burst;
	}
}

void *NPPS(void *arg)
{
	int i;
	int total_waiting_time = 0;
	int total_turnaround_time = 0;
	int total_response_time = 0;

	// ���μ��� �۾��� ó���� �� ���ŵǾ���� ���� �ʱ�ȭ
	p_init(q1, i_q1);

	// �켱������ ���� ������ ���μ��� ����
	quick_sort_by_priority_time();

	// ���� �� ���μ��� �۾� ó���� ���� ���μ��� ���� ����
	Cal_for_npps(q1, i_q1);

	//�����ٸ� ��� ���
	for (i = 0; i < i_q1; i++)
	{
		total_waiting_time += q1[i].waiting_time;
		total_turnaround_time += q1[i].turnaround_time;
		total_response_time += q1[i].response_time;
	}

	//��Ʈ ��Ʈ ����� ���� return_time������ ����
	quick_sort_by_return_time(q1, i_q1);

	printf("\tNPPS\n\n");

	gantt_chart(q1, i_q1);

	printf("\n\tAverage_Waiting_Time     : %-2.2lf\n", (double)total_waiting_time / (double)i_q1);
	printf("\tAverage_Turnaround_Time  : %-2.2lf\n", (double)total_turnaround_time / (double)i_q1);
	printf("\tAverage_Response_Time    : %-2.2lf\n\n", (double)total_response_time / (double)i_q1);

	print_table(q1, i_q1);
	pthread_exit(0);
}

//**************************************************//
// Q3 : HRN(Highest Response-Ratio Next)

void *HRN(void *arg)
{
	int i;
	int time, locate; //���� �ð��� ���μ��� ��ġ ������ ���� ����
	int total_burst_time = 0;
	int total_waiting_time = 0;
	int total_turnaround_time = 0;
	int total_response_time = 0;

	float hrr, temp; // HRN �˰����� �켱������ ������ ����

	// ���μ��� �۾��� ó���� �� ���ŵǾ���� ���� �ʱ�ȭ
	p_init(q3, i_q3);

	// total_burst_time ����
	for (i = 0; i < i_q3; i++)
	{
		total_burst_time += q3[i].burst;
	}

	// ���� �ð��� total_burst_time�� �����Ҷ����� �������� ����˴ϴ�.
	for (time = q3[0].arrive_time; time < total_burst_time;)
	{
		hrr = -99999;

		for (i = 0; i < i_q3; i++)
		{
			//�Ϸ���� ���� ���μ����� ���
			if (q3[i].completed != TRUE)
			{
				// HRN �˰��� ���� �켱���� ���
				temp = (q3[i].burst + (time - q3[i].arrive_time)) / q3[i].burst;
				
				// ���μ����� �켱������ hrr���� ũ�ٸ�
				if (hrr < temp)
				{
					//�켱���� �� �ε��� ����
					hrr = temp;
					locate = i;
				}
			}
		}

		// �켱������ ���� ���� ���μ��� �۾� ó�� �� ���� �ð� ����
		time += q3[locate].burst;

		// ó���� ���μ��� ���� ����
		q3[locate].waiting_time = time - q3[locate].arrive_time - q3[locate].burst;
		q3[locate].turnaround_time = time - q3[locate].arrive_time;
		q3[locate].return_time = q3[locate].turnaround_time + q3[locate].arrive_time;
		q3[locate].response_time = q3[locate].waiting_time;
		q3[locate].completed = TRUE;

		total_waiting_time += q3[locate].waiting_time;
		total_turnaround_time += q3[locate].turnaround_time;
		total_response_time += q3[locate].response_time;
	}
	//��Ʈ ��Ʈ ����� ���� return_time������ ����
	quick_sort_by_return_time(q3, i_q3);

	printf("\tHRN Algorithm\n\n");

	gantt_chart(q3, i_q3);

	printf("\n\tAverage_Waiting_Time     : %-2.2lf\n", (double)total_waiting_time / (double)i_q3);
	printf("\tAverage_Turnaround_Time  : %-2.2lf\n", (double)total_turnaround_time / (double)i_q3);
	printf("\tAverage_Response_Time    : %-2.2lf\n\n", (double)total_response_time / (double)i_q3);

	print_table(q3, i_q3);
	pthread_exit(0);
}

//**************************************************//
// Q3 : SJF(Shortest Job First)

void cal_for_sjf(Process *p, int len)
{
	int i, j;
	int cur_time = 0; //���� �ð�
	int min = 0; // �ּ� ���� �ð��� ���� process�� index�� ����

	// ù ��° ���μ����� ������ �ƹ� ���μ����� ���� ������ �ٷ� ����
	p[0].completed = TRUE;
	p[0].return_time = p[0].burst;
	p[0].turnaround_time = p[0].burst - p[0].arrive_time;
	p[0].waiting_time = 0;

	//ù��° ���μ��� �۾� ó�� �� ���� �ð� ����
	cur_time = p[0].burst;

	for (i = 1; i < len; i++)//�ּ� ���� �ð��� ������ ���μ��� �۾� ó���� ���� ����
	{
		for (j = 1; j < len; j++)
		{
			if (p[j].completed == TRUE)
			{//�̹� �Ϸ�� ���μ����� ��� ���� ������ �̵�
				continue;
			}

			else
			{//ó������ ���� ���μ����� ��� min ����
				min = j;
				break;
			}
		}

		for (j = 1; j < len; j++)
		{//�ּ� ���� �ð��� �����鼭, ó������ ���� ���μ��� Ž��
			if ((p[j].completed == FALSE) && (p[j].burst < p[min].burst))
			{//Ž�� ���� �� min ����
				min = j;
			}
		}

		// �ּ� ���� �ð��� ���� ���μ��� �۾� ó�� �� ���μ��� ���� �� ���� �ð� ����
		p[min].waiting_time = cur_time - p[min].arrive_time;
		p[min].completed = TRUE;
		cur_time += p[min].burst;
		p[min].return_time = cur_time;
		p[min].turnaround_time = p[min].return_time - p[min].arrive_time;
	}
}


void *SJF(void *arg)
{
	int i;
	int total_waiting_time = 0;
	int total_turnaround_time = 0;
	int total_response_time = 0;

	// ���μ��� �۾��� ó���� �� ���ŵǾ���� ���� �ʱ�ȭ
	p_init(q2, i_q2);

	//���μ��� �۾� ó���� ���� ���μ��� ���� ����
	cal_for_sjf(q2, i_q2);

	//���μ��� �۾��� ó���� �� �����Ǿ���� ���� ����
	for (i = 0; i < i_q2; i++)
	{
		q2[i].return_time = q2[i].turnaround_time + q2[i].arrive_time;
		q2[i].response_time = q2[i].waiting_time;
		total_waiting_time += q2[i].waiting_time;
		total_turnaround_time += q2[i].turnaround_time;
		total_response_time += q2[i].response_time;
	}

	printf("\tSJF Algorithm\n\n");

	//��Ʈ ��Ʈ ����� ���� return_time������ ����
	quick_sort_by_return_time(q2, i_q2);

	gantt_chart(q2, i_q2);

	printf("\n\tAverage_Waiting_Time     : %-2.2lf\n", (double)total_waiting_time / (double)i_q2);
	printf("\tAverage_Turnaround_Time  : %-2.2lf\n", (double)total_turnaround_time / (double)i_q2);
	printf("\tAverage_Response_Time    : %-2.2lf\n\n", (double)total_response_time / (double)i_q2);

	print_table(q2, i_q2);
}

void *MLQS(void *arg)
{

	sem_wait(&semaphore); //semaphore ��ٸ���
	//NPPS �����ٸ��� thread[1]���� ����
	printf("------------------------------------------------------------------\n");
	pthread_create(&thread[1], NULL, NPPS, NULL);
	pthread_join(thread[1], NULL);
	printf("------------------------------------------------------------------\n");
	

	//SJF �����ٸ��� thread[2]���� ����
	pthread_create(&thread[2], NULL, SJF, NULL);
	pthread_join(thread[2], NULL);
	printf("------------------------------------------------------------------\n");

	
	//HRN �����ٸ��� thread[3]���� ����
	pthread_create(&thread[3], NULL, HRN, NULL);
	pthread_join(thread[3], NULL);
	sem_post(&semaphore); //

	pthread_exit(0);
}


int main()
{
	sem_init(&semaphore, 0, 1); // semaphore �ʱ�ȭ

	//Queue 1, 2, 3 �ʱ�ȭ
	q1 = (Process *)malloc(sizeof(Process) * 100);
	q2 = (Process *)malloc(sizeof(Process) * 100);
	q3 = (Process *)malloc(sizeof(Process) * 100);

	// ������ ���� ���μ��� ������ �ޱ�
	FILE *fpp = NULL;
	fpp = fopen("sample.txt", "r");

	// ���� �޾ƿ��� ���� �� ���� �޽��� ���
	if (fpp == NULL)
	{
		printf("OPEN ERROR\n");
		return 0;
	}

	while (!feof(fpp))
	{
		// ���μ��� ���� �ޱ�
		fscanf(fpp, "%d %d %d %d %d",
			&class_num, &sub_id, &sub_arrive_time, &sub_burst, &sub_priority);

		if (feof(fpp))
		{
			break;
		}

		if (class_num == 1)// Queue 1�� ���μ��� ����
		{
			q1[i_q1].id = sub_id;
			q1[i_q1].arrive_time = sub_arrive_time;
			q1[i_q1].burst = sub_burst;
			q1[i_q1].priority = sub_priority;
			i_q1++;
		}
		else if (class_num == 2)// Queue 2�� ���μ��� ����
		{
			q2[i_q2].id = sub_id;
			q2[i_q2].arrive_time = sub_arrive_time;
			q2[i_q2].burst = sub_burst;
			q2[i_q2].priority = sub_priority;
			i_q2++;
		}
		else if (class_num == 3)// Queue 3�� ���μ��� ����
		{
			q3[i_q3].id = sub_id;
			q3[i_q3].arrive_time = sub_arrive_time;
			q3[i_q3].burst = sub_burst;
			q3[i_q3].priority = sub_priority;
			i_q3++;
		}

		// ���μ��� �� ���� +1
		p_count++;
	}

	// Multi level queue �����ٸ��� thread[0]���� ����
	pthread_create(&thread[0], NULL, MLQS, NULL);
	pthread_join(thread[0], NULL);

	// �Ҵ��� ���ҽ� ��ȯ
	fclose(fpp);
	free(q1);
	free(q2);
	free(q3);

	system("pause");
	return 0;
}


