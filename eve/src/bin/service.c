/*
 * Copyright (C) Canon Inc. 2014
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Canon Inc. nor the names of 
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CANON INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CANON INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <glib-object.h>
#include <libsoup/soup.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <string.h>
#include <EWebKit.h>

#include "service.h"

/* This structure contains information for a group of services presented on an item.
 * Group of service which represent a device
 * - zeroconf : services with the same name by with a différent id
 *              (group different protocol/type to access to a device)
 * - upnp     : services with the same base id (udn) 
 */
struct _Service_Item {
   Eve_Network_Type type;
   Eina_Stringshare *text;
#define MAX_SUBTEXT_LEN 128
   char *sub_text;
   struct {
      SoupSession *soup_session;
      SoupMessage *soup_msg;
      Eina_Stringshare *url;
      const void *data;
      size_t size;
   } icon;
   const void *widget;
   Eina_Bool widget_allowed;
   const void *parent_widget;
   const Service_Item *parent;
   Eina_List *children;
   Eina_List *services;
   struct {
      Eina_Stringshare *origin;
      union {
         Services *services;
         Device *device;
      } type;
      void *icon;
   } config;
};

typedef struct _Device_Config {
  Eina_Stringshare *url;
  Eina_Stringshare *port;
  Eina_Stringshare *friendlyName;
  Eina_Stringshare *modelDescription;
  Eina_Stringshare *modelName;
  Eina_Stringshare *icon_url;
} Device_Config;

static const char *_config_filename = NULL;

static char*
service_get_sub_name(Ewk_NetworkService *service, Eve_Network_Type type)
{
   if (type == NETWORK_TYPE_SERVICE_UPNP) {
      const char *name = ewk_network_service_name_get(service);
      char *ptr;
      name = strstr(name, ":serviceId:");
      name+= strlen(":serviceId:");
      ptr = strchr(name, ':');
      return ptr ? strndup(name, (ptr-name)) : strdup(name);
   
   } else if (type == NETWORK_TYPE_SERVICE_ZEROCONF) {
      const char *type = ewk_network_service_type_get(service);
      char *off;
      size_t len;
      type += strlen("zeroconf:_");
      off = strchr(type, '.');
      len = off - type;
      return strndup(type, len);
   }

   return NULL;
}

static const char*
service_get_name(Ewk_NetworkService *service, Eve_Network_Type type)
{
   if (type == NETWORK_TYPE_SERVICE_ZEROCONF) {
      const char *name = ewk_network_service_name_get(service);
      return eina_stringshare_add(name);
   }

   return NULL;
}

static char*
service_hash_key_get(Ewk_NetworkService *service, Eve_Network_Type *type)
{
    const char *type_str = ewk_network_service_type_get(service);

    if (!strncmp(type_str, "upnp:", 5)) {
        const char *srv_id = ewk_network_service_id_get(service);
        char *id;
        char *ptr;
        
        id = strdup(srv_id); 
        *type = NETWORK_TYPE_SERVICE_UPNP;
        strtok_r(id, ":", &ptr);
        memmove(id, ptr, strlen(ptr));
        return strtok(id, ":");

   } else if (!strncmp(type_str, "zeroconf:", 9)) {
       const char *srv_name = ewk_network_service_name_get(service);
       char *name;

       name = strdup(srv_name);
       *type = NETWORK_TYPE_SERVICE_ZEROCONF;
       return name;
   }

   return NULL;
}

static xmlXPathObjectPtr 
getNodeSet(xmlDocPtr doc, const xmlChar *ns, xmlChar *xpath)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	context = xmlXPathNewContext(doc);
	if (context) {
        if (xmlXPathRegisterNs(context, (xmlChar*)"ns", ns))
            return 0;

	    result = xmlXPathEvalExpression(xpath, context);
	    xmlXPathFreeContext(context);

	    if (result && xmlXPathNodeSetIsEmpty(result->nodesetval)) {
		    xmlXPathFreeObject(result);
            return 0;
        }
	}
	return result;
}

