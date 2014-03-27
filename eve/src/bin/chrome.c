#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include <Eina.h>
#include <Elementary.h>

#include "private.h"

#define BOOKMARK_MENU_PREALLOC_SIZE 32
#define SERVICES_MENU_PREALLOC_SIZE 32

typedef struct _More_Menu_Item More_Menu_Item;
typedef struct _More_Menu_Set_Params More_Menu_Set_Params;
typedef struct _More_Menu_Filter_Context More_Menu_Filter_Context;
typedef struct _More_Menu_Config More_Menu_Config;
typedef struct _More_Menu_Config_List More_Menu_Config_List;
typedef struct _More_Menu_Config_List_Int More_Menu_Config_List_Int;
typedef struct _More_Menu_Config_Spinner More_Menu_Config_Spinner;

typedef More_Menu_Item *(*More_Menu_Callback)(Browser_Window *win, More_Menu_Item *current_item);

static More_Menu_Item *more_menu_favorites(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_history_today(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_history_yesterday(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_history_this_week(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_history_most_visited(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_history_least_visited(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_history_by_domain(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_privacy_clear_everything(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_privacy_clear_cache(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_privacy_clear_history(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_privacy_clear_database(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_privacy_clear_cookies(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_home_page_current_set(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_home_page_default_set(Browser_Window *, More_Menu_Item *);
static More_Menu_Item *more_menu_services_registered(Browser_Window *, More_Menu_Item *);

static void on_more_item_click(void *data, Evas_Object *obj, void *event_info __UNUSED__);
static void on_more_item_back_click(void *data, Evas_Object *edje, const char *emission __UNUSED__, const char *source __UNUSED__);

static int more_item_compare(const void *item1, const void *item2);
static void more_list_expanded_cb(void *data, Evas_Object *o, void *event_info);

static char *tab_grid_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part __UNUSED__);
static Evas_Object *tab_grid_content_get(void *data, Evas_Object *obj __UNUSED__, const char *part __UNUSED__);
static Eina_Bool tab_grid_state_get(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *part __UNUSED__);
static void tab_grid_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__);

static char *more_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part __UNUSED__);
static char *page_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part);
static char *list_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part);
static Evas_Object *more_content_get(void *data, Evas_Object *obj, const char *part);
static Eina_Bool more_state_get(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *part __UNUSED__);
static void more_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__);

static char *service_list_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part);
static Evas_Object *service_content_get(void *data, Evas_Object *obj, const char *part);
static Eina_Bool service_state_get(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *part __UNUSED__);
static void on_service_item_click(void *data, Evas_Object *obj, void *event_info __UNUSED__);
static void on_valid_click(void *data, Evas_Object *obj, void *event_info __UNUSED__);

static void proxy_config_home_page_set(Config *config, const char *home_page);

typedef enum {
   ITEM_TYPE_LAST,
   ITEM_TYPE_STATIC_FOLDER,
   ITEM_TYPE_DYNAMIC_FOLDER,
   ITEM_TYPE_PAGE,
   ITEM_TYPE_CALLBACK,
   ITEM_TYPE_CALLBACK_NO_HIDE,
   ITEM_TYPE_SEPARATOR,
   ITEM_TYPE_CONFIG
} More_Menu_Item_Type;

typedef enum {
   ITEM_FLAG_NONE        = 0,
   ITEM_FLAG_DYNAMIC     = 1<<0,
   ITEM_FLAG_SELECTED    = 1<<1,
   ITEM_FLAG_ARROW       = 1<<2,
   ITEM_FLAG_SELECTABLE  = 1<<3,
   ITEM_FLAG_INDEX       = 1<<4,
} More_Menu_Item_Flags;

typedef enum {
   CONFIG_TYPE_CHECKBOX = 10,
   CONFIG_TYPE_LIST,
   CONFIG_TYPE_STRING,
   CONFIG_TYPE_PASSWORD,
   CONFIG_TYPE_SPINNER,
   CONFIG_TYPE_LIST_INT,
   CONFIG_TYPE_NETWORK_ORIGIN,
   CONFIG_TYPE_DEVICE,
   CONFIG_TYPE_UPNP_SERVICES,
   CONFIG_TYPE_SERVICES,
} More_Menu_Config_Type;

typedef enum {
   EVE_CONFIG_ENABLE_JAVASCRIPT,
   EVE_CONFIG_ENABLE_PLUGINS,
   EVE_CONFIG_HOME_PAGE,
   EVE_CONFIG_PROXY,
   EVE_CONFIG_USER_AGENT,
   EVE_CONFIG_TOUCH_INTERFACE,
   EVE_CONFIG_MOUSE_CURSOR,
   EVE_CONFIG_ENABLE_PRIVATE_MODE,
   EVE_CONFIG_AUTO_LOAD_IMAGES,
   EVE_CONFIG_AUTO_SHRINK_IMAGES,
   EVE_CONFIG_POPUP_ALLOW,
   EVE_CONFIG_RESTORE_STATE,
   EVE_CONFIG_FRAME_FLATTENING,
   EVE_CONFIG_TEXT_ONLY_ZOOM,
   EVE_CONFIG_MINIMUM_FONT_SIZE,
   EVE_CONFIG_COOKIE_POLICY,
   EVE_CONFIG_NETWORK_ORIGIN,
   EVE_CONFIG_DEVICE,
   EVE_CONFIG_SERVICES,
   EVE_CONFIG_LAST
} Eve_Config;

struct _More_Menu_Item
{
   More_Menu_Item_Type type;
   const char *text;
   void *next;
   void *data;
   More_Menu_Item_Flags flags;
};

struct _More_Menu_Filter_Context
{
   More_Menu_Item *current_item;
   double time;
};

struct _More_Menu_Config {
   More_Menu_Config_Type type;
   Eve_Config conf;
   void *conf_get;
   void *conf_set;
   void *data;
};

struct _More_Menu_Config_List {
   const char *title;
   const char *value;
};

struct _More_Menu_Config_List_Int {
   const char *title;
   const int value;
   Eina_Bool is_default : 1;
};

struct _More_Menu_Config_Spinner {
   const int min;
   const int max;
   const char *format;
};

struct _More_Menu_Set_Params {
   Evas_Object *chrome;
   Evas_Object *list;
   More_Menu_Item *root;
   const char *old_text;
};

static More_Menu_Item more_menu_history[] =
{
   { ITEM_TYPE_DYNAMIC_FOLDER, "Today", more_menu_history_today, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_INDEX},
   { ITEM_TYPE_DYNAMIC_FOLDER, "Yesterday", more_menu_history_yesterday, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_INDEX},
   { ITEM_TYPE_DYNAMIC_FOLDER, "This week", more_menu_history_this_week, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_INDEX},
   { ITEM_TYPE_SEPARATOR, NULL, NULL, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_DYNAMIC_FOLDER, "Most visited", more_menu_history_most_visited, NULL, ITEM_FLAG_ARROW },
   { ITEM_TYPE_DYNAMIC_FOLDER, "Least visited", more_menu_history_least_visited, NULL, ITEM_FLAG_ARROW },
   { ITEM_TYPE_SEPARATOR, NULL, NULL, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_DYNAMIC_FOLDER, "By domain", more_menu_history_by_domain, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_INDEX },
   { ITEM_TYPE_LAST, NULL, NULL, NULL, ITEM_FLAG_NONE }
};

static More_Menu_Item more_menu_config[] =
{
   { ITEM_TYPE_CONFIG, "Enable JavaScript",
     (More_Menu_Config[]) {{
       .type = CONFIG_TYPE_CHECKBOX,
       .conf = EVE_CONFIG_ENABLE_JAVASCRIPT,
       .conf_get = config_enable_javascript_get,
       .conf_set = config_enable_javascript_set,
     }}, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_CONFIG, "Enable plugins",
     (More_Menu_Config[]) {{
       .type = CONFIG_TYPE_CHECKBOX,
       .conf = EVE_CONFIG_ENABLE_PLUGINS,
       .conf_get = config_enable_plugins_get,
       .conf_set = config_enable_plugins_set,
     }}, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_CONFIG, "Private mode",
     (More_Menu_Config[]) {{
       .type = CONFIG_TYPE_CHECKBOX,
       .conf = EVE_CONFIG_ENABLE_PRIVATE_MODE,
       .conf_get = config_enable_private_mode_get,
       .conf_set = config_enable_private_mode_set,
     }}, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_CONFIG, "Save and restore session",
     (More_Menu_Config[]) {{
       .type = CONFIG_TYPE_CHECKBOX,
       .conf = EVE_CONFIG_RESTORE_STATE,
       .conf_get = config_restore_state_get,
       .conf_set = config_restore_state_set,
     }}, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_SEPARATOR, NULL, NULL, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_STATIC_FOLDER, "Home page",
     (More_Menu_Item[]) {
         { ITEM_TYPE_CALLBACK_NO_HIDE, "Set to current page", more_menu_home_page_current_set, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CALLBACK_NO_HIDE, "Set to default", more_menu_home_page_default_set, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CONFIG, "Set to an URL",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_STRING,
             .conf = EVE_CONFIG_HOME_PAGE,
             .conf_get = config_home_page_get,
             .conf_set = proxy_config_home_page_set,
           }}, NULL, ITEM_FLAG_ARROW },
         { ITEM_TYPE_LAST, NULL, NULL, NULL, ITEM_FLAG_NONE },
     }, NULL, ITEM_FLAG_ARROW },
   { ITEM_TYPE_STATIC_FOLDER, "Network discovery",
     (More_Menu_Item[]) {
         { ITEM_TYPE_CONFIG, "Enable auto authorize policy",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_CHECKBOX,
             .conf = EVE_CONFIG_ENABLE_JAVASCRIPT,
             .conf_get = config_enable_auto_network_access_get,
             .conf_set = config_enable_auto_network_access_set,
           }}, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_DYNAMIC_FOLDER, "Services registered", more_menu_services_registered, NULL, ITEM_FLAG_ARROW },
         { ITEM_TYPE_LAST, NULL, NULL, NULL, ITEM_FLAG_NONE },
     }, NULL, ITEM_FLAG_ARROW },

   { ITEM_TYPE_STATIC_FOLDER, "Privacy",
     (More_Menu_Item[]) {
         { ITEM_TYPE_CALLBACK_NO_HIDE, "Clear everything", more_menu_privacy_clear_everything, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_SEPARATOR, NULL, NULL, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CALLBACK_NO_HIDE, "Clear cache", more_menu_privacy_clear_cache, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CALLBACK_NO_HIDE, "Clear history", more_menu_privacy_clear_history, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CALLBACK_NO_HIDE, "Clear database", more_menu_privacy_clear_database, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CALLBACK_NO_HIDE, "Clear cookies", more_menu_privacy_clear_cookies, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_LAST, NULL, NULL, NULL, ITEM_FLAG_NONE },
     }, NULL, ITEM_FLAG_ARROW },
   { ITEM_TYPE_STATIC_FOLDER, "Tweaks",
     (More_Menu_Item[]) {
         { ITEM_TYPE_CONFIG, "Enable mouse cursor",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_CHECKBOX,
             .conf = EVE_CONFIG_MOUSE_CURSOR,
             .conf_get = config_enable_mouse_cursor_get,
             .conf_set = config_enable_mouse_cursor_set
           }}, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CONFIG, "Enable touch interface",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_CHECKBOX,
             .conf = EVE_CONFIG_TOUCH_INTERFACE,
             .conf_get = config_enable_touch_interface_get,
             .conf_set = config_enable_touch_interface_set
           }}, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CONFIG, "Automatically load images",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_CHECKBOX,
             .conf = EVE_CONFIG_AUTO_LOAD_IMAGES,
             .conf_get = config_enable_auto_load_images_get,
             .conf_set = config_enable_auto_load_images_set
           }}, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CONFIG, "Automatically shrink images",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_CHECKBOX,
             .conf = EVE_CONFIG_AUTO_SHRINK_IMAGES,
             .conf_get = config_enable_auto_shrink_images_get,
             .conf_set = config_enable_auto_shrink_images_set
           }}, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CONFIG, "Allow popup windows",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_CHECKBOX,
             .conf = EVE_CONFIG_POPUP_ALLOW,
             .conf_get = config_allow_popup_get,
             .conf_set = config_allow_popup_set
           }}, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CONFIG, "Frame flattening",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_CHECKBOX,
             .conf = EVE_CONFIG_FRAME_FLATTENING,
             .conf_get = config_frame_flattening_get,
             .conf_set = config_frame_flattening_set
           }}, NULL, ITEM_FLAG_NONE },
         { ITEM_TYPE_CONFIG, "Text zoom",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_LIST_INT,
             .conf = EVE_CONFIG_TEXT_ONLY_ZOOM,
             .conf_get = config_text_zoom_get,
             .conf_set = config_text_zoom_set,
             .data = (More_Menu_Config_List_Int[]) {
               { "10%", 10, EINA_FALSE },
               { "25%", 25, EINA_FALSE },
               { "50%", 50, EINA_FALSE },
               { "75%", 75, EINA_FALSE },
               { "100% (default)", 100, EINA_TRUE },
               { "125%", 125, EINA_FALSE },
               { "150%", 150, EINA_FALSE },
               { "175%", 175, EINA_FALSE },
               { "200%", 200, EINA_FALSE }
             }
           }}, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_SELECTABLE },
         { ITEM_TYPE_CONFIG, "Minimum font size",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_LIST_INT,
             .conf = EVE_CONFIG_MINIMUM_FONT_SIZE,
             .conf_get = config_minimum_font_size_get,
             .conf_set = config_minimum_font_size_set,
             .data = (More_Menu_Config_List_Int[]) {
               { "6pt", 6, EINA_FALSE },
               { "8pt", 8, EINA_FALSE },
               { "10pt", 10, EINA_FALSE },
               { "12pt (default)", 12, EINA_TRUE },
               { "14pt", 14, EINA_FALSE },
               { "16pt", 16, EINA_FALSE },
               { "18pt", 18, EINA_FALSE },
               { NULL, 0, EINA_FALSE }
             }
           }}, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_SELECTABLE },
         { ITEM_TYPE_CONFIG, "User agent",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_LIST,
             .conf = EVE_CONFIG_USER_AGENT,
             .conf_get = config_user_agent_get,
             .conf_set = config_user_agent_set,
             .data = (More_Menu_Config_List[]) {
               { "Eve", "Mozilla/5.0 (iPhone; U; CPU like Mac OS X; en) AppleWebKit/420+ (KHTML, like Gecko) Version/3.0 Mobile/1A543a Safari/419.3 " PACKAGE_NAME "/" PACKAGE_VERSION },
               { "iPhone", "Mozilla/5.0 (iPhone; U; CPU like Mac OS X; en) AppleWebKit/420+ (KHTML, like Gecko) Version/3.0 Mobile/1A543a Safari/419.3" },
               { "Android", "Mozilla/5.0 (Linux; U; Android 2.1; en-US;) AppleWebKit/530.17 (KHTML, like Gecko) Version/4.0 Mobile Safari/530.17" },
               { "Safari", "Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_6_3; en-US) AppleWebKit/533.17.8 (KHTML, like Gecko) Version/5.0.1 Safari/533.17.8" },
               { "Chrome", "Mozilla/5.0 (X11; U; Linux x86_64; en-US) AppleWebKit/534.7 (KHTML, like Gecko) Chrome/7.0.514.0 Safari/534.7" },
               { "Firefox", "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.0.2) Gecko/20121223 Firefox/3.8" },
               { "Internet Explorer", "Mozilla/5.0 (Windows; U; MSIE 9.0; Windows NT 9.0; en-US)" },
               { NULL, NULL }
             }
           }}, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_SELECTABLE },
         { ITEM_TYPE_CONFIG, "Cookie policy",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_LIST_INT,
             .conf = EVE_CONFIG_COOKIE_POLICY,
             .conf_get = config_cookie_policy_get,
             .conf_set = config_cookie_policy_set,
             .data = (More_Menu_Config_List_Int[]) {
               { "Accept all cookies", EWK_COOKIE_JAR_ACCEPT_ALWAYS, EINA_FALSE },
               { "Do not accept third-party cookies", EWK_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY, EINA_TRUE },
               { "Never accept", EWK_COOKIE_JAR_ACCEPT_NEVER, EINA_FALSE },
               { NULL, 0, EINA_FALSE },
             }
           }}, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_SELECTABLE },
         { ITEM_TYPE_CONFIG, "Backing Store",
           (More_Menu_Config[]) {{
             .type = CONFIG_TYPE_LIST_INT,
             .conf = EVE_CONFIG_MINIMUM_FONT_SIZE,
             .conf_get = config_backing_store_get,
             .conf_set = config_backing_store_set,
             .data = (More_Menu_Config_List_Int[]) {
               { "Single", BACKING_STORE_SINGLE, EINA_TRUE },
               { "Tiled", BACKING_STORE_TILED, EINA_FALSE },
               { NULL, 0, EINA_FALSE }
             }
           }}, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_SELECTABLE },
         { ITEM_TYPE_LAST, NULL, NULL, NULL, ITEM_FLAG_NONE },
     }, NULL, ITEM_FLAG_ARROW },
   { ITEM_TYPE_SEPARATOR, NULL, NULL, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_LAST, NULL, NULL, NULL, ITEM_FLAG_NONE }
};

