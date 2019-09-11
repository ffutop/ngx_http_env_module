#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;

static ngx_int_t ngx_http_env_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_env_filter_post_conf(ngx_conf_t *cf);

static ngx_command_t ngx_http_env_filter_commands[] = {
    ngx_null_command
};

static ngx_http_module_t ngx_http_env_filter_module_ctx = {
    NULL,                           /* preconfiguration (4) */
    ngx_http_env_filter_post_conf,  /* postconfiguration (9) */

    NULL,                           /* create main configuration (1) */
    NULL,                           /* init main configuration (6) */

    NULL,                           /* create server configuration (2) */
    NULL,                           /* merge server configuration (7) */

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

/* header filter for action 1 */
static ngx_int_t ngx_http_env_header_filter(ngx_http_request_t *r) {
    ngx_array_t *cookies;

    cookies = &r->headers_out.cookies;
    ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "env header filter.");
    /* TODO: */
    for (ngx_uint_t i=0;i<cookies->nelts;i++) {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "env header filter. Cookie: %V", (ngx_str_t *) cookies->elts + i);
    }
    return ngx_http_next_header_filter(r);
}

static ngx_int_t ngx_http_env_filter_post_conf(ngx_conf_t *cf) {
    /* register header filter */
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_env_header_filter;
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "add filter");
    return NGX_OK;
}
