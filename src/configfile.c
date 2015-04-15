/* -*- mode: c; c-file-style: "gnu" -*-
 * Copyright (C) 2014-2015 Cryptotronix, LLC.
 *
 * This file is part of libcryptoauth.
 *
 * libcryptoauth is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libcryptoauth is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libcryptoauth.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "util.h"
#include "command_util.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


static unsigned char
c2h(char c)
{
    if (c >= 'A')
        return (c - 'A' + 10);
    else
        return (c - '0');
}

static unsigned char
a2b(char *ptr)
{
  assert (NULL != ptr);
  return c2h( *ptr )*16 + c2h( *(ptr+1) );
}

struct lca_octet_buffer
parseStory (xmlDocPtr doc, xmlNodePtr cur) {

  xmlChar *key;
  char *key_cp;
  cur = cur->xmlChildrenNode;
  int x = 0;
  const char tok[] = " ";
  char * token;


  struct lca_octet_buffer result = {0,0};

  uint8_t *configzone = NULL;

  while (cur != NULL)
    {

      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (NULL != key)
        {
          key_cp = strdup(key);
          token = strtok(key_cp, tok);

          while (token!=NULL)
            {
              printf("%s", token);
              assert (NULL != (configzone = realloc (configzone, x + 1)));
              configzone[x] = a2b(token);
              //cz[x] = a2b(token);
              x+=1;

              // get the next token
              token = strtok(NULL, tok);

            }
          printf("\n");

          printf("ConfigZone: %s\n", key);
          xmlFree(key);
          free(key_cp);
        }

      cur = cur->next;
    }

  printf ("C: %p, l %d\n", configzone, x);
  result.ptr = configzone;
  result.len = x;

  return result;
}

int
config2bin(char *docname, struct lca_octet_buffer *out)
{

  xmlDocPtr doc;
  xmlNodePtr cur;
  struct lca_octet_buffer tmp;
  int rc = -1;

  assert (NULL != docname);
  assert (NULL != out);

  doc = xmlParseFile(docname);

  if (doc == NULL)
    {
      fprintf(stderr,"Document not parsed successfully. \n");
      rc = -2;
      goto OUT;
    }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL)
    {
      fprintf(stderr,"empty document\n");
      rc = -3;
      goto FREE;
    }

  if (xmlStrcmp(cur->name, (const xmlChar *) "ECC108Content.01"))
    {
      fprintf(stderr,"document of the wrong type, root node != ECC108Content.01");
      rc = -4;
      goto FREE;
    }

  cur = cur->xmlChildrenNode;

  while (cur != NULL)
    {
      if ((!xmlStrcmp(cur->name, (const xmlChar *)"ConfigZone")))
        {
          printf("Parsing!\n");
          tmp = parseStory (doc, cur);
          if (NULL != tmp.ptr)
            {
              out->ptr = tmp.ptr;
              out->len = tmp.len;
              rc = 0;
            }

        }

      cur = cur->next;
    }

 FREE:
  xmlFreeDoc(doc);
 OUT:
  return rc;
}


int
lca_burn_config_zone (int fd, struct lca_octet_buffer cz)
{
  bool result = false;
  int rc = -1;

  if (lca_is_config_locked (fd))
    return 0;

  assert (0 == cz.len % 4);
  assert (NULL != cz.ptr);

  int x = 0;

  for (x = 0; x < cz.len; x+=4)
    {
      if (write4 (fd, CONFIG_ZONE, x, &cz.ptr[x]))
        printf ("Write to %d success\n", x);
      else
        printf ("Write to %d Failure\n", x);
    }

  return 0;

}