static More_Menu_Item more_menu_root[] =
{
   { ITEM_TYPE_STATIC_FOLDER, "History", more_menu_history, NULL, ITEM_FLAG_ARROW },
   { ITEM_TYPE_DYNAMIC_FOLDER, "Favorites", more_menu_favorites, NULL, ITEM_FLAG_ARROW | ITEM_FLAG_INDEX },
   { ITEM_TYPE_STATIC_FOLDER, "Preferences", more_menu_config, NULL, ITEM_FLAG_ARROW },
   { ITEM_TYPE_SEPARATOR, NULL, NULL, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_PAGE, "ProFUSION", "http://profusion.mobi", NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_PAGE, "WebKit", "http://webkit.org", NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_PAGE, "Enlightenment", "http://enlightenment.org", NULL, ITEM_FLAG_NONE },
#ifdef STORM_TROOPER
   { ITEM_TYPE_SEPARATOR, NULL, NULL, NULL, ITEM_FLAG_NONE },
   { ITEM_TYPE_PAGE, "", "http://i.imgur.com/cJO3j.gif", NULL, ITEM_FLAG_NONE },
#endif
   { ITEM_TYPE_LAST, NULL, NULL, NULL, ITEM_FLAG_NONE }
};

static More_Menu_Config more_config_network_origin[] = 
{{
    .type = CONFIG_TYPE_NETWORK_ORIGIN,
    .conf = EVE_CONFIG_NETWORK_ORIGIN,
    .conf_get = NULL,
    .conf_set = NULL
}};

static More_Menu_Config more_config_device[] = 
{{
    .type = CONFIG_TYPE_DEVICE,
    .conf = EVE_CONFIG_DEVICE,
    .conf_get = device_allowed_get,
    .conf_set = NULL
}};

static More_Menu_Config more_config_upnp_services[] = 
{{
    .type = CONFIG_TYPE_UPNP_SERVICES,
    .conf = EVE_CONFIG_SERVICES,
    .conf_get = services_allowed_get,
    .conf_set = NULL
}};

static More_Menu_Config more_config_services[] = 
{{
    .type = CONFIG_TYPE_SERVICES,
    .conf = EVE_CONFIG_SERVICES,
    .conf_get = services_allowed_get,
    .conf_set = NULL
}};

static const Elm_Gengrid_Item_Class gic_default = {
   .func = {
       .text_get = tab_grid_text_get,
       .content_get = tab_grid_content_get,
       .state_get = tab_grid_state_get,
       .del = tab_grid_del
    }
};

static const Elm_Genlist_Item_Class glic_default = {
   .func = {
       .text_get = more_text_get,
       .content_get = more_content_get,
       .state_get = more_state_get,
       .del = more_del
    },
   .item_style = "ewebkit"
};

static const Elm_Genlist_Item_Class glic_config = {
   .func = {
       .text_get = more_text_get,
       .content_get = more_content_get,
       .state_get = more_state_get,
       .del = more_del
    },
   .item_style = "config"
};

static const Elm_Genlist_Item_Class glic_config_selectable = {
   .func = {
       .text_get = more_text_get,
       .content_get = more_content_get,
       .state_get = more_state_get,
       .del = more_del
    },
   .item_style = "config/selectable"
};

static const Elm_Genlist_Item_Class glic_separator = {
   .func = {
       .text_get = NULL,
       .content_get = NULL,
       .state_get = NULL,
       .del = more_del
    },
   .item_style = "separator"
};

static const Elm_Genlist_Item_Class glic_config_list = {
   .func = {
       .text_get = list_text_get,
       .content_get = more_content_get,
       .state_get = more_state_get,
       .del = more_del
    },
   .item_style = "config/double_label/ewebkit"
};

static const Elm_Genlist_Item_Class glic_config_origin = {
   .func = {
       .text_get = list_text_get,
       .content_get = more_content_get,
       .state_get = more_state_get,
       .del = more_del
    },
   .item_style = "config"
};

static const Elm_Genlist_Item_Class glic_config_service = {
   .func = {
       .text_get = list_text_get,
       .content_get = more_content_get,
       .state_get = more_state_get,
       .del = more_del
    },
   .item_style = "double_label/ewebkit"
};

static const Elm_Genlist_Item_Class glic_page = {
   .func = {
       .text_get = page_text_get,
       .content_get = more_content_get,
       .state_get = more_state_get,
       .del = more_del
    },
   .item_style = "double_label/ewebkit"
};

static const Elm_Genlist_Item_Class glic_service_origin = {
   .func = {
       .text_get = service_list_text_get,
       .content_get = service_content_get,
       .state_get = service_state_get,
    },
   .item_style = "ewebkit"
};

static const Elm_Genlist_Item_Class glic_device_list = {
   .func = {
       .text_get = service_list_text_get,
       .content_get = service_content_get,
       .state_get = service_state_get,
    },
   .item_style = "ewebkit"
};

static const Elm_Genlist_Item_Class glic_service_list = {
   .func = {
       .text_get = service_list_text_get,
       .content_get = service_content_get,
       .state_get = service_state_get,
    },
   .item_style = "double_label/ewebkit"
};


static void
proxy_config_home_page_set(Config *config, const char *home_page)
{
   char *tmp_uri = uri_sanitize(home_page);
   config_home_page_set(config, tmp_uri);
   free(tmp_uri);
}

static Eina_List *
_eina_hash_sorted_keys_get(Eina_Hash *hash, Eina_Compare_Cb compare_func)
{
   Eina_List *keyvals = NULL, *keys = NULL, *keyvals_iter;
   Eina_Iterator *iter = eina_hash_iterator_tuple_new(hash);
   Eina_Hash_Tuple *keyval;

   EINA_ITERATOR_FOREACH(iter, keyval)
     keyvals = eina_list_prepend(keyvals, keyval);
   keyvals = eina_list_sort(keyvals, 0, compare_func);
   EINA_LIST_FOREACH(keyvals, keyvals_iter, keyval)
     keys = eina_list_append(keys, keyval->key);

   eina_list_free(keyvals);
   eina_iterator_free(iter);

   return keys;
}

static More_Menu_Item *
_more_menu_history(Eina_Iterator *items, More_Menu_Item *current_item, Eina_Bool (*filter)(More_Menu_Filter_Context *ctx, Hist_Item *item))
{
   More_Menu_Item *bm_item;
   More_Menu_Item *ret = NULL, *new_ret;
   More_Menu_Filter_Context ctx;
   int n_items = 0;
   const char *url;

   ctx.time = ecore_time_unix_get();
   ctx.current_item = current_item;

   EINA_ITERATOR_FOREACH(items, url)
   {
      Hist_Item *item = hist_items_get(hist, url);

      if (!filter(&ctx, item))
         continue;

      bm_item = calloc(1, sizeof(More_Menu_Item));
      bm_item->type = ITEM_TYPE_PAGE;
      bm_item->text = eina_stringshare_add(hist_item_title_get(item));
      bm_item->next = (char *)hist_item_url_get(item);
      bm_item->flags = ITEM_FLAG_DYNAMIC;

      if (!ret)
         ret = calloc(1, sizeof(*ret) * BOOKMARK_MENU_PREALLOC_SIZE);
      else if (n_items % BOOKMARK_MENU_PREALLOC_SIZE == 0)
        {
           new_ret = realloc(ret, (BOOKMARK_MENU_PREALLOC_SIZE * n_items * sizeof(*ret)));
           if (new_ret)
              ret = new_ret;
           else
              goto realloc_error;
        }

      memcpy(&ret[n_items], bm_item, sizeof(*ret));
      free(bm_item);

      n_items++;
   }

realloc_error:

   if (!n_items)
      return NULL;

   bm_item = calloc(1, sizeof(More_Menu_Item));
   bm_item->type = ITEM_TYPE_LAST;
   new_ret = realloc(ret, (1 + n_items) * sizeof(*ret));
   if (!new_ret)
     {
        free(bm_item);
        free(ret);
        return NULL;
     }

   ret = new_ret;
   memcpy(&ret[n_items], bm_item, sizeof(*ret));
   free(bm_item);

   return ret;
}

static Eina_Bool
_domain_filter(More_Menu_Filter_Context *ctx, Hist_Item *item)
{
   char *domain = strstr(hist_item_url_get(item), "://");
   const char *filtered_domain = ctx->current_item->text;
   if (domain)
     {
        domain += 3;
        return !strncmp(domain, filtered_domain, strlen(filtered_domain));
     }

   return EINA_FALSE;
}

static More_Menu_Item *
_more_menu_history_by_domain(Browser_Window *win __UNUSED__, More_Menu_Item *current_item)
{
   More_Menu_Item *ret;
   Eina_Iterator *items = eina_hash_iterator_key_new(hist_items_hash_get(hist));

   ret = _more_menu_history(items, current_item, _domain_filter);
   eina_iterator_free(items);

   return ret;
}

static int
compare_domain_cb(const void *d1, const void *d2)
{
   const char *c1 = ((Eina_Hash_Tuple *)d1)->key;
   const char *c2 = ((Eina_Hash_Tuple *)d2)->key;

   return strcasecmp(c1, c2);
}

static More_Menu_Item *
more_menu_history_by_domain(Browser_Window *win __UNUSED__, More_Menu_Item *current_item __UNUSED__)
{
   More_Menu_Item *bm_item;
   More_Menu_Item *ret = NULL, *new_ret;
   Eina_List *keys;
   Eina_Iterator *items = eina_hash_iterator_key_new(hist_items_hash_get(hist));
   Eina_Hash *domains = eina_hash_string_superfast_new(NULL);
   const char *url;
   int n_items = 0;

   EINA_ITERATOR_FOREACH(items, url)
   {
      char *urlcopy = strdup(url);
      char *domain, *end;

      if ((domain = strcasestr(urlcopy, "http://")))
         domain += 7;
      else if ((domain = strcasestr(urlcopy, "https://")))
         domain += 8;
      else
         goto unknown_schema;

      if ((end = strchr(domain, '/')))
        {
           *end = '\0';
           eina_hash_set(domains, strdup(domain), (void *)1);
        }

unknown_schema:
      free(urlcopy);
   }
   eina_iterator_free(items);

   keys = _eina_hash_sorted_keys_get(domains, compare_domain_cb);
   items = eina_list_iterator_new(keys);
   EINA_ITERATOR_FOREACH(items, url)
   {
      bm_item = calloc(1, sizeof(More_Menu_Item));
      bm_item->type = ITEM_TYPE_DYNAMIC_FOLDER;
      bm_item->text = eina_stringshare_add(url);
      bm_item->next = _more_menu_history_by_domain;
      bm_item->flags = ITEM_FLAG_ARROW | ITEM_FLAG_DYNAMIC | ITEM_FLAG_INDEX;

      if (!n_items)
         ret = calloc(1, sizeof(*ret) * BOOKMARK_MENU_PREALLOC_SIZE);
      else if (n_items % BOOKMARK_MENU_PREALLOC_SIZE == 0)
        {
           new_ret = realloc(ret, (BOOKMARK_MENU_PREALLOC_SIZE * n_items * sizeof(*ret)));
           if (new_ret)
              ret = new_ret;
           else
              goto realloc_error;
        }

      memcpy(&ret[n_items], bm_item, sizeof(*ret));
      free(bm_item);

      n_items++;
   }
realloc_error:
   eina_list_free(keys);
   eina_iterator_free(items);
   eina_hash_free(domains);

   if (!n_items)
      return NULL;

   bm_item = calloc(1, sizeof(More_Menu_Item));
   bm_item->type = ITEM_TYPE_LAST;
   new_ret = realloc(ret, (1 + n_items) * sizeof(*ret));
   if (!new_ret)
     {
        free(bm_item);
        free(ret);
        return NULL;
     }

   ret = new_ret;
   memcpy(&ret[n_items], bm_item, sizeof(*ret));
   free(bm_item);

   return ret;
}

static Eina_Bool
_this_week_filter(More_Menu_Filter_Context *ctx, Hist_Item *item)
{
   double item_time = hist_item_last_visit_get(item);
   double now = ctx->time;
   return (now - item_time) <= 7 * 24 * 3600;
}

static Eina_Bool
_today_filter(More_Menu_Filter_Context *ctx, Hist_Item *item)
{
   double item_time = hist_item_last_visit_get(item);
   double now = ctx->time;
   return (now - item_time) <= 24 * 3600;
}

static Eina_Bool
_yesterday_filter(More_Menu_Filter_Context *ctx, Hist_Item *item)
{
   double item_time = hist_item_last_visit_get(item);
   double now = ctx->time;
   return (now - item_time) > 24 * 3600 && (now - item_time) <= 48 * 3600;
}

static const char *
_first_alpha_char(const char *str)
{
   if (!str) return NULL;
   while ((*str) && (!isalpha(*str))) str++;
   return str;
}

static int
compare_hist_cb(const void *d1, const void *d2)
{
   Hist_Item *f1 = ((Eina_Hash_Tuple *)d1)->data;
   Hist_Item *f2 = ((Eina_Hash_Tuple *)d2)->data;

   const char *c1, *c2;

   if (!f1) return(1);
   if (!f2) return(-1);

   c1 = _first_alpha_char(hist_item_title_get(f1));
   c2 = _first_alpha_char(hist_item_title_get(f2));

   return strcasecmp(c1, c2);
}