static void 
service_device_config_get(Device_Config *dev, Ewk_NetworkService *service)
{
    const char *config = ewk_network_service_config_get(service);
    const char *service_url = ewk_network_service_url_get(service);
    char *url = strdup(service_url);
    char *ptr;
    char *port;
    int base_url_len;
    xmlDocPtr doc;
    xmlNodePtr root;    
    xmlChar* xpathFriendlyName = (xmlChar*) "//ns:friendlyName";
    xmlChar* xpathModelName = (xmlChar*) "//ns:modelName";
    xmlChar* xpathModelDesc = (xmlChar*) "//ns:modelDescription";
    xmlChar* xpathIconList = (xmlChar*) "///ns:icon";
    xmlXPathObjectPtr friendlyNameObj;
    xmlXPathObjectPtr modelNameObj;
    xmlXPathObjectPtr modelDescObj;
    xmlXPathObjectPtr iconList;
    const xmlChar *ns = 0;

    if (!config || !service_url)
        return;

    /* Get base url and port */
    ptr = strchr(url, ':');
    ptr = strchr(++ptr, ':');
    ptr[0] = '\0';
    port = ++ptr;
    ptr = strchr(port, '/');
    ptr[0] = '\0';
    dev->url = eina_stringshare_add(url);
    dev->port = eina_stringshare_add(port);
    free(url);

    ptr = strchr(service_url, ':'); /* offset "protocol://" */
    ptr+= 3; /* skip "://" */
    ptr = strchr(ptr, '/');
    base_url_len = ptr - service_url;

    xmlKeepBlanksDefault(0); 

    doc = xmlParseMemory(config, strlen(config));
    if (!doc)
        return;

    root = xmlDocGetRootElement(doc);
    if (root)
        ns = root->ns->href;

    /*Get friendlyName attribute */
    friendlyNameObj = getNodeSet(doc, ns, xpathFriendlyName);
    if (friendlyNameObj) {
        xmlNodePtr node = friendlyNameObj->nodesetval->nodeTab[0];

        dev->friendlyName = eina_stringshare_add(node->children->content);

        xmlXPathFreeObject(friendlyNameObj);
    }

    /*Get modelName attribute */
    modelNameObj = getNodeSet(doc, ns, xpathModelName);
    if (modelNameObj) {
        xmlNodePtr node = modelNameObj->nodesetval->nodeTab[0];

        dev->modelName = eina_stringshare_add(node->children->content);

        xmlXPathFreeObject(modelNameObj);
    }

    /*Get modelDescription attribute */
    modelDescObj = getNodeSet(doc, ns, xpathModelDesc);
    if (modelDescObj) {
        xmlNodePtr node = modelDescObj->nodesetval->nodeTab[0];

        dev->modelDescription = eina_stringshare_add(node->children->content);

        xmlXPathFreeObject(modelDescObj);
    }
    
    /*Get icon url */
    iconList = getNodeSet(doc, ns, xpathIconList);
    if (iconList) {
        int i;
#define MIN_ICON_WIDTH 32
        int icon_width = 0;
        char *icon_url;
        xmlNodeSetPtr nodeSet = iconList->nodesetval;

        for (i=0; i < nodeSet->nodeNr; i++) {
            xmlNodePtr iconNode = nodeSet->nodeTab[i];
            xmlNodePtr childNode = iconNode->xmlChildrenNode; 
            xmlNodePtr next = childNode;
            char *mimetype = 0;
            char *width_str = 0;
            char *url = 0;
            int width;

            while (next) {
                char *value = (char*)xmlNodeListGetString(doc, next->xmlChildrenNode, 1);
                
                if (!strcmp((const char*)next->name, "mimetype")) {
                    if (strcmp(value, "image/png"))
                        break;
                    mimetype = strdup(value);
                } else if (!strcmp((const char*)next->name, "width"))
                    width_str = strdup(value);
                else if (!strcmp((const char*)next->name, "url"))
                    url = strdup(value);

                next = xmlNextElementSibling(next);
                xmlFree(value);
            }

            if (!mimetype)
                continue;

            width = atoi(width_str);

            if (!icon_width) {
                icon_width = width;
                icon_url = strdup(url);
            } 
            else if (( (width < icon_width) && (width >= MIN_ICON_WIDTH) ) ||
                ( (width > icon_width) && (icon_width < MIN_ICON_WIDTH) )) {
                free(icon_url);
                icon_url = strdup(url);
            }
                
            /*printf("Icon : \n"
                    "\t mimetype : %s\n"
                    "\t width    : %d\n"
                    "\t url      : %s\n",
                    mimetype, width, url);*/

            if (mimetype) free(mimetype);
            if (width) free(width_str);
            if (url) free(url);
        }
        
        /* store icon url */
        ptr = malloc(base_url_len + strlen(icon_url) + 1);
        strncpy(ptr, service_url, base_url_len);
        strcpy(&ptr[base_url_len], icon_url);
        dev->icon_url = eina_stringshare_add(ptr);
        free(ptr);

        xmlXPathFreeObject(iconList);
    }
}

