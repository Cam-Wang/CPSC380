int main(int argc, char * argv[])
{
    pid_t pid;   

    FILE *input_file, *output_file;

    char file_buffer[MAX_BUF_SIZE];

    int smem_id;

    key_t smem_key = KEY_CODE;

    void *temp_ptr = (void *)0;

    struct shared_mem_struct *shared_mem;

    if(argc == 1)//basic error checking
    {
        printf("Error: Please provide input file name\n");
        exit(EXIT_FAILURE);
    }

    if(argc != 2)//checks number of args
    {
        printf("Error: Incorrect number of arguments passed\n");
        exit(EXIT_FAILURE);
    }

    initializeWait();

    if ((pid = fork()) < 0)//check if fork worked
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid != 0)//parent process starts here
    {
        if((input_file = fopen(argv[1],"r")) == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        if((smem_id = shmget(smem_key, sizeof(struct shared_mem_struct), IPC_CREAT | 0666))<0)//create shared memory
        {  
            perror("shmget");
            exit(EXIT_FAILURE);
        }
        if((temp_ptr = shmat(smem_id, (void *)0, 0)) == (void *)-1)//attempts to attatch shared memory and error checks
        {  
            perror("shmat");
            exit(EXIT_FAILURE);
        }
        shared_mem = (struct shared_mem_struct *) temp_ptr;
        shared_mem->count = 0; // Set the counter to 0
        while(fgets(file_buffer, (MAX_BUF_SIZE-1), input_file) != NULL)
        {
            while(shared_mem->count != 0)//waits for child to consume all data out of shared memory
            {
                waitChild();
            }
            strcpy(shared_mem->buffer,file_buffer);
            shared_mem->count = strlen(shared_mem->buffer);
        }
        initializeWait();
        while(shared_mem->count != 0)
        {
            waitChild();
        }
        shared_mem->count = -1;
        signalChild(pid);
        waitpid(pid, NULL, 0);
        fclose(input_file);
        if(shmdt(shared_mem) == -1)
        {
            perror("shmdt");
            exit(EXIT_FAILURE);
        }
        if(shmctl(smem_id, IPC_RMID, 0) == -1)
        {
            perror("shmctl");
            exit(EXIT_FAILURE);
        }
        printf("\nSuccess: The input file provided by you has been successfully copied via shared memory to output file named \"ouput.txt\" in the current working directory.\n\n");

    }
    else//child process starts here
    {
        if((output_file = fopen("output.txt","w")) == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        if((smem_id = shmget(smem_key, sizeof(struct shared_mem_struct), IPC_CREAT | 0666))<0)
        {
            perror("shmget");
            exit(EXIT_FAILURE);
        }
        if((temp_ptr = shmat(smem_id, (void *)0, 0)) == (void *)-1)//attempting to attatch shared memory and error checks
        {
            perror("shmat");
            exit(EXIT_FAILURE);
        }
        shared_mem = (struct shared_mem_struct *) temp_ptr;
        while(shared_mem->count != -1)
        {
            while(shared_mem->count == 0)
            {
                waitParent();
            }
            if(shared_mem->count != -1)
            {
            fputs(shared_mem->buffer, output_file);//takes everything read from shared memory to output file
            shared_mem->count = 0;   
            signalParent(getppid());   
            }
        }
        fclose(output_file);
        if(shmdt(shared_mem) == -1)
        {
            perror("shmdt");
            exit(EXIT_FAILURE)
            kill(getpid(), SIGTERM);   
        }
        exit(EXIT_SUCCESS);
    }  