static int
compare_fav_cb(const void *d1, const void *d2)
{
   Fav_Item *f1 = ((Eina_Hash_Tuple *)d1)->data;
   Fav_Item *f2 = ((Eina_Hash_Tuple *)d2)->data;

   const char *c1, *c2;

   if (!f1) return(1);
   if (!f2) return(-1);

   c1 = _first_alpha_char(fav_item_title_get(f1));
   c2 = _first_alpha_char(fav_item_title_get(f2));

   return strcasecmp(c1, c2);
}

static void
index_selected(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   elm_genlist_item_bring_in(event_info, ELM_GENLIST_ITEM_SCROLLTO_TOP);
}

static More_Menu_Item *
more_menu_history_today(Browser_Window *win __UNUSED__, More_Menu_Item *current_item)
{
   Eina_List *keys = _eina_hash_sorted_keys_get(hist_items_hash_get(hist), compare_hist_cb);
   Eina_Iterator *iter = eina_list_iterator_new(keys);
   More_Menu_Item *items = _more_menu_history(iter, current_item, _today_filter);

   eina_list_free(keys);
   eina_iterator_free(iter);
   return items;
}

static More_Menu_Item *
more_menu_history_yesterday(Browser_Window *win __UNUSED__, More_Menu_Item *current_item)
{
   Eina_List *keys = _eina_hash_sorted_keys_get(hist_items_hash_get(hist), compare_hist_cb);
   Eina_Iterator *iter = eina_list_iterator_new(keys);
   More_Menu_Item *items = _more_menu_history(iter, current_item, _yesterday_filter);

   eina_list_free(keys);
   eina_iterator_free(iter);
   return items;
}

static More_Menu_Item *
more_menu_history_this_week(Browser_Window *win __UNUSED__, More_Menu_Item *current_item)
{
   Eina_List *keys = _eina_hash_sorted_keys_get(hist_items_hash_get(hist), compare_hist_cb);
   Eina_Iterator *iter = eina_list_iterator_new(keys);
   More_Menu_Item *items = _more_menu_history(iter, current_item, _this_week_filter);

   eina_list_free(keys);
   eina_iterator_free(iter);
   return items;
}

static int
_cb_compare_hist_visit_count_decr(const void *data1, const void *data2)
{
   Hist_Item *f1 = ((Eina_Hash_Tuple *)data1)->data;
   Hist_Item *f2 = ((Eina_Hash_Tuple *)data2)->data;

   return hist_item_visit_count_get(f2) - hist_item_visit_count_get(f1);
}

static int
_cb_compare_hist_visit_count_incr(const void *data1, const void *data2)
{
   Hist_Item *f1 = ((Eina_Hash_Tuple *)data1)->data;
   Hist_Item *f2 = ((Eina_Hash_Tuple *)data2)->data;

   return hist_item_visit_count_get(f1) - hist_item_visit_count_get(f2);
}

static More_Menu_Item *
more_menu_history_least_visited(Browser_Window *win __UNUSED__, More_Menu_Item *current_item)
{
   Eina_List *keys = _eina_hash_sorted_keys_get(hist_items_hash_get(hist), _cb_compare_hist_visit_count_incr);
   Eina_Iterator *iter = eina_list_iterator_new(keys);
   More_Menu_Item *items = _more_menu_history(iter, current_item, _this_week_filter);
   eina_list_free(keys);
   eina_iterator_free(iter);
   return items;
}

static More_Menu_Item *
more_menu_history_most_visited(Browser_Window *win __UNUSED__, More_Menu_Item *current_item)
{
   Eina_List *keys = _eina_hash_sorted_keys_get(hist_items_hash_get(hist), _cb_compare_hist_visit_count_decr);
   Eina_Iterator *iter = eina_list_iterator_new(keys);
   More_Menu_Item *items = _more_menu_history(iter, current_item, _this_week_filter);
   eina_list_free(keys);
   eina_iterator_free(iter);
   return items;
}

static More_Menu_Item *
more_menu_favorites(Browser_Window *win __UNUSED__, More_Menu_Item *current_item __UNUSED__)
{
   More_Menu_Item *bm_item;
   More_Menu_Item *ret = NULL, *new_ret;
   Eina_List *keys = _eina_hash_sorted_keys_get(fav_items_hash_get(fav), compare_fav_cb);
   Eina_Iterator *iter = eina_list_iterator_new(keys);
   int n_items = 0;
   const char *url;

   EINA_ITERATOR_FOREACH(iter, url)
   {
      Fav_Item *item = fav_items_get(fav, url);

      bm_item = calloc(1, sizeof(More_Menu_Item));
      bm_item->type = ITEM_TYPE_PAGE;
      bm_item->text = eina_stringshare_add(fav_item_title_get(item));
      bm_item->next = (char *)fav_item_url_get(item);
      bm_item->flags = ITEM_FLAG_DYNAMIC;

      if (!n_items)
         ret = calloc(1, sizeof(*ret) * BOOKMARK_MENU_PREALLOC_SIZE);
      else if (n_items % BOOKMARK_MENU_PREALLOC_SIZE == 0)
        {
           new_ret = realloc(ret, (BOOKMARK_MENU_PREALLOC_SIZE * n_items * sizeof(*ret)));
           if (new_ret)
              ret = new_ret;
           else
              goto realloc_error;
        }

      memcpy(&ret[n_items], bm_item, sizeof(*ret));
      free(bm_item);

      n_items++;
   }
realloc_error:
   eina_list_free(keys);
   eina_iterator_free(iter);

   if (!n_items)
     {
        free(ret);
        return NULL;
     }

   bm_item = calloc(1, sizeof(More_Menu_Item));
   bm_item->type = ITEM_TYPE_LAST;
   new_ret = realloc(ret, (1 + n_items) * sizeof(*ret));
   if (!new_ret)
     {
        free(bm_item);
        free(ret);
        return NULL;
     }

   ret = new_ret;
   memcpy(&ret[n_items], bm_item, sizeof(*ret));
   free(bm_item);

   return ret;
}

static void
on_view_mask_hidden(void *data, Evas_Object *o __UNUSED__,
                    const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Browser_Window *win = data;

   /* we steal focus away from elm's focus model and start to do things
    * manually here, so elm now has no clue what's up, so tell elm that its
    * toplevel widget for the chrome is to be unfocused so elm gives up
    * the focus and its idea of the world */
   elm_object_focus_set(elm_object_top_widget_get(win->current_chrome), EINA_FALSE);
   evas_object_focus_set(win->current_view, EINA_TRUE);
}

static void
on_fav_on(void *data, Evas_Object *o __UNUSED__,
          const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *view = data;
   const char *url = ewk_view_uri_get(view);

   if (url)
     {
        const Ewk_Text_With_Direction *title = ewk_view_title_get(view);
        fav_items_add(fav, url, fav_item_new(url, title->string, 1));
     }
}

static void
on_fav_off(void *data, Evas_Object *o __UNUSED__,
           const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *view = data;
   const char *url = ewk_view_uri_get(view);

   if (url)
      fav_items_del(fav, url);
}

static void
_is_favorite_check(Evas_Object *chrome, const char *url)
{
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Fav_Item *item;

   if (url && (item = fav_items_get(fav, url)))
     {
        fav_item_visit_count_set(item, fav_item_visit_count_get(item) + 1);
        edje_object_signal_emit(ed, "favorite,hilight", "");
     }
   else
      edje_object_signal_emit(ed, "favorite,default", "");
}

static void
_history_update(const char *url, const char *title)
{
   Hist_Item *item;

   if (!url || config_enable_private_mode_get(config))
      return;

   if ((item = hist_items_get(hist, url)))
     {
        hist_item_visit_count_set(item, hist_item_visit_count_get(item) + 1);
        hist_item_last_visit_set(item, ecore_time_unix_get());
        hist_item_title_set(item, title);
     }
   else
      hist_items_add(hist, url, hist_item_new(title, url, 1, ecore_time_unix_get()));
}

static const char *
_chrome_title_get(Evas_Object *chrome, char *buf, size_t bufsize)
{
   Evas_Object *view = evas_object_data_get(chrome, "view");
   const char *url = ewk_view_uri_get(view);
   const Ewk_Text_With_Direction *title_dir = ewk_view_title_get(view);
   const char *title = title_dir->string;
   int p = ewk_view_load_progress_get(view) * 100;

   if (!title)
     title = url;

   if (!title)
     title = "";

   if ((p > 0) && (p < 100))
     {
        snprintf(buf, bufsize, "(%d%%) %s", p, title);
        title = buf;
     }

   return title;
}

static void
_chrome_title_apply(Evas_Object *chrome)
{
   Browser_Window *win = evas_object_data_get(chrome, "win");
   Evas_Object *ed = elm_layout_edje_get(chrome);
   const char *title;
   char buf[4096];

   title = _chrome_title_get(chrome, buf, sizeof(buf));

   edje_object_part_text_set(ed, "page-title", title);

   if (win->current_chrome == chrome)
     window_title_set(win, title);
}

static void
_chrome_state_apply(Evas_Object *chrome, Evas_Object *view)
{
   Evas *canvas = evas_object_evas_get(chrome);
   const char *url = ewk_view_uri_get(view);
   const Ewk_Text_With_Direction *title_dir = ewk_view_title_get(view);
   const char *title = title_dir->string;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Evas_Object *text_url;
   Evas_Object *favicon;
   Evas_Object *old_icon;

   _chrome_title_apply(chrome);

   text_url = edje_object_part_swallow_get(ed, "url-entry");
   elm_object_text_set(text_url, url ? url : "");
   _is_favorite_check(chrome, url);
   _history_update(url, title ? title : url);

   old_icon = elm_object_part_content_unset(text_url, "icon");

   if (old_icon) evas_object_del(old_icon);
   if (url)
     {
        favicon = ewk_settings_icon_database_icon_object_get(url, canvas);
        if (favicon)
           elm_object_part_content_set(text_url, "icon", favicon);
     }

   edje_object_signal_emit(ed, ewk_view_back_possible(view) ? "button,back,enable" : "button,back,disable", "");
   edje_object_signal_emit(ed, ewk_view_forward_possible(view) ? "button,forward,enable" : "button,forward,disable", "");
}

static void
on_view_load_progress(void *data, Evas_Object *view __UNUSED__, void *event_info)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);

   double *progress = event_info;
   Edje_Message_Float msg = { *progress };
   edje_object_message_send(ed, EDJE_MESSAGE_FLOAT, 1, &msg);

   _chrome_title_apply(chrome);
}

static void
on_view_title_changed(void *data, Evas_Object *view,
                      void *event_info __UNUSED__)
{
   Evas_Object *chrome = data;

   _chrome_state_apply(chrome, view);
}

static void
on_view_uri_changed(void *data, Evas_Object *view, void *event_info __UNUSED__)
{
   Evas_Object *chrome = data;

   _chrome_state_apply(chrome, view);
}

static void
on_view_zoom_interactive(void *data, Evas_Object *view __UNUSED__,
                         void *event_info)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);

   const View_Zoom_Interactive *ev = event_info;

   char buf[256];

   snprintf(buf, sizeof(buf), "zoom=%0.1f", ev->zoom);
   edje_object_part_text_set(ed, "text.zoom", buf);
}

static void
on_view_zoom_interactive_start(void *data, Evas_Object *view __UNUSED__,
                               void *event_info __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);

   edje_object_signal_emit(ed, "zoom,show", "");
}

static void
on_view_zoom_interactive_end(void *data, Evas_Object *view __UNUSED__,
                             void *event_info __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);

   edje_object_signal_emit(ed, "zoom,hide", "");
}

static void
on_view_statusbar_text_set(void *data, Evas_Object *view __UNUSED__,
                           void *event_info)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   const char *text = event_info;

   edje_object_part_text_set(ed, "text.statusbar", text);
   edje_object_signal_emit(ed, "statusbar,show", "");
}

static void
on_view_link_hover_in(void *data, Evas_Object *view __UNUSED__,
                      void *event_info)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   const char **link = event_info;

   edje_object_part_text_set(ed, "text.statusbar", link[0]);
   edje_object_signal_emit(ed, "statusbar,show", "");
}

static void
on_view_link_hover_out(void *data, Evas_Object *view __UNUSED__,
                       void *event_info __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);

   edje_object_signal_emit(ed, "statusbar,hide", "");
}

static Eina_Bool
_view_popup_delete(void *notify)
{
   evas_object_del(notify);
   return EINA_FALSE;
}

static void
on_view_popup_delete(void *data, Evas_Object *view __UNUSED__, void *event_info __UNUSED__)
{
   ecore_idler_add(_view_popup_delete, data);
}

static More_Menu_Item *
more_menu_home_page_current_set(Browser_Window *win, More_Menu_Item *mmi __UNUSED__)
{
   char *tmp_uri = uri_sanitize(ewk_view_uri_get(win->current_view));
   config_home_page_set(config, tmp_uri);
   free(tmp_uri);
   return NULL;
}

static More_Menu_Item *
more_menu_home_page_default_set(Browser_Window *win __UNUSED__, More_Menu_Item *mmi __UNUSED__)
{
   config_home_page_set(config, DEFAULT_URL);
   return NULL;
}

static void
on_more_clear_click(void *data, Evas_Object *o __UNUSED__,
           const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *more_list = evas_object_data_get(chrome, "more-list");
   Network_Origin *origins;
   Eina_List *origins_list;

   elm_genlist_clear(more_list);

   network_origins_list_clear(network);
   network_save(network, NULL);
}

static inline Eina_Bool 
more_menu_services_item_append_new(More_Menu_Item **items, int *nbItems, const char* text, void* next, void* data) 
{
    More_Menu_Item *new_items;
    More_Menu_Item *bm_item;
    int n_items = *nbItems;

    bm_item = calloc(1, sizeof(More_Menu_Item));
    bm_item->type = ITEM_TYPE_CONFIG;
    bm_item->text = eina_stringshare_add(text);
    bm_item->next = next;
    bm_item->data = data;
    bm_item->flags = ITEM_FLAG_DYNAMIC;

    if (!*items)
        *items = calloc(1, sizeof(More_Menu_Item) * SERVICES_MENU_PREALLOC_SIZE);
    else if (n_items % SERVICES_MENU_PREALLOC_SIZE == 0)
    {
        new_items = realloc(*items, (SERVICES_MENU_PREALLOC_SIZE * n_items * sizeof(More_Menu_Item)));
        if (new_items)
            *items = new_items;
        else {
            free(bm_item);
            return EINA_FALSE;
        }
    }

    memcpy(&(*items)[n_items], bm_item, sizeof(More_Menu_Item));
    *nbItems = ++n_items;
    free(bm_item);
     
    return EINA_TRUE;
}

