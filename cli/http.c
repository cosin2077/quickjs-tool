#include <curl/curl.h>
#include "quickjs.h"

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    JSContext *ctx = (JSContext *)userp;
    size_t realsize = size * nmemb;
    JSValue *result = (JSValue *)userp;
    *result = JS_NewStringLen(ctx, (const char *)contents, realsize);
    return realsize;
}

static JSValue js_fetch(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1)
    {
        return JS_ThrowTypeError(ctx, "fetch: URL argument required");
    }
    const char *url = JS_ToCString(ctx, argv[0]);
    if (!url)
    {
        return JS_ThrowTypeError(ctx, "fetch: Invalid URL");
    }

    CURL *curl = curl_easy_init();
    JSValue result = JS_UNDEFINED;
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            JS_FreeValue(ctx, result);
            result = JS_Throw(ctx, JS_NewString(ctx, curl_easy_strerror(res)));
        }
        curl_easy_cleanup(curl);
    }
    JS_FreeCString(ctx, url);
    return result;
}

static const JSCFunctionListEntry js_http_funcs[] = {
    JS_CFUNC_DEF("fetch", 1, js_fetch),
};

static int js_http_init(JSContext *ctx, JSModuleDef *m)
{
    JS_SetModuleExportList(ctx, m, js_http_funcs, sizeof(js_http_funcs) / sizeof(js_http_funcs[0]));
    return 0;
}

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *m = JS_NewCModule(ctx, module_name, js_http_init);
    if (m)
    {
        JS_AddModuleExportList(ctx, m, js_http_funcs, sizeof(js_http_funcs) / sizeof(js_http_funcs[0]));
    }
    return m;
}