static void 
service_device_config_clear(Device_Config *dev)
{
  eina_stringshare_del(dev->url);
  eina_stringshare_del(dev->port);
  eina_stringshare_del(dev->friendlyName);
  eina_stringshare_del(dev->modelDescription);
  eina_stringshare_del(dev->modelName);
  eina_stringshare_del(dev->icon_url);
}

static void
soup_session_callback(SoupSession *session, SoupMessage *msg, gpointer userData)
{
    const gchar* header;    
    Service_Item *item = (Service_Item*)userData;

    if (SOUP_STATUS_IS_REDIRECTION(msg->status_code)) {
        header = soup_message_headers_get_one(msg->response_headers, "Location");
        if (header) {
            msg = soup_message_new("GET", header);
            soup_session_queue_message(session, msg, &soup_session_callback, 0);
        }
        return;
    }
    
    if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code))
        return;
    
    if (msg->response_body->length > 0) {
        item->icon.data = msg->response_body->data;
        item->icon.size = msg->response_body->length;
 
        if (item->widget) {
            /* Create graphic object for Services config 
             * This able to keep persistence of icon in config file
             */
            item->config.icon = service_icon_from_data_get(item->widget, item->icon.data, item->icon.size);

            service_item_widget_update(item->widget);
        }
    }
}

static void 
service_item_icon_data_load(Service_Item *item)
{
    static SoupSession *session = NULL;
    SoupMessage *msg;
    
    if (!session)
        session = soup_session_async_new();

    msg = soup_message_new("GET", item->icon.url);

    if (!msg) {
        ERR("cannot allocate a new soup message (session %p, url %s)", session, item->icon.url);
        return;
    }

    item->icon.soup_session = session;
    item->icon.soup_msg = msg;
    g_object_ref(msg);

    soup_session_queue_message(session, msg, soup_session_callback, item);
}

