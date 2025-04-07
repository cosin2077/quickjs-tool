#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjs/quickjs.h"
#include "quickjs/cutils.h"
#include <curl/curl.h>

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    char **buffer = (char **)userp;
    *buffer = realloc(*buffer, realsize + 1);
    memcpy(*buffer, contents, realsize);
    (*buffer)[realsize] = 0;
    return realsize;
}

static JSValue js_read_file(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *path = JS_ToCString(ctx, argv[0]);
    FILE *f = fopen(path, "r");
    if (!f) return JS_Throw(ctx, JS_NewString(ctx, "File not found"));
    
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = 0;
    fclose(f);
    
    JSValue result = JS_NewString(ctx, buf);
    free(buf);
    JS_FreeCString(ctx, path);
    return result;
}

static JSValue js_write_file(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *path = JS_ToCString(ctx, argv[0]);
    const char *content = JS_ToCString(ctx, argv[1]);
    FILE *f = fopen(path, "w");
    if (!f) return JS_Throw(ctx, JS_NewString(ctx, "Cannot open file"));
    
    fputs(content, f);
    fclose(f);
    
    JS_FreeCString(ctx, path);
    JS_FreeCString(ctx, content);
    return JS_UNDEFINED;
}

static JSValue js_http_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *url = JS_ToCString(ctx, argv[0]);
    CURL *curl = curl_easy_init();
    if (!curl) return JS_Throw(ctx, JS_NewString(ctx, "CURL init failed"));
    
    char *response = NULL;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return JS_Throw(ctx, JS_NewString(ctx, "HTTP request failed"));
    }
    
    JSValue result = JS_NewString(ctx, response ? response : "");
    free(response);
    curl_easy_cleanup(curl);
    JS_FreeCString(ctx, url);
    return result;
}

static JSValue js_exec_binary(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *path = JS_ToCString(ctx, argv[0]);
    JSValue args_array = argv[1];
    int arg_count;
    JSValue len_prop = JS_GetPropertyStr(ctx, args_array, "length");
    JS_ToInt32(ctx, &arg_count, len_prop);
    JS_FreeValue(ctx, len_prop);
    
    char *cmd = malloc(1024);
    strcpy(cmd, path);
    for (int i = 0; i < arg_count; i++) {
        JSValue arg = JS_GetPropertyUint32(ctx, args_array, i);
        const char *arg_str = JS_ToCString(ctx, arg);
        strcat(cmd, " ");
        strcat(cmd, arg_str);
        JS_FreeCString(ctx, arg_str);
        JS_FreeValue(ctx, arg);
    }
    
    FILE *pipe = popen(cmd, "r");
    if (!pipe) return JS_Throw(ctx, JS_NewString(ctx, "Exec failed"));
    
    char buffer[4096] = {0};
    fread(buffer, 1, sizeof(buffer) - 1, pipe);
    pclose(pipe);
    
    JSValue result = JS_NewString(ctx, buffer);
    free(cmd);
    JS_FreeCString(ctx, path);
    return result;
}

static JSValue js_console_log(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    for (int i = 0; i < argc; i++) {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (i > 0) printf(" ");
        printf("%s", str ? str : "undefined");
        JS_FreeCString(ctx, str);
    }
    printf("\n");
    return JS_UNDEFINED;
}

static void init_console(JSContext *ctx) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue console = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, console, "log", JS_NewCFunction(ctx, js_console_log, "log", 0));
    JS_SetPropertyStr(ctx, global, "console", console);
    JS_FreeValue(ctx, global);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <script.js>\n", argv[0]);
        return 1;
    }

    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);
    
    init_console(ctx);

    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "readFile", JS_NewCFunction(ctx, js_read_file, "readFile", 1));
    JS_SetPropertyStr(ctx, global, "writeFile", JS_NewCFunction(ctx, js_write_file, "writeFile", 2));
    JS_SetPropertyStr(ctx, global, "httpGet", JS_NewCFunction(ctx, js_http_get, "httpGet", 1));
    JS_SetPropertyStr(ctx, global, "execBinary", JS_NewCFunction(ctx, js_exec_binary, "execBinary", 2));
    
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("Cannot open %s\n", argv[1]);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = 0;
    fclose(f);
    
    JSValue result = JS_Eval(ctx, buf, len, argv[1], JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        JSValue exception = JS_GetException(ctx);
        const char *err = JS_ToCString(ctx, exception);
        printf("Error: %s\n", err);
        JS_FreeCString(ctx, err);
        JS_FreeValue(ctx, exception);
    }
    JS_FreeValue(ctx, result); // 释放 eval 的返回值
    
    free(buf);
    JS_FreeValue(ctx, global);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}