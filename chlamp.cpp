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
std::mutex mtx4;

char log[65536];
char tim[10];
char slt[20];
int vt[MAXCL];
int msgComplexcount[MAXCL];
int topo[MAXCL][MAXCL];
int ie, me, ms, md;
int PORT;
int clientsocks[MAXCL];
int clientList[MAXCL];
int nt, l, m, ncon, ne;
int bal = 14000;
char color = 'w';
int T = 1500;
int recmark[MAXCL];
int term = 0;
int smsgtype;
int rmsgtype;
int transacted, transactbuffer, childtransacted;
int markerrecievedpid = 0;
int readytoexit = 0;
int controlmsgcount;
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
    int amt;
}smsg, rmsg;

int mysoc, mypid;


//Get the current localtime and write it
//to the String named tim
void gettime(){
    time_t rawtime;
    struct tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);

    strftime (tim, 10,"%I:%M:%S%p",timeinfo);
}


//Connect to the smsg.did process
//and the send the message type information
//followed by the data message.
void sendMessage(){
    if(smsgtype == 0){
        me++;
        ms++;
        md+=nt*sizeof(int);
        cout << "Sending message to " << smsg.did << endl;
        if(color == 'w')transacted+=smsg.amt;
        else transactbuffer += smsg.amt;
        struct sockaddr_in address;
        int sock = 0;
        struct sockaddr_in serv_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(clientsocks[smsg.did]);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if(send(sock, &smsgtype, sizeof(smsgtype), 0)!=sizeof(smsgtype))cout << "\n-----------------------------\nError Sending Message\n----------------------------\n";
        if(send(sock, &smsg, sizeof(smsg), 0)!=sizeof(smsg))cout << "\n-----------------------------\nError Sending Message\n----------------------------\n";
        close(sock);
    }
    else if(smsgtype==1){
        cout << "Sending Marker to " << smsg.did << endl;
        controlmsgcount++;
        struct sockaddr_in address;
        int sock = 0;
        struct sockaddr_in serv_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(clientsocks[smsg.did]);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if(send(sock, &smsgtype, sizeof(smsgtype), 0)!=sizeof(smsgtype))cout << "\n-----------------------------\nError Sending Message\n----------------------------\n";
        if(send(sock, &smsg, sizeof(smsg), 0)!=sizeof(smsg))cout << "\n-----------------------------\nError Sending Message\n----------------------------\n";
        close(sock);
    }
    else if(smsgtype==2){
        cout << "Sending Terminate to " << smsg.did << endl;
        struct sockaddr_in address;
        int sock = 0;
        struct sockaddr_in serv_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(clientsocks[smsg.did]);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if(send(sock, &smsgtype, sizeof(smsgtype), 0)!=sizeof(smsgtype))cout << "\n-----------------------------\nError Sending Message\n----------------------------\n";
        if(send(sock, &smsg, sizeof(smsg), 0)!=sizeof(smsg))cout << "\n-----------------------------\nError Sending Message\n----------------------------\n";
        close(sock);
    }
    else if(smsgtype==3){
        cout << "Sending Message Complexity to " << smsg.did << endl;
        struct sockaddr_in address;
        int sock = 0;
        struct sockaddr_in serv_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(clientsocks[smsg.did]);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if(send(sock, &smsgtype, sizeof(smsgtype), 0)!=sizeof(smsgtype))cout << "\n-----------------------------\nError Sending Message\n----------------------------\n";
        if(send(sock, &smsg, sizeof(smsg), 0)!=sizeof(smsg))cout << "\n-----------------------------\nError Sending Message\n----------------------------\n";
        close(sock);
    }
    return;
}


//This function sums up the control
//message count of every process
void msgComplex(){
    int flag = 1, sum = 0;
    while(flag){
        flag =0;
        for(int i=0;i<nt;i++)
            if(i!=mypid && msgComplexcount[i] == 0){
                flag = 1;
                break;
            }
    }
    for(int i=0;i<nt;i++)
        if(i!=mypid)sum+=msgComplexcount[i];
    sum+=controlmsgcount;
    cout << "Total Message Complexity : " << sum << endl;
    sprintf(log, "%s\nMessage complexity for Chandy-Lamport Algo : %d.\n", log, sum);
}


//Wait until a marker has been recieved
//from each neighbour and then send the
//local snapshot to the coordinatior process
void snapshotCollector(){
    int flag = 1;
    while(flag){
        mtx.lock();
        flag = 0;
//        cout << "Rec Mark list : \n";
//        for(int i=0;i<nt;i++)
//            if(topo[mypid][i]!=0 && i!=mypid && i!=markerrecievedpid)
//                cout << i << " " << recmark[i] << endl;;
        for(int i=0;i<nt;i++)
            if(topo[mypid][i]!=0 && i!=mypid && i!=markerrecievedpid && recmark[i]==0){
                mtx.unlock();
                flag = 1;
                std::this_thread::sleep_for(1s);          //Check if all markers recieved every one sec.
                break;
            }
    }
    cout << "Snapshot taken by Process " << mypid << "\nAmount transacted: " << transacted << endl;
    if(mypid!=0){
        smsgtype = 1;
        smsg.sid = mypid;
        smsg.did = markerrecievedpid;
        smsg.amt = childtransacted + transacted;
        childtransacted = 0;
        sendMessage();
    }
    transacted = transactbuffer;
    transactbuffer = 0;
    color = 'w';
    mtx.unlock();
    if(term)readytoexit=1;
}