static Service_Item *
service_item_new( const Device_Config *dev, 
                  const char *text, 
                  char *sub_text, 
                  Ewk_NetworkService *service, 
                  Eve_Network_Type type,
                  const Service_Item *parent,
                  Eina_List **service_items)
{
    Service_Item *item;

    item = (Service_Item*)malloc(sizeof(Service_Item));

    memset(item, 0, sizeof(Service_Item));

    switch(type) 
    {
    case NETWORK_TYPE_DEVICE_UPNP:
        item->text = eina_stringshare_add(dev->friendlyName);

        item->icon.url = eina_stringshare_add(dev->icon_url);

        /* queue an icon request */
        service_item_icon_data_load(item);

        break;

    case NETWORK_TYPE_SERVICE_UPNP:
        item->text = eina_stringshare_add(dev->modelDescription ? dev->modelDescription : dev->modelName);
        
        item->icon.url = eina_stringshare_add(dev->icon_url);

        /* queue an icon request */
        service_item_icon_data_load(item);
        break;

    case NETWORK_TYPE_SERVICE_ZEROCONF:
        item->text = service_get_name(service, type);
        break;

    default:
        item->text = eina_stringshare_add(text);
    }

    if (service && (type != NETWORK_TYPE_SERVICE_UPNP))
        item->services = eina_list_append(item->services, service);
    
    if (sub_text) {
        item->sub_text = (char*)malloc(MAX_SUBTEXT_LEN);
        strncpy(item->sub_text, sub_text, MAX_SUBTEXT_LEN-1);
    }

    item->type = type;
    item->parent = parent;
    item->widget_allowed = EINA_TRUE;
    item->config.origin = eina_stringshare_add(text);

    *service_items = eina_list_append(*service_items, item);

    return item;
}

static void 
service_item_free(Service_Item *item)
{
    eina_stringshare_del(item->text);
    eina_stringshare_del(item->config.origin);

    if (item->icon.soup_msg) {
        soup_session_cancel_message(item->icon.soup_session, item->icon.soup_msg, SOUP_STATUS_CANCELLED);
        g_object_unref(item->icon.soup_msg);
    }

    if (item->sub_text)
        free(item->sub_text);

    free(item);
}

void service_items_clear(Eina_List **service_items)
{
  Eina_List *l;
  Service_Item *item;

  EINA_LIST_FOREACH(*service_items, l, item)
      service_item_free(item);

  *service_items = eina_list_free(*service_items);
}

int service_item_compare(const void *item1, const void *item2)
{
   int ret;
   Service_Item *si1 = elm_object_item_data_get((const Elm_Object_Item *)item1);
   Service_Item *si2 = elm_object_item_data_get((const Elm_Object_Item *)item2);

   if (!(ret = strcmp(si1->text, si2->text)))
       return 0;

   return (ret > 0 ? -1 : 1);
}

static void 
service_item_children_add(Service_Item *item, Service_Item *child)
{
  item->children = eina_list_append(item->children, child);
  child->parent_widget = item->widget;
}

Eve_Network_Type service_item_type_get(const Service_Item *item)
{
    return item->type;
}

char *service_item_text_get(const Service_Item *item)
{
    return item->text ? strdup(item->text) : NULL;
}

char *service_item_subtext_get(const Service_Item *item)
{
    return item->sub_text ? strdup(item->sub_text) : NULL;
}

Eina_Bool service_item_have_service(const Service_Item *item)
{
    return item->services ? EINA_TRUE : EINA_FALSE;
}

Eina_Bool service_item_have_double_label(const Service_Item *item)
{
    return item->sub_text ? EINA_TRUE : EINA_FALSE;
}

static Network_Origin*
service_config_network_origin_get(const Network *network, const char *origin)
{
    Network_Origin *network_origin;
    Eina_List *origins = network_origins_list_get(network);
    const char *l_origin;
    Eina_List *l;

    EINA_LIST_FOREACH(origins, l, network_origin)
    {
        l_origin = network_origin_origin_get(network_origin);
        if (!strcmp(origin, l_origin))
            return network_origin;
    }
    
    return NULL;
}

static Device*
service_config_device_get(const Network_Origin *network_origin, const char *url, const char *friendlyName)
{
    Device *device;
    Eina_List *devices = network_origin_devices_list_get(network_origin);
    const char *l_url;
    const char *l_friendlyName;
    Eina_List *l;

    EINA_LIST_FOREACH(devices, l, device)
    {
        l_url = device_url_get(device);
        l_friendlyName = device_friendly_name_get(device);
        if (!strncmp(url, l_url, strlen(l_url)) && 
            !strcmp(friendlyName, l_friendlyName))
            return device;
    }
    
    return NULL;
}

