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

#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <Eina.h>
#include "private.h"

typedef enum {
   NETWORK_TYPE_DEVICE_UPNP,
   NETWORK_TYPE_SERVICE_UPNP,
   NETWORK_TYPE_SERVICE_ZEROCONF,
   NETWORK_TYPE_SERVICE_UNKNOWN
} Eve_Network_Type;

typedef struct _Service_Item Service_Item;

typedef void (*Service_Item_Callback)(void *data, Service_Item *item);

/* Service_Item */
void service_items_clear(Eina_List **service_items);
int service_item_compare(const void *item1, const void *item2);

Eve_Network_Type service_item_type_get(const Service_Item *item);
char *service_item_text_get(const Service_Item *item);
char *service_item_subtext_get(const Service_Item *item);
Eina_Bool service_item_have_service(const Service_Item *item);
Eina_Bool service_item_have_double_label(const Service_Item *item);
void service_item_allowed_set(Service_Item *item, Eina_Bool allowed);
Eina_Bool service_item_allowed_get(const Service_Item *item);
Eina_Bool service_item_widget_allowed_get(const Service_Item *item);
void service_item_widget_set(Service_Item *item, void *obj);
void *service_item_parent_widget_get(const Service_Item *item);
Eina_List *service_item_children_get(const Service_Item *item);
void service_item_icon_data_get(Service_Item *item, const void **data, size_t *size);

Eina_Bool service_list_set(Eina_List *service_iter, Network *network, Eina_List **service_items, Service_Item_Callback func, void *data);
void service_config_filename_set(const char *filename);
const char *service_config_filename_get();
void service_config_register_services(Service_Item *item, Eina_Bool allowed);

extern void service_item_widget_update(const void *widget); 
extern void *service_icon_from_data_get(const void *widget, const void *data, size_t size);

#endif /* __SERVICE_H__ */
