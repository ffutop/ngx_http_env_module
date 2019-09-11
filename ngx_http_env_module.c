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
 *             Cookie: token                           (ignore)
 *
 *             Cookie: stg_token                   stg_token(ignored by server)
 *  env:pro ---------------------------->            --------------> backend server(pro)
 *             Cookie: token                            token
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;

typedef struct {
    ngx_str_t deployment_env;
    ngx_str_t outer_cookie;
    ngx_str_t inner_cookie;
} ngx_http_env_srv_conf_t;

static void * ngx_http_env_create_srv_conf(ngx_conf_t *cf);
static char * ngx_http_env_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static char * ngx_http_env_rewrite_cookie_parse(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_env_rewrite_handler(ngx_http_request_t *r, ngx_str_t *cookie);
static ngx_int_t ngx_http_env_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_env_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_env_post_conf(ngx_conf_t *cf);

static ngx_command_t ngx_http_env_commands[] = {
    { ngx_string("deployment_env"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_env_srv_conf_t, deployment_env),
      NULL},
    { ngx_string("env_rewrite_cookie"),
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
    ngx_conf_merge_str_value(conf->deployment_env, prev->deployment_env, "");
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

static ngx_int_t ngx_http_env_rewrite_handler(ngx_http_request_t *r, ngx_str_t *cookie) {
    ngx_http_env_srv_conf_t *escf;
    ngx_str_t *outer_cookie;
    ngx_str_t *inner_cookie;
    u_char *new_cookie_data;
    u_char *p;
    ngx_uint_t cursor;

    escf = ngx_http_get_module_srv_conf(r, ngx_http_env_module);
    outer_cookie = &escf->outer_cookie;
    inner_cookie = &escf->inner_cookie;

    for (cursor=0, p=cookie->data;cursor<cookie->len;cursor++, p++) {
        if (*p == ' ') {
            continue;
        }

        /* XXXX(1)XXXX outer_cookie(2) XXXX(3)XXXX */
        if (ngx_strncmp(p, (char *) outer_cookie->data, outer_cookie->len) == 0) {
            new_cookie_data = ngx_pcalloc(r->pool, cookie->len + inner_cookie->len - outer_cookie->len);
            /* copy (1) part */
            ngx_memcpy(new_cookie_data, cookie->data, cursor);
            /* change (2) part */
            ngx_memcpy(new_cookie_data+cursor, inner_cookie->data, inner_cookie->len);
            /* copy (3) part */
            ngx_memcpy(new_cookie_data+cursor+inner_cookie->len, cookie->data+cursor+outer_cookie->len, cookie->len-cursor-outer_cookie->len);
            cookie->data = new_cookie_data;
            cookie->len = cookie->len + inner_cookie->len - outer_cookie->len;
            return NGX_DECLINED;
        }
    }
    return NGX_DECLINED;
}

/* env handler for action 2 */
static ngx_int_t ngx_http_env_handler(ngx_http_request_t *r) {
    ngx_table_elt_t **cookies;
    ngx_table_elt_t *cookie;
    ngx_uint_t nelts;

    /* TODO: */
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

/* header filter for action 1 */
static ngx_int_t ngx_http_env_header_filter(ngx_http_request_t *r) {
    ngx_array_t *cookies;

    cookies = &r->headers_in.cookies;
    ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "env header filter.");
    /* TODO: */
    for (ngx_uint_t i=0;i<cookies->nelts;i++) {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "env header filter. Cookie: %V", (ngx_str_t *) cookies->elts + i);
    }
    return ngx_http_next_header_filter(r);
}

static ngx_int_t ngx_http_env_post_conf(ngx_conf_t *cf) {
    ngx_http_core_main_conf_t *cmcf;
    ngx_http_handler_pt *h;

    /* register handler */
    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    /*h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);*/
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if (h == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "failed to register env handler");
        return NGX_ERROR;
    }
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "env post config");
    *h = ngx_http_env_handler;

    /* register header filter */
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_env_header_filter;

    return NGX_OK;
}