static Eina_Bool 
service_config_update_services_type(Services *services, const char *type) 
{
    const char *types;
    char sub_text[MAX_SUBTEXT_LEN];
    Eina_Bool require_authority = EINA_FALSE;

    types = services_types_get(services);
    if (!types) {
        memset(sub_text, 0, MAX_SUBTEXT_LEN);
        strncpy(sub_text, type, MAX_SUBTEXT_LEN-1);
        services_types_set(services, sub_text);
        require_authority = EINA_TRUE;
    } else if (!strstr(types, type)) {
        if ((strlen(sub_text) + strlen(type)) < (MAX_SUBTEXT_LEN - 3)) {
            strcpy(sub_text, types);
            strcat(sub_text, ", ");
            strcat(sub_text, type);
            services_types_set(services, sub_text);
        }
        require_authority = EINA_TRUE;
    }

    return require_authority;
}

static Eina_Bool
service_config_services_set(Network_Origin *origin, Eina_Hash *services_hash, const char *key, Service_Item *item, const char *type, Ewk_NetworkService *service)
{
    static Device *l_device = NULL;
    Device *device = NULL;
    Services *services;
    Eina_Stringshare *key_share = eina_stringshare_add(key); 
    Eina_Bool require_authority = EINA_FALSE;

    switch(item->type) 
    {
    case NETWORK_TYPE_DEVICE_UPNP:
        /* Get Device configuration from persistent settings */
        device = service_config_device_get(origin, item->icon.url, item->text);
        if (!device) {
            char *url = strdup(item->icon.url);
            char *ptr;

            /* Truncate icon url "protocol://host" */
            ptr = strchr(url, ':');
            ptr = strchr(++ptr, ':');
            ptr[0] = '\0';
          
            device = device_new(url, item->text, NULL, EINA_TRUE);
            free(url);

        } else {
            /* Apply configuration settings */
            Eina_Bool allowed = device_allowed_get(device);
            Eina_List *l;
            Ewk_NetworkService *srv;

            item->widget_allowed = allowed;
            if (item->widget)
                service_item_widget_update(item->widget);

            EINA_LIST_FOREACH(item->services, l, srv)
              ewk_network_service_allowed_set(srv, allowed);
        }

        /* Registration of Device in Network_Origin list: 
         * network_origin_devices_add is called on a NSD valid action */
        if (!item->config.type.device)
            item->config.type.device = device;

        l_device = device;
        break;

    case NETWORK_TYPE_SERVICE_UPNP:
        /* Get Device configuration from persistent settings */
        device = l_device;
    case NETWORK_TYPE_SERVICE_ZEROCONF:
        /* Get Services configuration from persistent settings */
        if (device)
          services = device_services_get(device, key_share);
        else
          services = network_origin_services_get(origin, key_share);

        if (!services) {
            services = eina_hash_find(services_hash, key_share);
                                                                                       
            if (!services) {
                /* Create a new Services configuration settings */
                services = services_new(key_share, item->text, NULL, NULL, EINA_TRUE);
                require_authority = EINA_TRUE;
                                                                                       
                /* Registration of Services in Network_Origin hashtable: 
                 * network_origin_services_add is called on a NSD valid action */
                eina_hash_add(services_hash, services_id_get(services), services);
            }
                                                                                       
        } else {
            /* Apply configuration settings */
            Eina_Bool allowed = services_allowed_get(services);
            
            ewk_network_service_allowed_set(service, allowed);
            item->widget_allowed = allowed;
            if (item->widget)
                service_item_widget_update(item->widget);
        }
        
        if (!item->config.type.services)
            item->config.type.services = services;
        else if (item->config.type.services != services) 
            CRITICAL("error Services config missmatch");
        
        /* Update service per device if needed */
        service_config_update_services_type(services, type);
        break;

    default:
        WRN("wrong service type");
        break;  
    }
    
    eina_stringshare_del(key_share);

    return require_authority;
}

