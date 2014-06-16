#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <postgresql-9.3/libpq-fe.h>

#define MAXEVENTS 64

void noticeReceiver(void *arg, const PGresult *res)
{
    (void)arg;
    (void)res;
}

void noticeProcessor(void *arg, const char *message)
{
    (void)arg;
    (void)message;
}

int main (int argc, char *argv[])
{
  int efd;
  epoll_event events[MAXEVENTS];

  (void)argc;
  (void)argv;

  efd = epoll_create1 (0);
  if (efd == -1)
    {
      std::cerr << "epoll_create" << std::endl;
      abort ();
    }

  PGconn *conn=PQconnectStart("dbname=catchchallenger user=root");
  const ConnStatusType &connStatusType=PQstatus(conn);
  if(connStatusType==CONNECTION_BAD)
  {
     std::cerr << "pg connexion not OK" << std::endl;
     exit(1);
  }
  if(PQsetnonblocking(conn,1)!=0)
  {
     std::cerr << "pg no blocking error" << std::endl;
     exit(1);
  }
  int sock = PQsocket(conn);
  if (sock < 0)
  {
     std::cerr << "pg no sock" << std::endl;
     exit(1);
  }
  epoll_event event;
  event.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET ;
  event.data.fd = sock;

    // add the socket to the epoll file descriptors
    if(epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event) != 0)
    {
       std::cerr << "epoll_ctl, adding socket" << std::endl;
       exit(1);
    }

    PQsetNoticeReceiver(conn,noticeReceiver,NULL);
    PQsetNoticeProcessor(conn,noticeProcessor,NULL);

    int query_id=-1;
  /* The event loop */
  while (1)
    {
      int n, i;

      n = epoll_wait (efd, events, MAXEVENTS, -1);
      for (i = 0; i < n; i++)
        {
          if (events[i].events & EPOLLERR)
          {
             std::cerr << "epoll_ctl, socket error" << std::endl;
             continue;
          }
          else
          {
              const ConnStatusType &connStatusType=PQstatus(conn);
              if(connStatusType!=CONNECTION_OK)
              {
                  if(connStatusType==CONNECTION_MADE)
                    fprintf(stderr,"Connexion CONNECTION_MADE\n");
                  else if(connStatusType==CONNECTION_STARTED)
                    fprintf(stderr,"Connexion CONNECTION_STARTED\n");
                  else
                      fprintf(stderr,"Connexion not ok: %d\n",connStatusType);
              }
              if(connStatusType!=CONNECTION_BAD)
              {
                  const PostgresPollingStatusType &postgresPollingStatusType=PQconnectPoll(conn);
                  if(postgresPollingStatusType==PGRES_POLLING_FAILED)
                  {
                    fprintf(stderr,"Connexion status: PGRES_POLLING_FAILED, bye\n");
                    exit(1);
                  }
                  /*else if(postgresPollingStatusType==PGRES_POLLING_OK)
                    fprintf(stderr,"Connexion status: PGRES_POLLING_OK\n");*/
              }

              if (events[i].events & EPOLLOUT) //socket is ready for writing
                    std::cerr << "epoll_ctl, socket ready to write" << std::endl;

              if (events[i].events & EPOLLIN) //socket is ready for writing
              {
                std::cerr << "epoll_ctl, socket ready to read" << std::endl;
                PQconsumeInput(conn);
                PGnotify *notify;
                while ((notify = PQnotifies(conn)) != NULL)
                {
                    fprintf(stderr,
                            "ASYNC NOTIFY of '%s' received from backend PID %d\n",
                            notify->relname, notify->be_pid);
                    PQfreemem(notify);
                }
                if(PQisBusy(conn)==0)
                {
                    PGresult *result;
                    while((result = PQgetResult(conn)) != NULL)
                    {
                        if (PQresultStatus(result) != PGRES_TUPLES_OK)
                        {
                            fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(conn));
                            PQclear(result);
                        }
                        else
                        {
                            int nFields = PQnfields(result);
                            for (i = 0; i < nFields; i++)
                            {
                                int ptype = PQftype(result, i);
                                std::cout << PQfname(result, i);
                            }
                            std::cout << "\n------------\n";
                            /* next, print out the rows */
                            for (i = 0; i < PQntuples(result); i++)
                            {
                                for (int j = 0; j < nFields; j++)
                                    std::cout << PQgetvalue(result, i, j);
                                std::cout << "\n";
                            }
                            fprintf(stderr,"PG have result");
                        }
                        PQclear(result);
                    }
                    query_id=-1;
                    /*query_id=PQsendQuery(conn, "SELECT id,password FROM account WHERE id=1;");
                    if(query_id==0)
                    {
                        std::cerr << "query repeat send failed" << std::endl;
                        query_id=-1;
                    }*/
                }
              }

              if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) //socket closed, delete and create a new one
                 std::cerr << "epoll_ctl, socket closed" << std::endl;
           }
        }
  }

  PQfinish(conn);

  return EXIT_SUCCESS;
}