static More_Menu_Item *
more_menu_services_registered(Browser_Window *win __UNUSED__, More_Menu_Item *current_item __UNUSED__)
{
    Evas_Object *ed = elm_layout_edje_get(win->current_chrome);
    More_Menu_Item *bm_item;
    More_Menu_Item *ret = NULL, *new_ret;
    Eina_List *l, *origins = network_origins_list_get(network);
    Network_Origin *network_origin;
    int n_items = 0;
#define MAX_SUBTEXT_LEN 128

    EINA_LIST_FOREACH(origins, l, network_origin)
    {
        Eina_List *ll, *devices = network_origin_devices_list_get(network_origin);
        Eina_Iterator *service_itr = eina_hash_iterator_key_new(network_origin_services_hash_get(network_origin));
        Device *device;
        const char *service_id;

        if (more_menu_services_item_append_new(&ret, &n_items, 
                                               network_origin_origin_get(network_origin),
                                               more_config_network_origin, network_origin) == EINA_FALSE)
           goto realloc_error;

        EINA_ITERATOR_FOREACH(service_itr, service_id)
        {
            Services *services = network_origin_services_get(network_origin, service_id);

            if (more_menu_services_item_append_new(&ret, &n_items, 
                                                   services_model_get(services),
                                                   more_config_services, services) == EINA_FALSE)
               goto realloc_error;
        }
             
        EINA_LIST_FOREACH(devices, ll, device)
        {
            service_itr = eina_hash_iterator_key_new(device_services_hash_get(device));

            if (more_menu_services_item_append_new(&ret, &n_items, 
                                                   device_friendly_name_get(device),
                                                   more_config_device, device) == EINA_FALSE)
               goto realloc_error;

            EINA_ITERATOR_FOREACH(service_itr, service_id)
            {
                Services *services = device_services_get(device, service_id);
            
                if (more_menu_services_item_append_new(&ret, &n_items, 
                                                   services_model_get(services),
                                                   more_config_upnp_services, services) == EINA_FALSE)
                   goto realloc_error;
            }
        }
    }
    
    if (n_items) {
        /* Display the clear button on the more menu */
        edje_object_signal_callback_del(ed, "more,clear,clicked", "", on_more_clear_click);
        edje_object_signal_callback_add(ed, "more,clear,clicked", "", on_more_clear_click, win->current_chrome);
        edje_object_signal_emit(ed, "more,clear,show", "");
    }
    
realloc_error:

    if (!n_items)
        return NULL;

    bm_item = calloc(1, sizeof(More_Menu_Item));
    bm_item->type = ITEM_TYPE_LAST;
    new_ret = realloc(ret, (1 + n_items) * sizeof(*ret));
    if (!new_ret)
    {
        free(bm_item);
        free(ret);
        return NULL;
    }

    ret = new_ret;
    memcpy(&ret[n_items], bm_item, sizeof(*ret));
    free(bm_item);

    return ret;
}

static More_Menu_Item *
more_menu_privacy_clear_everything(Browser_Window *win __UNUSED__, More_Menu_Item *mmi)
{
   more_menu_privacy_clear_cache(win, mmi);
   more_menu_privacy_clear_history(win, mmi);
   more_menu_privacy_clear_database(win, mmi);
   more_menu_privacy_clear_cookies(win, mmi);

   return NULL;
}

static More_Menu_Item *
more_menu_privacy_clear_cache(Browser_Window *win __UNUSED__, More_Menu_Item *mmi __UNUSED__)
{
   /* FIXME: Eve does not support disk cache yet */
   return NULL;
}

static More_Menu_Item *
more_menu_privacy_clear_history(Browser_Window *win __UNUSED__, More_Menu_Item *mmi __UNUSED__)
{
  hist_free(hist);
  hist = hist_new(0);
  return NULL;
}

static More_Menu_Item *
more_menu_privacy_clear_database(Browser_Window *win __UNUSED__, More_Menu_Item *mmi __UNUSED__)
{
  /* FIXME: Clear HTML5 database */
  return NULL;
}

static More_Menu_Item *
more_menu_privacy_clear_cookies(Browser_Window *win __UNUSED__, More_Menu_Item *mmi __UNUSED__)
{
  ewk_cookies_clear();
  return NULL;
}

static void
_popup_item_selected(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Object *view = data;
   Elm_Object_Item *list_it = elm_list_selected_item_get(obj);
   const Eina_List *itr, *list = elm_list_items_get(obj);
   void *d;
   int i = 0;

   EINA_LIST_FOREACH(list, itr, d)
   {
      if (d == list_it)
         break;

      i++;
   }

   ewk_view_popup_selected_set(view, i);
   ewk_view_popup_destroy(view);
}

static void
on_view_popup_new(void *data __UNUSED__, Evas_Object *view, void *event_info)
{
   Ewk_Menu *menu = event_info;
   Ewk_Menu_Item *item;
   Eina_List *l;
   Evas_Object *li;
   Evas_Object *chrome = evas_object_data_get(view, "chrome");
   Evas_Object *notify = elm_notify_add(chrome);

   elm_object_style_set(notify, "ewebkit");
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_allow_events_set(notify, EINA_FALSE);
   elm_notify_orient_set(notify, ELM_NOTIFY_ORIENT_BOTTOM);

   li = elm_list_add(view);
   elm_object_style_set(li, "ewebkit");
   elm_list_select_mode_set(li, ELM_OBJECT_SELECT_MODE_ALWAYS);
   evas_object_size_hint_weight_set(li, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(li, EVAS_HINT_FILL, EVAS_HINT_FILL);

   EINA_LIST_FOREACH(menu->items, l, item)
   elm_list_item_append(li, item->text, NULL, NULL, _popup_item_selected,
                        view);

   elm_list_go(li);
   evas_object_show(li);
   elm_object_content_set(notify, li);

   evas_object_smart_callback_add(view, "popup,willdelete",
                                  on_view_popup_delete, notify);

   evas_object_show(notify);
}

static void 
on_service_item_change(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Service_Item *item = data;
   Eina_Bool state = elm_check_state_get(obj);

   service_item_allowed_set(item, state);
}

static void 
service_list_release(Evas_Object *view)
{
   Evas_Object *chrome = evas_object_data_get(view, "chrome");
   Evas_Object *hl = evas_object_data_get(chrome, "service-list");
   Eina_List *service_items = evas_object_data_get(hl, "service-items");
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Eina_List *service_iter = evas_object_data_get(chrome, "services");
   Eina_List *l;
   Ewk_NetworkServices *srv;

   /* release widgets */
   elm_genlist_clear(hl);
   service_items_clear(&service_items);
   evas_object_data_set(hl, "service-items", service_items);

   /* release ewk objects */
   EINA_LIST_FOREACH(service_iter, l, srv)
      ewk_network_services_free(srv);

   service_iter = eina_list_free(service_iter);
   evas_object_data_set(chrome, "services", service_iter);

   edje_object_signal_emit(ed, "hide,service,button", "");
}

static void
on_action_nsd(void *data, Evas_Object *o __UNUSED__,
           const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);

   edje_object_signal_emit(ed, "toggle,service", "");
}

static void
on_action_valid(void *data, Evas_Object *o __UNUSED__,
           const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Eina_List *service_iter = evas_object_data_get(chrome, "services");
   Evas_Object *hl = evas_object_data_get(chrome, "service-list");
   Eina_List *service_items = evas_object_data_get(hl, "service-items");
   Eina_List *l;
   Ewk_NetworkServices *srv;
   Service_Item *item;

   EINA_LIST_FOREACH(service_iter, l, srv)
      ewk_network_services_allowed_notify(srv);

   /* Save configuration service */
   EINA_LIST_FOREACH(service_items, l, item) 
   {
        Eina_Bool allowed = service_item_widget_allowed_get(item);

        /* Apply to the configuration */
        service_config_register_services(item, allowed);
   }
   network_save(network, NULL);

   service_iter = eina_list_free(service_iter);
   evas_object_data_set(chrome, "services", service_iter);

   edje_object_signal_emit(ed, "toggle,service", "");
   edje_object_signal_emit(ed, "permission,finished,service", "");
}

static void 
service_item_del(void *data, Evas_Object *obj, void *event_info)
{
   service_item_widget_set((Service_Item *)data, NULL);    
}

static void 
service_list_append(void *data, Service_Item *item) 
{
   Elm_Object_Item *gl_item = NULL;
   Evas_Object *list = (Evas_Object*)data;
   Eve_Network_Type type = service_item_type_get(item);

   switch (type) {
   case NETWORK_TYPE_DEVICE_UPNP:
      gl_item = elm_genlist_item_sorted_insert(list, &glic_device_list, item,
                                        service_item_parent_widget_get(item), 
                                        ELM_GENLIST_ITEM_TREE, &service_item_compare, NULL, NULL);
      elm_genlist_item_expanded_set(gl_item, EINA_FALSE);
      break;

   case NETWORK_TYPE_SERVICE_ZEROCONF:
      gl_item = elm_genlist_item_sorted_insert(list, &glic_service_list, item,
                                        service_item_parent_widget_get(item), 
                                        ELM_GENLIST_ITEM_NONE, &service_item_compare, NULL, NULL);
      break;

   case NETWORK_TYPE_SERVICE_UNKNOWN:
      gl_item = elm_genlist_item_sorted_insert(list, &glic_service_origin, item, NULL, 
                                        ELM_GENLIST_ITEM_TREE, &service_item_compare, NULL, NULL);
      elm_genlist_item_expanded_set(gl_item, EINA_TRUE);
      break;

   default:
      break;
   }

   if (gl_item)
      elm_object_item_del_cb_set(gl_item, service_item_del);

   service_item_widget_set(item, gl_item);
}

void *
service_icon_from_data_get(const void *widget, const void *data, size_t size) {
   const Elm_Object_Item *gl_item = widget;
   Evas_Object *icon;
   
   icon = evas_object_image_add(evas_object_evas_get(elm_object_item_widget_get(gl_item)));
   evas_object_image_memfile_set(icon, (void *)data, size, "png", NULL);
   
   return icon;
}

static void
on_service_refresh_click(void *data, Evas_Object *o __UNUSED__,
           const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Evas_Object *hl = evas_object_data_get(chrome, "service-list");
   Eina_List *service_items = evas_object_data_get(hl, "service-items");
   Eina_List *service_iter = evas_object_data_get(chrome, "services");
   Eina_List *service_updated_iter = evas_object_data_get(chrome, "services-updated");
   Eina_List *l, *updated_l;
   Ewk_NetworkServices *srv, *updated_srv;
   Eina_Bool found = EINA_FALSE;

   EINA_LIST_FOREACH(service_iter, l, srv)
      EINA_LIST_FOREACH(service_updated_iter, updated_l, updated_srv)
         if (srv == updated_srv) {
            found = EINA_TRUE;
            break;
         }

   if (found == EINA_TRUE) {
      elm_genlist_clear(hl);
      service_items_clear(&service_items);
      service_list_set(service_iter, network, &service_items, &service_list_append, hl);
      evas_object_data_set(hl, "service-items", service_items);
      elm_genlist_realized_items_update(hl);
   }

   eina_list_free(service_updated_iter);
   evas_object_data_set(chrome, "services-updated", service_updated_iter);
   edje_object_signal_emit(ed, "list,refresh,hide", "");
}

static void
on_service_cancel_click(void *data, Evas_Object *o __UNUSED__,
           const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Eina_List *service_iter = evas_object_data_get(chrome, "services");
   Eina_List *l;
   Ewk_NetworkServices *srv;

   EINA_LIST_FOREACH(service_iter, l, srv)
      ewk_network_services_denied_notify(srv);

   service_iter = eina_list_free(service_iter);
   evas_object_data_set(chrome, "services", service_iter);

   edje_object_signal_emit(ed, "toggle,service", "");
   edje_object_signal_emit(ed, "permission,finished,service", "");
}

static void
on_view_networkservices_request_started(void *data, Evas_Object *view, void *event_info)
{
   Ewk_NetworkServices* services = (Ewk_NetworkServices*)event_info;
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Eina_List *service_iter = evas_object_data_get(chrome, "services");
    
   service_iter = eina_list_append(service_iter, services);

   evas_object_data_set(chrome, "services", service_iter);
   edje_object_signal_emit(ed, "show,service,button", "");
}

static void
on_view_networkservices_request_updated(void *data, Evas_Object *view, void *event_info)
{
   Ewk_NetworkServices *services = (Ewk_NetworkServices*)event_info;
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Eina_List *service_iter = evas_object_data_get(chrome, "services-updated");
   Eina_Bool register_callback = EINA_FALSE;
    
   if (!service_iter)
      register_callback = EINA_TRUE;

   service_iter = eina_list_append(service_iter, services);

   evas_object_data_set(chrome, "services-updated", service_iter);

   if (register_callback == EINA_TRUE) {
      edje_object_signal_callback_add(ed, "list,refresh,clicked", "", on_service_refresh_click, chrome);
      edje_object_signal_emit(ed, "list,refresh,show", "");
   }
}

static void
on_view_networkservices_request_canceled(void *data, Evas_Object *view, void *event_info)
{
   Ewk_NetworkServices *services = (Ewk_NetworkServices*)event_info;
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Eina_List *service_iter = evas_object_data_get(chrome, "services");
    
   service_iter = eina_list_remove(service_iter, services);

   evas_object_data_set(chrome, "services", service_iter);
}

void service_item_widget_update(const void *widget) 
{
    elm_genlist_item_update((Elm_Object_Item *)widget);
}

static void
on_view_networkservices_request_finished(void *data, Evas_Object *view, void *event_info __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Evas_Object *hl = evas_object_data_get(chrome, "service-list");
   Eina_List *service_items = evas_object_data_get(hl, "service-items");
   Eina_List *service_iter = evas_object_data_get(chrome, "services");
   Eina_Bool require_authority;
   Eina_Bool auto_access;

   if (!service_iter)
      return;

   elm_genlist_clear(hl);
   
   require_authority = service_list_set(service_iter, network, &service_items, &service_list_append, hl);

   evas_object_data_set(hl, "service-items", service_items);

   auto_access = config_enable_auto_network_access_get(config);

   /* Apply configuration setting: network origin with the associate 
    * network services previously requested and validate */
   if ((require_authority == EINA_TRUE) || (auto_access == EINA_FALSE)) {
      edje_object_signal_emit(ed, "permission,request,service", "");
      edje_object_signal_emit(ed, "list,refresh,hide", "");
      edje_object_signal_emit(ed, "toggle,service", "");
   
      edje_object_signal_callback_del(ed, "list,cancel,clicked", "", on_service_cancel_click);
      edje_object_signal_callback_add(ed, "list,cancel,clicked", "", on_service_cancel_click, chrome);
   } else {
       Eina_List *l;
       Ewk_NetworkServices *srv;
       Service_Item *item;

       EINA_LIST_FOREACH(service_iter, l, srv)
          ewk_network_services_allowed_notify(srv);

       /* Save configuration service */
       EINA_LIST_FOREACH(service_items, l, item) 
       {
           Eina_Bool allowed = service_item_widget_allowed_get(item);

           /* Apply to the configuration */
           service_config_register_services(item, allowed);
       }
       network_save(network, NULL);

       service_iter = eina_list_free(service_iter);
       evas_object_data_set(chrome, "services", service_iter);
       edje_object_signal_emit(ed, "permission,finished,service", "");
   }
}