static char*
device_hash_key0_get(const Device_Config *dev)
{
    char *key;

    key = malloc(strlen(dev->friendlyName) + strlen(dev->url) + 2);
    sprintf(key, "%s@%s", dev->friendlyName, dev->url); 
    
    return key;
}

static char*
device_hash_key1_get(const Device_Config *dev)
{
    char *key;

    key = malloc(strlen(dev->url) + strlen(dev->port) + 2);
    sprintf(key, "%s:%s", dev->url, dev->port); 
    
    return key;
}

static Service_Item*
device_item_get(Eina_Hash **hash, 
                const Device_Config *dev,
                Ewk_NetworkService *service,
                Service_Item *parent,
                Eina_List **service_items,
                Service_Item_Callback list_append_func,
                void *user_data)
{
    Service_Item *item = NULL;
    char *key0;
    char *key1;

    key0 = device_hash_key0_get(dev);
    key1 = device_hash_key1_get(dev);

    item = eina_hash_find(hash[0], key0);
    if (!item) {
        item = eina_hash_find(hash[1], key1);
    }

    if (!item) {
        item = service_item_new(dev, parent->text, NULL, service, NETWORK_TYPE_DEVICE_UPNP, parent, service_items);

        eina_hash_add(hash[0], key0, item);           
        eina_hash_add(hash[1], key1, item);           

        service_item_children_add(parent, item);
   
        list_append_func(user_data, item);
    } else {
        free(key0);
        free(key1);

        item->services = eina_list_append(item->services, service);
    }
    
    return item;
}

Eina_Bool service_list_set(Eina_List *service_iter, Network *network, Eina_List **service_items, Service_Item_Callback list_append_func, void *user_data)
{
   unsigned long i; 
   unsigned long length;
   Ewk_NetworkServices *services;
   Eina_List *l, *ll;
   Eina_Bool require_authority = EINA_FALSE;
   Eina_Bool authority = EINA_FALSE;

   EINA_LIST_FOREACH(service_iter, l, services)
   {
      const char *origin;
      char *text;
      char *sub_text;
      Service_Item *origin_item = NULL;
      Service_Item *parent_item = NULL;
      Eina_Hash *device_items[2];
      Eina_Hash *service_items_hash = eina_hash_string_superfast_new(NULL);
      Eina_Hash *config_services = eina_hash_string_superfast_new(NULL);
      Network_Origin *network_origin;

      device_items[0] = eina_hash_string_superfast_new(NULL);
      device_items[1] = eina_hash_string_superfast_new(NULL);

      length = ewk_network_services_length_get(services);
      origin = ewk_network_services_origin_get(services);
      text = strdup(origin);

      origin_item = service_item_new(NULL, text, NULL, NULL, NETWORK_TYPE_SERVICE_UNKNOWN, NULL, service_items);

      list_append_func(user_data, origin_item);

      network_origin = service_config_network_origin_get(network, text);
      if (!network_origin)
          require_authority = EINA_TRUE;
            
      for (i=0; i<length; i++)
      {
         Device_Config dev;
         Ewk_NetworkService *srv = ewk_network_services_item_get(services, i);        
         Eina_Bool online = ewk_network_service_is_online(srv);
         Eve_Network_Type type;
         Service_Item *service_item = NULL;
         char *service_key;

         if (online == EINA_FALSE)
             continue;

         parent_item = origin_item;

         service_key = service_hash_key_get(srv, &type);
         service_item = eina_hash_find(service_items_hash, service_key);
 
         /* Group items per device */
         if (type == NETWORK_TYPE_SERVICE_UPNP) {
            memset(&dev, 0, sizeof(Device_Config));
            service_device_config_get(&dev, srv);
            parent_item = device_item_get(device_items, &dev, srv, origin_item, service_items, list_append_func, user_data);
            authority = service_config_services_set(network_origin, NULL, NULL, parent_item, NULL, NULL);
            if (authority == EINA_TRUE)
                require_authority = EINA_TRUE;
         }

         sub_text = service_get_sub_name(srv, type);

         if (service_item) {
            /* Concatenate UPnP or Zeroconf service "type" */
            if (sub_text && (strlen(service_item->sub_text) + strlen(sub_text)) < (MAX_SUBTEXT_LEN - 3)) { 
                strcat(service_item->sub_text, ", ");
                strcat(service_item->sub_text, sub_text);
            }
            
            if (type != NETWORK_TYPE_SERVICE_UPNP)
                service_item->services = eina_list_append(service_item->services, srv);

         } else {
            service_item = service_item_new(&dev, text, sub_text, srv, type, parent_item, service_items);
            eina_hash_add(service_items_hash, service_key, service_item);           
                        
            service_item_children_add(parent_item, service_item);

            list_append_func(user_data, service_item);
         }

         authority = service_config_services_set(network_origin, config_services, service_key, service_item, sub_text, srv);
         if (authority == EINA_TRUE)
            require_authority = EINA_TRUE;

         free(service_key);
         free(sub_text);

         if (type == NETWORK_TYPE_SERVICE_UPNP) {
            service_device_config_clear(&dev);
         }
      }

      eina_hash_free(device_items[0]);
      eina_hash_free(device_items[1]);
      eina_hash_free(service_items_hash);
      eina_hash_free(config_services);
   }

   return require_authority;
}

