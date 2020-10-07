#include<iostream>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/time.h>
#include<unistd.h>
#include<pthread.h>
#include<mutex>
#include<chrono>
#include<thread>
#include<time.h>

#define  MAXCL 100

using namespace std;

std::mutex mtx;
std::mutex mtx1;
std::mutex mtx2;
std::mutex mtx3;

char log[65536];
char tim[10];
char slt[20];
int vt[MAXCL];
int topo[MAXCL][MAXCL];
int ie, me, ms, md;
int PORT;
int clientsocks[MAXCL];
int clientList[MAXCL];
int nt, l, m, ncon, ne;
double a;

struct socmsg{
    int id;
    int soc;
};

struct nclient{
    int pid;
    int csock;
};

struct message{
    int sid;
    int did;
    int mvt[MAXCL];
}smsg, rmsg;

int mysoc, mypid;

void gettime(){
    time_t rawtime;
    struct tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);

    strftime (tim, 10,"%I:%M%p",timeinfo);
}


void* server(void * tid){
    int opt = 1;
    int serverSocket, addrl, tempSocket, maxClients = MAXCL, activity, i, valread, socdescp, temp;
    int maxsd;
    struct sockaddr_in ipaddr;
    srand(time(0));
    PORT = rand()%10000 + 5000;
    fd_set readfds;
    serverSocket = socket(AF_INET , SOCK_STREAM, 0);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    ipaddr.sin_family = AF_INET;
    ipaddr.sin_addr.s_addr = INADDR_ANY;
    ipaddr.sin_port = htons(PORT);
    if(bind(serverSocket, (struct sockaddr *)&ipaddr, sizeof(ipaddr))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    mysoc = serverSocket;
    listen(serverSocket, 3);
    addrl = sizeof(ipaddr);
    mtx1.unlock();
    while(1){
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        maxsd = serverSocket;
        for(i=0;i<maxClients;i++){
            socdescp = clientList[i];
            if(socdescp > 0)FD_SET(socdescp, &readfds);
            if(socdescp > maxsd)maxsd = socdescp;
        }
        activity = select(maxsd + 1, &readfds, NULL, NULL, NULL);
        if(FD_ISSET(serverSocket, &readfds))
        {
            tempSocket = accept(serverSocket, (struct sockaddr *)&ipaddr, (socklen_t*)&addrl);
            struct message msg;
            for(i=0;i<maxClients;i++){
                if(clientList[i] == 0){
                    clientList[i] = tempSocket;
                    read(tempSocket, &rmsg, sizeof(struct message));
                    vt[mypid]++;
                    for(int i=0;i<nt;i++){
                        if(vt[i]<rmsg.mvt[i] && i!= mypid)vt[i] = rmsg.mvt[i];
                    }
                    gettime();
                    me++;
                    cout << "Recieved message from " << rmsg.sid << endl;
                    sprintf(log, "%s\nProcess %d recieved message m%d%d from Process %d at %s: [", log, mypid, mypid, me, rmsg.sid, tim);
                    for(int i=0;i<nt;i++)sprintf(log, "%s %d", log, vt[i]);
                    sprintf(log, "%s ]", log);
                    break;
                }
            }
        }
        for(i=0;i<maxClients;i++){
            socdescp = clientList[i];
            if(FD_ISSET(socdescp, &readfds)){
                if((valread = read(socdescp, &temp, sizeof(int))) == 0){
                    getpeername(socdescp, (struct sockaddr*)&ipaddr, (socklen_t*)&addrl);
                    close(socdescp);
                    clientList[i] = 0;
                }
            }
        }
    }
}

void *getPid(void *tid){
    mtx1.lock();
    struct sockaddr_in address;
    int sock = 0;
    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9122);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    read(sock, &mypid, sizeof(int));
    cout << "My Process id: " << mypid << "." << endl;
    send(sock, &PORT, sizeof(int), 0);
    read(sock, &clientsocks, sizeof(int)*MAXCL);
    read(sock, &topo, sizeof(int)*MAXCL*MAXCL);
    read(sock, &nt, sizeof(int));
    read(sock, &ncon, sizeof(int));
    read(sock, &l, sizeof(int));
    read(sock, &a, sizeof(double));
    read(sock, &m, sizeof(int));
    cout << "Processes currently online:" << endl;
    mtx.lock();
    for(int i=0;i<nt;i++){
        if(clientsocks[i]>0 && i!=mypid){
            cout << i << " ";
            ne++;
        }
    }
    cout << endl;
    if(ne>=ncon)mtx2.unlock();
    if(ne<ncon)cout << "Waiting for " << ncon-ne << " processes to join..." << endl;
    mtx.unlock();
    cout << endl;
    struct nclient cl;
    mtx1.unlock();
    while(1){
        read(sock, &cl, sizeof(struct nclient));
        if(ne<ncon)cout << "Message From Server." << endl;
        mtx.lock();
        if(cl.csock>0){
            if(ne<ncon)cout << "New Process joined with id: " << cl.pid << " & socket: " << cl.csock << "." << endl;
            ne++;
            if(ne<ncon)cout << "Waiting for " << ncon-ne << " more processes to join..." << endl;
            if(ne>=ncon)mtx2.unlock();
        }
        else{
            //cout << "Process left with id: "<< cl.pid <<" &  socket: " << clientsocks[cl.pid] <<"." << endl;
        }
        clientsocks[cl.pid] = cl.csock;
        mtx.unlock();
    }
}


