#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_str_t deployment_env;
} ngx_http_env_main_conf_t;

static void * ngx_http_env_create_main_conf(ngx_conf_t *cf) {
    ngx_http_env_main_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_env_main_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "init main conf");

    ngx_str_null(&conf->deployment_env);
    return conf;
}

static char * ngx_http_env_init_main_conf(ngx_conf_t *cf, void *conf) {
    ngx_http_env_main_conf_t *emcf = conf;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "init main conf");
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "init main conf. env: %d", emcf->deployment_env.len);

    return NGX_CONF_OK;
}

char * ngx_http_deployment_env(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_env_main_conf_t *emcf = conf;

    ngx_str_t *value;

    value = cf->args->elts;
    if (cf->args->nelts == 2) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "value: %V %V", value, value+1);
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "set deployment_env");
        emcf->deployment_env.len = (value+1)->len;
        emcf->deployment_env.data = (value+1)->data;
        return NGX_CONF_OK;
    }
    return NGX_CONF_ERROR;
}

static ngx_command_t ngx_http_env_commands[] = {
    { ngx_string("deployment_env"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_deployment_env,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_env_main_conf_t, deployment_env),
      NULL},
    ngx_null_command
};

static ngx_http_module_t ngx_http_env_module_ctx = {
    NULL,                           /* preconfiguration */
    NULL,                           /* postconfiguration */

    ngx_http_env_create_main_conf,  /* create main configuration */
    ngx_http_env_init_main_conf,    /* init main configuration */

    NULL,                           /* create server configuration */
    NULL,                           /* merge server configuration */

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
