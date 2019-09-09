#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_str_t *deployment_env;
} ngx_http_env_srv_conf_t;

static void * ngx_http_env_create_srv_conf(ngx_conf_t *cf) {
    ngx_http_env_srv_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_env_srv_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    ngx_conf_log_error(NGX_LOG_INFO, cf, 0, "init srv conf");
    return conf;
}

static char * ngx_http_env_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_env_srv_conf_t *prev = parent;
    ngx_http_env_srv_conf_t *conf = child;
    ngx_conf_merge_str_value(conf->deployment_env, conf->deployment_env, "");
    ngx_conf_log_error(NGX_LOG_INFO, cf, 0, "merge srv conf");
    return NGX_CONF_OK;
}

static ngx_command_t ngx_http_env_commands[] = {
    { ngx_string("deployment_env"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_env_srv_conf_t, deployment_env),
      NULL},
    ngx_null_command
};

static ngx_http_module_t ngx_http_env_module_ctx = {
    NULL,                           /* preconfiguration */
    NULL,                           /* postconfiguration */

    NULL,                           /* create main configuration */
    NULL,                           /* init main configuration */

    ngx_http_env_create_srv_conf,   /* create server configuration */
    ngx_http_env_merge_srv_conf,    /* merge server configuration */

    NULL,                           /* create location configuration */
    NULL                            /* merge location configuration */
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
