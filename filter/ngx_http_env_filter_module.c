#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_str_t outer_cookie;
    ngx_str_t inner_cookie;
} ngx_http_env_filter_srv_conf_t;

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_str_t str_set_cookie;

static void * ngx_http_env_create_srv_conf(ngx_conf_t *cf);
static char * ngx_http_env_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static char * ngx_http_env_rewrite_cookie_parse(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_env_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_env_filter_post_conf(ngx_conf_t *cf);

static ngx_command_t ngx_http_env_filter_commands[] = {
    { ngx_string("env_rewrite_cookie_out"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE2,
      ngx_http_env_rewrite_cookie_parse,
      NGX_HTTP_SRV_CONF_OFFSET,
      0,
      NULL},
    ngx_null_command
};

static ngx_http_module_t ngx_http_env_filter_module_ctx = {
    NULL,                           /* preconfiguration (4) */
    ngx_http_env_filter_post_conf,  /* postconfiguration (9) */

    NULL,                           /* create main configuration (1) */
    NULL,                           /* init main configuration (6) */

    ngx_http_env_create_srv_conf,   /* create server configuration (2) */
    ngx_http_env_merge_srv_conf,    /* merge server configuration (7) */

    NULL,                           /* create location configuration (3) */
    NULL                            /* merge location configuration (8) */

        /* parse conf via ngx_command_t[] (5) */
};

ngx_module_t ngx_http_env_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_env_filter_module_ctx,
    ngx_http_env_filter_commands,
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
    ngx_http_env_filter_srv_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_env_filter_srv_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "init srv filter conf, conf: %p", (void *) conf);
    return conf;
}

static char * ngx_http_env_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_env_filter_srv_conf_t *prev = parent;
    ngx_http_env_filter_srv_conf_t *conf = child;
    ngx_conf_merge_str_value(conf->outer_cookie, prev->outer_cookie, "");
    ngx_conf_merge_str_value(conf->inner_cookie, prev->inner_cookie, "");
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "merge srv filter conf. %V:%V. parent: %p, child: %p", &prev->outer_cookie, &prev->inner_cookie, parent, child);
    return NGX_CONF_OK;
}

static char * ngx_http_env_rewrite_cookie_parse(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_env_filter_srv_conf_t *escf;
    ngx_str_t *value;

    escf = conf;
    value = cf->args->elts;
    escf->outer_cookie = value[1];
    escf->inner_cookie = value[2];
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "parse srv filter conf. %V:%V. conf: %p", &escf->outer_cookie, &escf->inner_cookie, (void *) escf);
    return NGX_CONF_OK;
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

/* header filter for action 1 */
static ngx_int_t ngx_http_env_header_filter(ngx_http_request_t *r) {
    ngx_http_env_filter_srv_conf_t *escf;
    ngx_list_t *headers;
    ngx_list_part_t *part;
    ngx_uint_t i;
    ngx_table_elt_t *header;

    headers = &r->headers_out.headers;
    part = &headers->part;
    header = part->elts;
    for (i=0;/* void */;i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (ngx_strncmp(&header[i].key, str_set_cookie.data, str_set_cookie.len) == 0) {
            escf = ngx_http_conf_get_module_srv_conf(r, ngx_http_env_filter_module);
            ngx_http_env_rewrite_cookie(r, &header[i].value, &escf->inner_cookie, &escf->outer_cookie);
            ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "env filter. %V: %V", &header[i].key, &header[i].value);
        }
    }
    return ngx_http_next_header_filter(r);
}

static ngx_int_t ngx_http_env_filter_post_conf(ngx_conf_t *cf) {

    ngx_http_env_filter_srv_conf_t *escf;
    escf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_env_filter_module);
    if (escf->inner_cookie.len == 0) {
        return NGX_OK;
    }

    ngx_str_set(&str_set_cookie, "Set-Cookie");
    /* register header filter */
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_env_header_filter;
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "add filter");
    return NGX_OK;
}