static void
on_tab_close(void *data, Evas_Object *o,
             const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Browser_Window *win = evas_object_data_get(o, "win");
   Evas_Object *chrome = evas_object_data_get(data, "chrome");
   Evas_Object *view = evas_object_data_get(chrome, "view");
   Elm_Object_Item *item = evas_object_data_get(o, "item");
   Eina_List *win_iter;

   elm_object_item_del(item);
   tab_close_chrome(win, chrome);

   if (win->current_chrome)
      {
         Evas_Object *ed = elm_layout_edje_get(win->current_chrome);
         edje_object_signal_emit(ed, "show,tab", "");
      }

   EINA_LIST_FOREACH(app.windows, win_iter, win)
   {
      Eina_List *chrome_iter;
      Evas_Object *chrome;

      EINA_LIST_FOREACH(win->chromes, chrome_iter, chrome)
      {
         Evas_Object *grid = evas_object_data_get(chrome, "tab-grid");

         if (!grid) continue;
         for (item = elm_gengrid_first_item_get(grid);
              item;
              item = elm_gengrid_item_next_get(item))
            {
               if (elm_object_item_data_get(item) == view)
                  {
                     elm_object_item_del(item);
                     break;
                  }
            }
      }
   }
}

static void
on_tab_gengrid_item_realized(void *data, Evas_Object *o __UNUSED__, void *event_info)
{
   Browser_Window *win = data;
   Evas_Object *item = (Evas_Object *)elm_gengrid_item_object_get(event_info);
   Evas_Object *view = elm_object_item_data_get(event_info);

   evas_object_data_set(item, "item", event_info);
   evas_object_data_set(item, "win", win);
   edje_object_signal_callback_add(item, "tab,close", "", on_tab_close, view);

   win->creating_tab = EINA_TRUE;
   elm_gengrid_item_selected_set(event_info, view == win->current_view);
   win->creating_tab = EINA_FALSE;
}

static void
on_action_clear(void *data, Evas_Object *o __UNUSED__,
               void *event_info __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Evas_Object *text_url = edje_object_part_swallow_get(ed, "url-entry");
   elm_object_text_set(text_url, "");
}

static void
on_action_back(void *data, Evas_Object *o __UNUSED__,
               const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *view = data;

   ewk_view_back(view);
}

static void
on_action_forward(void *data, Evas_Object *o __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   Evas_Object *view = data;

   ewk_view_forward(view);
}

void
chrome_config_apply(Evas_Object *chrome)
{
   Evas_Object *view = evas_object_data_get(chrome, "view");
   Browser_Window *win = evas_object_data_get(chrome, "win");

   ewk_view_setting_enable_scripts_set(view, config_enable_javascript_get(config));
   ewk_view_setting_enable_plugins_set(view, config_enable_plugins_get(config));
   ewk_view_setting_user_agent_set(view, config_user_agent_get(config));
   ewk_view_setting_private_browsing_set(view, config_enable_private_mode_get(config));
   ewk_view_setting_auto_load_images_set(view, config_enable_auto_load_images_get(config));
   ewk_view_setting_auto_shrink_images_set(view, config_enable_auto_shrink_images_get(config));
   ewk_view_setting_scripts_can_open_windows_set(view, config_allow_popup_get(config));
   view_touch_interface_set(view, config_enable_touch_interface_get(config));
   window_mouse_enabled_set(win->win, config_enable_mouse_cursor_get(config));
   ewk_view_text_zoom_set(view, config_text_zoom_get(config));
   ewk_view_setting_enable_frame_flattening_set(view, config_frame_flattening_get(config));
   ewk_view_setting_font_minimum_size_set(view, config_minimum_font_size_get(config));
   ewk_cookies_policy_set(config_cookie_policy_get(config));
}

static void
conf_updated(More_Menu_Config *mmc, void *new_value)
{
   Evas_Object *chrome, *view;
   Browser_Window *win;
   Eina_List *win_iter, *chrome_iter;

#define SET_PREF_TO_ALL_VIEWS(fn,newvalue)                    \
      EINA_LIST_FOREACH(app.windows, win_iter, win)           \
      {                                                       \
         EINA_LIST_FOREACH(win->chromes, chrome_iter, chrome) \
         {                                                    \
            view = evas_object_data_get(chrome, "view");      \
            fn(view, newvalue);                               \
         }                                                    \
      }

   switch (mmc->conf) {
   case EVE_CONFIG_ENABLE_JAVASCRIPT:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_setting_enable_scripts_set, *((int *)new_value));
         break;
      }
   case EVE_CONFIG_ENABLE_PLUGINS:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_setting_enable_plugins_set, *((int *)new_value));
         break;
      }
   case EVE_CONFIG_USER_AGENT:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_setting_user_agent_set, new_value);
         break;
      }
   case EVE_CONFIG_ENABLE_PRIVATE_MODE:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_setting_private_browsing_set, *((int *)new_value));
         break;
      }
   case EVE_CONFIG_AUTO_LOAD_IMAGES:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_setting_auto_load_images_set, *((int *)new_value));
         break;
      }
   case EVE_CONFIG_AUTO_SHRINK_IMAGES:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_setting_auto_shrink_images_set, *((int *)new_value));
         break;
      }
   case EVE_CONFIG_POPUP_ALLOW:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_setting_scripts_can_open_windows_set, *((int *)new_value));
         break;
      }
    case EVE_CONFIG_TEXT_ONLY_ZOOM:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_text_zoom_set, *((int*)new_value));
         break;
      }
    case EVE_CONFIG_FRAME_FLATTENING:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_setting_enable_frame_flattening_set, *((int*)new_value));
         break;
      }
    case EVE_CONFIG_MINIMUM_FONT_SIZE:
      {
         SET_PREF_TO_ALL_VIEWS(ewk_view_setting_font_minimum_size_set, *((int*)new_value));
         break;
      }
    case EVE_CONFIG_TOUCH_INTERFACE:
      {
         SET_PREF_TO_ALL_VIEWS(view_touch_interface_set, *((int*)new_value));
         break;
      }
   case EVE_CONFIG_RESTORE_STATE:
      /* This setting should be saved immediately, so that if the browser crashes before creating
         the initial configuration (and the user chose to save/restore the session), he doesn't
         loses the state when reopening the browser. */
      config_save(config, NULL);
      break;
   case EVE_CONFIG_MOUSE_CURSOR:
      {
         EINA_LIST_FOREACH(app.windows, win_iter, win)
         {
            window_mouse_enabled_set(win->win, *((int *)new_value));
         }
         break;
      }
   case EVE_CONFIG_COOKIE_POLICY:
      {
         ewk_cookies_policy_set((*(Ewk_Cookie_Policy *)new_value));
         break;
      }
   default: ;
   }

#undef SET_PREF_TO_ALL_VIEWS
}

static void
cb_config_bool_changed(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   More_Menu_Config *mmc = data;
   void (*conf_set)(Config *, Eina_Bool);

   if ((conf_set = mmc->conf_set))
      {
         conf_set(config, elm_check_state_get(obj));
         conf_updated(mmc, (int[]){ elm_check_state_get(obj) });
      }
}

static void
cb_config_device_changed(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   device_allowed_set((Device *)data, elm_check_state_get(obj));
   network_save(network, NULL);
}

static void
cb_config_services_changed(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   services_allowed_set((Services *)data, elm_check_state_get(obj));
   network_save(network, NULL);
}

static void
cb_config_int_changed(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   More_Menu_Config *mmc = data;
   void (*conf_set)(Config *, int);

   if ((conf_set = mmc->conf_set))
      {
         conf_set(config, elm_spinner_value_get(obj));
         conf_updated(mmc, (int[]){ elm_spinner_value_get(obj) });
      }
}

static void
cb_config_string_changed(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   More_Menu_Config *mmc = data;
   void (*conf_set)(Config *, const char *);

   if ((conf_set = mmc->conf_set))
      {
         conf_set(config, elm_object_text_get(obj));
         conf_updated(mmc, (void *)elm_object_text_get(obj));
      }
}

static void
cb_config_removed(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   More_Menu_Config *mmc = data;
   void (*conf_set)(Config *, const char *);

}

static Evas_Object *
config_icon_get(Evas_Object *parent, More_Menu_Item *mmi)
{
   More_Menu_Config *mmc = mmi->next;
   Evas_Object * evas_icon = NULL;

   if (!mmc->conf_get) return NULL;

   switch (mmc->type) {
   case CONFIG_TYPE_DEVICE:
      {
          Device *device = mmi->data;
          evas_icon = device_icon_get(device, evas_object_evas_get(parent), service_config_filename_get());
          break;
      }
   case CONFIG_TYPE_UPNP_SERVICES:
   case CONFIG_TYPE_SERVICES:
      {
          Services *services = mmi->data;
          evas_icon = services_icon_get(services, evas_object_evas_get(parent), service_config_filename_get());
          break;
      }
   default:
      return NULL;
   }
   
   if (evas_icon) {
       Evas_Object *icon;
       Evas_Object *inlined_icon;
       void *data;
       int w, h;
                                                                                                      
       icon = elm_icon_add(parent);
       inlined_icon = elm_image_object_get(icon);
                                                                                                      
       evas_object_image_size_get(evas_icon, &w, &h);
       data = evas_object_image_data_get(evas_icon, EINA_FALSE);
                                                                                                      
       evas_object_image_size_set(inlined_icon, w, h);
       evas_object_image_data_copy_set(inlined_icon, data);
     
       return icon;
   }

   return NULL;
}

static Evas_Object *
config_widget_get(Evas_Object *parent, More_Menu_Item *mmi)
{
   More_Menu_Config *mmc = mmi->next;

   if (!mmc->conf_get) return NULL;

   switch (mmc->type) {
   case CONFIG_TYPE_CHECKBOX:
      {
         Eina_Bool (*conf_get)(Config *);
         Evas_Object *toggle = elm_check_add(parent);

         conf_get = mmc->conf_get;
         elm_object_style_set(toggle, "ewebkit");
         elm_object_part_text_set(toggle, "on", "ON");
         elm_object_part_text_set(toggle, "off", "OFF");
         elm_check_state_set(toggle, conf_get(config));
         evas_object_smart_callback_add(toggle, "changed", cb_config_bool_changed, mmc);

         return toggle;
      }
   case CONFIG_TYPE_DEVICE:
      {
         Eina_Bool (*conf_get)(Device *);
         Evas_Object *toggle = elm_check_add(parent);

         conf_get = mmc->conf_get;
         elm_object_style_set(toggle, "ewebkit");
         elm_object_part_text_set(toggle, "on", "ON");
         elm_object_part_text_set(toggle, "off", "OFF");
         elm_check_state_set(toggle, conf_get(mmi->data));
         evas_object_smart_callback_add(toggle, "changed", cb_config_device_changed, mmi->data);

         return toggle;
      }

   case CONFIG_TYPE_SERVICES:
      {
         Eina_Bool (*conf_get)(Services *);
         Evas_Object *toggle = elm_check_add(parent);

         conf_get = mmc->conf_get;
         elm_object_style_set(toggle, "ewebkit");
         elm_object_part_text_set(toggle, "on", "ON");
         elm_object_part_text_set(toggle, "off", "OFF");
         elm_check_state_set(toggle, conf_get(mmi->data));
         evas_object_smart_callback_add(toggle, "changed", cb_config_services_changed, mmi->data);

         return toggle;
      }

   case CONFIG_TYPE_SPINNER:
      {
         int (*conf_get)(Config *);
         Evas_Object *spinner = elm_spinner_add(parent);
         More_Menu_Config_Spinner *spinner_params = mmc->data;

         conf_get = mmc->conf_get;
         elm_spinner_min_max_set(spinner, spinner_params->min, spinner_params->max);
         if (spinner_params->format) elm_spinner_label_format_set(spinner, spinner_params->format);
         elm_spinner_value_set(spinner, conf_get(config));
         evas_object_smart_callback_add(spinner, "changed", cb_config_int_changed, mmc);

         return spinner;
      }
   default:
      return NULL;
   }
}

