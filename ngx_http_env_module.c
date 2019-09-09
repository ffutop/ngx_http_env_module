#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_str_t deployment_env;
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
    ngx_conf_merge_str_value(conf->deployment_env, prev->deployment_env, "");
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

ngx_int_t ngx_http_env_handler(ngx_http_request_t *r) {

    /* TODO: */
    return NGX_OK;
}

static ngx_int_t ngx_http_env_post_conf(ngx_conf_t *cf) {
    ngx_http_core_main_conf_t *cmcf;
    ngx_http_handler_pt *h;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }
    *h = ngx_http_env_handler;
    return NGX_OK;
}

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
