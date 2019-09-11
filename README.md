# nginx-http-env-module

nginx http env module

## 背景

公司内部系统的单点登录系统通过 Cookie 维持会话，技术人员频繁在生产、预发、测试等环境切换，导致单点登录的 Cookie 频繁被覆盖，失去了单点登录的优势。

鉴于老系统过多，升级代价大。故开发一 Nginx 模块，以支持在客户端同时维持多份不同环境的 Cookie 

## 概念图

```c
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
```


## 静态编译

1. 下载 ngx-http-env-module 模块

2. 下载 nginx 源代码(推荐使用熟悉的版本)

```sh
curl -LO http://nginx.org/download/nginx-1.14.2.tar.gz
```

3. 解压并在 nginx 文件目录执行下列命令

```sh
tar -xvf nginx-1.14.2.tar.gz
cd nginx-1.14.2/
./configure --add-module=/path/to/ngx_http_env_module/handler --add-module=/path/to/ngx_http_env_module/filter
```

4. 编译并安装

```sh
make build install
```

5. 配置 NGINX 并启动

## 使用

> Syntax: `env_rewrite_cookie_in` `OUTER_COOKIE_KEY` `INNER_COOKIE_KEY`
> Default: -
> Context: http, server

针对请求头携带的 Cookie，如果 Cookie 的键匹配 `OUTER_COOKIE_KEY`，则将其改写成 `INNER_COOKIE_KEY`，再执行后续逻辑(例如发送反向代理请求)

> Syntax: `env_rewrite_cookie_out` `OUTER_COOKIE_KEY` `INNER_COOKIE_KEY`
> Default: -
> Context: http, server

针对响应头携带的 Set-Cookie，如果 Set-Cookie 的 Cookie 键匹配 `INNER_COOKIE_KEY`， 则将其改写成 `OUTER_COOKIE_KEY`，再向客户端发送响应头。

## 协作

欢迎 `issues` 和 `pull request` 

## 许可证

BSD © ffutop