static void
on_list_completely_hidden(void *data, Evas_Object *ed, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   More_Menu_Set_Params *params = data;
   Evas_Object *index;
   Browser_Window *win = evas_object_data_get(params->chrome, "win");
   int i;
   char last_index = '\0';

   if (!eina_list_data_find(win->list_history, params->root))
     {
        if (!params->root)
           params->root = more_menu_root;

        if (params->root == more_menu_root)
           win->list_history = eina_list_prepend(win->list_history, NULL);
        else
           win->list_history = eina_list_prepend(win->list_history, params->root);
     }

   elm_genlist_clear(params->list);
   index = evas_object_data_get(params->list, "more-index");
   if (index)
      elm_index_item_clear(index);

   if (params->root != more_menu_root)
     {
        edje_object_part_text_set(ed, "more-list-back-button-text", eina_stringshare_add(params->old_text ? params->old_text : "More"));
        edje_object_signal_callback_del(ed, "list,back,clicked", "", on_more_item_back_click);
        edje_object_signal_callback_add(ed, "list,back,clicked", "", on_more_item_back_click, params->list);

        edje_object_signal_emit(ed, "list,back,show", "");
     }

   for (i = 0; params->root[i].type != ITEM_TYPE_LAST; i++)
     {
        Elm_Object_Item *gl_item = NULL;
        switch (params->root[i].type) {
        case ITEM_TYPE_SEPARATOR:
           {
               gl_item = elm_genlist_item_append(params->list, &glic_separator, NULL,
                        NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
               elm_object_item_disabled_set(gl_item, EINA_TRUE);
               break;
           }
        case ITEM_TYPE_PAGE:
           {
                gl_item = elm_genlist_item_append(params->list, &glic_page, &(params->root[i]),
                        NULL, ELM_GENLIST_ITEM_NONE, on_more_item_click, &(params->root[i]));
                break;
           }
        case ITEM_TYPE_CONFIG:
           {
               More_Menu_Config *mmc = params->root[i].next;
               static Elm_Object_Item *gl_parent_device_item = NULL;
               static Elm_Object_Item *gl_parent_network_origin_item = NULL;

               if ((mmc->type == CONFIG_TYPE_LIST) || 
                   (mmc->type == CONFIG_TYPE_LIST_INT) || 
                   (mmc->type == CONFIG_TYPE_STRING))
                  gl_item = elm_genlist_item_append(params->list, &glic_config_list,
                      &(params->root[i]), NULL, ELM_GENLIST_ITEM_NONE, on_more_item_click, &(params->root[i]));
               else if (mmc->type == CONFIG_TYPE_NETWORK_ORIGIN) {
                  gl_item = elm_genlist_item_sorted_insert(params->list, &glic_config_origin, &(params->root[i]), 
                                                           NULL, ELM_GENLIST_ITEM_TREE, 
                                                           more_item_compare, NULL, NULL);
                  evas_object_smart_callback_del(params->list, "expanded", more_list_expanded_cb);
                  elm_genlist_item_expanded_set(gl_item, EINA_TRUE);
                  evas_object_smart_callback_add(params->list, "expanded", more_list_expanded_cb, params->list);
                  gl_parent_network_origin_item = gl_item;
               } else if (mmc->type == CONFIG_TYPE_DEVICE) {
                  gl_item = elm_genlist_item_sorted_insert(params->list, &glic_config_origin, &(params->root[i]), 
                                                           gl_parent_network_origin_item, ELM_GENLIST_ITEM_TREE, 
                                                           more_item_compare, NULL, NULL);
                  elm_genlist_item_expanded_set(gl_item, EINA_FALSE);
                  elm_object_item_del_cb_set(gl_item, service_item_del);
                  gl_parent_device_item = gl_item;
               } else if (mmc->type == CONFIG_TYPE_UPNP_SERVICES) {
                  /* postpone item insertion to expanded device callback (more_list_expand_request_cb) */
                  /*gl_item = elm_genlist_item_sorted_insert(params->list, &glic_config_service, &(params->root[i]), 
                                                           gl_parent_device_item, ELM_GENLIST_ITEM_NONE, 
                                                           more_item_compare, NULL, NULL);
                  elm_object_item_del_cb_set(gl_item, service_item_del);*/
               } else if (mmc->type == CONFIG_TYPE_SERVICES) {
                  gl_item = elm_genlist_item_sorted_insert(params->list, &glic_config_service, &(params->root[i]), 
                                                           gl_parent_network_origin_item, ELM_GENLIST_ITEM_NONE, 
                                                           more_item_compare, NULL, NULL);
                  elm_object_item_del_cb_set(gl_item, service_item_del);
               } else if (params->root[i].flags & ITEM_FLAG_SELECTABLE)
                  gl_item = elm_genlist_item_append(params->list, &glic_config_selectable,
                      &(params->root[i]), NULL, ELM_GENLIST_ITEM_NONE, on_more_item_click, &(params->root[i]));
               else
                  gl_item = elm_genlist_item_append(params->list, &glic_config,
                       &(params->root[i]), NULL, ELM_GENLIST_ITEM_NONE, on_more_item_click, &(params->root[i]));
               break;
           }
           /* fallthrough */
        default:
           if (params->root[i].flags & ITEM_FLAG_SELECTABLE)
              gl_item = elm_genlist_item_append(params->list, &glic_config_selectable,
                 &(params->root[i]), NULL, ELM_GENLIST_ITEM_NONE, on_more_item_click, &(params->root[i]));
           else
              gl_item = elm_genlist_item_append(params->list, &glic_default, &(params->root[i]), NULL,
                  ELM_GENLIST_ITEM_NONE, on_more_item_click, &(params->root[i]));
        }
        if (gl_item)
          {
             const char *temp = _first_alpha_char(params->root[i].text);

             if (index && temp && isalpha(*temp))
               {
                  char index_label[2];
                  index_label[0] = toupper(*temp);
                  index_label[1] = '\0';
                  if (index_label[0] != last_index)
                    {
                       elm_index_item_append(index, index_label, NULL, gl_item);
                       last_index = index_label[0];
                    }
               }
           }
     }

   edje_object_signal_callback_del(ed, "list,completely,hidden", "", on_list_completely_hidden);

   eina_stringshare_del(params->old_text);
   free(params);
}

static void
more_menu_set(Evas_Object *chrome, Evas_Object *list, More_Menu_Item *root, const char *old_text)
{
   Evas_Object *ed = elm_layout_edje_get(chrome);
   More_Menu_Set_Params *params = calloc(1, sizeof(*params));

   if (!params) return;

   params->chrome = chrome;
   params->list = list;
   params->root = root ? root : more_menu_root;
   params->old_text = eina_stringshare_add(old_text);

   if (params->root == more_menu_root)
     {
        edje_object_part_text_set(ed, "more-list-title", "More");
        edje_object_signal_emit(ed, "list,back,hide", "");

        on_list_completely_hidden(params, NULL, NULL, NULL);
      }
   else
      edje_object_signal_callback_add(ed, "list,completely,hidden", "", on_list_completely_hidden, params);
}

static void
more_menu_dynamic_destroy(More_Menu_Item *mmi)
{
   int i;
   for (i = 0; mmi[i].type != ITEM_TYPE_LAST; i++) eina_stringshare_del(mmi[i].text);
   free(mmi);
}

static void
on_more_item_back_click(void *data, Evas_Object *edje,
                        const char *emission __UNUSED__,
                        const char *source __UNUSED__)
{
   More_Menu_Item *mmi;
   Browser_Window *win = evas_object_data_get(edje, "win");
   Evas_Object *list = data;

   edje_object_signal_emit(edje, "more,clear,hide", "");

   edje_object_signal_emit(edje, "list,animate,right", "");
   edje_object_part_text_set(edje, "more-list-title", edje_object_part_text_get(edje, "more-list-back-button-text"));
   eina_stringshare_del(edje_object_part_text_get(edje, "more-list-back-button-text"));

   if ((mmi = win->list_history->data) && mmi->flags & ITEM_FLAG_DYNAMIC) more_menu_dynamic_destroy(mmi);

   win->list_history = eina_list_remove_list(win->list_history, win->list_history);
   win->list_history_titles = eina_list_remove_list(win->list_history_titles, win->list_history_titles);

   if (!win->list_history_titles)
      more_menu_set(win->current_chrome, list, win->list_history->data, "More");
   else
      more_menu_set(win->current_chrome, list, win->list_history->data, win->list_history_titles->data);
}

static void
callback_menu_config_list_set(Browser_Window *win __UNUSED__, More_Menu_Item *i)
{
   More_Menu_Config *p = i->data;
   More_Menu_Config_List *l = p->data;
   void (*conf_set)(Config *, const char *);
   int item;

   for (item = 0; l[item].title; item++)
      {
         if (!strcmp(l[item].title, i->text))
            {
               if ((conf_set = p->conf_set))
                  {
                     conf_set(config, l[item].value);
                     conf_updated(p, (void *)l[item].value);
                  }
               break;
            }
      }
}

static void
callback_menu_config_list_int_set(Browser_Window *win __UNUSED__, More_Menu_Item *i)
{
   More_Menu_Config *p = i->data;
   More_Menu_Config_List_Int *l = p->data;
   void (*conf_set)(Config *, const int);
   int item;

   for (item = 0; l[item].title; item++)
      {
         if (!strcmp(l[item].title, i->text))
            {
               if ((conf_set = p->conf_set))
                  {
                     conf_set(config, l[item].value);
                     conf_updated(p, (int[]){ l[item].value });
                  }
               break;
            }
      }
}

static More_Menu_Item *
more_menu_config_list_create(More_Menu_Item *i __UNUSED__, More_Menu_Config *p)
{
   More_Menu_Config_List *list = p->data;
   More_Menu_Item *mmi;
   const char *(*conf_get)(void *);
   const char *configuration = NULL;
   int n_items, item;

   if (!list) return NULL;
   for (n_items = 0; list[n_items].title; n_items++);
   if (!(mmi = calloc(n_items + 1, sizeof(*mmi)))) return NULL;
   if ((conf_get = p->conf_get)) configuration = conf_get(config);

   for (item = 0; item < n_items; item++) {
      mmi[item].text = eina_stringshare_add(list[item].title);
      mmi[item].next = callback_menu_config_list_set;
      mmi[item].type = ITEM_TYPE_CALLBACK_NO_HIDE;
      mmi[item].data = p;
      mmi[item].flags = (configuration && !strcmp(configuration, list[item].value)) ? ITEM_FLAG_SELECTED : ITEM_FLAG_NONE;
      mmi[item].flags |= ITEM_FLAG_DYNAMIC | ITEM_FLAG_SELECTABLE;
   }

   mmi[item].type = ITEM_TYPE_LAST;

   return mmi;
}

static More_Menu_Item *
more_menu_config_list_int_create(More_Menu_Item *i __UNUSED__, More_Menu_Config *p)
{
   More_Menu_Config_List_Int *list = p->data;
   More_Menu_Item *mmi;
   int (*conf_get)(void *);
   int configuration = 0;
   int n_items, item;

   if (!list) return NULL;
   for (n_items = 0; list[n_items].title; n_items++);
   if (!(mmi = calloc(n_items + 1, sizeof(*mmi)))) return NULL;
   if ((conf_get = p->conf_get)) configuration = conf_get(config);

   for (item = 0; item < n_items; item++) {
      mmi[item].text = eina_stringshare_add(list[item].title);
      mmi[item].next = callback_menu_config_list_int_set;
      mmi[item].type = ITEM_TYPE_CALLBACK_NO_HIDE;
      mmi[item].data = p;
      mmi[item].flags = configuration == list[item].value ? ITEM_FLAG_SELECTED : ITEM_FLAG_NONE;
      mmi[item].flags |= ITEM_FLAG_DYNAMIC | ITEM_FLAG_SELECTABLE;
   }

   mmi[item].type = ITEM_TYPE_LAST;

   return mmi;
}

static void
on_string_ask_ok_click(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Eina_Bool *response = data;
   *response = EINA_TRUE;
   ecore_main_loop_quit();
}

static void
on_string_ask_cancel_click(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Eina_Bool *response = data;
   *response = EINA_FALSE;
   ecore_main_loop_quit();
}

static void
more_menu_config_string_ask(Evas_Object *parent, More_Menu_Item *item, More_Menu_Config *mmc, Eina_Bool password)
{
   Eina_Bool response = EINA_FALSE;
   Evas_Object *bx_h, *bx_v, *lb;
   Evas_Object *bt_cancel, *bt_ok;
   Evas_Object *notify;
   Evas_Object *entry;
   const char *(*conf_get)(Config *);
   void (*conf_set)(Config *, const char *);

   if (!(conf_get = mmc->conf_get) || !(conf_set = mmc->conf_set)) return;

   notify = elm_notify_add(parent);
   elm_notify_orient_set(notify, ELM_NOTIFY_ORIENT_CENTER);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_allow_events_set(notify, EINA_FALSE);

   bx_v = elm_box_add(parent);
   elm_object_content_set(notify, bx_v);
   elm_box_horizontal_set(bx_v, 0);
   evas_object_size_hint_weight_set(bx_v, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bx_v);

   lb = elm_label_add(bx_v);
   elm_object_text_set(lb, item->text);
   elm_box_pack_end(bx_v, lb);
   evas_object_show(lb);

   entry = elm_entry_add(bx_v);
   elm_entry_password_set(entry, password);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_object_text_set(entry, conf_get(config));
   elm_box_pack_end(bx_v, entry);
   evas_object_show(entry);

   bx_h = elm_box_add(bx_v);
   elm_box_horizontal_set(bx_h, 1);
   elm_box_pack_end(bx_v, bx_h);
   evas_object_size_hint_weight_set(bx_h, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx_h, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bx_h);

   bt_cancel = elm_button_add(bx_h);
   elm_object_text_set(bt_cancel, "Cancel");
   elm_box_pack_end(bx_h, bt_cancel);
   evas_object_size_hint_weight_set(bt_cancel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt_cancel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(bt_cancel, "clicked", on_string_ask_cancel_click, &response);
   evas_object_show(bt_cancel);

   bt_ok = elm_button_add(bx_h);
   elm_object_text_set(bt_ok, "OK");
   elm_box_pack_end(bx_h, bt_ok);
   evas_object_size_hint_weight_set(bt_ok, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt_ok, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(bt_ok, "clicked", on_string_ask_ok_click, &response);
   evas_object_show(bt_ok);

   evas_object_show(notify);
   ecore_main_loop_begin();
   evas_object_hide(notify);

   if (response == EINA_TRUE)
      conf_set(config, elm_object_text_get(entry));

   evas_object_del(notify);
}

static More_Menu_Item *
more_menu_config_create(Evas_Object *parent, More_Menu_Item *item, More_Menu_Config *config)
{
   switch (config->type) {
   case CONFIG_TYPE_LIST:
      return more_menu_config_list_create(item, config);
   case CONFIG_TYPE_LIST_INT:
      return more_menu_config_list_int_create(item, config);
   case CONFIG_TYPE_STRING:
      more_menu_config_string_ask(parent, item, config, EINA_FALSE);
      break;
   case CONFIG_TYPE_PASSWORD:
      more_menu_config_string_ask(parent, item, config, EINA_TRUE);
      break;
   default: ;
   }
   return NULL;
}

static int
more_item_compare(const void *item1, const void *item2)
{
   int ret;
   More_Menu_Item *mmi1 = elm_object_item_data_get((const Elm_Object_Item *)item1);
   More_Menu_Item *mmi2 = elm_object_item_data_get((const Elm_Object_Item *)item2);
   More_Menu_Config *mmc1 = mmi1->next;
   More_Menu_Config *mmc2 = mmi2->next;
   const char *str1 = NULL;
   const char *str2 = NULL;
   
   if (mmc1->type == CONFIG_TYPE_DEVICE)
      str1 = device_friendly_name_get((Device *)mmi1->data);
   else if ((mmc1->type == CONFIG_TYPE_SERVICES) || (mmc1->type == CONFIG_TYPE_UPNP_SERVICES))
      str1 = services_model_get((Services *)mmi1->data);

   if (mmc2->type == CONFIG_TYPE_DEVICE)
      str2 = device_friendly_name_get((Device *)mmi2->data);
   else if ((mmc2->type == CONFIG_TYPE_SERVICES) || (mmc2->type == CONFIG_TYPE_UPNP_SERVICES))
      str2 = services_model_get((Services *)mmi2->data);

   if (!(ret = strcmp(str1, str2)))
      return ret;

   return (ret > 0 ? -1 : 1);
}

static void
on_more_item_click(void *data, Evas_Object *obj,
                   void *event_info __UNUSED__)
{
   Evas_Object *chrome = evas_object_data_get(obj, "chrome");
   Evas_Object *ed = elm_layout_edje_get(chrome);
   More_Menu_Item *mmi = data;
   Browser_Window *win = evas_object_data_get(chrome, "win");
   const char *old_text = edje_object_part_text_get(ed, "more-list-title");

   if (!mmi) return;

   if (mmi->flags & ITEM_FLAG_INDEX)
      edje_object_signal_emit(ed, "index,show", "");
   else
      edje_object_signal_emit(ed, "index,hide", "");

   switch (mmi->type) {
      case ITEM_TYPE_STATIC_FOLDER:
         win->list_history_titles = eina_list_prepend(win->list_history_titles, old_text);
         edje_object_signal_emit(ed, "list,animate,left", "");
         edje_object_part_text_set(ed, "more-list-title", mmi->text);
         more_menu_set(chrome, obj, mmi->next, old_text);
         break;

      case ITEM_TYPE_DYNAMIC_FOLDER:
      {
         More_Menu_Callback callback = mmi->next;
         if (!callback) return;

         More_Menu_Item *new_root = callback(win, mmi);
         if (new_root)
           {
              win->list_history_titles = eina_list_prepend(win->list_history_titles, old_text);
              edje_object_part_text_set(ed, "more-list-title", mmi->text);
              edje_object_signal_emit(ed, "list,animate,left", "");
              more_menu_set(chrome, obj, new_root, old_text);
           }
        break;
      }

      case ITEM_TYPE_CONFIG:
      {
         More_Menu_Config *conf;

         if (!mmi->next)
            return;

         conf = mmi->next;
         if ((conf->type == CONFIG_TYPE_CHECKBOX))
         {
             Evas_Object *end = elm_object_item_part_content_get(event_info, "elm.swallow.end");
             if (end) elm_check_state_set(end, !elm_check_state_get(end));
             elm_genlist_item_selected_set(event_info, EINA_FALSE);
             return;
         }

         More_Menu_Item *new_root = more_menu_config_create(win->win, mmi, conf);
         if (new_root)
            {
               win->list_history_titles = eina_list_prepend(win->list_history_titles, old_text);
               edje_object_signal_emit(ed, "list,animate,left", "");
               edje_object_part_text_set(ed, "more-list-title", mmi->text);
               more_menu_set(chrome, obj, new_root, old_text);
            }
         break;
      }

      case ITEM_TYPE_LAST:
      case ITEM_TYPE_SEPARATOR:
      break;

      case ITEM_TYPE_CALLBACK:
         edje_object_signal_emit(ed, "more,item,clicked", "");
         /* fallthrough */
      case ITEM_TYPE_CALLBACK_NO_HIDE:
      {
         More_Menu_Callback callback = mmi->next;
         Evas_Object *ed = elm_layout_edje_get(chrome);
         if (callback)
            callback(win, mmi);
         if (mmi->type == ITEM_TYPE_CALLBACK_NO_HIDE)
            on_more_item_back_click(obj, ed, NULL, NULL);
         break;
      }

      default:
      {
         Browser_Window *win = evas_object_data_get(chrome, "win");
         Evas_Object *ed = elm_layout_edje_get(chrome);
         if (win)
            ewk_view_uri_set(win->current_view, mmi->next);

         edje_object_signal_emit(ed, "more,item,clicked", "");
      }
      break;
     }
}

static void
tab_grid_item_click(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *chrome = data;
   Browser_Window *win = evas_object_data_get(chrome, "win");
   if (!win->creating_tab)
     {
        Evas_Object *ed = elm_layout_edje_get(chrome);
        edje_object_signal_emit(ed, "tab,item,clicked", "");
        tab_focus_chrome(win, chrome);
     }
}

static void
on_action_addtab(void *data, Evas_Object *o __UNUSED__,
                 const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *chrome = data;
   Browser_Window *win = evas_object_data_get(chrome, "win");
   Evas_Object *ed = elm_layout_edje_get(chrome);

   edje_object_signal_emit(ed, "tab,item,clicked", "");
   tab_add(win, config_home_page_get(config), NULL);
}

static void
on_action_tab_show(void *data, Evas_Object *o __UNUSED__,
                   const char *emission __UNUSED__,
                   const char *source __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *grid = evas_object_data_get(chrome, "tab-grid");
   Browser_Window *win = evas_object_data_get(chrome, "win");
   Eina_List *itr;
   Evas_Object *itr_chrome;

   elm_gengrid_clear(grid);

   EINA_LIST_FOREACH(win->chromes, itr, itr_chrome)
   {
      Evas_Object *itr_view = evas_object_data_get(itr_chrome, "view");
      elm_gengrid_item_append(grid, &gic_default, itr_view, tab_grid_item_click, itr_chrome);
   }
}

static void
on_action_more_hide(void *data, Evas_Object *o __UNUSED__,
                        const char *emission __UNUSED__,
                        const char *source __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *edje = elm_layout_edje_get(chrome);
   More_Menu_Item *mmi;
   Browser_Window *win = evas_object_data_get(chrome, "win");

   EINA_LIST_FREE(win->list_history, mmi)
     if (mmi && mmi->flags & ITEM_FLAG_DYNAMIC) more_menu_dynamic_destroy(mmi);

   eina_stringshare_del(edje_object_part_text_get(edje, "more-list-back-button-text"));
   eina_list_free(win->list_history_titles);
   win->list_history = NULL;
   win->list_history_titles = NULL;
}

static void
on_action_more_show(void *data, Evas_Object *o __UNUSED__,
                        const char *emission __UNUSED__,
                        const char *source __UNUSED__)
{
   Evas_Object *chrome = data;
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Evas_Object *hl = evas_object_data_get(chrome, "more-list");

   more_menu_set(chrome, hl, NULL, NULL);
   edje_object_signal_emit(ed, "index,hide", "");
}

static void
on_action_pause(void *data, Evas_Object *o __UNUSED__,
                const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *view = data;

   ewk_view_stop(view);
}

static void
on_action_reload(void *data, Evas_Object *o __UNUSED__,
                 const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *view = data;

   service_list_release(view);
   ewk_view_reload(view);
}

static void
on_action_home(void *data, Evas_Object *o __UNUSED__,
               const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *view = data;

   ewk_view_uri_set(view, config_home_page_get(config));
}

static void
on_action_load_page(void *data, Evas_Object *view, void *event_info __UNUSED__)
{
   Evas_Object *ewk_view = data;

   const char *entry_data = elm_object_text_get(view);
   char *uri;

   if ((uri = uri_sanitize(entry_data)))
     {
        ewk_view_uri_set(ewk_view, uri);
        evas_object_focus_set(ewk_view, EINA_TRUE);
        free(uri);
     }
}

static void
on_view_load_error(void *data __UNUSED__, Evas_Object *view __UNUSED__,
                   void *event_info)
{
   const Ewk_Frame_Load_Error *error = event_info;
   Evas_Object *frame = error->frame;
   char *msg;
   int len;
   const char template[] = ""
                           "<html>\n"
                           "  <head>\n"
                           "    <title>Error loading page.</title>\n"
                           "  </head>\n"
                           "  <body>\n"
                           "    <h1>Error loading page</h1>\n"
                           "    <p>Error description: <strong>%s</strong></p>\n"
                           "    <p>Failing address: <strong>%s</strong></p>\n"
                           "    <p>Go <a href=\"javascript:history.go(-1);\">back</a></p>"
                           "  </body>\n" "</html>\n";

   if (error->is_cancellation)
      return;

   if (!frame)
     {
        ERR("error loading '%s': %s", error->failing_url, error->description);
        return;
     }

   len = asprintf(&msg, template, error->description, error->failing_url);
   if (len < 0)
     {
        ERR("error loading '%s': %s", error->failing_url, error->description);
        return;
     }

   ewk_frame_contents_alternate_set
      (frame, msg, len, "text/html", NULL, NULL, error->failing_url);
   free(msg);
}

static void
on_view_download_request(void *data __UNUSED__, Evas_Object *view __UNUSED__,
                         void *event_info)
{
   Ewk_Download *download = event_info;

   INF("Download: %s", download->url);
}

static void
on_inputmethod_changed(void *data, Evas_Object *view, void *event_info)
{
   Eina_Bool active = (Eina_Bool)(long)event_info;
   Evas_Object *win = data;
   unsigned int imh;
   INF("IM changed: active=%d", active);

   if (!active)
     {
        elm_win_keyboard_mode_set(win, ELM_WIN_KEYBOARD_OFF);
        return;
     }

   imh = ewk_view_imh_get(view);
   INF("Imh:%d", imh);
   if (imh & EWK_IMH_TELEPHONE)
      elm_win_keyboard_mode_set(win, ELM_WIN_KEYBOARD_PHONE_NUMBER);
   else if (imh & EWK_IMH_NUMBER)
      elm_win_keyboard_mode_set(win, ELM_WIN_KEYBOARD_NUMERIC);
   else if (imh & EWK_IMH_URL)
      elm_win_keyboard_mode_set(win, ELM_WIN_KEYBOARD_URL);
   else if (imh & EWK_IMH_PASSWORD)
      elm_win_keyboard_mode_set(win, ELM_WIN_KEYBOARD_PASSWORD);

#if 0
   else if (imh & EWK_IMH_EMAIL)
      elm_win_keyboard_mode_set(win, ELM_WIN_KEYBOARD_
#endif
   else
      elm_win_keyboard_mode_set(win, ELM_WIN_KEYBOARD_ON);
}

static void
on_key_down(void *data, Evas *e __UNUSED__, Evas_Object *o __UNUSED__,
            void *event_info)
{
   Browser_Window *win = data;
   Evas_Object *view = win->current_view;
   Evas_Object *frame = ewk_view_frame_main_get(view);
   Evas_Event_Key_Down *ev = event_info;
   Eina_Bool control, shift, alt;
   const char *k = ev->keyname;

   control = evas_key_modifier_is_set(ev->modifiers, "Control");
   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");
   alt = evas_key_modifier_is_set(ev->modifiers, "Alt");

   INF("keyname=%s, key=%s, string=%s\n", ev->keyname, ev->key, ev->string);
   if (strcmp(k, "Keycode-122") == 0)
     {
        ev->event_flags &= EVAS_EVENT_FLAG_ON_HOLD;
        ewk_frame_scroll_add(frame, 0, 50);
     }
   else if (strcmp(k, "Keycode-123") == 0)
     {
        ev->event_flags &= EVAS_EVENT_FLAG_ON_HOLD;
        ewk_frame_scroll_add(frame, 0, -50);
     }
   else if ((strcmp(k, "Keycode-185") == 0) ||
            (control && ((strcmp(k, "plus") == 0) || strcmp(k, "equal") == 0)))
     {
        ev->event_flags &= EVAS_EVENT_FLAG_ON_HOLD;
        view_zoom_next_up(view);
     }
   else if ((strcmp(k, "Keycode-186") == 0) ||
            (control && (strcmp(k, "minus") == 0)))
     {
        ev->event_flags &= EVAS_EVENT_FLAG_ON_HOLD;
        view_zoom_next_down(view);
     }
   else if ((strcmp(k, "Keycode-186") == 0) ||
            (control && (strcmp(k, "minus") == 0)))
     {
        ev->event_flags &= EVAS_EVENT_FLAG_ON_HOLD;
        view_zoom_next_down(view);
     }
   else if (control && (strcmp(k, "0") == 0))
     view_zoom_reset(view);
   else if (control && (strcmp(k, "d") == 0))
     {
        const char *url = ewk_view_uri_get(view);
        if (url)
          {
             Evas_Object *ed = elm_layout_edje_get(win->current_chrome);
             Fav_Item *item = fav_items_get(fav, url);
             if (item)
               {
                  unsigned int count = fav_item_visit_count_get(item);
                  fav_item_visit_count_set(item, count + 1);
               }
             else
               {
                  const Ewk_Text_With_Direction *title = ewk_view_title_get(view);
                  fav_items_add(fav, url, fav_item_new(url, title->string, 1));
               }
             edje_object_signal_emit(ed, "favorite,hilight", "");
          }
     }
   else if (control && (strcmp(k, "l") == 0))
     {
        Evas_Object *ed = elm_layout_edje_get(win->current_chrome);
        edje_object_signal_emit(ed, "set,view,mask,visible", "");
     }
   else if (control && (strcmp(k, "r") == 0))
     {
        if (shift) ewk_view_reload_full(view);
        else ewk_view_reload(view);
     }
   else if (strcmp(k, "F11") == 0)
     {
        Eina_Bool setting = !elm_win_fullscreen_get(win->win);
        elm_win_fullscreen_set(win->win, setting);
        if (setting)
          {
             Evas_Object *notify, *label;

             // TODO: hint user with an elm_notify or theme signal
             // theme signal is better, but we need a global elm_layout

             label = elm_label_add(win->win);
             elm_object_style_set(label, "ewebkit");
             elm_object_text_set(label, "F11 to exit fullscreen");
             evas_object_show(label);

             notify = elm_notify_add(win->win);
             elm_object_style_set(notify, "ewebkit");
             elm_object_content_set(notify, label);
             elm_notify_orient_set(notify, ELM_NOTIFY_ORIENT_TOP);
             elm_notify_timeout_set(notify, 1.0);
             evas_object_show(notify);
          }
     }
   else if (alt && (strcmp(k, "Home") == 0))
     ewk_view_uri_set(view, config_home_page_get(config));
}

static char *
tab_grid_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   if (data)
     {
        const Ewk_Text_With_Direction *title = ewk_view_title_get(data);

        if (!title)
            return NULL;

        return strdup(title->string ? title->string : "");
     }

   return NULL;
}

static Evas_Object *
tab_grid_content_get(void *data, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   if (data)
     {
        Eina_Rectangle rect = { 0, 0, 1024, 768 };
        return ewk_view_screenshot_contents_get((const Evas_Object *)data, &rect, 0.15f);
     }

   return NULL;
}

static Eina_Bool
tab_grid_state_get(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   return EINA_FALSE;
}

static void
tab_grid_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__)
{
}

static char *
more_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   More_Menu_Item *mmi = data;

   if (!mmi)
      return strdup("");

   return strdup(mmi->text);
}

static char *
page_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part)
{
   More_Menu_Item *mmi = data;

   if (!mmi)
      return strdup("");

   if (!strcmp(part, "elm.text"))
      return strdup(mmi->text);

   if (!strcmp(part, "elm.text.sub"))
      return strdup(mmi->next);

   return NULL;
}