void service_item_allowed_set(Service_Item *item, Eina_Bool allowed)
{
   Eina_List *l;
   Ewk_NetworkService *service;

   EINA_LIST_FOREACH(item->services, l, service)
      ewk_network_service_allowed_set(service, allowed);

   item->widget_allowed = allowed;
}

Eina_Bool service_item_allowed_get(const Service_Item *item)
{
    if (item->type == NETWORK_TYPE_DEVICE_UPNP)
        return item->config.type.device ? device_allowed_get(item->config.type.device) : EINA_FALSE;
    else
        return item->config.type.services ? services_allowed_get(item->config.type.services) : EINA_FALSE;
}

Eina_Bool service_item_widget_allowed_get(const Service_Item *item)
{
    return item->widget_allowed;
}

void service_item_widget_set(Service_Item *item, void *obj)
{
    item->widget = obj;
}

void *service_item_parent_widget_get(const Service_Item *item)
{
    return (void*)(item->parent_widget);
}

Eina_List *service_item_children_get(const Service_Item *item)
{
  return item->children;
}

void service_item_icon_data_get(Service_Item *item, const void **data, size_t *size)
{
    *data = item->icon.data;
    *size = item->icon.size;
#if 1
    /* Create graphic object for Services config 
     * This able to keep persistence of icon in config file
     */
    if (item->widget && !(item->config.icon))
        item->config.icon = service_icon_from_data_get(item->widget, item->icon.data, item->icon.size);
#endif
}

void service_config_filename_set(const char *filename)
{
    _config_filename = filename;
}
const char *service_config_filename_get()
{
    return _config_filename;
}

static void 
service_config_update_services(Services *item_services, Services *services) {
    const char *item_types = services_types_get(item_services);
    const char *config_types = services_types_get(services);
    char *dup_item_types = strdup(item_types);
    char *ptr = dup_item_types;
    char *saveptr;
    char *type;
    char sub_text[MAX_SUBTEXT_LEN];
                                                                         
    while ((type = strtok_r(ptr, ", ", &saveptr)) != NULL) {
        ptr = NULL;
        if (strstr(config_types, type))
            continue;
        if (strlen(config_types) + strlen(type) < MAX_SUBTEXT_LEN - 3) {
            strcpy(sub_text, config_types);
            strcat(sub_text, ", ");
            strcat(sub_text, type);
            services_types_set(services, sub_text);
        }
    }
                                                                         
    free(dup_item_types);
                                                                         
    /* Check Services integrity */
    if (item_services != services)
        CRITICAL("error Services config missmatch");
}

