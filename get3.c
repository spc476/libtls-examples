/************************************************************************
*
* Copyright 2018 by Sean Conner.  All Rights Reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <tls.h>

struct tls_config *g_config = NULL;
struct tls        *g_tls    = NULL;
int                g_sock   = -1;
FILE              *g_fpin   = NULL;
FILE              *g_fpout  = NULL;

/***********************************************************************/

static void cleanup(void)
{
  if (g_tls != NULL)
  {
    tls_close(g_tls);
    tls_free(g_tls);
  }
  
  if (g_config != NULL) tls_config_free(g_config);
  if (g_sock   != -1)   close(g_sock);
  if (g_fpout  != NULL) fclose(g_fpout);
  if (g_fpin   != NULL) fclose(g_fpin);
}

/***********************************************************************/

static int connect_to_host(char const *host,char const *port)
{
  struct addrinfo  hints;
  struct addrinfo *results = NULL;
  int              sock;
  int              rc;
  
  memset(&hints,0,sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family   = AF_INET;
  
  if ((rc = getaddrinfo(host,port,&hints,&results)) != 0) return rc;
  
  sock = socket(AF_INET,SOCK_STREAM,0);
  if (sock < 0)
  {
    freeaddrinfo(results);
    return -1;
  }
  
  if (fcntl(sock,F_SETFL,O_NONBLOCK) < 0)
  {
    freeaddrinfo(results);
    close(sock);
    return -1;
  }
  
  rc = connect(sock,results->ai_addr,results->ai_addrlen);
  if ((rc == -1) && (errno == EINPROGRESS))
    return sock;
  
  freeaddrinfo(results);
  close(sock);
  return -1;
}

/***********************************************************************/

static void wait_for_io(int sock,char const *tag)
{
  struct pollfd fds;
  int           rc;
  
  fprintf(stderr,"waiting for IO (%s), we can switch now ... \n",tag);
  fds.fd     = sock;
  fds.events = POLLIN | POLLOUT;
  rc         = poll(&fds,1,-1);
  
  if (rc == -1)
  {
    perror("poll()");
    exit(EXIT_FAILURE);
  }
}

/***********************************************************************/

static ssize_t cb_read(struct tls *ctx,void *buf,size_t buflen,void *usr)
{
  ssize_t  in;
  int     *psock = usr;
  
  (void)ctx;
  
  while(1)
  {
    wait_for_io(*psock,"read");
    in = read(*psock,buf,buflen);
    if (in > 0)
      fwrite(buf,1,(unsigned)in,g_fpin);
    if ((in < 0) && (errno == EAGAIN))
      continue;
    return in;
  }
}

/***********************************************************************/

static ssize_t cb_write(struct tls *ctx,void const *buf,size_t buflen,void *usr)
{
  ssize_t  out;
  int     *psock = usr;

  (void)ctx;
  
  while(1)
  {
    wait_for_io(*psock,"write");
    out = write(*psock,buf,buflen);
    if (out > 0)
      fwrite(buf,1,(unsigned)out,g_fpout);
    if ((out < 0) && (errno == EAGAIN))
      continue;
    return out;
  }
}

/***********************************************************************/

int main(int argc,char *argv[])
{
  char  buffer[BUFSIZ];
  char *p;
  int   sock;
  int   rc;
  int   writebytes;
  
  if (argc == 1)
  {
    fprintf(stderr,"usage: %s host resource\n",argv[0]);
    return EXIT_FAILURE;
  }
  
  atexit(cleanup);
  
  g_fpin = fopen("input.bin","wb");
  if (g_fpin == NULL)
  {
    perror("input.bin");
    return EXIT_FAILURE;
  }
  
  g_fpout = fopen("output.bin","wb");
  if (g_fpout == NULL)
  {
    perror("output.bin");
    return EXIT_FAILURE;
  }
  
  if (tls_init() < 0)
  {
    perror("tls_init()");
    return EXIT_FAILURE;
  }
  
  g_config = tls_config_new();
  if (g_config == NULL)
  {
    perror("tls_config()");
    return EXIT_FAILURE;
  }
  
  rc = tls_config_set_protocols(g_config,TLS_PROTOCOLS_ALL);
  if (rc != 0)
  {
    perror(tls_config_error(g_config));
    return EXIT_FAILURE;
  }
  
  sock = connect_to_host(argv[1],"https");
  if (sock == -1)
  {
    perror("connect_to_host");
    return EXIT_FAILURE;
  }
  
  wait_for_io(sock,"connect");
  
  g_tls = tls_client();
  if (g_tls == NULL)
  {
    perror("tls_client()");
    return EXIT_FAILURE;
  }
  
  rc = tls_configure(g_tls,g_config);
  if (rc != 0)
  {
    perror(tls_error(g_tls));
    return EXIT_FAILURE;
  }
  
  rc = tls_connect_cbs(g_tls,cb_read,cb_write,&sock,argv[1]);
  if (rc != 0)
  {
    perror(tls_error(g_tls));
    return EXIT_FAILURE;
  }
  
  writebytes = snprintf(
            buffer,
            sizeof(buffer),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: TLSTester/1.0 (TLS Testing Program)\r\n"
            "Connection: close\r\n"
            "Accept: */*\r\n"
            "\r\n",
            argv[2],
            argv[1]
          );
  if ((writebytes < 0) || ((unsigned)writebytes > sizeof(buffer)))
  {
    fprintf(stderr,"snprintf() error\n");
    return EXIT_FAILURE;
  }
  
  p = buffer;
  
  while(writebytes > 0)
  {
    ssize_t outbytes = tls_write(g_tls,p,(size_t)writebytes);
    if ((outbytes == TLS_WANT_POLLIN) || (outbytes == TLS_WANT_POLLOUT))
      continue;
    if (outbytes < 0)
    {
      perror(tls_error(g_tls));
      return EXIT_FAILURE;
    }
    
    writebytes -= outbytes;
    p          += outbytes;
  }
  
  while(1)
  {
    ssize_t inbytes = tls_read(g_tls,buffer,sizeof(buffer));
    if ((inbytes == TLS_WANT_POLLIN) || (inbytes == TLS_WANT_POLLOUT))
      continue;
    if (inbytes < 0)
    {
      perror(tls_error(g_tls));
      return EXIT_FAILURE;
    }
    
    if (inbytes == 0) break;
    fwrite(buffer,1,(size_t)inbytes,stdout);
  }
  
  return EXIT_SUCCESS;
}

/***********************************************************************/