static const char *
_get_selected_string_value_title(More_Menu_Config_List *list, char *value){
   for (; list->title; list++) {
      if (!strcmp(value, list->value))
         return list->title;
   }
   return "";
}

static const char *
_get_selected_int_value_title(More_Menu_Config_List_Int *list, int value)
{
   for (; list->title; list++) {
      if (value == list->value)
         return list->title;
   }
   return "";
}

static char *
list_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part)
{
   More_Menu_Item *mmi = data;

   if (!mmi)
      return strdup("");

   if (!strcmp(part, "elm.text"))
      return strdup(mmi->text);

   if (!strcmp(part, "elm.text.sub"))
     {
        More_Menu_Config *mmc = mmi->next;

        void *(*conf_get)(void *);
        if ((conf_get = mmc->conf_get))
          {
             if (mmc->type == CONFIG_TYPE_LIST_INT)
                return strdup(_get_selected_int_value_title(mmc->data, (int64_t)conf_get(config)));
             else if (mmc->type == CONFIG_TYPE_LIST)
                return strdup(_get_selected_string_value_title(mmc->data, conf_get(config)));
             else if ((mmc->type == CONFIG_TYPE_SERVICES) || (mmc->type == CONFIG_TYPE_UPNP_SERVICES))
                 return mmi->data ? strdup(services_types_get(mmi->data)) : NULL;

             return NULL;
          }
     }

   return NULL;
}

static Evas_Object *
more_content_get(void *data, Evas_Object *obj, const char *part)
{
   if (!data)
      return NULL;

   More_Menu_Item *mmi = data;
   if (!strcmp(part, "elm.swallow.icon"))
     {
        Evas_Object *icon = config_icon_get(obj, mmi);
        if (!icon && mmi->flags & ITEM_FLAG_SELECTED)
          {
             icon = elm_icon_add(obj);
             elm_image_file_set(icon, PACKAGE_DATA_DIR "/default.edj", "list-selected");
          }
        else if (mmi->type == ITEM_TYPE_PAGE)
          {
             Evas_Object *chrome = evas_object_data_get(obj, "chrome");
             Evas *canvas = evas_object_evas_get(chrome);
             icon = ewk_settings_icon_database_icon_object_get(mmi->next, canvas);
          }
        return icon;
     }
   else if (!strcmp(part, "elm.swallow.end"))
     {
        Evas_Object *end = config_widget_get(obj, mmi);
        if (!end && mmi->flags & ITEM_FLAG_ARROW)
          {
             end = elm_icon_add(obj);
             elm_image_file_set(end, PACKAGE_DATA_DIR "/default.edj", "list-arrow");
          }
        return end;
     }

   return NULL;
}

static Eina_Bool
more_state_get(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   return EINA_FALSE;
}

static void
more_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__)
{
}

static void
more_list_expand_request_cb(void *data, Evas_Object *o, void *event_info)
{
   Elm_Object_Item *gl_item = event_info;
   elm_genlist_item_expanded_set(gl_item, EINA_TRUE);
}

static void
more_list_contract_request_cb(void *data, Evas_Object *o, void *event_info)
{
   Elm_Object_Item *gl_item = event_info;
   elm_genlist_item_expanded_set(gl_item, EINA_FALSE);
}

static void
more_service_item_del(void *data, Evas_Object *obj, void *event_info)
{
   free(data);
}