void service_config_register_services(Service_Item *item, Eina_Bool allowed) {
    Network_Origin *origin;
    Services *item_services;
    Device *item_device;

    origin = service_config_network_origin_get(network, item->config.origin);

    /* Securty policy for legacy devices */
    if (allowed && item->services)
    {
        Ewk_NetworkService *srv = eina_list_nth(item->services, 0);
        Eina_Bool corsEnable = ewk_network_service_is_cors_enable(srv);

        if (corsEnable == EINA_FALSE) 
        {
            char* dest = strdup(ewk_network_service_url_get(srv));
            char* ptr;
        
            ptr = strchr(dest, ':'); /* offset "protocol://" */
            ptr+= 3; /* skip "://" */
            ptr = strchr(ptr, '/');
            ptr[0] = '\0';

            ewk_security_policy_whitelist_origin_add(item->config.origin, dest, EINA_TRUE);

            free(dest);
        }
    }

    switch(item->type) 
    {
    case NETWORK_TYPE_DEVICE_UPNP:
        item_device = item->config.type.device;

        /* Apply new allowance to the Services config */
        device_allowed_set(item_device, allowed);

        if (!origin) {
            origin = network_origin_new(item->config.origin, NULL);
            network_origins_add(network, origin);
            if (item->config.icon != NULL)
                device_icon_set(item_device, item->config.icon);
            network_origin_devices_add(origin, item_device);
        } else {
            Device *device = service_config_device_get(origin, item->icon.url, item->text);

            /* Add a new Device to an existing Network_Origin */
            if (item->config.icon != NULL)
                device_icon_set(item_device, item->config.icon);

            if (!device)
                network_origin_devices_add(origin, item_device);
        }
        break;

    case NETWORK_TYPE_SERVICE_UPNP:
        item_services = item->config.type.services;

        /* Apply new allowance to the Services config */
        services_allowed_set(item_services, item->parent->widget_allowed);

        if (!origin) {
            origin = network_origin_new(item->config.origin, NULL);
            network_origins_add(network, origin);
            if (item->config.icon != NULL)
                services_icon_set(item_services, item->config.icon);
            network_origin_services_add(origin, services_id_get(item_services), item_services);
        } else {
            Device *device = item->parent->config.type.device; 
            Services *services = device_services_get(device, services_id_get(item_services));
            
            if (!device)
                CRITICAL("Device not register");

            if (!services) {
                /* Add a new Services to an existing Network_Origin */
                if (item->config.icon != NULL)
                    services_icon_set(item_services, item->config.icon);
                device_services_add(device, services_id_get(item_services), item_services);
            } else {
                /* Update types of Services config */
                service_config_update_services(item_services, services);
            }
        }
        break;

    case NETWORK_TYPE_SERVICE_ZEROCONF:
        item_services = item->config.type.services;

        /* Apply new allowance to the Services config */
        services_allowed_set(item_services, allowed);

        if (!origin) {
            origin = network_origin_new(item->config.origin, NULL);
            network_origins_add(network, origin);
            if (item->config.icon != NULL)
                services_icon_set(item_services, item->config.icon);
            network_origin_services_add(origin, services_id_get(item_services), item_services);
        } else {
            Services *services = network_origin_services_get(origin, services_id_get(item_services));
            if (!services) {
                /* Add a new Services to an existing Network_Origin */
                if (item->config.icon != NULL)
                    services_icon_set(item_services, item->config.icon);
                network_origin_services_add(origin, services_id_get(item_services), item_services);
            } else {
                /* Update types of Services config */
                service_config_update_services(item_services, services);
            }
        }
        break;

    default:
        WRN("wrong service type");
        break;  
    }
}
