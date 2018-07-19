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

int main(int argc,char *argv[])
{
  struct tls_config *config;
  
  if (argc == 1)
  {
    fprintf(stderr,"usage: %s host resource\n",argv[0]);
    return EXIT_FAILURE;
  }
  
  if (tls_init() < 0)
  {
    perror("tls_init()");
    return EXIT_FAILURE;
  }
  
  config = tls_config_new();
  if (config == NULL)
  {
    perror("tls_config()");
    return EXIT_FAILURE;
  }
  
  tls_config_free(config);
  return EXIT_SUCCESS;
}