//Process 0 becomes the Coordinator
//and all snapshots are initiated by this
//process until the total amount transacted
//in the system exceeds T
void snapCord(){
    mtx4.lock();
    int totaltransacted = 0;
    int snapshotcount = 0;
    std::this_thread::sleep_for(1s);
    if(mypid==0){
        while(totaltransacted < T){
            cout << "Initiated Snapshot" << endl;
            for(int i=0;i<nt;i++)recmark[i] = 0;
            mtx.lock();
            color = 'r';
            gettime();
            sprintf(log, "%sProcess %d takes its local snapshot at %s\nAmount Transacted Since last Snapshot : Rs. %d.\nCurrent Balance  : Rs. %d.\n", log, mypid, tim, transacted, bal);
            totaltransacted += transacted;
            for(int i=0;i<nt;i++){
                if(topo[mypid][i]!=0 && i!=mypid){
                    smsg.did = i;
                    smsg.sid = mypid;
                    smsg.amt = 0;
                    smsgtype = 1;
                    sendMessage();
                }
            }
            mtx.unlock();
            std::thread collectsnap(snapshotCollector);
            collectsnap.join();
            totaltransacted += childtransacted;
            childtransacted = 0;
            snapshotcount++;
            gettime();
            sprintf(log, "%sProcess %d Collected Global Snapshot at %s\nTotal Transacted Amount in System so far : Rs. %d.\n", log, mypid, tim, totaltransacted);
            cout << "Total Transacted: " << totaltransacted << " in snapshot " << snapshotcount<< endl;
        }
        cout << "Initiated Final Snapshot" << endl;
        gettime();
        sprintf(log, "%s\nProcess %d initiated TERMINATION at %s\nTotal Transacted Amount in System so far : Rs. %d.\n", log, mypid, tim, totaltransacted);
        for(int i=0;i<nt;i++)recmark[i] = 0;
        mtx.lock();
        term = 1;
        color = 'r';
        totaltransacted += transacted;
        for(int i=0;i<nt;i++){
            if(topo[mypid][i]!=0 && i!=mypid){
                smsg.did = i;
                smsg.sid = mypid;
                smsg.amt = 0;
                smsgtype = 2;
                sendMessage();
            }
        }
        mtx.unlock();
        std::thread collectsnap(snapshotCollector);
        collectsnap.join();
        totaltransacted += childtransacted;
        childtransacted = 0;
        cout << "Total Transacted: " << totaltransacted << " before terminating." << endl;
        gettime();
        sprintf(log, "%s\nSYSTEM SUCCESSFULY TERMINATED at %s\nTOTAL TRANSACTED : %d", log, tim, totaltransacted);
        msgComplex();
    }
}


//Every process creates its own server
//so that other processes may connect to it
//All incoming messages other than those
//from the master Server arrive here
//and are then either processed as they come
//or handed over as a job to a new thread.
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
                    read(tempSocket, &rmsgtype, sizeof(rmsgtype));
                    if(rmsgtype==0){
                        read(tempSocket, &rmsg, sizeof(struct message));
                        mtx.lock();
                        bal += rmsg.amt;
                        gettime();
                        me++;
                        cout << "Recieved message from " << rmsg.sid << endl;
                        sprintf(log, "%sProcess %d recieves Rs. %d from Process %d at %s\n", log, mypid, rmsg.amt, rmsg.sid, tim);
                        mtx.unlock();
                        break;
                    }
                    else if(rmsgtype==1){
                        read(tempSocket, &rmsg, sizeof(struct message));
                        mtx.lock();
                        if(color == 'w'){
                            color = 'r';
                            gettime();
                            sprintf(log, "%sProcess %d takes its local snapshot at %s\nAmount Transacted Since last Snapshot : Rs. %d.\nCurrent Balance  : Rs. %d.\n", log, mypid, tim, transacted, bal);
                            for(int i=0;i<nt;i++)recmark[i] = 0;
                            markerrecievedpid = rmsg.sid;
                            recmark[rmsg.sid] = 1;
                            for(int i=0;i<nt;i++){
                                if(topo[mypid][i]!=0 && i!=mypid && i!=markerrecievedpid && recmark[i]==0 && mypid!=0){
                                    smsg.did = i;
                                    smsg.sid = mypid;
                                    smsg.amt = 0;
                                    smsgtype = 1;
                                    sendMessage();
                                    cout << "\nProcess " << mypid << " sent marker to :" << i << endl;
                                }
                            }
                            std::thread collectsnap(snapshotCollector);
                            collectsnap.detach();
                        }
                        else{
                            cout << "\nRed process " << mypid << " recieved marker from " << rmsg.sid << endl;
                            childtransacted += rmsg.amt;
                            recmark[rmsg.sid] = 1;
                        }
                        mtx.unlock();
                        break;
                    }
                    else if(rmsgtype==2){
                        read(tempSocket, &rmsg, sizeof(struct message));
                        mtx.lock();
                        if(color == 'w'){
                            color = 'r';
                            term = 1;
                            sprintf(log, "%sProcess %d recieved terminate at %s\n", log, mypid, tim);
                            for(int i=0;i<nt;i++)recmark[i] = 0;
                            markerrecievedpid = rmsg.sid;
                            recmark[rmsg.sid] = 1;
                            for(int i=0;i<nt;i++){
                                if(topo[mypid][i]!=0 && i!=mypid && i!=markerrecievedpid && recmark[i]==0 && mypid!=0){
                                    smsg.did = i;
                                    smsg.sid = mypid;
                                    smsg.amt = 0;
                                    smsgtype = 2;
                                    sendMessage();
                                }
                            }
                            std::thread collectsnap(snapshotCollector);
                            collectsnap.detach();
                        }
                        else{
                            childtransacted += rmsg.amt;
                            recmark[rmsg.sid] = 1;
                        }
                        mtx.unlock();
                        break;
                    }
                    else if(rmsgtype==3){
                        read(tempSocket, &rmsg, sizeof(struct message));
                        mtx.lock();
                        msgComplexcount[rmsg.sid] = rmsg.amt;
                        mtx.unlock();
                        break;
                    }
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


