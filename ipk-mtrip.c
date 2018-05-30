/*IPK Project 2
*
* Bolshakova Ksenia (xbolsh00)
*/
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#define h_addr h_addr_list[0] /* for backward compatibility */

void reflect_function(int port) {
	int reflect_socket, bytesrx, bytestx;
	int optval = 1;
	struct sockaddr_in reflect_address, meter_address;
	socklen_t meterlen;
	char * buf = NULL;
	const char * hostaddrp;
    struct hostent *hostp;
    int counter = 0;
    int size = 0;

	
	buf = (char *) malloc(100);
	if (buf == NULL) {
		perror("Error in memory allocation.\n");
		exit(EXIT_FAILURE);
	}

	for(int i = 0; i < 100; i++) {
		buf[i] = 'a';
	}
	

	/* Vytvoreni soketu */
	if ((reflect_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0)
	{
		perror("ERROR: socket");
		exit(EXIT_FAILURE);
	}

	/* potlaceni defaultniho chovani rezervace portu ukonceni aplikace */ 
    setsockopt(reflect_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    /* adresa reflectu, potrebuje pro prirazeni pozadovaneho portu */
    bzero((char *) &reflect_address, sizeof(reflect_address));
    reflect_address.sin_family = AF_INET;
    reflect_address.sin_addr.s_addr = htonl(INADDR_ANY);
    reflect_address.sin_port = htons((unsigned short)port);

    if (bind(reflect_socket, (struct sockaddr *) &reflect_address, sizeof(reflect_address)) < 0) 
    {
        perror("ERROR: binding");
        exit(EXIT_FAILURE);
    }

    //Receive size of buffer
    meterlen = sizeof(meter_address);
    bytesrx = recvfrom(reflect_socket, buf, strlen(buf), 0, (struct sockaddr *) &meter_address, &meterlen);
    if (bytesrx < 0) 
        perror("ERROR: recvfrom:");

    hostp = gethostbyaddr((const char *)&meter_address.sin_addr.s_addr, 
	sizeof(meter_address.sin_addr.s_addr), AF_INET);
              
    hostaddrp = inet_ntoa(meter_address.sin_addr);
    //printf("Message (%lu) from %s:  %s\n", strlen(buf), hostaddrp, buf);
    
    
    /* odeslani zpravy zpet metrovi  */        
    bytestx = sendto(reflect_socket, buf, strlen(buf), 0, (struct sockaddr *) &meter_address, meterlen);
    if (bytestx < 0) 
        perror("ERROR: sendto:");
    size = atoi(buf);
 	//printf("SIZE: %d\n", size);
    
    free(buf);
 
    int buf_size = size + size*0.30;
	char buffer[buf_size];



    while(1) {
    	//printf("READY\n");

    	meterlen = sizeof(meter_address);
        bytesrx = recvfrom(reflect_socket, buffer, buf_size, 0, (struct sockaddr *) &meter_address, &meterlen);
        if (bytesrx < 0) 
            perror("ERROR: recvfrom:");

    
        hostp = gethostbyaddr((const char *)&meter_address.sin_addr.s_addr, 
		sizeof(meter_address.sin_addr.s_addr), AF_INET);
              
        hostaddrp = inet_ntoa(meter_address.sin_addr);
        //printf("Message (%lu) from %s:  %s\n", strlen(buffer), hostaddrp, buffer);
    
    
        /* odeslani zpravy zpet metrovi  */        
        bytestx = sendto(reflect_socket, buffer, buf_size, 0, (struct sockaddr *) &meter_address, meterlen);
        if (bytestx < 0) 
            perror("ERROR: sendto:");
        counter++;
    
    }

}

void full_buffer(char * buf, int size) {
	char c = 'A';
	for (int i = 0; i < size-2; i++) {
		if(c <= 'Z') {
			
			buf[i] = c;	
			c++;
			if (c == 'Z') {
				c = 'A';
			}
			
		}

	}
}

void meter_function(int port, const char* remote_hostname, int size, int timer) {
	int meter_socket, bytestx, bytesrx;
    socklen_t reflectlen;
    struct hostent *reflect;
    struct sockaddr_in reflect_address;
    char * buf = NULL;
    //char * receive_buf = NULL;
    int counter = 1;
    int counter_max = 0;
    clock_t start, stop;
	unsigned long t;

    /* ziskani adresy pomoci DNS */
    if ((reflect = gethostbyname(remote_hostname)) == NULL) {
        fprintf(stderr,"ERROR: no such host as %s\n", remote_hostname);
        exit(EXIT_FAILURE);
    }

    /* nalezeni IP adresy a inicializace struktury reflect_address */
    bzero((char *) &reflect_address, sizeof(reflect_address));
    reflect_address.sin_family = AF_INET;
    bcopy((char *)reflect->h_addr, (char *)&reflect_address.sin_addr.s_addr, reflect->h_length);
    reflect_address.sin_port = htons(port);

    /* Vytvoreni soketu */
	if ((meter_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0)
	{
		perror("ERROR: socket");
		exit(EXIT_FAILURE);
	}

	buf = (char *) malloc(100);
	if (buf == NULL) {
		perror("Error in memory allocation.\n");
		exit(EXIT_FAILURE);
	}
  	
  	int div = 0;
  	int count_digit = 0;
  	for(div = 1; div <= size; div *= 10) {
  		count_digit++;
  	}
  	int result = size;
  	int it = 0;
	do {
		div = div/10;
		buf[it] = (result / div) + 48;
		//printf("BUF[i]: %c\n", buf[i]);
		result = result % div;
		//printf("size: %d\n", size);
		it++;
		count_digit--;
	} while(count_digit > 0);


	reflectlen = sizeof(reflect_address);
    bytestx = sendto(meter_socket, buf, strlen(buf), 0, (struct sockaddr *) &reflect_address, reflectlen);
    if (bytestx < 0) 
    	perror("ERROR: sendto");
    	
    
    /* prijeti odpovedi a jeji vypsani */
    bytesrx = recvfrom(meter_socket, buf, strlen(buf), 0, (struct sockaddr *) &reflect_address, &reflectlen);
    if (bytesrx < 0) 
    	perror("ERROR: recvfrom");
 	//printf("BUFFER1: %s", buf);
    free(buf);
  
	/* Zprava */
	buf = (char *) malloc(size + 0.2*size);
	if (buf == NULL) {
		perror("Error in memory allocation.\n");
		exit(EXIT_FAILURE);
	}


	double time_sec = 0;
	double time = 0;

	time = timer % 100;
	time = time / 100;
	int size_max = size + 0.2*size;
	float time_wr = 0; //time from send to receive
	float size_wr = 0; //how many bytes returned

	int buffer_size = size + size*0.3;
	char receive_buf[buffer_size];

	double * buf_wr;
	buf_wr = (double *) malloc(sizeof(double)*100);
	if (buf_wr == NULL) {
		perror("Error in memory allocation.\n");
		exit(EXIT_FAILURE);
	}
	
	int iterator = 0;
	int memory = 0;
	while(time_sec < time) {
		start = clock ();
		/* odeslani zpravy na server */

		full_buffer(buf, size);
		//printf("BUFFER2: %s\n", buf);
    	reflectlen = sizeof(reflect_address);
    	bytestx = sendto(meter_socket, buf, strlen(buf), 0, (struct sockaddr *) &reflect_address, reflectlen);
    	if (bytestx < 0) 
      		perror("ERROR: sendto");
    	
    	counter_max++;
    	/* prijeti odpovedi a jeji vypsani */
    	bytesrx = recvfrom(meter_socket, receive_buf, buffer_size, 0, (struct sockaddr *) &reflect_address, &reflectlen);
    	if (bytesrx < 0) 
      		perror("ERROR: recvfrom");
      	
      	stop = clock();
      	time_sec += (double)(stop - start) / CLOCKS_PER_SEC;

      	time_wr = ((double)(stop - start) / CLOCKS_PER_SEC)*100;
      	size_wr = strlen(receive_buf);
      	if (size < size_max-150) {
      		size += 100;
      	}

      	buf_wr[iterator] = size_wr / time_wr;
      	
      	if ((iterator+2) == (100+memory)) {
      		memory += 100;
      		buf_wr = (double * ) realloc(buf_wr, sizeof(double)*(150+memory));
      	}

      	//printf("TIMER: %f \n", time_sec);
		
      	iterator++;   

	}

	double min = buf_wr[0];
	double max = buf_wr[0];
	double average = 0;
	double average_timer = 0;
	double difference = 0;
	double res = 0;
	for (int k = 0; k < iterator; k++) {
		average += buf_wr[k];

		if (min > buf_wr[k]) {
			min = buf_wr[k];
		}

		if (max < buf_wr[k]) {
			max = buf_wr[k];
		}
	}

	for (int l = 0; l < iterator; l++) {
		difference += (average - buf_wr[l]);
	}
	
	average = average * 8 * 0.0001;
	max = max*0.0008;
	min = min*0.0008;
	res = sqrt(difference);
	res *= 0.0008;
	average = average / iterator;
	average_timer = (time_sec*100) / iterator;

	printf("Průměrná přenosová rychlost: %f Mbit/s.\n", average);
	printf("Maximální naměřená rychlost: %f Mbit/s.\n", max);
	printf("Minimální naměřená rychlost: %f Mbit/s.\n", min);
	printf("Standardní odchylka: %f.\n", res);
	printf("Průměrný RTT paketů komunikace: %f seconds.\n", average_timer);
	
    free(buf);
   	free(buf_wr);

    

}

int main(int argc, char ** argv) {
	int argument;
	int control = 0;  //this variable will control if first argument is reflect or meter
	int port_number;
	const char *remote_hostname;
	int size = 0;
	int timer = 0;

	if (strcmp(argv[1], "reflect") == 0) {
		control = 1;

		while((argument = getopt(argc, argv, "p:")) != -1) {

			switch(argument) {

				case 'p':
					port_number = atoi(optarg);
					reflect_function(port_number);
					break;

				case '?':
					printf("Miss an argument.\n");
					break;
				default:
					fprintf(stderr, "invalid option -- %c\n", argument);
					break;

			} 
		}
		

	}
	
	else if(strcmp(argv[1], "meter") == 0) {
		control = 2;

		if(argc != 10) {
			printf("Bad count of argument.\n");
			exit(EXIT_FAILURE);
		}


		while((argument = getopt(argc, argv, "h:p:s:t:")) != -1) {
			
			switch(argument) {
				
				case 'h':
					remote_hostname = optarg;
					break;
				
				case 'p':
					if(!(atoi(optarg))) {
						fprintf(stderr, "Expected number of port.\n");
						exit(EXIT_FAILURE);
					}
					port_number = atoi(optarg);
					break;
				
				case 's':
					if (!(atoi(optarg))) {
						fprintf(stderr, "Expected size of data in socket in byte.\n");
						exit(EXIT_FAILURE);
					}
					
					size = atoi(optarg);
					break;
				
				case 't':
					if(!(atoi(optarg))) {
						fprintf(stderr, "Expected time in sec.\n");
						exit(EXIT_FAILURE);

					}
					timer = atoi(optarg);
					break;
				
				case '?':
					printf("Miss an argument\n");
					break;
				
				default:
					fprintf(stderr, "invalid option -- %c\n", argument);
					exit(EXIT_FAILURE);

			}
			
			
		}

		meter_function(port_number, remote_hostname, size, timer);
	}

	else {
		fprintf(stderr, "Expected the second argument is 'reflect' or 'meter'.\n" );
		exit(EXIT_FAILURE);
	}


}