void sendMessage(){
    gettime();
    me++;
    ms++;
    md+=nt*sizeof(int);
    struct sockaddr_in address;
    int sock = 0;
    cout << "Sending message to " << smsg.did << endl;
    sprintf(log, "%s\nProcess %d sent message m%d%d to Process %d at %s: [", log, mypid, mypid, me, smsg.did, tim);
    for(int i=0;i<nt;i++)sprintf(log, "%s %d", log, vt[i]);
    sprintf(log, "%s ]", log);
    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(clientsocks[smsg.did]);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    send(sock, &smsg, sizeof(struct message), 0);
    close(sock);
    return;
}

void sendLog(){
    struct sockaddr_in serv_addr;
    double d = (double)md/ms, sock = 0;
    sprintf(log, "%s\nAverage size of message for Process %d is %0.2lf bytes.\n", log, mypid, d);
    char tempmsg[1024];
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9123);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    read(sock, tempmsg, 1024);
    send(sock, &d, sizeof(d), 0);
    send(sock, log, strlen(log), 0);
    close(sock);
    return;
}


void work(){
    cout << "Executing internal event for " << l << "secs..." << endl;
    vt[mypid]++;
    gettime();
    ie++;
    sprintf(log, "%s\nProcess %d executed internal event e%d%d at %s: [", log, mypid, mypid, ie, tim);
    for(int i=0;i<nt;i++)sprintf(log, "%s %d", log, vt[i]);
    sprintf(log, "%s]", log);
    for(int i=0;i<l;i++)std::this_thread::sleep_for(1s);
}
int hcf(int a1, int a2){
    int hc = 1;
    for(int i=2;i<=(a1<a2?a1:a2);i++)if(a1%i==0 && a2%i==0)hc = i;
    return hc;
}
void autoWork(){
    int a2 = 1, b1, b2, rd, i, hc;
    cout << "Waiting for processes to join..." << endl;
    mtx2.lock();
    while(a!=(int)a){
        a*=10;
        a2*=10;
    }
    hc = hcf(a, a2);
    a /= hc;
    a2 /= hc;
    ncon = 0;
    for(int i=0;i<nt;i++)if(topo[mypid][i]!=0 && i!=mypid)ncon++;
    cout << "Started Auto Executing Tasks..." << endl;
    while(m>0){
        b1 = a;
        b2 = a2;
        while(b1-- && m--){
            std::thread worker(work);
            worker.join();
        }
        while(b2-- && m--){
            rd = rand()%(ncon);
            for(i=0;rd>=0;i++)if(topo[mypid][i]!=0 && i!=mypid)rd--;
            smsg.did = i-1;
            smsg.sid = mypid;
            vt[mypid]++;
            for(i=0;i<nt;i++)smsg.mvt[i] = vt[i];
            mtx.lock();
            sendMessage();
            mtx.unlock();
            for(int j=0;j<l;j++)std::this_thread::sleep_for(1s);
        }
    }
    l = rand()%(nt/2);
    for(int j=0;j<l;j++)std::this_thread::sleep_for(1s);
    sendLog();
    cout << "All tasks completed..." << endl;
}
int main(){
    pthread_t threads[3];
    int sthread, lthread, tid[2] = {0, 1}, w = 0, ch;
    char temp[1024];
    for (int i = 0; i < MAXCL; i++)clientList[i] = 0, clientsocks[i] = 0;
    ne++;
    srand(time(0));
    mtx1.lock();
    mtx2.lock();
    mtx3.lock();
    lthread = pthread_create(&threads[0], NULL, server, (void *)&tid[0]);
    sthread = pthread_create(&threads[1], NULL, getPid, (void *)&tid[1]);
    std::thread autow(autoWork);
    autow.join();
}