//This function establishes connection
//with the master Server so that it may
//send the master server his servers port no,
//and in turn the master server assigns it
//a id and also shares the ports of other
//connected process.
//Also it used for intimidation when a process leaves the system.
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
    read(sock, &nt, sizeof(nt));
    read(sock, &ncon, sizeof(ncon));
    read(sock, &l, sizeof(l));
    read(sock, &bal, sizeof(bal));
    read(sock, &T, sizeof(T));
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
//        else{
//            //cout << "Process left with id: "<< cl.pid <<" &  socket: " << clientsocks[cl.pid] <<"." << endl;
//        }
        clientsocks[cl.pid] = cl.csock;
        mtx.unlock();
    }
}


//This Function sends the log files
//to the master server so that all the log
//maybe printed in a single file.
void sendLog(){
    struct sockaddr_in serv_addr;
    double d = (double)md/ms, sock = 0;
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


//This is for simulating work done
//by the process by sleeping for
//a random period of time
void work(){
    vt[mypid]++;
    ie++;
    if(!term){
        gettime();
        sprintf(log, "%sProcess %d executed internal event e%d%d at %s\n", log, mypid, mypid, ie, tim);
        int x = rand()%l;
        cout << "Executing internal event for " << x << "secs..." << endl;
        for(int i=0;i<=x;i++)std::this_thread::sleep_for(1s);
    }
}


//This schedules the different tasks done by the process,
//like sending messages or doing work
void autoWork(){
    int rd, i;
    cout << "Waiting for processes to join..." << endl;
    mtx2.lock();
    ncon = 0;
    for(int i=0;i<nt;i++)if(topo[mypid][i]!=0 && i!=mypid)ncon++;
    cout << "Started Auto Executing Tasks..." << endl;
    while(!term){
        mtx4.unlock();
        std::thread worker(work);
        worker.join();
        rd = rand()%(ncon);
        for(i=0;rd>=0;i++)if(topo[mypid][i]!=0 && i!=mypid)rd--;
        smsg.did = i-1;
        smsg.sid = mypid;
        smsgtype = 0;
        mtx.lock();
        int x = rand()%(bal/10);
//            int x = 10;
        bal -= x;
        smsg.amt = x;
        if(!term){
            gettime();
            sprintf(log, "%sProcess %d sends Rs. %d to Process %d at %s\n", log, mypid, smsg.amt, smsg.did, tim);
            sendMessage();
        }
        mtx.unlock();
    }
//    for(int j=0;j<10;j++)std::this_thread::sleep_for(1s);
    while(!readytoexit)std::this_thread::sleep_for(1s);
    sendLog();
    mtx.lock();
    smsgtype=3;
    smsg.sid = mypid;
    smsg.did = 0;
    smsg.amt = controlmsgcount;
    sendMessage();
    mtx.unlock();
    cout << "Message Complexity : " << controlmsgcount << endl;
    cout << "All tasks completed..." << endl;
}



//The main function simply spawns various threads
//and waits for them to finish.
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
    mtx4.lock();
    lthread = pthread_create(&threads[0], NULL, server, (void *)&tid[0]);
    sthread = pthread_create(&threads[1], NULL, getPid, (void *)&tid[1]);
    std::thread autow(autoWork);
    std::thread snapcoord(snapCord);
    autow.join();
    snapcoord.join();
    while(!readytoexit)sleep(1);
}