static void
more_list_expanded_cb(void *data, Evas_Object *o, void *event_info)
{
   Eina_List *l;
   Elm_Object_Item *gl_item = event_info;
   More_Menu_Item *mmi = elm_object_item_data_get(gl_item);
   Evas_Object *list = elm_object_item_widget_get(gl_item);
   Elm_Object_Item *nitem;
   Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_NONE;
   More_Menu_Item *bm_item;

   const More_Menu_Config *mmc = mmi->next;
   const Elm_Genlist_Item_Class *ic;
   

   if (mmc == more_config_network_origin) {
      Network_Origin *network_origin = mmi->data;
      Eina_Iterator *service_itr = eina_hash_iterator_key_new(network_origin_services_hash_get(network_origin));
      const char *service_id;
      Eina_List *ll, *devices = network_origin_devices_list_get(network_origin);
      Device *device;

      EINA_ITERATOR_FOREACH(service_itr, service_id)
      {
         Services *services = network_origin_services_get(network_origin, service_id);

         bm_item = calloc(1, sizeof(More_Menu_Item));
         bm_item->type = ITEM_TYPE_CONFIG;
         bm_item->flags = ITEM_FLAG_DYNAMIC;
         bm_item->text = eina_stringshare_add(services_model_get(services));
         bm_item->next = more_config_services;
         bm_item->data = services;

         nitem = elm_genlist_item_sorted_insert(list, &glic_config_service, bm_item, gl_item,
                                                ELM_GENLIST_ITEM_NONE, &more_item_compare, NULL, NULL);

         elm_object_item_del_cb_set(nitem, more_service_item_del);
      }

      EINA_LIST_FOREACH(devices, ll, device)
      {
         bm_item = calloc(1, sizeof(More_Menu_Item));
         bm_item->type = ITEM_TYPE_CONFIG;
         bm_item->flags = ITEM_FLAG_DYNAMIC;
         bm_item->text = eina_stringshare_add(device_friendly_name_get(device));
         bm_item->next = more_config_device;
         bm_item->data = device;
        
         nitem = elm_genlist_item_sorted_insert(list, &glic_config_origin, bm_item, gl_item,
                                                ELM_GENLIST_ITEM_TREE, &more_item_compare, NULL, NULL);
         elm_genlist_item_expanded_set(nitem, EINA_FALSE);
         elm_object_item_del_cb_set(nitem, more_service_item_del);
      }

   } else if (mmc == more_config_device) {
      Device *device = mmi->data;
      Eina_Iterator *service_itr = eina_hash_iterator_key_new(device_services_hash_get(device));
      const char *service_id;

      EINA_ITERATOR_FOREACH(service_itr, service_id) 
      {
         Services *services = device_services_get(device, service_id);

         bm_item = calloc(1, sizeof(More_Menu_Item));
         bm_item->type = ITEM_TYPE_CONFIG;
         bm_item->flags = ITEM_FLAG_DYNAMIC;
         bm_item->text = eina_stringshare_add(services_model_get(services));
         bm_item->next = more_config_upnp_services;
         bm_item->data = services;

         nitem = elm_genlist_item_sorted_insert(list, &glic_config_service, bm_item, gl_item,
                                                ELM_GENLIST_ITEM_NONE, &more_item_compare, NULL, NULL);
         elm_object_item_del_cb_set(nitem, more_service_item_del);
      }
   }
}

static void
more_list_contracted_cb(void *data, Evas_Object *o, void *event_info)
{
   Elm_Object_Item *gl_item = event_info;
   elm_genlist_item_subitems_clear(gl_item);
}

static char *
service_list_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part)
{
   Service_Item *si = data;

   if (!si)
      return strdup("");

   if (!strcmp(part, "elm.text"))
      return service_item_text_get(si);

   if (!strcmp(part, "elm.text.sub"))
      return service_item_subtext_get(si);

   return NULL;
}

static Evas_Object *
service_content_get(void *data, Evas_Object *obj, const char *part)
{
   Service_Item *si = data;
   
   if (!si)
      return NULL;

   if (!strcmp(part, "elm.swallow.icon"))
     {
        Evas_Object *icon;
        const void *data;
        size_t size;

        service_item_icon_data_get(si, &data, &size);

        if (data && size > 0) {
             icon = elm_icon_add(obj);
             elm_image_memfile_set(icon, data, size, "png", NULL);
             return icon;
        }
        return NULL;
     }
   else if ((service_item_have_service(si) == EINA_TRUE) &&
            !strcmp(part, "elm.swallow.end"))
     {
        Evas_Object *end = elm_check_add(obj);

        elm_object_style_set(end, "ewebkit");
        elm_object_part_text_set(end, "on", "ON");
        elm_object_part_text_set(end, "off", "OFF");
        elm_check_state_set(end, service_item_allowed_get(si));
        evas_object_smart_callback_add(end, "changed", on_service_item_change, si);

        return end;
     }

   return NULL;
}

static Eina_Bool 
service_state_get(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   return EINA_FALSE;
}

static void
service_list_expand_request_cb(void *data, Evas_Object *o, void *event_info)
{
   Elm_Object_Item *gl_item = event_info;
   elm_genlist_item_expanded_set(gl_item, EINA_TRUE);
}

static void
service_list_contract_request_cb(void *data, Evas_Object *o, void *event_info)
{
   Elm_Object_Item *gl_item = event_info;
   elm_genlist_item_expanded_set(gl_item, EINA_FALSE);
}

static void
service_list_expanded_cb(void *data, Evas_Object *o, void *event_info)
{
   Eina_List *l;
   Elm_Object_Item *gl_item = event_info;
   Service_Item *si, *d = elm_object_item_data_get(gl_item);
   Evas_Object *list = elm_object_item_widget_get(gl_item);

   const Elm_Genlist_Item_Class *ic;

   EINA_LIST_FOREACH(service_item_children_get(d), l, si)
     {
        Elm_Object_Item *nitem;
        Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_NONE;
        
        if (service_item_children_get(si))
          {
             ic = &glic_service_origin;
             type = ELM_GENLIST_ITEM_TREE;
          }
        else
          ic = &glic_service_list;

        nitem = elm_genlist_item_sorted_insert(list, ic, si, gl_item, type, 
                                               &service_item_compare, NULL, NULL);
        elm_genlist_item_expanded_set(nitem, EINA_FALSE);
     }
}

static void
service_list_contracted_cb(void *data, Evas_Object *o, void *event_info)
{
   Elm_Object_Item *gl_item = event_info;
   elm_genlist_item_subitems_clear(gl_item);
}


Evas_Object *
chrome_add(Browser_Window *win, const char *url, Session_Item *session_item)
{
   Evas_Object *chrome = elm_layout_add(win->win);
   Evas_Object *ed = elm_layout_edje_get(chrome);
   Evas_Object *view;

   if (!elm_layout_file_set(chrome, PACKAGE_DATA_DIR "/default.edj", "chrome"))
     {
        int err = edje_object_load_error_get(ed);
        const char *msg = edje_load_error_str(err);

        CRITICAL("Could not load chrome theme: %s", msg);
        evas_object_del(chrome);
        return NULL;
     }

   view = view_add(win->win, config_backing_store_get(config));
   if (!view)
     {
        CRITICAL("Could not create view");
        goto error_view_create;
     }

   if (!session_item)
     {
        if (!(session_item = session_item_new(url, EINA_FALSE, 0, 0)))
          {
             CRITICAL("Could not create session object");
             goto error_session_create;
          }
        session_window_tabs_add(win->session_window, session_item);
     }
   evas_object_data_set(chrome, "session", session_item);

   evas_object_focus_set(view, 1);
   elm_object_part_content_set(chrome, "view", view);

   evas_object_data_set(chrome, "view", view);
   evas_object_data_set(chrome, "win", win);
   evas_object_data_set(view, "chrome", chrome);
   evas_object_data_set(ed, "win", win);

   evas_object_event_callback_add(view, EVAS_CALLBACK_KEY_DOWN, on_key_down,
                                  win);
   evas_object_smart_callback_add(view, "load,error", on_view_load_error, win);
   evas_object_smart_callback_add(view, "download,request",
                                  on_view_download_request, win);
   evas_object_smart_callback_add(view, "inputmethod,changed",
                                  on_inputmethod_changed, win->win);

   if (url)
      ewk_view_uri_set(view, url);

   Evas_Object *text_url = elm_entry_add(ed);
   elm_object_style_set(text_url, "ewebkit/url");
   elm_entry_single_line_set(text_url, EINA_TRUE);
   elm_object_part_content_set(chrome, "url-entry", text_url);

   evas_object_smart_callback_add
      (text_url, "activated", on_action_load_page, view);

   Evas_Object *ic = elm_icon_add(ed);
   elm_image_file_set(ic, PACKAGE_DATA_DIR "/default.edj", "clear-button");
   elm_object_part_content_set(text_url, "end", ic);
   evas_object_smart_callback_add(ic, "clicked", on_action_clear, chrome);

   Evas_Object *more_list = elm_genlist_add(ed);
   evas_object_data_set(more_list, "chrome", chrome);
   evas_object_data_set(chrome, "more-list", more_list);
   elm_object_part_content_set(chrome, "more-list-swallow", more_list);
   elm_object_style_set(more_list, "ewebkit");
   elm_scroller_bounce_set(more_list, EINA_FALSE, EINA_FALSE);
   evas_object_smart_callback_add(more_list, "expand,request", 
                                  more_list_expand_request_cb, more_list);
   evas_object_smart_callback_add(more_list, "contract,request", 
                                  more_list_contract_request_cb, more_list);
   evas_object_smart_callback_add(more_list, "expanded",
                                  more_list_expanded_cb, more_list);
   evas_object_smart_callback_add(more_list, "contracted", 
                                  more_list_contracted_cb, more_list);

   Evas_Object *more_index = elm_index_add(ed);
   evas_object_data_set(more_list, "more-index", more_index);
   elm_object_part_content_set(chrome, "more-list-index", more_index);
   evas_object_smart_callback_add(more_index, "selected", index_selected, NULL);
   elm_object_style_set(more_index, "ewebkit");

   Evas_Object *service_list = elm_genlist_add(ed);
   evas_object_smart_callback_add(service_list, "expand,request", 
                                  service_list_expand_request_cb, service_list);
   evas_object_smart_callback_add(service_list, "contract,request", 
                                  service_list_contract_request_cb, service_list);
   evas_object_smart_callback_add(service_list, "expanded",
                                  service_list_expanded_cb, service_list);
   evas_object_smart_callback_add(service_list, "contracted", 
                                  service_list_contracted_cb, service_list);
   evas_object_data_set(service_list, "chrome", chrome);
   evas_object_data_set(chrome, "service-list", service_list);
   elm_object_part_content_set(chrome, "service-list-swallow", service_list);
   elm_object_style_set(service_list, "ewebkit");
   elm_scroller_bounce_set(service_list, EINA_FALSE, EINA_FALSE);

   Evas_Object *tab_grid = elm_gengrid_add(ed);
   elm_object_style_set(tab_grid, "ewebkit");
   elm_gengrid_item_size_set(tab_grid, 140, 174);
   elm_gengrid_horizontal_set(tab_grid, EINA_FALSE);
   elm_gengrid_multi_select_set(tab_grid, EINA_FALSE);
   evas_object_data_set(chrome, "tab-grid", tab_grid);
   evas_object_data_set(tab_grid, "win", win);
   elm_object_part_content_set(chrome, "tab-grid-swallow", tab_grid);
   evas_object_smart_callback_add(tab_grid, "realized",
                                  on_tab_gengrid_item_realized, win);

   edje_object_signal_callback_add(ed, "action,back", "back", on_action_back, view);
   edje_object_signal_callback_add(ed, "action,forward", "forward", on_action_forward,
                                   view);
   edje_object_signal_callback_add(ed, "action,stop", "stop", on_action_pause,
                                   view);
   edje_object_signal_callback_add(ed, "action,reload", "reload", on_action_reload,
                                   view);
   edje_object_signal_callback_add(ed, "action,home", "home", on_action_home,
                                   view);
   edje_object_signal_callback_add(ed, "action,addtab", "addtab", on_action_addtab,
                                   chrome);

   edje_object_signal_callback_add(ed, "action,fav_on", "", on_fav_on, view);
   edje_object_signal_callback_add(ed, "action,fav_off", "", on_fav_off, view);

   edje_object_signal_callback_add(ed, "action,nsd", "nsd", on_action_nsd, chrome);
   edje_object_signal_callback_add(ed, "action,valid", "valid", on_action_valid, chrome);
   
   edje_object_signal_callback_add(ed, "view,mask,hidden", "", on_view_mask_hidden, win);

   edje_object_signal_callback_add(ed, "more,show", "",
                                   on_action_more_show, chrome);
   edje_object_signal_callback_add(ed, "more,hide", "",
                                   on_action_more_hide, chrome);
   edje_object_signal_callback_add(ed, "tab,show", "",
                                   on_action_tab_show, chrome);
   evas_object_smart_callback_add(view, "load,progress", on_view_load_progress,
                                  chrome);
   evas_object_smart_callback_add(view, "title,changed", on_view_title_changed,
                                  chrome);
   evas_object_smart_callback_add(view, "uri,changed", on_view_uri_changed,
                                  chrome);
   evas_object_smart_callback_add(view, "zoom,interactive",
                                  on_view_zoom_interactive, chrome);
   evas_object_smart_callback_add(view, "zoom,interactive,start",
                                  on_view_zoom_interactive_start, chrome);
   evas_object_smart_callback_add(view, "zoom,interactive,end",
                                  on_view_zoom_interactive_end, chrome);
   evas_object_smart_callback_add(view, "statusbar,text,set",
                                  on_view_statusbar_text_set, chrome);
   evas_object_smart_callback_add(view, "link,hover,in", on_view_link_hover_in,
                                  chrome);
   evas_object_smart_callback_add(view, "link,hover,out",
                                  on_view_link_hover_out, chrome);
   evas_object_smart_callback_add(view, "popup,create", on_view_popup_new,
                                  win);
   evas_object_smart_callback_add(view, "networkservices,request,started", 
                                  on_view_networkservices_request_started, chrome);
   evas_object_smart_callback_add(view, "networkservices,request,updated", 
                                  on_view_networkservices_request_updated, chrome);
   evas_object_smart_callback_add(view, "networkservices,request,canceled", 
                                  on_view_networkservices_request_canceled, chrome);
   evas_object_smart_callback_add(view, "networkservices,request,finished",
                                  on_view_networkservices_request_finished, chrome);

   edje_object_part_text_set(ed, "service-list-title", "Services");

   edje_object_signal_emit(ed, "panels,reset,hide", "");
   _chrome_state_apply(chrome, view);

   chrome_config_apply(chrome);
   if (url)
      ewk_view_uri_set(view, url);

   elm_naviframe_item_simple_push(win->pager, chrome);
   return chrome;

error_session_create:
   evas_object_del(view);

error_view_create:
   evas_object_del(chrome);
   return NULL;
}

/* notify this chrome is the current focused in its window */
void
chrome_focused_notify(Evas_Object *chrome)
{
   Browser_Window *win = evas_object_data_get(chrome, "win");
   const char *title;
   char buf[4096];

   title = _chrome_title_get(chrome, buf, sizeof(buf));
   window_title_set(win, title);
}
