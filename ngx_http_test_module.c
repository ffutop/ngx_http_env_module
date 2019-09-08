#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/**
 * TODO: 声明 NGX 适配的环境 stg, prt, pre, pro 
 */
char * ngx_http_env_declaration(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    return NGX_CONF_OK;
}

/**
 * TODO: 声明 NGX 需要改写的 Cookie Names 
 */
char * ngx_http_env_rewrite_cookie(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    return NGX_CONF_OK;
}

/**
 * TODO: NGX ENV MODULE handler
 */
ngx_int_t ngx_http_env_handler(ngx_http_request_t *r) {

    ngx_log_t *log;

    log = r->connection->log;
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0, "Success: invoke http env module");
    return NGX_OK;
}

static ngx_int_t ngx_http_env_init(ngx_conf_t *cf) {
    ngx_http_handler_pt *h;
    ngx_http_core_main_conf_t *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }
    *h = ngx_http_env_handler;
    return NGX_OK;
}

static ngx_command_t ngx_http_env_commands[] = {
    { ngx_string("env_declaration"),
      NGX_CONF_TAKE1|NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF,
      ngx_http_env_declaration,
      NGX_HTTP_LOC_CONF_OFFSET, /* TODO: update it */
      0,
      NULL },
    { ngx_string("env_rewrite_cookie"),
      NGX_CONF_1MORE|NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF,
      ngx_http_env_rewrite_cookie,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },
    ngx_null_command
};

static ngx_http_module_t ngx_http_env_module_ctx = {
    NULL,                           /* preconfiguration */
    ngx_http_env_init,              /* postconfiguration */ 

    NULL,                           /* create main configuration */
    NULL,                           /* init main configuration */

    NULL,                           /* create server configuration */
    NULL,                           /* merge server configuration */

    NULL,                           /* create location configuration */
    NULL                            /* merge location configuration */
};

ngx_module_t ngx_http_env_module = {
    NGX_MODULE_V1,
    &ngx_http_env_module_ctx,       /* module context */
    ngx_http_env_commands,          /* module directives */
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
