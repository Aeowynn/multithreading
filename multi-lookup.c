// Name: Kara James
// Project Assignment #2

#include "multi-lookup.h"
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define INPUTFS "%1024s"
#define SBUFSIZE 1025

//initialize mutex
pthread_mutex_t lock;
int MAX_NAME_LENGTH = 1025;	//chars
int MAX_IP_LENGTH = 16;	//chars
char errorstr[SBUFSIZE];
int i;
int DONE;
//inititialize queue from queue.c
queue q;

void* Request_fn(void* argv) {
	char* hostname;	//char for storing the hostname
	//initialize hostname to malloc-ed section //its own little box
	hostname = malloc (1025*sizeof(char));
	FILE* inputfp = NULL;
	
	//check to see if made it into Request Function
	printf("Requester Starting: argv is %s \n", ((char*)argv));
    
	//open the input file
	inputfp = fopen(argv, "r");
	if(!inputfp) {
		sprintf(errorstr, "Requester: Error Opening Input File: %s \n", ((char*)argv));	//prints error string //should print to stderror instead
		perror(errorstr);	//prints an error to terminal
	}

	//Scan file
	while(fscanf(inputfp, INPUTFS, hostname) > 0) {
		//check if queue is full
		while (queue_is_full(&q)) {
			int rnum = rand() % 100 + 1;
			//printf("Sleeping: %d \n", rnum);
			usleep(rnum);
		}
		//lock
		pthread_mutex_lock(&lock);
		//add to queue
		//printf("Pushing: %s\n", hostname);
		queue_push(&q, hostname);
		//unlock
		pthread_mutex_unlock(&lock);
		//malloc more memory
		hostname = malloc (1025*sizeof(char));
	}
	
	//free that extra bug
	free(hostname);
	//Close Input File
	fclose(inputfp);
	
	return NULL;
}

void* Resolve_fn(void* output) {
	char* hostname;	//char for storing the hostname
	char firstipstr[INET6_ADDRSTRLEN]; //holds ip address

	printf("Hi! I am a resolver thread!\n");
    //pop
    while(!(queue_is_empty(&q)) || !DONE) {
		//check if queue is empty
		while (queue_is_empty(&q)) {
			int rnum = rand() % 100 + 1;
			//printf("Resolver sleeping: %d \n", rnum);
			usleep(rnum);
		}
		//lock
		pthread_mutex_lock(&lock);
		//pop
		hostname = (char*)queue_pop(&q);
		//unlock
		pthread_mutex_unlock(&lock);
	    //Lookup hostname and get IP string
	    if(dnslookup(hostname, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE) {
			fprintf(stderr, "Resolver: dnslookup error: %s\n", hostname);
			strncpy(firstipstr, "", sizeof(firstipstr));
	    }
	    // Write to Output File
		//print to file
		//lock
		pthread_mutex_lock(&lock);
		//print to output file
	    fprintf(output, "%s,%s\n", hostname, firstipstr);
	    //unlock
		pthread_mutex_unlock(&lock);
		//free malloc-ed stuff
		free(hostname);
	}
    
    return NULL;
}

int main(int argc, char* argv[]) //argc = # of files plus 1 b/c multilookup
{
	printf("Starting Multi-Lookup's Main Function \n");
	//YAY QUEUE
	queue_init(&q, 50);
	//find number of cores, save to numCPU, it's 2
	int num = 2*sysconf( _SC_NPROCESSORS_ONLN );
	
	//Error checking for argc
    if(argc < MINARGS) {
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }
	
	//Setup Local Vars for threads
    pthread_t requesters[argc-2];
    pthread_t resolvers[num];
    int rc;
    long t;
    
	//Create Requester threads
	for(t=1; t < (argc - 1); t++){
	printf("Creating Requester thread %ld\n", t);
	printf("argvt: %lu, %s \n", t, argv[t]);
	rc = pthread_create(&(requesters[t-1]), NULL, Request_fn, argv[t]);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
			return 0;
		}
	}
	
	FILE* outputfp = NULL;
	// Open Output File
    outputfp = fopen(argv[argc-1], "w");
    if (!outputfp) {
		perror("Resolver: Error Opening Output File\n");
    }
	
	//Create Resolver threads
	for(t=0; t<num; t++) {
		printf("Creating Resolver thread %ld\n", t);
		rc = pthread_create(&(resolvers[t]), NULL, Resolve_fn, outputfp);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
			return 0;
		}
	}
	printf("Finished making threadzorz \n");
	
    //Wait for requesters
    for(t=1;t<(argc-1);t++) {
		pthread_join(requesters[t-1],NULL);
    }
    DONE=1;
    
    //Wait for resolvers
    for(t=0;t<num;t++) {
		pthread_join(resolvers[t],NULL);
    }
    // Close Output File 
    fclose(outputfp);
    
    printf("All of the threads were completed!\n");
    
    //deallocate queue space
    queue_cleanup(&q);
    
    return 0;
}
	
