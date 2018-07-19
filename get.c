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

#include <tls.h>

struct tls_config *g_config = NULL;
struct tls        *g_tls    = NULL;

/***********************************************************************/

void cleanup(void)
{
  if (g_tls)
  {
    tls_close(g_tls);
    tls_free(g_tls);
  }
  
  if (g_config) tls_config_free(g_config);
}

/***********************************************************************/

int main(int argc,char *argv[])
{
  char  buffer[BUFSIZ];
  char *p;
  int   rc;
  int   writebytes;
  
  if (argc == 1)
  {
    fprintf(stderr,"usage: %s host resource\n",argv[0]);
    return EXIT_FAILURE;
  }
  
  atexit(cleanup);
  
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
  
  rc = tls_connect(g_tls,argv[1],"https");
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
