/**
 * Developed By ffutop.
 *
 *   Client                                Nginx                        Server
 *
 *             Cookie: stg_token                        token
 *  env:stg <---------------------------- NGX Module <------------- backend server(stg)
 *
 *             Cookie: token                            token
 *  env:pro <----------------------------            <------------- backend server(pro)
 *
 * action 1: set cookie
 * -----------------------------------------------------------------------------
 * action 2: request with cookie
 *
 *             Cookie: stg_token                        token
 *  env:stg ----------------------------> NGX Module --------------> backend server(stg)
 *             Cookie: token                        (remove by NGX Module)
 *
 *             Cookie: stg_token                   stg_token(ignored by server)
 *  env:pro ---------------------------->            --------------> backend server(pro)
 *             Cookie: token                            token
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_str_t env;
    ngx_str_t outer_cookie;
    ngx_str_t inner_cookie;
} ngx_http_env_srv_conf_t;

static void * ngx_http_env_create_srv_conf(ngx_conf_t *cf);
static char * ngx_http_env_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static char * ngx_http_env_rewrite_cookie_parse(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_env_rewrite_handler(ngx_http_request_t *r, ngx_str_t *cookie);
static ngx_int_t ngx_http_env_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_env_post_conf(ngx_conf_t *cf);

static ngx_command_t ngx_http_env_commands[] = {
    { ngx_string("env_of_deployment"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_env_srv_conf_t, env),
      NULL},
    { ngx_string("env_rewrite_cookie_in"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE2,
      ngx_http_env_rewrite_cookie_parse,
      NGX_HTTP_SRV_CONF_OFFSET,
      0,
      NULL},
    ngx_null_command
};

static ngx_http_module_t ngx_http_env_module_ctx = {
    NULL,                           /* preconfiguration (4) */
    ngx_http_env_post_conf,         /* postconfiguration (9) */

    NULL,                           /* create main configuration (1) */
    NULL,                           /* init main configuration (6) */

    ngx_http_env_create_srv_conf,   /* create server configuration (2) */
    ngx_http_env_merge_srv_conf,    /* merge server configuration (7) */

    NULL,                           /* create location configuration (3) */
    NULL                            /* merge location configuration (8) */

        /* parse conf via ngx_command_t[] (5) */
};

ngx_module_t ngx_http_env_module = {
    NGX_MODULE_V1,
    &ngx_http_env_module_ctx,
    ngx_http_env_commands,
    NGX_HTTP_MODULE,                /* module type */
    NULL,                           /* init master */
    NULL,                           /* init module */
    NULL,                           /* init process */
    NULL,                           /* init thread */
    NULL,                           /* exit thread */
    NULL,                           /* exit process */
    NULL,                           /* exit master */
    NGX_MODULE_V1_PADDING
};

static void * ngx_http_env_create_srv_conf(ngx_conf_t *cf) {
    ngx_http_env_srv_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_env_srv_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "init srv conf, conf: %p", (void *) conf);
    return conf;
}

static char * ngx_http_env_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_env_srv_conf_t *prev = parent;
    ngx_http_env_srv_conf_t *conf = child;
    ngx_conf_merge_str_value(conf->outer_cookie, prev->outer_cookie, "");
    ngx_conf_merge_str_value(conf->inner_cookie, prev->inner_cookie, "");
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "merge srv conf. %V:%V. parent: %p, child: %p", &prev->outer_cookie, &prev->inner_cookie, parent, child);
    return NGX_CONF_OK;
}

static char * ngx_http_env_rewrite_cookie_parse(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_env_srv_conf_t *escf;
    ngx_str_t *value;

    escf = conf;
    value = cf->args->elts;
    escf->outer_cookie = value[1];
    escf->inner_cookie = value[2];
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "parse srv conf. %V:%V. conf: %p", &escf->outer_cookie, &escf->inner_cookie, (void *) escf);
    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_env_remove_cookie(ngx_http_request_t *r, ngx_str_t *cookie, ngx_str_t *pattern) {
    u_char *head, *tail;
    ngx_uint_t len, h_cursor, t_cursor;

    len = cookie->len;
    for (h_cursor=0, head=cookie->data;h_cursor<len;h_cursor++, head++) {
        /* escape blankspace */
        for (;h_cursor<len && *head == ' ';h_cursor++, head++);
        /* XXXX(1)XXXX pattern_cookie(2) XXXX(3)XXXX */
        if (ngx_strncmp(head, pattern->data, pattern->len) == 0) {
            t_cursor = h_cursor;
            tail = head;
            /* ensure length of this special Cookie k/v 'xxx=sss;' */
            for (;t_cursor<len && *tail != ';';t_cursor++, tail++);
            /* remove (2) part.  */
            if (t_cursor != len) {
                tail ++;
                t_cursor++;
                ngx_memcpy(head, tail, len - t_cursor);
                cookie->len = cookie->len - (t_cursor-h_cursor);
                return NGX_DECLINED;
            }
        }
        /* escape this cookie until ';' */
        for (;h_cursor<cookie->len && *head != ';';h_cursor++, head++);
    }
    return NGX_DECLINED;
}

static ngx_int_t ngx_http_env_rewrite_cookie(ngx_http_request_t *r, ngx_str_t *cookie, ngx_str_t *from_cookie, ngx_str_t *to_cookie) {
    u_char *head, *new_cookie_data;
    ngx_uint_t len, cursor;
    
    len = cookie->len;
    for (cursor=0, head=cookie->data;cursor<len;cursor++, head++) {
        /* escape blankspace */
        for (;cursor<len && *head == ' ';cursor++, head++);
        /* XXXX(1)XXXX from_cookie(2) XXXX(3)XXXX */
        if (ngx_strncmp(head, from_cookie->data, from_cookie->len) == 0) {
            new_cookie_data = ngx_pcalloc(r->pool, len + to_cookie->len - from_cookie->len);
            /* copy (1) part */
            ngx_memcpy(new_cookie_data, cookie->data, cursor);
            /* change (2) part */
            ngx_memcpy(new_cookie_data+cursor, to_cookie->data, to_cookie->len);
            /* copy (3) part */
            ngx_memcpy(new_cookie_data+cursor+to_cookie->len, cookie->data+cursor+from_cookie->len, len-cursor-from_cookie->len);
            cookie->data = new_cookie_data;
            cookie->len = len + to_cookie->len - from_cookie->len;
            return NGX_DECLINED;
        }
        /* escape this cookie until ';' */
        for(;cursor<len && *head != ';';cursor++, head++);
    }
    return NGX_DECLINED;
}

static ngx_int_t ngx_http_env_rewrite_handler(ngx_http_request_t *r, ngx_str_t *cookie) {
    ngx_http_env_srv_conf_t *escf;
    ngx_str_t *outer_cookie;
    ngx_str_t *inner_cookie;

    escf = ngx_http_get_module_srv_conf(r, ngx_http_env_module);
    outer_cookie = &escf->outer_cookie;
    inner_cookie = &escf->inner_cookie;

    ngx_http_env_remove_cookie(r, cookie, inner_cookie);
    ngx_http_env_rewrite_cookie(r, cookie, outer_cookie, inner_cookie);
    
    return NGX_DECLINED;
}

/* env handler for action 2 */
static ngx_int_t ngx_http_env_handler(ngx_http_request_t *r) {
    ngx_table_elt_t **cookies;
    ngx_table_elt_t *cookie;
    ngx_uint_t nelts;

    cookies = r->headers_in.cookies.elts;
    nelts = r->headers_in.cookies.nelts;
    for (ngx_uint_t i=0;i<nelts;i++) {
        cookie = cookies[i];
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "env handler. before: %V", &cookie->value);
        ngx_http_env_rewrite_handler(r, &cookie->value);
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "env handler. after : %V", &cookie->value);
    }
    return NGX_DECLINED;
}

static ngx_int_t ngx_http_env_post_conf(ngx_conf_t *cf) {
    ngx_http_core_main_conf_t *cmcf;
    ngx_http_env_srv_conf_t *escf;
    ngx_http_handler_pt *h;

    /* check enable handler or not */
    escf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_env_module);
    if (escf->env.len == 0) {
        return NGX_OK;
    }

    /* register handler */
    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if (h == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "failed to register env handler");
        return NGX_ERROR;
    }
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "env post config");
    *h = ngx_http_env_handler;

    return NGX_OK;
}
